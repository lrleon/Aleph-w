#include <fstream>
#include <iostream>
#include <string>

#include <tikzgeom_scene.H>

using namespace Aleph;

namespace
{

Polygon make_corridor()
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
  std::string output_path = "tikz_scene_beamer_example.tex";
  bool handout = false;

  for (int i = 1; i < argc; ++i)
    {
      const std::string arg = argv[i];
      if (arg == "--handout")
        handout = true;
      else
        output_path = arg;
    }

  std::ofstream out(output_path);
  if (not out)
    {
      std::cerr << "Cannot open output file: " << output_path << '\n';
      return 1;
    }

  Tikz_Scene scene(178, 108, 0, 0, true);
  scene.put_cartesian_axis()
      .set_point_radius_mm(0.70);

  const Polygon polygon = make_corridor();
  const Point source(2, 16);
  const Point target(22, 16);

  const ShortestPathDebugResult shortest =
      scene.visualize_shortest_path_with_portals(
          polygon,
          source,
          target,
          ShortestPathInPolygon(),
          tikz_area_style("black", "gray!15", 0.24),
          tikz_points_style("green!50!black"),
          tikz_points_style("blue"),
          tikz_wire_style("purple", true),
          tikz_path_style("orange!90!black"),
          true,
          tikz_points_style("orange!90!black"));

  scene.add(Text(Point(-1, 22),
                 "Tikz_Scene beamer export: portals=" +
                     std::to_string(shortest.portals.size())),
            make_tikz_draw_style("black"),
            Tikz_Plane::Layer_Overlay);

  Tikz_Beamer_Document_Options opts;
  opts.frame_title = handout ? "TikZ Scene (Handout)" : "TikZ Scene (Beamer)";
  opts.frame_options = "t";
  opts.class_options = "aspectratio=169";

  if (handout)
    scene.draw_handout(out, opts);
  else
    scene.draw_beamer(out, opts);

  std::cout << "Generated " << output_path << '\n'
            << "Mode: " << (handout ? "handout" : "beamer") << '\n'
            << "Portals: " << shortest.portals.size() << '\n'
            << "Compile with: pdflatex " << output_path << '\n';

  return 0;
}
