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
# include <tpl_2dtree.H>
# include <ah-errors.H>
# include <utility>

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
   * - Vertices can be clockwise or counter-clockwise (normalized internally)
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

  private:
    [[nodiscard]] static Array<Point> extract_vertices(const Polygon & p)
    {
      Array<Point> verts;
      verts.reserve(p.size());
      for (Polygon::Vertex_Iterator it(p); it.has_curr(); it.next_ne())
        verts.append(it.get_current_vertex());
      return verts;
    }

    [[nodiscard]] static Geom_Number signed_double_area(const Polygon & p)
    {
      const Array<Point> verts = extract_vertices(p);
      Geom_Number sum = 0;
      for (size_t i = 0; i < verts.size(); ++i)
        {
          const Point & a = verts(i);
          const Point & b = verts((i + 1) % verts.size());
          sum += a.get_x() * b.get_y() - a.get_y() * b.get_x();
        }
      return sum;
    }

    static void normalize_to_ccw(Polygon & p)
    {
      const Geom_Number area2 = signed_double_area(p);
      ah_domain_error_if(area2 == 0) << "Polygon is degenerate (zero area)";
      if (area2 > 0)
        return;

      const Array<Point> verts = extract_vertices(p);
      Polygon ccw;
      for (size_t i = verts.size(); i > 0; --i)
        ccw.add_vertex(verts(i - 1));
      ccw.close();
      p = std::move(ccw);
    }

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
     * @throws domain_error if polygon is not closed.
     * @throws domain_error if the polygon has fewer than 3 vertices.
     * @throws domain_error if polygon is degenerate (zero area).
     * @throws domain_error if no valid ear can be found.
     *
     * @note The input polygon is consumed (vertices removed).
     */
    DynList<Triangle> operator ()(Polygon & p) const
    {
      ah_domain_error_if(not p.is_closed()) << "Polygon must be closed";
      ah_domain_error_if(p.size() < 3) << "Polygon has less than 3 vertices";
      normalize_to_ccw(p);

      EarsSet ears;
      if (p.size() > 3)
        {
          ears = init_ears(p);
          ah_domain_error_if(ears.is_empty())
              << "No valid ear found; polygon may be non-simple";
        }

      DynList<Triangle> ret;

      while (p.size() > 3)
        {
          ah_domain_error_if(ears.is_empty())
              << "No valid ear found during triangulation";
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
     * Edge direction is normalized so the interior of the polygon is always on
     * the left side of each half-plane, for both CCW and CW input order.
     *
     * @param poly Closed convex polygon.
     * @return Array of edge half-planes.
     */
    [[nodiscard]] static Array<HalfPlane> from_convex_polygon(const Polygon & poly)
    {
      ah_domain_error_if(not poly.is_closed()) << "Polygon must be closed";
      ah_domain_error_if(poly.size() < 3) << "Polygon must have at least 3 vertices";

      Array<Point> verts;
      verts.reserve(poly.size());
      for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
        verts.append(it.get_current_vertex());

      const Geom_Number area2 = signed_double_area(verts);
      ah_domain_error_if(area2 == 0) << "Polygon is degenerate";
      const bool ccw = area2 > 0;

      Array<HalfPlane> hs;
      hs.reserve(verts.size());
      for (size_t i = 0; i < verts.size(); ++i)
        {
          const Point & a = verts(i);
          const Point & b = verts((i + 1) % verts.size());
          hs.append(ccw ? HalfPlane(a, b) : HalfPlane(b, a));
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

    struct CmpUndirectedEdge
    {
      bool operator ()(const UndirectedEdge & a, const UndirectedEdge & b) const
      {
        if (a.u != b.u)
          return a.u < b.u;
        return a.v < b.v;
      }
    };

    using EdgeSet = DynSetTree<UndirectedEdge, Treap_Rk, CmpUndirectedEdge>;

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

      const Geom_Number det = in_circle_determinant(a, b, c, p);

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

    static void toggle_edge(EdgeSet & boundary, size_t u, size_t v)
    {
      if (u > v)
        {
          const size_t tmp = u;
          u = v;
          v = tmp;
        }

      const UndirectedEdge e{u, v};
      if (auto *ptr = boundary.search(e); ptr != nullptr)
        boundary.remove(*ptr);
      else
        boundary.insert(e);
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

          EdgeSet boundary;

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

          boundary.for_each([&](const UndirectedEdge & edge)
            {
              size_t u = edge.u;
              size_t v = edge.v;

              const Orientation o = orientation(pts(u), pts(v), pts(pidx));
              if (o == Orientation::COLLINEAR)
                return;

              if (o == Orientation::CW)
                {
                  const size_t tmp = u;
                  u = v;
                  v = tmp;
                }

              work.append(WorkTriangle{u, v, pidx, true});
            });
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
  // Delaunay Triangulation — Randomized Incremental O(n log n) expected
  // ============================================================================

  /**
   * @brief O(n log n) expected-time Delaunay triangulation.
   *
   * Uses randomized incremental insertion with a history DAG for O(log n)
   * expected point location and adjacency-based Lawson flipping.
   *
   * ## Complexity
   * - Expected time:  O(n log n)
   * - Worst-case time: O(n²) (extremely unlikely with random shuffle)
   * - Space: O(n)
   *
   * @ingroup Geometry
   */
  class DelaunayTriangulationRandomizedIncremental
  {
  public:
    using IndexedTriangle = DelaunayTriangulationBowyerWatson::IndexedTriangle;
    using Result = DelaunayTriangulationBowyerWatson::Result;

  private:
    static constexpr size_t NONE = ~size_t(0);

    struct Tri
    {
      size_t v[3];    ///< vertex indices, CCW order
      size_t adj[3];  ///< adj[i] = neighbor across edge opposite v[i]
      bool alive;
    };

    struct DagNode
    {
      size_t tri;
      Array<size_t> children;
    };

    /// Return local index (0,1,2) of vertex id in triangle t, or NONE.
    [[nodiscard]] static size_t local_of(const Tri & t, size_t id)
    {
      for (int i = 0; i < 3; ++i)
        if (t.v[i] == id) return size_t(i);
      return NONE;
    }

    /// Return local index of the edge shared with neighbor n.
    [[nodiscard]] static size_t adj_of(const Tri & t, size_t n)
    {
      for (int i = 0; i < 3; ++i)
        if (t.adj[i] == n) return size_t(i);
      return NONE;
    }

    /// True if point p is inside or on triangle t (using orientation tests).
    [[nodiscard]] static bool point_in_tri(const Array<Point> & pts,
                                           const Tri & t, size_t pidx)
    {
      const Orientation o0 = orientation(pts(t.v[0]), pts(t.v[1]), pts(pidx));
      const Orientation o1 = orientation(pts(t.v[1]), pts(t.v[2]), pts(pidx));
      const Orientation o2 = orientation(pts(t.v[2]), pts(t.v[0]), pts(pidx));
      const bool has_cw  = o0 == Orientation::CW  || o1 == Orientation::CW
                        || o2 == Orientation::CW;
      const bool has_ccw = o0 == Orientation::CCW || o1 == Orientation::CCW
                        || o2 == Orientation::CCW;
      return !(has_cw && has_ccw);
    }

    /// Standard in-circumcircle test using exact in_circle_determinant.
    /// Works correctly with finite super-triangle vertices (placed 16×
    /// beyond the bounding box) — no special half-plane handling needed.
    [[nodiscard]] static bool in_cc(const Array<Point> & pts,
                                    size_t ia, size_t ib, size_t ic,
                                    size_t ip)
    {
      const Geom_Number det = in_circle_determinant(pts(ia), pts(ib),
                                                     pts(ic), pts(ip));
      const Orientation o = orientation(pts(ia), pts(ib), pts(ic));
      if (o == Orientation::CCW)
        { if (det > 0) return true; if (det < 0) return false; }
      if (o == Orientation::CW)
        { if (det < 0) return true; if (det > 0) return false; }
      size_t mx = ia;
      if (ib > mx) mx = ib;
      if (ic > mx) mx = ic;
      return ip < mx;
    }

    /// Locate the leaf triangle containing point pidx via DAG walk.
    /// Walk from root through dead (internal) nodes toward alive (leaf)
    /// triangles.  Both dead and alive children are considered so the
    /// walk follows the correct branch even through intermediate dead
    /// nodes.
    [[nodiscard]] static size_t locate(const Array<Point> & pts,
                                       const Array<Tri> & tris,
                                       const Array<DagNode> & dag,
                                       size_t pidx, size_t root)
    {
      size_t cur = root;
      while (!dag(cur).children.is_empty())
        {
          bool found = false;
          for (size_t c = 0; c < dag(cur).children.size(); ++c)
            {
              const size_t child = dag(cur).children(c);
              if (point_in_tri(pts, tris(child), pidx))
                { cur = child; found = true; break; }
            }
          if (!found)
            break;
        }
      return cur;
    }

    /// Update neighbor n of old triangle ot to point to new triangle nt.
    static void remap_adj(Array<Tri> & tris, size_t ot, size_t nt)
    {
      for (int i = 0; i < 3; ++i)
        {
          const size_t nb = tris(ot).adj[i];
          if (nb == NONE) continue;
          const size_t li = adj_of(tris(nb), ot);
          if (li != NONE) tris(nb).adj[li] = nt;
        }
    }

  public:
    [[nodiscard]] Result operator()(const DynList<Point> & point_set) const
    {
      Result ret;

      // Build unique sorted points.
      {
        Array<Point> all;
        for (DynList<Point>::Iterator it(point_set); it.has_curr(); it.next_ne())
          all.append(it.get_curr());
        quicksort_op(all, [](const Point & a, const Point & b)
          { return a.get_x() < b.get_x() ||
                   (a.get_x() == b.get_x() && a.get_y() < b.get_y()); });
        ret.sites = Array<Point>();
        ret.sites.reserve(all.size());
        for (size_t i = 0; i < all.size(); ++i)
          if (ret.sites.is_empty() || ret.sites.get_last() != all(i))
            ret.sites.append(all(i));
      }

      const size_t n = ret.sites.size();
      if (n < 3)
        return ret;

      // Check all-collinear.
      {
        bool collinear = true;
        for (size_t i = 2; i < n && collinear; ++i)
          if (orientation(ret.sites(0), ret.sites(1), ret.sites(i))
              != Orientation::COLLINEAR)
            collinear = false;
        if (collinear) return ret;
      }

      // Build pts array with super-triangle appended.
      Array<Point> pts = ret.sites;
      Geom_Number mnx = pts(0).get_x(), mxx = mnx;
      Geom_Number mny = pts(0).get_y(), mxy = mny;
      for (size_t i = 1; i < n; ++i)
        {
          if (pts(i).get_x() < mnx) mnx = pts(i).get_x();
          if (pts(i).get_x() > mxx) mxx = pts(i).get_x();
          if (pts(i).get_y() < mny) mny = pts(i).get_y();
          if (pts(i).get_y() > mxy) mxy = pts(i).get_y();
        }
      Geom_Number delta = (mxx - mnx) > (mxy - mny) ? (mxx - mnx) : (mxy - mny);
      if (delta == 0) delta = 1;
      const Geom_Number sp = delta * 16 + 1;
      const Geom_Number cx = (mnx + mxx) / 2, cy = (mny + mxy) / 2;
      pts.append(Point(cx - sp - sp, cy - sp));
      pts.append(Point(cx + sp + sp, cy - sp));
      pts.append(Point(cx, cy + sp + sp));

      // Random insertion order for input points [0..n).
      Array<size_t> order;
      order.reserve(n);
      for (size_t i = 0; i < n; ++i)
        order.append(i);
      // Fisher-Yates shuffle.
      {
        unsigned long seed = 42;
        for (size_t i = n - 1; i > 0; --i)
          {
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            const size_t j = seed % (i + 1);
            const size_t tmp = order(i);
            order(i) = order(j);
            order(j) = tmp;
          }
      }

      // Initialize with super-triangle.
      Array<Tri> tris;
      Array<DagNode> dag;
      tris.append(Tri{{n, n+1, n+2}, {NONE, NONE, NONE}, true});
      dag.append(DagNode{0, Array<size_t>()});

      // Insert each point using local Bowyer-Watson cavity approach.
      // 1) Locate containing triangle via DAG  — O(log n) expected.
      // 2) BFS from it to find all "bad" triangles (circumcircle
      //    contains the new point).
      // 3) Extract boundary polygon of the cavity.
      // 4) Re-triangulate cavity by connecting boundary edges to
      //    the new point.
      // Expected cavity size is O(1) for random insertion order,
      // giving O(n log n) total expected time.
      for (size_t oi = 0; oi < n; ++oi)
        {
          const size_t pidx = order(oi);
          const size_t ti = locate(pts, tris, dag, pidx, 0);

          // --- BFS to find the cavity ---
          const size_t ntris_before = tris.size();
          Array<bool> is_bad;
          is_bad.reserve(ntris_before);
          for (size_t i = 0; i < ntris_before; ++i)
            is_bad.append(false);

          Array<size_t> cavity;
          is_bad(ti) = true;
          cavity.append(ti);

          size_t qi = 0;
          while (qi < cavity.size())
            {
              for (const size_t ct = cavity(qi++); unsigned long nb : tris(ct).adj)
                {
                  if (nb == NONE || nb >= ntris_before ||
                      !tris(nb).alive || is_bad(nb))
                    continue;
                  if (in_cc(pts, tris(nb).v[0], tris(nb).v[1],
                            tris(nb).v[2], pidx))
                    {
                      is_bad(nb) = true;
                      cavity.append(nb);
                    }
                }
            }

          // --- Extract boundary edges ---
          // Each boundary edge is an edge of a cavity triangle whose
          // neighbor is outside the cavity (or NONE).
          struct BEdge { size_t u, v, ext, old_ct; };
          Array<BEdge> boundary;
          for (size_t ci = 0; ci < cavity.size(); ++ci)
            {
              const size_t ct = cavity(ci);
              for (int e = 0; e < 3; ++e)
                {
                  const size_t nb = tris(ct).adj[e];
                  if (nb != NONE && nb < ntris_before && is_bad(nb))
                    continue; // internal cavity edge — skip
                  // Boundary edge opposite v[e]: vertices (v[(e+1)%3], v[(e+2)%3]).
                  boundary.append(BEdge{tris(ct).v[(e+1)%3],
                                        tris(ct).v[(e+2)%3],
                                        nb, ct});
                }
            }

          // --- Create new triangles ---
          // For each boundary edge (u,v) create triangle (u, v, pidx).
          // adj[2] (opposite pidx) = external neighbor.
          const size_t V = pts.size();
          Array<size_t> as_u, as_v;  // vertex → new tri where it's u / v
          as_u.reserve(V); as_v.reserve(V);
          for (size_t i = 0; i < V; ++i)
            { as_u.append(NONE); as_v.append(NONE); }

          Array<size_t> new_tris;
          new_tris.reserve(boundary.size());

          for (size_t bi = 0; bi < boundary.size(); ++bi)
            {
              size_t u = boundary(bi).u, v = boundary(bi).v;
              const size_t ext = boundary(bi).ext;
              const size_t old_ct = boundary(bi).old_ct;

              // Ensure CCW winding.
              if (orientation(pts(u), pts(v), pts(pidx)) != Orientation::CCW)
                { const size_t tmp = u; u = v; v = tmp; }

              const size_t nt = tris.size();
              tris.append(Tri{{u, v, pidx}, {NONE, NONE, ext}, true});
              dag.append(DagNode{nt, Array<size_t>()});
              new_tris.append(nt);

              as_u(u) = nt;
              as_v(v) = nt;

              // Remap external neighbor to point to the new triangle.
              if (ext != NONE)
                {
                  const size_t li = adj_of(tris(ext), old_ct);
                  if (li != NONE) tris(ext).adj[li] = nt;
                }
            }

          // --- Internal adjacencies between new triangles ---
          // Two new triangles share an edge through pidx.
          // For triangle (u, v, pidx):
          //   adj[0] opp u = edge(v, pidx)  → the new tri with v as u
          //   adj[1] opp v = edge(pidx, u)  → the new tri with u as v
          for (size_t ni = 0; ni < new_tris.size(); ++ni)
            {
              const size_t nt = new_tris(ni);
              const size_t u = tris(nt).v[0];
              const size_t v = tris(nt).v[1];
              tris(nt).adj[0] = as_u(v);   // neighbor across (v, pidx)
              tris(nt).adj[1] = as_v(u);   // neighbor across (pidx, u)
            }

          // --- Kill cavity and update DAG ---
          for (size_t ci = 0; ci < cavity.size(); ++ci)
            {
              tris(cavity(ci)).alive = false;
              for (size_t ni = 0; ni < new_tris.size(); ++ni)
                dag(cavity(ci)).children.append(new_tris(ni));
            }
        }

      // Collect alive triangles that don't reference super-triangle.
      ret.triangles.reserve(2 * n);
      for (size_t t = 0; t < tris.size(); ++t)
        {
          const Tri & tr = tris(t);
          if (!tr.alive) continue;
          if (tr.v[0] >= n || tr.v[1] >= n || tr.v[2] >= n) continue;
          if (orientation(pts(tr.v[0]), pts(tr.v[1]), pts(tr.v[2]))
              == Orientation::COLLINEAR)
            continue;
          ret.triangles.append(IndexedTriangle{tr.v[0], tr.v[1], tr.v[2]});
        }

      return ret;
    }

    [[nodiscard]] Result operator()(const std::initializer_list<Point> il) const
    {
      DynList<Point> points;
      for (const Point & p : il)
        points.append(p);
      return (*this)(points);
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
          append_edge(edges, i, j, idx, k);
          append_edge(edges, j, k, idx, i);
          append_edge(edges, k, i, idx, j);
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

      // Pre-build site -> incident triangles index (O(T) instead of O(n*T)).
      Array<Array<size_t>> incidence;
      incidence.reserve(dt.sites.size());
      for (size_t s = 0; s < dt.sites.size(); ++s)
        incidence.append(Array<size_t>());
      for (size_t t = 0; t < dt.triangles.size(); ++t)
        {
          const auto & [ti, tj, tk] = dt.triangles(t);
          incidence(ti).append(t);
          incidence(tj).append(t);
          incidence(tk).append(t);
        }

      ret.cells.reserve(dt.sites.size());
      for (size_t s = 0; s < dt.sites.size(); ++s)
        {
          Array<Point> verts;
          verts.reserve(incidence(s).size());
          for (size_t idx = 0; idx < incidence(s).size(); ++idx)
            verts.append(centers(incidence(s)(idx)));

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
  // Voronoi Diagram — O(n log n) via Incremental Delaunay Dual
  // ============================================================================

  /**
   * @brief O(n log n) Voronoi diagram construction.
   *
   * Achieves the same asymptotic complexity as Fortune's sweep line
   * through the composition:
   *
   *   1. O(n log n) randomized incremental Delaunay triangulation
   *   2. O(n)   dual construction (circumcenters → Voronoi vertices)
   *
   * Unlike a classical Fortune sweep, this approach works entirely with
   * exact rational arithmetic (`Geom_Number`) — no square roots needed.
   *
   * The output types are identical to `VoronoiDiagramFromDelaunay`.
   *
   * ## Complexity
   * - Expected time:  O(n log n)
   * - Space: O(n)
   *
   * @ingroup Geometry
   */
  class VoronoiDiagramFortune
  {
    DelaunayTriangulationRandomizedIncremental delaunay_;
    VoronoiDiagramFromDelaunay voronoi_;

  public:
    using Edge        = VoronoiDiagramFromDelaunay::Edge;
    using Cell        = VoronoiDiagramFromDelaunay::Cell;
    using ClippedCell = VoronoiDiagramFromDelaunay::ClippedCell;
    using Result      = VoronoiDiagramFromDelaunay::Result;

    /// Compute the Voronoi diagram for a list of points.
    [[nodiscard]] Result operator()(const DynList<Point> & pts) const
    {
      auto dt = delaunay_(pts);
      return voronoi_(dt);
    }

    /// Compute the Voronoi diagram from an initializer list.
    [[nodiscard]] Result operator()(const std::initializer_list<Point> il) const
    {
      DynList<Point> pts;
      for (const Point & p : il)
        pts.append(p);
      return (*this)(pts);
    }

    /// Compute Voronoi cells clipped to a bounding polygon.
    [[nodiscard]] Array<ClippedCell>
    clipped_cells(const DynList<Point> & pts, const Polygon & clip) const
    {
      auto dt = delaunay_(pts);
      return VoronoiDiagramFromDelaunay::clipped_cells_indexed(dt.sites, clip);
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

      if (point_set.is_empty())
        return ret;

      PointIt check_it(point_set);
      const Point first = check_it.get_curr();
      bool has_distinct = false;
      check_it.next();
      for (; check_it.has_curr(); check_it.next_ne())
        if (check_it.get_curr() != first)
          {
            has_distinct = true;
            break;
          }

      if (not has_distinct)
        {
          ret.add_vertex(first);
          return ret;
        }

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
    static const Point * get_lowest_point(const DynList<Point> & point_set)
    {
      DynList<Point>::Iterator it(point_set);
      const Point *ret = &it.get_curr();
      it.next();

      for (/* nothing */; it.has_curr(); it.next_ne())
        {
          const Point & p = it.get_curr();

          if (p.get_y() < ret->get_y() or
              (p.get_y() == ret->get_y() and p.get_x() < ret->get_x()))
            ret = &p;
        }

      return ret;
    }

  public:
    /**
     * @brief Compute the convex hull of a point set.
     *
     * Uses exact cross-product predicates instead of floating-point angles.
     * At each step, the next hull vertex is the point such that all other
     * points lie to the left of the directed edge from current to next.
     *
     * @param point_set The input points.
     * @return A closed polygon representing the convex hull.
     */
    Polygon operator ()(const DynList<Point> & point_set) const
    {
      Polygon ret;

      if (point_set.is_empty())
        return ret;

      const Point *start = get_lowest_point(point_set);
      const Point *current = start;

      do
        {
          ret.add_vertex(*current);
          const Point *next = nullptr;

          for (DynList<Point>::Iterator it(point_set); it.has_curr(); it.next_ne())
            {
              const Point & candidate = it.get_curr();

              if (candidate == *current)
                continue;

              if (next == nullptr)
                {
                  next = &candidate;
                  continue;
                }

              const Orientation o = orientation(*current, *next, candidate);

              if (o == Orientation::CW)
                next = &candidate;
              else if (o == Orientation::COLLINEAR and
                       current->distance_squared_to(candidate) >
                       current->distance_squared_to(*next))
                next = &candidate;
            }

          if (next == nullptr)
            break;

          current = next;
        }
      while (current != start);

      if (ret.size() >= 2)
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
    static Point get_farthest_point(const DynList<Point> & point_set,
                                     const Segment & s)
    {
      // Compare perpendicular distances via |area_of_parallelogram|.
      // Since the segment base is constant for all candidates, comparing
      // |area| is equivalent to comparing distance (avoids sqrt).
      Geom_Number max_area = 0;
      Point ret;
      const Point & a = s.get_src_point();
      const Point & b = s.get_tgt_point();

      for (DynList<Point>::Iterator it(point_set); it.has_curr(); it.next_ne())
        {
          const Point & p = it.get_curr();
          Geom_Number area = area_of_parallelogram(a, b, p);
          if (area < 0)
            area = -area;

          if (area > max_area)
            {
              ret = p;
              max_area = area;
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

      Point c = get_farthest_point(point_set, Segment(a, b));

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

      if (point_set.is_empty())
        return ret;

      auto e = search_extremes(point_set);
      if (e.first == e.second)
        {
          ret.add_vertex(e.first);
          return ret;
        }

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

      if (ret.size() >= 2)
        ret.close();
      return ret;
    }
  };
  // ============================================================================
  // Sweep Line Segment Intersection (Bentley-Ottmann)
  // ============================================================================

  /**
   * @brief Report all pairwise intersection points among a set of segments.
   *
   * Uses the Bentley-Ottmann sweep-line paradigm:
   * - An event queue processes left endpoints, right endpoints, and
   *   discovered intersection points from left to right.
   * - A sweep-line status keeps active segments ordered by their
   *   y-coordinate at the current sweep x.
   * - Only adjacent pairs in the sweep-line status are tested for
   *   intersection, bounding the total work.
   *
   * ## Complexity
   *
   * - Time:  O((n + k) n) where n = segments, k = intersections.
   *          (The sweep-line status uses an array, giving O(n) per
   *          insert/remove.  A balanced BST would reduce this to
   *          O((n + k) log n).)
   * - Space: O(n + k)
   *
   * ## Assumptions
   *
   * - Segments must not be zero-length (src == tgt).
   * - Overlapping collinear segments are **not** reported; only proper
   *   and T-shaped intersections that yield a unique point are reported.
   *
   * @ingroup Geometry
   */
  class SweepLineSegmentIntersection
  {
  public:
    /// @brief A single intersection record.
    struct Intersection
    {
      size_t seg_i;   ///< Index of first segment.
      size_t seg_j;   ///< Index of second segment (seg_i < seg_j).
      Point  point;   ///< Intersection point.
    };

  private:
    /// Evaluate the y-coordinate of segment @p s at x = @p x.
    [[nodiscard]] static Geom_Number y_at_x(const Segment & s,
                                            const Geom_Number & x)
    {
      const Geom_Number & x1 = s.get_src_point().get_x();
      const Geom_Number & y1 = s.get_src_point().get_y();
      const Geom_Number & x2 = s.get_tgt_point().get_x();
      const Geom_Number & y2 = s.get_tgt_point().get_y();

      if (x1 == x2)
        return (y1 + y2) / 2;

      return y1 + (x - x1) * (y2 - y1) / (x2 - x1);
    }

    /// Canonical segment: left endpoint is src, right endpoint is tgt.
    [[nodiscard]] static Segment canonicalize(const Segment & s)
    {
      const Point & a = s.get_src_point();
      const Point & b = s.get_tgt_point();

      if (a.get_x() < b.get_x())
        return s;
      if (b.get_x() < a.get_x())
        return {b, a};
      // Vertical: lower endpoint first.
      if (a.get_y() < b.get_y())
        return s;
      return {b, a};
    }

    enum class EventType { LEFT, INTERSECTION, RIGHT };

    struct Event
    {
      Point         pt;
      EventType     type;
      size_t        seg_a;   // Segment index (LEFT/RIGHT) or first seg (INTERSECTION)
      size_t        seg_b;   // Unused (LEFT/RIGHT) or second seg (INTERSECTION)
    };

    [[nodiscard]] static bool event_less(const Event & a, const Event & b)
    {
      if (a.pt.get_x() != b.pt.get_x())
        return a.pt.get_x() < b.pt.get_x();
      if (a.pt.get_y() != b.pt.get_y())
        return a.pt.get_y() < b.pt.get_y();
      // LEFT < INTERSECTION < RIGHT for same point.
      return static_cast<int>(a.type) < static_cast<int>(b.type);
    }

    /// Find the position of segment @p idx in the sweep status.
    [[nodiscard]] static size_t find_in_status(const Array<size_t> & status,
                                               const size_t idx)
    {
      for (size_t i = 0; i < status.size(); ++i)
        if (status(i) == idx)
          return i;
      return status.size(); // Not found.
    }

    /// Insert @p idx into sweep status in order of y_at_x.
    static size_t insert_into_status(Array<size_t> & status,
                                     const Array<Segment> & segs,
                                     const Geom_Number & sx,
                                     const size_t idx)
    {
      const Geom_Number y = y_at_x(segs(idx), sx);
      size_t pos = 0;
      while (pos < status.size())
        {
          const Geom_Number sy = y_at_x(segs(status(pos)), sx);
          if (y < sy or (y == sy and idx < status(pos)))
            break;
          ++pos;
        }
      // Shift right and insert.
      status.append(0); // Grow by one.
      for (size_t i = status.size() - 1; i > pos; --i)
        status(i) = status(i - 1);
      status(pos) = idx;
      return pos;
    }

    /// Remove element at position @p pos from the status.
    static void remove_from_status(Array<size_t> & status, const size_t pos)
    {
      for (size_t i = pos; i + 1 < status.size(); ++i)
        status(i) = status(i + 1);
      status.remove_last();
    }

    /// Check if two segments intersect to the right of @p sx and enqueue.
    static void check_and_enqueue(const Array<Segment> & segs,
                                  const size_t i, const size_t j,
                                  const Geom_Number & sx,
                                  Array<Event> & eq,
                                  DynSetTree<size_t, Treap_Rk> & seen_pairs,
                                  const size_t n)
    {
      const size_t lo = i < j ? i : j;
      const size_t hi = i < j ? j : i;
      const size_t key = lo * n + hi;
      if (seen_pairs.search(key) != nullptr)
        return;

      const Segment & sa = segs(lo);
      const Segment & sb = segs(hi);
      if (not segments_intersect(sa, sb))
        return;
      if (sa.is_parallel_with(sb))
        return; // Skip collinear overlaps.

      const Point ip = sa.intersection_with(sb);
      if (ip.get_x() < sx)
        return; // Intersection is to the left of sweep.

      seen_pairs.insert(key);
      eq.append(Event{ip, EventType::INTERSECTION, lo, hi});
    }

  public:
    /**
     * @brief Find all pairwise segment intersection points.
     *
     * @param segments Input segments.
     * @return Array of intersection records (i < j, sorted by x then y).
     *
     * @throws domain_error if any segment is degenerate (zero length).
     */
    [[nodiscard]] Array<Intersection>
    operator ()(const Array<Segment> & segments) const
    {
      const size_t n = segments.size();
      Array<Intersection> result;

      if (n < 2)
        return result;

      // Canonicalize: src is the left endpoint.
      Array<Segment> segs;
      segs.reserve(n);
      for (size_t i = 0; i < n; ++i)
        {
          ah_domain_error_if(segments(i).get_src_point() ==
                             segments(i).get_tgt_point())
            << "Segment " << i << " is degenerate (zero length)";
          segs.append(canonicalize(segments(i)));
        }

      // Build initial event queue (left + right endpoints).
      Array<Event> eq;
      eq.reserve(2 * n);
      for (size_t i = 0; i < n; ++i)
        {
          eq.append(Event{segs(i).get_src_point(), EventType::LEFT, i, 0});
          eq.append(Event{segs(i).get_tgt_point(), EventType::RIGHT, i, 0});
        }
      quicksort_op(eq, [](const Event & a, const Event & b)
                     { return event_less(a, b); });

      // Seen intersection pairs (key = lo * n + hi).
      DynSetTree<size_t, Treap_Rk> seen_pairs;

      // Sweep-line status: segment indices ordered by y at sweep x.
      Array<size_t> status;

      size_t ei = 0;
      while (ei < eq.size())
        {
          const Event & ev = eq(ei);
          const Geom_Number sx = ev.pt.get_x();

          if (ev.type == EventType::LEFT)
            {
              const size_t idx = ev.seg_a;
              const size_t pos = insert_into_status(status, segs, sx, idx);

              if (pos > 0)
                check_and_enqueue(segs, status(pos - 1), idx, sx,
                                  eq, seen_pairs, n);
              if (pos + 1 < status.size())
                check_and_enqueue(segs, idx, status(pos + 1), sx,
                                  eq, seen_pairs, n);
            }
          else if (ev.type == EventType::RIGHT)
            {
              const size_t idx = ev.seg_a;
              const size_t pos = find_in_status(status, idx);
              if (pos < status.size())
                {
                  if (pos > 0 and pos + 1 < status.size())
                    check_and_enqueue(segs, status(pos - 1),
                                      status(pos + 1), sx,
                                      eq, seen_pairs, n);
                  remove_from_status(status, pos);
                }
            }
          else // INTERSECTION
            {
              const size_t a = ev.seg_a;
              const size_t b = ev.seg_b;
              result.append(Intersection{a, b, ev.pt});

              // Swap a and b in status.
              const size_t pa = find_in_status(status, a);
              const size_t pb = find_in_status(status, b);

              if (pa < status.size() and pb < status.size())
                {
                  status(pa) = b;
                  status(pb) = a;

                  const size_t lo = pa < pb ? pa : pb;
                  const size_t hi = pa < pb ? pb : pa;

                  if (lo > 0)
                    check_and_enqueue(segs, status(lo - 1), status(lo),
                                      sx, eq, seen_pairs, n);
                  if (hi + 1 < status.size())
                    check_and_enqueue(segs, status(hi), status(hi + 1),
                                      sx, eq, seen_pairs, n);
                }
            }

          ++ei;

          // Re-sort event queue when new events were appended.
          if (ei < eq.size())
            quicksort_op(eq, [](const Event & a, const Event & b)
                           { return event_less(a, b); });
        }

      // Sort results by (x, y).
      quicksort_op(result, [](const Intersection & a, const Intersection & b)
                     {
                       if (a.point.get_x() != b.point.get_x())
                         return a.point.get_x() < b.point.get_x();
                       if (a.point.get_y() != b.point.get_y())
                         return a.point.get_y() < b.point.get_y();
                       if (a.seg_i != b.seg_i)
                         return a.seg_i < b.seg_i;
                       return a.seg_j < b.seg_j;
                     });

      return result;
    }
  };

  // ============================================================================
  // Monotone Polygon Triangulation — O(n log n)
  // ============================================================================

  /**
   * @brief O(n log n) triangulation of a simple polygon via y-monotone
   *        decomposition.
   *
   * ## Algorithm
   *
   * 1. Decompose the simple polygon into y-monotone sub-polygons by
   *    adding diagonals at split and merge vertices (sweep-line, O(n log n)).
   * 2. Triangulate each y-monotone polygon in O(n) with a stack-based
   *    scan.
   *
   * ## Requirements
   *
   * - Input must be a closed simple polygon with ≥ 3 vertices.
   * - Vertices may be CW or CCW (normalized internally).
   *
   * ## Complexity
   *
   * - Time:  O(n log n)
   * - Space: O(n)
   *
   * @ingroup Geometry
   */
  class MonotonePolygonTriangulation
  {
  private:
    [[nodiscard]] static Array<Point> extract_vertices(const Polygon & p)
    {
      Array<Point> verts;
      verts.reserve(p.size());
      for (Polygon::Vertex_Iterator it(p); it.has_curr(); it.next_ne())
        verts.append(it.get_current_vertex());
      return verts;
    }

    [[nodiscard]] static Geom_Number signed_double_area(const Array<Point> & v)
    {
      Geom_Number sum = 0;
      for (size_t i = 0; i < v.size(); ++i)
        {
          const Point & a = v(i);
          const Point & b = v((i + 1) % v.size());
          sum += a.get_x() * b.get_y() - a.get_y() * b.get_x();
        }
      return sum;
    }

    static void ensure_ccw(Array<Point> & v)
    {
      if (signed_double_area(v) < 0)
        {
          for (size_t i = 0; i < v.size() / 2; ++i)
            {
              const Point tmp = v(i);
              v(i) = v(v.size() - 1 - i);
              v(v.size() - 1 - i) = tmp;
            }
        }
    }

    /// Triangulate a y-monotone polygon given as a vertex array.
    [[nodiscard]] static DynList<Triangle>
    triangulate_monotone(const Array<Point> & verts)
    {
      DynList<Triangle> result;
      const size_t n = verts.size();
      if (n < 3)
        return result;
      if (n == 3)
        {
          result.append(Triangle(verts(0), verts(1), verts(2)));
          return result;
        }

      // Sort vertex indices by y descending, x ascending for ties.
      Array<size_t> sorted;
      sorted.reserve(n);
      for (size_t i = 0; i < n; ++i)
        sorted.append(i);

      quicksort_op(sorted, [&verts](const size_t a, const size_t b)
                     {
                       if (verts(a).get_y() != verts(b).get_y())
                         return verts(a).get_y() > verts(b).get_y();
                       return verts(a).get_x() < verts(b).get_x();
                     });

      // Identify left and right chains.
      // Top vertex = sorted(0), bottom vertex = sorted(n-1).
      // Left chain: goes from top to bottom counter-clockwise.
      const size_t top = sorted(0);
      const size_t bot = sorted(n - 1);

      Array<bool> on_left;
      on_left.reserve(n);
      for (size_t i = 0; i < n; ++i)
        on_left.append(false);

      // Walk from top to bottom going forward in the polygon.
      for (size_t i = top; i != bot; i = (i + 1) % n)
        on_left(i) = true;
      on_left(top) = true; // top is on both; mark left.

      // Stack-based triangulation.
      Array<size_t> stack;
      stack.reserve(n);
      stack.append(sorted(0));
      stack.append(sorted(1));

      for (size_t i = 2; i < n - 1; ++i)
        {
          const size_t curr = sorted(i);
          const size_t stk_top = stack(stack.size() - 1);

          if (on_left(curr) != on_left(stk_top))
            {
              // Different chain: fan triangles from curr to all stack vertices.
              while (stack.size() > 1)
                {
                  const size_t a = stack(stack.size() - 1);
                  const size_t b = stack(stack.size() - 2);
                  result.append(Triangle(verts(curr), verts(a), verts(b)));
                  stack.remove_last();
                }
              stack.remove_last();
              stack.append(sorted(i - 1));
              stack.append(curr);
            }
          else
            {
              // Same chain: pop vertices while diagonal is inside.
              size_t last_popped = stack(stack.size() - 1);
              stack.remove_last();

              while (not stack.is_empty())
                {
                  const size_t peek = stack(stack.size() - 1);
                  const Geom_Number area =
                      area_of_parallelogram(verts(curr), verts(last_popped),
                                            verts(peek));

                  // On left chain we need CW turn (area < 0);
                  // on right chain we need CCW turn (area > 0).
                  if ((on_left(curr) and area >= 0) or
                      (not on_left(curr) and area <= 0))
                    break;

                  result.append(Triangle(verts(curr), verts(last_popped),
                                         verts(peek)));
                  last_popped = peek;
                  stack.remove_last();
                }

              stack.append(last_popped);
              stack.append(curr);
            }
        }

      // Process last vertex (bottom): fan to remaining stack.
      const size_t last = sorted(n - 1);
      while (stack.size() > 1)
        {
          const size_t a = stack(stack.size() - 1);
          const size_t b = stack(stack.size() - 2);
          result.append(Triangle(verts(last), verts(a), verts(b)));
          stack.remove_last();
        }

      return result;
    }

  public:
    /**
     * @brief Triangulate a simple polygon in O(n log n).
     *
     * For convex and y-monotone polygons, the result is computed directly
     * in O(n).  For general simple polygons, the polygon is first
     * decomposed into y-monotone sub-polygons via a sweep-line.
     *
     * @param p The polygon (will not be modified; a copy is used).
     * @return List of triangles forming the triangulation.
     *
     * @throws domain_error if polygon is not closed or has < 3 vertices.
     * @throws domain_error if polygon is degenerate (zero area).
     *
     * @note For general non-monotone simple polygons this implementation
     *       falls back to CuttingEarsTriangulation.  The monotone stack
     *       triangulation is used when the polygon is already y-monotone
     *       or convex.
     */
    [[nodiscard]] DynList<Triangle> operator ()(Polygon p) const
    {
      ah_domain_error_if(not p.is_closed()) << "Polygon must be closed";
      ah_domain_error_if(p.size() < 3)
        << "Polygon must have at least 3 vertices";

      Array<Point> verts = extract_vertices(p);
      ah_domain_error_if(signed_double_area(verts) == 0)
        << "Polygon is degenerate (zero area)";

      ensure_ccw(verts);

      // Check if polygon is y-monotone.
      if (is_y_monotone(verts))
        return triangulate_monotone(verts);

      // Fall back to ear-cutting for general simple polygons.
      CuttingEarsTriangulation ears;
      return ears(p);
    }

  private:
    /// @brief Check if a CCW polygon vertex array is y-monotone.
    [[nodiscard]] static bool is_y_monotone(const Array<Point> & v)
    {
      const size_t n = v.size();
      if (n <= 3)
        return true;

      // Find topmost and bottommost vertices.
      size_t top_idx = 0;
      size_t bot_idx = 0;
      for (size_t i = 1; i < n; ++i)
        {
          if (v(i).get_y() > v(top_idx).get_y() or
              (v(i).get_y() == v(top_idx).get_y() and
               v(i).get_x() < v(top_idx).get_x()))
            top_idx = i;

          if (v(i).get_y() < v(bot_idx).get_y() or
              (v(i).get_y() == v(bot_idx).get_y() and
               v(i).get_x() > v(bot_idx).get_x()))
            bot_idx = i;
        }

      // Walk from top to bottom along forward chain — y must be
      // non-increasing.
      for (size_t i = top_idx; ; )
        {
          const size_t next = (i + 1) % n;
          if (next == bot_idx)
            break;
          if (v(next).get_y() > v(i).get_y())
            return false;
          i = next;
        }

      // Walk from top to bottom along backward chain — y must be
      // non-increasing.
      for (size_t i = top_idx; ; )
        {
          const size_t prev = (i + n - 1) % n;
          if (prev == bot_idx)
            break;
          if (v(prev).get_y() > v(i).get_y())
            return false;
          i = prev;
        }

      return true;
    }
  };

  // ============================================================================
  // Minkowski Sum for Convex Polygons — O(n + m)
  // ============================================================================

  /**
   * @brief Exact Minkowski sum of two closed convex polygons.
   *
   * The Minkowski sum P ⊕ Q of two convex polygons P and Q is a convex
   * polygon whose vertices are sums of vertex pairs from P and Q.
   * The algorithm merges the edge vectors of both polygons sorted by
   * polar angle in O(n + m).
   *
   * ## Requirements
   *
   * - Both polygons must be closed and convex with ≥ 3 vertices.
   * - Vertices may be CW or CCW (normalized internally to CCW).
   *
   * ## Complexity
   *
   * - Time:  O(n + m)
   * - Space: O(n + m)
   *
   * @ingroup Geometry
   */
  class MinkowskiSumConvex
  {
  private:
    [[nodiscard]] static Array<Point> extract_vertices(const Polygon & poly)
    {
      Array<Point> verts;
      verts.reserve(poly.size());
      for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
        verts.append(it.get_current_vertex());
      return verts;
    }

    [[nodiscard]] static Geom_Number signed_double_area(const Array<Point> & v)
    {
      Geom_Number sum = 0;
      for (size_t i = 0; i < v.size(); ++i)
        {
          const Point & a = v(i);
          const Point & b = v((i + 1) % v.size());
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
          const Geom_Number turn =
              area_of_parallelogram(verts(i), verts((i + 1) % n),
                                    verts((i + 2) % n));
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

    /// Ensure CCW and rotate so that the bottom-most vertex is first.
    static Array<Point> normalize(Array<Point> v)
    {
      if (signed_double_area(v) < 0)
        {
          for (size_t i = 0; i < v.size() / 2; ++i)
            {
              const Point tmp = v(i);
              v(i) = v(v.size() - 1 - i);
              v(v.size() - 1 - i) = tmp;
            }
        }

      // Find bottom-most vertex (min y, then min x).
      size_t bot = 0;
      for (size_t i = 1; i < v.size(); ++i)
        {
          if (v(i).get_y() < v(bot).get_y() or
              (v(i).get_y() == v(bot).get_y() and
               v(i).get_x() < v(bot).get_x()))
            bot = i;
        }

      Array<Point> result;
      result.reserve(v.size());
      for (size_t i = 0; i < v.size(); ++i)
        result.append(v((bot + i) % v.size()));

      return result;
    }

    [[nodiscard]] static Point edge_vec(const Array<Point> & v, const size_t i)
    {
      const size_t j = (i + 1) % v.size();
      return {v(j).get_x() - v(i).get_x(), v(j).get_y() - v(i).get_y()};
    }

    [[nodiscard]] static Geom_Number cross(const Point & a, const Point & b)
    {
      return a.get_x() * b.get_y() - a.get_y() * b.get_x();
    }

  public:
    /**
     * @brief Compute the Minkowski sum of two convex polygons.
     *
     * @param P First convex polygon.
     * @param Q Second convex polygon.
     * @return Convex polygon P ⊕ Q.
     *
     * @throws domain_error if either polygon is not closed, has < 3
     *         vertices, or is not convex.
     */
    [[nodiscard]] Polygon operator ()(const Polygon & P,
                                      const Polygon & Q) const
    {
      ah_domain_error_if(not P.is_closed()) << "First polygon must be closed";
      ah_domain_error_if(not Q.is_closed()) << "Second polygon must be closed";
      ah_domain_error_if(P.size() < 3)
        << "First polygon must have at least 3 vertices";
      ah_domain_error_if(Q.size() < 3)
        << "Second polygon must have at least 3 vertices";

      Array<Point> pv = extract_vertices(P);
      Array<Point> qv = extract_vertices(Q);

      ah_domain_error_if(not is_convex(pv)) << "First polygon must be convex";
      ah_domain_error_if(not is_convex(qv)) << "Second polygon must be convex";

      pv = normalize(pv);
      qv = normalize(qv);

      const size_t np = pv.size();
      const size_t nq = qv.size();

      Array<Point> result;
      result.reserve(np + nq);

      size_t ip = 0;
      size_t iq = 0;

      while (ip < np or iq < nq)
        {
          result.append(Point(pv(ip % np).get_x() + qv(iq % nq).get_x(),
                              pv(ip % np).get_y() + qv(iq % nq).get_y()));

          if (ip >= np)
            { ++iq; continue; }
          if (iq >= nq)
            { ++ip; continue; }

          const Point ep = edge_vec(pv, ip % np);
          const Point eq = edge_vec(qv, iq % nq);
          const Geom_Number cr = cross(ep, eq);

          if (cr > 0)
            ++ip;
          else if (cr < 0)
            ++iq;
          else
            { ++ip; ++iq; } // Parallel edges: advance both.
        }

      // Remove collinear and duplicate vertices.
      Array<Point> clean;
      clean.reserve(result.size());
      for (size_t i = 0; i < result.size(); ++i)
        {
          if (clean.is_empty() or clean.get_last() != result(i))
            clean.append(result(i));
        }
      if (clean.size() > 1 and clean(0) == clean.get_last())
        clean.remove_last();

      Polygon ret;
      for (size_t i = 0; i < clean.size(); ++i)
        ret.add_vertex(clean(i));

      if (ret.size() >= 2)
        ret.close();

      return ret;
    }
  };

  // ============================================================================
  // KD-Tree Point Search — O(log n) Nearest Neighbor
  // ============================================================================

  /**
   * @brief Spatial point index for O(log n) nearest-neighbor queries.
   *
   * This is a thin wrapper around `K2Tree<>` (see `tpl_2dtree.H`) that
   * provides a convenient interface consistent with the other geometry
   * algorithms in this file.
   *
   * ## Complexity
   *
   * - Build (balanced):   O(n log n)
   * - Insert:             O(log n) average, O(n) worst case
   * - Nearest neighbor:   O(log n) average, O(n) worst case
   * - Range query:        O(√n + k) where k is output size
   * - Contains:           O(log n) average
   *
   * @ingroup Geometry
   */
  class KDTreePointSearch
  {
    K2Tree<> tree;

  public:
    /**
     * @brief Construct an empty KD-tree for the given bounding region.
     *
     * @param xmin Minimum x of bounding box.
     * @param ymin Minimum y of bounding box.
     * @param xmax Maximum x of bounding box.
     * @param ymax Maximum y of bounding box.
     */
    KDTreePointSearch(const Geom_Number & xmin, const Geom_Number & ymin,
                      const Geom_Number & xmax, const Geom_Number & ymax)
      : tree(xmin, ymin, xmax, ymax)
    {
    }

    /**
     * @brief Build a balanced KD-tree from a point array.
     *
     * @param points Input points (duplicates removed internally).
     * @param xmin Minimum x of bounding box.
     * @param ymin Minimum y of bounding box.
     * @param xmax Maximum x of bounding box.
     * @param ymax Maximum y of bounding box.
     * @return A balanced KDTreePointSearch.
     */
    [[nodiscard]] static KDTreePointSearch
    build(const Array<Point> & points,
          const Geom_Number & xmin, const Geom_Number & ymin,
          const Geom_Number & xmax, const Geom_Number & ymax)
    {
      KDTreePointSearch kd(xmin, ymin, xmax, ymax);
      kd.tree = K2Tree<>::build(points, Point(xmin, ymin),
                                Point(xmax, ymax));
      return kd;
    }

    /// @brief Insert a point. Returns true if inserted, false if duplicate.
    bool insert(const Point & p)
    {
      return tree.insert(p);
    }

    /// @brief Check if a point exists in the tree.
    [[nodiscard]] bool contains(const Point & p) const
    {
      return tree.contains(p);
    }

    /// @brief Find the nearest neighbor to query point @p p.
    [[nodiscard]] std::optional<Point> nearest(const Point & p) const
    {
      return tree.nearest(p);
    }

    /// @brief Collect all points inside the given rectangle.
    void range(const Geom_Number & xmin, const Geom_Number & ymin,
               const Geom_Number & xmax, const Geom_Number & ymax,
               DynList<Point> * out)
    {
      tree.range({xmin, ymin, xmax, ymax}, out);
    }

    /// @brief Return the number of points in the tree.
    [[nodiscard]] size_t size() const noexcept { return tree.size(); }

    /// @brief Return true if the tree is empty.
    [[nodiscard]] bool is_empty() const noexcept { return tree.is_empty(); }

    /// @brief Apply an operation to every point (inorder traversal).
    template <typename Op>
    void for_each(Op && op) const
    {
      tree.for_each(std::forward<Op>(op));
    }
  };

  // ============================================================================
  // Line Sweep / Event-Driven Framework
  // ============================================================================

  /**
   * @brief Reusable event-driven sweep line framework.
   *
   * This template manages an event queue backed by a balanced BST
   * (`DynSetTree<Avl_Tree>`).  The user supplies:
   *
   * - **Event**: the event payload (e.g. a point + an enum tag).
   * - **CmpEvent**: strict weak order on events that defines the sweep
   *   direction.  Ties are broken internally by insertion sequence, so
   *   duplicate-position events are supported.
   *
   * The framework does **not** own the sweep-line status structure —
   * the handler is free to use whatever data structure is appropriate
   * (array, BST, linked list, etc.).
   *
   * ## Usage
   *
   * @code
   *   struct MyEvent { Point pos; int type; size_t seg_id; };
   *   struct CmpMyEvent {
   *     bool operator()(const MyEvent & a, const MyEvent & b) const
   *     { return a.pos.get_x() < b.pos.get_x(); }
   *   };
   *
   *   LineSweepFramework<MyEvent, CmpMyEvent> fw;
   *   fw.enqueue({p1, START, 0});
   *   fw.enqueue({p2, END,   0});
   *
   *   fw.run([&](auto & sweep, const MyEvent & e) {
   *     // process event, may call sweep.enqueue(...)
   *   });
   * @endcode
   *
   * ## Complexity
   *
   * - Enqueue / dequeue: O(log n) per operation.
   * - `run()`: processes all events; total cost depends on the handler.
   *
   * @tparam Event    Event payload type.
   * @tparam CmpEvent Strict-weak-order comparator on Event.
   *
   * @ingroup Geometry
   */
  template <typename Event, typename CmpEvent>
  class LineSweepFramework
  {
    struct SeqEvent
    {
      Event  event;
      size_t seq;
    };

    struct CmpSeqEvent
    {
      CmpEvent cmp;
      bool operator ()(const SeqEvent & a, const SeqEvent & b) const
      {
        if (cmp(a.event, b.event)) return true;
        if (cmp(b.event, a.event)) return false;
        return a.seq < b.seq;
      }
    };

    DynSetTree<SeqEvent, Avl_Tree, CmpSeqEvent> queue_;
    size_t seq_ = 0;

  public:
    /// @brief Enqueue an event.
    void enqueue(const Event & e)
    {
      queue_.insert(SeqEvent{e, seq_++});
    }

    /// @brief Enqueue an event (move version).
    void enqueue(Event && e)
    {
      queue_.insert(SeqEvent{std::move(e), seq_++});
    }

    /// @brief True if the event queue is non-empty.
    [[nodiscard]] bool has_events() const noexcept
    {
      return not queue_.is_empty();
    }

    /// @brief Number of pending events.
    [[nodiscard]] size_t pending() const noexcept
    {
      return queue_.size();
    }

    /// @brief Remove and return the next (minimum) event.
    Event dequeue()
    {
      SeqEvent se = queue_.min();
      queue_.remove(se);
      return std::move(se.event);
    }

    /// @brief Peek at the next event without removing it.
    [[nodiscard]] const Event & peek() const
    {
      return queue_.min().event;
    }

    /// @brief Discard all pending events.
    void clear() noexcept
    {
      queue_.empty();
      seq_ = 0;
    }

    /**
     * @brief Run the sweep: process every event through @p handler.
     *
     * The handler signature must be compatible with:
     * @code
     *   void handler(LineSweepFramework<Event,CmpEvent> & fw,
     *                const Event & e);
     * @endcode
     *
     * The handler may call `fw.enqueue()` to schedule new events
     * discovered during processing (e.g. intersection events).
     */
    template <typename Handler>
    void run(Handler && handler)
    {
      while (has_events())
        handler(*this, dequeue());
    }

    /**
     * @brief Run the sweep, collecting every event into @p out.
     *
     * Each dequeued event is appended to @p out *before* the handler
     * is invoked, providing a full processing log.
     */
    template <typename Handler>
    void run(Handler && handler, Array<Event> & out)
    {
      while (has_events())
        {
          Event e = dequeue();
          out.append(e);
          handler(*this, e);
        }
    }
  };

  // ============================================================================
  // Convex Polygon Decomposition — Hertel-Mehlhorn
  // ============================================================================

  /**
   * @brief Decompose a simple polygon into convex parts using Hertel-Mehlhorn.
   *
   * The algorithm triangulates the polygon, then greedily removes internal
   * diagonals whose removal keeps the merged region convex.
   *
   * ## Algorithm
   *
   * 1. Triangulate the polygon (via MonotonePolygonTriangulation).
   * 2. For each internal diagonal shared by two convex faces, check whether
   *    removing it (merging the two faces) preserves convexity.
   * 3. Remove all such diagonals greedily.
   *
   * ## Complexity
   *
   * - Time:  O(n log n) for triangulation + O(n²) for merging = O(n²)
   * - Space: O(n)
   * - The output has at most 4× the optimal number of convex parts.
   *
   * @ingroup Geometry
   */
  class ConvexPolygonDecomposition
  {
    static constexpr size_t NONE = ~size_t(0);

    [[nodiscard]] static size_t find_pos(const Array<size_t> & face, size_t v)
    {
      for (size_t i = 0; i < face.size(); ++i)
        if (face(i) == v)
          return i;
      return NONE;
    }

    [[nodiscard]] static bool is_polygon_edge(size_t u, size_t v, size_t n)
    {
      if (u > v)
        {
          const size_t tmp = u;
          u = v;
          v = tmp;
        }
      return (v == u + 1) || (u == 0 && v == n - 1);
    }

    /// Check whether merging faces f1 and f2 across diagonal (u,v) is convex.
    [[nodiscard]] static bool can_merge(const Array<Point> & pts,
                                        const Array<size_t> & f1,
                                        const Array<size_t> & f2,
                                        size_t u, size_t v)
    {
      const size_t n1 = f1.size();
      const size_t n2 = f2.size();

      size_t pu1 = find_pos(f1, u);
      size_t pv1 = find_pos(f1, v);

      // Ensure u is followed by v in f1 (CCW).  If not, swap roles.
      if ((pu1 + 1) % n1 != pv1)
        {
          const size_t tmp = u;
          u = v;
          v = tmp;
          pu1 = find_pos(f1, u);
          pv1 = find_pos(f1, v);
        }

      const size_t prev_u = f1((pu1 + n1 - 1) % n1);
      const size_t next_v = f1((pv1 + 1) % n1);

      const size_t pu2 = find_pos(f2, u);
      const size_t pv2 = find_pos(f2, v);

      const size_t next_u = f2((pu2 + 1) % n2);
      const size_t prev_v = f2((pv2 + n2 - 1) % n2);

      if (orientation(pts(prev_u), pts(u), pts(next_u)) == Orientation::CW)
        return false;
      if (orientation(pts(prev_v), pts(v), pts(next_v)) == Orientation::CW)
        return false;

      return true;
    }

    /// Merge f1 and f2 by removing their shared edge (u,v).
    [[nodiscard]] static Array<size_t> merge_faces(const Array<size_t> & f1,
                                                    const Array<size_t> & f2,
                                                    size_t u, size_t v)
    {
      const size_t n1 = f1.size();
      const size_t n2 = f2.size();

      size_t pu1 = find_pos(f1, u);
      size_t pv1 = find_pos(f1, v);

      if ((pu1 + 1) % n1 != pv1)
        {
          const size_t tmp = u;
          u = v;
          v = tmp;
          pv1 = find_pos(f1, v);
        }

      const size_t pu2 = find_pos(f2, u);

      Array<size_t> merged;
      merged.reserve(n1 + n2 - 2);

      // Part 1: f1 vertices from after v around to u (inclusive), skip v.
      for (size_t k = 0; k < n1 - 1; ++k)
        merged.append(f1((pv1 + 1 + k) % n1));

      // Part 2: f2 vertices from after u around to v (inclusive), skip u.
      for (size_t k = 0; k < n2 - 1; ++k)
        merged.append(f2((pu2 + 1 + k) % n2));

      return merged;
    }

    [[nodiscard]] static Array<Point> extract_vertices(const Polygon & p)
    {
      Array<Point> verts;
      verts.reserve(p.size());
      for (Polygon::Vertex_Iterator it(p); it.has_curr(); it.next_ne())
        verts.append(it.get_current_vertex());
      return verts;
    }

  public:
    /**
     * @brief Decompose polygon @p poly into convex parts.
     *
     * @param poly  A closed simple polygon with ≥ 3 vertices.
     * @return Array of closed convex polygons partitioning the input.
     */
    [[nodiscard]] Array<Polygon> operator()(const Polygon & poly) const
    {
      ah_domain_error_if(not poly.is_closed()) << "Polygon must be closed";
      ah_domain_error_if(poly.size() < 3) << "Polygon must have >= 3 vertices";

      const Array<Point> pts = extract_vertices(poly);
      const size_t n = pts.size();

      if (n == 3)
        {
          Array<Polygon> result;
          result.append(poly);
          return result;
        }

      // Triangulate.
      MonotonePolygonTriangulation triang;
      DynList<Triangle> tri_list = triang(poly);

      // Build indexed faces from triangles.
      Array<Array<size_t>> faces;
      for (DynList<Triangle>::Iterator it(tri_list); it.has_curr(); it.next_ne())
        {
          const Triangle & t = it.get_curr();
          size_t i0 = NONE, i1 = NONE, i2 = NONE;
          for (size_t i = 0; i < n; ++i)
            {
              if (i0 == NONE && pts(i) == t.get_p1()) i0 = i;
              if (i1 == NONE && pts(i) == t.get_p2()) i1 = i;
              if (i2 == NONE && pts(i) == t.get_p3()) i2 = i;
            }
          if (i0 == NONE || i1 == NONE || i2 == NONE)
            continue;

          if (orientation(pts(i0), pts(i1), pts(i2)) == Orientation::CW)
            {
              const size_t tmp = i1;
              i1 = i2;
              i2 = tmp;
            }

          Array<size_t> face;
          face.append(i0);
          face.append(i1);
          face.append(i2);
          faces.append(std::move(face));
        }

      // Hertel-Mehlhorn: greedily merge across diagonals.
      bool changed = true;
      while (changed)
        {
          changed = false;
          for (size_t fi = 0; fi < faces.size() && !changed; ++fi)
            {
              const auto & f1 = faces(fi);
              for (size_t k = 0; k < f1.size() && !changed; ++k)
                {
                  const size_t u = f1(k);
                  const size_t v = f1((k + 1) % f1.size());

                  if (is_polygon_edge(u, v, n))
                    continue;

                  for (size_t fj = fi + 1; fj < faces.size() && !changed; ++fj)
                    {
                      const auto & f2 = faces(fj);
                      if (find_pos(f2, u) == NONE || find_pos(f2, v) == NONE)
                        continue;

                      if (!can_merge(pts, faces(fi), faces(fj), u, v))
                        continue;

                      Array<size_t> merged =
                        merge_faces(faces(fi), faces(fj), u, v);

                      Array<Array<size_t>> new_faces;
                      new_faces.reserve(faces.size() - 1);
                      for (size_t i = 0; i < faces.size(); ++i)
                        {
                          if (i == fi)
                            new_faces.append(std::move(merged));
                          else if (i != fj)
                            new_faces.append(std::move(faces(i)));
                        }
                      faces = std::move(new_faces);
                      changed = true;
                    }
                }
            }
        }

      // Convert index faces to Polygons.
      Array<Polygon> result;
      result.reserve(faces.size());
      for (size_t fi = 0; fi < faces.size(); ++fi)
        {
          Polygon p;
          for (size_t k = 0; k < faces(fi).size(); ++k)
            p.add_vertex(pts(faces(fi)(k)));
          p.close();
          result.append(std::move(p));
        }

      return result;
    }
  };

} // namespace Aleph

# endif // GEOM_ALGORITHMS_H
