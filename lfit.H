

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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

  LFit(const Array<double> & x, const Array<double> & y)
  {
    const size_t & n = y.size();
    if (x.size() != n)
      ALEPHTHROW(SizeMismatch, "x and y have different sizes");

    if (n == 1)
      ALEPHTHROW(SizeMismatch, "arrays have length = 1");

    gsl_fit_linear(&x.base(), 1, &y.base(), 1, y.size(), &c, &m,
		   &cov00, &cov01, &cov11, &sumsq);

    auto pearson_corr = gsl_stats_correlation(&x.base(), 1, &y.base(), 1, n);
    r2 = pow2(pearson_corr);
    if (not is_normal_number(r2))
      r2 = 1;

    double sum_sigma = 0, sum_mse = 0;
    Array<double> error;
    for (auto it = zip_it(x, y); it.has_curr(); it.next_ne())
      {
	auto t = it.get_curr_ne();
	auto diff = get<0>(t) - get<1>(t);
	auto e = fabs(diff);
	error.append(e);
	sum_sigma += e;
	sum_mse += pow2(diff);
      }

    sigma = sqrt(error.foldl<double>(0.0, [ym = sum_sigma/n] (auto acu, auto y)
      {
	return acu + pow2(y - ym);
      }) / n);

    mse = sqrt(sum_mse/n);
    // cout << "x = " << join(x) << endl
    // 	 << "y = " << join(y) << endl
    // 	 << *this << endl;
  }

  pair<double, double> predict(double x) const noexcept
  {
    double y = 0, y_err = 0;
    gsl_fit_linear_est(x, c, m, cov00, cov01, cov11, &y, &y_err);
    return make_pair(y, y_err);
  }
};



# endif
