# TikZ Geometry Guide

## Scope

This guide covers the new TikZ visualization API for computational geometry:

- `tikzgeom.H`: object-level drawing (`Tikz_Plane`, styles, layers).
- `tikzgeom_algorithms.H`: algorithm-level helpers (`visualize_*`, `put_*`).
- `tikzgeom_scene.H`: scene-level composition (`Tikz_Scene`) with unified export (standalone, beamer, handout).

The output format is LaTeX PGF/TikZ (`tikzpicture`), suitable for papers, slides, and reports.

## Core interface

### Plane

```cpp
#include <tikzgeom.H>

Tikz_Plane plane(180, 110, 6, 6); // width mm, height mm, x/y offsets mm
plane.put_cartesian_axis();
plane.set_point_radius_mm(0.9);
```

### Insert objects

```cpp
put_in_plane(plane, Point(0, 0));
put_in_plane(plane, Segment(Point(0, 0), Point(10, 20)));
put_in_plane(plane, Polygon(...));
```

Supported objects in `Tikz_Plane`:

- `Point`, `Polar_Point`
- `Segment`, `Triangle`
- `Ellipse`, `RotatedEllipse`
- `Text`
- `Polygon`, `Regular_Polygon`

### Draw output

```cpp
std::ofstream out("figure.tex");
plane.draw(out, true); // squarize=true keeps aspect ratio
```

`draw()` emits a full `tikzpicture` with clipping and mapped coordinates.

## Scene interface

`tikzgeom_scene.H` adds `Tikz_Scene`, a high-level wrapper around `Tikz_Plane`.

Use it when a single figure combines:

- manual overlays (`Point`, `Segment`, labels),
- multiple algorithm outputs (`visualize_*`), and
- one export entrypoint (`draw`, `to_tikz`, `draw_standalone`, `to_standalone`, `draw_beamer`, `draw_handout`, `draw_beamer_overlays`, `draw_handout_overlays`).

```cpp
#include <tikzgeom_scene.H>

Tikz_Scene scene(220, 130, 6, 6); // mm
scene.put_cartesian_axis().set_point_radius_mm(0.7);

auto arr = scene.visualize_segment_arrangement(segments, SegmentArrangement());
Polygon hull = scene.visualize_convex_hull(arr.vertices, AndrewMonotonicChainConvexHull());

scene.add(Text(Point(-20, 22), "Scene composition"), make_tikz_draw_style("black"),
          Tikz_Plane::Layer_Overlay);
scene.draw_beamer(std::cout);
// or: scene.draw_handout(std::cout);
```

Overlay deck export (`\\only<k>` in one frame):

```cpp
std::vector<Tikz_Scene> steps;
steps.push_back(step0);
steps.push_back(step1);
steps.push_back(step2);

Tikz_Beamer_Document_Options opts;
opts.frame_title = "Algorithm Steps";

Tikz_Scene::draw_beamer_overlays(std::cout, steps, opts);
// or: Tikz_Scene::draw_handout_overlays(std::cout, steps, opts);
```

Reference implementation: `Examples/tikz_funnel_beamer_example.cc`.

For algorithms not yet wrapped as methods, use:

```cpp
auto result = scene.run([&](Tikz_Plane & plane) {
  return visualize_alpha_shape(plane, points, alpha2, AlphaShape());
});
```

## Styles and layers

### Style model (`Tikz_Style`)

Important fields:

- colors: `draw_color`, `fill_color`, `text_color`
- line flags: `dashed`, `dotted`, `thick`, `with_arrow`
- fill flag: `fill`
- numeric: `line_width_mm`, `opacity`
- raw TikZ options: `extra_options`
- text font command: `text_font_command`

Helpers:

- `make_tikz_draw_style(color)`
- `make_tikz_fill_style(draw, fill)`

### Styled insert

```cpp
Tikz_Style hull = tikz_wire_style("red");
hull.thick = true;
put_in_plane(plane, hull_polygon, hull, Tikz_Plane::Layer_Foreground);
```

Layer constants:

- `Layer_Background`
- `Layer_Default`
- `Layer_Foreground`
- `Layer_Overlay`

Objects are rendered sorted by `(layer, insertion_order)`.

## Algorithm visualization helpers

`tikzgeom_algorithms.H` provides two categories:

- collection helpers: `put_points`, `put_polygons`, `put_polygon_vertices`
- algorithm helpers: `visualize_convex_hull`, `visualize_convex_intersection`,
  `visualize_boolean_operation`, `visualize_voronoi`, `visualize_power_diagram`,
  `visualize_segment_arrangement`, `visualize_shortest_path_in_polygon`,
  `visualize_shortest_path_with_portals`, `visualize_convex_decomposition`,
  `compute_shortest_path_funnel_trace`, `put_funnel_trace_step`,
  `visualize_alpha_shape`, plus `put_delaunay_result`, `put_voronoi_result`,
  `put_power_diagram_result`, `put_segment_arrangement_result`,
  `put_alpha_shape_result`, `put_path`, and `put_portals`

Example:

```cpp
#include <tikzgeom_algorithms.H>

AndrewMonotonicChainConvexHull hull_algo;
Polygon hull = visualize_convex_hull(plane, points, hull_algo);
```

All `visualize_*` functions return the computed algorithm result while also drawing it.

## Current examples

The following examples generate standalone `.tex` files:

- `Examples/tikz_polygons_example.cc`
- `Examples/tikz_convex_hull_example.cc`
- `Examples/tikz_intersection_example.cc`
- `Examples/tikz_voronoi_power_example.cc`
- `Examples/tikz_advanced_algorithms_example.cc`
- `Examples/tikz_funnel_animation_example.cc`
- `Examples/tikz_funnel_beamer_example.cc`
- `Examples/tikz_funnel_beamer_twocol_example.cc`
- `Examples/tikz_funnel_beamer_handout_example.cc`
- `Examples/tikz_scene_example.cc`
- `Examples/tikz_scene_beamer_example.cc`
- `Examples/tikz_scene_overlays_example.cc`

Build and run:

```bash
cmake -S . -B build -DBUILD_EXAMPLES=ON
cmake --build build --target \
  tikz_polygons_example tikz_convex_hull_example \
  tikz_intersection_example tikz_voronoi_power_example \
  tikz_advanced_algorithms_example tikz_funnel_animation_example \
  tikz_funnel_beamer_example tikz_funnel_beamer_twocol_example \
  tikz_funnel_beamer_handout_example tikz_scene_example \
  tikz_scene_beamer_example tikz_scene_overlays_example

./build/Examples/tikz_polygons_example /tmp/tikz_polygons_example.tex
./build/Examples/tikz_advanced_algorithms_example /tmp/tikz_advanced_algorithms_example.tex
./build/Examples/tikz_funnel_animation_example /tmp/tikz_funnel_animation_example.tex
./build/Examples/tikz_funnel_beamer_example /tmp/tikz_funnel_beamer_example.tex
./build/Examples/tikz_funnel_beamer_twocol_example /tmp/tikz_funnel_beamer_twocol_example.tex
./build/Examples/tikz_funnel_beamer_handout_example /tmp/tikz_funnel_beamer_handout_example.tex
./build/Examples/tikz_scene_example /tmp/tikz_scene_example.tex
./build/Examples/tikz_scene_beamer_example /tmp/tikz_scene_beamer_example.tex
./build/Examples/tikz_scene_beamer_example /tmp/tikz_scene_handout_example.tex --handout
./build/Examples/tikz_scene_overlays_example /tmp/tikz_scene_overlays_example.tex
./build/Examples/tikz_scene_overlays_example /tmp/tikz_scene_overlays_handout.tex --handout
```

## How interface scales to more complex objects

The API can stay stable as complexity grows:

- New primitive object:
  - Add it to `Tikz_Plane::Object` variant.
  - Implement box/mapping extents.
  - Implement draw routine.
  - Add `put_in_plane` overload(s).

- New algorithm visualization (convex decomposition, arrangement, shortest path, etc):
  - Keep algorithm implementation in `geom_algorithms.H`.
  - Add one helper in `tikzgeom_algorithms.H` that converts result data to primitives.
  - Return the original result type and draw with styles/layers.

- New scene-level workflow (multi-algorithm figures):
  - Keep manual geometry in `Tikz_Scene::add(...)`.
  - Keep algorithm calls as `scene.visualize_*` wrappers.
  - For non-wrapped algorithms, use `scene.run(lambda_with_plane)`.
  - Export once with `scene.draw_standalone(...)`, `scene.draw_beamer(...)`, `scene.draw_handout(...)`, or scene vectors via `Tikz_Scene::draw_beamer_overlays(...)` / `draw_handout_overlays(...)`.

This preserves the same user mental model:

1. Compute or call `visualize_*`.
2. Optionally add overlays (points, labels, axes).
3. Export with `scene.draw_*` (`draw_standalone`, `draw_beamer`, `draw_handout`, overlays) or raw `plane.draw(...)`.
