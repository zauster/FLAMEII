/*!
 * \file flame2/exe/fifo_task_queue.hpp
 * \author Shawn Chin
 * \date 2012
 * \copyright Copyright (c) 2012 STFC Rutherford Appleton Laboratory
 * \copyright Copyright (c) 2012 University of Sheffield
 * \copyright GNU Lesser General Public License
 * \brief Basic Task Queue that uses FIFO scheduling
 */
#ifndef EXE__FIFO_TASK_QUEUE_HPP_
#define EXE__FIFO_TASK_QUEUE_HPP_
#include <queue>
#include "worker_thread.hpp"
#include "task_queue_interface.hpp"

namespace flame { namespace exe {

class FIFOTaskQueue : public TaskQueue {
  public:
    typedef boost::ptr_vector<WorkerThread> WorkerVector;

    /*!
     * \brief Constructor
     * \param[in] slots Number of slots
     *
     * Populates the vector of worker threads and initialses the threads.
     *
     * Throws flame::exceptions::invalid_argument if an invalid value is given
     * for slots.
     */
    explicit FIFOTaskQueue(size_t slots);
    /*!
     * \brief Destructor
     *
     * Enqueue the termination task (signal worker threads to wrap up) and waits
     * for all worker threads to complete before destroying this object.
     */
    ~FIFOTaskQueue();

    /*!
     * \brief Adds a task to the queue
     *
     * This method is meant to be called by the Scheduler
     */
    void Enqueue(Task::id_type task_id);

    /*!
     * \brief Process a completed task
     *
     * This method is meant to be called by a worker thread.
     *
     * This triggers the callback function of the parent scheduler.
     */
    void TaskDone(Task::id_type task_id);

    //! \brief Specify tasks than can be split (not applicable)
    void SetSplittable(Task::TaskType /*task_type*/) {
      throw flame::exceptions::not_implemented("Non-splitting queue");
    }

    //! \brief Specify maximum splits per task (not applicable)
    void SetMaxTasksPerSplit(size_t /*max_tasks_per_split*/) {
      throw flame::exceptions::not_implemented("Non-splitting queue");
    }

    //! \brief Returns maximum splits per task (not applicable)
    size_t GetMaxTasksPerSplit(void) const {
      throw flame::exceptions::not_implemented("Non-splitting queue");
    }

    //! \brief Specify minimum vector size after split (not applicable)
    void SetMinVectorSize(size_t /*min_vector_size*/) {
      throw flame::exceptions::not_implemented("Non-splitting queue");
    }

    //! \brief Returns minimum vector size after split (not applicable)
    size_t GetMinVectorSize(void) const {
      throw flame::exceptions::not_implemented("Non-splitting queue");
    }

    /*!
     * \brief Returns the next available task.
     *
     * If there are none available, the calling thread will be blocked
     *
     * This method is meant to be called by a Worker Thread
     */
    Task::id_type GetNextTask();

    //! \brief Returns true if the queue is empty
    bool empty() const;

  protected:
    size_t slots_;  //! Number of processing slots (worker threads)
    WorkerVector workers_;  //! Collection of worker threads

  private:
    std::queue<Task::id_type> queue_;  //! FIFO task queue
};

}}  // namespace flame::exe
#endif  // EXE__FIFO_TASK_QUEUE_HPP_