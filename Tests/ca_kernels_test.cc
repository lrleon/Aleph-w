/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_kernels_test.cc
 * @brief Phase 9 tests for `ca-kernels.H`.
 *
 * Coverage:
 *  - Built-in Laplacian, mean, Sobel and Gaussian kernels.
 *  - Canonical neighbour-weight ordering compatible with Moore<2, R>.
 *  - Kernel application on scalar neighbourhood buffers.
 *  - Export to `Aleph::Matrix` for matrix-backed inspection.
 *  - Shape and bounds validation.
 */

#include <array>
#include <stdexcept>

#include <gtest/gtest.h>

#include <ca-kernels.H>
#include <ca-traits.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{

constexpr double eps = 1e-12;

}  // namespace

TEST(CAKernels, Laplacian5pHasExpectedWeightsAndZeroMass)
{
  const auto k = laplacian_5p_kernel<double>();

  EXPECT_DOUBLE_EQ(k.center(), -4.0);
  EXPECT_DOUBLE_EQ(k(0, 1), 1.0);
  EXPECT_DOUBLE_EQ(k(1, 0), 1.0);
  EXPECT_DOUBLE_EQ(k(1, 2), 1.0);
  EXPECT_DOUBLE_EQ(k(2, 1), 1.0);
  EXPECT_DOUBLE_EQ(k(0, 0), 0.0);
  EXPECT_DOUBLE_EQ(k.sum(), 0.0);

  // Moore<2,1> order is row-major over the 3x3 stencil, skipping centre.
  EXPECT_DOUBLE_EQ(k.neighbour_weight(0), 0.0);  // (-1, -1)
  EXPECT_DOUBLE_EQ(k.neighbour_weight(1), 1.0);  // (-1,  0)
  EXPECT_DOUBLE_EQ(k.neighbour_weight(3), 1.0);  // ( 0, -1)
  EXPECT_DOUBLE_EQ(k.neighbour_weight(4), 1.0);  // ( 0,  1)
  EXPECT_DOUBLE_EQ(k.neighbour_weight(6), 1.0);  // ( 1,  0)
}

TEST(CAKernels, Laplacian9pAndSmoothersHaveExpectedMass)
{
  const auto lap9 = laplacian_9p_kernel<double>();
  const auto mean3 = mean_3x3_kernel<double>();
  const auto mean5 = mean_5x5_kernel<double>();
  const auto gauss3 = gaussian_3x3_kernel<double>();
  const auto gauss5 = gaussian_5x5_kernel<double>();

  EXPECT_NEAR(lap9.sum(), 0.0, eps);
  EXPECT_NEAR(mean3.sum(), 1.0, eps);
  EXPECT_NEAR(mean5.sum(), 1.0, eps);
  EXPECT_NEAR(gauss3.sum(), 1.0, eps);
  EXPECT_NEAR(gauss5.sum(), 1.0, eps);

  EXPECT_DOUBLE_EQ(mean5.center(), 1.0 / 25.0);
  EXPECT_DOUBLE_EQ(gauss3.center(), 4.0 / 16.0);
  EXPECT_DOUBLE_EQ(gauss5.center(), 36.0 / 256.0);
}

TEST(CAKernels, ApplyUsesCentreAndMooreOrderedNeighbours)
{
  const auto mean = mean_3x3_kernel<double>();
  const std::array<double, 8> neighbours{1.0, 2.0, 3.0, 4.0,
                                         6.0, 7.0, 8.0, 9.0};
  const double conv = mean.apply(5.0, Neighbor_View<double>(neighbours.data(),
                                                           neighbours.size()));
  EXPECT_NEAR(conv, 5.0, eps);

  const auto lap = laplacian_5p_kernel<double>();
  const std::array<double, 8> constant{2.0, 2.0, 2.0, 2.0,
                                       2.0, 2.0, 2.0, 2.0};
  EXPECT_NEAR(lap.apply(2.0, Neighbor_View<double>(constant.data(),
                                                   constant.size())),
              0.0, eps);
}

TEST(CAKernels, SobelDetectsLinearGradients)
{
  const auto sx = sobel_x_kernel<double>();
  const auto sy = sobel_y_kernel<double>();

  // Values are f(row, col) = col in a 3x3 window centred at (0,0).
  const std::array<double, 8> x_neighbours{-1.0, 0.0, 1.0,
                                           -1.0, 1.0,
                                           -1.0, 0.0, 1.0};
  EXPECT_NEAR(sx.apply(0.0, Neighbor_View<double>(x_neighbours.data(),
                                                  x_neighbours.size())),
              8.0, eps);
  EXPECT_NEAR(sy.apply(0.0, Neighbor_View<double>(x_neighbours.data(),
                                                  x_neighbours.size())),
              0.0, eps);

  // Values are f(row, col) = row.
  const std::array<double, 8> y_neighbours{-1.0, -1.0, -1.0,
                                           0.0, 0.0,
                                           1.0, 1.0, 1.0};
  EXPECT_NEAR(sy.apply(0.0, Neighbor_View<double>(y_neighbours.data(),
                                                  y_neighbours.size())),
              8.0, eps);
  EXPECT_NEAR(sx.apply(0.0, Neighbor_View<double>(y_neighbours.data(),
                                                  y_neighbours.size())),
              0.0, eps);
}

TEST(CAKernels, MatrixExportPreservesWeights)
{
  const auto k = gaussian_3x3_kernel<double>();
  auto m = k.to_matrix();

  EXPECT_DOUBLE_EQ(m.get_entry(0, 0), 1.0 / 16.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 1), 4.0 / 16.0);
  EXPECT_DOUBLE_EQ(m.get_entry(2, 2), 1.0 / 16.0);
}

TEST(CAKernels, ValidatesShapeAndBounds)
{
  EXPECT_THROW((Kernel2D<double, 3, 3>{1.0, 2.0}), std::length_error);
  EXPECT_THROW((Kernel2D<double, 3, 3>{{{1.0, 2.0, 3.0},
                                       {4.0, 5.0},
                                       {6.0, 7.0, 8.0}}}),
               std::length_error);

  const auto k = laplacian_5p_kernel<double>();
  EXPECT_THROW(k(3, 0), std::out_of_range);
  EXPECT_THROW(k.weight_at_offset(-2, 0), std::out_of_range);
  EXPECT_THROW(k.neighbour_weight(8), std::out_of_range);

  const std::array<double, 2> too_short{1.0, 2.0};
  EXPECT_THROW(k.apply(0.0, Neighbor_View<double>(too_short.data(),
                                                  too_short.size())),
               std::length_error);
}
