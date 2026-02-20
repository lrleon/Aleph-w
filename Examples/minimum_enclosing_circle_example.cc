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
 * @file minimum_enclosing_circle_example.cc
 * @brief Minimum enclosing circle via Welzl's algorithm.
 *
 * Demonstrates:
 * - `MinimumEnclosingCircle` in expected O(n)
 * - Verification that every point is inside the result
 *
 * @see geom_algorithms.H
 * @see MinimumEnclosingCircle
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

int main()
{
  print_banner("Minimum Enclosing Circle");

  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(5, 2));
  pts.append(Point(9, 8));
  pts.append(Point(3, 4));
  pts.append(Point(4, 4));
  pts.append(Point(8, 1));
  pts.append(Point(7, 7));
  pts.append(Point(1, 6));

  MinimumEnclosingCircle mec;
  const auto circle = mec(pts);

  cout << "Center: (" << circle.center.get_x() << ", "
       << circle.center.get_y() << ")" << endl;
  cout << "Radius: " << circle.radius() << endl;
  cout << "Radius^2: " << circle.radius_squared << endl;

  // Verify all points are inside
  bool all_inside = true;
  for (DynList<Point>::Iterator it(pts); it.has_curr(); it.next_ne())
    {
      const Point & p = it.get_curr();
      const bool inside = circle.contains(p);
      cout << "  (" << p.get_x() << ", " << p.get_y() << ") -> "
           << (inside ? "inside" : "OUTSIDE") << endl;
      if (not inside)
        all_inside = false;
    }

  assert(all_inside);
  cout << "All points contained: YES" << endl;

  // Test with initializer list
  const auto c2 = mec({Point(0, 0), Point(4, 0), Point(4, 4), Point(0, 4)});
  cout << "\nSquare test: center = (" << c2.center.get_x() << ", "
       << c2.center.get_y() << "), radius = " << c2.radius() << endl;
  assert(c2.center == Point(2, 2));
  assert(c2.radius_squared == 8);

  cout << "STATUS: OK" << endl;
  return 0;
}
