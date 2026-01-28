
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

/**
 * @file useCondVar.H
 * @brief Wrapper class for POSIX condition variables
 * 
 * This file provides a simple C++ wrapper around pthread condition
 * variables, offering a cleaner interface for thread synchronization.
 * 
 * ## Features
 * 
 * - **Wait**: Block until signaled
 * - **Signal**: Wake one waiting thread
 * - **Broadcast**: Wake all waiting threads
 * 
 * ## Usage Example
 * 
 * ```cpp
 * pthread_mutex_t mutex;
 * pthread_cond_t cond;
 * 
 * pthread_mutex_init(&mutex, nullptr);
 * pthread_cond_init(&cond, nullptr);
 * 
 * // In waiting thread:
 * {
 *     UseMutex lock(&mutex);
 *     UseCondVar cv(&cond, &mutex);
 *     while (!condition_met)
 *         cv.wait();
 * }
 * 
 * // In signaling thread:
 * {
 *     UseMutex lock(&mutex);
 *     condition_met = true;
 *     UseCondVar cv(&cond, &mutex);
 *     cv.signal();
 * }
 * ```
 * 
 * @note Always hold the associated mutex when calling wait/signal/broadcast.
 * @note Consider using std::condition_variable for modern C++ code.
 * 
 * @see useMutex.H For the companion mutex wrapper
 * 
 * @author Leandro Rabindranath León *
 *  @ingroup Utilities
 *  @ingroup ThreadsAndConcurrency

 */


#ifndef USECONDVAR_H
#define USECONDVAR_H

#include <pthread.h>

/**
 * @class UseCondVar
 * @brief Wrapper class for POSIX condition variables
 * 
 * Provides a simple interface for pthread condition variable operations.
 * Does not own the condition variable or mutex - they must be managed
 * separately. *
 *  @ingroup Utilities

 */
class UseCondVar 
{
private:

  pthread_cond_t *cond;
  pthread_mutex_t *mutex;

public:

  /**
   * @brief Construct wrapper for condition variable and mutex
   * @param c Pointer to initialized condition variable
   * @param m Pointer to initialized mutex (must be held when waiting) *
 *  @ingroup Utilities

   */
  UseCondVar(pthread_cond_t *c, pthread_mutex_t *m)
  {
    mutex = m;
    cond = c;
  }

  /**
   * @brief Wait for the condition variable to be signaled
   * @return 0 on success, error code on failure
   * @note The mutex must be held when calling this function *
 *  @ingroup Utilities

   */
  int wait()
  {
    int retVal = pthread_cond_wait(cond, mutex);
    return retVal;
  }

  /**
   * @brief Signal one waiting thread
   * @return 0 on success, error code on failure *
 *  @ingroup Utilities

   */
  int signal()
  {
    int retVal = pthread_cond_signal(cond);
    return retVal;
  }
	
  /**
   * @brief Wake all waiting threads
   * @return 0 on success, error code on failure *
 *  @ingroup Utilities

   */
  int broadcast()
  {
    int retVal = pthread_cond_broadcast(cond);
    return retVal;
  }

  /// Destructor (does not destroy the condition variable)
  ~UseCondVar()
  {
    //nullptr;
  }
};
#endif

