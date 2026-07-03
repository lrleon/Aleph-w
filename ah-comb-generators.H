
/*
                          Aleph_w

  Data structures & Algorithms
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

/** @file ah-comb-generators.H
 *  @brief Lazy (coroutine-based) permutation and combination enumeration.
 *
 *  `ah-comb.H` already exposes the *step* primitives that make lazy
 *  enumeration possible — `next_permutation` (lexicographic, STL-style) and
 *  `next_combination_indices` (k-subsets of `[0, n)`) both advance one step
 *  in place. This header wraps those steps into `Aleph::Generator`-based
 *  sequences so callers don't have to hand-roll the `do { ... } while
 *  (next_*(...))` loop, and can `break` out early without having enumerated
 *  the rest.
 *
 *  The eager whole-sequence builders in `ah-comb.H` (`build_perms`,
 *  `build_combs`, and friends — Cartesian-product combinatorics over a
 *  `DynList<DynList<T>>`) are a **different, unrelated** feature (see the
 *  terminology note in `ah-comb.H`) and are untouched by this header.
 *
 *  @see ah-comb.H `next_permutation`, `next_combination_indices`,
 *       `combination_count`, and the eager Cartesian-product builders.
 *  @see ah-generator.H `Aleph::Generator<T>`, the underlying lazy sequence
 *       type.
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath Leon
 */

#ifndef AH_COMB_GENERATORS_H
#define AH_COMB_GENERATORS_H

#include <algorithm>

#include <ah-comb.H>
#include <ah-generator.H>
#include <tpl_array.H>

namespace Aleph
{

/** @brief Lazily enumerate all permutations of `a`, in lexicographic order.
 *
 *  Sorts a working copy of `a` ascending (per `cmp`) — the lexicographic
 *  starting point — then repeatedly applies `next_permutation` to advance
 *  it, `co_yield`ing after each step. For `n` elements with no ties, this
 *  yields exactly `n!` permutations.
 *
 *  @tparam T Element type. Must be movable and comparable via `Compare`.
 *  @tparam Compare Strict weak ordering over `T`. Defaults to
 *          `Aleph::less<T>`. Duplicate-valued elements (ties under
 *          `Compare`) collapse permutations exactly as `next_permutation`
 *          does — the yielded count is `n!` divided by the multinomial of
 *          tie-group sizes, matching `std::next_permutation`.
 *  @param a Sequence to permute (taken by value: the generator owns and
 *         mutates its own copy).
 *  @param cmp Comparator instance (defaulted).
 *  @return A lazy sequence of `n!` permutations (or fewer, with ties).
 *
 *  @warning **The yielded array is reused in place**: every `co_yield`
 *           exposes the *same* backing `Array<T>`, mutated one step
 *           further on each resume. This avoids an O(n) copy per
 *           permutation, but means the reference is only valid *between*
 *           resumes — copy it (`Array<T> snapshot = *it;`) if you need to
 *           keep more than one permutation alive at a time.
 *  @note Complexity: O(1) amortized element swaps per yielded permutation
 *        (the classic `next_permutation` bound); O(n log n) once up front
 *        to sort the starting point.
 *
 *  @par Example
 *  @code
 *  for (const Array<int> &p : lazy_permutations(Array<int>{1, 2, 3}))
 *    process(p);   // sees {1,2,3}, {1,3,2}, {2,1,3}, {2,3,1}, {3,1,2}, {3,2,1}
 *  @endcode
 *
 *  @ingroup Algorithms
 */
template <typename T, class Compare = Aleph::less<T>>
Aleph::Generator<Array<T>> lazy_permutations(Array<T> a, Compare cmp = Compare())
{
  /// Start from the lexicographically smallest permutation so the loop
  /// below enumerates the complete sequence.
  std::sort(a.begin(), a.end(), cmp);
  co_yield a;
  while (next_permutation(a, cmp, false))
    co_yield a;
}

/** @brief Lazily enumerate all `k`-element combinations of `a`'s elements.
 *
 *  Combinations are drawn positionally (index-based, via
 *  `next_combination_indices`) and yielded in the same relative order as
 *  `a`: for input `[x0, x1, ..., x(n-1)]`, the first combination is
 *  `[x0, ..., x(k-1)]` and the last is `[x(n-k), ..., x(n-1)]`. Yields
 *  exactly `combination_count(n, k)` combinations for `k <= n` (`1` when
 *  `k == 0`).
 *
 *  @tparam T Element type. Must be copyable.
 *  @param a Source sequence (taken by value: the generator owns and reads
 *         from its own copy — see the note on coroutine reference
 *         parameters below).
 *  @param k Size of each combination.
 *  @return A lazy sequence of `C(a.size(), k)` combinations, each a fresh
 *          `Array<T>` of size `k`.
 *  @throws std::domain_error if `k > a.size()`.
 *  @note Complexity: O(k) amortized per yielded combination (the classic
 *        `next_combination_indices` bound) plus O(k) to materialize each
 *        combination's values.
 *  @note Each combination is materialized as its own `Array<T>` before it is
 *        yielded (materializing values from indices requires a copy anyway).
 *        The iterator still returns a reference into the suspended coroutine
 *        frame, so that reference is valid only until the generator advances.
 *        Copy it (`Array<T> snapshot = c;`) if you need to keep it.
 *  @note `a` is taken **by value**, not by reference, on purpose: a
 *        coroutine's reference parameters bind to whatever the caller
 *        passed, but the coroutine body doesn't start running until the
 *        first resume (it starts suspended) — by which point a temporary
 *        argument (e.g. `lazy_combinations(Array<int>{1,2,3}, 2)`) has
 *        already been destroyed. Taking `a` by value sidesteps this
 *        well-known C++20 coroutine pitfall entirely, at the cost of one
 *        copy of the source sequence.
 *
 *  @par Example
 *  @code
 *  for (const Array<char> &c : lazy_combinations(Array<char>{'a','b','c'}, 2))
 *    process(c);   // sees {a,b}, {a,c}, {b,c}
 *  @endcode
 *
 *  @ingroup Algorithms
 */
template <typename T>
Aleph::Generator<Array<T>> lazy_combinations(Array<T> a, size_t k)
{
  const size_t n = a.size();
  ah_domain_error_if(k > n) << "lazy_combinations: k=" << k << " cannot exceed n=" << n;

  if (k == 0)
    {
      co_yield Array<T>();
      co_return;
    }

  /// Start from the first k-combination: indices [0, 1, ..., k-1].
  Array<size_t> idx(k);
  idx.putn(k);
  for (size_t i = 0; i < k; ++i)
    idx[i] = i;

  do
    {
      Array<T> combo(k);
      for (size_t i = 0; i < k; ++i)
        combo.append(a[idx[i]]);
      co_yield combo;
    }
  while (next_combination_indices(idx, n, false));
}

} // end namespace Aleph

#endif // AH_COMB_GENERATORS_H
