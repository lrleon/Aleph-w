
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

/** @file ah-dry.H
 *  @brief Container traversal and functional operation mixins.
 *
 *  This file provides generic traversal classes and mixins that implement
 *  functional operations (for_each, exists, all, filter, map, etc.) for
 *  containers through their iterators. These follow the DRY principle.
 *
 *  ## Constness
 *
 *  A query never modifies the container: every member that only reads
 *  traverses the container through its constant `traverse()`, which hands
 *  out each item as `const T &`. Only the members meant to modify, such as
 *  `mutable_for_each()` or the non-constant `find_ptr()` and `nth()`, use
 *  the modifiable traversal, and they exist only for containers whose
 *  modifiable `traverse()` hands out `T &`.
 *
 *  ## Operations
 *
 *  The operations are called through `std::invoke`, so a pointer to member
 *  works as a projection: `c.filter(&Item::is_valid)`,
 *  `c.maps(&Point::get_x)`.
 *
 *  ## Eager results; lazy pipelines with std::views
 *
 *  Every member builds its whole result, usually a `DynList`. A chain such
 *  as `c.filter(p).maps(f).take(3)` therefore visits every item and builds
 *  two intermediate lists. For a lazy pipeline that touches only what it
 *  needs, use the C++20 views, which work with the Aleph containers that
 *  provide `begin()`/`end()` (see ah-ranges.H, which requires GCC 12 or
 *  later):
 *
 *  @code
 *  for (int x : c | std::views::filter(p) | std::views::transform(f)
 *                 | std::views::take(3))
 *    use(x);  // f is called only three times
 *  @endcode
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

#ifndef AH_DRY_H
#define AH_DRY_H

#include "ahFunctional.H"
#include <ah-concepts.H>
#include <concepts>
#include <functional>
#include <limits>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <ah-errors.H>

namespace Aleph {
template <class>
class Array;
}

namespace Aleph::dry_detail
{
  /** @brief Operation that needs to modify the item it receives.

      Only declared: the mixins ask, in unevaluated contexts, whether a
      container's modifiable `traverse()` accepts it, which tells whether
      that traversal hands out modifiable references.

      @tparam T item type.
  */
  template <class T>
  struct Modifying_Probe
  {
    /** @brief Declared only; never called.
        @param[in] item item to modify.
        @return whether the traversal goes on.
    */
    bool operator () (T & item) const noexcept;
  };

  /** @brief Operation that only reads the item and never throws.

      Only declared: the mixins ask, in unevaluated contexts, whether a
      container's constant `traverse()` is `noexcept` with it, which tells
      whether the traversal itself may throw.

      @tparam T item type.
  */
  template <class T>
  struct Reading_Probe
  {
    /** @brief Declared only; never called.
        @param[in] item item to read.
        @return whether the traversal goes on.
    */
    bool operator () (const T & item) const noexcept;
  };

  /** @brief The modifiable traversal of `Container` hands out `T &`.

      True when `c.traverse(op)` on a non-constant container accepts an
      operation that needs a modifiable reference. The answer is reliable
      for containers whose `traverse()` overloads are constrained by what
      they hand out; an unconstrained `traverse()` always answers yes.

      @tparam Container the CRTP-derived container.
      @tparam T item type.
  */
  template <class Container, class T>
  concept MutablyTraversable =
      requires(Container & c, Modifying_Probe<T> & op) { c.traverse(op); };

  /** @brief `Container` can be compared as a set: it has `size()`, a
      `Key_Type` and a `search()` that returns a pointer.

      @tparam Container the CRTP-derived container.
  */
  template <class Container>
  concept SetSearchable =
      requires(const Container & c, const typename Container::Key_Type & k)
        {
          c.size();
          c.search(k) == nullptr;
        };

  /** @brief The constant traversal of `Container` cannot throw by itself.

      True if `traverse()` on a constant container is `noexcept` when its
      operation is. A container whose `traverse()` has no `noexcept`
      specification is taken as possibly throwing.

      @tparam Container the CRTP-derived container.
      @tparam T item type.
  */
  template <class Container, class T>
  inline constexpr bool const_traversal_is_noexcept =
      noexcept(std::declval<const Container &>().traverse(std::declval<Reading_Probe<T> &>()));

  /** @brief The modifiable traversal of `Container` cannot throw by itself.

      Only meaningful when `MutablyTraversable<Container, T>` holds.

      @tparam Container the CRTP-derived container.
      @tparam T item type.
  */
  template <class Container, class T>
  inline constexpr bool mutable_traversal_is_noexcept =
      noexcept(std::declval<Container &>().traverse(std::declval<Modifying_Probe<T> &>()));

  /** @brief Advancing an iterator positioned on an item cannot throw. */
  template <class It>
  inline constexpr bool advance_is_noexcept = []
    {
      if constexpr (requires(It & it) { it.next_ne(); })
        return noexcept(std::declval<It &>().next_ne());
      else
        return noexcept(std::declval<It &>().next());
    }();

  /** @brief Reading the current item of an iterator positioned on one
      cannot throw. */
  template <class It>
  inline constexpr bool current_is_noexcept = []
    {
      if constexpr (requires(It & it) { it.get_curr_ne(); })
        return noexcept(std::declval<It &>().get_curr_ne());
      else
        return noexcept(std::declval<It &>().get_curr());
    }();

  /** @brief Advance an iterator that is positioned on an item.

      Uses `next_ne()` when the iterator has it: the caller has already
      checked `has_curr()`, so the check of `next()` is redundant.

      @param[in,out] it iterator positioned on an item.
      @throw anything that `next()` throws, if the iterator has no
             `next_ne()`.
  */
  template <class It>
  void advance(It & it) noexcept(advance_is_noexcept<It>)
  {
    if constexpr (requires { it.next_ne(); })
      it.next_ne();
    else
      it.next();
  }

  /** @brief Current item of an iterator that is positioned on one.

      Uses `get_curr_ne()` when the iterator has it: the caller has
      already checked `has_curr()`.

      @param[in] it iterator positioned on an item.
      @return what `get_curr_ne()`, or else `get_curr()`, returns.
      @throw anything that `get_curr()` throws, if the iterator has no
             `get_curr_ne()`.
  */
  template <class It>
  decltype(auto) current(It & it) noexcept(current_is_noexcept<It>)
  {
    if constexpr (requires { it.get_curr_ne(); })
      return it.get_curr_ne();
    else
      return it.get_curr();
  }

  using Aleph::concepts_detail::map_result_t;

  /** @brief Item type of a container returned by the operation of
      `flat_map()`: its `Item_Type`, or else its `value_type`. */
  template <class C>
  struct inner_item
  {
    using type = typename C::value_type;
  };

  /** @brief Item type of an Aleph container returned by the operation of
      `flat_map()`. */
  template <class C>
    requires requires { typename C::Item_Type; }
  struct inner_item<C>
  {
    using type = typename C::Item_Type;
  };

  /** @brief `op` folds an accumulator `Acc` with an item `T`.

      `acc = op(acc, item)` must be valid, with the accumulator passed as an
      lvalue or as an rvalue.

      @tparam Op the folding operation.
      @tparam Acc the accumulator type.
      @tparam T the item type.
  */
  template <class Op, class Acc, class T>
  concept FoldOperation =
      requires(Acc & acc, Op & op, const T & item) { acc = std::invoke(op, std::move(acc), item); }
      or requires(Acc & acc, Op & op, const T & item) { acc = std::invoke(op, acc, item); };

  /** @brief `op` folds an item `T` with an accumulator `Acc` from the right.

      `acc = op(item, acc)` must be valid, with the accumulator passed as an
      lvalue or as an rvalue.

      @tparam Op the folding operation.
      @tparam Acc the accumulator type.
      @tparam T the item type.
  */
  template <class Op, class Acc, class T>
  concept FoldRightOperation =
      requires(Acc & acc, Op & op, const T & item) { acc = std::invoke(op, item, std::move(acc)); }
      or requires(Acc & acc, Op & op, const T & item) { acc = std::invoke(op, item, acc); };

  /** @brief Store in `acc` the result of a folding step `step()`.

      If the step returns a reference, which may be to `acc` itself, the
      result is moved out first, so that `acc` is never assigned from
      itself.

      @param[in,out] acc the accumulator.
      @param[in] step callable that runs the folding step.
      @throw anything that `step` or the assignment of `Acc` throws.
  */
  template <class Acc, class Step>
  void assign_fold(Acc & acc, Step && step)
  {
    if constexpr (std::is_reference_v<std::invoke_result_t<Step &>>)
      {
        Acc next = step();
        acc = std::move(next);
      }
    else
      acc = step();
  }

  /** @brief One step of a left fold: `acc = op(acc, item)`.

      The accumulator is moved into `op` when `op` accepts an rvalue, so
      that folding into a container costs O(1) per step instead of a copy
      of the whole accumulator, as `std::accumulate` does since C++20.

      @param[in,out] acc the accumulator.
      @param[in] op the folding operation.
      @param[in] item the current item.
      @throw anything that `op` or the assignment of `Acc` throws.
  */
  template <class Acc, class Op, class T>
  void fold_left_step(Acc & acc, Op & op, const T & item)
  {
    if constexpr (std::is_invocable_v<Op &, Acc &&, const T &>)
      assign_fold(acc, [&] () -> decltype(auto) { return std::invoke(op, std::move(acc), item); });
    else
      assign_fold(acc, [&] () -> decltype(auto) { return std::invoke(op, acc, item); });
  }

  /** @brief One step of a right fold: `acc = op(item, acc)`.

      @param[in,out] acc the accumulator.
      @param[in] op the folding operation.
      @param[in] item the current item.
      @throw anything that `op` or the assignment of `Acc` throws.
  */
  template <class Acc, class Op, class T>
  void fold_right_step(Acc & acc, Op & op, const T & item)
  {
    if constexpr (std::is_invocable_v<Op &, const T &, Acc &&>)
      assign_fold(acc, [&] () -> decltype(auto) { return std::invoke(op, item, std::move(acc)); });
    else
      assign_fold(acc, [&] () -> decltype(auto) { return std::invoke(op, item, acc); });
  }
} // namespace Aleph::dry_detail

/**  Generic traversal of the container through its iterator.

     This class implements a conditioned traversal on the container
     through its iterator.

     It is assumed that `Container` exports its iterator with

     typename Container::Iterator

     The modifiable traversal hands out what the iterator's `get_curr()`
     returns; the constant one hands out each item as `const T &`.

     \ingroup Sequences
*/
template <class Container>
struct GenericTraverse
{
private:
  // True if building the iterator on `Source`, walking it and calling
  // `operation` with `Arg` cannot throw.
  template <class Operation, class Arg, class Source>
  static constexpr bool traverse_is_noexcept() noexcept
  {
    using It = typename Container::Iterator;
    return std::is_nothrow_constructible_v<It, Source>
           and noexcept(std::declval<It &>().has_curr())
           and Aleph::dry_detail::advance_is_noexcept<It>
           and Aleph::dry_detail::current_is_noexcept<It>
           and Aleph::NothrowPredicateWith<Operation &, Arg>;
  }

  // The container a constant traversal builds its iterator on: the
  // constant one, or the same object without `const` when the iterator
  // only accepts a modifiable container; the traversal only reads.
  template <class Deferred>
  using const_source_t =
      std::conditional_t<std::is_constructible_v<
                             typename Aleph::concepts_detail::defer<Container, Deferred>::type::Iterator,
                             const typename Aleph::concepts_detail::defer<Container, Deferred>::type &>,
                         const typename Aleph::concepts_detail::defer<Container, Deferred>::type &,
                         typename Aleph::concepts_detail::defer<Container, Deferred>::type &>;

public:
  /** Traverse the container via its iterator and performs a conditioned
      operation on each item.

      `traverse(operation)` instantiates the internal iterator of the
      class and traverses each item performing `operation(item)`.

      `operation` must have the following signature:

      bool operation(typename Container::Item_Type & item)

      If `operation(item)` returns `true` then the iterator is advanced
      and the next item processed. Otherwise, the traversal stops.

      \param[in] operation to be performed on each item
      \return `true` if all the items were visited (`operation` on each
      one always returned `true`) or `false` if the traversal was
      stopped because there was a `false` result on an item.
      \throw anything that could throw `operation`
  */
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, typename Aleph::concepts_detail::traverse_arg<Container, Operation>::type>
  bool traverse(Operation & operation)
    noexcept(traverse_is_noexcept<Operation,
                                  typename Aleph::concepts_detail::traverse_arg<Container, Operation>::type,
                                  Container &>())
  {
    for (typename Container::Iterator it(*static_cast<Container *>(this)); it.has_curr();
         Aleph::dry_detail::advance(it))
      if (not operation(Aleph::dry_detail::current(it)))
        return false;
    return true;
  }

  /** @brief Traverse a constant container; each item is handed out as a
             constant reference.

      `operation` must accept `const Item_Type &`: an operation that needs
      to modify the item does not compile on a constant container.

      @param[in] operation to be performed on each item.
      @return `true` if all the items were visited; `false` if
              `operation` stopped the traversal.
      @throw anything that `operation` throws.
  */
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, typename Aleph::concepts_detail::traverse_carg<Container, Operation>::type>
  bool traverse(Operation & operation) const
    noexcept(traverse_is_noexcept<Operation,
                                  typename Aleph::concepts_detail::traverse_carg<Container, Operation>::type,
                                  const_source_t<Operation>>())
  {
    using Source = const_source_t<Operation>;
    auto & self = *const_cast<Container *>(static_cast<const Container *>(this));
    for (typename Container::Iterator it(static_cast<Source>(self)); it.has_curr();
         Aleph::dry_detail::advance(it))
      {
        const auto & item = Aleph::dry_detail::current(it);
        if (not operation(item))
          return false;
      }
    return true;
  }

  /** @brief Overload of `traverse(Operation &) const` that accepts rvalues.

      The operation is passed (invoked) as a named lvalue to the lvalue
      overload rather than being perfectly forwarded.

      @param[in] operation to be performed on each item.
      @return `true` if all the items were visited; `false` otherwise.
      @throw anything that `operation` throws.
  */
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, typename Aleph::concepts_detail::traverse_carg<Container, Operation>::type>
  bool traverse(Operation && operation) const
    noexcept(traverse_is_noexcept<Operation,
                                  typename Aleph::concepts_detail::traverse_carg<Container, Operation>::type,
                                  const_source_t<Operation>>())
  {
    return traverse<Operation>(operation);
  }

  /** @brief Overload of `traverse(Operation &)` that accepts rvalues.

      The operation is passed (invoked) as a named lvalue to the lvalue
      overload rather than being perfectly forwarded.

      @param[in] operation to be performed on each item.
      @return `true` if all the items were visited; `false` otherwise.
      @throw anything that `operation` throws.
  */
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, typename Aleph::concepts_detail::traverse_arg<Container, Operation>::type>
  bool traverse(Operation && operation)
    noexcept(traverse_is_noexcept<Operation,
                                  typename Aleph::concepts_detail::traverse_arg<Container, Operation>::type,
                                  Container &>())
  {
    return traverse<Operation>(operation);
  }
};

/** @brief Invoke `c.traverse(op)` as a free function (lvalue operation).
 *
 *  Convenience wrapper so that `traverse(container, op)` can be used in
 *  generic code without depending on the container type. The container is
 *  constant, so `op` receives each item as a constant reference.
 *
 *  @tparam Container Any type that provides a `traverse(Operation&)` member.
 *  @tparam Operation Callable with signature `bool(const Item_Type &)`.
 *  @param[in] c  Container to traverse.
 *  @param[in] op Operation to apply to each element; return `false` to stop.
 *  @return `true` if all elements were visited, `false` if traversal was stopped.
 *  @throws anything that `op` throws.
 */
template <class Container, class Operation>
bool traverse(const Container & c, Operation & op) noexcept(noexcept(c.traverse(op)))
{
  return c.traverse(op);
}

/** @brief Invoke `c.traverse(op)` as a free function (rvalue overload).
 *
 *  Overload for temporary / rvalue callables such as lambda expressions.
 *  The operation @p op is passed (invoked) as a named lvalue to the member
 *  overload rather than being perfectly forwarded.
 *
 *  @tparam Container Any type that provides a `traverse(Operation&)` member.
 *  @tparam Operation Callable with signature `bool(const Item_Type &)`.
 *  @param[in] c  Container to traverse.
 *  @param[in] op Operation passed (invoked) as a named lvalue to the member
 *               overload rather than being perfectly forwarded.
 *  @return `true` if all elements were visited, `false` if traversal was stopped.
 *  @throws anything that `op` throws.
 */
template <class Container, class Operation>
bool traverse(const Container & c, Operation && op) noexcept(noexcept(c.traverse(op)))
{
  return c.traverse(op);
}

/** Common sequential searching methods on containers.

    This class implements common sequential searching on containers.

    \note Take into account that any of these searches takes \f$O(n)\f$ of
    complexity for the worst case and that this is independent of type
    of container. For example, a hash table or a binary search tree,
    exports its own search that is much faster that any of these
    primitives. As an advice: do not use this method inside loops. If
    you find yourself in this situation, then consider to revise your
    design and to use another search method. Use these primitives very
    few times for algorithms whose complexity is by far greater that
    \f$O(n)\f$ and when you cannot index with an adequate data
    structure.

    \warning Be very careful about the fact that many of these
    primitives return pointers or references to container's data. In
    many cases, alteration of that data will corrupt the internal
    state of the container.

    \ingroup Sequences
*/
template <class Container, typename Type>
class LocateFunctions
{
  Container * me() noexcept
  {
    return static_cast<Container *>(this);
  }

  const Container * const_me() const noexcept
  {
    return static_cast<const Container *>(this);
  }

  // True if calling `operation` with an item, through the constant
  // traversal of the container, cannot throw.
  template <class Operation>
  static constexpr bool const_search_is_noexcept() noexcept
  {
    using C = typename Aleph::concepts_detail::defer<Container, Operation>::type;
    return std::is_nothrow_invocable_v<Operation &, const Type &>
           and Aleph::dry_detail::const_traversal_is_noexcept<C, Type>;
  }

  // True if calling `operation` with an item, through the modifiable
  // traversal of the container, cannot throw.
  template <class Operation>
  static constexpr bool mutable_search_is_noexcept() noexcept
  {
    using C = typename Aleph::concepts_detail::defer<Container, Operation>::type;
    return std::is_nothrow_invocable_v<Operation &, Type &>
           and Aleph::dry_detail::mutable_traversal_is_noexcept<C, Type>;
  }

  /** True if find_item() cannot throw: neither the search nor building
      the returned tuple, which copies the found item or holds a
      default-constructed one if nothing matches.
  */
  template <class Operation>
  static constexpr bool find_item_is_noexcept() noexcept
  {
    using TT = std::tuple<bool, Type>;
    return const_search_is_noexcept<Operation>()
           and std::is_nothrow_constructible_v<TT, bool, const Type &>
           and std::is_nothrow_default_constructible_v<Type>
           and std::is_nothrow_constructible_v<TT, bool, Type>;
  }

  // Address of the first item that satisfies `operation`, found with the
  // constant traversal, or nullptr.
  template <class Operation>
  const Type * first_const_ptr(Operation & operation) const
    noexcept(const_search_is_noexcept<Operation>())
  {
    const Type * ptr = nullptr;
    const_me()->traverse([&ptr, &operation] (const Type & item)
      {
        if (std::invoke(operation, item))
          {
            ptr = &item;
            return false;
          }
        return true;
      });
    return ptr;
  }

  // Address of the n-th item, found with the constant traversal, or
  // nullptr if there are no more than n items.
  const Type * const_nth_ptr(const size_t n) const
    noexcept(Aleph::dry_detail::const_traversal_is_noexcept<Container, Type>)
  {
    const Type * ptr = nullptr;
    size_t i = 0;
    const_me()->traverse([&ptr, &i, n] (const Type & item)
      {
        if (i++ != n)
          return true;
        ptr = &item;
        return false;
      });
    return ptr;
  }

  // Address of the n-th item, found with the modifiable traversal, or
  // nullptr if there are no more than n items.
  Type * mutable_nth_ptr(const size_t n)
    noexcept(Aleph::dry_detail::mutable_traversal_is_noexcept<Container, Type>)
  {
    Type * ptr = nullptr;
    size_t i = 0;
    me()->traverse([&ptr, &i, n] (Type & item)
      {
        if (i++ != n)
          return true;
        ptr = &item;
        return false;
      });
    return ptr;
  }

public:
  /** Return a properly initialized iterator positioned at the first
      item on the container.

      @return iterator positioned at the first element
  */
  [[nodiscard]] auto get_it() const
  {
    return typename Container::Iterator(*const_me());
  }

  /** Return a properly initialized iterator positioned at the `pos`
      item on the container.

      @param[in] pos zero‑based position
      @return iterator positioned at the requested position
      @throw what the iterator's `next()` throws if `pos` exceeds the
             number of items, usually `overflow_error`.
  */
  [[nodiscard]] auto get_it(const size_t pos) const
  {
    auto ret = typename Container::Iterator(*const_me());
    for (size_t i = 0; i < pos; ++i)
      ret.next();
    return ret;
  }

  /** Alias of `get_it()`.

      @return iterator positioned at the first element
  */
  [[nodiscard]] auto get_itor() const
  {
    return get_it();
  }

  /** Return the n‑th element without bounds checking.

      Available only if the container's modifiable traversal hands out
      modifiable references.

      @warning No bounds checking is performed: `n` must be smaller than
               the number of items.
      @param[in] n zero‑based index
      @return mutable reference to the n‑th element
  */
  template <class C = Container>  // a template so that Clang 14 checks the constraint on use
    requires Aleph::dry_detail::MutablyTraversable<C, Type>
  Type & nth_ne(const size_t n)
    noexcept(Aleph::dry_detail::mutable_traversal_is_noexcept<C, Type>)
  {
    return *mutable_nth_ptr(n);
  }

  /** Constant overload of `nth_ne(size_t)`.

      @warning No bounds checking is performed: `n` must be smaller than
               the number of items.
      @param[in] n zero‑based index
      @return const reference to the n‑th element
  */
  const Type & nth_ne(const size_t n) const
    noexcept(Aleph::dry_detail::const_traversal_is_noexcept<Container, Type>)
  {
    return *const_nth_ptr(n);
  }

  /** Return the n-th item of the container.

      The notion of ordinal depends on the type of container. On a list,
      probably will be the insertion order. On binary search trees will
      be the nth smaller item. On hash tables will be pseudo random.

      Available only if the container's modifiable traversal hands out
      modifiable references; otherwise the constant overload is used.

      \warning Frequent use of this method will definitively degrade the
      performance. Try not to use this method inside loops. In general,
      if you falls in this situation, then consider your design and to
      use a faster approach.

      \param[in] n the nth item to find
      \return a valid reference to the item into the container.
      \throw out_of_range if n is greater or equal that the size of
      container.
   */
  template <class C = Container>  // a template so that Clang 14 checks the constraint on use
    requires Aleph::dry_detail::MutablyTraversable<C, Type>
  Type & nth(const size_t n)
  {
    Type * ptr = mutable_nth_ptr(n);
    // Test the pointer, not the count: i == n + 1 overflows for n == SIZE_MAX
    ah_out_of_range_error_if(ptr == nullptr) << "index out of range";
    return *ptr;
  }

  /** Const overload of `nth(size_t)`.

      @param[in] n zero‑based index
      @return const reference to the n‑th element
      @throw out_of_range if n is out of range
  */
  const Type & nth(const size_t n) const
  {
    const Type * ptr = const_nth_ptr(n);
    ah_out_of_range_error_if(ptr == nullptr) << "index out of range";
    return *ptr;
  }

  /** Find a pointer to an item in the container according to a
      searching criterion.

      `find_ptr(operation)` traverses the container and on each item
      performs `operation(item)`. If the result of `operation` is `true`,
      then the traversal is stopped and a pointer to the current item
      (which matches `operation`) is returned.

      `operation` must have the following signature:

      bool operation(typename Container::Item_Type & item)

      Available only if the container's modifiable traversal hands out
      modifiable references; otherwise the constant overload is used.

      \warning Frequent use of this method will definitively degrade the
      performance. Try not to use this method inside loops. In general,
      if you fall into the situation, then consider your design and use a
      faster approach.

      \param[in] operation to be performed on each item for matching a
      searching criterion.
      \return a valid pointer to an item if this was found or `nullptr`
      otherwise.
      \throw anything that `operation` throws.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, Type &>
             and Aleph::dry_detail::MutablyTraversable<
                 typename Aleph::concepts_detail::defer<Container, Operation>::type, Type>
  [[nodiscard]] Type * find_ptr(Operation & operation)
    noexcept(mutable_search_is_noexcept<Operation>())
  {
    Type * ptr = nullptr;
    me()->traverse([&ptr, &operation] (Type & item)
      {
        if (std::invoke(operation, item))
          {
            ptr = &item;
            return false;
          }
        return true;
      });
    return ptr;
  }

  /** Const overload of `find_ptr(Operation&)`.

      @param[in] operation matching criterion; it receives each item as
                 `const Type &`.
      @return const pointer to matching element or nullptr
      @throw anything that `operation` throws.
  */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const Type &>
  [[nodiscard]] const Type * find_ptr(Operation & operation) const
    noexcept(const_search_is_noexcept<Operation>())
  {
    return first_const_ptr(operation);
  }

  /** @brief Overload of find_ptr() const that accepts rvalues.
   *
   *  The operation is passed (invoked) as a named lvalue to the
   *  lvalue-reference overload rather than being perfectly forwarded.
   *
   *  @tparam Operation Matching criterion callable.
   *  @param[in] operation matching criterion.
   *  @return const pointer to matching element or nullptr.
   *  @throws May throw the same exceptions as the corresponding lvalue
   *          overload; no additional exception-safety guarantees are provided.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const Type &>
  [[nodiscard]] const Type * find_ptr(Operation && operation) const
    noexcept(const_search_is_noexcept<Operation>())
  {
    return find_ptr<Operation>(operation);
  }

  /** @brief Overload of find_ptr() that accepts rvalues.
   *
   *  The operation is passed (invoked) as a named lvalue to the
   *  lvalue-reference overload rather than being perfectly forwarded.
   *
   *  @tparam Operation Matching criterion callable.
   *  @param[in] operation matching criterion.
   *  @return pointer to matching element or nullptr.
   *  @throws May throw the same exceptions as the corresponding lvalue
   *          overload; no additional exception-safety guarantees are provided.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, Type &>
             and Aleph::dry_detail::MutablyTraversable<
                 typename Aleph::concepts_detail::defer<Container, Operation>::type, Type>
  [[nodiscard]] Type * find_ptr(Operation && operation)
    noexcept(mutable_search_is_noexcept<Operation>())
  {
    return find_ptr<Operation>(operation);
  }

  /** Find the position of an item in the container according to a
      searching criterion.

      `find_index(operation)` traverses the container and on each item
      perform `operation(item)`. If the result of `operation` is `true`,
      then the traversal is stopped and the position of the current item
      (which matches `operation`) is returned.

      `operation` must have the following signature:

      bool operation(const typename Container::Item_Type & item)

      \warning Frequent use of this method will definitively degrade
      the performance. Try not to use this method inside loops. In
      general, if you falls in this situation, the consider your
      design and use a faster approach.

      \param[in] operation to be performed on each item for matching a
      searching criterion.
      \return the position of the first matching item. If no item
      matches, the number of items, as `end()` in the STL.
      \throw anything that `operation` throws.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const Type &>
  [[nodiscard]] size_t find_index(Operation & operation) const
    noexcept(const_search_is_noexcept<Operation>())
  {
    size_t i = 0;
    const_me()->traverse([&i, &operation] (const Type & item)
      {
        if (std::invoke(operation, item))
          return false;
        ++i;
        return true;
      });
    return i;
  }

  /** @brief Overload of find_index() that accepts rvalues.
   *
   *  The operation is passed (invoked) as a named lvalue to the
   *  lvalue-reference overload rather than being perfectly forwarded.
   *
   *  @tparam Operation Search criterion callable.
   *  @param[in] operation matching criterion.
   *  @return position of first matching element or size if none.
   *  @throws May throw the same exceptions as the corresponding lvalue
   *          overload; no additional exception-safety guarantees are provided.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const Type &>
  [[nodiscard]] size_t find_index(Operation && operation) const
    noexcept(const_search_is_noexcept<Operation>())
  {
    return find_index<Operation>(operation);
  }

  /** Safe sequential searching of an item matching a criterion.

      `find_item(operation)` traverses the container and on each item
      perform `operation(item)`. If the result of `operation` is `true`,
      then the traversal is stopped and duple containing a copy of found
      item is returned.

      The method is said safe because returns a copy of item. It needs a
      default constructor of `Type` for the case where nothing matches;
      find_opt() does not.

      `operation` must have the following signature:

      bool operation(const typename Container::Item_Type & item)

      \param[in] operation to be used as searching criterion
      \return a duple std::tuple<bool, Type>. The first field indicates if
      the item was found and the second contains a copy of found
      item. If no item is found, then the first field is `false` and the
      second is the result of default constructor on the type stored in
      the container.
      \throw anything that `operation`, the copy of the found item or the
      default constructor of `Type` throws. The method is `noexcept` only
      when none of them can throw.
      \see find_opt()
  */
  template <class Operation, class T = Type>  // T defers the checks on Type to calls (Clang 14)
    requires Aleph::InvocablePredicateWith<Operation &, const Type &>
             and std::default_initializable<T> and std::copy_constructible<T>
  [[nodiscard]] std::tuple<bool, Type> find_item(Operation & operation) const
    noexcept(find_item_is_noexcept<Operation>())
  {
    using TT = std::tuple<bool, Type>;
    // The container's constant find_ptr(): a container may hide this
    // class's, as DynArray does.
    auto ptr = const_me()->find_ptr(operation);
    return ptr ? TT(true, *ptr) : TT(false, Type());
  }

  /** @brief Overload of find_item() that accepts rvalues.
   *
   *  The operation is passed (invoked) as a named lvalue to the
   *  lvalue-reference overload rather than being perfectly forwarded.
   *
   *  @tparam Operation Searching criterion callable.
   *  @param[in] operation matching criterion.
   *  @return a duple std::tuple<bool, Type>.
   *  @throws May throw the same exceptions as the corresponding lvalue
   *          overload; no additional exception-safety guarantees are provided.
   */
  template <class Operation, class T = Type>
    requires Aleph::InvocablePredicateWith<Operation &, const Type &>
             and std::default_initializable<T> and std::copy_constructible<T>
  [[nodiscard]] std::tuple<bool, Type> find_item(Operation && operation) const
    noexcept(find_item_is_noexcept<Operation>())
  {
    return find_item<Operation>(operation);
  }

  /** @brief Copy of the first item satisfying a criterion, if any.
   *
   *  The `std::optional` counterpart of find_item(): it does not need a
   *  default constructor of `Type`, nor builds an item when nothing
   *  matches.
   *
   *  @tparam Operation Searching criterion callable, with signature
   *          `bool(const Type &)`.
   *  @param[in] operation matching criterion.
   *  @return a copy of the first matching item, or `std::nullopt`.
   *  @throws anything that `operation` or the copy of the item throws.
   */
  template <class Operation, class T = Type>  // T defers the checks on Type to calls (Clang 14)
    requires Aleph::InvocablePredicateWith<Operation &, const Type &>
             and std::copy_constructible<T>
  [[nodiscard]] std::optional<Type> find_opt(Operation & operation) const
  {
    auto ptr = const_me()->find_ptr(operation);
    if (ptr == nullptr)
      return std::nullopt;
    return std::optional<Type>(*ptr);
  }

  /** @brief Overload of find_opt() that accepts rvalues.
   *
   *  @tparam Operation Searching criterion callable.
   *  @param[in] operation matching criterion.
   *  @return a copy of the first matching item, or `std::nullopt`.
   *  @throws anything that `operation` or the copy of the item throws.
   */
  template <class Operation, class T = Type>
    requires Aleph::InvocablePredicateWith<Operation &, const Type &>
             and std::copy_constructible<T>
  [[nodiscard]] std::optional<Type> find_opt(Operation && operation) const
  {
    return find_opt<Operation>(operation);
  }

  /** @brief Test if an item satisfying a criterion is present in the container.
   *
   *  @tparam Operation Search criterion callable.
   *  @param[in] operation matching criterion.
   *  @return true if an item satisfying operation is found, false otherwise.
   *  @throws Anything propagated by evaluating operation(item).
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const Type &>
  [[nodiscard]] bool contains_if(Operation && operation) const
    noexcept(const_search_is_noexcept<Operation>())
  {
    return first_const_ptr(operation) != nullptr;
  }

  /** @brief Test if an item is present in the container using equality.
   *
   *  @param[in] item element to search for.
   *  @return true if item is found, false otherwise.
   *  @throws Anything propagated by `operator==` for `Type`.
   */
  template <class T = Type>  // a template so that Clang 14 checks the constraint on use
    requires requires(const T & a, const T & b) { { a == b } -> std::convertible_to<bool>; }
  [[nodiscard]] bool contains(const Type & item) const
  {
    return contains_if([&item] (const Type & x)
      {
        return x == item;
      });
  }
};

/** Special constructors common to `Aleph-w` (\f$\aleph_\omega\f$)
    containers.

    Basically, the constructors of this class append a sequence of items.

    \warning Note that these constructors require that the class is
    correctly initialized, which not only very often is not the case, but
    it could be impossible in most cases. The most part of situations,
    the derived class must be initialized before to start to append new
    items. But this is not the case during this construction. So, don't
    use this class unless you are absolutely sure that the derived
    class has been initialized.

    \ingroup Sequences
*/
template <class Container, typename T>
struct SpecialCtors
{
  /// Default constructor; it appends nothing.
  SpecialCtors() = default;

  /// Copy constructor; the derived container copies its items.
  SpecialCtors(const SpecialCtors &) = default;

  /// Move constructor; the derived container moves its items.
  SpecialCtors(SpecialCtors &&) noexcept {}

  /// Copy assignment; the derived container copies its items.
  SpecialCtors & operator = (const SpecialCtors &) = default;

  /** @brief Move assignment; the derived container moves its items.
      @return `*this`.
  */
  SpecialCtors & operator = (SpecialCtors &&) noexcept
  {
    return *this;
  }

  /** Build the container by inserting all items of the list `l`.

      @param[in] l list of items to insert
      @throw bad_alloc if there is no enough memory
  */
  SpecialCtors(const Aleph::DynList<T> & l)
  {
    l.for_each([this] (const T & item)
      {
        static_cast<Container *>(this)->append(item);
      });
  }

  /** Construct the container from a range of iterators.

      `It` must behave as an iterator: an arithmetic type, as in
      `DynList<int> l(3, 7)`, does not select this constructor.

      @param[in] b begin iterator
      @param[in] e end iterator
      @throw bad_alloc if there is no enough memory
  */
  template <class It>
    requires (not std::is_arithmetic_v<It>
              and requires(It it) { *it; ++it; it != it; })
  SpecialCtors(It b, It e)
  {
    for (It it = b; it != e; ++it)
      static_cast<Container *>(this)->append(*it);
  }

  /** Construct the container from an initializer list.

      @param[in] l initializer list of items
      @throw bad_alloc if there is no enough memory
  */
  SpecialCtors(std::initializer_list<T> l)
  {
    for (const auto & item : l)
      static_cast<Container *>(this)->append(item);
  }
};

/** Common methods to the `Aleph-w` (\f$\aleph_\omega\f$) containers.

    This class contains many and practice methods that are common to any
    `Aleph-w` (\f$\aleph_\omega\f$) container.

    The members that only read traverse the container with its constant
    `traverse()`, so their operations receive each item as `const T &`.
    The operations are called through `std::invoke`: a pointer to member
    works as a projection.

    \ingroup Sequences
*/
template <class Container, typename T>
class FunctionalMethods
{
  Container * me() noexcept
  {
    return static_cast<Container *>(this);
  }

  const Container * const_me() const noexcept
  {
    return static_cast<const Container *>(this);
  }

  // Constant traversal handing each item, as `const T &`, to `operation`.
  template <class Operation>
  bool const_traverse(Operation && operation) const
  {
    return const_me()->traverse(operation);
  }

public:
  /** Appends a new element into the container by constructing it
      from the given args.

      `emplace(args)` builds a `T` from `args` with direct
      initialization, as `T item(args...)`, and moves it into the
      container with `append()`. It accepts only the arguments a
      constructor of `T` takes: unlike a cast, it never converts a pointer
      to an integer nor drops `const`.

      \note The semantic of append depends on container. In general,
      this has some sense for lists and arrays, and it means insertion at
      the end of sequence. On other type of container `append()` is
      equivalent to `insert()`.

      \param[in] args variadic arguments list
      \throw bad_alloc if there is no enough memory, and anything that
      the constructor of `T` throws.
  */
  template <typename... Args>
    requires std::constructible_from<T, Args...>
             and requires(typename Aleph::concepts_detail::defer<Container, std::tuple<Args...>>::type & c, T && x)
               { c.append(std::move(x)); }
  void emplace(Args &&... args)
  {
    T item(std::forward<Args>(args)...);
    (void) me()->append(std::move(item));
  }

  /** \overload emplace(Args && ... args) */
  template <typename... Args>
    requires std::constructible_from<T, Args...>
             and requires(typename Aleph::concepts_detail::defer<Container, std::tuple<Args...>>::type & c, T && x)
               { c.append(std::move(x)); }
  void emplace_end(Args &&... args)
  {
    T item(std::forward<Args>(args)...);
    (void) me()->append(std::move(item));
  }

  /** Insert a new element into the container by constructing it
      from the given args.

      `emplace_ins(args)` builds a `T` from `args` with direct
      initialization, as `T item(args...)`, and moves it into the
      container with `insert()`.

      \note The semantic of `insert()` depends on the container. In general,
      this has some sense for lists and arrays, and it means insertion at
      the beginning of the sequence. On another type of container `append()`
      is equivalent to `insert()`.

      \param[in] args variadic arguments list
      \throw bad_alloc if there is no enough memory, and anything that
      the constructor of `T` throws.
  */
  template <typename... Args>
    requires std::constructible_from<T, Args...>
             and requires(typename Aleph::concepts_detail::defer<Container, std::tuple<Args...>>::type & c, T && x)
               { c.insert(std::move(x)); }
  void emplace_ins(Args &&... args)
  {
    T item(std::forward<Args>(args)...);
    (void) me()->insert(std::move(item));
  }

  /** Insert n variadic items

      @param[in] args items to be inserted
      @return the number of inserted items
      @throw bad_alloc if there is no enough memory; the items inserted
      before stay inserted.
  */
  template <typename... Args>
    requires (std::convertible_to<const Args &, const T &> and ...)
             and requires(typename Aleph::concepts_detail::defer<Container, std::tuple<Args...>>::type & c, const T & x)
               { c.insert(x); }
  size_t ninsert(const Args &... args)
  {
    ((void) me()->insert(static_cast<const T &>(args)), ...);
    return sizeof...(args);
  }

  /** Append n variadic items

      @param[in] args items to be appended
      @return the number of appended items
      @throw bad_alloc if there is no enough memory; the items appended
      before stay appended.
  */
  template <typename... Args>
    requires (std::convertible_to<const Args &, const T &> and ...)
             and requires(typename Aleph::concepts_detail::defer<Container, std::tuple<Args...>>::type & c, const T & x)
               { c.append(x); }
  size_t nappend(const Args &... args)
  {
    ((void) me()->append(static_cast<const T &>(args)), ...);
    return sizeof...(args);
  }

  /** Traverse all the container and performs an operation on each
      element.

      `for_each(operation)` traverses the container, and on each element
      `item` is performed `operation(item)`.

      `operation` must have the following signature:

      void operation(const T & item)

      Overloading of this method allows that the signature can be
      lightly different; for example, remove the reference. To modify the
      items, use mutable_for_each().

      \param[in] operation to be done on each element.
      \throw anything that can throw `operation`
  */
  template <class Operation>
    requires Aleph::InvocableWith<Operation &, const T &>
  void for_each(Operation & operation) const
  {
    const_traverse([&operation] (const T & item)
      {
        std::invoke(operation, item);
        return true;
      });
  }

  /** @brief Overload of for_each() that accepts rvalues.
   *
   *  The operation is passed (invoked) as a named lvalue to the
   *  lvalue-reference overload rather than being perfectly forwarded.
   *
   *  @tparam Operation Callable with signature void(const T &).
   *  @param[in] operation to be performed on each item.
   *  @throws May throw the same exceptions as the corresponding lvalue
   *          overload; no additional exception-safety guarantees are provided.
   */
  template <class Operation>
    requires Aleph::InvocableWith<Operation &, const T &>
  void for_each(Operation && operation) const
  {
    for_each<Operation>(operation);
  }

  /** @brief Alias of for_each().
   *  @param[in] operation to be performed on each item.
   *  @throws anything that `operation` throws.
   */
  template <class Operation>
    requires Aleph::InvocableWith<Operation &, const T &>
  void each(Operation & operation) const
  {
    for_each(operation);
  }

  /** @brief Alias of for_each() that accepts rvalues.
   *
   *  The operation is passed (invoked) as a named lvalue to the
   *  lvalue-reference overload rather than being perfectly forwarded.
   *
   *  @tparam Operation Callable with signature void(const T &).
   *  @param[in] operation to be performed on each item.
   *  @throws May throw the same exceptions as the corresponding lvalue
   *          overload; no additional exception-safety guarantees are provided.
   */
  template <class Operation>
    requires Aleph::InvocableWith<Operation &, const T &>
  void each(Operation && operation) const
  {
    for_each<Operation>(operation);
  }

  /** Traverse the container starting at `pos` taking one item every `slice`,
      performing an operation on each visited element.

      `each(pos, slice, operation)` visits the items at positions `pos`,
      `pos + slice`, `pos + 2 slice`, ... and on each one performs
      `operation(item)`, which receives it as `const T &`. Nothing is
      visited if `slice` is zero or `pos` is not smaller than the number
      of items.

      \param[in] pos position of the first visited item (0-based)
      \param[in] slice distance between two consecutive visited items
      \param[in] operation to be done on each visited element.
      \throw anything that can throw `operation`
  */
  template <class Operation>
    requires Aleph::InvocableWith<Operation &, const T &>
  void each(const size_t pos, const size_t slice, Operation & operation) const
  {
    if (slice == 0)
      return;
    size_t i = 0;
    size_t next = pos;
    const_traverse([&i, &next, slice, &operation] (const T & item)
      {
        if (i++ != next)
          return true;
        std::invoke(operation, item);
        if (std::numeric_limits<size_t>::max() - next < slice)
          return false;  // the next position does not exist
        next += slice;
        return true;
      });
  }

  /// \overload each(size_t pos, size_t slice, Operation & operation) const
  template <class Operation>
    requires Aleph::InvocableWith<Operation &, const T &>
  void each(const size_t pos, const size_t slice, Operation && operation) const
  {
    each<Operation>(pos, slice, operation);
  }

  /** Traverse the container starting at `pos` taking one item every
      `slice`, performing an operation that may modify each visited item.

      Available only if the container's modifiable traversal hands out
      modifiable references. Be very careful with the fact that this
      method allows modifying the elements themselves, which could badly
      alter the internal state of the container. This would be the case
      for heaps, binary trees and hash tables.

      \param[in] pos position of the first visited item (0-based)
      \param[in] slice distance between two consecutive visited items
      \param[in] operation to be done on each visited element, with
      signature `void(T &)`.
      \throw anything that can throw `operation`
  */
  template <class Operation>
    requires Aleph::InvocableWith<Operation &, T &>
             and Aleph::dry_detail::MutablyTraversable<
                 typename Aleph::concepts_detail::defer<Container, Operation>::type, T>
  void each(const size_t pos, const size_t slice, Operation & operation)
  {
    if (slice == 0)
      return;
    size_t i = 0;
    size_t next = pos;
    me()->traverse([&i, &next, slice, &operation] (T & item)
      {
        if (i++ != next)
          return true;
        std::invoke(operation, item);
        if (std::numeric_limits<size_t>::max() - next < slice)
          return false;  // the next position does not exist
        next += slice;
        return true;
      });
  }

  /// \overload each(size_t pos, size_t slice, Operation & operation)
  template <class Operation>
    requires Aleph::InvocableWith<Operation &, T &>
             and Aleph::dry_detail::MutablyTraversable<
                 typename Aleph::concepts_detail::defer<Container, Operation>::type, T>
  void each(const size_t pos, const size_t slice, Operation && operation)
  {
    each<Operation>(pos, slice, operation);
  }

  /** @brief Apply a mutable operation to each element of the container.
   *
   *  Traverses the container and calls @p operation on every element,
   *  allowing the operation to modify each item in place. Available only
   *  if the container's modifiable traversal hands out modifiable
   *  references.
   *
   *  @tparam Operation Callable with signature `void(T &)`.
   *  @param  operation Functor applied to each element; may modify it.
   *  @throws Anything that @p operation or the underlying `traverse` may throw.
   */
  template <class Operation>
    requires Aleph::InvocableWith<Operation &, T &>
             and Aleph::dry_detail::MutablyTraversable<
                 typename Aleph::concepts_detail::defer<Container, Operation>::type, T>
  void mutable_for_each(Operation & operation)
  {
    me()->traverse([&operation] (T & item)
      {
        std::invoke(operation, item);
        return true;
      });
  }

  /** \overload mutable_for_each(Operation & operation) */
  template <class Operation>
    requires Aleph::InvocableWith<Operation &, T &>
             and Aleph::dry_detail::MutablyTraversable<
                 typename Aleph::concepts_detail::defer<Container, Operation>::type, T>
  void mutable_for_each(Operation && operation)
  {
    mutable_for_each<Operation>(operation);
  }

  /** Check if all the elements of the container satisfy a condition.

      `all(operation)` checks if for each element `item` of container
      `operation(item)` returns `true`.

      This method has complexity \f$O(n)\f$ in average and worst case.

      \param[in] operation to be used as condition
      \return `true` if all the elements satisfy the criterion: `false`
      otherwise.
      \throw anything that could throw `operation`
  */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] bool all(Operation & operation) const
  {
    return const_traverse([&operation] (const T & item) -> bool
      {
        return static_cast<bool>(std::invoke(operation, item));
      });
  }

  /** @brief Overload of all() that accepts rvalues.
   *
   *  The operation is passed (invoked) as a named lvalue to the
   *  lvalue-reference overload rather than being perfectly forwarded.
   *
   *  @tparam Operation Callable with signature bool(const T &).
   *  @param[in] operation condition to test.
   *  @return `true` if all elements satisfy the condition.
   *  @throws May throw the same exceptions as the corresponding lvalue
   *          overload; no additional exception-safety guarantees are provided.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] bool all(Operation && operation) const
  {
    return all<Operation>(operation);
  }

  /** Test for existence in the container of an element satisfying a
      criterion.

      `exists(op)` returns `true` if it exists any element `item` in container
      for which `op(item)` return `true`.

      This method has complexity \f$O(n)\f$ in average and worst case.

      \param[in] op operation for testing existence
      \return `true` if it exists an item for which `op` return true;
      `false` otherwise.
      \throw anything that could throw `op`
  */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] bool exists(Operation & op) const
  {
    return not const_traverse([&op] (const T & item) -> bool
      {
        return not std::invoke(op, item);
      });
  }

  /** @brief Overload of exists() that accepts rvalues.
   *
   *  The operation is passed (invoked) as a named lvalue to the
   *  lvalue-reference overload rather than being perfectly forwarded.
   *
   *  @tparam Operation Callable with signature bool(const T &).
   *  @param[in] op predicate to test.
   *  @return `true` if any element satisfies the predicate.
   *  @throws May throw the same exceptions as the corresponding lvalue
   *          overload; no additional exception-safety guarantees are provided.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] bool exists(Operation && op) const
  {
    return exists<Operation>(op);
  }

  /** @brief Test that no element satisfies a criterion.
   *
   *  Equivalent to `not exists(op)`; it stops at the first element that
   *  satisfies `op`.
   *
   *  @tparam Operation Callable with signature bool(const T &).
   *  @param[in] op predicate to test.
   *  @return `true` if no element satisfies `op`, including when the
   *          container is empty.
   *  @throws anything that `op` throws.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] bool none(Operation & op) const
  {
    return not exists(op);
  }

  /** @brief Overload of none() that accepts rvalues.
   *  @param[in] op predicate to test.
   *  @return `true` if no element satisfies `op`.
   *  @throws anything that `op` throws.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] bool none(Operation && op) const
  {
    return none<Operation>(op);
  }

  /** @brief Count the elements that satisfy a criterion.
   *
   *  @tparam Operation Callable with signature bool(const T &).
   *  @param[in] op predicate to test.
   *  @return the number of elements for which `op` is `true`.
   *  @throws anything that `op` throws.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] size_t count_if(Operation & op) const
  {
    size_t count = 0;
    const_traverse([&count, &op] (const T & item)
      {
        if (std::invoke(op, item))
          ++count;
        return true;
      });
    return count;
  }

  /** @brief Overload of count_if() that accepts rvalues.
   *  @param[in] op predicate to test.
   *  @return the number of elements for which `op` is `true`.
   *  @throws anything that `op` throws.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] size_t count_if(Operation && op) const
  {
    return count_if<Operation>(op);
  }

  /** Map the elements of the container.

      `maps(op)` produces a dynamic list resulting of mapping of each
      element of container `item` to the result of operation
      `op(item)`.

      The item type of the result is that of `op(item)`, without
      references and `const`: mapping `int` items with `x / 2.0` gives a
      `DynList<double>`, and with `std::to_string` a
      `DynList<std::string>`. To force a type, give it as the first
      template argument:

          c.template maps<long>([] (int item) { return item * 4; });

      `operation` should have the following signature:

      R operation(const T & item)

      \param[in] op operation to be performed in order to do the
      transformation on an `item`; it may also be a pointer to member,
      as `&Point::get_x`.
      \return a `DynList` object containing the mapped items. The
      order of resulting list is the same than the order of visit of the
      iterator for the container.
      \throw anything that could throw `op` or `bad_alloc` if there is
      no enough memory
  */
  template <typename R = void, class Operation>
    requires Aleph::InvocableWith<Operation &, const T &>
             and requires(Aleph::DynList<Aleph::dry_detail::map_result_t<R, Operation &, T>> & l,
                          Operation & op, const T & item)
               { l.append(std::invoke(op, item)); }
  [[nodiscard]] Aleph::DynList<Aleph::dry_detail::map_result_t<R, Operation &, T>>
  maps(Operation & op) const
  {
    Aleph::DynList<Aleph::dry_detail::map_result_t<R, Operation &, T>> ret_val;
    const_traverse([&ret_val, &op] (const T & item)
      {
        ret_val.append(std::invoke(op, item));
        return true;
      });
    return ret_val;
  }

  /** @brief Overload of maps() that accepts rvalues.
   *
   *  The operation is passed (invoked) as a named lvalue to the
   *  lvalue-reference overload rather than being perfectly forwarded.
   *
   *  @tparam R item type of the result, or `void` (the default) to take
   *          the type that `op` returns.
   *  @tparam Operation Transformation callable.
   *  @param[in] op transformation operation.
   *  @return DynList with mapped elements.
   *  @throws May throw the same exceptions as the corresponding lvalue
   *          overload; no additional exception-safety guarantees are provided.
   */
  template <typename R = void, class Operation>
    requires Aleph::InvocableWith<Operation &, const T &>
             and requires(Aleph::DynList<Aleph::dry_detail::map_result_t<R, Operation &, T>> & l,
                          Operation & op, const T & item)
               { l.append(std::invoke(op, item)); }
  [[nodiscard]] Aleph::DynList<Aleph::dry_detail::map_result_t<R, Operation &, T>>
  maps(Operation && op) const
  {
    return maps<R, Operation>(op);
  }

  /** Conditional mapping of the elements of the container.

      `maps_if(prop, op)` traverses each item of container, on each
      item it tests the proposition `prop`. If this last is true, then
      the item is mapped through the function `op(item)`.

      The item type of the result is deduced as in maps().

      \param[in] op operation to be performed in order to do the
      transformation on an `item`.
      \param[in] prop a lambda returning a `bool` which perform the
      logical test.
      \return a `DynList` object containing the mapped items. The
      order of resulting list is the same as the order of visit of the
      iterator for the container.
      \throw anything that could throw `op` or `bad_alloc` if there is
      no enough memory
  */
  template <typename R = void, class Prop, class Operation>
    requires Aleph::InvocablePredicateWith<Prop &, const T &>
             and Aleph::InvocableWith<Operation &, const T &>
             and requires(Aleph::DynList<Aleph::dry_detail::map_result_t<R, Operation &, T>> & l,
                          Operation & op, const T & item)
               { l.append(std::invoke(op, item)); }
  [[nodiscard]] Aleph::DynList<Aleph::dry_detail::map_result_t<R, Operation &, T>>
  maps_if(Prop prop, Operation & op) const
  {
    Aleph::DynList<Aleph::dry_detail::map_result_t<R, Operation &, T>> ret_val;
    const_traverse([&ret_val, &prop, &op] (const T & item)
      {
        if (std::invoke(prop, item))
          ret_val.append(std::invoke(op, item));
        return true;
      });
    return ret_val;
  }

  /** @brief Overload of maps_if() that accepts rvalues.
   *
   *  The operation is passed (invoked) as a named lvalue to the
   *  lvalue-reference overload rather than being perfectly forwarded.
   *
   *  @tparam R item type of the result, or `void` (the default) to take
   *          the type that `op` returns.
   *  @tparam Prop Predicate callable.
   *  @tparam Operation Transformation callable.
   *  @param[in] prop predicate.
   *  @param[in] op transformation operation.
   *  @return DynList with conditionally mapped elements.
   *  @throws May throw the same exceptions as the corresponding lvalue
   *          overload; no additional exception-safety guarantees are provided.
   */
  template <typename R = void, class Prop, class Operation>
    requires Aleph::InvocablePredicateWith<Prop &, const T &>
             and Aleph::InvocableWith<Operation &, const T &>
             and requires(Aleph::DynList<Aleph::dry_detail::map_result_t<R, Operation &, T>> & l,
                          Operation & op, const T & item)
               { l.append(std::invoke(op, item)); }
  [[nodiscard]] Aleph::DynList<Aleph::dry_detail::map_result_t<R, Operation &, T>>
  maps_if(Prop prop, Operation && op) const
  {
    return maps_if<R, Prop, Operation>(prop, op);
  }

  /** Synonym of maps().

      This method is a synonym for maps() with the classic functional name
      "map". It exists because early C++ compilers had difficulty
      distinguishing between the functional map operation and the standard
      library's map container. Modern compilers no longer have this issue.

      @param[in] op transformation operation.
      @return DynList with mapped elements.
      @throw anything that `op` throws, or `bad_alloc`.
      @see maps()
  */
  template <typename R = void, class Operation>
    requires Aleph::InvocableWith<Operation &, const T &>
             and requires(Aleph::DynList<Aleph::dry_detail::map_result_t<R, Operation &, T>> & l,
                          Operation & op, const T & item)
               { l.append(std::invoke(op, item)); }
  [[nodiscard]] Aleph::DynList<Aleph::dry_detail::map_result_t<R, Operation &, T>>
  map(Operation & op) const
  {
    return maps<R, Operation>(op);
  }

  /** @brief Overload of map() that accepts rvalues.
   *
   *  @tparam R item type of the result, or `void` (the default) to take
   *          the type that `op` returns.
   *  @tparam Operation Transformation callable.
   *  @param[in] op transformation operation.
   *  @return DynList with mapped elements.
   *  @throw anything that `op` throws, or `bad_alloc`.
   *  @see maps()
   */
  template <typename R = void, class Operation>
    requires Aleph::InvocableWith<Operation &, const T &>
             and requires(Aleph::DynList<Aleph::dry_detail::map_result_t<R, Operation &, T>> & l,
                          Operation & op, const T & item)
               { l.append(std::invoke(op, item)); }
  [[nodiscard]] Aleph::DynList<Aleph::dry_detail::map_result_t<R, Operation &, T>>
  map(Operation && op) const
  {
    return maps<R, Operation>(op);
  }

  /** Synonym of maps_if().

      This method is a synonym for maps_if() with the classic functional name
      "map_if". It exists for consistency with map() as a synonym of maps().

      @param[in] prop predicate.
      @param[in] op transformation operation.
      @return DynList with conditionally mapped elements.
      @throw anything that `prop` or `op` throws, or `bad_alloc`.
      @see maps_if()
  */
  template <typename R = void, class Prop, class Operation>
    requires Aleph::InvocablePredicateWith<Prop &, const T &>
             and Aleph::InvocableWith<Operation &, const T &>
             and requires(Aleph::DynList<Aleph::dry_detail::map_result_t<R, Operation &, T>> & l,
                          Operation & op, const T & item)
               { l.append(std::invoke(op, item)); }
  [[nodiscard]] Aleph::DynList<Aleph::dry_detail::map_result_t<R, Operation &, T>>
  map_if(Prop prop, Operation & op) const
  {
    return maps_if<R, Prop, Operation>(prop, op);
  }

  /** @brief Overload of map_if() that accepts rvalues.
   *
   *  @tparam R item type of the result, or `void` (the default) to take
   *          the type that `op` returns.
   *  @tparam Prop Predicate callable.
   *  @tparam Operation Transformation callable.
   *  @param[in] prop predicate.
   *  @param[in] op transformation operation.
   *  @return DynList with conditionally mapped elements.
   *  @throw anything that `prop` or `op` throws, or `bad_alloc`.
   *  @see maps_if()
   */
  template <typename R = void, class Prop, class Operation>
    requires Aleph::InvocablePredicateWith<Prop &, const T &>
             and Aleph::InvocableWith<Operation &, const T &>
             and requires(Aleph::DynList<Aleph::dry_detail::map_result_t<R, Operation &, T>> & l,
                          Operation & op, const T & item)
               { l.append(std::invoke(op, item)); }
  [[nodiscard]] Aleph::DynList<Aleph::dry_detail::map_result_t<R, Operation &, T>>
  map_if(Prop prop, Operation && op) const
  {
    return maps_if<R, Prop, Operation>(prop, op);
  }

  /** @brief Map each element to a container and concatenate the results.
   *
   *  `flat_map(op)` appends, in traversal order, all the items of each
   *  container `op(item)`. The result holds the item type of that
   *  container: its `Item_Type` for an Aleph container, or else its
   *  `value_type`.
   *
   *  @tparam Operation Callable with signature `C(const T &)`, where `C`
   *          is an Aleph container with for_each() or a range.
   *  @param[in] op operation that maps an item to a container.
   *  @return DynList with the concatenation of the containers.
   *  @throws anything that `op` throws, or `bad_alloc`.
   */
  template <class Operation>
    requires Aleph::InvocableWith<Operation &, const T &>
  [[nodiscard]] auto flat_map(Operation & op) const
  {
    using Inner = std::decay_t<std::invoke_result_t<Operation &, const T &>>;
    using R = typename Aleph::dry_detail::inner_item<Inner>::type;
    Aleph::DynList<R> ret_val;
    const_traverse([&ret_val, &op] (const T & item)
      {
        auto && sub = std::invoke(op, item);
        if constexpr (requires { sub.for_each(std::declval<void (*)(const R &)>()); })
          sub.for_each([&ret_val] (const R & x)
            {
              ret_val.append(x);
            });
        else
          for (auto && x : sub)
            ret_val.append(x);
        return true;
      });
    return ret_val;
  }

  /** @brief Overload of flat_map() that accepts rvalues.
   *  @param[in] op operation that maps an item to a container.
   *  @return DynList with the concatenation of the containers.
   *  @throws anything that `op` throws, or `bad_alloc`.
   */
  template <class Operation>
    requires Aleph::InvocableWith<Operation &, const T &>
  [[nodiscard]] auto flat_map(Operation && op) const
  {
    return flat_map<Operation>(op);
  }

  /** Convert container to DynList.
   *
   *  @return DynList<T> containing all elements in traversal order.
   *  @throw bad_alloc if there is no enough memory
   */
  [[nodiscard]] Aleph::DynList<T> to_dynlist() const
  {
    Aleph::DynList<T> ret;
    const_traverse([&ret] (const T & item)
      {
        ret.append(item);
        return true;
      });
    return ret;
  }

  /** Convert container to std::vector.
   *
   *  @return std::vector<T> containing all elements in traversal order.
   *  @throw bad_alloc if there is no enough memory
   */
  [[nodiscard]] std::vector<T> to_vector() const
  {
    std::vector<T> ret;
    if constexpr (requires { const_me()->size(); })
      ret.reserve(const_me()->size());
    const_traverse([&ret] (const T & item)
      {
        ret.push_back(item);
        return true;
      });
    return ret;
  }

  /** Fold the elements of the container to a specific result.

      `foldl(init, op)` set an internal variable `acc` of type `Acc` to
      `init` value. Then it traverses the container, and on each `item`
      it performs:

      acc = op(acc, item);

      So `acc` serves as a sort of accumulator. If `op` accepts it as an
      rvalue, `acc` is moved into `op` instead of copied, so folding into
      a container costs O(1) per item, as `std::accumulate` does.

      `op` should have the following signature:

      Acc op(Acc acc, const T & item);

      Since `foldl` is overloaded with several operation structures,
      there is a certain flexibility with the parameter qualifiers. You
      could, for example, declare `acc` and/or `item` by value.

      The first template parameter `Acc` is the folded type; it is
      deduced from `init`. For example, to determine the maximum of all
      elements:

      c.foldl(std::numeric_limits<int>::min(), [] (int acc, int item)
            {
              return std::max(acc, item);
            });

      and to sum the inverted elements as `double`:

      c.template foldl<double>(0, [] (double acc, int item)
            {
              return acc + 1.0/item;
            });

      \param[in] init initial value of folded value (or accumulator).
      \param[in] op operation to be performed on each item and used for
            folding.
      \return the final folded computation.
      \throw anything that could throw `op`
  */
  template <typename Acc = T, class Op>
    requires Aleph::dry_detail::FoldOperation<Op, Acc, T>
  [[nodiscard]] Acc foldl(const Acc & init, Op & op) const
  {
    Acc acc = init;
    const_traverse([&acc, &op] (const T & item)
      {
        Aleph::dry_detail::fold_left_step(acc, op, item);
        return true;
      });
    return acc;
  }

  /** @brief Overload of foldl() that accepts rvalues.
   *
   *  The operation is passed (invoked) as a named lvalue to the
   *  lvalue-reference overload rather than being perfectly forwarded.
   *
   *  @tparam Acc accumulator type, deduced from `init`.
   *  @tparam Op Folding callable.
   *  @param[in] init initial accumulator value.
   *  @param[in] op folding operation.
   *  @return folded result.
   *  @throws May throw the same exceptions as the corresponding lvalue
   *          overload; no additional exception-safety guarantees are provided.
   */
  template <typename Acc = T, class Op>
    requires Aleph::dry_detail::FoldOperation<Op, Acc, T>
  [[nodiscard]] Acc foldl(const Acc & init, Op && op) const
  {
    return foldl<Acc, Op>(init, op);
  }

  /** Alias for foldl().

      @tparam Acc accumulator type, deduced from `init`.
      @param[in] init initial accumulator value
      @param[in] op folding operation
      @return folded result
      @throw anything that `op` throws.
  */
  template <typename Acc = T, class Op>
    requires Aleph::dry_detail::FoldOperation<Op, Acc, T>
  [[nodiscard]] Acc fold_left(const Acc & init, Op & op) const
  {
    return foldl<Acc, Op>(init, op);
  }

  /** @brief Overload of fold_left() that accepts rvalues.
   *
   *  The operation is passed (invoked) as a named lvalue to the
   *  lvalue-reference overload rather than being perfectly forwarded.
   *
   *  @tparam Acc accumulator type, deduced from `init`.
   *  @tparam Op Folding callable.
   *  @param[in] init initial accumulator value.
   *  @param[in] op folding operation.
   *  @return folded result.
   *  @throws May throw the same exceptions as the corresponding lvalue
   *          overload; no additional exception-safety guarantees are provided.
   */
  template <typename Acc = T, class Op>
    requires Aleph::dry_detail::FoldOperation<Op, Acc, T>
  [[nodiscard]] Acc fold_left(const Acc & init, Op && op) const
  {
    return foldl<Acc, Op>(init, op);
  }

  /** Simplified version of foldl() where the folded type is the same
      type of elements stored in the container.

      @param[in] init initial accumulator value.
      @param[in] operation folding operation.
      @return folded result.
      @throw anything that `operation` throws.
      @see foldl(const Acc & init, Op & op)
  */
  template <class Operation>
    requires Aleph::dry_detail::FoldOperation<Operation, T, T>
  [[nodiscard]] T fold(const T & init, Operation & operation) const
  {
    return foldl<T, Operation>(init, operation);
  }

  /** @brief Overload of fold() that accepts rvalues.
   *
   *  The operation is passed (invoked) as a named lvalue to the
   *  lvalue-reference overload rather than being perfectly forwarded.
   *
   *  @tparam Operation Folding callable.
   *  @param[in] init initial accumulator value.
   *  @param[in] operation folding operation.
   *  @return folded result.
   *  @throws May throw the same exceptions as the corresponding lvalue
   *          overload; no additional exception-safety guarantees are provided.
   */
  template <class Operation>
    requires Aleph::dry_detail::FoldOperation<Operation, T, T>
  [[nodiscard]] T fold(const T & init, Operation && operation) const
  {
    return foldl<T, Operation>(init, operation);
  }

  /** @brief Fold the elements from the last to the first.
   *
   *  `foldr(init, op)` computes `op(x0, op(x1, ... op(xn-1, init)))` for
   *  the items `x0, ..., xn-1` in traversal order: the operation receives
   *  the item first and the accumulator second, as Haskell's `foldr`.
   *  The accumulator is moved into `op` when `op` accepts an rvalue.
   *
   *  @tparam Acc accumulator type, deduced from `init`.
   *  @tparam Op Folding callable, with signature `Acc(const T &, Acc)`.
   *  @param[in] init initial accumulator value.
   *  @param[in] op folding operation.
   *  @return folded result.
   *  @throws anything that `op` throws, or `bad_alloc`.
   *  @note It keeps a list of pointers to the n items: O(n) extra memory.
   */
  template <typename Acc = T, class Op>
    requires Aleph::dry_detail::FoldRightOperation<Op, Acc, T>
  [[nodiscard]] Acc foldr(const Acc & init, Op & op) const
  {
    Aleph::DynList<const T *> reversed;  // the last item first
    const_traverse([&reversed] (const T & item)
      {
        reversed.insert(&item);
        return true;
      });
    Acc acc = init;
    reversed.for_each([&acc, &op] (const T * item)
      {
        Aleph::dry_detail::fold_right_step(acc, op, *item);
      });
    return acc;
  }

  /** @brief Overload of foldr() that accepts rvalues.
   *  @param[in] init initial accumulator value.
   *  @param[in] op folding operation.
   *  @return folded result.
   *  @throws anything that `op` throws, or `bad_alloc`.
   */
  template <typename Acc = T, class Op>
    requires Aleph::dry_detail::FoldRightOperation<Op, Acc, T>
  [[nodiscard]] Acc foldr(const Acc & init, Op && op) const
  {
    return foldr<Acc, Op>(init, op);
  }

  /** @brief Fold the elements with no initial value.
   *
   *  `reduce(op)` folds from the left, as foldl(), taking the first item
   *  as the initial accumulator: for the items `x0, x1, ..., xn-1` it
   *  computes `op(... op(op(x0, x1), x2) ..., xn-1)`.
   *
   *  @tparam Op Folding callable, with signature `T(T, const T &)`.
   *  @param[in] op folding operation.
   *  @return the folded value, or `std::nullopt` if the container is empty.
   *  @throws anything that `op` or the copy of `T` throws.
   */
  template <class Op, class U = T>  // U defers the check on T to calls (Clang 14)
    requires Aleph::dry_detail::FoldOperation<Op, T, T> and std::copy_constructible<U>
  [[nodiscard]] std::optional<T> reduce(Op & op) const
  {
    std::optional<T> acc;
    const_traverse([&acc, &op] (const T & item)
      {
        if (not acc.has_value())
          acc.emplace(item);
        else
          Aleph::dry_detail::fold_left_step(*acc, op, item);
        return true;
      });
    return acc;
  }

  /** @brief Overload of reduce() that accepts rvalues.
   *  @param[in] op folding operation.
   *  @return the folded value, or `std::nullopt` if the container is empty.
   *  @throws anything that `op` or the copy of `T` throws.
   */
  template <class Op, class U = T>
    requires Aleph::dry_detail::FoldOperation<Op, T, T> and std::copy_constructible<U>
  [[nodiscard]] std::optional<T> reduce(Op && op) const
  {
    return reduce<Op>(op);
  }

  /** Filter the elements of a container according to a matching
      criterion.

      This method builds a dynamic list with copies of items of
      container matching a criterion defined by `operation`, which
      should have the following signature:

      bool operation(const T & item)

      If `operation` return `true` then `item` matches the criterion;
      otherwise, `operation` must return `false`.

      For example, if the container has integer, then the following
      code snippet would return a list containing the items greater
      than 100:

      c.filter([] (auto item) { return item > 100; });

      \param[in] operation defining the filter criterion
      \return a `DynList<T>` with the matched elements.
      \throw anything that could throw `operation` or `bad_alloc` if
      there is no enough memory
  */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] Aleph::DynList<T> filter(Operation & operation) const
  {
    Aleph::DynList<T> ret_val;
    const_traverse([&ret_val, &operation] (const T & item)
      {
        if (std::invoke(operation, item))
          ret_val.append(item);
        return true;
      });
    return ret_val;
  }

  /** @brief Overload of filter() that accepts rvalues.
   *
   *  The operation is passed (invoked) as a named lvalue to the
   *  lvalue-reference overload rather than being perfectly forwarded.
   *
   *  @tparam Operation Filter callable.
   *  @param[in] operation filter predicate.
   *  @return DynList<T> with matching elements.
   *  @throws May throw the same exceptions as the corresponding lvalue
   *          overload; no additional exception-safety guarantees are provided.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] Aleph::DynList<T> filter(Operation && operation) const
  {
    return filter<Operation>(operation);
  }

  /** Filter the elements of a container according to a matching
      criterion and return a pointer to the matched items in the container.

      This method builds a dynamic list with stores pointers to the
      items of matching a criterion defined by `operation`, which
      should have the following signature:

      bool operation(const T & item)

      If `operation` return `true` then `item` matches the criterion;
      otherwise, `operation` must return `false`.

      For example, if the container has integer, then the following
      code snippet would return a list containing the items greater
      than 100:

      c.ptr_filter([] (auto item) { return item > 100; });

      \param[in] operation defining the filter criterion
      \return a `DynList<const T*>` with the pointers to the matched elements.
      \throw anything that could throw `operation` or `bad_alloc` if
      there is no enough memory
  */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] Aleph::DynList<const T *> ptr_filter(Operation & operation) const
  {
    Aleph::DynList<const T *> ret_val;
    const_traverse([&ret_val, &operation] (const T & item)
      {
        if (std::invoke(operation, item))
          ret_val.append(&item);
        return true;
      });
    return ret_val;
  }

  /** @brief Overload of ptr_filter() that accepts rvalues.
   *
   *  The operation is passed (invoked) as a named lvalue to the
   *  lvalue-reference overload rather than being perfectly forwarded.
   *
   *  @tparam Operation Filter callable.
   *  @param[in] operation filter predicate.
   *  @return DynList<const T*> with pointers to matching elements.
   *  @throws May throw the same exceptions as the corresponding lvalue
   *          overload; no additional exception-safety guarantees are provided.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] Aleph::DynList<const T *> ptr_filter(Operation && operation) const
  {
    return ptr_filter<Operation>(operation);
  }

  /**  Filter the elements of a container according to a matching
       criterion and determine its positions respect to the traversal
       of container.

       `pfilter(operation)` is very similar to `filter()`, but instead
       of building a list of filtered elements, it builds a list of
       pairs with form `(item, pos)`, where `item` is a copy of
       filtered element and `pos` is its position respect to the
       traversal order. The position is relative to the container type.

       The pair is defined with a tuple:

       std::tuple<T, size_t>

       \param[in] operation that defines the filter criterion
       \return a DynList
       \throw anything that `operation` throws, or bad_alloc if there is
       no enough memory
       \see filter(Operation & operation)
  */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] Aleph::DynList<std::tuple<T, size_t>> pfilter(Operation & operation) const
  {
    using TT = std::tuple<T, size_t>;
    Aleph::DynList<TT> ret_val;
    size_t i = 0;
    const_traverse([&ret_val, &operation, &i] (const T & item)
      {
        if (std::invoke(operation, item))
          ret_val.append(TT(item, i));
        ++i;
        return true;
      });
    return ret_val;
  }

  /** @brief Overload of pfilter() that accepts rvalues.
   *
   *  The operation is passed (invoked) as a named lvalue to the
   *  lvalue-reference overload rather than being perfectly forwarded.
   *
   *  @tparam Operation Filter callable.
   *  @param[in] operation filter predicate.
   *  @return DynList<std::tuple<T, size_t>> with matching elements and positions.
   *  @throws May throw the same exceptions as the corresponding lvalue
   *          overload; no additional exception-safety guarantees are provided.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] Aleph::DynList<std::tuple<T, size_t>> pfilter(Operation && operation) const
  {
    return pfilter<Operation>(operation);
  }

  /** Exclusive partition of container according to a filter criterion.

      `partition(op)` traverses the container and filters its elements
      according to the filter criterion defined by `op`. The filtered
      elements are copied to a first list and the not filtered ones to a
      second list. When all the container is traversed, a pair
      containing these lists is returned.

      The `op` requirements are the same as for `filter()`.

      \param[in] op operation instrumenting the filter criterion
      \return a `std::pair<DynList<T>, DynList<T>>. `first` contains the
      filtered elements and `second` the non-filtered ones.
      \throw anything that could throw op  or `bad_alloc` if there is
      no enough memory
      \see filter()
  */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] std::pair<Aleph::DynList<T>, Aleph::DynList<T>> partition(Operation & op) const
  {
    std::pair<Aleph::DynList<T>, Aleph::DynList<T>> ret_val;
    const_traverse([&ret_val, &op] (const T & item)
      {
        if (std::invoke(op, item))
          ret_val.first.append(item);
        else
          ret_val.second.append(item);
        return true;
      });
    return ret_val;
  }

  /** @brief Overload of partition() that accepts rvalues.
   *
   *  The operation is passed (invoked) as a named lvalue to the
   *  lvalue-reference overload rather than being perfectly forwarded.
   *
   *  @tparam Operation Filter callable.
   *  @param[in] op filter predicate.
   *  @return pair of lists (matched, not matched).
   *  @throws May throw the same exceptions as the corresponding lvalue
   *          overload; no additional exception-safety guarantees are provided.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] std::pair<Aleph::DynList<T>, Aleph::DynList<T>> partition(Operation && op) const
  {
    return partition<Operation>(op);
  }

  /** Exclusive partition of container in the nth item

      `partition(n)` traverses the container and produces a pair of
      lists. The first one contains the first `n` elements and the
      second one the `this->size() - n` remaining elements.

      \param[in] n the first `n` items of the first list
      \return pair of lists (first n items, the rest).
      \throw `bad_alloc` if there is no enough memory
  */
  [[nodiscard]] std::pair<Aleph::DynList<T>, Aleph::DynList<T>> partition(size_t n) const
  {
    size_t i = 0;
    std::pair<Aleph::DynList<T>, Aleph::DynList<T>> ret_val;
    const_traverse([&ret_val, &i, n] (const T & item)
      {
        if (i++ < n)
          ret_val.first.append(item);
        else
          ret_val.second.append(item);
        return true;
      });
    return ret_val;
  }

  /** Split the container into two halves by alternating elements.

      Elements at even positions go to the first list, odd positions to the second.

      @return pair of lists (even‑indexed, odd‑indexed)
      @throw `bad_alloc` if there is no enough memory
  */
  [[nodiscard]] std::pair<Aleph::DynList<T>, Aleph::DynList<T>> split_half() const
  {
    size_t i = 0;
    std::pair<Aleph::DynList<T>, Aleph::DynList<T>> ret_val;
    const_traverse([&ret_val, &i] (const T & item)
      {
        if (i % 2 == 0)
          ret_val.first.append(item);
        else
          ret_val.second.append(item);
        i++;
        return true;
      });
    return ret_val;
  }

  /** Exclusive partition of container according to a filter criterion.

      This method has exactly the same semantic as
      `partition(Operation & op)`, excepts than instead of returning a
      `std::pair` it returns a `std::tuple`.

      \param[in] op operation instrumenting the filter criterion
      \return a `std::tuple<DynList<T>, DynList<T>>. `first` contains the
      filtered elements and `second` the non-filtered ones.
      \throw anything that could throw op  or `bad_alloc` if there is
      no enough memory
      \see partition(Operation & op)
  */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] std::tuple<Aleph::DynList<T>, Aleph::DynList<T>> tpartition(Operation & op) const
  {
    Aleph::DynList<T> r1, r2;
    const_traverse([&r1, &r2, &op] (const T & item)
      {
        if (std::invoke(op, item))
          r1.append(item);
        else
          r2.append(item);
        return true;
      });
    return std::tuple<Aleph::DynList<T>, Aleph::DynList<T>>(std::move(r1), std::move(r2));
  }

  /** @brief Overload of tpartition() that accepts rvalues.
   *
   *  The operation is passed (invoked) as a named lvalue to the
   *  lvalue-reference overload rather than being perfectly forwarded.
   *
   *  @tparam Operation Filter callable.
   *  @param[in] op filter predicate.
   *  @return tuple of lists (matched, not matched).
   *  @throws May throw the same exceptions as the corresponding lvalue
   *          overload; no additional exception-safety guarantees are provided.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] std::tuple<Aleph::DynList<T>, Aleph::DynList<T>> tpartition(Operation && op) const
  {
    return tpartition<Operation>(op);
  }

  /** Count the number of elements of a container.

      This method counts the number of elements stored in the container.

      \note Take in account that this method computes; it does not
      retrieve. Consequently, it always takes \f$O(n)\f$. However, for
      many containers this number is already stored and retrievable in
      \f$O(1)\f$ through the method `size()`

      \return the number of elements stored in the container.
      \throw nothing, unless the traversal of the container throws.
  */
  [[nodiscard]] size_t length() const
    noexcept(Aleph::dry_detail::const_traversal_is_noexcept<Container, T>)
  {
    size_t count = 0;
    const_traverse([&count] (const T &) noexcept
      {
        ++count;
        return true;
      });
    return count;
  }

  /** Return a list with the elements of container in reverse order
      respect to its traversal order.

      \return a `DynList<T>` inversely ordered according to the
      traversal order.
      \throw bad_alloc if there is no enough memory
  */
  [[nodiscard]] Aleph::DynList<T> rev() const
  {
    Aleph::DynList<T> ret;
    const_traverse([&ret] (const T & item)
      {
        ret.insert(item);
        return true;
      });
    return ret;
  }

  /** Return a list with the first n elements seen in the container
      during its traversal.

      The complexity of this method is \f$O(n)\f$ where n can be less
      than the number of elements of container.

      \param[in] n number of elements to take
      \return A `DynList<T>` having the first `n` elements according to
      its traversal order, or all of them if `n` is not less than the
      number of elements.
      \throw bad_alloc if there is no enough memory
   */
  [[nodiscard]] Aleph::DynList<T> take(const size_t n) const
  {
    size_t i = 0;
    Aleph::DynList<T> ret;
    const_traverse([&i, &ret, n] (const T & item)
      {
        if (i++ >= n)
          return false;
        ret.append(item);
        return true;
      });
    return ret;
  }

  /** Return a list with the elements at positions `i`, `i + step`,
      `i + 2 step`, ... that are not greater than `j`, respect to the
      traversal order.

      The complexity of this method is \f$O(\min(j, N))\f$, where N is
      the number of elements of the container.

      \param[in] i position of the first element taken
      \param[in] j greatest position that may be taken
      \param[in] step distance between two consecutive taken positions
      \return A `DynList<T>` with the selected elements in traversal
      order. It is empty if `step` is zero, if `i` is greater than `j`, or
      if `i` is not smaller than the number of elements, as take(n) and
      drop(n) do with an `n` out of range.
      \throw bad_alloc if there is no enough memory
  */
  [[nodiscard]] Aleph::DynList<T> take(size_t i, const size_t j, const size_t step = 1) const
  {
    Aleph::DynList<T> ret;
    if (step == 0 or i > j)
      return ret;
    size_t pos = 0;
    const_traverse([&pos, &i, j, step, &ret] (const T & item)
      {
        if (pos++ != i)
          return true;
        ret.append(item);
        if (j - i < step)  // the next position would pass j, or overflow
          return false;
        i += step;
        return true;
      });
    return ret;
  }

  /** Drop the first n elements seen in the container during its
      traversal.

      The complexity of this method is \f$O(N)\f$ where N always is the
      number of elements of container.

      \param[in] n number of elements to drop
      \return A `DynList<T>` having the remainder \f$N - n\f$ elements
      according to traversal order; it is empty if `n` is not less than
      `N`.
      \throw bad_alloc if there is no enough memory
   */
  [[nodiscard]] Aleph::DynList<T> drop(const size_t n) const
  {
    size_t i = 0;
    Aleph::DynList<T> ret;
    const_traverse([&i, &ret, n] (const T & item)
      {
        if (i++ >= n)
          ret.append(item);
        return true;
      });
    return ret;
  }

  /** @brief The longest prefix of elements that satisfy a criterion.
   *
   *  It stops at the first element that does not satisfy `op`.
   *
   *  @tparam Operation Callable with signature bool(const T &).
   *  @param[in] op predicate.
   *  @return DynList with the leading elements that satisfy `op`, in
   *          traversal order.
   *  @throws anything that `op` throws, or `bad_alloc`.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] Aleph::DynList<T> take_while(Operation & op) const
  {
    Aleph::DynList<T> ret;
    const_traverse([&ret, &op] (const T & item)
      {
        if (not std::invoke(op, item))
          return false;
        ret.append(item);
        return true;
      });
    return ret;
  }

  /** @brief Overload of take_while() that accepts rvalues.
   *  @param[in] op predicate.
   *  @return DynList with the leading elements that satisfy `op`.
   *  @throws anything that `op` throws, or `bad_alloc`.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] Aleph::DynList<T> take_while(Operation && op) const
  {
    return take_while<Operation>(op);
  }

  /** @brief The elements after the longest prefix that satisfies a criterion.
   *
   *  @tparam Operation Callable with signature bool(const T &).
   *  @param[in] op predicate; it is not called again after it first
   *             returns `false`.
   *  @return DynList with the elements from the first one that does not
   *          satisfy `op` to the end, in traversal order.
   *  @throws anything that `op` throws, or `bad_alloc`.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] Aleph::DynList<T> drop_while(Operation & op) const
  {
    Aleph::DynList<T> ret;
    bool dropping = true;
    const_traverse([&ret, &op, &dropping] (const T & item)
      {
        if (dropping and std::invoke(op, item))
          return true;
        dropping = false;
        ret.append(item);
        return true;
      });
    return ret;
  }

  /** @brief Overload of drop_while() that accepts rvalues.
   *  @param[in] op predicate.
   *  @return DynList with the elements from the first one that does not
   *          satisfy `op` to the end.
   *  @throws anything that `op` throws, or `bad_alloc`.
   */
  template <class Operation>
    requires Aleph::InvocablePredicateWith<Operation &, const T &>
  [[nodiscard]] Aleph::DynList<T> drop_while(Operation && op) const
  {
    return drop_while<Operation>(op);
  }

  /** Drop the first n elements seen from container.

      It calls `remove()` `n` times, so it is available only for
      containers whose `remove()` takes no arguments and removes the
      first element, such as `DynList`. On the others its constraint is
      not satisfied: `requires { c.mutable_drop(n); }` is false, instead
      of the body failing to compile when used.

      The complexity of this method is `n` times that of `remove()`.

      \param[in] n number of elements to remove
      \throw what `remove()` throws when the container runs out of
      elements, for instance `underflow_error` on `DynList`. The elements
      already removed stay removed.
  */
  template <class C = Container>  // a template so that Clang 14 checks the constraint on use
    requires requires (C & c) { c.remove(); }
  void mutable_drop(const size_t n)
  {
    for (size_t i = 0; i < n; ++i)
      me()->remove();
  }
};

/** Generic list of items stored in a container.

    \ingroup Sequences
*/
template <class Container, typename T>
struct GenericItems
{
  /** Return a list of all the elements of a container sorted by
      traversal order.

      \return a `DynList<T>` containing all the elements of the
      container
      \throw bad_alloc if there is no enough memory
  */
  [[nodiscard]] Aleph::DynList<T> items() const
  {
    return static_cast<const Container *>(this)->Container::template maps<T>([] (const T & key)
      {
        return key;
      });
  }

  /** \overload items()
      @return a `DynList<T>` containing all the elements of the container
      @throw bad_alloc if there is no enough memory
  */
  [[nodiscard]] Aleph::DynList<T> keys() const
  {
    return items();
  }
};

/** Alias to GenricItems functor

    \ingroup Sequences
*/
template <class Container, typename T>
using GenericKeys = GenericItems<Container, T>;

/** @brief Mixin providing equality comparison for sequence containers.
 *
 *  This class implements the `equal_to()` method and `operator ==` for
 *  containers whose elements are ordered and do not necessarily
 *  export a fast `search()` method.
 *
 *  The test is performed by comparing elements one by one in traversal
 *  order, so the order and the repetitions count: it is the equality of
 *  lists, stacks and queues.
 *
 *  @tparam Container container type to which the equality test will
 *  be applied (must provide `size()` and an iterator).
 *  @ingroup Sequences
 */
template <class Container>
class EqualSequenceMethod
{
  const Container * const_me() const noexcept
  {
    return static_cast<const Container *>(this);
  }

public:
  /** @brief Equality test between `this` and `r`.
   *
   *  The test is performed by comparing elements one by one in
   *  traversal order.
   *
   *  @param[in] r Container on which the comparison will be performed.
   *  @return `true` if the containers have the same size and all the
   *          elements are equal in the same order.
   *  @throws Propagates any exception from item comparison or iterator
   *          operations.
   */
  [[nodiscard]] bool equal_to(const Container & r) const
  {
    if (const_me() == &r)
      return true;

    if (const_me()->size() != r.size())
      return false;

    typename Container::Iterator it1(*const_me());
    typename Container::Iterator it2(r);

    // Same size: it2 has a current item whenever it1 has one
    for (; it1.has_curr(); Aleph::dry_detail::advance(it1), Aleph::dry_detail::advance(it2))
      if (not (Aleph::dry_detail::current(it1) == Aleph::dry_detail::current(it2)))
        return false;

    return true;
  }

  /** @brief Equality operator. Forwards to equal_to(r).
   *
   *  A hidden friend with two parameters of the same type: the C++20
   *  reversed candidate converts its arguments in the same way, so it is
   *  never ambiguous. As a member of this base class it was, and Clang 14
   *  rejected `a == b` in constraints.
   *
   *  @param[in] l Container to compare.
   *  @param[in] r Container to compare with.
   *  @return Result of l.equal_to(r).
   *  @throws Propagates same exceptions as equal_to().
   */
  [[nodiscard]] friend bool operator == (const Container & l, const Container & r)
  {
    return l.equal_to(r);
  }

  /** @brief Inequality operator. Negation of equal_to(r).
   *  @param[in] l Container to compare.
   *  @param[in] r Container to compare with.
   *  @return Negation of l.equal_to(r).
   *  @throws Propagates same exceptions as equal_to().
   */
  [[nodiscard]] friend bool operator != (const Container & l, const Container & r)
  {
    return not l.equal_to(r);
  }
};

/** Equality test for sets.

    The test compares the sizes and then searches each element of `this`
    in the other container with `search()`. That is the equality of sets
    only: it ignores the order and the repetitions, so it is offered only
    to containers that have `Key_Type` and `search()`, and is meant for
    containers that keep no repeated keys, such as hash tables.

    Sequences use EqualSequenceMethod, and sorted containers that may
    keep repeated keys use EqualSortedMethod.

    \ingroup Sequences
*/
template <class Container>
class EqualToMethod
{
  const Container * const_me() const noexcept
  {
    return static_cast<const Container *>(this);
  }

public:
  /** Test if elements of `this` are exactly contained in another
      container.

      This method serves for testing if two containers contain the same
      elements. First, the container sizes are tested for equality. If
      they have the same size, then the testing is done by traversing
      `this`. Each seen element is searched in the another container
      with the method `search()`.

      @warning On some container, the size is computed, not retrieved.
      So take in account this fact.

      \param[in] r container on which the searches will be performed.
      \return `true` if the container have the same size and all the
      elements of `this` are present in `r`
      \throw anything that `size()`, `search()` or the comparator or hash
      function of the container throws.
  */
  template <class C = Container>  // checked on use: Clang 14 checks a
                                  // non-template member's constraint early
  [[nodiscard]] bool equal_to(const Container & r) const
    requires Aleph::dry_detail::SetSearchable<C>
  {
    if (const_me() == &r)  // Use const_me() to get correct pointer in case of multiple inheritance
      return true;

    if (const_me()->size() != r.size())
      return false;

    return const_me()->all([&r] (const typename C::Key_Type & k)
      {
        return r.search(k) != nullptr;
      });
  }

  /** @brief Equality operator: `l.equal_to(r)`.

      A hidden friend with two parameters of the same type, so that the
      C++20 reversed candidate is never ambiguous; a template so that its
      constraint is checked on use (Clang 14 checks the constraint of a
      non-template member when the class is instantiated).

      @param[in] l container to compare.
      @param[in] r container to compare with.
      @return result of l.equal_to(r).
      @throw what equal_to() throws.
  */
  template <class C = Container>
    requires Aleph::dry_detail::SetSearchable<C>
  [[nodiscard]] friend bool operator == (const Container & l, const Container & r)
  {
    return l.equal_to(r);
  }

  /** Negation of equal_to()
      @param[in] l container to compare.
      @param[in] r container to compare with.
      @return negation of l.equal_to(r).
      @throw what equal_to() throws.
  */
  template <class C = Container>
    requires Aleph::dry_detail::SetSearchable<C>
  [[nodiscard]] friend bool operator != (const Container & l, const Container & r)
  {
    return not l.equal_to(r);
  }
};

/** @brief Equality test for containers sorted by a comparator.

    The two containers are traversed in order at the same time, and two
    elements are equal if neither precedes the other according to the
    comparator returned by `key_comp()`. It is O(n), and it counts the
    repetitions, so it is right for sorted containers that may keep
    repeated keys, as `DynSetTree` with `insert_dup()`.

    @tparam Container container type; it must provide `size()`,
    `key_comp()` and an iterator that visits the elements in order.
    \ingroup Sequences
*/
template <class Container>
class EqualSortedMethod
{
  const Container * const_me() const noexcept
  {
    return static_cast<const Container *>(this);
  }

public:
  /** @brief Equality test between `this` and `r`.
   *
   *  @param[in] r container to compare with.
   *  @return `true` if both containers have the same size and their
   *          elements are equivalent one by one in sorted order.
   *  @throws anything that the comparator or the iterators throw.
   */
  [[nodiscard]] bool equal_to(const Container & r) const
  {
    if (const_me() == &r)
      return true;

    if (const_me()->size() != r.size())
      return false;

    const auto cmp = const_me()->key_comp();
    typename Container::Iterator it1(*const_me());
    typename Container::Iterator it2(r);

    // Same size: it2 has a current item whenever it1 has one
    for (; it1.has_curr(); Aleph::dry_detail::advance(it1), Aleph::dry_detail::advance(it2))
      {
        const auto & a = Aleph::dry_detail::current(it1);
        const auto & b = Aleph::dry_detail::current(it2);
        if (cmp(a, b) or cmp(b, a))
          return false;
      }

    return true;
  }

  /** @brief Equality operator. Forwards to equal_to(r).
   *
   *  A hidden friend with two parameters of the same type: the C++20
   *  reversed candidate converts its arguments in the same way, so it is
   *  never ambiguous. As a member of this base class it was, and Clang 14
   *  rejected `a == b` in constraints.
   *
   *  @param[in] l Container to compare.
   *  @param[in] r Container to compare with.
   *  @return Result of l.equal_to(r).
   *  @throws Propagates same exceptions as equal_to().
   */
  [[nodiscard]] friend bool operator == (const Container & l, const Container & r)
  {
    return l.equal_to(r);
  }

  /** @brief Inequality operator. Negation of equal_to(r).
   *  @param[in] l Container to compare.
   *  @param[in] r Container to compare with.
   *  @return Negation of l.equal_to(r).
   *  @throws Propagates same exceptions as equal_to().
   */
  [[nodiscard]] friend bool operator != (const Container & l, const Container & r)
  {
    return not l.equal_to(r);
  }
};

/** Common methods to mapping containers.

    For containers whose items are `std::pair<Key, Data>`.

    \ingroup Sequences
*/
template <class Container, typename Key, typename Data>
class MapSequencesMethods
{
  const Container * const_me() const noexcept
  {
    return static_cast<const Container *>(this);
  }

  Container * me() noexcept
  {
    return static_cast<Container *>(this);
  }

public:
  /** Return the domain set of container.

      This is a template method whose unique parameter is the type of
      container where the domain set will be stored. By default, this
      type is `DynList`.

      Often the elements of domain are called "keys" and those of codomain
      or range "values".

      \return A new container, according the passed template parameter,
      containing all the elements of domain of mapping.
      \throw bad_alloc if there is no enough memory
  */
  template <template <typename> class C = Aleph::DynList>
  [[nodiscard]] C<Key> keys() const
  {
    C<Key> ret_val;
    const_me()->for_each([&ret_val] (const std::pair<Key, Data> & p)
      {
        ret_val.append(p.first);
      });
    return ret_val;
  }

  /** Return the range set of container.

      This is a template method whose unique parameter is the type of
      container where the range set will be stored. By default this
      type is `DynList`.

      Often the elements of range are called "values" and those of domain
      "keys".

      \return A new container, according the passed template parameter,
      containing all the elements of range of mapping.
      \throw bad_alloc if there is no enough memory
  */
  template <template <typename> class C = Aleph::DynList>
  [[nodiscard]] C<Data> values() const
  {
    C<Data> ret_val;
    const_me()->for_each([&ret_val] (const std::pair<Key, Data> & p)
      {
        ret_val.append(p.second);
      });
    return ret_val;
  }

  /** Return a container of pointers to the elements of range set of
      container.

      This is a template method whose unique parameter is the type of
      container where the pointers set will be stored. By default this
      type is `DynList`.

      This method returns a container with pointers to the values, so
      that they can be modified; it needs a container whose modifiable
      traversal hands out modifiable references.

      \return A new container, according the passed template parameter,
      containing all the pointers to elements of range of mapping.
      \throw bad_alloc if there is no enough memory
  */
  template <template <typename> class C = Aleph::DynList>
    requires Aleph::dry_detail::MutablyTraversable<
                 typename Aleph::concepts_detail::defer<Container, C<Data>>::type, std::pair<Key, Data>>
  [[nodiscard]] C<Data *> values_ptr()
  {
    C<Data *> ret_val;
    me()->traverse([&ret_val] (std::pair<Key, Data> & p)
      {
        ret_val.append(&p.second);
        return true;
      });
    return ret_val;
  }

  /** Constant overload of values_ptr().

      \return A new container with constant pointers to the values.
      \throw bad_alloc if there is no enough memory
  */
  template <template <typename> class C = Aleph::DynList>
  [[nodiscard]] C<const Data *> values_ptr() const
  {
    C<const Data *> ret_val;
    const_me()->for_each([&ret_val] (const std::pair<Key, Data> & p)
      {
        ret_val.append(&p.second);
      });
    return ret_val;
  }

  /** Return a container with the entire mapping.

      `items()` returns a container copy of an entire mapping. Each
      element of returned mapping is a pair <key, value>.

      \return A new container, according the passed template parameter,
      containing all the pairs key,value of mapping.
      \throw bad_alloc if there is no enough memory
  */
  template <template <typename> class C = Aleph::DynList>
  [[nodiscard]] C<std::pair<Key, Data>> items() const
  {
    C<std::pair<Key, Data>> ret;
    const_me()->for_each([&ret] (const std::pair<Key, Data> & p)
      {
        ret.append(p);
      });
    return ret;
  }

  /** Return a container with the entire mapping where the range could
      be modified.

      `items_ptr()` returns a container copy of an entire
      mapping. Each element of returned mapping is a pair <key,
      pointer to value>. It needs a container whose modifiable traversal
      hands out modifiable references.

      \return A new container, according the passed template parameter,
      containing all the pairs key,pointer-value of mapping.
      \throw bad_alloc if there is no enough memory
  */
  template <template <typename> class C = Aleph::DynList>
    requires Aleph::dry_detail::MutablyTraversable<
                 typename Aleph::concepts_detail::defer<Container, C<Data>>::type, std::pair<Key, Data>>
  [[nodiscard]] C<std::pair<Key, Data *>> items_ptr()
  {
    C<std::pair<Key, Data *>> ret_val;
    me()->traverse([&ret_val] (std::pair<Key, Data> & p)
      {
        ret_val.append(std::pair<Key, Data *>(p.first, &p.second));
        return true;
      });
    return ret_val;
  }

  /** Constant overload of items_ptr().

      \return A new container with pairs <key, constant pointer to value>.
      \throw bad_alloc if there is no enough memory
  */
  template <template <typename> class C = Aleph::DynList>
  [[nodiscard]] C<std::pair<Key, const Data *>> items_ptr() const
  {
    C<std::pair<Key, const Data *>> ret_val;
    const_me()->for_each([&ret_val] (const std::pair<Key, Data> & p)
      {
        ret_val.append(std::pair<Key, const Data *>(p.first, &p.second));
      });
    return ret_val;
  }

  /** Return the image or element mapped to `key`.

       @param[in] key key to look up.
       \return a modifiable reference to value associated to the key.
       \throw domain_error if key is not present in the mapping
  */
  Data & operator () (const Key & key)
  {
    // CRTP: `find` lives on the derived `Container`, not on this base.
    // Casting via `me()` is required for AppleClang (which performs the
    // member lookup eagerly at template definition time) and is the
    // correct pattern regardless of compiler leniency.
    return me()->find(key);
  }

  /** Return the image or element mapped to `key`.

       @param[in] key key to look up.
       \return a constant reference to value associated to the key.
       \throw domain_error if key is not present in the mapping.
  */
  const Data & operator () (const Key & key) const
  {
    return const_me()->find(key);
  }
};
#endif  // AH_DRY_H
