
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

# ifndef AHSINGLETON_H
# define AHSINGLETON_H

# define Make_Singleton(name)                   \
  public:                                       \
                                                \
  static const name & get_instance()            \
  {                                             \
    static name instance;                       \
    return instance;                            \
  }                                             \
                                                \
  name(const name&) = delete;                   \
  void operator = (const name&) = delete;

                                                
class Singleton
{
public:

  static Singleton & get_instance()
  {
    static Singleton instance;
    return instance;
  }
  
private:
  
  Singleton() {};

public:
  
  Singleton(Singleton const&) = delete;
  void operator = (Singleton const&) = delete;
};

# endif // AHSINGLETON_H
