

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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
