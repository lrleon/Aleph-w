
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
# ifndef ARRAY_IT_H
# define ARRAY_IT_H

# include <cassert>
# include <tuple>
# include <aleph.H>
# include <ahDefs.H>
# include <htlist.H>

namespace Aleph {

template <typename T> class Array_Container;
  
  /** Wrapper que provee la interfaz de un iterador para un arreglo C++.

      @ingroups Secuencias
  */
    template <class T>
class Array_Iterator
{
  T * ptr = nullptr;
  long dim = 0;
  long num_items = 0;
  long idx = 0;
  long first = 0;
  long last = 0;
  long pos = 0;

public:

  using Item_Type = T;

  T * get_base() noexcept { return ptr; }

  Array_Iterator() noexcept {}

  Array_Iterator(T * p, size_t sz, size_t n)
    : ptr(p), dim(sz), num_items(n), last(n - 1)
  {
    assert(ptr);
     if (num_items > dim)
       throw std::domain_error("Array_Iterator(): num_items greater tan dim");
  }

  Array_Iterator(NoExceptionCtor, T * p, size_t sz, size_t n)
    : ptr(p), dim(sz), num_items(n), last(n - 1)
  {
    assert(ptr);
    if (num_items > dim)
      throw std::domain_error("Array_Iterator(): num_items greater tan dim");
  }

  Array_Iterator(T * p, size_t sz, size_t n, long f, long l)
    : ptr(p), dim(sz), num_items(n), idx(f), first(f), last(l)
  {
    assert(ptr);
     if (num_items > dim)
       throw std::domain_error("Array_Iterator(): num_items greater tan dim");
     if (first >= dim)
       throw std::domain_error("Array_Iterator(): first >= dim");
     if (last >= dim)
       throw std::domain_error("Array_Iterator(): last >= dim");
  }

  /** Constructor no throwing exception. 

      It is similar to the previous buy it is guaranteed that no
      exception will be thrown
   */
  Array_Iterator(NoExceptionCtor, T * p, size_t sz, size_t n, long f, long l)
    : ptr(p), dim(sz), num_items(n), idx(f), first(f), last(l)
  {
    assert(ptr);
  }

  Array_Iterator(const Array_Container<T> & c)
    : Array_Iterator(c.base(), c.capacity(), c.size()) {}

  bool has_curr() const noexcept { return pos >= 0 and pos < num_items; }

  bool has_current() const noexcept { return has_curr(); }

  bool is_last() const noexcept { return pos == num_items - 1; }

  long get_pos() const noexcept { return pos; }

  T & get_curr() const
  {
    if (pos < 0)
      throw std::underflow_error("MemArray::Iterator::get_curr(): has not current");

    if (pos >= num_items)
      throw std::overflow_error("MemArray::Iterator::get_curr(): has not current");

    return ptr[idx]; 
  }

  T & get_current() const { return get_curr(); }

  void next()
  {
    if (num_items == 0 or pos >= num_items)
      throw std::overflow_error("MemArray::Iterator::next(): has not current");
    if (++idx == dim)
      idx = 0;
    ++pos; 
  }

  void prev()
  {
    if (num_items == 0 or pos < 0)
      throw std::underflow_error("MemArray::Iterator::prev(): has not current");
    if (--idx < 0)
      idx = dim - 1;
    --pos; 
  }

  void reset() noexcept
  { 
    idx = first;
    pos = 0; 
  }

  void reset_first() noexcept { reset(); }

  void reset_last() noexcept
  { 
    idx = last; 
    pos = num_items - 1;
  }

  void end() noexcept
  {
    put_itor_at_the_end(*this);
  }
};

  template <typename T> inline
Array_Iterator<T> get_array_it(const T * array, size_t n)
{
  return Array_Iterator<T>(array, n, n);
}

  template <typename T> inline
Array_Container<T> get_array_it(const T * array, size_t n)
{
  return Array_Iterator<T>(array, n);
}


template <typename T>
class Array_Container 
  : public GenericTraverse<Array_Container<T>>,
    public LocateFunctions<Array_Container<T>, T>,
    public FunctionalMethods<Array_Container<T>, T>,
    public GenericKeys<Array_Container<T>, T>,
    public EqualToMethod<Array_Container<T>>,
    public StlAlephIterator<Array_Container<T>>
{
  T * base = nullptr;
  const size_t n = 0;

public:

  T * get_base() const noexcept { return  base; }

  Array_Container(T * base_ptr, const size_t d) : base(base_ptr), n(d) 
  {
    // empty
  }

  bool is_empty() const noexcept { return n == 0; }

  size_t size() const noexcept { return n; }

  size_t capacity() const noexcept { return n; }

  T & get_first() const
  {
    if (n == 0)
      throw std::underflow_error("Array_Container::get_first(): n == 0");
    return base[0];
  }

  T & get_last() const
  {
    if (n == 0)
      throw std::underflow_error("Array_Container::get_first(): n == 0");
    return base[n - 1];
  }

  struct Iterator : public Array_Iterator<T>
  {
    Iterator(const Array_Container & c) : Array_Iterator<T>(c.base, c.n, c.n) {}
  };

  Iterator get_it() const { return Iterator(*this); }
};

} // end namespace Aleph





# endif // ARRAY_IT_H
