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
# ifndef Q_CONSUMER_THREADS_H
# define Q_CONSUMER_THREADS_H

# include <thread>
# include <condition_variable>
# include <mutex>
# include <tpl_dynListQueue.H>
 # include <ah-errors.H>

using namespace std;

template <typename T>
class ConsumerQueueEvent
{
  enum class Status { Ready, Executing };

  atomic<Status> status = { Status::Ready };

public:

  T item; /// this is a item extracted from the queue

  /** Derived classes from this one implement the action that must
      performed when the item is extracted from the queue.

      After event execution this event return a bool. A `true` return
      value indicates that the thread can continue consuming others
      items form the queue. At the contrary, a `false` return value
      indicates that the thread must finish
  */
  bool run() = 0;
};

/** Pool of consumer threads form a queue.

    @ingroup utils
 */
template <typename T>
class QueueTheadsPool
{
public:

  class ConsumerQueueEvent
  {
    friend class QueueTheadsPool;

    enum class Status { Ready, Executing };

    Status status = Status::Ready;

  public:

    T item; /// this is a item extracted from the queue

    /** Derived classes from this one implement the action that must
	performed when the item is extracted from the queue.

	After event execution this event return a bool. A `true` return
	value indicates that the thread can continue consuming others
	items form the queue. At the contrary, a `false` return value
	indicates that the thread must finish
    */
    virtual void run() = 0;

    virtual ~ConsumerQueueEvent() {}
  };

  using Event = ConsumerQueueEvent;

private:

  mutex lck;
  condition_variable cond;
  DynList<ConsumerQueueEvent*> event_list;
  size_t num_threads = 0;
  size_t num_active = 0;
  bool suspended = true;
  bool shutting_down = false;
  bool shutdown_done = false;
  condition_variable shutdown_cv;

  DynListQueue<T> q;

public:

  ~QueueTheadsPool()
  {
    unique_lock<mutex> cs(lck);
    ah_domain_error_if(not shutdown_done)
      << "~QueueTheadsPool: shutdown not done";
  }

private:

  static void handler(QueueTheadsPool<T> * threads_pool,
                      ConsumerQueueEvent * event)
  {
    threads_pool->run_event(event);
  }

  inline void run_event(ConsumerQueueEvent * event);

public:

  void create_thread(ConsumerQueueEvent * event)
  {
    unique_lock<mutex> critical_section(lck);

    auto th = thread(handler, this, event);
    th.detach();

    // check if event is already owned by another thread
    ah_domain_error_if(event_list.exists([event] (auto e) { return e == event; }))
      << "Thread creation with an event owned by another";

    event_list.append(event);
    ++num_threads;
  }

  bool put(T && item)
  {
    try
      {
        unique_lock<mutex> critical_section(lck);
        if (num_threads == 0)
          ah_domain_error_if(true) << "there is no any thread created";
        q.put(forward<T>(item));
        if (not suspended)
          cond.notify_one(); // for waking up a thread
      }
    catch (...)
      {
        return false;
      }

    return true;
  }

  void resume()
  {
    unique_lock<mutex> critical_section(lck);
    if (shutting_down)
      return;

    suspended = false;
    cond.notify_all();
  }

  void suspend()
  {
    unique_lock<mutex> critical_section(lck);
    if (shutting_down)
      return;

    suspended = true;
    cond.notify_all();
  }

  bool is_suspended()
  {
    unique_lock<mutex> critical_section(lck);
    return suspended;
  }

  bool is_shutdown()
  {
    unique_lock<mutex> critical_section(lck);
    return shutdown_done;
  }

  void shutdown()
  {
    unique_lock<mutex> critical_section(lck);
    if (shutting_down)
      return;

    shutting_down = true;
    cond.notify_all();

    shutdown_cv.wait(critical_section, [this] { return num_threads == 0; } );
    shutdown_done = true;
  }
};


template <typename T>
void QueueTheadsPool<T>::run_event(ConsumerQueueEvent * event)
{
  unique_lock<mutex> critical_section(lck);

  while (true)
    {
      cond.wait(critical_section,
                [this] { return not ((q.is_empty() and not shutting_down) or
                                     suspended); });

      if (shutting_down)
        break;

      if (suspended or q.is_empty())
        continue;

      event->item = q.get();
      ++num_active;
      critical_section.unlock();

      try
        {
          event->status = ConsumerQueueEvent::Status::Executing;
          event->run();
          event->status = ConsumerQueueEvent::Status::Ready;

          critical_section.lock();
          --num_active;
        }
      catch (...)
        {
          event->status = ConsumerQueueEvent::Status::Ready;
          critical_section.lock();
          --num_active;
        }
    }

  --num_threads;
  shutdown_cv.notify_one();
}


# endif // Q_CONSUMER_THREADS_H
