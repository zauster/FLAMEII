/*!
 * \file xparser2/xparser2.cpp
 * \author Shawn Chin, Simon Coakley
 * \date January 2013
 * \copyright Copyright (c) 2013 STFC Rutherford Appleton Laboratory
 * \copyright Copyright (c) 2013 University of Sheffield
 * \copyright GNU Lesser General Public License
 * \brief Xparser
 */
#include <fstream>
#include <map>
#include <boost/lexical_cast.hpp>
#include "flame2/io/io_manager.hpp"
#include "printer.hpp"
#include "codegen/gen_snippets.hpp"
#include "codegen/gen_datastruct.hpp"
#include "codegen/gen_makefile.hpp"
#include "codegen/gen_headerfile.hpp"
#include "codegen/gen_maincpp.hpp"
#include "codegen/gen_model.hpp"
#include "codegen/gen_agent.hpp"
#include "file_generator.hpp"
#include "xparser2.hpp"

namespace gen = xparser::codegen;

void generateConditionFunction(flame::model::XCondition * condition, xparser::Printer * p) {
    std::map<std::string, std::string> variables;

    if (condition->isNot) p->Print("!");
    p->Print("(");
    if (condition->isValues) {
        /* Handle lhs */
        if (condition->lhsIsAgentVariable) {
            variables["lhs"] = condition->lhs;
            p->Print("a.$lhs$", variables);
        } else if (condition->lhsIsMessageVariable) {
            variables["lhs"] = condition->lhs;
            p->Print("m.$lhs$", variables);
        } else if (condition->lhsIsValue) {
            variables["lhsDouble"] = boost::lexical_cast<std::string>(condition->lhsDouble);
            p->Print("$lhsDouble$", variables);
        }
        /* Handle operator */
        variables["op"] = condition->op;
        p->Print(" $op$ ", variables);
        /* Handle rhs */
        if (condition->rhsIsAgentVariable) {
            variables["rhs"] = condition->rhs;
            p->Print("a.$rhs$", variables);
        } else if (condition->rhsIsMessageVariable) {
            variables["rhs"] = condition->rhs;
            p->Print("m.$rhs$", variables);
        } else if (condition->rhsIsValue) {
            variables["rhsDouble"] = boost::lexical_cast<std::string>(condition->rhsDouble);
            p->Print("$rhsDouble$", variables);
        }
    }
    if (condition->isConditions) {
        generateConditionFunction(condition->lhsCondition, p);
        variables["op"] = condition->op;
        p->Print(" $op$ ", variables);
        generateConditionFunction(condition->rhsCondition, p);
    }
    if (condition->isTime) {
        p->Print("iteration_loop");
        // If time period is not 'iteration' then
        p->Print("%$time_period_iterations$");

        p->Print(" == ");

        if (condition->timePhaseIsVariable) {
            variables["timePhaseVariable"] = condition->timePhaseVariable;
            p->Print("a.$timePhaseVariable$", variables);
        } else {
            variables["timePhaseValue"] = boost::lexical_cast<std::string>(condition->timePhaseValue);
            p->Print("$timePhaseValue$", variables);
        }
        // ToDo (SC) Handle time duration?
    }
    p->Print(")");
}

void generateConditionCreation(std::string cname, flame::model::XCondition * condition, xparser::Printer * p) {
    std::map<std::string, std::string> variables;
    variables["cname"] = cname;

    if (condition->isNot) p->Print("$cname$->isNot = true;\n", variables);
    if (condition->isValues) {
        p->Print("$cname$->isValues = true;\n", variables);
        /* Handle lhs */
        if (condition->lhsIsAgentVariable) {
            p->Print("$cname$->lhsIsAgentVariable = true;\n", variables);
            variables["lhs"] = condition->lhs;
            p->Print("$cname$->lhs = \"$lhs$\";\n", variables);
        }
        else if (condition->lhsIsMessageVariable) {
            p->Print("$cname$->lhsIsMessageVariable = true;\n", variables);
            variables["lhs"] = condition->lhs;
            p->Print("$cname$->lhs = \"$lhs$\";\n", variables);
        }
        else if (condition->lhsIsValue) {
            p->Print("$cname$->lhsIsValue = true;\n", variables);
            variables["lhs"] = boost::lexical_cast<std::string>(condition->lhsDouble);
            p->Print("$cname$->lhsDouble = $lhs$;\n", variables);
        }
        /* Handle operator */
        variables["op"] = condition->op;
        p->Print("$cname$->op = \"$op$\";\n", variables);
        /* Handle rhs */
        if (condition->rhsIsAgentVariable) {
            p->Print("$cname$->rhsIsAgentVariable = true;\n", variables);
            variables["rhs"] = condition->lhs;
            p->Print("$cname$->rhs = \"$rhs$\";\n", variables);
        }
        else if (condition->rhsIsMessageVariable) {
            p->Print("$cname$->rhsIsMessageVariable = true;\n", variables);
            variables["rhs"] = condition->lhs;
            p->Print("$cname$->rhs = \"$rhs$\";\n", variables);
        }
        else if (condition->rhsIsValue) {
            p->Print("$cname$->rhsIsValue = true;\n", variables);
            variables["rhs"] = boost::lexical_cast<std::string>(condition->rhsDouble);
            p->Print("$cname$->rhsDouble = $rhs$;\n", variables);
        }
    }
    if (condition->isConditions) {
        p->Print("$cname$->isConditions = true;\n", variables);
        p->Print("$cname$->lhsCondition = new model::XCondition;\n", variables);
        generateConditionCreation(cname + "->lhsCondition", condition->lhsCondition, p);
        variables["op"] = condition->op;
        p->Print("$cname$->op = \"$op$\";\n", variables);
        p->Print("$cname$->rhsCondition = new model::XCondition;\n", variables);
        generateConditionCreation(cname + "->rhsCondition", condition->rhsCondition, p);
    }
    if (condition->isTime) {
        p->Print("$cname$->isTime = true;\n", variables);
        variables["timePeriod"] = condition->timePeriod;
        p->Print("$cname$->timePeriod = \"$timePeriod$\";\n", variables);
        if (condition->timePhaseIsVariable) {
            p->Print("$cname$->timePhaseIsVariable = true;\n", variables);
            variables["timePhaseVariable"] = condition->timePhaseVariable;
            p->Print("$cname$->timePhaseVariable = \"$timePhaseVariable$\";\n", variables);
        } else {
            variables["timePhaseValue"] = boost::lexical_cast<std::string>(condition->timePhaseValue);
            p->Print("$cname$->timePhaseValue = $timePhaseValue$;\n", variables);
        }
        if (condition->foundTimeDuration) {
            p->Print("$cname$->foundTimeDuration = true;\n", variables);
            variables["timeDuration"] = boost::lexical_cast<std::string>(condition->timeDuration);
            p->Print("$cname$->timeDuration = $timeDuration$;\n", variables);
        }
    }
}

void generateConditionFunctionFiles(flame::model::XModel * model) {
    boost::ptr_vector<flame::model::XMachine>::iterator agent;
    boost::ptr_vector<flame::model::XFunction>::iterator func;
    std::map<std::string, std::string> variables;

    // Condition functions cpp
    std::ofstream condition_filter_methodscppfile;
    condition_filter_methodscppfile.open ("flame_generated_condition_filter_methods.cpp");
    // create printer instance
    xparser::Printer p2(condition_filter_methodscppfile);
    p2.Print("#include \"flame2.hpp\"\n");
    p2.Print("#include \"flame_generated_message_datatypes.hpp\"\n");
    // Generate function condition functions
    boost::ptr_vector<flame::model::XMachine> * agents = model->getAgents();
    for (agent = agents->begin(); agent != agents->end(); ++agent) {
        variables["agent_name"] = (*agent).getName();
        boost::ptr_vector<flame::model::XFunction> * funcs = (*agent).getFunctions();
        for (func = funcs->begin(); func != funcs->end(); ++func) {
            // Conditions
            if ((*func).getCondition()) {
                variables["func_name"] = (*func).getName();
                variables["func_current_state"] = (*func).getCurrentState();
                variables["func_next_state"] = (*func).getNextState();
                p2.Print("\nbool $agent_name$_$func_name$_$func_current_state$_$func_next_state$_condition() {\n", variables);
                p2.Indent();
                p2.Print("return ");
                generateConditionFunction((*func).getCondition(), &p2);
                p2.Print(";\n");
                p2.Outdent();
                p2.Print("}\n");
            }
            // Filters
            // ToDo SC
        }
    }
    // close file when done
    condition_filter_methodscppfile.close();

    // Condition function header
    std::ofstream condition_filter_methodshppfile;
    condition_filter_methodshppfile.open ("flame_generated_condition_filter_methods.hpp");
    xparser::Printer p3(condition_filter_methodshppfile);
    p3.Print("#ifndef FLAME_GENERATED_CONDITION_FILTER_METHODS_HPP_\n");
    p3.Print("#define FLAME_GENERATED_CONDITION_FILTER_METHODS_HPP_\n");
    p3.Print("#include \"flame2.hpp\"\n\n");
    // Generate function condition functions
    for (agent = agents->begin(); agent != agents->end(); ++agent) {
        variables["agent_name"] = (*agent).getName();
        boost::ptr_vector<flame::model::XFunction> * funcs = (*agent).getFunctions();
        for (func = funcs->begin(); func != funcs->end(); ++func) {
            // Conditions
            if ((*func).getCondition()) {
                variables["func_name"] = (*func).getName();
                variables["func_current_state"] = (*func).getCurrentState();
                variables["func_next_state"] = (*func).getNextState();
                p3.Print("bool $agent_name$_$func_name$_$func_current_state$_$func_next_state$_condition();\n", variables);
            }
        }
    }
    p3.Print("\n#endif  // FLAME_GENERATED_CONDITION_FILTER_METHODS_HPP_\n");
    condition_filter_methodshppfile.close();
}

int main(int argc, const char* argv[]) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0];
        std::cerr << " MODEL_FILE" << std::endl;
        exit(1);
     }

    std::string model_path = argv[1];

    // Load and validate model
    flame::model::XModel model;
    flame::io::IOManager::GetInstance().loadModel(model_path, &model);
    if (model.validate() != 0) {
        std::cerr << "Invalid model" << std::endl;
        exit(2);
    }
    std::map<std::string, std::string> variables;
    boost::ptr_vector<flame::model::XMachine>::iterator agent;
    boost::ptr_vector<flame::model::XVariable>::iterator variable;
    boost::ptr_vector<flame::model::XFunction>::iterator func;
    boost::ptr_vector<flame::model::XIOput>::iterator ioput;
    boost::ptr_vector<flame::model::XMessage>::iterator message;
    boost::ptr_vector<flame::model::XMachine> * agents;

    // File generator to manage file writing
    xparser::FileGenerator filegen;
    // Makefile generator
    xparser::codegen::GenMakefile makefile;

    // Write out headef file for agent function definitions
    xparser::codegen::GenHeaderFile func_def_hpp;
    agents = model.getAgents();
    for (agent = agents->begin(); agent != agents->end(); ++agent) {
        boost::ptr_vector<flame::model::XFunction> * funcs = agent->getFunctions();
        for (func = funcs->begin(); func != funcs->end(); ++func) {
            func_def_hpp.Insert(gen::GenAgentFunctionHeader(func->getName()));
        }
    }
    filegen.Output("agent_function_definitions.hpp", func_def_hpp);
    makefile.AddHeaderFile("agent_function_definitions.hpp");
    
    // main.cpp generator
    xparser::codegen::GenMainCpp maincpp;
    
    xparser::codegen::GenModel genmodel;

    maincpp.Insert(genmodel);

    // create printer instance
    xparser::Printer p(std::cout);
    /*  -- This should be inserted automatically by gen_*file
    p.Print("#include \"flame2/exe/task_manager.hpp\"\n");
    p.Print("#include \"flame2/exe/scheduler.hpp\"\n");
    p.Print("#include \"flame2/exe/splitting_fifo_task_queue.hpp\"\n");
    p.Print("#include \"flame2/io/io_manager.hpp\"\n");
    p.Print("#include \"flame2/model/model.hpp\"\n");
    p.Print("#include \"flame2/mb/message_board_manager.hpp\"\n");
    p.Print("#include \"flame2/sim/simulation.hpp\"\n\n");
    p.Print("#include \"flame2.hpp\"\n");
    p.Print("#include \"message_datatypes.hpp\"\n\n");
    */

    // Agents
    agents = model.getAgents();
    for (agent = agents->begin(); agent != agents->end(); ++agent) {
        xparser::codegen::GenAgent genagent((*agent).getName());
        // Agent memory
        boost::ptr_vector<flame::model::XVariable> * vars = (*agent).getVariables();
        for (variable = vars->begin(); variable != vars->end(); ++variable)
            genagent.AddVar((*variable).getType(), (*variable).getName());
        maincpp.Insert(genagent);
        // Agent functions
        boost::ptr_vector<flame::model::XFunction> * funcs = (*agent).getFunctions();
        for (func = funcs->begin(); func != funcs->end(); ++func) {
            variables["agent_name"] = (*agent).getName();
            variables["func_name"] = (*func).getName();
            variables["func_current_state"] = (*func).getCurrentState();
            variables["func_next_state"] = (*func).getNextState();
            p.Print("model.addAgentFunction(\"$agent_name$\", \"$func_name$\", \"$func_current_state$\", \"$func_next_state$\");\n", variables);
            // Condition
            /*if ((*func).getCondition()) {
                p.Print("condition = function->addCondition();\n");
                generateConditionCreation("condition", (*func).getCondition(), &p);
            }*/
            // Outputs
            boost::ptr_vector<flame::model::XIOput> * outputs = (*func).getOutputs();
            for (ioput = outputs->begin(); ioput != outputs->end(); ++ioput) {
                variables["message_name"] = (*ioput).getMessageName();
                p.Print("model.addAgentFunctionOutput(\"$agent_name$\", \"$func_name$\", \"$func_current_state$\", \"$func_next_state$\", \"$message_name$\");\n", variables);
            }
            // Inputs
            boost::ptr_vector<flame::model::XIOput> * inputs = (*func).getInputs();
            for (ioput = inputs->begin(); ioput != inputs->end(); ++ioput) {
                variables["message_name"] = (*ioput).getMessageName();
                p.Print("model.addAgentFunctionInput(\"$agent_name$\", \"$func_name$\", \"$func_current_state$\", \"$func_next_state$\", \"$message_name$\");\n", variables);
                /*if ((*ioput).getFilter()) {
                    p.Print("condition = ioput->addFilter();\n");
                    generateConditionCreation("condition", (*ioput).getFilter(), &p);
                }
                if ((*ioput).isRandom()) p.Print("ioput->setRandom(true);\n");
                if ((*ioput).isSort()) {
                    p.Print("ioput->setSort(true);\n");
                    variables["sort_key"] = (*ioput).getSortKey();
                    variables["sort_order"] = (*ioput).getSortOrder();
                    p.Print("ioput->setSortKey(\"$sort_key$\");\n", variables);
                    p.Print("ioput->setSortOrder(\"$sort_order$\");\n", variables);
                }*/
            }
            // Memory Access info
            if ((*func).getMemoryAccessInfoAvailable()) {
                //p.Print("function->setMemoryAccessInfoAvailable(true);\n");
                std::vector<std::string>::iterator var;
                std::vector<std::string> * readOnly = (*func).getReadOnlyVariables();
                for (var = readOnly->begin(); var != readOnly->end(); ++var) {
                    variables["read_only_var"] = (*var);
                    p.Print("model.addAgentFunctionReadOnlyVariable(\"$agent_name$\", \"$func_name$\", \"$func_current_state$\", \"$func_next_state$\", \"$read_only_var$\");\n", variables);
                }
                std::vector<std::string> * readWrite = (*func).getReadWriteVariables();
                for (var = readWrite->begin(); var != readWrite->end(); ++var) {
                    variables["read_write_var"] = (*var);
                    p.Print("model.addAgentFunctionReadWriteVariable(\"$agent_name$\", \"$func_name$\", \"$func_current_state$\", \"$func_next_state$\", \"$read_write_var$\");\n", variables);
                }
            }
        }
    }
    // Messages
    boost::ptr_vector<flame::model::XMessage> * messages = model.getMessages();
    for (message = messages->begin(); message != messages->end(); ++message) {
        variables["message_name"] = (*message).getName();
        p.Print("model.addMessage(\"$message_name$\");\n", variables);
        boost::ptr_vector<flame::model::XVariable> * vars = (*message).getVariables();
        for (variable = vars->begin(); variable != vars->end(); ++variable) {
            variables["var_name"] = (*variable).getName();
            variables["var_type"] = (*variable).getType();
            p.Print("model.addMessageVariable(\"$message_name$\", \"$var_type$\", \"$var_name$\");\n", variables);
        }
    }
    p.Print("\n// Validate model\n");
    p.Print("model.validate();\n");
    // Register agent functions
    p.Print("\n// Register agent functions\n");
    for (agent = agents->begin(); agent != agents->end(); ++agent) {
        boost::ptr_vector<flame::model::XFunction> * funcs = (*agent).getFunctions();
        for (func = funcs->begin(); func != funcs->end(); ++func) {
            variables["func_name"] = (*func).getName();
            p.Print("model.registerAgentFunction(\"$func_name$\", &$func_name$);\n", variables);
        }
    }
    // Register messages
    p.Print("// Register message types\n");
    for (message = messages->begin(); message != messages->end(); ++message) {
        variables["message_name"] = (*message).getName();
        p.Print("model.registerMessageType<$message_name$_message>(\"$message_name$\");\n", variables);
    }


    filegen.Output("main.cpp", maincpp);
    makefile.AddSourceFile("main.cpp");
    
    // Message datatypes header
    xparser::codegen::GenHeaderFile msg_datatype_h;
    for (message = messages->begin(); message != messages->end(); ++message) {
        xparser::codegen::GenDataStruct msg((*message).getName() + "_message");
        boost::ptr_vector<flame::model::XVariable> * vars = (*message).getVariables();
        for (variable = vars->begin(); variable != vars->end(); ++variable)
            msg.AddVar((*variable).getType(), (*variable).getName());
        msg_datatype_h.Insert(msg);
    }
    filegen.Output("message_datatypes.hpp", msg_datatype_h);
    makefile.AddHeaderFile("message_datatypes.hpp");

    // Add user agent function files to Makefile
    std::vector<std::string> * functionFiles = model.getFunctionFiles();
    std::vector<std::string>::iterator functionFile;
    for (functionFile = functionFiles->begin(); functionFile != functionFiles->end();
            ++functionFile) {
        makefile.AddSourceFile((*functionFile));
    }

    // Output Makefile now that all hpp and cpp files have been added
    filegen.Output("Makefile", makefile);

    /*
    1. main.cpp
      - register agents
      - register agent functions
        - state change
        - mem read/write
        - message send/recv
      - register messages
      - registed datatypes (?)
    2. model_datatypes.cpp
      - generate message datatypes
      - generate user-defined datatypes
    3. data.xsd
      - schema to validate input data
    4. Makefile
    */

    return 0;
}
