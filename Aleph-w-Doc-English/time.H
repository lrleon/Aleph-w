
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
  @file time.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TIME_H
# define TIME_H

# include <sys/time.h>

typedef struct timespec Time;


# define MSEC 1000
# define USEC 1000000
# define NSEC 1000000000


# define EVENT_TIME(e) ((e)->get_key())


# define EVENT_SEC(e) (EVENT_TIME(e).tv_sec)
# define EVENT_NSEC(e) (EVENT_TIME(e).tv_nsec)

     
inline long msec_to_nsec(const long & msec)
{
  assert(msec >= 0 and msec < MSEC);

  return msec*USEC;
}

     
inline long usec_to_nsec(const long & usec)
{
  assert(usec >= 0 and usec < USEC);

  return usec*MSEC;
}

     
inline char * time_to_char(const Time & t, char * str)
{
  sprintf(str, "(%ld sec, %ld nsec)", t.tv_sec, t.tv_nsec);

  return str;
}

     
inline Time timeval_to_time(const struct timeval & current_time)
{
  assert(current_time.tv_usec >= 0 and current_time.tv_usec < USEC);

  Time ret_val;
  ret_val.tv_sec = current_time.tv_sec;
  ret_val.tv_nsec = usec_to_nsec(current_time.tv_usec);

  assert(ret_val.tv_nsec >= 0 and ret_val.tv_nsec < NSEC);

  return ret_val;
}

     
inline Time read_current_time()
{
  struct timeval current_time;
  gettimeofday(&current_time, NULL);

  return timeval_to_time(current_time);
}

     
inline Time time_plus_msec(const Time & current_time, 
				 const int &  msec)
{
  assert(current_time.tv_nsec >= 0 and current_time.tv_nsec < NSEC);

  const long sec = msec/MSEC; /* Compute seconds inside msec */

  const long remain = msec % MSEC;

  const long total_nsec = current_time.tv_nsec + msec_to_nsec(remain);

  Time _t;
  _t.tv_sec  = current_time.tv_sec + sec + total_nsec/NSEC;
  _t.tv_nsec = total_nsec%NSEC;

  assert(_t.tv_nsec >= 0 and _t.tv_nsec < NSEC);

  return _t;
}

     
inline bool operator == (const Time & l, const Time & r)
{
  assert(l.tv_nsec >= 0 and l.tv_nsec < NSEC);
  assert(r.tv_nsec >= 0 and r.tv_nsec < NSEC);

  return l.tv_sec == r.tv_sec and l.tv_nsec == r.tv_nsec;
}

     
inline bool operator < (const Time & l, const Time & r)
{
  assert(l.tv_nsec >= 0 and l.tv_nsec < NSEC);
  assert(r.tv_nsec >= 0 and r.tv_nsec < NSEC);

  if (l.tv_sec not_eq r.tv_sec)
    return l.tv_sec < r.tv_sec;

  return l.tv_nsec < r.tv_nsec;
}


     
  inline bool operator <= (const Time & l, const Time & r)
{
  assert(l.tv_nsec >= 0 and l.tv_nsec < NSEC);
  assert(r.tv_nsec >= 0 and r.tv_nsec < NSEC);

  if (l.tv_sec not_eq r.tv_sec)
    return l.tv_sec < r.tv_sec; 

  return l.tv_nsec <= r.tv_nsec;
}


     
inline bool operator > (const Time & l, const Time & r)
{
  return not (l <= r);
}


     
inline bool operator >= (const Time& l, const Time& r)
{
  return not (l < r);
}


# endif //  TIME_H
