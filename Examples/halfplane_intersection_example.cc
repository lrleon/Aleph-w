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
 * @file halfplane_intersection_example.cc
 * @brief 2D linear-programming feasible region via half-plane intersection.
 *
 * Demonstrates:
 * - `HalfPlaneIntersection` bounded half-plane intersection.
 * - Interpreting the output polygon as the feasible region of linear
 *   constraints in 2D.
 *
 * @see geom_algorithms.H
 * @see HalfPlaneIntersection
 * @ingroup Geometry
 */

# include <geom_algorithms.H>

# include <cassert>
# include <fstream>
# include <iomanip>
# include <iostream>

using namespace Aleph;
using namespace std;

static void print_banner(const char * title)
{
  cout << "[Aleph Geometry Example] " << title << "\n";
  cout << "============================================================\n";
}

static Geom_Number objective(const Point & p)
{
  // Maximize z = 3x + 2y
  return 3 * p.get_x() + 2 * p.get_y();
}

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
        out << ", ";

      out << geom_number_to_double(p.get_x()) << " "
          << geom_number_to_double(p.get_y());
    }

  if (not first)
    out << ", " << geom_number_to_double(first_point.get_x()) << " "
        << geom_number_to_double(first_point.get_y());

  out << "))";
}

static bool export_csv(const char * path,
                       const Polygon & feasible,
                       const Point & best_point,
                       const Geom_Number & best_value)
{
  ofstream file(path);
  if (not file)
    return false;

  file << fixed << setprecision(8);
  file << "type,index,x,y,objective,wkt\n";

  size_t idx = 0;
  for (Polygon::Vertex_Iterator it(feasible); it.has_curr(); it.next_ne(), ++idx)
    {
      const Vertex & v = it.get_current_vertex();
      file << "vertex," << idx << ","
           << geom_number_to_double(v.get_x()) << ","
           << geom_number_to_double(v.get_y()) << ","
           << objective(v) << ",\n";
    }

  file << "optimum,-1,"
       << geom_number_to_double(best_point.get_x()) << ","
       << geom_number_to_double(best_point.get_y()) << ","
       << best_value << ",\n";

  file << "polygon,-1,,,,\"";
  write_polygon_wkt(file, feasible);
  file << "\"\n";
  return true;
}

int main(int argc, char ** argv)
{
  using HP = HalfPlaneIntersection::HalfPlane;

  print_banner("Half-Plane Intersection / 2D LP");
  cout << "Feasible region of constraints:" << endl;
  cout << "  x >= 0, y >= 0, x <= 4, y <= 4, x + y <= 6" << endl;
  cout << "Objective: maximize z = 3x + 2y" << endl;

  Array<HP> hps;
  hps.append(HP(Point(0, 1), Point(0, 0)));  // x >= 0
  hps.append(HP(Point(0, 0), Point(1, 0)));  // y >= 0
  hps.append(HP(Point(4, 0), Point(4, 1)));  // x <= 4
  hps.append(HP(Point(1, 4), Point(0, 4)));  // y <= 4
  hps.append(HP(Point(6, 0), Point(0, 6)));  // x + y <= 6

  HalfPlaneIntersection hpi;
  Polygon feasible = hpi(hps);

  assert(feasible.is_closed());
  assert(feasible.size() >= 3);

  cout << "\nFeasible polygon vertices:" << endl;
  Point best_point;
  Geom_Number best_value = 0;
  bool initialized = false;

  for (Polygon::Vertex_Iterator it(feasible); it.has_curr(); it.next_ne())
    {
      const Vertex & v = it.get_current_vertex();
      const Geom_Number z = objective(v);
      cout << "  (" << geom_number_to_double(v.get_x()) << ", "
           << geom_number_to_double(v.get_y()) << "), z=" << z << endl;

      if (not initialized or z > best_value)
        {
          best_value = z;
          best_point = v;
          initialized = true;
        }
    }

  assert(initialized);
  cout << "\nOptimal vertex for z=3x+2y: ("
       << geom_number_to_double(best_point.get_x()) << ", "
       << geom_number_to_double(best_point.get_y()) << ")" << endl;
  cout << "Optimal value z* = " << best_value << endl;

  const char * csv_path =
      (argc > 1) ? argv[1] : "halfplane_intersection_output.csv";
  if (export_csv(csv_path, feasible, best_point, best_value))
    cout << "CSV/WKT exported to: " << csv_path << endl;
  else
    cout << "Warning: cannot export CSV to: " << csv_path << endl;

  cout << "STATUS: OK" << endl;

  return 0;
}
