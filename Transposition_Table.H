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


/** @file Transposition_Table.H
 *  @brief Generic memoization / transposition-table support for state search.
 *
 *  This module keeps the storage layer intentionally small:
 *
 *  - a generic keyed transposition table over Aleph hash maps,
 *  - reusable statistics for probes/stores/replacements,
 *  - a minimal replacement-policy protocol,
 *  - a concept for domains that can expose a stable state key.
 *
 *  The table itself is algorithm-agnostic. Specific engines such as Negamax or
 *  Alpha-Beta define their own entry types and decide when a cached entry is
 *  valid for reuse.
 *
 *  @author Leandro Rabindranath León
 */

#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include <concepts>
#include <utility>

#include <state_search_common.H>

namespace Aleph
{

/** @brief Stored bound classification for memoized search entries. */
enum class TranspositionBound
{
  Exact,       ///< Exact value for the stored search configuration.
  Lower_Bound, ///< Value is a lower bound (fail-high style).
  Upper_Bound  ///< Value is an upper bound (fail-low style).
};

/** @brief Outcome of one store/update attempt in a transposition table. */
enum class TranspositionStoreStatus
{
  Inserted, ///< A new key was inserted.
  Replaced, ///< An existing key was updated according to replacement policy.
  Rejected  ///< Existing entry was kept and candidate was discarded.
};

/** @brief Statistics collected by memo/transposition storage. */
struct TranspositionStats
{
  size_t probes = 0;            ///< Number of probe attempts.
  size_t hits = 0;              ///< Number of successful probes.
  size_t misses = 0;            ///< Number of failed probes.
  size_t cutoffs = 0;           ///< Number of search cutoffs enabled by cached data.
  size_t stores = 0;            ///< Number of accepted stores (insertions or replacements).
  size_t replacements = 0;      ///< Number of accepted overwrites of existing entries.
  size_t rejected_updates = 0;  ///< Number of refused overwrites.
};

/** @brief Minimal protocol for replacement policies.
 *
 *  A replacement policy receives the candidate and the currently stored entry
 *  for the same key, and returns `true` when the candidate should overwrite the
 *  current one.
 */
template <typename Policy, typename Entry>
concept TranspositionReplacementPolicy =
  requires(const Policy &policy, const Entry &candidate, const Entry &current)
  {
    { policy(candidate, current) } -> std::convertible_to<bool>;
  };

/** @brief Minimal protocol for memo tables used by search engines. */
template <typename Table, typename Key, typename Entry>
concept MemoTable =
  requires(Table &table, const Key &key, const Entry &entry)
  {
    { table.probe(key) } -> std::same_as<Entry *>;
    { table.store(key, entry) } -> std::same_as<TranspositionStoreStatus>;
  };

/** @brief Generic concept for domains that can expose a stable state key. */
template <typename Provider>
concept SearchStateKeyProvider =
  requires(Provider &provider, const typename Provider::State &state)
  {
    typename Provider::State_Key;
    { provider.state_key(state) } -> std::convertible_to<typename Provider::State_Key>;
  };

/** @brief Concept for a set suitable for tracking globally visited states.
 *
 *  Compatible with @ref SearchStorageSet and the Aleph `DynHashSet` family.
 *  The engine uses `contains(key)` to check for prior visits and `insert(key)`
 *  to record a new one. Inserting an already-present key must be a no-op.
 */
template <typename Set, typename Key>
concept VisitedStateSet =
  requires(Set &set, const Key &key)
  {
    { set.contains(key) } -> std::convertible_to<bool>;
    set.insert(key);
  };

/** @brief Concept for a map tracking the best bound seen per visited state.
 *
 *  Compatible with @ref SearchStorageMap. The engine uses `search(key)` to
 *  probe for an existing bound and `insert(key, bound)` to record one.
 *  Updating an existing entry is done by mutating the value through the
 *  pointer returned by `search`.
 */
template <typename Map, typename Key, typename Objective>
concept VisitedBoundMap =
  requires(Map &map, const Key &key, const Objective &obj)
  {
    map.insert(key, obj);
    map.search(key);
  };

/** @brief Minimal replacement policy: always replace the current entry. */
template <typename Entry>
struct Replace_Always
{
  bool operator()(const Entry &, const Entry &) const noexcept
  {
    return true;
  }
};

/** @brief Replacement policy that prefers entries searched to greater depth. */
template <typename Entry>
  requires requires(const Entry &entry)
  {
    { entry.depth } -> std::convertible_to<size_t>;
  }
struct Prefer_Deeper_Entry
{
  bool operator()(const Entry &candidate, const Entry &current) const noexcept
  {
    return candidate.depth > current.depth;
  }
};

/** @brief Generic transposition / memo table over Aleph open-addressing maps.
 *
 *  @tparam Key Stable cache key type.
 *  @tparam Entry Cached value associated with `Key`.
 *  @tparam HashMapTable Aleph hash-table backend, `MapOLhash` by default.
 *  @tparam Cmp Equality comparator for keys.
 *  @tparam ReplacePolicy Policy deciding whether a new entry overwrites an
 *          existing entry for the same key.
 */
template <typename Key, typename Entry,
          template <typename, typename, class> class HashMapTable = MapOLhash,
          class Cmp = Aleph::equal_to<Key>,
          typename ReplacePolicy = Replace_Always<Entry>>
  requires TranspositionReplacementPolicy<ReplacePolicy, Entry>
class Transposition_Table
{
public:
  static constexpr bool enabled = true; ///< Compile-time marker for search adapters.

  using Key_Type = Key;
  using Entry_Type = Entry;
  using Map_Type = SearchStorageMap<Key, Entry, HashMapTable, Cmp>;
  using Replace_Policy = ReplacePolicy;

  Transposition_Table() = default;

  explicit Transposition_Table(ReplacePolicy replace)
    : replace_(std::move(replace))
  {
    // empty
  }

  /** @brief Remove all stored entries. */
  void clear() noexcept
  {
    table_.empty();
  }

  /** @brief Reset only the accumulated storage statistics. */
  void reset_stats() noexcept
  {
    stats_ = {};
  }

  /** @brief Number of currently stored entries. */
  [[nodiscard]] size_t size() const noexcept
  {
    return table_.size();
  }

  /** @brief Return `true` if the table contains no entries. */
  [[nodiscard]] bool is_empty() const noexcept
  {
    return table_.is_empty();
  }

  /** @brief Read-only access to accumulated storage statistics. */
  [[nodiscard]] const TranspositionStats &stats() const noexcept
  {
    return stats_;
  }

  /** @brief Probe one key and return the stored entry if present. */
  [[nodiscard]] Entry *probe(const Key &key)
  {
    ++stats_.probes;

    auto *pair = table_.search(key);
    if (pair == nullptr)
      {
        ++stats_.misses;
        return nullptr;
      }

    ++stats_.hits;
    return &pair->second;
  }

  /** @brief Probe one key and return the stored entry if present. */
  [[nodiscard]] Entry *probe(Key &&key)
  {
    ++stats_.probes;

    auto *pair = table_.search(std::move(key));
    if (pair == nullptr)
      {
        ++stats_.misses;
        return nullptr;
      }

    ++stats_.hits;
    return &pair->second;
  }

  /** @brief Store a candidate entry for `key`.
   *
   *  If the key is absent, the entry is inserted. Otherwise the configured
   *  replacement policy decides whether the current entry is overwritten.
   *
   *  @return Status describing whether the candidate was inserted, replaced or
   *          rejected.
   */
  [[nodiscard]] TranspositionStoreStatus store(const Key &key, const Entry &entry)
  {
    if (auto *current = table_.search(key); current != nullptr)
      {
        if (not replace_(entry, current->second))
          {
            ++stats_.rejected_updates;
            return TranspositionStoreStatus::Rejected;
          }

        current->second = entry;
        ++stats_.stores;
        ++stats_.replacements;
        return TranspositionStoreStatus::Replaced;
      }

    (void) table_.insert(key, entry);
    ++stats_.stores;
    return TranspositionStoreStatus::Inserted;
  }

  /** @brief Store a candidate entry for `key` using move semantics. */
  [[nodiscard]] TranspositionStoreStatus store(Key &&key, Entry entry)
  {
    if (auto *current = table_.search(key); current != nullptr)
      {
        if (not replace_(entry, current->second))
          {
            ++stats_.rejected_updates;
            return TranspositionStoreStatus::Rejected;
          }

        current->second = std::move(entry);
        ++stats_.stores;
        ++stats_.replacements;
        return TranspositionStoreStatus::Replaced;
      }

    (void) table_.insert(std::move(key), std::move(entry));
    ++stats_.stores;
    return TranspositionStoreStatus::Inserted;
  }

private:
  Map_Type table_;
  ReplacePolicy replace_ = {};
  TranspositionStats stats_;
};

} // end namespace Aleph

#endif // TRANSPOSITION_TABLE_H
