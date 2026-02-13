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

/**
 * @file closest_pair_example.cc
 * @brief Closest pair of points via divide and conquer.
 *
 * Demonstrates:
 * - `ClosestPairDivideAndConquer` in O(n log n)
 * - Optional brute-force verification on the same dataset
 *
 * @see geom_algorithms.H
 * @see ClosestPairDivideAndConquer
 * @ingroup Geometry
 */

# include <geom_algorithms.H>

# include <cassert>
# include <iostream>

using namespace Aleph;
using namespace std;

static void print_banner(const char * title)
{
  cout << "[Aleph Geometry Example] " << title << "\n";
  cout << "============================================================\n";
}

static Geom_Number brute_distance_squared(const DynList<Point> & pts,
                                          Point & out_a, Point & out_b)
{
  Array<Point> arr;
  for (DynList<Point>::Iterator it(pts); it.has_curr(); it.next_ne())
    arr.append(it.get_curr());

  assert(arr.size() >= 2);
  Geom_Number best = arr(0).distance_squared_to(arr(1));
  out_a = arr(0);
  out_b = arr(1);

  for (size_t i = 0; i < arr.size(); ++i)
    for (size_t j = i + 1; j < arr.size(); ++j)
      if (const Geom_Number d2 = arr(i).distance_squared_to(arr(j)); d2 < best)
        {
          best = d2;
          out_a = arr(i);
          out_b = arr(j);
        }

  return best;
}

int main()
{
  print_banner("Closest Pair");

  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(5, 2));
  pts.append(Point(9, 8));
  pts.append(Point(3, 4));
  pts.append(Point(4, 4));
  pts.append(Point(8, 1));
  pts.append(Point(7, 7));
  pts.append(Point(4, 5));

  ClosestPairDivideAndConquer cp;
  const auto r = cp(pts);

  cout << "Closest pair: (" << r.first.get_x() << ", " << r.first.get_y() << ") and "
       << "(" << r.second.get_x() << ", " << r.second.get_y() << ")" << endl;
  cout << "distance^2 = " << r.distance_squared << endl;

  Point ba, bb;
  const Geom_Number brute_d2 = brute_distance_squared(pts, ba, bb);
  cout << "Brute-force distance^2 = " << brute_d2 << endl;

  assert(r.distance_squared == brute_d2);
  cout << "Validation OK: divide-and-conquer matches brute force." << endl;

  DynList<Point> dup;
  dup.append(Point(10, 10));
  dup.append(Point(2, 3));
  dup.append(Point(10, 10)); // duplicate point
  const auto r_dup = cp(dup);
  assert(r_dup.distance_squared == 0);
  cout << "Duplicate-point scenario validated: distance^2 = 0." << endl;
  cout << "STATUS: OK" << endl;
  return 0;
}
