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

/// @file polygon_simplification_example.cc
/// @brief Demonstrates Douglas-Peucker and Visvalingam-Whyatt simplification
///        with TikZ output.

# include <tikzgeom_algorithms.H>
# include <cassert>
# include <cmath>
# include <random>
# include <iostream>
# include <fstream>

using namespace std;
using namespace Aleph;

static Polygon make_noisy_circle(size_t n, double radius, double noise,
                                 unsigned seed = 42)
{
  mt19937 rng(seed);
  uniform_real_distribution<double> dist(-noise, noise);

  Polygon poly;
  for (size_t i = 0; i < n; ++i)
    {
      double angle = 2.0 * M_PI * i / n;
      double r = radius + dist(rng);
      poly.add_vertex(Point(r * cos(angle), r * sin(angle)));
    }
  poly.close();
  return poly;
}

int main()
{
  // 1. Generate noisy circle
  Polygon original = make_noisy_circle(40, 10.0, 1.5);
  auto orig_verts = GeomPolygonUtils::extract_vertices(original);
  cout << "Original: " << orig_verts.size() << " vertices" << endl;

  // 2. Douglas-Peucker at two epsilon values
  DouglasPeuckerSimplification dp;
  Polygon dp_mild = dp.simplify_polygon(original, Geom_Number(1, 2)); // 0.5
  Polygon dp_aggressive = dp.simplify_polygon(original, Geom_Number(3));

  auto dpv_mild = GeomPolygonUtils::extract_vertices(dp_mild);
  auto dpv_aggr = GeomPolygonUtils::extract_vertices(dp_aggressive);
  cout << "DP mild:       " << dpv_mild.size() << " vertices" << endl;
  cout << "DP aggressive: " << dpv_aggr.size() << " vertices" << endl;

  // 3. Visvalingam-Whyatt at two thresholds
  VisvalingamWhyattSimplification vw;
  Polygon vw_mild = vw.simplify_polygon(original, Geom_Number(1));
  Polygon vw_aggressive = vw.simplify_polygon(original, Geom_Number(10));

  auto vwv_mild = GeomPolygonUtils::extract_vertices(vw_mild);
  auto vwv_aggr = GeomPolygonUtils::extract_vertices(vw_aggressive);
  cout << "VW mild:       " << vwv_mild.size() << " vertices" << endl;
  cout << "VW aggressive: " << vwv_aggr.size() << " vertices" << endl;

  // 4. Assert simplified vertices are subsets of original
  auto is_subset = [&](const Array<Point> & sub)
    {
      for (size_t i = 0; i < sub.size(); ++i)
        {
          bool found = false;
          for (size_t j = 0; j < orig_verts.size(); ++j)
            if (orig_verts(j) == sub(i))
              { found = true; break; }
          assert(found && "Simplified vertex not in original");
        }
    };
  is_subset(dpv_mild);
  is_subset(dpv_aggr);
  is_subset(vwv_mild);
  is_subset(vwv_aggr);
  cout << "All simplified vertices are subsets of original. OK" << endl;

  // 5. Produce TikZ output with 4 sub-figures
  ofstream tex("polygon_simplification_output.tex");
  tex << "\\documentclass[border=5mm]{standalone}\n"
      << "\\usepackage{tikz}\n"
      << "\\usepackage{subcaption}\n"
      << "\\begin{document}\n"
      << "\\begin{figure}\n";

  auto draw_subfig = [&](const char * caption, const Polygon & simplified)
    {
      Tikz_Plane plane(60, 60);
      put_simplification_result(plane, original, simplified);
      tex << "\\begin{minipage}{0.45\\textwidth}\\centering\n";
      plane.draw(tex);
      tex << "\\captionof{subfigure}{" << caption << "}\n"
          << "\\end{minipage}\\hfill\n";
    };

  draw_subfig("Original (40 pts)", original);
  draw_subfig("DP mild (eps=0.5)", dp_mild);
  tex << "\n\\medskip\n\n";
  draw_subfig("DP aggressive (eps=3)", dp_aggressive);
  draw_subfig("VW aggressive (area=10)", vw_aggressive);

  tex << "\\end{figure}\n"
      << "\\end{document}\n";

  cout << "TikZ output written to polygon_simplification_output.tex" << endl;

  return 0;
}
