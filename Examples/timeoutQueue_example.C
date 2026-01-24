/**
 * @file timeoutQueue_example.C
 * @brief Example demonstrating TimeoutQueue usage for timed event scheduling
 *
 * @doxygen
 * @brief TimeoutQueue - Priority-based timed event scheduler
 *
 * This example demonstrates:
 * - Basic event scheduling with absolute times
 * - Event cancellation and rescheduling
 * - Multiple events with different trigger times
 * - Event lifecycle management
 * - Shutdown behavior
 *
 * ## Key Concepts
 *
 * - **Event**: Derive from TimeoutQueue::Event and override EventFct()
 * - **Scheduling**: Events execute when their trigger time arrives
 * - **Thread-safe**: All operations are protected by mutex
 * - **Background thread**: Monitors queue and triggers events
 *
 * ## Compilation
 *
 * ```bash
 * g++ -std=gnu++20 -O2 -I.. -pthread -o timeoutQueue_example \
 *     timeoutQueue_example.C ../timeoutQueue.C ../useMutex.C -lpthread
 * ```
 *
 * ## Usage
 *
 * ```bash
 * ./timeoutQueue_example
 * ```
 *
 * @enddoxygen
 */

# include <iostream>
# include <iomanip>
# include <sstream>
# include <atomic>
# include <chrono>
# include <thread>
# include <timeoutQueue.H>

using namespace std;

// Helper to get current time plus milliseconds
static Time time_from_now_ms(int ms)
{
  return time_plus_msec(read_current_time(), ms);
}

// Helper to format time for display
static string format_time(const Time& t)
{
  ostringstream oss;
  oss << t.tv_sec << "." << setfill('0') << setw(9) << t.tv_nsec;
  return oss.str();
}

// Get current timestamp string for logging
static string now_str()
{
  auto now = chrono::system_clock::now();
  auto ms = chrono::duration_cast<chrono::milliseconds>(
    now.time_since_epoch()) % 1000;
  time_t tt = chrono::system_clock::to_time_t(now);
  tm local_tm;
  localtime_r(&tt, &local_tm);

  ostringstream oss;
  oss << setfill('0') << setw(2) << local_tm.tm_hour << ":"
      << setfill('0') << setw(2) << local_tm.tm_min << ":"
      << setfill('0') << setw(2) << local_tm.tm_sec << "."
      << setfill('0') << setw(3) << ms.count();
  return oss.str();
}

// =============================================================================
// Example 1: Simple Timed Event
// =============================================================================

class SimpleEvent : public TimeoutQueue::Event
{
  string message;

public:
  SimpleEvent(const Time& t, const string& msg)
    : Event(t), message(msg) {}

  void EventFct() override
  {
    cout << "[" << now_str() << "] SimpleEvent: " << message << endl;
  }
};

void demo_simple_event(TimeoutQueue& queue)
{
  cout << "\n=== Demo 1: Simple Timed Event ===" << endl;
  cout << "[" << now_str() << "] Scheduling event for 500ms from now..." << endl;

  auto* event = new SimpleEvent(time_from_now_ms(500), "Hello from timed event!");
  queue.schedule_event(event);

  // Wait for execution
  this_thread::sleep_for(chrono::milliseconds(700));
  delete event;
}

// =============================================================================
// Example 2: Multiple Events with Priority Ordering
// =============================================================================

class NumberedEvent : public TimeoutQueue::Event
{
  int number;
  atomic<bool>& executed_flag;

public:
  NumberedEvent(const Time& t, int num, atomic<bool>& flag)
    : Event(t), number(num), executed_flag(flag) {}

  void EventFct() override
  {
    cout << "[" << now_str() << "] Event #" << number << " executed" << endl;
    executed_flag = true;
  }
};

void demo_priority_ordering(TimeoutQueue& queue)
{
  cout << "\n=== Demo 2: Multiple Events Execute in Time Order ===" << endl;

  atomic<bool> flags[4] = {false, false, false, false};

  // Schedule events in non-chronological order
  cout << "[" << now_str() << "] Scheduling events out of order..." << endl;

  auto* e3 = new NumberedEvent(time_from_now_ms(300), 3, flags[2]);
  auto* e1 = new NumberedEvent(time_from_now_ms(100), 1, flags[0]);
  auto* e4 = new NumberedEvent(time_from_now_ms(400), 4, flags[3]);
  auto* e2 = new NumberedEvent(time_from_now_ms(200), 2, flags[1]);

  queue.schedule_event(e3);
  queue.schedule_event(e1);
  queue.schedule_event(e4);
  queue.schedule_event(e2);

  cout << "  Events scheduled: #3 @300ms, #1 @100ms, #4 @400ms, #2 @200ms" << endl;
  cout << "  Expected order: 1, 2, 3, 4" << endl;

  // Wait for all events
  this_thread::sleep_for(chrono::milliseconds(600));

  delete e1;
  delete e2;
  delete e3;
  delete e4;
}

// =============================================================================
// Example 3: Event Cancellation
// =============================================================================

class CancellableEvent : public TimeoutQueue::Event
{
  string name;

public:
  CancellableEvent(const Time& t, const string& n)
    : Event(t), name(n) {}

  void EventFct() override
  {
    cout << "[" << now_str() << "] CancellableEvent '" << name << "' executed" << endl;
  }

  const string& get_name() const { return name; }
};

void demo_cancellation(TimeoutQueue& queue)
{
  cout << "\n=== Demo 3: Event Cancellation ===" << endl;

  auto* keep_event = new CancellableEvent(time_from_now_ms(200), "KEEP");
  auto* cancel_event = new CancellableEvent(time_from_now_ms(300), "CANCEL");

  queue.schedule_event(keep_event);
  queue.schedule_event(cancel_event);

  cout << "[" << now_str() << "] Scheduled 'KEEP' @200ms and 'CANCEL' @300ms" << endl;

  // Cancel one event
  this_thread::sleep_for(chrono::milliseconds(50));
  bool canceled = queue.cancel_event(cancel_event);
  cout << "[" << now_str() << "] Canceled 'CANCEL' event: "
       << (canceled ? "success" : "failed") << endl;
  cout << "  Event status: "
       << (cancel_event->get_execution_status() == TimeoutQueue::Event::Canceled
           ? "Canceled" : "Other") << endl;

  // Wait and observe
  this_thread::sleep_for(chrono::milliseconds(400));
  cout << "  Only 'KEEP' should have executed above." << endl;

  delete keep_event;
  delete cancel_event;
}

// =============================================================================
// Example 4: Event Rescheduling
// =============================================================================

class ReschedulableEvent : public TimeoutQueue::Event
{
  string name;
  int execution_count = 0;

public:
  ReschedulableEvent(const Time& t, const string& n)
    : Event(t), name(n) {}

  void EventFct() override
  {
    ++execution_count;
    cout << "[" << now_str() << "] ReschedulableEvent '" << name
         << "' executed (count: " << execution_count << ")" << endl;
  }

  int get_count() const { return execution_count; }
};

void demo_rescheduling(TimeoutQueue& queue)
{
  cout << "\n=== Demo 4: Event Rescheduling ===" << endl;

  auto* event = new ReschedulableEvent(time_from_now_ms(500), "Rescheduled");

  cout << "[" << now_str() << "] Originally scheduled for 500ms" << endl;
  queue.schedule_event(event);

  // Reschedule to earlier time
  this_thread::sleep_for(chrono::milliseconds(100));
  cout << "[" << now_str() << "] Rescheduling to 100ms from now (200ms total)" << endl;
  queue.reschedule_event(time_from_now_ms(100), event);

  this_thread::sleep_for(chrono::milliseconds(300));
  cout << "  Event executed early due to rescheduling." << endl;

  delete event;
}

// =============================================================================
// Example 5: Self-Rescheduling Periodic Event
// =============================================================================

class PeriodicEvent : public TimeoutQueue::Event
{
  TimeoutQueue* queue;
  string name;
  int interval_ms;
  int remaining_executions;

public:
  PeriodicEvent(const Time& t, TimeoutQueue* q, const string& n,
                int interval, int max_exec)
    : Event(t), queue(q), name(n), interval_ms(interval),
      remaining_executions(max_exec) {}

  void EventFct() override
  {
    cout << "[" << now_str() << "] PeriodicEvent '" << name
         << "' tick (remaining: " << remaining_executions - 1 << ")" << endl;

    --remaining_executions;
    if (remaining_executions > 0)
      queue->reschedule_event(time_from_now_ms(interval_ms), this);
  }

  bool is_done() const { return remaining_executions <= 0; }
};

void demo_periodic_event(TimeoutQueue& queue)
{
  cout << "\n=== Demo 5: Self-Rescheduling Periodic Event ===" << endl;

  auto* periodic = new PeriodicEvent(
    time_from_now_ms(100), &queue, "Heartbeat", 150, 4);

  cout << "[" << now_str() << "] Starting periodic event (4 ticks, 150ms interval)" << endl;
  queue.schedule_event(periodic);

  // Wait for all executions: 100 + 3*150 = 550ms + buffer
  this_thread::sleep_for(chrono::milliseconds(800));

  delete periodic;
}

// =============================================================================
// Example 6: Event with Callback
// =============================================================================

class CallbackEvent : public TimeoutQueue::Event
{
  function<void()> callback;

public:
  CallbackEvent(const Time& t, function<void()> cb)
    : Event(t), callback(move(cb)) {}

  void EventFct() override
  {
    if (callback)
      callback();
  }
};

void demo_callback_event(TimeoutQueue& queue)
{
  cout << "\n=== Demo 6: Event with Lambda Callback ===" << endl;

  int counter = 0;

  auto* event1 = new CallbackEvent(time_from_now_ms(100), [&counter]() {
    ++counter;
    cout << "[" << now_str() << "] Lambda callback 1, counter = " << counter << endl;
  });

  auto* event2 = new CallbackEvent(time_from_now_ms(200), [&counter]() {
    ++counter;
    cout << "[" << now_str() << "] Lambda callback 2, counter = " << counter << endl;
  });

  queue.schedule_event(event1);
  queue.schedule_event(event2);

  this_thread::sleep_for(chrono::milliseconds(400));

  cout << "  Final counter value: " << counter << endl;

  delete event1;
  delete event2;
}

// =============================================================================
// Example 7: Event Status Inspection
// =============================================================================

void print_status(const TimeoutQueue::Event* e, const string& name)
{
  const char* status_names[] = {
    "Out_Queue", "In_Queue", "Canceled", "Executing", "Executed", "To_Delete", "Deleted"
  };
  cout << "  " << name << " status: "
       << status_names[static_cast<int>(e->get_execution_status())] << endl;
}

void demo_event_status(TimeoutQueue& queue)
{
  cout << "\n=== Demo 7: Event Lifecycle Status ===" << endl;

  auto* event = new SimpleEvent(time_from_now_ms(200), "Status demo");

  cout << "[" << now_str() << "] Event created:" << endl;
  print_status(event, "event");

  queue.schedule_event(event);
  cout << "[" << now_str() << "] After scheduling:" << endl;
  print_status(event, "event");

  this_thread::sleep_for(chrono::milliseconds(300));
  cout << "[" << now_str() << "] After execution:" << endl;
  print_status(event, "event");

  delete event;
}

// =============================================================================
// Example 8: Cancel and Delete
// =============================================================================

void demo_cancel_delete(TimeoutQueue& queue)
{
  cout << "\n=== Demo 8: Cancel and Delete Event ===" << endl;

  TimeoutQueue::Event* event = new SimpleEvent(time_from_now_ms(500), "To be deleted");

  cout << "[" << now_str() << "] Event pointer before: " << event << endl;
  queue.schedule_event(event);

  this_thread::sleep_for(chrono::milliseconds(100));
  queue.cancel_delete_event(event);

  cout << "[" << now_str() << "] Event pointer after cancel_delete: " << event << endl;
  cout << "  (Should be nullptr - memory freed by cancel_delete_event)" << endl;
}

// =============================================================================
// Main
// =============================================================================

int main()
{
  cout << "========================================" << endl;
  cout << "  TimeoutQueue Example Demonstrations  " << endl;
  cout << "========================================" << endl;

  TimeoutQueue queue;

  try
    {
      demo_simple_event(queue);
      demo_priority_ordering(queue);
      demo_cancellation(queue);
      demo_rescheduling(queue);
      demo_periodic_event(queue);
      demo_callback_event(queue);
      demo_event_status(queue);
      demo_cancel_delete(queue);

      cout << "\n=== Shutdown ===" << endl;
      cout << "[" << now_str() << "] Calling queue.shutdown()..." << endl;
      queue.shutdown();

      // Wait for background thread to finish
      this_thread::sleep_for(chrono::milliseconds(100));
      cout << "[" << now_str() << "] Queue shut down successfully." << endl;
    }
  catch (const exception& e)
    {
      cerr << "Error: " << e.what() << endl;
      queue.shutdown();
      return 1;
    }

  cout << "\n========================================" << endl;
  cout << "  All demonstrations completed!        " << endl;
  cout << "========================================" << endl;

  return 0;
}
