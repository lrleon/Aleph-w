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

/**
 * @file stat_utils_test.cc
 * @brief Comprehensive tests for stat_utils.H
 */

#include <gtest/gtest.h>
#include <vector>
#include <array>
#include <list>
#include <cmath>
#include <stat_utils.H>

using namespace std;
using namespace Aleph;

// =============================================================================
// Sum Tests
// =============================================================================

TEST(SumTest, EmptyVector)
{
  vector<double> v;
  EXPECT_DOUBLE_EQ(sum(v), 0.0);
}

TEST(SumTest, SingleElement)
{
  vector<double> v = {5.0};
  EXPECT_DOUBLE_EQ(sum(v), 5.0);
}

TEST(SumTest, MultipleElements)
{
  vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0};
  EXPECT_DOUBLE_EQ(sum(v), 15.0);
}

TEST(SumTest, NegativeNumbers)
{
  vector<double> v = {-1.0, -2.0, 3.0};
  EXPECT_DOUBLE_EQ(sum(v), 0.0);
}

TEST(SumTest, IntegerVector)
{
  vector<int> v = {1, 2, 3, 4, 5};
  EXPECT_EQ(sum(v), 15);
}

// =============================================================================
// Mean Tests
// =============================================================================

TEST(MeanTest, EmptyContainerThrows)
{
  vector<double> v;
  EXPECT_THROW(mean(v), invalid_argument);
}

TEST(MeanTest, SingleElement)
{
  vector<double> v = {5.0};
  EXPECT_DOUBLE_EQ(mean(v), 5.0);
}

TEST(MeanTest, MultipleElements)
{
  vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0};
  EXPECT_DOUBLE_EQ(mean(v), 3.0);
}

TEST(MeanTest, NegativeNumbers)
{
  vector<double> v = {-10.0, 10.0};
  EXPECT_DOUBLE_EQ(mean(v), 0.0);
}

TEST(MeanTest, WithList)
{
  list<double> l = {1.0, 2.0, 3.0};
  EXPECT_DOUBLE_EQ(mean(l), 2.0);
}

// =============================================================================
// Variance Tests
// =============================================================================

TEST(VarianceTest, EmptyContainerThrows)
{
  vector<double> v;
  EXPECT_THROW(variance(v), invalid_argument);
}

TEST(VarianceTest, SingleElementThrows)
{
  vector<double> v = {5.0};
  EXPECT_THROW(variance(v, false), invalid_argument);  // Sample variance
}

TEST(VarianceTest, SingleElementPopulation)
{
  vector<double> v = {5.0};
  EXPECT_DOUBLE_EQ(variance(v, true), 0.0);  // Population variance
}

TEST(VarianceTest, TwoElements)
{
  vector<double> v = {0.0, 2.0};
  // Sample variance: ((0-1)^2 + (2-1)^2) / 1 = 2
  EXPECT_DOUBLE_EQ(variance(v, false), 2.0);
}

TEST(VarianceTest, SampleVsPopulation)
{
  vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0};
  double sample_var = variance(v, false);
  double pop_var = variance(v, true);
  // Sample variance should be larger (n-1 divisor vs n)
  EXPECT_GT(sample_var, pop_var);
}

TEST(VarianceTest, ConstantValues)
{
  vector<double> v = {5.0, 5.0, 5.0, 5.0};
  EXPECT_DOUBLE_EQ(variance(v), 0.0);
}

TEST(VarianceTest, NumericalStability)
{
  // Test Welford's algorithm with large values
  vector<double> v = {1e10, 1e10 + 1, 1e10 + 2};
  double var = variance(v);
  EXPECT_NEAR(var, 1.0, 1e-6);
}

// =============================================================================
// Standard Deviation Tests
// =============================================================================

TEST(StddevTest, Basic)
{
  vector<double> v = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
  double s = stddev(v);
  // Sample stddev of this data is approximately 2.14
  EXPECT_NEAR(s, 2.14, 0.1);
}

TEST(StddevTest, IsSquareRootOfVariance)
{
  vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0};
  EXPECT_NEAR(stddev(v), sqrt(variance(v)), 1e-10);
}

// =============================================================================
// Min/Max Tests
// =============================================================================

TEST(MinMaxTest, EmptyContainerThrows)
{
  vector<double> v;
  EXPECT_THROW(min_value(v), invalid_argument);
  EXPECT_THROW(max_value(v), invalid_argument);
  EXPECT_THROW(min_max(v), invalid_argument);
}

TEST(MinMaxTest, SingleElement)
{
  vector<double> v = {5.0};
  EXPECT_DOUBLE_EQ(min_value(v), 5.0);
  EXPECT_DOUBLE_EQ(max_value(v), 5.0);
  auto [min_v, max_v] = min_max(v);
  EXPECT_DOUBLE_EQ(min_v, 5.0);
  EXPECT_DOUBLE_EQ(max_v, 5.0);
}

TEST(MinMaxTest, MultipleElements)
{
  vector<double> v = {3.0, 1.0, 4.0, 1.0, 5.0, 9.0, 2.0};
  EXPECT_DOUBLE_EQ(min_value(v), 1.0);
  EXPECT_DOUBLE_EQ(max_value(v), 9.0);
  auto [min_v, max_v] = min_max(v);
  EXPECT_DOUBLE_EQ(min_v, 1.0);
  EXPECT_DOUBLE_EQ(max_v, 9.0);
}

TEST(MinMaxTest, NegativeNumbers)
{
  vector<double> v = {-5.0, -1.0, -10.0, -3.0};
  EXPECT_DOUBLE_EQ(min_value(v), -10.0);
  EXPECT_DOUBLE_EQ(max_value(v), -1.0);
}

// =============================================================================
// Percentile Tests
// =============================================================================

TEST(PercentileTest, EmptyContainerThrows)
{
  vector<double> v;
  EXPECT_THROW(percentile(v, 50), invalid_argument);
}

TEST(PercentileTest, OutOfRangeThrows)
{
  vector<double> v = {1.0, 2.0, 3.0};
  EXPECT_THROW(percentile(v, -1), invalid_argument);
  EXPECT_THROW(percentile(v, 101), invalid_argument);
}

TEST(PercentileTest, Percentile0)
{
  vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0};
  EXPECT_DOUBLE_EQ(percentile(v, 0), 1.0);
}

TEST(PercentileTest, Percentile100)
{
  vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0};
  EXPECT_DOUBLE_EQ(percentile(v, 100), 5.0);
}

TEST(PercentileTest, Percentile50IsMedian)
{
  vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0};
  EXPECT_DOUBLE_EQ(percentile(v, 50), median(v));
}

TEST(PercentileTest, UnsortedInput)
{
  vector<double> v = {5.0, 1.0, 3.0, 2.0, 4.0};
  EXPECT_DOUBLE_EQ(percentile(v, 50), 3.0);
}

// =============================================================================
// Median Tests
// =============================================================================

TEST(MedianTest, EmptyContainerThrows)
{
  vector<double> v;
  EXPECT_THROW(median(v), invalid_argument);
}

TEST(MedianTest, SingleElement)
{
  vector<double> v = {5.0};
  EXPECT_DOUBLE_EQ(median(v), 5.0);
}

TEST(MedianTest, OddCount)
{
  vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0};
  EXPECT_DOUBLE_EQ(median(v), 3.0);
}

TEST(MedianTest, EvenCount)
{
  vector<double> v = {1.0, 2.0, 3.0, 4.0};
  EXPECT_DOUBLE_EQ(median(v), 2.5);
}

TEST(MedianTest, UnsortedData)
{
  vector<double> v = {5.0, 1.0, 3.0};
  EXPECT_DOUBLE_EQ(median(v), 3.0);
}

// =============================================================================
// Quartiles and IQR Tests
// =============================================================================

TEST(QuartilesTest, Basic)
{
  vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
  auto [q1, q2, q3] = quartiles(v);
  EXPECT_NEAR(q1, 3.25, 0.01);
  EXPECT_NEAR(q2, 5.5, 0.01);
  EXPECT_NEAR(q3, 7.75, 0.01);
}

TEST(IqrTest, Basic)
{
  vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
  auto [q1, q2, q3] = quartiles(v);
  EXPECT_NEAR(iqr(v), q3 - q1, 0.01);
}

// =============================================================================
// Mode Tests
// =============================================================================

TEST(ModeTest, EmptyContainerThrows)
{
  vector<int> v;
  EXPECT_THROW(mode(v), invalid_argument);
}

TEST(ModeTest, SingleElement)
{
  vector<int> v = {5};
  EXPECT_EQ(mode(v), 5);
}

TEST(ModeTest, AllDifferent)
{
  vector<int> v = {1, 2, 3, 4, 5};
  // When all frequencies are equal, returns first encountered
  int m = mode(v);
  EXPECT_TRUE(m >= 1 and m <= 5);
}

TEST(ModeTest, ClearMode)
{
  vector<int> v = {1, 2, 2, 3, 3, 3, 4};
  EXPECT_EQ(mode(v), 3);
}

TEST(ModeTest, Multimodal)
{
  vector<int> v = {1, 1, 2, 2, 3};
  EXPECT_TRUE(is_multimodal(v));
}

TEST(ModeTest, NotMultimodal)
{
  vector<int> v = {1, 2, 2, 3};
  EXPECT_FALSE(is_multimodal(v));
}

// =============================================================================
// Skewness Tests
// =============================================================================

TEST(SkewnessTest, TooFewElementsThrows)
{
  vector<double> v = {1.0, 2.0};
  EXPECT_THROW(skewness(v), invalid_argument);
}

TEST(SkewnessTest, SymmetricDistribution)
{
  vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0};
  EXPECT_NEAR(skewness(v), 0.0, 0.1);
}

TEST(SkewnessTest, RightSkewed)
{
  vector<double> v = {1.0, 1.0, 1.0, 1.0, 10.0};
  EXPECT_GT(skewness(v), 0.0);  // Positive skewness
}

TEST(SkewnessTest, LeftSkewed)
{
  vector<double> v = {10.0, 10.0, 10.0, 10.0, 1.0};
  EXPECT_LT(skewness(v), 0.0);  // Negative skewness
}

TEST(SkewnessTest, ConstantValues)
{
  vector<double> v = {5.0, 5.0, 5.0, 5.0};
  EXPECT_DOUBLE_EQ(skewness(v), 0.0);
}

// =============================================================================
// Kurtosis Tests
// =============================================================================

TEST(KurtosisTest, TooFewElementsThrows)
{
  vector<double> v = {1.0, 2.0, 3.0};
  EXPECT_THROW(kurtosis(v), invalid_argument);
}

TEST(KurtosisTest, UniformDistribution)
{
  // Uniform distribution has negative excess kurtosis
  vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
  double k = kurtosis(v);
  EXPECT_LT(k, 0.0);  // Platykurtic
}

TEST(KurtosisTest, ConstantValues)
{
  vector<double> v = {5.0, 5.0, 5.0, 5.0, 5.0};
  EXPECT_DOUBLE_EQ(kurtosis(v), 0.0);
}

// =============================================================================
// Coefficient of Variation Tests
// =============================================================================

TEST(CoefficientOfVariationTest, ZeroMeanThrows)
{
  vector<double> v = {-1.0, 1.0};
  EXPECT_THROW(coefficient_of_variation(v), invalid_argument);
}

TEST(CoefficientOfVariationTest, Basic)
{
  vector<double> v = {10.0, 10.0, 10.0, 10.0};
  EXPECT_DOUBLE_EQ(coefficient_of_variation(v), 0.0);
}

TEST(CoefficientOfVariationTest, PositiveValue)
{
  vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0};
  double cv = coefficient_of_variation(v);
  EXPECT_GT(cv, 0.0);
}

// =============================================================================
// Covariance Tests
// =============================================================================

TEST(CovarianceTest, DifferentSizesThrows)
{
  vector<double> x = {1.0, 2.0, 3.0};
  vector<double> y = {1.0, 2.0};
  EXPECT_THROW(covariance(x, y), invalid_argument);
}

TEST(CovarianceTest, TooFewElementsThrows)
{
  vector<double> x = {1.0};
  vector<double> y = {1.0};
  EXPECT_THROW(covariance(x, y, false), invalid_argument);
}

TEST(CovarianceTest, PerfectPositive)
{
  vector<double> x = {1.0, 2.0, 3.0, 4.0, 5.0};
  vector<double> y = {1.0, 2.0, 3.0, 4.0, 5.0};
  EXPECT_GT(covariance(x, y), 0.0);
}

TEST(CovarianceTest, PerfectNegative)
{
  vector<double> x = {1.0, 2.0, 3.0, 4.0, 5.0};
  vector<double> y = {5.0, 4.0, 3.0, 2.0, 1.0};
  EXPECT_LT(covariance(x, y), 0.0);
}

TEST(CovarianceTest, NoCorrelation)
{
  vector<double> x = {1.0, 2.0, 3.0};
  vector<double> y = {2.0, 2.0, 2.0};  // Constant
  EXPECT_NEAR(covariance(x, y), 0.0, 1e-10);
}

// =============================================================================
// Correlation Tests
// =============================================================================

TEST(CorrelationTest, PerfectPositive)
{
  vector<double> x = {1.0, 2.0, 3.0, 4.0, 5.0};
  vector<double> y = {2.0, 4.0, 6.0, 8.0, 10.0};
  EXPECT_NEAR(correlation(x, y), 1.0, 1e-10);
}

TEST(CorrelationTest, PerfectNegative)
{
  vector<double> x = {1.0, 2.0, 3.0, 4.0, 5.0};
  vector<double> y = {10.0, 8.0, 6.0, 4.0, 2.0};
  EXPECT_NEAR(correlation(x, y), -1.0, 1e-10);
}

TEST(CorrelationTest, ZeroVarianceThrows)
{
  vector<double> x = {1.0, 1.0, 1.0};  // Constant
  vector<double> y = {1.0, 2.0, 3.0};
  EXPECT_THROW(correlation(x, y), invalid_argument);
}

TEST(CorrelationTest, PartialCorrelation)
{
  vector<double> x = {1.0, 2.0, 3.0, 4.0, 5.0};
  vector<double> y = {1.0, 2.0, 1.5, 3.5, 5.0};
  double r = correlation(x, y);
  EXPECT_GT(r, 0.0);
  EXPECT_LT(r, 1.0);
}

// =============================================================================
// Histogram Tests
// =============================================================================

TEST(HistogramTest, ZeroBinsThrows)
{
  vector<double> v = {1.0, 2.0, 3.0};
  EXPECT_THROW(histogram(v, 0), invalid_argument);
}

TEST(HistogramTest, EmptyContainerThrows)
{
  vector<double> v;
  EXPECT_THROW(histogram(v, 5), invalid_argument);
}

TEST(HistogramTest, SingleBin)
{
  vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0};
  auto h = histogram(v, 1);
  EXPECT_EQ(h.size(), 1u);
  EXPECT_EQ(h[0].second, 5u);  // All in one bin
}

TEST(HistogramTest, MultipleBins)
{
  vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0};
  auto h = histogram(v, 5);
  EXPECT_EQ(h.size(), 5u);
  // Each value should fall into its own bin
  size_t total = 0;
  for (const auto & [center, count] : h)
    total += count;
  EXPECT_EQ(total, 5u);
}

TEST(HistogramTest, ConstantValues)
{
  vector<double> v = {5.0, 5.0, 5.0, 5.0};
  auto h = histogram(v, 3);
  // All values are equal, should be in single bin
  EXPECT_EQ(h.size(), 1u);
  EXPECT_EQ(h[0].second, 4u);
}

// =============================================================================
// Stats Structure Tests
// =============================================================================

TEST(StatsTest, DefaultConstruction)
{
  Stats<double> s;
  EXPECT_EQ(s.count, 0u);
  EXPECT_FALSE(s.is_valid());
}

TEST(StatsTest, RangeMethod)
{
  Stats<double> s;
  s.min = 1.0;
  s.max = 10.0;
  EXPECT_DOUBLE_EQ(s.range(), 9.0);
}

// =============================================================================
// compute_all_stats Tests
// =============================================================================

TEST(ComputeAllStatsTest, EmptyContainer)
{
  vector<double> v;
  auto s = compute_all_stats(v);
  EXPECT_EQ(s.count, 0u);
  EXPECT_FALSE(s.is_valid());
}

TEST(ComputeAllStatsTest, SingleElement)
{
  vector<double> v = {5.0};
  auto s = compute_all_stats(v);
  EXPECT_EQ(s.count, 1u);
  EXPECT_TRUE(s.is_valid());
  EXPECT_DOUBLE_EQ(s.mean, 5.0);
  EXPECT_DOUBLE_EQ(s.sum, 5.0);
  EXPECT_DOUBLE_EQ(s.min, 5.0);
  EXPECT_DOUBLE_EQ(s.max, 5.0);
  EXPECT_DOUBLE_EQ(s.median, 5.0);
}

TEST(ComputeAllStatsTest, MultipleElements)
{
  vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0};
  auto s = compute_all_stats(v);

  EXPECT_EQ(s.count, 5u);
  EXPECT_DOUBLE_EQ(s.sum, 15.0);
  EXPECT_DOUBLE_EQ(s.mean, 3.0);
  EXPECT_DOUBLE_EQ(s.min, 1.0);
  EXPECT_DOUBLE_EQ(s.max, 5.0);
  EXPECT_DOUBLE_EQ(s.median, 3.0);
  EXPECT_GT(s.variance, 0.0);
  EXPECT_GT(s.stddev, 0.0);
  EXPECT_DOUBLE_EQ(s.range(), 4.0);
}

TEST(ComputeAllStatsTest, QuartilesComputed)
{
  vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
  auto s = compute_all_stats(v);

  EXPECT_LT(s.q1, s.median);
  EXPECT_LT(s.median, s.q3);
  EXPECT_NEAR(s.iqr, s.q3 - s.q1, 0.001);
}

TEST(ComputeAllStatsTest, HigherMomentsComputed)
{
  vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
  auto s = compute_all_stats(v);

  // Symmetric distribution should have skewness near 0
  EXPECT_NEAR(s.skewness, 0.0, 0.1);
  // Kurtosis is computed for 10+ elements
  // Uniform has negative excess kurtosis
  EXPECT_LT(s.kurtosis, 0.0);
}

// =============================================================================
// Legacy compute_stats Tests (backward compatibility)
// =============================================================================

TEST(LegacyComputeStatsTest, ArrayVersion)
{
  double data[] = {5.0, 1.0, 3.0, 2.0, 4.0};
  double avg, var, med, min_val, max_val;

  compute_stats(data, 0, 4, avg, var, med, min_val, max_val);

  EXPECT_DOUBLE_EQ(avg, 3.0);
  EXPECT_DOUBLE_EQ(med, 3.0);
  EXPECT_DOUBLE_EQ(min_val, 1.0);
  EXPECT_DOUBLE_EQ(max_val, 5.0);
  EXPECT_GT(var, 0.0);
}

TEST(LegacyComputeStatsTest, ArrayWithOffset)
{
  double data[] = {100.0, 1.0, 2.0, 3.0, 100.0};
  double avg, var, med, min_val, max_val;

  // Only use data[1..3]
  compute_stats(data, 1, 3, avg, var, med, min_val, max_val);

  EXPECT_DOUBLE_EQ(avg, 2.0);
  EXPECT_DOUBLE_EQ(med, 2.0);
  EXPECT_DOUBLE_EQ(min_val, 1.0);
  EXPECT_DOUBLE_EQ(max_val, 3.0);
}

TEST(LegacyComputeStatsTest, ContainerVersion)
{
  vector<double> v = {5.0, 1.0, 3.0, 2.0, 4.0};
  double avg, var, med, min_val, max_val;

  compute_stats(v, avg, var, med, min_val, max_val);

  EXPECT_DOUBLE_EQ(avg, 3.0);
  EXPECT_DOUBLE_EQ(med, 3.0);
  EXPECT_DOUBLE_EQ(min_val, 1.0);
  EXPECT_DOUBLE_EQ(max_val, 5.0);
}

TEST(LegacyComputeStatsTest, EvenCount)
{
  double data[] = {1.0, 2.0, 3.0, 4.0};
  double avg, var, med, min_val, max_val;

  compute_stats(data, 0, 3, avg, var, med, min_val, max_val);

  EXPECT_DOUBLE_EQ(med, 2.5);  // (2+3)/2
}

TEST(LegacyComputeStatsTest, EmptyRange)
{
  double data[] = {1.0, 2.0, 3.0};
  double avg, var, med, min_val, max_val;

  // l > r means empty range
  compute_stats(data, 2, 1, avg, var, med, min_val, max_val);

  EXPECT_DOUBLE_EQ(avg, 0.0);
  EXPECT_DOUBLE_EQ(var, 0.0);
  EXPECT_DOUBLE_EQ(med, 0.0);
}

// =============================================================================
// Edge Cases and Numerical Stability
// =============================================================================

TEST(EdgeCaseTest, VeryLargeNumbers)
{
  vector<double> v = {1e15, 1e15 + 1, 1e15 + 2, 1e15 + 3, 1e15 + 4};
  auto s = compute_all_stats(v);

  // Mean should be accurate
  EXPECT_NEAR(s.mean, 1e15 + 2, 1.0);
  // Variance should be stable (Welford's algorithm)
  EXPECT_NEAR(s.variance, 2.5, 0.1);
}

TEST(EdgeCaseTest, VerySmallNumbers)
{
  vector<double> v = {1e-15, 2e-15, 3e-15, 4e-15, 5e-15};
  auto s = compute_all_stats(v);

  EXPECT_NEAR(s.mean, 3e-15, 1e-16);
  EXPECT_GT(s.variance, 0.0);
}

TEST(EdgeCaseTest, MixedSigns)
{
  vector<double> v = {-100.0, -50.0, 0.0, 50.0, 100.0};
  auto s = compute_all_stats(v);

  EXPECT_DOUBLE_EQ(s.mean, 0.0);
  EXPECT_DOUBLE_EQ(s.median, 0.0);
}

// =============================================================================
// Container Type Tests
// =============================================================================

TEST(ContainerTest, WorksWithArray)
{
  array<double, 5> a = {1.0, 2.0, 3.0, 4.0, 5.0};
  EXPECT_DOUBLE_EQ(mean(a), 3.0);
  EXPECT_DOUBLE_EQ(median(a), 3.0);
}

TEST(ContainerTest, WorksWithList)
{
  list<double> l = {1.0, 2.0, 3.0, 4.0, 5.0};
  EXPECT_DOUBLE_EQ(mean(l), 3.0);
  EXPECT_DOUBLE_EQ(median(l), 3.0);
}

TEST(ContainerTest, WorksWithDynArray)
{
  DynArray<double> a;
  a.append(1.0);
  a.append(2.0);
  a.append(3.0);
  a.append(4.0);
  a.append(5.0);

  EXPECT_DOUBLE_EQ(mean(a), 3.0);
  auto s = compute_all_stats(a);
  EXPECT_EQ(s.count, 5u);
}

// =============================================================================
// Type Tests
// =============================================================================

TEST(TypeTest, WorksWithInt)
{
  vector<int> v = {1, 2, 3, 4, 5};
  EXPECT_EQ(sum(v), 15);
  EXPECT_EQ(mode(v), 1);  // All unique, returns first
}

TEST(TypeTest, WorksWithFloat)
{
  vector<float> v = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
  EXPECT_FLOAT_EQ(mean(v), 3.0f);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
