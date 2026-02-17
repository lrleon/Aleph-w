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
 * @file segment_segment_intersection_example.cc
 * @brief Dedicated O(1) segment-segment intersection demo.
 *
 * Shows the direct 2-segment API (`SegmentSegmentIntersection`) with exact
 * arithmetic and explicit classification:
 * - no intersection
 * - unique intersection point
 * - collinear overlap interval
 *
 * @ingroup Geometry
 */

#include <cassert>
#include <iostream>
#include <geom_algorithms.H>

using namespace std;

static const char * kind_str(SegmentSegmentIntersection::Kind kind)
{
  switch (kind)
  {
    case SegmentSegmentIntersection::Kind::NONE: return "NONE";
    case SegmentSegmentIntersection::Kind::POINT: return "POINT";
    case SegmentSegmentIntersection::Kind::OVERLAP: return "OVERLAP";
  }
  return "?";
}

static void print_case(const char * name,
                       const Segment & a,
                       const Segment & b,
                       const SegmentSegmentIntersection::Result & result)
{
  cout << "Case: " << name << '\n';
  cout << "  A = " << a.get_src_point().to_string()
       << " -> " << a.get_tgt_point().to_string() << '\n';
  cout << "  B = " << b.get_src_point().to_string()
       << " -> " << b.get_tgt_point().to_string() << '\n';
  cout << "  kind = " << kind_str(result.kind) << '\n';

  if (result.kind == SegmentSegmentIntersection::Kind::POINT)
    cout << "  point = " << result.point.to_string() << '\n';
  else if (result.kind == SegmentSegmentIntersection::Kind::OVERLAP)
    cout << "  overlap = "
         << result.overlap.get_src_point().to_string()
         << " -> "
         << result.overlap.get_tgt_point().to_string() << '\n';

  cout << '\n';
}

int main()
{
  cout << "[Aleph Geometry Example] Dedicated O(1) Segment-Segment Intersection\n";
  cout << "====================================================================\n\n";

  SegmentSegmentIntersection pair_ix;

  // 1) Proper crossing.
  {
    const Segment a(Point(0, 0), Point(4, 4));
    const Segment b(Point(0, 4), Point(4, 0));
    const auto result = pair_ix(a, b);
    print_case("Proper cross", a, b, result);
    assert(result.kind == SegmentSegmentIntersection::Kind::POINT);
    assert(result.point == Point(2, 2));
  }

  // 2) Endpoint touch.
  {
    const Segment a(Point(0, 0), Point(4, 0));
    const Segment b(Point(4, 0), Point(4, 3));
    const auto result = pair_ix(a, b);
    print_case("Endpoint touching", a, b, result);
    assert(result.kind == SegmentSegmentIntersection::Kind::POINT);
    assert(result.point == Point(4, 0));
  }

  // 3) Collinear overlap (interval result).
  {
    const Segment a(Point(0, 0), Point(6, 0));
    const Segment b(Point(2, 0), Point(5, 0));
    const auto result = pair_ix(a, b);
    print_case("Collinear overlap", a, b, result);
    assert(result.kind == SegmentSegmentIntersection::Kind::OVERLAP);
    assert(result.overlap.get_src_point() == Point(2, 0));
    assert(result.overlap.get_tgt_point() == Point(5, 0));
  }

  // 4) Disjoint segments.
  {
    const Segment a(Point(0, 0), Point(1, 1));
    const Segment b(Point(3, 0), Point(4, 1));
    const auto result = pair_ix(a, b);
    print_case("Disjoint", a, b, result);
    assert(result.kind == SegmentSegmentIntersection::Kind::NONE);
    assert(not result.intersects());
  }

  // 5) Free-function convenience + exact rational point.
  {
    const Segment a(Point(0, 0), Point(3, 0));
    const Segment b(Point(1, 1), Point(2, -1));
    const auto result = segment_segment_intersection(a, b);
    print_case("Free function exact rational", a, b, result);
    assert(result.kind == SegmentSegmentIntersection::Kind::POINT);
    assert(result.point.get_x() == Geom_Number(3, 2));
    assert(result.point.get_y() == Geom_Number(0));
  }

  cout << "STATUS: OK\n";
  return 0;
}
