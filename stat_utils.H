# ifndef STAT_UTILS_H
# define STAT_UTILS_H

# include <limits>
# include <sort_utils.H>
# include <dyn_sort_utils.H>


    template <class T>
void compute_stats(T * data, int l, int r,
		   T & avg, T & var, T & med,
		   T & _min, T & _max)
{
      /* ordenar el arreglo para obtener cuantiles */
  quick_sort_rec(data, l, r);

  _min = data[l];
  _max = data[r];
  
  /* cálculo de mediana */
  int n = (r - l + 1);
  if (n % 2 == 0)
    med = (data[n/2] + data[n/2 + 1]) / 2;
  else
    med = data[n/2];

  /* calcular sumas */
  T sum  = T();
  T sum2 = T();
  for (int i = l; i <= r; i++)
    {
      sum += data[i];
      sum2 += data[i]*data[i];
    }

  avg = sum/n;

  var = (n*sum2 - sum*sum)/(n*(n - 1));
}

# endif //  STAT_UTILS_H
