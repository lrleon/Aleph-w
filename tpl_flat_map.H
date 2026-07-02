
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

/** @file tpl_flat_map.H
 *  @brief Sorted-array map (`Aleph::FlatMap`), a cache-friendly ordered map.
 *
 *  `FlatMap<Key, T, Compare>` keeps two parallel contiguous arrays — one of
 *  sorted keys, one of values — mirroring the design of C++23
 *  `std::flat_map`. Binary searches touch only the key array, so lookups are
 *  as cache-friendly as they can get; insertions and removals shift both
 *  arrays (O(n)).
 *
 *  | Operation | FlatMap | DynMapTree (AVL/RB) |
 *  |-----------|----------|---------------------|
 *  | find / at / operator[] (hit) | O(log n), key array only | O(log n), pointer chasing |
 *  | insert / erase | O(n) shift | O(log n) |
 *  | iteration | O(n), sequential memory | O(n), pointer chasing |
 *  | memory per entry | sizeof(Key) + sizeof(T) | + node overhead |
 *
 *  Prefer `FlatMap` for lookup- and iteration-heavy workloads whose
 *  modification phase is bulk or infrequent. Prefer `DynMapTree` when
 *  insertions and removals are frequent.
 *
 *  @note Like `std::flat_map` (and unlike node-based maps), dereferencing an
 *        iterator yields a **proxy** `{const Key &first; T &second;}` rather
 *        than a reference to a stored `std::pair`. Use `auto [k, v] = *it;`
 *        or `it->second`; do not form `std::pair<Key,T> &` references.
 *
 *  @note This is a native Aleph implementation used under every supported
 *        standard; it does not alias `std::flat_map` (see the rationale in
 *        `tpl_flat_set.H`).
 *
 *  @see tpl_flat_set.H Sorted-array set counterpart (and rationale notes).
 *  @see tpl_dynMapTree.H Balanced-tree maps (fast mutation).
 *
 *  @ingroup Maps
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_FLAT_MAP_H
#define TPL_FLAT_MAP_H

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <utility>

#include <ah-errors.H>
#include <ahFunction.H>
#include <htlist.H>
#include <tpl_memArray.H>

namespace Aleph {

/** @brief Ordered map stored as two parallel sorted contiguous arrays.
 *
 *  Keys live in a sorted `MemArray<Key>`; the value associated with
 *  `keys[i]` lives at `values[i]`. Lookups binary-search the key array only.
 *  Keys equivalent under `Compare` are unique.
 *
 *  @tparam Key Type of the keys. Must be default-constructible,
 *          move-constructible and move-assignable (from `MemArray`).
 *  @tparam T Type of the mapped values. Same base requirements.
 *  @tparam Compare Strict weak ordering over `Key`. Defaults to
 *          `Aleph::less<Key>`.
 *
 *  @par Iterators
 *  Random-access proxy iterators: `*it` is a lightweight object with public
 *  members `first` (a `const Key &`) and `second` (a `T &`, or `const T &`
 *  through `const_iterator`). Any insertion or removal invalidates all
 *  iterators.
 *
 *  @par Aleph conventions
 *  Following the library convention (see `Array`), `empty()` **clears** the
 *  container and `is_empty()` is the emptiness predicate.
 *
 *  @par Exception Safety
 *  `insert` and `operator[]` offer the strong guarantee up to the element
 *  shift; if a key/value assignment throws mid-shift the guarantee degrades
 *  to basic (the map remains valid). `erase` always removes the entry; a
 *  failed shrinking reallocation is ignored.
 *
 *  @par Thread Safety
 *  Distinct instances may be used from distinct threads. Concurrent access
 *  to the same instance requires external synchronization.
 *
 *  @par Example
 *  @code
 *  FlatMap<std::string, int> m = {{"b", 2}, {"a", 1}};
 *  m["c"] = 3;                       // insert
 *  m["a"] += 10;                     // update
 *  for (auto [k, v] : m)             // sorted by key: a, b, c
 *    std::cout << k << " = " << v << "\n";
 *  @endcode
 *
 *  @ingroup Maps
 */
template <typename Key, typename T, class Compare = Aleph::less<Key>>
class FlatMap
{
  MemArray<Key> keys_;
  MemArray<T> vals_;
  Compare cmp_;

  // Index of the first key not less than k (lower bound position).
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

  // Index of the first key greater than k (upper bound position).
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

  // True if pos holds a key equivalent to k. Assumes pos == lower_idx(k).
  bool match_at(size_t pos, const Key &k) const
  {
    return pos < keys_.size() and not cmp_(k, keys_(pos));
  }

  // Opens one slot at pos in both arrays. Capacity is secured on both
  // arrays before any logical change so a failed allocation leaves the
  // map untouched (strong guarantee up to the shifts).
  void make_room(const size_t pos)
  {
    if (keys_.size() == keys_.capacity())
      keys_.reserve(keys_.capacity() << 1);
    if (vals_.size() == vals_.capacity())
      vals_.reserve(vals_.capacity() << 1);
    keys_.putn(1);
    vals_.putn(1);
    Key *kp = keys_.get_ptr();
    T *vp = vals_.get_ptr();
    for (size_t i = keys_.size() - 1; i > pos; --i)
      {
        kp[i] = std::move(kp[i - 1]);
        vp[i] = std::move(vp[i - 1]);
      }
  }

  // Removes the slot at pos from both arrays. Failed shrinking
  // reallocations are ignored: the logical removal always succeeds.
  void remove_at(const size_t pos)
  {
    Key *kp = keys_.get_ptr();
    T *vp = vals_.get_ptr();
    const size_t last = keys_.size() - 1;
    for (size_t i = pos; i < last; ++i)
      {
        kp[i] = std::move(kp[i + 1]);
        vp[i] = std::move(vp[i + 1]);
      }
    try
      {
        (void) keys_.get();
      }
    catch (const std::bad_alloc &)
      { /* shrink failed; logical removal already done */
      }
    try
      {
        (void) vals_.get();
      }
    catch (const std::bad_alloc &)
      { /* shrink failed; logical removal already done */
      }
  }

  // Sorts freshly loaded parallel arrays by key and drops duplicate keys,
  // keeping the first occurrence of each equivalence class.
  void sort_and_unique()
  {
    const size_t m = keys_.size();
    MemArray<std::pair<Key, T>> tmp(m);
    Key *kp = keys_.get_ptr();
    T *vp = vals_.get_ptr();
    for (size_t i = 0; i < m; ++i)
      tmp.put(std::pair<Key, T>(std::move(kp[i]), std::move(vp[i])));

    std::pair<Key, T> *tp = tmp.get_ptr();
    std::stable_sort(tp, tp + m, [this](const auto &a, const auto &b)
      {
        return cmp_(a.first, b.first);
      });

    size_t out = 0;
    for (size_t i = 0; i < m; ++i)
      if (out == 0 or cmp_(kp[out - 1], tp[i].first))
        {
          kp[out] = std::move(tp[i].first);
          vp[out] = std::move(tp[i].second);
          ++out;
        }
    const size_t excess = m - out;
    if (excess > 0)
      {
        try { (void) keys_.get(excess); } catch (const std::bad_alloc &)
          { /* shrink failed */ }
        try { (void) vals_.get(excess); } catch (const std::bad_alloc &)
          { /* shrink failed */ }
      }
  }

public:
  using Item_Type = std::pair<Key, T>;   ///< Aleph convention: element type.
  using Key_Type = Key;                  ///< Aleph convention: key type.
  using key_type = Key;                  ///< STL convention: key type.
  using mapped_type = T;                 ///< STL convention: mapped type.
  using value_type = std::pair<Key, T>;  ///< STL convention: value type.
  using key_compare = Compare;           ///< STL convention: comparator type.
  using size_type = size_t;              ///< STL convention: size type.

  /** @brief Random-access proxy iterator over `(key, value)` entries.
   *
   *  Dereferencing yields a proxy with public members `first`
   *  (`const Key &`) and `second` (`T &` when `IsConst == false`,
   *  `const T &` otherwise), matching the access model of C++23
   *  `std::flat_map`. The proxy is a value: bind it with `auto`, not
   *  `auto &` (e.g. `for (auto [k, v] : m)`).
   *
   *  @tparam IsConst Whether the iterator provides const access to values.
   */
  template <bool IsConst>
  class basic_iterator
  {
    using VPtr = std::conditional_t<IsConst, const T *, T *>;

    const Key *k_ = nullptr;
    VPtr v_ = nullptr;

  public:
    /// Proxy returned by `operator*`: references into the parallel arrays.
    struct reference
    {
      const Key &first;                                    ///< The entry's key.
      std::conditional_t<IsConst, const T &, T &> second;  ///< The entry's value.
    };

    /// Proxy returned by `operator->`, keeps the `reference` alive.
    struct pointer
    {
      reference ref;  ///< Materialized reference proxy.
      /// Give access to the members of the proxy.
      reference *operator ->() noexcept
      {
        return &ref;
      }
    };

    using iterator_category = std::random_access_iterator_tag;  ///< Category.
    using value_type = std::pair<Key, T>;                       ///< Materialized entry type.
    using difference_type = std::ptrdiff_t;                     ///< Signed distance type.

    /// Construct a singular iterator.
    basic_iterator() = default;

    /// Construct from parallel key/value cursors (internal use).
    basic_iterator(const Key *k, VPtr v) noexcept : k_(k), v_(v) {}

    /// Convert a mutable iterator into a const iterator.
    template <bool B, class = std::enable_if_t<IsConst and not B>>
    basic_iterator(const basic_iterator<B> &it) noexcept : k_(it.key_ptr()), v_(it.val_ptr())
    {}

    /// Cursor into the key array (internal use).
    const Key *key_ptr() const noexcept
    {
      return k_;
    }

    /// Cursor into the value array (internal use).
    VPtr val_ptr() const noexcept
    {
      return v_;
    }

    /// Dereference to the `(key, value)` proxy.
    reference operator * () const noexcept
    {
      return {*k_, *v_};
    }

    /// Member access through the proxy (e.g. `it->second`).
    pointer operator ->() const noexcept
    {
      return {{*k_, *v_}};
    }

    /// Proxy for the entry `i` positions away.
    reference operator [] (difference_type i) const noexcept
    {
      return {*(k_ + i), *(v_ + i)};
    }

    /// Pre-increment.
    basic_iterator &operator ++ () noexcept
    {
      ++k_;
      ++v_;
      return *this;
    }

    /// Post-increment.
    basic_iterator operator ++ (int) noexcept
    {
      basic_iterator ret = *this;
      ++*this;
      return ret;
    }

    /// Pre-decrement.
    basic_iterator &operator -- () noexcept
    {
      --k_;
      --v_;
      return *this;
    }

    /// Post-decrement.
    basic_iterator operator -- (int) noexcept
    {
      basic_iterator ret = *this;
      --*this;
      return ret;
    }

    /// Advance by `i` positions.
    basic_iterator &operator += (difference_type i) noexcept
    {
      k_ += i;
      v_ += i;
      return *this;
    }

    /// Retreat by `i` positions.
    basic_iterator &operator -= (difference_type i) noexcept
    {
      return *this += -i;
    }

    /// Iterator `i` positions forward.
    basic_iterator operator + (difference_type i) const noexcept
    {
      basic_iterator ret = *this;
      return ret += i;
    }

    /// Iterator `i` positions backward.
    basic_iterator operator - (difference_type i) const noexcept
    {
      basic_iterator ret = *this;
      return ret -= i;
    }

    /// Signed distance between iterators.
    difference_type operator - (const basic_iterator &it) const noexcept
    {
      return k_ - it.key_ptr();
    }

    /// Equality: same position.
    bool operator == (const basic_iterator &it) const noexcept
    {
      return k_ == it.key_ptr();
    }

    /// Inequality: different position.
    bool operator != (const basic_iterator &it) const noexcept
    {
      return k_ != it.key_ptr();
    }

    /// Strict ordering by position.
    bool operator < (const basic_iterator &it) const noexcept
    {
      return k_ < it.key_ptr();
    }

    /// Ordering by position.
    bool operator <= (const basic_iterator &it) const noexcept
    {
      return k_ <= it.key_ptr();
    }

    /// Strict ordering by position.
    bool operator > (const basic_iterator &it) const noexcept
    {
      return k_ > it.key_ptr();
    }

    /// Ordering by position.
    bool operator >= (const basic_iterator &it) const noexcept
    {
      return k_ >= it.key_ptr();
    }
  };

  using iterator = basic_iterator<false>;       ///< Mutable-value iterator.
  using const_iterator = basic_iterator<true>;  ///< Const-value iterator.

  /** @brief Construct an empty map.
   *
   *  @param cap Initial capacity hint (rounded up to a power of two by the
   *         backing `MemArray`s).
   *  @throws std::bad_alloc if the initial buffers cannot be allocated.
   */
  explicit FlatMap(size_t cap = MemArray<Key>::Min_Dim) : keys_(cap), vals_(cap) {}

  /** @brief Construct an empty map with a specific comparator.
   *
   *  @param cmp Comparator instance to copy.
   *  @param cap Initial capacity hint.
   *  @throws std::bad_alloc if the initial buffers cannot be allocated.
   */
  explicit FlatMap(const Compare &cmp, size_t cap = MemArray<Key>::Min_Dim)
    : keys_(cap), vals_(cap), cmp_(cmp)
  {}

  /** @brief Construct from an iterator range of pairs.
   *
   *  Loads all entries, then sorts once by key and removes duplicate keys:
   *  O(m log m) for m input entries.
   *
   *  @tparam It Input iterator whose value type converts to
   *          `std::pair<Key, T>`.
   *  @param first Beginning of the range.
   *  @param last End of the range.
   *  @param cmp Comparator instance (defaulted).
   *  @throws std::bad_alloc if memory allocation fails.
   *  @note On duplicate (equivalent) keys the first occurrence wins.
   */
  template <class It, class = std::enable_if_t<not std::is_integral_v<It>>>
  FlatMap(It first, It last, const Compare &cmp = Compare())
    : keys_(MemArray<Key>::Min_Dim), vals_(MemArray<T>::Min_Dim), cmp_(cmp)
  {
    for (; first != last; ++first)
      {
        const auto &p = *first;
        keys_.put(p.first);
        vals_.put(p.second);
      }
    sort_and_unique();
  }

  /** @brief Construct from an initializer list of pairs.
   *
   *  @param l Entries to load; duplicate keys are dropped (first wins).
   *  @param cmp Comparator instance (defaulted).
   *  @throws std::bad_alloc if memory allocation fails.
   */
  FlatMap(std::initializer_list<std::pair<Key, T>> l, const Compare &cmp = Compare())
    : FlatMap(l.begin(), l.end(), cmp)
  {}

  /// Copy constructor (requires copyable `Key` and `T`).
  FlatMap(const FlatMap &) = default;

  /// Move constructor. The source is left valid but unspecified.
  FlatMap(FlatMap &&) noexcept = default;

  /// Copy assignment (requires copyable `Key` and `T`).
  FlatMap &operator = (const FlatMap &) = default;

  /// Move assignment. The source is left valid but unspecified.
  FlatMap &operator = (FlatMap &&) noexcept = default;

  ~FlatMap() = default;

  /** @brief Swap contents with `m` in O(1).
   *  @param m Map to swap with.
   */
  void swap(FlatMap &m) noexcept(std::is_nothrow_swappable_v<Compare>)
  {
    keys_.swap(m.keys_);
    vals_.swap(m.vals_);
    std::swap(cmp_, m.cmp_);
  }

  // -- capacity -------------------------------------------------------------

  /// Return the number of stored entries. O(1).
  [[nodiscard]] size_t size() const noexcept
  {
    return keys_.size();
  }

  /// Return `true` if the map holds no entries. O(1).
  [[nodiscard]] bool is_empty() const noexcept
  {
    return keys_.is_empty();
  }

  /// Return the capacity of the backing key array. O(1).
  [[nodiscard]] size_t capacity() const noexcept
  {
    return keys_.capacity();
  }

  /** @brief Reserve capacity for at least `cap` entries.
   *  @param cap Desired capacity (rounded up to a power of two).
   *  @throws std::bad_alloc if memory allocation fails.
   */
  void reserve(size_t cap)
  {
    keys_.reserve(cap);
    vals_.reserve(cap);
  }

  /** @brief Remove all entries (Aleph convention). Capacity is kept.
   *  @note Following the Aleph container convention (see `Array::empty()`),
   *        this **mutates** the map; use `is_empty()` to test for emptiness.
   */
  void empty() noexcept
  {
    keys_.empty();
    vals_.empty();
  }

  /// Remove all entries. Alias of `empty()`. Capacity is kept.
  void clear() noexcept
  {
    empty();
  }

  // -- lookup ---------------------------------------------------------------

  /** @brief Find the entry with key `k`.
   *  @param k Key to search for.
   *  @return Iterator to the matching entry, or `end()` if absent.
   *  @note Complexity: O(log n) over the contiguous key array.
   */
  iterator find(const Key &k)
  {
    const size_t pos = lower_idx(k);
    return match_at(pos, k) ? begin() + pos : end();
  }

  /// \overload find(const Key & k)
  const_iterator find(const Key &k) const
  {
    const size_t pos = lower_idx(k);
    return match_at(pos, k) ? begin() + pos : end();
  }

  /** @brief Test whether key `k` is mapped.
   *  @param k Key to search for.
   *  @return `true` if an equivalent key is stored.
   *  @note Complexity: O(log n).
   */
  [[nodiscard]] bool contains(const Key &k) const
  {
    return match_at(lower_idx(k), k);
  }

  /** @brief Count entries with key `k` (0 or 1).
   *  @param k Key to search for.
   *  @return 1 if present, 0 otherwise.
   *  @note Complexity: O(log n).
   */
  [[nodiscard]] size_t count(const Key &k) const
  {
    return contains(k) ? 1 : 0;
  }

  /** @brief First entry whose key is not less than `k`.
   *  @param k Key to bound.
   *  @return Iterator to the bound, or `end()`.
   *  @note Complexity: O(log n).
   */
  iterator lower_bound(const Key &k)
  {
    return begin() + lower_idx(k);
  }

  /// \overload lower_bound(const Key & k)
  const_iterator lower_bound(const Key &k) const
  {
    return begin() + lower_idx(k);
  }

  /** @brief First entry whose key is greater than `k`.
   *  @param k Key to bound.
   *  @return Iterator to the bound, or `end()`.
   *  @note Complexity: O(log n).
   */
  iterator upper_bound(const Key &k)
  {
    return begin() + upper_idx(k);
  }

  /// \overload upper_bound(const Key & k)
  const_iterator upper_bound(const Key &k) const
  {
    return begin() + upper_idx(k);
  }

  /** @brief Range of entries with key equivalent to `k`.
   *  @param k Key to bound.
   *  @return Pair `{lower_bound(k), upper_bound(k)}` (length 0 or 1).
   *  @note Complexity: O(log n).
   */
  std::pair<iterator, iterator> equal_range(const Key &k)
  {
    return {lower_bound(k), upper_bound(k)};
  }

  /// \overload equal_range(const Key & k)
  std::pair<const_iterator, const_iterator> equal_range(const Key &k) const
  {
    return {lower_bound(k), upper_bound(k)};
  }

  /** @brief Checked access to the value mapped to `k`.
   *  @param k Key to search for.
   *  @return Mutable reference to the mapped value.
   *  @throws std::out_of_range if `k` is not mapped.
   *  @note Complexity: O(log n).
   */
  T &at(const Key &k)
  {
    const size_t pos = lower_idx(k);
    ah_out_of_range_error_if(not match_at(pos, k)) << "FlatMap::at(): key not found";
    return vals_(pos);
  }

  /// \overload at(const Key & k)
  const T &at(const Key &k) const
  {
    const size_t pos = lower_idx(k);
    ah_out_of_range_error_if(not match_at(pos, k)) << "FlatMap::at(): key not found";
    return vals_(pos);
  }

  /** @brief Access the value mapped to `k`, inserting a default if absent.
   *  @param k Key to search for or insert.
   *  @return Mutable reference to the mapped value.
   *  @throws std::bad_alloc if a growing reallocation fails.
   *  @note Complexity: O(log n) on hit, O(n) on insertion.
   */
  T &operator [] (const Key &k)
  {
    const size_t pos = lower_idx(k);
    if (match_at(pos, k))
      return vals_(pos);
    make_room(pos);
    keys_.get_ptr()[pos] = k;
    vals_.get_ptr()[pos] = T();
    return vals_(pos);
  }

  /** @brief Access the value mapped to `k`, inserting a default if absent.
   *
   *  Same as the const-reference overload, but moves `k` on insertion.
   *
   *  @param k Key to search for or insert (moved on insertion).
   *  @return Mutable reference to the mapped value.
   *  @throws std::bad_alloc if a growing reallocation fails.
   */
  T &operator [] (Key &&k)
  {
    const size_t pos = lower_idx(k);
    if (match_at(pos, k))
      return vals_(pos);
    make_room(pos);
    keys_.get_ptr()[pos] = std::move(k);
    vals_.get_ptr()[pos] = T();
    return vals_(pos);
  }

  // -- positional access ----------------------------------------------------

  /** @brief Key of the i-th entry in sorted order (checked).
   *  @param i Zero-based rank.
   *  @return Const reference to the i-th key.
   *  @throws std::out_of_range if `i >= size()`.
   *  @note Complexity: O(1).
   */
  const Key &nth_key(size_t i) const
  {
    return keys_[i];
  }

  /** @brief Value of the i-th entry in sorted order (checked).
   *  @param i Zero-based rank.
   *  @return Mutable reference to the i-th value.
   *  @throws std::out_of_range if `i >= size()`.
   *  @note Complexity: O(1).
   */
  T &nth_value(size_t i)
  {
    return vals_[i];
  }

  /// \overload nth_value(size_t i)
  const T &nth_value(size_t i) const
  {
    return vals_[i];
  }

  // -- modifiers ------------------------------------------------------------

  /** @brief Insert `(k, v)` if `k` is not mapped.
   *  @param k Key to insert.
   *  @param v Value to associate.
   *  @return Pair of (iterator to the entry, `true` if inserted / `false`
   *          if the key was already mapped — the existing value is kept).
   *  @throws std::bad_alloc if a growing reallocation fails.
   *  @note Complexity: O(log n) search + O(n) shift.
   */
  std::pair<iterator, bool> insert(const Key &k, const T &v)
  {
    const size_t pos = lower_idx(k);
    if (match_at(pos, k))
      return {begin() + pos, false};
    make_room(pos);
    keys_.get_ptr()[pos] = k;
    vals_.get_ptr()[pos] = v;
    return {begin() + pos, true};
  }

  /// \overload insert(const Key & k, const T & v) moving key and value.
  std::pair<iterator, bool> insert(Key &&k, T &&v)
  {
    const size_t pos = lower_idx(k);
    if (match_at(pos, k))
      return {begin() + pos, false};
    make_room(pos);
    keys_.get_ptr()[pos] = std::move(k);
    vals_.get_ptr()[pos] = std::move(v);
    return {begin() + pos, true};
  }

  /** @brief Insert a `(key, value)` pair if the key is not mapped.
   *  @param p Pair to insert.
   *  @return Pair of (iterator to the entry, insertion flag).
   *  @throws std::bad_alloc if a growing reallocation fails.
   */
  std::pair<iterator, bool> insert(const std::pair<Key, T> &p)
  {
    return insert(p.first, p.second);
  }

  /// \overload insert(const std::pair<Key, T> & p) moving the pair.
  std::pair<iterator, bool> insert(std::pair<Key, T> &&p)
  {
    return insert(std::move(p.first), std::move(p.second));
  }

  /** @brief Insert `(k, v)` or overwrite the value if `k` is mapped.
   *  @param k Key to insert or update.
   *  @param v Value to associate.
   *  @return Pair of (iterator to the entry, `true` if inserted / `false`
   *          if an existing value was overwritten).
   *  @throws std::bad_alloc if a growing reallocation fails.
   *  @note Complexity: O(log n) on update, O(n) on insertion.
   */
  std::pair<iterator, bool> insert_or_assign(const Key &k, const T &v)
  {
    const size_t pos = lower_idx(k);
    if (match_at(pos, k))
      {
        vals_(pos) = v;
        return {begin() + pos, false};
      }
    make_room(pos);
    keys_.get_ptr()[pos] = k;
    vals_.get_ptr()[pos] = v;
    return {begin() + pos, true};
  }

  /// \overload insert_or_assign(const Key & k, const T & v) moving both.
  std::pair<iterator, bool> insert_or_assign(Key &&k, T &&v)
  {
    const size_t pos = lower_idx(k);
    if (match_at(pos, k))
      {
        vals_(pos) = std::move(v);
        return {begin() + pos, false};
      }
    make_room(pos);
    keys_.get_ptr()[pos] = std::move(k);
    vals_.get_ptr()[pos] = std::move(v);
    return {begin() + pos, true};
  }

  /** @brief Build a key from `kargs` and insert it with a default value.
   *
   *  Convenience over `insert(Key(kargs...), T())`: the key is constructed
   *  once and moved into position; the value is default-constructed.
   *
   *  @tparam KArgs Argument types forwarded to `Key`'s constructor.
   *  @param kargs Arguments for the key.
   *  @return Pair of (iterator to the entry, insertion flag).
   *  @throws std::bad_alloc if a growing reallocation fails.
   */
  template <class... KArgs>
  std::pair<iterator, bool> emplace(KArgs &&...kargs)
  {
    return insert(Key(std::forward<KArgs>(kargs)...), T());
  }

  /** @brief Remove the entry with key `k`, if present.
   *  @param k Key to remove.
   *  @return Number of removed entries (0 or 1).
   *  @note Complexity: O(log n) search + O(n) shift. Never throws on the
   *        removal itself; failed shrinking reallocations are ignored.
   */
  size_t erase(const Key &k)
  {
    const size_t pos = lower_idx(k);
    if (not match_at(pos, k))
      return 0;
    remove_at(pos);
    return 1;
  }

  /** @brief Remove the entry at iterator `it`.
   *  @param it Valid dereferenceable iterator into this map.
   *  @return Iterator to the entry following the removed one.
   *  @throws std::out_of_range if `it` does not point into the map.
   *  @note Complexity: O(n) shift. All other iterators are invalidated.
   */
  iterator erase(const_iterator it)
  {
    const size_t pos = it.key_ptr() - keys_.get_ptr();
    ah_out_of_range_error_if(pos >= size()) << "FlatMap::erase(): iterator out of range";
    remove_at(pos);
    return begin() + pos;
  }

  // -- iteration ------------------------------------------------------------

  /// Iterator to the entry with the smallest key. O(1).
  iterator begin() noexcept
  {
    return iterator(keys_.get_ptr(), vals_.get_ptr());
  }

  /// Iterator past the entry with the greatest key. O(1).
  iterator end() noexcept
  {
    return iterator(keys_.get_ptr() + size(), vals_.get_ptr() + size());
  }

  /// Const iterator to the entry with the smallest key. O(1).
  const_iterator begin() const noexcept
  {
    return const_iterator(keys_.get_ptr(), vals_.get_ptr());
  }

  /// Const iterator past the entry with the greatest key. O(1).
  const_iterator end() const noexcept
  {
    return const_iterator(keys_.get_ptr() + size(), vals_.get_ptr() + size());
  }

  /// Const iterator to the entry with the smallest key. O(1).
  const_iterator cbegin() const noexcept
  {
    return begin();
  }

  /// Const iterator past the entry with the greatest key. O(1).
  const_iterator cend() const noexcept
  {
    return end();
  }

  /// Pointer to the sorted, contiguous key storage. O(1).
  const Key *keys_data() const noexcept
  {
    return keys_.get_ptr();
  }

  /// Pointer to the contiguous value storage (parallel to keys). O(1).
  T *values_data() noexcept
  {
    return vals_.get_ptr();
  }

  /// \overload values_data()
  const T *values_data() const noexcept
  {
    return vals_.get_ptr();
  }

  /** @brief Copy all keys, in ascending order, into a `DynList`.
   *  @return List of keys sorted according to `Compare`.
   *  @throws std::bad_alloc if memory allocation fails.
   *  @note Complexity: O(n) copies.
   */
  DynList<Key> keys() const
  {
    DynList<Key> ret;
    const Key *p = keys_.get_ptr();
    for (size_t i = 0; i < size(); ++i)
      ret.append(p[i]);
    return ret;
  }

  /** @brief Copy all values, in ascending key order, into a `DynList`.
   *  @return List of values ordered by their keys.
   *  @throws std::bad_alloc if memory allocation fails.
   *  @note Complexity: O(n) copies.
   */
  DynList<T> values() const
  {
    DynList<T> ret;
    const T *p = vals_.get_ptr();
    for (size_t i = 0; i < size(); ++i)
      ret.append(p[i]);
    return ret;
  }

  /** @brief Traverse entries in key order while `operation` returns `true`.
   *
   *  Aleph-style bounded traversal: `operation` receives `(const Key &,
   *  T &)` in ascending key order; returning `false` stops the walk.
   *
   *  @tparam Operation Callable `bool(const Key &, T &)`.
   *  @param operation Operation to apply.
   *  @return `true` if all entries were visited, `false` if stopped early.
   */
  template <class Operation>
  bool traverse(Operation operation)
  {
    const Key *kp = keys_.get_ptr();
    T *vp = vals_.get_ptr();
    const size_t m = size();
    for (size_t i = 0; i < m; ++i)
      if (not operation(kp[i], vp[i]))
        return false;
    return true;
  }

  /** @brief Const traversal in key order while `operation` returns `true`.
   *
   *  @tparam Operation Callable `bool(const Key &, const T &)`.
   *  @param operation Operation to apply.
   *  @return `true` if all entries were visited, `false` if stopped early.
   */
  template <class Operation>
  bool traverse(Operation operation) const
  {
    const Key *kp = keys_.get_ptr();
    const T *vp = vals_.get_ptr();
    const size_t m = size();
    for (size_t i = 0; i < m; ++i)
      if (not operation(kp[i], vp[i]))
        return false;
    return true;
  }

  // -- comparison -----------------------------------------------------------

  /** @brief Equality: same size and pairwise equal keys and values.
   *  @param m Map to compare against.
   *  @return `true` if both maps hold equal entries in the same order.
   *  @note Complexity: O(n). Requires `Key` and `T` equality comparable.
   */
  bool operator == (const FlatMap &m) const
  {
    return size() == m.size() and
      std::equal(keys_.get_ptr(), keys_.get_ptr() + size(), m.keys_.get_ptr()) and
      std::equal(vals_.get_ptr(), vals_.get_ptr() + size(), m.vals_.get_ptr());
  }

  /// Inequality: negation of `operator==`.
  bool operator != (const FlatMap &m) const
  {
    return not (*this == m);
  }
};

}  // end namespace Aleph

#endif  // TPL_FLAT_MAP_H
