
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
#ifndef AH_MAPPING_H
#define AH_MAPPING_H

# include <iostream>
# include <ah-errors.H>
# include <tpl_dynMapTree.H>
# include <tpl_odhash.H>

template<typename Key, class ValueType>
class AHMapping
{
  DynMapTree<Key, ValueType> tbl;

public:

  AHMapping()
  {}

  template<typename ... Pairs>
  AHMapping(const Key &key, const ValueType &value, Pairs ... pairs)
    : AHMapping(pairs...)
  {
    tbl.insert(key, value);
  }

  AHMapping(const AHMapping &mapping)
    : tbl(mapping.tbl)
  {}

  void insert(const Key &key, ValueType value)
  {
    tbl.insert(key, value);
  }

  ValueType operator[](const Key &key) const
  {
    auto ptr = tbl.search(key);
    ah_domain_error_if(ptr == nullptr) << "key not found";
    return ptr->second;
  }

  AHMapping<ValueType, Key> inverse() const
  {
    AHMapping<ValueType, Key> ret;
    for (auto it = tbl.get_it(); it.has_curr(); it.next())
      {
        auto curr = it.get_curr();
        ret.insert(curr.second, curr.first);
      }

    return ret;
  }

  void remove(const Key &key)
  {
    tbl.remove(key);
  }

  bool valid_key(const Key &key)
  { return tbl.has(key); }

  DynList<string> keys() const
  { return tbl.keys(); }
};

#endif
