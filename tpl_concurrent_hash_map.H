
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

/** @file tpl_concurrent_hash_map.H
 *  @brief Sharded concurrent hash map (`Aleph::ConcurrentHashMap`).
 *
 *  `ConcurrentHashMap<Key, T, Cmp, Shards>` partitions its key space across
 *  a fixed number of independently-locked shards, each an ordinary
 *  `Aleph::DynMapHashTable<Key, T>` protected by its own `std::shared_mutex`.
 *  There is no single global lock: two threads operating on keys that hash
 *  to different shards run fully in parallel (including two concurrent
 *  writers), and any number of readers within the *same* shard run
 *  concurrently with each other.
 *
 *  This is deliberately the simplest safe design, not a lock-free one: a
 *  lock-free hash map needs hazard pointers or epoch-based reclamation to
 *  free removed nodes safely, which is substantially harder to get right
 *  and verify than sharded locking. Sharding already removes the single
 *  global bottleneck that makes `Synchronized<DynMapHashTable<...>>` (one
 *  mutex for the whole table) a poor fit for high-concurrency workloads.
 *
 *  @par Thread-safety
 *  Every public member function is safe to call concurrently from any
 *  number of threads. Internally, each call maps its key to one shard
 *  (`hash(key) % Shards`, computed without locking anything) and then takes
 *  only that shard's `std::shared_mutex`: `contains()`/`find_copy()`/
 *  `with_value()` take a shared (read) lock, `insert()`/`insert_or_assign()`/
 *  `erase()`/`with_value_mut()` take an exclusive (write) lock. `size()`
 *  and `snapshot()` visit every shard in turn, each under its own lock.
 *
 *  @par What `size()`/`snapshot()` do *not* guarantee
 *  Because different shards are locked one at a time rather than all
 *  together, `size()` and `snapshot()` are consistent *per shard*, not
 *  transactionally consistent across the *whole* map: a concurrent
 *  insert/erase racing with `size()` may or may not be reflected in the
 *  count depending on exactly when it lands relative to which shard
 *  `size()` has already visited. The same applies to `snapshot()`: it is a
 *  union of independent per-shard point-in-time views, not one global
 *  linearization point. Neither method blocks other shards while visiting
 *  one, so this looseness is the price of not serializing the whole map
 *  behind a single lock.
 *
 *  @par No live iteration
 *  This type intentionally has no `begin()`/`end()` or other way to hold an
 *  iterator that outlives a single call: an iterator into a shard would
 *  either need to keep that shard locked for the iterator's entire
 *  lifetime (defeating the purpose of sharding) or risk becoming invalid
 *  the moment another thread mutates that shard. Use `snapshot()` to get
 *  an independent, owned copy of the entries instead, or `with_value()`/
 *  `with_value_mut()` to operate on one entry while its shard is locked.
 *
 *  @par References must not escape a lock
 *  `with_value()`/`with_value_mut()` invoke the caller's callback while
 *  holding the shard's lock and forbid the callback from *returning* a
 *  reference (enforced with a `static_assert`, mirroring
 *  `Aleph::Synchronized::with_lock()` in `concurrency_utils.H`). This
 *  catches the most common mistake (`return v;` from a callback taking
 *  `T&`/`const T&`) at compile time, but -- like `with_lock()` -- it
 *  cannot detect a callback that *captures* a pointer/reference to its
 *  argument into external state (e.g. a callback that assigns its
 *  parameter's address into a variable captured by reference); as with
 *  `with_lock()`, the callback should not do that, since the entry is
 *  only guaranteed valid while the shard's lock is held.
 *
 *  @par Example of use
 *  ```cpp
 *  Aleph::ConcurrentHashMap<std::string, int, Aleph::equal_to<std::string>, 16> cmap;
 *
 *  // Thread 1
 *  cmap.insert("apples", 10);
 *
 *  // Thread 2 (Runs in parallel without locking Thread 1, if hashes diff shards)
 *  cmap.insert_or_assign("oranges", 20);
 *
 *  // Thread 3 (Updates value safely using a lock over the specific shard)
 *  cmap.with_value_mut("apples", [](int& v) { v += 5; });
 *  ```
 *
 *  @see concurrency_utils.H for `Aleph::Synchronized`/`Aleph::RwSynchronized`
 *       (single-lock wrappers around an arbitrary type) and
 *       `Aleph::BoundedChannel` (blocking MPMC channel).
 *  @see tpl_dynSetHash.H for `Aleph::DynMapHashTable`, used internally as
 *       each shard's sequential map implementation.
 *  @ingroup Utilities
 *  @ingroup ThreadsAndConcurrency
 *  @author Leandro Rabindranath Leon
 *  @author (Documentation & Audit extensions)
 */

#ifndef TPL_CONCURRENT_HASH_MAP_H
#define TPL_CONCURRENT_HASH_MAP_H

#include <array>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <type_traits>
#include <utility>

#include <ah-errors.H>
#include <tpl_dynSetHash.H>

namespace Aleph
{

/** @brief Sharded concurrent hash map: `Shards` independently-locked
 *         `DynMapHashTable` partitions, no single global lock.
 *
 *  See the file-level documentation in `tpl_concurrent_hash_map.H` for the
 *  full design rationale, thread-safety contract, and the specific
 *  consistency model of `size()`/`snapshot()`.
 *
 *  @tparam Key Key type. Must satisfy whatever `Cmp` and the hash function
 *          require (typically equality-comparable and hashable).
 *  @tparam T Mapped value type.
 *  @tparam Cmp Key equality comparator, defaulting to `Aleph::equal_to<Key>`.
 *  @tparam Shards Number of independently-locked partitions. Must be at
 *          least 1; higher values reduce lock contention between keys that
 *          hash to different shards at the cost of `Shards` separate
 *          (smaller) hash tables and mutexes.
 */
template <typename Key, typename T,
          class Cmp = Aleph::equal_to<Key>,
          size_t Shards = 64>
class ConcurrentHashMap
{
  static_assert(Shards > 0, "ConcurrentHashMap requires at least one shard");

  using Map = DynMapHashTable<Key, T, LhashTable, Cmp>;

public:
  /// Hash function pointer type, matching `DynMapHashTable`'s own.
  using Hash_Fct_Ptr = typename Map::Hash_Fct_Ptr;

private:
  // Internal structure representing a single partition of the hash map.
  // Each Shard is protected by its own shared_mutex. alignas(64) pads each
  // Shard to its own cache line: without it, two Shard objects placed
  // next to each other by the allocator could share a cache line, and
  // then threads locking/unlocking *different* shards' mutexes under
  // write contention would still ping-pong that line between cores
  // (false sharing) -- defeating part of the point of sharding. 64 bytes
  // matches the cache-line size assumed elsewhere in Aleph (see
  // concurrency_utils.H's SpscQueue::head_/tail_).
  struct alignas(64) Shard
  {
    mutable std::shared_mutex mutex;
    Map map;

    Shard(Hash_Fct_Ptr hash_fct, const Cmp &cmp)
      : map(Primes::DefaultPrime, hash_fct, cmp)
    {}
  };

  std::array<std::unique_ptr<Shard>, Shards> shards_;
  Hash_Fct_Ptr hash_fct_;

  /** @brief Resolve which shard owns `key`.
   *
   *  @param[in] key Key to hash.
   *  @return The shard whose lock and table must be used for `key`.
   *  @pre `hash_fct_` is a deterministic, pure function of `key` alone:
   *       calling it twice with an equal key must yield the same shard
   *       index, or a key can effectively "move" between shards between
   *       calls and become invisible to later lookups.
   *  @pre `hash_fct_` must not throw: this function is `noexcept` but
   *       calls `hash_fct_(key)` directly, so a throwing hash function
   *       would call `std::terminate()` here instead of propagating.
   */
  [[nodiscard]] Shard &shard_for(const Key &key) noexcept
  {
    return *shards_[hash_fct_(key) % Shards];
  }

  /// @brief `const` overload of shard_for(const Key&).
  [[nodiscard]] const Shard &shard_for(const Key &key) const noexcept
  {
    return *shards_[hash_fct_(key) % Shards];
  }

public:
  /** @brief Construct an empty map with `Shards` independently-locked
   *         partitions.
   *
   *  @param[in] hash_fct Hash function applied to keys to pick a shard and
   *             to place entries within it. Defaults to the same
   *             `dft_hash_ptr_fct<Key>` used by `DynMapHashTable`'s own
   *             default constructor.
   *  @param[in] cmp Key equality comparator, copied into every shard.
   *  @throws std::bad_alloc if allocating any of the `Shards` shards fails.
   *  @note Not thread-safe against concurrent use of the object being
   *        constructed (ordinary construction rules).
   */
  explicit ConcurrentHashMap(Hash_Fct_Ptr hash_fct = dft_hash_ptr_fct<Key>,
                              const Cmp &cmp = Cmp())
    : hash_fct_(hash_fct)
  {
    for (auto &shard : shards_)
      shard = std::make_unique<Shard>(hash_fct, cmp);
  }

  /// @brief Deleted copy constructor: shards own non-copyable
  /// `std::shared_mutex` instances.
  ConcurrentHashMap(const ConcurrentHashMap &) = delete;
  /// @brief Deleted copy assignment operator.
  ConcurrentHashMap &operator=(const ConcurrentHashMap &) = delete;
  /// @brief Deleted move constructor: no defined ordering exists to
  /// safely transfer locked shards out from under concurrent callers.
  ConcurrentHashMap(ConcurrentHashMap &&) = delete;
  /// @brief Deleted move assignment operator.
  ConcurrentHashMap &operator=(ConcurrentHashMap &&) = delete;

  /** @brief Insert `key` with a copy of `value`, only if `key` is absent.
   *
   *  @param[in] key Key to insert.
   *  @param[in] value Value to copy in.
   *  @return `true` if inserted, `false` if `key` was already present (the
   *          existing value is left untouched).
   *  @throws Whatever `Key`'s or `T`'s copy constructor throws, or
   *          `std::bad_alloc`. If it throws, the map is left exactly as it
   *          was before the call. Also throws `std::system_error` if
   *          acquiring the shard's `std::shared_mutex` fails at the OS
   *          level.
   *  @note Safe to call concurrently from any number of threads. Blocks
   *        only other operations on the same shard.
   */
  bool insert(const Key &key, const T &value)
  {
    Shard &shard = shard_for(key);
    std::unique_lock lock(shard.mutex);
    return shard.map.insert(key, value) != nullptr;
  }

  /** @brief Insert `key` with `value` moved in, only if `key` is absent.
   *
   *  @param[in] key Key to insert.
   *  @param[in,out] value Value to move in.
   *  @return `true` if inserted, `false` if `key` was already present.
   *  @throws Whatever `Key`'s copy constructor or `T`'s move constructor
   *          throws, or `std::bad_alloc`. If it throws, the map is left
   *          exactly as it was before the call. Also throws
   *          `std::system_error` if acquiring the shard's
   *          `std::shared_mutex` fails at the OS level.
   *  @warning On a `false` return (duplicate `key`), `value` has still
   *           been moved from: the underlying table constructs the
   *           entry's `Pair` (moving `value` into it) *before* discovering
   *           the key already exists and discarding that entry. Do not
   *           read `value` after a `false` return; if you need to reuse
   *           it, check `contains(key)` first, or capture a copy before
   *           calling `insert`.
   *  @note Safe to call concurrently from any number of threads. Blocks
   *        only other operations on the same shard.
   */
  bool insert(const Key &key, T &&value)
  {
    Shard &shard = shard_for(key);
    std::unique_lock lock(shard.mutex);
    return shard.map.insert(key, std::move(value)) != nullptr;
  }

  /** @brief Insert `key` with `value`, or overwrite the existing value if
   *         `key` is already present.
   *
   *  @param[in] key Key to insert or update.
   *  @param[in] value New value, moved into the map either as a fresh
   *             entry or via move-assignment over the existing one.
   *  @throws Whatever `Key`'s copy constructor, `T`'s move constructor, or
   *          `T`'s move assignment throws, or `std::bad_alloc`. If
   *          updating an existing entry's move assignment throws, that
   *          entry is left in whatever state `T::operator=(T&&)` leaves a
   *          failed assignment in (the map itself remains structurally
   *          valid: the key is still present). Also throws
   *          `std::system_error` if acquiring the shard's
   *          `std::shared_mutex` fails at the OS level.
   *  @note Safe to call concurrently from any number of threads. Blocks
   *        only other operations on the same shard.
   */
  void insert_or_assign(const Key &key, T value)
  {
    Shard &shard = shard_for(key);
    std::unique_lock lock(shard.mutex);

    // We do a double lookup here (search + insert) because DynMapHashTable
    // requires checking for existence first to avoid duplicating or throwing.
    auto *pair = shard.map.search(key);
    if (pair != nullptr)
      pair->second = std::move(value);
    else
      shard.map.insert(key, std::move(value));
  }

  /** @brief Remove `key` if present.
   *
   *  @param[in] key Key to remove.
   *  @return `true` if `key` was present and removed, `false` otherwise.
   *  @throws std::system_error if acquiring the shard's `std::shared_mutex`
   *          fails at the OS level; nothing else (unlinking a located
   *          entry does not move- or copy-construct `T`, only destroys
   *          it).
   *  @note Safe to call concurrently from any number of threads. Blocks
   *        only other operations on the same shard.
   */
  bool erase(const Key &key)
  {
    Shard &shard = shard_for(key);
    std::unique_lock lock(shard.mutex);

    // remove_by_data() removes the already-located entry directly (via
    // pointer arithmetic back to its owning bucket), unlike
    // DynMapHashTable::remove(key), which re-searches internally and
    // move-constructs the removed value only to have it discarded here --
    // paying an extra lookup and a throwing-move risk for nothing.
    auto *pair = shard.map.search(key);
    if (pair == nullptr)
      return false;

    shard.map.remove_by_data(pair->second);
    return true;
  }

  /** @brief Check whether `key` is present.
   *
   *  @param[in] key Key to look up.
   *  @return `true` if present at the moment of the check.
   *  @throws std::system_error if acquiring the shard's `std::shared_mutex`
   *          fails at the OS level; nothing otherwise.
   *  @note Safe to call concurrently from any number of threads, including
   *        concurrently with other readers of the same shard. A
   *        concurrent insert/erase of the same key may change the answer
   *        immediately after this returns.
   */
  [[nodiscard]] bool contains(const Key &key) const
  {
    const Shard &shard = shard_for(key);
    std::shared_lock lock(shard.mutex);
    return shard.map.contains(key);
  }

  /** @brief Look up `key` and return a copy of its value, if present.
   *
   *  @param[in] key Key to look up.
   *  @return A copy of the value if `key` is present, `std::nullopt`
   *          otherwise.
   *  @throws Whatever `T`'s copy constructor throws. Also throws
   *          `std::system_error` if acquiring the shard's
   *          `std::shared_mutex` fails at the OS level.
   *  @note Safe to call concurrently from any number of threads, including
   *        concurrently with other readers of the same shard.
   */
  [[nodiscard]] std::optional<T> find_copy(const Key &key) const
    requires std::is_copy_constructible_v<T>
  {
    const Shard &shard = shard_for(key);
    std::shared_lock lock(shard.mutex);
    const auto *pair = shard.map.search(key);
    if (pair == nullptr)
      return std::nullopt;
    return pair->second;
  }

  /** @brief Invoke `f` with a `const` reference to `key`'s value, while
   *         the owning shard is read-locked.
   *
   *  @tparam F Callable, invocable as `f(const T&)`, whose return value
   *          (if any) must not be a reference.
   *  @param[in] key Key to look up.
   *  @param[in] f Callback invoked with the value if `key` is present.
   *  @return `true` if `key` was present and `f` was invoked, `false`
   *          otherwise.
   *  @throws Whatever `f` throws, propagated after the lock is released.
   *          Also throws `std::system_error` if acquiring the shard's
   *          `std::shared_mutex` fails at the OS level (before `f` is
   *          ever invoked).
   *  @note Safe to call concurrently from any number of threads, including
   *        concurrently with other readers of the same shard. `f` runs
   *        with the shard locked: keep it short, and never call back into
   *        this same `ConcurrentHashMap` from within `f` (that would
   *        deadlock if it targets the same shard).
   */
  template <typename F>
  bool with_value(const Key &key, F &&f) const
  {
    using Result = std::invoke_result_t<F, const T &>;
    static_assert(not std::is_reference_v<Result>,
                  "ConcurrentHashMap::with_value callback must not return a reference");
    const Shard &shard = shard_for(key);
    std::shared_lock lock(shard.mutex);
    const auto *pair = shard.map.search(key);
    if (pair == nullptr)
      return false;
    std::invoke(std::forward<F>(f), std::as_const(pair->second));
    return true;
  }

  /** @brief Invoke `f` with a mutable reference to `key`'s value, while
   *         the owning shard is write-locked.
   *
   *  @tparam F Callable, invocable as `f(T&)`, whose return value (if any)
   *          must not be a reference.
   *  @param[in] key Key to look up.
   *  @param[in] f Callback invoked with the value if `key` is present; may
   *             mutate it in place.
   *  @return `true` if `key` was present and `f` was invoked, `false`
   *          otherwise.
   *  @throws Whatever `f` throws, propagated after the lock is released.
   *          Also throws `std::system_error` if acquiring the shard's
   *          `std::shared_mutex` fails at the OS level (before `f` is
   *          ever invoked).
   *  @note Safe to call concurrently from any number of threads. `f` runs
   *        with the shard exclusively locked: keep it short, and never
   *        call back into this same `ConcurrentHashMap` from within `f`.
   */
  template <typename F>
  bool with_value_mut(const Key &key, F &&f)
  {
    using Result = std::invoke_result_t<F, T &>;
    static_assert(not std::is_reference_v<Result>,
                  "ConcurrentHashMap::with_value_mut callback must not return a reference");
    Shard &shard = shard_for(key);
    std::unique_lock lock(shard.mutex);
    auto *pair = shard.map.search(key);
    if (pair == nullptr)
      return false;
    std::invoke(std::forward<F>(f), pair->second);
    return true;
  }

  /** @brief Remove every entry from every shard.
   *
   *  @throws std::system_error if acquiring a shard's `std::shared_mutex`
   *          fails at the OS level; nothing otherwise (not `noexcept`
   *          specifically because that lock acquisition is not).
   *  @note Safe to call concurrently with other operations, but is not a
   *        single atomic point in time across the whole map: shards are
   *        cleared one at a time, each under its own lock, so a
   *        concurrent insert into a not-yet-cleared shard can still be
   *        present when this returns.
   */
  void clear()
  {
    for (auto &shard : shards_)
      {
        std::unique_lock lock(shard->mutex);
        shard->map.clear();
      }
  }

  /** @brief Return the total number of entries across all shards.
   *
   *  @return Sum of each shard's entry count, each read under that
   *          shard's own lock.
   *  @throws std::system_error if acquiring a shard's `std::shared_mutex`
   *          fails at the OS level; nothing otherwise.
   *  @note Safe to call concurrently with other operations. Not a
   *        transactionally consistent count across the whole map -- see
   *        the class-level note on `size()`/`snapshot()` consistency.
   */
  [[nodiscard]] size_t size() const
  {
    size_t total = 0;
    for (const auto &shard : shards_)
      {
        std::shared_lock lock(shard->mutex);
        total += shard->map.size();
      }
    return total;
  }

  /** @brief Check whether every shard is currently empty.
   *
   *  @return `true` if no shard held any entry at the moment each was
   *          checked.
   *  @throws std::system_error if acquiring a shard's `std::shared_mutex`
   *          fails at the OS level; nothing otherwise.
   *  @note Same consistency caveat as `size()`.
   */
  [[nodiscard]] bool is_empty() const
  {
    for (const auto &shard : shards_)
      {
        std::shared_lock lock(shard->mutex);
        if (not shard->map.is_empty())
          return false;
      }
    return true;
  }

  /** @brief Return an independent copy of every `(key, value)` pair
   *         currently in the map.
   *
   *  @return An `Array` holding a copy of every entry seen while visiting
   *          each shard in turn.
   *  @throws Whatever `Key`'s or `T`'s copy constructor throws, or
   *          `std::bad_alloc` from growing the returned `Array`. Also
   *          throws `std::system_error` if acquiring a shard's
   *          `std::shared_mutex` fails at the OS level.
   *  @note Safe to call concurrently with other operations. Each shard is
   *        visited under its own lock and released before moving to the
   *        next, so this is a union of independent per-shard point-in-time
   *        views, not one global linearization point -- see the
   *        class-level note on `size()`/`snapshot()` consistency. The
   *        returned `Array` is a fully independent copy: mutating the map
   *        afterward never affects it.
   */
  [[nodiscard]] Array<std::pair<Key, T>> snapshot() const
    requires std::is_copy_constructible_v<Key> and std::is_copy_constructible_v<T>
  {
    Array<std::pair<Key, T>> result;
    size_t reserved = 0;
    for (const auto &shard : shards_)
      {
        std::shared_lock lock(shard->mutex);
        // Grow the reservation by this (already-locked) shard's own O(1)
        // size *before* copying its entries in, so appends below never
        // trigger a reallocation -- without a separate extra pass over
        // every shard (and its lock) just to compute a grand total first.
        reserved += shard->map.size();
        result.reserve(reserved);
        for (const auto &entry : shard->map)
          result.append(entry);
      }
    return result;
  }
};

}  // namespace Aleph

#endif  // TPL_CONCURRENT_HASH_MAP_H