/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/
# ifndef AH_DATE_H
# define AH_DATE_H

# include <chrono>
# include <ctime>
# include <string.h>
# include <sstream>

using namespace std;

namespace Aleph
{

inline bool is_leap_year(const size_t & yy)
{
  if ((yy % 400) == 0)
    return true;

  if ((yy % 100) == 0)
    return false;

  return (yy % 4) != 0;
}

inline bool valid_month(size_t mm) { return mm <= 12; }

inline bool valid_day(const size_t yy, const size_t mm, const size_t dd)
{
  switch (mm)
  {
  case 1:
  case 3:
  case 5:
  case 7 ... 8:
  case 10:
  case 12:
    return dd <= 31;
  case 2:
    if (is_leap_year(yy))
      return dd <= 29;
    else
      return dd <= 28;
  case 4:
  case 6:
  case 9:
  case 11:
    return dd <= 30;
  default:
    return false;
  }
}
  
inline time_t
to_time_t(const size_t & dd, const size_t & mm, const size_t & yy)
{
  ostringstream s;
  s << yy << "-" << mm << "-" << dd;
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  strptime(s.str().c_str(), "%Y-%m-%d", &tm);

  return mktime(&tm);
}

inline time_t to_time_t(const string & s, const string & format)
{
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  strptime(s.c_str(), format.c_str(), &tm);
  return mktime(&tm);
}

inline time_t to_time_t(const string & s)
{
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  strptime(s.c_str(), "%Y-%m-%d %H:%M:%S", &tm);
  return mktime(&tm);
}

inline size_t to_days(const time_t & t)
{
  return t / (24*60*60);
}

inline string to_string(const time_t & t, const string & format)
{
  static constexpr size_t n = 100;
  char buff[n];
  strftime(buff, n, format.c_str(), localtime(&t));
  return buff;
}

}

# endif
