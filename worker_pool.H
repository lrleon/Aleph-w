# ifndef WORKER_POOL_H
# define WORKER_POOL_H

# include <thread>
# include <mutex>
# include <condition_variable>
# include <memory>

# include <ah-errors.H>
# include <tpl_dynListQueue.H>

using namespace std;
using namespace Aleph;

template <class ParsDesc>
class WorkersSet
{
public:

  using WorkerFct = bool (*)(ParsDesc & pars_ptr);

private:

  WorkerFct worker_fct = nullptr;
  size_t num_threads = 0;
  mutex m;
  condition_variable cond;

  size_t num_workers = 0;

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
          {
            if (num_workers == 0)
              cond.notify_one();
            continue;
          }

        ParsDesc * pars_ptr = q.get();

        lock.unlock();

        try
          {
            shut_down = (*worker_fct)(*pars_ptr);
          }
         catch (...) { /* Exceptions are only caught */ }

        lock.lock();
        --num_workers;

        if (shut_down)
          {
            cond.notify_all();
            return;
          }
      }
  }
  
public:
  
  WorkersSet(WorkerFct worker_fct, const size_t n = 16)
    : worker_fct(worker_fct), num_threads(n)
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

  void prepare_num_workers(size_t n)
  {
    unique_lock<mutex> lock(m);
    num_workers = n;
  }

  void call(ParsDesc & pars)
  {
    unique_lock<mutex> lock(m);
    q.put(&pars);
    ++num_workers;
    cond.notify_one();
  }

  bool is_working() const
  {
    unique_lock<mutex> lock(mutex);
    return not q.is_empty();
  }

  void wait_until_works_done()
  {
    unique_lock<mutex> lock(m);
    cond.wait(lock, [this] { return num_workers == 0; });

    ah_domain_error_if(num_workers > 0)
      << "Inconsistency: " << num_workers << " workers still pending";
  }
};


# endif
