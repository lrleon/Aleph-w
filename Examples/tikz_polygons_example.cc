#include <fstream>
#include <iostream>
#include <string>

#include <tikzgeom.H>

using namespace Aleph;

namespace
{

Polygon make_convex_polygon()
{
  Polygon poly;
  poly.add_vertex(Point(-40, -20));
  poly.add_vertex(Point(-12, -26));
  poly.add_vertex(Point(8, -6));
  poly.add_vertex(Point(-4, 20));
  poly.add_vertex(Point(-34, 16));
  poly.close();
  return poly;
}

Polygon make_concave_polygon()
{
  Polygon poly;
  poly.add_vertex(Point(18, -20));
  poly.add_vertex(Point(58, -20));
  poly.add_vertex(Point(62, 2));
  poly.add_vertex(Point(41, -4));
  poly.add_vertex(Point(30, 16));
  poly.add_vertex(Point(16, 6));
  poly.close();
  return poly;
}

void add_polygon_vertices(Tikz_Plane & plane, const Polygon & poly)
{
  for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
    put_in_plane(plane, it.get_current_vertex().to_point());
}

void add_regular_polygon_vertices(Tikz_Plane & plane, const Regular_Polygon & poly)
{
  for (size_t i = 0; i < poly.size(); ++i)
    put_in_plane(plane, poly.get_vertex(i));
}

} // namespace

int main(int argc, char * argv[])
{
  const std::string output_path =
      argc > 1 ? argv[1] : "tikz_polygons_example.tex";

  std::ofstream out(output_path);
  if (not out)
    {
      std::cerr << "Cannot open output file: " << output_path << '\n';
      return 1;
    }

  Tikz_Plane plane(190, 120, 5, 5);
  plane.put_cartesian_axis();
  plane.set_point_radius_mm(0.9);

  const Polygon convex = make_convex_polygon();
  const Polygon concave = make_concave_polygon();
  const Regular_Polygon regular(Point(96, 6), 17.0, 7, 0.2);

  put_in_plane(plane, convex);
  put_in_plane(plane, concave);
  put_in_plane(plane, regular);

  add_polygon_vertices(plane, convex);
  add_polygon_vertices(plane, concave);
  add_regular_polygon_vertices(plane, regular);

  put_in_plane(plane, Text(Point(-24, 27), "Convexo"));
  put_in_plane(plane, Text(Point(33, 24), "Concavo"));
  put_in_plane(plane, Text(Point(96, 30), "Regular (7 lados)"));

  out << "\\documentclass[tikz,border=8pt]{standalone}\n"
      << "\\usepackage{tikz}\n"
      << "\\begin{document}\n\n";

  plane.draw(out, true);

  out << "\n\\end{document}\n";

  std::cout << "Generated " << output_path << '\n'
            << "Compile with: pdflatex " << output_path << '\n';

  return 0;
}

