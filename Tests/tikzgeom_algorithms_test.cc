#include <gtest/gtest.h>

#include <cctype>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include <tikzgeom_algorithms.H>
#include <tikzgeom_scene.H>
#include <tpl_r_star_tree.H>

using namespace Aleph;

namespace
{

bool has_nan_or_inf(std::string s)
{
  for (char & c : s)
    c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));

  return s.find("nan") != std::string::npos or
         s.find("inf") != std::string::npos;
}

#ifdef ALEPH_PDFLATEX_EXECUTABLE

/** @brief Quote one controlled path for a POSIX shell command. */
std::string shell_quote(const std::string & value)
{
  std::string quoted = "'";
  for (const char c : value)
    if (c == '\'')
      quoted += "'\\''";
    else
      quoted += c;
  return quoted + "'";
}

/** @brief Own a unique temporary directory for one LaTeX smoke test. */
class Scoped_Latex_Temp_Directory
{
  std::filesystem::path path_;

public:
  Scoped_Latex_Temp_Directory()
  {
    const auto stamp = std::chrono::high_resolution_clock::now()
        .time_since_epoch().count();
    path_ = std::filesystem::temp_directory_path() /
            ("aleph-tikz-latex-" + std::to_string(stamp));
    std::filesystem::create_directories(path_);
  }

  Scoped_Latex_Temp_Directory(const Scoped_Latex_Temp_Directory &) = delete;
  Scoped_Latex_Temp_Directory & operator=(
      const Scoped_Latex_Temp_Directory &) = delete;

  ~Scoped_Latex_Temp_Directory()
  {
    std::error_code error;
    std::filesystem::remove_all(path_, error);
  }

  /** @brief Return the owned temporary directory path. */
  [[nodiscard]] const std::filesystem::path & path() const noexcept
  {
    return path_;
  }
};

/** @brief Write an entire generated LaTeX document to disk. */
bool write_text_file(const std::filesystem::path & path,
                     const std::string & text)
{
  std::ofstream output(path);
  output << text;
  return output.good();
}

/** @brief Read a complete diagnostic file, or an empty string if absent. */
std::string read_text_file(const std::filesystem::path & path)
{
  std::ifstream input(path);
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}

#endif

Polygon make_convex_a()
{
  Polygon p;
  p.add_vertex(Point(-20, -10));
  p.add_vertex(Point(10, -12));
  p.add_vertex(Point(18, 4));
  p.add_vertex(Point(-8, 18));
  p.close();
  return p;
}

Polygon make_convex_b()
{
  Polygon p;
  p.add_vertex(Point(-14, -18));
  p.add_vertex(Point(22, -2));
  p.add_vertex(Point(8, 22));
  p.add_vertex(Point(-18, 8));
  p.close();
  return p;
}

Polygon make_shortest_path_polygon()
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(20, 0));
  p.add_vertex(Point(20, 20));
  p.add_vertex(Point(12, 20));
  p.add_vertex(Point(12, 8));
  p.add_vertex(Point(8, 8));
  p.add_vertex(Point(8, 20));
  p.add_vertex(Point(0, 20));
  p.close();
  return p;
}

Polygon make_concave_for_decomposition()
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(12, 0));
  p.add_vertex(Point(12, 10));
  p.add_vertex(Point(7, 10));
  p.add_vertex(Point(7, 4));
  p.add_vertex(Point(4, 4));
  p.add_vertex(Point(4, 10));
  p.add_vertex(Point(0, 10));
  p.close();
  return p;
}

Polygon make_convex_for_calipers()
{
  Polygon p;
  p.add_vertex(Point(-10, -4));
  p.add_vertex(Point(10, -6));
  p.add_vertex(Point(16, 2));
  p.add_vertex(Point(7, 12));
  p.add_vertex(Point(-8, 10));
  p.close();
  return p;
}

Polygon make_visibility_polygon()
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(20, 0));
  p.add_vertex(Point(20, 18));
  p.add_vertex(Point(11, 18));
  p.add_vertex(Point(11, 8));
  p.add_vertex(Point(7, 8));
  p.add_vertex(Point(7, 18));
  p.add_vertex(Point(0, 18));
  p.close();
  return p;
}

size_t count_points(const DynList<Point> & pts)
{
  size_t n = 0;
  for (DynList<Point>::Iterator it(pts); it.has_curr(); it.next_ne())
    ++n;
  return n;
}

size_t count_points(const Array<Point> & pts)
{
  return pts.size();
}

} // namespace

TEST(TikzGeomAlgorithmsTest, LayerOrderIsRespected)
{
  Tikz_Plane plane(100, 60);

  Tikz_Style back = tikz_wire_style("green");
  Tikz_Style front = tikz_wire_style("red");

  put_in_plane(plane, Segment(Point(0, 0), Point(50, 50)), front, 10);
  put_in_plane(plane, Segment(Point(0, 50), Point(50, 0)), back, -10);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();

  const size_t green_pos = result.find("draw=green");
  const size_t red_pos = result.find("draw=red");

  ASSERT_NE(green_pos, std::string::npos);
  ASSERT_NE(red_pos, std::string::npos);
  EXPECT_LT(green_pos, red_pos);
}

TEST(TikzGeomAlgorithmsTest, VisualizeConvexHull)
{
  DynList<Point> pts;
  pts.append(Point(-15, -8));
  pts.append(Point(-8, 10));
  pts.append(Point(0, -10));
  pts.append(Point(8, 8));
  pts.append(Point(14, -3));
  pts.append(Point(2, 16));

  Tikz_Plane plane(120, 80);
  AndrewMonotonicChainConvexHull hull_algo;

  Polygon hull = visualize_convex_hull(
      plane, pts, hull_algo,
      tikz_points_style("black"),
      tikz_wire_style("red"),
      tikz_points_style("red"));

  EXPECT_GE(hull.size(), 3U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();

  EXPECT_NE(result.find("draw=red"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeConvexIntersection)
{
  const Polygon a = make_convex_a();
  const Polygon b = make_convex_b();

  Tikz_Plane plane(120, 80);
  Polygon inter = visualize_convex_intersection(plane, a, b);

  EXPECT_GT(inter.size(), 0U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();

  EXPECT_NE(result.find("fill=red!30"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeVoronoi)
{
  DynList<Point> sites;
  sites.append(Point(-18, -8));
  sites.append(Point(-6, 14));
  sites.append(Point(8, -10));
  sites.append(Point(16, 9));
  sites.append(Point(2, 20));

  Tikz_Plane plane(150, 100);
  const auto vor = visualize_voronoi(plane, sites, VoronoiDiagram(), true);

  EXPECT_EQ(vor.sites.size(), 5U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();

  EXPECT_NE(result.find("dashed"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeDelaunay)
{
  DynList<Point> points;
  points.append(Point(-12, -8));
  points.append(Point(-8, 9));
  points.append(Point(0, 14));
  points.append(Point(11, 9));
  points.append(Point(14, -7));
  points.append(Point(2, -13));
  points.append(Point(0, 0));

  Tikz_Plane plane(160, 110);
  const auto dt = visualize_delaunay(plane, points);

  EXPECT_GE(dt.sites.size(), 5U);
  EXPECT_GT(dt.triangles.size(), 0U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();
  EXPECT_NE(result.find("draw=blue"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizePowerDiagram)
{
  Array<PowerDiagram::WeightedSite> sites;
  sites.append({Point(-16, -6), Geom_Number(1)});
  sites.append({Point(-2, 12), Geom_Number(4)});
  sites.append({Point(12, -8), Geom_Number(2)});
  sites.append({Point(18, 10), Geom_Number(6)});

  Tikz_Plane plane(150, 100);
  const auto pd = visualize_power_diagram(plane, sites);

  EXPECT_EQ(pd.sites.size(), 4U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();

  EXPECT_NE(result.find("violet"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeSegmentArrangement)
{
  Array<Segment> segments;
  segments.append(Segment(Point(-20, 0), Point(20, 0)));
  segments.append(Segment(Point(0, -20), Point(0, 20)));
  segments.append(Segment(Point(-20, -20), Point(20, 20)));

  Tikz_Plane plane(150, 100);
  const auto arrangement = visualize_segment_arrangement(
      plane, segments, SegmentArrangement(),
      true, true, false,
      tikz_area_style("teal!60!black", "teal!12", 0.30),
      tikz_wire_style("teal!70!black"),
      tikz_points_style("teal!70!black"),
      true);

  EXPECT_GT(arrangement.vertices.size(), 4U);
  EXPECT_GT(arrangement.edges.size(), 3U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();

  EXPECT_NE(result.find("teal!70!black"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeSegmentArrangementFaceColoring)
{
  Array<Segment> segments;
  segments.append(Segment(Point(0, 0), Point(20, 0)));
  segments.append(Segment(Point(20, 0), Point(20, 20)));
  segments.append(Segment(Point(20, 20), Point(0, 20)));
  segments.append(Segment(Point(0, 20), Point(0, 0)));
  segments.append(Segment(Point(0, 0), Point(20, 20)));

  Tikz_Plane plane(150, 100);
  const auto arrangement = visualize_segment_arrangement(
      plane, segments, SegmentArrangement(),
      true, true, false,
      tikz_area_style("teal!60!black", "teal!12", 0.30),
      tikz_wire_style("teal!70!black"),
      tikz_points_style("teal!70!black"),
      true);

  EXPECT_GT(arrangement.faces.size(), 1U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();

  EXPECT_NE(result.find("blue!20"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeShortestPathInPolygon)
{
  const Polygon polygon = make_shortest_path_polygon();
  const Point source(2, 16);
  const Point target(18, 16);

  Tikz_Plane plane(170, 110);
  const auto path = visualize_shortest_path_in_polygon(
      plane, polygon, source, target, ShortestPathInPolygon());

  EXPECT_GE(count_points(path), 2U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();

  EXPECT_NE(result.find("draw=red"), std::string::npos);
  EXPECT_NE(result.find("fill=green!50!black"), std::string::npos);
  EXPECT_NE(result.find("fill=blue"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeAlphaShape)
{
  DynList<Point> points;
  points.append(Point(-12, -8));
  points.append(Point(-10, 9));
  points.append(Point(-1, 14));
  points.append(Point(10, 10));
  points.append(Point(13, -7));
  points.append(Point(0, -13));
  points.append(Point(0, 0));

  Tikz_Plane plane(170, 110);
  const auto alpha_shape = visualize_alpha_shape(
      plane, points, Geom_Number(10000), AlphaShape(),
      true);

  EXPECT_GT(alpha_shape.boundary_edges.size(), 0U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();

  EXPECT_NE(result.find("orange!90!black"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeRegularTriangulation)
{
  Array<RegularTriangulationBowyerWatson::WeightedSite> sites;
  sites.append({Point(-10, -6), Geom_Number(0)});
  sites.append({Point(-2, 10), Geom_Number(2)});
  sites.append({Point(10, -4), Geom_Number(1)});
  sites.append({Point(15, 8), Geom_Number(3)});
  sites.append({Point(2, -12), Geom_Number(0)});

  Tikz_Plane plane(160, 110);
  const auto rt = visualize_regular_triangulation(plane, sites);

  EXPECT_GE(rt.sites.size(), 4U);
  EXPECT_GT(rt.triangles.size(), 0U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();
  EXPECT_NE(result.find("blue!60"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeClosestPair)
{
  DynList<Point> points;
  points.append(Point(-12, -8));
  points.append(Point(-2, 5));
  points.append(Point(1, 7));
  points.append(Point(2, 7));
  points.append(Point(10, -6));

  Tikz_Plane plane(150, 100);
  const auto cp = visualize_closest_pair(plane, points);

  EXPECT_TRUE(cp.distance_squared >= 0);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();
  EXPECT_NE(result.find("draw=red"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeRotatingCalipers)
{
  const Polygon poly = make_convex_for_calipers();
  Tikz_Plane plane(160, 110);

  const auto rc = visualize_rotating_calipers(plane, poly);

  EXPECT_TRUE(rc.diameter.distance_squared > 0);
  EXPECT_TRUE(rc.width.width_squared >= 0);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();
  EXPECT_NE(result.find("draw=red"), std::string::npos);
  EXPECT_NE(result.find("draw=blue"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeHalfPlaneIntersection)
{
  const Polygon clip = make_convex_for_calipers();
  const auto hps = HalfPlaneIntersection::from_convex_polygon(clip);

  Tikz_Plane plane(160, 110);
  const Polygon inter = visualize_half_plane_intersection(plane, hps);

  EXPECT_GT(inter.size(), 0U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();
  EXPECT_NE(result.find("fill=red!25"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeMinkowskiSum)
{
  Polygon p;
  p.add_vertex(Point(-4, -2));
  p.add_vertex(Point(3, -2));
  p.add_vertex(Point(1, 4));
  p.close();

  Polygon q;
  q.add_vertex(Point(-3, -1));
  q.add_vertex(Point(2, -1));
  q.add_vertex(Point(0, 3));
  q.close();

  Tikz_Plane plane(160, 110);
  const Polygon sum = visualize_minkowski_sum(plane, p, q);

  EXPECT_GT(sum.size(), 0U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();
  EXPECT_NE(result.find("fill=red!26"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeMonotoneTriangulation)
{
  const Polygon concave = make_concave_for_decomposition();
  Tikz_Plane plane(170, 110);

  const DynList<Triangle> tris = visualize_monotone_triangulation(plane, concave);
  size_t tri_count = 0;
  for (DynList<Triangle>::Iterator it(tris); it.has_curr(); it.next_ne())
    ++tri_count;
  EXPECT_GT(tri_count, 0U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();
  EXPECT_NE(result.find("blue!65"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeVisibilityPolygon)
{
  const Polygon polygon = make_visibility_polygon();
  const Point query(2, 16);

  Tikz_Plane plane(170, 110);
  const Polygon vis = visualize_visibility_polygon(plane, polygon, query);
  EXPECT_GT(vis.size(), 0U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();
  EXPECT_NE(result.find("orange!90!black"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeLineSweep)
{
  Array<Segment> segments;
  segments.append(Segment(Point(-15, -8), Point(15, 10)));
  segments.append(Segment(Point(-12, 11), Point(14, -7)));
  segments.append(Segment(Point(-15, 2), Point(15, 2)));

  Tikz_Plane plane(170, 110);
  const auto intersections = visualize_line_sweep(plane, segments);

  EXPECT_GT(intersections.size(), 0U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();
  EXPECT_NE(result.find("blue!60"), std::string::npos);
  EXPECT_NE(result.find("fill=red"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeKDTreePartitions)
{
  Array<Point> points;
  points.append(Point(10, 10));
  points.append(Point(20, 30));
  points.append(Point(50, 40));
  points.append(Point(70, 80));
  points.append(Point(90, 20));
  const auto kd = KDTreePointSearch::build(points, 0, 0, 100, 100);

  Tikz_Plane plane(180, 110);
  const auto snap = visualize_kdtree_partitions(plane, kd, true, true);

  EXPECT_EQ(snap.points.size(), points.size());
  EXPECT_GT(snap.partitions.size(), 0U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();
  EXPECT_NE(result.find("fill=red"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeRangeTreeAndQuery)
{
  DynList<Point> points;
  points.append(Point(1, 1));
  points.append(Point(3, 5));
  points.append(Point(4, 4));
  points.append(Point(7, 2));
  points.append(Point(9, 8));

  RangeTree2D tree;
  tree.build(points);

  Tikz_Plane plane(180, 110);
  const auto viz = visualize_range_tree_query(
      plane, tree, Rectangle(2, 1, 6, 5));

  EXPECT_GT(viz.snapshot.nodes.size(), 0U);
  EXPECT_GT(viz.query_hits.size(), 0U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();
  EXPECT_NE(result.find("draw=red"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeAABBTreeAndQuery)
{
  AABBTree tree;
  Array<AABBTree::Entry> entries;
  entries.append({Rectangle(0, 0, 5, 5), 0});
  entries.append({Rectangle(3, 3, 8, 8), 1});
  entries.append({Rectangle(10, 10, 15, 15), 2});
  tree.build(entries);

  Tikz_Plane plane(180, 110);
  const auto viz = visualize_aabb_tree_query(
      plane, tree, Rectangle(2, 2, 6, 6));

  EXPECT_GT(viz.snapshot.nodes.size(), 0U);
  EXPECT_GT(viz.query_hit_ids.size(), 0U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();
  EXPECT_NE(result.find("draw=red"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeRTreeAndQuery)
{
  RTree<int, 4, 2> tree;
  tree.insert(Rectangle(0, 0, 5, 5), 0);
  tree.insert(Rectangle(3, 3, 8, 8), 1);
  tree.insert(Rectangle(10, 10, 15, 15), 2);

  Tikz_Plane plane(180, 110);
  const auto viz = visualize_rtree_query(plane, tree, Rectangle(2, 2, 6, 6));

  EXPECT_GT(viz.snapshot.nodes.size(), 0U);
  EXPECT_GT(viz.query_hit_boxes.size(), 0U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();
  EXPECT_NE(result.find("draw=red"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeRStarTreeAndQuery)
{
  RStarTree<int, 4, 2> tree;
  tree.insert(Rectangle(0, 0, 5, 5), 0);
  tree.insert(Rectangle(3, 3, 8, 8), 1);
  tree.insert(Rectangle(10, 10, 15, 15), 2);

  Tikz_Plane plane(180, 110);
  const auto viz = visualize_rtree_query(plane, tree, Rectangle(2, 2, 6, 6));

  EXPECT_GT(viz.snapshot.nodes.size(), 0U);
  EXPECT_GT(viz.query_hit_boxes.size(), 0U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();
  EXPECT_NE(result.find("draw=red"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeRTreeWithoutQuery)
{
  RTree<int, 4, 2> tree;
  for (int i = 0; i < 20; ++i)
    tree.insert(Rectangle(i, i, i + 2, i + 2), i);

  Tikz_Plane plane(180, 110);
  const auto snapshot = visualize_rtree(plane, tree);
  EXPECT_GT(snapshot.nodes.size(), 0U);

  std::ostringstream output;
  plane.draw(output);
  EXPECT_FALSE(has_nan_or_inf(output.str()));
}

TEST(TikzGeomAlgorithmsTest, VisualizeShortestPathWithPortals)
{
  const Polygon polygon = make_shortest_path_polygon();
  const Point source(2, 16);
  const Point target(18, 16);

  Tikz_Plane plane(170, 110);
  const auto debug = visualize_shortest_path_with_portals(
      plane, polygon, source, target, ShortestPathInPolygon());

  EXPECT_GE(debug.portals.size(), 2U);
  EXPECT_GE(count_points(debug.path), 2U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();

  EXPECT_NE(result.find("draw=purple"), std::string::npos);
  EXPECT_NE(result.find("dashed"), std::string::npos);
  EXPECT_NE(result.find("draw=red"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, VisualizeConvexDecomposition)
{
  const Polygon concave = make_concave_for_decomposition();

  Tikz_Plane plane(170, 110);
  const auto parts = visualize_convex_decomposition(
      plane, concave, ConvexPolygonDecomposition(),
      true, tikz_wire_style("black", true),
      true);

  EXPECT_GT(parts.size(), 1U);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();

  EXPECT_NE(result.find("blue!20"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomAlgorithmsTest, ComputeFunnelTrace)
{
  const Polygon polygon = make_shortest_path_polygon();
  const Point source(2, 16);
  const Point target(18, 16);

  const FunnelTraceResult trace =
      compute_shortest_path_funnel_trace(polygon, source, target);

  EXPECT_GE(trace.portals.size(), 2U);
  EXPECT_GT(trace.steps.size(), 0U);
  EXPECT_GE(count_points(trace.final_path), 2U);
  EXPECT_EQ(trace.final_path(0), source);
  EXPECT_EQ(trace.final_path(trace.final_path.size() - 1), target);
}

TEST(TikzGeomAlgorithmsTest, PutFunnelTraceStepRenders)
{
  const Polygon polygon = make_shortest_path_polygon();
  const Point source(2, 16);
  const Point target(18, 16);

  const FunnelTraceResult trace =
      compute_shortest_path_funnel_trace(polygon, source, target);
  ASSERT_GT(trace.steps.size(), 0U);

  Tikz_Plane plane(170, 110);
  put_funnel_trace_step(plane, polygon, source, target, trace, 0);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();

  EXPECT_NE(result.find("draw=purple"), std::string::npos);
  EXPECT_NE(result.find("draw=orange!90!black"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzGeomSceneTest, ComposeMultipleAlgorithmsInSingleScene)
{
  Array<Segment> segments;
  segments.append(Segment(Point(-20, 0), Point(20, 0)));
  segments.append(Segment(Point(0, -20), Point(0, 20)));
  segments.append(Segment(Point(-18, -14), Point(18, 14)));

  Tikz_Scene scene(190, 120, 0, 0, true);
  scene.put_cartesian_axis()
      .set_point_radius_mm(0.65);

  const auto arrangement = scene.visualize_segment_arrangement(
      segments,
      SegmentArrangement(),
      true,
      true,
      false,
      tikz_area_style("teal!60!black", "teal!12", 0.30),
      tikz_wire_style("teal!70!black"),
      tikz_points_style("teal!70!black"),
      true);

  DynList<Point> arrangement_points;
  for (size_t i = 0; i < arrangement.vertices.size(); ++i)
    arrangement_points.append(arrangement.vertices(i));

  const Polygon hull = scene.visualize_convex_hull(
      arrangement_points,
      AndrewMonotonicChainConvexHull(),
      tikz_points_style("black"),
      tikz_wire_style("red"),
      tikz_points_style("red"),
      Tikz_Plane::Layer_Default,
      Tikz_Plane::Layer_Overlay,
      true);

  EXPECT_GE(hull.size(), 3U);

  const std::string tikz = scene.to_tikz();
  EXPECT_NE(tikz.find("draw=teal!70!black"), std::string::npos);
  EXPECT_NE(tikz.find("draw=red"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(tikz));
}

TEST(TikzGeomSceneTest, NewVisualizationWrappers)
{
  Tikz_Scene scene(190, 120, 0, 0, true);

  DynList<Point> points;
  points.append(Point(-10, -7));
  points.append(Point(-2, 9));
  points.append(Point(8, -6));
  points.append(Point(12, 8));
  points.append(Point(0, 0));

  const auto dt = scene.visualize_delaunay(points);
  EXPECT_GT(dt.triangles.size(), 0U);

  const auto cp = scene.visualize_closest_pair(points);
  EXPECT_TRUE(cp.distance_squared >= 0);

  Array<Segment> segments;
  segments.append(Segment(Point(-12, -8), Point(12, 10)));
  segments.append(Segment(Point(-12, 10), Point(12, -8)));
  const auto intersections = scene.visualize_line_sweep(segments);
  EXPECT_GT(intersections.size(), 0U);

  Array<Point> kd_points;
  kd_points.append(Point(5, 5));
  kd_points.append(Point(15, 20));
  kd_points.append(Point(35, 10));
  kd_points.append(Point(45, 30));
  const auto kd = KDTreePointSearch::build(kd_points, 0, 0, 50, 40);
  const auto kd_snap = scene.visualize_kdtree_partitions(kd);
  EXPECT_GT(kd_snap.partitions.size(), 0U);

  RangeTree2D range_tree;
  DynList<Point> range_points;
  range_points.append(Point(1, 1));
  range_points.append(Point(3, 4));
  range_points.append(Point(6, 2));
  range_tree.build(range_points);
  const auto rv = scene.visualize_range_tree_query(
      range_tree, Rectangle(0, 0, 4, 5));
  EXPECT_GT(rv.query_hits.size(), 0U);

  AABBTree aabb_tree;
  Array<AABBTree::Entry> aabb_entries;
  aabb_entries.append({Rectangle(0, 0, 4, 4), 0});
  aabb_entries.append({Rectangle(2, 2, 8, 8), 1});
  aabb_tree.build(aabb_entries);
  const auto av = scene.visualize_aabb_tree_query(
      aabb_tree, Rectangle(1, 1, 3, 3));
  EXPECT_GT(av.query_hit_ids.size(), 0U);

  const std::string tikz = scene.to_tikz();
  EXPECT_NE(tikz.find("draw=blue"), std::string::npos);
  EXPECT_NE(tikz.find("draw=red"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(tikz));
}

TEST(TikzGeomSceneTest, StandaloneExportWrapsTikzPicture)
{
  Tikz_Scene scene(120, 80, 2, 3, true);
  scene.add(Segment(Point(0, 0), Point(10, 15)), tikz_wire_style("blue"));

  Tikz_Standalone_Document_Options opts;
  opts.class_options = "tikz,border=6pt";
  const std::string latex = scene.to_standalone(opts);

  EXPECT_NE(latex.find("\\documentclass[tikz,border=6pt]{standalone}"),
            std::string::npos);
  EXPECT_NE(latex.find("}\n\\usepackage{tikz}\n"), std::string::npos);
  EXPECT_EQ(latex.find("}\\n\\usepackage{tikz}\\n"), std::string::npos);
  EXPECT_NE(latex.find("\\usepackage{tikz}"), std::string::npos);
  EXPECT_NE(latex.find("\\begin{tikzpicture}"), std::string::npos);
  EXPECT_NE(latex.find("\\end{document}"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(latex));
}

TEST(TikzGeomSceneTest, StandaloneAndOverlayExportsCollectPatternDependency)
{
  Tikz_Scene patterned(120, 80, 0, 0, true);
  Tikz_Style pattern = tikz_area_style("black", "gray!20", 0.5);
  pattern.pattern = "north east lines";
  pattern.pattern_color = "black";
  patterned.add(Triangle(Point(0, 0), Point(10, 0), Point(5, 8)), pattern);

  const std::string standalone = patterned.to_standalone();
  const size_t library_pos = standalone.find("\\usetikzlibrary{patterns}");
  ASSERT_NE(library_pos, std::string::npos);
  EXPECT_LT(library_pos, standalone.find("\\begin{document}"));

  std::vector<Tikz_Scene> steps;
  steps.push_back(patterned);
  steps.emplace_back(120, 80, 0, 0, true);
  steps.back().add(Point(1, 1), tikz_points_style("red"));

  const std::string overlays = Tikz_Scene::to_beamer_overlays(steps);
  const size_t overlay_library_pos =
      overlays.find("\\usetikzlibrary{patterns}");
  ASSERT_NE(overlay_library_pos, std::string::npos);
  EXPECT_LT(overlay_library_pos, overlays.find("\\begin{document}"));
}

TEST(TikzGeomSceneTest, PatternDependencyCanComeOnlyFromExplicitLegend)
{
  Tikz_Scene scene(120, 80, 0, 0, true);
  Tikz_Style pattern = tikz_area_style("black", "gray!20", 0.5);
  pattern.pattern = "north east lines";
  pattern.pattern_color = "black";
  scene.add_legend_entry("Patterned region", pattern);

  const std::string standalone = scene.to_standalone();
  EXPECT_NE(standalone.find("\\usetikzlibrary{patterns}"),
            std::string::npos);
  EXPECT_NE(standalone.find("Patterned region"), std::string::npos);
}

TEST(TikzGeomSceneTest, PatternDependencyCanComeOnlyFromRegisteredStyle)
{
  Tikz_Scene scene(120, 80, 0, 0, true);
  Tikz_Style pattern = tikz_area_style("black", "gray!20", 0.5);
  pattern.pattern = "grid";
  pattern.pattern_color = "gray";
  scene.register_tikz_style("patternedArea", pattern);

  const std::string standalone = scene.to_standalone();
  EXPECT_NE(standalone.find("\\usetikzlibrary{patterns}"),
            std::string::npos);
  EXPECT_NE(standalone.find("\\tikzset{patternedArea/.style="),
            std::string::npos);
}

TEST(TikzGeomSceneTest, BeamerExportWrapsSingleFrame)
{
  Tikz_Scene scene(120, 80, 2, 3, true);
  scene.add(Segment(Point(0, 0), Point(10, 15)), tikz_wire_style("blue"));

  Tikz_Beamer_Document_Options opts;
  opts.class_options = "aspectratio=43";
  opts.frame_title = "Scene Demo";
  opts.frame_options = "t";

  const std::string latex = scene.to_beamer(opts);

  EXPECT_NE(latex.find("\\documentclass[aspectratio=43]{beamer}"),
            std::string::npos);
  EXPECT_NE(latex.find("\\setbeamertemplate{navigation symbols}{}"),
            std::string::npos);
  EXPECT_NE(latex.find("\\begin{frame}[t]{Scene Demo}"), std::string::npos);
  EXPECT_NE(latex.find("\\usepackage{graphicx}"), std::string::npos);
  EXPECT_NE(latex.find("\\scalebox{"), std::string::npos);
  EXPECT_NE(latex.find("\\begin{tikzpicture}"), std::string::npos);
  EXPECT_EQ(latex.find("\\documentclass[handout,aspectratio=43]{beamer}"),
            std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(latex));
}

TEST(TikzGeomSceneTest, HandoutExportAddsHandoutClassOption)
{
  Tikz_Scene scene(120, 80, 0, 0, true);
  scene.add(Point(1, 1), tikz_points_style("red"));

  Tikz_Beamer_Document_Options opts;
  opts.class_options = "aspectratio=169";
  opts.frame_title = "Handout Demo";

  const std::string latex = scene.to_handout(opts);

  EXPECT_NE(latex.find("\\documentclass[handout,aspectratio=169]{beamer}"),
            std::string::npos);
  EXPECT_NE(latex.find("\\begin{frame}[t]{Handout Demo}"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(latex));

  opts.class_options = "handout,aspectratio=169";
  const std::string latex_no_dup = scene.to_handout(opts);
  EXPECT_NE(latex_no_dup.find("\\documentclass[handout,aspectratio=169]{beamer}"),
            std::string::npos);
  EXPECT_EQ(latex_no_dup.find("handout,handout"), std::string::npos);
}

TEST(TikzGeomSceneTest, BeamerOverlaysExportWrapsOnlyBlocks)
{
  std::vector<Tikz_Scene> steps;
  steps.emplace_back(120, 80, 0, 0, true);
  steps.back().add(Point(2, 3), tikz_points_style("red"));
  steps.emplace_back(120, 80, 0, 0, true);
  steps.back().add(Segment(Point(0, 0), Point(12, 9)), tikz_wire_style("blue"));

  Tikz_Beamer_Document_Options opts;
  opts.class_options = "aspectratio=169";
  opts.frame_title = "Overlay Demo";
  opts.frame_options = "t";

  const std::string latex = Tikz_Scene::to_beamer_overlays(steps, opts);

  EXPECT_NE(latex.find("\\documentclass[aspectratio=169]{beamer}"),
            std::string::npos);
  EXPECT_NE(latex.find("\\begin{frame}[t]{Overlay Demo}"), std::string::npos);
  EXPECT_NE(latex.find("\\only<1>{"), std::string::npos);
  EXPECT_NE(latex.find("\\only<2>{"), std::string::npos);
  EXPECT_NE(latex.find("\\scalebox{"), std::string::npos);
  EXPECT_EQ(latex.find("\\only<3>{"), std::string::npos);
  EXPECT_EQ(latex.find("\\documentclass[handout,aspectratio=169]{beamer}"),
            std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(latex));
}

TEST(TikzGeomSceneTest, HandoutOverlaysAddsHandoutClassOption)
{
  std::vector<Tikz_Scene> steps;
  steps.emplace_back(120, 80, 0, 0, true);
  steps.back().add(Point(4, 5), tikz_points_style("black"));
  steps.emplace_back(120, 80, 0, 0, true);
  steps.back().add(Segment(Point(0, 0), Point(5, 5)),
                   tikz_wire_style("blue"));

  Tikz_Beamer_Document_Options opts;
  opts.class_options = "handout,aspectratio=43";
  opts.frame_title = "Overlay Handout";

  const std::string latex = Tikz_Scene::to_handout_overlays(steps, opts);
  EXPECT_NE(latex.find("\\documentclass[handout,aspectratio=43]{beamer}"),
            std::string::npos);
  EXPECT_EQ(latex.find("handout,handout"), std::string::npos);
  EXPECT_EQ(latex.find("\\only<1>{"), std::string::npos);
  EXPECT_NE(latex.find("draw=blue"), std::string::npos);
  EXPECT_EQ(latex.find("fill=black"), std::string::npos);
  const size_t picture = latex.find("\\begin{tikzpicture}");
  ASSERT_NE(picture, std::string::npos);
  EXPECT_EQ(latex.find("\\begin{tikzpicture}", picture + 1),
            std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(latex));
}

TEST(TikzGeomSceneTest, BeamerRejectsInvalidPictureBounds)
{
  Tikz_Scene scene(120, 80, 0, 0, true);
  Tikz_Beamer_Document_Options opts;
  opts.max_picture_height_mm = 0.0;
  EXPECT_THROW(scene.to_beamer(opts), std::domain_error);
}

TEST(TikzGeomSceneTest, BeamerOverlaysHandlesEmptySteps)
{
  const std::vector<Tikz_Scene> steps;
  const std::string latex = Tikz_Scene::to_beamer_overlays(steps);

  EXPECT_NE(latex.find("No overlays provided."), std::string::npos);
  EXPECT_EQ(latex.find("\\only<1>{"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(latex));
}

TEST(TikzGeomSceneTest, GeneratedLatexDocumentsCompileWithoutLayoutOverflow)
{
#ifndef ALEPH_PDFLATEX_EXECUTABLE
  GTEST_SKIP() << "pdflatex, standalone, beamer, or TikZ is unavailable";
#else
  Tikz_Scene patterned(220, 130, 0, 0, true);
  Tikz_Style pattern = tikz_area_style("black", "orange!20", 0.5);
  pattern.pattern = "north east lines";
  pattern.pattern_color = "orange!80!black";
  patterned.add(Triangle(Point(0, 0), Point(20, 0), Point(10, 16)), pattern);

  std::vector<Tikz_Scene> steps;
  for (size_t i = 0; i < 3; ++i)
    {
      steps.emplace_back(178, 108, 0, 0, true);
      steps.back().add(
          Segment(Point(-20, static_cast<long>(i)),
                  Point(20, 20 - static_cast<long>(i))),
          tikz_wire_style(i == 2 ? "blue" : "black"));
    }

  Tikz_Beamer_Document_Options options;
  options.frame_title = "TikZ layout smoke test";
  const std::string beamer = Tikz_Scene::to_beamer_overlays(steps, options);
  const std::string handout = Tikz_Scene::to_handout_overlays(steps, options);

  Scoped_Latex_Temp_Directory temporary;
  const std::vector<std::pair<std::string, std::string>> documents = {
      {"standalone", patterned.to_standalone()},
      {"beamer", beamer},
      {"handout", handout}
    };

  for (const auto & [name, document] : documents)
    {
      const std::filesystem::path tex = temporary.path() / (name + ".tex");
      const std::filesystem::path transcript =
          temporary.path() / (name + ".stdout");
      ASSERT_TRUE(write_text_file(tex, document));

      const std::string command =
          shell_quote(ALEPH_PDFLATEX_EXECUTABLE) +
          " -interaction=nonstopmode -halt-on-error -output-directory=" +
          shell_quote(temporary.path().string()) + " " +
          shell_quote(tex.string()) + " > " +
          shell_quote(transcript.string()) + " 2>&1";
      const int status = std::system(command.c_str());
      const std::string diagnostics = read_text_file(transcript);
      ASSERT_EQ(status, 0) << name << " failed:\n" << diagnostics;

      const std::string log =
          read_text_file(temporary.path() / (name + ".log"));
      EXPECT_EQ(log.find("Overfull"), std::string::npos)
          << name << " contains a layout overflow:\n" << log;
      EXPECT_EQ(log.find("LaTeX Error"), std::string::npos)
          << name << " contains a LaTeX error:\n" << log;
      EXPECT_EQ(log.find("Undefined control sequence"), std::string::npos)
          << name << " contains an undefined command:\n" << log;
    }
#endif
}
