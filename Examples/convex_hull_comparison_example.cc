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
 * @file convex_hull_comparison_example.cc
 * @brief Compare 5 convex hull algorithms on one dataset.
 *
 * Demonstrates:
 * - `AndrewMonotonicChainConvexHull`
 * - `GrahamScanConvexHull`
 * - `BruteForceConvexHull`
 * - `GiftWrappingConvexHull`
 * - `QuickHull`
 *
 * @see geom_algorithms.H
 * @ingroup Geometry
 */

# include <geom_algorithms.H>

# include <cassert>
# include <chrono>
# include <iomanip>
# include <iostream>
# include <sstream>

using namespace Aleph;
using namespace std;

static void print_banner(const char * title)
{
  cout << "[Aleph Geometry Example] " << title << "\n";
  cout << "============================================================\n";
}

static DynList<Point> build_point_set()
{
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(6, 1));
  pts.append(Point(10, 5));
  pts.append(Point(8, 10));
  pts.append(Point(3, 12));
  pts.append(Point(-1, 8));
  pts.append(Point(-2, 3));
  pts.append(Point(4, 4));   // interior
  pts.append(Point(5, 6));   // interior
  pts.append(Point(2, 7));   // interior
  pts.append(Point(7, 7));   // interior
  pts.append(Point(1, 2));   // interior
  return pts;
}

static string hull_signature(const Polygon & poly)
{
  if (poly.size() == 0)
    return "[]";

  Array<Point> verts;
  verts.reserve(poly.size());
  for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
    verts.append(it.get_current_vertex());

  quicksort_op(verts, [](const Point & a, const Point & b)
               {
                 if (a.get_x() < b.get_x())
                   return true;
                 if (b.get_x() < a.get_x())
                   return false;
                 return a.get_y() < b.get_y();
               });

  Array<Point> unique;
  unique.reserve(verts.size());
  for (size_t i = 0; i < verts.size(); ++i)
    if (unique.is_empty() or unique.get_last() != verts(i))
      unique.append(verts(i));

  ostringstream out;
  out << "[";
  for (size_t i = 0; i < unique.size(); ++i)
    {
      if (i > 0)
        out << ";";
      out << unique(i).get_x() << "," << unique(i).get_y();
    }
  out << "]";
  return out.str();
}

template <typename Algo>
static Polygon timed_hull(const char * name, Algo & algo,
                          const DynList<Point> & pts, double & micros)
{
  const auto t0 = chrono::high_resolution_clock::now();
  Polygon h = algo(pts);
  const auto t1 = chrono::high_resolution_clock::now();
  micros = chrono::duration<double, std::micro>(t1 - t0).count();

  cout << "  " << setw(28) << left << name
       << " vertices=" << setw(3) << h.size()
       << " time=" << fixed << setprecision(2) << micros << " us" << endl;
  return h;
}

int main()
{
  print_banner("Convex Hull Comparison");

  DynList<Point> pts = build_point_set();
  cout << "Input points: " << pts.size() << endl;

  BruteForceConvexHull brute;
  GiftWrappingConvexHull gift;
  QuickHull quick;
  AndrewMonotonicChainConvexHull andrew;
  GrahamScanConvexHull graham;

  double t_brute = 0;
  double t_gift = 0;
  double t_quick = 0;
  double t_andrew = 0;
  double t_graham = 0;

  const Polygon h_brute = timed_hull("BruteForceConvexHull", brute, pts, t_brute);
  const Polygon h_gift = timed_hull("GiftWrappingConvexHull", gift, pts, t_gift);
  const Polygon h_quick = timed_hull("QuickHull", quick, pts, t_quick);
  const Polygon h_andrew = timed_hull("AndrewMonotonicChainConvexHull", andrew, pts, t_andrew);
  const Polygon h_graham = timed_hull("GrahamScanConvexHull", graham, pts, t_graham);

  const string ref = hull_signature(h_andrew);
  cout << "\nReference signature (Andrew): " << ref << endl;

  assert(hull_signature(h_brute) == ref);
  assert(hull_signature(h_gift) == ref);
  assert(hull_signature(h_quick) == ref);
  assert(hull_signature(h_graham) == ref);

  cout << "All 5 algorithms produced the same hull vertex set." << endl;
  cout << "STATUS: OK" << endl;
  return 0;
}
