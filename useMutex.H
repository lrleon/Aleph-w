
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

  ~UseMutex()
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



