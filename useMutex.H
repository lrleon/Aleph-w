# ifndef USEMUTEX_H
# define USEMUTEX_H

# include <pthread.h>
# include <errno.h>
# include <ahDefs.H>
# include <ahUtils.H>
 
extern void init_mutex(pthread_mutex_t *);

extern void init_mutex(pthread_mutex_t &);

extern void destroy_mutex(pthread_mutex_t *);

extern void destroy_mutex(pthread_mutex_t &);  

using namespace Aleph;

class UseMutex
{
  pthread_mutex_t * mutex;
  bool              unlock_when_destroy;

public:

  void unlock() 
  { 
    if (mutex == nullptr)
      throw std::domain_error("unlock: nullptr pointer to mutex");

    pthread_mutex_unlock(mutex); 
  }

  void lock() 
  {
    if (mutex == nullptr)
      throw std::domain_error("lock: nullptr pointer to mutex");

    pthread_mutex_lock(mutex); 
  }

  UseMutex(pthread_mutex_t *m) : mutex(m), unlock_when_destroy(true)
  {
    lock();
  }
	
  UseMutex(pthread_mutex_t & m): mutex(&m), unlock_when_destroy(true)
  {
    lock();
  }

  void enter() { lock(); }

  void leave() { unlock(); }
	
  ~UseMutex() throw(std::domain_error)
  {
    if (unlock_when_destroy)
      unlock();
  }

  void disallow_unlock() { unlock_when_destroy = false; }

  void allow_unlock()  { unlock_when_destroy = true; }
};


# define CTOR_USE_MUTEX(name, mutex) name(mutex)

# define CTOR_INH_USE_MUTEX(mutex) UseMutex(mutex)

# define USE_MUTEX(name, mutex) UseMutex name(mutex)

# define CRITICAL_SECTION(mutex) UseMutex critical_section(mutex)

# endif // USEMUTEX_H



