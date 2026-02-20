#include <fstream>
#include <iostream>
#include <string>

#include <tikzgeom_algorithms.H>

using namespace Aleph;

namespace
{

Array<Point> make_kd_points()
{
  Array<Point> pts;
  pts.append(Point(10, 10));
  pts.append(Point(20, 30));
  pts.append(Point(35, 12));
  pts.append(Point(48, 42));
  pts.append(Point(60, 18));
  pts.append(Point(78, 36));
  pts.append(Point(88, 8));
  return pts;
}

DynList<Point> make_range_points()
{
  DynList<Point> pts;
  pts.append(Point(2, 2));
  pts.append(Point(6, 7));
  pts.append(Point(9, 3));
  pts.append(Point(13, 8));
  pts.append(Point(16, 1));
  pts.append(Point(19, 6));
  return pts;
}

AABBTree make_aabb_tree()
{
  AABBTree tree;
  Array<AABBTree::Entry> entries;
  entries.append({Rectangle(0, 0, 4, 4), 0});
  entries.append({Rectangle(3, 2, 9, 7), 1});
  entries.append({Rectangle(11, 1, 15, 5), 2});
  entries.append({Rectangle(14, 4, 19, 9), 3});
  entries.append({Rectangle(6, 8, 10, 12), 4});
  tree.build(entries);
  return tree;
}

} // namespace

int main(int argc, char * argv[])
{
  const std::string output_path =
      argc > 1 ? argv[1] : "tikz_tree_structures_example.tex";

  std::ofstream out(output_path);
  if (not out)
    {
      std::cerr << "Cannot open output file: " << output_path << '\n';
      return 1;
    }

  Tikz_Plane kd_plane(200, 120, 6, 6);
  kd_plane.put_cartesian_axis();
  kd_plane.put_coordinate_grid(5, 5, true);
  kd_plane.enable_auto_legend(true);
  kd_plane.set_point_radius_mm(0.65);

  const auto kd = KDTreePointSearch::build(make_kd_points(), 0, 0, 100, 50);
  const auto kd_snap = visualize_kdtree_partitions(kd_plane, kd, true, true);
  put_in_plane(kd_plane,
               Text(Point(-2, 52),
                    "KD-tree partitions=" + std::to_string(kd_snap.partitions.size())),
               make_tikz_draw_style("black"),
               Tikz_Plane::Layer_Overlay);

  Tikz_Plane range_plane(200, 120, 6, 6);
  range_plane.put_cartesian_axis();
  range_plane.put_coordinate_grid(2, 2, true);
  range_plane.enable_auto_legend(true);
  range_plane.set_point_radius_mm(0.75);

  RangeTree2D range_tree;
  range_tree.build(make_range_points());
  const Rectangle range_query(5, 2, 15, 7);
  const auto rv = visualize_range_tree_query(range_plane, range_tree, range_query);
  put_in_plane(range_plane,
               Text(Point(0, 10),
                    "Range-tree nodes=" + std::to_string(rv.snapshot.nodes.size()) +
                    ", hits=" + std::to_string(rv.query_hits.size())),
               make_tikz_draw_style("black"),
               Tikz_Plane::Layer_Overlay);

  Tikz_Plane aabb_plane(200, 120, 6, 6);
  aabb_plane.put_cartesian_axis();
  aabb_plane.put_coordinate_grid(2, 2, true);
  aabb_plane.enable_auto_legend(true);

  const AABBTree aabb_tree = make_aabb_tree();
  const Rectangle aabb_query(2, 1, 8, 6);
  const auto av = visualize_aabb_tree_query(aabb_plane, aabb_tree, aabb_query);
  put_in_plane(aabb_plane,
               Text(Point(-1, 13),
                    "AABB nodes=" + std::to_string(av.snapshot.nodes.size()) +
                    ", hits=" + std::to_string(av.query_hit_ids.size())),
               make_tikz_draw_style("black"),
               Tikz_Plane::Layer_Overlay);

  out << "\\documentclass[tikz,border=8pt]{standalone}\n"
      << "\\usepackage{tikz}\n"
      << "\\begin{document}\n\n";

  kd_plane.draw(out, true);
  out << "\n\\vspace{4mm}\n\n";
  range_plane.draw(out, true);
  out << "\n\\vspace{4mm}\n\n";
  aabb_plane.draw(out, true);
  out << "\n\\end{document}\n";

  std::cout << "Generated " << output_path << '\n';
  std::cout << "Compile with: pdflatex " << output_path << '\n';
  return 0;
}
