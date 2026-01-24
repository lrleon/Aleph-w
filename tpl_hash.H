
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/** @file tpl_hash.H
 *  @brief Unified hash table interface.
 *
 *  Provides HashMap and HashSet as convenient wrappers around
 *  DynSetHash and DynMapHash for STL-like hash container usage.
 *
 *  ## Features
 *  - Simple key-value and set interfaces
 *  - Automatic resizing
 *  - Configurable hash functions
 *
 *  @see tpl_dynSetHash.H Underlying implementation
 *  @ingroup Hash
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_HASH_H
# define TPL_HASH_H

# include <initializer_list>
# include <tpl_dynSetHash.H>
# include <tpl_dynMapOhash.H>
# include <ah-errors.H>

namespace Aleph
{
  template <typename Key,
            template <typename, class> class HashSetTable = ODhashTable,
            class Cmp = Aleph::equal_to<Key>>
  struct HashSet : public HashSetTable<Key, Cmp>
  {
    typedef HashSetTable<Key, Cmp> Base;
    using Base::Base;

    void add(const DynList<Key> & l)
    {
      l.for_each([this](const Key & k)
                   {
                     this->insert(k);
                   });
    }

    Special_Ctors(HashSet, Key);

    HashSet() : Base() {}

    Key &get_first() const { return this->get_it().get_curr(); }

    Key &get_last() const
    {
      auto it = this->get_it();
      it.reset_last();
      return it.get_curr();
    }
  };

  template
  <typename Key, typename Data,
   template <typename, typename, class> class HashMapTable = MapODhash,
   class Cmp = Aleph::equal_to<Key>>
  struct HashMap : public HashMapTable<Key, Data, Cmp>
  {
    typedef HashMapTable<Key, Data, Cmp> Base;
    using Base::Base;

    void add(std::initializer_list<Key> lk, std::initializer_list<Data> ld)
    {
      ah_range_error_if(lk.size() != ld.size())
        << "size mismatch between domain and range";

      auto itk = lk.begin();
      auto itd = ld.begin();
      for (; itk != lk.end(); ++itk, ++itd)
        this->insert(*itk, *itd);
    }

    HashMap() : Base() {}

    HashMap(std::initializer_list<Key> lk, std::initializer_list<Data> ld)
      : Base()
    {
      add(lk, ld);
    }

    template <template <typename T> class Container = DynList>
    HashMap(const Container<Key> & c, std::initializer_list<Data> ld) : Base()
    {
      ah_range_error_if(c.size() != ld.size())
        << "size mismatch between domain and range";

      auto itd = ld.begin();
      c.for_each(/* Lambda */ [this, &itd](const Key & key)
                   {
                     set_entry(key, *itd++);
                   });
    }

    Generate_Proxy_Operator(HashMap);
  };
} // end namespace Aleph


# endif //  TPL_HASH_H
