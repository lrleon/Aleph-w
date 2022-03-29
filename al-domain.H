

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

# ifndef AL_DOMAIN_H
# define AL_DOMAIN_H

# include <tpl_hash.H>

namespace Aleph
{

  template <typename T = int>
class AlDomain : public Aleph::HashSet<T, SetODhash>
{
  mutable bool keys_build = false;
  mutable DynList<T> domain;

public:

  using Base = Aleph::HashSet<T, SetODhash>;
  using Base::Base;

  const DynList<T> & keys() const
  {
    if (not keys_build)
      {
        domain = sort(this->Base::keys());
        keys_build = true;
      }
    return domain;
  }

  DynList<T> to_list() const { return this->keys(); }

  std::string to_str() const
  {
    return sort(to_list()).template foldl<std::string>
      ("", [] (const std::string & s, const T & item)
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

    IntRange(int start, int end, int step = 1)
    {
      if (step < 0)
        throw std::domain_error("negative step");

      for (long i = start; i <= end; i += step)
        insert(i);
    }

    IntRange(size_t n)
    {
      for (size_t i = 0; i < n; ++i)
        insert(i);
    }

  };

} // end namespace Aleph

# endif // AL_DOMAIN_H
