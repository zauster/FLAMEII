/*!
 * \file flame2/exe/scheduler.cpp
 * \author Shawn Chin
 * \date 2012
 * \copyright Copyright (c) 2012 STFC Rutherford Appleton Laboratory
 * \copyright Copyright (c) 2012 University of Sheffield
 * \copyright GNU Lesser General Public License
 * \brief Implementation of Scheduler
 */
// Note: for some versions of Boost, valgrind --leak-check=full may claim
// than "8 bytes in block" still reachable. This is not so and is safe to
// ignore.
// See: http://stackoverflow.com/questions/6321602/boost-thread-leakage-c
#include <stdexcept>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/foreach.hpp>
#include "flame2/config.hpp"
#include "flame2/io/io_manager.hpp"
#include "flame2/exceptions/all.hpp"
#include "task_manager.hpp"
#include "scheduler.hpp"

namespace flame { namespace exe {

/*!
 * \brief assigns a task type to the given queue
 * \param[in] qid Queue ID to assign the task type to
 * \param[in] type The task type to assign to the queue
 *
 * Throws flame::exceptions:invalid_argument if the queue id is invalid
 * or if the type has already been assigned.
 */
void Scheduler::AssignType(QueueId qid, Task::TaskType type) {
  if (!IsValidQueueId(qid)) {
    throw flame::exceptions::invalid_argument("invalid queue id");
  }

  RouteMap::iterator lb = route_.lower_bound(type);
  if (lb != route_.end() && !(route_.key_comp()(type, lb->first))) {
    throw flame::exceptions::invalid_argument("type already assigned");
  } else {
    route_.insert(lb, RouteMap::value_type(type, qid));
  }
}

/*!
 * \brief Specity tasks that can be split
 * \param[in] type Task type that should be split
 *
 * Throws flame::exceptions::invalid_argument if type has not yet been assigned
 * to a queue.
 */
void Scheduler::SetSplittable(Task::TaskType type) {
  RouteMap::iterator iter = route_.find(type);
  if (iter == route_.end()) {
    throw flame::exceptions::invalid_argument("unassigned type");
  } else {
    queues_[iter->second].SetSplittable(type);
  }
}

/*!
 * \brief Specifies the maximum number of subtask a created per split
 * \param[in] type Task type
 * \param[in] max_tasks_per_split max tasks per split
 *
 * Throws flame::exceptions::invalid_argument if type has not yet been assigned
 * to a queue.
 */
void Scheduler::SetMaxTasksPerSplit(Task::TaskType type,
                                    size_t max_tasks_per_split) {
  RouteMap::iterator iter = route_.find(type);
  if (iter == route_.end()) {
    throw flame::exceptions::invalid_argument("unassigned type");
  } else {
    queues_[iter->second].SetMaxTasksPerSplit(max_tasks_per_split);
  }
}

/*!
 * \brief Returns the maximum number of subtask a created per split
 * \param[in] type Task type
 *
 * Throws flame::exceptions::invalid_argument if type has not yet been assigned
 * to a queue.
 */
size_t Scheduler::GetMaxTasksPerSplit(Task::TaskType type) const {
  RouteMap::const_iterator iter = route_.find(type);
  if (iter == route_.end()) {
    throw flame::exceptions::invalid_argument("unassigned type");
  } else {
    return queues_[iter->second].GetMaxTasksPerSplit();
  }
}

/*!
 * \brief Specifies the minimum vector size to maintain when splitting task
 * \param[in] type Task type
 * \param[in] min_vector_size min vector size
 *
 * Throws flame::exceptions::invalid_argument if type has not yet been assigned
 * to a queue.
 */
void Scheduler::SetMinVectorSize(Task::TaskType type, size_t min_vector_size) {
  RouteMap::iterator iter = route_.find(type);
  if (iter == route_.end()) {
    throw flame::exceptions::invalid_argument("unassigned type");
  } else {
    queues_[iter->second].SetMinVectorSize(min_vector_size);
  }
}

/*!
 * \brief Returns the minimum vector size to maintain when splitting task
 * \param[in] type Task type
 *
 * Throws flame::exceptions::invalid_argument if type has not yet been assigned
 * to a queue.
 */
size_t Scheduler::GetMinVectorSize(Task::TaskType type) const {
  RouteMap::const_iterator iter = route_.find(type);
  if (iter == route_.end()) {
    throw flame::exceptions::invalid_argument("unassigned type");
  } else {
    return queues_[iter->second].GetMinVectorSize();
  }
}

//! \brief Returns true if the given id is a valid queue id
bool Scheduler::IsValidQueueId(QueueId id) {
  return (id < queues_.size());
}

/*!
 * \brief Callback function used to indicate that a task is completed
 *
 * Assigned to each associated task queue for reverse communication
 */
void Scheduler::TaskDoneCallback(Task::id_type task_id) {
  boost::unique_lock<boost::mutex> lock(doneq_mutex_);
  doneq_.push_back(task_id);
  doneq_cond_.notify_one();
}

/*!
 * \brief Runs a single iteration
 *
 * Tasks are retrieved from the Task Manager and all ready tasks (no
 * dependencies) are enqueued so they can be processed by the queues.
 *
 * A call to this method is blocking and returns once all tasks have been
 * completed.
 */
void Scheduler::RunIteration() {
  TaskManager &tm = TaskManager::GetInstance();
  tm.Finalise();

  // inform IO manager of the current iteration count
  flame::io::IOManager &io = flame::io::IOManager::GetInstance();
  io.setIteration(iter_count_);

  // sanity check. avoid deadlock if no ready tasks
  if (!tm.IterTaskAvailable()) {
    throw flame::exceptions::flame_exe_exception("No runnable tasks");
  }

  while (!tm.IterCompleted()) {  // repeat till all tasks completed
    while (tm.IterTaskAvailable()) {  // schedule ready tasks
      EnqueueTask(tm.IterTaskPop());
    }

    { // deal with tasks that have been completed by workers
      boost::unique_lock<boost::mutex> lock(doneq_mutex_);
      if (doneq_.empty()) {
        doneq_cond_.wait(lock);
      }
      while (!doneq_.empty()) {
        tm.IterTaskDone(doneq_.back());
        doneq_.pop_back();
      }
    }  // lock freed on exiting block scope
  }

  tm.IterReset();  // prepare for next iteration
  ++iter_count_;  // increment iteration count (for next iter)
}

/*!
  * \brief Equeues task within the associated queue
  *
  * The queue is selected based on the task type and contents of
  * route_ map which is populated by AssignType().
  *
  * Throws flame::exceptions::invalid_type if a tasks with an unregistered
  * type is encountered.
  */
void Scheduler::EnqueueTask(Task::id_type task_id) {
  TaskManager &tm = TaskManager::GetInstance();
  Task& task = tm.GetTask(task_id);

  try {
    QueueId qid = route_.at(task.get_task_type());  // identify queue
    queues_.at(qid).Enqueue(task.get_task_id());
  } catch(const std::out_of_range& E) {
    throw flame::exceptions::invalid_type("unassigned task type");
  }
}

}}  // namespace flame::exe
