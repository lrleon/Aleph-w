
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

/** @file tpl_flat_set.H
 *  @brief Sorted-array set (`Aleph::FlatSet`), a cache-friendly ordered set.
 *
 *  `FlatSet<Key, Compare>` keeps its keys sorted inside a single contiguous
 *  `MemArray<Key>`. Lookups are binary searches over contiguous memory
 *  (O(log n) with excellent cache behavior); insertions and removals shift
 *  elements (O(n)). It is the classic *flat container* trade-off, in the
 *  spirit of C++23 `std::flat_set` and `boost::container::flat_set`:
 *
 *  | Operation | FlatSet | DynSetTree (AVL/RB) |
 *  |-----------|----------|---------------------|
 *  | find / contains | O(log n), contiguous | O(log n), pointer chasing |
 *  | insert / erase | O(n) shift | O(log n) |
 *  | iteration | O(n), sequential memory | O(n), pointer chasing |
 *  | memory per element | sizeof(Key) | sizeof(Key) + node overhead |
 *
 *  Prefer `FlatSet` for lookup- and iteration-heavy workloads whose
 *  modification phase is bulk or infrequent. Prefer `DynSetTree` when
 *  insertions and removals are frequent and interleaved with lookups.
 *
 *  @note This is a native Aleph implementation and is used under every
 *        supported standard (C++17/20/23). It deliberately does **not**
 *        alias `std::flat_set` when available: the standard adaptor has
 *        different reference/iterator semantics, and aliasing would make the
 *        container's exact behavior depend on `-std`. The detection macro
 *        `ALEPH_HAS_STD_FLAT_MAP` (see `ah-cpp-compat.H`) remains available
 *        for consumers that want the standard adaptor itself.
 *
 *  @see tpl_flat_map.H Sorted-array map counterpart.
 *  @see tpl_dynSetTree.H Balanced-tree sets (fast mutation).
 *
 *  @ingroup Sets
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_FLAT_SET_H
#define TPL_FLAT_SET_H

#include <algorithm>
#include <initializer_list>
#include <utility>

#include <ah-errors.H>
#include <ahFunction.H>
#include <tpl_memArray.H>

namespace Aleph
{

/** @brief Ordered set stored as a sorted contiguous array.
 *
 *  Keys are kept sorted according to `Compare` inside a `MemArray<Key>`.
 *  All lookup operations are binary searches over contiguous memory; all
 *  mutating operations preserve the sorted invariant by shifting elements.
 *
 *  Duplicate keys (keys equivalent under `Compare`) are not stored: an
 *  insertion of an equivalent key leaves the set unchanged.
 *
 *  @tparam Key Type of the stored keys. Must be default-constructible,
 *          move-constructible and move-assignable (requirements inherited
 *          from `MemArray`).
 *  @tparam Compare Strict weak ordering over `Key`. Defaults to
 *          `Aleph::less<Key>`.
 *
 *  @par Iterators
 *  Iterators are plain `const Key *` pointers (contiguous, random access).
 *  As with `std::set`, elements are immutable through iterators: mutating a
 *  key in place would break the sorted invariant. Any insertion or removal
 *  invalidates all iterators.
 *
 *  @par Aleph conventions
 *  Following the library convention (see `Array`), `empty()` **clears** the
 *  container and `is_empty()` is the emptiness predicate.
 *
 *  @par Exception Safety
 *  `insert` offers the strong guarantee up to the element shift: if the key
 *  copy/move assignment throws mid-shift, the guarantee degrades to basic
 *  (the container remains valid, contents unspecified). `erase` always
 *  removes the element; a failed shrinking reallocation is ignored.
 *
 *  @par Thread Safety
 *  Distinct instances may be used from distinct threads. Concurrent access
 *  to the same instance requires external synchronization.
 *
 *  @par Example
 *  @code
 *  FlatSet<int> s = {5, 1, 3, 1};   // builds {1, 3, 5}
 *  s.insert(4);                       // {1, 3, 4, 5}
 *  if (s.contains(3)) ...
 *  for (int x : s)                    // sorted iteration
 *    std::cout << x << " ";
 *  @endcode
 *
 *  @ingroup Sets
 */
template <typename Key, class Compare = Aleph::less<Key>>
class FlatSet
{
  MemArray<Key> keys_;
  Compare cmp_;

  // Index of the first element not less than k (lower bound position).
  size_t lower_idx(const Key &k) const
  {
    size_t lo = 0, hi = keys_.size();
    while (lo < hi)
      {
        const size_t mid = lo + ((hi - lo) >> 1);
        if (cmp_(keys_(mid), k))
          lo = mid + 1;
        else
          hi = mid;
      }
    return lo;
  }

  // Index of the first element greater than k (upper bound position).
  size_t upper_idx(const Key &k) const
  {
    size_t lo = 0, hi = keys_.size();
    while (lo < hi)
      {
        const size_t mid = lo + ((hi - lo) >> 1);
        if (not cmp_(k, keys_(mid)))
          lo = mid + 1;
        else
          hi = mid;
      }
    return lo;
  }

  // True if pos holds a key equivalent to k. Assumes pos == lower_idx(k),
  // so only the second half of the equivalence needs testing.
  bool match_at(size_t pos, const Key &k) const
  {
    return pos < keys_.size() and not cmp_(k, keys_(pos));
  }

  // Opens one slot at pos by shifting [pos, n) one place right.
  void make_room(size_t pos)
  {
    keys_.putn(1);
    Key *p = keys_.get_ptr();
    for (size_t i = keys_.size() - 1; i > pos; --i)
      p[i] = std::move(p[i - 1]);
  }

  // Removes the slot at pos by shifting (pos, n) one place left. A failed
  // shrinking reallocation is ignored: the logical removal always succeeds.
  void remove_at(size_t pos)
  {
    Key *p = keys_.get_ptr();
    const size_t last = keys_.size() - 1;
    for (size_t i = pos; i < last; ++i)
      p[i] = std::move(p[i + 1]);
    try
      {
        (void) keys_.get();
      }
    catch (...)
      {
        // get() only throws via the shrinking reallocation; the element
        // count was already decremented, so the removal took effect.
      }
  }

  // Sorts the freshly loaded backing array and drops duplicates, keeping
  // the first occurrence of each equivalence class.
  void sort_and_unique()
  {
    Key *p = keys_.get_ptr();
    std::stable_sort(p, p + keys_.size(), cmp_);
    size_t m = 0;
    for (size_t i = 0; i < keys_.size(); ++i)
      if (m == 0 or cmp_(p[m - 1], p[i]))
        {
          if (i != m)
            p[m] = std::move(p[i]);
          ++m;
        }
    const size_t excess = keys_.size() - m;
    if (excess > 0)
      (void) keys_.get(excess);
  }

public:

  using Item_Type = Key;            ///< Aleph convention: element type.
  using Key_Type = Key;             ///< Aleph convention: key type.
  using key_type = Key;             ///< STL convention: key type.
  using value_type = Key;           ///< STL convention: value type.
  using key_compare = Compare;      ///< STL convention: comparator type.
  using size_type = size_t;         ///< STL convention: size type.
  using iterator = const Key *;       ///< Random-access iterator (immutable).
  using const_iterator = const Key *; ///< Random-access const iterator.

  /** @brief Construct an empty set.
   *
   *  @param cap Initial capacity hint (rounded up to a power of two by the
   *         backing `MemArray`).
   *  @throws std::bad_alloc if the initial buffer cannot be allocated.
   */
  explicit FlatSet(size_t cap = MemArray<Key>::Min_Dim) : keys_(cap) {}

  /** @brief Construct an empty set with a specific comparator.
   *
   *  @param cmp Comparator instance to copy.
   *  @param cap Initial capacity hint.
   *  @throws std::bad_alloc if the initial buffer cannot be allocated.
   */
  explicit FlatSet(const Compare &cmp, size_t cap = MemArray<Key>::Min_Dim)
    : keys_(cap), cmp_(cmp) {}

  /** @brief Construct from an iterator range.
   *
   *  Loads all elements, then sorts once and removes duplicates: O(m log m)
   *  for m input elements, which beats m individual O(n) insertions.
   *
   *  @tparam It Input iterator whose value type converts to `Key`.
   *  @param first Beginning of the range.
   *  @param last End of the range.
   *  @param cmp Comparator instance (defaulted).
   *  @throws std::bad_alloc if memory allocation fails.
   *  @note On duplicate (equivalent) inputs the first occurrence wins.
   */
  template <class It,
            class = std::enable_if_t<not std::is_integral_v<It>>>
  FlatSet(It first, It last, const Compare &cmp = Compare())
    : keys_(MemArray<Key>::Min_Dim), cmp_(cmp)
  {
    for (; first != last; ++first)
      keys_.put(*first);
    sort_and_unique();
  }

  /** @brief Construct from an initializer list.
   *
   *  @param l Elements to load; duplicates are dropped (first wins).
   *  @param cmp Comparator instance (defaulted).
   *  @throws std::bad_alloc if memory allocation fails.
   */
  FlatSet(std::initializer_list<Key> l, const Compare &cmp = Compare())
    : FlatSet(l.begin(), l.end(), cmp) {}

  /// Copy constructor (requires copyable `Key`).
  FlatSet(const FlatSet &) = default;

  /// Move constructor. The source is left valid but unspecified.
  FlatSet(FlatSet &&) noexcept = default;

  /// Copy assignment (requires copyable `Key`).
  FlatSet &operator=(const FlatSet &) = default;

  /// Move assignment. The source is left valid but unspecified.
  FlatSet &operator=(FlatSet &&) noexcept = default;

  ~FlatSet() = default;

  /** @brief Swap contents with `s` in O(1).
   *  @param s Set to swap with.
   */
  void swap(FlatSet &s) noexcept(std::is_nothrow_swappable_v<Compare>)
  {
    keys_.swap(s.keys_);
    std::swap(cmp_, s.cmp_);
  }

  // -- capacity -------------------------------------------------------------

  /// Return the number of stored keys. O(1).
  [[nodiscard]] size_t size() const noexcept { return keys_.size(); }

  /// Return `true` if the set holds no keys. O(1).
  [[nodiscard]] bool is_empty() const noexcept { return keys_.is_empty(); }

  /// Return the capacity of the backing array. O(1).
  [[nodiscard]] size_t capacity() const noexcept { return keys_.capacity(); }

  /** @brief Reserve capacity for at least `cap` keys.
   *  @param cap Desired capacity (rounded up to a power of two).
   *  @throws std::bad_alloc if memory allocation fails.
   */
  void reserve(size_t cap) { keys_.reserve(cap); }

  /** @brief Remove all keys (Aleph convention). Capacity is kept.
   *  @note Following the Aleph container convention (see `Array::empty()`),
   *        this **mutates** the set; use `is_empty()` to test for emptiness.
   */
  void empty() noexcept { keys_.empty(); }

  /// Remove all keys. Alias of `empty()`. Capacity is kept.
  void clear() noexcept { keys_.clear(); }

  // -- lookup ---------------------------------------------------------------

  /** @brief Find a key.
   *  @param k Key to search for.
   *  @return Iterator to the matching key, or `end()` if absent.
   *  @note Complexity: O(log n) over contiguous memory.
   */
  const_iterator find(const Key &k) const
  {
    const size_t pos = lower_idx(k);
    return match_at(pos, k) ? begin() + pos : end();
  }

  /** @brief Test membership.
   *  @param k Key to search for.
   *  @return `true` if an equivalent key is stored.
   *  @note Complexity: O(log n).
   */
  [[nodiscard]] bool contains(const Key &k) const
  {
    return match_at(lower_idx(k), k);
  }

  /** @brief Count occurrences of a key (0 or 1).
   *  @param k Key to search for.
   *  @return 1 if present, 0 otherwise.
   *  @note Complexity: O(log n).
   */
  [[nodiscard]] size_t count(const Key &k) const
  {
    return contains(k) ? 1 : 0;
  }

  /** @brief First element not less than `k`.
   *  @param k Key to bound.
   *  @return Iterator to the first element `e` with `not cmp(e, k)`, or `end()`.
   *  @note Complexity: O(log n).
   */
  const_iterator lower_bound(const Key &k) const
  {
    return begin() + lower_idx(k);
  }

  /** @brief First element greater than `k`.
   *  @param k Key to bound.
   *  @return Iterator to the first element `e` with `cmp(k, e)`, or `end()`.
   *  @note Complexity: O(log n).
   */
  const_iterator upper_bound(const Key &k) const
  {
    return begin() + upper_idx(k);
  }

  /** @brief Range of elements equivalent to `k`.
   *  @param k Key to bound.
   *  @return Pair `{lower_bound(k), upper_bound(k)}` (length 0 or 1).
   *  @note Complexity: O(log n).
   */
  std::pair<const_iterator, const_iterator> equal_range(const Key &k) const
  {
    return {lower_bound(k), upper_bound(k)};
  }

  /** @brief Positional access to the i-th smallest key (checked).
   *  @param i Zero-based rank.
   *  @return Const reference to the i-th key in sorted order.
   *  @throws std::out_of_range if `i >= size()`.
   *  @note Complexity: O(1). This is the flat-container bonus over trees.
   */
  const Key &nth(size_t i) const { return keys_[i]; }

  /** @brief Positional access without bounds checking.
   *  @param i Zero-based rank (must be `< size()`).
   *  @return Const reference to the i-th key in sorted order.
   */
  const Key &operator()(size_t i) const noexcept { return keys_(i); }

  /** @brief Smallest key (checked).
   *  @return Const reference to the minimum.
   *  @throws std::underflow_error if the set is empty.
   */
  const Key &get_first() const
  {
    ah_underflow_error_if(is_empty()) << "FlatSet::get_first(): empty set";
    return keys_(0);
  }

  /** @brief Greatest key (checked).
   *  @return Const reference to the maximum.
   *  @throws std::underflow_error if the set is empty.
   */
  const Key &get_last() const
  {
    ah_underflow_error_if(is_empty()) << "FlatSet::get_last(): empty set";
    return keys_(keys_.size() - 1);
  }

  /// Smallest key (checked). Alias of `get_first()`.
  const Key &min() const { return get_first(); }

  /// Greatest key (checked). Alias of `get_last()`.
  const Key &max() const { return get_last(); }

  // -- modifiers ------------------------------------------------------------

  /** @brief Insert a copy of `k` if no equivalent key exists.
   *  @param k Key to insert.
   *  @return Pair of (iterator to the key in the set, `true` if inserted /
   *          `false` if an equivalent key was already present).
   *  @throws std::bad_alloc if a growing reallocation fails.
   *  @note Complexity: O(log n) search + O(n) shift.
   */
  std::pair<const_iterator, bool> insert(const Key &k)
  {
    const size_t pos = lower_idx(k);
    if (match_at(pos, k))
      return {begin() + pos, false};
    make_room(pos);
    keys_.get_ptr()[pos] = k;
    return {begin() + pos, true};
  }

  /** @brief Insert `k` by moving if no equivalent key exists.
   *  @param k Key to move in.
   *  @return Pair of (iterator to the key in the set, insertion flag).
   *  @throws std::bad_alloc if a growing reallocation fails.
   *  @note Complexity: O(log n) search + O(n) shift.
   */
  std::pair<const_iterator, bool> insert(Key &&k)
  {
    const size_t pos = lower_idx(k);
    if (match_at(pos, k))
      return {begin() + pos, false};
    make_room(pos);
    keys_.get_ptr()[pos] = std::move(k);
    return {begin() + pos, true};
  }

  /** @brief Construct a key in place and insert it.
   *
   *  The key is constructed once from `args` and then moved into position,
   *  so this is a convenience over `insert(Key(args...))`, not a true
   *  in-place construction (the sorted position must be searched first).
   *
   *  @tparam Args Argument types forwarded to `Key`'s constructor.
   *  @param args Arguments forwarded to `Key`'s constructor.
   *  @return Pair of (iterator to the key in the set, insertion flag).
   *  @throws std::bad_alloc if a growing reallocation fails.
   */
  template <class... Args>
  std::pair<const_iterator, bool> emplace(Args &&...args)
  {
    return insert(Key(std::forward<Args>(args)...));
  }

  /** @brief Remove the key equivalent to `k`, if present.
   *  @param k Key to remove.
   *  @return Number of removed elements (0 or 1).
   *  @note Complexity: O(log n) search + O(n) shift. Never throws on the
   *        removal itself; a failed shrinking reallocation is ignored.
   */
  size_t erase(const Key &k)
  {
    const size_t pos = lower_idx(k);
    if (not match_at(pos, k))
      return 0;
    remove_at(pos);
    return 1;
  }

  /** @brief Remove the key at iterator `it`.
   *  @param it Valid dereferenceable iterator into this set.
   *  @return Iterator to the element following the removed one.
   *  @throws std::out_of_range if `it` does not point into the set.
   *  @note Complexity: O(n) shift. All other iterators are invalidated.
   */
  const_iterator erase(const_iterator it)
  {
    const size_t pos = it - begin();
    ah_out_of_range_error_if(pos >= size())
      << "FlatSet::erase(): iterator out of range";
    remove_at(pos);
    return begin() + pos;
  }

  // -- iteration ------------------------------------------------------------

  /// Iterator to the smallest key. O(1).
  const_iterator begin() const noexcept { return keys_.get_ptr(); }

  /// Iterator past the greatest key. O(1).
  const_iterator end() const noexcept { return keys_.get_ptr() + keys_.size(); }

  /// Const iterator to the smallest key. O(1).
  const_iterator cbegin() const noexcept { return begin(); }

  /// Const iterator past the greatest key. O(1).
  const_iterator cend() const noexcept { return end(); }

  /// Pointer to the underlying sorted, contiguous storage. O(1).
  const Key *data() const noexcept { return keys_.get_ptr(); }

  /** @brief Traverse keys in sorted order while `operation` returns `true`.
   *
   *  Aleph-style bounded traversal: `operation` receives each key (as
   *  `const Key &`) in ascending order; returning `false` stops the walk.
   *
   *  @tparam Operation Callable `bool(const Key &)`.
   *  @param operation Operation to apply.
   *  @return `true` if all keys were visited, `false` if stopped early.
   */
  template <class Operation>
  bool traverse(Operation operation) const
  {
    const Key *p = keys_.get_ptr();
    const size_t m = keys_.size();
    for (size_t i = 0; i < m; ++i)
      if (not operation(p[i]))
        return false;
    return true;
  }

  // -- comparison -----------------------------------------------------------

  /** @brief Equality: same size and pairwise equal elements.
   *  @param s Set to compare against.
   *  @return `true` if both sets hold equal elements in the same order.
   *  @note Complexity: O(n). Requires `Key` to be equality comparable.
   */
  bool operator==(const FlatSet &s) const
  {
    return size() == s.size() and std::equal(begin(), end(), s.begin());
  }

  /// Inequality: negation of `operator==`.
  bool operator!=(const FlatSet &s) const { return not(*this == s); }
};

} // end namespace Aleph

#endif // TPL_FLAT_SET_H