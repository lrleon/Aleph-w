

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

  template <typename T, template <typename> class Container,
            class Equal = std::equal_to<T>>
  std::pair<DynList<DynList<T>>, size_t>
  sequential_groups(const Container<T> & c, Equal && eq = Equal())
  {
    return sequential_groups(c, eq);
  }

  template <typename T, template <typename> class Container, class Equal>
  std::pair<DynList<T>, size_t>
  unique_sequential(const Container<T> & c, Equal & eq)
  {
    using P = std::pair<DynList<T>, size_t>;
    if (c.is_empty())
      return P(DynList<T>(), 0);

    DynList<T> ret;

    auto it = c.get_it();                    // put the first item
    auto & curr_item = it.get_curr();
    ret.append(curr_item);

    size_t count = 1; // count the number of groups

    for (it.next(); it.has_curr(); it.next_ne())
      {
        auto & curr = it.get_curr_ne();
        if (not eq(curr, curr_item)) // group change?
          {
            curr_item = curr;
            ret.append(curr_item);
            ++count; // increase the number of groups
          }
      }

    return P(ret, count);
  }

  template <typename T, template <typename> class Container,
            class Equal = std::equal_to<T>>
  std::pair<DynList<T>, size_t>
  unique_sequential(const Container<T> & c, Equal && eq = Equal())
  {
    return unique_sequential(c, eq);
  }

  template <class Itor1, class Itor2 = Itor1>
  class Pair_Iterator
  {
    Itor1 it1;
    Itor2 it2;

  public:

    Pair_Iterator(Itor1 i1, Itor2 i2) : it1(i1), it2(i2) {}

    template <class C1, class C2>
    Pair_Iterator(const C1 & c1, const C2 & c2)
      : Pair_Iterator(c1.get_it(), c2.get.get_it()) {}

    bool has_curr() const noexcept { return it1.has_curr() and it2.has_curr(); }

    bool has_curr1() const noexcept { return it1.has_curr; }

    bool has_curr2() const noexcept { return it2.has_curr; }

    auto get_curr() const
    {
      return std::make_pair(it1.get_curr(), it2.get_curr());
    }

    auto get_curr_ne() const noexcept
    {
      return std::make_pair(it1.get_curr_ne(), it2.get_curr_ne());
    }

    void next()
    {
      it1.next();
      it2.next();
    }

    void next_ne() noexcept
    {
      it1.next_ne();
      it2.next_ne();
    }

    /// return true if the two iterator were completely traversed
    bool was_traversed() const noexcept
    {
      return not (it1.has_curr() or it2.has_curr());
    }
  };

  template <class C1, class C2>
  Pair_Iterator<typename C1::Iterator, typename C2::Iterator> inline
  get_pair_it(const C1 & c1, const C2 & c2)
  {
    using I1 = typename C1::Iterator;
    using I2 = typename C2::Iterator;
    I1 i1(c1);
    I2 i2(c2);
    return Pair_Iterator<I1, I2>(i1, i2);
  }

  template <class C1, class C2>
  Pair_Iterator<typename C1::Iterator, typename C2::Iterator> inline
  get_pair_it(const C1 & c1, const C2 & c2, size_t pos)
  {
    using I1 = typename C1::Iterator;
    using I2 = typename C2::Iterator;
    I1 i1(c1);
    I2 i2(c2);
    for (size_t i = 0; i < pos; ++i)
      {
        i1.next();
        i2.next();
      }
    return Pair_Iterator<I1, I2>(i1, i2);
  }

  template <class C> inline void insert_in_container(C &, size_t&) {}

  template <class C, typename T, typename ... Args> inline
  void insert_in_container(C & c, size_t & n, const T & item, Args & ... args)
  {
    c.insert(item);
    ++n;
    insert_in_container(c, n, args...);
  }

  template <class C, typename ... Args> inline
  size_t insert_in_container(C & c, Args ... args)
  {
    size_t n = 0;
    insert_in_container(c, n, args...);
    return n;
  }

  template <class C, typename ... Args>
  C build_container(Args ... args)
  {
    C c;
    append_in_container(c, args...);
    return c;
  }

  template <class SrcC, class TgtC>
  TgtC assign_container(const SrcC & srcc)
  {
    TgtC ret;
    for (auto it = srcc.get_it(); it.has_curr(); it.next_ne())
      ret.append(it.get_curr_ne());

    return ret;
  }

  template <typename T, typename ... Args>
  DynList<T> build_dynlist(Args ... args)
  {
    return build_container<DynList<T>>(args...);
  }

  template <class C> inline void remove_from_container(C &, size_t&) {}

  template <class C, typename T, typename ... Args> inline
  void remove_from_container(C & c, size_t & n, const T & item, Args & ... args)
  {
    c.remove(item);
    ++n;
    remove_from_container(c, n, args...);
  }

  template <class C, typename ... Args> inline
  size_t remove_from_container(C & c, Args ... args)
  {
    size_t n = 0;
    remove_from_container(c, n, args...);
    return n;
  }

  template <class C> inline void append_in_container(C &, size_t&) {}

  template <class C, typename T, typename ... Args> inline
  void append_in_container(C & c, size_t & n, const T & item, Args & ... args)
  {
    c.append(item);
    ++n;
    append_in_container(c, n, args...);
  }

  template <class C, typename ... Args> inline
  size_t append_in_container(C & c, Args ... args)
  {
    size_t n = 0;
    append_in_container(c, n, args...);
    return n;
  }

  // These function are defined in tpl_dynSetHash.H

  // union
  template <typename T, template <typename> class Container> inline
  DynList<T> join(const Container<T> & c1, const Container<T> & c2);

  template <typename T, template <typename> class Container> inline
  DynList<T> intercept(const Container<T> & c1, const Container<T> & c2);

  template <typename T, template <typename> class Container> inline
  DynList<T> unique(const Container<T> & c);

  template <typename T, template <typename> class Container> inline
  DynList<T> repeated(const Container<T> & c);

  template <typename T, template <typename> class Container> inline
  DynList<std::pair<T, size_t>> repeated_with_index(const Container<T> & c);

  template <typename T,
            template <typename> class C1,
            template <typename> class C2>
  DynList<T> flatten(const C2<C1<T>> & c)
  {
    DynList<T> ret;
    for (typename C2<C1<T>>::Iterator it_c = c.get_it(); it_c.has_curr();
         it_c.next_ne())
      {
        C1<T> & curr_c = it_c.get_curr_ne();
        for (typename C1<T>::Iterator it = curr_c.get_it(); it.has_curr(); it.next_ne())
          ret.append(it.get_curr_ne());
      }
    return ret;
  }

  template <typename T,
            template <typename> class C1,
            template <typename> class C2,
            template <typename> class C3>
  DynList<T> flatten(const C3<C2<C1<T>>> & c)
  {
    DynList<T> ret;
    for (typename C3<C2<C1<T>>>::Iterator it = c.get_it(); it.has_curr(); it.next_ne())
      ret.append(flatten(it.get_curr_ne()));
    return ret;
  }

  template <typename T,
            template <typename> class C1,
            template <typename> class C2,
            template <typename> class C3,
            template <typename> class C4>
  DynList<T> flatten(const C4<C3<C2<C1<T>>>> & c)
  {
    DynList<T> ret;
    for (typename C4<C3<C2<C1<T>>>>::Iterator it = c.get_it(); it.has_curr();
         it.next_ne())
      ret.append(flatten(it.get_curr_ne()));
    return ret;
  }

  template <typename T> inline
  bool is_inside(const T & val, const DynList<T> & values)
  {
    for (auto & v : values)
      if (val == v)
        return true;
    return false;
  }

  template <typename T>
  bool is_equal(const T &) { return false; }

  template <typename T, typename ... Args>
  bool is_equal(const T & val, const T & rhs, Args & ... args)
  {
    return val == rhs or is_equal(val, args...);
  }


} // end namespace Aleph

# endif // AH_FUNCTIONAL_H
