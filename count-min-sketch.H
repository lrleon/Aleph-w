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

# ifndef COUNT_MIN_SKETCH_H
# define COUNT_MIN_SKETCH_H

/** @file count-min-sketch.H
 *  @brief Count-Min Sketch for frequency estimation in streams.
 *
 *  Provides the `Count_Min_Sketch` class for approximate counting of 
 *  events in a stream with constant memory and sub-linear error bounds.
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */

# include <cmath>
# include <algorithm>
# include <concepts>
# include <vector>

# include <ah-errors.H>
# include <tpl_array.H>
# include <hash-fct.H>

namespace Aleph
{

  /** @brief Count-Min Sketch frequency estimator.
   *
   *  Provides approximate frequency queries for elements in a stream.
   *  The estimate is always an upper bound (no false negatives).
   *
   *  Memory complexity is O(w * d), where:
   *  - w = width (related to error magnitude)
   *  - d = depth (related to confidence)
   *
   *  @tparam T        Type of elements to count.
   *  @tparam CounterT Integral type for counters (default: size_t).
   */
  template <typename T, std::integral CounterT = size_t>
  class Count_Min_Sketch
  {
    size_t w_;             /**< Width of the sketch. */
    size_t d_;             /**< Depth (number of hash functions). */
    Array<CounterT> table_; /**< Flat array for counters [d_ x w_]. */
    Array<uint32_t> seeds_; /**< Random seeds for hash functions. */
    CounterT total_count_ = 0; /**< Total updates processed. */

  public:
    /** @brief Construct with explicit dimensions.
     *  @param[in] w Width (columns).
     *  @param[in] d Depth (rows / hash functions).
     */
    Count_Min_Sketch(const size_t w, const size_t d)
      : w_(w), d_(d)
    {
      ah_domain_error_if(w == 0 or d == 0) 
        << "Count_Min_Sketch: dimensions must be positive";
      
      table_ = Array<CounterT>(w_ * d_, CounterT(0));
      seeds_.reserve(d_);
      
      for (size_t i = 0; i < d_; ++i)
        seeds_.append(static_cast<uint32_t>(dft_hash_fct(i, 0x9e3779b9)));
    }

    /** @brief Create a sketch with target error bounds.
     *
     *  Calculates optimal w and d for given epsilon and delta.
     *  Estimate will be within epsilon * total_count from actual with
     *  probability at least 1 - delta.
     *
     *  @param[in] epsilon Relative error bound (must be > 0).
     *  @param[in] delta   Confidence bound (error probability, must be in (0, 1)).
     *  @return A new Count_Min_Sketch instance.
     */
    static Count_Min_Sketch from_error_bounds(const double epsilon, const double delta)
    {
      ah_domain_error_if(epsilon <= 0.0) 
        << "Count_Min_Sketch::from_error_bounds: epsilon must be > 0 (got " << epsilon << ")";
      ah_domain_error_if(delta <= 0.0 || delta >= 1.0) 
        << "Count_Min_Sketch::from_error_bounds: delta must be in (0, 1) (got " << delta << ")";

      return Count_Min_Sketch(
          static_cast<size_t>(std::ceil(std::exp(1.0) / epsilon)),
          static_cast<size_t>(std::ceil(std::log(1.0 / delta))));
    }

    /** @brief Increment count for an element.
     *  @param[in] val   Element to update.
     *  @param[in] count Increment value (defaults to 1).
     */
    void update(const T & val, CounterT count = 1)
    {
      for (size_t i = 0; i < d_; ++i)
        {
          const size_t h = murmur3hash(val, seeds_[i]) % w_;
          table_(i * w_ + h) += count;
        }
      total_count_ += count;
    }

    /** @brief Estimate frequency of an element.
     *  @return Approximate frequency (always >= actual).
     */
    [[nodiscard]] CounterT estimate(const T & val) const
    {
      CounterT min_val = std::numeric_limits<CounterT>::max();
      for (size_t i = 0; i < d_; ++i)
        {
          const size_t h = murmur3hash(val, seeds_[i]) % w_;
          min_val = std::min(min_val, table_[i * w_ + h]);
        }
      return min_val;
    }

    /** @brief Merge another sketch into this one.
     *  @throws std::domain_error if dimensions or seeds mismatch.
     */
    void merge(const Count_Min_Sketch & other)
    {
      ah_domain_error_if(w_ != other.w_ or d_ != other.d_)
        << "Count_Min_Sketch::merge: dimension mismatch";
      
      ah_domain_error_if(seeds_ != other.seeds_)
        << "Count_Min_Sketch::merge: hash function seeds mismatch";
      
      for (size_t i = 0; i < table_.size(); ++i)
        table_(i) += other.table_[i];
      
      total_count_ += other.total_count_;
    }

    /** @brief Clear all counters. */
    void clear()
    {
      for (size_t i = 0; i < table_.size(); ++i)
        table_(i) = 0;
      total_count_ = 0;
    }

    [[nodiscard]] size_t width() const noexcept { return w_; }
    [[nodiscard]] size_t depth() const noexcept { return d_; }
    [[nodiscard]] CounterT total_count() const noexcept { return total_count_; }
  };

} // namespace Aleph

# endif // COUNT_MIN_SKETCH_H
