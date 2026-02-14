#include <fstream>
#include <iostream>
#include <string>

#include <tikzgeom_algorithms.H>

using namespace Aleph;

int main(int argc, char * argv[])
{
  const std::string output_path =
      argc > 1 ? argv[1] : "tikz_voronoi_power_example.tex";

  std::ofstream out(output_path);
  if (not out)
    {
      std::cerr << "Cannot open output file: " << output_path << '\n';
      return 1;
    }

  DynList<Point> sites;
  sites.append(Point(-24, -10));
  sites.append(Point(-8, 16));
  sites.append(Point(6, -14));
  sites.append(Point(20, 8));
  sites.append(Point(32, -4));
  sites.append(Point(12, 22));

  // Figure 1: Delaunay + Voronoi.
  Tikz_Plane vor_plane(210, 120, 6, 6);
  vor_plane.put_cartesian_axis();
  vor_plane.set_point_radius_mm(0.75);

  DelaunayTriangulationRandomizedIncremental dt_algo;
  const auto dt = dt_algo(sites);
  put_delaunay_result(vor_plane, dt,
                      tikz_wire_style("blue"),
                      false);

  visualize_voronoi(
      vor_plane, sites, VoronoiDiagram(),
      false,
      tikz_area_style("gray!50!black", "gray!15", 0.25),
      tikz_wire_style("black"),
      tikz_wire_style("black", true, true),
      tikz_points_style("red"),
      Geom_Number(65));

  put_in_plane(vor_plane, Text(Point(-26, 26), "Voronoi + Delaunay Overlay"),
               make_tikz_draw_style("black"), Tikz_Plane::Layer_Overlay);

  // Figure 2: Power diagram.
  Array<PowerDiagram::WeightedSite> weighted;
  weighted.append({Point(-22, -8), Geom_Number(2)});
  weighted.append({Point(-7, 15), Geom_Number(9)});
  weighted.append({Point(8, -11), Geom_Number(1)});
  weighted.append({Point(22, 10), Geom_Number(5)});
  weighted.append({Point(34, -3), Geom_Number(7)});
  weighted.append({Point(10, 21), Geom_Number(3)});

  Tikz_Plane power_plane(210, 120, 6, 6);
  power_plane.put_cartesian_axis();
  power_plane.set_point_radius_mm(0.75);

  visualize_power_diagram(
      power_plane, weighted, PowerDiagram(),
      true,
      tikz_area_style("violet", "violet!18", 0.35),
      tikz_wire_style("violet"),
      tikz_points_style("purple"));

  put_in_plane(power_plane, Text(Point(-24, 26), "Power Diagram (Weighted Voronoi)"),
               make_tikz_draw_style("black"), Tikz_Plane::Layer_Overlay);

  out << "\\documentclass[tikz,border=8pt]{standalone}\n"
      << "\\usepackage{tikz}\n"
      << "\\begin{document}\n\n";
  vor_plane.draw(out, true);
  out << "\n\\vspace{5mm}\n\n";
  power_plane.draw(out, true);
  out << "\n\\end{document}\n";

  std::cout << "Generated " << output_path << '\n'
            << "Compile with: pdflatex " << output_path << '\n';

  return 0;
}

