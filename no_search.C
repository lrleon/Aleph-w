
# include <gsl/gsl_rng.h> 
# include <tpl_dynSetTree.H>
# include <iostream>

using namespace std;

// busca en el arreglo a con n números comprendidos entre 0..m un
// número que no esté en el arreglo
int not_in(int a[], int n, int m)
{
  int s, e, b, mid;
  int l = 0, r = m;

  while (true)
    {
      s = e = b = 0;
      mid = (l + r) / 2;

      for (int i = 0; i < n; ++i)
	if (a[i] >= l and a[i] <= r)
	  if (a[i] < mid) 
	    ++s;
	  else if (mid < a[i])
	    ++b;
	  else 
	    ++e;

      if (s == 0 and l < mid)
	return l;
      else if (b == 0 and r > mid)
	return r;
      else if (e == 0)
	return mid;

      if (s < b)
	r = mid - 1;
      else
	l = mid + 1;
    }
}


int main(int argn, char * argc[])
{
  int m = 101;
  int n;
  unsigned long seed;
  gsl_rng * r = gsl_rng_alloc (gsl_rng_mt19937);

  if (argn > 1)
    m = atoi(argc[1]);

  if (argn > 2)
    n = std::min(atoi(argc[2]), m -1);
  else
    n = m - 1;

  if (argn > 3)
    seed = atoi(argc[3]);
  else
    seed = time(NULL) % gsl_rng_max(r);

  cout << argc[0] << " " << m << " " << n << " " << seed << endl;

  gsl_rng_set(r, seed);

  int * a = new int [n];

  DynSetBinTree<int> s;

  for (int i = 0; i < n; ++i)
    {
    repeat:
      int value = gsl_rng_uniform_int(r, m + 1);
      if (s.exist(value))
	goto repeat;

      a[i] = value;
      s.put(value);
      //      cout << value << " ";
    }

  cout << endl;

  int val = not_in(a, n, m);

  cout << endl << endl
       << val  << " no esta en el arreglo" << endl;

  if (s.exist(val))
    cout << "Algo esta mal pues " << val << " esta en el arreglo" << endl;
  else
    cout << "La conclusion es correcta" << endl;

  delete [] a;

  gsl_rng_free(r);
}
