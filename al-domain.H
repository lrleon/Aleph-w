
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
# ifndef AL_DOMAIN_H
# define AL_DOMAIN_H

# include <tpl_hash.H>

namespace Aleph
{

  template <typename T = int>
struct AlDomain : public Aleph::HashSet<T, SetODhash>
{
  using Base = Aleph::HashSet<T, SetODhash>;
  using Base::Base;

  template <template <typename> class Container = DynList>
  Container<T> to_list() const
  {
    Container<T> ret_val;
    this->for_each(/* Lambda */ [&ret_val] (const T & item)
	     {
	       ret_val.append(item);
	     });
    return ret_val;    
  }

  std::string to_str() const
  {
    return sort(to_list()).template foldl<std::string> 
      ("", /* Lambda */ [] (const std::string & s, const T & item)
       {
	 return s + " " + Aleph::to_str(item);
       });
  }
};

template <typename T> inline
ostream & operator << (ostream & s, const AlDomain<T> & dom)
{
  return s << dom.to_str();
}

struct IntRange : public AlDomain<int>
{
  IntRange() = delete; 

  IntRange(int start, int end,  int step = 1)
  {
    if (step < 0)
      throw std::domain_error("negative step");

    cout << "step = " << step << endl;
    for (long i = start; i <= end; i += step)
      insert(i);
  }

  IntRange(size_t n)
  {
    for (int i = 0; i < n; ++i)
      insert(i);
  }

};

} // end namespace Aleph

# endif // AL_DOMAIN_H
