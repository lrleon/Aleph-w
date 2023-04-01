# ifndef WORKER_POOL_H
# define WORKER_POOL_H

# include <thread>
# include <mutex>
# include <condition_variable>
# include <memory>

# include <ah-errors.H>
# include <tpl_arrayQueue.H>

using namespace std;
using namespace Aleph;

class WorkersSet
{
public:

  using WorkerFct = bool (*)(void*);

private:

  WorkerFct worker_fct = nullptr;
  size_t num_threads = 0;
  mutex m;
  condition_variable cond;

  atomic<size_t> num_workers = 0;

  FixedQueue<void*> q;
  DynList<thread> threads;
  bool shut_down = false;
  bool job_done = false;

  condition_variable job_done_cond;
  mutex job_done_mutex;

  void worker_handler()
  {
    unique_lock<mutex> lock(m);
    while (true)
      {
        cond.wait(lock, [this] { return num_workers > 0 or shut_down; });
         if (shut_down)
          return;

         while (num_workers > 0)
           {
             cond.wait(lock,
                       [this]
                       {
                         return (not q.is_empty()) or shut_down;
                  });
             
             if (shut_down)
               return;

             if (not q.is_empty())
               {
                 void * pars_ptr = q.get();

                 if (not job_done)
                   {
                     lock.unlock();
                     try
                       {
                         job_done = (*worker_fct)(pars_ptr);
                       }
                     catch (exception & e)
                       {
                         cout << "Warning: workers exception "
                              << e.what() << endl;
                       }
                     lock.lock();
                     free(pars_ptr);
                   }
                 --num_workers;
               }
           }

         assert(q.is_empty());
         job_done_cond.notify_one();
         continue;
      }
  }
  
public:
  
  WorkersSet(WorkerFct worker_fct,
             const size_t qsize, const size_t n = 16)
    : worker_fct(worker_fct), num_threads(n), q(qsize)
  {
    for (size_t i = 0; i < n; ++i)
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

  // schedule a call to a worker with parameters contained in
  // pars_ptr. After to have called the workers, the run-time will
  // execute a delete on pars_ptr. The client is responsible of
  // allocating pars_ptr as well as to guarantee its ocnsistency
  // respect the expected values
  void schedule_call(void * pars_ptr)
  {
    unique_lock<mutex> lock(m);
    q.put(pars_ptr);
    cond.notify_one();
  }

  bool is_jobs_done() const
  {
    unique_lock<mutex> lock(mutex);
    return job_done;
  }

  void wait_until_all_workers_finished_or_job_is_done()
  {
    {
      unique_lock<mutex> lock(job_done_mutex);
      job_done_cond.wait(lock);
    }
  }
};


# endif
