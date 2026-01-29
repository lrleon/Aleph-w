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
  atomic<int> execution_count{0};

  ReschedulingEvent(const Time& t, TimeoutQueue* q, int max_resc)
    : Event(t), queue(q), reschedule_count(0), max_reschedules(max_resc) {}

  void EventFct() override
  {
    ++execution_count;
    if (reschedule_count < max_reschedules)
      {
        ++reschedule_count;
        queue->reschedule_event(time_from_now_ms(50), this);
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
  auto* event = new TestEvent(time_from_now_ms(50));

  EXPECT_EQ(event->get_execution_status(), TimeoutQueue::Event::Out_Queue);

  g_queue->schedule_event(event);
  EXPECT_EQ(event->get_execution_status(), TimeoutQueue::Event::In_Queue);

  this_thread::sleep_for(chrono::milliseconds(200));
  EXPECT_TRUE(event->executed);

  delete event;
}

TEST(TimeoutQueueTest, ScheduleWithExplicitTime)
{
  auto* event = new TestEvent(time_from_now_ms(1000)); // Will be overridden
  Time trigger_time = time_from_now_ms(50);

  g_queue->schedule_event(trigger_time, event);

  this_thread::sleep_for(chrono::milliseconds(200));
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

  bool canceled = g_queue->cancel_event(event);
  EXPECT_FALSE(canceled);

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
  auto* event = new TimingEvent(time_from_now_ms(500));

  g_queue->schedule_event(event);
  this_thread::sleep_for(chrono::milliseconds(50));

  g_queue->reschedule_event(time_from_now_ms(50), event);

  this_thread::sleep_for(chrono::milliseconds(200));
  EXPECT_TRUE(event->executed);
  EXPECT_LT(event->elapsed_ms(), 300);

  delete event;
}

TEST(TimeoutQueueTest, RescheduleNotInQueue)
{
  auto* event = new TestEvent(time_from_now_ms(100));

  g_queue->reschedule_event(time_from_now_ms(50), event);

  this_thread::sleep_for(chrono::milliseconds(200));
  EXPECT_TRUE(event->executed);

  delete event;
}

TEST(TimeoutQueueTest, SelfReschedulingEvent)
{
  auto* event = new ReschedulingEvent(time_from_now_ms(50), g_queue, 2);

  g_queue->schedule_event(event);

  this_thread::sleep_for(chrono::milliseconds(400));
  EXPECT_EQ(event->execution_count, 3);

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
  const int expected_delay = 100;
  const int tolerance = 50;

  auto* event = new TimingEvent(time_from_now_ms(expected_delay));

  g_queue->schedule_event(event);

  this_thread::sleep_for(chrono::milliseconds(expected_delay + 100));
  ASSERT_TRUE(event->executed);

  int actual_delay = event->elapsed_ms();
  EXPECT_GE(actual_delay, expected_delay - tolerance);
  EXPECT_LE(actual_delay, expected_delay + tolerance);

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
            if (g_queue->cancel_event(events[i]))
              ++canceled_count;
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
  auto* event = new TimingEvent(time_from_now_ms(1000)); // Will be overridden

  g_queue->schedule_after_ms(100, event);

  this_thread::sleep_for(chrono::milliseconds(250));
  ASSERT_TRUE(event->executed);
  EXPECT_LT(event->elapsed_ms(), 200);

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
  auto* e1 = new TestEvent(time_from_now_ms(50));
  auto* e2 = new TestEvent(time_from_now_ms(100));
  g_queue->schedule_event(e1);
  g_queue->schedule_event(e2);

  this_thread::sleep_for(chrono::milliseconds(250));

  EXPECT_EQ(g_queue->executed_count(), 2u);

  // Cancel some events
  auto* e3 = new TestEvent(time_from_now_ms(500));
  auto* e4 = new TestEvent(time_from_now_ms(600));
  g_queue->schedule_event(e3);
  g_queue->schedule_event(e4);

  g_queue->cancel_event(e3);
  g_queue->cancel_event(e4);

  EXPECT_EQ(g_queue->canceled_count(), 2u);

  delete e1;
  delete e2;
  delete e3;
  delete e4;
}

TEST(TimeoutQueueTest, ResetStats)
{
  // Ensure some stats exist
  auto* e = new TestEvent(time_from_now_ms(50));
  g_queue->schedule_event(e);
  this_thread::sleep_for(chrono::milliseconds(150));

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
  // This should not crash and should print a warning
  testing::internal::CaptureStderr();

  TimeoutQueue* queue = new TimeoutQueue();
  auto* event = new TestEvent(time_from_now_ms(1000));
  queue->schedule_event(event);

  // Delete without calling shutdown() - should auto-shutdown
  delete queue;

  string output = testing::internal::GetCapturedStderr();
  EXPECT_NE(output.find("Warning"), string::npos);
  EXPECT_NE(output.find("shutdown"), string::npos);

  delete event;
}

TEST(TimeoutQueueTest, DeleteEventInQueue)
{
  // Test that deleting an event that's still In_Queue prints warning (doesn't crash)
  testing::internal::CaptureStderr();

  auto* event = new TestEvent(time_from_now_ms(1000));
  g_queue->schedule_event(event);

  EXPECT_EQ(event->get_execution_status(), TimeoutQueue::Event::In_Queue);

  // Cancel first to remove from queue, then delete
  g_queue->cancel_event(event);

  // Now it's safe to delete
  delete event;

  string output = testing::internal::GetCapturedStderr();
  // Should not have warning since we canceled first
  EXPECT_EQ(output, "");
}

// Note: DeleteEventInQueueDirectly would be UB in practice.
// The warning is tested indirectly - if someone does this by mistake,
// they'll see the warning instead of a crash/terminate.

TEST(TimeoutQueueTest, CancelDuringTimeout)
{
  // Regression test for getMin() bug: event canceled during wait_until
  // should not cause next event to be lost
  g_queue->reset_stats();

  auto* e1 = new TestEvent(time_from_now_ms(100));
  auto* e2 = new TestEvent(time_from_now_ms(200));

  g_queue->schedule_event(e1);
  g_queue->schedule_event(e2);

  // Wait until just before e1 should fire, then cancel it
  this_thread::sleep_for(chrono::milliseconds(90));
  bool canceled = g_queue->cancel_event(e1);
  EXPECT_TRUE(canceled);

  // e2 should still execute (not be lost due to getMin() bug)
  this_thread::sleep_for(chrono::milliseconds(200));
  EXPECT_FALSE(e1->executed);
  EXPECT_TRUE(e2->executed);

  EXPECT_EQ(g_queue->executed_count(), 1u);
  EXPECT_EQ(g_queue->canceled_count(), 1u);

  delete e1;
  delete e2;
}

TEST(TimeoutQueueTest, RescheduleDuringTimeout)
{
  // Similar test: reschedule top event while worker is waiting for it
  auto* e1 = new TestEvent(time_from_now_ms(100));
  auto* e2 = new TestEvent(time_from_now_ms(300));

  g_queue->schedule_event(e1);
  g_queue->schedule_event(e2);

  // Reschedule e1 to much later
  this_thread::sleep_for(chrono::milliseconds(50));
  g_queue->reschedule_event(time_from_now_ms(500), e1);

  // e2 should execute at its original time
  this_thread::sleep_for(chrono::milliseconds(350));
  EXPECT_TRUE(e2->executed);
  EXPECT_FALSE(e1->executed);

  // e1 should execute later
  this_thread::sleep_for(chrono::milliseconds(300));
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
    (void) ev;
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
  event->set_completion_callback([&](auto*, auto status) {
    callback_called = true;
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
  this_thread::sleep_for(chrono::milliseconds(100));

  // Callback should have been called by worker thread
  EXPECT_TRUE(callback_called);
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
    callback_called = true;
    EXPECT_EQ(status, ev->get_execution_status());
  });

  g_queue->schedule_event(event);
  this_thread::sleep_for(chrono::milliseconds(200));

  EXPECT_TRUE(callback_called);
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
