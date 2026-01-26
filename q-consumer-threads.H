
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


/** @file q-consumer-threads.H
 *  @brief Queue-based consumer thread pool.
 *
 *  Pool of threads consuming from a shared queue for
 *  producer-consumer patterns.
 *
 *  ## Features
 *  - Thread-safe queue
 *  - Configurable pool size
 *  - Graceful shutdown
 *
 *  @see worker_pool.H Alternative thread pool
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# ifndef Q_CONSUMER_THREADS_H
# define Q_CONSUMER_THREADS_H

# include <thread>
# include <condition_variable>
# include <mutex>
# include <tpl_dynListQueue.H>
# include <ah-errors.H>

template <typename T>
class ConsumerQueueEvent
{
  enum class Status { Ready, Executing };

  std::atomic<Status> status = { Status::Ready };

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

    @ingroup Utilities
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

  std::mutex lck;
  std::condition_variable cond;
  DynList<ConsumerQueueEvent*> event_list;
  size_t num_threads = 0;
  size_t num_active = 0;
  bool suspended = true;
  bool shutting_down = false;
  bool shutdown_done = false;
  std::condition_variable shutdown_cv;

  DynListQueue<T> q;

public:

  ~QueueTheadsPool()
  {
    std::unique_lock<std::mutex> cs(lck);
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
    std::unique_lock<std::mutex> critical_section(lck);

    auto th = std::thread(handler, this, event);
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
        std::unique_lock<std::mutex> critical_section(lck);
        if (num_threads == 0)
          ah_domain_error_if(true) << "there is no any thread created";
        q.put(std::forward<T>(item));
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
    std::unique_lock<std::mutex> critical_section(lck);
    if (shutting_down)
      return;

    suspended = false;
    cond.notify_all();
  }

  void suspend()
  {
    std::unique_lock<std::mutex> critical_section(lck);
    if (shutting_down)
      return;

    suspended = true;
    cond.notify_all();
  }

  bool is_suspended()
  {
    std::unique_lock<std::mutex> critical_section(lck);
    return suspended;
  }

  bool is_shutdown()
  {
    std::unique_lock<std::mutex> critical_section(lck);
    return shutdown_done;
  }

  void shutdown()
  {
    std::unique_lock<std::mutex> critical_section(lck);
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
  std::unique_lock<std::mutex> critical_section(lck);

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
