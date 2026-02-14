#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <tikzgeom_scene.H>

using namespace Aleph;

namespace
{

Polygon make_polygon_a()
{
  Polygon p;
  p.add_vertex(Point(-20, -8));
  p.add_vertex(Point(12, -12));
  p.add_vertex(Point(22, 6));
  p.add_vertex(Point(-6, 20));
  p.close();
  return p;
}

Polygon make_polygon_b()
{
  Polygon p;
  p.add_vertex(Point(-14, -18));
  p.add_vertex(Point(24, -4));
  p.add_vertex(Point(10, 24));
  p.add_vertex(Point(-24, 8));
  p.close();
  return p;
}

Tikz_Scene make_scene_canvas()
{
  Tikz_Scene scene(178, 108, 0, 0, true);
  scene.put_cartesian_axis()
      .set_point_radius_mm(0.70);
  return scene;
}

void put_step_title(Tikz_Scene & scene, const std::string & text)
{
  scene.add(Text(Point(-30, 28), text),
            make_tikz_draw_style("black"),
            Tikz_Plane::Layer_Overlay);
}

} // namespace

int main(int argc, char * argv[])
{
  std::string output_path = "tikz_scene_overlays_example.tex";
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

  const Polygon a = make_polygon_a();
  const Polygon b = make_polygon_b();

  std::vector<Tikz_Scene> steps;
  steps.reserve(3);

  {
    Tikz_Scene scene = make_scene_canvas();
    scene.add(a, tikz_area_style("blue", "blue!15", 0.42), Tikz_Plane::Layer_Default);
    scene.add(b, tikz_area_style("orange", "orange!20", 0.42), Tikz_Plane::Layer_Default + 1);
    put_step_title(scene, "Step 1/3: input polygons A and B");
    steps.push_back(scene);
  }

  {
    Tikz_Scene scene = make_scene_canvas();
    const Polygon inter = scene.visualize_convex_intersection(
        a,
        b,
        ConvexPolygonIntersectionBasic(),
        tikz_area_style("blue", "blue!15", 0.42),
        tikz_area_style("orange", "orange!20", 0.42),
        tikz_area_style("red", "red!35", 0.62));
    put_step_title(
        scene,
        "Step 2/3: convex intersection, vertices=" + std::to_string(inter.size()));
    steps.push_back(scene);
  }

  {
    Tikz_Scene scene = make_scene_canvas();
    const Array<Polygon> united = scene.visualize_boolean_operation(
        a,
        b,
        BooleanPolygonOperations::Op::UNION,
        BooleanPolygonOperations(),
        tikz_area_style("blue", "blue!10", 0.34),
        tikz_area_style("green!60!black", "green!15", 0.34),
        tikz_area_style("purple!70!black", "purple!30", 0.56));
    put_step_title(
        scene,
        "Step 3/3: boolean union, components=" + std::to_string(united.size()));
    steps.push_back(scene);
  }

  Tikz_Beamer_Document_Options opts;
  opts.class_options = "aspectratio=169";
  opts.frame_options = "t";
  opts.frame_title = handout ? "TikZ Scene Overlays (Handout)"
                             : "TikZ Scene Overlays (Beamer)";

  if (handout)
    Tikz_Scene::draw_handout_overlays(out, steps, opts);
  else
    Tikz_Scene::draw_beamer_overlays(out, steps, opts);

  std::cout << "Generated " << output_path << '\n'
            << "Mode: " << (handout ? "handout" : "beamer") << '\n'
            << "Overlays: " << steps.size() << '\n'
            << "Compile with: pdflatex " << output_path << '\n';

  return 0;
}
