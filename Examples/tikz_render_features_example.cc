#include <fstream>
#include <iostream>
#include <string>

#include <tikzgeom_scene.H>

using namespace Aleph;

int main(int argc, char * argv[])
{
  const std::string output_path =
      argc > 1 ? argv[1] : "tikz_render_features_example.tex";

  std::ofstream out(output_path);
  if (not out)
    {
      std::cerr << "Cannot open output file: " << output_path << '\n';
      return 1;
    }

  Tikz_Scene scene(220, 130, 6, 6, true);
  scene.put_cartesian_axis()
      .put_coordinate_grid(2.0, 2.0, true)
      .enable_native_tikz_layers(true)
      .enable_auto_legend(true)
      .set_point_radius_mm(0.75);

  Tikz_Style grid_style;
  grid_style.draw_color = "gray!45";
  grid_style.dotted = true;
  grid_style.line_width_mm = 0.2;
  scene.set_grid_style(grid_style);

  Tikz_Style route_style;
  route_style.draw_color = "blue!70!black";
  route_style.thick = true;
  scene.register_tikz_style("route", route_style);

  Polygon poly;
  poly.add_vertex(Point(-8, -4));
  poly.add_vertex(Point(8, -4));
  poly.add_vertex(Point(11, 5));
  poly.add_vertex(Point(-5, 9));
  poly.close();

  Tikz_Style hatch;
  hatch.draw_color = "black";
  hatch.fill = true;
  hatch.fill_color = "orange!18";
  hatch.pattern = "north east lines";
  hatch.pattern_color = "orange!80!black";
  scene.add(poly, hatch, Tikz_Plane::Layer_Background);

  scene.add_legend_entry("Obstacle", hatch);

  Tikz_Style named_route;
  named_route.tikz_style_name = "route";
  named_route.draw_color = "";
  scene.add(Segment(Point(-12, -8), Point(12, 10)),
            named_route, Tikz_Plane::Layer_Foreground);

  scene.add_legend_entry("Route", route_style);

  const Array<Point> points = []
  {
    Array<Point> pts;
    pts.append(Point(-10, -2));
    pts.append(Point(-2, 6));
    pts.append(Point(6, -1));
    return pts;
  }();

  Tikz_Style point_style = tikz_points_style("red");
  scene.add_points(points, point_style, Tikz_Plane::Layer_Overlay);

  Tikz_Style label_style = make_tikz_draw_style("black");
  label_style.text_anchor = "west";
  put_point_label_in_plane(scene.plane(), points(0), "A", "above right", label_style);
  put_point_label_in_plane(scene.plane(), points(1), "B", "below left", label_style);
  put_point_label_in_plane(scene.plane(), points(2), "C", "above", label_style);

  Tikz_Style bez_style = make_tikz_draw_style("purple");
  bez_style.thick = true;
  put_cubic_bezier_native_in_plane(scene.plane(),
                                   Point(-12, 9), Point(-4, 15),
                                   Point(4, -12), Point(12, -6),
                                   bez_style, Tikz_Plane::Layer_Overlay);

  scene.add_legend_entry("Bezier", bez_style);

  scene.add(Text(Point(-15, 16), "Render features demo"),
            make_tikz_draw_style("black"),
            Tikz_Plane::Layer_Overlay);

  scene.draw_standalone(out);

  std::cout << "Generated " << output_path << '\n';
  std::cout << "Compile with: pdflatex " << output_path << '\n';
  return 0;
}
