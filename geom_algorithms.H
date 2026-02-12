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
 *  Convex hull algorithms to compute the hull of a point set:
 *
 *  | Algorithm | Complexity | Best For |
 *  |-----------|------------|----------|
 *  | AndrewMonotonicChainConvexHull | O(n log n) | General use, deterministic |
 *  | GrahamScanConvexHull | O(n log n) | General use, angle-based |
 *  | BruteForceConvexHull | O(n³) | Small sets, educational |
 *  | GiftWrappingConvexHull | O(nh) | Few hull points (h small) |
 *  | QuickHull | O(n log n) avg, O(n²) worst | General use |
 *
 *  where n = number of points, h = number of hull points.
 *
 *  ## Closest Pair
 *
 *  - **ClosestPairDivideAndConquer**: Computes the closest pair of points in
 *    O(n log n) time using divide & conquer.
 *
 *  ## Rotating Calipers (Convex Polygons)
 *
 *  - **RotatingCalipersConvexPolygon**: Computes diameter in O(n) and minimum
 *    width in O(n²) for convex polygons.
 *
 *  ## Point-in-Polygon
 *
 *  - **PointInPolygonWinding**: Exact point location in simple polygons
 *    (inside/boundary/outside) using winding number.
 *
 *  ## Polygon Intersection (Basic)
 *
 *  - **ConvexPolygonIntersectionBasic**: Exact convex-convex polygon
 *    intersection via Sutherland-Hodgman clipping.
 *
 *  ## Half-Plane Intersection
 *
 *  - **HalfPlaneIntersection**: Exact bounded half-plane intersection using
 *    angle-sorted deque (O(n log n)).
 *
 *  ## Delaunay / Voronoi
 *
 *  - **DelaunayTriangulationBowyerWatson**: Incremental Delaunay triangulation
 *    using Bowyer-Watson.
 *  - **VoronoiDiagramFromDelaunay**: Voronoi diagram derived as the dual of a
 *    Delaunay triangulation (bounded edges + unbounded rays).
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
 *  @ingroup Geometry
 *  @author Leandro Rabindranath León
 *  @author Alejandro J. Mujica
 */

# ifndef GEOM_ALGORITHMS_H
# define GEOM_ALGORITHMS_H

# include <polygon.H>
# include <htlist.H>
# include <tpl_dynDlist.H>
# include <tpl_dynSetTree.H>
# include <tpl_sort_utils.H>
# include <ah-errors.H>
#include <utility>

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
   * @ingroup Geometry
   * @author Leandro Rabindranath León
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
    static bool diagonalize(const Polygon & p, const Segment & s)
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
             diagonalize(p, Segment(a.to_point(), b.to_point()));
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
          if (const Vertex & next = p.get_next_vertex(curr); diagonal(p, prev, next))
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
    DynList<Triangle> operator ()(Polygon & p) const
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

          ret.append(Triangle(prev.to_point(), curr->to_point(), next.to_point()));

          p.remove_vertex(*curr);
        }

      assert(p.size() == 3);

      const Vertex & a = p.get_first_vertex();
      const Vertex & b = a.next_vertex();
      const Vertex & c = b.next_vertex();

      ret.append(Triangle(a.to_point(), b.to_point(), c.to_point()));

      return ret;
    }
  };

  // ============================================================================
  // Closest Pair of Points
  // ============================================================================

  /**
   * @brief Closest pair of points via divide and conquer.
   *
   * Computes the pair of points with minimum Euclidean distance using a classic
   * O(n log n) divide-and-conquer algorithm.
   *
   * ## Policy
   *
   * - Uses exact arithmetic (`Geom_Number`) for distance comparisons.
   * - Duplicate points are detected after sorting; if found, returns distance 0.
   * - Throws if the input has fewer than 2 points.
   *
   * ## Complexity
   *
   * - Time: O(n log n)
   * - Space: O(n)
   *
   * @ingroup Geometry
   */
  class ClosestPairDivideAndConquer
  {
  public:
    struct Result
    {
      Point first;
      Point second;
      Geom_Number distance_squared;
    };

  private:
    struct LexicographicCmp
    {
      bool operator ()(const Point & p1, const Point & p2) const
      {
        if (p1.get_x() < p2.get_x())
          return true;

        if (p2.get_x() < p1.get_x())
          return false;

        return p1.get_y() < p2.get_y();
      }
    };

    struct ByYCmp
    {
      bool operator ()(const Point & p1, const Point & p2) const
      {
        if (p1.get_y() < p2.get_y())
          return true;

        if (p2.get_y() < p1.get_y())
          return false;

        if (p1.get_x() < p2.get_x())
          return true;

        if (p2.get_x() < p1.get_x())
          return false;

        return false;
      }
    };

    [[nodiscard]] static Geom_Number dist2(const Point & a, const Point & b)
    {
      return a.distance_squared_to(b);
    }

    [[nodiscard]] static Result make_result(const Point & a, const Point & b)
    {
      return {a, b, dist2(a, b)};
    }

    [[nodiscard]] static Result brute_force(const Array<Point> & px,
                                            const size_t l, const size_t r)
    {
      assert(r - l >= 2);

      Result best = make_result(px(l), px(l + 1));

      for (size_t i = l; i < r; ++i)
        for (size_t j = i + 1; j < r; ++j)
          {
            Result cand = make_result(px(i), px(j));
            if (cand.distance_squared < best.distance_squared)
              best = cand;
          }

      return best;
    }

    [[nodiscard]] static Result recurse(const Array<Point> & px, const size_t l,
                                        const size_t r, Array<Point> & py)
    {
      const size_t n = r - l;
      if (n <= 3)
        return brute_force(px, l, r);

      const size_t mid = l + n / 2;
      const Point & mid_point = px(mid);

      Array<Point> left_py;
      Array<Point> right_py;
      left_py.reserve(mid - l);
      right_py.reserve(r - mid);

      LexicographicCmp x_cmp;
      for (size_t i = 0; i < py.size(); ++i)
        {
          const Point & p = py(i);
          if (x_cmp(p, mid_point))
            left_py.append(p);
          else
            right_py.append(p);
        }

      const Result best_left = recurse(px, l, mid, left_py);
      const Result best_right = recurse(px, mid, r, right_py);
      Result best = best_left.distance_squared < best_right.distance_squared ?
                      best_left :
                      best_right;

      Array<Point> strip;
      strip.reserve(py.size());
      for (size_t i = 0; i < py.size(); ++i)
        {
          const Point & p = py(i);
          if (const Geom_Number dx = p.get_x() - mid_point.get_x(); dx * dx < best.distance_squared)
            strip.append(p);
        }

      for (size_t i = 0; i < strip.size(); ++i)
        for (size_t j = i + 1; j < strip.size(); ++j)
          {
            if (const Geom_Number dy = strip(j).get_y() - strip(i).get_y(); dy * dy >= best.distance_squared)
              break;

            if (Result cand = make_result(strip(i), strip(j)); cand.distance_squared < best.distance_squared)
              best = cand;
          }

      return best;
    }

  public:
    /**
     * @brief Compute the closest pair of points.
     *
     * @param point_set Input set of points.
     * @return The closest pair and its squared distance.
     *
     * @throws domain_error if point_set has fewer than 2 points.
     */
    [[nodiscard]] Result operator ()(DynList<Point> & point_set) const
    {
      Array<Point> px;
      for (DynList<Point>::Iterator it(point_set); it.has_curr(); it.next_ne())
        px.append(it.get_curr());

      ah_domain_error_if(px.size() < 2)
        << "Closest pair requires at least 2 points";

      quicksort_op(px, LexicographicCmp());

      // Early-exit on duplicate points: exact minimum distance is zero.
      for (size_t i = 1; i < px.size(); ++i)
        if (px(i) == px(i - 1))
          return {px(i - 1), px(i), 0};

      Array<Point> py = px;
      quicksort_op(py, ByYCmp());

      return recurse(px, 0, px.size(), py);
    }

    /**
     * @brief Convenience wrapper returning the closest segment.
     *
     * @param point_set Input set of points.
     * @return Segment between the closest pair.
     */
    [[nodiscard]] Segment closest_segment(DynList<Point> & point_set) const
    {
      Result r = (*this)(point_set);
      return {r.first, r.second};
    }
  };

  // ============================================================================
  // Rotating Calipers (Convex Polygon Metrics)
  // ============================================================================

  /**
   * @brief Rotating calipers metrics for convex polygons.
   *
   * Computes two standard convex-polygon metrics:
   *
   * - Diameter (farthest pair of vertices).
   * - Minimum width (minimum distance between two parallel supporting lines).
   *
   * ## Requirements
   *
   * - The input polygon must be closed.
   * - The input polygon must be convex (collinear consecutive triples allowed).
   *
   * ## Complexity
   *
   * - Diameter: O(n) time.
   * - Minimum width: O(n^2) time (exact baseline scan).
   * - Space: O(n) for temporary vertex storage.
   *
   * @ingroup Geometry
   */
  class RotatingCalipersConvexPolygon
  {
  public:
    struct DiameterResult
    {
      Point first;
      Point second;
      Geom_Number distance_squared;
    };

    struct WidthResult
    {
      Point edge_first;
      Point edge_second;
      Point antipodal;
      Geom_Number width_squared;
    };

  private:
    [[nodiscard]] static Array<Point> extract_vertices(const Polygon & poly)
    {
      ah_domain_error_if(not poly.is_closed()) << "Polygon must be closed";

      ah_domain_error_if(poly.size() < 2) << "Rotating calipers requires at least 2 vertices";

      Array<Point> verts;
      verts.reserve(poly.size());
      for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
        verts.append(it.get_current_vertex());

      return verts;
    }

    [[nodiscard]] static bool is_convex(const Array<Point> & verts)
    {
      if (verts.size() < 3)
        return true;

      int sign = 0;
      const size_t n = verts.size();
      for (size_t i = 0; i < n; ++i)
        {
          const Point & a = verts(i);
          const Point & b = verts((i + 1) % n);
          const Point & c = verts((i + 2) % n);
          const Geom_Number turn = area_of_parallelogram(a, b, c);

          if (turn == 0)
            continue;

          const int curr = turn > 0 ? 1 : -1;
          if (sign == 0)
            sign = curr;
          else if (sign != curr)
            return false;
        }

      return true;
    }

    [[nodiscard]] static DiameterResult make_diameter(const Point & a, const Point & b)
    {
      return {a, b, a.distance_squared_to(b)};
    }

  public:
    /**
     * @brief Compute convex polygon diameter (farthest vertex pair).
     *
     * @param poly Closed convex polygon.
     * @return Farthest pair and squared distance.
     *
     * @throws domain_error if polygon is not closed, has < 2 vertices, or is non-convex.
     */
    static DiameterResult diameter(const Polygon & poly)
    {
      const Array<Point> verts = extract_vertices(poly);
      ah_domain_error_if(not is_convex(verts))
      << "Polygon must be convex";

      const size_t n = verts.size();
      if (n == 2)
        return make_diameter(verts(0), verts(1));

      auto edge_area = [&verts, n](const size_t i, const size_t k) -> Geom_Number
        {
          Geom_Number area =
              area_of_parallelogram(verts(i), verts((i + 1) % n), verts(k));
          if (area < 0)
            area = -area;
          return area;
        };

      size_t j = 1;
      while (edge_area(0, (j + 1) % n) > edge_area(0, j))
        j = (j + 1) % n;

      DiameterResult best = make_diameter(verts(0), verts(j));

      for (size_t i = 0; i < n; ++i)
        {
          const size_t ni = (i + 1) % n;
          while (edge_area(i, (j + 1) % n) > edge_area(i, j))
            j = (j + 1) % n;

          if (DiameterResult cand1 = make_diameter(verts(i), verts(j)); cand1.distance_squared > best.distance_squared)
            best = cand1;

          if (DiameterResult cand2 = make_diameter(verts(ni), verts(j)); cand2.distance_squared > best.distance_squared)
            best = cand2;
        }

      return best;
    }

    /**
     * @brief Compute the minimum width of a closed convex polygon.
     *
     * The width for an edge is the maximum perpendicular distance from vertices
     * to the edge line; the minimum width is the minimum of these edge widths.
     *
     * @param poly Closed convex polygon.
     * @return Supporting edge, antipodal vertex, and squared width.
     *
     * @note This implementation uses an exact O(n^2) scan over edges and
     *       vertices for robustness.
     *
     * @throws domain_error if the polygon is not closed, has < 2 vertices, or is non-convex.
     */
    static WidthResult minimum_width(const Polygon & poly)
    {
      const Array<Point> verts = extract_vertices(poly);
      ah_domain_error_if(not is_convex(verts)) << "Polygon must be convex";

      const size_t n = verts.size();
      if (n == 2)
        return {verts(0), verts(1), verts(0), 0};

      auto area_to_edge = [&verts, n](const size_t i, const size_t k) -> Geom_Number
        {
          Geom_Number area =
              area_of_parallelogram(verts(i), verts((i + 1) % n), verts(k));
          if (area < 0)
            area = -area;
          return area;
        };

      auto edge_len2 = [&verts, n](const size_t i) -> Geom_Number
        {
          return verts(i).distance_squared_to(verts((i + 1) % n));
        };

      size_t best_i = 0;
      size_t best_ni = 1;
      size_t best_j = 0;
      Geom_Number best_width_sq = 0;
      bool initialized = false;

      for (size_t i = 0; i < n; ++i)
        {
          const size_t ni = (i + 1) % n;

          const Geom_Number len2 = edge_len2(i);
          if (len2 == 0)
            continue;

          size_t j = 0;
          Geom_Number max_num = area_to_edge(i, 0);
          for (size_t k = 1; k < n; ++k)
            {
              if (const Geom_Number cand_num = area_to_edge(i, k); cand_num > max_num)
                {
                  max_num = cand_num;
                  j = k;
                }
            }

          if (const Geom_Number width_sq = (max_num * max_num) / len2; not initialized or width_sq < best_width_sq)
            {
              best_i = i;
              best_ni = ni;
              best_j = j;
              best_width_sq = width_sq;
              initialized = true;
            }
        }

      if (not initialized)
        return {verts(0), verts(1), verts(0), 0};

      return {verts(best_i), verts(best_ni), verts(best_j), best_width_sq};
    }
  };

  // ============================================================================
  // Point-in-Polygon
  // ============================================================================

  /**
   * @brief Exact point-in-polygon classification via winding number.
   *
   * Classifies a point relative to a simple closed polygon as:
   * - Inside
   * - Boundary
   * - Outside
   *
   * Uses exact geometric predicates (`orientation`, `on_segment`) with
   * `Geom_Number`, avoiding floating-point robustness issues.
   *
   * ## Requirements
   *
   * - Polygon must be closed.
   * - Polygon must have at least 3 vertices.
   * - Polygon should be simple (non-self-intersecting).
   *
   * ## Complexity
   *
   * - Time: O(n), where n is number of polygon edges.
   * - Space: O(1).
   *
   * @ingroup Geometry
   */
  class PointInPolygonWinding
  {
  public:
    enum class Location
    {
      Outside,
      Boundary,
      Inside
    };

    /**
     * @brief Classify point location with respect to a polygon.
     *
     * @param poly Closed simple polygon.
     * @param p Query point.
     * @return Location::Outside, Location::Boundary, or Location::Inside.
     *
     * @throws domain_error if polygon is open or has fewer than 3 vertices.
     */
    static Location locate(const Polygon & poly, const Point & p)
    {
      ah_domain_error_if(not poly.is_closed()) << "Polygon must be closed";

      ah_domain_error_if(poly.size() < 3) << "Point-in-polygon requires at least 3 vertices";

      int winding_number = 0;

      for (Polygon::Segment_Iterator it(poly); it.has_curr(); it.next_ne())
        {
          const Segment edge = it.get_current_segment();
          const Point & a = edge.get_src_point();
          const Point & b = edge.get_tgt_point();

          if (on_segment(edge, p))
            return Location::Boundary;

          if (a.get_y() <= p.get_y())
            {
              if (b.get_y() > p.get_y() and
                  orientation(a, b, p) == Orientation::CCW)
                ++winding_number;
            }
          else
            {
              if (b.get_y() <= p.get_y() and
                  orientation(a, b, p) == Orientation::CW)
                --winding_number;
            }
        }

      return winding_number == 0 ? Location::Outside : Location::Inside;
    }

    /**
     * @brief Return true if the point is inside or on the boundary.
     *
     * @param poly Closed simple polygon.
     * @param p Query point.
     * @return true iff location is Inside or Boundary.
     */
    [[nodiscard]] static bool contains(const Polygon & poly, const Point & p)
    {
      const Location loc = locate(poly, p);
      return loc != Location::Outside;
    }

    /**
     * @brief Return true only for strict interior points.
     *
     * @param poly Closed simple polygon.
     * @param p Query point.
     * @return true iff location is Inside.
     */
    [[nodiscard]] static bool strictly_contains(const Polygon & poly,
                                                const Point & p)
    {
      return locate(poly, p) == Location::Inside;
    }
  };

  // ============================================================================
  // Polygon Intersection (Basic)
  // ============================================================================

  /**
   * @brief Basic exact intersection for closed convex polygons.
   *
   * Computes the intersection of two closed convex polygons using the
   * Sutherland-Hodgman clipping algorithm.
   *
   * ## Scope
   *
   * - Convex subject polygon.
   * - Convex clip polygon.
   * - Exact arithmetic (`Geom_Number`) through robust predicates.
   *
   * ## Complexity
   *
   * - Time: O(n * m), where n and m are vertex counts.
   * - Space: O(n + m).
   *
   * @ingroup Geometry
   */
  class ConvexPolygonIntersectionBasic
  {
    [[nodiscard]] static Array<Point> extract_vertices(const Polygon & poly)
    {
      ah_domain_error_if(not poly.is_closed()) << "Polygon must be closed";

      ah_domain_error_if(poly.size() < 3) << "Polygon must have at least 3 vertices";

      Array<Point> verts;
      verts.reserve(poly.size());
      for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
        verts.append(it.get_current_vertex());

      return verts;
    }

    [[nodiscard]] static Geom_Number signed_double_area(const Array<Point> & verts)
    {
      Geom_Number sum = 0;
      const size_t n = verts.size();
      for (size_t i = 0; i < n; ++i)
        {
          const Point & a = verts(i);
          const Point & b = verts((i + 1) % n);
          sum += a.get_x() * b.get_y() - a.get_y() * b.get_x();
        }

      return sum;
    }

    [[nodiscard]] static bool is_convex(const Array<Point> & verts)
    {
      if (verts.size() < 3)
        return false;

      int sign = 0;
      const size_t n = verts.size();
      for (size_t i = 0; i < n; ++i)
        {
          const Orientation o = orientation(verts(i),
                                            verts((i + 1) % n),
                                            verts((i + 2) % n));
          if (o == Orientation::COLLINEAR)
            continue;

          const int curr = (o == Orientation::CCW) ? 1 : -1;
          if (sign == 0)
            sign = curr;
          else if (sign != curr)
            return false;
        }

      return true;
    }

    [[nodiscard]] static bool inside_half_plane(const Point & p, const Point & a,
                                                const Point & b,
                                                const bool clip_ccw)
    {
      const Orientation o = orientation(a, b, p);
      return clip_ccw ? (o != Orientation::CW) : (o != Orientation::CCW);
    }

    [[nodiscard]] static Point line_intersection(const Point & s, const Point & e,
                                                 const Point & a, const Point & b)
    {
      const Geom_Number rx = e.get_x() - s.get_x();
      const Geom_Number ry = e.get_y() - s.get_y();
      const Geom_Number sx = b.get_x() - a.get_x();
      const Geom_Number sy = b.get_y() - a.get_y();

      const Geom_Number den = rx * sy - ry * sx;
      if (den == 0)
        {
          if (orientation(a, b, s) == Orientation::COLLINEAR)
            return s;
          if (orientation(a, b, e) == Orientation::COLLINEAR)
            return e;
          return s;
        }

      const Geom_Number qpx = a.get_x() - s.get_x();
      const Geom_Number qpy = a.get_y() - s.get_y();
      const Geom_Number t = (qpx * sy - qpy * sx) / den;

      return {s.get_x() + t * rx, s.get_y() + t * ry};
    }

    static void push_clean(Array<Point> & out, const Point & p)
    {
      if (out.is_empty())
        {
          out.append(p);
          return;
        }

      if (out.get_last() == p)
        return;

      while (out.size() >= 2)
        {
          const Point & a = out(out.size() - 2);
          const Point & b = out(out.size() - 1);
          if (orientation(a, b, p) != Orientation::COLLINEAR)
            break;

          if (const Segment ab(a, b); on_segment(ab, p))
            return;

          out.remove_last();
        }

      out.append(p);
    }

    [[nodiscard]] static Array<Point> normalize_vertices(const Array<Point> & pts)
    {
      Array<Point> ret;
      ret.reserve(pts.size());
      for (size_t i = 0; i < pts.size(); ++i)
        push_clean(ret, pts(i));

      if (ret.size() > 1 and ret(0) == ret.get_last())
        ret.remove_last();

      return ret;
    }

    [[nodiscard]] static Polygon build_polygon(const Array<Point> & pts)
    {
      Polygon ret;
      Array<Point> clean = normalize_vertices(pts);

      for (size_t i = 0; i < clean.size(); ++i)
        ret.add_vertex(clean(i));

      if (ret.size() >= 2)
        ret.close();

      return ret;
    }

  public:
    /**
     * @brief Intersect two closed convex polygons.
     *
     * @param subject Convex subject polygon.
     * @param clip Convex clipping polygon.
     * @return Intersection polygon (possibly empty, segment, or polygonal area).
     *
     * @throws domain_error if any polygon is open, has < 3 vertices, or is non-convex.
     */
    [[nodiscard]] Polygon operator ()(const Polygon & subject,
                                      const Polygon & clip) const
    {
      const Array<Point> subj = extract_vertices(subject);
      const Array<Point> clp = extract_vertices(clip);

      ah_domain_error_if(not is_convex(subj)) << "Subject polygon must be convex";
      ah_domain_error_if(not is_convex(clp)) << "Clip polygon must be convex";

      const Geom_Number clip_area2 = signed_double_area(clp);
      ah_domain_error_if(clip_area2 == 0) << "Clip polygon is degenerate";

      const bool clip_ccw = clip_area2 > 0;

      Array<Point> output = subj;

      for (size_t i = 0; i < clp.size(); ++i)
        {
          if (output.is_empty())
            break;

          const Point & a = clp(i);
          const Point & b = clp((i + 1) % clp.size());

          const Array<Point> input = output;
          output = Array<Point>();
          output.reserve(input.size() + 2);

          Point s = input.get_last();
          bool s_inside = inside_half_plane(s, a, b, clip_ccw);

          for (size_t j = 0; j < input.size(); ++j)
            {
              const Point & e = input(j);
              const bool e_inside = inside_half_plane(e, a, b, clip_ccw);

              if (e_inside)
                {
                  if (not s_inside)
                    push_clean(output, line_intersection(s, e, a, b));
                  push_clean(output, e);
                }
              else if (s_inside)
                push_clean(output, line_intersection(s, e, a, b));

              s = e;
              s_inside = e_inside;
            }

          output = normalize_vertices(output);
        }

      return build_polygon(output);
    }
  };

  // ============================================================================
  // Half-Plane Intersection
  // ============================================================================

  /**
   * @brief Exact bounded intersection of half-planes.
   *
   * Each half-plane is represented by a directed line (p -> q); the feasible side
   * is the left side of that line (including boundary).
   *
   * This class computes the bounded polygon defined by the intersection of a set
   * of half-planes using the classic angle-sorted deque algorithm.
   *
   * ## Complexity
   *
   * - Time: O(n log n) for n half-planes.
   * - Space: O(n).
   *
   * @ingroup Geometry
   */
  class HalfPlaneIntersection
  {
  public:
    struct HalfPlane
    {
      Point p;
      Point q;

      HalfPlane() = default;

      HalfPlane(Point p_, Point q_) : p(std::move(p_)), q(std::move(q_)) {}

      [[nodiscard]] Geom_Number dx() const { return q.get_x() - p.get_x(); }

      [[nodiscard]] Geom_Number dy() const { return q.get_y() - p.get_y(); }

      [[nodiscard]] Geom_Number offset() const
      {
        // n = (-dy, dx), inequality n.x*x + n.y*y >= offset
        return -dy() * p.get_x() + dx() * p.get_y();
      }

      [[nodiscard]] bool outside(const Point & x) const
      {
        return orientation(p, q, x) == Orientation::CW;
      }
    };

  private:
    [[nodiscard]] static bool upper_half(const HalfPlane & h)
    {
      return (h.dy() > 0) or (h.dy() == 0 and h.dx() >= 0);
    }

    [[nodiscard]] static Geom_Number cross_dir(const HalfPlane & a,
                                               const HalfPlane & b)
    {
      return a.dx() * b.dy() - a.dy() * b.dx();
    }

    [[nodiscard]] static Geom_Number dot_dir(const HalfPlane & a,
                                             const HalfPlane & b)
    {
      return a.dx() * b.dx() + a.dy() * b.dy();
    }

    [[nodiscard]] static bool same_direction(const HalfPlane & a,
                                             const HalfPlane & b)
    {
      return cross_dir(a, b) == 0 and dot_dir(a, b) > 0;
    }

    [[nodiscard]] static bool parallel(const HalfPlane & a, const HalfPlane & b)
    {
      return cross_dir(a, b) == 0;
    }

    [[nodiscard]] static Point line_intersection(const HalfPlane & a,
                                                 const HalfPlane & b)
    {
      const Geom_Number rx = a.dx();
      const Geom_Number ry = a.dy();
      const Geom_Number sx = b.dx();
      const Geom_Number sy = b.dy();

      const Geom_Number den = rx * sy - ry * sx;
      ah_domain_error_if(den == 0) << "Parallel half-plane boundaries";

      const Geom_Number qpx = b.p.get_x() - a.p.get_x();
      const Geom_Number qpy = b.p.get_y() - a.p.get_y();
      const Geom_Number t = (qpx * sy - qpy * sx) / den;

      return {a.p.get_x() + t * rx, a.p.get_y() + t * ry};
    }

    static void push_clean(Array<Point> & out, const Point & p)
    {
      if (out.is_empty())
        {
          out.append(p);
          return;
        }

      if (out.get_last() == p)
        return;

      while (out.size() >= 2)
        {
          const Point & a = out(out.size() - 2);
          const Point & b = out(out.size() - 1);
          if (orientation(a, b, p) != Orientation::COLLINEAR)
            break;

          const Segment ab(a, b);
          if (on_segment(ab, p))
            return;

          out.remove_last();
        }

      out.append(p);
    }

    [[nodiscard]] static Array<Point> normalize_vertices(const Array<Point> & pts)
    {
      Array<Point> ret;
      ret.reserve(pts.size());
      for (size_t i = 0; i < pts.size(); ++i)
        push_clean(ret, pts(i));

      if (ret.size() > 1 and ret(0) == ret.get_last())
        ret.remove_last();

      return ret;
    }

    [[nodiscard]] static Polygon build_polygon(const Array<Point> & pts)
    {
      Polygon ret;
      const Array<Point> clean = normalize_vertices(pts);

      for (size_t i = 0; i < clean.size(); ++i)
        ret.add_vertex(clean(i));

      if (ret.size() >= 2)
        ret.close();

      return ret;
    }

  public:
    /**
     * @brief Build half-planes from edges of a closed convex polygon.
     *
     * Polygon edges are used with their native direction; for CCW polygons this
     * corresponds to inward half-planes.
     *
     * @param poly Closed convex polygon.
     * @return Array of edge half-planes.
     */
    [[nodiscard]] static Array<HalfPlane> from_convex_polygon(const Polygon & poly)
    {
      ah_domain_error_if(not poly.is_closed()) << "Polygon must be closed";
      ah_domain_error_if(poly.size() < 3) << "Polygon must have at least 3 vertices";

      Array<HalfPlane> hs;
      hs.reserve(poly.size());
      for (Polygon::Segment_Iterator it(poly); it.has_curr(); it.next_ne())
        {
          const Segment e = it.get_current_segment();
          hs.append(HalfPlane(e.get_src_point(), e.get_tgt_point()));
        }
      return hs;
    }

    /**
     * @brief Intersect half-planes and return bounded feasible polygon.
     *
     * @param halfplanes Input half-planes.
     * @return Bounded intersection polygon; empty polygon if infeasible or unbounded.
     */
    [[nodiscard]] Polygon operator ()(const Array<HalfPlane> & halfplanes) const
    {
      if (halfplanes.size() < 3)
        return {};

      Array<HalfPlane> hps = halfplanes;
      quicksort_op(hps, [](const HalfPlane & a, const HalfPlane & b)
                     {
                       const bool ha = upper_half(a);
                       const bool hb = upper_half(b);
                       if (ha != hb)
                         return ha and not hb;

                       if (const Geom_Number cr = cross_dir(a, b); cr != 0)
                         return cr > 0;

                       // Same direction: keep stronger constraints first.
                       return a.offset() > b.offset();
                     });

      Array<HalfPlane> unique;
      unique.reserve(hps.size());
      for (size_t i = 0; i < hps.size(); ++i)
        {
          const HalfPlane & hp = hps(i);
          if (unique.is_empty())
            {
              unique.append(hp);
              continue;
            }

          if (const HalfPlane & last = unique.get_last(); same_direction(last, hp))
            {
              if (hp.offset() > last.offset())
                {
                  unique.remove_last();
                  unique.append(hp);
                }
              continue;
            }

          unique.append(hp);
        }

      DynDlist<HalfPlane> dq;
      DynDlist<Point> intersections;

      for (size_t i = 0; i < unique.size(); ++i)
        {
          const HalfPlane & hp = unique(i);

          while (not intersections.is_empty() and hp.outside(intersections.get_last()))
            {
              dq.remove_last();
              intersections.remove_last();
            }

          while (not intersections.is_empty() and hp.outside(intersections.get_first()))
            {
              dq.remove_first();
              intersections.remove_first();
            }

          if (not dq.is_empty() and parallel(dq.get_last(), hp))
            {
              if (same_direction(dq.get_last(), hp))
                {
                  if (hp.offset() > dq.get_last().offset())
                    {
                      dq.remove_last();
                      if (not intersections.is_empty())
                        intersections.remove_last();
                    }
                  else
                    continue;
                }
              else // Opposite parallel boundaries cannot define a bounded polygon here.
                return {};
            }

          if (not dq.is_empty())
            intersections.append(line_intersection(dq.get_last(), hp));

          dq.append(hp);
        }

      while (not intersections.is_empty() and dq.get_first().outside(intersections.get_last()))
        {
          dq.remove_last();
          intersections.remove_last();
        }

      while (not intersections.is_empty() and dq.get_last().outside(intersections.get_first()))
        {
          dq.remove_first();
          intersections.remove_first();
        }

      if (dq.size() < 3)
        return {};

      if (parallel(dq.get_last(), dq.get_first()))
        return {};

      const Point closing = line_intersection(dq.get_last(), dq.get_first());

      Array<Point> verts;
      verts.reserve(intersections.size() + 1);

      DynDlist<Point> tmp = intersections;
      while (not tmp.is_empty())
        verts.append(tmp.remove_first());
      verts.append(closing);

      return build_polygon(verts);
    }

    /**
     * @brief Overload for initializer-list convenience.
     *
     * @param il Input half-planes.
     * @return Bounded intersection polygon.
     */
    [[nodiscard]] Polygon operator ()(const std::initializer_list<HalfPlane> il) const
    {
      Array<HalfPlane> hps;
      hps.reserve(il.size());
      for (const HalfPlane & hp: il)
        hps.append(hp);
      return (*this)(hps);
    }
  };

  // ============================================================================
  // Delaunay Triangulation (Bowyer-Watson)
  // ============================================================================

  /**
   * @brief Exact Delaunay's triangulation via Bowyer-Watson incremental insertion.
   *
   * The algorithm removes duplicate points, inserts points incrementally into
   * super-triangle triangulation, and retriangulates the cavity of triangles
   * whose circumcircle contains the inserted point.
   *
   * ## Policy
   *
   * - Points are sorted lexicographically (x, then y), and duplicates are removed.
   * - Collinear inputs return empty triangulation.
   * - Cocircular ties are broken deterministically by insertion index.
   * - Output triangles are indexed against the returned unique sites array.
   *
   * ## Complexity
   *
   * - Worst-case time: O(n^2)
   * - Typical random input: near O(n log n)
   * - Space: O(n)
   *
   * @ingroup Geometry
   */
  class DelaunayTriangulationBowyerWatson
  {
  public:
    struct IndexedTriangle
    {
      size_t i;
      size_t j;
      size_t k;
    };

    struct Result
    {
      Array<Point> sites;
      Array<IndexedTriangle> triangles;
    };

  private:
    struct WorkTriangle
    {
      size_t a;
      size_t b;
      size_t c;
      bool alive;
    };

    struct UndirectedEdge
    {
      size_t u;
      size_t v;
    };

    [[nodiscard]] static bool lexicographic_less(const Point & p1,
                                                 const Point & p2)
    {
      if (p1.get_x() < p2.get_x())
        return true;
      if (p2.get_x() < p1.get_x())
        return false;
      return p1.get_y() < p2.get_y();
    }

    [[nodiscard]] static bool all_collinear(const Array<Point> & pts)
    {
      if (pts.size() < 3)
        return true;

      for (size_t i = 2; i < pts.size(); ++i)
        if (orientation(pts(0), pts(1), pts(i)) != Orientation::COLLINEAR)
          return false;

      return true;
    }

    [[nodiscard]] static bool point_in_circumcircle(const Array<Point> & pts,
                                                    const size_t ia,
                                                    const size_t ib,
                                                    const size_t ic,
                                                    const size_t ip)
    {
      const Point & a = pts(ia);
      const Point & b = pts(ib);
      const Point & c = pts(ic);
      const Point & p = pts(ip);

      const Geom_Number adx = a.get_x() - p.get_x();
      const Geom_Number ady = a.get_y() - p.get_y();
      const Geom_Number bdx = b.get_x() - p.get_x();
      const Geom_Number bdy = b.get_y() - p.get_y();
      const Geom_Number cdx = c.get_x() - p.get_x();
      const Geom_Number cdy = c.get_y() - p.get_y();

      const Geom_Number ad2 = adx * adx + ady * ady;
      const Geom_Number bd2 = bdx * bdx + bdy * bdy;
      const Geom_Number cd2 = cdx * cdx + cdy * cdy;

      const Geom_Number det = ad2 * (bdx * cdy - bdy * cdx) -
                              bd2 * (adx * cdy - ady * cdx) +
                              cd2 * (adx * bdy - ady * bdx);

      const Orientation o = orientation(a, b, c);
      if (o == Orientation::CCW)
        {
          if (det > 0)
            return true;
          if (det < 0)
            return false;
        }
      if (o == Orientation::CW)
        {
          if (det < 0)
            return true;
          if (det > 0)
            return false;
        }

      // Cocircular / degenerate tie-break for deterministic output.
      size_t max_idx = ia;
      if (ib > max_idx) max_idx = ib;
      if (ic > max_idx) max_idx = ic;
      return ip < max_idx;
    }

    [[nodiscard]] static Array<Point> unique_points(const DynList<Point> & point_set)
    {
      Array<Point> all;
      for (DynList<Point>::Iterator it(point_set); it.has_curr(); it.next_ne())
        all.append(it.get_curr());

      quicksort_op(all, [](const Point & p1, const Point & p2)
                     {
                       return lexicographic_less(p1, p2);
                     });

      Array<Point> ret;
      ret.reserve(all.size());
      for (size_t i = 0; i < all.size(); ++i)
        if (ret.is_empty() or ret.get_last() != all(i))
          ret.append(all(i));

      return ret;
    }

    static void toggle_edge(Array<UndirectedEdge> & boundary, size_t u, size_t v)
    {
      if (u > v)
        {
          const size_t tmp = u;
          u = v;
          v = tmp;
        }

      for (size_t i = 0; i < boundary.size(); ++i)
        if (boundary(i).u == u and boundary(i).v == v)
          {
            boundary(i) = boundary.get_last();
            boundary.remove_last();
            return;
          }

      boundary.append(UndirectedEdge{u, v});
    }

  public:
    /**
     * @brief Compute Delaunay triangulation of a point set.
     *
     * @param point_set Input points.
     * @return Unique sites and triangulation indices.
     */
    [[nodiscard]] Result operator ()(const DynList<Point> & point_set) const
    {
      Result ret;
      ret.sites = unique_points(point_set);
      const size_t n = ret.sites.size();

      if (n < 3 or all_collinear(ret.sites))
        return ret;

      Array<Point> pts = ret.sites;

      Geom_Number minx = pts(0).get_x();
      Geom_Number maxx = pts(0).get_x();
      Geom_Number miny = pts(0).get_y();
      Geom_Number maxy = pts(0).get_y();

      for (size_t i = 1; i < n; ++i)
        {
          if (pts(i).get_x() < minx) minx = pts(i).get_x();
          if (pts(i).get_x() > maxx) maxx = pts(i).get_x();
          if (pts(i).get_y() < miny) miny = pts(i).get_y();
          if (pts(i).get_y() > maxy) maxy = pts(i).get_y();
        }

      const Geom_Number dx = maxx - minx;
      const Geom_Number dy = maxy - miny;
      Geom_Number delta = dx > dy ? dx : dy;
      if (delta == 0)
        delta = 1;

      const Geom_Number span = delta * 16 + 1;
      const Geom_Number two_span = span + span;
      const Geom_Number midx = (minx + maxx) / 2;
      const Geom_Number midy = (miny + maxy) / 2;

      const Point s0(midx - two_span, midy - span);
      const Point s1(midx + two_span, midy - span);
      const Point s2(midx, midy + two_span);

      pts.append(s0);
      pts.append(s1);
      pts.append(s2);

      const size_t i0 = n;
      const size_t i1 = n + 1;
      const size_t i2 = n + 2;

      Array<WorkTriangle> work;
      work.append(WorkTriangle{i0, i1, i2, true});

      for (size_t pidx = 0; pidx < n; ++pidx)
        {
          Array<size_t> bad;
          bad.reserve(work.size());

          for (size_t t = 0; t < work.size(); ++t)
            {
              const auto & [a, b, c, alive] = work(t);
              if (not alive)
                continue;

              if (point_in_circumcircle(pts, a, b, c, pidx))
                bad.append(t);
            }

          if (bad.is_empty())
            continue;

          Array<UndirectedEdge> boundary;
          boundary.reserve(bad.size() * 3 + 1);

          for (size_t bad_idx = 0; bad_idx < bad.size(); ++bad_idx)
            {
              auto & [a, b, c, alive] = work(bad(bad_idx));
              if (not alive)
                continue;

              toggle_edge(boundary, a, b);
              toggle_edge(boundary, b, c);
              toggle_edge(boundary, c, a);
              alive = false;
            }

          for (size_t e = 0; e < boundary.size(); ++e)
            {
              size_t u = boundary(e).u;
              size_t v = boundary(e).v;

              const Orientation o = orientation(pts(u), pts(v), pts(pidx));
              if (o == Orientation::COLLINEAR)
                continue;

              if (o == Orientation::CW)
                {
                  const size_t tmp = u;
                  u = v;
                  v = tmp;
                }

              work.append(WorkTriangle{u, v, pidx, true});
            }
        }

      ret.triangles.reserve(work.size());
      for (size_t t = 0; t < work.size(); ++t)
        {
          const WorkTriangle & tr = work(t);
          if (not tr.alive)
            continue;

          if (tr.a >= n or tr.b >= n or tr.c >= n)
            continue;

          if (orientation(ret.sites(tr.a), ret.sites(tr.b), ret.sites(tr.c)) ==
              Orientation::COLLINEAR)
            continue;

          ret.triangles.append(IndexedTriangle{tr.a, tr.b, tr.c});
        }

      return ret;
    }

    /**
     * @brief Convenience overload using initializer-list input.
     *
     * @param il Input points.
     * @return Unique sites and triangulation indices.
     */
    [[nodiscard]] Result operator ()(const std::initializer_list<Point> il) const
    {
      DynList<Point> points;
      for (const Point & p: il)
        points.append(p);
      return (*this)(points);
    }

    /**
     * @brief Convert indexed triangulation to geometric triangles.
     *
     * @param result Triangulation result.
     * @return List of geometric triangles.
     */
    static DynList<Triangle> as_triangles(const Result & result)
    {
      DynList<Triangle> out;
      for (size_t tria_idx = 0; tria_idx < result.triangles.size(); ++tria_idx)
        {
          const auto & [i, j, k] = result.triangles(tria_idx);
          out.append(Triangle(result.sites(i),
                              result.sites(j),
                              result.sites(k)));
        }
      return out;
    }
  };

  // ============================================================================
  // Voronoi Diagram (Dual of Delaunay)
  // ============================================================================

  /**
   * @brief Voronoi diagram derived from Delaunay triangulation.
   *
   * The dual construction is used:
   * - Voronoi vertices are circumcenters of Delaunay triangles.
   * - Internal Delaunay edges map to bounded Voronoi segments.
   * - Hull Delaunay edges map to unbounded Voronoi rays.
   *
   * @ingroup Geometry
   */
  class VoronoiDiagramFromDelaunay
  {
  public:
    struct Edge
    {
      size_t site_u;
      size_t site_v;
      Point src;
      Point tgt;
      bool unbounded;
      Point direction;
    };

    struct Cell
    {
      size_t site_index;
      Point site;
      bool bounded;
      Array<Point> vertices;
    };

    struct ClippedCell
    {
      size_t site_index;
      Point site;
      Polygon polygon;
    };

    struct Result
    {
      Array<Point> sites;
      Array<Point> vertices;
      Array<Edge> edges;
      Array<Cell> cells;
    };

  private:
    struct EdgeAdj
    {
      size_t u;
      size_t v;
      size_t tri;
      size_t third;
    };

    DelaunayTriangulationBowyerWatson delaunay;

    [[nodiscard]] static Point circumcenter(const Point & a,
                                            const Point & b,
                                            const Point & c)
    {
      const Geom_Number & ax = a.get_x();
      const Geom_Number & ay = a.get_y();
      const Geom_Number & bx = b.get_x();
      const Geom_Number & by = b.get_y();
      const Geom_Number & cx = c.get_x();
      const Geom_Number & cy = c.get_y();

      const Geom_Number a2 = ax * ax + ay * ay;
      const Geom_Number b2 = bx * bx + by * by;
      const Geom_Number c2 = cx * cx + cy * cy;

      const Geom_Number d = ax * (by - cy) + bx * (cy - ay) + cx * (ay - by);
      ah_domain_error_if(d == 0) << "Circumcenter undefined for collinear points";
      const Geom_Number den = d + d;

      const Geom_Number ux = (a2 * (by - cy) + b2 * (cy - ay) + c2 * (ay - by)) / den;
      const Geom_Number uy = (a2 * (cx - bx) + b2 * (ax - cx) + c2 * (bx - ax)) / den;

      return {ux, uy};
    }

    static void append_edge(Array<EdgeAdj> & edges, size_t a, size_t b,
                            const size_t tri, const size_t third)
    {
      if (a > b)
        {
          const size_t tmp = a;
          a = b;
          b = tmp;
        }

      edges.append(EdgeAdj{a, b, tri, third});
    }

    [[nodiscard]] static Array<Point> extract_vertices(const Polygon & p)
    {
      ah_domain_error_if(not p.is_closed()) << "Polygon must be closed";
      ah_domain_error_if(p.size() < 3) << "Polygon must have at least 3 vertices";

      Array<Point> verts;
      verts.reserve(p.size());
      for (Polygon::Vertex_Iterator it(p); it.has_curr(); it.next_ne())
        verts.append(it.get_current_vertex());

      return verts;
    }

    [[nodiscard]] static bool is_convex(const Array<Point> & verts)
    {
      if (verts.size() < 3)
        return false;

      int sign = 0;
      const size_t n = verts.size();
      for (size_t i = 0; i < n; ++i)
        {
          const Orientation o = orientation(verts(i),
                                            verts((i + 1) % n),
                                            verts((i + 2) % n));
          if (o == Orientation::COLLINEAR)
            continue;

          const int curr = (o == Orientation::CCW) ? 1 : -1;
          if (sign == 0)
            sign = curr;
          else if (sign != curr)
            return false;
        }

      return true;
    }

    [[nodiscard]] static HalfPlaneIntersection::HalfPlane
    bisector_halfplane_for_site(const Point & s, const Point & t)
    {
      const Point mid((s.get_x() + t.get_x()) / 2,
                      (s.get_y() + t.get_y()) / 2);

      const Geom_Number dx = t.get_x() - s.get_x();
      const Geom_Number dy = t.get_y() - s.get_y();
      const Point q(mid.get_x() - dy, mid.get_y() + dx);

      return {mid, q}; // the left side contains s
    }

    [[nodiscard]] static Array<ClippedCell>
    indexed_clipped_cells(const Array<Point> & sites, const Array<Polygon> & polys)
    {
      ah_domain_error_if(sites.size() != polys.size())
        << "Sites and clipped polygons size mismatch";

      Array<ClippedCell> ret;
      ret.reserve(sites.size());

      for (size_t i = 0; i < sites.size(); ++i)
        {
          ClippedCell cell;
          cell.site_index = i;
          cell.site = sites(i);
          cell.polygon = polys(i);
          ret.append(std::move(cell));
        }

      return ret;
    }

  public:
    /**
     * @brief Build Voronoi from a precomputed Delaunay triangulation.
     *
     * @param dt Delaunay triangulation result.
     * @return Voronoi vertices, edges, and per-site cells.
     */
    [[nodiscard]] Result
    operator ()(const DelaunayTriangulationBowyerWatson::Result & dt) const
    {
      Result ret;
      ret.sites = dt.sites;

      if (dt.triangles.is_empty())
        return ret;

      Array<Point> centers;
      centers.reserve(dt.triangles.size());
      for (size_t idx = 0; idx < dt.triangles.size(); ++idx)
        {
          const auto & [i, j, k] = dt.triangles(idx);
          centers.append(circumcenter(dt.sites(i), dt.sites(j), dt.sites(k)));
        }
      ret.vertices = centers;

      Array<EdgeAdj> edges;
      edges.reserve(dt.triangles.size() * 3 + 1);
      for (size_t idx = 0; idx < dt.triangles.size(); ++idx)
        {
          const auto & [i, j, k] = dt.triangles(idx);
          append_edge(edges, i, j, i, k);
          append_edge(edges, j, k, i, i);
          append_edge(edges, k, i, i, j);
        }

      quicksort_op(edges, [](const EdgeAdj & a, const EdgeAdj & b)
                     {
                       if (a.u != b.u)
                         return a.u < b.u;
                       if (a.v != b.v)
                         return a.v < b.v;
                       return a.tri < b.tri;
                     });

      Array<unsigned char> on_hull;
      on_hull.reserve(dt.sites.size());
      for (size_t i = 0; i < dt.sites.size(); ++i)
        on_hull.append(0);

      size_t i = 0;
      while (i < edges.size())
        {
          size_t j = i + 1;
          while (j < edges.size() and
                 edges(j).u == edges(i).u and edges(j).v == edges(i).v)
            ++j;

          if (const size_t cnt = j - i; cnt >= 2)
            {
              const Point & p1 = centers(edges(i).tri);
              const Point & p2 = centers(edges(i + 1).tri);
              ret.edges.append(Edge{edges(i).u, edges(i).v, p1, p2, false, Point()});
            }
          else
            {
              const auto & [u, v, tri, third] = edges(i);
              on_hull(u) = 1;
              on_hull(v) = 1;

              const Point & pu = dt.sites(u);
              const Point & pv = dt.sites(v);
              const Point & pw = dt.sites(third);

              const Geom_Number ex = pv.get_x() - pu.get_x();
              const Geom_Number ey = pv.get_y() - pu.get_y();

              Geom_Number dirx = -ey;
              Geom_Number diry = ex;
              if (orientation(pu, pv, pw) == Orientation::CCW)
                {
                  dirx = ey;
                  diry = -ex;
                }

              const Point & src = centers(tri);
              ret.edges.append(Edge{u, v, src, src, true, Point(dirx, diry)});
            }

          i = j;
        }

      ret.cells.reserve(dt.sites.size());
      for (size_t s = 0; s < dt.sites.size(); ++s)
        {
          Array<Point> verts;
          for (size_t t = 0; t < dt.triangles.size(); ++t)
            {
              const auto & [i, j, k] = dt.triangles(t);
              if (i == s or j == s or k == s)
                verts.append(centers(t));
            }

          const Point site = dt.sites(s);
          if (verts.size() > 1)
            {
              quicksort_op(verts, [&site](const Point & a, const Point & b)
                             {
                               const Geom_Number ax = a.get_x() - site.get_x();
                               const Geom_Number ay = a.get_y() - site.get_y();
                               const Geom_Number bx = b.get_x() - site.get_x();
                               const Geom_Number by = b.get_y() - site.get_y();

                               const bool au = (ay > 0) or (ay == 0 and ax >= 0);
                               const bool bu = (by > 0) or (by == 0 and bx >= 0);
                               if (au != bu)
                                 return au and not bu;

                               if (const Geom_Number cr = ax * by - ay * bx; cr != 0)
                                 return cr > 0;

                               return (ax * ax + ay * ay) < (bx * bx + by * by);
                             });
            }

          Array<Point> clean;
          clean.reserve(verts.size());
          for (size_t k = 0; k < verts.size(); ++k)
            {
              if (clean.is_empty() or clean.get_last() != verts(k))
                clean.append(verts(k));
            }
          if (clean.size() > 1 and clean(0) == clean.get_last())
            clean.remove_last();

          Cell cell;
          cell.site_index = s;
          cell.site = site;
          cell.bounded = on_hull(s) == 0;
          cell.vertices = clean;
          ret.cells.append(std::move(cell));
        }

      return ret;
    }

    /**
     * @brief Build Voronoi from a point set (computes Delaunay first).
     *
     * @param point_set Input points.
     * @return Voronoi diagram.
     */
    [[nodiscard]] Result operator ()(const DynList<Point> & point_set) const
    {
      return (*this)(delaunay(point_set));
    }

    /**
     * @brief Convenience overload using initializer-list input.
     *
     * @param il Input points.
     * @return Voronoi diagram.
     */
    [[nodiscard]] Result operator ()(const std::initializer_list<Point> il) const
    {
      return (*this)(delaunay(il));
    }

    /**
     * @brief Clip Voronoi cells against a closed convex polygon.
     *
     * This produces bounded polygons for all cells, including originally
     * unbounded hull-site cells.
     *
     * @param sites Voronoi sites.
     * @param clip Closed convex clipping polygon.
     * @return One clipped convex polygon per site (same order as sites).
     *
     * @throws domain_error if the clip is not closed/convex or has < 3 vertices.
     */
    static Array<Polygon> clipped_cells(const Array<Point> & sites,
                                        const Polygon & clip)
    {
      const Array<Point> clip_verts = extract_vertices(clip);
      ah_domain_error_if(not is_convex(clip_verts)) << "Clip polygon must be convex";

      const Array<HalfPlaneIntersection::HalfPlane> clip_hps =
          HalfPlaneIntersection::from_convex_polygon(clip);

      Array<Polygon> ret;
      ret.reserve(sites.size());

      for (size_t i = 0; i < sites.size(); ++i)
        {
          HalfPlaneIntersection hpi;
          Array<HalfPlaneIntersection::HalfPlane> hps = clip_hps;
          hps.reserve(clip_hps.size() + sites.size());

          for (size_t j = 0; j < sites.size(); ++j)
            {
              if (j == i)
                continue;

              if (sites(j) == sites(i))
                continue;

              hps.append(bisector_halfplane_for_site(sites(i), sites(j)));
            }

          ret.append(hpi(hps));
        }

      return ret;
    }

    /**
     * @brief Clip Voronoi cells against a closed convex polygon.
     *
     * @param vor Voronoi result.
     * @param clip Closed convex clipping polygon.
     * @return One clipped convex polygon per site.
     */
    [[nodiscard]] static Array<Polygon> clipped_cells(const Result & vor,
                                                      const Polygon & clip)
    {
      return clipped_cells(vor.sites, clip);
    }

    /**
     * @brief Compute Voronoi and clip its cells against a closed convex polygon.
     *
     * @param point_set Input sites.
     * @param clip Closed convex clipping polygon.
     * @return One clipped convex polygon per site.
     */
    [[nodiscard]] Array<Polygon> clipped_cells(const DynList<Point> & point_set,
                                               const Polygon & clip) const
    {
      return clipped_cells(delaunay(point_set).sites, clip);
    }

    /**
     * @brief Convenience overload with initializer-list input.
     *
     * @param il Input sites.
     * @param clip Closed convex clipping polygon.
     * @return One clipped convex polygon per site.
     */
    [[nodiscard]] Array<Polygon>
    clipped_cells(const std::initializer_list<Point> il, const Polygon & clip) const
    {
      return clipped_cells(delaunay(il).sites, clip);
    }

    /**
     * @brief Clip Voronoi cells and return explicit site-indexed records.
     *
     * @param sites Voronoi sites.
     * @param clip Closed convex clipping polygon.
     * @return Array of {site_index, site, polygon} records.
     */
    static Array<ClippedCell>
    clipped_cells_indexed(const Array<Point> & sites, const Polygon & clip)
    {
      return indexed_clipped_cells(sites, clipped_cells(sites, clip));
    }

    /**
     * @brief Clip Voronoi cells (from result) into site-indexed records.
     *
     * @param vor Voronoi result.
     * @param clip Closed convex clipping polygon.
     * @return Array of {site_index, site, polygon} records.
     */
    [[nodiscard]] static Array<ClippedCell>
    clipped_cells_indexed(const Result & vor, const Polygon & clip)
    {
      return clipped_cells_indexed(vor.sites, clip);
    }

    /**
     * @brief Compute/clip Voronoi cells and return site-indexed records.
     *
     * @param point_set Input sites.
     * @param clip Closed convex clipping polygon.
     * @return Array of {site_index, site, polygon} records.
     */
    [[nodiscard]] Array<ClippedCell>
    clipped_cells_indexed(const DynList<Point> & point_set, const Polygon & clip) const
    {
      return clipped_cells_indexed(delaunay(point_set).sites, clip);
    }

    /**
     * @brief Convenience overload with initializer-list input.
     *
     * @param il Input sites.
     * @param clip Closed convex clipping polygon.
     * @return Array of {site_index, site, polygon} records.
     */
    [[nodiscard]] Array<ClippedCell>
    clipped_cells_indexed(const std::initializer_list<Point> il,
                          const Polygon & clip) const
    {
      return clipped_cells_indexed(delaunay(il).sites, clip);
    }
  };

  // ============================================================================
  // Convex Hull Algorithms
  // ============================================================================

  /**
   * @brief Andrew's monotonic chain convex hull algorithm.
   *
   * Computes the convex hull by sorting points lexicographically and building
   * the lower and upper chains with a stack-like process.
   *
   * ## Policy
   *
   * - Duplicate points are removed before processing.
   * - Collinear points on hull edges are excluded, keeping only extreme
   *   endpoints of each edge.
   * - Degenerate cases are preserved:
   *   - 0 unique points -> empty polygon
   *   - 1 unique point  -> 1-vertex (open) polygon
   *   - 2 unique points -> 2-vertex closed degenerate hull
   *
   * ## Complexity
   *
   * - Time: O(n log n) due to sorting
   * - Space: O(n)
   *
   * @ingroup Geometry
   */
  class AndrewMonotonicChainConvexHull
  {
    struct LexicographicCmp
    {
      bool operator ()(const Point & p1, const Point & p2) const
      {
        if (p1.get_x() < p2.get_x())
          return true;

        if (p2.get_x() < p1.get_x())
          return false;

        return p1.get_y() < p2.get_y();
      }
    };

    static Array<Point> sorted_unique_points(const DynList<Point> & point_set)
    {
      Array<Point> points;
      for (DynList<Point>::Iterator it(point_set); it.has_curr(); it.next_ne())
        points.append(it.get_curr());

      if (points.size() <= 1)
        return points;

      quicksort_op(points, LexicographicCmp());

      Array<Point> unique;
      unique.reserve(points.size());
      unique.append(points(0));
      for (size_t i = 1; i < points.size(); ++i)
        if (points(i) != unique.get_last())
          unique.append(points(i));

      return unique;
    }

    [[nodiscard]] static Geom_Number turn(const Point & a, const Point & b, const Point & c)
    {
      return area_of_parallelogram(a, b, c);
    }

  public:
    /**
     * @brief Compute the convex hull of a point set.
     *
     * @param point_set The input points.
     * @return A polygon representing the convex hull.
     */
    Polygon operator ()(const DynList<Point> & point_set)
    {
      Polygon ret;
      Array<Point> points = sorted_unique_points(point_set);
      const size_t n = points.size();

      if (n == 0)
        return ret;

      if (n == 1)
        {
          ret.add_vertex(points(0));
          return ret;
        }

      Array<Point> lower;
      lower.reserve(n);
      for (size_t i = 0; i < n; ++i)
        {
          while (lower.size() >= 2 &&
                 turn(lower(lower.size() - 2), lower(lower.size() - 1), points(i)) <= 0)
            lower.remove_last();
          lower.append(points(i));
        }

      Array<Point> upper;
      upper.reserve(n);
      for (size_t i = n; i > 0; --i)
        {
          const Point & p = points(i - 1);
          while (upper.size() >= 2 &&
                 turn(upper(upper.size() - 2), upper(upper.size() - 1), p) <= 0)
            upper.remove_last();
          upper.append(p);
        }

      // Remove duplicate endpoints before concatenating chains.
      lower.remove_last();
      upper.remove_last();

      for (size_t i = 0; i < lower.size(); ++i)
        ret.add_vertex(lower(i));

      for (size_t i = 0; i < upper.size(); ++i)
        ret.add_vertex(upper(i));

      if (ret.size() >= 2)
        ret.close();

      return ret;
    }
  };

  /**
   * @brief Graham scan convex hull algorithm.
   *
   * Computes the convex hull by choosing the lowest point as pivot, sorting
   * the remaining points by polar angle around the pivot, and scanning with
   * a stack-like process.
   *
   * ## Policy
   *
   * - Duplicate points are removed before processing.
   * - Collinear points on hull edges are excluded, keeping only extreme
   *   endpoints of each edge.
   * - Degenerate cases are preserved:
   *   - 0 unique points -> empty polygon
   *   - 1 unique point  -> 1-vertex (open) polygon
   *   - 2 unique points -> 2-vertex closed degenerate hull
   *
   * ## Complexity
   *
   * - Time: O(n log n) due to sorting
   * - Space: O(n)
   *
   * @ingroup Geometry
   */
  class GrahamScanConvexHull
  {
    struct LexicographicCmp
    {
      bool operator ()(const Point & p1, const Point & p2) const
      {
        if (p1.get_x() < p2.get_x())
          return true;

        if (p2.get_x() < p1.get_x())
          return false;

        return p1.get_y() < p2.get_y();
      }
    };

    static Array<Point> sorted_unique_points(const DynList<Point> & point_set)
    {
      Array<Point> points;
      for (DynList<Point>::Iterator it(point_set); it.has_curr(); it.next_ne())
        points.append(it.get_curr());

      if (points.size() <= 1)
        return points;

      quicksort_op(points, LexicographicCmp());

      Array<Point> unique;
      unique.reserve(points.size());
      unique.append(points(0));
      for (size_t i = 1; i < points.size(); ++i)
        if (points(i) != unique.get_last())
          unique.append(points(i));

      return unique;
    }

    [[nodiscard]] static Geom_Number turn(const Point & a, const Point & b, const Point & c)
    {
      return area_of_parallelogram(a, b, c);
    }

  public:
    /**
     * @brief Compute the convex hull of a point set.
     *
     * @param point_set The input points.
     * @return A polygon representing the convex hull.
     */
    Polygon operator ()(const DynList<Point> & point_set)
    {
      Polygon ret;
      Array<Point> points = sorted_unique_points(point_set);
      const size_t n = points.size();

      if (n == 0)
        return ret;

      if (n == 1)
        {
          ret.add_vertex(points(0));
          return ret;
        }

      // Pivot = lowest y, and lowest x on ties.
      size_t pivot_idx = 0;
      for (size_t i = 1; i < n; ++i)
        {
          if (points(i).get_y() < points(pivot_idx).get_y())
            {
              pivot_idx = i;
              continue;
            }

          if (points(i).get_y() == points(pivot_idx).get_y() &&
              points(i).get_x() < points(pivot_idx).get_x())
            pivot_idx = i;
        }

      const Point pivot = points(pivot_idx);
      if (pivot_idx != 0)
        {
          const Point tmp = points(0);
          points(0) = points(pivot_idx);
          points(pivot_idx) = tmp;
        }

      Array<Point> polar;
      polar.reserve(n - 1);
      for (size_t i = 1; i < n; ++i)
        polar.append(points(i));

      quicksort_op(polar, [&pivot](const Point & a, const Point & b)
                     {
                       const Geom_Number area = area_of_parallelogram(pivot, a, b);
                       if (area > 0)
                         return true;
                       if (area < 0)
                         return false;

                       // Same angle: keep nearer first; later we keep only farthest.
                       return pivot.distance_squared_to(a) <
                              pivot.distance_squared_to(b);
                     });

      // Keep only the farthest point per polar direction.
      Array<Point> filtered;
      filtered.reserve(polar.size());
      for (size_t i = 0; i < polar.size();)
        {
          size_t j = i;
          while (j + 1 < polar.size() &&
                 area_of_parallelogram(pivot, polar(j), polar(j + 1)) == 0)
            ++j;
          filtered.append(polar(j));
          i = j + 1;
        }

      if (filtered.size() == 1)
        {
          ret.add_vertex(pivot);
          ret.add_vertex(filtered(0));
          ret.close();
          return ret;
        }

      Array<Point> hull;
      hull.reserve(filtered.size() + 1);
      hull.append(pivot);
      hull.append(filtered(0));

      for (size_t i = 1; i < filtered.size(); ++i)
        {
          const Point & p = filtered(i);
          while (hull.size() >= 2 &&
                 turn(hull(hull.size() - 2), hull(hull.size() - 1), p) <= 0)
            hull.remove_last();
          hull.append(p);
        }

      for (size_t i = 0; i < hull.size(); ++i)
        ret.add_vertex(hull(i));

      if (ret.size() >= 2)
        ret.close();

      return ret;
    }
  };

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
   * @ingroup Geometry
   * @author Alejandro J. Mujica
   */
  class BruteForceConvexHull
  {
    struct CmpSegment
    {
      static bool cmp_point(const Point & p1, const Point & p2)
      {
        if (p1.get_x() < p2.get_x())
          return true;

        return not (p2.get_x() < p1.get_x()) and p1.get_y() < p2.get_y();
      }

      bool operator ()(const Segment & s1, const Segment & s2) const
      {
        if (cmp_point(s1.get_src_point(), s2.get_src_point()))
          return true;

        return not (cmp_point(s2.get_src_point(), s1.get_src_point())) and
               cmp_point(s1.get_tgt_point(), s2.get_tgt_point());
      }
    };

    using SegmentSet = DynSetTree<Segment, Treap_Rk, CmpSegment>;
    using PointIt = DynList<Point>::Iterator;

    static bool are_all_points_on_left(const DynList<Point> & l, const Segment & s)
    {
      for (PointIt it(l); it.has_curr(); it.next_ne())
        if (const Point & p = it.get_curr(); p.is_to_right_from(s))
          return false;

      return true;
    }

    static SegmentSet extreme_edges(const DynList<Point> & point_set)
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

              if (Segment s(p_i, p_j); are_all_points_on_left(point_set, s))
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
    Polygon operator ()(const DynList<Point> & point_set) const
    {
      Polygon ret;

      SegmentSet extremes = extreme_edges(point_set);

      const Segment first_segment = extremes.remove_pos(0);
      ret.add_vertex(first_segment.get_src_point());
      ret.add_vertex(first_segment.get_tgt_point());

      while (true)
        {
          const Vertex & last_vertex = ret.get_last_vertex();

          const Segment *ptr = extremes.find_ptr([&last_vertex](const Segment & s)
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
   * @ingroup Geometry
   * @author Alejandro J. Mujica
   */
  class GiftWrappingConvexHull
  {
    static Point * get_lowest_point(const DynList<Point> & point_set)
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
    Polygon operator ()(const DynList<Point> & point_set) const
    {
      Polygon ret;

      const Point *lowest_point = get_lowest_point(point_set);
      const Point *p_i = lowest_point;
      ret.add_vertex(*lowest_point);

      auto last_segment = Segment(Point(0, 0), Point(1, 0));

      while (true)
        {
          const Point *p_k = nullptr;

          double min_angle = std::numeric_limits<double>::max();

          for (DynList<Point>::Iterator it(point_set); it.has_curr(); it.next_ne())
            {
              Point & p_j = it.get_curr();

              if (p_j == *p_i)
                continue;

              const double angle =
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
   * @ingroup Geometry
   * @author Alejandro J. Mujica
   */
  class QuickHull
  {
    static Point get_fartest_point(const DynList<Point> & point_set, const Segment & s)
    {
      Geom_Number max_distance = 0;
      Point ret;

      for (DynList<Point>::Iterator it(point_set); it.has_curr(); it.next_ne())
        {
          const Point & p = it.get_curr();

          Segment s1 = s.get_perpendicular(p);

          if (Geom_Number sz = s1.size(); sz > max_distance)
            {
              ret = p;
              max_distance = sz;
            }
        }

      return ret;
    }

    static std::pair<DynList<Point>, DynList<Point>>
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

    static DynList<Point> quick_hull(DynList<Point> & point_set, const Point & a,
                                     const Point & b)
    {
      if (point_set.is_empty())
        return {};

      Point c = get_fartest_point(point_set, Segment(a, b));

      auto r = get_right_points(point_set, a, b, c);

      DynList<Point> ret = quick_hull(r.first, a, c);
      DynList<Point> tmp = quick_hull(r.second, c, b);
      ret.append(c);
      ret.concat(tmp);

      return ret;
    }

    static std::pair<Point, Point> search_extremes(const DynList<Point> & point_set)
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

    static std::pair<DynList<Point>, DynList<Point>>
    partition(const DynList<Point> & point_set, const Point & a, const Point & b)
    {
      std::pair<DynList<Point>, DynList<Point>> ret;

      for (DynList<Point>::Iterator it(point_set); it.has_curr(); it.next_ne())
        if (const Point & p = it.get_curr(); p.is_to_right_from(a, b))
          ret.first.append(p);
        else
          ret.second.append(p);

      return ret;
    }

  public:
    /**
     * @brief Compute the convex hull of a point set.
     *
     * @param point_set The input points.
     * @return A closed polygon representing the convex hull.
     */
    Polygon operator ()(const DynList<Point> & point_set) const
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
