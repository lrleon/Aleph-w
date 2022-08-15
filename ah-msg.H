
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

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

# ifndef AH_MSG_H
# define AH_MSG_H

# include <sstream>

namespace Aleph
{

inline void __build_msg(std::ostringstream &) {}

template <typename T, class... Args> static inline
void __build_msg(std::ostringstream & s, const T & p, const Args &... args)
{
  s << p;
  __build_msg(s, args...);
}

template <class... Args> inline
std::string build_string(const Args & ... args)
{
  std::ostringstream s;
  __build_msg(s, args...);
  return s.str();
}

template <class... Args> inline
std::string build_error_msg(const Args & ... args)
{
  return build_string(args...);
}

}
# endif
