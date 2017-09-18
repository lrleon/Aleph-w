# ifndef TPL_CON_QUEUE_H
# define TPL_CON_QUEUE_H

# include <mutex>
# include <condition_variable>
# include <tpl_dynListQueue.H>

using namespace std;

template <typename T>
class Concurrent_Queue
{
  mutex lck;
  condition_variable cond;
  DynListQueue<T> q;

public:

  T & put(const T & item)
  {
    lock_guard<mutex> critical_section(lck);
    auto & ref = q.put(item);
    cond.notify_one();
    return ref;
  }

  T & put(T && item)
  {
    lock_guard<mutex> critical_section(lck);
    auto & ref = q.put(forward<T>(item));
    cond.notify_one();
    return ref;
  }

  T & push(const T & item) { return put(item); }

  T & push(T && item) { return put(forward<T>(item)); }

  T get()
  {
    lock_guard<mutex> critical_section(lck);
    while (q.is_empty())
      cond.wait(lck);
    return q.get();
  }

  size_t size()
  {
    lock_guard<mutex> critical_section(lck);
    return q.size();
  }

  size_t is_empty()
  {
    lock_guard<mutex> critical_section(lck);
    return q.is_empty();
  }
};



# endif // TPL_CON_QUEUE_H
