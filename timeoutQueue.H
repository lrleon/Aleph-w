
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
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
 * 
 * @note Uses C++11 threading primitives (std::mutex, std::condition_variable, std::thread)
 * 
 * @author Leandro Rabindranath León *
 *  @ingroup Utilities

 */

# ifndef TIMEOUTQUEUE_H
# define TIMEOUTQUEUE_H

# include <unistd.h>
# include <mutex>
# include <condition_variable>
# include <thread>
# include <chrono>
# include <functional>
# include <string>
# include <atomic>
# include <cstdint>
# include <unordered_map>
# include <tpl_binHeap.H>
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
 *  @ingroup Utilities

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
 *  @ingroup Utilities

   */
  class Event : private BinHeapVtl<Time>::Node
  {
    friend class TimeoutQueue;

  public:

    /**
     * @brief Possible states of an event in its lifecycle
 *
 *  @ingroup Utilities
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

    /// Optional callback type for post-execution notification
    using CompletionCallback = std::function<void(Event*, Execution_Status)>;

    /// Type for unique event identifiers
    using EventId = uint64_t;

    /// Invalid/null event ID
    static constexpr EventId InvalidId = 0;

  private:

    static std::atomic<EventId> nextId;

    EventId event_id;
    Execution_Status execution_status;
    std::string event_name;
    CompletionCallback on_completed;

    void set_trigger_time(const Time _t)
    {
      assert(_t.tv_nsec >= 0 and _t.tv_nsec < NSEC);
      EVENT_TIME(this) = _t;
    }

    void set_execution_status(Execution_Status status)
    {
      execution_status = status;
    }

    void invoke_completion_callback()
    {
      if (on_completed)
        on_completed(this, execution_status);
    }

  protected:

    Event(const Time& t, const std::string& name = "") :
      BinHeapVtl<Time>::Node(t), event_id(++nextId), 
      execution_status(Out_Queue), event_name(name)
    {
      assert(EVENT_NSEC(this) >= 0 and EVENT_NSEC(this) < NSEC);
    }

    Event(const long sec, const long nsec, const std::string& name = "") 
      : event_id(++nextId), execution_status(Out_Queue), event_name(name)
    {
      assert(nsec >= 0 and nsec < NSEC);

      EVENT_SEC(this)  = sec;
      EVENT_NSEC(this) = nsec;
    }

  public:

    ~Event() override
    {
      ah_runtime_error_if(execution_status == In_Queue)
        << "Attempting to delete an event in queue (" << this << ")";
    }

    [[nodiscard]] Execution_Status get_execution_status() const { return execution_status; }

    [[nodiscard]] Time getAbsoluteTime() const { return EVENT_TIME(this); }

    void set_for_deletion() { execution_status = To_Delete; }

    /// Get the unique event ID (auto-generated on construction)
    [[nodiscard]] EventId get_id() const { return event_id; }

    /// Get the event name (for debugging)
    [[nodiscard]] const std::string& get_name() const { return event_name; }

    /// Set the event name
    void set_name(const std::string& name) { event_name = name; }

    /// Set completion callback (called after EventFct completes or event is canceled)
    void set_completion_callback(CompletionCallback cb) { on_completed = std::move(cb); }

    /**
     * @brief Event handler function to be overridden
     * 
     * This pure virtual function is called when the event's trigger
     * time arrives. Override this in derived classes to implement
     * custom event behavior. *
 *  @ingroup Utilities

     */
    virtual void EventFct() = 0;
  }; /* end class Event */

private:

  BinHeapVtl<Time> prioQueue;
  std::unordered_map<Event::EventId, Event*> eventMap;  // For O(1) lookup by ID

  mutable std::mutex       mtx;
  std::condition_variable  cond;
  std::thread              workerThread;

  bool                     isShutdown;
  bool                     isPaused = false;

  size_t                   executedCount = 0;
  size_t                   canceledCount = 0;

  std::condition_variable  emptyCondition;

  void triggerEvent();

public:

  /// Default constructor - starts the background thread
  TimeoutQueue();

  /// Destructor - shuts down the queue and joins the thread
  ~TimeoutQueue();

  /**
   * @brief Schedule an event at a specific time
   * @param trigger_time Absolute trigger time
   * @param event Event to schedule *
 *  @ingroup Utilities

   */
  void schedule_event(const Time &trigger_time, Event*);

  /**
   * @brief Schedule an event using its internal trigger time
   * @param event Event to schedule (uses event's getAbsoluteTime()) *
 *  @ingroup Utilities

   */
  void schedule_event(Event*);

  /**
   * @brief Cancel a scheduled event
   * @param event Event to cancel
   * @return true if event was in queue and canceled *
 *  @ingroup Utilities

   */
  bool cancel_event(Event*);

  /**
   * @brief Reschedule an event to a new time
   * @param trigger_time New absolute trigger time
   * @param event Event to reschedule *
 *  @ingroup Utilities

   */
  void reschedule_event(const Time &trigger_time, Event*);

  /**
   * @brief Cancel and delete an event
   * @param event Pointer to event (set to nullptr after deletion) *
 *  @ingroup Utilities

   */
  void cancel_delete_event(Event *&);

  /// Shut down the queue and stop the background thread
  void shutdown();

  /// Get the background thread ID
  [[nodiscard]] std::thread::id getThreadId() const { return workerThread.get_id(); }

  /// Get the number of pending events in the queue
  [[nodiscard]] size_t size() const;

  /// Check if the queue has no pending events
  [[nodiscard]] bool is_empty() const;

  /// Check if queue is running (not shut down)
  [[nodiscard]] bool is_running() const;

  /**
   * @brief Schedule an event relative to current time
   * @param ms_from_now Milliseconds from now to trigger
   * @param event Event to schedule
   */
  void schedule_after_ms(int ms_from_now, Event* event);

  /**
   * @brief Get the trigger time of the next (soonest) event
   * @return Time of next event, or {0,0} if queue is empty
   */
  [[nodiscard]] Time next_event_time() const;

  /**
   * @brief Cancel all pending events in the queue
   * @return Number of events that were canceled
   */
  size_t clear_all();

  /// Get count of events that have been executed
  [[nodiscard]] size_t executed_count() const;

  /// Get count of events that have been canceled
  [[nodiscard]] size_t canceled_count() const;

  /// Reset statistics counters to zero
  void reset_stats();

  /// Pause event execution (events remain scheduled but won't execute)
  void pause();

  /// Resume event execution after pause
  void resume();

  /// Check if queue is paused
  [[nodiscard]] bool is_paused() const;

  /**
   * @brief Block until all pending events have been executed or canceled
   * @param timeout_ms Maximum time to wait in milliseconds (0 = wait forever)
   * @return true if queue became empty, false if timeout
   */
  bool wait_until_empty(int timeout_ms = 0);

  /**
   * @brief Find a scheduled event by its ID
   * @param id The event ID to search for
   * @return Pointer to the event if found and still in queue, nullptr otherwise
   */
  [[nodiscard]] Event* find_by_id(Event::EventId id) const;

  /**
   * @brief Cancel a scheduled event by its ID
   * @param id The event ID to cancel
   * @return true if event was found and canceled, false if not found or not in queue
   */
  bool cancel_by_id(Event::EventId id);
};


# endif /* TIMEOUTQUEUE_H */

