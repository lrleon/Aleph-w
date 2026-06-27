
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

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
	  
	  now.start();
	  GiftWrappingConvexHull()(points);
	  gw[i] += now.elapsed();

	  now.start();
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

