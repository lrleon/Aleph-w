
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
 *  @brief Linear regression (least squares fitting) using GSL.
 *
 *  This file provides the LFit struct for performing simple linear
 *  regression on paired (x, y) data using the GNU Scientific Library.
 *
 *  ## Features
 *
 *  - Least squares linear fitting: y = c + m*x
 *  - Coefficient of determination (R²)
 *  - Mean squared error and standard deviation
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

# include <gsl/gsl_statistics_double.h>
# include <gsl/gsl_fit.h>

# include <string>

# include <ah-zip.H>
# include <tpl_array.H>
# include <ah-string-utils.H>

# include <aleph-exceptions.H>

/**
 * @brief Linear regression calculator using least squares method.
 *
 * Computes the best-fit line y = c + m*x that minimizes the sum of
 * squared residuals. Uses GSL for numerical computation.
 *
 * ## Computed Statistics
 *
 * | Field | Description |
 * |-------|-------------|
 * | c | Y-intercept |
 * | m | Slope |
 * | r2 | Coefficient of determination (0-1) |
 * | sumsq | Sum of squared residuals |
 * | sigma | Standard deviation of errors |
 * | mse | Mean squared error |
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
  mutable double sumsq = 0; ///< Sum of squared residuals
  mutable double r2 = 0; ///< Coefficient of determination
  mutable double sigma = 0; ///< Standard deviation of errors
  mutable double mse = 0; ///< Mean squared error

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
   * Fits the model y = c + m*x to minimize sum of squared errors.
   * Computes R², standard deviation, and MSE.
   *
   * @param x Array of independent variable values
   * @param y Array of dependent variable values
   *
   * @throw SizeMismatch if x and y have different sizes
   * @throw SizeMismatch if arrays have only 1 element
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
    const size_t & n = y.size();
    if (x.size() != n)
      ALEPHTHROW(SizeMismatch, "x and y have different sizes");

    if (n == 1)
      ALEPHTHROW(SizeMismatch, "arrays have length = 1");

    gsl_fit_linear(&x.base(), 1, &y.base(), 1, y.size(), &c, &m,
                   &cov00, &cov01, &cov11, &sumsq);

    const auto pearson_corr = gsl_stats_correlation(&x.base(), 1, &y.base(), 1, n);
    r2 = pow2(pearson_corr);
    if (not is_normal_number(r2))
      r2 = 1;

    double sum_sigma = 0, sum_mse = 0;
    Array<double> error;
    for (auto it = zip_it(x, y); it.has_curr(); it.next_ne())
      {
        auto t = it.get_curr();
        const auto diff = get<0>(t) - get<1>(t);
        auto e = fabs(diff);
        error.append(e);
        sum_sigma += e;
        sum_mse += pow2(diff);
      }

    sigma = sqrt(error.foldl<double>(0.0, [ym = sum_sigma / n](auto acu, auto y)
                                       {
                                         return acu + pow2(y - ym);
                                       }) / n);

    mse = sqrt(sum_mse / n);
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
  std::pair<double, double> predict(double x) const noexcept
  {
    double y = 0, y_err = 0;
    gsl_fit_linear_est(x, c, m, cov00, cov01, cov11, &y, &y_err);
    return std::make_pair(y, y_err);
  }
};


# endif
