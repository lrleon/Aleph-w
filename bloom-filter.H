/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
# ifndef BLOOM_FILTER_H
# define BLOOM_FILTER_H

# include <cassert>
# include <cstddef>
# include <memory>
# include <tuple>
# include <vector>
# include <limits>
# include <ctime>
# include <utility>
# include <gsl/gsl_rng.h>
# include <cmath>
# include <iostream>
# include <ahFunctional.H>
# include <bitArray.H>
# include <hash-fct.H>
# include <tpl_dynSetHash.H>
# include <ah-errors.H>

namespace Aleph
{
  template <typename T>
  /**
      Bloom filter implementation.

      A Bloom filter is a probabilistic set data structure that can test
      membership with no false negatives and a tunable false positive rate.

      This implementation:
      - Stores bits in a BitArray.
      - Generates `k` per-hash seeds from a PRNG (GSL mt19937).
      - Uses a user-provided hash function `(item, seed) -> size_t`.

      @tparam T Item type.
   */
  class Bloom_Filter
  {
  public:
    using Hash_Fct = size_t (*)(const T &, unsigned long seed);

  private:
    BitArray bits;

    Hash_Fct hash_fct = nullptr;
    size_t num_hash = 0;
    std::vector<unsigned long> seeds;
    size_t num_ins = 0;

    bool have_same_hashes(const Bloom_Filter & f) const noexcept
    {
      return get_m() == f.get_m() and get_k() == f.get_k() and
             hash_fct == f.hash_fct and seeds == f.seeds;
    }

  public:
    /**
        Estimate the parameters of a Bloom filter.

        Given an expected number of items `n` and a target false positive
        probability `p`, returns a tuple `(m, k)` where:

        - `m` is the number of bits of the filter.
        - `k` is the number of hash functions.

        @param[in] n Expected number of inserted items. Must be > 0.
        @param[in] p Desired false positive probability. Must satisfy 0 < p < 1.
        @return `(m, k)` pair.
     */
    static std::tuple<size_t, size_t> estimate(const size_t n, const double p)
    {
      assert(n > 0);
      assert(p > 0 and p < 1);
      const double log2 = std::log(2);
      const auto m = static_cast<size_t>(std::ceil(-(n * std::log(p)) / (log2 * log2)));
      const auto k = static_cast<size_t>(std::ceil((1.0 * m / n) * log2));
      return std::make_tuple(m, k);
    }

    /**
        Return the number of bits of the filter.

        This is the Bloom filter capacity in bits ("m" in the literature).
     */
    size_t get_m() const noexcept { return bits.size(); }

    /**
        Return the number of hash functions used by the filter.

        This is "k" in the literature.
     */
    size_t get_k() const noexcept { return num_hash; }

    /**
        Return the number of insertions performed on the filter.

        Note: this is a counter of insertions, not the number of distinct items.
     */
    size_t get_n() const noexcept { return num_ins; }

    /**
        Return the number of bits set to 1.

        This is commonly denoted as "x" in Bloom filter formulas.
     */
    size_t get_x() const noexcept
    {
      size_t x = 0;
      for (size_t i = 0; i < bits.size(); ++i)
        if (bits(i) == 1)
          ++x;
      return x;
    }

    /**
        @brief Return the number of insertions.

        Alias of get_n().

        @return Number of insertions.
     */
    size_t size() const noexcept { return get_n(); }

    /**
        @brief Return the capacity in bits.

        Alias of get_m().

        @return Number of bits.
     */
    size_t capacity() const noexcept { return get_m(); }

    /**
        Construct a Bloom filter with explicit parameters.

        @param[in] dim Number of bits of the filter.
        @param[in] __num_hash Number of hash functions.
        @param[in] __hash_fct Hash function of signature `(item, seed) -> size_t`.
        @param[in] seed Seed used to generate internal per-hash seeds.

        @throw std::invalid_argument if `dim == 0`, `__num_hash == 0` or
        `__hash_fct == nullptr`.
        @throw std::bad_alloc if the internal random generator cannot be allocated.
    */
    Bloom_Filter(const size_t dim, size_t __num_hash,
                 Hash_Fct __hash_fct = dft_hash_fct,
                 const unsigned long seed = static_cast<unsigned long>(std::time(nullptr)))
      : bits(0), hash_fct(__hash_fct), num_hash(__num_hash),
        seeds(__num_hash)
    {
      ah_invalid_argument_if(dim == 0) << "Bloom filter dimension must be > 0";
      ah_invalid_argument_if(num_hash == 0) << "Bloom filter number of hashes must be > 0";
      ah_invalid_argument_if(hash_fct == nullptr) << "Bloom filter hash function is null";

      bits.reserve(dim);
      const std::unique_ptr<gsl_rng, void(*)(gsl_rng *)>
          r(gsl_rng_alloc(gsl_rng_mt19937), gsl_rng_free);
      ah_bad_alloc_if(r.get() == nullptr);
      gsl_rng_set(r.get(), seed % gsl_rng_max(r.get()));

      for (auto & s: seeds)
        s = gsl_rng_get(r.get());
    }

    /**
        Construct a Bloom filter using a desired capacity in insertions.

        This constructor derives `(m, k)` from `(n, p)` using estimate().

        @param[in] n Expected number of inserted items.
        @param[in] p Desired false positive probability.
        @param[in] seed Seed used to generate internal per-hash seeds.
        @param[in] __hash_fct Hash function of signature `(item, seed) -> size_t`.
    */
    Bloom_Filter(const size_t n, const double p,
                 unsigned long seed = static_cast<unsigned long>(std::time(nullptr)),
                 Hash_Fct __hash_fct = dft_hash_fct)
      : Bloom_Filter(std::get<0>(estimate(n, p)), std::get<1>(estimate(n, p)),
                     __hash_fct, seed)
    {
      // empty
    }

    /**
        Swap `this` with `f` in O(1).

        After swap, both instances remain valid.
     */
    void swap(Bloom_Filter & f) noexcept
    {
      bits.swap(f.bits);
      std::swap(hash_fct, f.hash_fct);
      std::swap(num_hash, f.num_hash);
      seeds.swap(f.seeds);
      std::swap(num_ins, f.num_ins);
    }

    /**
        @brief Copy constructor.

        Copies all parameters (`m`, `k`, hash function, seeds) and the current
        bitset.
     */
    Bloom_Filter(const Bloom_Filter & f)
      : bits(f.bits), hash_fct(f.hash_fct), num_hash(f.num_hash),
        seeds(f.seeds), num_ins(f.num_ins)
    {
      // empty
    }

    /**
        @brief Move constructor.

        After the move, the moved-from filter remains valid.
     */
    Bloom_Filter(Bloom_Filter && f) noexcept
    {
      swap(f);
    }

    /**
        Copy assignment.

        Provides strong exception safety via copy-and-swap.
     */
    auto &operator =(const Bloom_Filter & f)
    {
      Bloom_Filter tmp(f);
      swap(tmp);
      return *this;
    }

    /**
        @brief Move assignment.

        @return `*this`.
     */
    auto &operator =(Bloom_Filter && f) noexcept
    {
      swap(f);
      return *this;
    }

    /** @brief Destructor. */
    virtual ~Bloom_Filter() = default;

    /**
        Insert an item.

        This operation never produces false negatives: an inserted item
        will always be reported as present by contains().
    */
    auto &insert(const T & item) noexcept
    {
      const size_t & m = bits.size();
      for (const auto & seed: seeds)
        bits.fast_write((*hash_fct)(item, seed) % m, 1);

      ++num_ins;

      return *this;
    }

    /**
        @brief Alias of insert().

        @param[in] item Item to insert.
        @return `*this`.
     */
    auto &append(const T & item) noexcept { return insert(item); }

    /**
        Test membership.

        This operation may return false positives but never false negatives.
    */
    bool contains(const T & item) const noexcept
    {
      const size_t & m = bits.size();
      for (const auto & seed: seeds)
        if (not bits.fast_read((*hash_fct)(item, seed) % m))
          return false;

      return true;
    }

    /**
        Return the internally generated per-hash seeds.

        The returned list has size `get_k()`.
     */
    DynList<size_t> hash_seeds() const
    {
      DynList<size_t> ret;
      for (const auto & seed: seeds)
        ret.append(seed);
      return ret;
    }

    /**
        Return the bit positions used by `item`.

        @return A list of size `get_k()`, with values in `[0, get_m())`.
     */
    DynList<size_t> hashes(const T & item) const
    {
      DynList<size_t> ret;
      const size_t & m = bits.size();
      for (const auto & seed: seeds)
        ret.append((*hash_fct)(item, seed) % m);
      return ret;
    }

    /**
        @brief Return the intersection of hashes(i1) and hashes(i2).

        @param[in] i1 First item.
        @param[in] i2 Second item.
        @return List of bit positions common to both items.
     */
    DynList<size_t> common_hashes(const T & i1, const T & i2) const
    {
      return intercept(hashes(i1), hashes(i2));
    }

    /**
        Return the indexes of bits set to 1.

        This is an O(m) operation.
     */
    DynList<size_t> set_bits() const
    {
      DynList<size_t> ret;
      for (size_t i = 0; i < bits.size(); ++i)
        if (bits(i) == 1)
          ret.append(i);

      return ret;
    }

    /**
     * @brief Estimate the number of inserted items from the number of bits set.
     *
     * Uses the standard Bloom filter relation:
     * `x = m * (1 - exp(-k*n/m))` and solves for `n`.
     *
     * @param[in] x Number of bits set to 1.
     * @return Estimated insertion count.
     */
    size_t expected_size(const size_t x) const noexcept
    {
      const auto m = static_cast<double>(capacity());
      const auto k = static_cast<double>(get_k());
      if (m <= 0 or k <= 0)
        return 0;

      const double p0 = 1.0 - (static_cast<double>(x) / m);
      if (p0 <= 0)
        return std::numeric_limits<size_t>::max();

      const double n = std::ceil(-(m * std::log(p0)) / k);
      if (not std::isfinite(n) or n < 0)
        return std::numeric_limits<size_t>::max();

      return static_cast<size_t>(n);
    }

    /**
        @brief Convenience overload.

        Equivalent to `expected_size(get_x())`.
     */
    size_t expected_size() const noexcept
    {
      return expected_size(get_x());
    }

    /**
        In-place union.

        Requires both filters to be compatible (same seeds/size/k).
        Updates `num_ins` using expected_size(get_x()).
     */
    auto &operator |=(const Bloom_Filter & f)
    {
      ah_domain_error_if(not have_same_hashes(f))
        << "Bloom filter have different hashes";

      bits |= f.bits;

      const auto x = get_x();

      num_ins = expected_size(x);

      return *this;
    }

    /**
        In-place intersection.

        Requires both filters to be compatible (same seeds/size/k).
     */
    auto &operator &=(const Bloom_Filter & f)
    {
      ah_domain_error_if(not have_same_hashes(f))
        << "Bloom filter have different hashes";

      bits &= f.bits;

      const auto est = expected_size();
      const auto sum = num_ins + f.num_ins;
      num_ins = sum >= est ? sum - est : 0;

      return *this;
    }

    /** Return the union of two compatible Bloom filters. */
    friend Bloom_Filter
    operator |(const Bloom_Filter & f1, const Bloom_Filter & f2)
    {
      Bloom_Filter ret = f1;
      ret |= f2;
      return ret;
    }

    /** Return the intersection of two compatible Bloom filters. */
    friend Bloom_Filter
    operator &(const Bloom_Filter & f1, const Bloom_Filter & f2)
    {
      Bloom_Filter ret = f1;
      ret &= f2;
      return ret;
    }
  };
} // end namespace Aleph

# endif // BLOOM_FILTER_H
