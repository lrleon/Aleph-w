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

# include <cstdio>
# include <typeinfo>
# include <timeoutQueue.H>
# include <ah-errors.H>

using namespace std::chrono;

// Initialize static event ID counter
std::atomic<TimeoutQueue::Event::EventId> TimeoutQueue::Event::nextId{0};

// Convert POSIX timespec to std::chrono::time_point (system_clock)
static auto timespec_to_timepoint(const Time & t)
{
  return system_clock::time_point(duration_cast<system_clock::duration>(
                                                                        seconds(t.tv_sec) + nanoseconds(t.tv_nsec)));
}

TimeoutQueue::TimeoutQueue() : isShutdown(false)
{
  workerThread = std::thread(&TimeoutQueue::triggerEvent, this);
}

TimeoutQueue::~TimeoutQueue()
{ {
    std::lock_guard<std::mutex> lock(mtx);
    if (not isShutdown)
      {
#ifndef NDEBUG
        ah_warning(std::cerr)
            << "TimeoutQueue destructor called without prior shutdown(). "
            << "Invoking shutdown() automatically." << std::endl;
#endif
        shutdown_locked();
      }
  }

  if (workerThread.joinable())
    workerThread.join();
}

void TimeoutQueue::schedule_event(const Time & trigger_time,
                                  TimeoutQueue::Event *event)
{
  ah_invalid_argument_if(event == nullptr)
    << "nullptr event";

  event->set_trigger_time(trigger_time);
  schedule_event(event);
}

void TimeoutQueue::schedule_event(TimeoutQueue::Event *event)
{
  ah_invalid_argument_if(event == nullptr)
    << "nullptr event";
  ah_domain_error_if(event->time_key().tv_nsec < 0 or event->time_key().tv_nsec >= NSEC)
    << "event nsec out of range: " << event->time_key().tv_nsec;

  std::lock_guard<std::mutex> lock(mtx);

  event_registry.insert(event);

  ah_invalid_argument_if(event->get_execution_status() == Event::In_Queue)
      << "Event has already been inserted in timeoutQueue";

  if (isShutdown)
    return;

  event->set_execution_status(Event::In_Queue);

  prio_queue.insert(event);
  event_map[event->get_id()] = event;

  cond.notify_one();
}

bool TimeoutQueue::cancel_event(TimeoutQueue::Event *event)
{
  ah_invalid_argument_if(event == nullptr)
    << "nullptr event";

  Event::CompletionCallback callback;
  bool became_empty = false; {
    std::lock_guard<std::mutex> lock(mtx);

    if (event_registry.contains(event) == false)
      return false;

    if (event->get_execution_status() != Event::In_Queue)
      return false;

    callback = event->on_completed;

    prio_queue.remove(event);
    if (event_map.contains(event->get_id()))
      event_map.remove(event->get_id());
    event_registry.remove(event);

    event->set_execution_status(Event::Canceled);
    ++canceledCount;

    became_empty = prio_queue.size() == 0;
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
  bool became_empty = false; {
    bool was_in_queue = false;
    std::lock_guard<std::mutex> lock(mtx);

    ah_invalid_argument_unless(event_registry.contains(local))
      << "Event " << local << " not found in timeoutQueue";

    if (local->get_execution_status() == Event::In_Queue)
      {
        prio_queue.remove(local);
        if (event_map.contains(local->get_id()))
          event_map.remove(local->get_id());
        event_registry.remove(local);
        became_empty = prio_queue.size() == 0;
        was_in_queue = true;
      }

    if (local->get_execution_status() == Event::Executing)
      {
        // Worker thread will invoke callback and delete after EventFct() returns
        local->set_execution_status(Event::To_Delete);
        event_registry.remove(local);

        // Also remove from event_map to prevent find_by_id() from returning a pointer
        // that is about to be deleted by the worker thread.
        if (event_map.contains(local->get_id()))
          event_map.remove(local->get_id());

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

  delete local;
  event = nullptr;

  if (callback)
    callback(nullptr, final_status);

  if (became_empty)
    emptyCondition.notify_all();

  cond.notify_one();
}

void TimeoutQueue::reschedule_event(const Time & trigger_time,
                                    TimeoutQueue::Event *event)
{
  ah_invalid_argument_if(event == nullptr)
    << "nullptr event";

  std::lock_guard<std::mutex> lock(mtx);

  event_registry.insert(event);

  if (isShutdown)
    return;

  if (event->get_execution_status() == Event::In_Queue)
    prio_queue.remove(event); // eventMap entry stays, ID doesn't change

  event->set_trigger_time(trigger_time);

  event->set_execution_status(Event::In_Queue);

  prio_queue.insert(event);
  event_map[event->get_id()] = event; // Re-add in case it wasn't there

  cond.notify_one();
}

void TimeoutQueue::triggerEvent()
{
  std::unique_lock<std::mutex> lock(mtx);

  while (true)
    {
      // Sleep if there are no events or if paused
      while ((prio_queue.size() == 0 or isPaused) and not isShutdown)
        cond.wait(lock);

      if (isShutdown)
        break;

      // Read the soonest event
      auto *event_to_schedule = static_cast<Event *>(prio_queue.top());

      // Compute time when the event must be triggered (wall clock)
      const Time original_trigger_time = event_to_schedule->time_key();
      const auto trigger_sys = timespec_to_timepoint(original_trigger_time);

      // Anchor both clocks under the same lock to avoid skew
      const auto sys_now = system_clock::now();
      const auto steady_now = steady_clock::now();

      // Compute delta in system_clock domain, clamp negative to zero
      auto delta = trigger_sys - sys_now;
      if (delta < system_clock::duration::zero())
        delta = system_clock::duration::zero();

      // Convert delta to steady_clock and build a steady deadline
      const auto deadline_steady =
          steady_now + duration_cast<steady_clock::duration>(delta);

      // Wait until deadline or notification (immune to wall-clock jumps)
      const auto wait_result = cond.wait_until(lock, deadline_steady);

      if (isShutdown)
        break;

      // If paused, go back to waiting
      if (isPaused)
        continue;

      if (wait_result == std::cv_status::timeout)
        {
          if (prio_queue.size() == 0)
            continue;

          // Peek at the soonest event without extracting it
          // If the top changed (original was canceled/rescheduled) and
          // the new top is in the future, go back to wait for it
          if (const auto *next = static_cast<Event *>(prio_queue.top()); next != event_to_schedule and
                                                                         next->time_key() > original_trigger_time)
            continue;

          // Now extract the event we are going to execute
          auto *event_to_execute = static_cast<Event *>(prio_queue.getMin());

          event_to_execute->set_execution_status(Event::Executing);

          lock.unlock();

          try { event_to_execute->EventFct(); }
          catch (...)
            {
              ah_warning(std::cerr) << "Uncaught exception in TimeoutQueue event execution (ID "
                  << event_to_execute->get_id() << ", name: '"
                  << event_to_execute->get_name() << "')" << std::endl;
            }

          lock.lock();

          ++executedCount;

          const auto current_status = event_to_execute->get_execution_status();
          const Event::CompletionCallback callback = event_to_execute->on_completed;
          auto final_status = Event::Executed;

          if (current_status == Event::To_Delete)
            {
              final_status = Event::Deleted;
              event_to_execute->set_execution_status(Event::Deleted);
            }
          else if (current_status == Event::In_Queue)
            {
              // Event was rescheduled during EventFct() - still in queue, don't touch
              final_status = Event::Executed;
            }
          else
            {
              // Cleanup: event should be removed from internal registries.
              // In rare races, another thread may already have removed it; in that
              // case DynMapTree::remove() (and similar) can throw. We ignore such
              // failures here to avoid terminating the worker thread.
              try
                {
                  event_map.remove(event_to_execute->get_id());
                }
              catch (...)
                {
                  // Already removed or inconsistent; ignore in post-execution cleanup.
                }

              try
                {
                  event_registry.remove(event_to_execute);
                }
              catch (...)
                {
                  // Already removed or inconsistent; ignore in post-execution cleanup.
                }
              event_to_execute->set_execution_status(Event::Executed);
            }

          const bool became_empty = prio_queue.size() == 0;

          lock.unlock();

          if (final_status == Event::Deleted)
            {
              delete event_to_execute;
              if (callback)
                callback(nullptr, final_status);
            }
          else if (callback)
            callback(event_to_execute, final_status);

          if (became_empty)
            emptyCondition.notify_all();

          lock.lock();
        }
    }

  // Shutdown requested - cancel all pending events
  while (prio_queue.size() > 0)
    {
      auto *event = static_cast<Event *>(prio_queue.getMin());
      const Event::CompletionCallback callback = event->on_completed;
      event_map.remove(event->get_id());
      event_registry.remove(event);
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
  return prio_queue.size();
}

bool TimeoutQueue::is_empty() const
{
  std::lock_guard<std::mutex> lock(mtx);
  return prio_queue.size() == 0;
}

bool TimeoutQueue::is_running() const
{
  std::lock_guard<std::mutex> lock(mtx);
  return not isShutdown;
}

void TimeoutQueue::schedule_after_ms(int ms_from_now, Event *event)
{
  const Time trigger_time = time_plus_msec(read_current_time(), ms_from_now);
  schedule_event(trigger_time, event);
}

Time TimeoutQueue::next_event_time() const
{
  std::lock_guard<std::mutex> lock(mtx);
  if (prio_queue.size() == 0)
    return {0, 0};
  return static_cast<Event *>(prio_queue.top())->time_key();
}

size_t TimeoutQueue::clear_all()
{
  std::unique_lock<std::mutex> lock(mtx);

  size_t count = 0;
  while (prio_queue.size() > 0)
    {
      auto *event = static_cast<Event *>(prio_queue.getMin());
      const Event::CompletionCallback callback = event->on_completed;
      event_map.remove(event->get_id());
      event_registry.remove(event);
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

bool TimeoutQueue::wait_until_empty(const int timeout_ms)
{
  std::unique_lock<std::mutex> lock(mtx);

  if (prio_queue.size() == 0)
    return true;

  if (timeout_ms <= 0)
    {
      emptyCondition.wait(lock, [this]() { return prio_queue.size() == 0 or isShutdown; });
      return prio_queue.size() == 0;
    }

  return emptyCondition.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                                 [this]() { return prio_queue.size() == 0 or isShutdown; });
}

TimeoutQueue::Event * TimeoutQueue::find_by_id(const Event::EventId id) const
{
  if (id == Event::InvalidId)
    return nullptr;

  std::lock_guard<std::mutex> lock(mtx);

  if (const auto ptr_pair = event_map.search(id);
    ptr_pair != nullptr and ptr_pair->second->get_execution_status() == Event::In_Queue)
    return ptr_pair->second;

  return nullptr;
}

bool TimeoutQueue::cancel_by_id(const Event::EventId id)
{
  if (id == Event::InvalidId)
    return false;

  Event::CompletionCallback callback;
  Event *event = nullptr;
  bool became_empty = false; {
    std::lock_guard<std::mutex> lock(mtx);

    const auto event_pair = event_map.search(id);
    if (event_pair == nullptr)
      return false;

    event = event_pair->second;
    if (event->get_execution_status() != Event::In_Queue)
      return false;

    callback = event->on_completed;
    prio_queue.remove(event);
    event_map.remove(id);
    event_registry.remove(event);
    event->set_execution_status(Event::Canceled);
    ++canceledCount;

    became_empty = prio_queue.size() == 0;
  }

  if (callback)
    callback(event, Event::Canceled);
  cond.notify_one();

  if (became_empty)
    emptyCondition.notify_all();

  return true;
}
