#include <gtest/gtest.h>
#include <lfit.H>
#include <ah-random.H> // for ALEPH_WITH_GSL
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <limits>
#include <random>

#if ALEPH_WITH_GSL
#  include <gsl/gsl_fit.h>
#  include <gsl/gsl_statistics_double.h>
#endif

class LFitTest : public ::testing::Test
{
protected:
  void SetUp() override {}
  void TearDown() override {}
  
  bool approx_equal(double a, double b, double eps = 1e-6) {
    return std::abs(a - b) < eps;
  }
};

// =============================================================================
// Basic Fitting Tests
// =============================================================================

TEST_F(LFitTest, PerfectLineFit)
{
  // y = 2x + 1
  Array<double> x = {1, 2, 3, 4, 5};
  Array<double> y = {3, 5, 7, 9, 11};
  
  LFit fit(x, y);
  
  EXPECT_TRUE(approx_equal(fit.m, 2.0, 1e-10));
  EXPECT_TRUE(approx_equal(fit.c, 1.0, 1e-10));
  EXPECT_TRUE(approx_equal(fit.r2, 1.0, 1e-10));
  EXPECT_TRUE(approx_equal(fit.sumsq, 0.0, 1e-10));
}

TEST_F(LFitTest, HorizontalLine)
{
  Array<double> x = {1, 2, 3, 4, 5};
  Array<double> y = {5, 5, 5, 5, 5};
  
  LFit fit(x, y);
  
  EXPECT_TRUE(approx_equal(fit.m, 0.0, 1e-10));
  EXPECT_TRUE(approx_equal(fit.c, 5.0, 1e-10));
}

TEST_F(LFitTest, VerticalIncrement)
{
  // y = x (45 degree line)
  Array<double> x = {0, 1, 2, 3, 4};
  Array<double> y = {0, 1, 2, 3, 4};
  
  LFit fit(x, y);
  
  EXPECT_TRUE(approx_equal(fit.m, 1.0, 1e-10));
  EXPECT_TRUE(approx_equal(fit.c, 0.0, 1e-10));
  EXPECT_TRUE(approx_equal(fit.r2, 1.0, 1e-10));
}

TEST_F(LFitTest, NegativeSlope)
{
  // y = -2x + 10
  Array<double> x = {1, 2, 3, 4, 5};
  Array<double> y = {8, 6, 4, 2, 0};
  
  LFit fit(x, y);
  
  EXPECT_TRUE(approx_equal(fit.m, -2.0, 1e-10));
  EXPECT_TRUE(approx_equal(fit.c, 10.0, 1e-10));
  EXPECT_TRUE(approx_equal(fit.r2, 1.0, 1e-10));
}

// =============================================================================
// Noisy Data Tests
// =============================================================================

TEST_F(LFitTest, SlightNoise)
{
  // y = 2x + 1 with small noise
  Array<double> x = {1, 2, 3, 4, 5};
  Array<double> y = {3.1, 4.9, 7.1, 8.9, 11.0};
  
  LFit fit(x, y);
  
  EXPECT_TRUE(approx_equal(fit.m, 2.0, 0.2));
  EXPECT_TRUE(approx_equal(fit.c, 1.0, 0.2));
  EXPECT_GT(fit.r2, 0.95);
}

TEST_F(LFitTest, ModerateNoise)
{
  Array<double> x = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  Array<double> y = {2.5, 4.2, 6.1, 7.8, 10.3, 12.1, 13.9, 16.2, 17.8, 20.1};
  
  LFit fit(x, y);
  
  EXPECT_GT(fit.m, 1.5);
  EXPECT_LT(fit.m, 2.5);
  EXPECT_GT(fit.r2, 0.9);
}

// =============================================================================
// Prediction Tests
// =============================================================================

TEST_F(LFitTest, PredictWithinRange)
{
  Array<double> x = {1, 2, 3, 4, 5};
  Array<double> y = {3, 5, 7, 9, 11};
  
  LFit fit(x, y);
  
  auto [pred, err] = fit.predict(3.0);
  EXPECT_TRUE(approx_equal(pred, 7.0, 1e-10));
}

TEST_F(LFitTest, PredictExtrapolation)
{
  Array<double> x = {1, 2, 3, 4, 5};
  Array<double> y = {3, 5, 7, 9, 11};
  
  LFit fit(x, y);
  
  auto [pred, err] = fit.predict(10.0);
  EXPECT_TRUE(approx_equal(pred, 21.0, 1e-10));
}

TEST_F(LFitTest, PredictMultipleValues)
{
  Array<double> x = {0, 1, 2, 3, 4};
  Array<double> y = {1, 3, 5, 7, 9};
  
  LFit fit(x, y);
  
  for (double xval = 0; xval <= 4; xval += 0.5)
  {
    auto [pred, err] = fit.predict(xval);
    double expected = 2.0 * xval + 1.0;
    EXPECT_TRUE(approx_equal(pred, expected, 1e-10));
  }
}

// =============================================================================
// Edge Cases Tests
// =============================================================================

TEST_F(LFitTest, TwoPoints)
{
  Array<double> x = {1, 5};
  Array<double> y = {2, 10};
  
  LFit fit(x, y);
  
  EXPECT_TRUE(approx_equal(fit.m, 2.0, 1e-10));
  EXPECT_TRUE(approx_equal(fit.r2, 1.0, 1e-10));
}

TEST_F(LFitTest, LargeNumbers)
{
  Array<double> x = {1000, 2000, 3000, 4000, 5000};
  Array<double> y = {2000, 4000, 6000, 8000, 10000};
  
  LFit fit(x, y);
  
  EXPECT_TRUE(approx_equal(fit.m, 2.0, 1e-6));
}

TEST_F(LFitTest, SmallNumbers)
{
  Array<double> x = {0.001, 0.002, 0.003, 0.004, 0.005};
  Array<double> y = {0.002, 0.004, 0.006, 0.008, 0.010};
  
  LFit fit(x, y);
  
  EXPECT_TRUE(approx_equal(fit.m, 2.0, 1e-6));
}

TEST_F(LFitTest, NegativeValues)
{
  Array<double> x = {-5, -4, -3, -2, -1};
  Array<double> y = {-10, -8, -6, -4, -2};
  
  LFit fit(x, y);
  
  EXPECT_TRUE(approx_equal(fit.m, 2.0, 1e-10));
}

// =============================================================================
// Statistical Properties Tests
// =============================================================================

TEST_F(LFitTest, RSquaredRange)
{
  Array<double> x = {1, 2, 3, 4, 5};
  Array<double> y = {2.1, 3.9, 6.1, 8.0, 10.2};
  
  LFit fit(x, y);
  
  EXPECT_GE(fit.r2, 0.0);
  EXPECT_LE(fit.r2, 1.0);
}

TEST_F(LFitTest, ResidualSum)
{
  Array<double> x = {1, 2, 3, 4, 5};
  Array<double> y = {3, 5, 7, 9, 11};
  
  LFit fit(x, y);
  
  EXPECT_GE(fit.sumsq, 0.0);
  EXPECT_TRUE(approx_equal(fit.sumsq, 0.0, 1e-10));
}

TEST_F(LFitTest, MSECalculation)
{
  Array<double> x = {1, 2, 3, 4, 5};
  Array<double> y = {3.1, 5.2, 6.9, 9.1, 10.8};
  
  LFit fit(x, y);
  
  EXPECT_GE(fit.mse, 0.0);
  EXPECT_GT(fit.sigma, 0.0);
}

// =============================================================================
// Different Data Patterns Tests
// =============================================================================

TEST_F(LFitTest, UniformSpacing)
{
  Array<double> x = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  Array<double> y;
  
  for (size_t i = 0; i < x.size(); ++i)
    y.append(3.0 * x[i] + 2.0);
  
  LFit fit(x, y);
  
  EXPECT_TRUE(approx_equal(fit.m, 3.0, 1e-10));
  EXPECT_TRUE(approx_equal(fit.c, 2.0, 1e-10));
}

TEST_F(LFitTest, NonUniformSpacing)
{
  Array<double> x = {1, 3, 5, 10, 15};
  Array<double> y = {2, 6, 10, 20, 30};
  
  LFit fit(x, y);
  
  EXPECT_TRUE(approx_equal(fit.m, 2.0, 1e-10));
}

TEST_F(LFitTest, ReversedOrder)
{
  Array<double> x = {5, 4, 3, 2, 1};
  Array<double> y = {11, 9, 7, 5, 3};
  
  LFit fit(x, y);
  
  EXPECT_TRUE(approx_equal(fit.m, 2.0, 1e-10));
  EXPECT_TRUE(approx_equal(fit.c, 1.0, 1e-10));
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST_F(LFitTest, LargeDataset)
{
  Array<double> x, y;
  
  for (int i = 0; i < 1000; ++i)
  {
    x.append(i);
    y.append(2.5 * i + 10.0);
  }
  
  LFit fit(x, y);
  
  EXPECT_TRUE(approx_equal(fit.m, 2.5, 1e-10));
  EXPECT_TRUE(approx_equal(fit.c, 10.0, 1e-8));
  EXPECT_TRUE(approx_equal(fit.r2, 1.0, 1e-10));
}

TEST_F(LFitTest, ManyPredictions)
{
  Array<double> x = {1, 2, 3, 4, 5};
  Array<double> y = {2, 4, 6, 8, 10};
  
  LFit fit(x, y);
  
  for (int i = -10; i <= 20; ++i)
  {
    auto [pred, err] = fit.predict(i);
    EXPECT_TRUE(approx_equal(pred, 2.0 * i, 1e-10));
  }
}

TEST_F(LFitTest, RepeatConstruction)
{
  Array<double> x = {1, 2, 3, 4, 5};
  Array<double> y = {3, 5, 7, 9, 11};
  
  for (int i = 0; i < 100; ++i)
  {
    LFit fit(x, y);
    EXPECT_TRUE(approx_equal(fit.m, 2.0, 1e-10));
    EXPECT_TRUE(approx_equal(fit.c, 1.0, 1e-10));
  }
}

// =============================================================================
// Comparison Tests
// =============================================================================

TEST_F(LFitTest, DifferentSlopes)
{
  Array<double> x = {1, 2, 3, 4, 5};
  
  Array<double> y1 = {2, 4, 6, 8, 10};
  LFit fit1(x, y1);
  
  Array<double> y2 = {3, 6, 9, 12, 15};
  LFit fit2(x, y2);
  
  EXPECT_TRUE(approx_equal(fit1.m, 2.0, 1e-10));
  EXPECT_TRUE(approx_equal(fit2.m, 3.0, 1e-10));
  EXPECT_GT(fit2.m, fit1.m);
}

TEST_F(LFitTest, DifferentIntercepts)
{
  Array<double> x = {1, 2, 3, 4, 5};
  
  Array<double> y1 = {3, 5, 7, 9, 11};
  LFit fit1(x, y1);
  
  Array<double> y2 = {5, 7, 9, 11, 13};
  LFit fit2(x, y2);
  
  EXPECT_TRUE(approx_equal(fit1.c, 1.0, 1e-10));
  EXPECT_TRUE(approx_equal(fit2.c, 3.0, 1e-10));
  EXPECT_GT(fit2.c, fit1.c);
}

// =============================================================================
// The errors of the fit (they were taken from x - y, not from the residuals)
// =============================================================================

TEST_F(LFitTest, AnExactLineHasNoErrors)
{
  // y = 2x + 1: the residuals are 0, and so are the errors; before, mse was
  // 4.243, the root of the mean of the squares of x - y
  Array<double> x = {1, 2, 3, 4, 5};
  Array<double> y = {3, 5, 7, 9, 11};

  LFit fit(x, y);

  EXPECT_EQ(fit.sumsq, 0.0);
  EXPECT_EQ(fit.mse, 0.0);
  EXPECT_EQ(fit.sigma, 0.0);
}

TEST_F(LFitTest, TheErrorsComeFromTheResiduals)
{
  Array<double> x = {1, 2, 3, 4, 5, 6, 7, 8};
  Array<double> y = {3.1, 5.2, 6.9, 9.1, 10.8, 13.3, 14.9, 17.2};

  LFit fit(x, y);

  const size_t n = x.size();
  double sum = 0;
  for (size_t i = 0; i < n; ++i)
    {
      const double e = y[i] - (fit.c + fit.m * x[i]);
      sum += e * e;
    }

  // the sum of the squares of the residuals, y - (c + m x), and from it the
  // mean squared error and the standard deviation of the errors
  EXPECT_NEAR(fit.sumsq, sum, 1e-12 * sum);
  EXPECT_EQ(fit.mse, fit.sumsq / n);
  EXPECT_EQ(fit.sigma, std::sqrt(fit.sumsq / (n - 2.0)));
  EXPECT_GT(fit.sigma, 0.0);
  EXPECT_NEAR(fit.mse, sum / n, 1e-12 * sum);

  // the variance of the errors, sigma^2, is the one of the covariances
  EXPECT_NEAR(fit.cov11, fit.sigma * fit.sigma / (n * 5.25), 1e-12 * fit.cov11);
}

TEST_F(LFitTest, TwoPointsGiveNoStandardDeviationOfTheErrors)
{
  // With two points, the line passes through them and the errors have no
  // degree of freedom: sigma is undefined, as the covariances of GSL are
  Array<double> x = {1, 5};
  Array<double> y = {2, 10};

  LFit fit(x, y);

  EXPECT_EQ(fit.sumsq, 0.0);
  EXPECT_EQ(fit.mse, 0.0);
  EXPECT_TRUE(std::isnan(fit.sigma));
}

// LFit throws SizeMismatch until 8.0.0, of aleph-exceptions.H, deprecated
#if defined(_MSC_VER) && !defined(__clang__)
#  pragma warning(push)
#  pragma warning(disable : 4996)
#else
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

TEST_F(LFitTest, EmptyArraysThrow)
{
  // Before, the correlation read the first element of the empty arrays
  Array<double> x, y;
  EXPECT_THROW((void) LFit(x, y), SizeMismatch);

  Array<double> one = {1};
  EXPECT_THROW((void) LFit(one, one), SizeMismatch);
}

TEST_F(LFitTest, ArraysOfDifferentSizesThrow)
{
  Array<double> x = {1, 2, 3};
  Array<double> y = {1, 2};
  EXPECT_THROW((void) LFit(x, y), SizeMismatch);
  EXPECT_THROW((void) LFit(y, x), SizeMismatch);
}

#if defined(_MSC_VER) && !defined(__clang__)
#  pragma warning(pop)
#else
#  pragma GCC diagnostic pop
#endif

// SizeMismatch is a std::runtime_error: a program that catches that, or
// std::exception, which LFit throws also from 8.0.0, does not name it
TEST_F(LFitTest, ItsErrorsAreRuntimeErrors)
{
  Array<double> x = {1, 2, 3};
  Array<double> y = {1, 2};
  Array<double> e;
  EXPECT_THROW((void) LFit(x, y), std::runtime_error);
  EXPECT_THROW((void) LFit(e, e), std::exception);
}

#if ALEPH_WITH_GSL
namespace
{
  // Bit by bit where this test and GSL round each operation alike: x86-64,
  // whose GSL is built without fused multiply-adds, when the test is built
  // without them too. Elsewhere, relative to the greater of gsl and scale:
  // a fused multiply-add rounds the residuals, which cancel the deviations
  // of y, otherwise. With -mfma, the fit differs at most by 1.4e-12, and the
  // sums of squares and the covariances by 1.1e-9, in these data
  constexpr bool exact =
#  if (defined(__x86_64__) || defined(_M_X64)) && !defined(__FMA__) && !defined(__FP_FAST_FMA)
    true;
#  else
    false;
#  endif

  ::testing::AssertionResult same_as_gsl(const double ours, const double gsl,
                                         const double tolerance, const double scale = 0)
  {
    if (std::memcmp(&ours, &gsl, sizeof ours) == 0 or (std::isnan(ours) and std::isnan(gsl)))
      return ::testing::AssertionSuccess();
    if (not exact and std::abs(ours - gsl) <= tolerance * std::max(std::abs(gsl), scale))
      return ::testing::AssertionSuccess();
    return ::testing::AssertionFailure() << std::setprecision(17) << ours << " instead of "
                                         << gsl << " of GSL";
  }
}

TEST_F(LFitTest, TheFitIsThatOfGsl)
{
  // For each scale of x, from 2^-12 to 2^12, and each size, lines with
  // noise, an exact one, a horizontal one, a vertical one and ones far from
  // 0, against gsl_fit_linear(), gsl_stats_correlation() and
  // gsl_fit_linear_est(); the degenerate ones, and the two points, which
  // the line fits exactly, only bit by bit
  std::mt19937_64 g(2026);
  std::uniform_real_distribution<double> u(-1, 1);
  size_t compared = 0;
  for (int scale = -12; scale <= 12; scale += 3)
    for (const size_t n : {2, 3, 4, 5, 7, 10, 31, 100, 1000})
      for (int rep = 0; rep < 10; ++rep)
        {
          const double sx = std::ldexp(1.0, scale), sy = std::ldexp(1.0, -scale / 2);
          const double slope = u(g) * 10, intercept = u(g) * 1000;
          const bool degenerate = rep < 3 or n == 2; // exact fits
          Array<double> x, y;
          for (size_t i = 0; i < n; ++i)
            {
              const double xi = (rep % 4 == 3 ? static_cast<double>(i) : u(g)) * sx
                                + (rep >= 8 ? 1e6 : 0);
              x.append(rep == 2 ? -1.5 : xi);
              y.append(rep == 1 ? 3.25 : slope * x[i] + intercept + (rep == 0 ? 0 : u(g) * sy));
            }
          if (degenerate and not exact)
            continue;

          LFit fit(x, y);

          double c, m, cov00, cov01, cov11, sumsq;
          gsl_fit_linear(&x.base(), 1, &y.base(), 1, n, &c, &m, &cov00, &cov01, &cov11, &sumsq);
          double r2 = pow2(gsl_stats_correlation(&x.base(), 1, &y.base(), 1, n));
          if (not is_normal_number(r2))
            r2 = 1;

          double ymax = 0;
          for (size_t i = 0; i < n; ++i)
            ymax = std::max(ymax, std::abs(y[i]));

          const auto where = [&] { return "scale " + std::to_string(scale) + ", n "
                                     + std::to_string(n) + ", case " + std::to_string(rep); };
          EXPECT_TRUE(same_as_gsl(fit.c, c, 1e-10, ymax)) << where();
          EXPECT_TRUE(same_as_gsl(fit.m, m, 1e-10)) << where();
          EXPECT_TRUE(same_as_gsl(fit.cov00, cov00, 1e-6)) << where();
          EXPECT_TRUE(same_as_gsl(fit.cov01, cov01, 1e-6)) << where();
          EXPECT_TRUE(same_as_gsl(fit.cov11, cov11, 1e-6)) << where();
          EXPECT_TRUE(same_as_gsl(fit.sumsq, sumsq, 1e-6)) << where();
          EXPECT_TRUE(same_as_gsl(fit.r2, r2, 1e-10)) << where();

          for (const double xp : {0.0, u(g) * sx, 3 * sx})
            {
              double yp, yp_err;
              gsl_fit_linear_est(xp, c, m, cov00, cov01, cov11, &yp, &yp_err);
              const auto [ours, ours_err] = fit.predict(xp);
              EXPECT_TRUE(same_as_gsl(ours, yp, 1e-10, ymax)) << where() << ", x " << xp;
              EXPECT_TRUE(same_as_gsl(ours_err, yp_err, 1e-6)) << where() << ", x " << xp;
            }
          ++compared;
        }
  EXPECT_GT(compared, exact ? 800u : 500u);
}
#else
// Without GSL (ALEPH_WITH_GSL is 0), the cases that compare with it
// report SKIPPED instead of disappearing from the run, as
// docs/ci_sanitizer_policy.md asks of a case that a configuration
// cannot run; the other cases check the fit by themselves
TEST_F(LFitTest, TheFitIsThatOfGsl)
{
  GTEST_SKIP() << "without GSL (ALEPH_WITH_GSL is 0)";
}
#endif // ALEPH_WITH_GSL
