#include <fstream>
#include <iostream>
#include <string>

#include <tikzgeom_scene.H>

using namespace Aleph;

namespace
{

Polygon make_convex_for_calipers()
{
  Polygon p;
  p.add_vertex(Point(-12, -6));
  p.add_vertex(Point(12, -8));
  p.add_vertex(Point(17, 3));
  p.add_vertex(Point(6, 14));
  p.add_vertex(Point(-10, 11));
  p.close();
  return p;
}

Polygon make_visibility_room()
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(22, 0));
  p.add_vertex(Point(22, 20));
  p.add_vertex(Point(12, 20));
  p.add_vertex(Point(12, 9));
  p.add_vertex(Point(8, 9));
  p.add_vertex(Point(8, 20));
  p.add_vertex(Point(0, 20));
  p.close();
  return p;
}

} // namespace

int main(int argc, char * argv[])
{
  const std::string output_path =
      argc > 1 ? argv[1] : "tikz_visibility_calipers_example.tex";

  std::ofstream out(output_path);
  if (not out)
    {
      std::cerr << "Cannot open output file: " << output_path << '\n';
      return 1;
    }

  Tikz_Scene scene(220, 120, 5, 5, true);
  scene.put_cartesian_axis();
  scene.set_point_radius_mm(0.7);

  const Polygon convex = make_convex_for_calipers();
  const auto rc = scene.visualize_rotating_calipers(convex);

  const Polygon room = make_visibility_room();
  const Point query(3, 17);
  const Polygon vis = scene.visualize_visibility_polygon(room, query);

  scene.add(Text(Point(-15, 24),
                 "Rotating calipers: diameter^2=" +
                     std::to_string(geom_number_to_double(rc.diameter.distance_squared))),
            make_tikz_draw_style("black"),
            Tikz_Plane::Layer_Overlay);

  scene.add(Text(Point(-15, 21),
                 "Visibility polygon vertices=" + std::to_string(vis.size())),
            make_tikz_draw_style("black"),
            Tikz_Plane::Layer_Overlay);

  scene.draw_standalone(out);

  std::cout << "Generated " << output_path << '\n';
  std::cout << "Compile with: pdflatex " << output_path << '\n';
  return 0;
}
