#include <fstream>
#include <iostream>
#include <string>

#include <tikzgeom_scene.H>

using namespace Aleph;

namespace
{

Array<Segment> make_arrangement_segments()
{
  Array<Segment> segments;
  segments.append(Segment(Point(-30, 0), Point(30, 0)));
  segments.append(Segment(Point(0, -24), Point(0, 24)));
  segments.append(Segment(Point(-26, -18), Point(26, 18)));
  segments.append(Segment(Point(-26, 18), Point(26, -18)));
  return segments;
}

Polygon make_corridor(const Geom_Number & shift_x)
{
  Polygon p;
  p.add_vertex(Point(70 + shift_x, 0));
  p.add_vertex(Point(94 + shift_x, 0));
  p.add_vertex(Point(94 + shift_x, 20));
  p.add_vertex(Point(84 + shift_x, 20));
  p.add_vertex(Point(84 + shift_x, 8));
  p.add_vertex(Point(80 + shift_x, 8));
  p.add_vertex(Point(80 + shift_x, 20));
  p.add_vertex(Point(70 + shift_x, 20));
  p.close();
  return p;
}

} // namespace

int main(int argc, char * argv[])
{
  const std::string output_path =
      argc > 1 ? argv[1] : "tikz_scene_example.tex";

  std::ofstream out(output_path);
  if (not out)
    {
      std::cerr << "Cannot open output file: " << output_path << '\n';
      return 1;
    }

  Tikz_Scene scene(230, 130, 6, 6, true);
  scene.put_cartesian_axis()
      .set_point_radius_mm(0.65);

  const auto arrangement = scene.visualize_segment_arrangement(
      make_arrangement_segments(),
      SegmentArrangement(),
      true,
      true,
      false,
      tikz_area_style("teal!60!black", "teal!12", 0.32),
      tikz_wire_style("teal!70!black"),
      tikz_points_style("teal!80!black"),
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
      Tikz_Plane::Layer_Foreground,
      Tikz_Plane::Layer_Overlay,
      true);

  const Polygon corridor = make_corridor(0);
  const Point source(72, 16);
  const Point target(92, 16);

  const ShortestPathDebugResult shortest =
      scene.visualize_shortest_path_with_portals(
          corridor,
          source,
          target,
          ShortestPathInPolygon(),
          tikz_area_style("black", "gray!15", 0.26),
          tikz_points_style("green!50!black"),
          tikz_points_style("blue"),
          tikz_wire_style("purple", true),
          tikz_path_style("orange!90!black"),
          true,
          tikz_points_style("orange!90!black"),
          Tikz_Plane::Layer_Default,
          Tikz_Plane::Layer_Foreground + 5);

  scene.add(Text(Point(-35, 27),
                 "Scene API: arrangement + hull + shortest path"),
            make_tikz_draw_style("black"),
            Tikz_Plane::Layer_Overlay);

  scene.add(Text(Point(66, 24),
                 "Portals=" + std::to_string(shortest.portals.size()) +
                     ", Hull vertices=" + std::to_string(hull.size())),
            make_tikz_draw_style("black"),
            Tikz_Plane::Layer_Overlay);

  scene.draw_standalone(out);

  std::cout << "Generated " << output_path << '\n'
            << "Arrangement vertices: " << arrangement.vertices.size() << '\n'
            << "Hull vertices: " << hull.size() << '\n'
            << "Portals: " << shortest.portals.size() << '\n'
            << "Compile with: pdflatex " << output_path << '\n';

  return 0;
}
