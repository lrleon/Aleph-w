#!/usr/bin/env bash
#
# Checks Aleph-w as an external consumer sees it once installed.
#
# For each variant (a plain build and a build with sanitizers) it builds the
# library, installs it into a private prefix and runs consumer/package-check
# against that prefix: the identity of the package (release number, source
# revision, sanitizer configuration), the instrumentation and the MemArray
# annotations that a sanitizer package gives its consumers through
# Aleph::Aleph, and that every installed header compiles by itself with the
# installation as the only source of Aleph headers.
#
# Usage:
#   scripts/installed_package_check.sh [options]
#
# Options:
#   --work DIR          working directory (default: a temporary one, removed
#                       at the end; a directory given here is kept)
#   --variant NAME      plain, sanitizers or all (default: all)
#   --build-type TYPE   CMAKE_BUILD_TYPE of the library and of the consumer
#                       (default: Release)
#   --jobs N            parallel jobs (default: number of processors)
#   --skip-headers      do not compile every installed header
#   -h, --help          show this help
#
# CC, CXX and CMAKE_GENERATOR are honoured as CMake honours them. Nothing is
# installed outside the working directory and the network is not used.

set -euo pipefail

usage()
{
  sed -n '2,/^$/p' "$0" | sed 's/^# \{0,1\}//'
}

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work=""
variant="all"
build_type="Release"
jobs="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
check_headers="ON"

while [ $# -gt 0 ]; do
  case "$1" in
    --work) work="${2:?--work needs a directory}"; shift 2 ;;
    --variant) variant="${2:?--variant needs a name}"; shift 2 ;;
    --build-type) build_type="${2:?--build-type needs a value}"; shift 2 ;;
    --jobs) jobs="${2:?--jobs needs a number}"; shift 2 ;;
    --skip-headers) check_headers="OFF"; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

case "$variant" in
  plain) variants="plain" ;;
  sanitizers) variants="sanitizers" ;;
  all) variants="plain sanitizers" ;;
  *) echo "unknown variant: $variant (plain, sanitizers or all)" >&2; exit 2 ;;
esac

if [ -z "$work" ]; then
  work="$(mktemp -d "${TMPDIR:-/tmp}/aleph-package-check.XXXXXX")"
  trap 'rm -rf "$work"' EXIT
else
  mkdir -p "$work"
  work="$(cd "$work" && pwd)"
fi

check_variant()
{
  local name="$1"
  local build="$work/build-$name"
  local prefix="$work/prefix-$name"
  local check="$work/check-$name"
  local sanitizers="OFF"
  local headers="$check_headers"

  if [ "$name" = "sanitizers" ]; then
    sanitizers="ON"
    # The headers are the same files in both packages: the plain variant
    # compiles every one of them, unless it is not run.
    case " $variants " in *" plain "*) headers="OFF" ;; esac
  fi

  echo "==> [$name] building and installing the library into $prefix"
  rm -rf "$prefix" "$check"
  cmake -S "$root" -B "$build" \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF \
        -DALEPH_USE_SANITIZERS="$sanitizers" \
        -DCMAKE_INSTALL_PREFIX="$prefix"
  cmake --build "$build" --target Aleph --parallel "$jobs"
  cmake --install "$build"

  # What the build recorded is what the consumer must find.
  local revision
  revision="$(sed -n 's/^# define ALEPH_SOURCE_REVISION "\(.*\)"$/\1/p' \
              "$build/generated/aleph-version.H")"
  if [ -z "$revision" ]; then
    echo "[$name] no source revision in $build/generated/aleph-version.H" >&2
    exit 1
  fi

  echo "==> [$name] checking the installed package as an external consumer"
  cmake -S "$root/consumer/package-check" -B "$check" \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DCMAKE_PREFIX_PATH="$prefix" \
        -DALEPH_EXPECT_SOURCE_REVISION="$revision" \
        -DALEPH_EXPECT_SANITIZERS="$sanitizers" \
        -DALEPH_CHECK_EVERY_HEADER="$headers"

  # The package under test, not another Aleph-w installed on the system.
  local found
  found="$(sed -n 's/^Aleph_DIR:PATH=//p' "$check/CMakeCache.txt")"
  case "$found" in
    "$prefix"/*) ;;
    *) echo "[$name] the consumer found Aleph in '$found', not under $prefix" >&2; exit 1 ;;
  esac

  cmake --build "$check" --parallel "$jobs"
  ctest --test-dir "$check" --output-on-failure
  echo "==> [$name] ok: revision $revision, sanitizers $sanitizers"
}

for name in $variants; do
  check_variant "$name"
done

echo "Installed package check passed: $variants"
