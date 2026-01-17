
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

 */

/*
  Aleph Distributed System License & Copyright Note

  Copyright (c) 1999, 2000
  UNIVERSIDAD DE LOS ANDES (ULA) Merida - VENEZUELA
  All rights reserved.

  - Centro de Estudios en Microcomputacion y Sistemas Distribuidos (CEMISID)
  - Centro Nacional de Calculo Cientifico de la ULA (CECALCULA)

  PERMISSION TO USE, COPY, MODIFY AND DISTRIBUTE THIS SOFTWARE AND ITS
  DOCUMENTATION IS HEREBY GRANTED, PROVIDED THAT BOTH THE COPYRIGHT
  NOTICE AND THIS PERMISSION NOTICE APPEAR IN ALL COPIES OF THE
  SOFTWARE, DERIVATIVE WORKS OR MODIFIED VERSIONS, AND ANY PORTIONS
  THEREOF, AND THAT BOTH NOTICES APPEAR IN SUPPORTING DOCUMENTATION.

  LibAleph and Aleph are distributed in the hope that it will be
  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  UNIVERSIDAD DE LOS ANDES requests users of this software to return to

  Leandro Leon  ( lrleon@cemisid.ing.ula.ve )
  CEMISID
  Ed La Hechicera
  3er piso, ala Norte
  Facultad de Ingenieria
  Univerisdad de Los Andes
  Merida - VENEZUELA

  or to     software@cemisid.ing.ula.ve

  any improvements or extensions that they make and grant Universidad
  de Los Andes (ULA) the rights to redistribute these changes.

  Aleph is (or was) granted by:
  - Consejo de Desarrollo Cientifico, Humanistico, Tecnico de la ULA  (CDCHT)
  - Consejo Nacional de Investigaciones Cientificas y Tecnologicas (CONICIT)

  This file is part of Alephlib, the container of data structures and
  algorithms used in the Aleph distributed system
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

