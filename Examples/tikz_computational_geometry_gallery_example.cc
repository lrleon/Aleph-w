// Comprehensive gallery of Aleph-w's computational-geometry module rendered
// through the TikZ backend: primitives, hulls, triangulations, Voronoi/power
// diagrams, boolean/decomposition operations, simplification/offset/
// smoothing, visibility/shortest-path, arrangements, and convex-polygon
// algorithms (Minkowski sum, half-plane intersection, rotating calipers,
// alpha shapes). See Examples/tikz_tree_structures_example.cc for the
// companion gallery of spatial data structures (KD-tree, range tree, AABB
// tree, quadtree, R-tree/R*-tree).

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <tikzgeom_algorithms.H>

using namespace Aleph;

namespace
{

// ---------------------------------------------------------------------------
// Panel 1: primitives & polygons
// ---------------------------------------------------------------------------

Polygon make_irregular_polygon()
{
  Polygon p;
  p.add_vertex(Point(-18, -10));
  p.add_vertex(Point(-4, -14));
  p.add_vertex(Point(10, -8));
  p.add_vertex(Point(6, 4));
  p.add_vertex(Point(-8, 8));
  p.close();
  return p;
}

// ---------------------------------------------------------------------------
// Panel 2: convex hull
// ---------------------------------------------------------------------------

DynList<Point> make_hull_points()
{
  DynList<Point> pts;
  pts.append(Point(-20, -10));
  pts.append(Point(-14, 8));
  pts.append(Point(-6, -16));
  pts.append(Point(0, 2));
  pts.append(Point(4, -12));
  pts.append(Point(10, 14));
  pts.append(Point(16, -4));
  pts.append(Point(18, 10));
  pts.append(Point(-2, 16));
  pts.append(Point(6, 6));
  pts.append(Point(-10, -2));
  pts.append(Point(12, -14));
  return pts;
}

// ---------------------------------------------------------------------------
// Panel 3: Delaunay + Voronoi overlay
// ---------------------------------------------------------------------------

DynList<Point> make_voronoi_sites()
{
  // Chosen so no Delaunay triangle is near-degenerate: a nearly-collinear
  // triple has a legitimately distant circumcenter (Geom_Number is exact
  // rational, so this is correct math, not a precision bug), which makes for
  // a visually confusing demo panel with a Voronoi edge shooting far off
  // frame. All triangles here have comparable area.
  DynList<Point> pts;
  pts.append(Point(-20, -4));
  pts.append(Point(-8, 16));
  pts.append(Point(4, -18));
  pts.append(Point(16, 4));
  pts.append(Point(20, 18));
  pts.append(Point(2, 4));
  pts.append(Point(-14, -14));
  return pts;
}

// ---------------------------------------------------------------------------
// Panel 4: constrained Delaunay triangulation
// ---------------------------------------------------------------------------

DynList<Point> make_cdt_points()
{
  DynList<Point> pts;
  pts.append(Point(-16, -10));
  pts.append(Point(-16, 10));
  pts.append(Point(16, 10));
  pts.append(Point(16, -10));
  pts.append(Point(-4, -2));
  pts.append(Point(4, 4));
  pts.append(Point(0, 0));
  return pts;
}

DynList<Segment> make_cdt_constraints()
{
  DynList<Segment> segs;
  segs.append(Segment(Point(-4, -2), Point(4, 4)));
  return segs;
}

// ---------------------------------------------------------------------------
// Panel 5: minimum enclosing circle + closest pair (shared point set)
// ---------------------------------------------------------------------------

DynList<Point> make_mec_points()
{
  DynList<Point> pts;
  pts.append(Point(-14, -6));
  pts.append(Point(-8, 12));
  pts.append(Point(4, 16));
  pts.append(Point(16, 6));
  pts.append(Point(12, -10));
  pts.append(Point(-2, -14));
  pts.append(Point(0, 2));
  pts.append(Point(1, 3));
  return pts;
}

// ---------------------------------------------------------------------------
// Panel 6: convex decomposition
// ---------------------------------------------------------------------------

Polygon make_decomposition_polygon()
{
  Polygon p;  // a six-pointed star outline (non-convex)
  p.add_vertex(Point(0, 16));
  p.add_vertex(Point(4, 5));
  p.add_vertex(Point(15, 5));
  p.add_vertex(Point(6, -2));
  p.add_vertex(Point(9, -14));
  p.add_vertex(Point(0, -7));
  p.add_vertex(Point(-9, -14));
  p.add_vertex(Point(-6, -2));
  p.add_vertex(Point(-15, 5));
  p.add_vertex(Point(-4, 5));
  p.close();
  return p;
}

// ---------------------------------------------------------------------------
// Panel 7: boolean intersection
// ---------------------------------------------------------------------------

Polygon make_boolean_a()
{
  Polygon p;
  p.add_vertex(Point(-14, -10));
  p.add_vertex(Point(6, -10));
  p.add_vertex(Point(6, 10));
  p.add_vertex(Point(-14, 10));
  p.close();
  return p;
}

Polygon make_boolean_b()
{
  Polygon p;
  p.add_vertex(Point(-4, -4));
  p.add_vertex(Point(16, -4));
  p.add_vertex(Point(16, 16));
  p.add_vertex(Point(-4, 16));
  p.close();
  return p;
}

// ---------------------------------------------------------------------------
// Panel 8: Douglas-Peucker simplification
// ---------------------------------------------------------------------------

Polygon make_noisy_polygon()
{
  Polygon p;
  p.add_vertex(Point(-16, -8));
  p.add_vertex(Point(-12, -9));
  p.add_vertex(Point(-8, -7));
  p.add_vertex(Point(-4, -10));
  p.add_vertex(Point(0, -8));
  p.add_vertex(Point(6, -9));
  p.add_vertex(Point(12, -7));
  p.add_vertex(Point(14, 2));
  p.add_vertex(Point(12, 8));
  p.add_vertex(Point(6, 10));
  p.add_vertex(Point(0, 9));
  p.add_vertex(Point(-6, 10));
  p.add_vertex(Point(-12, 9));
  p.add_vertex(Point(-14, 2));
  p.close();
  return p;
}

// ---------------------------------------------------------------------------
// Panel 9: polygon offset
// ---------------------------------------------------------------------------

Polygon make_offset_polygon()
{
  Polygon p;
  p.add_vertex(Point(-10, -8));
  p.add_vertex(Point(10, -8));
  p.add_vertex(Point(12, 0));
  p.add_vertex(Point(10, 8));
  p.add_vertex(Point(-10, 8));
  p.close();
  return p;
}

// ---------------------------------------------------------------------------
// Panel 10: Chaikin smoothing
// ---------------------------------------------------------------------------

Polygon make_smoothing_polygon()
{
  Polygon p;  // a jagged five-pointed star
  p.add_vertex(Point(0, 14));
  p.add_vertex(Point(4, 3));
  p.add_vertex(Point(13, 3));
  p.add_vertex(Point(6, -4));
  p.add_vertex(Point(8, -13));
  p.add_vertex(Point(0, -7));
  p.add_vertex(Point(-8, -13));
  p.add_vertex(Point(-6, -4));
  p.add_vertex(Point(-13, 3));
  p.add_vertex(Point(-4, 3));
  p.close();
  return p;
}

// ---------------------------------------------------------------------------
// Panel 11: visibility polygon
// ---------------------------------------------------------------------------

Polygon make_visibility_room()
{
  Polygon p;  // an L-shaped room with a wall pier
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(24, 0));
  p.add_vertex(Point(24, 10));
  p.add_vertex(Point(14, 10));
  p.add_vertex(Point(14, 20));
  p.add_vertex(Point(0, 20));
  p.close();
  return p;
}

// ---------------------------------------------------------------------------
// Panel 12: shortest path with funnel portals
// ---------------------------------------------------------------------------

Polygon make_corridor_polygon()
{
  Polygon p;  // a bent corridor with a protruding obstacle
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(26, 0));
  p.add_vertex(Point(26, 22));
  p.add_vertex(Point(16, 22));
  p.add_vertex(Point(16, 10));
  p.add_vertex(Point(11, 10));
  p.add_vertex(Point(11, 22));
  p.add_vertex(Point(0, 22));
  p.close();
  return p;
}

// ---------------------------------------------------------------------------
// Panel 13: trapezoidal map
// ---------------------------------------------------------------------------

Polygon make_trapezoidal_polygon()
{
  Polygon p;
  p.add_vertex(Point(-12, -8));
  p.add_vertex(Point(8, -12));
  p.add_vertex(Point(14, 0));
  p.add_vertex(Point(4, 10));
  p.add_vertex(Point(-10, 6));
  p.close();
  return p;
}

// ---------------------------------------------------------------------------
// Panel 14: segment arrangement
// ---------------------------------------------------------------------------

Array<Segment> make_arrangement_segments()
{
  Array<Segment> segs;
  segs.append(Segment(Point(-16, 0), Point(16, 0)));
  segs.append(Segment(Point(0, -14), Point(0, 14)));
  segs.append(Segment(Point(-14, -10), Point(14, 10)));
  segs.append(Segment(Point(-14, 10), Point(14, -10)));
  segs.append(Segment(Point(-10, -14), Point(10, 14)));
  return segs;
}

// ---------------------------------------------------------------------------
// Panel 15: Bentley-Ottmann line sweep
// ---------------------------------------------------------------------------

Array<Segment> make_sweep_segments()
{
  Array<Segment> segs;
  segs.append(Segment(Point(-16, -6), Point(16, 8)));
  segs.append(Segment(Point(-14, 9), Point(15, -7)));
  segs.append(Segment(Point(-16, 3), Point(16, 3)));
  segs.append(Segment(Point(-6, -12), Point(-6, 12)));
  segs.append(Segment(Point(8, -12), Point(8, 12)));
  return segs;
}

// ---------------------------------------------------------------------------
// Panel 16: Minkowski sum
// ---------------------------------------------------------------------------

Polygon make_minkowski_a()
{
  Polygon p;
  p.add_vertex(Point(-6, -3));
  p.add_vertex(Point(5, -4));
  p.add_vertex(Point(3, 5));
  p.close();
  return p;
}

Polygon make_minkowski_b()
{
  Polygon p;
  p.add_vertex(Point(-3, -2));
  p.add_vertex(Point(4, -2));
  p.add_vertex(Point(0, 5));
  p.close();
  return p;
}

// ---------------------------------------------------------------------------
// Panel 17: half-plane intersection
// ---------------------------------------------------------------------------

Array<HalfPlaneIntersection::HalfPlane> make_half_planes()
{
  using HP = HalfPlaneIntersection::HalfPlane;
  Array<HP> hps;
  hps.append(HP(Point(0, 1), Point(0, 0)));    // x >= 0
  hps.append(HP(Point(0, 0), Point(1, 0)));    // y >= 0
  hps.append(HP(Point(10, 0), Point(10, 1)));  // x <= 10
  hps.append(HP(Point(1, 10), Point(0, 10)));  // y <= 10
  hps.append(HP(Point(10, 0), Point(0, 10)));  // x + y <= 10
  return hps;
}

// ---------------------------------------------------------------------------
// Panel 18: rotating calipers
// ---------------------------------------------------------------------------

Polygon make_calipers_polygon()
{
  Polygon p;
  p.add_vertex(Point(-14, -4));
  p.add_vertex(Point(-4, -12));
  p.add_vertex(Point(10, -8));
  p.add_vertex(Point(16, 4));
  p.add_vertex(Point(6, 14));
  p.add_vertex(Point(-10, 10));
  p.close();
  return p;
}

// ---------------------------------------------------------------------------
// Panel 19: alpha shape
// ---------------------------------------------------------------------------

DynList<Point> make_alpha_points()
{
  DynList<Point> pts;
  pts.append(Point(-16, -8));
  pts.append(Point(-12, 10));
  pts.append(Point(-2, 15));
  pts.append(Point(8, 13));
  pts.append(Point(15, 6));
  pts.append(Point(17, -6));
  pts.append(Point(8, -14));
  pts.append(Point(-4, -16));
  pts.append(Point(2, 2));
  pts.append(Point(-5, 3));
  return pts;
}

// ---------------------------------------------------------------------------
// Panel 20: power diagram (weighted Voronoi)
// ---------------------------------------------------------------------------

Array<PowerDiagram::WeightedSite> make_power_sites()
{
  // A ring of boundary sites plus two interior sites: only sites strictly
  // inside the convex hull of the input can have a *bounded* power cell (a
  // hull site's cell is always unbounded, exactly as in a plain Voronoi
  // diagram), so the two interior sites are what make the filled cells
  // visible here.
  Array<PowerDiagram::WeightedSite> sites;
  sites.append({Point(-16, -10), Geom_Number(2)});
  sites.append({Point(16, -10), Geom_Number(3)});
  sites.append({Point(20, 8), Geom_Number(2)});
  sites.append({Point(0, 18), Geom_Number(4)});
  sites.append({Point(-20, 8), Geom_Number(3)});
  sites.append({Point(-6, 3), Geom_Number(5)});
  sites.append({Point(6, -2), Geom_Number(2)});
  return sites;
}

void add_caption(Tikz_Plane & plane, const Point & at, const std::string & text)
{
  put_in_plane(plane, Text(at, text), make_tikz_draw_style("black"),
              Tikz_Plane::Layer_Overlay);
}

} // namespace

int main(int argc, char * argv[])
{
  const std::string output_path =
      argc > 1 ? argv[1] : "tikz_computational_geometry_gallery_example.tex";

  std::ofstream out(output_path);
  if (not out)
    {
      std::cerr << "Cannot open output file: " << output_path << '\n';
      return 1;
    }

  std::vector<Tikz_Plane> panels;
  panels.reserve(20);
  auto & p = panels;  // shorthand

  // 1. Primitives & polygons showcase.
  p.emplace_back(190, 120, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    put_in_plane(plane, Segment(Point(-24, -12), Point(-14, 4)), tikz_wire_style("blue"));
    put_in_plane(plane, Triangle(Point(-8, -14), Point(2, -14), Point(-3, -4)),
                tikz_area_style("teal", "teal!18", 0.4));
    put_in_plane(plane, Ellipse(Point(12, -8), Geom_Number(6), Geom_Number(3)),
                tikz_wire_style("purple"));
    put_in_plane(plane, RotatedEllipse(Point(12, 6), Geom_Number(7), Geom_Number(3),
                                       Geom_Number(3, 5), Geom_Number(4, 5)),
                tikz_wire_style("magenta"));
    put_in_plane(plane, Regular_Polygon(Point(-14, 12), 5.0, 6),
                tikz_area_style("orange!80!black", "orange!20", 0.4));
    put_in_plane(plane, make_irregular_polygon(), tikz_wire_style("black"));
    put_in_plane(plane, Point(0, 0), tikz_points_style("red"));
    add_caption(plane, Point(-24, 20), "Primitives: segment, triangle, ellipse, "
                "rotated ellipse, regular hexagon, irregular polygon, point");
  }

  // 2. Convex hull (Andrew monotone chain).
  p.emplace_back(180, 110, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    plane.set_point_radius_mm(0.7);
    const Polygon hull = visualize_convex_hull(
        plane, make_hull_points(), AndrewMonotonicChainConvexHull());
    add_caption(plane, Point(-22, 20),
               "Convex Hull (Andrew): h=" + std::to_string(hull.size()));
  }

  // 3. Delaunay triangulation + Voronoi overlay.
  p.emplace_back(190, 115, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    plane.set_point_radius_mm(0.75);
    const DynList<Point> sites = make_voronoi_sites();
    DelaunayTriangulationRandomizedIncremental dt_algo;
    const auto dt = dt_algo(sites);
    put_delaunay_result(plane, dt, tikz_wire_style("blue!70"), false);
    visualize_voronoi(plane, sites, VoronoiDiagram(), false,
                      tikz_area_style("gray!50!black", "gray!15", 0.25),
                      tikz_wire_style("black"),
                      tikz_wire_style("black", true, true),
                      tikz_points_style("red"), Geom_Number(60));
    add_caption(plane, Point(-25, 24),
               "Delaunay + Voronoi: sites=" + std::to_string(dt.sites.size()) +
               ", triangles=" + std::to_string(dt.triangles.size()));
  }

  // 4. Constrained Delaunay triangulation.
  p.emplace_back(180, 105, 5, 5);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    const auto cdt = visualize_cdt(plane, make_cdt_points(), make_cdt_constraints());
    add_caption(plane, Point(-18, 13),
               "CDT: triangles=" + std::to_string(cdt.triangles.size()) +
               ", constraints=" + std::to_string(cdt.constrained_edges.size()));
  }

  // 5. Minimum enclosing circle + closest pair (same point set).
  p.emplace_back(180, 110, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    const DynList<Point> pts = make_mec_points();
    const auto circle = visualize_mec(plane, pts);
    const auto [first, second, distance_squared] = visualize_closest_pair(plane, pts);
    add_caption(plane, Point(-18, 20),
               "MEC r=" + std::to_string(geom_number_to_double(circle.radius())) +
               " + closest pair d^2=" +
               std::to_string(geom_number_to_double(distance_squared)));
  }

  // 6. Convex decomposition (six-pointed star).
  p.emplace_back(170, 115, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    const Array<Polygon> parts =
        visualize_convex_decomposition(plane, make_decomposition_polygon());
    add_caption(plane, Point(-16, 19),
               "Convex Decomposition: parts=" + std::to_string(parts.size()));
  }

  // 7. Boolean intersection of two overlapping rectangles.
  p.emplace_back(170, 110, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    const auto result = visualize_boolean_operation(
        plane, make_boolean_a(), make_boolean_b(),
        BooleanPolygonOperations::Op::INTERSECTION);
    add_caption(plane, Point(-16, 19),
               "Boolean Intersection: parts=" + std::to_string(result.size()));
  }

  // 8. Douglas-Peucker simplification.
  p.emplace_back(180, 110, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    const Polygon simplified =
        visualize_douglas_peucker(plane, make_noisy_polygon(), Geom_Number(1, 2));
    add_caption(plane, Point(-18, 14),
               "Douglas-Peucker: original=" +
               std::to_string(make_noisy_polygon().size()) +
               ", simplified=" + std::to_string(simplified.size()));
  }

  // 9. Polygon offset (inward).
  p.emplace_back(160, 100, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    const auto result = visualize_polygon_offset(plane, make_offset_polygon(), Geom_Number(-3));
    add_caption(plane, Point(-13, 12),
               "Polygon Offset (-3): polygons=" + std::to_string(result.polygons.size()));
  }

  // 10. Chaikin smoothing.
  p.emplace_back(160, 105, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    const Polygon smoothed =
        visualize_chaikin_smoothing(plane, make_smoothing_polygon(), 3);
    add_caption(plane, Point(-14, 16),
               "Chaikin Smoothing (3 iters): vertices=" + std::to_string(smoothed.size()));
  }

  // 11. Visibility polygon.
  p.emplace_back(170, 110, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    const Point query(3, 3);
    const Polygon vis = visualize_visibility_polygon(plane, make_visibility_room(), query);
    add_caption(plane, Point(-1, 24),
               "Visibility Polygon: vertices=" + std::to_string(vis.size()));
  }

  // 12. Shortest path with funnel portals.
  p.emplace_back(190, 115, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    const Point source(3, 3);
    const Point target(20, 18);
    const auto debug = visualize_shortest_path_with_portals(
        plane, make_corridor_polygon(), source, target, ShortestPathInPolygon());
    size_t path_nodes = 0;
    for (DynList<Point>::Iterator it(debug.path); it.has_curr(); it.next_ne())
      ++path_nodes;
    add_caption(plane, Point(-1, 26),
               "Shortest Path + Portals: path nodes=" + std::to_string(path_nodes) +
               ", portals=" + std::to_string(debug.portals.size()));
  }

  // 13. Trapezoidal map point location.
  p.emplace_back(170, 110, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    const auto res = visualize_trapezoidal_map(plane, make_trapezoidal_polygon());
    add_caption(plane, Point(-15, 14),
               "Trapezoidal Map: input segments=" + std::to_string(res.num_input_segments));
  }

  // 14. Segment arrangement.
  p.emplace_back(170, 110, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    const auto arrangement = visualize_segment_arrangement(
        plane, make_arrangement_segments(), SegmentArrangement(),
        true, true, false,
        tikz_area_style("teal!60!black", "teal!12", 0.35),
        tikz_wire_style("teal!70!black"),
        tikz_points_style("teal!80!black"), true);
    add_caption(plane, Point(-18, 18),
               "Segment Arrangement: V=" + std::to_string(arrangement.vertices.size()) +
               ", E=" + std::to_string(arrangement.edges.size()));
  }

  // 15. Bentley-Ottmann line sweep.
  p.emplace_back(170, 110, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    plane.set_point_radius_mm(0.7);
    const auto intersections = visualize_line_sweep(plane, make_sweep_segments());
    add_caption(plane, Point(-18, 15),
               "Line Sweep intersections=" + std::to_string(intersections.size()));
  }

  // 16. Minkowski sum.
  p.emplace_back(160, 100, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    const Polygon sum = visualize_minkowski_sum(plane, make_minkowski_a(), make_minkowski_b());
    add_caption(plane, Point(-9, 10), "Minkowski Sum: vertices=" + std::to_string(sum.size()));
  }

  // 17. Half-plane intersection.
  p.emplace_back(150, 100, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    const Polygon feasible = visualize_half_plane_intersection(plane, make_half_planes());
    add_caption(plane, Point(-1, 12),
               "Half-Plane Intersection: vertices=" + std::to_string(feasible.size()));
  }

  // 18. Rotating calipers (diameter + minimum width).
  p.emplace_back(170, 110, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    const auto rc = visualize_rotating_calipers(plane, make_calipers_polygon());
    add_caption(plane, Point(-16, 18),
               "Rotating Calipers: diameter^2=" +
               std::to_string(geom_number_to_double(rc.diameter.distance_squared)));
  }

  // 19. Alpha shape.
  p.emplace_back(180, 115, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    plane.set_point_radius_mm(0.75);
    const auto alpha = visualize_alpha_shape(plane, make_alpha_points(), Geom_Number(180));
    add_caption(plane, Point(-19, 20),
               "Alpha Shape: boundary edges=" + std::to_string(alpha.boundary_edges.size()));
  }

  // 20. Power diagram (weighted Voronoi).
  p.emplace_back(200, 115, 6, 6);
  {
    Tikz_Plane & plane = p.back();
    plane.put_cartesian_axis();
    plane.set_point_radius_mm(0.75);
    // draw_cells=false: PowerDiagram's bounded-cell vertex lists are not
    // reliably in simple cyclic order around their site, so building a
    // filled cell polygon can throw "closing causes an intersection"
    // (reproduced with this exact site data; see the geometry review notes).
    // Only edges/sites are drawn until that ordering bug is fixed upstream.
    visualize_power_diagram(plane, make_power_sites(), PowerDiagram(), false,
                            tikz_area_style("violet", "violet!18", 0.35),
                            tikz_wire_style("violet"), tikz_points_style("purple"));
    add_caption(plane, Point(-26, 23), "Power Diagram (Weighted Voronoi)");
  }

  out << "\\documentclass[tikz,border=8pt]{standalone}\n"
      << "\\usepackage{tikz}\n"
      << "\\begin{document}\n\n";

  for (size_t i = 0; i < panels.size(); ++i)
    {
      panels[i].draw(out, true);
      if (i + 1 < panels.size())
        out << "\n\\vspace{4mm}\n\n";
    }

  out << "\n\\end{document}\n";

  out.close();
  if (not out)
    {
      std::cerr << "Failed writing output file: " << output_path << '\n';
      return 1;
    }

  std::cout << "Generated " << output_path << " (" << panels.size() << " panels)\n";
  std::cout << "Compile with: pdflatex " << output_path << '\n';
  return 0;
}