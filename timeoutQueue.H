/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file timeoutQueue.H
 * @brief Priority queue for scheduling timed events
 * 
 * This file provides a thread-safe priority queue for scheduling events
 * that should be triggered at specific absolute times. It uses a background
 * thread to monitor the queue and execute events when their trigger time
 * arrives.
 * 
 * ## Features
 * 
 * - **Priority-based scheduling**: Events ordered by trigger time using a binary heap
 * - **Thread-safe**: Mutex-protected operations with condition variable signaling
 * - **Background execution**: Dedicated thread monitors and triggers events
 * - **Event lifecycle**: Supports scheduling, cancellation, rescheduling, and deletion
 * - **Status tracking**: Events maintain execution status throughout their lifecycle
 * 
 * ## Event States
 * 
 * Events transition through these states:
 * - `Out_Queue`: Not currently scheduled
 * - `In_Queue`: Waiting for trigger time
 * - `Canceled`: Removed before execution
 * - `Executing`: Currently running EventFct()
 * - `Executed`: Completed execution
 * - `To_Delete`: Marked for cleanup
 * - `Deleted`: Cleaned up
 * 
 * ## Usage Example
 * 
 * ```cpp
 * class MyEvent : public TimeoutQueue::Event {
 * public:
 *     MyEvent(const Time& t) : Event(t) {}
 *     void EventFct() override {
 *         std::cout << "Event triggered!" << std::endl;
 *     }
 * };
 * 
 * TimeoutQueue queue;
 * 
 * // Schedule event 5 seconds from now
 * Time trigger_time = current_time();
 * trigger_time.tv_sec += 5;
 * 
 * MyEvent* event = new MyEvent(trigger_time);
 * queue.schedule_event(trigger_time, event);
 * 
 * // Later: cancel if needed
 * queue.cancel_event(event);
 * 
 * // Cleanup
 * queue.shutdown();
 * ```
 * 
 * ## Implementation Notes
 * 
 * - Uses BinHeapVtl for efficient O(log n) priority queue operations
 * - Background thread sleeps until next event or new scheduling
 * - Condition variable wakes thread when queue is modified
 * 
 * @see tpl_binHeap.H For the underlying priority queue
 * @see ah-time.H For time utilities
 * @see useMutex.H For threading utilities
 * 
 * @author Leandro Rabindranath León *
 *  @ingroup Utilidades

 */

# ifndef TIMEOUTQUEUE_H
# define TIMEOUTQUEUE_H

# include <unistd.h>
# include <pthread.h>
# include <tpl_binHeap.H>
# include <useMutex.H>
# include <ah-time.H>

/**
 * @class TimeoutQueue
 * @brief Thread-safe priority queue for scheduling timed events
 * 
 * Manages a collection of events that should be triggered at specific
 * absolute times. A background thread monitors the queue and executes
 * events when their scheduled time arrives.
 * 
 * ## Thread Safety
 * 
 * All public methods are thread-safe and can be called from any thread.
 * The background thread handles event execution. *
 *  @ingroup Utilidades

 */
class TimeoutQueue
{
public:

  /**
   * @class Event
   * @brief Base class for scheduled events
   * 
   * Derive from this class and override EventFct() to create
   * custom events that can be scheduled in the TimeoutQueue. *
 *  @ingroup Utilidades

   */
  class Event : private BinHeapVtl<Time>::Node
  {
    friend class TimeoutQueue;

  public:

    /**
     * @brief Possible states of an event in its lifecycle
 *
 *  @ingroup Utilidades
     */
    enum Execution_Status
      {
        Out_Queue,   ///< Not currently in any queue
        In_Queue,    ///< Scheduled and waiting for trigger time
        Canceled,    ///< Removed from queue before execution
        Executing,   ///< Currently executing EventFct()
        Executed,    ///< Completed execution
        To_Delete,   ///< Marked for cleanup
        Deleted      ///< Memory freed
      };

  private:

    Execution_Status execution_status;

    void set_trigger_time(Time _t)
    {
      assert(_t.tv_nsec >= 0 and _t.tv_nsec < NSEC);
      EVENT_TIME(this) = _t;
    }

    void set_execution_status(Execution_Status status)
    {
      execution_status = status;
    }

  protected:

    Event(const Time& t) :
      BinHeapVtl<Time>::Node(t), execution_status(Out_Queue)
    {
      assert(EVENT_NSEC(this) >= 0 and EVENT_NSEC(this) < NSEC);
    }

    Event(long sec, long nsec) : execution_status(Out_Queue)
    {
      assert(nsec >= 0 and nsec < NSEC);

      EVENT_SEC(this)  = sec;
      EVENT_NSEC(this) = nsec;
    }

  public:

    virtual ~Event()
    {
      if (execution_status == In_Queue)
        AH_ERROR("Attempting to delete an event in queue (0x%x)", this);
    }

    Execution_Status get_execution_status() const { return execution_status; }

    Time getAbsoluteTime() { return EVENT_TIME(this); }

    void set_for_deletion() { execution_status = To_Delete; }

    /**
     * @brief Event handler function to be overridden
     * 
     * This pure virtual function is called when the event's trigger
     * time arrives. Override this in derived classes to implement
     * custom event behavior. *
 *  @ingroup Utilidades

     */
    virtual void EventFct() = 0;
  }; /* end class Event */

private:

  BinHeapVtl<Time> prioQueue;

  pthread_mutex_t  mutex;
  pthread_cond_t   cond;
  pthread_t        threadId;

  bool             isShutdown;

  static int       instanceCounter;

  void *triggerEvent();

  static void* triggerEventThread(void*);

public:

  /// Default constructor - starts the background thread
  TimeoutQueue();

  /// Destructor - shuts down the queue and joins the thread
  ~TimeoutQueue();

  /**
   * @brief Schedule an event at a specific time
   * @param t Absolute trigger time
   * @param event Event to schedule *
 *  @ingroup Utilidades

   */
  void schedule_event(const Time &, Event*);

  /**
   * @brief Schedule an event using its internal trigger time
   * @param event Event to schedule (uses event's getAbsoluteTime()) *
 *  @ingroup Utilidades

   */
  void schedule_event(Event*);

  /**
   * @brief Cancel a scheduled event
   * @param event Event to cancel
   * @return true if event was in queue and canceled *
 *  @ingroup Utilidades

   */
  bool cancel_event(Event*);

  /**
   * @brief Reschedule an event to a new time
   * @param t New absolute trigger time
   * @param event Event to reschedule *
 *  @ingroup Utilidades

   */
  void reschedule_event(const Time &, Event*);

  /**
   * @brief Cancel and delete an event
   * @param event Pointer to event (set to nullptr after deletion) *
 *  @ingroup Utilidades

   */
  void cancel_delete_event(Event *&);

  /// Shut down the queue and stop the background thread
  void shutdown();

  /// Get the background thread ID
  const pthread_t & getThreadId() const { return threadId; }
};


# endif /* TIMEOUTQUEUE_H */

