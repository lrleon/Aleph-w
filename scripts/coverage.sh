#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -P "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
ROOT_DIR="$(cd -P "${SCRIPT_DIR}/.." && pwd -P)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-coverage}"
COVERAGE_CLEAN="${COVERAGE_CLEAN:-1}"
CTEST_ARGS=("$@")

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required command: $1" >&2
    exit 1
  fi
}

normalize_path() {
  local path="$1"

  if command -v realpath >/dev/null 2>&1; then
    if realpath -m / >/dev/null 2>&1; then
      realpath -m "${path}"
      return
    fi
  fi

  if command -v ruby >/dev/null 2>&1; then
    ruby -e 'require "pathname"; puts Pathname.new(ARGV[0]).expand_path.to_s' "${path}"
    return
  fi

  if command -v realpath >/dev/null 2>&1; then
    realpath "${path}"
    return
  fi

  echo "Missing required command: realpath (with -m) or ruby for path normalization" >&2
  exit 1
}

require_cmd cmake
require_cmd ctest
require_cmd gcovr
BUILD_DIR="$(normalize_path "${BUILD_DIR}")"

cd "${ROOT_DIR}"

if [ "${COVERAGE_CLEAN}" = "1" ]; then
  echo "[coverage] Cleaning previous coverage build: ${BUILD_DIR}"
  rm -rf "${BUILD_DIR}"
fi

echo "[coverage] Configure preset 'coverage'"
cmake --preset coverage

echo "[coverage] Build preset 'coverage'"
cmake --build --preset coverage --parallel

echo "[coverage] Run tests preset 'coverage'"
set +e
ctest --preset coverage "${CTEST_ARGS[@]}"
TEST_STATUS=$?
set -e

if [ "${TEST_STATUS}" -ne 0 ]; then
  echo "[coverage] Some tests failed (ctest exit code: ${TEST_STATUS})."
  echo "[coverage] Continuing to generate coverage reports."
fi

GCOVR_ARGS=(
  --root "${ROOT_DIR}"
  --object-directory "${BUILD_DIR}"
  --gcov-exclude "(^|.*/)CMakeFiles/[0-9.]+/CompilerId(C|CXX)/.*"
  --gcov-exclude "(^|.*/)CompilerId(C|CXX)/.*"
  --gcov-ignore-errors source_not_found
  --gcov-ignore-errors no_working_dir_found
  --gcov-ignore-errors output_error
  --gcov-ignore-parse-errors negative_hits.warn
  --exclude "(^|.*/)Tests/.*"
  --exclude "(^|.*/)Examples/.*"
  --exclude "(^|.*/)build-coverage/.*"
  --exclude "(^|.*/)cmake-build-.*/.*"
  --exclude "(^|.*/)build-.*/.*"
  --exclude "(^|.*/)CMakeFiles/[0-9.]+/CompilerId(C|CXX)/.*"
)

echo "[coverage] Generate XML report"
gcovr "${GCOVR_ARGS[@]}" \
  --xml-pretty --output "${BUILD_DIR}/coverage.xml"

echo "[coverage] Generate HTML report"
gcovr "${GCOVR_ARGS[@]}" \
  --html-details --output "${BUILD_DIR}/coverage.html"

echo "[coverage] Generate text summary"
gcovr "${GCOVR_ARGS[@]}" \
  --sort uncovered-percent --print-summary | tee "${BUILD_DIR}/coverage.txt"

echo "[coverage] Reports:"
echo "  ${BUILD_DIR}/coverage.html"
echo "  ${BUILD_DIR}/coverage.xml"
echo "  ${BUILD_DIR}/coverage.txt"

if [ "${TEST_STATUS}" -ne 0 ]; then
  echo "[coverage] Returning non-zero because tests failed."
  exit "${TEST_STATUS}"
fi
