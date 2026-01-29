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


// Note: Updated to reflect header Doxygen group changes.

# include <cstdio>
# include <typeinfo>
# include <timeoutQueue.H>
# include <ah-errors.H>

using namespace std::chrono;

// Initialize static event ID counter
std::atomic<TimeoutQueue::Event::EventId> TimeoutQueue::Event::nextId{0};

// Convert POSIX timespec to std::chrono::time_point (system_clock)
static auto timespec_to_timepoint(const Time& t)
{
  return system_clock::time_point(duration_cast<system_clock::duration>(
      seconds(t.tv_sec) + nanoseconds(t.tv_nsec)));
}


TimeoutQueue::TimeoutQueue() : isShutdown(false)
{
  workerThread = std::thread(&TimeoutQueue::triggerEvent, this);
}


TimeoutQueue::~TimeoutQueue()
{
  {
    std::lock_guard<std::mutex> lock(mtx);
    if (not isShutdown)
      {
        std::cerr << "Warning: TimeoutQueue destructor called without prior shutdown(). "
                  << "Invoking shutdown() automatically." << std::endl;
        shutdown_locked();
      }
  }

  if (workerThread.joinable())
    workerThread.join();
}


void TimeoutQueue::schedule_event(const Time & trigger_time,
                                  TimeoutQueue::Event *event)
{
  ah_invalid_argument_if(event == nullptr) << "event is nullptr";

  event->set_trigger_time(trigger_time);
  schedule_event(event);
}


void TimeoutQueue::schedule_event(TimeoutQueue::Event *event)
{
  ah_invalid_argument_if(event == nullptr) << "event is nullptr";
  ah_domain_error_if(EVENT_NSEC(event) < 0 or EVENT_NSEC(event) >= NSEC)
    << "event nsec out of range: " << EVENT_NSEC(event);

  std::lock_guard<std::mutex> lock(mtx);

  ah_invalid_argument_if(event->get_execution_status() == Event::In_Queue)
      << "Event has already been inserted in timeoutQueue";

  if (isShutdown)
    return;

  event->set_execution_status(Event::In_Queue);

  prioQueue.insert(event);
  eventMap[event->get_id()] = event;

  cond.notify_one();
}


bool TimeoutQueue::cancel_event(TimeoutQueue::Event *event)
{
  Event::CompletionCallback callback;
  bool became_empty = false;

  {
    std::lock_guard<std::mutex> lock(mtx);

    if (event->get_execution_status() != Event::In_Queue)
      return false;

    callback = event->on_completed;

    prioQueue.remove(event);
    eventMap.erase(event->get_id());

    event->set_execution_status(Event::Canceled);
    ++canceledCount;

    became_empty = prioQueue.size() == 0;
  }

  if (callback)
    callback(event, Event::Canceled);

  if (became_empty)
    emptyCondition.notify_all();

  cond.notify_one();

  return true;
}


void TimeoutQueue::cancel_delete_event(Event *& event)
{
  if (event == nullptr)
    return;

  Event *local = event;
  Event::CompletionCallback callback;
  Event::Execution_Status final_status = Event::Deleted;
  bool became_empty = false;
  bool was_in_queue = false;

  {
    std::lock_guard<std::mutex> lock(mtx);

    if (local->get_execution_status() == Event::In_Queue)
      {
        prioQueue.remove(local);
        eventMap.erase(local->get_id());
        became_empty = prioQueue.size() == 0;
        was_in_queue = true;
      }

    if (local->get_execution_status() == Event::Executing)
      {
        // Worker thread will invoke callback and delete after EventFct() returns
        local->set_execution_status(Event::To_Delete);
        event = nullptr;
        cond.notify_one();
        return;
      }

    callback = local->on_completed;

    if (was_in_queue)
      ++canceledCount;

    local->set_execution_status(Event::Deleted);
    final_status = Event::Deleted;
  }

  if (callback)
    callback(local, final_status);

  delete local;
  event = nullptr;

  if (became_empty)
    emptyCondition.notify_all();

  cond.notify_one();
}


void TimeoutQueue::reschedule_event(const Time & trigger_time,
                                    TimeoutQueue::Event *event)
{
  ah_invalid_argument_if(event == nullptr) << "event is nullptr";

  std::lock_guard<std::mutex> lock(mtx);

  if (isShutdown)
    return;

  if (event->get_execution_status() == Event::In_Queue)
    prioQueue.remove(event);  // eventMap entry stays, ID doesn't change

  event->set_trigger_time(trigger_time);

  event->set_execution_status(Event::In_Queue);

  prioQueue.insert(event);
  eventMap[event->get_id()] = event;  // Re-add in case it wasn't there

  cond.notify_one();
}


void TimeoutQueue::triggerEvent()
{
  std::unique_lock<std::mutex> lock(mtx);

  while (true)
    {
      // Sleep if there are no events or if paused
      while ((prioQueue.size() == 0 or isPaused) and not isShutdown)
        cond.wait(lock);

      if (isShutdown)
        break;

      // Read the soonest event
      auto* event_to_schedule = static_cast<Event*>(prioQueue.top());

      // Compute time when the event must be triggered
      const Time original_trigger_time = EVENT_TIME(event_to_schedule);
      auto trigger_point = timespec_to_timepoint(original_trigger_time);

      // Compute duration to wait using steady clock to avoid wall clock jumps
      const auto now = system_clock::now();
      auto remaining = trigger_point - now;
      if (remaining < system_clock::duration::zero())
        remaining = system_clock::duration::zero();

      // Wait until trigger time or notification
      const auto wait_result = cond.wait_for(
        lock, duration_cast<std::chrono::steady_clock::duration>(remaining));

      if (isShutdown)
        break;

      // If paused, go back to waiting
      if (isPaused)
        continue;

      if (wait_result == std::cv_status::timeout)
        {
          if (prioQueue.size() == 0)
            continue;

          // Peek at the soonest event without extracting it
          // If the top changed (original was canceled/rescheduled) and
          // the new top is in the future, go back to wait for it
          if (const auto* next = static_cast<Event*>(prioQueue.top()); next != event_to_schedule and
              EVENT_TIME(next) > original_trigger_time)
            continue;

          // Now extract the event we are going to execute
          auto* event_to_execute = static_cast<Event*>(prioQueue.getMin());

          event_to_execute->set_execution_status(Event::Executing);

          lock.unlock();

          try { event_to_execute->EventFct(); }
          catch (...) { /* Exceptions are only caught */ }

          lock.lock();

          ++executedCount;
          eventMap.erase(event_to_execute->get_id());

          const Event::CompletionCallback callback = event_to_execute->on_completed;
          auto final_status = Event::Executed;

          if (event_to_execute->get_execution_status() == Event::To_Delete)
            {
              final_status = Event::Deleted;
              event_to_execute->set_execution_status(Event::Deleted);
            }
          else
            {
              event_to_execute->set_execution_status(Event::Executed);
            }

          const bool became_empty = prioQueue.size() == 0;

          lock.unlock();

          if (callback)
            callback(event_to_execute, final_status);

          if (final_status == Event::Deleted)
            delete event_to_execute;

          if (became_empty)
            emptyCondition.notify_all();

          lock.lock();
        }
    }

  // Shutdown requested - cancel all pending events
  while (prioQueue.size() > 0)
    {
      auto* event = static_cast<Event*>(prioQueue.getMin());
      const Event::CompletionCallback callback = event->on_completed;
      eventMap.erase(event->get_id());
      // Set final status BEFORE callback to avoid use-after-free:
      // User code may delete the event in the callback
      event->set_execution_status(Event::Canceled);
      ++canceledCount;
      lock.unlock();
      if (callback)
        callback(event, Event::Canceled);
      lock.lock();
    }

  emptyCondition.notify_all();
}


void TimeoutQueue::shutdown()
{
  std::lock_guard<std::mutex> lock(mtx);
  shutdown_locked();
}


size_t TimeoutQueue::size() const
{
  std::lock_guard<std::mutex> lock(mtx);
  return prioQueue.size();
}


bool TimeoutQueue::is_empty() const
{
  std::lock_guard<std::mutex> lock(mtx);
  return prioQueue.size() == 0;
}


bool TimeoutQueue::is_running() const
{
  std::lock_guard<std::mutex> lock(mtx);
  return not isShutdown;
}


void TimeoutQueue::schedule_after_ms(int ms_from_now, Event* event)
{
  Time trigger_time = time_plus_msec(read_current_time(), ms_from_now);
  schedule_event(trigger_time, event);
}


Time TimeoutQueue::next_event_time() const
{
  std::lock_guard<std::mutex> lock(mtx);
  if (prioQueue.size() == 0)
    return {0, 0};
  return EVENT_TIME(static_cast<Event*>(prioQueue.top()));
}


size_t TimeoutQueue::clear_all()
{
  std::unique_lock<std::mutex> lock(mtx);

  size_t count = 0;
  while (prioQueue.size() > 0)
    {
      auto* event = static_cast<Event*>(prioQueue.getMin());
      const Event::CompletionCallback callback = event->on_completed;
      eventMap.erase(event->get_id());
      event->set_execution_status(Event::Canceled);
      ++count;
      ++canceledCount;
      lock.unlock();
      if (callback)
        callback(event, Event::Canceled);
      lock.lock();
    }

  cond.notify_one();
  emptyCondition.notify_all();

  return count;
}


size_t TimeoutQueue::executed_count() const
{
  std::lock_guard<std::mutex> lock(mtx);
  return executedCount;
}


size_t TimeoutQueue::canceled_count() const
{
  std::lock_guard<std::mutex> lock(mtx);
  return canceledCount;
}


void TimeoutQueue::reset_stats()
{
  std::lock_guard<std::mutex> lock(mtx);
  executedCount = 0;
  canceledCount = 0;
}


void TimeoutQueue::pause()
{
  std::lock_guard<std::mutex> lock(mtx);
  isPaused = true;
}


void TimeoutQueue::resume()
{
  std::lock_guard<std::mutex> lock(mtx);
  isPaused = false;
  cond.notify_one();
}


bool TimeoutQueue::is_paused() const
{
  std::lock_guard<std::mutex> lock(mtx);
  return isPaused;
}


bool TimeoutQueue::wait_until_empty(int timeout_ms)
{
  std::unique_lock<std::mutex> lock(mtx);

  if (prioQueue.size() == 0)
    return true;

  if (timeout_ms <= 0)
    {
      emptyCondition.wait(lock, [this]() { return prioQueue.size() == 0 or isShutdown; });
      return prioQueue.size() == 0;
    }

  return emptyCondition.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                                  [this]() { return prioQueue.size() == 0 or isShutdown; });
}


TimeoutQueue::Event* TimeoutQueue::find_by_id(Event::EventId id) const
{
  if (id == Event::InvalidId)
    return nullptr;

  std::lock_guard<std::mutex> lock(mtx);

  if (const auto it = eventMap.find(id); it != eventMap.end() and it->second->get_execution_status() == Event::In_Queue)
    return it->second;

  return nullptr;
}


bool TimeoutQueue::cancel_by_id(Event::EventId id)
{
  if (id == Event::InvalidId)
    return false;

  Event::CompletionCallback callback;
  Event* event = nullptr;
  bool became_empty = false;

  {
    std::lock_guard<std::mutex> lock(mtx);

    const auto it = eventMap.find(id);
    if (it == eventMap.end())
      return false;

    event = it->second;
    if (event->get_execution_status() != Event::In_Queue)
      return false;

    callback = event->on_completed;
    prioQueue.remove(event);
    eventMap.erase(it);
    event->set_execution_status(Event::Canceled);
    ++canceledCount;

    became_empty = prioQueue.size() == 0;
  }

  if (callback)
    callback(event, Event::Canceled);
  cond.notify_one();

  if (became_empty)
    emptyCondition.notify_all();

  return true;
}
