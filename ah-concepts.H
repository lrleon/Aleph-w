
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


/** @file ah-concepts.H
 *  @brief C++20 concepts hub: comparison, BST policy, and Aleph container
 *         concepts.
 *
 *  Defines twelve concepts used throughout Aleph-w to constrain template
 *  parameters:
 *
 *  - **BinaryPredicate<F, T>** — base concept: `f(a, b)` returns
 *    something convertible to `bool`.
 *  - **StrictWeakOrder<F, T>** — wraps `std::strict_weak_order` for BST
 *    comparators, invoked the way BST classes actually call them: through
 *    a `const F&`.
 *  - **EqualityComparator<F, T>** — wraps `std::equivalence_relation` for
 *    hash table equality comparators, likewise invoked through `const F&`.
 *  - **BSTPolicy<T, Key>** — minimal interface a `Tree<Key, Compare>`
 *    instantiation must expose to be usable by `DynSetTree`.
 *  - **AlephArrayConvertible<T>** — exposes `Item_Type` and `get_it()`.
 *  - **AlephSequentialContainer<C>** — Aleph mutable sequential container
 *    interface (`size`, `is_empty`, `append`, `mutable_for_each`).
 *  - **AlephSequentialIterableContainer<C>** — refines the above with a
 *    `const`-iterable `get_it()`.
 *  - **IntegerSortableValue<T>** — integral type accepted by counting/radix
 *    sort, excluding `bool`.
 *  - **ClosedBinaryOp<F, T>** — `f(a, b)` on two `const T&` returns
 *    something convertible to `T` (segment/Fenwick/sparse tables).
 *  - **StaticMonoid<M, T>** — `M::identity()` and `M::combine(a, b)`.
 *  - **CallableWith<F, Args...>** — `f(args...)` is valid exactly as written.
 *  - **PredicateWith<F, Args...>** — `f(args...)` is valid and usable as a
 *    condition (contextual conversion to `bool`).
 *
 *  Using distinct concept names produces clear error messages:
 *  a BST constraint failure mentions "StrictWeakOrder", while a
 *  hash table failure mentions "EqualityComparator".
 *
 *  @ingroup Functors
 *  @author Leandro Rabindranath Leon
 */

# ifndef AH_CONCEPTS_H
# define AH_CONCEPTS_H

# include <concepts>
# include <type_traits>
# include <cstddef>
# include <functional>
# include <utility>

namespace Aleph
{
  /** @brief A callable that takes two `const T&` and returns bool.
   *  @tparam F functor type.
   *  @tparam T value type.
   */
  template <typename F, typename T>
  concept BinaryPredicate =
      requires(const F & f, const T & a, const T & b)
        {
          { f(a, b) } -> std::convertible_to<bool>;
        };

  /** @brief Strict weak ordering constraint for BST comparators.
   *
   *  Used by BST classes (`Gen_Avl_Tree`, `Gen_Rb_Tree`, etc.) to
   *  constrain the `Compare` template parameter. The stored comparator is
   *  invoked through *both* shapes depending on the call site: read-only
   *  paths bind it through a `const` tree (`const F&`), while mutating
   *  paths (`insert()`, `search_and_stack_avl()`, ...) hold the tree by a
   *  non-`const` `this` and invoke the `cmp` member as a non-`const`
   *  lvalue. If `F` overloads `operator()` differently for its `const`
   *  and non-`const` forms, checking only one shape lets a functor pass
   *  the constraint here and still fail deep inside `tpl_avl.H` on
   *  whichever shape wasn't checked — the exact failure mode this
   *  concept exists to catch. Requiring both shapes closes that gap
   *  while still accepting the common case of a single `const
   *  operator()` (callable through either shape, since a `const` method
   *  can be invoked from a non-`const` lvalue too).
   *
   *  @tparam F functor type.
   *  @tparam T key type.
   */
  template <typename F, typename T>
  concept StrictWeakOrder =
      std::strict_weak_order<F, T, T> and
      std::strict_weak_order<const F &, const T &, const T &>;

  /** @brief Equivalence relation constraint for equality comparators.
   *
   *  Used by hash table classes (`GenLhashTable`, `ODhashTable`, etc.)
   *  to constrain the `Cmp` template parameter. Requires both the
   *  non-`const` and `const` invocation shapes for the same reason as
   *  `StrictWeakOrder`: mutating hash-table paths (e.g.
   *  `ODhashTable::allocate_bucket()`) invoke the stored `cmp` as a
   *  non-`const` lvalue, while other paths bind it through `const`.
   *
   *  @tparam F functor type.
   *  @tparam T key type.
   */
  template <typename F, typename T>
  concept EqualityComparator =
      std::equivalence_relation<F, T, T> and
      std::equivalence_relation<const F &, const T &, const T &>;

  /** @brief Concept for BST tree policies used by `DynSetTree`.
   *
   *  Checks that an instantiated tree type `T = Tree<Key, Compare>`
   *  exposes the minimal interface required by `DynSetTree`:
   *  a `Node` type alias and the core operations `getRoot`, `search`,
   *  `search_or_insert`, `insert_dup`, `remove`, `verify`, `swap`,
   *  and `get_compare`.
   *
   *  Optional operations (`join`, `select`, `position`, …) are NOT
   *  checked here because not every tree type supports them.
   *
   *  @tparam T  the fully instantiated tree type, e.g. `Avl_Tree<int>`.
   *  @tparam Key  the key type stored in the tree.
   *
   *  @ingroup Functors
   */
  template <typename T, typename Key>
  concept BSTPolicy =
      requires { typename T::Node; } and
      requires(T & t, T & t2, const T & ct,
               typename T::Node * p, const Key & key)
        {
          { t.getRoot() } -> std::convertible_to<typename T::Node *>;
          { t.search(key) } -> std::convertible_to<typename T::Node *>;
          { t.search_or_insert(p) } -> std::convertible_to<typename T::Node *>;
          { t.insert_dup(p) } -> std::convertible_to<typename T::Node *>;
          { t.remove(key) } -> std::convertible_to<typename T::Node *>;
          { ct.verify() } -> std::convertible_to<bool>;
          { t.swap(t2) };
          { t.get_compare() };
          requires not requires { ct.getRoot(); } or requires {
            { ct.getRoot() } -> std::convertible_to<const typename T::Node *>;
          };
          requires (not requires { ct.search(key); } or
                    requires
                    {
                      { ct.search(key) } -> std::convertible_to<const typename T::Node *>;
                    });
        };

  /**
   * @brief Concept that identifies types that can be converted or used as an Aleph Array.
   *
   * A type @p T satisfies AlephArrayConvertible if it defines a nested type @c Item_Type
   * and provides a @c get_it() method, which is the standard way in Aleph-w to obtain
   * an iterator for the container.
   *
   * @tparam T The type to check.
   */
  template <typename T>
  concept AlephArrayConvertible = requires(T c)
    {
      typename T::Item_Type;
      { c.get_it() };
    };

  /** @brief Concept for Aleph-w mutable sequential containers.
   *
   *  A type `C` satisfies `AlephSequentialContainer` if and only if it exposes
   *  all of the following:
   *
   *  | Expression | Requirement |
   *  |---|---|
   *  | `typename C::Item_Type` | element type is defined |
   *  | `cc.size()` | returns something convertible to `size_t` |
   *  | `cc.is_empty()` | returns something convertible to `bool` |
   *  | `c.append(val)` | accepts `const Item_Type &`, inserts at back |
   *  | `c.mutable_for_each(f)` | accepts `void(Item_Type &)` callable |
   *
   *  The four Aleph sequential containers all satisfy this concept:
   *  - `DynList<T>` (`htlist.H`)
   *  - `DynDlist<T>` (`tpl_dynDlist.H`)
   *  - `Array<T>` (`tpl_array.H`)
   *  - `DynArray<T>` (`tpl_dynArray.H`)
   *
   *  The concept is primarily used to constrain the `fill()` and `iota()`
   *  free functions in `ahFunctional.H`.
   *
   *  @tparam C  Container type to check.
   *
   *  @see fill()  In-place fill with a constant value.
   *  @see iota()  In-place fill with sequential values.
   *  @ingroup Functors
   */
  template <typename C>
  concept AlephSequentialContainer =
      requires { typename C::Item_Type; } and
      requires(C & c, const C & cc, const typename C::Item_Type & val)
        {
          { cc.size()     } -> std::convertible_to<std::size_t>;
          { cc.is_empty() } -> std::convertible_to<bool>;
          c.append(val);
        } and
      // mutable_for_each must accept a void(Item_Type&) callable
      requires(C & c)
        {
          c.mutable_for_each([](typename C::Item_Type &) {});
        };

  /** @brief Concept for Aleph-w sequential containers that can be iterated.
   *
   *  This refines `AlephSequentialContainer` by additionally requiring
   *  `get_it()` to be available in a `const` container, so that algorithms
   *  can traverse items in read-only mode.
   *
   *  It is intended for Aleph-w *sequential* containers such as:
   *  - `DynList<T>`
   *  - `DynDlist<T>`
   *  - `Array<T>`
   *  - `DynArray<T>`
   *
   *  Although some associative containers may define methods with names like
   *  `append()`, they typically do not satisfy the full sequential interface
   *  required here (notably `mutable_for_each()` with an `Item_Type &`), so
   *  they are excluded by design.
   *
   *  @tparam C  Container type to check.
   *
   *  @ingroup Functors
   */
  template <typename C>
  concept AlephSequentialIterableContainer =
      AlephSequentialContainer<C> and
      requires(const C & cc)
        {
          cc.get_it();
        };

  /** @brief Integral value accepted by linear-time integer sorting algorithms.
   *
   *  This concept accepts integral types except `bool`, which avoids accidental
   *  use of bit-like logical values in counting/radix sorting APIs.
   */
  template <typename T>
  concept IntegerSortableValue =
      std::is_integral_v<std::remove_cv_t<T>> and
      not std::is_same_v<std::remove_cv_t<T>, bool>;

  /** @brief A binary functor closed over `T`.
   *
   *  `f(a, b)` is well-formed for `const T& a, const T& b` and returns
   *  something convertible to `T`. This is the shared shape behind
   *  `SparseTableOp`, `DisjointSparseTableOp`, `SegmentTreeOp` and
   *  `FenwickBinaryOp` (each kept as an alias for source compatibility),
   *  which differ only in which *semantic* law the caller must also
   *  provide (associativity, idempotency, ...) — this concept checks
   *  only the call signature, never the semantics.
   *
   *  @tparam F functor type.
   *  @tparam T value type.
   */
  template <typename F, typename T>
  concept ClosedBinaryOp =
      requires(const F & f, const T & a, const T & b)
        {
          { f(a, b) } -> std::convertible_to<T>;
        };

  /** @brief A static monoid: `M::identity()` and `M::combine(a, b)`.
   *
   *  Unlike `ClosedBinaryOp`, the combining operation lives on the type
   *  `M` itself (static members), not on an instance passed by the
   *  caller. This is the shape `LCTMonoid` (kept as an alias) requires
   *  from a link-cut tree's path-aggregation policy.
   *
   *  @tparam M monoid policy type, exposing `identity()` and `combine`.
   *  @tparam T value type.
   */
  template <typename M, typename T>
  concept StaticMonoid =
      requires(const T & a, const T & b)
        {
          { M::identity() } -> std::convertible_to<T>;
          { M::combine(a, b) } -> std::convertible_to<T>;
        };

  /** @brief `f(args...)` is a valid call expression, exactly as written.
   *
   *  Mirrors a plain call `f(args...)` with `F` and `Args` carrying the
   *  value category and `const`-ness of the call site (e.g.
   *  `CallableWith<Op &, const T &>` for `op(item)` with `op` a non-`const`
   *  lvalue and `item` a `const T &`). Unlike `std::invocable`, it does
   *  not go through `std::invoke`, so it rejects member pointers, which a
   *  plain call cannot use either. The result type is unconstrained.
   *
   *  @tparam F callable type, including its reference/`const` category.
   *  @tparam Args argument types, including their reference/`const` category.
   */
  template <typename F, typename... Args>
  concept CallableWith =
      requires(F && f, Args &&... args)
        {
          std::forward<F>(f)(std::forward<Args>(args)...);
        };

  /** @brief `f(args...)` is a valid call whose result is usable as a condition.
   *
   *  Mirrors `if (f(args...))` or `not f(args...)`: the result only needs
   *  to be *contextually* convertible to `bool`. This is deliberately
   *  weaker than `std::predicate`, which demands an *implicit* conversion
   *  and would reject results with only an `explicit operator bool()`
   *  (such as `std::optional` or smart pointers) that those call sites
   *  accept.
   *
   *  @tparam F callable type, including its reference/`const` category.
   *  @tparam Args argument types, including their reference/`const` category.
   */
  template <typename F, typename... Args>
  concept PredicateWith =
      CallableWith<F, Args...> and
      requires(F && f, Args &&... args)
        {
          static_cast<bool>(std::forward<F>(f)(std::forward<Args>(args)...));
        };

  namespace concepts_detail
  {
    /** @brief `T` itself, made dependent on `Deferred`.
     *
     *  For requires-clauses on member templates of CRTP mixins that name
     *  the derived `Container`: routing it through a type that depends on
     *  the member template's own parameter defers its use until the
     *  constraint is checked. Named directly, Clang 14 binds it when the
     *  CRTP base is instantiated, while `Container` is still incomplete,
     *  and then either hard-errors or reports "incomplete type".
     *
     *  @tparam T the type to forward.
     *  @tparam Deferred any type dependent on the member template.
     */
    template <class T, class Deferred>
    struct defer
    {
      using type = T;
    };

    /** @brief What `Container::Iterator::get_curr()` yields, deferred.
     *
     *  The argument type a mixin's iterator-driven `traverse()` passes to
     *  its operation. Deferred on `Deferred` for the same reason as
     *  `defer`.
     *
     *  @tparam Container the CRTP-derived container.
     *  @tparam Deferred any type dependent on the member template.
     */
    template <class Container, class Deferred>
    struct traverse_arg
    {
      using type = decltype(std::declval<typename defer<Container, Deferred>::type::Iterator &>().get_curr());
    };

    template <class Sig>
    struct invoke_as;

    /** @brief Calls a callable exactly as `std::function<R(A...)>` would.
     *
     *  For template parameters that replaced a `std::function<R(A...)>`
     *  parameter, constrained with `std::is_invocable_r_v<R, Op &, A...>`
     *  (the requirement of `std::function`'s constructor): arguments are
     *  forwarded as `std::function::operator()` forwards them, the call goes
     *  through `std::invoke` so member pointers keep working, and the result
     *  is implicitly converted to `R`.
     *
     *  @tparam R result type.
     *  @tparam A argument types.
     */
    template <class R, class... A>
    struct invoke_as<R(A...)>
    {
      /** @brief Invoke `f(a...)` converting the result to `R`.
       *  @param f callable, invoked as a non-`const` lvalue.
       *  @param a arguments.
       *  @return the result of the call, converted to `R`.
       *  @throws anything `f` throws.
       */
      template <class F>
      static R call(F & f, A... a)
      {
        return std::invoke(f, std::forward<A>(a)...);
      }
    };
  } // namespace concepts_detail

} // namespace Aleph

# endif // AH_CONCEPTS_H
