
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file tpl_random_queue.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_RANDOM_QUEUE_H
# define TPL_RANDOM_QUEUE_H

# include <gsl/gsl_rng.h>
# include <tpl_dynArray.H>

namespace Aleph {

/** 
  @brief Random queue.

  @details Random_Set<T> implements a set of elements of type T
  whose extraction is random.

  It is used as a backend to generate the random numbers
  gnu gsl library for the tornado generator set low
  the seed that returns the call time() the time system
  construction.

  @ingroup Secuencias 
*/
    template <class T> 
class Random_Set
{
private:

  DynArray<T> array;
  gsl_rng * r;

public:

/** @brief The set type on which iterates. */
  typedef Random_Set Set_Type;

/** @brief The type of item returning get_current(). */
  typedef T Item_Type;

/** @brief Returns the random number generator. */
  gsl_rng * get_rng() const { return r; }
  
/** @brief Instancia una cola aleatoria. */
  Random_Set() : array(0), r(NULL)
  {
    r = gsl_rng_alloc (gsl_rng_mt19937);
    if (r == NULL)
      throw std::bad_alloc();

    gsl_rng_set(r, time(NULL) % gsl_rng_max(r));
  }

  ~Random_Set()
  {
    if (r != NULL)
      gsl_rng_free(r);
  }

/** @brief Inserts a random element to the tail.
Shoot bad_alloc if there is insufficient memory. */
  void put(const T & item) { array[array.size()] = item; }

/** @brief Randomly selects an item from the queue and removes it.
Returns copy of the deleted item.
Shoot underflow_error if the queue is empty. */
  T get()
  {
    if (array.size() == 0)
      throw std::underflow_error("Random set is empty");

    const size_t pos = gsl_rng_uniform_int(r, array.size()); /* Al azar */
    T ret_val = array.access(pos);
    array.access(pos) = array.access(array.size() - 1);
    array.cut(array.size() - 1);
    return ret_val;
  }

/** @brief Returns true if the random queue is empty. */
  bool is_empty() const { return array.size() == 0; }
};

} /* End namespace Aleph */

# endif // TPL_RANDOM_QUEUE_H
