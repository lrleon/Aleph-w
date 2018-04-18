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

# ifndef TPL_HASH_H
# define TPL_HASH_H

# include <initializer_list>
# include <tpl_dynSetHash.H>
# include <tpl_dynSetOhash.H>

namespace Aleph 
{

template <typename Key,
	  template <typename, class> class HashSetTable = ODhashTable,
	  class Cmp                                     = Aleph::equal_to<Key>>
struct HashSet : public HashSetTable<Key, Cmp>
{
  typedef HashSetTable<Key, Cmp> Base;
  using Base::Base;

  void add (const DynList<Key> & l)
  {
    l.for_each([this] (const Key & k)
	       {
		 this->insert(k);
	       });
  }

  Special_Ctors(HashSet, Key);

  HashSet() : Base() {}
};

template 
<typename Key, typename Data,
 template <typename, typename, class> class HashMapTable = MapODhash,
 class Cmp = Aleph::Dft_Pair_Cmp<Key, Data>>
struct HashMap : public HashMapTable<Key, Data, Cmp>
{
  typedef HashMapTable<Key, Data, Cmp> Base;
  using Base::Base;

  void add (std::initializer_list<Key> lk, std::initializer_list<Data> ld)
  {
    if (lk.size() != ld.size())
      throw std::range_error("size mismatch between domain and range");

    auto itk = lk.begin();
    auto itd = ld.begin();
    for (; itk != lk.end(); itk++, itd++)
      this->insert(*itk, *itd);
  }

  HashMap() : Base() { }

  HashMap(std::initializer_list<Key> lk, std::initializer_list<Data> ld) 
    : Base()
  {
    add(lk, ld);
  }

  template <template <typename T> class Container = DynList>
  HashMap(const Container<Key> & c, std::initializer_list<Data> ld) : Base()
  {
    if (c.size() != ld.size())
      throw std::range_error("size mismatch between domain and range");

    auto itd = ld.begin();
    c.for_each(/* Lambda */ [this, &itd] (const Key & key)
	       {
		 set_entry(key, *itd++); 
	       });
  }

  Generate_Proxy_Operator(HashMap);
};




} // end namespace Aleph


# endif //  TPL_HASH_H
