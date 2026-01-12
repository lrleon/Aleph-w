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

    Main functions include:
    - **Range generation**: `range()`, `nrange()`, `contiguous_range()`, `rep()`
    - **Iteration**: `for_each()`, `enum_for_each()`, `each()`
    - **Predicates**: `all()`, `exists()`, `none()`, `contains()`, `count_if()`
    - **Transformation**: `maps()`, `filter()`, `reverse()`, `flatten()`, `flat_map()`
    - **Folding**: `foldl()`, `foldr()`, `sum()`, `product()`, `scanl()`
    - **Zipping**: `zip()`, `zipEq()`, `tzip()`, `zip_longest()`, `tzip_longest()`,
                   `zip_longest_opt()`
    - **Grouping**: `group_by()`, `group_by_eq()`, `group_by_reduce()`,
                    `sequential_groups()`, `unique_sequential()`
    - **Finding**: `find_ptr()`, `find_opt()`, `min_ptr()`, `max_ptr()`, `minmax_ptr()`
    - **Comparison**: `eq()`, `lesser()`, `diff()`
    - **Partitioning**: `partition()`, `take_while()`, `drop_while()`

    @ingroup Algorithms
 *  @author Leandro Rabindranath León
*/

# include <stdexcept>
# include <utility>
# include <tuple>
# include <functional>
# include <algorithm>
# include <optional>
# include <ah-errors.H>
# include <ah-ranges.H>

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
      __builtin_unreachable();  // Satisfies compiler after throw
    }

    [[noreturn]] virtual const T & get_item() const override
    {
      ah_invalid_argument() << "Access from None type";
      __builtin_unreachable();
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

  /** @brief Generate @a n contiguous values starting from @a start.

      Creates a sequence of consecutive values: start, start+1, start+2, ..., start+n-1.

      @tparam T Numeric type (must support increment operator).
      @tparam Container Container type (defaults to DynList).
      @param start Starting value.
      @param n Number of values to generate.
      @return Container with n consecutive values.

      @par Example:
      @code
      auto seq = contiguous_range(5, 4);  // {5, 6, 7, 8}
      auto zeros = contiguous_range(0, 3); // {0, 1, 2}
      @endcode

      @see range() for generating ranges with custom step.
      @ingroup Algorithms
  */
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

      @ingroup Algorithms
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

      @ingroup Algorithms
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

  /** @brief Create a list of pointers to items in a container.

      Traverses the container and collects pointers to each element.
      Useful for algorithms that need to modify elements in-place
      or for creating indirect access patterns.

      @tparam Container Container type with Item_Type and iterator.
      @param c Source container.
      @return DynList of pointers to each element.

      @warning The returned pointers are only valid while the original
               container exists and the elements are not removed.

      @par Example:
      @code
      DynList<int> nums = {1, 2, 3};
      auto ptrs = pointers_list(nums);
      *ptrs.get_first() = 10;  // modifies nums[0]
      @endcode

      @ingroup Algorithms
  */
  template <class Container> [[nodiscard]]
  DynList<typename Container::Item_Type*> pointers_list(Container & c)
  {
    DynList<typename Container::Item_Type*> ret;
    for (auto it = c.get_it(); it.has_curr(); it.next_ne())
      ret.append(&it.get_curr());
    return ret;
  }

  /** @overload pointers_list for const containers.
      @return DynList of const pointers.
  */
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

  /** @brief Execute an operation repeatedly over a range of indices.

      Calls @a op() for each index from @a start to @a end (inclusive).
      The operation receives no arguments; this is purely for side effects.

      @tparam Op Callable type with signature `void()`.
      @param start Starting index (inclusive).
      @param end Ending index (inclusive).
      @param op Operation to execute at each iteration.

      @par Example:
      @code
      int count = 0;
      each(1, 5, [&count]() { ++count; });  // count = 5
      @endcode

      @ingroup Algorithms
  */
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

  /** @brief Apply an operation to each element in a container.

      Traverses all elements in the container and applies the given
      operation to each one. This is a side-effect operation that
      returns the container for chaining.

      @tparam Container Container type with traverse() method.
      @tparam Operation Callable type `(const Item_Type&) -> void`.
      @param container Source container.
      @param operation Operation to apply to each element.
      @return Reference to the container (for method chaining).

      @par Example:
      @code
      DynList<int> nums = {1, 2, 3};
      for_each(nums, [](int x) { std::cout << x << " "; });
      // Output: 1 2 3
      @endcode

      @see enum_for_each() for version with index.
      @ingroup Algorithms
  */
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

  /** @brief Apply an operation to each element and its index.

      Like for_each(), but also passes the zero-based index of each
      element to the operation. Useful when you need to know the
      position of each element during iteration.

      @tparam Container Container type with get_it() method.
      @tparam Operation Callable type `(const Item_Type&, size_t) -> void`.
      @param container Source container.
      @param operation Operation receiving (element, index).

      @par Example:
      @code
      DynList<std::string> names = {"Alice", "Bob", "Carol"};
      enum_for_each(names, [](const std::string& name, size_t i) {
        std::cout << i << ": " << name << '\n';
      });
      // Output:
      // 0: Alice
      // 1: Bob
      // 2: Carol
      @endcode

      @see for_each() for version without index.
      @see enumerate() for getting pairs of (element, index).
      @ingroup Algorithms
  */
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

  /** @brief Return true if all elements satisfy a predicate.

      Tests whether the predicate returns true for every element
      in the container. Short-circuits on the first false result.
      Returns true for empty containers (vacuous truth).

      @tparam Container Container type with traverse() method.
      @tparam Operation Predicate type `(const Item_Type&) -> bool`.
      @param container Source container.
      @param operation Predicate to test each element.
      @return true if operation(item) is true for all items.

      @par Example:
      @code
      DynList<int> nums = {2, 4, 6, 8};
      bool all_even = all(nums, [](int x) { return x % 2 == 0; });
      // all_even = true
      @endcode

      @see exists() for testing if any element satisfies predicate.
      @see none() for testing if no element satisfies predicate.
      @ingroup Algorithms
  */
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


  /** @brief Return true if at least one element satisfies a predicate.

      Tests whether the predicate returns true for at least one element
      in the container. Short-circuits on the first true result.
      Returns false for empty containers.

      @tparam Container Container type with traverse() method.
      @tparam Operation Predicate type `(const Item_Type&) -> bool`.
      @param container Source container.
      @param operation Predicate to test each element.
      @return true if operation(item) is true for any item.

      @par Example:
      @code
      DynList<int> nums = {1, 3, 5, 6, 7};
      bool has_even = exists(nums, [](int x) { return x % 2 == 0; });
      // has_even = true (because of 6)
      @endcode

      @see all() for testing if all elements satisfy predicate.
      @see none() for testing if no element satisfies predicate.
      @ingroup Algorithms
  */
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
      @ingroup Algorithms
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

      @ingroup Algorithms
  */
  template < typename T, class Container, class Operation> [[nodiscard]] inline
  T foldl(const Container & container, const T & init, Operation operation)
  {
#if ALEPH_HAS_RANGES
    if constexpr (std::ranges::range<Container>)
      return detail::ranges_fold_left(container, init, std::move(operation));
    else
#endif
    {
      T ret_val = init;
      for (auto it = container.get_it(); it.has_curr(); it.next_ne())
        ret_val = operation(ret_val, it.get_curr());
      return ret_val;
    }
  }


  /** @brief Zip two containers into a list of pairs.

      Combines elements from two containers pairwise. Iteration stops
      when the shorter container is exhausted.

      @tparam Container1 Type of the first container.
      @tparam Container2 Type of the second container.
      @param a First container.
      @param b Second container.
      @return DynList of pairs (a[i], b[i]).

      @par Example:
      @code
      DynList<int> nums = {1, 2, 3};
      DynList<std::string> names = {"one", "two", "three"};
      auto pairs = zip(nums, names);
      // pairs = {(1, "one"), (2, "two"), (3, "three")}
      @endcode

      @see zipEq() for version that throws on length mismatch.
      @see zip_longest() for version that pads shorter container.
      @see tzip() for tuple-based version.
      @ingroup Algorithms
  */
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

  /** @brief Zip two containers into a list of tuples.

      Like zip() but returns tuples instead of pairs.
      Tuples provide better integration with structured bindings
      and tuple utilities.

      @tparam Container1 Type of the first container.
      @tparam Container2 Type of the second container.
      @param a First container.
      @param b Second container.
      @return DynList of tuples (a[i], b[i]).

      @par Example:
      @code
      DynList<int> xs = {1, 2};
      DynList<double> ys = {1.1, 2.2};
      auto tuples = tzip(xs, ys);
      for (const auto& [x, y] : tuples)
        std::cout << x << ", " << y << '\n';
      @endcode

      @see zip() for pair-based version.
      @see tzipEq() for version that throws on length mismatch.
      @see tzip_longest() for version that pads shorter container.
      @ingroup Algorithms
  */
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

  /** @brief Zip two containers; throw if lengths differ.

      Like zip() but verifies that both containers have equal length.
      Throws std::length_error if container sizes don't match.

      @tparam Container1 Type of the first container.
      @tparam Container2 Type of the second container.
      @param a First container.
      @param b Second container.
      @return DynList of pairs (a[i], b[i]).
      @throw std::length_error if container sizes mismatch.

      @par Example:
      @code
      DynList<int> xs = {1, 2, 3};
      DynList<int> ys = {4, 5, 6};
      auto pairs = zipEq(xs, ys);  // OK, same length

      DynList<int> zs = {7, 8};
      // zipEq(xs, zs);  // throws std::length_error!
      @endcode

      @see zip() for version that stops at shorter container.
      @see tzipEq() for tuple-based version.
      @ingroup Algorithms
  */
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

  /** @brief Zip two containers into tuples; throw if lengths differ.

      Like tzip() but verifies that both containers have equal length.
      Throws std::length_error if container sizes don't match.

      @tparam Container1 Type of the first container.
      @tparam Container2 Type of the second container.
      @param a First container.
      @param b Second container.
      @return DynList of tuples (a[i], b[i]).
      @throw std::length_error if container sizes mismatch.

      @see tzip() for version that stops at shorter container.
      @see zipEq() for pair-based version.
      @ingroup Algorithms
  */
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

  /** @brief Return pairs of (element, index).

      Creates a list of pairs where each pair contains an element
      and its zero-based index in the container. Similar to Python's
      enumerate() but returns pairs instead of tuples.

      @tparam Container Container type with for_each() method.
      @param c Source container.
      @return DynList of pairs (element, index).

      @par Example:
      @code
      DynList<std::string> names = {"Alice", "Bob", "Carol"};
      for (const auto& [name, idx] : enumerate(names))
        std::cout << idx << ": " << name << '\n';
      // Output:
      // 0: Alice
      // 1: Bob
      // 2: Carol
      @endcode

      @see enumerate_tuple() for tuple-based version.
      @see enum_for_each() for applying operation with index.
      @ingroup Algorithms
  */
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


  /** @brief Check equality of two containers using a predicate.

      Compares two containers element-by-element using the given
      equality predicate. Returns true only if both containers have
      the same length and all corresponding pairs are equal.

      @tparam C1 Type of the first container.
      @tparam C2 Type of the second container.
      @tparam Eq Equality predicate type (defaults to std::equal_to).
      @param c1 First container.
      @param c2 Second container.
      @param e Equality predicate for comparing elements.
      @return true if containers have equal length and all elements compare equal.

      @par Example:
      @code
      DynList<int> a = {1, 2, 3};
      DynList<int> b = {1, 2, 3};
      DynList<int> c = {1, 2, 4};

      eq(a, b);  // true
      eq(a, c);  // false
      @endcode

      @see diff() for inequality check.
      @see are_eq() for detailed mismatch information.
      @ingroup Algorithms
  */
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

  /** @brief Detailed equality check returning mismatch position and values.

      Like eq() but returns detailed information about the comparison:
      - Whether the containers are equal
      - The position of first mismatch (if any)
      - The mismatched values from both containers

      @tparam C1 Type of the first container.
      @tparam C2 Type of the second container.
      @tparam Eq Equality predicate type.
      @param c1 First container.
      @param c2 Second container.
      @param e Equality predicate.
      @return Tuple of (equal, position, value1, value2).

      @par Example:
      @code
      DynList<int> a = {1, 2, 5, 4};
      DynList<int> b = {1, 2, 3, 4};
      auto [equal, pos, v1, v2] = are_eq(a, b);
      // equal=false, pos=2, v1=5, v2=3
      @endcode

      @see eq() for simple boolean result.
      @ingroup Algorithms
  */
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

  /** @brief Lexicographical comparison between two containers.

      Compares two containers lexicographically (dictionary order).
      Returns true if c1 is lexicographically less than c2.

      @tparam C1 Type of the first container.
      @tparam C2 Type of the second container.
      @tparam Cmp Comparator type (defaults to std::less).
      @param c1 First container.
      @param c2 Second container.
      @param cmp Comparator for element comparison.
      @return true if c1 < c2 lexicographically.

      @par Example:
      @code
      DynList<int> a = {1, 2, 3};
      DynList<int> b = {1, 2, 4};
      DynList<int> c = {1, 2};

      lesser(a, b);  // true (3 < 4)
      lesser(c, a);  // true (shorter prefix)
      lesser(a, a);  // false (equal)
      @endcode

      @ingroup Algorithms
  */
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

  /** @brief Check if two containers differ.

      Complement of eq(). Returns true if containers have different
      lengths or any corresponding elements differ.

      @tparam C1 Type of the first container.
      @tparam C2 Type of the second container.
      @tparam Eq Equality predicate type.
      @param c1 First container.
      @param c2 Second container.
      @param e Equality predicate.
      @return true if containers are not equal.

      @see eq() for equality check.
      @ingroup Algorithms
  */
  template <class C1, class C2,
            class Eq = std::equal_to<typename C1::Item_Type>> [[nodiscard]] inline
  bool diff(const C1 & c1, const C2 & c2, Eq e = Eq())
  {
    return not eq(c1, c2, e);
  }

  /** @brief Separate a list of pairs into two lists.

      The inverse of zip(). Takes a container of pairs and produces
      two separate containers with the first and second elements
      respectively.

      @tparam Container Container type holding pairs.
      @param l Source container of pairs.
      @return std::pair of two DynLists (firsts, seconds).

      @par Example:
      @code
      DynList<std::pair<int, std::string>> pairs = {
        {1, "one"}, {2, "two"}, {3, "three"}
      };
      auto [nums, names] = unzip(pairs);
      // nums = {1, 2, 3}
      // names = {"one", "two", "three"}
      @endcode

      @see zip() for the inverse operation.
      @see tunzip() for tuple-based version.
      @ingroup Algorithms
  */
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

  /** @brief Separate a list of tuples into two containers.

      The inverse of tzip(). Takes a container of 2-tuples and produces
      two separate containers with the first and second elements
      respectively.

      @tparam Container Container template type.
      @tparam T1 Type of first tuple elements.
      @tparam T2 Type of second tuple elements.
      @param l Source container of tuples.
      @return std::tuple of two containers (firsts, seconds).

      @par Example:
      @code
      DynList<std::tuple<int, double>> tuples = {
        {1, 1.1}, {2, 2.2}, {3, 3.3}
      };
      auto [ints, doubles] = tunzip<DynList>(tuples);
      // ints = {1, 2, 3}
      // doubles = {1.1, 2.2, 3.3}
      @endcode

      @see tzip() for the inverse operation.
      @see unzip() for pair-based version.
      @ingroup Algorithms
  */
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


  /** @brief Return a reversed copy of the container.

      Creates a new container with elements in reverse order.
      The original container is not modified.

      @tparam T Element type.
      @tparam Container Container template type with insert() method.
      @param l Source container.
      @return New container with elements in reverse order.

      @par Example:
      @code
      DynList<int> nums = {1, 2, 3, 4, 5};
      auto rev = reverse(nums);
      // rev = {5, 4, 3, 2, 1}
      // nums unchanged: {1, 2, 3, 4, 5}
      @endcode

      @ingroup Algorithms
  */
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

  /** @brief Generate all sequential tuples (sliding windows) of size n.

      Creates a list of lists where each inner list is a consecutive
      sequence of n elements from the source container. This is a
      sliding window operation.

      @tparam Container Container type with iterator.
      @param c Source container.
      @param n Size of each tuple (window size).
      @return DynList of DynLists, each containing n consecutive elements.

      @par Example:
      @code
      DynList<int> nums = {1, 2, 3, 4, 5};
      auto windows = gen_seq_list_tuples(nums, 3);
      // windows = { {1, 2, 3}, {2, 3, 4}, {3, 4, 5} }
      @endcode

      @note The source container must have at least n elements.
      @ingroup Algorithms
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

  /** @brief Group consecutive equal elements together.

      Partitions a container into groups of consecutive elements that
      are equal according to the given equality predicate. Each group
      is a DynList containing all consecutive equal elements.

      @tparam T Element type.
      @tparam Container Container template type.
      @tparam Equal Equality predicate type.
      @param c Source container.
      @param eq Equality predicate for comparing elements.
      @return Pair of (list of groups, number of groups).

      @par Example:
      @code
      DynList<int> nums = {1, 1, 2, 2, 2, 4, 4, 0, 0, 1};
      auto [groups, count] = sequential_groups(nums);
      // groups = { {1, 1}, {2, 2, 2}, {4, 4}, {0, 0}, {1} }
      // count = 5
      @endcode

      @see group_by() for grouping by a key function.
      @see unique_sequential() for getting unique consecutive values.
      @ingroup Algorithms
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

  /** @brief Extract unique consecutive items.

      Removes consecutive duplicates from a container, keeping only
      the first element of each run of equal elements. Similar to
      Unix `uniq` command.

      @tparam T Element type.
      @tparam Container Container template type.
      @tparam Equal Equality predicate type.
      @param c Source container.
      @param eq Equality predicate for comparing elements.
      @return Pair of (list of unique elements, count).

      @par Example:
      @code
      DynList<int> nums = {1, 1, 2, 2, 2, 3, 1, 1};
      auto [unique, count] = unique_sequential(nums);
      // unique = {1, 2, 3, 1}
      // count = 4
      @endcode

      @note Only removes consecutive duplicates. Non-consecutive duplicates
            are preserved. For global unique, sort first.

      @see sequential_groups() for getting groups of consecutive equals.
      @ingroup Algorithms
  */
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

  /** @brief Iterator that zips two other iterators.

      Synchronizes two iterators so they advance together. Produces
      pairs of elements from both iterators. Useful for iterating
      over two containers in lockstep.

      @tparam Itor1 Type of first iterator.
      @tparam Itor2 Type of second iterator (defaults to Itor1).

      @par Example:
      @code
      DynList<int> nums = {1, 2, 3};
      DynList<std::string> names = {"one", "two", "three"};
      auto it = get_pair_it(nums, names);
      while (it.has_curr()) {
        auto [num, name] = it.get_curr_ne();
        std::cout << num << " = " << name << '\n';
        it.next_ne();
      }
      @endcode

      @see get_pair_it() for creating Pair_Iterator instances.
      @ingroup Algorithms
  */
  template <class Itor1, class Itor2 = Itor1>
  class Pair_Iterator
  {
    Itor1 it1;
    Itor2 it2;

  public:

    /** @brief Construct from two iterators. */
    Pair_Iterator(Itor1 i1, Itor2 i2) : it1(i1), it2(i2) {}

    /** @brief Construct from two containers.
        @param c1 First container.
        @param c2 Second container.
    */
    template <class C1, class C2>
    Pair_Iterator(const C1 & c1, const C2 & c2)
      : Pair_Iterator(c1.get_it(), c2.get_it()) {}

    /** @brief Check if both iterators have current elements.
        @return true if both iterators are valid.
    */
    bool has_curr() const noexcept { return it1.has_curr() and it2.has_curr(); }

    /** @brief Check if first iterator has current element. */
    bool has_curr1() const noexcept { return it1.has_curr(); }

    /** @brief Check if second iterator has current element. */
    bool has_curr2() const noexcept { return it2.has_curr(); }

    /** @brief Get current pair (bounds-checked).
        @return Pair of current elements.
        @throw std::overflow_error if either iterator is exhausted.
    */
    auto get_curr() const
    {
      return std::make_pair(it1.get_curr(), it2.get_curr());
    }

    /** @brief Get current pair (no bounds check).
        @return Pair of current elements.
        @note Requires has_curr() to be true.
    */
    auto get_curr_ne() const noexcept
    {
      return std::make_pair(it1.get_curr_ne(), it2.get_curr_ne());
    }

    /** @brief Advance both iterators (bounds-checked).
        @throw std::overflow_error if either iterator is exhausted.
    */
    void next()
    {
      it1.next();
      it2.next();
    }

    /** @brief Advance both iterators (no bounds check).
        @note Requires has_curr() to be true.
    */
    void next_ne() noexcept
    {
      it1.next_ne();
      it2.next_ne();
    }

    /** @brief Check if both iterators were completely traversed.
        @return true if both iterators are exhausted.
        @note Useful for verifying equal container lengths.
    */
    bool was_traversed() const noexcept
    {
      return not (it1.has_curr() or it2.has_curr());
    }
  };

  /** @brief Create a Pair_Iterator for two containers.

      Factory function that creates a Pair_Iterator from two containers,
      automatically deducing the iterator types.

      @tparam C1 Type of first container.
      @tparam C2 Type of second container.
      @param c1 First container.
      @param c2 Second container.
      @return Pair_Iterator positioned at the beginning of both containers.

      @see Pair_Iterator for usage examples.
      @ingroup Algorithms
  */
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

  /** @brief Flatten a nested container of one level.

      Concatenates all inner containers into a single flat list.

      @tparam T Element type.
      @tparam C1 Inner container template.
      @tparam C2 Outer container template.
      @param c Nested container (container of containers).
      @return DynList with all elements from all inner containers.

      @par Example:
      @code
      DynList<DynList<int>> nested = {{1, 2}, {3, 4, 5}, {6}};
      auto flat = flatten(nested);
      // flat = {1, 2, 3, 4, 5, 6}
      @endcode

      @see flat_map() for map followed by flatten.
      @ingroup Algorithms
  */
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

  /** @brief Flatten a nested container of two levels.

      Recursively flattens a container nested two levels deep.

      @tparam T Element type.
      @tparam C1, C2, C3 Container template types (innermost to outermost).
      @param c Triply-nested container.
      @return DynList with all elements from all inner containers.

      @ingroup Algorithms
  */
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

  /** @brief Flatten a nested container of three levels.

      Recursively flattens a container nested three levels deep.

      @tparam T Element type.
      @tparam C1, C2, C3, C4 Container template types (innermost to outermost).
      @param c Quadruply-nested container.
      @return DynList with all elements from all inner containers.

      @ingroup Algorithms
  */
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

  /** @brief Flatten a nested container of four levels.

      Recursively flattens a container nested four levels deep.

      @tparam T Element type.
      @tparam C1, C2, C3, C4, C5 Container template types (innermost to outermost).
      @param c Five-times-nested container.
      @return DynList with all elements from all inner containers.

      @ingroup Algorithms
  */
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

  /** @brief Check if a value is present in a list.

      Linear search through the list to find the value.

      @tparam T Element type (must support operator==).
      @param val Value to search for.
      @param values List to search in.
      @return true if val is found in values.

      @par Example:
      @code
      DynList<int> nums = {1, 2, 3, 4, 5};
      is_inside(3, nums);  // true
      is_inside(6, nums);  // false
      @endcode

      @see contains() for searching in any container type.
      @ingroup Algorithms
  */
  template <typename T> [[nodiscard]] inline
  bool is_inside(const T & val, const DynList<T> & values)
  {
    for (const auto & v : values)
      if (val == v)
        return true;
    return false;
  }

  /** @brief Variadic check for equality against multiple values.

      Tests if a value equals any of the provided comparison values.
      Short-circuits on first match.

      @tparam T Type of value to test.
      @param val Value to compare (no comparisons, returns false).
      @return false (base case with no values to compare).

      @par Example:
      @code
      is_equal(5, 1, 3, 5, 7);  // true (5 == 5)
      is_equal(4, 1, 3, 5, 7);  // false
      @endcode

      @ingroup Algorithms
  */
  template <typename T> [[nodiscard]] inline
  bool is_equal(const T &) { return false; }

  /** @overload is_equal
      @param val Value to compare.
      @param rhs First comparison value.
      @param args Additional comparison values.
      @return true if val equals rhs or any of args.
  */
  template <typename T, typename U, typename ... Args> [[nodiscard]] inline
  bool is_equal(const T & val, const U & rhs, const Args & ... args)
  {
    return (val == rhs) or is_equal(val, args...);
  }

  /** @brief Return true if no element satisfies @a operation.

      This is the complement of exists().

      @ingroup Algorithms
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

      Returns a pointer to the first element in @a container for which
      @a pred returns true, or nullptr if no such element exists.

      @warning The returned pointer is only valid as long as the container
               exists and the element is not removed. **Never use this function
               on a temporary container**, as the pointer will become dangling
               immediately. For example:
               @code
               // WRONG: dangling pointer!
               auto *p = find_ptr(get_list(), pred);  // list destroyed here
               use(*p);  // undefined behavior!

               // CORRECT: keep container alive
               auto list = get_list();
               auto *p = find_ptr(list, pred);
               if (p) use(*p);  // OK
               @endcode
               If you need a safe copy instead of a pointer, use find_opt().

      @param container Source container (must outlive the returned pointer).
      @param pred Predicate to test each element.
      @return Pointer to the first matching element, or nullptr if not found.
      @see find_opt For a safer alternative that returns a copy.
      @ingroup Algorithms
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

  /** @brief Find the first element satisfying @a pred (safe version).

      Returns a copy of the first element in @a container for which
      @a pred returns true, wrapped in std::optional. Returns std::nullopt
      if no such element exists.

      Unlike find_ptr(), this function is safe to use with temporary containers
      because it returns a copy of the element, not a pointer.

      @code
      // Safe: returns a copy
      auto opt = find_opt(get_list(), pred);
      if (opt) use(*opt);  // OK, opt contains a copy
      @endcode

      @param container Source container.
      @param pred Predicate to test each element.
      @return std::optional containing a copy of the first matching element,
              or std::nullopt if not found.
      @see find_ptr For a pointer-returning version (use with caution).
      @ingroup Algorithms
  */
  template <class Container, class Pred> [[nodiscard]] inline
  std::optional<typename Container::Item_Type>
  find_opt(const Container & container, Pred & pred)
  {
    std::optional<typename Container::Item_Type> result;
    container.traverse([&result, &pred] (const auto & item)
                       {
                         if (pred(item))
                           {
                             result = item;
                             return false;
                           }
                         return true;
                       });
    return result;
  }

  /** @overload find_opt with rvalue predicate. */
  template <class Container, class Pred> [[nodiscard]] inline
  std::optional<typename Container::Item_Type>
  find_opt(const Container & container, Pred && pred)
  {
    return find_opt<Container, Pred>(container, pred);
  }

  /** @brief Right fold (reduce).

      Computes f(x1, f(x2, ... f(xn, init)...)).

      @note Unlike foldl, this traverses from right to left conceptually,
            but physically still iterates left-to-right, so the associativity
            and operation order differ from foldl.
      @ingroup Algorithms
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
      @ingroup Algorithms
  */
  template <class Container, typename T = typename Container::Item_Type>
  [[nodiscard]] inline T sum(const Container & container, const T & init = T{})
  {
#if ALEPH_HAS_RANGES
    if constexpr (std::ranges::range<Container>)
      return detail::ranges_fold_left(container, init, std::plus<>{});
    else
#endif
    {
      T result = init;
      for (auto it = container.get_it(); it.has_curr(); it.next_ne())
        result = result + it.get_curr();
      return result;
    }
  }

  /** @brief Compute product of all elements.

      @param container Source container.
      @param init Starting value (defaults to T{1}).
      @return Product of all elements times init.
      @ingroup Algorithms
  */
  template <class Container, typename T = typename Container::Item_Type>
  [[nodiscard]] inline T product(const Container & container, const T & init = T{1})
  {
#if ALEPH_HAS_RANGES
    if constexpr (std::ranges::range<Container>)
      return detail::ranges_fold_left(container, init, std::multiplies<>{});
    else
#endif
    {
      T result = init;
      for (auto it = container.get_it(); it.has_curr(); it.next_ne())
        result = result * it.get_curr();
      return result;
    }
  }

  /** @brief Concatenate two containers into a new DynList.

      @param c1 First container.
      @param c2 Second container.
      @return DynList with elements of c1 followed by elements of c2.
      @ingroup Algorithms
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
      @ingroup Algorithms
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
      @ingroup Algorithms
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
      @ingroup Algorithms
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
      @ingroup Algorithms
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
      @ingroup Algorithms
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
      @ingroup Algorithms
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
      @ingroup Algorithms
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
      @ingroup Algorithms
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
      @ingroup Algorithms
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
      @ingroup Algorithms
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
      @ingroup Algorithms
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


  /** @brief Zip two containers, padding the shorter one with a fill value.

      Unlike `zip()` which stops at the shorter container, `zip_longest()`
      continues until both containers are exhausted. When one container
      is exhausted before the other, its corresponding values in the
      resulting pairs are filled with the specified fill value.

      This is similar to Python's `itertools.zip_longest()`.

      @tparam Container1 Type of the first container.
      @tparam Container2 Type of the second container.
      @param a First container.
      @param b Second container.
      @param fill_a Value to use when container `a` is exhausted.
      @param fill_b Value to use when container `b` is exhausted.
      @return DynList of pairs containing all elements from both containers.

      @par Example:
      @code
      DynList<int> short_list = {1, 2};
      DynList<int> long_list = {10, 20, 30, 40};

      auto result = zip_longest(short_list, long_list, 0, -1);
      // result contains: {(1, 10), (2, 20), (0, 30), (0, 40)}

      // With different fill values:
      auto result2 = zip_longest(long_list, short_list, -1, 0);
      // result2 contains: {(10, 1), (20, 2), (30, 0), (40, 0)}
      @endcode

      @see zip() for version that stops at the shorter container.
      @see tzip_longest() for tuple-based version.
      @ingroup Algorithms
  */
  template <class Container1, class Container2> [[nodiscard]] inline
  DynList<std::pair<typename Container1::Item_Type,
                    typename Container2::Item_Type>>
  zip_longest(const Container1 & a, const Container2 & b,
              const typename Container1::Item_Type & fill_a = typename Container1::Item_Type(),
              const typename Container2::Item_Type & fill_b = typename Container2::Item_Type())
  {
    using T1 = typename Container1::Item_Type;
    using T2 = typename Container2::Item_Type;
    DynList<std::pair<T1, T2>> ret_val;

    auto it1 = a.get_it();
    auto it2 = b.get_it();

    // Process while both have elements
    while (it1.has_curr() and it2.has_curr())
      {
        ret_val.append(std::pair<T1, T2>(it1.get_curr(), it2.get_curr()));
        it1.next_ne();
        it2.next_ne();
      }

    // Process remaining elements from first container
    while (it1.has_curr())
      {
        ret_val.append(std::pair<T1, T2>(it1.get_curr(), fill_b));
        it1.next_ne();
      }

    // Process remaining elements from second container
    while (it2.has_curr())
      {
        ret_val.append(std::pair<T1, T2>(fill_a, it2.get_curr()));
        it2.next_ne();
      }

    return ret_val;
  }


  /** @brief Zip two containers into tuples, padding the shorter one.

      Like `zip_longest()` but returns tuples instead of pairs.
      This provides better integration with structured bindings and
      tuple utilities.

      @tparam Container1 Type of the first container.
      @tparam Container2 Type of the second container.
      @param a First container.
      @param b Second container.
      @param fill_a Value to use when container `a` is exhausted.
      @param fill_b Value to use when container `b` is exhausted.
      @return DynList of tuples containing all elements from both containers.

      @par Example:
      @code
      DynList<std::string> names = {"Alice", "Bob"};
      DynList<int> scores = {100, 95, 88, 72};

      auto result = tzip_longest(names, scores, "N/A", 0);
      // result contains: {("Alice", 100), ("Bob", 95), ("N/A", 88), ("N/A", 72)}

      // Using structured bindings:
      for (const auto& [name, score] : result)
        std::cout << name << ": " << score << '\n';
      @endcode

      @see zip_longest() for pair-based version.
      @see tzip() for version that stops at the shorter container.
      @ingroup Algorithms
  */
  template <class Container1, class Container2> [[nodiscard]] inline
  DynList<std::tuple<typename Container1::Item_Type,
                     typename Container2::Item_Type>>
  tzip_longest(const Container1 & a, const Container2 & b,
               const typename Container1::Item_Type & fill_a = typename Container1::Item_Type(),
               const typename Container2::Item_Type & fill_b = typename Container2::Item_Type())
  {
    using T1 = typename Container1::Item_Type;
    using T2 = typename Container2::Item_Type;
    using Tuple = std::tuple<T1, T2>;
    DynList<Tuple> ret_val;

    auto it1 = a.get_it();
    auto it2 = b.get_it();

    while (it1.has_curr() and it2.has_curr())
      {
        ret_val.append(Tuple(it1.get_curr(), it2.get_curr()));
        it1.next_ne();
        it2.next_ne();
      }

    while (it1.has_curr())
      {
        ret_val.append(Tuple(it1.get_curr(), fill_b));
        it1.next_ne();
      }

    while (it2.has_curr())
      {
        ret_val.append(Tuple(fill_a, it2.get_curr()));
        it2.next_ne();
      }

    return ret_val;
  }


  /** @brief Zip two containers using optionals for missing values.

      Like `zip_longest()` but uses `std::optional` to represent missing
      values instead of requiring explicit fill values. This is useful
      when there's no sensible default value, or when you need to
      distinguish between a real value and a fill value.

      @tparam Container1 Type of the first container.
      @tparam Container2 Type of the second container.
      @param a First container.
      @param b Second container.
      @return DynList of pairs of optionals.

      @par Example:
      @code
      DynList<int> a = {1, 2, 3};
      DynList<std::string> b = {"x"};

      auto result = zip_longest_opt(a, b);
      // result contains:
      //   ({1}, {"x"})
      //   ({2}, nullopt)
      //   ({3}, nullopt)

      for (const auto& [opt_a, opt_b] : result)
        {
          if (opt_a) std::cout << "a=" << *opt_a;
          if (opt_b) std::cout << " b=" << *opt_b;
          std::cout << '\n';
        }
      @endcode

      @see zip_longest() for version with explicit fill values.
      @ingroup Algorithms
  */
  template <class Container1, class Container2> [[nodiscard]] inline
  DynList<std::pair<std::optional<typename Container1::Item_Type>,
                    std::optional<typename Container2::Item_Type>>>
  zip_longest_opt(const Container1 & a, const Container2 & b)
  {
    using T1 = typename Container1::Item_Type;
    using T2 = typename Container2::Item_Type;
    using Opt1 = std::optional<T1>;
    using Opt2 = std::optional<T2>;
    DynList<std::pair<Opt1, Opt2>> ret_val;

    auto it1 = a.get_it();
    auto it2 = b.get_it();

    while (it1.has_curr() and it2.has_curr())
      {
        ret_val.append(std::make_pair(Opt1(it1.get_curr()), Opt2(it2.get_curr())));
        it1.next_ne();
        it2.next_ne();
      }

    while (it1.has_curr())
      {
        ret_val.append(std::make_pair(Opt1(it1.get_curr()), std::nullopt));
        it1.next_ne();
      }

    while (it2.has_curr())
      {
        ret_val.append(std::make_pair(std::nullopt, Opt2(it2.get_curr())));
        it2.next_ne();
      }

    return ret_val;
  }


  /** @brief Group consecutive elements by a key function.

      Partitions a container into groups of consecutive elements that
      share the same key (as computed by the key function). Each group
      is a DynList containing all consecutive elements with the same key.

      This is similar to Python's `itertools.groupby()` or SQL's GROUP BY
      (but only for consecutive elements with the same key).

      @tparam T Element type of the container.
      @tparam Container Container type (template parameter).
      @tparam KeyFunc Function type `(const T&) -> Key`.
      @param c Source container.
      @param key_func Function that extracts the grouping key from each element.
      @return DynList of pairs, where each pair contains:
              - first: The key value for the group
              - second: DynList of consecutive elements with that key

      @par Example:
      @code
      DynList<int> nums = {1, 1, 2, 3, 3, 3, 2, 2};

      // Group by identity (consecutive equal values)
      auto groups = group_by(nums, [](int x) { return x; });
      // groups contains:
      //   (1, {1, 1})
      //   (2, {2})
      //   (3, {3, 3, 3})
      //   (2, {2, 2})

      // Group by even/odd
      auto parity_groups = group_by(nums, [](int x) { return x % 2; });
      // parity_groups contains:
      //   (1, {1, 1})      // odd
      //   (0, {2})         // even
      //   (1, {3, 3, 3})   // odd
      //   (0, {2, 2})      // even
      @endcode

      @note Unlike SQL GROUP BY, this only groups CONSECUTIVE elements.
            Non-consecutive elements with the same key will form separate groups.
      @note For grouping all elements regardless of position, sort the
            container first or use a map-based approach.

      @see sequential_groups() for grouping by equality.
      @see unique_sequential() for getting unique consecutive values.
      @ingroup Algorithms
  */
  template <typename T, template <typename> class Container, class KeyFunc>
  [[nodiscard]]
  DynList<std::pair<std::invoke_result_t<KeyFunc, const T&>, DynList<T>>>
  group_by(const Container<T> & c, KeyFunc key_func)
  {
    using Key = std::invoke_result_t<KeyFunc, const T&>;
    using Group = DynList<T>;
    using Result = DynList<std::pair<Key, Group>>;

    Result ret;
    if (c.is_empty())
      return ret;

    auto it = c.get_it();
    Key curr_key = key_func(it.get_curr());
    Group * curr_group = &ret.append(std::make_pair(curr_key, Group())).second;
    curr_group->append(it.get_curr());

    for (it.next_ne(); it.has_curr(); it.next_ne())
      {
        const auto & item = it.get_curr();
        Key new_key = key_func(item);

        if (new_key != curr_key)
          {
            curr_key = new_key;
            curr_group = &ret.append(std::make_pair(curr_key, Group())).second;
          }

        curr_group->append(item);
      }

    return ret;
  }


  /** @brief Group consecutive elements by a key function with custom equality.

      Like `group_by()` but uses a custom equality predicate for comparing keys
      instead of `operator==`. This is useful when the key type doesn't have
      `operator==` defined, or when you need custom comparison semantics.

      @tparam T Element type of the container.
      @tparam Container Container type (template parameter).
      @tparam KeyFunc Function type `(const T&) -> Key`.
      @tparam KeyEqual Predicate type `(const Key&, const Key&) -> bool`.
      @param c Source container.
      @param key_func Function that extracts the grouping key from each element.
      @param key_equal Predicate that returns true if two keys are equal.
      @return DynList of pairs (key, group).

      @par Example:
      @code
      struct Point { double x, y; };
      DynList<Point> points = {{1.0, 2.0}, {1.001, 2.001}, {5.0, 5.0}};

      // Group by approximate x coordinate (within 0.01)
      auto groups = group_by_eq(points,
        [](const Point& p) { return p.x; },
        [](double a, double b) { return std::abs(a - b) < 0.01; });
      // First group: {(1.0, 2.0), (1.001, 2.001)}
      // Second group: {(5.0, 5.0)}
      @endcode

      @see group_by() for version using operator==.
      @ingroup Algorithms
  */
  template <typename T, template <typename> class Container,
            class KeyFunc, class KeyEqual>
  [[nodiscard]]
  DynList<std::pair<std::invoke_result_t<KeyFunc, const T&>, DynList<T>>>
  group_by_eq(const Container<T> & c, KeyFunc key_func, KeyEqual key_equal)
  {
    using Key = std::invoke_result_t<KeyFunc, const T&>;
    using Group = DynList<T>;
    using Result = DynList<std::pair<Key, Group>>;

    Result ret;
    if (c.is_empty())
      return ret;

    auto it = c.get_it();
    Key curr_key = key_func(it.get_curr());
    Group * curr_group = &ret.append(std::make_pair(curr_key, Group())).second;
    curr_group->append(it.get_curr());

    for (it.next_ne(); it.has_curr(); it.next_ne())
      {
        const auto & item = it.get_curr();
        Key new_key = key_func(item);

        if (not key_equal(new_key, curr_key))
          {
            curr_key = new_key;
            curr_group = &ret.append(std::make_pair(curr_key, Group())).second;
          }

        curr_group->append(item);
      }

    return ret;
  }


  /** @brief Group consecutive elements and apply a reducer to each group.

      Combines `group_by()` with a reduction operation. For each group of
      consecutive elements sharing the same key, applies a reducer function
      to produce a single output value.

      This is similar to SQL's `GROUP BY ... aggregate()` pattern.

      @tparam T Element type of the container.
      @tparam Container Container type (template parameter).
      @tparam KeyFunc Function type `(const T&) -> Key`.
      @tparam Reducer Function type `(const DynList<T>&) -> R`.
      @param c Source container.
      @param key_func Function that extracts the grouping key from each element.
      @param reducer Function that reduces each group to a single value.
      @return DynList of pairs (key, reduced_value).

      @par Example:
      @code
      DynList<int> nums = {1, 1, 1, 2, 2, 3, 3, 3, 3};

      // Count elements in each group
      auto counts = group_by_reduce(nums,
        [](int x) { return x; },                    // key = value itself
        [](const DynList<int>& g) { return g.size(); });  // count
      // counts contains: {(1, 3), (2, 2), (3, 4)}

      // Sum elements in each group
      auto sums = group_by_reduce(nums,
        [](int x) { return x; },
        [](const DynList<int>& g) { return sum(g); });
      // sums contains: {(1, 3), (2, 4), (3, 12)}
      @endcode

      @see group_by() for version without reduction.
      @ingroup Algorithms
  */
  template <typename T, template <typename> class Container,
            class KeyFunc, class Reducer>
  [[nodiscard]]
  auto group_by_reduce(const Container<T> & c, KeyFunc key_func, Reducer reducer)
    -> DynList<std::pair<std::invoke_result_t<KeyFunc, const T&>,
                         std::invoke_result_t<Reducer, const DynList<T>&>>>
  {
    using Key = std::invoke_result_t<KeyFunc, const T&>;
    using ReducedType = std::invoke_result_t<Reducer, const DynList<T>&>;
    using Result = DynList<std::pair<Key, ReducedType>>;

    auto groups = group_by(c, key_func);
    Result ret;

    for (auto it = groups.get_it(); it.has_curr(); it.next_ne())
      {
        auto & [key, group] = it.get_curr();
        ret.append(std::make_pair(key, reducer(group)));
      }

    return ret;
  }


} // end namespace Aleph

#endif // AH_FUNCTIONAL_H
