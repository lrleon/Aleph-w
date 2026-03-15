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

/** @file cuckoo-filter.H
 *  @brief Probabilistic set membership with Cuckoo filters.
 *
 *  A Cuckoo filter is a space-efficient probabilistic data structure that
 *  supports adding, deleting, and checking for the presence of items.
 *  Compared to Bloom filters, Cuckoo filters support deletion and provide
 *  better space efficiency for low false positive rates.
 *
 *  ## Key Properties
 *
 *  - **Supports Deletion**: Unlike standard Bloom filters, items can be
 * removed.
 *  - **High Space Efficiency**: Better than Bloom filters for FP rates < 3%.
 *  - **Lookup Performance**: Constant time O(1) worst-case (2 bucket reads).
 *  - **Insertion Performance**: Amortized O(1).
 *
 *  ## How It Works
 *
 *  The filter consists of an array of buckets, where each bucket can hold
 *  multiple fingerprints (partial hashes).
 *
 *  1. An item `x` has two candidate buckets: `h1(x)` and `h2(x)`.
 *  2. `h2(x) = h1(x) ⊕ hash(fingerprint(x))`. This property allows
 *     finding the other candidate bucket without knowing the original item.
 *  3. On insertion, if both buckets are full, an existing fingerprint is
 *     "kicked out" to its alternative bucket, possibly triggering a chain
 *     of displacements (like a cuckoo bird).
 *
 *  @tparam T Item type.
 *  @tparam FingerprintBits Number of bits per fingerprint (e.g., 8, 12, 16).
 *  @tparam EntriesPerBucket Associativity (number of slots per bucket).
 *
 *  @par False Positive Rate
 *  The theoretical FP rate is approximately
 *  `2 * EntriesPerBucket / 2^FingerprintBits`.  With the defaults
 *  (8-bit fingerprints, 4 entries per bucket) this is ~3.1 %.
 *
 *  @note Inserting the **same item twice** stores two fingerprints.
 *  After a single `remove()` the filter will still report the item as
 *  present.  Callers that need set semantics should guard with
 *  `contains()` before inserting.
 *
 *  @ingroup Hash
 *  @author Leandro Rabindranath León
 */

#ifndef ALEPH_CUCKOO_FILTER_H
#define ALEPH_CUCKOO_FILTER_H

#include <cassert>
#include <cstddef>
#include <cmath>
#include <optional>
#include <random>
#include <algorithm>
#include <hash-fct.H>
#include <ah-errors.H>
#include <ahUtils.H>
#include <tpl_array.H>

namespace Aleph {
/** @brief Industrial-grade Cuckoo Filter implementation. */
template <typename T, size_t FingerprintBits = 8, size_t EntriesPerBucket = 4> class Cuckoo_Filter
{
  static_assert(FingerprintBits > 0 and FingerprintBits <= 32, "Fingerprint bits must be between 1 and 32");
  static_assert(EntriesPerBucket > 0, "Entries per bucket must be > 0");

  using Fingerprint = uint32_t;

  struct Bucket
  {
    Fingerprint slots[EntriesPerBucket];

    Bucket()
    {
      std::fill(slots, slots + EntriesPerBucket, 0);
    }

    void clear()
    {
      std::fill(slots, slots + EntriesPerBucket, 0);
    }

    bool insert(Fingerprint fp)
    {
      for (size_t i = 0; i < EntriesPerBucket; ++i)
        if (slots[i] == 0)
          {
            slots[i] = fp;
            return true;
          }
      return false;
    }

    bool contains(Fingerprint fp) const
    {
      for (size_t i = 0; i < EntriesPerBucket; ++i)
        if (slots[i] == fp)
          return true;
      return false;
    }

    bool remove(Fingerprint fp)
    {
      for (size_t i = 0; i < EntriesPerBucket; ++i)
        if (slots[i] == fp)
          {
            slots[i] = 0;
            return true;
          }
      return false;
    }

    void swap_random(Fingerprint &fp, std::mt19937 &rng)
    {
      std::uniform_int_distribution<size_t> dist(0, EntriesPerBucket - 1);
      std::swap(fp, slots[dist(rng)]);
    }

    void restore(Fingerprint fp, size_t idx)
    {
      slots[idx] = fp;
    }

    [[nodiscard]] size_t swap_random_slot(Fingerprint &fp, std::mt19937 &rng)
    {
      std::uniform_int_distribution<size_t> dist(0, EntriesPerBucket - 1);
      const size_t idx = dist(rng);
      std::swap(fp, slots[idx]);
      return idx;
    }
  };

  /// Victim entry: a fingerprint displaced during a failed kick chain.
  struct Victim
  {
    size_t index = 0;
    Fingerprint fp = 0;
  };

  Aleph::Array<Bucket> buckets;
  size_t num_items = 0;
  size_t num_buckets = 0;
  size_t bucket_mask = 0;
  mutable std::mt19937 rng_;

  static constexpr size_t max_kicks = 500;
  static constexpr size_t stash_size = 4;
  Victim stash_[stash_size] = {};
  size_t stash_count_ = 0;

  // Mask to keep only the desired number of bits for fingerprint
  static constexpr Fingerprint fp_mask = (FingerprintBits == 32) ? 0xFFFFFFFF : (1U << FingerprintBits) - 1;

  [[nodiscard]] Fingerprint get_fingerprint(const T &item) const
  {
    auto fp = static_cast<Fingerprint>(murmur3hash(item, 0x12345678) & fp_mask);
    if (fp == 0)
      fp = 1;  // 0 is reserved for empty slot
    return fp;
  }

  [[nodiscard]] size_t index_hash(const T &item) const noexcept
  {
    return dft_hash_fct(item) & bucket_mask;
  }

  [[nodiscard]] size_t alt_index(size_t i, Fingerprint fp) const
  {
    // h2(x) = h1(x) XOR hash(fp)
    // Both operands are in [0, bucket_mask], so the XOR result
    // stays in the same range, preserving perfect reversibility.
    return i ^ (murmur3hash(fp, 0x87654321) & bucket_mask);
  }

public:
  /**
   * @brief Construct a Cuckoo Filter with a given capacity.
   * @param capacity Minimum number of items to support.
   * @param seed Optional RNG seed for deterministic kick sequences.
   *             Pass a fixed value in tests to eliminate flakiness.
   *             Defaults to a non-deterministic device seed.
   */
  explicit Cuckoo_Filter(size_t capacity,
                         std::optional<std::uint32_t> seed = std::nullopt)
    : rng_(seed.has_value() ? std::mt19937(seed.value())
                            : std::mt19937(std::random_device{}()))
  {
    // Allocate with a safety margin so bounded cuckoo kick sequences succeed.
    // At 100% load the filter cannot insert reliably; kTargetLoad = 0.9 gives
    // ~11% extra slots so insertions stay well below the kick-cycle limit.
    constexpr double kTargetLoad = 0.9;
    const size_t needed_buckets =
      static_cast<size_t>(std::ceil(capacity / (EntriesPerBucket * kTargetLoad)));
    // Round up to a power of 2 so that bucket_mask works as a modulo
    num_buckets = next_power_of_2(needed_buckets);
    if (num_buckets == 0)
      num_buckets = 1;
    bucket_mask = num_buckets - 1;
    buckets.putn(num_buckets);
  }

  /** @brief Return the number of items currently in the filter. */
  [[nodiscard]] size_t size() const noexcept
  {
    return num_items;
  }

  /** @brief Return whether the filter contains no items. */
  [[nodiscard]] bool empty() const noexcept
  {
    return num_items == 0;
  }

  /** @brief Synonym for empty() in traditional Aleph style. */
  [[nodiscard]] bool is_empty() const noexcept
  {
    return empty();
  }

  /** @brief Return the total number of fingerprint slots (including stash). */
  [[nodiscard]] size_t capacity() const noexcept
  {
    return num_buckets * EntriesPerBucket + stash_size;
  }

  /** @brief Return the load factor of the filter (relative to capacity()). */
  [[nodiscard]] double load_factor() const noexcept
  {
    return 1.0 * num_items / capacity();
  }

  /**
   * @brief Insert an item into the filter.
   * @param item Item to insert.
   * @return `true` if insertion succeeded, `false` if filter is full.
   */
  bool insert(const T &item)
  {
    const Fingerprint fp = get_fingerprint(item);
    size_t i1 = index_hash(item);
    size_t i2 = alt_index(i1, fp);

    if (buckets(i1).insert(fp))
      {
        ++num_items;
        return true;
      }

    if (buckets(i2).insert(fp))
      {
        ++num_items;
        return true;
      }

    // Both buckets full, start kicking
    std::uniform_int_distribution<int> coin(0, 1);
    size_t curr_idx = coin(rng_) ? i1 : i2;
    Fingerprint curr_fp = fp;
    struct Kick_Record
    {
      size_t bucket_idx = 0;
      size_t slot_idx = 0;
      Fingerprint original_fp = 0;
    };
    Kick_Record history[max_kicks];
    size_t history_size = 0;

    for (size_t kick = 0; kick < max_kicks; ++kick)
      {
        const size_t slot_idx = buckets(curr_idx).swap_random_slot(curr_fp, rng_);
        history[history_size++] = {curr_idx, slot_idx, curr_fp};
        curr_idx = alt_index(curr_idx, curr_fp);
        if (buckets(curr_idx).insert(curr_fp))
          {
            ++num_items;
            return true;
          }
      }

    // Kick chain exhausted — save the displaced fingerprint in the stash
    if (stash_count_ < stash_size)
      {
        stash_[stash_count_++] = {curr_idx, curr_fp};
        ++num_items;
        return true;
      }

    while (history_size > 0)
      {
        const Kick_Record & rec = history[--history_size];
        buckets(rec.bucket_idx).restore(rec.original_fp, rec.slot_idx);
      }

    return false;  // Filter and stash both full
  }

  /** @brief Alias for insert(). */
  bool append(const T &item)
  {
    return insert(item);
  }

  /**
   * @brief Test membership.
   * @param item Item to check.
   * @return `true` if item is likely present, `false` if definitely not.
   */
  [[nodiscard]] bool contains(const T &item) const
  {
    const Fingerprint fp = get_fingerprint(item);
    const size_t i1 = index_hash(item);
    const size_t i2 = alt_index(i1, fp);

    if (buckets(i1).contains(fp) or buckets(i2).contains(fp))
      return true;

    for (size_t i = 0; i < stash_count_; ++i)
      if (stash_[i].fp == fp and (stash_[i].index == i1 or stash_[i].index == i2))
        return true;

    return false;
  }

  /**
   * @brief Remove an item from the filter.
   * @param item Item to remove.
   * @return `true` if an instance of the item was removed.
   */
  bool remove(const T &item)
  {
    const Fingerprint fp = get_fingerprint(item);
    const size_t i1 = index_hash(item);
    const size_t i2 = alt_index(i1, fp);

    if (buckets(i1).remove(fp) or buckets(i2).remove(fp))
      {
        --num_items;
        return true;
      }

    for (size_t i = 0; i < stash_count_; ++i)
      if (stash_[i].fp == fp and (stash_[i].index == i1 or stash_[i].index == i2))
        {
          stash_[i] = stash_[--stash_count_];
          --num_items;
          return true;
        }

    return false;
  }

  /** @brief Clear the filter. */
  void clear()
  {
    for (size_t i = 0; i < num_buckets; ++i)
      buckets(i).clear();
    num_items = 0;
    stash_count_ = 0;
  }
};
}  // namespace Aleph

#endif  // ALEPH_CUCKOO_FILTER_H
