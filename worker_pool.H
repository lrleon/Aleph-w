# ifndef WORKER_POOL_H
# define WORKER_POOL_H

# include <thread>
# include <mutex>
# include <condition_variable>
# include <memory>

# include <tpl_dynListQueue.H>

using namespace std;
using namespace Aleph;

template <class ParsDesc>
class WorkersSet
{
public:

  using WorkerFct = bool (*)(ParsDesc & pars_ptr);

private:

  WorkerFct worker_fct = nullptr;  size_t num_workers = 0;
  mutex m;
  condition_variable cond;
  DynListQueue<ParsDesc*> q;
  DynList<thread> threads;
  bool shut_down = false;

  void worker_handler()
  {
    unique_lock<mutex> lock(m);
    while (true)
      {
        cond.wait(lock);
        if (shut_down)
          return;

        if (q.is_empty())
          continue;

        ParsDesc * pars_ptr = q.get();

        lock.unlock();

        try
          {
            shut_down = (*worker_fct)(*pars_ptr);
          }
         catch (...) { /* Exceptions are only cauthg */ }

        lock.lock();

        if (shut_down)
          {
            cond.notify_all();
            return;
          }
      }
  }
  
public:
  
  WorkersSet(WorkerFct worker_fct, const size_t n = 16)
    : worker_fct(worker_fct), num_workers(n)
  {
    for (int i = 0; i < n; ++i)
      threads.append(thread(&WorkersSet::worker_handler, this));
  }

  void shutdown()
  {
    unique_lock<mutex> lock(m);
    shut_down = true;
    cond.notify_all();
  }

  ~WorkersSet()
  {
    shutdown();
    threads.mutable_for_each([] (thread & th) { th.join(); });
  }

  void call(ParsDesc & pars)
  {
    unique_lock<mutex> lock(m);
    q.put(&pars);
    cond.notify_one();
  }
};


# endif
