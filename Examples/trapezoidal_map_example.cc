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

/// @file trapezoidal_map_example.cc
/// @brief Demonstrates TrapezoidalMapPointLocation with an L-shaped polygon.

# include <iostream>
# include <fstream>
# include <tikzgeom_algorithms.H>

using namespace std;
using namespace Aleph;

using TM = TrapezoidalMapPointLocation;
using LT = TM::LocationType;

static const char * location_name(LT t)
{
  switch (t)
    {
    case LT::TRAPEZOID:  return "TRAPEZOID";
    case LT::ON_SEGMENT: return "ON_SEGMENT";
    case LT::ON_POINT:   return "ON_POINT";
    }
  return "UNKNOWN";
}

int main()
{
  // Build an L-shaped polygon (6 vertices).
  //  (0,4)---(2,4)
  //    |       |
  //    |  (2,2)--(4,2)
  //    |           |
  //  (0,0)------(4,0)
  Polygon L;
  L.add_vertex(Point(0, 0));
  L.add_vertex(Point(4, 0));
  L.add_vertex(Point(4, 2));
  L.add_vertex(Point(2, 2));
  L.add_vertex(Point(2, 4));
  L.add_vertex(Point(0, 4));
  L.close();

  cout << "=== Trapezoidal Map Point Location Example ===\n\n";
  cout << "L-shaped polygon with 6 vertices, 6 edges.\n\n";

  // Construct the trapezoidal map.
  TM tm;
  auto result = tm(L);

  // Count active trapezoids.
  size_t active = 0;
  for (size_t i = 0; i < result.trapezoids.size(); ++i)
    if (result.trapezoids(i).active) ++active;
  cout << "Trapezoidal map: " << active << " active trapezoids, "
       << result.dag.size() << " DAG nodes.\n\n";

  // Query several points.
  struct QueryCase
  {
    Point p;
    const char * desc;
  };

  QueryCase queries[] =
      {
        {Point(1, 1), "inside (lower part)"},
        {Point(1, 3), "inside (upper part)"},
        {Point(3, 1), "inside (right part)"},
        {Point(3, 3), "outside (reflex area)"},
        {Point(5, 1), "outside (far right)"},
        {Point(0, 0), "vertex"},
        {Point(2, 0), "edge midpoint"},
        {Point(2, 2), "reflex vertex"},
      };

  cout << "Point queries:\n";
  for (const auto & q : queries)
    {
      auto loc = result.locate(q.p);
      bool inside = result.contains(q.p);
      cout << "  (" << q.p.get_x() << ", " << q.p.get_y() << ") "
           << q.desc << " => " << location_name(loc.type)
           << (inside ? " [INSIDE]" : " [OUTSIDE]") << "\n";
    }

  // Generate TikZ output.
  const char * tex_file = "trapezoidal_map_example_output.tex";
  cout << "\nWriting TikZ output to " << tex_file << " ...\n";

  Tikz_Plane plane(80, 80);
  put_trapezoidal_map_result(plane, result);

  // Add query points color-coded.
  for (const auto & q : queries)
    {
      bool inside = result.contains(q.p);
      auto loc = result.locate(q.p);
      std::string color = "red"; // outside
      if (loc.type == LT::ON_SEGMENT or loc.type == LT::ON_POINT)
        color = "blue"; // boundary
      else if (inside)
        color = "green!70!black"; // inside

      Tikz_Style ps = tikz_points_style(color);
      put_in_plane(plane, q.p, ps, Tikz_Plane::Layer_Overlay);
    }

  ofstream out(tex_file);
  plane.draw(out);
  out.close();

  cout << "Done. Compile with: pdflatex " << tex_file << "\n";
  return 0;
}
