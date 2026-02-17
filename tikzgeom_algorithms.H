# ifndef TIKZGEOM_ALGORITHMS_H
# define TIKZGEOM_ALGORITHMS_H

# include "geom_algorithms.H"
# include "tikzgeom.H"

namespace Aleph
{
  namespace detail
  {
    inline std::string tikz_palette_color(const size_t idx)
    {
      static const char *colors[] =
          {
            "blue!20", "orange!25", "green!20", "red!20", "cyan!22",
            "magenta!20", "yellow!28", "teal!22", "lime!24", "brown!20"
          };
      static constexpr size_t kNumColors = sizeof(colors) / sizeof(colors[0]);
      return colors[idx % kNumColors];
    }

    struct SPP_ITri
    {
      size_t v[3];
      size_t adj[3];
    };

    struct SPP_Portal
    {
      Point left;
      Point right;
    };

    constexpr size_t SPP_NONE = ~static_cast<size_t>(0);

    [[nodiscard]] inline size_t spp_find_index(const Array<Point> & pts,
                                               const Point & p)
    {
      for (size_t i = 0; i < pts.size(); ++i)
        if (pts(i) == p) return i;
      return SPP_NONE;
    }

    [[nodiscard]] inline Array<SPP_ITri> spp_build_tris(const Array<Point> & pts,
                                                        const DynList<Triangle> & tl)
    {
      Array<SPP_ITri> tris;
      for (DynList<Triangle>::Iterator it(tl); it.has_curr(); it.next_ne())
        {
          const Triangle & t = it.get_curr();
          SPP_ITri ti{};
          ti.v[0] = spp_find_index(pts, t.get_p1());
          ti.v[1] = spp_find_index(pts, t.get_p2());
          ti.v[2] = spp_find_index(pts, t.get_p3());
          ti.adj[0] = ti.adj[1] = ti.adj[2] = SPP_NONE;
          tris.append(ti);
        }

      struct EdgeEntry
      {
        size_t u;
        size_t v;
        size_t tri;
        size_t local;
      };

      Array<EdgeEntry> edges;
      edges.reserve(tris.size() * 3);

      for (size_t ti = 0; ti < tris.size(); ++ti)
        for (int e = 0; e < 3; ++e)
          {
            size_t u = tris(ti).v[(e + 1) % 3];
            size_t v = tris(ti).v[(e + 2) % 3];
            if (u > v)
              std::swap(u, v);
            edges.append(EdgeEntry{u, v, ti, static_cast<size_t>(e)});
          }

      quicksort_op(edges, [](const EdgeEntry & a, const EdgeEntry & b)
                     {
                       if (a.u != b.u) return a.u < b.u;
                       if (a.v != b.v) return a.v < b.v;
                       return a.tri < b.tri;
                     });

      for (size_t i = 0; i + 1 < edges.size(); ++i)
        if (edges(i).u == edges(i + 1).u and edges(i).v == edges(i + 1).v)
          {
            const size_t t1 = edges(i).tri;
            const size_t l1 = edges(i).local;
            const size_t t2 = edges(i + 1).tri;
            const size_t l2 = edges(i + 1).local;
            tris(t1).adj[l1] = t2;
            tris(t2).adj[l2] = t1;
            ++i;
          }

      return tris;
    }

    [[nodiscard]] inline bool spp_point_in_triangle(const Array<Point> & pts,
                                                    const SPP_ITri & t,
                                                    const Point & p)
    {
      const Orientation o0 = orientation(pts(t.v[0]), pts(t.v[1]), p);
      const Orientation o1 = orientation(pts(t.v[1]), pts(t.v[2]), p);
      const Orientation o2 = orientation(pts(t.v[2]), pts(t.v[0]), p);
      const bool has_cw = o0 == Orientation::CW or o1 == Orientation::CW
                          or o2 == Orientation::CW;
      const bool has_ccw = o0 == Orientation::CCW or o1 == Orientation::CCW
                           or o2 == Orientation::CCW;
      return not (has_cw and has_ccw);
    }

    [[nodiscard]] inline size_t spp_find_tri(const Array<Point> & pts,
                                             const Array<SPP_ITri> & tris,
                                             const Point & p)
    {
      for (size_t i = 0; i < tris.size(); ++i)
        if (spp_point_in_triangle(pts, tris(i), p))
          return i;
      return SPP_NONE;
    }

    [[nodiscard]] inline Array<size_t> spp_find_sleeve(const Array<SPP_ITri> & tris,
                                                       const size_t src,
                                                       const size_t dst)
    {
      if (src == dst)
        {
          Array<size_t> s;
          s.append(src);
          return s;
        }

      Array<size_t> parent;
      parent.reserve(tris.size());
      for (size_t i = 0; i < tris.size(); ++i)
        parent.append(SPP_NONE);
      parent(src) = src;

      DynList<size_t> queue;
      queue.append(src);

      while (not queue.is_empty())
        {
          const size_t cur = queue.remove_first();
          if (cur == dst)
            break;

          for (unsigned long nb: tris(cur).adj)
            if (nb != SPP_NONE and parent(nb) == SPP_NONE)
              {
                parent(nb) = cur;
                queue.append(nb);
              }
        }

      Array<size_t> path;
      if (parent(dst) == SPP_NONE)
        return path;

      for (size_t cur = dst; cur != src; cur = parent(cur))
        path.append(cur);
      path.append(src);

      for (size_t i = 0; i < path.size() / 2; ++i)
        std::swap(path(i), path(path.size() - 1 - i));

      return path;
    }

    [[nodiscard]] inline Geom_Number spp_cross(const Point & a,
                                               const Point & b,
                                               const Point & c)
    {
      return (b.get_x() - a.get_x()) * (c.get_y() - a.get_y()) -
             (b.get_y() - a.get_y()) * (c.get_x() - a.get_x());
    }

    [[nodiscard]] inline Array<SPP_Portal>
    shortest_path_portals(const Polygon & polygon,
                          const Point & source,
                          const Point & target)
    {
      ah_domain_error_if(not polygon.is_closed()) << "Polygon must be closed";
      ah_domain_error_if(polygon.size() < 3) << "Polygon must have >= 3 vertices";
      ah_domain_error_if(not PointInPolygonWinding::contains(polygon, source))
        << "Source must be inside the polygon";
      ah_domain_error_if(not PointInPolygonWinding::contains(polygon, target))
        << "Target must be inside the polygon";

      Array<SPP_Portal> portals;
      if (source == target)
        {
          portals.append(SPP_Portal{source, source});
          return portals;
        } {
        const Segment seg(source, target);
        bool blocked = false;
        for (Polygon::Segment_Iterator it(polygon); it.has_curr() and not blocked; it.next_ne())
          if (const Segment edge = it.get_current_segment();
            seg.intersects_properly_with(edge))
            blocked = true;
        if (not blocked)
          {
            portals.append(SPP_Portal{source, source});
            portals.append(SPP_Portal{target, target});
            return portals;
          }
      }

      Array<Point> pts;
      for (Polygon::Vertex_Iterator it(polygon); it.has_curr(); it.next_ne())
        pts.append(it.get_current_vertex());

      CuttingEarsTriangulation triangulator;
      const DynList<Triangle> tri_list = triangulator(polygon);

      ah_domain_error_if(tri_list.is_empty()) << "Triangulation failed";

      const Array<SPP_ITri> tris = spp_build_tris(pts, tri_list);
      const size_t src_t = spp_find_tri(pts, tris, source);
      const size_t dst_t = spp_find_tri(pts, tris, target);

      ah_domain_error_if(src_t == SPP_NONE) << "Could not locate source in triangulation";
      ah_domain_error_if(dst_t == SPP_NONE) << "Could not locate target in triangulation";

      const Array<size_t> sleeve = spp_find_sleeve(tris, src_t, dst_t);

      portals.append(SPP_Portal{source, source});

      if (sleeve.size() <= 1)
        {
          portals.append(SPP_Portal{target, target});
          return portals;
        }

      for (size_t i = 0; i + 1 < sleeve.size(); ++i)
        {
          size_t s0 = 0, s1 = 0;
          size_t v_prev = 0;

          int sc = 0;
          bool found[3] = {false, false, false};
          for (int a = 0; a < 3; ++a)
            for (const unsigned long b: tris(sleeve(i + 1)).v)
              if (tris(sleeve(i)).v[a] == b)
                {
                  found[a] = true;
                  break;
                }

          for (int a = 0; a < 3; ++a)
            if (found[a])
              {
                if (sc == 0) s0 = tris(sleeve(i)).v[a];
                else s1 = tris(sleeve(i)).v[a];
                ++sc;
              }
            else
              v_prev = tris(sleeve(i)).v[a];

          const Geom_Number c = spp_cross(pts(v_prev), pts(s0), pts(s1));
          if (c < 0)
            portals.append(SPP_Portal{pts(s0), pts(s1)});
          else
            portals.append(SPP_Portal{pts(s1), pts(s0)});
        }

      portals.append(SPP_Portal{target, target});
      return portals;
    }
  } // namespace detail

  /** @file tikzgeom_algorithms.H
   *  @ingroup Geometry
   *  @brief Helpers to visualize computational-geometry algorithm results in TikZ.
   *
   *  This header builds on top of `tikzgeom.H` and provides convenience
   *  functions for common outputs:
   *  - point clouds
   *  - convex hull overlays
   *  - polygon intersection / boolean results
   *  - Delaunay triangulation
   *  - Voronoi and Power diagrams
   */

  /** @brief Creates a style optimized for point clouds.
   *
   *  Generates a @ref Tikz_Style where points are drawn as small filled circles.
   *  The draw and fill colors are set to the same value.
   *
   *  @param color The color for the points.
   *  @param opacity The opacity for the points (0.0 to 1.0). -1 means default.
   *  @return A @ref Tikz_Style suitable for `put_points`.
   */
  inline Tikz_Style tikz_points_style(const std::string & color = "black",
                                      const double opacity = -1.0)
  {
    Tikz_Style s;
    s.draw_color = color;
    s.fill_color = color;
    s.opacity = opacity;
    return s;
  }

  /** @brief Creates a style optimized for wireframe segments and polygons.
   *
   *  @param color The stroke color.
   *  @param dashed Whether to use a dashed line style.
   *  @param with_arrow Whether to add an arrowhead to segments.
   *  @return A @ref Tikz_Style suitable for drawing outlines.
   */
  inline Tikz_Style tikz_wire_style(const std::string & color = "black",
                                    const bool dashed = false,
                                    const bool with_arrow = false)
  {
    Tikz_Style s;
    s.draw_color = color;
    s.dashed = dashed;
    s.with_arrow = with_arrow;
    return s;
  }

  /** @brief Creates a style optimized for polyline paths.
   *
   *  This style is intended for highlighting paths, such as the result of a
   *  shortest-path algorithm. It defaults to a thick, solid line.
   *
   *  @param color The path color.
   *  @param with_arrow Whether to add an arrowhead to the path segments.
   *  @return A prominent @ref Tikz_Style for paths.
   */
  inline Tikz_Style tikz_path_style(const std::string & color = "red",
                                    const bool with_arrow = false)
  {
    Tikz_Style s = tikz_wire_style(color, false, with_arrow);
    s.thick = true;
    return s;
  }

  /** @brief Creates a style for drawing filled polygons.
   *
   *  This style is used for representing areas, like polygon intersections or
   *  Voronoi cells. It enables filling and sets an opacity for visualization.
   *
   *  @param draw_color The stroke color for the polygon boundary.
   *  @param fill_color The fill color for the polygon's interior.
   *  @param opacity The opacity of the fill (0.0 to 1.0).
   *  @return A @ref Tikz_Style for filled shapes.
   */
  inline Tikz_Style tikz_area_style(const std::string & draw_color = "black",
                                    const std::string & fill_color = "gray!25",
                                    const double opacity = 0.6)
  {
    Tikz_Style s;
    s.draw_color = draw_color;
    s.fill_color = fill_color;
    s.fill = true;
    s.opacity = opacity;
    return s;
  }

  /** @brief Inserts all points from an `Array<Point>` into the plane.
   *
   *  @param plane The target TikZ plane.
   *  @param pts An array of points to insert.
   *  @param style The style to apply to each point.
   *  @param layer The layer on which to draw the points.
   */
  inline void put_points(Tikz_Plane & plane, const Array<Point> & pts,
                         const Tikz_Style & style = tikz_points_style(),
                         const int layer = Tikz_Plane::Layer_Default)
  {
    for (size_t i = 0; i < pts.size(); ++i)
      put_in_plane(plane, pts(i), style, layer);
  }

  /** @brief Inserts all points from a `DynList<Point>` into the plane.
   *
   *  @param plane The target TikZ plane.
   *  @param pts A dynamic list of points to insert.
   *  @param style The style to apply to each point.
   *  @param layer The layer on which to draw the points.
   */
  inline void put_points(Tikz_Plane & plane, const DynList<Point> & pts,
                         const Tikz_Style & style = tikz_points_style(),
                         const int layer = Tikz_Plane::Layer_Default)
  {
    for (DynList<Point>::Iterator it(pts); it.has_curr(); it.next_ne())
      put_in_plane(plane, it.get_curr(), style, layer);
  }

  /** @brief Adds a text label to each point in an `Array<Point>`.
   *
   *  Labels are generated as `prefix + index`. For example, `p0`, `p1`, etc.
   *
   *  @param plane The target TikZ plane.
   *  @param pts The array of points to label.
   *  @param prefix A string prefix for each label.
   *  @param placement The TikZ placement string (e.g., "above", "below left").
   *  @param style The style for the text labels.
   *  @param layer The layer on which to draw the labels.
   */
  inline void put_point_labels(Tikz_Plane & plane,
                               const Array<Point> & pts,
                               const std::string & prefix = "p",
                               const std::string & placement = "above right",
                               const Tikz_Style & style = make_tikz_draw_style("black"),
                               const int layer = Tikz_Plane::Layer_Overlay)
  {
    for (size_t i = 0; i < pts.size(); ++i)
      put_point_label_in_plane(plane, pts(i),
                               prefix + std::to_string(i),
                               placement, style, layer);
  }

  /** @brief Adds a text label to each point in a `DynList<Point>`.
   *
   *  Labels are generated as `prefix + index` based on iteration order.
   *
   *  @param plane The target TikZ plane.
   *  @param pts The list of points to label.
   *  @param prefix A string prefix for each label.
   *  @param placement The TikZ placement string (e.g., "above", "below left").
   *  @param style The style for the text labels.
   *  @param layer The layer on which to draw the labels.
   */
  inline void put_point_labels(Tikz_Plane & plane,
                               const DynList<Point> & pts,
                               const std::string & prefix = "p",
                               const std::string & placement = "above right",
                               const Tikz_Style & style = make_tikz_draw_style("black"),
                               const int layer = Tikz_Plane::Layer_Overlay)
  {
    size_t idx = 0;
    for (DynList<Point>::Iterator it(pts); it.has_curr(); it.next_ne(), ++idx)
      put_point_label_in_plane(plane, it.get_curr(),
                               prefix + std::to_string(idx),
                               placement, style, layer);
  }

  /** @brief Inserts all polygons from an `Array<Polygon>` into the plane.
   *
   *  @param plane The target TikZ plane.
   *  @param polys An array of polygons to insert.
   *  @param style The style to apply to each polygon.
   *  @param layer The layer on which to draw the polygons.
   */
  inline void put_polygons(Tikz_Plane & plane, const Array<Polygon> & polys,
                           const Tikz_Style & style = tikz_wire_style(),
                           const int layer = Tikz_Plane::Layer_Default)
  {
    for (size_t i = 0; i < polys.size(); ++i)
      put_in_plane(plane, polys(i), style, layer);
  }

  /** @brief Inserts the vertices of a polygon as styled points.
   *
   *  @param plane The target TikZ plane.
   *  @param poly The polygon whose vertices are to be drawn.
   *  @param style The style for the vertex points.
   *  @param layer The layer on which to draw the points.
   */
  inline void put_polygon_vertices(Tikz_Plane & plane, const Polygon & poly,
                                   const Tikz_Style & style = tikz_points_style(),
                                   const int layer = Tikz_Plane::Layer_Overlay)
  {
    for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
      put_in_plane(plane, it.get_current_vertex().to_point(), style, layer);
  }

  /** @brief Inserts the vertices of a regular polygon as styled points.
   *
   *  @param plane The target TikZ plane.
   *  @param poly The regular polygon whose vertices are to be drawn.
   *  @param style The style for the vertex points.
   *  @param layer The layer on which to draw the points.
   */
  inline void put_polygon_vertices(Tikz_Plane & plane, const Regular_Polygon & poly,
                                   const Tikz_Style & style = tikz_points_style(),
                                   const int layer = Tikz_Plane::Layer_Overlay)
  {
    for (size_t i = 0; i < poly.size(); ++i)
      put_in_plane(plane, poly.get_vertex(i), style, layer);
  }

  /** @brief Constructs a `Polygon` from an ordered array of vertices.
   *
   *  @param vertices An array of `Point`s in the desired order.
   *  @param close If true, closes the polygon by adding an edge from the last
   *         vertex back to the first.
   *  @return The constructed `Polygon`.
   */
  inline Polygon polygon_from_vertices(const Array<Point> & vertices,
                                       const bool close = true)
  {
    Polygon poly;
    for (size_t i = 0; i < vertices.size(); ++i)
      poly.add_vertex(vertices(i));

    if (close and poly.size() >= 2)
      poly.close();

    return poly;
  }

  /** @brief Constructs a `Polygon` from a list of indices into a vertex array.
   *
   *  This is useful for algorithms that return faces or boundaries as lists of
   *  indices (e.g., `SegmentArrangement`).
   *
   *  @param vertices The master array of `Point` vertices.
   *  @param indices The indices of the vertices that form this polygon.
   *  @param close If true, closes the polygon.
   *  @return The constructed `Polygon`.
   */
  inline Polygon polygon_from_vertex_indices(const Array<Point> & vertices,
                                             const DynList<size_t> & indices,
                                             const bool close = true)
  {
    Polygon poly;
    for (DynList<size_t>::Iterator it(indices); it.has_curr(); it.next_ne())
      if (const size_t idx = it.get_curr(); idx < vertices.size())
        poly.add_vertex(vertices(idx));

    if (close and poly.size() >= 2)
      poly.close();

    return poly;
  }

  /** @brief Runs a convex hull algorithm and visualizes the result.
   *
   *  This helper function takes a list of points, computes their convex hull
   *  using the provided algorithm object, and inserts both the original points
   *  and the resulting hull polygon into the `Tikz_Plane`.
   *
   *  @tparam HullAlgorithm The type of the convex hull algorithm object. It must
   *          be a callable that takes a `DynList<Point>` and returns a `Polygon`.
   *  @param plane The target TikZ plane.
   *  @param points The input point cloud.
   *  @param hull_algorithm An instance of the hull algorithm to use.
   *  @param point_style The style for the input points.
   *  @param hull_style The style for the convex hull polygon's outline.
   *  @param hull_vertex_style The style for the vertices of the hull.
   *  @param point_layer The layer for the input points.
   *  @param hull_layer The layer for the hull polygon and its vertices.
   *  @param draw_hull_vertices If true, the vertices of the hull are drawn.
   *  @return The computed convex hull `Polygon`.
   */
  template <typename HullAlgorithm>
  Polygon visualize_convex_hull(Tikz_Plane & plane,
                                const DynList<Point> & points,
                                const HullAlgorithm & hull_algorithm,
                                const Tikz_Style & point_style =
                                    tikz_points_style("black", 0.6),
                                const Tikz_Style & hull_style =
                                    tikz_wire_style("red"),
                                const Tikz_Style & hull_vertex_style =
                                    tikz_points_style("red"),
                                const int point_layer = Tikz_Plane::Layer_Default,
                                const int hull_layer = Tikz_Plane::Layer_Foreground,
                                const bool draw_hull_vertices = true)
  {
    put_points(plane, points, point_style, point_layer);

    Polygon hull = hull_algorithm(points);
    if (hull.size() > 0)
      put_in_plane(plane, hull, hull_style, hull_layer);

    if (draw_hull_vertices and hull.size() > 0)
      put_polygon_vertices(plane, hull, hull_vertex_style, hull_layer + 1);

    return hull;
  }

  /** @brief Visualizes the intersection of two convex polygons.
   *
   *  Draws the subject and clip polygons with semi-transparent fills and
   *  overlays their intersection with a distinct, solid fill.
   *
   *  @param plane The target TikZ plane.
   *  @param subject The subject polygon.
   *  @param clip The clipping polygon.
   *  @param intersection_algorithm The algorithm to use for intersection.
   *  @param subject_style The style for the subject polygon.
   *  @param clip_style The style for the clip polygon.
   *  @param result_style The style for the resulting intersection polygon.
   *  @param input_layer The base layer for the input polygons.
   *  @param result_layer The layer for the intersection polygon.
   *  @return The `Polygon` representing the intersection.
   */
  inline Polygon visualize_convex_intersection(
      Tikz_Plane & plane,
      const Polygon & subject,
      const Polygon & clip,
      const ConvexPolygonIntersectionBasic & intersection_algorithm = {},
      const Tikz_Style & subject_style = tikz_area_style("blue", "blue!15", 0.45),
      const Tikz_Style & clip_style = tikz_area_style("orange", "orange!20", 0.45),
      const Tikz_Style & result_style = tikz_area_style("red", "red!30", 0.60),
      const int input_layer = Tikz_Plane::Layer_Default,
      const int result_layer = Tikz_Plane::Layer_Foreground)
  {
    put_in_plane(plane, subject, subject_style, input_layer);
    put_in_plane(plane, clip, clip_style, input_layer + 1);

    Polygon inter = intersection_algorithm(subject, clip);
    if (inter.size() > 0)
      put_in_plane(plane, inter, result_style, result_layer);

    return inter;
  }

  /** @brief Visualizes a boolean operation (union, intersection, difference) on two polygons.
   *
   *  Draws the two input polygons (`a` and `b`) with semi-transparent fills
   *  and overlays the result of the boolean operation with a distinct style.
   *  This function can handle non-convex polygons.
   *
   *  @param plane The target TikZ plane.
   *  @param a The subject polygon.
   *  @param b The clip polygon.
   *  @param op The boolean operation to perform (e.g., `Op::Union`).
   *  @param bop The boolean operations algorithm object.
   *  @param a_style The style for polygon `a`.
   *  @param b_style The style for polygon `b`.
   *  @param result_style The style for the resulting polygon(s).
   *  @param input_layer The base layer for the input polygons.
   *  @param result_layer The layer for the resulting polygon(s).
   *  @return An `Array<Polygon>` containing the result of the operation.
   *          The result can be multiple disjoint polygons.
   */
  inline Array<Polygon> visualize_boolean_operation(
      Tikz_Plane & plane,
      const Polygon & a,
      const Polygon & b,
      const BooleanPolygonOperations::Op op,
      const BooleanPolygonOperations & bop = {},
      const Tikz_Style & a_style = tikz_area_style("blue", "blue!15", 0.35),
      const Tikz_Style & b_style = tikz_area_style("green!60!black", "green!20", 0.35),
      const Tikz_Style & result_style = tikz_area_style("red", "red!35", 0.65),
      const int input_layer = Tikz_Plane::Layer_Default,
      const int result_layer = Tikz_Plane::Layer_Foreground)
  {
    put_in_plane(plane, a, a_style, input_layer);
    put_in_plane(plane, b, b_style, input_layer + 1);

    Array<Polygon> result = bop(a, b, op);
    put_polygons(plane, result, result_style, result_layer);
    return result;
  }

  /** @brief Insert Delaunay triangulation as triangle outlines.
   *
   *  Optionally draws sites as points on top.
   */
  inline void put_delaunay_result(
      Tikz_Plane & plane,
      const DelaunayTriangulationBowyerWatson::Result & dt,
      const Tikz_Style & triangle_style = tikz_wire_style("blue"),
      const bool draw_sites = true,
      const Tikz_Style & site_style = tikz_points_style("black"),
      const int triangle_layer = Tikz_Plane::Layer_Default,
      const int site_layer = Tikz_Plane::Layer_Foreground)
  {
    for (size_t i = 0; i < dt.triangles.size(); ++i)
      {
        const auto & tri = dt.triangles(i);
        Triangle t(dt.sites(tri.i), dt.sites(tri.j), dt.sites(tri.k));
        put_in_plane(plane, t, triangle_style, triangle_layer);
      }

    if (draw_sites)
      put_points(plane, dt.sites, site_style, site_layer);
  }

  /** @brief Compute and insert Delaunay triangulation as triangle outlines. */
  [[nodiscard]] inline DelaunayTriangulationBowyerWatson::Result
  visualize_delaunay(
      Tikz_Plane & plane,
      const DynList<Point> & points,
      const DelaunayTriangulationBowyerWatson & algorithm = {},
      const Tikz_Style & triangle_style = tikz_wire_style("blue"),
      const bool draw_sites = true,
      const Tikz_Style & site_style = tikz_points_style("black"))
  {
    auto dt = algorithm(points);
    put_delaunay_result(plane, dt, triangle_style, draw_sites, site_style);
    return dt;
  }

  [[nodiscard]] inline Point ray_endpoint(const Point & src, const Point & direction,
                                          const Geom_Number & length)
  {
    const Geom_Number & dx = direction.get_x();
    const Geom_Number & dy = direction.get_y();
    const Geom_Number n2 = dx * dx + dy * dy;
    if (n2 == 0)
      return src;

    const Geom_Number n = square_root(n2);
    const Geom_Number scale = length / n;
    return {src.get_x() + dx * scale, src.get_y() + dy * scale};
  }

  /** @brief Insert Voronoi diagram structures (cells, edges, sites).
   *
   *  Works with `VoronoiDiagram::Result` and
   *  `VoronoiDiagramFromDelaunay::Result`.
   */
  template <typename VoronoiResult>
  void put_voronoi_result(
      Tikz_Plane & plane,
      const VoronoiResult & vor,
      const bool draw_cells = false,
      const Tikz_Style & cell_style = tikz_area_style("gray!50!black", "gray!15", 0.35),
      const Tikz_Style & edge_style = tikz_wire_style("black"),
      const Tikz_Style & unbounded_edge_style = tikz_wire_style("black", true, true),
      const Tikz_Style & site_style = tikz_points_style("red"),
      const Geom_Number & unbounded_ray_length = Geom_Number(50),
      const int cell_layer = Tikz_Plane::Layer_Background,
      const int edge_layer = Tikz_Plane::Layer_Default,
      const int site_layer = Tikz_Plane::Layer_Foreground)
  {
    if (draw_cells)
      {
        for (size_t i = 0; i < vor.cells.size(); ++i)
          {
            const auto & cell = vor.cells(i);
            if (not cell.bounded or cell.vertices.size() < 2)
              continue;

            Polygon poly = polygon_from_vertices(cell.vertices, true);
            put_in_plane(plane, poly, cell_style, cell_layer);
          }
      }

    for (size_t i = 0; i < vor.edges.size(); ++i)
      if (const auto & e = vor.edges(i); e.unbounded)
        {
          const Point tgt = ray_endpoint(e.src, e.direction, unbounded_ray_length);
          put_in_plane(plane, Segment(e.src, tgt), unbounded_edge_style, edge_layer);
        }
      else
        put_in_plane(plane, Segment(e.src, e.tgt), edge_style, edge_layer);

    put_points(plane, vor.sites, site_style, site_layer);
  }

  /** @brief Compute and insert Voronoi diagram for input sites.
   *
   *  Returns the generated Voronoi result so callers can reuse it.
   */
  inline VoronoiDiagram::Result visualize_voronoi(
      Tikz_Plane & plane,
      const DynList<Point> & sites,
      const VoronoiDiagram & algorithm = {},
      const bool draw_cells = false,
      const Tikz_Style & cell_style = tikz_area_style("gray!50!black", "gray!15", 0.35),
      const Tikz_Style & edge_style = tikz_wire_style("black"),
      const Tikz_Style & unbounded_edge_style = tikz_wire_style("black", true, true),
      const Tikz_Style & site_style = tikz_points_style("red"),
      const Geom_Number & unbounded_ray_length = Geom_Number(50))
  {
    VoronoiDiagram::Result vor = algorithm(sites);
    put_voronoi_result(plane, vor,
                       draw_cells, cell_style,
                       edge_style, unbounded_edge_style, site_style,
                       unbounded_ray_length,
                       Tikz_Plane::Layer_Background,
                       Tikz_Plane::Layer_Default,
                       Tikz_Plane::Layer_Foreground);
    return vor;
  }

  /** @brief Insert Power diagram structures (cells, edges, weighted sites). */
  inline void put_power_diagram_result(
      Tikz_Plane & plane,
      const PowerDiagram::Result & pd,
      const bool draw_cells = true,
      const Tikz_Style & cell_style = tikz_area_style("violet", "violet!18", 0.35),
      const Tikz_Style & edge_style = tikz_wire_style("violet"),
      const Tikz_Style & site_style = tikz_points_style("purple"),
      const int cell_layer = Tikz_Plane::Layer_Background,
      const int edge_layer = Tikz_Plane::Layer_Default,
      const int site_layer = Tikz_Plane::Layer_Foreground)
  {
    if (draw_cells)
      {
        for (size_t i = 0; i < pd.cells.size(); ++i)
          {
            const auto & cell = pd.cells(i);
            if (cell.vertices.size() < 2)
              continue;

            Polygon poly = polygon_from_vertices(cell.vertices, true);
            put_in_plane(plane, poly, cell_style, cell_layer);
          }
      }

    for (size_t i = 0; i < pd.edges.size(); ++i)
      {
        const auto & e = pd.edges(i);
        put_in_plane(plane, Segment(e.src, e.tgt), edge_style, edge_layer);
      }

    for (size_t i = 0; i < pd.sites.size(); ++i)
      put_in_plane(plane, pd.sites(i).position, site_style, site_layer);
  }

  /** @brief Compute and insert Power diagram for weighted sites. */
  inline PowerDiagram::Result visualize_power_diagram(
      Tikz_Plane & plane,
      const Array<PowerDiagram::WeightedSite> & sites,
      const PowerDiagram & algorithm = {},
      const bool draw_cells = true,
      const Tikz_Style & cell_style = tikz_area_style("violet", "violet!18", 0.35),
      const Tikz_Style & edge_style = tikz_wire_style("violet"),
      const Tikz_Style & site_style = tikz_points_style("purple"))
  {
    PowerDiagram::Result res = algorithm(sites);
    put_power_diagram_result(plane, res, draw_cells,
                             cell_style, edge_style, site_style,
                             Tikz_Plane::Layer_Background,
                             Tikz_Plane::Layer_Default,
                             Tikz_Plane::Layer_Foreground);
    return res;
  }

  /** @brief Insert segment arrangement structures (faces, edges, vertices). */
  inline void put_segment_arrangement_result(
      Tikz_Plane & plane,
      const SegmentArrangement::Result & arrangement,
      const bool draw_faces = true,
      const bool draw_vertices = true,
      const bool draw_unbounded_face = false,
      const Tikz_Style & face_style =
          tikz_area_style("teal!60!black", "teal!12", 0.30),
      const Tikz_Style & edge_style = tikz_wire_style("teal!70!black"),
      const Tikz_Style & vertex_style = tikz_points_style("teal!70!black"),
      const int face_layer = Tikz_Plane::Layer_Background,
      const int edge_layer = Tikz_Plane::Layer_Default,
      const int vertex_layer = Tikz_Plane::Layer_Foreground,
      const bool color_faces_by_index = false)
  {
    if (draw_faces)
      {
        size_t face_color_idx = 0;
        for (size_t i = 0; i < arrangement.faces.size(); ++i)
          {
            const auto & [boundary, unbounded] = arrangement.faces(i);
            if (unbounded and not draw_unbounded_face)
              continue;

            Polygon poly =
                polygon_from_vertex_indices(arrangement.vertices, boundary, true);
            if (poly.size() >= 3)
              {
                Tikz_Style style = face_style;
                if (color_faces_by_index)
                  {
                    style.fill = true;
                    style.fill_color = detail::tikz_palette_color(face_color_idx++);
                    if (style.draw_color.empty())
                      style.draw_color = "black";
                  }

                put_in_plane(plane, poly, style, face_layer);
              }
          }
      }

    for (size_t i = 0; i < arrangement.edges.size(); ++i)
      {
        const auto & e = arrangement.edges(i);
        if (e.src >= arrangement.vertices.size() or
            e.tgt >= arrangement.vertices.size())
          continue;

        put_in_plane(plane,
                     Segment(arrangement.vertices(e.src), arrangement.vertices(e.tgt)),
                     edge_style, edge_layer);
      }

    if (draw_vertices)
      put_points(plane, arrangement.vertices, vertex_style, vertex_layer);
  }

  /** @brief Compute and insert arrangement for input segments. */
  inline SegmentArrangement::Result visualize_segment_arrangement(
      Tikz_Plane & plane,
      const Array<Segment> & segments,
      const SegmentArrangement & algorithm = {},
      const bool draw_faces = true,
      const bool draw_vertices = true,
      const bool draw_unbounded_face = false,
      const Tikz_Style & face_style =
          tikz_area_style("teal!60!black", "teal!12", 0.30),
      const Tikz_Style & edge_style = tikz_wire_style("teal!70!black"),
      const Tikz_Style & vertex_style = tikz_points_style("teal!70!black"),
      const bool color_faces_by_index = false)
  {
    SegmentArrangement::Result res = algorithm(segments);
    put_segment_arrangement_result(plane, res,
                                   draw_faces, draw_vertices, draw_unbounded_face,
                                   face_style, edge_style, vertex_style,
                                   Tikz_Plane::Layer_Background,
                                   Tikz_Plane::Layer_Default,
                                   Tikz_Plane::Layer_Foreground,
                                   color_faces_by_index);
    return res;
  }

  /** @brief Draw KD-tree partitions from a debug snapshot. */
  inline void put_kdtree_partitions_result(
      Tikz_Plane & plane,
      const KDTreePointSearch::DebugSnapshot & snapshot,
      const bool draw_partition_boxes = false,
      const bool draw_points = true,
      const Tikz_Style & partition_style = tikz_wire_style("gray!55", true),
      const Tikz_Style & split_style = tikz_wire_style("blue!70"),
      const Tikz_Style & point_style = tikz_points_style("red"),
      const int partition_layer = Tikz_Plane::Layer_Background,
      const int split_layer = Tikz_Plane::Layer_Default,
      const int point_layer = Tikz_Plane::Layer_Foreground)
  {
    for (size_t i = 0; i < snapshot.partitions.size(); ++i)
      {
        const auto & part = snapshot.partitions(i);
        if (draw_partition_boxes)
          put_in_plane(plane, part.region, partition_style, partition_layer);

        if (part.is_leaf)
          continue;

        const Geom_Number x0 = part.region.get_xmin();
        const Geom_Number x1 = part.region.get_xmax();
        const Geom_Number y0 = part.region.get_ymin();
        const Geom_Number y1 = part.region.get_ymax();
        Segment split_seg = part.split_on_x ?
                              Segment(Point(part.split_value, y0),
                                      Point(part.split_value, y1)) :
                              Segment(Point(x0, part.split_value),
                                      Point(x1, part.split_value));

        Tikz_Style style = split_style;
        style.draw_color = detail::tikz_palette_color(part.depth);
        put_in_plane(plane, split_seg, style, split_layer);
      }

    if (draw_points)
      put_points(plane, snapshot.points, point_style, point_layer);
  }

  /** @brief Visualize KD-tree recursive space partitions. */
  inline KDTreePointSearch::DebugSnapshot visualize_kdtree_partitions(
      Tikz_Plane & plane,
      const KDTreePointSearch & kd_tree,
      const bool draw_partition_boxes = false,
      const bool draw_points = true,
      const Tikz_Style & partition_style = tikz_wire_style("gray!55", true),
      const Tikz_Style & split_style = tikz_wire_style("blue!70"),
      const Tikz_Style & point_style = tikz_points_style("red"))
  {
    const auto snapshot = kd_tree.debug_snapshot();
    put_kdtree_partitions_result(
                                 plane, snapshot, draw_partition_boxes, draw_points,
                                 partition_style, split_style, point_style);
    return snapshot;
  }

  /** @brief Draw range-tree X-axis hierarchy and optional query highlights. */
  inline void put_range_tree_result(
      Tikz_Plane & plane,
      const RangeTree2D::DebugSnapshot & snapshot,
      const bool draw_points = true,
      const Rectangle *query_rect = nullptr,
      const DynList<Point> *query_hits = nullptr,
      const Tikz_Style & split_style = tikz_wire_style("purple"),
      const Tikz_Style & point_style = tikz_points_style("black"),
      const Tikz_Style & query_rect_style = tikz_wire_style("red", true),
      const Tikz_Style & query_hit_style = tikz_points_style("red"),
      const int split_layer = Tikz_Plane::Layer_Default,
      const int point_layer = Tikz_Plane::Layer_Foreground)
  {
    if (snapshot.x_sorted_points.size() == 0)
      return;

    Geom_Number ymin = snapshot.x_sorted_points(0).get_y();
    Geom_Number ymax = ymin;
    for (size_t i = 1; i < snapshot.x_sorted_points.size(); ++i)
      {
        if (snapshot.x_sorted_points(i).get_y() < ymin)
          ymin = snapshot.x_sorted_points(i).get_y();
        if (snapshot.x_sorted_points(i).get_y() > ymax)
          ymax = snapshot.x_sorted_points(i).get_y();
      }

    for (size_t i = 0; i < snapshot.nodes.size(); ++i)
      {
        const auto & node = snapshot.nodes(i);
        if (node.is_leaf)
          continue;

        Tikz_Style style = split_style;
        style.draw_color = detail::tikz_palette_color(node.tree_index);
        put_in_plane(plane,
                     Segment(Point(node.split_x, ymin), Point(node.split_x, ymax)),
                     style, split_layer);
      }

    if (draw_points)
      put_points(plane, snapshot.x_sorted_points, point_style, point_layer);

    if (query_rect != nullptr)
      put_in_plane(plane, *query_rect, query_rect_style, point_layer + 1);

    if (query_hits != nullptr)
      put_points(plane, *query_hits, query_hit_style, point_layer + 2);
  }

  /** @brief Visualize range-tree structure without a query overlay. */
  inline RangeTree2D::DebugSnapshot visualize_range_tree(
      Tikz_Plane & plane,
      const RangeTree2D & tree,
      const bool draw_points = true,
      const Tikz_Style & split_style = tikz_wire_style("purple"),
      const Tikz_Style & point_style = tikz_points_style("black"))
  {
    const auto snapshot = tree.debug_snapshot();
    put_range_tree_result(plane, snapshot, draw_points, nullptr, nullptr,
                          split_style, point_style);
    return snapshot;
  }

  /// @brief Result bundle for range-tree query visualization.
  struct RangeTreeQueryVizResult
  {
    RangeTree2D::DebugSnapshot snapshot;
    Rectangle query_rect;
    DynList<Point> query_hits;
  };

  /** @brief Visualize range-tree plus a query rectangle and matching points. */
  inline RangeTreeQueryVizResult visualize_range_tree_query(
      Tikz_Plane & plane,
      const RangeTree2D & tree,
      const Rectangle & query_rect,
      const bool draw_points = true,
      const Tikz_Style & split_style = tikz_wire_style("purple"),
      const Tikz_Style & point_style = tikz_points_style("black"),
      const Tikz_Style & query_rect_style = tikz_wire_style("red", true),
      const Tikz_Style & query_hit_style = tikz_points_style("red"))
  {
    RangeTreeQueryVizResult out;
    out.snapshot = tree.debug_snapshot();
    out.query_rect = query_rect;
    out.query_hits = tree.query(query_rect.get_xmin(), query_rect.get_xmax(),
                                query_rect.get_ymin(), query_rect.get_ymax());
    put_range_tree_result(plane, out.snapshot, draw_points, &out.query_rect, &out.query_hits,
                          split_style, point_style, query_rect_style, query_hit_style);
    return out;
  }

  /** @brief Draw AABB tree hierarchy and optional query results. */
  inline void put_aabb_tree_result(
      Tikz_Plane & plane,
      const AABBTree::DebugSnapshot & snapshot,
      const Rectangle *query_rect = nullptr,
      const Array<size_t> *query_hit_ids = nullptr,
      const Tikz_Style & node_bbox_style = tikz_wire_style("teal!70!black"),
      const Tikz_Style & leaf_bbox_style = tikz_wire_style("blue!70"),
      const Tikz_Style & query_rect_style = tikz_wire_style("red", true),
      const Tikz_Style & query_hit_style = tikz_wire_style("red"),
      const int node_layer = Tikz_Plane::Layer_Default,
      const int query_layer = Tikz_Plane::Layer_Foreground)
  {
    for (size_t i = 0; i < snapshot.nodes.size(); ++i)
      {
        const auto & node = snapshot.nodes(i);
        Tikz_Style style = node.is_leaf ? leaf_bbox_style : node_bbox_style;
        style.draw_color = detail::tikz_palette_color(node.depth);
        put_in_plane(plane, node.bbox, style, node_layer);
      }

    if (query_rect != nullptr)
      put_in_plane(plane, *query_rect, query_rect_style, query_layer);

    if (query_hit_ids != nullptr)
      for (size_t i = 0; i < snapshot.nodes.size(); ++i)
        if (snapshot.nodes(i).is_leaf)
          for (size_t j = 0; j < query_hit_ids->size(); ++j)
            if (snapshot.nodes(i).user_index == (*query_hit_ids)(j))
              {
                put_in_plane(plane, snapshot.nodes(i).bbox, query_hit_style, query_layer + 1);
                break;
              }
  }

  /** @brief Visualize AABB tree hierarchy. */
  inline AABBTree::DebugSnapshot visualize_aabb_tree(
      Tikz_Plane & plane,
      const AABBTree & tree,
      const Tikz_Style & node_bbox_style = tikz_wire_style("teal!70!black"),
      const Tikz_Style & leaf_bbox_style = tikz_wire_style("blue!70"))
  {
    const auto snapshot = tree.debug_snapshot();
    put_aabb_tree_result(plane, snapshot, nullptr, nullptr,
                         node_bbox_style, leaf_bbox_style);
    return snapshot;
  }

  /// @brief Result bundle for AABB query visualization.
  struct AABBTreeQueryVizResult
  {
    AABBTree::DebugSnapshot snapshot;
    Rectangle query_rect;
    Array<size_t> query_hit_ids;
  };

  /** @brief Visualize AABB tree with a rectangle query overlay. */
  inline AABBTreeQueryVizResult visualize_aabb_tree_query(
      Tikz_Plane & plane,
      const AABBTree & tree,
      const Rectangle & query_rect,
      const Tikz_Style & node_bbox_style = tikz_wire_style("teal!70!black"),
      const Tikz_Style & leaf_bbox_style = tikz_wire_style("blue!70"),
      const Tikz_Style & query_rect_style = tikz_wire_style("red", true),
      const Tikz_Style & query_hit_style = tikz_wire_style("red"))
  {
    AABBTreeQueryVizResult out;
    out.snapshot = tree.debug_snapshot();
    out.query_rect = query_rect;
    out.query_hit_ids = tree.query(query_rect);
    put_aabb_tree_result(
                         plane, out.snapshot, &out.query_rect, &out.query_hit_ids,
                         node_bbox_style, leaf_bbox_style, query_rect_style, query_hit_style);
    return out;
  }

  /** @brief Insert a point path as connected segments and optional waypoints. */
  inline void put_path(
      Tikz_Plane & plane,
      const DynList<Point> & path,
      const Tikz_Style & segment_style = tikz_path_style("red"),
      const bool draw_waypoints = true,
      const Tikz_Style & waypoint_style = tikz_points_style("red"),
      const int segment_layer = Tikz_Plane::Layer_Foreground,
      const int waypoint_layer = Tikz_Plane::Layer_Overlay)
  {
    bool has_prev = false;
    Point prev;

    for (DynList<Point>::Iterator it(path); it.has_curr(); it.next_ne())
      {
        const Point & curr = it.get_curr();
        if (has_prev)
          put_in_plane(plane, Segment(prev, curr), segment_style, segment_layer);

        prev = curr;
        has_prev = true;
      }

    if (draw_waypoints)
      put_points(plane, path, waypoint_style, waypoint_layer);
  }

  /// @brief Insert an `Array<Point>` path as connected segments and waypoints.
  inline void put_path(
      Tikz_Plane & plane,
      const Array<Point> & path,
      const Tikz_Style & segment_style = tikz_path_style("red"),
      const bool draw_waypoints = true,
      const Tikz_Style & waypoint_style = tikz_points_style("red"),
      const int segment_layer = Tikz_Plane::Layer_Foreground,
      const int waypoint_layer = Tikz_Plane::Layer_Overlay)
  {
    if (path.size() == 0)
      return;

    for (size_t i = 1; i < path.size(); ++i)
      put_in_plane(plane, Segment(path(i - 1), path(i)), segment_style, segment_layer);

    if (draw_waypoints)
      put_points(plane, path, waypoint_style, waypoint_layer);
  }

  using FunnelPortal = detail::SPP_Portal;

  /** @brief Insert funnel portals as segment connectors. */
  inline void put_portals(
      Tikz_Plane & plane,
      const Array<FunnelPortal> & portals,
      const bool skip_degenerate = true,
      const Tikz_Style & portal_style = tikz_wire_style("purple", true),
      const int portal_layer = Tikz_Plane::Layer_Default)
  {
    for (size_t i = 0; i < portals.size(); ++i)
      {
        const auto & [left, right] = portals(i);
        if (skip_degenerate and left == right)
          continue;

        put_in_plane(plane, Segment(left, right), portal_style, portal_layer);
      }
  }

  /** @brief Result bundle for shortest-path + funnel portal visualization. */
  struct ShortestPathDebugResult
  {
    DynList<Point> path;
    Array<FunnelPortal> portals;
  };

  /// @brief One SSFA (funnel) iteration snapshot.
  struct FunnelTraceStep
  {
    size_t portal_index = 0;
    Point portal_left;
    Point portal_right;
    Point apex;
    Point left_boundary;
    Point right_boundary;
    Array<Point> committed_path;
    bool tightened_left = false;
    bool tightened_right = false;
    bool emitted_left = false;
    bool emitted_right = false;
  };

  /// @brief Full trace for shortest-path funnel processing.
  struct FunnelTraceResult
  {
    Array<FunnelPortal> portals;
    Array<FunnelTraceStep> steps;
    Array<Point> final_path;
  };

  /** @brief Compute a full SSFA trace (portal-by-portal states).
   *
   *  This is useful for didactic/animation rendering.
   */
  inline FunnelTraceResult compute_shortest_path_funnel_trace(
      const Polygon & polygon,
      const Point & source,
      const Point & target)
  {
    FunnelTraceResult trace;
    trace.portals = detail::shortest_path_portals(polygon, source, target);

    if (trace.portals.size() == 0)
      return trace;

    Point apex = source;
    Point fl = source;
    Point fr = source;
    size_t ai = 0;
    size_t li = 0;
    size_t ri = 0;

    Array<Point> committed;
    committed.append(source);

    for (size_t i = 1; i < trace.portals.size(); ++i)
      {
        const Point & pr = trace.portals(i).right;
        const Point & pl = trace.portals(i).left;

        FunnelTraceStep step;
        step.portal_index = i;
        step.portal_left = pl;
        step.portal_right = pr;
        step.apex = apex;
        step.left_boundary = fl;
        step.right_boundary = fr;
        step.committed_path = committed;

        bool restart = false;

        if (detail::spp_cross(apex, fr, pr) >= 0)
          {
            if (apex == fr or detail::spp_cross(apex, fl, pr) < 0)
              {
                fr = pr;
                ri = i;
                step.tightened_right = true;
              }
            else
              {
                committed.append(fl);
                step.emitted_left = true;

                apex = fl;
                ai = li;
                fl = apex;
                fr = apex;
                li = ai;
                ri = ai;
                i = ai;
                restart = true;
              }
          }

        if (not restart and detail::spp_cross(apex, fl, pl) <= 0)
          {
            if (apex == fl or detail::spp_cross(apex, fr, pl) > 0)
              {
                fl = pl;
                li = i;
                step.tightened_left = true;
              }
            else
              {
                committed.append(fr);
                step.emitted_right = true;

                apex = fr;
                ai = ri;
                fl = apex;
                fr = apex;
                li = ai;
                ri = ai;
                i = ai;
                restart = true;
              }
          }

        step.apex = apex;
        step.left_boundary = fl;
        step.right_boundary = fr;
        step.committed_path = committed;
        trace.steps.append(step);

        if (restart)
          continue; // this is deliberately after appending the step, so the "restart" state is visible in the trace
      }

    committed.append(target);
    trace.final_path = committed;
    return trace;
  }

  /** @brief Render one funnel-trace frame in a plane.
   *
   *  `step_index` is clamped to the last available step.
   */
  inline void put_funnel_trace_step(
      Tikz_Plane & plane,
      const Polygon & polygon,
      const Point & source,
      const Point & target,
      const FunnelTraceResult & trace,
      size_t step_index,
      const Tikz_Style & polygon_style = tikz_area_style("black", "gray!15", 0.22),
      const Tikz_Style & source_style = tikz_points_style("green!50!black"),
      const Tikz_Style & target_style = tikz_points_style("blue"),
      const Tikz_Style & all_portals_style = tikz_wire_style("purple", true),
      const Tikz_Style & active_portal_style = tikz_path_style("purple"),
      const Tikz_Style & funnel_leg_style = tikz_path_style("orange!90!black"),
      const Tikz_Style & committed_style = tikz_path_style("red"),
      const bool draw_waypoints = true,
      const Tikz_Style & waypoint_style = tikz_points_style("red"),
      const int polygon_layer = Tikz_Plane::Layer_Default,
      const int portal_layer = Tikz_Plane::Layer_Foreground,
      const int highlight_layer = Tikz_Plane::Layer_Overlay)
  {
    put_in_plane(plane, polygon, polygon_style, polygon_layer);
    put_in_plane(plane, source, source_style, highlight_layer + 1);
    put_in_plane(plane, target, target_style, highlight_layer + 1);

    put_portals(plane, trace.portals, true, all_portals_style, portal_layer);

    if (trace.steps.size() == 0)
      return;

    if (step_index >= trace.steps.size())
      step_index = trace.steps.size() - 1;

    const FunnelTraceStep & step = trace.steps(step_index);

    // Active portal segment.
    if (step.portal_left != step.portal_right)
      put_in_plane(plane, Segment(step.portal_left, step.portal_right),
                   active_portal_style, highlight_layer);

    // Current funnel legs.
    if (step.apex != step.left_boundary)
      put_in_plane(plane, Segment(step.apex, step.left_boundary),
                   funnel_leg_style, highlight_layer);
    if (step.apex != step.right_boundary)
      put_in_plane(plane, Segment(step.apex, step.right_boundary),
                   funnel_leg_style, highlight_layer);

    // Committed path prefix.
    put_path(plane, step.committed_path,
             committed_style, draw_waypoints, waypoint_style,
             highlight_layer + 1, highlight_layer + 2);

    put_in_plane(plane, step.apex, tikz_points_style("orange!90!black"), highlight_layer + 2);
    put_in_plane(plane, step.left_boundary, tikz_points_style("orange!90!black"), highlight_layer + 2);
    put_in_plane(plane, step.right_boundary, tikz_points_style("orange!90!black"), highlight_layer + 2);
  }

  /** @brief Visualize the shortest path inside a simple polygon. */
  inline DynList<Point> visualize_shortest_path_in_polygon(
      Tikz_Plane & plane,
      const Polygon & polygon,
      const Point & source,
      const Point & target,
      const ShortestPathInPolygon & algorithm = {},
      const Tikz_Style & polygon_style =
          tikz_area_style("black", "gray!15", 0.25),
      const Tikz_Style & source_style = tikz_points_style("green!50!black"),
      const Tikz_Style & target_style = tikz_points_style("blue"),
      const Tikz_Style & path_style = tikz_path_style("red"),
      const bool draw_waypoints = true,
      const Tikz_Style & waypoint_style = tikz_points_style("red"),
      const int polygon_layer = Tikz_Plane::Layer_Default,
      const int path_layer = Tikz_Plane::Layer_Foreground)
  {
    put_in_plane(plane, polygon, polygon_style, polygon_layer);
    put_in_plane(plane, source, source_style, path_layer + 1);
    put_in_plane(plane, target, target_style, path_layer + 1);

    DynList<Point> path = algorithm(polygon, source, target);
    put_path(plane, path,
             path_style, draw_waypoints, waypoint_style,
             path_layer, path_layer + 1);
    return path;
  }

  /** @brief Visualize the shortest path plus funnel portals.
   *
   *  This helper draws:
   *  - polygon, source and target points
   *  - funnel portals (dashed segments)
   *  - final shortest path
   */
  inline ShortestPathDebugResult visualize_shortest_path_with_portals(
      Tikz_Plane & plane,
      const Polygon & polygon,
      const Point & source,
      const Point & target,
      const ShortestPathInPolygon & algorithm = {},
      const Tikz_Style & polygon_style =
          tikz_area_style("black", "gray!15", 0.25),
      const Tikz_Style & source_style = tikz_points_style("green!50!black"),
      const Tikz_Style & target_style = tikz_points_style("blue"),
      const Tikz_Style & portal_style = tikz_wire_style("purple", true),
      const Tikz_Style & path_style = tikz_path_style("red"),
      const bool draw_waypoints = true,
      const Tikz_Style & waypoint_style = tikz_points_style("red"),
      const int polygon_layer = Tikz_Plane::Layer_Default,
      const int portal_layer = Tikz_Plane::Layer_Foreground,
      const int path_layer = Tikz_Plane::Layer_Overlay)
  {
    put_in_plane(plane, polygon, polygon_style, polygon_layer);
    put_in_plane(plane, source, source_style, portal_layer + 1);
    put_in_plane(plane, target, target_style, portal_layer + 1);

    ShortestPathDebugResult out;
    out.portals = detail::shortest_path_portals(polygon, source, target);
    put_portals(plane, out.portals, true, portal_style, portal_layer);

    out.path = algorithm(polygon, source, target);
    put_path(plane, out.path,
             path_style, draw_waypoints, waypoint_style,
             path_layer, path_layer + 1);
    return out;
  }

  /** @brief Insert convex decomposition polygons.
   *
   *  Each convex part can be colored using a repeating palette.
   */
  inline void put_convex_decomposition_result(
      Tikz_Plane & plane,
      const Array<Polygon> & parts,
      const bool color_parts_by_index = true,
      const Tikz_Style & part_style =
          tikz_area_style("blue!60!black", "blue!15", 0.40),
      const int part_layer = Tikz_Plane::Layer_Default)
  {
    size_t color_idx = 0;
    for (size_t i = 0; i < parts.size(); ++i)
      {
        Tikz_Style style = part_style;
        if (color_parts_by_index)
          {
            style.fill = true;
            style.fill_color = detail::tikz_palette_color(color_idx++);
            if (style.draw_color.empty())
              style.draw_color = "black";
          }

        put_in_plane(plane, parts(i), style, part_layer);
      }
  }

  /** @brief Compute and insert convex decomposition for a polygon. */
  inline Array<Polygon> visualize_convex_decomposition(
      Tikz_Plane & plane,
      const Polygon & polygon,
      const ConvexPolygonDecomposition & algorithm = {},
      const bool draw_input_polygon = true,
      const Tikz_Style & input_style = tikz_wire_style("black", true),
      const bool color_parts_by_index = true,
      const Tikz_Style & part_style =
          tikz_area_style("blue!60!black", "blue!15", 0.40),
      const int input_layer = Tikz_Plane::Layer_Default,
      const int part_layer = Tikz_Plane::Layer_Foreground)
  {
    if (draw_input_polygon)
      put_in_plane(plane, polygon, input_style, input_layer);

    Array<Polygon> parts = algorithm(polygon);
    put_convex_decomposition_result(plane, parts,
                                    color_parts_by_index,
                                    part_style,
                                    part_layer);
    return parts;
  }

  /** @brief Insert alpha-shape structures (kept triangles, boundary, sites). */
  inline void put_alpha_shape_result(
      Tikz_Plane & plane,
      const AlphaShape::Result & alpha_shape,
      const bool draw_kept_triangles = false,
      const Tikz_Style & triangle_style = tikz_wire_style("gray!55"),
      const Tikz_Style & boundary_style = tikz_path_style("orange!90!black"),
      const bool draw_sites = true,
      const Tikz_Style & site_style = tikz_points_style("black"),
      const int triangle_layer = Tikz_Plane::Layer_Background,
      const int boundary_layer = Tikz_Plane::Layer_Foreground,
      const int site_layer = Tikz_Plane::Layer_Overlay)
  {
    if (draw_kept_triangles)
      {
        for (size_t n = 0; n < alpha_shape.triangles.size(); ++n)
          {
            const auto & [i, j, k] = alpha_shape.triangles(n);
            if (i >= alpha_shape.sites.size() or
                j >= alpha_shape.sites.size() or
                k >= alpha_shape.sites.size())
              continue;

            put_in_plane(plane,
                         Triangle(alpha_shape.sites(i),
                                  alpha_shape.sites(j),
                                  alpha_shape.sites(k)),
                         triangle_style, triangle_layer);
          }
      }

    for (size_t i = 0; i < alpha_shape.boundary_edges.size(); ++i)
      put_in_plane(plane, alpha_shape.boundary_edges(i), boundary_style, boundary_layer);

    if (draw_sites)
      put_points(plane, alpha_shape.sites, site_style, site_layer);
  }

  /** @brief Compute and insert alpha-shape for input points. */
  inline AlphaShape::Result visualize_alpha_shape(
      Tikz_Plane & plane,
      const DynList<Point> & points,
      const Geom_Number & alpha_squared,
      const AlphaShape & algorithm = {},
      const bool draw_kept_triangles = false,
      const Tikz_Style & triangle_style = tikz_wire_style("gray!55"),
      const Tikz_Style & boundary_style = tikz_path_style("orange!90!black"),
      const bool draw_sites = true,
      const Tikz_Style & site_style = tikz_points_style("black"))
  {
    AlphaShape::Result res = algorithm(points, alpha_squared);
    put_alpha_shape_result(plane, res,
                           draw_kept_triangles,
                           triangle_style,
                           boundary_style,
                           draw_sites,
                           site_style,
                           Tikz_Plane::Layer_Background,
                           Tikz_Plane::Layer_Foreground,
                           Tikz_Plane::Layer_Overlay);
    return res;
  }

  /** @brief Draw regular (weighted Delaunay) triangulation output. */
  inline void put_regular_triangulation_result(
      Tikz_Plane & plane,
      const RegularTriangulationBowyerWatson::Result & rt,
      const Tikz_Style & triangle_style = tikz_wire_style("blue!60"),
      const bool draw_sites = true,
      const Tikz_Style & site_style = tikz_points_style("black"),
      const int triangle_layer = Tikz_Plane::Layer_Default,
      const int site_layer = Tikz_Plane::Layer_Foreground)
  {
    for (size_t i = 0; i < rt.triangles.size(); ++i)
      {
        const auto & tri = rt.triangles(i);
        Triangle t(rt.sites(tri.i).position,
                   rt.sites(tri.j).position,
                   rt.sites(tri.k).position);
        put_in_plane(plane, t, triangle_style, triangle_layer);
      }

    if (draw_sites)
      for (size_t i = 0; i < rt.sites.size(); ++i)
        put_in_plane(plane, rt.sites(i).position, site_style, site_layer);
  }

  /** @brief Compute and insert regular (weighted Delaunay) triangulation. */
  [[nodiscard]] inline RegularTriangulationBowyerWatson::Result
  visualize_regular_triangulation(
      Tikz_Plane & plane,
      const Array<RegularTriangulationBowyerWatson::WeightedSite> & weighted_sites,
      const RegularTriangulationBowyerWatson & algorithm = {},
      const Tikz_Style & triangle_style = tikz_wire_style("blue!60"),
      const bool draw_sites = true,
      const Tikz_Style & site_style = tikz_points_style("black"))
  {
    auto result = algorithm(weighted_sites);
    put_regular_triangulation_result(
                                     plane, result, triangle_style, draw_sites, site_style);
    return result;
  }

  /** @brief Draw closest-pair result over the full point set. */
  inline void put_closest_pair_result(
      Tikz_Plane & plane,
      const DynList<Point> & points,
      const ClosestPairDivideAndConquer::Result & result,
      const Tikz_Style & points_style = tikz_points_style("black"),
      const Tikz_Style & pair_style = tikz_path_style("red"),
      const Tikz_Style & pair_points_style = tikz_points_style("red"),
      const int points_layer = Tikz_Plane::Layer_Default,
      const int pair_layer = Tikz_Plane::Layer_Foreground)
  {
    put_points(plane, points, points_style, points_layer);
    put_in_plane(plane, Segment(result.first, result.second), pair_style, pair_layer);
    put_in_plane(plane, result.first, pair_points_style, pair_layer + 1);
    put_in_plane(plane, result.second, pair_points_style, pair_layer + 1);
  }

  /** @brief Compute and draw the closest pair from an input point set. */
  [[nodiscard]] inline ClosestPairDivideAndConquer::Result
  visualize_closest_pair(
      Tikz_Plane & plane,
      const DynList<Point> & points,
      const ClosestPairDivideAndConquer & algorithm = {},
      const Tikz_Style & points_style = tikz_points_style("black"),
      const Tikz_Style & pair_style = tikz_path_style("red"),
      const Tikz_Style & pair_points_style = tikz_points_style("red"))
  {
    auto result = algorithm(points);
    put_closest_pair_result(
                            plane, points, result,
                            points_style, pair_style, pair_points_style);
    return result;
  }

  /// @brief Bundle of rotating-calipers results used by visualization helpers.
  struct RotatingCalipersResult
  {
    RotatingCalipersConvexPolygon::DiameterResult diameter;
    RotatingCalipersConvexPolygon::WidthResult width;
  };

  /** @brief Draw diameter and minimum-width witnesses for a convex polygon. */
  inline void put_rotating_calipers_result(
      Tikz_Plane & plane,
      const Polygon & polygon,
      const RotatingCalipersResult & result,
      const Tikz_Style & polygon_style = tikz_wire_style("gray!55"),
      const Tikz_Style & diameter_style = tikz_path_style("red"),
      const Tikz_Style & width_style = tikz_path_style("blue"),
      const Tikz_Style & witness_style = tikz_points_style("orange!90!black"),
      const int polygon_layer = Tikz_Plane::Layer_Default,
      const int witness_layer = Tikz_Plane::Layer_Foreground)
  {
    put_in_plane(plane, polygon, polygon_style, polygon_layer);
    put_in_plane(plane, Segment(result.diameter.first, result.diameter.second),
                 diameter_style, witness_layer);
    put_in_plane(plane, Segment(result.width.edge_first, result.width.edge_second),
                 width_style, witness_layer);
    put_in_plane(plane, result.width.antipodal, witness_style, witness_layer + 1);
  }

  /** @brief Compute and draw rotating-calipers diameter and minimum width. */
  [[nodiscard]] inline RotatingCalipersResult
  visualize_rotating_calipers(
      Tikz_Plane & plane,
      const Polygon & polygon,
      const Tikz_Style & polygon_style = tikz_wire_style("gray!55"),
      const Tikz_Style & diameter_style = tikz_path_style("red"),
      const Tikz_Style & width_style = tikz_path_style("blue"),
      const Tikz_Style & witness_style = tikz_points_style("orange!90!black"))
  {
    RotatingCalipersResult result{
          RotatingCalipersConvexPolygon::diameter(polygon),
          RotatingCalipersConvexPolygon::minimum_width(polygon)
        };
    put_rotating_calipers_result(
                                 plane, polygon, result, polygon_style, diameter_style, width_style, witness_style);
    return result;
  }

  /** @brief Draw half-plane boundaries and their bounded intersection polygon. */
  inline void put_half_plane_intersection_result(
      Tikz_Plane & plane,
      const Array<HalfPlaneIntersection::HalfPlane> & halfplanes,
      const Polygon & intersection,
      const Tikz_Style & boundary_style = tikz_wire_style("gray!60", true, true),
      const Tikz_Style & result_style = tikz_area_style("red", "red!25", 0.50),
      const int boundary_layer = Tikz_Plane::Layer_Default,
      const int result_layer = Tikz_Plane::Layer_Foreground)
  {
    for (size_t i = 0; i < halfplanes.size(); ++i)
      put_in_plane(plane,
                   Segment(halfplanes(i).p, halfplanes(i).q),
                   boundary_style, boundary_layer);

    if (intersection.size() > 0)
      put_in_plane(plane, intersection, result_style, result_layer);
  }

  /** @brief Compute and draw bounded half-plane intersection. */
  [[nodiscard]] inline Polygon
  visualize_half_plane_intersection(
      Tikz_Plane & plane,
      const Array<HalfPlaneIntersection::HalfPlane> & halfplanes,
      const HalfPlaneIntersection & algorithm = {},
      const Tikz_Style & boundary_style = tikz_wire_style("gray!60", true, true),
      const Tikz_Style & result_style = tikz_area_style("red", "red!25", 0.50))
  {
    Polygon intersection = algorithm(halfplanes);
    put_half_plane_intersection_result(
                                       plane, halfplanes, intersection, boundary_style, result_style);
    return intersection;
  }

  /** @brief Draw input polygons and their Minkowski sum result. */
  inline void put_minkowski_sum_result(
      Tikz_Plane & plane,
      const Polygon & first,
      const Polygon & second,
      const Polygon & result,
      const Tikz_Style & first_style = tikz_area_style("blue", "blue!14", 0.30),
      const Tikz_Style & second_style = tikz_area_style("green!60!black", "green!16", 0.30),
      const Tikz_Style & result_style = tikz_area_style("red", "red!26", 0.60),
      const int input_layer = Tikz_Plane::Layer_Default,
      const int result_layer = Tikz_Plane::Layer_Foreground)
  {
    put_in_plane(plane, first, first_style, input_layer);
    put_in_plane(plane, second, second_style, input_layer + 1);
    if (result.size() > 0)
      put_in_plane(plane, result, result_style, result_layer);
  }

  /** @brief Compute and draw Minkowski sum of two convex polygons. */
  [[nodiscard]] inline Polygon
  visualize_minkowski_sum(
      Tikz_Plane & plane,
      const Polygon & first,
      const Polygon & second,
      const MinkowskiSumConvex & algorithm = {},
      const Tikz_Style & first_style = tikz_area_style("blue", "blue!14", 0.30),
      const Tikz_Style & second_style = tikz_area_style("green!60!black", "green!16", 0.30),
      const Tikz_Style & result_style = tikz_area_style("red", "red!26", 0.60))
  {
    Polygon result = algorithm(first, second);
    put_minkowski_sum_result(
                             plane, first, second, result, first_style, second_style, result_style);
    return result;
  }

  /** @brief Draw a monotone triangulation result plus polygon boundary. */
  inline void put_monotone_triangulation_result(
      Tikz_Plane & plane,
      const Polygon & polygon,
      const DynList<Triangle> & triangles,
      const Tikz_Style & polygon_style = tikz_wire_style("black"),
      const Tikz_Style & triangle_style = tikz_wire_style("blue!65"),
      const int polygon_layer = Tikz_Plane::Layer_Default,
      const int triangle_layer = Tikz_Plane::Layer_Foreground)
  {
    put_in_plane(plane, polygon, polygon_style, polygon_layer);
    for (DynList<Triangle>::Iterator it(triangles); it.has_curr(); it.next_ne())
      put_in_plane(plane, it.get_curr(), triangle_style, triangle_layer);
  }

  /** @brief Compute and draw triangulation via monotone partition pipeline. */
  [[nodiscard]] inline DynList<Triangle>
  visualize_monotone_triangulation(
      Tikz_Plane & plane,
      const Polygon & polygon,
      const MonotonePolygonTriangulation & algorithm = {},
      const Tikz_Style & polygon_style = tikz_wire_style("black"),
      const Tikz_Style & triangle_style = tikz_wire_style("blue!65"))
  {
    DynList<Triangle> triangles = algorithm(polygon);
    put_monotone_triangulation_result(
                                      plane, polygon, triangles, polygon_style, triangle_style);
    return triangles;
  }

  /** @brief Draw visibility polygon with source polygon and query point. */
  inline void put_visibility_polygon_result(
      Tikz_Plane & plane,
      const Polygon & polygon,
      const Point & query_point,
      const Polygon & visibility_polygon,
      const Tikz_Style & polygon_style = tikz_wire_style("black"),
      const Tikz_Style & visibility_style = tikz_area_style("orange!90!black", "orange!25", 0.50),
      const Tikz_Style & query_style = tikz_points_style("red"),
      const int polygon_layer = Tikz_Plane::Layer_Default,
      const int visibility_layer = Tikz_Plane::Layer_Foreground)
  {
    put_in_plane(plane, polygon, polygon_style, polygon_layer);
    if (visibility_polygon.size() > 0)
      put_in_plane(plane, visibility_polygon, visibility_style, visibility_layer);
    put_in_plane(plane, query_point, query_style, visibility_layer + 1);
  }

  /** @brief Compute and draw visibility polygon from a query point. */
  [[nodiscard]] inline Polygon
  visualize_visibility_polygon(
      Tikz_Plane & plane,
      const Polygon & polygon,
      const Point & query_point,
      const VisibilityPolygon & algorithm = {},
      const Tikz_Style & polygon_style = tikz_wire_style("black"),
      const Tikz_Style & visibility_style = tikz_area_style("orange!90!black", "orange!25", 0.50),
      const Tikz_Style & query_style = tikz_points_style("red"))
  {
    Polygon visibility_polygon = algorithm(polygon, query_point);
    put_visibility_polygon_result(
                                  plane, polygon, query_point, visibility_polygon,
                                  polygon_style, visibility_style, query_style);
    return visibility_polygon;
  }

  /** @brief Draw line segments and all sweep-line intersection points. */
  inline void put_line_sweep_result(
      Tikz_Plane & plane,
      const Array<Segment> & segments,
      const Array<SweepLineSegmentIntersection::Intersection> & intersections,
      const Tikz_Style & segment_style = tikz_wire_style("blue!60"),
      const Tikz_Style & intersection_style = tikz_points_style("red"),
      const int segment_layer = Tikz_Plane::Layer_Default,
      const int intersection_layer = Tikz_Plane::Layer_Foreground)
  {
    for (size_t i = 0; i < segments.size(); ++i)
      put_in_plane(plane, segments(i), segment_style, segment_layer);

    for (size_t i = 0; i < intersections.size(); ++i)
      put_in_plane(plane, intersections(i).point, intersection_style, intersection_layer);
  }

  /** @brief Compute and draw Bentley-Ottmann line-sweep intersections. */
  [[nodiscard]] inline Array<SweepLineSegmentIntersection::Intersection>
  visualize_line_sweep(
      Tikz_Plane & plane,
      const Array<Segment> & segments,
      const SweepLineSegmentIntersection & algorithm = {},
      const Tikz_Style & segment_style = tikz_wire_style("blue!60"),
      const Tikz_Style & intersection_style = tikz_points_style("red"))
  {
    Array<SweepLineSegmentIntersection::Intersection> intersections = algorithm(segments);
    put_line_sweep_result(
                          plane, segments, intersections, segment_style, intersection_style);
    return intersections;
  }

  /** @brief Insert constrained Delaunay triangulation result.
   *
   *  Draws all triangles in wire style, constrained edges in a distinct
   *  foreground style, and optionally sites as points.
   */
  inline void put_cdt_result(
      Tikz_Plane & plane,
      const ConstrainedDelaunayTriangulation::Result & cdt,
      const Tikz_Style & triangle_style = tikz_wire_style("blue!60"),
      const Tikz_Style & constraint_style = tikz_path_style("red"),
      const bool draw_sites = true,
      const Tikz_Style & site_style = tikz_points_style("black"),
      const int triangle_layer = Tikz_Plane::Layer_Default,
      const int constraint_layer = Tikz_Plane::Layer_Foreground,
      const int site_layer = Tikz_Plane::Layer_Overlay)
  {
    for (size_t i = 0; i < cdt.triangles.size(); ++i)
      {
        const auto & tri = cdt.triangles(i);
        Triangle t(cdt.sites(tri.i), cdt.sites(tri.j), cdt.sites(tri.k));
        put_in_plane(plane, t, triangle_style, triangle_layer);
      }

    for (size_t i = 0; i < cdt.constrained_edges.size(); ++i)
      {
        const auto & e = cdt.constrained_edges(i);
        Segment s(cdt.sites(e.u), cdt.sites(e.v));
        put_in_plane(plane, s, constraint_style, constraint_layer);
      }

    if (draw_sites)
      put_points(plane, cdt.sites, site_style, site_layer);
  }

  /** @brief Compute and insert constrained Delaunay triangulation. */
  [[nodiscard]] inline ConstrainedDelaunayTriangulation::Result
  visualize_cdt(
      Tikz_Plane & plane,
      const DynList<Point> & points,
      const DynList<Segment> & constraints,
      const ConstrainedDelaunayTriangulation & algorithm = {},
      const Tikz_Style & triangle_style = tikz_wire_style("blue!60"),
      const Tikz_Style & constraint_style = tikz_path_style("red"),
      const bool draw_sites = true,
      const Tikz_Style & site_style = tikz_points_style("black"))
  {
    auto result = algorithm(points, constraints);
    put_cdt_result(plane, result,
                   triangle_style, constraint_style,
                   draw_sites, site_style);
    return result;
  }

  // ---------- Minimum Enclosing Circle visualization ----------

  /**
   * @brief Draw a precomputed minimum enclosing circle result.
   *
   * Draws the enclosing circle as an `Ellipse` and optionally the input
   * points on top.
   */
  inline void put_mec_result(
      Tikz_Plane & plane,
      const MinimumEnclosingCircle::Circle & circle,
      const DynList<Point> & points,
      const Tikz_Style & circle_style = tikz_wire_style("blue!70"),
      const bool draw_sites = true,
      const Tikz_Style & site_style = tikz_points_style("black"),
      const int circle_layer = Tikz_Plane::Layer_Default,
      const int site_layer = Tikz_Plane::Layer_Overlay)
  {
    const Geom_Number r = circle.radius();
    put_in_plane(plane, Ellipse(circle.center, r, r),
                 circle_style, circle_layer);

    if (draw_sites)
      put_points(plane, points, site_style, site_layer);
  }

  /**
   * @brief Compute minimum enclosing circle and insert into @p plane.
   *
   * @return The computed circle.
   */
  [[nodiscard]] inline MinimumEnclosingCircle::Circle
  visualize_mec(
      Tikz_Plane & plane,
      const DynList<Point> & points,
      const MinimumEnclosingCircle & algorithm = {},
      const Tikz_Style & circle_style = tikz_wire_style("blue!70"),
      const bool draw_sites = true,
      const Tikz_Style & site_style = tikz_points_style("black"))
  {
    auto circle = algorithm(points);
    put_mec_result(plane, circle, points,
                   circle_style, draw_sites, site_style);
    return circle;
  }

  // ==========================================================================
  // Simplification visualization
  // ==========================================================================

  /**
   * @brief Draw original and simplified polygons overlaid.
   *
   * @param plane            Target TikZ canvas.
   * @param original         The original polygon (drawn faded).
   * @param simplified       The simplified polygon (drawn bold).
   * @param original_style   Style for the original polygon.
   * @param simplified_style Style for the simplified polygon.
   * @param draw_vertices    Whether to draw vertices of the simplified polygon.
   * @param vertex_style     Style for simplified polygon vertices.
   */
  inline void put_simplification_result(
      Tikz_Plane & plane,
      const Polygon & original,
      const Polygon & simplified,
      const Tikz_Style & original_style = tikz_wire_style("gray!50"),
      const Tikz_Style & simplified_style = tikz_wire_style("blue!80", 1.2),
      const bool draw_vertices = true,
      const Tikz_Style & vertex_style = tikz_points_style("red"))
  {
    put_in_plane(plane, original, original_style);
    put_in_plane(plane, simplified, simplified_style);
    if (draw_vertices)
      put_polygon_vertices(plane, simplified, vertex_style);
  }

  /**
   * @brief Simplify a polygon with Douglas-Peucker and draw the result.
   *
   * @return The simplified polygon.
   */
  [[nodiscard]] inline Polygon visualize_douglas_peucker(
      Tikz_Plane & plane,
      const Polygon & poly,
      const Geom_Number & epsilon,
      const DouglasPeuckerSimplification & algorithm = {},
      const Tikz_Style & original_style = tikz_wire_style("gray!50"),
      const Tikz_Style & simplified_style = tikz_wire_style("blue!80", 1.2),
      const bool draw_vertices = true,
      const Tikz_Style & vertex_style = tikz_points_style("red"))
  {
    Polygon simplified = algorithm.simplify_polygon(poly, epsilon);
    put_simplification_result(plane, poly, simplified,
                              original_style, simplified_style,
                              draw_vertices, vertex_style);
    return simplified;
  }

  /**
   * @brief Simplify a polygon with Visvalingam-Whyatt and draw the result.
   *
   * @return The simplified polygon.
   */
  [[nodiscard]] inline Polygon visualize_visvalingam_whyatt(
      Tikz_Plane & plane,
      const Polygon & poly,
      const Geom_Number & area_threshold,
      const VisvalingamWhyattSimplification & algorithm = {},
      const Tikz_Style & original_style = tikz_wire_style("gray!50"),
      const Tikz_Style & simplified_style = tikz_wire_style("green!70!black", 1.2),
      const bool draw_vertices = true,
      const Tikz_Style & vertex_style = tikz_points_style("red"))
  {
    Polygon simplified = algorithm.simplify_polygon(poly, area_threshold);
    put_simplification_result(plane, poly, simplified,
                              original_style, simplified_style,
                              draw_vertices, vertex_style);
    return simplified;
  }

  // ==========================================================================
  // Polygon offset visualization
  // ==========================================================================

  /**
   * @brief Draw a polygon offset result overlaid on the original polygon.
   *
   * @param plane           Target TikZ canvas.
   * @param original        The original polygon (drawn faded).
   * @param result          The offset result (one or more polygons).
   * @param original_style  Style for the original polygon.
   * @param offset_style    Style for offset polygon(s).
   * @param draw_vertices   Whether to draw vertices of offset polygon(s).
   * @param vertex_style    Style for offset polygon vertices.
   */
  inline void put_offset_result(
      Tikz_Plane & plane,
      const Polygon & original,
      const PolygonOffset::Result & result,
      const Tikz_Style & original_style = tikz_wire_style("gray!50"),
      const Tikz_Style & offset_style = tikz_wire_style("blue!80", 1.2),
      const bool draw_vertices = true,
      const Tikz_Style & vertex_style = tikz_points_style("red"))
  {
    put_in_plane(plane, original, original_style);
    for (size_t i = 0; i < result.polygons.size(); ++i)
      {
        put_in_plane(plane, result.polygons(i), offset_style);
        if (draw_vertices)
          put_polygon_vertices(plane, result.polygons(i), vertex_style);
      }
  }

  /**
   * @brief Offset a polygon and draw the result.
   *
   * @return The offset result.
   */
  [[nodiscard]] inline PolygonOffset::Result visualize_polygon_offset(
      Tikz_Plane & plane,
      const Polygon & poly,
      const Geom_Number & distance,
      const PolygonOffset & algorithm = {},
      PolygonOffset::JoinType join = PolygonOffset::JoinType::Miter,
      const Geom_Number & miter_limit = Geom_Number(2),
      const Tikz_Style & original_style = tikz_wire_style("gray!50"),
      const Tikz_Style & offset_style = tikz_wire_style("blue!80", 1.2),
      const bool draw_vertices = true,
      const Tikz_Style & vertex_style = tikz_points_style("red"))
  {
    PolygonOffset::Result result = algorithm(poly, distance, join, miter_limit);
    put_offset_result(plane, poly, result,
                      original_style, offset_style,
                      draw_vertices, vertex_style);
    return result;
  }

  // ==========================================================================
  // Chaikin smoothing visualization
  // ==========================================================================

  /**
   * @brief Draw original and smoothed polygons overlaid.
   *
   * @param plane           Target TikZ canvas.
   * @param original        The original polygon (drawn faded).
   * @param smoothed        The smoothed polygon (drawn bold).
   * @param original_style  Style for the original polygon.
   * @param smoothed_style  Style for the smoothed polygon.
   * @param draw_vertices   Whether to draw vertices of the smoothed polygon.
   * @param vertex_style    Style for smoothed polygon vertices.
   */
  inline void put_smoothing_result(
      Tikz_Plane & plane,
      const Polygon & original,
      const Polygon & smoothed,
      const Tikz_Style & original_style = tikz_wire_style("gray!50"),
      const Tikz_Style & smoothed_style = tikz_wire_style("violet!80", 1.2),
      const bool draw_vertices = true,
      const Tikz_Style & vertex_style = tikz_points_style("red"))
  {
    put_in_plane(plane, original, original_style);
    put_in_plane(plane, smoothed, smoothed_style);
    if (draw_vertices)
      put_polygon_vertices(plane, smoothed, vertex_style);
  }

  /**
   * @brief Smooth a polygon with Chaikin subdivision and draw the result.
   *
   * @return The smoothed polygon.
   */
  [[nodiscard]] inline Polygon visualize_chaikin_smoothing(
      Tikz_Plane & plane,
      const Polygon & poly,
      const size_t iterations,
      const Geom_Number & ratio = Geom_Number(1, 4),
      const Tikz_Style & original_style = tikz_wire_style("gray!50"),
      const Tikz_Style & smoothed_style = tikz_wire_style("violet!80", 1.2),
      const bool draw_vertices = true,
      const Tikz_Style & vertex_style = tikz_points_style("red"))
  {
    Polygon smoothed = ChaikinSmoothing::smooth_polygon(poly, iterations, ratio);
    put_smoothing_result(plane, poly, smoothed,
                         original_style, smoothed_style,
                         draw_vertices, vertex_style);
    return smoothed;
  }

  // ==========================================================================
  // Trapezoidal Map Visualization
  // ==========================================================================

  /**
   * @brief Draw the trapezoidal map result into a Tikz_Plane.
   *
   * - Active trapezoids are drawn as filled quadrilaterals (alternating
   *   palette colors).
   * - Vertical extensions from input-segment endpoints are drawn as
   *   dashed gray lines.
   * - Input segments are drawn in bold red.
   * - Optional query points are color-coded by location type.
   */
  inline void put_trapezoidal_map_result(
      Tikz_Plane & plane,
      const TrapezoidalMapPointLocation::Result & res,
      const Tikz_Style & segment_style = tikz_path_style("red"),
      const Tikz_Style & extension_style = tikz_wire_style("gray!60", true),
      const bool draw_trapezoids = true,
      const double trapezoid_opacity = 0.3,
      const int trap_layer = Tikz_Plane::Layer_Background,
      const int seg_layer = Tikz_Plane::Layer_Foreground,
      const int ext_layer = Tikz_Plane::Layer_Default)
  {
    using Trap = TrapezoidalMapPointLocation::Trapezoid;
    constexpr size_t NONE = TrapezoidalMapPointLocation::NONE;

    // Draw filled trapezoids.
    if (draw_trapezoids)
      {
        size_t color_idx = 0;
        for (size_t i = 0; i < res.trapezoids.size(); ++i)
          {
            const Trap & t = res.trapezoids(i);
            if (! t.active) continue;

            // A trapezoid has 4 corners defined by:
            //   top-left: intersection of left vertical with top segment
            //   top-right: intersection of right vertical with top segment
            //   bot-right: intersection of right vertical with bottom segment
            //   bot-left: intersection of left vertical with bottom segment
            // For simplicity, approximate corners using the endpoints
            // and the x-coordinates of the left/right boundary points.
            const Point & lp = res.points(t.leftp);
            const Point & rp = res.points(t.rightp);
            const Segment & top_seg = res.segments(t.top);
            const Segment & bot_seg = res.segments(t.bottom);

            // Compute y-coordinates at left and right x-boundaries.
            auto y_at_x = [](const Segment & seg, const Geom_Number & x)
              -> Geom_Number
              {
                const Geom_Number & x1 = seg.get_src_point().get_x();
                const Geom_Number & x2 = seg.get_tgt_point().get_x();
                const Geom_Number & y1 = seg.get_src_point().get_y();
                const Geom_Number & y2 = seg.get_tgt_point().get_y();
                if (x1 == x2) return (y1 + y2) / 2;
                return y1 + (y2 - y1) * (x - x1) / (x2 - x1);
              };

            const Geom_Number lx = lp.get_x();
            const Geom_Number rx = rp.get_x();
            if (lx == rx)
              {
                ++color_idx;
                continue;
              } // degenerate

            const Point tl_corner(lx, y_at_x(top_seg, lx));
            const Point tr_corner(rx, y_at_x(top_seg, rx));
            const Point br_corner(rx, y_at_x(bot_seg, rx));
            const Point bl_corner(lx, y_at_x(bot_seg, lx));

            // Draw as 4 segments (avoids Polygon self-intersection checks
            // that can fail when corners are collinear).
            Tikz_Style ts;
            ts.draw_color = detail::tikz_palette_color(color_idx);
            ts.fill_color = detail::tikz_palette_color(color_idx);
            ts.opacity = trapezoid_opacity;

            // Build polygon with try/catch — skip if degenerate.
            try
              {
                Polygon quad;
                quad.add_vertex(bl_corner);
                quad.add_vertex(br_corner);
                quad.add_vertex(tr_corner);
                quad.add_vertex(tl_corner);
                quad.close();
                ts.fill = true;
                put_in_plane(plane, quad, ts, trap_layer);
              }
            catch (...)
              {
                // Degenerate quad — draw edges instead.
                put_in_plane(plane, Segment(bl_corner, br_corner),
                             ts, trap_layer);
                put_in_plane(plane, Segment(br_corner, tr_corner),
                             ts, trap_layer);
                put_in_plane(plane, Segment(tr_corner, tl_corner),
                             ts, trap_layer);
                put_in_plane(plane, Segment(tl_corner, bl_corner),
                             ts, trap_layer);
              }
            ++color_idx;
          }
      }

    // Draw vertical extensions from input endpoints.
    for (size_t i = 0; i < res.num_input_points; ++i)
      {
        // Find the topmost and bottommost trapezoid boundaries at this point.
        // For simplicity, draw a short vertical dashed line through each
        // input point spanning the local trapezoid.
        const Point & p = res.points(i);
        // Walk the trapezoids to find the vertical extent at p.
        Geom_Number top_y = p.get_y();
        Geom_Number bot_y = p.get_y();
        for (size_t j = 0; j < res.trapezoids.size(); ++j)
          {
            const Trap & t = res.trapezoids(j);
            if (! t.active) continue;
            if (t.leftp == NONE or t.rightp == NONE) continue;
            const Point & lp = res.points(t.leftp);
            const Point & rp = res.points(t.rightp);
            if (p.get_x() < lp.get_x() or p.get_x() > rp.get_x())
              continue;
            // This trapezoid covers x-coordinate of p.
            auto y_at_x = [](const Segment & seg, const Geom_Number & x)
              -> Geom_Number
              {
                const Geom_Number & x1 = seg.get_src_point().get_x();
                const Geom_Number & x2 = seg.get_tgt_point().get_x();
                const Geom_Number & y1 = seg.get_src_point().get_y();
                const Geom_Number & y2 = seg.get_tgt_point().get_y();
                if (x1 == x2) return (y1 + y2) / 2;
                return y1 + (y2 - y1) * (x - x1) / (x2 - x1);
              };
            const Geom_Number ty = y_at_x(res.segments(t.top), p.get_x());
            const Geom_Number by = y_at_x(res.segments(t.bottom), p.get_x());
            if (ty > top_y) top_y = ty;
            if (by < bot_y) bot_y = by;
          }
        if (top_y > bot_y)
          put_in_plane(plane, Segment(Point(p.get_x(), bot_y),
                                      Point(p.get_x(), top_y)),
                       extension_style, ext_layer);
      }

    // Draw input segments.
    for (size_t i = 0; i < res.num_input_segments; ++i)
      put_in_plane(plane, res.segments(i), segment_style, seg_layer);
  }

  /**
   * @brief Build a trapezoidal map and draw it.
   *
   * Convenience wrapper that constructs the map and draws it in one call.
   */
  [[nodiscard]] inline TrapezoidalMapPointLocation::Result
  visualize_trapezoidal_map(
      Tikz_Plane & plane,
      const Array<Segment> & segments,
      const Tikz_Style & segment_style = tikz_path_style("red"),
      const Tikz_Style & extension_style = tikz_wire_style("gray!60", true),
      const bool draw_trapezoids = true,
      const double trapezoid_opacity = 0.3)
  {
    const TrapezoidalMapPointLocation tm;
    auto result = tm(segments);
    put_trapezoidal_map_result(plane, result, segment_style, extension_style,
                               draw_trapezoids, trapezoid_opacity);
    return result;
  }

  /**
   * @brief Build a trapezoidal map from a polygon and draw it.
   */
  [[nodiscard]] inline TrapezoidalMapPointLocation::Result
  visualize_trapezoidal_map(
      Tikz_Plane & plane,
      const Polygon & polygon,
      const Tikz_Style & segment_style = tikz_path_style("red"),
      const Tikz_Style & extension_style = tikz_wire_style("gray!60", true),
      const bool draw_trapezoids = true,
      const double trapezoid_opacity = 0.3)
  {
    TrapezoidalMapPointLocation tm;
    auto result = tm(polygon);
    put_trapezoidal_map_result(plane, result, segment_style, extension_style,
                               draw_trapezoids, trapezoid_opacity);
    return result;
  }
} // namespace Aleph

# endif // TIKZGEOM_ALGORITHMS_H
