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

# ifndef HYPERLOGLOG_H
# define HYPERLOGLOG_H

/** @file hyperloglog.H
 *  @brief HyperLogLog for approximate cardinality estimation.
 *
 *  Provides the `HyperLogLog` class for estimating the number of unique
 *  elements in a stream with very low memory footprint and high accuracy.
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */

# include <cmath>
# include <algorithm>
# include <bit>
# include <cstdint>

# include <ah-errors.H>
# include <tpl_array.H>
# include <hash-fct.H>

namespace Aleph
{
  /** @brief HyperLogLog cardinality estimator.
   *
   *  Estimates the count of distinct elements (NDV) in a stream.
   *  Typical error is 1.04/sqrt(m), where m is the number of registers.
   *
   *  @tparam T Type of elements to count.
   */
  template <typename T>
  class HyperLogLog
  {
    uint8_t b_; /**< Number of bits for bucket index. */
    size_t m_; /**< Number of registers (2^b_). */
    double alpha_m_; /**< Bias correction constant. */
    Array<uint8_t> registers_; /**< Max leading zeros observed per bucket. */

    /** @brief Compute 64-bit hash using MurmurHash3. */
    [[nodiscard]] static uint64_t hash64(const T & val) noexcept
    {
      // Aleph's murmur3hash returns size_t (64-bit on x86_64).
      // We force a 64-bit interpretation for consistency.
      return static_cast<uint64_t>(murmur3hash(val, 0x12345678));
    }

    /** @brief Validate b and compute m = 2^b.
     *  @internal
     */
    [[nodiscard]] static size_t compute_m_or_throw(const uint8_t b)
    {
      ah_domain_error_if(b < 4 or b > 16)
        << "HyperLogLog: b must be in range [4, 16] (got " << static_cast<int>(b) << ")";
      return static_cast<size_t>(1) << b;
    }

  public:
    /** @brief Construct with precision parameter.
     *  @param[in] b Number of bits for index [4..16]. m = 2^b registers.
     */
    explicit HyperLogLog(const uint8_t b = 12)
      : b_(b), m_(compute_m_or_throw(b))
    {
      registers_ = Array<uint8_t>(m_, static_cast<uint8_t>(0));

      // Standard constants from Flajolet et al.
      if (m_ == 16) alpha_m_ = 0.673;
      else if (m_ == 32) alpha_m_ = 0.697;
      else if (m_ == 64) alpha_m_ = 0.709;
      else alpha_m_ = 0.7213 / (1.0 + 1.079 / static_cast<double>(m_));
    }

    /** @brief Add an element to the set.
     *  @note Complexity: O(1).
     */
    void update(const T & val)
    {
      const uint64_t x = hash64(val);
      const size_t j = x >> (64 - b_); // upper b bits for index
      const uint64_t w = x << b_ | (static_cast<uint64_t>(1) << (b_ - 1)); // remaining bits

      // count leading zeros in the hash part not used for indexing

      if (const auto rho = static_cast<uint8_t>(std::countl_zero(w) + 1); rho > registers_[j])
        registers_(j) = rho;
    }

    /** @brief Estimate current cardinality.
     *  @return Approximate number of unique elements.
     */
    [[nodiscard]] double estimate() const noexcept
    {
      double z = 0.0;
      size_t v = 0; // count of empty registers for Linear Counting
      for (size_t i = 0; i < m_; ++i)
        {
          z += std::pow(2.0, -static_cast<double>(registers_[i]));
          if (registers_[i] == 0)
            ++v;
        }

      // Cast each factor individually to avoid size_t overflow on 32-bit
      double e = alpha_m_ * (static_cast<double>(m_) * static_cast<double>(m_)) / z;

      // Small range correction (Linear Counting)
      if (e <= 2.5 * static_cast<double>(m_))
        {
          if (v > 0)
            e = static_cast<double>(m_) * std::log(static_cast<double>(m_) / static_cast<double>(v));
        }
      // Large range correction
      else if (e > (1.0 / 30.0) * std::pow(2.0, 64.0))
        e = -std::pow(2.0, 64.0) * std::log(1.0 - e / std::pow(2.0, 64.0));

      return e;
    }

    /** @brief Merge another HyperLogLog into this one.
     *  @throws std::domain_error if register count (b) mismatches.
     */
    void merge(const HyperLogLog & other)
    {
      ah_domain_error_if(b_ != other.b_) << "HyperLogLog::merge: precision mismatch";

      for (size_t i = 0; i < m_; ++i)
        registers_(i) = std::max(registers_[i], other.registers_[i]);
    }

    /** @brief Reset all registers. */
    void clear()
    {
      for (size_t i = 0; i < m_; ++i)
        registers_(i) = 0;
    }

    [[nodiscard]] size_t num_registers() const noexcept { return m_; }
  };
} // namespace Aleph

# endif // HYPERLOGLOG_H
