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
 * @file convex_polygon_distance_gjk_example.cc
 * @brief Convex polygon distance with GJK.
 *
 * Demonstrates:
 * - `ConvexPolygonDistanceGJK` for separated polygons
 * - overlap detection (distance = 0)
 *
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

int main()
{
  print_banner("Convex Polygon Distance (GJK)");

  Polygon a;
  a.add_vertex(Point(0, 0));
  a.add_vertex(Point(1, 0));
  a.add_vertex(Point(1, 1));
  a.add_vertex(Point(0, 1));
  a.close();

  Polygon b;
  b.add_vertex(Point(2, 0));
  b.add_vertex(Point(3, 0));
  b.add_vertex(Point(3, 1));
  b.add_vertex(Point(2, 1));
  b.close();

  ConvexPolygonDistanceGJK gjk;
  const auto sep = gjk(a, b);

  cout << "Separated case:\n";
  cout << "  intersects = " << (sep.intersects ? "true" : "false") << "\n";
  cout << "  distance^2 = " << sep.distance_squared << "\n";
  cout << "  distance   = " << sep.distance << "\n";
  cout << "  closest A  = (" << sep.closest_on_first.get_x() << ", "
       << sep.closest_on_first.get_y() << ")\n";
  cout << "  closest B  = (" << sep.closest_on_second.get_x() << ", "
       << sep.closest_on_second.get_y() << ")\n";
  assert(not sep.intersects);
  assert(sep.distance_squared == 1);

  Polygon c;
  c.add_vertex(Point(0, 0));
  c.add_vertex(Point(3, 0));
  c.add_vertex(Point(3, 3));
  c.add_vertex(Point(0, 3));
  c.close();

  Polygon d;
  d.add_vertex(Point(2, 2));
  d.add_vertex(Point(4, 2));
  d.add_vertex(Point(4, 4));
  d.add_vertex(Point(2, 4));
  d.close();

  const auto ov = gjk(c, d);
  cout << "\nOverlapping case:\n";
  cout << "  intersects = " << (ov.intersects ? "true" : "false") << "\n";
  cout << "  distance^2 = " << ov.distance_squared << "\n";
  assert(ov.intersects);
  assert(ov.distance_squared == 0);

  cout << "STATUS: OK\n";
  return 0;
}

