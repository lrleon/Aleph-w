/* Aleph-w

     / \\  | | ___ _ __ | |__      __      __
    / _ \\ | |/ _ \\ '_ \\| '_ \\ ____\\ \\ /\\ / / Data structures \& Algorithms
   / ___ \\| |  __/ |_) | | | |_____\\ V  V /  version 1.9c
  /_/   \\_\\_|\\___| .__/|_| |_|      \\_/\\_/   https://github.com/lrleon/Aleph-w
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

# include <pthread.h>
# include <assert.h>
# include <stdio.h>
# include <unistd.h>
# include <errno.h>
# include <string.h>
# include <typeinfo>
# include <timeoutQueue.H>
# include <ah-errors.H>


int TimeoutQueue::instanceCounter = 0;


TimeoutQueue::TimeoutQueue() : isShutdown(false)
{
  if (instanceCounter >= 1)
    EXIT("Double instantiation (%d) of TimeoutQueue", instanceCounter);

  ++instanceCounter;

  init_mutex(mutex);

  pthread_cond_init(&cond, nullptr);

  const int status =
    pthread_create(&threadId, nullptr , triggerEventThread, this);

  if (status != 0)
    EXIT("Cannot create triggerEventThread (error code = %d)", status);
}


TimeoutQueue::~TimeoutQueue()
{
  if (not isShutdown)
    EXIT("TimeoutQueue is not shut down");

  destroy_mutex(mutex);

  pthread_cond_destroy(&cond);
}


void TimeoutQueue::schedule_event(const Time &          trigger_time,
                                  TimeoutQueue::Event * event)
{
  assert(event != nullptr);

  event->set_trigger_time(trigger_time);
  schedule_event(event);
}


void TimeoutQueue::schedule_event(TimeoutQueue::Event * event)
{
  assert(event != nullptr);
  assert(EVENT_NSEC(event) >= 0 and EVENT_NSEC(event) < NSEC);

  CRITICAL_SECTION(mutex);

  ah_invalid_argument_if(event->get_execution_status() == Event::In_Queue)
      << "Event has already inserted in timemeQueue";

  if (isShutdown)
    return;

  event->set_execution_status(Event::In_Queue);

  prioQueue.insert(event);

  pthread_cond_signal(&cond);
}


bool TimeoutQueue::cancel_event(TimeoutQueue::Event* event)
{
  CRITICAL_SECTION(mutex);

  if (event->get_execution_status() != Event::In_Queue)
    return false;

  event->set_execution_status(Event::Canceled);

  prioQueue.remove(event);

  pthread_cond_signal(&cond);

  return true;
}


void TimeoutQueue::cancel_delete_event(Event *& event)
{
  if (event == nullptr)
    return;

  CRITICAL_SECTION(mutex);

  if (event->get_execution_status() == Event::In_Queue)
    prioQueue.remove(event);

  if (event->get_execution_status() == Event::Executing)
    event->set_execution_status(Event::To_Delete);
  else
    {
      event->set_execution_status(Event::Deleted);
      delete event;
    }

  event = nullptr;
  pthread_cond_signal(&cond);
}


void TimeoutQueue::reschedule_event(const Time &          trigger_time,
                                    TimeoutQueue::Event * event)
{
  assert(event != nullptr);

  CRITICAL_SECTION(mutex);

  if (event->get_execution_status() == Event::In_Queue)
    prioQueue.remove(event);

  event->set_trigger_time(trigger_time);

  event->set_execution_status(Event::In_Queue);

  prioQueue.insert(event);

  pthread_cond_signal(&cond);
}


void * TimeoutQueue::triggerEvent()
{
  Event *event_to_schedule;
  Event *event_to_execute;
  int status = 0;

  {
    CRITICAL_SECTION(mutex);

    while (true)
      {
        /* sleep if there is no events */
        while ((prioQueue.size() == 0) and (not isShutdown))
          pthread_cond_wait(&cond, &mutex);

        if (isShutdown)
          goto end; /* if shutdown is activated, get out */

        /* read the soonest event */
        event_to_schedule = static_cast<Event*>(prioQueue.top());

        /* compute time when the event must triggered */
        const Time & t = EVENT_TIME(event_to_schedule);

        do
          { /* sleep during t units of time, but be immune to signals
               interruptions (status will be EINTR in the case where the
               thread was signalized) */
            status = pthread_cond_timedwait(&cond, &mutex, &t);

            if (isShutdown)
              goto end;/* thread was signaled because shutdown was requested */

          } while (status == EINTR);

        if (status == ETIMEDOUT) /* soonest event could be executed */
          {     /* event to execute could be changed if it was canceled */
            event_to_execute = static_cast<Event*>(prioQueue.getMin());

            if (event_to_execute != event_to_schedule and
                EVENT_TIME(event_to_execute) > EVENT_TIME(event_to_schedule))
              continue; /* go to schedule another event */

            event_to_execute->set_execution_status(Event::Executing);

            critical_section.leave();

            try { event_to_execute->EventFct(); } /* execute event */
            catch (...) { /* Exceptions are only cauthg */ }

            critical_section.enter();

            if (event_to_execute->get_execution_status() == Event::To_Delete)
              {
                event_to_execute->set_execution_status(Event::Deleted);

                delete event_to_execute;
              }

            /* from this point, event cannot be longer acceded because
               it may delete itself */
          } /* end if (status == ETIMEDOUT) */
      } /* end while (1) */

  end: /* shutdown has been requested */

    /* extract all events from priority queue */
    while (prioQueue.size() > 0)
      static_cast<Event*>(prioQueue.getMin())->
        set_execution_status(Event::Canceled);
  } /* end of critical section */

  pthread_exit(nullptr);
}


void TimeoutQueue::shutdown()
{
  CRITICAL_SECTION(mutex);

  isShutdown = true;

  pthread_cond_signal(&cond);
}


void* TimeoutQueue::triggerEventThread(void *obj)
{
  TimeoutQueue *timeoutQueue = static_cast<TimeoutQueue*>(obj);

  return timeoutQueue->triggerEvent();
}
