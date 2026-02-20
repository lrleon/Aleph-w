#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <tikzgeom_scene.H>

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

std::string step_event(const FunnelTraceStep & step)
{
  if (step.emitted_left) return "emit-left";
  if (step.emitted_right) return "emit-right";
  if (step.tightened_left and step.tightened_right) return "tighten-both";
  if (step.tightened_left) return "tighten-left";
  if (step.tightened_right) return "tighten-right";
  return "noop";
}

Tikz_Scene make_scene_canvas()
{
  Tikz_Scene scene(210, 115, 6, 6, true);
  scene.put_cartesian_axis();
  scene.set_point_radius_mm(0.75);
  return scene;
}

Tikz_Scene render_trace_step_scene(const Polygon & polygon,
                                   const Point & source,
                                   const Point & target,
                                   const FunnelTraceResult & trace,
                                   const size_t step_index)
{
  Tikz_Scene scene = make_scene_canvas();

  scene.put_funnel_trace_step(
      polygon,
      source,
      target,
      trace,
      step_index,
      tikz_area_style("black", "gray!15", 0.22),
      tikz_points_style("green!50!black"),
      tikz_points_style("blue"),
      tikz_wire_style("purple", true),
      tikz_path_style("purple"),
      tikz_path_style("orange!90!black"),
      tikz_path_style("red"),
      true,
      tikz_points_style("red"));

  const FunnelTraceStep & step = trace.steps(step_index);
  scene.add(Text(Point(-1, 22),
                 "Step " + std::to_string(step_index + 1) +
                     "/" + std::to_string(trace.steps.size()) +
                     ", portal=" + std::to_string(step.portal_index) +
                     ", event=" + step_event(step)),
            make_tikz_draw_style("black"),
            Tikz_Plane::Layer_Overlay);

  return scene;
}

Tikz_Scene render_final_scene(const Polygon & polygon,
                              const Point & source,
                              const Point & target)
{
  Tikz_Scene scene = make_scene_canvas();

  const auto debug = scene.visualize_shortest_path_with_portals(
      polygon, source, target, ShortestPathInPolygon());

  scene.add(Text(Point(-1, 22),
                 "Final shortest path with portals: path nodes=" +
                     std::to_string(debug.path.size())),
            make_tikz_draw_style("black"),
            Tikz_Plane::Layer_Overlay);

  return scene;
}

} // namespace

int main(int argc, char * argv[])
{
  const std::string output_path =
      argc > 1 ? argv[1] : "tikz_funnel_beamer_example.tex";

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

  std::vector<Tikz_Scene> overlays;
  overlays.reserve(trace.steps.size() + 1);

  for (size_t i = 0; i < trace.steps.size(); ++i)
    overlays.push_back(render_trace_step_scene(polygon, source, target, trace, i));

  overlays.push_back(render_final_scene(polygon, source, target));

  Tikz_Beamer_Document_Options opts;
  opts.class_options = "aspectratio=169";
  opts.frame_options = "t";
  opts.frame_title = "Shortest Path Funnel Trace (SSFA)";

  Tikz_Scene::draw_beamer_overlays(out, overlays, opts);

  std::cout << "Generated " << output_path << '\n'
            << "Overlays: " << overlays.size() << '\n'
            << "Compile with: pdflatex " << output_path << '\n';

  return 0;
}
