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
# include <iterator>
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
     *  @throws std::domain_error if k == 0.
     */
    explicit MinHash(size_t k = 128)
      : k_(k)
    {
      ah_domain_error_if(k == 0) << "MinHash: k must be > 0";
      
      signature_.reserve(k);
      for (size_t i = 0; i < k; ++i)
        signature_.append(std::numeric_limits<uint64_t>::max());
      seeds_.reserve(k_);

      // Use dft_hash_fct to generate independent seeds
      for (size_t i = 0; i < k_; ++i)
        seeds_.append(static_cast<uint32_t>(dft_hash_fct(i, 0xdeadbeef)));
    }

    /** @brief Add an element to the set.
     *  @param[in] val The element to incorporate into the signature.
     *  @throws none
     *  @note Complexity: O(k).
     */
    void update(const T & val)
    {
      for (size_t i = 0; i < k_; ++i)
        if (const auto h = static_cast<uint64_t>(murmur3hash(val, seeds_[i])); h < signature_[i])
          signature_(i) = h;
    }

    /** @brief Add all elements in a range to the MinHash signature.
     *
     *  Calls `update(val)` for each element in `[beg, end)`.
     *
     *  @tparam Itor Iterator whose value type is compatible with `update(const T&)`.
     *  @param[in] beg Iterator to the first element.
     *  @param[in] end Past-the-end iterator.
     *  @throws none
     *  @note Complexity: O(k * n) where n = distance(beg, end).
     */
    template <std::input_iterator Itor>
    void update(Itor beg, const Itor & end)
    {
      while (beg != end)
        {
          update(*beg);
          ++beg;
        }
    }

    /** @brief Estimate Jaccard similarity with another signature.
     *  @param[in] other The MinHash signature to compare against.
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

    /** @brief Returns the current signature.
     *  @return Const reference to the internal Array of k minimized hash values.
     *  @throws none
     *  @note Complexity: O(1).
     */
    [[nodiscard]] const Array<uint64_t> &get_signature() const noexcept
    {
      return signature_;
    }

    /** @brief Reset the signature to the initial all-maximum state.
     *  @throws none
     *  @note Complexity: O(k).
     */
    void clear()
    {
      constexpr uint64_t inf = std::numeric_limits<uint64_t>::max();
      for (size_t i = 0; i < k_; ++i)
        signature_(i) = inf;
    }

    /** @brief Merge another MinHash signature into this one.
     *
     *  Combines two MinHash signatures by taking the element-wise minimum,
     *  which corresponds to the union of the underlying sets.
     *
     *  @param[in] other Another MinHash with the same number of hashes.
     *  @return Reference to `*this` after the merge.
     *  @throws std::domain_error if signature sizes differ.
     */
    MinHash & merge(const MinHash & other)
    {
      ah_domain_error_if(k_ != other.k_)
        << "MinHash::merge: signature size mismatch ("
        << k_ << " vs " << other.k_ << ")";
      for (size_t i = 0; i < k_; ++i)
        signature_(i) = std::min(signature_(i), other.signature_(i));
      return *this;
    }

    /** @brief Number of hash functions used in the signature.
     *  @return size_t The value k passed at construction (signature size k_).
     *  @throws none
     *  @note Complexity: O(1).
     */
    [[nodiscard]] size_t size() const noexcept { return k_; }
  };
} // namespace Aleph

# endif // MINHASH_H
