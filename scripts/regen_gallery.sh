#!/usr/bin/env bash
# Phase 28 — regenerate the CA visual gallery under docs/gallery/.
#
# Builds the `ca_gallery_gen` example target and runs it. The output is
# deterministic (fixed seeds), so re-running on the same toolchain produces
# byte-identical PNGs.
#
# Usage:
#   scripts/regen_gallery.sh [build-dir] [output-dir]
#
# Defaults: build-dir=build, output-dir=docs/gallery
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-${REPO_ROOT}/build}"
OUT_DIR="${2:-${REPO_ROOT}/docs/gallery}"

cd "${REPO_ROOT}"

if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  echo "Configuring ${BUILD_DIR} ..."
  cmake -S . -B "${BUILD_DIR}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=OFF \
    -DBUILD_EXAMPLES=ON \
    -DALEPH_BUILD_X11_VIEWER=OFF
fi

echo "Building ca_gallery_gen ..."
cmake --build "${BUILD_DIR}" --target ca_gallery_gen

BIN="$(find "${BUILD_DIR}" -name ca_gallery_gen -type f -perm -u+x | head -1)"
if [[ -z "${BIN}" ]]; then
  echo "error: ca_gallery_gen binary not found under ${BUILD_DIR}" >&2
  exit 1
fi

mkdir -p "${OUT_DIR}"
echo "Rendering into ${OUT_DIR} ..."
"${BIN}" "${OUT_DIR}"

echo "Gallery regenerated. $(ls "${OUT_DIR}"/*.png | wc -l) PNG(s) under ${OUT_DIR}."