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
   *  ## Thread-safety
   *  This class is **not thread-safe**. External synchronization is required
   *  if multiple threads access the same HyperLogLog instance concurrently,
   *  especially if at least one thread modifies it (via update, merge, or clear).
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

    /** @brief Compute 64-bit hash using MurmurHash3.
     *  @note Requires a 64-bit platform; fails at compile time otherwise.
     */
    [[nodiscard]] static uint64_t hash64(const T & val) noexcept
    {
      static_assert(sizeof(size_t) >= 8,
        "HyperLogLog requires 64-bit size_t; murmur3hash is 32-bit on this platform");
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

    /** @brief Compute bias-correction constant alpha_m from Flajolet et al.
     *  @internal
     */
    [[nodiscard]] static double compute_alpha(const size_t m) noexcept
    {
      if (m == 16) return 0.673;
      if (m == 32) return 0.697;
      if (m == 64) return 0.709;
      return 0.7213 / (1.0 + 1.079 / static_cast<double>(m));
    }

  public:
    /** @brief Construct with precision parameter.
     *  @param[in] b Number of bits for index [4..16]. m = 2^b registers.
     *  @throws std::domain_error if b is outside [4, 16].
     *  @note Not thread-safe: constructors should not be called concurrently on the same memory.
     */
    explicit HyperLogLog(const uint8_t b = 12)
      : b_(b), m_(compute_m_or_throw(b)),
        alpha_m_(compute_alpha(m_)),
        registers_(m_, static_cast<uint8_t>(0))
    {}

    /** @brief Add an element to the set.
     *
     *  @param[in] val Element to incorporate into the cardinality sketch.
     *  @throws none
     *  @note Complexity: O(1).
     *  @note Not thread-safe: concurrent calls to update() must be externally synchronized.
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
     *
     *  @return Approximate number of distinct elements seen so far.
     *  @throws none
     *  @note Complexity: O(m) where m = 2^b.
     *  @note Thread-safe for concurrent reads as long as no concurrent update() or merge() is in progress.
     */
    [[nodiscard]] double estimate() const noexcept
    {
      double z = 0.0;
      size_t v = 0; // count of empty registers for Linear Counting
      for (size_t i = 0; i < m_; ++i)
        {
          // More efficient and stable than std::pow(2.0, -registers_[i])
          z += std::ldexp(1.0, -static_cast<int>(registers_[i]));
          if (registers_[i] == 0)
            ++v;
        }

      // Raw estimate
      double e = alpha_m_ * (static_cast<double>(m_) * static_cast<double>(m_)) / z;

      // Small range correction (Linear Counting)
      if (e <= 2.5 * static_cast<double>(m_))
        {
          if (v > 0)
            e = static_cast<double>(m_) * std::log(static_cast<double>(m_) / static_cast<double>(v));
        }
      // Large range correction: use log1p for numerical stability; saturate when ratio >= 1
      else
        {
          static constexpr double two_to_64 = 18446744073709551616.0;
          if (e > (1.0 / 30.0) * two_to_64)
            {
              const double ratio = e / two_to_64;
              if (ratio >= 1.0)
                return two_to_64; // saturate at hash-space limit
              e = -two_to_64 * std::log1p(-ratio);
            }
        }

      return e;
    }

    /** @brief Merge another HyperLogLog into this one (union of sets).
     *
     *  Combines two sketches by taking the element-wise maximum of their
     *  registers, which corresponds to the union of the underlying sets.
     *
     *  @param[in] other HyperLogLog with the same precision parameter b.
     *  @throws std::domain_error if the precision parameters (b) differ.
     *  @note Complexity: O(m) where m = 2^b.
     *  @note Not thread-safe: concurrent calls must be externally synchronized.
     */
    void merge(const HyperLogLog & other)
    {
      ah_domain_error_if(b_ != other.b_) << "HyperLogLog::merge: precision mismatch";

      for (size_t i = 0; i < m_; ++i)
        registers_(i) = std::max(registers_[i], other.registers_[i]);
    }

    /** @brief Reset all registers to zero.
     *
     *  @throws none
     *  @note Complexity: O(m) where m = 2^b.
     *  @note Not thread-safe: concurrent calls must be externally synchronized.
     */
    void clear()
    {
      for (size_t i = 0; i < m_; ++i)
        registers_(i) = 0;
    }

    /** @brief Number of HyperLogLog registers (2^b).
     *  @return size_t The register count m = 2^b used by this estimator.
     *  @throws none
     *  @note Complexity: O(1).
     *  @note Thread-safe for concurrent reads as long as no concurrent modification is in progress.
     */
    [[nodiscard]] size_t num_registers() const noexcept { return m_; }
  };
} // namespace Aleph

# endif // HYPERLOGLOG_H
