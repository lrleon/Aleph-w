#include <fstream>
#include <iostream>
#include <string>

#include <tikzgeom_algorithms.H>

using namespace Aleph;

namespace
{

Polygon make_funnel_polygon()
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

} // namespace

int main(int argc, char * argv[])
{
  const std::string output_path =
      argc > 1 ? argv[1] : "tikz_funnel_animation_example.tex";

  std::ofstream out(output_path);
  if (not out)
    {
      std::cerr << "Cannot open output file: " << output_path << '\n';
      return 1;
    }

  const Polygon polygon = make_funnel_polygon();
  const Point source(2, 16);
  const Point target(22, 16);

  const FunnelTraceResult trace =
      compute_shortest_path_funnel_trace(polygon, source, target);

  out << "\\documentclass[a4paper]{article}\n"
      << "\\usepackage[margin=12mm]{geometry}\n"
      << "\\usepackage{tikz}\n"
      << "\\pagestyle{empty}\n"
      << "\\begin{document}\n\n";

  if (trace.steps.size() == 0)
    {
      Tikz_Plane frame(210, 115, 6, 6);
      frame.put_cartesian_axis();
      frame.set_point_radius_mm(0.75);

      const auto debug = visualize_shortest_path_with_portals(
          frame, polygon, source, target, ShortestPathInPolygon());

      put_in_plane(frame,
                   Text(Point(-1, 22),
                        "No funnel iterations (direct/degenerate case), portals=" +
                        std::to_string(debug.portals.size())),
                   make_tikz_draw_style("black"),
                   Tikz_Plane::Layer_Overlay);

      frame.draw(out, true);
      out << "\n";
    }
  else
    {
      for (size_t i = 0; i < trace.steps.size(); ++i)
        {
          Tikz_Plane frame(210, 115, 6, 6);
          frame.put_cartesian_axis();
          frame.set_point_radius_mm(0.75);

          put_funnel_trace_step(
              frame,
              polygon,
              source,
              target,
              trace,
              i,
              tikz_area_style("black", "gray!15", 0.22),
              tikz_points_style("green!50!black"),
              tikz_points_style("blue"),
              tikz_wire_style("purple", true),
              tikz_path_style("purple"),
              tikz_path_style("orange!90!black"),
              tikz_path_style("red"),
              true,
              tikz_points_style("red"));

          const FunnelTraceStep & step = trace.steps(i);
          std::string event = "tighten";
          if (step.emitted_left) event = "emit-left";
          else if (step.emitted_right) event = "emit-right";
          else if (step.tightened_left and step.tightened_right) event = "tighten-both";
          else if (step.tightened_left) event = "tighten-left";
          else if (step.tightened_right) event = "tighten-right";

          put_in_plane(frame,
                       Text(Point(-1, 22),
                            "Funnel step " + std::to_string(i + 1) +
                            "/" + std::to_string(trace.steps.size()) +
                            ", portal=" + std::to_string(step.portal_index) +
                            ", event=" + event),
                       make_tikz_draw_style("black"),
                       Tikz_Plane::Layer_Overlay);

          frame.draw(out, true);

          if (i + 1 < trace.steps.size())
            out << "\n\\newpage\n\n";
        }

      out << "\n\\newpage\n\n";

      Tikz_Plane final_frame(210, 115, 6, 6);
      final_frame.put_cartesian_axis();
      final_frame.set_point_radius_mm(0.75);

      const auto debug = visualize_shortest_path_with_portals(
          final_frame, polygon, source, target, ShortestPathInPolygon());

      put_in_plane(final_frame,
                   Text(Point(-1, 22),
                        "Final shortest path with portals: path nodes=" +
                        std::to_string(debug.path.size())),
                   make_tikz_draw_style("black"),
                   Tikz_Plane::Layer_Overlay);

      final_frame.draw(out, true);
      out << "\n";
    }

  out << "\\end{document}\n";

  std::cout << "Generated " << output_path << '\n'
            << "Frames: " << trace.steps.size() << " + final summary page" << '\n'
            << "Compile with: pdflatex " << output_path << '\n';

  return 0;
}
