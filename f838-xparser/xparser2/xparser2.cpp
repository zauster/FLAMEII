/*!
 * \file xparser2/xparser2.cpp
 * \author Shawn Chin, Simon Coakley
 * \date January 2013
 * \copyright Copyright (c) 2013 STFC Rutherford Appleton Laboratory
 * \copyright Copyright (c) 2013 University of Sheffield
 * \copyright GNU Lesser General Public License
 * \brief Xparser
 *
 * Files generated:
 *  1. main.cpp
 *    - register agents
 *    - register agent functions
 *      - state change
 *      - mem read/write
 *      - message send/recv
 *    - register messages
 *    - registed datatypes
 * 2. message_datatypes.hpp
 * 3. Makefile
 * 4. data.xsd (TODO)
 *    - schema to validate input data
 */
#include <fstream>
#include <boost/lexical_cast.hpp>
#include "flame2/io/io_manager.hpp"
#include "codegen/gen_snippets.hpp"
#include "codegen/gen_datastruct.hpp"
#include "codegen/gen_makefile.hpp"
#include "codegen/gen_headerfile.hpp"
#include "codegen/gen_maincpp.hpp"
#include "codegen/gen_model.hpp"
#include "codegen/gen_agent.hpp"
#include "codegen/gen_agentfunc.hpp"
#include "codegen/gen_message_registration.hpp"
#include "file_generator.hpp"

namespace gen = xparser::codegen;  // namespace shorthand

void generate_agents(flame::model::XModel *model,
                    gen::GenMainCpp *maincpp);

void generate_agent_func_definition(flame::model::XModel *model,
                                    gen::GenMainCpp *maincpp,
                                    gen::GenHeaderFile *func_def_hpp);
                                    
void generate_messages(flame::model::XModel *model,
                       gen::GenMainCpp *maincpp,
                       gen::GenHeaderFile *msg_datatype_h);
                       
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

    // File generator to manage file writing
    xparser::FileGenerator filegen;
    gen::GenMakefile makefile; // Makefile generator
    gen::GenMainCpp maincpp; // main.cpp generator

    // initialise model and environment
    gen::GenModel genmodel;
    maincpp.Insert(genmodel);

    // output to main.cpp code to define agents
    generate_agents(&model, &maincpp);

    // Write out header file for agent function definitions
    gen::GenHeaderFile func_def_hpp;
    generate_agent_func_definition(&model, &maincpp, &func_def_hpp);
    filegen.Output("agent_function_definitions.hpp", func_def_hpp);
    makefile.AddHeaderFile("agent_function_definitions.hpp");
    
    // Define and register messages
    gen::GenHeaderFile msg_datatype_h;
    generate_messages(&model, &maincpp, &msg_datatype_h);
    filegen.Output("message_datatypes.hpp", msg_datatype_h);
    makefile.AddHeaderFile("message_datatypes.hpp");

    // output completed main.cpp file
    filegen.Output("main.cpp", maincpp);
    makefile.AddSourceFile("main.cpp");
    
    // Add user agent function files to Makefile
    std::vector<std::string> * functionFiles = model.getFunctionFiles();
    std::vector<std::string>::iterator functionFile;
    for (functionFile = functionFiles->begin(); functionFile != functionFiles->end();
            ++functionFile) {
        makefile.AddSourceFile((*functionFile));
    }

    // Output Makefile now that all hpp and cpp files have been added
    filegen.Output("Makefile", makefile);

    return 0;
}

void generate_agent_func_definition(flame::model::XModel *model,
                                    gen::GenMainCpp *maincpp,
                                    gen::GenHeaderFile *func_def_hpp) {
  gen::AgentFunctionHeaderSnippets agent_func_headers;
  gen::RegisterAgentFuncSnippets register_agent_func;
  boost::ptr_vector<flame::model::XFunction>::iterator func;
  boost::ptr_vector<flame::model::XMachine>::iterator agent;
  boost::ptr_vector<flame::model::XMachine> *agents = model->getAgents();

  // for each agent function
  for (agent = agents->begin(); agent != agents->end(); ++agent) {
    boost::ptr_vector<flame::model::XFunction> * funcs = agent->getFunctions();
    for (func = funcs->begin(); func != funcs->end(); ++func) {
      agent_func_headers.Add(func->getName());  // func declaration
      register_agent_func.Add(func->getName()); // func registrations
    }
  }
  
  // append generated content to apropriate files
  maincpp->Insert(register_agent_func);
  func_def_hpp->Insert(agent_func_headers);
}
                                    
void generate_messages(flame::model::XModel *model,
                       gen::GenMainCpp *maincpp,
                       gen::GenHeaderFile *msg_datatype_h) {
  boost::ptr_vector<flame::model::XVariable>::iterator v;
  boost::ptr_vector<flame::model::XMessage>::iterator m;
  boost::ptr_vector<flame::model::XMessage> *messages = model->getMessages();
  for (m = messages->begin(); m != messages->end(); ++m) {
    gen::GenMessageRegistration msg_reg(m->getName());
    gen::GenDataStruct msg_datatype(m->getName() + "_message");
    
    // populate message vars
    boost::ptr_vector<flame::model::XVariable> *vars = m->getVariables();
    for (v = vars->begin(); v != vars->end(); ++v) {
      msg_reg.AddVar(v->getType(), v->getName());
      msg_datatype.AddVar(v->getType(), v->getName());
    }
    
    // Append to main.cpp
    maincpp->Insert(msg_reg);

    // Append to message_datatype.hpp
    msg_datatype_h->Insert(msg_datatype);
  }
}
                       
void generate_agents(flame::model::XModel *model,
                     xparser::codegen::GenMainCpp *maincpp) {
  boost::ptr_vector<flame::model::XMachine>::iterator agent;
  boost::ptr_vector<flame::model::XMachine> *agents = model->getAgents();
  for (agent = agents->begin(); agent != agents->end(); ++agent) {
    std::string agent_name = agent->getName();
    gen::GenAgent gen_agent(agent_name);
    
    // iterate through agent memory var
    boost::ptr_vector<flame::model::XVariable> *vars = agent->getVariables();
    boost::ptr_vector<flame::model::XVariable>::iterator v;
    for (v = vars->begin(); v != vars->end(); ++v) {
      gen_agent.AddVar(v->getType(), v->getName());
    }
    maincpp->Insert(gen_agent);
    
    // iterate throught agent functions
    boost::ptr_vector<flame::model::XFunction> *funcs = agent->getFunctions();
    boost::ptr_vector<flame::model::XFunction>::iterator f;
    for (f = funcs->begin(); f != funcs->end(); ++f) {
      gen::GenAgentFunc gen_func(agent_name, f->getName(),
                                 f->getCurrentState(), f->getNextState());
      // loop outputs
      boost::ptr_vector<flame::model::XIOput>::iterator ioput;
      boost::ptr_vector<flame::model::XIOput> * outputs = f->getOutputs();
      for (ioput = outputs->begin(); ioput != outputs->end(); ++ioput) {
        gen_func.AddInput(ioput->getMessageName());
      }
      
      // loop inputs
      boost::ptr_vector<flame::model::XIOput> * inputs = f->getInputs();
      for (ioput = inputs->begin(); ioput != inputs->end(); ++ioput) {
        gen_func.AddOutput(ioput->getMessageName());
      }

      // memory access
      std::vector<std::string>::const_iterator s;
      std::vector<std::string> *rw = f->getReadWriteVariables();
      for (s = rw->begin(); s != rw->end(); ++s) {
        gen_func.AddReadWriteVar(*s);
      }
      
      // specify read-only vars
      std::vector<std::string> *ro = f->getReadOnlyVariables();
      for (s = ro->begin(); s != ro->end(); ++s) {
        gen_func.AddReadOnlyVar(*s);
      }

      // append func def to main.cpp
      maincpp->Insert(gen_func);
    }
  }
}

/*
void generateConditionFunction(flame::model::XCondition * condition, xparser::Printer * p) {
    std::map<std::string, std::string> variables;

    if (condition->isNot) p->Print("!");
    p->Print("(");
    if (condition->isValues) {
        // Handle lhs
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
        // Handle operator
        variables["op"] = condition->op;
        p->Print(" $op$ ", variables);
        // Handle rhs
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
        // Handle lhs 
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
        // Handle operator 
        variables["op"] = condition->op;
        p->Print("$cname$->op = \"$op$\";\n", variables);
        // Handle rhs 
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
*/
