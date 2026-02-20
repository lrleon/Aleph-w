#include <fstream>
#include <iostream>
#include <string>

#include <tikzgeom_algorithms.H>

using namespace Aleph;

namespace
{

Polygon make_convex_a()
{
  Polygon p;
  p.add_vertex(Point(-48, -16));
  p.add_vertex(Point(-14, -22));
  p.add_vertex(Point(-3, 8));
  p.add_vertex(Point(-30, 22));
  p.close();
  return p;
}

Polygon make_convex_b()
{
  Polygon p;
  p.add_vertex(Point(-34, -24));
  p.add_vertex(Point(-2, -5));
  p.add_vertex(Point(-14, 24));
  p.add_vertex(Point(-43, 8));
  p.close();
  return p;
}

Polygon make_concave_a()
{
  Polygon p;
  p.add_vertex(Point(12, -18));
  p.add_vertex(Point(54, -18));
  p.add_vertex(Point(58, 8));
  p.add_vertex(Point(42, 2));
  p.add_vertex(Point(28, 20));
  p.add_vertex(Point(10, 10));
  p.close();
  return p;
}

Polygon make_concave_b()
{
  Polygon p;
  p.add_vertex(Point(24, -26));
  p.add_vertex(Point(62, -8));
  p.add_vertex(Point(56, 26));
  p.add_vertex(Point(32, 16));
  p.add_vertex(Point(18, 26));
  p.add_vertex(Point(8, -4));
  p.close();
  return p;
}

} // namespace

int main(int argc, char * argv[])
{
  const std::string output_path =
      argc > 1 ? argv[1] : "tikz_intersection_example.tex";

  std::ofstream out(output_path);
  if (not out)
    {
      std::cerr << "Cannot open output file: " << output_path << '\n';
      return 1;
    }

  Tikz_Plane plane(210, 120, 6, 6);
  plane.put_cartesian_axis();
  plane.set_point_radius_mm(0.8);

  const Polygon convex_a = make_convex_a();
  const Polygon convex_b = make_convex_b();
  const Polygon concave_a = make_concave_a();
  const Polygon concave_b = make_concave_b();

  // Left: convex-convex exact intersection.
  visualize_convex_intersection(plane, convex_a, convex_b);
  put_in_plane(plane, Text(Point(-46, 28), "Convex Intersection"),
               make_tikz_draw_style("black"), Tikz_Plane::Layer_Overlay);

  // Right: boolean intersection for general simple polygons.
  visualize_boolean_operation(plane, concave_a, concave_b,
                              BooleanPolygonOperations::Op::INTERSECTION);
  put_in_plane(plane, Text(Point(14, 31), "Boolean Intersection"),
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

