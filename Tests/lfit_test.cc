#include <gtest/gtest.h>
#include <lfit.H>
#include <cmath>

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
