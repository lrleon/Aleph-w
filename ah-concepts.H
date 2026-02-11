
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
 *  @brief C++20 concepts for constraining comparison functors.
 *
 *  Defines three concepts used throughout Aleph-w to constrain
 *  template parameters that act as binary predicates:
 *
 *  - **BinaryPredicate<F, T>** — base concept: `f(a, b)` returns
 *    something convertible to `bool`.
 *  - **StrictWeakOrder<F, T>** — wraps `std::strict_weak_order<F, T, T>`
 *    for BST comparators.
 *  - **EqualityComparator<F, T>** — wraps `std::equivalence_relation<F, T, T>`
 *    for hash table equality comparators.
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
   *  constrain the `Compare` template parameter. Based on
   *  `std::strict_weak_order`, which requires the functor to be a
   *  regular invocable returning a boolean-testable result.
   *
   *  @tparam F functor type.
   *  @tparam T key type.
   */
  template <typename F, typename T>
  concept StrictWeakOrder = std::strict_weak_order<F, T, T>;

  /** @brief Equivalence relation constraint for equality comparators.
   *
   *  Used by hash table classes (`GenLhashTable`, `ODhashTable`, etc.)
   *  to constrain the `Cmp` template parameter. Based on
   *  `std::equivalence_relation`, which requires the functor to be a
   *  regular invocable returning a boolean-testable result.
   *
   *  @tparam F functor type.
   *  @tparam T key type.
   */
  template <typename F, typename T>
  concept EqualityComparator = std::equivalence_relation<F, T, T>;

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
      requires { typename T::Node; } &&
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
          requires (!requires { ct.getRoot(); } ||
                    requires
                    {
                      { ct.getRoot() } -> std::convertible_to<const typename T::Node *>;
                    });
          requires (!requires { ct.search(key); } ||
                    requires
                    {
                      { ct.search(key) } -> std::convertible_to<const typename T::Node *>;
                    });
        };

} // namespace Aleph

# endif // AH_CONCEPTS_H
