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

  /// @brief Style optimized for point clouds.
  inline Tikz_Style tikz_points_style(const std::string & color = "black",
                                      const double opacity = -1.0)
  {
    Tikz_Style s;
    s.draw_color = color;
    s.fill_color = color;
    s.opacity = opacity;
    return s;
  }

  /// @brief Style optimized for wireframe segments/polygons.
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

  /// @brief Style optimized for polyline paths (shortest-path overlays, etc).
  inline Tikz_Style tikz_path_style(const std::string & color = "red",
                                    const bool with_arrow = false)
  {
    Tikz_Style s = tikz_wire_style(color, false, with_arrow);
    s.thick = true;
    return s;
  }

  /// @brief Style optimized for filled polygons.
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

  /// @brief Insert all points from an `Array<Point>`.
  inline void put_points(Tikz_Plane & plane, const Array<Point> & pts,
                         const Tikz_Style & style = tikz_points_style(),
                         const int layer = Tikz_Plane::Layer_Default)
  {
    for (size_t i = 0; i < pts.size(); ++i)
      put_in_plane(plane, pts(i), style, layer);
  }

  /// @brief Insert all points from a `DynList<Point>`.
  inline void put_points(Tikz_Plane & plane, const DynList<Point> & pts,
                         const Tikz_Style & style = tikz_points_style(),
                         const int layer = Tikz_Plane::Layer_Default)
  {
    for (DynList<Point>::Iterator it(pts); it.has_curr(); it.next_ne())
      put_in_plane(plane, it.get_curr(), style, layer);
  }

  /// @brief Insert all polygons from an `Array<Polygon>`.
  inline void put_polygons(Tikz_Plane & plane, const Array<Polygon> & polys,
                           const Tikz_Style & style = tikz_wire_style(),
                           const int layer = Tikz_Plane::Layer_Default)
  {
    for (size_t i = 0; i < polys.size(); ++i)
      put_in_plane(plane, polys(i), style, layer);
  }

  /// @brief Insert all vertices of a polygon as points.
  inline void put_polygon_vertices(Tikz_Plane & plane, const Polygon & poly,
                                   const Tikz_Style & style = tikz_points_style(),
                                   const int layer = Tikz_Plane::Layer_Overlay)
  {
    for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
      put_in_plane(plane, it.get_current_vertex().to_point(), style, layer);
  }

  /// @brief Insert all vertices of a regular polygon as points.
  inline void put_polygon_vertices(Tikz_Plane & plane, const Regular_Polygon & poly,
                                   const Tikz_Style & style = tikz_points_style(),
                                   const int layer = Tikz_Plane::Layer_Overlay)
  {
    for (size_t i = 0; i < poly.size(); ++i)
      put_in_plane(plane, poly.get_vertex(i), style, layer);
  }

  /// @brief Build a polygon from ordered vertices.
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

  /// @brief Build a polygon from indexed vertices.
  inline Polygon polygon_from_vertex_indices(const Array<Point> & vertices,
                                             const DynList<size_t> & indices,
                                             const bool close = true)
  {
    Polygon poly;
    for (DynList<size_t>::Iterator it(indices); it.has_curr(); it.next_ne())
      {
        const size_t idx = it.get_curr();
        if (idx < vertices.size())
          poly.add_vertex(vertices(idx));
      }

    if (close and poly.size() >= 2)
      poly.close();

    return poly;
  }

  /** @brief Visualize a convex hull computation.
   *
   *  Draws the input point cloud and the resulting hull polygon.
   *  The hull is computed using the supplied algorithm object.
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

  /** @brief Visualize convex polygon intersection.
   *
   *  Draws subject/clip polygons and their intersection.
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

  /** @brief Visualize boolean operation on polygons.
   *
   *  Returns the resulting polygons and inserts them into the plane.
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
      {
        const auto & e = vor.edges(i);

        if (e.unbounded)
          {
            const Point tgt = ray_endpoint(e.src, e.direction, unbounded_ray_length);
            put_in_plane(plane, Segment(e.src, tgt), unbounded_edge_style, edge_layer);
          }
        else
          put_in_plane(plane, Segment(e.src, e.tgt), edge_style, edge_layer);
      }

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
    put_voronoi_result(
                       plane, vor,
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
            const auto & face = arrangement.faces(i);
            if (face.unbounded and not draw_unbounded_face)
              continue;

            Polygon poly = polygon_from_vertex_indices(
                                                       arrangement.vertices, face.boundary, true);
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
          continue;
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
} // namespace Aleph

# endif // TIKZGEOM_ALGORITHMS_H
