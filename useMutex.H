
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/**
 * @file useMutex.H
 * @brief RAII-style wrapper for POSIX mutexes
 * 
 * This file provides utilities for working with POSIX mutexes in a
 * C++ RAII style, ensuring automatic lock release and simplifying
 * critical section management.
 * 
 * ## Features
 * 
 * - **RAII locking**: Automatic lock on construction, unlock on destruction
 * - **Explicit control**: Manual lock/unlock methods when needed
 * - **Unlock suppression**: Option to skip automatic unlock
 * - **Convenience macros**: Easy critical section definition
 * 
 * ## Classes
 * 
 * - `UseMutex`: Main RAII lock guard class
 * 
 * ## Utility Functions
 * 
 * - `init_mutex()`: Initialize a pthread mutex
 * - `destroy_mutex()`: Destroy a pthread mutex
 * 
 * ## Usage Example
 * 
 * ```cpp
 * pthread_mutex_t mutex;
 * init_mutex(&mutex);
 * 
 * // Method 1: RAII in block scope
 * {
 *     UseMutex guard(&mutex);
 *     // ... critical section ...
 * }  // Automatically unlocked
 * 
 * // Method 2: Using macro
 * {
 *     CRITICAL_SECTION(&mutex);
 *     // ... critical section ...
 * }
 * 
 * // Method 3: Manual control
 * UseMutex guard(&mutex);
 * guard.disallow_unlock();  // Don't unlock on destruction
 * // ... transfer to another context ...
 * 
 * destroy_mutex(&mutex);
 * ```
 * 
 * ## Macros
 * 
 * - `USE_MUTEX(name, mutex)`: Declare a named lock guard
 * - `CRITICAL_SECTION(mutex)`: Create an anonymous critical section
 * - `CTOR_USE_MUTEX(name, mutex)`: For member initializer lists
 * - `CTOR_INH_USE_MUTEX(mutex)`: For base class initialization
 * 
 * @note Consider using std::lock_guard or std::unique_lock for modern C++.
 * 
 * @see useCondVar.H For condition variable support
 * 
 * @author Leandro Rabindranath León *
 *  @ingroup Utilities

 */

# ifndef USEMUTEX_H
# define USEMUTEX_H

# include <pthread.h>
# include <errno.h>
# include <ahDefs.H>
# include <ahUtils.H>

# include <ah-errors.H>

/// Initialize a pthread mutex (pointer version)
extern void init_mutex(pthread_mutex_t *);

/// Initialize a pthread mutex (reference version)
extern void init_mutex(pthread_mutex_t &);

/// Destroy a pthread mutex (pointer version)
extern void destroy_mutex(pthread_mutex_t *);

/// Destroy a pthread mutex (reference version)
extern void destroy_mutex(pthread_mutex_t &);

using namespace Aleph;

/**
 * @class UseMutex
 * @brief RAII-style mutex lock guard
 * 
 * Automatically acquires a lock on construction and releases it on
 * destruction, unless explicitly disabled.
 * 
 * ## Thread Safety
 * 
 * This class wraps POSIX mutexes, providing the same thread safety
 * guarantees as the underlying pthread_mutex_t. *
 *  @ingroup Utilities

 */
class UseMutex
{
  pthread_mutex_t * mutex;
  bool              unlock_when_destroy;

public:

  /**
   * @brief Explicitly unlock the mutex
   * @throws std::domain_error if mutex pointer is null *
 *  @ingroup Utilities

   */
  void unlock()
  {
    ah_domain_error_if(mutex == nullptr)
        << "unlock: nullptr pointer to mutex";

    pthread_mutex_unlock(mutex);
  }

  /**
   * @brief Explicitly lock the mutex
   * @throws std::domain_error if mutex pointer is null *
 *  @ingroup Utilities

   */
  void lock()
  {
    ah_domain_error_if(mutex == nullptr)
        << "lock: nullptr pointer to mutex";

    pthread_mutex_lock(mutex);
  }

  /**
   * @brief Construct and lock (pointer version)
   * @param m Pointer to initialized mutex *
 *  @ingroup Utilities

   */
  UseMutex(pthread_mutex_t *m) : mutex(m), unlock_when_destroy(true)
  {
    lock();
  }

  /**
   * @brief Construct and lock (reference version)
   * @param m Reference to initialized mutex *
 *  @ingroup Utilities

   */
  UseMutex(pthread_mutex_t & m): mutex(&m), unlock_when_destroy(true)
  {
    lock();
  }

  /// Alias for lock()
  void enter() { lock(); }

  /// Alias for unlock()
  void leave() { unlock(); }

  /// Destructor - unlocks if allowed
  ~UseMutex()
  {
    if (unlock_when_destroy)
      unlock();
  }

  /// Prevent automatic unlock on destruction
  void disallow_unlock() { unlock_when_destroy = false; }

  /// Allow automatic unlock on destruction (default)
  void allow_unlock()  { unlock_when_destroy = true; }
};


/// @def CTOR_USE_MUTEX(name, mutex)
/// @brief For member initializer lists
# define CTOR_USE_MUTEX(name, mutex) name(mutex)

/// @def CTOR_INH_USE_MUTEX(mutex)
/// @brief For base class initialization in derived classes
# define CTOR_INH_USE_MUTEX(mutex) UseMutex(mutex)

/// @def USE_MUTEX(name, mutex)
/// @brief Declare a named UseMutex guard
# define USE_MUTEX(name, mutex) UseMutex name(mutex)

/// @def CRITICAL_SECTION(mutex)
/// @brief Create an anonymous critical section guard
# define CRITICAL_SECTION(mutex) UseMutex critical_section(mutex)

# endif // USEMUTEX_H



