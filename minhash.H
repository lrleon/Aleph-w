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

# ifndef MINHASH_H
# define MINHASH_H

/** @file minhash.H
 *  @brief MinHash for estimating Jaccard similarity between sets.
 *
 *  Provides the `MinHash` class for computing compact signatures of sets
 *  that can be used to efficiently estimate their Jaccard similarity index.
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */

# include <algorithm>
# include <limits>
# include <cstdint>

# include <ah-errors.H>
# include <tpl_array.H>
# include <hash-fct.H>

namespace Aleph
{
  /** @brief MinHash signature generator.
   *
   *  Maintain a signature of size `k` representing a set.
   *  The Jaccard similarity between two sets can be estimated by the
   *  fraction of positions where their MinHash signatures agree.
   *
   *  @tparam T Element type.
   */
  template <typename T>
  class MinHash
  {
    size_t k_; /**< Signature size. */
    Array<uint64_t> signature_; /**< Minimized hash values. */
    Array<uint32_t> seeds_; /**< Random seeds for hash functions. */

  public:
    /** @brief Construct with signature size.
     *  @param[in] k Number of hashes to use (higher k -> better precision).
     */
    explicit MinHash(size_t k = 128)
      : k_(k)
    {
      ah_domain_error_if(k == 0) << "MinHash: k must be > 0";

      signature_ = Array<uint64_t>(k_, std::numeric_limits<uint64_t>::max());
      seeds_.reserve(k_);

      // Use dft_hash_fct to generate independent seeds
      for (size_t i = 0; i < k_; ++i)
        seeds_.append(static_cast<uint32_t>(dft_hash_fct(i, 0xdeadbeef)));
    }

    /** @brief Add an element to the set.
     *  @note Complexity: O(k).
     */
    void update(const T & val)
    {
      for (size_t i = 0; i < k_; ++i)
        if (const auto h = static_cast<uint64_t>(murmur3hash(val, seeds_[i])); h < signature_[i])
          signature_(i) = h;
    }

    /** @brief Process multiple elements. */
    template <typename Itor>
    void update(Itor beg, const Itor & end)
    {
      while (beg != end)
        update(*beg++);
    }

    /** @brief Estimate Jaccard similarity with another signature.
     *  @return Estimated Jaccard index in [0.0, 1.0].
     *  @throws std::domain_error if signature sizes mismatch.
     */
    [[nodiscard]] double similarity(const MinHash & other) const
    {
      ah_domain_error_if(k_ != other.k_)
        << "MinHash::similarity: signature size mismatch";

      size_t matches = 0;
      for (size_t i = 0; i < k_; ++i)
        if (signature_[i] == other.signature_[i])
          ++matches;

      return static_cast<double>(matches) / static_cast<double>(k_);
    }

    /** @brief Returns the current signature. */
    [[nodiscard]] const Array<uint64_t> &get_signature() const noexcept
    {
      return signature_;
    }

    /** @brief Reset the signature. */
    void clear()
    {
      constexpr uint64_t inf = std::numeric_limits<uint64_t>::max();
      for (size_t i = 0; i < k_; ++i)
        signature_(i) = inf;
    }

    [[nodiscard]] size_t size() const noexcept { return k_; }
  };
} // namespace Aleph

# endif // MINHASH_H
