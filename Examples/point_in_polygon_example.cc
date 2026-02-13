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
 * @file point_in_polygon_example.cc
 * @brief Point-in-polygon classification with winding number.
 *
 * Demonstrates:
 * - `PointInPolygonWinding::locate()` (Inside/Boundary/Outside)
 * - `PointInPolygonWinding::contains()` convenience predicate
 *
 * @see geom_algorithms.H
 * @see PointInPolygonWinding
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

static const char * to_cstr(const PointInPolygonWinding::Location loc)
{
  switch (loc)
    {
    case PointInPolygonWinding::Location::Inside: return "Inside";
    case PointInPolygonWinding::Location::Boundary: return "Boundary";
    case PointInPolygonWinding::Location::Outside: return "Outside";
    }
  return "Unknown";
}

static Polygon build_concave_polygon()
{
  Polygon poly;
  poly.add_vertex(Point(0, 0));
  poly.add_vertex(Point(8, 0));
  poly.add_vertex(Point(8, 3));
  poly.add_vertex(Point(5, 3));
  poly.add_vertex(Point(5, 6));
  poly.add_vertex(Point(8, 6));
  poly.add_vertex(Point(8, 9));
  poly.add_vertex(Point(0, 9));
  poly.close();
  return poly;
}

int main()
{
  print_banner("Point-in-Polygon");

  Polygon poly = build_concave_polygon();
  PointInPolygonWinding pip;

  Array<Point> queries;
  queries.append(Point(1, 1));  // inside
  queries.append(Point(6, 4));  // outside (inside the concavity notch)
  queries.append(Point(5, 4));  // boundary
  queries.append(Point(-1, 2)); // outside
  queries.append(Point(2, 8));  // inside

  Array<const char *> labels;
  labels.append("q0");
  labels.append("q1");
  labels.append("q2");
  labels.append("q3");
  labels.append("q4");

  for (size_t i = 0; i < queries.size(); ++i)
    {
      const Point & p = queries(i);
      const auto loc = pip.locate(poly, p);
      cout << labels(i) << " = (" << geom_number_to_double(p.get_x()) << ", "
           << geom_number_to_double(p.get_y()) << ") -> " << to_cstr(loc) << endl;
    }

  assert(pip.locate(poly, queries(0)) == PointInPolygonWinding::Location::Inside);
  assert(pip.locate(poly, queries(1)) == PointInPolygonWinding::Location::Outside);
  assert(pip.locate(poly, queries(2)) == PointInPolygonWinding::Location::Boundary);
  assert(pip.locate(poly, queries(3)) == PointInPolygonWinding::Location::Outside);
  assert(pip.locate(poly, queries(4)) == PointInPolygonWinding::Location::Inside);

  cout << "\nValidation OK: inside / boundary / outside classifications match." << endl;
  cout << "STATUS: OK" << endl;
  return 0;
}
