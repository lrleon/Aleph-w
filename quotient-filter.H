
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

/** @file quotient-filter.H
 *  @brief Quotient filter: a cache-friendly probabilistic set with deletion.
 *
 *  A quotient filter is a compact, cache-friendly probabilistic data
 *  structure for approximate set membership, similar to a Bloom filter
 *  but with three critical advantages:
 *
 *  1. **Supports deletion** without false negatives.
 *  2. **Cache-friendly**: all data lives in a contiguous array with
 *     linear probing, resulting in excellent locality.
 *  3. **Mergeable**: two filters with the same hash parameters can be
 *     merged in linear time.
 *
 *  ## How It Works
 *
 *  Each element is hashed to a p-bit fingerprint. The fingerprint is
 *  split into a **quotient** (upper q bits) and a **remainder** (lower
 *  r bits). The quotient selects a *canonical slot* in a 2^q-slot
 *  array; the remainder is stored there. Three metadata bits per slot
 *  handle collisions via linear probing within contiguous *runs*:
 *
 *  - `is_occupied`: the canonical slot for some stored quotient.
 *  - `is_continuation`: this slot is not the first in its run.
 *  - `is_shifted`: this slot holds a remainder shifted from its
 *    canonical position.
 *
 *  Lookup, insertion and deletion are O(1) amortized when the load
 *  factor is bounded below 75%.
 *
 *  ## False Positive Rate
 *
 *  The false positive probability is approximately `2^{-r}`, where
 *  r is the remainder size in bits. Since `p = q + r` and the table
 *  has `2^q` slots, choosing a larger r directly reduces the FP rate
 *  at the cost of more bits per slot.
 *
 *  ## Complexity
 *
 *  | Operation | Average | Worst case |
 *  |-----------|---------|------------|
 *  | insert    | O(1)    | O(n)       |
 *  | contains  | O(1)    | O(n)       |
 *  | remove    | O(1)    | O(n)       |
 *  | merge     | O(2^q)  | O(2^q)     |
 *  | space     | (r+3) * 2^q bits     |
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  // 2^20 slots, 8-bit remainders -> ~0.4% FP rate
 *  Quotient_Filter<std::string> qf(20, 8);
 *
 *  qf.insert("apple");
 *  qf.insert("banana");
 *
 *  qf.contains("apple");   // true (no false negatives)
 *  qf.contains("cherry");  // almost certainly false
 *
 *  qf.remove("apple");
 *  qf.contains("apple");   // false (deletion works!)
 *  ```
 *
 *  ## When to Use
 *
 *  - Prefer over Bloom filter when **deletion** is needed.
 *  - Prefer over Bloom filter when **cache performance** matters
 *    (e.g., SSD page-cache filters, network deduplication).
 *  - Prefer over Bloom filter when two filters must be **merged**.
 *  - Prefer Bloom filter when the required FP rate demands many
 *    independent hash functions (Bloom scales better at very low FP).
 *
 *  ## References
 *
 *  - Bender, M. A. et al. "Don't Thrash: How to Cache Your Hash on
 *    Flash." VLDB 2012.
 *  - Pandey, P. et al. "A General-Purpose Counting Filter: Making
 *    Every Bit Count." SIGMOD 2017.
 *
 *  @see bloom-filter.H   Bloom filter (no deletion, lower FP at high k)
 *  @see count-min-sketch.H Count-Min Sketch (frequency estimation)
 *  @see hyperloglog.H    HyperLogLog (cardinality estimation)
 *
 *  @ingroup Hash
 *  @author Leandro Rabindranath Leon
 */

#ifndef QUOTIENT_FILTER_H
#define QUOTIENT_FILTER_H

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <algorithm>
#include <utility>

#include <ah-errors.H>
#include <tpl_array.H>
#include <hash-fct.H>

namespace Aleph {

/** @brief Quotient filter for probabilistic set membership with deletion.
 *
 *  @tparam T Element type. Must be hashable via `murmur3hash(T, seed)`.
 */
template <typename T> class Quotient_Filter
{
  uint8_t  q_;
  uint8_t  r_;
  size_t   num_slots_;
  size_t   num_elems_;
  uint32_t seed_;

  // Per-slot storage packed into a uint64_t:
  //   bit 0: is_occupied   (belongs to the SLOT INDEX, not shifted data)
  //   bit 1: is_continuation
  //   bit 2: is_shifted
  //   bits 3..(r+2): remainder
  Array<uint64_t> slots_;

  static constexpr uint64_t OCC_BIT   = 1ULL << 0;
  static constexpr uint64_t CONT_BIT  = 1ULL << 1;
  static constexpr uint64_t SHFT_BIT  = 1ULL << 2;
  static constexpr uint64_t META_MASK = 0x7ULL;

  [[nodiscard]] uint64_t rem_mask() const noexcept
  {
    return (1ULL << r_) - 1;
  }

  [[nodiscard]] bool get_occ(size_t i) const noexcept
  {
    return (slots_[i] & OCC_BIT) != 0;
  }

  [[nodiscard]] bool get_cont(size_t i) const noexcept
  {
    return (slots_[i] & CONT_BIT) != 0;
  }

  [[nodiscard]] bool get_shft(size_t i) const noexcept
  {
    return (slots_[i] & SHFT_BIT) != 0;
  }

  // A slot is truly empty when it holds no data and no occupied marker.
  [[nodiscard]] bool slot_empty(size_t i) const noexcept
  {
    return slots_[i] == 0;
  }

  // A slot holds data if continuation or shifted is set, or if it
  // is the unshifted start of a run (occupied and not empty remainder
  // region... actually simplest: it holds data iff it is not
  // "metadata-only"). A slot is "has element" iff is_shifted or
  // is_continuation is set, OR (not shifted and not continuation and
  // the slot is occupied — i.e., it's a run start at its canonical
  // position).  Equivalently: a slot has an element stored iff
  // is_continuation or is_shifted, or (is_occupied and not shifted
  // and not continuation), which simplifies to: the slot is not
  // completely empty AND (if only occupied is set, there IS data
  // here — it's the canonical run start).
  //
  // Actually the precise check: a slot has data iff any of the three
  // bits is set (since occupied-only slots DO store data: the first
  // element of an unshifted run).
  // CORRECTION: that's wrong. A slot can have occupied=1 but hold
  // another quotient's shifted data. The occupied bit marks that
  // SOME run for this quotient exists, stored here or elsewhere.
  //
  // The simplest correct check: a slot holds element data iff
  // shifted or continuation is set, OR (it is unshifted and not a
  // continuation — i.e., it is a cluster start, meaning it stores
  // the first element of a run at its canonical position).
  //
  // Even simpler: a slot is "element-occupied" iff it is not fully
  // empty (meta != 0) and not "occupied-only" (meta == OCC_BIT).
  // Wait, that's also wrong. A slot with only OCC_BIT set has the
  // run start element at its canonical position!
  //
  // Let me think again. Slots that have data:
  //   1. occ=1, cont=0, shft=0: canonical run start (has data)
  //   2. occ=X, cont=0, shft=1: shifted run start (has data)
  //   3. occ=X, cont=1, shft=1: shifted continuation (has data)
  //   4. occ=X, cont=1, shft=0: impossible (continuation must be shifted
  //      unless it's at its canonical position, but continuation means
  //      it's not the first in its run, so it must be shifted)
  //      Actually wait — cont=1, shft=0: this IS possible if the
  //      continuation happens to land at its original quotient's
  //      canonical position... no, cont=1 means it's not the FIRST
  //      in the run, and shft=0 means it IS at its canonical position.
  //      Can this happen? Only if two elements have the same quotient
  //      and the continuation of the run happens to be at the same
  //      canonical slot. But runs are contiguous and sorted by
  //      remainder, and for the same quotient all have the same
  //      canonical slot. So the 2nd element of a run at slot fq+1
  //      would have cont=1, shft=1 (since it's shifted from fq).
  //      Unless fq itself has two elements? No, each slot holds one
  //      element. The run for quotient fq starts at slot fq (if
  //      unshifted), and continues at fq+1, fq+2, etc. with cont=1,
  //      shft=1.
  //
  // Bottom line: a slot has data stored iff (cont or shft) is set,
  // OR (both are clear AND occ is set — canonical unshifted run start).
  // Equivalently: (slots_[i] & META_MASK) != 0 and it's not the case
  // that the slot has ONLY the occupied bit with nothing stored.
  //
  // SIMPLIFICATION: In practice, a slot "has data" iff
  //   (slots_[i] & (CONT_BIT | SHFT_BIT)) != 0   ||
  //   slot is a canonical start (occ=1, cont=0, shft=0)
  // Which is the same as (slots_[i] & META_MASK) != 0 (any bit set).
  // But the problem is occ-only: does occ=1, cont=0, shft=0 ALWAYS
  // mean data is stored? YES. If occupied is set and the element is
  // not shifted, it means this slot IS the start of the run, so data
  // is here.
  //
  // FINAL ANSWER: has_element(i) iff (slots_[i] & META_MASK) != 0.
  // This is the same as !slot_empty(i).
  [[nodiscard]] bool has_element(const size_t i) const noexcept
  {
    return (slots_[i] & META_MASK) != 0;
  }

  // A slot is a "run start" if it has data and is_continuation is false.
  [[nodiscard]] bool is_run_start(const size_t i) const noexcept
  {
    return has_element(i) and not get_cont(i);
  }

  void set_occ(const size_t i, const bool v) noexcept
  {
    if (v)
      slots_(i) |= OCC_BIT;
    else
      slots_(i) &= ~OCC_BIT;
  }

  void set_cont(const size_t i, const bool v) noexcept
  {
    if (v)
      slots_(i) |= CONT_BIT;
    else
      slots_(i) &= ~CONT_BIT;
  }

  void set_shft(const size_t i, const bool v) noexcept
  {
    if (v)
      slots_(i) |= SHFT_BIT;
    else
      slots_(i) &= ~SHFT_BIT;
  }

  [[nodiscard]] uint64_t get_rem(const size_t i) const noexcept
  {
    return (slots_[i] >> 3) & rem_mask();
  }

  void set_rem(const size_t i, const uint64_t rem) noexcept
  {
    slots_(i) = (slots_[i] & META_MASK) | ((rem & rem_mask()) << 3);
  }

  // Set the full "element data" portion of a slot (cont, shft, rem),
  // but preserve the occupied bit which belongs to the slot index.
  void set_element(const size_t i, const bool cont, const bool shft, const uint64_t rem) noexcept
  {
    const bool occ = get_occ(i);
    slots_(i)      = ((rem & rem_mask()) << 3) | (shft ? SHFT_BIT : 0) | (cont ? CONT_BIT : 0) | (occ ? OCC_BIT : 0);
  }

  // Clear the element data (cont, shft, rem) but preserve occupied.
  void clear_element(const size_t i) noexcept
  {
    const bool occ = get_occ(i);
    slots_(i)      = occ ? OCC_BIT : 0;
  }

  [[nodiscard]] size_t incr(const size_t i) const noexcept
  {
    return (i + 1) & (num_slots_ - 1);
  }

  [[nodiscard]] size_t decr(const size_t i) const noexcept
  {
    return (i - 1) & (num_slots_ - 1);
  }

  [[nodiscard]] std::pair<size_t, uint64_t> fingerprint(const T &item) const
  {
    const size_t   h  = murmur3hash(item, seed_);
    const size_t   fq = h & (num_slots_ - 1);
    const uint64_t fr = (static_cast<uint64_t>(h) >> q_) & rem_mask();
    return {fq, fr};
  }

  // Find the start of the run whose canonical slot is `fq`.
  // Precondition: is_occupied(fq) is true.
  [[nodiscard]] size_t find_run_start(size_t fq) const noexcept
  {
    // 1. Walk backward from fq to find the cluster start.
    size_t j = fq;
    while (get_shft(j))
      j = decr(j);

    // 2. Walk forward: for each occupied canonical slot between
    //    j and fq, skip past its run.
    size_t i = j;
    while (j != fq)
      {
        do  // skip current run
          i = incr(i);
        while (get_cont(i));

        do  // advance to next occupied canonical slot
          j = incr(j);
        while (not get_occ(j));
      }
    return i;
  }

  // Find the first slot at or after `start` that has no element data.
  [[nodiscard]] size_t find_first_unused(size_t start) const noexcept
  {
    size_t i = start;
    while (has_element(i))
      i = incr(i);
    return i;
  }

  // Shift element data one slot to the right, from `pos` up to (but
  // not including) `empty`.  The occupied bit is never shifted — it
  // belongs to the slot index and is preserved in place.
  void shift_elements_right(size_t pos, size_t empty) noexcept
  {
    while (empty != pos)
      {
        const size_t prev         = decr(empty);
        const bool   occ_at_empty = get_occ(empty);
        slots_(empty)             = slots_[prev];
        set_occ(empty, occ_at_empty);  // restore occupied at destination
        set_shft(empty, true);         // everything shifted is shifted
        empty = prev;
      }
  }

  enum class Insert_Result
  {
    inserted,
    duplicate,
    full
  };

  // Core insert of a pre-computed (fq, fr) pair.
  [[nodiscard]] Insert_Result do_insert(size_t fq, uint64_t fr)
  {
    if (num_elems_ >= num_slots_)
      return Insert_Result::full;

    // Case 1: canonical slot is completely empty.
    if (slot_empty(fq))
      {
        slots_(fq) = OCC_BIT | ((fr & rem_mask()) << 3);
        ++num_elems_;
        return Insert_Result::inserted;
      }

    const bool was_occupied = get_occ(fq);
    set_occ(fq, true);

    if (was_occupied)
      {
        // A run for this quotient already exists.
        const size_t s = find_run_start(fq);

        // Scan the run for a duplicate or ordered insertion point.
        size_t pos = s;
        do
          {
            const uint64_t cur = get_rem(pos);
            if (cur == fr)
              return Insert_Result::duplicate;
            if (cur > fr)
              break;
            pos = incr(pos);
        } while (get_cont(pos));

        // `pos` is where we insert (before element at pos).
        // If we scanned past the last continuation without finding
        // cur > fr, pos is one past the run's end — still correct.

        const size_t empty = find_first_unused(pos);
        shift_elements_right(pos, empty);

        const bool at_run_start = (pos == s);
        set_element(pos, not at_run_start, pos != fq, fr);

        // If we displaced the old run start, it becomes a continuation.
        if (at_run_start)
          set_cont(incr(pos), true);

        ++num_elems_;
        return Insert_Result::inserted;
      }

    // No run for this quotient exists yet.  We must find where the
    // new single-element run should be inserted within the cluster.
    // Since we already set occupied(fq), find_run_start now locates
    // the correct position for this new run.
    const size_t s = find_run_start(fq);

    const size_t empty = find_first_unused(s);
    shift_elements_right(s, empty);

    set_element(s, false, s != fq, fr);
    ++num_elems_;
    return Insert_Result::inserted;
  }

  // Core delete: shift elements left after removing position `pos`.
  // The occupied bit is preserved at each index.
  void shift_elements_left(size_t pos) noexcept
  {
    size_t curr = pos;
    size_t nxt  = incr(curr);

    while (has_element(nxt) and get_shft(nxt))
      {
        const bool occ_at_curr = get_occ(curr);
        slots_(curr)           = slots_[nxt];
        set_occ(curr, occ_at_curr);

        // A run start that lands at its canonical slot is no longer shifted.
        // By the quotient ordering invariant, a run start at position i
        // where occupied(i) is set must be the run for quotient i.
        if (not get_cont(curr) and get_occ(curr))
          set_shft(curr, false);

        curr = nxt;
        nxt  = incr(nxt);
      }

    // Clear the last vacated slot, preserving its occupied bit.
    clear_element(curr);
  }

public:
  /** @brief Construct a quotient filter.
   *
   *  @param[in] q Number of quotient bits (2^q slots). Must be in [1, 32].
   *  @param[in] r Remainder bits per slot (FP rate ~ 2^{-r}). Must be in [1, 60].
   *  @param[in] seed Hash seed (default: 0x5F3759DF).
   *  @throws std::domain_error if q or r are out of range, or q + r > 64.
   */
  Quotient_Filter(uint8_t q, uint8_t r, uint32_t seed = 0x5F3759DF)
    : q_(q), r_(r), num_slots_(0), num_elems_(0), seed_(seed), slots_()
  {
    ah_domain_error_if(q < 1 or q > 32) << "Quotient_Filter: q must be in [1, 32] (got " << static_cast<int>(q) << ")";
    ah_domain_error_if(r < 1 or r > 60) << "Quotient_Filter: r must be in [1, 60] (got " << static_cast<int>(r) << ")";
    ah_domain_error_if(q + r > 64) << "Quotient_Filter: q + r must be <= 64 (got " << static_cast<int>(q + r) << ")";

    num_slots_ = size_t{1} << q;
    slots_     = Array<uint64_t>(num_slots_, uint64_t{0});
  }

  /** @brief Construct from desired capacity and false positive rate.
   *
   *  @param[in] expected_n Expected number of elements.
   *  @param[in] fp_rate    Target false positive probability.
   *  @param[in] seed       Hash seed.
   *  @throws std::domain_error if expected_n == 0 or fp_rate not in (0,1).
   */
  static Quotient_Filter from_capacity(size_t expected_n, double fp_rate, uint32_t seed = 0x5F3759DF)
  {
    ah_domain_error_if(expected_n == 0) << "Quotient_Filter::from_capacity: expected_n must be > 0";
    ah_domain_error_if(fp_rate <= 0.0 or fp_rate >= 1.0)
      << "Quotient_Filter::from_capacity: fp_rate must be in (0, 1) (got " << fp_rate << ")";

    const double slots_needed = static_cast<double>(expected_n) / 0.75;
    auto         q_v          = static_cast<uint8_t>(std::max(1.0, std::ceil(std::log2(slots_needed))));
    auto         r_v          = static_cast<uint8_t>(std::max(1.0, std::ceil(-std::log2(fp_rate))));
    if (q_v + r_v > 64)
      q_v = static_cast<uint8_t>(64 - r_v);
    return Quotient_Filter(q_v, r_v, seed);
  }

  /** @brief Insert an element.
   *
   *  After insertion, `contains(item)` is guaranteed true.
   *  Duplicates are silently ignored.
   *
   *  @throws std::overflow_error if the filter is full.
   */
  Quotient_Filter &insert(const T &item)
  {
    auto [fq, fr] = fingerprint(item);
    switch (do_insert(fq, fr))
      {
      case Insert_Result::inserted:
      case Insert_Result::duplicate:
        return *this;
      case Insert_Result::full:
        if (contains(item))
          return *this;
        ah_overflow_error() << "Quotient_Filter::insert: filter is full (" << num_elems_ << "/" << num_slots_ << ")";
      }
    return *this;
  }

  /** @brief Test membership.
   *
   *  May return false positives (~2^{-r}), never false negatives.
   */
  [[nodiscard]] bool contains(const T &item) const noexcept
  {
    auto [fq, fr] = fingerprint(item);

    if (not get_occ(fq))
      return false;

    size_t s = find_run_start(fq);
    do
      {
        uint64_t cur = get_rem(s);
        if (cur == fr)
          return true;
        if (cur > fr)
          return false;
        s = incr(s);
    } while (get_cont(s));

    return false;
  }

  /** @brief Remove an element.
   *
   *  @return `true` if the fingerprint was found and removed.
   */
  bool remove(const T &item) noexcept
  {
    auto [fq, fr] = fingerprint(item);

    if (not get_occ(fq))
      return false;

    size_t s   = find_run_start(fq);
    size_t pos = s;

    // Locate the remainder within the run.
    do
      {
        if (get_rem(pos) == fr)
          goto found;
        if (get_rem(pos) > fr)
          return false;
        pos = incr(pos);
    } while (get_cont(pos));
    return false;

    found: {
      const bool was_run_start = not get_cont(pos);
      size_t     nxt           = incr(pos);
      bool       nxt_is_cont   = has_element(nxt) and get_cont(nxt);

      if (was_run_start and nxt_is_cont)
        set_cont(nxt, false);  // promote next to run start

      if (was_run_start and not nxt_is_cont)
        set_occ(fq, false);  // run becomes empty

      shift_elements_left(pos);
      --num_elems_;
      return true;
    }
  }

  /** @brief Number of elements. O(1). */
  [[nodiscard]] size_t size() const noexcept
  {
    return num_elems_;
  }

  /** @brief Number of slots (2^q). O(1). */
  [[nodiscard]] size_t capacity() const noexcept
  {
    return num_slots_;
  }

  /** @brief Current load factor. O(1). */
  [[nodiscard]] double load_factor() const noexcept
  {
    return static_cast<double>(num_elems_) / num_slots_;
  }

  /** @brief True if empty. O(1). */
  [[nodiscard]] bool is_empty() const noexcept
  {
    return num_elems_ == 0;
  }

  /** @brief Quotient bits. */
  [[nodiscard]] uint8_t q() const noexcept
  {
    return q_;
  }

  /** @brief Remainder bits. */
  [[nodiscard]] uint8_t r() const noexcept
  {
    return r_;
  }

  /** @brief Theoretical FP probability ~ 2^{-r}. */
  [[nodiscard]] double false_positive_rate() const noexcept
  {
    return std::ldexp(1.0, -static_cast<int>(r_));
  }

  /** @brief Hash seed. */
  [[nodiscard]] uint32_t seed() const noexcept
  {
    return seed_;
  }

  /** @brief Memory usage in bytes. */
  [[nodiscard]] size_t memory_usage() const noexcept
  {
    return num_slots_ * sizeof(uint64_t);
  }

  /** @brief Remove all elements. O(2^q). */
  void clear() noexcept
  {
    for (size_t i = 0; i < num_slots_; ++i)
      slots_(i) = 0;
    num_elems_ = 0;
  }

  /** @brief Merge another filter (same q, r, seed). O(2^q).
   *  @throws std::domain_error on parameter mismatch.
   */
  void merge(const Quotient_Filter &other)
  {
    ah_domain_error_if(q_ != other.q_ or r_ != other.r_) << "Quotient_Filter::merge: parameter mismatch";
    ah_domain_error_if(seed_ != other.seed_) << "Quotient_Filter::merge: seed mismatch";

    for (size_t canonical = 0; canonical < other.num_slots_; ++canonical)
      {
        if (not other.get_occ(canonical))
          continue;

        size_t s = other.find_run_start(canonical);
        do
          {
            const auto result = do_insert(canonical, other.get_rem(s));
            ah_overflow_error_if(result == Insert_Result::full)
              << "Quotient_Filter::merge: destination filter is full (" << num_elems_ << "/" << num_slots_ << ")";
            s = other.incr(s);
        } while (other.get_cont(s));
      }
  }

  /** @brief Estimate (q, r) for n elements and target FP rate. */
  [[nodiscard]] static std::pair<uint8_t, uint8_t> estimate(size_t n, double fp_rate)
  {
    ah_domain_error_if(n == 0) << "n must be > 0";
    ah_domain_error_if(fp_rate <= 0.0 or fp_rate >= 1.0) << "fp_rate must be in (0, 1)";
    const double slots = static_cast<double>(n) / 0.75;
    auto         qv    = static_cast<uint8_t>(std::max(1.0, std::ceil(std::log2(slots))));
    auto         rv    = static_cast<uint8_t>(std::max(1.0, std::ceil(-std::log2(fp_rate))));
    if (qv + rv > 64)
      qv = static_cast<uint8_t>(64 - rv);
    return {qv, rv};
  }

  /** @brief Swap in O(1). */
  void swap(Quotient_Filter &other) noexcept
  {
    std::swap(q_, other.q_);
    std::swap(r_, other.r_);
    std::swap(num_slots_, other.num_slots_);
    std::swap(num_elems_, other.num_elems_);
    std::swap(seed_, other.seed_);
    slots_.swap(other.slots_);
  }
};

}  // end namespace Aleph

#endif  // QUOTIENT_FILTER_H