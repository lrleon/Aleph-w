
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


/** @file ah-time.H
 *  @brief Low-level time utilities using POSIX timespec.
 *
 *  Provides Time type and macros for time arithmetic using
 *  struct timespec. Includes conversion macros for milliseconds,
 *  microseconds, and nanoseconds.
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# ifndef AH_TIME_H
# define AH_TIME_H

# include <sys/time.h>
# include <stdio.h>

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


inline char * time_to_char(const Time & t, char * str, size_t str_size = 64)
{
  snprintf(str, str_size, "(%ld sec, %ld nsec)", t.tv_sec, t.tv_nsec);

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
  gettimeofday(&current_time, nullptr);

  return timeval_to_time(current_time);
}


inline Time time_plus_msec(const Time & current_time, 
				 const int &  msec)
{
  assert(current_time.tv_nsec >= 0 and current_time.tv_nsec < NSEC);

  const long sec = msec/MSEC; /* compute seconds inside msec */

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


# endif // AH_TIME_H
