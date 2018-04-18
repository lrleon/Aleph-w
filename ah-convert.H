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

# ifndef AH_CONVERT_H
# define AH_CONVERT_H 1

# include <tpl_agraph.H>
# include <tpl_dynArray.H>

template <class T>
DynList<T> to_DynList(const Array<T> & a)
{
  return a.template maps<T>([] (const auto & item) { return item; });
}

template <class C>
Array<typename C::Item_Type> to_Array(const C & c)
{
  Array<typename C::Item_Type> ret;
  for (auto it = c.get_it(); it.has_curr(); it.next())
    ret.append(it.get_curr());
  return ret;
}

# endif // AH_CONVERT_H


