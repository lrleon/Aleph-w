
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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
# include <iostream>

# include <gsl/gsl_rng.h>

# include <geom_algorithms.H>
# include <ahNow.H>

using namespace std;

using Points = DynList<Point>;

Points generate_points(size_t n, gsl_rng *& rng)
{
  Points ret;
  
  for (size_t i = 0; i < n; ++i)
    ret.append(Point(gsl_rng_get(rng), gsl_rng_get(rng)));

  return ret;
}

int main()
{
  constexpr size_t num_exp = 6;
  constexpr size_t num_test = 12;
  size_t sample_size = 128;

  gsl_rng * rng = gsl_rng_alloc(gsl_rng_mt19937);
  gsl_rng_set(rng, time(nullptr));

  double sz[num_exp];
  double bf[num_exp];
  double gw[num_exp];
  double qh[num_exp];

  for (size_t i = 0; i < num_exp; ++i)
    {
      cout << "Testing sample size: " << sample_size << endl;
      
      sz[i] = sample_size;
      
      bf[i] = gw[i] = qh[i] = 0.0;
      
      for (size_t j = 0; j < num_test; ++j)
	{
	  Points points = generate_points(sample_size, rng);
	  
	  Now now;
	  now.start();
	  QuickHull()(points);
	  qh[i] += now.elapsed();
	  GiftWrappingConvexHull()(points);
	  gw[i] += now.elapsed();
	  BruteForceConvexHull()(points);
	  bf[i] += now.elapsed();	  
	}
      
      sample_size = sample_size << 1;
    }

  cout << "Sample size\tQuick hull\tGift wrapping\tBrute force\n"
       << "===========\t===========\t=============\t==========\n";

  for (size_t i = 0; i < num_exp; ++i)
    cout << sz[i] << '\t' << qh[i]/num_test << '\t' << gw[i]/num_test
	 << '\t' << bf[i]/num_test << endl;

  gsl_rng_free(rng);
  
  return 0;
}

