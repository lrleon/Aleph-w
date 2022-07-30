
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
