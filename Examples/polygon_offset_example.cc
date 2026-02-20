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

/// @file polygon_offset_example.cc
/// @brief Demonstrates PolygonOffset on convex and non-convex polygons.

# include <iostream>
# include <cmath>
# include <tikzgeom_algorithms.H>

using namespace std;
using namespace Aleph;

static Geom_Number poly_area(const Polygon & p)
{
  Geom_Number a = GeomPolygonUtils::signed_double_area(p) / 2;
  if (a < 0) a = -a;
  return a;
}

int main()
{
  // ----- L-shaped polygon -----
  Polygon L;
  L.add_vertex(Point(0, 0));
  L.add_vertex(Point(4, 0));
  L.add_vertex(Point(4, 2));
  L.add_vertex(Point(2, 2));
  L.add_vertex(Point(2, 4));
  L.add_vertex(Point(0, 4));
  L.close();

  // ----- Star polygon -----
  Polygon star;
  const double R = 5.0, r = 2.0;
  for (int i = 0; i < 5; ++i)
    {
      double angle_out = M_PI / 2.0 + i * 2.0 * M_PI / 5.0;
      double angle_in = angle_out + M_PI / 5.0;
      star.add_vertex(Point(Geom_Number(R * cos(angle_out)),
                            Geom_Number(R * sin(angle_out))));
      star.add_vertex(Point(Geom_Number(r * cos(angle_in)),
                            Geom_Number(r * sin(angle_in))));
    }
  star.close();

  PolygonOffset off;

  // -- L-shape offsets --
  cout << "=== L-shape ===" << endl;
  cout << "Original: " << L.size() << " vertices, area = "
       << poly_area(L).get_d() << endl;

  auto L_out1 = off(L, Geom_Number(1));
  if (!L_out1.is_empty())
    cout << "Outward d=1: " << L_out1.polygons(0).size() << " vertices, area = "
         << poly_area(L_out1.polygons(0)).get_d() << endl;

  auto L_out2 = off(L, Geom_Number(2));
  if (!L_out2.is_empty())
    cout << "Outward d=2: " << L_out2.polygons(0).size() << " vertices, area = "
         << poly_area(L_out2.polygons(0)).get_d() << endl;

  auto L_in = off(L, Geom_Number(Geom_Number(-1) / 2));
  if (!L_in.is_empty())
    cout << "Inward d=-0.5: " << L_in.polygons(0).size() << " vertices, area = "
         << poly_area(L_in.polygons(0)).get_d() << endl;

  // -- Star offsets --
  cout << "\n=== Star ===" << endl;
  cout << "Original: " << star.size() << " vertices, area = "
       << poly_area(star).get_d() << endl;

  auto S_out = off(star, Geom_Number(Geom_Number(1) / 2));
  if (!S_out.is_empty())
    {
      Geom_Number total = 0;
      for (size_t i = 0; i < S_out.size(); ++i)
        total += poly_area(S_out.polygons(i));
      cout << "Outward d=0.5: " << S_out.size() << " polygon(s), total area = "
           << total.get_d() << endl;
    }

  // -- Miter vs Bevel comparison --
  cout << "\n=== Miter vs Bevel (L-shape, d=1) ===" << endl;
  auto miter = off(L, Geom_Number(1), PolygonOffset::JoinType::Miter);
  auto bevel = off(L, Geom_Number(1), PolygonOffset::JoinType::Bevel);
  if (!miter.is_empty())
    cout << "Miter: " << miter.polygons(0).size() << " vertices, area = "
         << poly_area(miter.polygons(0)).get_d() << endl;
  if (!bevel.is_empty())
    cout << "Bevel: " << bevel.polygons(0).size() << " vertices, area = "
         << poly_area(bevel.polygons(0)).get_d() << endl;

  // ----- TikZ output -----
  const string filename = "polygon_offset_output.tex";
  ofstream out(filename);
  if (!out.is_open())
    {
      cerr << "Cannot open " << filename << endl;
      return 1;
    }

  out << "\\documentclass[border=5mm]{standalone}\n"
      << "\\usepackage{tikz}\n"
      << "\\begin{document}\n";

  // Sub-figure 1: Original L
  {
    Tikz_Plane plane(8, 8);
    put_in_plane(plane, L, tikz_wire_style("black", 1.0));
    put_polygon_vertices(plane, L, tikz_points_style("black"));
    out << "% Original L-shape\n";
    plane.draw(out);
    out << "\n\\hspace{1cm}\n";
  }

  // Sub-figure 2: Outward d=1
  if (!L_out1.is_empty())
    {
      Tikz_Plane plane(10, 10);
      put_in_plane(plane, L, tikz_wire_style("gray!50"));
      put_in_plane(plane, L_out1.polygons(0), tikz_wire_style("blue!80", 1.2));
      put_polygon_vertices(plane, L_out1.polygons(0), tikz_points_style("red"));
      out << "% Outward d=1\n";
      plane.draw(out);
      out << "\n\\hspace{1cm}\n";
    }

  // Sub-figure 3: Outward d=2
  if (!L_out2.is_empty())
    {
      Tikz_Plane plane(12, 12);
      put_in_plane(plane, L, tikz_wire_style("gray!50"));
      put_in_plane(plane, L_out2.polygons(0), tikz_wire_style("green!60!black", 1.2));
      put_polygon_vertices(plane, L_out2.polygons(0), tikz_points_style("red"));
      out << "% Outward d=2\n";
      plane.draw(out);
      out << "\n\\hspace{1cm}\n";
    }

  // Sub-figure 4: Inward d=-0.5
  if (!L_in.is_empty())
    {
      Tikz_Plane plane(8, 8);
      put_in_plane(plane, L, tikz_wire_style("gray!50"));
      put_in_plane(plane, L_in.polygons(0), tikz_wire_style("red!80", 1.2));
      put_polygon_vertices(plane, L_in.polygons(0), tikz_points_style("blue"));
      out << "% Inward d=-0.5\n";
      plane.draw(out);
      out << "\n";
    }

  out << "\\end{document}\n";
  out.close();
  cout << "\nTikZ output written to " << filename << endl;

  return 0;
}
