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
 * @file polygon_intersection_example.cc
 * @brief Convex polygon intersection demo (Sutherland-Hodgman).
 *
 * Demonstrates:
 * - `ConvexPolygonIntersectionBasic` (Sutherland-Hodgman clipping)
 * - Export of the intersection polygon (text formats)
 *
 * @see geom_algorithms.H
 * @see ConvexPolygonIntersectionBasic
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

static Geom_Number polygon_area(const Polygon & poly)
{
  Geom_Number acc = 0;
  if (not poly.is_closed() or poly.size() < 3)
    return 0;

  Array<Point> v;
  v.reserve(poly.size());
  for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
    v.append(it.get_current_vertex());

  for (size_t i = 0; i < v.size(); ++i)
    {
      const Point & a = v(i);
      const Point & b = v((i + 1) % v.size());
      acc += a.get_x() * b.get_y() - a.get_y() * b.get_x();
    }

  if (acc < 0)
    acc = -acc;
  return acc / 2;
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
                       const Polygon & subject,
                       const Polygon & clip,
                       const Polygon & inter)
{
  ofstream file(path);
  if (not file)
    return false;

  file << fixed << setprecision(8);
  file << "name,vertex_count,area,wkt\n";

  auto write_row = [&file](const char * name, const Polygon & p)
  {
      file << name << "," << p.size() << "," << polygon_area(p) << ",\"";
      write_polygon_wkt(file, p);
      file << "\"\n";
  };

  write_row("subject", subject);
  write_row("clip", clip);
  write_row("intersection", inter);
  return true;
}

static void print_polygon(const char * name, const Polygon & poly)
{
  cout << name << " (vertices=" << poly.size()
       << ", closed=" << (poly.is_closed() ? "yes" : "no") << ")" << endl;
  for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
    {
      const Vertex & p = it.get_current_vertex();
      cout << "  (" << geom_number_to_double(p.get_x()) << ", "
           << geom_number_to_double(p.get_y()) << ")" << endl;
    }
}

int main(int argc, char ** argv)
{
  print_banner("Convex Polygon Intersection");

  Polygon a;
  a.add_vertex(Point(0, 0));
  a.add_vertex(Point(7, 0));
  a.add_vertex(Point(7, 5));
  a.add_vertex(Point(0, 5));
  a.close();

  Polygon b;
  b.add_vertex(Point(3, -1));
  b.add_vertex(Point(9, 2));
  b.add_vertex(Point(6, 7));
  b.add_vertex(Point(2, 6));
  b.close();

  ConvexPolygonIntersectionBasic intersector;
  Polygon inter = intersector(a, b);

  print_polygon("Subject", a);
  print_polygon("Clip", b);
  print_polygon("Intersection", inter);

  const Geom_Number area_inter = polygon_area(inter);
  cout << "Intersection area = " << area_inter << " (exact rational)" << endl;
  assert(inter.is_closed());
  assert(inter.size() >= 3);
  assert(area_inter > 0);

  Polygon c;
  c.add_vertex(Point(20, 20));
  c.add_vertex(Point(22, 20));
  c.add_vertex(Point(22, 22));
  c.add_vertex(Point(20, 22));
  c.close();
  Polygon disjoint = intersector(a, c);
  assert(disjoint.size() == 0);

  cout << "Disjoint case validated: intersection is empty." << endl;

  const char * csv_path =
      (argc > 1) ? argv[1] : "polygon_intersection_output.csv";
  if (export_csv(csv_path, a, b, inter))
    cout << "CSV/WKT exported to: " << csv_path << endl;
  else
    cout << "Warning: cannot export CSV to: " << csv_path << endl;

  cout << "STATUS: OK" << endl;
  return 0;
}
