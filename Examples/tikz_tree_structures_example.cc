#include <fstream>
#include <iostream>
#include <string>

#include <quadtree.H>
#include <tikzgeom_algorithms.H>
#include <tpl_r_star_tree.H>

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

// QuadTree exposes its node structure through public accessors
// (get_root(), QuadNode::get_nw_child()/.../get_se_child(), is_leaf(),
// get_points_set()) but has no built-in TikZ visualizer, so this example
// walks it directly to collect leaf regions (by depth) and stored points.
struct QuadTreeSnapshot
{
  Array<Rectangle> leaf_regions;
  Array<size_t> leaf_depths;
  Array<Point> points;
};

void collect_quadtree_rec(QuadNode * node, const size_t depth, QuadTreeSnapshot & snap)
{
  if (node == nullptr)
    return;

  if (node->is_leaf())
    {
      snap.leaf_regions.append(Rectangle(node->get_min_x(), node->get_min_y(),
                                         node->get_max_x(), node->get_max_y()));
      snap.leaf_depths.append(depth);
      for (Point & p : node->get_points_set())
        snap.points.append(p);
      return;
    }

  collect_quadtree_rec(node->get_nw_child(), depth + 1, snap);
  collect_quadtree_rec(node->get_ne_child(), depth + 1, snap);
  collect_quadtree_rec(node->get_sw_child(), depth + 1, snap);
  collect_quadtree_rec(node->get_se_child(), depth + 1, snap);
}

QuadTreeSnapshot collect_quadtree(QuadTree & tree)
{
  QuadTreeSnapshot snap;
  collect_quadtree_rec(tree.get_root(), 0, snap);
  return snap;
}

// Colors cycle by subdivision depth so sibling quadrants at the same depth
// share a color and successive splits are visually distinguishable.
std::string depth_color(const size_t depth)
{
  static const char * colors[] =
      {"blue!55", "orange!70!black", "green!55!black", "red!60", "violet!65", "brown!60"};
  constexpr size_t num_colors = sizeof(colors) / sizeof(colors[0]);
  return colors[depth % num_colors];
}

Array<Point> make_quadtree_points()
{
  // Deliberately uneven spread (a dense cluster plus scattered outliers) so
  // the quadtree subdivides non-uniformly across depths. All coordinates
  // stay inside [0, 100) since QuadNode::contains() uses a half-open
  // [min, max) region: a point exactly on the upper/right root boundary
  // would silently fail to insert.
  Array<Point> pts;
  pts.append(Point(12, 15));
  pts.append(Point(18, 22));
  pts.append(Point(15, 30));
  pts.append(Point(22, 12));
  pts.append(Point(28, 28));
  pts.append(Point(20, 20));
  pts.append(Point(70, 75));
  pts.append(Point(80, 65));
  pts.append(Point(75, 82));
  pts.append(Point(85, 88));
  pts.append(Point(60, 15));
  pts.append(Point(90, 20));
  pts.append(Point(45, 60));
  pts.append(Point(35, 85));
  pts.append(Point(8, 92));
  pts.append(Point(95, 5));
  return pts;
}

Array<Rectangle> make_rtree_rectangles()
{
  Array<Rectangle> rects;
  for (int i = 0; i < 24; ++i)
    {
      const int col = i % 6;
      const int row = i / 6;
      const Geom_Number x0 = Geom_Number(col * 10 + (i % 3));
      const Geom_Number y0 = Geom_Number(row * 9 + (i % 2) * 2);
      rects.append(Rectangle(x0, y0, x0 + 6, y0 + 5));
    }
  return rects;
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

  // QuadTree: no dedicated visualizer exists yet (unlike KD/Range/AABB
  // above), so leaf regions are collected by walking the public node API
  // directly (see collect_quadtree_rec) and drawn colored by subdivision
  // depth, one wireframe rectangle per leaf.
  Tikz_Plane quadtree_plane(200, 120, 6, 6);
  quadtree_plane.put_cartesian_axis();
  quadtree_plane.put_coordinate_grid(10, 10, true);
  quadtree_plane.set_point_radius_mm(0.7);

  QuadTree quadtree(Geom_Number(0), Geom_Number(100), Geom_Number(0), Geom_Number(100), 3);
  for (const Point & p : make_quadtree_points())
    quadtree.insert(p);

  const QuadTreeSnapshot qsnap = collect_quadtree(quadtree);
  for (size_t i = 0; i < qsnap.leaf_regions.size(); ++i)
    put_in_plane(quadtree_plane, qsnap.leaf_regions(i),
                 tikz_wire_style(depth_color(qsnap.leaf_depths(i))),
                 Tikz_Plane::Layer_Default);
  put_points(quadtree_plane, qsnap.points, tikz_points_style("black"));
  put_in_plane(quadtree_plane,
               Text(Point(-2, 105),
                    "QuadTree leaves=" + std::to_string(qsnap.leaf_regions.size()) +
                    ", points=" + std::to_string(qsnap.points.size())),
               make_tikz_draw_style("black"),
               Tikz_Plane::Layer_Overlay);

  // R-tree (Guttman quadratic split) and R*-tree, built from the *same*
  // rectangles and queried with the same rectangle, so the two split
  // heuristics can be compared directly: R*-tree typically yields less
  // node overlap for the same data.
  Tikz_Plane rtree_plane(200, 120, 6, 6);
  rtree_plane.put_cartesian_axis();
  rtree_plane.put_coordinate_grid(10, 10, true);

  RTree<int, 4, 2> rtree;
  RStarTree<int, 4, 2> rstar_tree;
  {
    const Array<Rectangle> rects = make_rtree_rectangles();
    for (size_t i = 0; i < rects.size(); ++i)
      {
        rtree.insert(rects(i), static_cast<int>(i));
        rstar_tree.insert(rects(i), static_cast<int>(i));
      }
  }
  const Rectangle rtree_query(8, 5, 28, 20);
  const auto rt_result = visualize_rtree_query(rtree_plane, rtree, rtree_query);
  put_in_plane(rtree_plane,
               Text(Point(-2, 47),
                    "R-tree (Guttman) nodes=" + std::to_string(rt_result.snapshot.nodes.size()) +
                    ", hits=" + std::to_string(rt_result.query_hit_boxes.size())),
               make_tikz_draw_style("black"),
               Tikz_Plane::Layer_Overlay);

  Tikz_Plane rstar_plane(200, 120, 6, 6);
  rstar_plane.put_cartesian_axis();
  rstar_plane.put_coordinate_grid(10, 10, true);

  const auto rstar_result = visualize_rtree_query(rstar_plane, rstar_tree, rtree_query);
  put_in_plane(rstar_plane,
               Text(Point(-2, 47),
                    "R*-tree nodes=" + std::to_string(rstar_result.snapshot.nodes.size()) +
                    ", hits=" + std::to_string(rstar_result.query_hit_boxes.size())),
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
  out << "\n\\vspace{4mm}\n\n";
  quadtree_plane.draw(out, true);
  out << "\n\\vspace{4mm}\n\n";
  rtree_plane.draw(out, true);
  out << "\n\\vspace{4mm}\n\n";
  rstar_plane.draw(out, true);
  out << "\n\\end{document}\n";

  std::cout << "Generated " << output_path << '\n';
  std::cout << "Compile with: pdflatex " << output_path << '\n';
  return 0;
}