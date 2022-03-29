

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

  template <typename Key, typename Data,
            class    Cmp = Aleph::equal_to<Key>>
  using DynMapHash = DynMapHashTable<Key, Data, LhashTable, Cmp>;


  // Implementation de coming from ahFunctional.H

  template <typename T, template <typename> class Container> inline
  DynList<T> join(const Container<T> & c1, const Container<T> & c2)
  {
    DynSetLhash<T> table(c1);
    c2.for_each([&table] (const T & item)
                {
                  table.insert(item);
                });
    return table.keys();
  }

  template <typename T, template <typename> class Container = DynList> inline
  DynList<T> intercept(const Container<T> & c1, const Container<T> & c2)
  {
    DynSetLhash<T> set1 = c1;
    DynSetLhash<T> set2 = c2;
    return set1.filter([&set2] (const T & i) { return set2.contains(i); });
  }

  template <typename T, template <typename> class Container> inline
  DynList<T> unique(const Container<T> & c)
  {
    return DynSetLhash<T>(c).keys();
  }

  template <typename T, template <typename> class Container> inline
  DynList<T> repeated(const Container<T> & c)
  {
    DynList<T> ret;
    DynSetLhash<T> table;
  
    c.for_each([&table, &ret] (const T & i)
               {
                 auto * ptr = table.insert(i);
                 if (ptr == nullptr)
                   ret.append(i);
               });

    return ret;
  }

  template <typename T, template <typename> class Container> inline
  DynList<std::pair<T, size_t>> repeated_with_index(const Container<T> & c)
  {
    DynList<std::pair<T, size_t>> ret;
    DynSetLhash<T> table;
  
    size_t i = 0;
    c.for_each([&table, &ret, &i] (const T & item)
               {
                 auto * ptr = table.insert(item);
                 if (ptr == nullptr)
                   ret.append(std::pair<T, size_t>(item, i));
                 ++i;
               });

    return ret;
  }

} // end namespace Aleph

# endif // TPL_DYNSETHASH_H




