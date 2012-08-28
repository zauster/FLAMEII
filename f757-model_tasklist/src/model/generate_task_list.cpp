/*!
 * \file src/model/generate_task_list.cpp
 * \author Simon Coakley
 * \date 2012
 * \copyright Copyright (c) 2012 STFC Rutherford Appleton Laboratory
 * \copyright Copyright (c) 2012 University of Sheffield
 * \copyright GNU Lesser General Public License
 * \brief Generates task list
 */
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/graphviz.hpp>
#include "./model_manager.hpp"
#include "./task.hpp"

namespace flame { namespace model {

/*!
 * \brief Generates task list
 * \return The return code
 * Produces task list with state/data/communication dependencies.
 */
int ModelManager::generate_task_list() {
/*    int rc;
    // Catalog state dependencies
    rc = catalog_state_dependencies(&model_, &tasks_);
    // Catalog communication dependencies
    rc = catalog_communication_dependencies(&model_, &tasks_);
    // Check for dependency loops
    rc = check_dependency_loops(&model_);
    // Calculate dependencies
    rc = calculate_dependencies(&tasks_);
    // Catalog data dependencies
    rc = catalog_data_dependencies(&model_, &tasks_);
    // Calculate task list using dependencies
    rc = calculate_task_list(&tasks_);

#ifdef TESTBUILD
    // Output dependency graph to view via graphviz dot
    write_dependency_graph("dgraph.dot", &tasks_);
#endif
*/

    //Todo Map to Task not string
    std::map<std::string, vertex_descriptor> name2vertex;
    /*std::string vertex_name = "test";
    vertex_descriptor v;
    if (name2vertex.find(vertex_name) == name2vertex.end()) {
        v = add_vertex(graph_);
        name2vertex.insert(make_pair(vertex_name, v));
    } else {
        v = name2vertex[vertex_name];
    }*/

    std::map<vertex_descriptor, std::string> vertex2name;

    vertex_descriptor v1 = add_vertex(graph_);
    name2vertex.insert(make_pair(std::string("a"), v1));
    vertex2name.insert(make_pair(v1, std::string("a")));
    vertex_descriptor v2 = add_vertex(graph_);
    name2vertex.insert(make_pair(std::string("b"), v2));
    vertex2name.insert(make_pair(v2, std::string("b")));
    vertex_descriptor v3 = add_vertex(graph_);
    name2vertex.insert(make_pair(std::string("c"), v3));
    vertex2name.insert(make_pair(v3, std::string("c")));
    add_edge(v1, v2, graph_);
    add_edge(v1, v3, graph_);

    typedef boost::property_map<Graph, boost::vertex_index_t>::type IndexMap;
    IndexMap index = get(boost::vertex_index, graph_);

    std::vector<std::string> name_vec(num_vertices(graph_));
    boost::iterator_property_map<std::vector<std::string>::iterator, IndexMap> name(name_vec.begin(), index);
    std::map<std::string, vertex_descriptor>::iterator i;
    for (i = name2vertex.begin(); i != name2vertex.end(); ++i)
       name[i->second] = i->first;

    std::vector<vertex_descriptor> topo_order(num_vertices(graph_));
    boost::topological_sort(graph_, topo_order.rbegin());
    std::cout << "topological order:\n";
     for (std::vector<vertex_descriptor>::iterator i = topo_order.begin();
         i != topo_order.end(); ++i) {
       std::cout << name[*i] << "\n";
       std::cout << vertex2name.find(*i)->second << "\n";
     }

     boost::write_graphviz(std::cout, graph_);

    return 0;
}

int ModelManager::catalog_state_dependencies_functions(
        XModel * model, std::vector<Task*> * tasks) {
    std::vector<XMachine*>::iterator agent;
    std::vector<XFunction*>::iterator f;
    /* For each agent */
    for (agent = model->getAgents()->begin();
            agent != model->getAgents()->end(); ++agent) {
        /* For each function */
        for (f = (*agent)->getFunctions()->begin();
                f != (*agent)->getFunctions()->end(); ++f) {
            /* Add function as a task to the task list */
            Task * task = new Task;
            task->setParentName((*agent)->getName());
            task->setName((*f)->getName());
            task->setTaskType(Task::xfunction);
            task->setPriorityLevel(5);
            tasks->push_back(task);
            /* Associate task with function */
            (*f)->setTask(task);
        }
    }

    return 0;
}

int ModelManager::catalog_state_dependencies_transitions(XModel * model) {
    std::vector<XMachine*>::iterator agent;
    std::vector<XFunction*>::iterator f;
    std::vector<XFunction*>::iterator f2;
    /* For each agent */
    for (agent = model->getAgents()->begin();
            agent != model->getAgents()->end(); ++agent) {
        /* For each function */
        for (f = (*agent)->getFunctions()->begin();
                f != (*agent)->getFunctions()->end(); ++f) {
            /* Add state dependencies to tasks */
            /* For each transition functions start state
             * find transition functions that end in that state */
            for (f2 = (*agent)->getFunctions()->begin();
                    f2 != (*agent)->getFunctions()->end(); ++f2) {
                if ((*f)->getCurrentState() == (*f2)->getNextState()) {
                    Dependency * d = new Dependency;
                    d->setName((*f)->getCurrentState());
                    d->setDependencyType(Dependency::state);
                    d->setTask((*f2)->getTask());
                    (*f)->getTask()->addDependency(d);
                }
            }
        }
    }
    return 0;
}

int ModelManager::catalog_state_dependencies(
        XModel * model, std::vector<Task*> * tasks) {
    catalog_state_dependencies_functions(model, tasks);
    catalog_state_dependencies_transitions(model);

    return 0;
}

int ModelManager::catalog_communication_dependencies_syncs(
        XModel * model,
        std::vector<Task*> * tasks) {
    std::vector<XMessage*>::iterator message;
    /* Add sync_start and sync_finish
     * for each message type */
    for (message = model->getMessages()->begin();
         message != model->getMessages()->end(); ++message) {
        /* Add sync start tasks to the task list */
        Task * syncStartTask = new Task;
        syncStartTask->setParentName((*message)->getName());
        syncStartTask->setName("sync_start");
        syncStartTask->setTaskType(Task::sync_start);
        syncStartTask->setPriorityLevel(10);
        tasks->push_back(syncStartTask);
        (*message)->setSyncStartTask(syncStartTask);
        /* Add sync finish tasks to the task list */
        Task * syncFinishTask = new Task;
        syncFinishTask->setParentName((*message)->getName());
        syncFinishTask->setName("sync_finish");
        syncFinishTask->setTaskType(Task::sync_finish);
        syncFinishTask->setPriorityLevel(1);
        tasks->push_back(syncFinishTask);
        (*message)->setSyncFinishTask(syncFinishTask);
        /* Add dependency between start and finish sync tasks */
        syncFinishTask->addParent(
                (*message)->getName(),
                Dependency::communication,
                syncStartTask);
    }
    return 0;
}

int ModelManager::catalog_communication_dependencies_ioput(XModel * model,
        std::vector<XFunction*>::iterator function) {
    std::vector<XIOput*>::iterator ioput;
    std::vector<XMessage*>::iterator message;

    /* Find outputting functions */
    for (ioput = (*function)->getOutputs()->begin();
         ioput != (*function)->getOutputs()->end(); ++ioput) {
        /* Find associated messages */
        for (message = model->getMessages()->begin();
             message != model->getMessages()->end(); ++message) {
            if ((*message)->getName() == (*ioput)->getMessageName())
                /* Add communication dependency */
                (*message)->getSyncStartTask()->addParent(
                        (*ioput)->getMessageName(),
                        Dependency::communication,
                        (*function)->getTask());
        }
    }

    /* Find inputting functions */
    for (ioput = (*function)->getInputs()->begin();
         ioput != (*function)->getInputs()->end(); ++ioput) {
        /* Find associated messages */
        for (message = model->getMessages()->begin();
                message != model->getMessages()->end(); ++message) {
            if ((*message)->getName() == (*ioput)->getMessageName())
                /* Add communication dependency */
                (*function)->getTask()->addParent(
                        (*ioput)->getMessageName(),
                        Dependency::communication,
                        (*message)->getSyncFinishTask());
        }
    }

    return 0;
}

int ModelManager::catalog_communication_dependencies(XModel * model,
        std::vector<Task*> * tasks) {
    std::vector<XMachine*>::iterator agent;
    std::vector<XFunction*>::iterator function;

    /* Remove unused messages or
     * messages not read or
     * messages not sent and give warning. */

    catalog_communication_dependencies_syncs(model, tasks);

    /* Find dependencies */
    /* For each agent */
    for (agent = model->getAgents()->begin();
         agent != model->getAgents()->end(); ++agent) {
        /* For each function */
        for (function = (*agent)->getFunctions()->begin();
             function != (*agent)->getFunctions()->end(); ++function) {
            catalog_communication_dependencies_ioput(model, function);
        }
    }

    return 0;
}

int ModelManager::catalog_data_dependencies_variable(
        std::vector<XMachine*>::iterator agent,
        std::vector<XVariable*>::iterator variable,
        std::vector<Task*> * tasks) {
    std::vector<XFunction*>::iterator function;
    std::vector<XVariable*>::iterator variableFind;
    /* Add variable to disk task */
    Task * dataTask = new Task;
    dataTask->setParentName((*agent)->getName());
    dataTask->setName((*variable)->getName());
    dataTask->setTaskType(Task::io_pop_write);
    dataTask->setPriorityLevel(0);
    tasks->push_back(dataTask);
    /* Add dependency parents to task */
    /* Find the last function that writes each variable */
    XFunction * lastFunction = 0;
    for (function = (*agent)->getFunctions()->begin();
            function != (*agent)->getFunctions()->end(); ++function) {
        variableFind = std::find(
                (*function)->getReadWriteVariables()->begin(),
                (*function)->getReadWriteVariables()->end(), (*variable));
        if (variableFind != (*function)->getReadWriteVariables()->end()
                || lastFunction == 0) lastFunction = (*function);
    }
    /* Add data dependency */
    dataTask->addParent((*variable)->getName(), Dependency::data,
            lastFunction->getTask());
    /* Give function higher level */
    dataTask->setLevel(lastFunction->getTask()->getLevel()+1);

    return 0;
}

/*!
 * \brief Catalogs data dependencies
 * \ingroup FLAME_MODEL
 * \param[in] model The FLAME model
 * \param[out] tasks The task list
 * \return Return error code
 *
 * For each agent memory variable add a task for writing the variable to disk.
 */
int ModelManager::catalog_data_dependencies(XModel * model,
        std::vector<Task*> * tasks) {
    std::vector<XMachine*>::iterator agent;
    std::vector<XVariable*>::iterator variable;
    std::vector<XVariable*>::iterator variableFind;
    std::vector<XFunction*>::iterator function;

    /* For each agent */
    for (agent = model->getAgents()->begin();
         agent != model->getAgents()->end(); ++agent) {
        for (variable = (*agent)->getVariables()->begin();
                variable != (*agent)->getVariables()->end(); ++variable) {
            catalog_data_dependencies_variable(agent, variable, tasks);
        }
    }

    return 0;
}

int ModelManager::check_dependency_loops(XModel * model) {
    return 0;
}

std::string ModelManager::taskTypeToString(Task::TaskType t) {
    /* Convert Task task to printable string */
    if (t == Task::io_pop_write) return "disk";
    else if (t == Task::sync_finish) return "comm";
    else if (t == Task::sync_start) return "comm";
    else if (t == Task::xfunction) return "func";
    /* If task not recognised return empty string */
    else
        return "";
}

void ModelManager::printTaskList(std::vector<Task*> * tasks) {
    std::vector<Task*>::iterator task;

    fprintf(stdout, "Level\tPriority\tType\tName\n");
    fprintf(stdout, "-----\t--------\t----\t----\n");
    for (task = tasks->begin(); task != tasks->end(); ++task) {
        fprintf(stdout, "%u\t%u\t\t%s\t%s\n",
                static_cast<unsigned int>((*task)->getLevel()),
                static_cast<unsigned int>((*task)->getPriorityLevel()),
                taskTypeToString((*task)->getTaskType()).c_str(),
                (*task)->getFullName().c_str());
    }
}

int ModelManager::calculate_dependencies(std::vector<Task*> * tasks) {
    std::vector<Task*>::iterator task;
    size_t ii;

    /* Initialise task levels to be zero */
    for (task = tasks->begin(); task != tasks->end(); ++task)
        (*task)->setLevel(0);

    /* Calculate layers of dependency graph */
    /* This is achieved by finding functions with no dependencies */
    /* giving them a layer no, taking those functions away and doing
     * the operation again. */
    /* Boolean to track if all tasks have been leveled */
    bool finished = false;
    /* The current level that is being populated */
    size_t currentLevel = 1;
    /* Loop while tasks still being leveled */
    while (!finished) {
        finished = true;    /* Set finished to be true, until unleveled task */
        /* For every task */
        for (task = tasks->begin(); task != tasks->end(); ++task) {
            /* If task is not leveled */
            if ((*task)->getLevel() == 0) {
                /* Boolean to track if any dependencies
                 * still need to be leveled */
                bool unleveled_dependency = false;
                /* For each dependency */
                /* Didn't use iterator here as caused valgrind errors */
                for (ii = 0; ii < (*task)->getParents().size(); ii++) {
                    Dependency * dependency = (*task)->getParents().at(ii);
                    /* If the dependency is not leveled or just been leveled
                     * at the current level that is being populated */
                    if ((dependency)->getTask()->getLevel() == 0 ||
                        (dependency)->getTask()->getLevel() == currentLevel)
                        /* Set that current task has an unleveled dependency */
                        unleveled_dependency = true;
                }
                /* If no unleveled dependencies */
                if (!unleveled_dependency)
                    /* Add task to current level */
                    (*task)->setLevel(currentLevel);
                else
                    /* Else leveling has not finished */
                    finished = false;
            }
        }
        /* Increment current level */
        currentLevel++;
    }

    return 0;
}

bool compare_task_levels(Task * i, Task * j) {
    /* If same level then use priority level */
    if (i->getLevel() == j->getLevel()) {
        return (i->getPriorityLevel() > j->getPriorityLevel());
    } else {
        return (i->getLevel() < j->getLevel());
    }
}

int ModelManager::calculate_task_list(std::vector<Task*> * tasks) {
    /* Sort the task list by level */
    sort(tasks->begin(), tasks->end(), compare_task_levels);

    printTaskList(tasks);

    return 0;
}

void ModelManager::write_dependency_graph_dependencies(
        std::vector<Task*>::iterator task, FILE *file) {
    size_t ii;
    /* For every dependency */
    /* Didn't use iterator here as caused valgrind errors */
    for (ii = 0; ii < (*task)->getParents().size(); ii++) {
        Dependency * dependency = (*task)->getParents().at(ii);
        fputs("\t", file);
        fputs((*task)->getParentName().c_str(), file);
        fputs("_", file);
        fputs((*task)->getName().c_str(), file);
        fputs(" -> ", file);
        fputs(dependency->getTask()->getParentName().c_str(), file);
        fputs("_", file);
        fputs(dependency->getTask()->getName().c_str(), file);
        fputs(" [ label = \"<", file);
        /* Check dependency type and output
         * appropriate description */
        if (dependency->getDependencyType() == Dependency::communication)
            fputs("Message: ", file);
        else if (dependency->getDependencyType() == Dependency::data)
            fputs("Memory: ", file);
        else if (dependency->getDependencyType() == Dependency::state)
            fputs("State: ", file);
        fputs(dependency->getName().c_str(), file);
        fputs(">\" ];\n", file);
    }
}

void ModelManager::write_dependency_graph(
        std::string filename, std::vector<Task*> * tasks) {
    /* File to write to */
    FILE *file;
    std::vector<Task*>::iterator task;


    /* print out the location of the source file */
    printf("Writing file: %s\n", filename.c_str());
    /* open the file to write to */
    file = fopen(filename.c_str(), "w");

    fputs("digraph dependency_graph {\n", file);
    fputs("\trankdir=BT;\n", file);
    fputs("\tsize=\"8,5;\"\n", file);
    fputs("\tnode [shape = rect];\n", file);

    fputs("\t\n\t/* Tasks */\n", file);
    /* For each task */
    for (task = tasks->begin(); task != tasks->end(); ++task) {
        fputs("\t", file);
        fputs((*task)->getParentName().c_str(), file);
        fputs("_", file);
        fputs((*task)->getName().c_str(), file);
        fputs("[label = \"", file);
        fputs((*task)->getParentName().c_str(), file);
        fputs("\\n", file);
        fputs((*task)->getName().c_str(), file);
        fputs("\"]\n", file);

        write_dependency_graph_dependencies(task, file);
    }
    fputs("}", file);

    /* Close the file */
    fclose(file);
}

}}  // namespace flame::model
