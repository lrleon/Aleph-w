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
 * @file voronoi_clipped_cells_example.cc
 * @brief Demonstrates clipped, site-indexed Voronoi cells.
 *
 * This example builds Voronoi cells from a point set, clips every cell against
 * a convex bounding polygon, and exports the result as tabular CSV with WKT.
 *
 * Usage:
 *   ./voronoi_clipped_cells_example [output.csv]
 *
 * If `output.csv` is omitted, a default file
 * `voronoi_clipped_cells_output.csv` is generated in the current directory.
 */

# include <geom_algorithms.H>

# include <cassert>
# include <fstream>
# include <iomanip>
# include <iostream>

using namespace Aleph;
using namespace std;

static void write_polygon_wkt(ostream & out, const Polygon & poly)
{
  out << "POLYGON((";

  bool first = true;
  Point first_point;

  for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
    {
      const Point & p = it.get_current_vertex();
      if (first)
        {
          first_point = p;
          first = false;
        }
      else
        {
          out << ", ";
        }

      out << geom_number_to_double(p.get_x()) << " "
          << geom_number_to_double(p.get_y());
    }

  if (not first)
    {
      out << ", "
          << geom_number_to_double(first_point.get_x()) << " "
          << geom_number_to_double(first_point.get_y());
    }

  out << "))";
}

static void export_cells_csv(
    ostream & out,
    const Array<VoronoiDiagramFromDelaunay::ClippedCell> & cells)
{
  out << "site_index,site_x,site_y,vertex_count,wkt\n";

  for (size_t i = 0; i < cells.size(); ++i)
    {
      const auto & c = cells(i);
      out << c.site_index << ","
          << geom_number_to_double(c.site.get_x()) << ","
          << geom_number_to_double(c.site.get_y()) << ","
          << c.polygon.size() << ",\"";

      write_polygon_wkt(out, c.polygon);
      out << "\"\n";
    }
}

int main(int argc, char ** argv)
{
  VoronoiDiagramFromDelaunay voronoi;

  // Site set (deterministic, includes interior and near-hull points).
  auto vor = voronoi({
      Point(0, 0), Point(6, 0), Point(8, 4), Point(5, 8), Point(1, 7),
      Point(3, 3), Point(5, 4)
    });

  // Convex clipping window for bounded output.
  Polygon clip;
  clip.add_vertex(Point(-2, -2));
  clip.add_vertex(Point(10, -2));
  clip.add_vertex(Point(10, 10));
  clip.add_vertex(Point(-2, 10));
  clip.close();

  Array<VoronoiDiagramFromDelaunay::ClippedCell> cells =
      voronoi.clipped_cells_indexed(vor, clip);

  cout << fixed << setprecision(4);
  cout << "Voronoi clipped cells (site-indexed)\n";
  cout << "===================================\n";
  cout << "Sites: " << vor.sites.size() << "\n";
  cout << "Cells: " << cells.size() << "\n\n";

  assert(cells.size() == vor.sites.size());

  PointInPolygonWinding pip;
  for (size_t i = 0; i < cells.size(); ++i)
    {
      const auto & c = cells(i);
      cout << "Cell #" << c.site_index
           << " site=(" << geom_number_to_double(c.site.get_x())
           << ", " << geom_number_to_double(c.site.get_y()) << ")"
           << " vertices=" << c.polygon.size() << "\n";

      assert(c.polygon.is_closed());
      assert(c.polygon.size() >= 3);
      assert(pip.contains(c.polygon, c.site));
    }

  const char * path = (argc > 1) ? argv[1] : "voronoi_clipped_cells_output.csv";
  ofstream file(path);
  if (not file)
    {
      cerr << "Cannot open output file: " << path << "\n";
      return 1;
    }

  file << fixed << setprecision(8);
  export_cells_csv(file, cells);
  file.close();

  cout << "\nCSV/WKT exported to: " << path << "\n";
  cout << "Format: site_index, site_x, site_y, vertex_count, WKT polygon\n";

  return 0;
}

