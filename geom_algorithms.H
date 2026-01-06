/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

/** @file geom_algorithms.H
 *  @brief Computational geometry algorithms.
 *
 *  This file provides fundamental algorithms for computational geometry:
 *
 *  ## Triangulation
 *
 *  - **CuttingEarsTriangulation**: Triangulate a simple polygon using the
 *    ear-cutting algorithm. Complexity: O(n²)
 *
 *  ## Convex Hull
 *
 *  Three algorithms to compute the convex hull of a point set:
 *
 *  | Algorithm | Complexity | Best For |
 *  |-----------|------------|----------|
 *  | BruteForceConvexHull | O(n³) | Small sets, educational |
 *  | GiftWrappingConvexHull | O(nh) | Few hull points (h small) |
 *  | QuickHull | O(n log n) avg, O(n²) worst | General use |
 *
 *  where n = number of points, h = number of hull points.
 *
 *  ## Dependencies
 *
 *  Requires `polygon.H` for geometric primitives:
 *  - Point: 2D point with x, y coordinates
 *  - Segment: Line segment between two points
 *  - Polygon: Sequence of connected vertices
 *  - Triangle: Three-point polygon
 *  - Vertex: Point with connectivity information
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  #include <geom_algorithms.H>
 *
 *  // Create a point set
 *  DynList<Point> points;
 *  points.append(Point(0, 0));
 *  points.append(Point(4, 0));
 *  points.append(Point(4, 4));
 *  points.append(Point(0, 4));
 *  points.append(Point(2, 2));  // Interior point
 *
 *  // Compute convex hull
 *  QuickHull qh;
 *  Polygon hull = qh(points);  // Returns square (excludes interior point)
 *
 *  // Triangulate a polygon
 *  Polygon square;
 *  square.add_vertex(Point(0, 0));
 *  square.add_vertex(Point(4, 0));
 *  square.add_vertex(Point(4, 4));
 *  square.add_vertex(Point(0, 4));
 *  square.close();
 *
 *  CuttingEarsTriangulation triangulator;
 *  DynList<Triangle> triangles = triangulator(square);
 *  ```
 *
 *  @see polygon.H Geometric primitives
 *  @see Point, Segment, Polygon, Triangle
 *
 *  @ingroup Geometria
 *  @author Leandro Rabindranath León
 *  @author Alejandro J. Mujica
 */

# ifndef GEOM_ALGORITHMS_H
# define GEOM_ALGORITHMS_H

# include <polygon.H>
# include <htlist.H>
# include <tpl_dynSetTree.H>
# include <tpl_sort_utils.H>
# include <ah-errors.H>

namespace Aleph
{

// ============================================================================
// Triangulation Algorithms
// ============================================================================

/**
 * @brief Polygon triangulation using the ear-cutting algorithm.
 *
 * Decomposes a simple polygon into a set of non-overlapping triangles.
 * An "ear" is a triangle formed by three consecutive vertices where
 * the diagonal between the first and third vertices lies entirely
 * inside the polygon.
 *
 * ## Algorithm
 *
 * 1. Find all ears (vertices whose diagonal is inside the polygon)
 * 2. Remove an ear, creating a triangle
 * 3. Update the ear status of adjacent vertices
 * 4. Repeat until only a triangle remains
 *
 * ## Complexity
 *
 * - Time: O(n²) where n = number of vertices
 * - Space: O(n) for the ears set
 *
 * ## Requirements
 *
 * - Input must be a simple polygon (no self-intersections)
 * - Polygon must have at least 3 vertices
 * - Vertices should be in counter-clockwise order
 *
 * ## Example
 *
 * ```cpp
 * // Create a convex quadrilateral
 * Polygon quad;
 * quad.add_vertex(Point(0, 0));
 * quad.add_vertex(Point(4, 0));
 * quad.add_vertex(Point(4, 3));
 * quad.add_vertex(Point(0, 3));
 * quad.close();
 *
 * CuttingEarsTriangulation triangulator;
 * DynList<Triangle> triangles = triangulator(quad);
 * // Result: 2 triangles
 * ```
 *
 * @warning The input polygon is modified (vertices are removed).
 *          Pass a copy if you need to preserve the original.
 *
 * @see Triangle Output triangle type
 * @see Polygon Input polygon type
 *
 * @ingroup Geometria
 * @author Alejandro J. Mujica
 */
class CuttingEarsTriangulation
{
  using EarsSet = DynSetTree<const Vertex *, Treap_Rk>;

public:

  /**
   * @brief Check if a segment is a valid diagonal of the polygon.
   *
   * A diagonal is valid if it doesn't intersect any edge of the polygon
   * (except at endpoints).
   *
   * @param p The polygon.
   * @param s The candidate diagonal segment.
   * @return true if s is a valid diagonal.
   */
  static bool diagonalie(const Polygon & p, const Segment & s)
  {
    for (Polygon::Segment_Iterator it(p); it.has_curr(); it.next_ne())
      {
        if (Segment curr = it.get_current_segment(); 
            (curr.get_src_point() != s.get_src_point()) and
            (curr.get_tgt_point() != s.get_src_point()) and
            (curr.get_src_point() != s.get_tgt_point()) and
            (curr.get_tgt_point() != s.get_tgt_point()) and
            s.intersects_with(curr))
          return false;
      }
    return true;
  }

  /**
   * @brief Check if vertex b is inside the cone formed at vertex a.
   *
   * The cone is defined by the edges adjacent to a (from prev to next).
   *
   * @param p The polygon.
   * @param a The vertex forming the cone apex.
   * @param b The vertex to test.
   * @return true if b is visible from a within the polygon.
   */
  static bool in_cone(const Polygon & p, const Vertex & a, const Vertex & b)
  {
    // a0 -> a -> a1 are consecutive vertices
    const Vertex & a0 = p.get_prev_vertex(a);
    const Vertex & a1 = p.get_next_vertex(a);

    if (a0.is_to_left_on_from(a, a1))
      return a0.is_to_left_from(a, b) and a1.is_to_left_from(b, a);

    return not (a1.is_to_left_on_from(a, b) and
                a0.is_to_left_on_from(b, a));
  }

  /**
   * @brief Check if segment (a, b) is a valid internal diagonal.
   *
   * @param p The polygon.
   * @param a First vertex of the diagonal.
   * @param b Second vertex of the diagonal.
   * @return true if (a, b) is a valid diagonal.
   */
  static bool diagonal(const Polygon & p, const Vertex & a, const Vertex & b)
  {
    return in_cone(p, a, b) and in_cone(p, b, a) and
           diagonalie(p, Segment(a, b));
  }

  /**
   * @brief Initialize the set of ear vertices.
   *
   * @param p The polygon.
   * @return Set of pointers to ear vertices.
   */
  static EarsSet init_ears(const Polygon & p)
  {
    EarsSet ret;

    for (Polygon::Vertex_Iterator it(p); it.has_curr(); it.next_ne())
      {
        Vertex & curr = it.get_current_vertex();
        const Vertex & prev = p.get_prev_vertex(curr);
        const Vertex & next = p.get_next_vertex(curr);
        if (diagonal(p, prev, next))
          ret.insert(&curr);
      }

    return ret;
  }

public:

  /**
   * @brief Triangulate the polygon.
   *
   * @param p The polygon to triangulate (will be modified).
   * @return List of triangles forming the triangulation.
   *
   * @throws domain_error if the polygon has fewer than 3 vertices.
   *
   * @note The input polygon is consumed (vertices removed).
   */
  DynList<Triangle> operator ()(Polygon & p)
  {
    ah_domain_error_if(p.size() < 3) << "Polygon has less than 3 vertices";

    EarsSet ears = init_ears(p);

    DynList<Triangle> ret;

    while (p.size() > 3)
      {
        const Vertex *curr = ears.remove_pos(0);

        const Vertex & prev = p.get_prev_vertex(*curr);
        const Vertex & prev_prev = p.get_prev_vertex(prev);
        const Vertex & next = p.get_next_vertex(*curr);
        const Vertex & next_next = p.get_next_vertex(next);

        if (diagonal(p, prev_prev, next))
          ears.insert(&prev);
        else
          ears.remove(&prev);

        if (diagonal(p, prev, next_next))
          ears.insert(&next);
        else
          ears.remove(&next);

        ret.append(Triangle(prev, *curr, next));

        p.remove_vertex(*curr);
      }

    assert(p.size() == 3);

    const Vertex & a = p.get_first_vertex();
    const Vertex & b = a.next_vertex();
    const Vertex & c = b.next_vertex();

    ret.append(Triangle(a, b, c));

    return ret;
  }
};

// ============================================================================
// Convex Hull Algorithms
// ============================================================================

/**
 * @brief Brute force convex hull algorithm.
 *
 * Computes the convex hull by testing all pairs of points to find
 * extreme edges (edges where all other points are on one side).
 *
 * ## Algorithm
 *
 * For each pair of points (p, q):
 *   If all other points are to the left of segment (p, q),
 *   then (p, q) is an edge of the convex hull.
 *
 * ## Complexity
 *
 * - Time: O(n³) - checks n² pairs, each against n points
 * - Space: O(n) for the edge set
 *
 * ## Use Cases
 *
 * - Educational purposes
 * - Very small point sets (< 20 points)
 * - Verification of other algorithms
 *
 * @see GiftWrappingConvexHull O(nh) algorithm
 * @see QuickHull O(n log n) average algorithm
 *
 * @ingroup Geometria
 * @author Alejandro J. Mujica
 */
class BruteForceConvexHull
{
  struct CmpSegment
  {
    bool cmp_point(const Point & p1, const Point & p2) const
    {
      if (p1.get_x() < p2.get_x())
        return true;

      return not (p2.get_x() < p1.get_x()) and p1.get_y() < p2.get_y();
    }

    bool operator ()(const Segment & s1, const Segment & s2)
    {
      if (cmp_point(s1.get_src_point(), s2.get_src_point()))
        return true;

      return not (cmp_point(s2.get_src_point(), s1.get_src_point())) and
             cmp_point(s1.get_tgt_point(), s2.get_tgt_point());
    }
  };

  using SegmentSet = DynSetTree<Segment, Treap_Rk, CmpSegment>;
  using PointIt = DynList<Point>::Iterator;

  bool are_all_points_on_left(DynList<Point> & l, const Segment & s)
  {
    for (PointIt it(l); it.has_curr(); it.next_ne())
      {
        const Point & p = it.get_curr();

        if (p.is_to_right_from(s))
          return false;
      }

    return true;
  }

  SegmentSet extreme_edges(DynList<Point> & point_set)
  {
    SegmentSet ret;

    for (PointIt i(point_set); i.has_curr(); i.next_ne())
      {
        const Point & p_i = i.get_curr();

        for (PointIt j(point_set); j.has_curr(); j.next_ne())
          {
            const Point & p_j = j.get_curr();

            if (p_i == p_j)
              continue;

            Segment s(p_i, p_j);

            if (are_all_points_on_left(point_set, s))
              ret.insert(s);
          }
      }

    return ret;
  }

public:

  /**
   * @brief Compute the convex hull of a point set.
   *
   * @param point_set The input points.
   * @return A closed polygon representing the convex hull.
   */
  Polygon operator ()(DynList<Point> & point_set)
  {
    Polygon ret;

    SegmentSet extremes = extreme_edges(point_set);

    Segment first_segment = extremes.remove_pos(0);
    ret.add_vertex(first_segment.get_src_point());
    ret.add_vertex(first_segment.get_tgt_point());

    while (true)
      {
        const Vertex & last_vertex = ret.get_last_vertex();

        Segment *ptr = extremes.find_ptr([&last_vertex](const Segment & s)
                                           {
                                             return s.get_src_point() == last_vertex;
                                           });

        assert(ptr != nullptr);

        if (ptr->get_tgt_point() == ret.get_first_vertex())
          break;

        ret.add_vertex(ptr->get_tgt_point());

        extremes.remove(*ptr);
      }

    ret.close();
    return ret;
  }
};

/**
 * @brief Gift wrapping (Jarvis march) convex hull algorithm.
 *
 * Computes the convex hull by starting from the lowest point and
 * "wrapping" around the point set, always selecting the point that
 * makes the smallest counter-clockwise angle.
 *
 * ## Algorithm
 *
 * 1. Start with the lowest point (guaranteed to be on hull)
 * 2. Find the point that makes the smallest angle with the last edge
 * 3. Add it to the hull
 * 4. Repeat until returning to the starting point
 *
 * ## Complexity
 *
 * - Time: O(nh) where n = total points, h = hull points
 * - Space: O(1) additional
 *
 * ## Use Cases
 *
 * - When the convex hull has few points (h << n)
 * - When output-sensitive algorithm is preferred
 *
 * @see QuickHull For general-purpose convex hull
 *
 * @ingroup Geometria
 * @author Alejandro J. Mujica
 */
class GiftWrappingConvexHull
{
  Point * get_lowest_point(DynList<Point> & point_set)
  {
    DynList<Point>::Iterator it(point_set);
    Point *ret = &it.get_curr();
    it.next();

    for (/* nothing */; it.has_curr(); it.next_ne())
      {
        Point & p = it.get_curr();

        if (p.get_y() < ret->get_y())
          ret = &p;
      }

    return ret;
  }

public:

  /**
   * @brief Compute the convex hull of a point set.
   *
   * @param point_set The input points.
   * @return A closed polygon representing the convex hull.
   */
  Polygon operator ()(DynList<Point> & point_set)
  {
    Polygon ret;

    Point *lowest_point = get_lowest_point(point_set);
    Point *p_i = lowest_point;
    ret.add_vertex(*lowest_point);

    Segment last_segment = Segment(Point(0, 0), Point(1, 0));

    while (true)
      {
        Point *p_k = nullptr;

        double min_angle = std::numeric_limits<double>::max();

        for (DynList<Point>::Iterator it(point_set); it.has_curr(); it.next_ne())
          {
            Point & p_j = it.get_curr();

            if (p_j == *p_i)
              continue;

            double angle =
                Segment(*p_i, p_j).counterclockwise_angle_with(last_segment);

            if (angle < min_angle)
              {
                min_angle = angle;
                p_k = &p_j;
              }
          }

        assert(p_k != nullptr);

        if (p_k == lowest_point)
          break;

        ret.add_vertex(*p_k);

        last_segment = ret.get_last_segment();

        p_i = p_k;
      }

    ret.close();
    return ret;
  }
};

/**
 * @brief QuickHull convex hull algorithm.
 *
 * Computes the convex hull using a divide-and-conquer approach similar
 * to QuickSort. Recursively finds the farthest point from a line and
 * partitions the remaining points.
 *
 * ## Algorithm
 *
 * 1. Find leftmost and rightmost points (guaranteed on hull)
 * 2. Partition points into those above and below the line
 * 3. For each partition, find the farthest point from the line
 * 4. Recursively process the sub-partitions
 *
 * ## Complexity
 *
 * - Time: O(n log n) average, O(n²) worst case
 * - Space: O(n) for recursive calls
 *
 * ## Use Cases
 *
 * - General-purpose convex hull computation
 * - Large point sets
 * - When average-case performance matters more than worst-case
 *
 * @ingroup Geometria
 * @author Alejandro J. Mujica
 */
class QuickHull
{
  Point get_fartest_point(DynList<Point> & point_set, const Segment & s)
  {
    Geom_Number max_distance = 0;
    Point ret;

    for (DynList<Point>::Iterator it(point_set); it.has_curr(); it.next_ne())
      {
        const Point & p = it.get_curr();

        Segment s1 = s.get_perpendicular(p);

        Geom_Number sz = s1.size();

        if (sz > max_distance)
          {
            ret = p;
            max_distance = sz;
          }
      }

    return ret;
  }

  std::pair<DynList<Point>, DynList<Point>>
  get_right_points(DynList<Point> & point_set,
                   const Point & a, const Point & b, const Point & c)
  {
    std::pair<DynList<Point>, DynList<Point>> ret;

    while (not point_set.is_empty())
      {
        Point p = point_set.remove_first();

        if (p != a and p != c and p.is_to_right_from(a, c))
          {
            ret.first.append(p);
            continue;
          }

        if (p != c and p != b and p.is_to_right_from(c, b))
          ret.second.append(p);
      }

    return ret;
  }

  DynList<Point> quick_hull(DynList<Point> & point_set, const Point & a,
                            const Point & b)
  {
    if (point_set.is_empty())
      return DynList<Point>();

    Point c = get_fartest_point(point_set, Segment(a, b));

    auto r = get_right_points(point_set, a, b, c);

    DynList<Point> ret = quick_hull(r.first, a, c);
    DynList<Point> tmp = quick_hull(r.second, c, b);
    ret.append(c);
    ret.concat(tmp);

    return ret;
  }

  std::pair<Point, Point> search_extremes(DynList<Point> & point_set)
  {
    DynList<Point>::Iterator it(point_set);
    Point leftmost = it.get_curr();
    Point rightmost = it.get_curr();
    it.next();

    for (/* nothing */; it.has_curr(); it.next_ne())
      {
        const Point & p = it.get_curr();

        if (p.get_x() < leftmost.get_x())
          leftmost = p;

        if (p.get_x() > rightmost.get_x())
          rightmost = p;
      }

    return std::make_pair(leftmost, rightmost);
  }

  std::pair<DynList<Point>, DynList<Point>>
  partition(DynList<Point> & point_set, const Point & a, const Point & b)
  {
    std::pair<DynList<Point>, DynList<Point>> ret;

    for (DynList<Point>::Iterator it(point_set); it.has_curr(); it.next_ne())
      {
        const Point & p = it.get_curr();

        if (p.is_to_right_from(a, b))
          ret.first.append(p);
        else
          ret.second.append(p);
      }

    return ret;
  }

public:

  /**
   * @brief Compute the convex hull of a point set.
   *
   * @param point_set The input points.
   * @return A closed polygon representing the convex hull.
   */
  Polygon operator ()(DynList<Point> & point_set)
  {
    Polygon ret;

    auto e = search_extremes(point_set);
    auto p = partition(point_set, e.first, e.second);

    DynList<Point> s1 = quick_hull(p.first, e.first, e.second);
    DynList<Point> s2 = quick_hull(p.second, e.second, e.first);

    DynList<Point> convex_set;
    convex_set.append(e.first);
    convex_set.concat(s1);
    convex_set.append(e.second);
    convex_set.concat(s2);

    for (DynList<Point>::Iterator it(convex_set); it.has_curr(); it.next_ne())
      ret.add_vertex(it.get_curr());

    ret.close();
    return ret;
  }
};

} // namespace Aleph

# endif // GEOM_ALGORITHMS_H
