
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
