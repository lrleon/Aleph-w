
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

/** @file compact-quotient-filter.H
 *  @brief Memory-optimized quotient filter with bit-packed slots.
 *
 *  This is a space-optimized variant of \ref Aleph::Quotient_Filter that uses
 *  bit-packing to store slot metadata and remainders compactly. Instead
 *  of storing each slot in a full `uint64_t`, slots are packed into a
 *  `BitArray` using exactly `3 + r` bits per slot.
 *
 *  ## Space Savings
 *
 *  With r=8 remainder bits:
 *  - Standard `Quotient_Filter`: 64 bits/slot
 *  - `Compact_Quotient_Filter`: 11 bits/slot (3 metadata + 8 remainder)
 *  - **82% memory reduction**
 *
 *  With 2^20 slots (1M slots):
 *  - Standard: 8 MB
 *  - Compact: ~1.4 MB
 *
 *  ## Performance Trade-off
 *
 *  Bit-packing requires shift/mask operations on every slot access,
 *  which can be 15-30% slower than direct uint64_t indexing. Use this
 *  variant when memory is constrained and the performance cost is
 *  acceptable.
 *
 *  @tparam T Element type. Must be hashable via `murmur3hash(T, seed)`.
 *
 *  @see quotient-filter.H for the standard (faster) variant.
 *
 *  @ingroup Hash
 *  @author Leandro Rabindranath Leon
 */

#ifndef COMPACT_QUOTIENT_FILTER_H
#define COMPACT_QUOTIENT_FILTER_H

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <algorithm>
#include <utility>

#include <ah-errors.H>
#include <bitArray.H>
#include <hash-fct.H>

namespace Aleph {

/** @brief Memory-optimized quotient filter with bit-packed slots.
 *
 *  @tparam T Element type. Must be hashable via `murmur3hash(T, seed)`.
 */
template <typename T> class Compact_Quotient_Filter
{
  uint8_t  q_;
  uint8_t  r_;
  size_t   num_slots_;
  size_t   num_elems_;
  uint32_t seed_;

  // Bit-packed storage: each slot uses (3 + r) bits.
  // Layout per slot:
  //   bit 0: is_occupied
  //   bit 1: is_continuation
  //   bit 2: is_shifted
  //   bits 3..(r+2): remainder
  BitArray bits_;

  [[nodiscard]] size_t bits_per_slot() const noexcept
  {
    return 3 + r_;
  }

  [[nodiscard]] size_t slot_offset(const size_t i) const noexcept
  {
    return i * bits_per_slot();
  }

  [[nodiscard]] uint64_t rem_mask() const noexcept
  {
    return (1ULL << r_) - 1;
  }

  [[nodiscard]] bool get_bit_at(const size_t slot, const size_t bit_idx) const noexcept
  {
    return bits_.read_bit(slot_offset(slot) + bit_idx) != 0;
  }

  void set_bit_at(const size_t slot, const size_t bit_idx, const bool v) noexcept
  {
    bits_.write_bit(slot_offset(slot) + bit_idx, v ? 1 : 0);
  }

  [[nodiscard]] bool get_occ(const size_t i) const noexcept
  {
    return get_bit_at(i, 0);
  }

  [[nodiscard]] bool get_cont(const size_t i) const noexcept
  {
    return get_bit_at(i, 1);
  }

  [[nodiscard]] bool get_shft(const size_t i) const noexcept
  {
    return get_bit_at(i, 2);
  }

  [[nodiscard]] bool slot_empty(const size_t i) const noexcept
  {
    const size_t off = slot_offset(i);
    for (size_t b = 0; b < bits_per_slot(); ++b)
      if (bits_.read_bit(off + b))
        return false;
    return true;
  }

  [[nodiscard]] bool has_element(const size_t i) const noexcept
  {
    return get_occ(i) or get_cont(i) or get_shft(i);
  }

  [[nodiscard]] bool is_run_start(const size_t i) const noexcept
  {
    return has_element(i) and not get_cont(i);
  }

  void set_occ(const size_t i, const bool v) noexcept
  {
    set_bit_at(i, 0, v);
  }

  void set_cont(const size_t i, const bool v) noexcept
  {
    set_bit_at(i, 1, v);
  }

  void set_shft(const size_t i, const bool v) noexcept
  {
    set_bit_at(i, 2, v);
  }

  [[nodiscard]] uint64_t get_rem(const size_t i) const noexcept
  {
    uint64_t     rem = 0;
    const size_t off = slot_offset(i) + 3;
    for (size_t b = 0; b < r_; ++b)
      if (bits_.read_bit(off + b))
        rem |= (1ULL << b);
    return rem;
  }

  void set_rem(const size_t i, const uint64_t rem) noexcept
  {
    const size_t off = slot_offset(i) + 3;
    for (size_t b = 0; b < r_; ++b)
      bits_.write_bit(off + b, (rem & (1ULL << b)) ? 1 : 0);
  }

  void set_element(const size_t i, const bool cont, const bool shft, const uint64_t rem) noexcept
  {
    const bool occ = get_occ(i);
    set_cont(i, cont);
    set_shft(i, shft);
    set_rem(i, rem);
    set_occ(i, occ);
  }

  void clear_element(const size_t i) noexcept
  {
    const bool occ = get_occ(i);
    set_cont(i, false);
    set_shft(i, false);
    set_rem(i, 0);
    set_occ(i, occ);
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

  [[nodiscard]] size_t find_run_start(const size_t fq) const noexcept
  {
    size_t j = fq;
    while (get_shft(j))
      j = decr(j);

    size_t i = j;
    while (j != fq)
      {
        do
          i = incr(i);
        while (get_cont(i));

        do
          j = incr(j);
        while (not get_occ(j));
      }
    return i;
  }

  [[nodiscard]] size_t find_first_unused(const size_t start) const noexcept
  {
    size_t i = start;
    while (has_element(i))
      i = incr(i);
    return i;
  }

  void shift_elements_right(const size_t pos, size_t empty) noexcept
  {
    while (empty != pos)
      {
        const size_t prev         = decr(empty);
        const bool   occ_at_empty = get_occ(empty);

        const bool     prev_cont = get_cont(prev);
        const uint64_t prev_rem  = get_rem(prev);

        set_element(empty, prev_cont, true, prev_rem);
        set_occ(empty, occ_at_empty);

        empty = prev;
      }
  }

  bool do_insert(const size_t fq, const uint64_t fr)
  {
    if (num_elems_ >= num_slots_)
      return false;

    if (slot_empty(fq))
      {
        set_occ(fq, true);
        set_rem(fq, fr);
        ++num_elems_;
        return true;
      }

    const bool was_occupied = get_occ(fq);
    set_occ(fq, true);

    if (was_occupied)
      {
        const size_t s   = find_run_start(fq);
        size_t       pos = s;

        do
          {
            uint64_t cur = get_rem(pos);
            if (cur == fr)
              return false;
            if (cur > fr)
              break;
            pos = incr(pos);
        } while (get_cont(pos));

        const size_t empty = find_first_unused(pos);
        shift_elements_right(pos, empty);

        const bool at_run_start = (pos == s);
        set_element(pos, not at_run_start, pos != fq, fr);

        if (at_run_start)
          set_cont(incr(pos), true);

        ++num_elems_;
        return true;
      }

    const size_t s     = find_run_start(fq);
    const size_t empty = find_first_unused(s);
    shift_elements_right(s, empty);

    set_element(s, false, s != fq, fr);
    ++num_elems_;
    return true;
  }

  void shift_elements_left(const size_t pos) noexcept
  {
    size_t curr = pos;
    size_t nxt  = incr(curr);

    while (has_element(nxt) and get_shft(nxt))
      {
        const bool occ_at_curr = get_occ(curr);

        const bool     nxt_cont = get_cont(nxt);
        const bool     nxt_shft = get_shft(nxt);
        const uint64_t nxt_rem  = get_rem(nxt);

        set_element(curr, nxt_cont, nxt_shft, nxt_rem);
        set_occ(curr, occ_at_curr);

        if (not get_cont(curr) and get_occ(curr))
          set_shft(curr, false);

        curr = nxt;
        nxt  = incr(nxt);
      }

    clear_element(curr);
  }

public:
  /** @brief Construct a compact quotient filter.
   *
   *  @param[in] q Number of quotient bits (2^q slots). Must be in [1, 32].
   *  @param[in] r Remainder bits per slot (FP rate ~ 2^{-r}). Must be in [1, 60].
   *  @param[in] seed Hash seed (default: 0x5F3759DF).
   *  @throws std::domain_error if q or r are out of range, or q + r > 64.
   */
  Compact_Quotient_Filter(uint8_t q, uint8_t r, uint32_t seed = 0x5F3759DF)
    : q_(q), r_(r), num_slots_(0), num_elems_(0), seed_(seed), bits_()
  {
    ah_domain_error_if(q < 1 or q > 32) << "Compact_Quotient_Filter: q must be in [1, 32] (got " << static_cast<int>(q)
                                        << ")";
    ah_domain_error_if(r < 1 or r > 60) << "Compact_Quotient_Filter: r must be in [1, 60] (got " << static_cast<int>(r)
                                        << ")";
    ah_domain_error_if(q + r > 64) << "Compact_Quotient_Filter: q + r must be <= 64 (got " << static_cast<int>(q + r)
                                   << ")";

    num_slots_              = size_t{1} << q;
    const size_t total_bits = num_slots_ * bits_per_slot();
    bits_.reserve(total_bits);
  }

  /** @brief Construct from desired capacity and false positive rate.
   *
   *  @param[in] expected_n Expected number of elements.
   *  @param[in] fp_rate    Target false positive probability.
   *  @param[in] seed       Hash seed.
   *  @throws std::domain_error if expected_n == 0, fp_rate not in (0,1),
   *          or the requested (expected_n, fp_rate) cannot be represented
   *          within the supported parameter ranges.
   */
  static Compact_Quotient_Filter from_capacity(size_t expected_n, double fp_rate, uint32_t seed = 0x5F3759DF)
  {
    ah_domain_error_if(expected_n == 0) << "Compact_Quotient_Filter::from_capacity: expected_n must be > 0";
    ah_domain_error_if(fp_rate <= 0.0 or fp_rate >= 1.0)
      << "Compact_Quotient_Filter::from_capacity: fp_rate must be in (0, 1) (got " << fp_rate << ")";

    const double slots_needed = static_cast<double>(expected_n) / 0.75;
    const auto q_d = std::max(1.0, std::ceil(std::log2(slots_needed)));
    const auto r_d = std::max(1.0, std::ceil(-std::log2(fp_rate)));

    int q_v = static_cast<int>(q_d);
    int r_v = static_cast<int>(r_d);

    ah_domain_error_if(r_v < 1 or r_v > 60)
      << "Compact_Quotient_Filter::from_capacity: requested fp_rate requires r = " << r_v
      << " remainder bits, which is outside the supported range [1, 60]";

    if (q_v + r_v > 64)
      q_v = 64 - r_v;

    ah_domain_error_if(q_v < 1 or q_v > 32)
      << "Compact_Quotient_Filter::from_capacity: requested (expected_n, fp_rate) leads to q = " << q_v
      << ", which is outside the supported range [1, 32]";

    return Compact_Quotient_Filter(static_cast<uint8_t>(q_v), static_cast<uint8_t>(r_v), seed);
  }

  /** @brief Insert an element.
   *
   *  After insertion, `contains(item)` is guaranteed true.
   *  Duplicates are silently ignored.
   *
   *  @throws std::overflow_error if the filter is full.
   */
  Compact_Quotient_Filter &insert(const T &item)
  {
    ah_overflow_error_if(num_elems_ >= num_slots_)
      << "Compact_Quotient_Filter::insert: filter is full (" << num_elems_ << "/" << num_slots_ << ")";

    auto [fq, fr] = fingerprint(item);
    do_insert(fq, fr);
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
      const bool   was_run_start = not get_cont(pos);
      const size_t nxt           = incr(pos);
      const bool   nxt_is_cont   = has_element(nxt) and get_cont(nxt);

      if (was_run_start and nxt_is_cont)
        set_cont(nxt, false);

      if (was_run_start and not nxt_is_cont)
        set_occ(fq, false);

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
    return (num_slots_ * bits_per_slot() + 7) / 8;
  }

  /** @brief Remove all elements. O(2^q). */
  void clear() noexcept
  {
    const size_t total_bits = num_slots_ * bits_per_slot();
    for (size_t i = 0; i < total_bits; ++i)
      bits_.write_bit(i, 0);
    num_elems_ = 0;
  }

  /** @brief Merge another filter (same q, r, seed). O(2^q).
   *  @throws std::domain_error on parameter mismatch.
   */
  void merge(const Compact_Quotient_Filter &other)
  {
    ah_domain_error_if(q_ != other.q_ or r_ != other.r_) << "Compact_Quotient_Filter::merge: parameter mismatch";
    ah_domain_error_if(seed_ != other.seed_) << "Compact_Quotient_Filter::merge: seed mismatch";

    for (size_t canonical = 0; canonical < other.num_slots_; ++canonical)
      {
        if (not other.get_occ(canonical))
          continue;

        size_t s = other.find_run_start(canonical);
        do
          {
            do_insert(canonical, other.get_rem(s));
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
  void swap(Compact_Quotient_Filter &other) noexcept
  {
    std::swap(q_, other.q_);
    std::swap(r_, other.r_);
    std::swap(num_slots_, other.num_slots_);
    std::swap(num_elems_, other.num_elems_);
    std::swap(seed_, other.seed_);
    bits_.swap(other.bits_);
  }
};

}  // end namespace Aleph

#endif  // COMPACT_QUOTIENT_FILTER_H
