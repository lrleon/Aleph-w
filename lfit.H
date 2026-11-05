
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


/** @file lfit.H
 *  @brief Linear regression (least squares fitting).
 *
 *  This file provides the LFit struct for performing simple linear
 *  regression on paired (x, y) data. It makes the computations of
 *  `gsl_fit_linear()`, `gsl_stats_correlation()` and
 *  `gsl_fit_linear_est()` of the GNU Scientific Library, in their order
 *  of operations, without GSL: built without fused multiply-adds, as GSL
 *  is for x86-64, its results are those of GSL, bit by bit; where the
 *  compiler fuses multiply-adds, as on arm64, the fit and the prediction
 *  may differ in their last bits, as the builds of GSL there do.
 *
 *  ## Features
 *
 *  - Least squares linear fitting: y = c + m*x
 *  - Coefficient of determination (R²)
 *  - Mean squared error and standard deviation of the errors
 *  - Prediction with error estimates
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  Array<double> x = {1, 2, 3, 4, 5};
 *  Array<double> y = {2.1, 3.9, 6.1, 8.0, 10.2};
 *
 *  LFit fit(x, y);
 *  std::cout << "y = " << fit.c << " + " << fit.m << " * x\n";
 *  std::cout << "R² = " << fit.r2 << "\n";
 *
 *  auto [predicted, error] = fit.predict(6.0);
 *  ```
 *
 *  @see line.H Line equation class
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# ifndef LFIT_H
# define LFIT_H

# include <cmath>
# include <cstddef>
# include <limits>
# include <string>
# include <utility>

# include <ah-zip.H>
# include <tpl_array.H>
# include <ah-string-utils.H>

# include <aleph-exceptions.H>

/**
 * @brief Linear regression calculator using least squares method.
 *
 * Computes the best-fit line y = c + m*x that minimizes the sum of
 * squared residuals, with the algorithms of GSL (see the file).
 *
 * ## Computed Statistics
 *
 * | Field | Description |
 * |-------|-------------|
 * | c | Y-intercept |
 * | m | Slope |
 * | cov00, cov01, cov11 | Covariances of c and m |
 * | r2 | Coefficient of determination (0-1) |
 * | sumsq | Sum of the squared residuals, y - (c + m x) |
 * | sigma | Standard deviation of the errors: the root of sumsq / (n - 2) |
 * | mse | Mean squared error: sumsq / n |
 *
 * @ingroup Utilities
 */
struct LFit
{
  mutable double c = 0; ///< Y-intercept of fitted line
  mutable double m = 1; ///< Slope of fitted line
  mutable double cov00 = 0; ///< Covariance matrix element (0,0)
  mutable double cov01 = 0; ///< Covariance matrix element (0,1)
  mutable double cov11 = 0; ///< Covariance matrix element (1,1)
  mutable double sumsq = 0; ///< Sum of the squares of the residuals, y - (c + m x)
  mutable double r2 = 0; ///< Coefficient of determination
  /// Standard deviation of the errors: the root of sumsq / (n - 2), the
  /// estimate from which the covariances come; NaN for two points
  mutable double sigma = 0;
  mutable double mse = 0; ///< Mean squared error: sumsq / n

  /**
   * @brief Output stream operator for fit results.
   *
   * @param out Output stream
   * @param f LFit object to output
   * @return Reference to output stream
   */
  friend std::ostream &operator <<(std::ostream & out, const LFit & f)
  {
    return out << "r2 = " << f.r2 << std::endl
           << "sumsq = " << f.sumsq << std::endl
           << "sigma = " << f.sigma << std::endl
           << "mse   = " << f.mse << std::endl
           << "c     = " << f.c << std::endl
           << "m     = " << f.m << std::endl;
  }

  /// Default constructor creates an identity fit (y = x)
  LFit() noexcept {}

  /**
   * @brief Construct and compute linear regression.
   *
   * Fits the model y = c + m*x to minimize sum of squared errors, as
   * `gsl_fit_linear()` does, and computes R², as the square of
   * `gsl_stats_correlation()`, the mean squared error and the standard
   * deviation of the errors.
   *
   * @param x Array of independent variable values
   * @param y Array of dependent variable values
   *
   * @throw SizeMismatch if x and y have different sizes
   * @throw SizeMismatch if the arrays are empty or have only 1 element
   *
   * @par Example
   * @code
   * Array<double> x = {1, 2, 3, 4};
   * Array<double> y = {2, 4, 6, 8};
   * LFit fit(x, y);  // Perfect fit: y = 2x, R² = 1.0
   * @endcode
   */
  LFit(const Array<double> & x, const Array<double> & y)
  {
    const size_t n = y.size();
    if (x.size() != n)
      ALEPHTHROW(SizeMismatch, "x and y have different sizes");

    if (n == 0)
      ALEPHTHROW(SizeMismatch, "arrays are empty");

    if (n == 1)
      ALEPHTHROW(SizeMismatch, "arrays have length = 1");

    fit(&x.base(), &y.base(), n);

    r2 = pow2(correlation(&x.base(), &y.base(), n));
    if (not is_normal_number(r2))
      r2 = 1;

    mse = sumsq / n;
    sigma = n > 2 ? std::sqrt(sumsq / (n - 2.0)) : std::numeric_limits<double>::quiet_NaN();
  }

  /**
   * @brief Predict y-value and error for given x.
   *
   * Uses the fitted model to predict y at the given x value,
   * along with an error estimate based on the covariance matrix.
   *
   * @param x Independent variable value
   * @return Pair (predicted_y, error_estimate)
   *
   * @par Example
   * @code
   * LFit fit(x_data, y_data);
   * auto [y, err] = fit.predict(10.0);
   * // y ± err is the predicted range
   * @endcode
   */
  std::pair<double, double> predict(const double x) const noexcept
  {
    // as gsl_fit_linear_est()
    const double y = c + m * x;
    const double y_err = std::sqrt(cov00 + x * (2 * cov01 + cov11 * x));
    return std::make_pair(y, y_err);
  }

private:
  // gsl_fit_linear(), in its order of operations: the means of x and y,
  // and those of the squares and of the products of their deviations, by
  // Welford's recurrences; the sum of the squares of the residuals, from
  // the deviations; and the covariances, from that sum over n - 2. The
  // divisor i + 1 of the recurrences is counted in a double, which is
  // exact below 2^53, instead of converting i for each element
  void fit(const double * x, const double * y, const size_t n) noexcept
  {
    double m_x = 0, m_y = 0, m_dx2 = 0, m_dxdy = 0;
    double ip1 = 1;
    for (size_t i = 0; i < n; ++i, ip1 += 1)
      {
        m_x += (x[i] - m_x) / ip1;
        m_y += (y[i] - m_y) / ip1;
      }

    ip1 = 1;
    for (size_t i = 0; i < n; ++i, ip1 += 1)
      {
        const double dx = x[i] - m_x;
        const double dy = y[i] - m_y;
        m_dx2 += (dx * dx - m_dx2) / ip1;
        m_dxdy += (dx * dy - m_dxdy) / ip1;
      }

    const double b = m_dxdy / m_dx2;
    const double a = m_y - m_x * b;

    double d2 = 0;
    for (size_t i = 0; i < n; ++i)
      {
        const double dx = x[i] - m_x;
        const double dy = y[i] - m_y;
        const double d = dy - b * dx;
        d2 += d * d;
      }

    const double s2 = d2 / (n - 2.0);
    c = a;
    m = b;
    cov00 = s2 * (1.0 / n) * (1 + m_x * m_x / m_dx2);
    cov11 = s2 * 1.0 / (n * m_dx2);
    cov01 = s2 * (-m_x) / (n * m_dx2);
    sumsq = d2;
  }

  // gsl_stats_correlation(), with its sums in long double, by Welford's
  // recurrences, and the roots of double, which C takes for a long double
  // in GSL. i and i + 1 are counted in doubles, exactly below 2^53, and
  // each mean is updated after its deviation, which changes no value and
  // leaves Clang the registers of x87 for more of the sums
  static double correlation(const double * x, const double * y, const size_t n) noexcept
  {
    long double sum_xsq = 0, sum_ysq = 0, sum_cross = 0;
    long double mean_x = x[0], mean_y = y[0];
    double di = 1;
    for (size_t i = 1; i < n; ++i, di += 1)
      {
        const double ip1 = di + 1;
        const long double delta_x = x[i] - mean_x;
        mean_x += delta_x / ip1;
        const long double delta_y = y[i] - mean_y;
        mean_y += delta_y / ip1;
        const long double ratio = di / ip1;
        sum_xsq += delta_x * delta_x * ratio;
        sum_ysq += delta_y * delta_y * ratio;
        sum_cross += delta_x * delta_y * ratio;
      }

    const long double r = sum_cross / (std::sqrt(static_cast<double>(sum_xsq))
                                       * std::sqrt(static_cast<double>(sum_ysq)));
    return static_cast<double>(r);
  }
};


# endif
