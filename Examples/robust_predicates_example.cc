
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
 * @file robust_predicates_example.cc
 * @brief Demonstrates robust geometry predicates using exact arithmetic.
 *
 * This example showcases the exact orientation, intersection detection,
 * and intersection point computation provided by point.H, all using
 * Geom_Number (mpq_class) for exact rational arithmetic.
 *
 * ## Scenarios
 *
 * 1. Orientation classification of point triples
 * 2. Segment intersection detection (various configurations)
 * 3. Exact rational intersection point computation
 * 4. Road network crossing analysis
 *
 * @ingroup Geometry
 */

#include <iostream>
#include <cassert>
#include <geom_algorithms.H>
#include <point.H>

using namespace std;

static void print_banner(const char * title)
{
  cout << "[Aleph Geometry Example] " << title << "\n";
  cout << "============================================================\n";
}

static const char * orientation_str(Orientation o)
{
  switch (o)
  {
    case Orientation::CCW:      return "Counter-Clockwise";
    case Orientation::CW:       return "Clockwise";
    case Orientation::COLLINEAR: return "Collinear";
  }
  return "?";
}

static const char * in_circle_str(InCircleResult r)
{
  switch (r)
  {
    case InCircleResult::INSIDE: return "INSIDE";
    case InCircleResult::ON_CIRCLE: return "ON_CIRCLE";
    case InCircleResult::OUTSIDE: return "OUTSIDE";
    case InCircleResult::DEGENERATE: return "DEGENERATE";
  }
  return "?";
}


// ===================== Scenario 1 =====================

void scenario_orientation()
{
  cout << "=== Scenario 1: Orientation Classification ===" << endl;
  cout << endl;
  cout << "Classifying triples of points as CCW, CW, or Collinear using" << endl;
  cout << "exact rational arithmetic (no floating-point error)." << endl;
  cout << endl;

  Point a(0, 0), b(4, 0), c(2, 3);
  cout << "  Triple (0,0)-(4,0)-(2,3): "
       << orientation_str(orientation(a, b, c)) << endl;
  assert(orientation(a, b, c) == Orientation::CCW);

  cout << "  Triple (0,0)-(2,3)-(4,0): "
       << orientation_str(orientation(a, c, b)) << endl;
  assert(orientation(a, c, b) == Orientation::CW);

  Point d(1, 1), e(2, 2), f(3, 3);
  cout << "  Triple (1,1)-(2,2)-(3,3): "
       << orientation_str(orientation(d, e, f)) << endl;
  assert(orientation(d, e, f) == Orientation::COLLINEAR);

  cout << endl;
}


// ===================== Scenario 2 =====================

void scenario_intersection_detection()
{
  cout << "=== Scenario 2: Segment Intersection Detection ===" << endl;
  cout << endl;

  // X-cross
  Segment s1(Point(0, 0), Point(4, 4));
  Segment s2(Point(0, 4), Point(4, 0));
  cout << "  X-cross (0,0)-(4,4) vs (0,4)-(4,0): "
       << (segments_intersect(s1, s2) ? "INTERSECT" : "no") << endl;
  assert(segments_intersect(s1, s2));

  // T-shaped
  Segment s3(Point(0, 0), Point(6, 0));
  Segment s4(Point(3, 0), Point(3, 5));
  cout << "  T-shaped (0,0)-(6,0) vs (3,0)-(3,5): "
       << (segments_intersect(s3, s4) ? "INTERSECT" : "no") << endl;
  assert(segments_intersect(s3, s4));

  // Parallel
  Segment s5(Point(0, 0), Point(4, 0));
  Segment s6(Point(0, 2), Point(4, 2));
  cout << "  Parallel (0,0)-(4,0) vs (0,2)-(4,2): "
       << (segments_intersect(s5, s6) ? "INTERSECT" : "no") << endl;
  assert(!segments_intersect(s5, s6));

  // Collinear overlap
  Segment s7(Point(0, 0), Point(3, 0));
  Segment s8(Point(2, 0), Point(5, 0));
  cout << "  Collinear overlap (0,0)-(3,0) vs (2,0)-(5,0): "
       << (segments_intersect(s7, s8) ? "INTERSECT" : "no") << endl;
  assert(segments_intersect(s7, s8));

  // Disjoint
  Segment s9(Point(0, 0), Point(1, 1));
  Segment s10(Point(5, 5), Point(6, 7));
  cout << "  Disjoint (0,0)-(1,1) vs (5,5)-(6,7): "
       << (segments_intersect(s9, s10) ? "INTERSECT" : "no") << endl;
  assert(!segments_intersect(s9, s10));

  // 4-point overload
  cout << "  4-point API (0,0)-(2,2) vs (0,2)-(2,0): "
       << (segments_intersect(Point(0,0), Point(2,2), Point(0,2), Point(2,0))
           ? "INTERSECT" : "no") << endl;
  assert(segments_intersect(Point(0,0), Point(2,2), Point(0,2), Point(2,0)));

  cout << endl;
}


// ===================== Scenario 3 =====================

void scenario_exact_intersection()
{
  cout << "=== Scenario 3: Exact Intersection Points ===" << endl;
  cout << endl;
  cout << "All intersection points are computed in exact rational arithmetic" << endl;
  cout << "(mpq_class), so there is no floating-point rounding error." << endl;
  cout << endl;

  // Simple X: result is (1,1)
  Segment s1(Point(0, 0), Point(2, 2));
  Segment s2(Point(0, 2), Point(2, 0));
  Point p1 = segment_intersection_point(s1, s2);
  cout << "  (0,0)-(2,2) x (0,2)-(2,0) = " << p1.to_string() << endl;
  assert(p1.get_x() == 1 && p1.get_y() == 1);

  // Exact rational: intersection at (3/2, 0)
  Segment h(Point(0, 0), Point(3, 0));
  Segment d(Point(1, 1), Point(2, -1));
  Point p2 = segment_intersection_point(h, d);
  cout << "  (0,0)-(3,0) x (1,1)-(2,-1) = " << p2.to_string()
       << "  [exact: x=" << p2.get_x() << "]" << endl;
  assert(p2.get_x() == Geom_Number(3, 2));
  assert(p2.get_y() == 0);

  // Non-trivial: intersection at (7/3, 2/3)
  Segment a(Point(0, 0), Point(7, 2));
  Segment b(Point(0, 3), Point(3, 0));
  Point p3 = segment_intersection_point(a, b);
  cout << "  (0,0)-(7,2) x (0,3)-(3,0) = " << p3.to_string()
       << "  [exact: x=" << p3.get_x() << " y=" << p3.get_y() << "]" << endl;
  assert(p3.get_x() == Geom_Number(7, 3));
  assert(p3.get_y() == Geom_Number(2, 3));

  // Vertical x diagonal
  Segment v(Point(3, 0), Point(3, 6));
  Segment diag(Point(0, 0), Point(6, 6));
  Point p4 = segment_intersection_point(v, diag);
  cout << "  Vertical x=3 x diagonal y=x: " << p4.to_string() << endl;
  assert(p4.get_x() == 3 && p4.get_y() == 3);

  cout << endl;
}


// ===================== Scenario 4 =====================

void scenario_road_network()
{
  cout << "=== Scenario 4: Road Network Crossing Analysis ===" << endl;
  cout << endl;
  cout << "Given a small network of road segments, detect which pairs cross." << endl;
  cout << endl;

  struct Road
  {
    const char * name;
    Segment seg;
  };

  Array<Road> roads;
  roads.append({"Main St", Segment(Point(0, 2), Point(10, 2))});
  roads.append({"Broadway", Segment(Point(3, 0), Point(3, 8))});
  roads.append({"Diagonal Av", Segment(Point(0, 0), Point(8, 6))});
  roads.append({"Park Rd", Segment(Point(6, 0), Point(6, 8))});
  const size_t n = roads.size();

  for (size_t i = 0; i < n; ++i)
    for (size_t j = i + 1; j < n; ++j)
      if (segments_intersect(roads[i].seg, roads[j].seg))
      {
        if (roads[i].seg.is_parallel_with(roads[j].seg))
        {
          cout << "  " << roads[i].name << " overlaps with "
               << roads[j].name << " (collinear)" << endl;
        }
        else
        {
          Point ix = segment_intersection_point(roads[i].seg, roads[j].seg);
          cout << "  " << roads[i].name << " crosses " << roads[j].name
               << " at " << ix.to_string() << endl;
        }
      }

  cout << endl;

  // Verify known crossings
  // Main St x Broadway at (3,2)
  Point ix1 = segment_intersection_point(roads[0].seg, roads[1].seg);
  assert(ix1.get_x() == 3 && ix1.get_y() == 2);

  // Main St x Diagonal Av: y=2 => 2 = (6/8)*x => x = 8/3
  Point ix2 = segment_intersection_point(roads[0].seg, roads[2].seg);
  assert(ix2.get_x() == Geom_Number(8, 3));

  cout << "  All crossing points verified with exact arithmetic." << endl;
  cout << endl;
}


// ===================== Scenario 5 =====================

void scenario_in_circle_in_delaunay_context()
{
  cout << "=== Scenario 5: in_circle() in a Delaunay Context ===" << endl;
  cout << endl;
  cout << "Checking local Delaunay legality using exact in-circle predicates." << endl;
  cout << endl;

  const Point a(0, 0), b(4, 0), c(0, 4);
  const Point d_inside(1, 1);
  const Point d_outside(5, 5);

  const InCircleResult r1 = in_circle(a, b, c, d_inside);
  const InCircleResult r2 = in_circle(a, b, c, d_outside);

  cout << "  in_circle((0,0),(4,0),(0,4),(1,1)) = " << in_circle_str(r1) << endl;
  cout << "  in_circle((0,0),(4,0),(0,4),(5,5)) = " << in_circle_str(r2) << endl;
  assert(r1 == InCircleResult::INSIDE);
  assert(r2 == InCircleResult::OUTSIDE);

  DynList<Point> pts;
  pts.append(a);
  pts.append(b);
  pts.append(c);
  pts.append(Point(4, 4));
  pts.append(Point(2, 1));

  DelaunayTriangulationBowyerWatson delaunay;
  const auto dt = delaunay(pts);

  cout << "  Delaunay triangles built from the same predicate logic: "
       << dt.triangles.size() << endl;
  assert(not dt.triangles.is_empty());
  cout << endl;
}


int main()
{
  print_banner("Robust Predicates");
  cout << endl;

  scenario_orientation();
  scenario_intersection_detection();
  scenario_exact_intersection();
  scenario_road_network();
  scenario_in_circle_in_delaunay_context();

  cout << "All scenarios completed successfully." << endl;
  cout << "STATUS: OK" << endl;
  return 0;
}
