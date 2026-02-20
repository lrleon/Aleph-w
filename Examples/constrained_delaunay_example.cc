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
 * @file constrained_delaunay_example.cc
 * @brief Constrained Delaunay Triangulation (CDT) example.
 *
 * Demonstrates:
 * - `ConstrainedDelaunayTriangulation` on a point set with constraint edges.
 * - Verifying that all constraint edges appear in the output.
 *
 * @see geom_algorithms.H
 * @see ConstrainedDelaunayTriangulation
 * @ingroup Geometry
 */

# include <geom_algorithms.H>

# include <cassert>
# include <iomanip>
# include <iostream>

using namespace Aleph;
using namespace std;

static void print_point(const Point & p)
{
  cout << "(" << geom_number_to_double(p.get_x())
       << ", " << geom_number_to_double(p.get_y()) << ")";
}

int main()
{
  cout << fixed << setprecision(3);
  cout << "[Aleph Geometry Example] Constrained Delaunay Triangulation\n";
  cout << "============================================================\n";

  // Rectangular domain with interior points and crossing constraints.
  DynList<Point> sites;
  sites.append(Point(0, 0));
  sites.append(Point(8, 0));
  sites.append(Point(8, 6));
  sites.append(Point(0, 6));
  sites.append(Point(3, 2));
  sites.append(Point(5, 4));
  sites.append(Point(2, 5));
  sites.append(Point(6, 1));

  // Constraint edges: two crossing diagonals of the rectangle.
  DynList<Segment> constraints;
  constraints.append(Segment(Point(0, 0), Point(8, 6)));
  constraints.append(Segment(Point(8, 0), Point(0, 6)));

  ConstrainedDelaunayTriangulation cdt;
  auto result = cdt(sites, constraints);

  cout << "\nUnique sites: " << result.sites.size() << endl;
  cout << "Triangles: " << result.triangles.size() << endl;
  cout << "Constrained edges: " << result.constrained_edges.size() << endl;

  assert(result.sites.size() >= 8);
  assert(not result.triangles.is_empty());

  cout << "\nTriangles:" << endl;
  for (size_t i = 0; i < result.triangles.size(); ++i)
    {
      const auto & tr = result.triangles(i);
      cout << "  T" << i << ": [" << tr.i << ", " << tr.j << ", " << tr.k
           << "]  ";
      print_point(result.sites(tr.i));
      cout << " ";
      print_point(result.sites(tr.j));
      cout << " ";
      print_point(result.sites(tr.k));
      cout << endl;
    }

  cout << "\nConstrained edges:" << endl;
  for (size_t i = 0; i < result.constrained_edges.size(); ++i)
    {
      const auto & e = result.constrained_edges(i);
      cout << "  E" << i << ": [" << e.u << ", " << e.v << "]  ";
      print_point(result.sites(e.u));
      cout << " -> ";
      print_point(result.sites(e.v));
      cout << endl;
    }

  // Verify all constrained edges are in the triangulation.
  auto has_edge = [&](size_t pu, size_t pv) -> bool
    {
      for (size_t t = 0; t < result.triangles.size(); ++t)
        {
          const auto & tri = result.triangles(t);
          size_t vs[3] = {tri.i, tri.j, tri.k};
          for (int e = 0; e < 3; ++e)
            if ((vs[e] == pu and vs[(e + 1) % 3] == pv) or
                (vs[e] == pv and vs[(e + 1) % 3] == pu))
              return true;
        }
      return false;
    };

  for (size_t i = 0; i < result.constrained_edges.size(); ++i)
    {
      const auto & e = result.constrained_edges(i);
      assert(has_edge(e.u, e.v));
    }

  // The two crossing diagonals intersect — CDT automatically splits them
  // at the intersection point. Verify the split sub-edges exist.
  assert(result.constrained_edges.size() >= 4);

  // Convert to geometric triangles.
  DynList<Triangle> geo_tris =
      ConstrainedDelaunayTriangulation::as_triangles(result);
  assert(geo_tris.size() == result.triangles.size());

  cout << "\nValidation OK: all constraints present in CDT." << endl;
  cout << "STATUS: OK" << endl;
  return 0;
}
