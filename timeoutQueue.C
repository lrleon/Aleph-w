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


# include <cassert>
# include <cstdio>
# include <typeinfo>
# include <timeoutQueue.H>
# include <ah-errors.H>

using namespace std::chrono;

// Initialize static event ID counter
std::atomic<TimeoutQueue::Event::EventId> TimeoutQueue::Event::nextId{0};

// Convert POSIX timespec to std::chrono::time_point
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
  if (not isShutdown)
    EXIT("TimeoutQueue is not shut down");

  if (workerThread.joinable())
    workerThread.join();
}


void TimeoutQueue::schedule_event(const Time & trigger_time,
                                  TimeoutQueue::Event *event)
{
  assert(event != nullptr);

  event->set_trigger_time(trigger_time);
  schedule_event(event);
}


void TimeoutQueue::schedule_event(TimeoutQueue::Event *event)
{
  assert(event != nullptr);
  assert(EVENT_NSEC(event) >= 0 and EVENT_NSEC(event) < NSEC);

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
  std::lock_guard<std::mutex> lock(mtx);

  if (event->get_execution_status() != Event::In_Queue)
    return false;

  const auto callback = event->on_completed;

  prioQueue.remove(event);
  eventMap.erase(event->get_id());

  // Invoke callback BEFORE setting final status to avoid race
  if (callback)
    callback(event, Event::Canceled);

  event->set_execution_status(Event::Canceled);
  ++canceledCount;

  cond.notify_one();

  return true;
}


void TimeoutQueue::cancel_delete_event(Event *& event)
{
  if (event == nullptr)
    return;

  std::lock_guard<std::mutex> lock(mtx);

  if (event->get_execution_status() == Event::In_Queue)
    {
      prioQueue.remove(event);
      eventMap.erase(event->get_id());
      ++canceledCount;
    }

  if (event->get_execution_status() == Event::Executing)
    event->set_execution_status(Event::To_Delete);
  else
    {
      event->set_execution_status(Event::Deleted);
      delete event;
    }

  event = nullptr;
  cond.notify_one();
}


void TimeoutQueue::reschedule_event(const Time & trigger_time,
                                    TimeoutQueue::Event *event)
{
  assert(event != nullptr);

  std::lock_guard<std::mutex> lock(mtx);

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
      const Time& t = EVENT_TIME(event_to_schedule);
      auto trigger_point = timespec_to_timepoint(t);

      // Wait until trigger time or notification
      const auto wait_result = cond.wait_until(lock, trigger_point);

      if (isShutdown)
        break;

      // If paused, go back to waiting
      if (isPaused)
        continue;

      if (wait_result == std::cv_status::timeout)
        {
          // Check if the event is still the soonest (could have been canceled)
          auto* event_to_execute = static_cast<Event*>(prioQueue.getMin());

          if (event_to_execute != event_to_schedule and
              EVENT_TIME(event_to_execute) > EVENT_TIME(event_to_schedule))
            continue; // Go to schedule another event

          event_to_execute->set_execution_status(Event::Executing);

          lock.unlock();

          try { event_to_execute->EventFct(); }
          catch (...) { /* Exceptions are only caught */ }

          lock.lock();

          ++executedCount;
          eventMap.erase(event_to_execute->get_id());

          // Copy callback before setting final status to avoid race condition:
          // Once we set status to Executed/Deleted, user code may delete the event
          // while we're still trying to invoke the callback
          auto callback = event_to_execute->on_completed;
          auto final_status = Event::Executed;

          if (event_to_execute->get_execution_status() == Event::To_Delete)
            {
              final_status = Event::Deleted;
              // Invoke callback BEFORE setting final status to avoid race:
              // Once status is Deleted, we delete the event ourselves
              if (callback)
                callback(event_to_execute, final_status);
              event_to_execute->set_execution_status(Event::Deleted);
              delete event_to_execute;
            }
          else
            {
              // Invoke callback BEFORE setting final status to avoid race:
              // Once status is Executed, user code may delete the event
              if (callback)
                callback(event_to_execute, final_status);
              event_to_execute->set_execution_status(Event::Executed);
            }

          // Notify if queue became empty
          if (prioQueue.size() == 0)
            emptyCondition.notify_all();
        }
    }

  // Shutdown requested - cancel all pending events
  while (prioQueue.size() > 0)
    {
      auto* event = static_cast<Event*>(prioQueue.getMin());
      auto callback = event->on_completed;
      eventMap.erase(event->get_id());
      // Invoke callback BEFORE setting final status to avoid race
      if (callback)
        callback(event, Event::Canceled);
      event->set_execution_status(Event::Canceled);
      ++canceledCount;
    }

  emptyCondition.notify_all();
}


void TimeoutQueue::shutdown()
{
  std::lock_guard<std::mutex> lock(mtx);

  isShutdown = true;

  cond.notify_one();
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
  std::lock_guard<std::mutex> lock(mtx);

  size_t count = 0;
  while (prioQueue.size() > 0)
    {
      auto* event = static_cast<Event*>(prioQueue.getMin());
      auto callback = event->on_completed;
      eventMap.erase(event->get_id());
      // Invoke callback BEFORE setting final status to avoid race
      if (callback)
        callback(event, Event::Canceled);
      event->set_execution_status(Event::Canceled);
      ++count;
      ++canceledCount;
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

  std::lock_guard<std::mutex> lock(mtx);

  const auto it = eventMap.find(id);
  if (it == eventMap.end())
    return false;

  Event* event = it->second;
  if (event->get_execution_status() != Event::In_Queue)
    return false;

  const auto callback = event->on_completed;
  prioQueue.remove(event);
  eventMap.erase(it);
  // Invoke callback BEFORE setting final status to avoid race
  if (callback)
    callback(event, Event::Canceled);
  event->set_execution_status(Event::Canceled);
  ++canceledCount;
  cond.notify_one();

  if (prioQueue.size() == 0)
    emptyCondition.notify_all();

  return true;
}
