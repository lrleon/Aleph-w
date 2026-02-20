#include <fstream>
#include <iostream>
#include <string>

#include <tikzgeom_algorithms.H>

using namespace Aleph;

namespace
{

DynList<Point> make_points()
{
  DynList<Point> pts;
  pts.append(Point(-12, -8));
  pts.append(Point(-8, 10));
  pts.append(Point(0, -12));
  pts.append(Point(8, 9));
  pts.append(Point(14, -4));
  pts.append(Point(2, 16));
  pts.append(Point(0, 0));
  return pts;
}

Polygon make_convex_p()
{
  Polygon p;
  p.add_vertex(Point(-6, -3));
  p.add_vertex(Point(4, -3));
  p.add_vertex(Point(2, 5));
  p.close();
  return p;
}

Polygon make_convex_q()
{
  Polygon q;
  q.add_vertex(Point(-4, -2));
  q.add_vertex(Point(3, -2));
  q.add_vertex(Point(0, 4));
  q.close();
  return q;
}

Array<HalfPlaneIntersection::HalfPlane> make_hps()
{
  using HP = HalfPlaneIntersection::HalfPlane;
  Array<HP> hps;
  hps.append(HP(Point(0, 1), Point(0, 0)));   // x >= 0
  hps.append(HP(Point(0, 0), Point(1, 0)));   // y >= 0
  hps.append(HP(Point(8, 0), Point(8, 1)));   // x <= 8
  hps.append(HP(Point(1, 8), Point(0, 8)));   // y <= 8
  hps.append(HP(Point(8, 0), Point(0, 8)));   // x + y <= 8
  return hps;
}

Array<Segment> make_segments()
{
  Array<Segment> segs;
  segs.append(Segment(Point(-14, -8), Point(14, 10)));
  segs.append(Segment(Point(-12, 11), Point(13, -8)));
  segs.append(Segment(Point(-14, 2), Point(14, 2)));
  return segs;
}

} // namespace

int main(int argc, char * argv[])
{
  const std::string output_path =
      argc > 1 ? argv[1] : "tikz_missing_primitives_example.tex";

  std::ofstream out(output_path);
  if (not out)
    {
      std::cerr << "Cannot open output file: " << output_path << '\n';
      return 1;
    }

  Tikz_Plane delaunay_plane(180, 105, 5, 5);
  delaunay_plane.put_cartesian_axis();
  delaunay_plane.set_point_radius_mm(0.65);
  const auto dt = visualize_delaunay(delaunay_plane, make_points());
  put_in_plane(delaunay_plane,
               Text(Point(-14, 19),
                    "Delaunay sites=" + std::to_string(dt.sites.size()) +
                    ", triangles=" + std::to_string(dt.triangles.size())),
               make_tikz_draw_style("black"),
               Tikz_Plane::Layer_Overlay);

  Tikz_Plane minkowski_plane(180, 105, 5, 5);
  minkowski_plane.put_cartesian_axis();
  const Polygon sum = visualize_minkowski_sum(
      minkowski_plane, make_convex_p(), make_convex_q());
  put_in_plane(minkowski_plane,
               Text(Point(-8, 10),
                    "Minkowski vertices=" + std::to_string(sum.size())),
               make_tikz_draw_style("black"),
               Tikz_Plane::Layer_Overlay);

  Tikz_Plane halfplane_plane(180, 105, 5, 5);
  halfplane_plane.put_cartesian_axis();
  const Polygon feasible = visualize_half_plane_intersection(
      halfplane_plane, make_hps());
  put_in_plane(halfplane_plane,
               Text(Point(-1, 10),
                    "Half-plane intersection vertices=" +
                    std::to_string(feasible.size())),
               make_tikz_draw_style("black"),
               Tikz_Plane::Layer_Overlay);

  Tikz_Plane sweep_plane(180, 105, 5, 5);
  sweep_plane.put_cartesian_axis();
  sweep_plane.set_point_radius_mm(0.7);
  const auto intersections = visualize_line_sweep(
      sweep_plane, make_segments());
  put_in_plane(sweep_plane,
               Text(Point(-15, 13),
                    "Line sweep intersections=" +
                    std::to_string(intersections.size())),
               make_tikz_draw_style("black"),
               Tikz_Plane::Layer_Overlay);

  out << "\\documentclass[tikz,border=8pt]{standalone}\n"
      << "\\usepackage{tikz}\n"
      << "\\begin{document}\n\n";

  delaunay_plane.draw(out, true);
  out << "\n\\vspace{4mm}\n\n";
  minkowski_plane.draw(out, true);
  out << "\n\\vspace{4mm}\n\n";
  halfplane_plane.draw(out, true);
  out << "\n\\vspace{4mm}\n\n";
  sweep_plane.draw(out, true);
  out << "\n\\end{document}\n";

  std::cout << "Generated " << output_path << '\n';
  std::cout << "Compile with: pdflatex " << output_path << '\n';
  return 0;
}
