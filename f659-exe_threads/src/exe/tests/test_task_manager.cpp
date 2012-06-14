/*!
 * \file src/exe/tests/test_task_manager.cpp
 * \author Shawn Chin
 * \date 2012
 * \copyright Copyright (c) 2012 STFC Rutherford Appleton Laboratory
 * \copyright Copyright (c) 2012 University of Sheffield
 * \copyright GNU Lesser General Public License
 * \brief Test suite for task manager
 */
#define BOOST_TEST_DYN_LINK
#include <vector>
#include <string>
#include "boost/test/unit_test.hpp"
#include "include/flame.h"
#include "../task_manager.hpp"

BOOST_AUTO_TEST_SUITE(TaskManager)

namespace exe = flame::exe;
namespace mem = flame::mem;

// dummy function
FLAME_AGENT_FUNC(func1) { return 0; }

BOOST_AUTO_TEST_CASE(taskmgr_initialise_and_test_singleton) {
  std::string task_name = "test_task";
  mem::MemoryManager& mgr = mem::MemoryManager::GetInstance();
  mgr.RegisterAgent("Circle");

  exe::TaskManager& tm1 = exe::TaskManager::GetInstance();
  tm1.CreateTask(task_name, "Circle", &func1);

  exe::TaskManager& tm2 = exe::TaskManager::GetInstance();
  exe::Task& task = tm2.GetTask(task_name);
  BOOST_CHECK_EQUAL(task.get_task_name(), task_name);


  tm2.Reset();  // TEST BUILD only method
  mgr.Reset();
  BOOST_CHECK_THROW(tm2.GetTask(task_name),
                    flame::exceptions::invalid_argument);
}

BOOST_AUTO_TEST_CASE(initialise_memory_manager) {
  mem::MemoryManager& mgr = mem::MemoryManager::GetInstance();
  mgr.RegisterAgent("Circle");
  mgr.RegisterAgentVar<int>("Circle", "x_int");
  mgr.RegisterAgentVar<double>("Circle", "y_dbl");
  mgr.HintPopulationSize("Circle", (size_t)10);
  BOOST_CHECK_EQUAL(mgr.GetAgentCount(), (size_t)1);

  mgr.GetVector<int>("Circle", "x_int")->push_back(1);
  mgr.GetVector<double>("Circle", "y_dbl")->push_back(0.1);
}

BOOST_AUTO_TEST_CASE(test_create_task) {
  exe::TaskManager& tm = exe::TaskManager::GetInstance();
  BOOST_CHECK_THROW(tm.CreateTask("t1", "NotAnAgent", &func1),
                    flame::exceptions::invalid_agent);
  BOOST_CHECK_THROW(tm.CreateTask("t1", "Circle", NULL),
                    flame::exceptions::invalid_argument);

  exe::Task& task = tm.CreateTask("t1", "Circle", &func1);
  task.AllowAccess("x_int");
  task.AllowAccess("y_dbl", true);
  mem::MemoryIteratorPtr m_iter = task.GetMemoryIterator();
  BOOST_CHECK_THROW(task.AllowAccess("NotAVar"),
                    flame::exceptions::invalid_variable);

  BOOST_CHECK_THROW(tm.GetTask("NotATask"),
                    flame::exceptions::invalid_argument);
  exe::Task& t1 = tm.GetTask("t1");
  BOOST_CHECK_EQUAL(t1.get_task_name(), "t1");

  // get by id
  exe::Task& t2 = tm.GetTask(t1.get_task_id());
  BOOST_CHECK_EQUAL(t2.get_task_name(), t1.get_task_name());
  BOOST_CHECK_THROW(tm.GetTask(1000), flame::exceptions::invalid_argument);

  // reset
  tm.Reset();
}

BOOST_AUTO_TEST_CASE(test_dep_management) {
  exe::TaskManager& tm = exe::TaskManager::GetInstance();

  // check initial data structure
  BOOST_CHECK_EQUAL(tm.get_num_roots(), 0);
  BOOST_CHECK_EQUAL(tm.get_num_leaves(), 0);
  BOOST_CHECK_EQUAL(tm.GetTaskCount(), 0);

  // add task
  tm.CreateTask("t1", "Circle", &func1);
  tm.CreateTask("t2", "Circle", &func1);
  tm.CreateTask("t3", "Circle", &func1);
  tm.CreateTask("t4", "Circle", &func1);
  BOOST_CHECK_EQUAL(tm.get_num_roots(), 4);
  BOOST_CHECK_EQUAL(tm.get_num_leaves(), 4);
  BOOST_CHECK_EQUAL(tm.GetTaskCount(), 4);

  BOOST_CHECK_THROW(tm.AddDependency("t1", "x"),
                    flame::exceptions::invalid_argument);
  BOOST_CHECK_THROW(tm.AddDependency("x", "t2"),
                    flame::exceptions::invalid_argument);

  tm.AddDependency("t3", "t1");
  BOOST_CHECK_EQUAL(tm.get_num_roots(), 3);
  BOOST_CHECK_EQUAL(tm.get_num_leaves(), 3);
  BOOST_CHECK_EQUAL(tm.GetTaskCount(), 4);
  BOOST_CHECK_EQUAL(tm.GetDependencies("t3").size(), 1);
  BOOST_CHECK_EQUAL(tm.GetDependencies("t1").size(), 0);
  BOOST_CHECK_EQUAL(tm.GetDependents("t3").size(), 0);
  BOOST_CHECK_EQUAL(tm.GetDependents("t1").size(), 1);

  tm.AddDependency("t4", "t1");
  tm.AddDependency("t4", "t2");
  tm.AddDependency("t4", "t3");
  BOOST_CHECK_EQUAL(tm.get_num_roots(), 2);
  BOOST_CHECK_EQUAL(tm.get_num_leaves(), 1);
  BOOST_CHECK_EQUAL(tm.GetDependencies("t1").size(), 0);
  BOOST_CHECK_EQUAL(tm.GetDependencies("t2").size(), 0);
  BOOST_CHECK_EQUAL(tm.GetDependencies("t3").size(), 1);
  BOOST_CHECK_EQUAL(tm.GetDependencies("t4").size(), 3);
  BOOST_CHECK_EQUAL(tm.GetDependents("t1").size(), 2);
  BOOST_CHECK_EQUAL(tm.GetDependents("t2").size(), 1);
  BOOST_CHECK_EQUAL(tm.GetDependents("t3").size(), 1);
  BOOST_CHECK_EQUAL(tm.GetDependents("t4").size(), 0);

  BOOST_CHECK_THROW(tm.GetDependencies("x"),
                    flame::exceptions::invalid_argument);
  BOOST_CHECK_THROW(tm.GetDependents("x"), flame::exceptions::invalid_argument);

  BOOST_CHECK_THROW(tm.AddDependency("t4", "t4"),
                    flame::exceptions::logic_error);

  BOOST_CHECK_THROW(tm.AddDependency("t1", "t4"),
                    flame::exceptions::logic_error);

  // reset
  tm.Reset();
}

BOOST_AUTO_TEST_CASE(test_task_iteration) {
  exe::TaskManager& tm = exe::TaskManager::GetInstance();
  tm.CreateTask("t1", "Circle", &func1);
  tm.CreateTask("t2", "Circle", &func1);
  tm.CreateTask("t3", "Circle", &func1);
  tm.CreateTask("t4", "Circle", &func1);
  tm.AddDependency("t3", "t1");
  tm.AddDependency("t4", "t1");
  tm.AddDependency("t4", "t2");
  tm.AddDependency("t4", "t3");

  // tm not yet finalised
  BOOST_CHECK_THROW(tm.IterReset(), flame::exceptions::logic_error);
  BOOST_CHECK_THROW(tm.IterTaskAvailable(), flame::exceptions::logic_error);
  BOOST_CHECK_THROW(tm.IterTaskPop(), flame::exceptions::logic_error);
  BOOST_CHECK_THROW(tm.IterTaskDone(100), flame::exceptions::logic_error);
  BOOST_CHECK_THROW(tm.IterCompleted(), flame::exceptions::logic_error);
  BOOST_CHECK_THROW(tm.IterGetReadyCount(), flame::exceptions::logic_error);
  BOOST_CHECK_THROW(tm.IterGetPendingCount(), flame::exceptions::logic_error);
  BOOST_CHECK_THROW(tm.IterGetAssignedCount(), flame::exceptions::logic_error);

  // Once finalised, tasks and deps can no longer be added
  tm.Finalise();
  BOOST_CHECK_THROW(tm.CreateTask("t8", "Circle", &func1),
                    flame::exceptions::logic_error);
  BOOST_CHECK_THROW(tm.AddDependency("t3", "t2"),
                    flame::exceptions::logic_error);

  BOOST_CHECK(tm.IterTaskAvailable());
  BOOST_CHECK(!tm.IterCompleted());
  BOOST_CHECK_EQUAL(tm.IterGetReadyCount(), 2);
  BOOST_CHECK_EQUAL(tm.IterGetPendingCount(), 2);
  BOOST_CHECK_EQUAL(tm.IterGetAssignedCount(), 0);

  exe::TaskManager::TaskId t1 = tm.get_id("t1");  // test-only routine
  exe::TaskManager::TaskId t2 = tm.get_id("t2");  // test-only routine
  exe::TaskManager::TaskId t3 = tm.get_id("t3");  // test-only routine
  exe::TaskManager::TaskId t4 = tm.get_id("t4");  // test-only routine

  // completing a task that has not been popped
  BOOST_CHECK_THROW(tm.IterTaskDone(t1), flame::exceptions::invalid_argument);

  unsigned int count = 0;
  while (tm.IterTaskAvailable()) {
    tm.IterTaskPop();
    count++;
    BOOST_CHECK_EQUAL(tm.IterGetReadyCount(), 2 - count);
    BOOST_CHECK_EQUAL(tm.IterGetAssignedCount(), count);
    BOOST_CHECK_EQUAL(tm.IterGetPendingCount(), 2);
  }
  BOOST_CHECK_EQUAL(count, 2);
  BOOST_CHECK_THROW(tm.IterTaskPop(), flame::exceptions::none_available);
  BOOST_CHECK(!tm.IterCompleted());

  // t2 complete
  tm.IterTaskDone(t2);
  BOOST_CHECK_EQUAL(tm.IterGetReadyCount(), 0);
  BOOST_CHECK_EQUAL(tm.IterGetPendingCount(), 2);
  BOOST_CHECK_EQUAL(tm.IterGetAssignedCount(), 1);
  BOOST_CHECK(!tm.IterTaskAvailable());
  BOOST_CHECK(!tm.IterCompleted());

  // t1 complete
  tm.IterTaskDone(t1);
  BOOST_CHECK_EQUAL(tm.IterGetReadyCount(), 1);
  BOOST_CHECK_EQUAL(tm.IterGetPendingCount(), 1);
  BOOST_CHECK_EQUAL(tm.IterGetAssignedCount(), 0);
  BOOST_CHECK(tm.IterTaskAvailable());
  BOOST_CHECK(!tm.IterCompleted());

  // pop t3 which should now be ready
  exe::TaskManager::TaskId t = tm.IterTaskPop();
  BOOST_CHECK_EQUAL(t, t3);
  BOOST_CHECK_EQUAL(tm.IterGetReadyCount(), 0);
  BOOST_CHECK_EQUAL(tm.IterGetPendingCount(), 1);
  BOOST_CHECK_EQUAL(tm.IterGetAssignedCount(), 1);
  BOOST_CHECK(!tm.IterTaskAvailable());
  BOOST_CHECK_THROW(tm.IterTaskPop(), flame::exceptions::none_available);
  BOOST_CHECK(!tm.IterCompleted());

  // t3 complete
  tm.IterTaskDone(t3);
  BOOST_CHECK_EQUAL(tm.IterGetReadyCount(), 1);
  BOOST_CHECK_EQUAL(tm.IterGetPendingCount(), 0);
  BOOST_CHECK_EQUAL(tm.IterGetAssignedCount(), 0);
  BOOST_CHECK(tm.IterTaskAvailable());
  BOOST_CHECK(!tm.IterCompleted());

  // pop t4 which should now be ready
  t = tm.IterTaskPop();
  BOOST_CHECK_EQUAL(t, t4);
  BOOST_CHECK_EQUAL(tm.IterGetReadyCount(), 0);
  BOOST_CHECK_EQUAL(tm.IterGetPendingCount(), 0);
  BOOST_CHECK_EQUAL(tm.IterGetAssignedCount(), 1);
  BOOST_CHECK(!tm.IterTaskAvailable());
  BOOST_CHECK_THROW(tm.IterTaskPop(), flame::exceptions::none_available);
  BOOST_CHECK(!tm.IterCompleted());

  // t4 complete
  tm.IterTaskDone(t4);
  BOOST_CHECK_EQUAL(tm.IterGetReadyCount(), 0);
  BOOST_CHECK_EQUAL(tm.IterGetPendingCount(), 0);
  BOOST_CHECK_EQUAL(tm.IterGetAssignedCount(), 0);
  BOOST_CHECK(!tm.IterTaskAvailable());
  BOOST_CHECK(tm.IterCompleted());

  // reset for next iteration
  tm.IterReset();
  BOOST_CHECK_EQUAL(tm.IterGetReadyCount(), 2);
  BOOST_CHECK_EQUAL(tm.IterGetPendingCount(), 2);
  BOOST_CHECK_EQUAL(tm.IterGetAssignedCount(), 0);
  BOOST_CHECK(tm.IterTaskAvailable());
  BOOST_CHECK(!tm.IterCompleted());

}

BOOST_AUTO_TEST_CASE(reset_memory_manager) {
  mem::MemoryManager& mgr = mem::MemoryManager::GetInstance();
  mgr.Reset();  // reset again so as not to affect next test suite
  BOOST_CHECK_EQUAL(mgr.GetAgentCount(), (size_t)0);
}

BOOST_AUTO_TEST_SUITE_END()
