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

# ifndef RESERVOIR_SAMPLING_H
# define RESERVOIR_SAMPLING_H

/** @file reservoir-sampling.H
 *  @brief Reservoir sampling for random selection from data streams.
 *
 *  Provides the `Reservoir_Sampler` class and convenience functions to
 *  randomly select `k` elements from a sequence of unknown length in a
 *  single pass (O(n) time, O(k) space).
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */

# include <chrono>
# include <cstdint>
# include <limits>
# include <memory>
# include <iterator>
# include <gsl/gsl_rng.h>

# include <ah-errors.H>
# include <tpl_array.H>

namespace Aleph
{

  /** @brief Incremental reservoir sampler (Algorithm R).
   *
   *  Maintain a representative random sample of size `k` from a stream
   *  of elements.
   *
   *  ## Thread-safety
   *  This class is **not thread-safe**. External synchronization is required
   *  for concurrent access from multiple threads.
   *
   *  @tparam T Element type.
   *  @note k is an unsigned size_t. If `k == 0`, no elements are stored and
   *        `get_sample()` returns an empty array.
   */
  template <typename T>
  class Reservoir_Sampler
  {
    using GslRngPtr = std::unique_ptr<gsl_rng, void(*)(gsl_rng *)>;

    size_t k_;             /**< Sample size. */
    size_t n_seen_ = 0;    /**< Total elements seen so far. */
    unsigned long seed_;   /**< Stored seed for RNG reset. */
    uint64_t rng_min_;     /**< Minimum value produced by RNG. */
    uint64_t rng_max_;     /**< Maximum value produced by RNG. */
    uint64_t rng_range_;   /**< Cached range of the RNG (max - min + 1). */
    Array<T> reservoir_;   /**< Current random sample. */
    GslRngPtr rng_;        /**< Random number generator. */

  public:
    /** @brief Initialize sampler.
     *
     *  @param[in] k    Size of the sample to maintain.
     *  @param[in] seed Optional seed for RNG (defaults to current time).
     *  @throws std::runtime_error if GSL RNG allocation fails (checked via 
     *          ah_runtime_error_unless).
     */
    explicit Reservoir_Sampler(const size_t k,
                               const unsigned long seed = static_cast<unsigned long>(
                                 std::chrono::high_resolution_clock::now().time_since_epoch().count()))
      : k_(k), seed_(seed),
        rng_(gsl_rng_alloc(gsl_rng_mt19937), gsl_rng_free)
    {
      ah_runtime_error_unless(rng_ != nullptr) << "Reservoir_Sampler: GSL RNG allocation failed";
      gsl_rng_set(rng_.get(), seed_);
      rng_min_ = static_cast<uint64_t>(gsl_rng_min(rng_.get()));
      rng_max_ = static_cast<uint64_t>(gsl_rng_max(rng_.get()));
      rng_range_ = rng_max_ - rng_min_ + 1ULL;
      if (k_ > 0)
        reservoir_.reserve(k_);
    }

    /** @brief Process a new element from the stream.
     *
     *  @param[in] val The element to potentially include in the sample.
     *  @note Complexity: O(1).
     *  @throws std::overflow_error if the stream length exceeds size_t or
     *          unsigned long capacity.
     *  @throws std::runtime_error if the stream length exceeds the range
     *          of the underlying random number generator.
     */
    void update(const T & val)
    {
      ah_overflow_error_if(n_seen_ == std::numeric_limits<size_t>::max())
        << "Reservoir_Sampler::update: stream length exceeded size_t capacity";

      if (k_ == 0)
        {
          ++n_seen_;
          return;
        }

      if (n_seen_ < k_)
        reservoir_.append(val);
      else
        {
          // Probability of replacement is k / (n_seen + 1)
          ah_runtime_error_unless(n_seen_ < rng_range_)
            << "Reservoir_Sampler::update: stream length exceeds RNG generator range (" << rng_range_ << ")";

          ah_overflow_error_if(n_seen_ >= std::numeric_limits<unsigned long>::max())
            << "Reservoir_Sampler::update: stream length exceeds unsigned long capacity";

          // Exclusive upper bound for the uniform random index in [0, n_seen_]
          const auto upper_bound = static_cast<unsigned long>(n_seen_ + 1);
          const size_t j = gsl_rng_uniform_int(rng_.get(), upper_bound);
          if (j < k_)
            reservoir_(j) = val;
        }
      ++n_seen_;
    }

    /** @brief Process multiple elements.
     *
     *  @tparam Itor Input iterator type.
     *  @tparam Sent Sentinel type for Itor.
     *  @param[in] beg Iterator to the first element of the range.
     *  @param[in] end Sentinel to the end of the range.
     *  @throws std::overflow_error if stream length exceeds safe limits.
     *  @throws std::runtime_error if RNG range is exceeded.
     */
    template <std::input_iterator Itor, std::sentinel_for<Itor> Sent>
    void update(Itor beg, Sent end)
    {
      while (beg != end)
        {
          update(*beg);
          ++beg;
        }
    }

    /** @brief Returns the current random sample. 
     *  @return const Array<T>& The current reservoir sample.
     *  @throws none
     */
    [[nodiscard]] const Array<T> & get_sample() const noexcept
    {
      return reservoir_;
    }

    /** @brief Total elements processed so far. 
     *  @return size_t Total number of elements processed.
     *  @throws none
     */
    [[nodiscard]] size_t total_seen() const noexcept { return n_seen_; }

    /** @brief Target sample size.
     *  @return size_t The target sample size (k_).
     *  @throws none
     */
    [[nodiscard]] size_t sample_size() const noexcept { return k_; }

    /** @brief RNG output range (max - min + 1).
     *  @return uint64_t The number of distinct values the RNG can produce.
     *  @throws none
     */
    [[nodiscard]] uint64_t rng_range() const noexcept { return rng_range_; }

    /** @brief Forcibly set the internal stream counter (for testing only).
     *
     *  Allows unit tests to exercise overflow and RNG-range-exhaustion paths
     *  without having to feed billions of elements.  Do not use in production.
     *  @param[in] n New value for the internal element counter.
     *  @throws none
     */
    void set_n_seen_for_testing(const size_t n) noexcept { n_seen_ = n; }

    /** @brief Reset the sampler while keeping k and seed.
     *
     *  Clears the reservoir, resets the element counter, and reseeds the
     *  RNG to its original state so the sampler is fully reproducible.
     *  @throws none
     */
    void clear()
    {
      reservoir_.clear();
      n_seen_ = 0;
      gsl_rng_set(rng_.get(), seed_);
    }
  };


  /** @brief Convenience function to sample k elements from a range.
   *
   *  @tparam Itor Input iterator type.
   *  @tparam Sent Sentinel type for Itor.
   *  @param[in] beg  Start of the range.
   *  @param[in] end  End of the range.
   *  @param[in] k    Number of elements to sample.
   *  @param[in] seed Random seed.
   *  @return Array of the iterator's value_type (derived from std::iterator_traits<Itor>::value_type) 
   *          containing the sampled elements.
   *  @throws std::runtime_error if GSL RNG allocation fails.
   *  @throws std::overflow_error if stream length exceeds safe limits.
   *
   *  @note Complexity: O(n) time, O(k) space.
   *  @ingroup Algorithms
   */
  template <std::input_iterator Itor, std::sentinel_for<Itor> Sent>
  [[nodiscard]] inline auto
  reservoir_sample(Itor beg, Sent end, size_t k,
                   unsigned long seed = static_cast<unsigned long>(
                     std::chrono::high_resolution_clock::now().time_since_epoch().count()))
  {
    using T = typename std::iterator_traits<Itor>::value_type;
    Reservoir_Sampler<T> sampler(k, seed);
    sampler.update(beg, end);
    return Array<T>(sampler.get_sample());
  }

} // namespace Aleph

# endif // RESERVOIR_SAMPLING_H
