# Geometry Module Guide

## Scope

The geometry codebase is organized roughly as:

- **Primitives**: basic geometric types (`Point`, `Segment`, `Triangle`, `Rectangle`, `Ellipse`, `Polygon`).
- **Predicates**: robust orientation / containment / intersection tests built on the primitives.
- **Algorithms**: higher-level routines built on predicates (triangulations, hulls, diagrams, visibility, shortest paths, spatial indices).

Most of the higher-level algorithms live in `geom_algorithms.H`.

## Numeric model: exact vs approximate

### Exact arithmetic (default)

The geometry module uses `Geom_Number` (GMP rationals) for the core predicates and constructions.

- **Pros**
  - Deterministic.
  - Robust orientation / in-circle / intersection decisions.
  - Avoids epsilon tuning.

- **Cons**
  - Slower than `double`.
  - Larger memory footprint.

### Approximate arithmetic

Some helper functions convert to/from `double`/`mpfr_class` for transcendental functions (`sin`, `cos`, `sqrt`, `hypot`).

Use approximate variants when:

- You need speed and can tolerate floating-point error.
- Your inputs are already floating-point and you are not near degeneracies.

## Choosing algorithm variants

### Triangulation

- **Cutting ears**: simplest, O(n²), good for moderate n.
- **Monotone**: preferred when you need better asymptotics and have the decomposition.

### Delaunay / Voronoi

- `DelaunayTriangulationRandomizedIncremental`: expected O(n log n), robust with exact predicates.
- `VoronoiDiagram`: built as the dual of Delaunay. Not a Fortune sweep-line.

### Weighted diagrams

- `RegularTriangulationBowyerWatson` / `PowerDiagram`: needed when site weights are non-uniform.

### Visibility / shortest path

- `VisibilityPolygon`: rotational sweep (exact angular comparisons).
- `ShortestPathInPolygon`: triangulation + sleeve + funnel.

## Precondition checklist

Many algorithms assume:

- **Simple polygons** (no self-intersections).
- **Closed polygons** where required.
- **Non-degenerate inputs** (e.g., no collinear triangle vertices).

When an algorithm cannot represent all outputs (e.g. results with holes), it may return a reduced representation (documented in the class comment).

## Visualization stack

Aleph-w currently has two LaTeX-oriented geometry visualization backends:

- `eepicgeom.H`: classic EEPIC output (`Eepic_Plane`).
- `tikzgeom.H`: PGF/TikZ output (`Tikz_Plane`) with style and layer support.
- `tikzgeom_scene.H`: scene-level composition (`Tikz_Scene`) to combine multiple visualizations in one export (standalone/beamer/handout).

For algorithm-level visual output, use:

- `tikzgeom_algorithms.H`: convenience helpers to draw convex hull, polygon intersections, Delaunay/Voronoi overlays, and power diagrams.
- Additional helpers now include:
  - `visualize_segment_arrangement(...)`
  - `visualize_shortest_path_in_polygon(...)`
  - `visualize_shortest_path_with_portals(...)`
  - `compute_shortest_path_funnel_trace(...)` + `put_funnel_trace_step(...)`
  - `visualize_convex_decomposition(...)`
  - `visualize_alpha_shape(...)`

## Interface stability for more complex objects

The current TikZ API is intentionally layered:

- **Object layer** (`tikzgeom.H`): `put_in_plane(plane, object[, style, layer])`
- **Algorithm layer** (`tikzgeom_algorithms.H`): `visualize_*` helpers returning the algorithm result and drawing it.
- **Scene layer** (`tikzgeom_scene.H`): `scene.add(...)`, `scene.visualize_*`, and `scene.draw_standalone(...)`/`scene.draw_beamer(...)`/`scene.draw_handout(...)`, plus multi-step decks via `Tikz_Scene::draw_beamer_overlays(...)` and `draw_handout_overlays(...)`.

This keeps the interface stable even when complexity grows:

- Adding a new primitive usually means adding a new `put_in_plane` overload.
- Adding a new algorithm visualization usually means adding a new `visualize_*` helper that composes existing primitives.
- Existing user code that only calls `put_in_plane(plane, obj)` remains valid.

See `docs/TIKZGEOM_GUIDE.md` for full usage examples and extension guidelines.
