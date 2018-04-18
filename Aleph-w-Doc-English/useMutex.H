
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
  @file useMutex.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef USEMUTEX_H
# define USEMUTEX_H

# include <autosprintf.h>
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
    if (mutex == NULL)
      throw std::domain_error("unlock: NULL pointer to mutex");

    pthread_mutex_unlock(mutex); 
  }

  void lock() 
  {
    if (mutex == NULL)
      throw std::domain_error("lock: NULL pointer to mutex");

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
