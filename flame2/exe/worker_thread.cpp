/*!
 * \file flame2/exe/worker_thread.cpp
 * \author Shawn Chin
 * \date 2012
 * \copyright Copyright (c) 2012 STFC Rutherford Appleton Laboratory
 * \copyright Copyright (c) 2012 University of Sheffield
 * \copyright GNU Lesser General Public License
 * \brief Basic worker thread
 */
#include <ctime>
#include <cstdlib>
#include <boost/thread/mutex.hpp>
#include "flame2/config.hpp"
#include "task_queue_interface.hpp"
#include "worker_thread.hpp"
namespace flame { namespace exe {

WorkerThread::WorkerThread(TaskQueue* taskqueue_ptr) : tq_(taskqueue_ptr) {}

//! Starts the thread
void WorkerThread::Init() {
  thread_ = boost::thread(&WorkerThread::ProcessQueue, this);
}

//! Waits for thread to complete
void WorkerThread::join() {
  thread_.join();
}

/*!
 * \brief Business logic for the thread
 *
 * Retrieves tasks from the parent queue and runs them. Continue until a
 * Termination task is issued.
 */
void WorkerThread::ProcessQueue() {
  #ifdef DBGBUILD
    srand(1);  // for debug, fix thread-local random seed
  #else
    srand(time(NULL));
  #endif

  // #ifdef TESTBUILD
  //   boost::thread::id tid = boost::this_thread::get_id();
  //   std::cout << ">> Thread started : " << tid << std::endl;
  // #endif
  Task::id_type task_id = tq_->GetNextTask();  // calls wait() if queue empty

  while (!Task::IsTermTask(task_id)) {
    RunTask(task_id);
    tq_->TaskDone(task_id);  // register completed task
    task_id = tq_->GetNextTask();  // calls wait() if queue empty
  }
  // #ifdef TESTBUILD
  //   std::cout << ">> (Thread ending) : " << tid << std::endl;
  // #endif
}

//! Runs a given task
void WorkerThread::RunTask(Task::id_type task_id) {
  // #ifdef TESTBUILD
  //   std::cout << " - " << boost::this_thread::get_id()
  //            << " running task " << task_id << std::endl;
  // #endif
  tq_->GetTaskById(task_id).Run();
}

}}  // namespace flame::exe
