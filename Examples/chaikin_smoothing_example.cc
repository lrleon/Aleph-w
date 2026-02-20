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

/// @file chaikin_smoothing_example.cc
/// @brief Demonstrates Chaikin corner-cutting polygon smoothing with TikZ output.

# include <tikzgeom_algorithms.H>
# include <cassert>
# include <cmath>
# include <iostream>
# include <fstream>

using namespace std;
using namespace Aleph;

/// Build a 5-pointed star polygon centered at the origin.
static Polygon make_star(double outer_r = 10.0, double inner_r = 4.0)
{
  Polygon poly;
  for (int i = 0; i < 10; ++i)
    {
      double angle = M_PI / 2.0 + 2.0 * M_PI * i / 10.0;
      double r = (i % 2 == 0) ? outer_r : inner_r;
      poly.add_vertex(Point(r * cos(angle), r * sin(angle)));
    }
  poly.close();
  return poly;
}

/// Build an L-shaped polygon.
static Polygon make_L_shape()
{
  Polygon poly;
  poly.add_vertex(Point(0, 0));
  poly.add_vertex(Point(6, 0));
  poly.add_vertex(Point(6, 3));
  poly.add_vertex(Point(3, 3));
  poly.add_vertex(Point(3, 8));
  poly.add_vertex(Point(0, 8));
  poly.close();
  return poly;
}

int main()
{
  // 1. Star polygon
  Polygon star = make_star();
  auto star_verts = GeomPolygonUtils::extract_vertices(star);
  cout << "Star: " << star_verts.size() << " vertices" << endl;

  // Smooth at 1..4 iterations
  for (size_t k = 1; k <= 4; ++k)
    {
      Polygon smoothed = ChaikinSmoothing::smooth_polygon(star, k);
      auto sv = GeomPolygonUtils::extract_vertices(smoothed);
      cout << "  iter " << k << ": " << sv.size() << " vertices" << endl;
    }

  // 2. L-shaped polygon
  Polygon lshape = make_L_shape();
  auto l_verts = GeomPolygonUtils::extract_vertices(lshape);
  cout << "L-shape: " << l_verts.size() << " vertices" << endl;

  for (size_t k = 1; k <= 3; ++k)
    {
      Polygon smoothed = ChaikinSmoothing::smooth_polygon(lshape, k);
      auto sv = GeomPolygonUtils::extract_vertices(smoothed);
      cout << "  iter " << k << ": " << sv.size() << " vertices" << endl;
    }

  // 3. TikZ output
  ofstream tex("chaikin_smoothing_output.tex");
  tex << "\\documentclass[border=5mm]{standalone}\n"
      << "\\usepackage{tikz}\n"
      << "\\usepackage{subcaption}\n"
      << "\\begin{document}\n"
      << "\\begin{figure}\n";

  auto draw_subfig = [&](const char * caption, const Polygon & original,
                          const Polygon & smoothed)
    {
      Tikz_Plane plane(60, 60);
      put_smoothing_result(plane, original, smoothed);
      tex << "\\begin{minipage}{0.45\\textwidth}\\centering\n";
      plane.draw(tex);
      tex << "\\captionof{subfigure}{" << caption << "}\n"
          << "\\end{minipage}\\hfill\n";
    };

  // Star: original, 1, 2, 4 iterations
  draw_subfig("Star original", star, star);
  Polygon star1 = ChaikinSmoothing::smooth_polygon(star, 1);
  draw_subfig("Star iter=1", star, star1);
  tex << "\n\\medskip\n\n";
  Polygon star2 = ChaikinSmoothing::smooth_polygon(star, 2);
  draw_subfig("Star iter=2", star, star2);
  Polygon star4 = ChaikinSmoothing::smooth_polygon(star, 4);
  draw_subfig("Star iter=4", star, star4);

  tex << "\n\\bigskip\n\n";

  // L-shape: original, 1, 2, 3 iterations
  draw_subfig("L-shape original", lshape, lshape);
  Polygon l1 = ChaikinSmoothing::smooth_polygon(lshape, 1);
  draw_subfig("L-shape iter=1", lshape, l1);
  tex << "\n\\medskip\n\n";
  Polygon l2 = ChaikinSmoothing::smooth_polygon(lshape, 2);
  draw_subfig("L-shape iter=2", lshape, l2);
  Polygon l3 = ChaikinSmoothing::smooth_polygon(lshape, 3);
  draw_subfig("L-shape iter=3", lshape, l3);

  tex << "\\end{figure}\n"
      << "\\end{document}\n";

  cout << "TikZ output written to chaikin_smoothing_output.tex" << endl;

  return 0;
}
