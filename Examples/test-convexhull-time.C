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

