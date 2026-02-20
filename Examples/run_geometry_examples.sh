#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-$ROOT_DIR/build}"

TARGETS=(
  geom_example
  robust_predicates_example
  delaunay_voronoi_example
  point_in_polygon_example
  polygon_intersection_example
  halfplane_intersection_example
  convex_hull_comparison_example
  closest_pair_example
  rotating_calipers_example
  voronoi_clipped_cells_example
)

echo "[geometry-examples] root:  $ROOT_DIR"
echo "[geometry-examples] build: $BUILD_DIR"

if [[ ! -d "$BUILD_DIR" ]]; then
  echo "[geometry-examples] build directory not found; configuring..."
  cmake -S "$ROOT_DIR" -B "$BUILD_DIR"
fi

echo "[geometry-examples] building targets..."
cmake --build "$BUILD_DIR" --target "${TARGETS[@]}"

run_cmd() {
  echo
  echo ">>> $*"
  "$@"
}

run_cmd "$BUILD_DIR/Examples/geom_example" -s advanced
run_cmd "$BUILD_DIR/Examples/robust_predicates_example"
run_cmd "$BUILD_DIR/Examples/delaunay_voronoi_example"
run_cmd "$BUILD_DIR/Examples/point_in_polygon_example"
run_cmd "$BUILD_DIR/Examples/polygon_intersection_example"
run_cmd "$BUILD_DIR/Examples/halfplane_intersection_example"
run_cmd "$BUILD_DIR/Examples/convex_hull_comparison_example"
run_cmd "$BUILD_DIR/Examples/closest_pair_example"
run_cmd "$BUILD_DIR/Examples/rotating_calipers_example"
run_cmd "$BUILD_DIR/Examples/voronoi_clipped_cells_example"

echo
echo "[geometry-examples] all examples finished successfully"
