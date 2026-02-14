#include <gtest/gtest.h>

#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include <tikzgeom_algorithms.H>
#include <tikzgeom_scene.H>

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

  Tikz_Beamer_Document_Options opts;
  opts.class_options = "handout,aspectratio=43";
  opts.frame_title = "Overlay Handout";

  const std::string latex = Tikz_Scene::to_handout_overlays(steps, opts);
  EXPECT_NE(latex.find("\\documentclass[handout,aspectratio=43]{beamer}"),
            std::string::npos);
  EXPECT_EQ(latex.find("handout,handout"), std::string::npos);
  EXPECT_NE(latex.find("\\only<1>{"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(latex));
}

TEST(TikzGeomSceneTest, BeamerOverlaysHandlesEmptySteps)
{
  const std::vector<Tikz_Scene> steps;
  const std::string latex = Tikz_Scene::to_beamer_overlays(steps);

  EXPECT_NE(latex.find("No overlays provided."), std::string::npos);
  EXPECT_EQ(latex.find("\\only<1>{"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(latex));
}
