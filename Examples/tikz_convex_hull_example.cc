#include <fstream>
#include <iostream>
#include <string>

#include <tikzgeom_algorithms.H>

using namespace Aleph;

int main(int argc, char * argv[])
{
  const std::string output_path =
      argc > 1 ? argv[1] : "tikz_convex_hull_example.tex";

  std::ofstream out(output_path);
  if (not out)
    {
      std::cerr << "Cannot open output file: " << output_path << '\n';
      return 1;
    }

  DynList<Point> pts;
  pts.append(Point(-28, -8));
  pts.append(Point(-24, 16));
  pts.append(Point(-19, 7));
  pts.append(Point(-15, -14));
  pts.append(Point(-10, 10));
  pts.append(Point(-8, -3));
  pts.append(Point(-3, 17));
  pts.append(Point(2, 4));
  pts.append(Point(6, -13));
  pts.append(Point(11, 12));
  pts.append(Point(17, -4));
  pts.append(Point(20, 14));
  pts.append(Point(24, -10));
  pts.append(Point(26, 5));

  Tikz_Plane plane(180, 110, 6, 6);
  plane.put_cartesian_axis();
  plane.set_point_radius_mm(0.9);

  Tikz_Style input_style = tikz_points_style("black", 0.7);
  Tikz_Style hull_style = tikz_wire_style("red");
  hull_style.thick = true;
  Tikz_Style hull_vertex_style = tikz_points_style("red");

  AndrewMonotonicChainConvexHull hull_algo;
  Polygon hull = visualize_convex_hull(
      plane, pts, hull_algo,
      input_style, hull_style, hull_vertex_style,
      Tikz_Plane::Layer_Default,
      Tikz_Plane::Layer_Foreground,
      true);

  put_in_plane(plane, Text(Point(-26, 20), "Convex Hull (Andrew)"),
               make_tikz_draw_style("black"), Tikz_Plane::Layer_Overlay);
  put_in_plane(plane, Text(Point(11, -16), "n = 14, h = " + std::to_string(hull.size())),
               make_tikz_draw_style("black"), Tikz_Plane::Layer_Overlay);

  out << "\\documentclass[tikz,border=8pt]{standalone}\n"
      << "\\usepackage{tikz}\n"
      << "\\begin{document}\n\n";
  plane.draw(out, true);
  out << "\n\\end{document}\n";

  std::cout << "Generated " << output_path << '\n'
            << "Compile with: pdflatex " << output_path << '\n';

  return 0;
}

