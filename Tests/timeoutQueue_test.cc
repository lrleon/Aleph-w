/**
 * @file timeoutQueue_test.cc
 * @brief Comprehensive tests for TimeoutQueue class
 *
 * Note: Updated to reflect header Doxygen group changes.
 *
 * Note: TimeoutQueue is a singleton (only one instance allowed).
 * All tests share a single global queue instance.
 */

# include <gtest/gtest.h>
# include <stdexcept>
# include <atomic>
# include <chrono>
# include <thread>
# include <vector>
# include <mutex>
# include <condition_variable>
# include <timeoutQueue.H>

using namespace std;

// Global queue (singleton)
static TimeoutQueue* g_queue = nullptr;

// Helper to get the current time plus milliseconds
static Time time_from_now_ms(int ms)
{
  return time_plus_msec(read_current_time(), ms);
}

/**
 * @brief Wait until an event reaches an expected lifecycle state.
 * @param event Event whose state is observed.
 * @param expected Expected lifecycle state.
 * @param timeout Maximum amount of time to wait.
 * @return true if the expected state was observed before the timeout.
 */
static bool wait_for_status(
  const TimeoutQueue::Event* event,
  const TimeoutQueue::Event::Execution_Status expected,
  const chrono::milliseconds timeout)
{
  const auto deadline = chrono::steady_clock::now() + timeout;
  while (chrono::steady_clock::now() < deadline)
    {
      if (event->get_execution_status() == expected)
        return true;
      this_thread::sleep_for(chrono::milliseconds(10));
    }
  return event->get_execution_status() == expected;
}

/**
 * @brief Spin-wait until a predicate becomes true or a timeout elapses.
 *
 * Replaces fixed @c sleep_for() delays used as ad-hoc synchronization with the
 * background worker thread. Those delays are unreliable when the worker is
 * starved of CPU (e.g. on heavily loaded CI runners): the test wakes up before
 * the event completed and then reads stale state or deletes an event the worker
 * still references. This helper returns as soon as the predicate holds, keeping
 * the common case fast while tolerating arbitrarily long scheduling delays.
 *
 * @param pred Callable returning bool; evaluated repeatedly until true.
 * @param timeout Maximum amount of time to wait.
 * @return true if the predicate held before the timeout, false otherwise.
 */
template <typename Pred>
static bool wait_until(Pred pred, const chrono::milliseconds timeout)
{
  const auto deadline = chrono::steady_clock::now() + timeout;
  while (chrono::steady_clock::now() < deadline)
    {
      if (pred())
        return true;
      this_thread::sleep_for(chrono::milliseconds(2));
    }
  return pred();
}

// Test event that tracks execution
class TestEvent : public TimeoutQueue::Event
{
public:
  atomic<bool> executed{false};
  atomic<int> execution_count{0};
  function<void()> callback;

  TestEvent(const Time& t) : Event(t) {}
  TestEvent(const Time& t, function<void()> cb) : Event(t), callback(move(cb)) {}

  void EventFct() override
  {
    executed = true;
    ++execution_count;
    if (callback) callback();
  }
};

// Test event that signals a condition variable
class SignalingEvent : public TimeoutQueue::Event
{
public:
  mutex& mtx;
  condition_variable& cv;
  bool& flag;

  SignalingEvent(const Time& t, mutex& m, condition_variable& c, bool& f)
    : Event(t), mtx(m), cv(c), flag(f) {}

  void EventFct() override
  {
    lock_guard<mutex> lock(mtx);
    flag = true;
    cv.notify_all();
  }
};

// Test event that records execution time
class TimingEvent : public TimeoutQueue::Event
{
public:
  chrono::steady_clock::time_point scheduled_at;
  chrono::steady_clock::time_point executed_at;
  atomic<bool> executed{false};

  TimingEvent(const Time& t) : Event(t), scheduled_at(chrono::steady_clock::now()) {}

  void EventFct() override
  {
    executed_at = chrono::steady_clock::now();
    executed = true;
  }

  [[nodiscard]] int elapsed_ms() const
  {
    return chrono::duration_cast<chrono::milliseconds>(
      executed_at - scheduled_at).count();
  }
};

// Test event that can reschedule itself
class ReschedulingEvent : public TimeoutQueue::Event
{
public:
  TimeoutQueue* queue;
  int reschedule_count;
  int max_reschedules;
  int reschedule_step_ms;
  atomic<int> execution_count{0};

  ReschedulingEvent(const Time& t, TimeoutQueue* q, int max_resc,
                    int step_ms = 50)
    : Event(t), queue(q), reschedule_count(0), max_reschedules(max_resc),
      reschedule_step_ms(step_ms) {}

  void EventFct() override
  {
    ++execution_count;
    if (reschedule_count < max_reschedules)
      {
        ++reschedule_count;
        queue->reschedule_event(time_from_now_ms(reschedule_step_ms), this);
      }
  }
};

// =============================================================================
// Test Environment - manages the singleton TimeoutQueue
// =============================================================================

class TimeoutQueueEnvironment : public ::testing::Environment
{
public:
  void SetUp() override
  {
    g_queue = new TimeoutQueue();
  }

  void TearDown() override
  {
    if (g_queue)
      {
        g_queue->shutdown();
        this_thread::sleep_for(chrono::milliseconds(100));
        delete g_queue;
        g_queue = nullptr;
      }
  }
};

// =============================================================================
// Basic Functionality Tests
// =============================================================================

TEST(TimeoutQueueTest, ScheduleAndExecuteSingleEvent)
{
  mutex mtx;
  condition_variable cv;
  bool executed = false;

  bool completed = false;
  bool callback_done = false;

  auto* event = new SignalingEvent(time_from_now_ms(100), mtx, cv, executed);
  event->set_completion_callback([&](TimeoutQueue::Event*, TimeoutQueue::Event::Execution_Status status) {
    lock_guard<mutex> lock(mtx);
    completed = (status == TimeoutQueue::Event::Executed);
    callback_done = true;
    cv.notify_all();
  });
  g_queue->schedule_event(event);

  unique_lock<mutex> lock(mtx);
  ASSERT_TRUE(cv.wait_for(lock, chrono::milliseconds(500),
                          [&]{ return callback_done; }));
  EXPECT_TRUE(executed);
  EXPECT_TRUE(completed);

  delete event;
}

TEST(TimeoutQueueTest, EventExecutionStatus)
{
  const auto completion_timeout = chrono::seconds(30);

  auto* event = new TestEvent(time_from_now_ms(50));

  EXPECT_EQ(event->get_execution_status(), TimeoutQueue::Event::Out_Queue);

  g_queue->schedule_event(event);
  EXPECT_EQ(event->get_execution_status(), TimeoutQueue::Event::In_Queue);

  // Wait on the lifecycle state instead of a fixed sleep. A fixed delay is
  // unreliable on heavily loaded CI runners where the worker thread may not run
  // within the window, leaving the event unexecuted (and turning the following
  // delete into a use-after-free). wait_for_status returns as soon as the event
  // completes and only blocks longer when scheduling is genuinely delayed. Once
  // the status is Executed the worker no longer references the event (no
  // completion callback is set), so deleting it afterwards is safe.
  const bool executed =
    wait_for_status(event, TimeoutQueue::Event::Executed, completion_timeout);
  if (not executed)
    {
      // The worker never ran the event: it is still In_Queue (or Executing).
      // Deleting it directly would abort, so hand it back to the queue for a
      // safe teardown before failing.
      const auto status = event->get_execution_status();
      TimeoutQueue::Event* pending = event;
      try
        {
          g_queue->cancel_delete_event(pending);
        }
      catch (const invalid_argument&)
        {
          // The worker executed the event between the timeout check and this
          // call, so it is no longer in the registry; delete it ourselves.
          delete event;
        }
      ADD_FAILURE() << "event did not reach Executed before timeout; "
                    << "last status was " << status;
      return;
    }

  EXPECT_TRUE(event->executed);

  delete event;
}

TEST(TimeoutQueueTest, ScheduleWithExplicitTime)
{
  const auto completion_timeout = chrono::seconds(30);

  auto* event = new TestEvent(time_from_now_ms(1000)); // Will be overridden
  Time trigger_time = time_from_now_ms(50);

  g_queue->schedule_event(trigger_time, event);

  // Wait on the lifecycle state rather than a fixed sleep: the worker thread may
  // not run within a fixed window on a loaded CI runner, which would both fail
  // the assertion and turn the delete below into a use-after-free.
  const bool executed =
    wait_for_status(event, TimeoutQueue::Event::Executed, completion_timeout);
  if (not executed)
    {
      const auto status = event->get_execution_status();
      TimeoutQueue::Event* pending = event;
      try
        {
          g_queue->cancel_delete_event(pending);
        }
      catch (const invalid_argument&)
        {
          delete event;
        }
      ADD_FAILURE() << "event did not reach Executed before timeout; "
                    << "last status was " << status;
      return;
    }

  EXPECT_TRUE(event->executed);

  delete event;
}

TEST(TimeoutQueueTest, GetAbsoluteTime)
{
  Time t = time_from_now_ms(100);
  auto* event = new TestEvent(t);

  Time event_time = event->getAbsoluteTime();
  EXPECT_EQ(event_time.tv_sec, t.tv_sec);
  EXPECT_EQ(event_time.tv_nsec, t.tv_nsec);

  delete event;
}

// =============================================================================
// Cancellation Tests
// =============================================================================

TEST(TimeoutQueueTest, CancelEventBeforeExecution)
{
  auto* event = new TestEvent(time_from_now_ms(500));

  g_queue->schedule_event(event);
  EXPECT_EQ(event->get_execution_status(), TimeoutQueue::Event::In_Queue);

  bool canceled = g_queue->cancel_event(event);
  EXPECT_TRUE(canceled);
  EXPECT_EQ(event->get_execution_status(), TimeoutQueue::Event::Canceled);

  this_thread::sleep_for(chrono::milliseconds(100));
  EXPECT_FALSE(event->executed);

  delete event;
}

TEST(TimeoutQueueTest, CancelEventNotInQueue)
{
  auto* event = new TestEvent(time_from_now_ms(100));

  // Now returns false if event is not in registry
  EXPECT_FALSE(g_queue->cancel_event(event));

  delete event;
}

TEST(TimeoutQueueTest, CancelDeleteEvent)
{
  TimeoutQueue::Event* event = new TestEvent(time_from_now_ms(500));

  g_queue->schedule_event(event);
  g_queue->cancel_delete_event(event);

  EXPECT_EQ(event, nullptr);
}

TEST(TimeoutQueueTest, CancelDeleteNullEvent)
{
  TimeoutQueue::Event* event = nullptr;
  EXPECT_NO_THROW(g_queue->cancel_delete_event(event));
  EXPECT_EQ(event, nullptr);
}

// =============================================================================
// Rescheduling Tests
// =============================================================================

TEST(TimeoutQueueTest, RescheduleEvent)
{
  const int original_delay_ms = 60000;
  const int rescheduled_delay_ms = 200;
  const auto completion_timeout = chrono::seconds(30);

  auto* event = new TimingEvent(time_from_now_ms(original_delay_ms));
  g_queue->schedule_event(event);
  this_thread::sleep_for(chrono::milliseconds(100));

  g_queue->reschedule_event(time_from_now_ms(rescheduled_delay_ms), event);

  const bool completed = wait_for_status(
    event, TimeoutQueue::Event::Executed, completion_timeout);
  if (not completed)
    {
      const auto status = event->get_execution_status();
      TimeoutQueue::Event* pending_event = event;
      try
        {
          g_queue->cancel_delete_event(pending_event);
        }
      catch (const invalid_argument&)
        {
          if (event->get_execution_status() != TimeoutQueue::Event::Executed)
            throw;
          delete event;
        }
      ADD_FAILURE() << "rescheduled event did not complete before timeout; "
                    << "last status was " << status;
      return;
    }

  EXPECT_TRUE(event->executed);
  EXPECT_LT(event->elapsed_ms(), original_delay_ms);

  delete event;
}

TEST(TimeoutQueueTest, RescheduleNotInQueue)
{
  auto* event = new TestEvent(time_from_now_ms(100));

  // Now throws exception if event is not in registry
  EXPECT_THROW(g_queue->reschedule_event(time_from_now_ms(50), event), std::invalid_argument);

  delete event;
}

TEST(TimeoutQueueTest, SelfReschedulingEvent)
{
  // Pasos de 150 ms y espera total de 1000 ms para absorber jitter en
  // runners de CI virtualizados (macOS arm64 Debug en particular).
  // El test sigue verificando la lógica de auto-reprogramación: el evento
  // debe ejecutarse exactamente max_reschedules+1 = 3 veces.
  const int step_ms = 150;
  const int max_reschedules = 2;
  auto* event = new ReschedulingEvent(time_from_now_ms(step_ms), g_queue,
                                      max_reschedules, step_ms);

  g_queue->schedule_event(event);

  this_thread::sleep_for(chrono::milliseconds(1000));
  EXPECT_EQ(event->execution_count, max_reschedules + 1);

  delete event;
}

// =============================================================================
// Multiple Events Tests
// =============================================================================

TEST(TimeoutQueueTest, MultipleEventsExecuteInOrder)
{
  vector<int> execution_order;
  mutex order_mutex;

  auto make_event = [&](int id, int delay_ms) {
    return new TestEvent(time_from_now_ms(delay_ms), [&, id]() {
      lock_guard<mutex> lock(order_mutex);
      execution_order.push_back(id);
    });
  };

  auto* e1 = make_event(1, 150);
  auto* e2 = make_event(2, 50);
  auto* e3 = make_event(3, 100);

  g_queue->schedule_event(e1);
  g_queue->schedule_event(e2);
  g_queue->schedule_event(e3);

  this_thread::sleep_for(chrono::milliseconds(400));

  {
    lock_guard<mutex> lock(order_mutex);
    ASSERT_EQ(execution_order.size(), 3u);
    EXPECT_EQ(execution_order[0], 2);
    EXPECT_EQ(execution_order[1], 3);
    EXPECT_EQ(execution_order[2], 1);
  }

  delete e1;
  delete e2;
  delete e3;
}

TEST(TimeoutQueueTest, ManyEventsStressTest)
{
  const int num_events = 30;
  vector<TestEvent*> events;
  atomic<int> total_executed{0};

  for (int i = 0; i < num_events; ++i)
    {
      auto* e = new TestEvent(time_from_now_ms(50 + i * 10), [&]() {
        ++total_executed;
      });
      events.push_back(e);
      g_queue->schedule_event(e);
    }

  this_thread::sleep_for(chrono::milliseconds(600));

  EXPECT_EQ(total_executed, num_events);

  for (auto* e : events)
    delete e;
}

// =============================================================================
// Edge Cases and Error Handling
// =============================================================================

TEST(TimeoutQueueTest, EventWithImmediateTime)
{
  Time now = read_current_time();
  auto* event = new TestEvent(now);

  g_queue->schedule_event(event);

  this_thread::sleep_for(chrono::milliseconds(100));
  EXPECT_TRUE(event->executed);

  delete event;
}

TEST(TimeoutQueueTest, EventThrowsException)
{
  auto* throwing_event = new TestEvent(time_from_now_ms(50), []() {
    throw runtime_error("Test exception");
  });

  auto* normal_event = new TestEvent(time_from_now_ms(100));

  g_queue->schedule_event(throwing_event);
  g_queue->schedule_event(normal_event);

  this_thread::sleep_for(chrono::milliseconds(300));

  EXPECT_TRUE(throwing_event->executed);
  EXPECT_TRUE(normal_event->executed);

  delete throwing_event;
  delete normal_event;
}

TEST(TimeoutQueueTest, SetForDeletion)
{
  auto* event = new TestEvent(time_from_now_ms(100));

  event->set_for_deletion();
  EXPECT_EQ(event->get_execution_status(), TimeoutQueue::Event::To_Delete);

  delete event;
}

// =============================================================================
// Timing Accuracy Tests
// =============================================================================

TEST(TimeoutQueueTest, TimingAccuracy)
{
  // Tolerancias asimétricas: la queue no debe disparar mucho antes de tiempo
  // (límite inferior estricto), pero en VMs de CI el jitter del scheduler
  // puede añadir varios cientos de ms al disparo (límite superior generoso).
  const int expected_delay = 200;
  const int lower_tolerance = 30;
  const int upper_tolerance = 250;

  auto* event = new TimingEvent(time_from_now_ms(expected_delay));

  g_queue->schedule_event(event);

  this_thread::sleep_for(
    chrono::milliseconds(expected_delay + upper_tolerance + 100));
  ASSERT_TRUE(event->executed);

  int actual_delay = event->elapsed_ms();
  EXPECT_GE(actual_delay, expected_delay - lower_tolerance);
  EXPECT_LE(actual_delay, expected_delay + upper_tolerance);

  delete event;
}

// =============================================================================
// Thread Safety Tests
// =============================================================================

TEST(TimeoutQueueTest, ConcurrentScheduling)
{
  atomic<int> executed_count{0};
  const int num_threads = 4;
  const int events_per_thread = 5;
  vector<thread> threads;
  vector<TestEvent*> all_events;
  mutex events_mutex;

  for (int t = 0; t < num_threads; ++t)
    {
      threads.emplace_back([&, t]() {
        for (int i = 0; i < events_per_thread; ++i)
          {
            auto* e = new TestEvent(time_from_now_ms(50 + i * 20), [&]() {
              ++executed_count;
            });
            {
              lock_guard<mutex> lock(events_mutex);
              all_events.push_back(e);
            }
            g_queue->schedule_event(e);
          }
      });
    }

  for (auto& t : threads)
    t.join();

  this_thread::sleep_for(chrono::milliseconds(400));

  EXPECT_EQ(executed_count, num_threads * events_per_thread);

  for (auto* e : all_events)
    delete e;
}

TEST(TimeoutQueueTest, ConcurrentCancellation)
{
  const int num_events = 10;
  vector<TestEvent*> events;
  atomic<int> canceled_count{0};

  for (int i = 0; i < num_events; ++i)
    {
      auto* e = new TestEvent(time_from_now_ms(500));
      events.push_back(e);
      g_queue->schedule_event(e);
    }

  vector<thread> threads;
  for (int t = 0; t < 2; ++t)
    {
      threads.emplace_back([&, t]() {
        for (size_t i = t; i < events.size(); i += 2)
          {
            try {
              if (g_queue->cancel_event(events[i]))
                ++canceled_count;
            } catch (const std::invalid_argument&) {
              // Ignore if already canceled/deleted
            }
          }
      });
    }

  for (auto& t : threads)
    t.join();

  EXPECT_EQ(canceled_count, num_events);

  for (auto* e : events)
    delete e;
}

// =============================================================================
// Utility Methods Tests (new features)
// =============================================================================

TEST(TimeoutQueueTest, SizeAndIsEmpty)
{
  EXPECT_TRUE(g_queue->is_empty());
  EXPECT_EQ(g_queue->size(), 0u);

  auto* e1 = new TestEvent(time_from_now_ms(500));
  auto* e2 = new TestEvent(time_from_now_ms(600));

  g_queue->schedule_event(e1);
  EXPECT_FALSE(g_queue->is_empty());
  EXPECT_EQ(g_queue->size(), 1u);

  g_queue->schedule_event(e2);
  EXPECT_EQ(g_queue->size(), 2u);

  g_queue->cancel_event(e1);
  EXPECT_EQ(g_queue->size(), 1u);

  g_queue->cancel_event(e2);
  EXPECT_TRUE(g_queue->is_empty());

  delete e1;
  delete e2;
}

TEST(TimeoutQueueTest, IsRunning)
{
  EXPECT_TRUE(g_queue->is_running());
}

TEST(TimeoutQueueTest, ScheduleAfterMs)
{
  // Tolerancias asimétricas para acomodar el jitter del scheduler en
  // runners de CI virtualizados (en particular macOS Debug). El test
  // sigue verificando que el evento dispara cerca del delay pedido, no
  // sólo que "eventualmente se ejecuta".
  const int expected_delay = 100;
  const int lower_tolerance = 20;
  const int upper_tolerance = 250;

  auto* event = new TimingEvent(time_from_now_ms(1000)); // Will be overridden

  g_queue->schedule_after_ms(expected_delay, event);

  this_thread::sleep_for(
    chrono::milliseconds(expected_delay + upper_tolerance + 100));
  ASSERT_TRUE(event->executed);

  const int actual_delay = event->elapsed_ms();
  EXPECT_GE(actual_delay, expected_delay - lower_tolerance);
  EXPECT_LE(actual_delay, expected_delay + upper_tolerance);

  delete event;
}

TEST(TimeoutQueueTest, NextEventTime)
{
  Time empty_time = g_queue->next_event_time();
  EXPECT_EQ(empty_time.tv_sec, 0);
  EXPECT_EQ(empty_time.tv_nsec, 0);

  auto* e1 = new TestEvent(time_from_now_ms(500));
  auto* e2 = new TestEvent(time_from_now_ms(200));

  g_queue->schedule_event(e1);
  Time t1 = g_queue->next_event_time();
  EXPECT_EQ(t1.tv_sec, e1->getAbsoluteTime().tv_sec);

  g_queue->schedule_event(e2);
  Time t2 = g_queue->next_event_time();
  EXPECT_EQ(t2.tv_sec, e2->getAbsoluteTime().tv_sec); // e2 is sooner

  g_queue->cancel_event(e1);
  g_queue->cancel_event(e2);

  delete e1;
  delete e2;
}

TEST(TimeoutQueueTest, ExecutedStatus)
{
  auto* event = new TestEvent(time_from_now_ms(50));

  g_queue->schedule_event(event);
  this_thread::sleep_for(chrono::milliseconds(200));

  EXPECT_TRUE(event->executed);
  EXPECT_EQ(event->get_execution_status(), TimeoutQueue::Event::Executed);

  delete event;
}

TEST(TimeoutQueueTest, ClearAll)
{
  vector<TestEvent*> events;
  for (int i = 0; i < 5; ++i)
    {
      auto* e = new TestEvent(time_from_now_ms(500 + i * 100));
      events.push_back(e);
      g_queue->schedule_event(e);
    }

  EXPECT_EQ(g_queue->size(), 5u);

  size_t cleared = g_queue->clear_all();
  EXPECT_EQ(cleared, 5u);
  EXPECT_TRUE(g_queue->is_empty());

  for (auto* e : events)
    {
      EXPECT_EQ(e->get_execution_status(), TimeoutQueue::Event::Canceled);
      delete e;
    }
}

TEST(TimeoutQueueTest, Statistics)
{
  g_queue->reset_stats();

  size_t initial_executed = g_queue->executed_count();
  size_t initial_canceled = g_queue->canceled_count();
  EXPECT_EQ(initial_executed, 0u);
  EXPECT_EQ(initial_canceled, 0u);

  // Execute some events
  mutex mtx;
  condition_variable cv;
  atomic<int> executed_callbacks{0};
  auto* e1 = new TestEvent(time_from_now_ms(50));
  auto* e2 = new TestEvent(time_from_now_ms(100));
  const auto on_executed =
    [&](TimeoutQueue::Event*, TimeoutQueue::Event::Execution_Status status)
    {
      if (status == TimeoutQueue::Event::Executed)
        ++executed_callbacks;
      cv.notify_all();
    };
  e1->set_completion_callback(on_executed);
  e2->set_completion_callback(on_executed);

  g_queue->schedule_event(e1);
  g_queue->schedule_event(e2);

  unique_lock<mutex> lock(mtx);
  ASSERT_TRUE(cv.wait_for(lock, chrono::seconds(30),
                          [&] { return executed_callbacks.load() == 2; }));
  lock.unlock();

  EXPECT_EQ(g_queue->executed_count(), 2u);

  // Cancel some events
  auto* e3 = new TestEvent(time_from_now_ms(500));
  auto* e4 = new TestEvent(time_from_now_ms(600));
  g_queue->schedule_event(e3);
  g_queue->schedule_event(e4);

  EXPECT_TRUE(g_queue->cancel_event(e3));
  EXPECT_TRUE(g_queue->cancel_event(e4));

  EXPECT_EQ(g_queue->canceled_count(), 2u);

  delete e1;
  delete e2;
  delete e3;
  delete e4;
}

TEST(TimeoutQueueTest, ResetStats)
{
  // Ensure some stats exist
  mutex mtx;
  condition_variable cv;
  atomic<bool> completed{false};

  auto* e = new TestEvent(time_from_now_ms(50));
  e->set_completion_callback(
    [&](TimeoutQueue::Event*, TimeoutQueue::Event::Execution_Status status)
    {
      completed = status == TimeoutQueue::Event::Executed;
      cv.notify_all();
    });

  g_queue->schedule_event(e);

  unique_lock<mutex> lock(mtx);
  ASSERT_TRUE(cv.wait_for(lock, chrono::seconds(30),
                          [&] { return completed.load(); }));
  lock.unlock();

  // Reset and verify
  g_queue->reset_stats();
  EXPECT_EQ(g_queue->executed_count(), 0u);
  EXPECT_EQ(g_queue->canceled_count(), 0u);

  delete e;
}

// =============================================================================
// New Features Tests
// =============================================================================

TEST(TimeoutQueueTest, PauseAndResume)
{
  g_queue->reset_stats();
  EXPECT_FALSE(g_queue->is_paused());

  auto* e1 = new TestEvent(time_from_now_ms(100));
  g_queue->schedule_event(e1);

  // Pause before event triggers
  g_queue->pause();
  EXPECT_TRUE(g_queue->is_paused());

  // Wait past trigger time - event should NOT execute
  this_thread::sleep_for(chrono::milliseconds(200));
  EXPECT_FALSE(e1->executed);

  // Resume - event should execute now
  g_queue->resume();
  EXPECT_FALSE(g_queue->is_paused());

  this_thread::sleep_for(chrono::milliseconds(150));
  EXPECT_TRUE(e1->executed);

  delete e1;
}

TEST(TimeoutQueueTest, WaitUntilEmpty)
{
  auto* e1 = new TestEvent(time_from_now_ms(100));
  auto* e2 = new TestEvent(time_from_now_ms(150));

  g_queue->schedule_event(e1);
  g_queue->schedule_event(e2);

  EXPECT_FALSE(g_queue->is_empty());

  // Wait for all events to complete
  bool completed = g_queue->wait_until_empty(500);
  EXPECT_TRUE(completed);
  EXPECT_TRUE(g_queue->is_empty());
  EXPECT_TRUE(e1->executed);
  EXPECT_TRUE(e2->executed);

  delete e1;
  delete e2;
}

TEST(TimeoutQueueTest, WaitUntilEmptyTimeout)
{
  auto* e = new TestEvent(time_from_now_ms(500));
  g_queue->schedule_event(e);

  // Wait with short timeout - should timeout
  bool completed = g_queue->wait_until_empty(50);
  EXPECT_FALSE(completed);
  EXPECT_FALSE(g_queue->is_empty());

  // Cancel and cleanup
  g_queue->cancel_event(e);
  delete e;
}

TEST(TimeoutQueueTest, WaitUntilEmptyAfterCancel)
{
  auto* e = new TestEvent(time_from_now_ms(300));
  g_queue->schedule_event(e);

  EXPECT_TRUE(g_queue->cancel_event(e));

  EXPECT_TRUE(g_queue->wait_until_empty(200));
  EXPECT_TRUE(g_queue->is_empty());

  delete e;
}

TEST(TimeoutQueueTest, WaitUntilEmptyAfterCancelDelete)
{
  TimeoutQueue::Event* e = new TestEvent(time_from_now_ms(300));
  g_queue->schedule_event(e);

  g_queue->cancel_delete_event(e);
  EXPECT_EQ(e, nullptr);

  EXPECT_TRUE(g_queue->wait_until_empty(200));
  EXPECT_TRUE(g_queue->is_empty());
}

TEST(TimeoutQueueTest, EventName)
{
  // Create event with name
  class NamedEvent : public TimeoutQueue::Event
  {
  public:
    NamedEvent(const Time& t, const string& name) : Event(t, name) {}
    void EventFct() override {}
  };

  auto* e = new NamedEvent(time_from_now_ms(100), "TestEventName");
  EXPECT_EQ(e->get_name(), "TestEventName");

  e->set_name("NewName");
  EXPECT_EQ(e->get_name(), "NewName");

  delete e;
}

TEST(TimeoutQueueTest, CompletionCallback)
{
  atomic<bool> callback_called{false};
  atomic<int> final_status{-1};

  auto* e = new TestEvent(time_from_now_ms(50));
  e->set_completion_callback([&](TimeoutQueue::Event* ev, TimeoutQueue::Event::Execution_Status status) {
    (void) ev;
    callback_called = true;
    final_status = static_cast<int>(status);
  });

  g_queue->schedule_event(e);
  this_thread::sleep_for(chrono::milliseconds(200));

  EXPECT_TRUE(callback_called);
  EXPECT_EQ(final_status, static_cast<int>(TimeoutQueue::Event::Executed));

  delete e;
}

TEST(TimeoutQueueTest, CompletionCallbackOnCancel)
{
  atomic<bool> callback_called{false};
  atomic<int> final_status{-1};

  auto* e = new TestEvent(time_from_now_ms(500));
  e->set_completion_callback([&](TimeoutQueue::Event*, TimeoutQueue::Event::Execution_Status status) {
    callback_called = true;
    final_status = static_cast<int>(status);
  });

  g_queue->schedule_event(e);
  g_queue->clear_all();

  this_thread::sleep_for(chrono::milliseconds(50));

  EXPECT_TRUE(callback_called);
  EXPECT_EQ(final_status, static_cast<int>(TimeoutQueue::Event::Canceled));

  delete e;
}

TEST(TimeoutQueueTest, EventId)
{
  auto* e1 = new TestEvent(time_from_now_ms(500));
  auto* e2 = new TestEvent(time_from_now_ms(600));

  // Each event should have a unique ID
  EXPECT_NE(e1->get_id(), TimeoutQueue::Event::InvalidId);
  EXPECT_NE(e2->get_id(), TimeoutQueue::Event::InvalidId);
  EXPECT_NE(e1->get_id(), e2->get_id());

  delete e1;
  delete e2;
}

TEST(TimeoutQueueTest, FindById)
{
  auto* e1 = new TestEvent(time_from_now_ms(500));
  auto* e2 = new TestEvent(time_from_now_ms(600));

  auto id1 = e1->get_id();
  auto id2 = e2->get_id();

  g_queue->schedule_event(e1);
  g_queue->schedule_event(e2);

  // Should find scheduled events
  EXPECT_EQ(g_queue->find_by_id(id1), e1);
  EXPECT_EQ(g_queue->find_by_id(id2), e2);

  // Invalid ID should return nullptr
  EXPECT_EQ(g_queue->find_by_id(TimeoutQueue::Event::InvalidId), nullptr);
  EXPECT_EQ(g_queue->find_by_id(999999), nullptr);

  g_queue->cancel_event(e1);
  g_queue->cancel_event(e2);

  // After cancel, should not find
  EXPECT_EQ(g_queue->find_by_id(id1), nullptr);

  delete e1;
  delete e2;
}

TEST(TimeoutQueueTest, CancelById)
{
  auto* e1 = new TestEvent(time_from_now_ms(500));
  auto* e2 = new TestEvent(time_from_now_ms(600));

  auto id1 = e1->get_id();
  auto id2 = e2->get_id();

  g_queue->schedule_event(e1);
  g_queue->schedule_event(e2);

  EXPECT_EQ(g_queue->size(), 2u);

  // Cancel by ID
  EXPECT_TRUE(g_queue->cancel_by_id(id1));
  EXPECT_EQ(g_queue->size(), 1u);
  EXPECT_EQ(e1->get_execution_status(), TimeoutQueue::Event::Canceled);

  // Cancel same ID again should fail
  EXPECT_FALSE(g_queue->cancel_by_id(id1));

  // Cancel invalid ID should fail
  EXPECT_FALSE(g_queue->cancel_by_id(TimeoutQueue::Event::InvalidId));
  EXPECT_FALSE(g_queue->cancel_by_id(999999));

  // Cancel second event
  EXPECT_TRUE(g_queue->cancel_by_id(id2));
  EXPECT_TRUE(g_queue->is_empty());

  delete e1;
  delete e2;
}

TEST(TimeoutQueueTest, CancelByIdWithCallback)
{
  atomic<bool> callback_called{false};
  auto* e = new TestEvent(time_from_now_ms(500));
  auto id = e->get_id();

  e->set_completion_callback([&](TimeoutQueue::Event*, TimeoutQueue::Event::Execution_Status status) {
    callback_called = true;
    EXPECT_EQ(status, TimeoutQueue::Event::Canceled);
  });

  g_queue->schedule_event(e);
  EXPECT_TRUE(g_queue->cancel_by_id(id));

  EXPECT_TRUE(callback_called);

  delete e;
}

// =============================================================================
// Regression Tests for Bug Fixes
// =============================================================================

TEST(TimeoutQueueTest, DestructorWithoutShutdown)
{
  // Test that destructor auto-shutdowns if shutdown() wasn't called
  // In Debug builds, this should print a warning to stderr
  // In Release builds, it should silently auto-shutdown
  testing::internal::CaptureStderr();

  TimeoutQueue* queue = new TimeoutQueue();
  auto* event = new TestEvent(time_from_now_ms(1000));
  queue->schedule_event(event);

  // Delete without calling shutdown() - should auto-shutdown
  delete queue;

  string output = testing::internal::GetCapturedStderr();
# ifndef NDEBUG
  // In debug builds, expect warning message
  EXPECT_NE(output.find("WARNING"), string::npos);
  EXPECT_NE(output.find("shutdown"), string::npos);
# else
  // In release builds, no warning should be printed
  EXPECT_EQ(output.find("WARNING"), string::npos);
# endif

  delete event;
}

TEST(TimeoutQueueTest, CancelBeforeDeleteIsSafe)
{
  // Test that canceling an event before deletion is safe (no error)
  testing::internal::CaptureStderr();

  auto* event = new TestEvent(time_from_now_ms(1000));
  g_queue->schedule_event(event);

  EXPECT_EQ(event->get_execution_status(), TimeoutQueue::Event::In_Queue);

  // Cancel first to remove from the queue, then delete
  g_queue->cancel_event(event);

  // Now it's safe to delete
  delete event;

  string output = testing::internal::GetCapturedStderr();
  // Should not have warning/error since we canceled first
  EXPECT_EQ(output, "");
}

// Death test: Verify that deleting an In_Queue event throws fatal error
// Disabled under ThreadSanitizer: TSAN does not support fork() after threads
// have been created, and ASSERT_DEATH uses fork().
#if defined(__SANITIZE_THREAD__)
  // GCC defines __SANITIZE_THREAD__ when compiled with -fsanitize=thread
#  define TSAN_ENABLED 1
#endif

#ifdef __clang__
  // Clang uses __has_feature(thread_sanitizer)
#  if __has_feature(thread_sanitizer)
#    define TSAN_ENABLED 1
#  endif
#endif

#ifdef TSAN_ENABLED
TEST(TimeoutQueueDeathTest, DISABLED_DeleteEventInQueueDirectlyThrows)
#else
TEST(TimeoutQueueDeathTest, DeleteEventInQueueDirectlyThrows)
#endif
{
  // This test verifies fail-fast behavior to prevent use-after-free.
  // When Event::~Event() aborts from destructor, std::terminate() is called.
  ASSERT_DEATH({
    TimeoutQueue queue;
    auto* event = new TestEvent(time_from_now_ms(1000));
    queue.schedule_event(event);
    // Deleting without cancel should abort, causing process termination
    delete event;
    queue.shutdown();
  }, "In_Queue.*use-after-free");
}

TEST(TimeoutQueueTest, CancelDuringTimeout)
{
  // Regression test for getMin() bug: an event canceled while the worker
  // is inside wait_until must not cause the next event to be lost.
  //
  // Timing mirrors RescheduleDuringTimeout. GitHub-hosted macOS runners
  // (both macos-15 arm64 and Rosetta-emulated macos-15-intel) routinely
  // see sleep_for jitter exceeding 200 ms when ctest runs --parallel.
  // Two margins matter:
  //
  //   1. e1's trigger (500 ms) must NOT fire before we issue the cancel
  //      (after a 200 ms sleep) — 300 ms of slack absorbs severe drift.
  //      If e1 fires first, the worker moves it to Executing and the
  //      subsequent cancel returns false, breaking every assertion below.
  //   2. e2's trigger (1500 ms) stays well past the cancel point so the
  //      cancel demonstrably happens during the worker's wait_until on
  //      e1's deadline, and the final 2000 ms wait clears e2 with margin.
  g_queue->reset_stats();

  auto* e1 = new TestEvent(time_from_now_ms(500));
  auto* e2 = new TestEvent(time_from_now_ms(1500));

  g_queue->schedule_event(e1);
  g_queue->schedule_event(e2);

  // Cancel e1 while the worker is inside wait_until on its deadline.
  this_thread::sleep_for(chrono::milliseconds(200));
  bool canceled = g_queue->cancel_event(e1);
  EXPECT_TRUE(canceled);

  // e2 (≈1500 ms total, ≈1300 ms from here) must still fire; the extra
  // ~700 ms of slack absorbs scheduler drift on slow runners.
  this_thread::sleep_for(chrono::milliseconds(2000));
  EXPECT_FALSE(e1->executed);
  EXPECT_TRUE(e2->executed);

  EXPECT_EQ(g_queue->executed_count(), 1u);
  EXPECT_EQ(g_queue->canceled_count(), 1u);

  delete e1;
  delete e2;
}

TEST(TimeoutQueueTest, RescheduleDuringTimeout)
{
  // Reschedule the top event while the worker is waiting for it.
  //
  // Timing is calibrated for slow virtualized CI runners (macos-15-intel
  // runs x86_64 emulated under Rosetta, where sleep_for jitter routinely
  // exceeds 200 ms). Two margins are critical:
  //
  //   1. e1's original trigger (500 ms) must NOT fire before we issue the
  //      reschedule (after a 200 ms sleep) — 300 ms of slack absorbs even
  //      severe scheduler drift.
  //   2. e1's rescheduled trigger (200 + 2000 = 2200 ms) must stay well
  //      ahead of the mid-test assertion (~1500 ms) and well behind the
  //      final assertion (~3000 ms), so the order of events is observable
  //      regardless of jitter.
  auto* e1 = new TestEvent(time_from_now_ms(500));
  auto* e2 = new TestEvent(time_from_now_ms(1200));

  g_queue->schedule_event(e1);
  g_queue->schedule_event(e2);

  // Reschedule e1 to fire much later (absolute trigger ≈ t=2200 ms)
  this_thread::sleep_for(chrono::milliseconds(200));
  g_queue->reschedule_event(time_from_now_ms(2000), e1);

  // e2 should have executed at its original time (≈1200 ms),
  // e1 should still be waiting on its new trigger (≈2200 ms).
  this_thread::sleep_for(chrono::milliseconds(1300));
  EXPECT_TRUE(e2->executed);
  EXPECT_FALSE(e1->executed);

  // e1 should now have fired
  this_thread::sleep_for(chrono::milliseconds(1500));
  EXPECT_TRUE(e1->executed);

  delete e1;
  delete e2;
}

TEST(TimeoutQueueTest, NullEventValidation)
{
  // Test that passing nullptr throws exception (not assertion failure in Release)
  Time t = time_from_now_ms(100);

  EXPECT_THROW(g_queue->schedule_event(nullptr), std::invalid_argument);
  EXPECT_THROW(g_queue->schedule_event(t, nullptr), std::invalid_argument);
  EXPECT_THROW(g_queue->reschedule_event(t, nullptr), std::invalid_argument);
}

TEST(TimeoutQueueTest, InvalidNsecValidation)
{
  // The implementation enforces tv_nsec bounds with assertions inside Event::set_trigger_time()
  // and with ah_domain_error_if inside schedule_event(Event*).
  // In Debug builds, the assert triggers first (process death).
  // In Release builds, assertions are disabled but ah_domain_error_if throws std::domain_error.
# ifndef NDEBUG
  ASSERT_DEATH(
    {
      TimeoutQueue q;
      auto* e = new TestEvent(time_from_now_ms(1000));
      Time bad = read_current_time();
      bad.tv_nsec = 2000000000; // Invalid: >= 1e9
      q.schedule_event(bad, e);
    },
    "");
# else
  TimeoutQueue q;
  auto* e = new TestEvent(time_from_now_ms(1000));
  Time bad = read_current_time();
  bad.tv_nsec = 2000000000; // Invalid: >= 1e9
  EXPECT_THROW(q.schedule_event(bad, e), std::domain_error);
  delete e; // Clean up since schedule_event threw before taking ownership
  q.shutdown();
# endif
}

TEST(TimeoutQueueTest, ScheduleSameEventTwiceThrows)
{
  auto* e = new TestEvent(time_from_now_ms(500));
  g_queue->schedule_event(e);
  EXPECT_THROW(g_queue->schedule_event(e), std::invalid_argument);
  g_queue->cancel_event(e);
  delete e;
}

TEST(TimeoutQueueTest, ShutdownCancelsPendingEventsAndInvokesCallback)
{
  TimeoutQueue q;

  std::mutex m;
  std::condition_variable cv;
  int callbacks = 0;

  auto* e = new TestEvent(time_from_now_ms(1000));
  e->set_completion_callback([&](TimeoutQueue::Event* ev, TimeoutQueue::Event::Execution_Status st) {
    std::lock_guard<std::mutex> lk(m);
    EXPECT_EQ(ev->get_execution_status(), st);
    EXPECT_EQ(st, TimeoutQueue::Event::Canceled);
    ++callbacks;
    cv.notify_all();
  });

  q.schedule_event(e);
  q.shutdown();

  std::unique_lock<std::mutex> lk(m);
  ASSERT_TRUE(cv.wait_for(lk, std::chrono::milliseconds(500), [&]{ return callbacks == 1; }));

  EXPECT_FALSE(q.is_running());

  delete e;
}

TEST(TimeoutQueueTest, CancelDeleteEventCallback)
{
  // Test that cancel_delete_event invokes completion callback
  atomic<bool> callback_called{false};
  atomic<int> callback_status{-1};

  TimeoutQueue::Event* event = new TestEvent(time_from_now_ms(500));

  event->set_completion_callback([&](TimeoutQueue::Event* ev,
                                     TimeoutQueue::Event::Execution_Status status) {
    EXPECT_EQ(ev, nullptr);  // Event already destroyed when status is Deleted
    callback_called = true;
    callback_status = static_cast<int>(status);
  });

  g_queue->schedule_event(event);
  g_queue->cancel_delete_event(event);

  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_status, static_cast<int>(TimeoutQueue::Event::Deleted));
  EXPECT_EQ(event, nullptr);
}

TEST(TimeoutQueueTest, CancelDeleteExecutingEvent)
{
  // Test cancel_delete_event on an event that's currently Executing
  mutex mtx;
  condition_variable cv;
  atomic<bool> event_started{false};
  atomic<bool> can_finish{false};

  class BlockingEvent : public TimeoutQueue::Event
  {
  public:
    atomic<bool>& started;
    atomic<bool>& finish_flag;

    BlockingEvent(const Time& t, atomic<bool>& s, atomic<bool>& f)
      : Event(t), started(s), finish_flag(f) {}

    void EventFct() override
    {
      started = true;
      while (!finish_flag)
        this_thread::sleep_for(chrono::milliseconds(10));
    }
  };

  TimeoutQueue::Event* event = new BlockingEvent(
    time_from_now_ms(50), event_started, can_finish);

  atomic<bool> callback_called{false};
  atomic<TimeoutQueue::Event*> callback_ev{reinterpret_cast<TimeoutQueue::Event*>(0x1)}; // sentinel
  event->set_completion_callback([&](auto* ev, auto status) {
    callback_ev = ev;
    callback_called = true;
    EXPECT_EQ(ev, nullptr);  // Event already destroyed when status is Deleted
    EXPECT_EQ(status, TimeoutQueue::Event::Deleted);
  });

  g_queue->schedule_event(event);

  // Wait for event to start executing
  while (!event_started)
    this_thread::sleep_for(chrono::milliseconds(10));

  // Try to cancel_delete while it's executing
  // Should mark as To_Delete and worker will delete it
  g_queue->cancel_delete_event(event);
  EXPECT_EQ(event, nullptr);

  // Let event finish
  can_finish = true;

  // Wait deterministically for the worker to leave EventFct(), delete the event,
  // and invoke the callback. A fixed sleep here is unreliable under CPU
  // starvation and can leak unfinished worker activity into the next test.
  EXPECT_TRUE(wait_until([&] { return callback_called.load(); },
                         chrono::seconds(5)))
    << "Worker did not invoke the deletion callback in time";

  // Callback should have been called by worker thread
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_ev.load(), nullptr);
}

TEST(TimeoutQueueTest, CallbackReceivesNullptrOnlyForDeleted)
{
  // Verify that Deleted status yields nullptr, while Executed/Canceled yield
  // a valid pointer.
  atomic<TimeoutQueue::Event*> exec_ptr{nullptr};
  atomic<TimeoutQueue::Event*> cancel_ptr{nullptr};
  atomic<TimeoutQueue::Event*> delete_ptr{reinterpret_cast<TimeoutQueue::Event*>(0x1)};
  atomic<bool> exec_done{false};

  // 1. Executed path: callback must receive non-null pointer.
  //    Synchronize on the callback itself instead of a fixed sleep: under CPU
  //    starvation the worker thread may take far longer than any fixed delay to
  //    run the event, which previously caused this test to fail intermittently
  //    (and to delete an event the worker still owned).
  TimeoutQueue::Event* e1 = new TestEvent(time_from_now_ms(50));
  e1->set_completion_callback([&](TimeoutQueue::Event* ev, auto status) {
    EXPECT_EQ(status, TimeoutQueue::Event::Executed);
    EXPECT_NE(ev, nullptr);
    exec_ptr = ev;
    exec_done = true;  // set last: signals the worker is done touching the event
  });
  g_queue->schedule_event(e1);

  const bool executed =
    wait_until([&] { return exec_done.load(); }, chrono::seconds(5));
  EXPECT_TRUE(executed) << "Executed completion callback was not invoked in time";
  EXPECT_NE(exec_ptr.load(), nullptr);
  if (executed)
    delete e1;  // safe: status is Executed and the worker no longer references it
  else
    g_queue->cancel_delete_event(e1);  // best-effort safe removal before return

  // 2. Canceled path (cancel_event): the callback runs synchronously in this
  //    thread, so no waiting is required.
  auto* e2 = new TestEvent(time_from_now_ms(500));
  e2->set_completion_callback([&](TimeoutQueue::Event* ev, auto status) {
    EXPECT_EQ(status, TimeoutQueue::Event::Canceled);
    EXPECT_NE(ev, nullptr);
    cancel_ptr = ev;
  });
  g_queue->schedule_event(e2);
  EXPECT_TRUE(g_queue->cancel_event(e2));
  EXPECT_NE(cancel_ptr.load(), nullptr);
  delete e2;

  // 3. Deleted path (cancel_delete_event): callback must receive nullptr
  TimeoutQueue::Event* e3 = new TestEvent(time_from_now_ms(500));
  e3->set_completion_callback([&](TimeoutQueue::Event* ev, auto status) {
    EXPECT_EQ(status, TimeoutQueue::Event::Deleted);
    EXPECT_EQ(ev, nullptr);
    delete_ptr = ev;
  });
  g_queue->schedule_event(e3);
  g_queue->cancel_delete_event(e3);
  EXPECT_EQ(delete_ptr.load(), nullptr);
  EXPECT_EQ(e3, nullptr);
}

TEST(TimeoutQueueTest, CompletionCallbackOrderCorrect)
{
  // Test that completion callback is called AFTER status is set
  // (regression test for callback/status ordering inconsistency)
  atomic<bool> callback_called{false};
  atomic<TimeoutQueue::Event::Execution_Status> status_in_callback{
    TimeoutQueue::Event::Out_Queue};

  auto* event = new TestEvent(time_from_now_ms(50));

  event->set_completion_callback([&](TimeoutQueue::Event* ev,
                                     TimeoutQueue::Event::Execution_Status status) {
    // Status should already be set when callback is invoked
    status_in_callback = ev->get_execution_status();
    EXPECT_EQ(status, ev->get_execution_status());
    callback_called = true;  // Must be last: main thread deletes after seeing this
  });

  g_queue->schedule_event(event);

  // Spin until callback completes to avoid racing with delete
  while (!callback_called)
    this_thread::sleep_for(chrono::milliseconds(10));

  EXPECT_EQ(status_in_callback, TimeoutQueue::Event::Executed);

  delete event;
}

TEST(TimeoutQueueTest, CompletionCallbackCanClearAllWithoutDeadlock)
{
  atomic<bool> callback_called{false};

  auto* e1 = new TestEvent(time_from_now_ms(50), [&]() { g_queue->clear_all(); });
  auto* e2 = new TestEvent(time_from_now_ms(200));

  e1->set_completion_callback([&](auto*, auto) { callback_called = true; });

  g_queue->schedule_event(e1);
  g_queue->schedule_event(e2);

  this_thread::sleep_for(chrono::milliseconds(400));

  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(e1->executed);
  EXPECT_FALSE(e2->executed); // should have been canceled by clear_all()
  EXPECT_TRUE(g_queue->is_empty());

  delete e1;
  delete e2;
}

TEST(TimeoutQueueTest, MultipleEventsWithSameTime)
{
  // Test that multiple events scheduled for the same time all execute
  const int num_events = 5;
  vector<TestEvent*> events;
  atomic<int> executed_count{0};

  Time same_time = time_from_now_ms(100);

  for (int i = 0; i < num_events; ++i)
    {
      auto* e = new TestEvent(same_time, [&]() { ++executed_count; });
      events.push_back(e);
      g_queue->schedule_event(e);
    }

  this_thread::sleep_for(chrono::milliseconds(300));

  EXPECT_EQ(executed_count, num_events);

  for (auto* e : events)
    {
      EXPECT_TRUE(e->executed);
      delete e;
    }
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::AddGlobalTestEnvironment(new TimeoutQueueEnvironment());
  return RUN_ALL_TESTS();
}
