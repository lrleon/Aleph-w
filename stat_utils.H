/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
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

/**
 * @file stat_utils.H
 * @brief Basic statistical utilities for arrays
 * 
 * This file provides a simple function to compute basic descriptive
 * statistics on an array of numeric values in a single pass.
 * 
 * ## Features
 * 
 * The compute_stats function calculates in one call:
 * - **Average (mean)**: Sum of values divided by count
 * - **Variance**: Sample variance using the computational formula
 * - **Median**: Middle value (or average of two middle values)
 * - **Minimum**: Smallest value
 * - **Maximum**: Largest value
 * 
 * ## Algorithm
 * 
 * 1. Sorts the array using quicksort to enable median calculation
 * 2. Extracts min/max from sorted endpoints
 * 3. Calculates median based on array parity
 * 4. Computes sum and sum of squares in one pass
 * 5. Derives average and variance from sums
 * 
 * ## Complexity
 * 
 * - **Time**: O(n log n) due to sorting
 * - **Space**: O(log n) for quicksort recursion
 * 
 * @note The input array is modified (sorted) by this function.
 * 
 * ## Usage Example
 * 
 * ```cpp
 * double data[] = {1.0, 5.0, 3.0, 2.0, 4.0};
 * double avg, var, med, min_val, max_val;
 * 
 * compute_stats(data, 0, 4, avg, var, med, min_val, max_val);
 * 
 * // avg = 3.0, med = 3.0, min_val = 1.0, max_val = 5.0
 * ```
 * 
 * @see sort_utils.H For sorting algorithms used
 * 
 * @author Leandro Rabindranath León *
 *  @ingroup Utilidades

 */

# ifndef STAT_UTILS_H
# define STAT_UTILS_H

# include <limits>
# include <sort_utils.H>
# include <dyn_sort_utils.H>

/**
 * @brief Compute basic descriptive statistics for an array range
 * 
 * Calculates average, variance, median, minimum, and maximum for
 * the elements in data[l..r].
 * 
 * @tparam T Numeric type supporting arithmetic operations
 * @param data Array of values (will be sorted)
 * @param l Left index (inclusive)
 * @param r Right index (inclusive)
 * @param avg Output: arithmetic mean
 * @param var Output: sample variance
 * @param med Output: median value
 * @param _min Output: minimum value
 * @param _max Output: maximum value
 * 
 * @note The array data[l..r] is sorted as a side effect.
 * @note The variance uses the sample variance formula: Var = (n*sum² - sum²)/(n*(n-1))
 * 
 * @warning Requires at least 2 elements for valid variance calculation. *
 *  @ingroup Utilidades

 */
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
