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
#ifndef AH_DISPATCHER_H
#define AH_DISPATCHER_H

# include <iostream>
# include <tpl_dynMapTree.H>
# include <tpl_odhash.H>

template <typename Key, class Operation>
class AHDispatcher
{
  DynMapTree<Key, Operation> tbl;

public:

  AHDispatcher() {}

  template <typename ... Pairs>
  AHDispatcher(const Key & key, Operation op, Pairs ... pairs)
    : AHDispatcher(pairs...)
  {
    tbl.insert(key, op);
  }  

  void insert(const Key & key, Operation op)
  {
    tbl.insert(key, op);
  }

  template <typename ... Args>
  auto run(const Key & key, Args ... args) const
  {
    auto ptr = tbl.search(key);
    if (ptr == nullptr)
      {
	cerr << "Tried to run dispatcher with a key" 
	     << " which has not been registered" << endl;
	abort();
      }
    return (*ptr->second)(args...);
  }

  void remove(const Key & key)
  {
    tbl.remove(key);
  }

  bool valid_key(const Key & key) { return tbl.has(key); }

  DynList<string> keys() const { return tbl.keys(); }
};


template <typename Key, class Operation>
class AhHashDispatcher
{
  using P = pair<Key, Operation>;

  struct Equal
  {
    bool operator () (const P & p1, const P & p2) const
    {
      return p1.first == p2.first;
    }
  };

  static size_t fst_hash(const P & p) { return dft_hash_fct(p.first); }
  static size_t snd_hash(const P & p) { return snd_hash_fct(p.first); }

  ODhashTable<P, Equal> tbl;

public:

  void insert(const Key & key, Operation op)
  {
    tbl.insert(P(key, op));
  }

  template <typename ... Args>
  auto run(const Key & key, Args ... args) const
  {
    P p; p.first = key;
    return tbl.find(p).second (args...);
  }

  void remove(const Key & key)
  {
    tbl.remove(key);
  }
};




#endif
