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
# ifndef AHTYPES_H
# define AHTYPES_H


namespace Aleph {

  template <typename T>
bool is_primitive_type() { return false; }

# define DECLARE_PRIMITIVE(T)			\
  template <>					\
  bool is_primitive_type <T> ()			\
  {						\
    return true;				\
  }

  DECLARE_PRIMITIVE(char);

  DECLARE_PRIMITIVE(unsigned char);

  DECLARE_PRIMITIVE(short);

  DECLARE_PRIMITIVE(unsigned short);

  DECLARE_PRIMITIVE(int);

  DECLARE_PRIMITIVE(unsigned int);

  DECLARE_PRIMITIVE(long long);

  DECLARE_PRIMITIVE(unsigned long long);

  DECLARE_PRIMITIVE(float);

  DECLARE_PRIMITIVE(double);

  DECLARE_PRIMITIVE(long double);


  template <class T> T & swap(T & t1, T & t2)
  {
    return t1.swap(t2);
  }

# define DECLARE_SWAP(T)			\
  template <> T & swap <T> (T & t1, T & t2)	\
  {						\
    T tmp = t1;					\
    t1 = t2;					\
    t2 = tmp;					\
    return t1;
  }

  DECLARE_SWAP(char);

  DECLARE_SWAP(unsigned char);

  DECLARE_SWAP(short);

  DECLARE_SWAP(unsigned shor);

  DECLARE_SWAP(int);

  DECLARE_SWAP(unsigned int);

  DECLARE_SWAP(long);

  DECLARE_SWAP(unsigned long);

  DECLARE_SWAP(long long);

  DECLARE_SWAP(unsigned long long);

  DECLARE_SWAP(float);

  DECLARE_SWAP(double);

  DECLARE_SWAP(long double);

} // end namespace Aleph

# endif // AHTYPES_H
