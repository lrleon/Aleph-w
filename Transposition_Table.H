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

# ifndef TRANSPOSITION_TABLE_H
# define TRANSPOSITION_TABLE_H

#include <concepts>
#include <utility>

#include <state_search_common.H>

namespace Aleph {

/** @brief Stored bound classification for memoized search entries. */
enum class TranspositionBound
{
  Exact,        ///< Exact value for the stored search configuration.
  Lower_Bound,  ///< Value is a lower bound (fail-high style).
  Upper_Bound   ///< Value is an upper bound (fail-low style).
};

/** @brief Outcome of one store/update attempt in a transposition table. */
enum class TranspositionStoreStatus
{
  Inserted,  ///< A new key was inserted.
  Replaced,  ///< An existing key was updated according to replacement policy.
  Rejected   ///< Existing entry was kept and candidate was discarded.
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
 *
 *  @tparam Policy Functor type.
 *  @tparam Entry Entry type to compare.
 */
template <typename Policy, typename Entry>
concept TranspositionReplacementPolicy
  = requires(const Policy &policy, const Entry &candidate, const Entry &current) {
      { policy(candidate, current) } -> std::convertible_to<bool>;
    };

/** @brief Minimal protocol for memo tables used by search engines.
 *
 *  @tparam Table Concrete table type.
 *  @tparam Key Key type.
 *  @tparam Entry Stored entry type.
 */
template <typename Table, typename Key, typename Entry>
concept MemoTable = requires(Table &table, const Key &key, const Entry &entry) {
  { table.probe(key) } -> std::same_as<Entry *>;
  { table.store(key, entry) } -> std::same_as<TranspositionStoreStatus>;
};

/** @brief Minimal replacement policy: always replace the current entry.
 *
 *  @tparam Entry Type of entry stored in the transposition table.
 */
template <typename Entry>
struct Replace_Always
{
  /** @brief Decide whether to overwrite a stored entry.
   *  @return Always true.
   */
  bool operator()(const Entry &, const Entry &) const noexcept
  {
    return true;
  }
};

/** @brief Replacement policy that prefers entries searched to greater depth.
 *
 *  @tparam Entry Entry type (must expose `.depth`).
 */
template <typename Entry>
  requires requires(const Entry &entry) {
    { entry.depth } -> std::convertible_to<size_t>;
  }
struct Prefer_Deeper_Entry
{
  /** @brief Decide which entry to keep according to remaining depth.
   *  @param candidate New entry proposed for storage.
   *  @param current Entry already stored for the same key.
   *  @return True if `candidate.depth > current.depth`.
   */
  bool operator()(const Entry &candidate, const Entry &current) const noexcept
  {
    return candidate.depth > current.depth;
  }
};

/** @brief Transposition-table container built on top of Aleph hash maps.
 *
 *  Thread-safety: all member functions require external synchronization. The
 *  table stores entries in-place and returns mutable pointers when probing; as
 *  such, callers must ensure only one thread manipulates the table at a time.
 *
 *  @tparam Key Stable cache key type.
 *  @tparam Entry Cached value associated with `Key`.
 *  @tparam HashMapTable Aleph hash-table backend, `MapOLhash` by default.
 *  @tparam Cmp Equality comparator for keys.
 *  @tparam ReplacePolicy Policy deciding whether a new entry overwrites an
 *          existing entry for the same key.
 */
template <typename Key,
          typename Entry,
          template <typename, typename, class> class HashMapTable = MapOLhash,
          class Cmp = Aleph::equal_to<Key>,
          typename ReplacePolicy = Replace_Always<Entry>>
  requires TranspositionReplacementPolicy<ReplacePolicy, Entry>
class Transposition_Table
{
public:
  static constexpr bool enabled = true;  ///< Compile-time marker for search adapters.

  using Key_Type = Key;
  using Entry_Type = Entry;
  using Map_Type = SearchStorageMap<Key, Entry, HashMapTable, Cmp>;
  using Replace_Policy = ReplacePolicy;

  /** @brief Build an empty table with the default replacement policy.
   *  @note Complexity: O(1).
   */
  Transposition_Table() = default;

  /** @brief Build a table with a custom replacement policy instance.
   *  @param replace Replacement policy functor.
   *  @note Complexity: O(1).
   */
  explicit Transposition_Table(ReplacePolicy replace) : replace_(std::move(replace))
  {
    // empty
  }

  /** @brief Remove all stored entries.
   *  @note Complexity: O(n) due to hash table clearing.
   */
  void clear() noexcept
  {
    table_.empty();
  }

  /** @brief Reset only the accumulated storage statistics.
   *  @note Complexity: O(1).
   */
  void reset_stats() noexcept
  {
    stats_ = {};
  }

  /** @brief Number of currently stored entries.
   *  @return Count of entries. Complexity: O(1).
   */
  [[nodiscard]] size_t size() const noexcept
  {
    return table_.size();
  }

  /** @brief Return `true` if the table contains no entries.
   *  @return True when empty. Complexity: O(1).
   */
  [[nodiscard]] bool is_empty() const noexcept
  {
    return table_.is_empty();
  }

  /** @brief Read-only access to accumulated storage statistics.
   *  @return Reference to internal stats structure. Complexity: O(1).
   */
  [[nodiscard]] const TranspositionStats &stats() const noexcept
  {
    return stats_;
  }

  /** @brief Probe one key and return the stored entry if present.
   *  @param key Cache key.
   *  @return Mutable pointer to the stored entry, or `nullptr` on miss.
   *  @note Complexity: expected O(1) for hash-table lookup.
   */
  [[nodiscard]] Entry *probe(const Key &key)
  {
    return probe_impl(key);
  }

  /** @brief Probe one key using move semantics.
   *  @param key Cache key (consumed).
   *  @return Mutable pointer to the stored entry, or `nullptr` on miss.
   *  @note Complexity: expected O(1) for hash-table lookup.
   */
  [[nodiscard]] Entry *probe(Key &&key)
  {
    return probe_impl(std::move(key));
  }

  /** @brief Store a candidate entry for `key`.
   *
   *  If the key is absent, the entry is inserted. Otherwise the configured
   *  replacement policy decides whether the current entry is overwritten.
   *
   *  @param key Cache key.
   *  @param entry Candidate entry to store.
   *  @return Status describing whether the candidate was inserted, replaced or
   *          rejected.
   *  @note Complexity: expected O(1).
   */
  [[nodiscard]] TranspositionStoreStatus store(const Key &key, const Entry &entry)
  {
    return store_impl(key, entry);
  }

  /** @brief Store a candidate entry for `key` using move semantics.
   *  @param key Cache key (consumed).
   *  @param entry Entry moved into storage.
   *  @return Insert/replace/reject status.
   *  @note Complexity: expected O(1).
   */
  [[nodiscard]] TranspositionStoreStatus store(Key &&key, Entry entry)
  {
    return store_impl(std::move(key), std::move(entry));
  }

private:
  template <typename KeyLike>
  [[nodiscard]] Entry *probe_impl(KeyLike &&key)
  {
    ++stats_.probes;

    auto *pair = table_.search(std::forward<KeyLike>(key));
    if (pair == nullptr)
      {
        ++stats_.misses;
        return nullptr;
      }

    ++stats_.hits;
    return &pair->second;
  }

  template <typename KeyLike, typename EntryLike>
  [[nodiscard]] TranspositionStoreStatus store_impl(KeyLike &&key, EntryLike &&entry)
  {
    if (auto *current = table_.search(key); current != nullptr)
      {
        if (not replace_(entry, current->second))
          {
            ++stats_.rejected_updates;
            return TranspositionStoreStatus::Rejected;
          }

        current->second = std::forward<EntryLike>(entry);
        ++stats_.stores;
        ++stats_.replacements;
        return TranspositionStoreStatus::Replaced;
      }

    (void) table_.insert(std::forward<KeyLike>(key), std::forward<EntryLike>(entry));
    ++stats_.stores;
    return TranspositionStoreStatus::Inserted;
  }

  Map_Type table_;
  ReplacePolicy replace_ = {};
  TranspositionStats stats_;
};

}  // end namespace Aleph

# endif  // TRANSPOSITION_TABLE_H
