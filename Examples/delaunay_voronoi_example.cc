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
 * @file delaunay_voronoi_example.cc
 * @brief Delaunay triangulation + Voronoi dual + clipped cells.
 *
 * Demonstrates:
 * - `DelaunayTriangulationBowyerWatson` on an input site set.
 * - `VoronoiDiagramFromDelaunay` dual construction.
 * - Clipping all Voronoi cells to a convex window.
 *
 * @see geom_algorithms.H
 * @see DelaunayTriangulationBowyerWatson
 * @see VoronoiDiagramFromDelaunay
 * @ingroup Geometry
 */

# include <geom_algorithms.H>

# include <cassert>
# include <iomanip>
# include <iostream>

using namespace Aleph;
using namespace std;

static void print_banner(const char * title)
{
  cout << "[Aleph Geometry Example] " << title << "\n";
  cout << "============================================================\n";
}

static void print_point(const Point & p)
{
  cout << "(" << geom_number_to_double(p.get_x())
       << ", " << geom_number_to_double(p.get_y()) << ")";
}

static Polygon make_clip_window()
{
  Polygon clip;
  clip.add_vertex(Point(-2, -2));
  clip.add_vertex(Point(10, -2));
  clip.add_vertex(Point(10, 10));
  clip.add_vertex(Point(-2, 10));
  clip.close();
  return clip;
}

int main()
{
  cout << fixed << setprecision(3);
  print_banner("Delaunay + Voronoi");

  DynList<Point> sites;
  sites.append(Point(0, 0));
  sites.append(Point(6, 0));
  sites.append(Point(8, 4));
  sites.append(Point(5, 8));
  sites.append(Point(1, 7));
  sites.append(Point(3, 3));
  sites.append(Point(5, 4));

  DelaunayTriangulationBowyerWatson delaunay;
  DelaunayTriangulationBowyerWatson::Result dt = delaunay(sites);

  cout << "\nUnique sites: " << dt.sites.size() << endl;
  cout << "Delaunay triangles: " << dt.triangles.size() << endl;
  assert(dt.sites.size() >= 3);
  assert(not dt.triangles.is_empty());

  for (size_t i = 0; i < dt.triangles.size(); ++i)
    {
      const auto & tr = dt.triangles(i);
      cout << "  T" << i << ": [" << tr.i << ", " << tr.j << ", " << tr.k << "]  ";
      print_point(dt.sites(tr.i));
      cout << " ";
      print_point(dt.sites(tr.j));
      cout << " ";
      print_point(dt.sites(tr.k));
      cout << endl;
    }

  VoronoiDiagramFromDelaunay voronoi;
  VoronoiDiagramFromDelaunay::Result vor = voronoi(dt);
  cout << "\nVoronoi vertices: " << vor.vertices.size() << endl;
  cout << "Voronoi edges: " << vor.edges.size() << endl;
  cout << "Voronoi cells (raw): " << vor.cells.size() << endl;
  assert(vor.sites.size() == dt.sites.size());

  Polygon clip = make_clip_window();
  Array<VoronoiDiagramFromDelaunay::ClippedCell> clipped =
      VoronoiDiagramFromDelaunay::clipped_cells_indexed(vor, clip);

  cout << "Voronoi cells (clipped): " << clipped.size() << endl;
  assert(clipped.size() == dt.sites.size());

  PointInPolygonWinding pip;
  for (size_t i = 0; i < clipped.size(); ++i)
    {
      const auto & c = clipped(i);
      assert(c.site_index == i);
      assert(c.polygon.is_closed());
      assert(c.polygon.size() >= 3);
      assert(pip.contains(c.polygon, c.site));
    }

  cout << "\nFirst 3 clipped cells:" << endl;
  for (size_t i = 0; i < clipped.size() and i < 3; ++i)
    {
      const auto & c = clipped(i);
      cout << "  cell[" << c.site_index << "] site=";
      print_point(c.site);
      cout << " vertices=" << c.polygon.size() << endl;
    }

  cout << "\nValidation OK: clipped cells are indexed and contain their site." << endl;
  cout << "STATUS: OK" << endl;
  return 0;
}
