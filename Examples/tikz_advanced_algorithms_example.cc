#include <fstream>
#include <iostream>
#include <string>

#include <tikzgeom_algorithms.H>

using namespace Aleph;

namespace
{

Array<Segment> make_arrangement_segments()
{
  Array<Segment> segments;
  segments.append(Segment(Point(-28, 0), Point(28, 0)));
  segments.append(Segment(Point(0, -24), Point(0, 24)));
  segments.append(Segment(Point(-24, -18), Point(24, 18)));
  segments.append(Segment(Point(-24, 18), Point(24, -18)));
  return segments;
}

Polygon make_shortest_path_polygon()
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(24, 0));
  p.add_vertex(Point(24, 20));
  p.add_vertex(Point(14, 20));
  p.add_vertex(Point(14, 8));
  p.add_vertex(Point(10, 8));
  p.add_vertex(Point(10, 20));
  p.add_vertex(Point(0, 20));
  p.close();
  return p;
}

DynList<Point> make_alpha_points()
{
  DynList<Point> points;
  points.append(Point(-18, -10));
  points.append(Point(-14, 12));
  points.append(Point(-4, 17));
  points.append(Point(8, 15));
  points.append(Point(17, 8));
  points.append(Point(20, -8));
  points.append(Point(10, -16));
  points.append(Point(-2, -18));
  points.append(Point(2, 2));
  points.append(Point(-6, 4));
  return points;
}

Polygon make_decomposition_polygon()
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(16, 0));
  p.add_vertex(Point(16, 12));
  p.add_vertex(Point(10, 12));
  p.add_vertex(Point(10, 5));
  p.add_vertex(Point(6, 5));
  p.add_vertex(Point(6, 12));
  p.add_vertex(Point(0, 12));
  p.close();
  return p;
}

} // namespace

int main(int argc, char * argv[])
{
  const std::string output_path =
      argc > 1 ? argv[1] : "tikz_advanced_algorithms_example.tex";

  std::ofstream out(output_path);
  if (not out)
    {
      std::cerr << "Cannot open output file: " << output_path << '\n';
      return 1;
    }

  // Figure 1: Segment arrangement.
  Tikz_Plane arrangement_plane(210, 115, 6, 6);
  arrangement_plane.put_cartesian_axis();
  arrangement_plane.set_point_radius_mm(0.65);

  const auto arrangement = visualize_segment_arrangement(
      arrangement_plane,
      make_arrangement_segments(),
      SegmentArrangement(),
      true,
      true,
      false,
      tikz_area_style("teal!60!black", "teal!12", 0.35),
      tikz_wire_style("teal!70!black"),
      tikz_points_style("teal!80!black"),
      true);

  put_in_plane(arrangement_plane,
               Text(Point(-30, 26),
                    "Segment Arrangement: V=" + std::to_string(arrangement.vertices.size()) +
                    ", E=" + std::to_string(arrangement.edges.size())),
               make_tikz_draw_style("black"),
               Tikz_Plane::Layer_Overlay);

  // Figure 2: Shortest path in polygon.
  Tikz_Plane shortest_plane(210, 115, 6, 6);
  shortest_plane.put_cartesian_axis();
  shortest_plane.set_point_radius_mm(0.75);

  const Polygon corridor = make_shortest_path_polygon();
  const Point source(2, 16);
  const Point target(22, 16);

  const ShortestPathDebugResult shortest_debug = visualize_shortest_path_with_portals(
      shortest_plane,
      corridor,
      source,
      target,
      ShortestPathInPolygon(),
      tikz_area_style("black", "gray!15", 0.28),
      tikz_points_style("green!50!black"), // source
      tikz_points_style("blue"),           // target
      tikz_wire_style("purple", true),     // portals
      tikz_path_style("red"),              // final path
      true,
      tikz_points_style("red"));

  size_t path_nodes = 0;
  for (DynList<Point>::Iterator it(shortest_debug.path); it.has_curr(); it.next_ne())
    ++path_nodes;

  put_in_plane(shortest_plane,
               Text(Point(-1, 22),
                    "Shortest Path + Portals: path nodes=" +
                    std::to_string(path_nodes) +
                    ", portals=" + std::to_string(shortest_debug.portals.size())),
               make_tikz_draw_style("black"),
               Tikz_Plane::Layer_Overlay);

  // Figure 3: Convex decomposition.
  Tikz_Plane decomp_plane(210, 115, 6, 6);
  decomp_plane.put_cartesian_axis();
  decomp_plane.set_point_radius_mm(0.70);

  const Polygon decomp_input = make_decomposition_polygon();
  const Array<Polygon> decomp_parts = visualize_convex_decomposition(
      decomp_plane,
      decomp_input,
      ConvexPolygonDecomposition(),
      true,
      tikz_wire_style("black", true),
      true,
      tikz_area_style("blue!60!black", "blue!15", 0.38));

  put_in_plane(decomp_plane,
               Text(Point(-2, 14),
                    "Convex Decomposition: parts=" +
                    std::to_string(decomp_parts.size())),
               make_tikz_draw_style("black"),
               Tikz_Plane::Layer_Overlay);

  // Figure 4: Alpha shape.
  Tikz_Plane alpha_plane(210, 115, 6, 6);
  alpha_plane.put_cartesian_axis();
  alpha_plane.set_point_radius_mm(0.75);

  const Geom_Number alpha_squared = Geom_Number(9000);
  const AlphaShape::Result alpha_shape = visualize_alpha_shape(
      alpha_plane,
      make_alpha_points(),
      alpha_squared,
      AlphaShape(),
      true,
      tikz_wire_style("gray!55"),
      tikz_path_style("orange!90!black"),
      true,
      tikz_points_style("black"));

  put_in_plane(alpha_plane,
               Text(Point(-22, 22),
                    "Alpha Shape: boundary edges=" +
                    std::to_string(alpha_shape.boundary_edges.size())),
               make_tikz_draw_style("black"),
               Tikz_Plane::Layer_Overlay);

  out << "\\documentclass[tikz,border=8pt]{standalone}\n"
      << "\\usepackage{tikz}\n"
      << "\\begin{document}\n\n";

  arrangement_plane.draw(out, true);
  out << "\n\\vspace{5mm}\n\n";
  shortest_plane.draw(out, true);
  out << "\n\\vspace{5mm}\n\n";
  decomp_plane.draw(out, true);
  out << "\n\\vspace{5mm}\n\n";
  alpha_plane.draw(out, true);

  out << "\n\\end{document}\n";

  std::cout << "Generated " << output_path << '\n'
            << "Compile with: pdflatex " << output_path << '\n';

  return 0;
}
