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

# ifndef SIMHASH_H
# define SIMHASH_H

/** @file simhash.H
 *  @brief SimHash for estimating cosine similarity between sets of features.
 *
 *  Provides the `SimHash` class for computing bitwise fingerprints of 
 *  weighted feature sets. SimHash is particularly useful for detecting
 *  near-duplicates in documents or large datasets.
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */

# include <algorithm>
# include <cmath>
# include <cstdint>
# include <bit>

# include <ah-errors.H>
# include <tpl_array.H>
# include <hash-fct.H>

namespace Aleph
{

  /** @brief SimHash fingerprint generator.
   *
   *  Maintains a 64-bit fingerprint representing a weighted set of features.
   *  The similarity between two SimHash values is proportional to the 
   *  Hamming distance between their fingerprints.
   *
   *  @tparam T Feature type.
   */
  template <typename T>
  class SimHash
  {
  public:
    /** @brief Number of bits in the SimHash fingerprint (always 64).
     *
     *  The fingerprint is always a 64-bit value.  Note that the underlying
     *  `murmur3hash()` returns `size_t`, which is 32-bit on non-x86_64
     *  platforms; on those builds the upper 32 bits of every fingerprint
     *  will be zero, effectively halving the collision resistance.
     */
    static constexpr size_t FINGERPRINT_SIZE = 64;

  private:
    Array<double> v_; /**< Accumulator vector for each bit. */

  public:
    /** @brief Construct SimHash accumulator. */
    SimHash()
      : v_(FINGERPRINT_SIZE, 0.0)
    {}

    /** @brief Add a feature to the set.
     *
     *  @param[in] val    The feature to add.
     *  @param[in] weight Importance of this feature (default: 1.0).
     */
    void update(const T & val, const double weight = 1.0)
    {
      const auto hash = static_cast<uint64_t>(murmur3hash(val, 0x9e3779b9));
      for (size_t i = 0; i < FINGERPRINT_SIZE; ++i)
        {
          if ((hash >> i) & 1)
            v_(i) += weight;
          else
            v_(i) -= weight;
        }
    }

    /** @brief Add all features in a range to the SimHash accumulator.
     *
     *  Calls `update(item.first, item.second)` for each element in `[beg, end)`.
     *  Each element must expose `.first` (the feature value) and `.second`
     *  (its weight as `double`), e.g. `std::pair<T, double>`.
     *
     *  @tparam Itor Iterator whose value type provides `.first` and `.second`.
     *  @param[in] beg Iterator to the first feature.
     *  @param[in] end Past-the-end iterator.
     *  @note Complexity: O(FINGERPRINT_SIZE * n) where n = distance(beg, end).
     */
    template <typename Itor>
    void update(Itor beg, const Itor & end)
    {
      while (beg != end)
        {
          update(beg->first, beg->second);
          ++beg;
        }
    }

    /** @brief Returns the fingerprint. */
    [[nodiscard]] uint64_t get_fingerprint() const noexcept
    {
      uint64_t fingerprint = 0;
      for (size_t i = 0; i < FINGERPRINT_SIZE; ++i)
        if (v_(i) > 0)
          fingerprint |= (static_cast<uint64_t>(1) << i);
      
      return fingerprint;
    }

    /** @brief Estimate similarity with another fingerprint.
     *  @return Estimated similarity in [0.0, 1.0] based on Hamming distance.
     */
    [[nodiscard]] static double similarity(const uint64_t f1, const uint64_t f2) noexcept
    {
      const int distance = std::popcount(f1 ^ f2);
      return 1.0 - (static_cast<double>(distance) / static_cast<double>(FINGERPRINT_SIZE));
    }

    /** @brief Reset the accumulator. */
    void clear()
    {
      for (size_t i = 0; i < FINGERPRINT_SIZE; ++i)
        v_(i) = 0.0;
    }
  };

} // namespace Aleph

# endif // SIMHASH_H
