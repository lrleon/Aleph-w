
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
#ifndef AH_FUNCTIONAL_H
#define AH_FUNCTIONAL_H

/** @file ahFunctional.H
    @brief Functional programming utilities for Aleph-w containers.

    This file contains various functional patterns like map, filter, fold,
    zip, range generation, and container manipulation helpers.

    @ingroup Algos
*/

# include <stdexcept>
# include <utility>
# include <tuple>
# include <functional>
# include <algorithm>
# include <ah-errors.H>

namespace Aleph
{
  /** @brief Abstract base class for optional-like results.
      @tparam T Item type.
  */
  template <typename T> struct Found_Item
  {
    virtual T & get_item() = 0;
    virtual const T & get_item() const = 0;
    virtual bool is_found() const noexcept = 0;
    virtual ~Found_Item() = default;
  };

  /** @brief Represents a missing value. */
  template <typename T> struct None : public Found_Item<T>
  {
    [[noreturn]] virtual T & get_item() override
    {
      ah_invalid_argument() << "Access from None type";
      throw std::logic_error("Access from None type"); // unreachable but satisfies compiler
    }

    [[noreturn]] virtual const T & get_item() const override
    {
      ah_invalid_argument() << "Access from None type";
      throw std::logic_error("Access from None type");
    }

    virtual bool is_found() const noexcept override { return false; }
  };

  /** @brief Represents a found value (stored by reference). */
  template <typename T> struct Some : public Found_Item<T>
  {
    T & item;

    Some(T & i) : item(i) {}

    virtual T & get_item() override { return item; }
    virtual const T & get_item() const override { return item; }
    virtual bool is_found() const noexcept override { return true; }
  };

  /** @brief Default mapping operation (identity). */
  template <typename tgtT, typename srcT>
  struct Dft_Map_Op
  {
    tgtT operator () (const srcT & item) const noexcept { return static_cast<tgtT>(item); }
  };

  /** @brief Default folding operation (returns default-constructed accumulator). */
  template <typename TR, typename TD>
  struct Dft_Fold_Op
  {
    TR operator () (const TR & /*acc */, const TD & /* val */)
      const noexcept { return TR(); }
  };


  template <typename T> class DynList;

  /** @brief Generate a range of values [start, end] with a given step.

      @tparam T Numeric type.
      @tparam Container Container type (defaults to DynList).
      @param start Start value (inclusive).
      @param end End value (inclusive).
      @param step Increment between values.
      @return Container with the range.
  */
  template <typename T = int, template <typename> class Container = DynList>
  [[nodiscard]] inline Container<T> range(const T start, const T end, const T step = 1)
  {
    Container<T> ret_val;
    for (T i = start; i <= end; i += step)
      ret_val.append(i);
    return ret_val;
  }

  /** @brief Generate exactly @a n values evenly spaced between [start, end].

      @tparam T Numeric type.
      @tparam Container Container type (defaults to DynList).
      @param start Start value.
      @param end End value.
      @param n Number of values to generate.
      @return Container with @a n values.
      @throw std::domain_error if n < 1.
  */
  template <typename T = int, template <typename> class Container = DynList>
  [[nodiscard]] inline Container<T> nrange(const T start, const T end, const size_t n)
  {
    ah_domain_error_if(n == 0) << "nrange: n must be greater than 0";

    Container<T> ret_val;
    if (n == 1)
      {
        ret_val.append(start);
        return ret_val;
      }

    const auto step = static_cast<double>(end - start) / (n - 1);
    for (size_t i = 0; i < n; ++i)
      ret_val.append(static_cast<T>(start + i * step));

    return ret_val;
  }

  /** @brief Generate a range [start, end] and apply an operation to each value.

      @tparam T Numeric type.
      @tparam Container Result container type.
      @tparam Op Mapping operation.
      @param start Start value (inclusive).
      @param end End value (inclusive).
      @param step Increment.
      @param op Operation to apply.
      @return Container with the mapped values.
  */
  template <typename T = int, template <typename> class Container = DynList,
            class Op>
  [[nodiscard]] inline auto set_range(const T start, const T end, const T step, Op & op)
    -> Container<std::decay_t<decltype(op(start))>>
  {
    Container<std::decay_t<decltype(op(start))>> ret_val;
    for (T i = start; i <= end; i += step)
      ret_val.append(op(i));
    return ret_val;
  }

  /** @overload set_range */
  template <typename T = int, template <typename> class Container = DynList,
            class Op>
  [[nodiscard]] inline auto set_range(const T start, const T end,
                        const T step = 1, Op && op = Op())
    -> Container<std::decay_t<decltype(op(start))>>
  {
    return set_range<T, Container, Op>(start, end, step, op);
  }

  /** @brief Generate @a n contiguous values starting from @a start. */
  template <typename T = int, template <typename> class Container = DynList>
  [[nodiscard]] inline Container<T> contiguous_range(T start, const size_t n)
  {
    Container<T> ret_val;
    for (size_t i = 0; i < n; ++i)
      ret_val.append(start++);
    return ret_val;
  }


  /** @brief Return a range 0, 1, ... n - 1.

      @param[in] n size of range.
      @return a container with the range.

      @ingroup Algos
  */
  template <typename T = int, template <typename> class Container = DynList>
  [[nodiscard]] inline Container<T> range(const T n)
  {
    Container<T> ret_val;
    for (T i = 0; i < n; ++i)
      ret_val.append(i);
    return ret_val;
  }


  /** @brief Create a sequence of repeated items.

      @param[in] n number of items.
      @param[in] item the value to be repeated.
      @return a container with n instances of item value.

      @ingroup Algos
  */
  template <typename T = int> [[nodiscard]] inline
  DynList<T> rep(size_t n, const T & item)
  {
    DynList<T> ret_val;
    for (size_t i = 0; i < n; ++i)
      ret_val.append(item);
    return ret_val;
  }

  /** @overload rep */
  template <typename T = int> [[nodiscard]] inline
  DynList<T> rep(size_t n, T && item = T())
  {
    return rep<T>(n, item);
  }

  /** @brief Create a list of pointers to items in a container. */
  template <class Container> [[nodiscard]]
  DynList<typename Container::Item_Type*> pointers_list(Container & c)
  {
    DynList<typename Container::Item_Type*> ret;
    for (auto it = c.get_it(); it.has_curr(); it.next_ne())
      ret.append(&it.get_curr());
    return ret;
  }

  /** @overload pointers_list for const containers. */
  template <class Container> [[nodiscard]]
  DynList<const typename Container::Item_Type *>
  pointers_list(const Container & c)
  {
    using T = const typename Container::Item_Type *;
    DynList<T> ret;
    for (auto it = c.get_it(); it.has_curr(); it.next_ne())
      ret.append(&it.get_curr());
    return ret;
  }

  /** @brief Execute an operation @a n times. */
  template <class Op>
  void each(size_t start, size_t end, Op & op)
  {
    for (size_t i = start; i <= end; ++i)
      op();
  }

  /** @overload each */
  template <class Op>
  void each(size_t start, size_t end, Op && op)
  {
    each(start, end, op);
  }

  /** @overload each */
  template <class Op>
  void each(size_t n, Op & op)
  {
    if (n == 0) return;
    each(0, n - 1, op);
  }

  /** @overload each */
  template <class Op>
  void each(size_t n, Op && op)
  {
    each(n, op);
  }

  /** @brief Extract a sublist using a stride.

      @param c Source container.
      @param pos Start position.
      @param stride Number of elements to skip between picks.
      @return New list with picked elements.
  */
  template <class Container> [[nodiscard]]
  DynList<typename Container::Item_Type>
  sublist(const Container & c, size_t pos, size_t stride)
  {
    DynList<typename Container::Item_Type> ret;
    try
      {
        for (auto it = c.get_it(pos); it.has_curr();
             each(0, stride - 1, [&it] () { it.next(); }))
          ret.append(it.get_curr());
      }
    catch (const std::overflow_error &) { /* end of container reached */ }

    return ret;
  }

  /** @overload sublist from start. */
  template <class Container> [[nodiscard]]
  DynList<typename Container::Item_Type>
  sublist(const Container & c, size_t stride)
  {
    return sublist(c, 0, stride);
  }

  /** @brief Apply @a operation to each element in @a container. */
  template <class Container, class Operation> inline
  Container & for_each(Container & container, Operation & operation)
  {
    container.traverse([&operation] (const auto & item)
                       {
                         operation(item);
                         return true;
                       });
    return container;
  }

  /** @overload for_each for const containers. */
  template <class Container, class Operation> inline
  const Container & for_each(const Container & container, Operation & operation)
  {
    container.traverse([&operation] (const auto & item)
                       {
                         operation(item);
                         return true;
                       });
    return container;
  }

  /** @overload for_each with rvalue operation. */
  template <class Container, class Operation> inline
  Container& for_each(Container & container,
		      Operation && operation = Operation())
  {
    return for_each<Container, Operation>(container, operation);
  }

  /** @overload for_each with rvalue operation for const containers. */
  template <class Container, class Operation> inline const Container &
  for_each(const Container & container, Operation && operation = Operation())
  {
    return for_each<Container, Operation>(container, operation);
  }

  /** @brief Apply @a operation to each element and its index. */
  template <class Container, class Operation> inline
  void enum_for_each(const Container & container, Operation & operation)
  {
    size_t i = 0;
    for (auto it = container.get_it(); it.has_curr(); it.next_ne(), ++i)
      operation(it.get_curr(), i);
  }

  /** @overload enum_for_each with rvalue operation. */
  template <class Container, class Operation> inline
  void enum_for_each(const Container & container, Operation && operation)
  {
    enum_for_each(container, operation);
  }

  /** @brief Return true if all elements satisfy @a operation. */
  template <class Container, class Operation> inline
  bool all(Container & container, Operation & operation)
  {
    return container.traverse(operation);
  }

  /** @overload all for const containers. */
  template <class Container, class Operation> inline
  bool all(const Container & container, Operation & operation)
  {
    return container.template traverse<Operation>(operation);
  }

  /** @overload all with rvalue operation. */
  template <class Container, class Operation> inline
  bool all(Container & container, Operation && operation = Operation())
  {
    return all<Container, Operation>(container, operation);
  }

  /** @overload all with rvalue operation for const containers. */
  template <class Container, class Operation> inline
  bool all(const Container & container, Operation && operation = Operation())
  {
    return all<Container, Operation>(container, operation);
  }


  /** @brief Return true if at least one element satisfies @a operation. */
  template <class Container, class Operation> inline
  bool exists(Container & container, Operation & operation)
  {
    return not
      container.traverse([&operation] (const auto & item)
                         {
                           return not operation(item);
                         });
  }

  /** @overload exists for const containers. */
  template <class Container, class Operation> inline
  bool exists(const Container & container, Operation & operation)
  {
    return not
      container.traverse([&operation] (const auto & item)
                         {
                           return not operation(item);
                         });
  }

  /** @overload exists with rvalue operation. */
  template <class Container, class Operation> inline
  bool exists(Container & container, Operation && operation = Operation())
  {
    return exists<Container, Operation>(container, operation);
  }

  /** @overload exists with rvalue operation for const containers. */
  template <class Container, class Operation> inline
  bool exists(const Container & container, Operation && operation = Operation())
  {
    return exists<Container, Operation>(container, operation);
  }

  /** @brief Default filter operation (always true). */
  template <typename T>
  struct Dft_Filter_Op
  {
    bool operator () (const T &) const noexcept { return true; }
  };


  /** @brief Filter elements that satisfy @a operation.

      @tparam Container1 Source container.
      @tparam Container2 Result container (defaults to DynList).
      @tparam Operation Predicate.
      @param container Source container.
      @param operation Predicate to test each element.
      @return New container with filtered elements.
  */
  template <class Container1,
            template <typename> class Container2 = Aleph::DynList,
            class Operation = Dft_Filter_Op<typename Container1::Item_Type>>
  [[nodiscard]] inline Container2<typename Container1::Item_Type>
  filter(Container1 & container, Operation & operation)
  {
    Container2<typename Container1::Item_Type> ret_val;
    container.
      for_each([&ret_val, &operation] (const auto & item)
               {
                 if (operation(item))
                   ret_val.append(item);
               });
    return ret_val;
  }

  /** @overload filter for const containers. */
  template <class Container1,
            template <typename> class Container2 = Aleph::DynList,
            class Operation = Dft_Filter_Op<typename Container1::Item_Type>>
  [[nodiscard]] inline Container2<typename Container1::Item_Type>
  filter(const Container1 & container, Operation & operation)
  {
    Container2<typename Container1::Item_Type> ret_val;
    container.for_each
      ([&ret_val, &operation] (const auto & item)
       {
         if (operation(item))
           ret_val.append(item);
       });
    return ret_val;
  }

  /** @overload filter with rvalue operation. */
  template <class Container1,
            template <typename> class Container2 = Aleph::DynList,
            class Operation = Dft_Filter_Op<typename Container1::Item_Type>>
  [[nodiscard]] inline Container2<typename Container1::Item_Type>
  filter(Container1 & container, Operation && operation)
  {
    return filter<Container1, Container2, Operation>(container, operation);
  }


  /** @overload filter with rvalue operation for const containers. */
  template <class Container1,
            template <typename> class Container2 = Aleph::DynList,
            class Operation = Dft_Filter_Op<typename Container1::Item_Type>>
  [[nodiscard]] inline Container2<typename Container1::Item_Type>
  filter(const Container1 & container, Operation && operation)
  {
    return filter<Container1, Container2, Operation>(container, operation);
  }

  /** @brief Classic map operation.

      Return a new list with each element of container mapped to the
      result of @a op(item).

      @note The name maps is used to avoid clashes with std::map.
      @ingroup Algos
  */
  template <typename T, class C, class Op>
  [[nodiscard]] DynList<T> maps(const C & c, Op op)
  {
    DynList<T> ret;
    for (auto it = c.get_it(); it.has_curr(); it.next_ne())
      ret.append(op(it.get_curr()));
    return ret;
  }

  /** @brief Classic left fold (reduce).

      Computes f(...f(f(init, x1), x2), ..., xn).

      @ingroup Algos
  */
  template < typename T, class Container, class Operation> [[nodiscard]] inline
  T foldl(const Container & container, const T & init, Operation operation)
  {
    T ret_val = init;
    for (auto it = container.get_it(); it.has_curr(); it.next_ne())
      ret_val = operation(ret_val, it.get_curr());
    return ret_val;
  }


  /** @brief Zip two containers into a list of pairs. */
  template <class Container1, class Container2> [[nodiscard]] inline
  DynList<std::pair<typename Container1::Item_Type,
                    typename Container2::Item_Type>>
  zip(const Container1 & a, const Container2 & b)
  {
    typedef typename Container1::Item_Type T1;
    typedef typename Container2::Item_Type T2;
    DynList<std::pair<T1, T2>> ret_val;

    typename Container1::Iterator it1(a);
    typename Container2::Iterator it2(b);
    for (; it1.has_curr() and it2.has_curr(); it1.next_ne(), it2.next_ne())
      ret_val.append(std::pair<T1, T2>(it1.get_curr(), it2.get_curr()));

    return ret_val;
  }

  /** @brief Zip two containers into a list of tuples. */
  template <class Container1, class Container2> [[nodiscard]] inline
  DynList<std::tuple<typename Container1::Item_Type,
                     typename Container2::Item_Type>>
  tzip(const Container1 & a, const Container2 & b)
  {
    typedef typename Container1::Item_Type T1;
    typedef typename Container2::Item_Type T2;
    using Tuple = std::tuple<T1, T2>;
    DynList<Tuple> ret_val;

    typename Container1::Iterator it1(a);
    typename Container2::Iterator it2(b);
    for (; it1.has_curr() and it2.has_curr(); it1.next_ne(), it2.next_ne())
      ret_val.append(Tuple(it1.get_curr(), it2.get_curr()));

    return ret_val;
  }

  /** @brief Zip two containers; throw if lengths differ. */
  template <class Container1, class Container2> [[nodiscard]] inline
  DynList<std::pair<typename Container1::Item_Type,
                    typename Container2::Item_Type>>
  zipEq(const Container1 & a, const Container2 & b)
  {

    typedef typename Container1::Item_Type T1;
    typedef typename Container2::Item_Type T2;
    DynList<std::pair<T1, T2>> ret_val;

    auto it1 = a.get_it();
    auto it2 = b.get_it();
    for (; it1.has_curr() and it2.has_curr(); it1.next_ne(), it2.next_ne())
      ret_val.append(std::pair<T1, T2>(it1.get_curr(), it2.get_curr()));

    if (it1.has_curr() or it2.has_curr())
      ah_length_error() << "Container sizes mismatch";

    return ret_val;
  }

  /** @brief Zip two containers into tuples; throw if lengths differ. */
  template <class Container1, class Container2> [[nodiscard]] inline
  DynList<std::tuple<typename Container1::Item_Type,
                     typename Container2::Item_Type>>
  tzipEq(const Container1 & a, const Container2 & b)
  {
    typedef typename Container1::Item_Type T1;
    typedef typename Container2::Item_Type T2;
    using Tuple = std::tuple<T1, T2>;
    DynList<Tuple> ret_val;

    typename Container1::Iterator it1(a);
    typename Container2::Iterator it2(b);
    for (; it1.has_curr() and it2.has_curr(); it1.next_ne(), it2.next_ne())
      ret_val.append(Tuple(it1.get_curr(), it2.get_curr()));

    if (it1.has_curr() or it2.has_curr())
      ah_length_error() << "Container sizes mismatch";

    return ret_val;
  }

  /** @brief Return pairs of (element, index). */
  template <class Container> [[nodiscard]]
  auto inline enumerate(const Container & c)
  {
    using Item = typename Container::Item_Type;
    using Pair = std::pair<Item, size_t>;
    DynList<Pair> ret;
    size_t i = 0;
    c.for_each([&i, &ret] (const Item & item) { ret.append(Pair(item, i++)); });
    return ret;
  }


  /** @brief Equality check for two containers using predicate @a e. */
  template <class C1, class C2,
            class Eq = std::equal_to<typename C1::Item_Type>> [[nodiscard]] inline
  bool eq(const C1 & c1, const C2 & c2, Eq e = Eq())
  {
    auto it1 = c1.get_it();
    auto it2 = c2.get_it();
    for (; it1.has_curr() and it2.has_curr(); it1.next_ne(), it2.next_ne())
      if (not (e(it1.get_curr(), it2.get_curr())))
        return false;

    return not (it1.has_curr() or it2.has_curr());
  }

  /** @brief Equality operator for DynList. */
  template <typename T> [[nodiscard]] inline
  bool operator == (const DynList<T> & l1, const DynList<T> & l2)
  {
    return eq(l1, l2);
  }

  /** @overload eq */
  template <class C1, class C2, class Eq> [[nodiscard]] inline
  bool containers_eq(const C1 & c1, const C2 & c2, Eq e)
  {
    return eq(c1, c2, e);
  }

  /** @brief Detailed equality check returning mismatch position and values. */
  template <class C1, class C2,
            class Eq = std::equal_to<typename C1::Item_Type>> [[nodiscard]] inline
  std::tuple<bool, size_t, typename C1::Item_Type, typename C2::Item_Type>
  are_eq(const C1 & c1, const C2 & c2, Eq e = Eq())
  {
    using T = typename C1::Item_Type;
    auto it1 = c1.get_it();
    auto it2 = c2.get_it();
    size_t n = 0;
    for (; it1.has_curr() and it2.has_curr(); it1.next_ne(), it2.next_ne(), n++)
      {
        auto & i1 = it1.get_curr();
        auto & i2 = it2.get_curr();
        if (not (e(i1, i2)))
          return std::make_tuple(false, n, i1, i2);
      }

    return std::make_tuple(not (it1.has_curr() or it2.has_curr()), n, T(), T());
  }

  /** @brief Lexicographical comparison between two containers. */
  template <class C1, class C2,
            class Cmp = std::less<typename C1::Item_Type>> [[nodiscard]] inline
  bool lesser(const C1 & c1, const C2 & c2, Cmp cmp = Cmp())
  {
    auto it1 = c1.get_it();
    auto it2 = c2.get_it();
    for (; it1.has_curr() and it2.has_curr(); it1.next_ne(), it2.next_ne())
      {
        auto & curr1 = it1.get_curr();
        auto & curr2 = it2.get_curr();
        if (cmp(curr1, curr2))
          return true;
        else if (cmp(curr2, curr1))
          return false;
      }

    if (not it1.has_curr() and not it2.has_curr())
      return false;

    return it2.has_curr();
  }

  /** @brief Check if two containers differ. */
  template <class C1, class C2,
            class Eq = std::equal_to<typename C1::Item_Type>> [[nodiscard]] inline
  bool diff(const C1 & c1, const C2 & c2, Eq e = Eq())
  {
    return not eq(c1, c2, e);
  }

  /** @brief Separate a list of pairs into two lists. */
  template <class Container> [[nodiscard]] inline
  auto unzip(const Container & l)
  {
    using T1 = std::decay_t<decltype(l.get_first().first)>;
    using T2 = std::decay_t<decltype(l.get_first().second)>;
    DynList<T1> l1;
    DynList<T2> l2;
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      {
        auto & curr = it.get_curr();
        l1.append(curr.first);
        l2.append(curr.second);
      }

    return std::make_pair(std::move(l1), std::move(l2));
  }

  /** @brief Separate a list of tuples into two containers. */
  template <template <typename> class Container, typename T1, typename T2>
  [[nodiscard]] inline std::tuple<Container<T1>, Container<T2>>
  tunzip(const Container<std::tuple<T1, T2>> & l)
  {
    Container<T1> l1;
    Container<T2> l2;
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      {
        auto & curr = it.get_curr();
        l1.append(std::get<0>(curr));
        l2.append(std::get<1>(curr));
      }

    return std::make_tuple(std::move(l1), std::move(l2));
  }


  /** @brief Partition a container into two based on a predicate.

      @param c Source container.
      @param operation Predicate (true goes to first list, false to second).
      @return Pair of containers.
  */
  template <class SrcContainer,
            template <typename> class TgtContainer = Aleph::DynList>
  [[nodiscard]] inline std::pair<TgtContainer<typename SrcContainer::Item_Type>,
                    TgtContainer<typename SrcContainer::Item_Type>> partition
  (const SrcContainer & c,
   std::function<bool(const typename SrcContainer::Item_Type &)> operation)
  {
    typedef typename SrcContainer::Item_Type Type;
    typedef std::pair<TgtContainer<Type>, TgtContainer<Type>> Pair;

    Pair ret_val;
    for_each(c, [&ret_val, &operation] (const Type & item)
                {
                  if (operation(item))
                    ret_val.first.append(item);
                  else
                    ret_val.second.append(item);
                });
    return ret_val;
  }

  /** @brief Return pairs of (index, key). */
  template <class Container> [[nodiscard]] inline
  DynList<std::pair<size_t, typename Container::Key_Type>>
  indexes(const Container & c)
  {
    using T = typename Container::Key_Type;
    using Pair = std::pair<size_t, T>;
    size_t i = 0;

    return c.Container:: template maps<Pair>([&i] (const T & d)
    { return Pair(i++, d); });
  }


  /** @brief Return tuples of (index, key). */
  template <class Container> [[nodiscard]] inline
  DynList<std::tuple<size_t, typename Container::Key_Type>>
  tindexes(const Container & c)
  {
    using T = typename Container::Key_Type;
    using Tuple = std::tuple<size_t, typename Container::Key_Type>;
    size_t i = 0;
    return c.Container::template maps<std::tuple<size_t, T>>([&i] (const T & d)
    {
      return Tuple(i++, d);
    });
  }


  /** @brief Return a reversed copy of the container. */
  template <typename T, template <typename> class Container>
  [[nodiscard]] inline Container<T> reverse(const Container<T> & l)
  {
    Container<T> ret_val;
    l.for_each([&ret_val] (const T & item)
               {
                 ret_val.insert(item);
               });
    return ret_val;
  }

  /** Generates all sequential tuples of size n contained in the
      container c.

      If for example c = { 1, 2, 3, 4, 5, 6 ..., N}

      Then gen_seq_list_tuples(c, 4) = { (1,2,3,4), (2,3,4,5), ... }

      @param[in] c container from which, sequentially, is desired
      generate the tuples

      @param[in] n size of each tuple.
      @return Lists of lists. Each list has size n and contains a
      tuple.

      @ingroup Algos
  */
  template <class Container> [[nodiscard]]
  auto gen_seq_list_tuples(const Container & c, size_t n)
  {
    using T = typename Container::Item_Type;
    typename Container::Iterator it(c);
    DynList<T> l;
    for (size_t i = 0; i < n; ++i, it.next())
      l.append(it.get_curr());

    DynList<DynList<T>> ret;
    ret.append(l);
    for (; it.has_curr(); it.next_ne())
      {
        l.remove_first();
        l.append(it.get_curr());
        ret.append(l);
      }

    return ret;
  }

  /** Returns the different groups contained in the container `c`.

      For example, if container `c` is `{1, 1, 2, 2, 2, 4, 4, 0, 0, 1}`,
      then

      sequential_groups(c)

      will return a pair whose first item is a list of list as follows:

      `{ {1, 1}, {2, 2, 2}, {4, 4}, {0, 0}, {1} }`

      and the second item is the number of found groups 5.

      @ingroup Algos
  */
  template <typename T, template <typename> class Container, class Equal>
  [[nodiscard]] std::pair<DynList<DynList<T>>, size_t>
  sequential_groups(const Container<T> & c, Equal & eq)
  {
    using P = std::pair<DynList<DynList<T>>, size_t>;
    if (c.is_empty())
      return P(DynList<DynList<T> >(), 0);

    DynList<DynList<T>> ret; // this will be the result

    DynList<T> * group = &ret.append(DynList<T>()); // creates a first group

    auto it = c.get_it();                    // put the firstitem into the group
    auto curr_item = it.get_curr();
    group->append(curr_item);

    size_t count = 1; // count the number of groups

    for (it.next(); it.has_curr(); it.next_ne())
      {
        auto & curr = it.get_curr();
        if (not eq(curr, curr_item)) // group change?
          {
            curr_item = curr;
            group = &ret.append(DynList<T>()); // create new group and insert it
            ++count; // increase the number of groups
          }

        group->append(curr);
      }

    return P(ret, count);
  }

  /** @overload sequential_groups */
  template <typename T, template <typename> class Container,
            class Equal = std::equal_to<T>>
  [[nodiscard]] std::pair<DynList<DynList<T>>, size_t>
  sequential_groups(const Container<T> & c, Equal && eq = Equal())
  {
    return sequential_groups(c, eq);
  }

  /** @brief Extract unique consecutive items. */
  template <typename T, template <typename> class Container, class Equal>
  [[nodiscard]] std::pair<DynList<T>, size_t>
  unique_sequential(const Container<T> & c, Equal & eq)
  {
    using P = std::pair<DynList<T>, size_t>;
    if (c.is_empty())
      return P(DynList<T>(), 0);

    DynList<T> ret;

    auto it = c.get_it();                    // put the first item
    auto curr_item = it.get_curr();
    ret.append(curr_item);

    size_t count = 1; // count the number of groups

    for (it.next(); it.has_curr(); it.next_ne())
      {
        auto & curr = it.get_curr();
        if (not eq(curr, curr_item)) // group change?
          {
            curr_item = curr;
            ret.append(curr_item);
            ++count; // increase the number of groups
          }
      }

    return P(ret, count);
  }

  /** @overload unique_sequential */
  template <typename T, template <typename> class Container,
            class Equal = std::equal_to<T>>
  [[nodiscard]] std::pair<DynList<T>, size_t>
  unique_sequential(const Container<T> & c, Equal && eq = Equal())
  {
    return unique_sequential(c, eq);
  }

  /** @brief Iterator that zips two other iterators. */
  template <class Itor1, class Itor2 = Itor1>
  class Pair_Iterator
  {
    Itor1 it1;
    Itor2 it2;

  public:

    Pair_Iterator(Itor1 i1, Itor2 i2) : it1(i1), it2(i2) {}

    template <class C1, class C2>
    Pair_Iterator(const C1 & c1, const C2 & c2)
      : Pair_Iterator(c1.get_it(), c2.get_it()) {}

    bool has_curr() const noexcept { return it1.has_curr() and it2.has_curr(); }

    bool has_curr1() const noexcept { return it1.has_curr(); }

    bool has_curr2() const noexcept { return it2.has_curr(); }

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

  /** @brief Get a Pair_Iterator for two containers. */
  template <class C1, class C2>
  [[nodiscard]] Pair_Iterator<typename C1::Iterator, typename C2::Iterator> inline
  get_pair_it(const C1 & c1, const C2 & c2)
  {
    using I1 = typename C1::Iterator;
    using I2 = typename C2::Iterator;
    I1 i1(c1);
    I2 i2(c2);
    return Pair_Iterator<I1, I2>(i1, i2);
  }

  /** @overload get_pair_it with start position. */
  template <class C1, class C2>
  [[nodiscard]] Pair_Iterator<typename C1::Iterator, typename C2::Iterator> inline
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

  /// @cond INTERNAL
  template <class C> inline void insert_in_container(C &, size_t&) {}

  template <class C, typename T, typename ... Args> inline
  void insert_in_container(C & c, size_t & n, const T & item, Args & ... args)
  {
    c.insert(item);
    ++n;
    insert_in_container(c, n, args...);
  }
  /// @endcond

  /** @brief Insert multiple items into a container.
      @return Number of items inserted.
  */
  template <class C, typename ... Args> inline
  size_t insert_in_container(C & c, Args ... args)
  {
    size_t n = 0;
    insert_in_container(c, n, args...);
    return n;
  }

  /// @cond INTERNAL
  template <class C> inline void append_in_container(C &, size_t&) {}

  template <class C, typename T, typename ... Args> inline
  void append_in_container(C & c, size_t & n, const T & item, Args & ... args)
  {
    c.append(item);
    ++n;
    append_in_container(c, n, args...);
  }
  /// @endcond

  /** @brief Append multiple items into a container.
      @return Number of items appended.
  */
  template <class C, typename ... Args> inline
  size_t append_in_container(C & c, Args ... args)
  {
    size_t n = 0;
    append_in_container(c, n, args...);
    return n;
  }

  /** @brief Build a container with the given items. */
  template <class C, typename ... Args>
  [[nodiscard]] C build_container(Args ... args)
  {
    C c;
    append_in_container(c, args...);
    return c;
  }

  /** @brief Convert one container type to another. */
  template <class SrcC, class TgtC>
  [[nodiscard]] TgtC assign_container(const SrcC & srcc)
  {
    TgtC ret;
    for (auto it = srcc.get_it(); it.has_curr(); it.next_ne())
      ret.append(it.get_curr());

    return ret;
  }

  /** @brief Build a DynList with the given items. */
  template <typename T, typename ... Args>
  [[nodiscard]] DynList<T> build_dynlist(Args ... args)
  {
    return build_container<DynList<T>>(args...);
  }

  /// @cond INTERNAL
  template <class C> inline void remove_from_container(C &, size_t&) {}

  template <class C, typename T, typename ... Args> inline
  void remove_from_container(C & c, size_t & n, const T & item, Args & ... args)
  {
    c.remove(item);
    ++n;
    remove_from_container(c, n, args...);
  }
  /// @endcond

  /** @brief Remove multiple items from a container.
      @return Number of items removed.
  */
  template <class C, typename ... Args> inline
  size_t remove_from_container(C & c, Args ... args)
  {
    size_t n = 0;
    remove_from_container(c, n, args...);
    return n;
  }

  // These function are defined in tpl_dynSetHash.H

  // union
  template <typename T, template <typename> class Container> [[nodiscard]] inline
  DynList<T> join(const Container<T> & c1, const Container<T> & c2);

  template <typename T, template <typename> class Container> [[nodiscard]] inline
  DynList<T> intercept(const Container<T> & c1, const Container<T> & c2);

  template <typename T, template <typename> class Container> [[nodiscard]] inline
  DynList<T> unique(const Container<T> & c);

  template <typename T, template <typename> class Container> [[nodiscard]] inline
  DynList<T> repeated(const Container<T> & c);

  template <typename T, template <typename> class Container> [[nodiscard]] inline
  DynList<std::pair<T, size_t>> repeated_with_index(const Container<T> & c);

  /** @brief Flatten a nested container of one level. */
  template <typename T,
            template <typename> class C1,
            template <typename> class C2>
  [[nodiscard]] DynList<T> flatten(const C2<C1<T>> & c)
  {
    DynList<T> ret;
    for (auto it_c = c.get_it(); it_c.has_curr(); it_c.next_ne())
      {
        const auto & curr_c = it_c.get_curr();
        for (auto it = curr_c.get_it(); it.has_curr(); it.next_ne())
          ret.append(it.get_curr());
      }
    return ret;
  }

  /** @brief Flatten a nested container of two levels. */
  template <typename T,
            template <typename> class C1,
            template <typename> class C2,
            template <typename> class C3>
  [[nodiscard]] DynList<T> flatten(const C3<C2<C1<T>>> & c)
  {
    DynList<T> ret;
    for (auto it = c.get_it(); it.has_curr(); it.next_ne())
      ret.append(flatten(it.get_curr()));
    return ret;
  }

  /** @brief Flatten a nested container of three levels. */
  template <typename T,
            template <typename> class C1,
            template <typename> class C2,
            template <typename> class C3,
            template <typename> class C4>
  [[nodiscard]] DynList<T> flatten(const C4<C3<C2<C1<T>>>> & c)
  {
    DynList<T> ret;
    for (auto it = c.get_it(); it.has_curr(); it.next_ne())
      ret.append(flatten(it.get_curr()));
    return ret;
  }

  /** @brief Flatten a nested container of four levels. */
  template <typename T,
            template <typename> class C1,
            template <typename> class C2,
            template <typename> class C3,
            template <typename> class C4,
            template <typename> class C5>
  [[nodiscard]] DynList<T> flatten(const C5<C4<C3<C2<C1<T>>>>> & c)
  {
    DynList<T> ret;
    for (auto it = c.get_it(); it.has_curr(); it.next_ne())
      ret.append(flatten(it.get_curr()));
    return ret;
  }

  /** @brief Check if @a val is present in @a values. */
  template <typename T> [[nodiscard]] inline
  bool is_inside(const T & val, const DynList<T> & values)
  {
    for (const auto & v : values)
      if (val == v)
        return true;
    return false;
  }

  /** @brief Variadic check for equality against multiple values. */
  template <typename T> [[nodiscard]] inline
  bool is_equal(const T &) { return false; }

  /** @overload is_equal */
  template <typename T, typename U, typename ... Args> [[nodiscard]] inline
  bool is_equal(const T & val, const U & rhs, const Args & ... args)
  {
    return (val == rhs) or is_equal(val, args...);
  }

  /** @brief Return true if no element satisfies @a operation.

      This is the complement of exists().

      @ingroup Algos
  */
  template <class Container, class Operation> [[nodiscard]] inline
  bool none(const Container & container, Operation & operation)
  {
    return not exists(container, operation);
  }

  /** @overload none with rvalue operation. */
  template <class Container, class Operation> [[nodiscard]] inline
  bool none(const Container & container, Operation && operation = Operation())
  {
    return none<Container, Operation>(container, operation);
  }

  /** @brief Find the first element satisfying @a pred.

      @param container Source container.
      @param pred Predicate to test each element.
      @return Pointer to the first matching element, or nullptr if not found.
      @ingroup Algos
  */
  template <class Container, class Pred> [[nodiscard]] inline
  typename Container::Item_Type *
  find_ptr(Container & container, Pred & pred)
  {
    typename Container::Item_Type * result = nullptr;
    container.traverse([&result, &pred] (auto & item)
                       {
                         if (pred(item))
                           {
                             result = &item;
                             return false;
                           }
                         return true;
                       });
    return result;
  }

  /** @overload find_ptr for const containers. */
  template <class Container, class Pred> [[nodiscard]] inline
  const typename Container::Item_Type *
  find_ptr(const Container & container, Pred & pred)
  {
    const typename Container::Item_Type * result = nullptr;
    container.traverse([&result, &pred] (const auto & item)
                       {
                         if (pred(item))
                           {
                             result = &item;
                             return false;
                           }
                         return true;
                       });
    return result;
  }

  /** @overload find_ptr with rvalue predicate. */
  template <class Container, class Pred> [[nodiscard]] inline
  typename Container::Item_Type *
  find_ptr(Container & container, Pred && pred)
  {
    return find_ptr<Container, Pred>(container, pred);
  }

  /** @overload find_ptr with rvalue predicate for const containers. */
  template <class Container, class Pred> [[nodiscard]] inline
  const typename Container::Item_Type *
  find_ptr(const Container & container, Pred && pred)
  {
    return find_ptr<Container, Pred>(container, pred);
  }

  /** @brief Right fold (reduce).

      Computes f(x1, f(x2, ... f(xn, init)...)).

      @note Unlike foldl, this traverses from right to left conceptually,
            but physically still iterates left-to-right, so the associativity
            and operation order differ from foldl.
      @ingroup Algos
  */
  template <typename T, class Container, class Operation> [[nodiscard]] inline
  T foldr(const Container & container, const T & init, Operation operation)
  {
    DynList<T> reversed;
    for (auto it = container.get_it(); it.has_curr(); it.next_ne())
      reversed.insert(it.get_curr());

    T ret_val = init;
    for (auto it = reversed.get_it(); it.has_curr(); it.next_ne())
      ret_val = operation(it.get_curr(), ret_val);
    return ret_val;
  }

  /** @brief Compute sum of all elements.

      @param container Source container.
      @param init Starting value (defaults to T{}).
      @return Sum of all elements plus init.
      @ingroup Algos
  */
  template <class Container, typename T = typename Container::Item_Type>
  [[nodiscard]] inline T sum(const Container & container, const T & init = T{})
  {
    T result = init;
    for (auto it = container.get_it(); it.has_curr(); it.next_ne())
      result = result + it.get_curr();
    return result;
  }

  /** @brief Compute product of all elements.

      @param container Source container.
      @param init Starting value (defaults to T{1}).
      @return Product of all elements times init.
      @ingroup Algos
  */
  template <class Container, typename T = typename Container::Item_Type>
  [[nodiscard]] inline T product(const Container & container, const T & init = T{1})
  {
    T result = init;
    for (auto it = container.get_it(); it.has_curr(); it.next_ne())
      result = result * it.get_curr();
    return result;
  }

  /** @brief Concatenate two containers into a new DynList.

      @param c1 First container.
      @param c2 Second container.
      @return DynList with elements of c1 followed by elements of c2.
      @ingroup Algos
  */
  template <class C1, class C2> [[nodiscard]] inline
  DynList<typename C1::Item_Type>
  concat(const C1 & c1, const C2 & c2)
  {
    DynList<typename C1::Item_Type> result;
    for (auto it = c1.get_it(); it.has_curr(); it.next_ne())
      result.append(it.get_curr());
    for (auto it = c2.get_it(); it.has_curr(); it.next_ne())
      result.append(it.get_curr());
    return result;
  }

  /** @brief Return elements while predicate is true (take_while).

      @param c Source container.
      @param pred Predicate to test.
      @return DynList with initial elements satisfying pred.
      @ingroup Algos
  */
  template <class Container, class Pred> [[nodiscard]] inline
  DynList<typename Container::Item_Type>
  take_while(const Container & c, Pred pred)
  {
    DynList<typename Container::Item_Type> result;
    for (auto it = c.get_it(); it.has_curr(); it.next_ne())
      {
        const auto & item = it.get_curr();
        if (not pred(item))
          break;
        result.append(item);
      }
    return result;
  }

  /** @brief Skip elements while predicate is true (drop_while).

      @param c Source container.
      @param pred Predicate to test.
      @return DynList with remaining elements after predicate becomes false.
      @ingroup Algos
  */
  template <class Container, class Pred> [[nodiscard]] inline
  DynList<typename Container::Item_Type>
  drop_while(const Container & c, Pred pred)
  {
    DynList<typename Container::Item_Type> result;
    bool dropping = true;
    for (auto it = c.get_it(); it.has_curr(); it.next_ne())
      {
        const auto & item = it.get_curr();
        if (dropping and pred(item))
          continue;
        dropping = false;
        result.append(item);
      }
    return result;
  }

  /** @brief Apply operation and flatten results (flatMap/concatMap).

      For each element, apply op to get a container, then flatten all results.

      @param container Source container.
      @param op Operation returning a container for each element.
      @return Flattened DynList of all results.
      @ingroup Algos
  */
  template <class Container, class Op> [[nodiscard]] inline
  auto flat_map(const Container & container, Op op)
    -> DynList<typename std::decay_t<decltype(op(std::declval<typename Container::Item_Type>()))>::Item_Type>
  {
    using ResultItemType = typename std::decay_t<decltype(op(std::declval<typename Container::Item_Type>()))>::Item_Type;
    DynList<ResultItemType> result;
    for (auto it = container.get_it(); it.has_curr(); it.next_ne())
      {
        auto sub = op(it.get_curr());
        for (auto sit = sub.get_it(); sit.has_curr(); sit.next_ne())
          result.append(sit.get_curr());
      }
    return result;
  }

  /** @brief Compute running sums (scanl with addition).

      Returns a list of partial sums including init.

      @param container Source container.
      @param init Starting value.
      @return DynList with all intermediate accumulator values.
      @ingroup Algos
  */
  template <typename T, class Container> [[nodiscard]] inline
  DynList<T> scanl_sum(const Container & container, const T & init)
  {
    DynList<T> result;
    T acc = init;
    result.append(acc);
    for (auto it = container.get_it(); it.has_curr(); it.next_ne())
      {
        acc = acc + it.get_curr();
        result.append(acc);
      }
    return result;
  }

  /** @brief Prefix scan with custom operation.

      Computes running accumulation: [init, op(init, x1), op(op(init, x1), x2), ...]

      @param container Source container.
      @param init Initial accumulator value.
      @param op Binary operation.
      @return DynList with all intermediate accumulator values.
      @ingroup Algos
  */
  template <typename T, class Container, class Op> [[nodiscard]] inline
  DynList<T> scanl(const Container & container, const T & init, Op op)
  {
    DynList<T> result;
    T acc = init;
    result.append(acc);
    for (auto it = container.get_it(); it.has_curr(); it.next_ne())
      {
        acc = op(acc, it.get_curr());
        result.append(acc);
      }
    return result;
  }

  /** @brief Find the minimum element in a container.

      @param container Source container.
      @param cmp Comparator (defaults to std::less).
      @return Pointer to the minimum element, or nullptr if container is empty.
      @ingroup Algos
  */
  template <class Container,
            class Cmp = std::less<typename Container::Item_Type>>
  [[nodiscard]] inline const typename Container::Item_Type *
  min_ptr(const Container & container, Cmp cmp = Cmp())
  {
    auto it = container.get_it();
    if (not it.has_curr())
      return nullptr;

    const typename Container::Item_Type * min_elem = &it.get_curr();
    for (it.next_ne(); it.has_curr(); it.next_ne())
      {
        const auto & curr = it.get_curr();
        if (cmp(curr, *min_elem))
          min_elem = &curr;
      }
    return min_elem;
  }

  /** @brief Find the maximum element in a container.

      @param container Source container.
      @param cmp Comparator (defaults to std::less, finds max by reversing comparison).
      @return Pointer to the maximum element, or nullptr if container is empty.
      @ingroup Algos
  */
  template <class Container,
            class Cmp = std::less<typename Container::Item_Type>>
  [[nodiscard]] inline const typename Container::Item_Type *
  max_ptr(const Container & container, Cmp cmp = Cmp())
  {
    auto it = container.get_it();
    if (not it.has_curr())
      return nullptr;

    const typename Container::Item_Type * max_elem = &it.get_curr();
    for (it.next_ne(); it.has_curr(); it.next_ne())
      {
        const auto & curr = it.get_curr();
        if (cmp(*max_elem, curr))
          max_elem = &curr;
      }
    return max_elem;
  }

  /** @brief Find both min and max elements in a single pass.

      @param container Source container.
      @param cmp Comparator (defaults to std::less).
      @return Pair of pointers (min, max), or (nullptr, nullptr) if container is empty.
      @ingroup Algos
  */
  template <class Container,
            class Cmp = std::less<typename Container::Item_Type>>
  [[nodiscard]] inline std::pair<const typename Container::Item_Type *,
                                  const typename Container::Item_Type *>
  minmax_ptr(const Container & container, Cmp cmp = Cmp())
  {
    using T = typename Container::Item_Type;
    auto it = container.get_it();
    if (not it.has_curr())
      return {nullptr, nullptr};

    const T * min_elem = &it.get_curr();
    const T * max_elem = min_elem;
    for (it.next_ne(); it.has_curr(); it.next_ne())
      {
        const auto & curr = it.get_curr();
        if (cmp(curr, *min_elem))
          min_elem = &curr;
        if (cmp(*max_elem, curr))
          max_elem = &curr;
      }
    return {min_elem, max_elem};
  }

  /** @brief Count elements satisfying a predicate.

      @param container Source container.
      @param pred Predicate to test.
      @return Number of elements satisfying pred.
      @ingroup Algos
  */
  template <class Container, class Pred> [[nodiscard]] inline
  size_t count_if(const Container & container, Pred pred)
  {
    size_t count = 0;
    for (auto it = container.get_it(); it.has_curr(); it.next_ne())
      if (pred(it.get_curr()))
        ++count;
    return count;
  }

  /** @brief Check if container contains a specific value.

      @param container Source container.
      @param value Value to search for.
      @return true if value is found.
      @ingroup Algos
  */
  template <class Container> [[nodiscard]] inline
  bool contains(const Container & container,
                const typename Container::Item_Type & value)
  {
    for (auto it = container.get_it(); it.has_curr(); it.next_ne())
      if (it.get_curr() == value)
        return true;
    return false;
  }

  /** @brief Zip containers with an index (enumerate with zip).

      Returns list of tuples (index, element).

      @param container Source container.
      @return DynList of tuples (size_t, T).
      @ingroup Algos
  */
  template <class Container> [[nodiscard]] inline
  DynList<std::tuple<size_t, typename Container::Item_Type>>
  enumerate_tuple(const Container & container)
  {
    using T = typename Container::Item_Type;
    using Tuple = std::tuple<size_t, T>;
    DynList<Tuple> result;
    size_t i = 0;
    for (auto it = container.get_it(); it.has_curr(); it.next_ne(), ++i)
      result.append(Tuple(i, it.get_curr()));
    return result;
  }


} // end namespace Aleph

#endif // AH_FUNCTIONAL_H
