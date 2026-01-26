
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


/**
 * @file stat_utils.H
 * @brief Comprehensive statistical utilities for numeric data
 *
 * This header provides a complete set of descriptive statistics functions
 * for analyzing numeric data. It includes basic measures (mean, median,
 * variance) as well as advanced statistics (skewness, kurtosis, percentiles).
 *
 * ## Features
 *
 * ### Basic Statistics
 * - **Mean (average)**: Arithmetic mean of values
 * - **Variance**: Sample or population variance
 * - **Standard deviation**: Square root of variance
 * - **Median**: Middle value of sorted data
 * - **Min/Max**: Extreme values
 * - **Sum**: Total of all values
 *
 * ### Advanced Statistics
 * - **Percentiles**: Arbitrary percentile (0-100)
 * - **Quartiles**: Q1, Q2 (median), Q3
 * - **Interquartile Range (IQR)**: Q3 - Q1
 * - **Mode**: Most frequent value
 * - **Coefficient of Variation**: stddev/mean
 * - **Skewness**: Measure of distribution asymmetry
 * - **Kurtosis**: Measure of distribution tailedness
 *
 * ### Data Analysis
 * - **Histogram**: Frequency distribution
 * - **Correlation**: Pearson correlation coefficient
 * - **Covariance**: Measure of joint variability
 *
 * ## Algorithms
 *
 * - Uses Welford's algorithm for numerically stable variance computation
 * - Quicksort for median/percentile calculations
 * - Single-pass algorithms where possible
 *
 * ## Usage Example
 *
 * @code{.cpp}
 * #include <stat_utils.H>
 * #include <iostream>
 *
 * std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
 *
 * // Using Stats struct
 * auto stats = compute_all_stats(data);
 * std::cout << "Mean: " << stats.mean << std::endl;
 * std::cout << "Median: " << stats.median << std::endl;
 * std::cout << "Std Dev: " << stats.stddev << std::endl;
 *
 * // Individual functions
 * double m = mean(data);
 * double p90 = percentile(data, 90);
 * double corr = correlation(data1, data2);
 * @endcode
 *
 * @ingroup Utilities
 * @author Leandro Rabindranath Leon
 */

#ifndef STAT_UTILS_H
#define STAT_UTILS_H

#include <cmath>
#include <limits>
#include <algorithm>
#include <vector>
#include <map>
#include <stdexcept>
#include <tpl_sort_utils.H>
#include <tpl_dynArray.H>

namespace Aleph {

// =============================================================================
// Stats Result Structure
// =============================================================================

/**
 * @brief Container for comprehensive statistical results.
 *
 * Holds all computed statistics from a dataset analysis.
 * Use with compute_all_stats() for complete analysis.
 *
 * @tparam T Numeric type (float, double, etc.)
 */
template <typename T>
struct Stats
{
  size_t count = 0;        ///< Number of elements
  T sum = T();             ///< Sum of all values
  T mean = T();            ///< Arithmetic mean
  T variance = T();        ///< Sample variance
  T stddev = T();          ///< Standard deviation
  T median = T();          ///< Median (50th percentile)
  T min = T();             ///< Minimum value
  T max = T();             ///< Maximum value
  T q1 = T();              ///< First quartile (25th percentile)
  T q3 = T();              ///< Third quartile (75th percentile)
  T iqr = T();             ///< Interquartile range (Q3 - Q1)
  T skewness = T();        ///< Skewness (asymmetry)
  T kurtosis = T();        ///< Excess kurtosis (tailedness)
  T coef_variation = T();  ///< Coefficient of variation (stddev/mean)

  /**
   * @brief Check if statistics are valid.
   * @return true if count > 0
   */
  [[nodiscard]] bool is_valid() const noexcept { return count > 0; }

  /**
   * @brief Get the range (max - min).
   * @return Range of values
   */
  [[nodiscard]] T range() const noexcept { return max - min; }
};

// =============================================================================
// Basic Statistical Functions
// =============================================================================

/**
 * @brief Compute the sum of elements.
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container
 * @return Sum of all elements
 */
template <typename Container>
[[nodiscard]] auto sum(const Container & data)
  -> std::decay_t<decltype(*std::begin(data))>
{
  using T = std::decay_t<decltype(*std::begin(data))>;
  T result = T();
  for (const auto & x : data)
    result += x;
  return result;
}

/**
 * @brief Compute the arithmetic mean.
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container
 * @return Arithmetic mean
 * @throws std::invalid_argument if container is empty
 */
template <typename Container>
[[nodiscard]] auto mean(const Container & data)
  -> std::decay_t<decltype(*std::begin(data))>
{
  using T = std::decay_t<decltype(*std::begin(data))>;
  size_t n = 0;
  T s = T();
  for (const auto & x : data)
    {
      s += x;
      ++n;
    }
  if (n == 0)
    throw std::invalid_argument("mean: empty container");
  return s / static_cast<T>(n);
}

/**
 * @brief Compute variance using Welford's numerically stable algorithm.
 *
 * This algorithm avoids numerical instability that can occur with the
 * traditional formula when dealing with large values or values close
 * to each other.
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container
 * @param population If true, compute population variance (divide by n);
 *                   if false (default), compute sample variance (divide by n-1)
 * @return Variance
 * @throws std::invalid_argument if container has fewer than 2 elements (sample)
 *         or is empty (population)
 */
template <typename Container>
[[nodiscard]] auto variance(const Container & data, bool population = false)
  -> std::decay_t<decltype(*std::begin(data))>
{
  using T = std::decay_t<decltype(*std::begin(data))>;

  size_t n = 0;
  T m = T();      // Running mean
  T m2 = T();     // Sum of squared differences from mean

  // Welford's online algorithm
  for (const auto & x : data)
    {
      ++n;
      T delta = x - m;
      m += delta / static_cast<T>(n);
      T delta2 = x - m;
      m2 += delta * delta2;
    }

  if (population)
    {
      if (n == 0)
        throw std::invalid_argument("variance: empty container");
      return m2 / static_cast<T>(n);
    }
  else
    {
      if (n < 2)
        throw std::invalid_argument("variance: need at least 2 elements for sample variance");
      return m2 / static_cast<T>(n - 1);
    }
}

/**
 * @brief Compute standard deviation.
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container
 * @param population If true, compute population stddev; if false (default), sample stddev
 * @return Standard deviation
 */
template <typename Container>
[[nodiscard]] auto stddev(const Container & data, bool population = false)
  -> std::decay_t<decltype(*std::begin(data))>
{
  return std::sqrt(variance(data, population));
}

/**
 * @brief Compute minimum value.
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container
 * @return Minimum value
 * @throws std::invalid_argument if container is empty
 */
template <typename Container>
[[nodiscard]] auto min_value(const Container & data)
  -> std::decay_t<decltype(*std::begin(data))>
{
  auto it = std::begin(data);
  auto end = std::end(data);
  if (it == end)
    throw std::invalid_argument("min_value: empty container");

  auto result = *it;
  for (++it; it != end; ++it)
    if (*it < result)
      result = *it;
  return result;
}

/**
 * @brief Compute maximum value.
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container
 * @return Maximum value
 * @throws std::invalid_argument if container is empty
 */
template <typename Container>
[[nodiscard]] auto max_value(const Container & data)
  -> std::decay_t<decltype(*std::begin(data))>
{
  auto it = std::begin(data);
  auto end = std::end(data);
  if (it == end)
    throw std::invalid_argument("max_value: empty container");

  auto result = *it;
  for (++it; it != end; ++it)
    if (*it > result)
      result = *it;
  return result;
}

/**
 * @brief Compute minimum and maximum values in one pass.
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container
 * @return Pair of (min, max)
 * @throws std::invalid_argument if container is empty
 */
template <typename Container>
[[nodiscard]] auto min_max(const Container & data)
  -> std::pair<std::decay_t<decltype(*std::begin(data))>,
               std::decay_t<decltype(*std::begin(data))>>
{
  using T = std::decay_t<decltype(*std::begin(data))>;
  auto it = std::begin(data);
  auto end = std::end(data);
  if (it == end)
    throw std::invalid_argument("min_max: empty container");

  T min_val = *it;
  T max_val = *it;
  for (++it; it != end; ++it)
    {
      if (*it < min_val)
        min_val = *it;
      if (*it > max_val)
        max_val = *it;
    }
  return {min_val, max_val};
}

// =============================================================================
// Percentile and Median Functions
// =============================================================================

/**
 * @brief Compute a percentile value.
 *
 * Uses linear interpolation between data points.
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container (will be copied and sorted)
 * @param p Percentile (0-100)
 * @return Percentile value
 * @throws std::invalid_argument if container is empty or p is out of range
 */
template <typename Container>
[[nodiscard]] auto percentile(const Container & data, double p)
  -> std::decay_t<decltype(*std::begin(data))>
{
  using T = std::decay_t<decltype(*std::begin(data))>;

  if (p < 0 or p > 100)
    throw std::invalid_argument("percentile: p must be in [0, 100]");

  // Copy to vector for sorting
  std::vector<T> sorted(std::begin(data), std::end(data));
  if (sorted.empty())
    throw std::invalid_argument("percentile: empty container");

  std::sort(sorted.begin(), sorted.end());

  if (p == 0)
    return sorted.front();
  if (p == 100)
    return sorted.back();

  // Calculate index
  double index = (p / 100.0) * (sorted.size() - 1);
  size_t lower = static_cast<size_t>(std::floor(index));
  size_t upper = static_cast<size_t>(std::ceil(index));

  if (lower == upper)
    return sorted[lower];

  // Linear interpolation
  double fraction = index - lower;
  return static_cast<T>(sorted[lower] * (1 - fraction) + sorted[upper] * fraction);
}

/**
 * @brief Compute the median (50th percentile).
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container (will be copied and sorted)
 * @return Median value
 * @throws std::invalid_argument if container is empty
 */
template <typename Container>
[[nodiscard]] auto median(const Container & data)
  -> std::decay_t<decltype(*std::begin(data))>
{
  return percentile(data, 50);
}

/**
 * @brief Compute quartiles (Q1, Q2, Q3).
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container
 * @return Tuple of (Q1, Q2/median, Q3)
 */
template <typename Container>
[[nodiscard]] auto quartiles(const Container & data)
  -> std::tuple<std::decay_t<decltype(*std::begin(data))>,
                std::decay_t<decltype(*std::begin(data))>,
                std::decay_t<decltype(*std::begin(data))>>
{
  using T = std::decay_t<decltype(*std::begin(data))>;
  T q1 = percentile(data, 25);
  T q2 = percentile(data, 50);
  T q3 = percentile(data, 75);
  return {q1, q2, q3};
}

/**
 * @brief Compute the interquartile range (IQR = Q3 - Q1).
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container
 * @return Interquartile range
 */
template <typename Container>
[[nodiscard]] auto iqr(const Container & data)
  -> std::decay_t<decltype(*std::begin(data))>
{
  return percentile(data, 75) - percentile(data, 25);
}

// =============================================================================
// Mode
// =============================================================================

/**
 * @brief Compute the mode (most frequent value).
 *
 * For continuous data, consider discretizing first.
 * If multiple values have the same frequency, returns the first one encountered.
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container
 * @return Mode value
 * @throws std::invalid_argument if container is empty
 */
template <typename Container>
[[nodiscard]] auto mode(const Container & data)
  -> std::decay_t<decltype(*std::begin(data))>
{
  using T = std::decay_t<decltype(*std::begin(data))>;

  auto it = std::begin(data);
  auto end = std::end(data);
  if (it == end)
    throw std::invalid_argument("mode: empty container");

  std::map<T, size_t> freq;
  for (const auto & x : data)
    ++freq[x];

  T mode_val = freq.begin()->first;
  size_t max_count = freq.begin()->second;

  for (const auto & [val, count] : freq)
    {
      if (count > max_count)
        {
          max_count = count;
          mode_val = val;
        }
    }

  return mode_val;
}

/**
 * @brief Check if data is multimodal.
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container
 * @return true if multiple values share the maximum frequency
 */
template <typename Container>
[[nodiscard]] bool is_multimodal(const Container & data)
{
  using T = std::decay_t<decltype(*std::begin(data))>;

  std::map<T, size_t> freq;
  for (const auto & x : data)
    ++freq[x];

  if (freq.empty())
    return false;

  size_t max_count = 0;
  for (const auto & [val, count] : freq)
    if (count > max_count)
      max_count = count;

  size_t modes = 0;
  for (const auto & [val, count] : freq)
    if (count == max_count)
      ++modes;

  return modes > 1;
}

// =============================================================================
// Higher Moments
// =============================================================================

/**
 * @brief Compute skewness (measure of asymmetry).
 *
 * Positive skewness: right tail is longer
 * Negative skewness: left tail is longer
 * Zero skewness: symmetric distribution
 *
 * Uses the adjusted Fisher-Pearson standardized moment coefficient.
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container
 * @return Skewness value
 * @throws std::invalid_argument if container has fewer than 3 elements
 */
template <typename Container>
[[nodiscard]] auto skewness(const Container & data)
  -> std::decay_t<decltype(*std::begin(data))>
{
  using T = std::decay_t<decltype(*std::begin(data))>;

  T m = mean(data);
  T s = stddev(data);

  if (s == T())
    return T();  // All values are equal

  size_t n = 0;
  T sum3 = T();
  for (const auto & x : data)
    {
      T diff = (x - m) / s;
      sum3 += diff * diff * diff;
      ++n;
    }

  if (n < 3)
    throw std::invalid_argument("skewness: need at least 3 elements");

  // Adjusted Fisher-Pearson coefficient
  T factor = static_cast<T>(n) / ((n - 1) * (n - 2));
  return factor * sum3;
}

/**
 * @brief Compute excess kurtosis (measure of tailedness).
 *
 * Positive kurtosis: heavy tails (leptokurtic)
 * Negative kurtosis: light tails (platykurtic)
 * Zero kurtosis: normal distribution (mesokurtic)
 *
 * Returns excess kurtosis (kurtosis - 3).
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container
 * @return Excess kurtosis value
 * @throws std::invalid_argument if container has fewer than 4 elements
 */
template <typename Container>
[[nodiscard]] auto kurtosis(const Container & data)
  -> std::decay_t<decltype(*std::begin(data))>
{
  using T = std::decay_t<decltype(*std::begin(data))>;

  T m = mean(data);
  T s = stddev(data);

  if (s == T())
    return T();  // All values are equal

  size_t n = 0;
  T sum4 = T();
  for (const auto & x : data)
    {
      T diff = (x - m) / s;
      T diff2 = diff * diff;
      sum4 += diff2 * diff2;
      ++n;
    }

  if (n < 4)
    throw std::invalid_argument("kurtosis: need at least 4 elements");

  // Excess kurtosis with bias correction
  T n_t = static_cast<T>(n);
  T factor1 = (n_t * (n_t + 1)) / ((n_t - 1) * (n_t - 2) * (n_t - 3));
  T factor2 = (3 * (n_t - 1) * (n_t - 1)) / ((n_t - 2) * (n_t - 3));

  return factor1 * sum4 - factor2;
}

/**
 * @brief Compute coefficient of variation (CV = stddev / mean).
 *
 * Useful for comparing variability of datasets with different means.
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container
 * @return Coefficient of variation
 * @throws std::invalid_argument if mean is zero
 */
template <typename Container>
[[nodiscard]] auto coefficient_of_variation(const Container & data)
  -> std::decay_t<decltype(*std::begin(data))>
{
  using T = std::decay_t<decltype(*std::begin(data))>;
  T m = mean(data);
  if (m == T())
    throw std::invalid_argument("coefficient_of_variation: mean is zero");
  return stddev(data) / std::abs(m);
}

// =============================================================================
// Correlation and Covariance
// =============================================================================

/**
 * @brief Compute covariance between two datasets.
 *
 * @tparam Container1 First container type
 * @tparam Container2 Second container type
 * @param x First dataset
 * @param y Second dataset
 * @param population If true, population covariance; if false (default), sample covariance
 * @return Covariance
 * @throws std::invalid_argument if containers have different sizes or fewer than 2 elements
 */
template <typename Container1, typename Container2>
[[nodiscard]] auto covariance(const Container1 & x, const Container2 & y,
                               bool population = false)
  -> std::decay_t<decltype(*std::begin(x))>
{
  using T = std::decay_t<decltype(*std::begin(x))>;

  auto it_x = std::begin(x);
  auto it_y = std::begin(y);
  auto end_x = std::end(x);
  auto end_y = std::end(y);

  // Welford-style online algorithm for covariance
  size_t n = 0;
  T mean_x = T();
  T mean_y = T();
  T c = T();  // Co-moment

  while (it_x != end_x and it_y != end_y)
    {
      ++n;
      T dx = *it_x - mean_x;
      mean_x += dx / static_cast<T>(n);
      T dy = *it_y - mean_y;
      mean_y += dy / static_cast<T>(n);
      c += dx * (*it_y - mean_y);

      ++it_x;
      ++it_y;
    }

  if (it_x != end_x or it_y != end_y)
    throw std::invalid_argument("covariance: containers have different sizes");

  if (population)
    {
      if (n == 0)
        throw std::invalid_argument("covariance: empty containers");
      return c / static_cast<T>(n);
    }
  else
    {
      if (n < 2)
        throw std::invalid_argument("covariance: need at least 2 elements");
      return c / static_cast<T>(n - 1);
    }
}

/**
 * @brief Compute Pearson correlation coefficient.
 *
 * Returns a value in [-1, 1]:
 * - +1: Perfect positive correlation
 * - 0: No correlation
 * - -1: Perfect negative correlation
 *
 * @tparam Container1 First container type
 * @tparam Container2 Second container type
 * @param x First dataset
 * @param y Second dataset
 * @return Correlation coefficient
 * @throws std::invalid_argument if containers have different sizes or constant values
 */
template <typename Container1, typename Container2>
[[nodiscard]] auto correlation(const Container1 & x, const Container2 & y)
  -> std::decay_t<decltype(*std::begin(x))>
{
  using T = std::decay_t<decltype(*std::begin(x))>;

  T cov = covariance(x, y, true);  // Use population covariance
  T sx = stddev(x, true);          // Population stddev
  T sy = stddev(y, true);

  if (sx == T() or sy == T())
    throw std::invalid_argument("correlation: one or both datasets have zero variance");

  return cov / (sx * sy);
}

// =============================================================================
// Histogram
// =============================================================================

/**
 * @brief Compute a histogram of the data.
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container
 * @param num_bins Number of bins
 * @return Vector of pairs (bin_center, count)
 * @throws std::invalid_argument if container is empty or num_bins is 0
 */
template <typename Container>
[[nodiscard]] auto histogram(const Container & data, size_t num_bins)
  -> std::vector<std::pair<std::decay_t<decltype(*std::begin(data))>,
                           size_t>>
{
  using T = std::decay_t<decltype(*std::begin(data))>;

  if (num_bins == 0)
    throw std::invalid_argument("histogram: num_bins must be > 0");

  auto [min_val, max_val] = min_max(data);
  T range = max_val - min_val;

  if (range == T())
    {
      // All values are equal
      return {{min_val, std::distance(std::begin(data), std::end(data))}};
    }

  T bin_width = range / static_cast<T>(num_bins);

  std::vector<size_t> counts(num_bins, 0);

  for (const auto & x : data)
    {
      size_t bin = static_cast<size_t>((x - min_val) / bin_width);
      if (bin >= num_bins)
        bin = num_bins - 1;  // Handle max value
      ++counts[bin];
    }

  std::vector<std::pair<T, size_t>> result;
  result.reserve(num_bins);

  for (size_t i = 0; i < num_bins; ++i)
    {
      T center = min_val + (i + 0.5) * bin_width;
      result.emplace_back(center, counts[i]);
    }

  return result;
}

// =============================================================================
// Comprehensive Stats Function
// =============================================================================

/**
 * @brief Compute all statistics for a dataset.
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container
 * @return Stats struct with all computed values
 */
template <typename Container>
[[nodiscard]] auto compute_all_stats(const Container & data)
  -> Stats<std::decay_t<decltype(*std::begin(data))>>
{
  using T = std::decay_t<decltype(*std::begin(data))>;
  Stats<T> s;

  // Count elements
  for (const auto & x : data)
    {
      s.sum += x;
      ++s.count;
    }

  if (s.count == 0)
    return s;

  s.mean = s.sum / static_cast<T>(s.count);

  // Min/Max
  auto [min_val, max_val] = min_max(data);
  s.min = min_val;
  s.max = max_val;

  // Variance and stddev (need at least 2 elements)
  if (s.count >= 2)
    {
      s.variance = variance(data, false);
      s.stddev = std::sqrt(s.variance);

      // Coefficient of variation
      if (s.mean != T())
        s.coef_variation = s.stddev / std::abs(s.mean);
    }

  // Percentiles
  s.median = percentile(data, 50);
  s.q1 = percentile(data, 25);
  s.q3 = percentile(data, 75);
  s.iqr = s.q3 - s.q1;

  // Higher moments (need at least 3/4 elements)
  if (s.count >= 3)
    s.skewness = skewness(data);

  if (s.count >= 4)
    s.kurtosis = kurtosis(data);

  return s;
}

// =============================================================================
// Legacy API (backward compatible)
// =============================================================================

/**
 * @brief Compute basic descriptive statistics for an array range.
 *
 * @deprecated Use compute_all_stats() instead.
 *
 * Calculates average, variance, median, minimum, and maximum for
 * the elements in data[l..r].
 *
 * @tparam T Numeric type supporting arithmetic operations
 * @param data Array of values (will be sorted)
 * @param l Left index (inclusive)
 * @param r Right index (inclusive)
 * @param avg Output: arithmetic mean
 * @param var Output: sample variance
 * @param med Output: median value
 * @param _min Output: minimum value
 * @param _max Output: maximum value
 *
 * @note The array data[l..r] is sorted as a side effect.
 * @note Uses Welford's algorithm for numerically stable variance.
 *
 * @warning Requires at least 2 elements for valid variance calculation.
 */
template <class T>
void compute_stats(T * data, int l, int r,
                   T & avg, T & var, T & med,
                   T & _min, T & _max)
{
  if (l > r)
    {
      avg = var = med = _min = _max = T();
      return;
    }

  // Sort the array for quantiles
  std::sort(data + l, data + r + 1);

  _min = data[l];
  _max = data[r];

  // Calculate median (correctly using l offset)
  int n = r - l + 1;
  int mid = l + n / 2;
  if (n % 2 == 0)
    med = (data[mid - 1] + data[mid]) / 2;
  else
    med = data[mid];

  // Welford's algorithm for mean and variance
  T m = T();
  T m2 = T();
  for (int i = l; i <= r; ++i)
    {
      int k = i - l + 1;
      T delta = data[i] - m;
      m += delta / static_cast<T>(k);
      T delta2 = data[i] - m;
      m2 += delta * delta2;
    }

  avg = m;
  var = (n > 1) ? m2 / static_cast<T>(n - 1) : T();
}

/**
 * @brief Compute basic descriptive statistics for a container.
 *
 * Non-destructive version that copies data before sorting.
 *
 * @tparam Container Container type with begin()/end()
 * @param data Input container
 * @param avg Output: arithmetic mean
 * @param var Output: sample variance
 * @param med Output: median value
 * @param _min Output: minimum value
 * @param _max Output: maximum value
 */
	template <typename Container>
	void compute_stats(const Container & data,
	                   std::decay_t<decltype(*std::begin(data))> & avg,
	                   std::decay_t<decltype(*std::begin(data))> & var,
	                   std::decay_t<decltype(*std::begin(data))> & med,
	                   std::decay_t<decltype(*std::begin(data))> & _min,
	                   std::decay_t<decltype(*std::begin(data))> & _max)
	{
	  using T = std::decay_t<decltype(*std::begin(data))>;

	  std::vector<T> sorted(std::begin(data), std::end(data));
	  if (sorted.empty())
	    {
	      avg = var = med = _min = _max = T();
	      return;
	    }

	  std::sort(sorted.begin(), sorted.end());

	  _min = sorted.front();
	  _max = sorted.back();

	  const size_t n = sorted.size();
	  const size_t mid = n / 2;
	  if (n % 2 == 0)
	    med = (sorted[mid - 1] + sorted[mid]) / 2;
	  else
	    med = sorted[mid];

	  // Welford's algorithm for mean and variance
	  T m = T();
	  T m2 = T();
	  size_t k = 0;
	  for (const auto & x : sorted)
	    {
	      ++k;
	      T delta = x - m;
	      m += delta / static_cast<T>(k);
	      T delta2 = x - m;
	      m2 += delta * delta2;
	    }

	  avg = m;
	  var = (n > 1) ? m2 / static_cast<T>(n - 1) : T();
	}

} // namespace Aleph

// Export commonly used functions to global namespace
using Aleph::Stats;
using Aleph::compute_stats;
using Aleph::compute_all_stats;
using Aleph::sum;
using Aleph::mean;
using Aleph::variance;
using Aleph::stddev;
using Aleph::median;
using Aleph::percentile;
using Aleph::quartiles;
using Aleph::iqr;
using Aleph::mode;
using Aleph::skewness;
using Aleph::kurtosis;
using Aleph::coefficient_of_variation;
using Aleph::correlation;
using Aleph::covariance;
using Aleph::histogram;
using Aleph::min_value;
using Aleph::max_value;
using Aleph::min_max;

#endif // STAT_UTILS_H
