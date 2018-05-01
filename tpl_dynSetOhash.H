
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
# ifndef TPL_DYNSETOHASH_H
# define TPL_DYNSETOHASH_H

# include <tpl_olhash.H>
# include <tpl_odhash.H>

namespace Aleph
{  
  /**

     @ingroup Hash
  */
  template <typename Key, typename Data, 
	    class Cmp = Aleph::Dft_Pair_Cmp<Key, Data, Aleph::equal_to<Key>>,
	    template <typename, class> class HashTable = ODhashTable>
    struct MapOpenHash : public HashTable<std::pair<Key, Data>, Cmp>
    {
      using Pair = std::pair<Key, Data>;
      using Base = HashTable<Pair, Cmp>;
      using Base::Base;
      using Base::insert;
      using Hash_Fct = std::function<size_t(const Key &)>;
      using Hash_Fct_Ptr = size_t (*) (const Key &);

      MapOpenHash(size_t len                  = Primes::DefaultPrime,
       		  Hash_Fct_Ptr first_hash_fct  = dft_hash_fct<Key>,
		  Hash_Fct_Ptr second_hash_fct = snd_hash_fct<Key>,
		  Cmp  cmp                     = Cmp(),
		  float lower_alpha            = hash_default_lower_alpha, 
		  float upper_alpha            = hash_default_upper_alpha,
		  bool with_resize             = true)
       	: Base(len, std::bind(pair_hash_fct<Key, Data, Hash_Fct>,
			      first_hash_fct, std::placeholders::_1),
	       std::bind(pair_hash_fct<Key, Data, Hash_Fct>,
			 second_hash_fct, std::placeholders::_1),
	       cmp, lower_alpha, upper_alpha, with_resize) {}

      static Pair * key_to_pair(Key * ptr) noexcept { return (Pair*) ptr; }

      static Pair * data_to_pair(Data * ptr) noexcept
      {
	Pair * zero = 0;
	return (Pair*) ((long) ptr - (long) &zero->second); 
      }

      static Data & get_data(const Key & key) noexcept
      {
	return key_to_pair(&const_cast<Key&>(key))->second;    
      }

      static const Key & get_key(Data * data_ptr) noexcept
      {
	return data_to_pair(data_ptr)->first;
      }  

      using Value_Type = Data;

      Pair * insert(const Key & key, const Data & data)
      {
	return this->Base::insert(Pair(key, data));
      }

      Pair * insert(const Key & key, Data && data)
      {
	return this->Base::insert(Pair(key, std::forward<Data>(data)));
      }

      Pair * insert(Key && key, Data && data)
      {
	return this->Base::insert(Pair(std::forward<Key>(key), 
				       std::forward<Data>(data)));
      }

      Pair * insert(Key && key, const Data & data)
      {
	return this->Base::insert(Pair(std::forward<Key>(key), data));
      }

      Pair * search(const Key & key) const noexcept
      {
	return this->Base::search(Pair(key, Data()));
      }

      Pair * search(Key && key) const noexcept
      {
	return this->Base::search(Pair(move(key), Data()));
      }

      bool has(const Key & key) const noexcept
      {
	return search(key) != nullptr;
      }

      bool has(Key && key) const noexcept
      {
	return search(move(key)) != nullptr;
      }

      bool contains(const Key & key) const noexcept { return has(key); }

      bool contains(Key && key) const noexcept { return has(move(key)); }

      Data & find(const Key & key)
      {
	return Base::find(Pair(key, Data())).second;
      }

      Data & find(Key && key)
      {
	return Base::find(Pair(move(key), Data())).second;
      }

      const Data & find(const Key & key) const
      {
	return Base::find(Pair(key, Data())).second;
      }

      const Data & find(Key && key) const
      {
	return Base::find(Pair(move(key), Data())).second;
      }

      Data & operator [] (const Key & key)
      { 
	return this->search_or_insert(Pair(key, Data()))->second;
      }

      const Data & operator [] (const Key & key) const
      {
	return this->find(key);
      }

      Data & operator [] (Key && key)
      { 
	return this->search_or_insert(Pair(move(key), Data()))->second;
      }

      const Data & operator [] (Key && key) const
      {
	return this->find(move(key));
      }

      void remove_by_data(Data & data)
      {
	Base::remove_ptr(data_to_pair(&data));
      }

      void remove(const Key & key)
      {
	Base::remove(Pair(key, Data()));
      }

      void remove(Key && key)
      {
	Base::remove(Pair(move(key), Data()));
      }

      using Iterator = typename Base::Iterator;

      DynList<Key> keys() const
      {
	return this->template maps<Key>([] (auto p) { return p.first; });
      }

      DynList<Key> values() const
      {
	return this->template maps<Data>([] (auto p) { return p.second; });
      }

      DynList<Data*> values_ptr()
      {
	DynList<Data*> ret;
	for (Iterator it(*this); it.has_curr(); it.next())
	  ret.append(&it.get_curr().second);
	return ret;
      }

      DynList<Pair*> items_ptr()
      {
	DynList<Pair*> ret;
	for (Iterator it(*this); it.has_curr(); it.next())
	  ret.append(&it.get_curr());
	return ret;
      }
    };

  template <typename Key, typename Data, 
	    class Cmp = Aleph::Dft_Pair_Cmp<Key, Data, Aleph::equal_to<Key>>>
    struct MapOLhash : public MapOpenHash<Key, Data, Cmp, OLhashTable>
    {
      using Base = MapOpenHash<Key, Data, Cmp, OLhashTable>;
      using Base::Base;
    };
  
  
  template <typename Key, typename Data, 
	    class Cmp = Aleph::Dft_Pair_Cmp<Key, Data, Aleph::equal_to<Key>>>
    struct MapODhash : public MapOpenHash<Key, Data, Cmp, ODhashTable>
    {
      using Base = MapOpenHash<Key, Data, Cmp, ODhashTable>;
      using Base::Base;
    };
  
} // end namespace Aleph

# endif // TPL_DYNSETOHASH_H
