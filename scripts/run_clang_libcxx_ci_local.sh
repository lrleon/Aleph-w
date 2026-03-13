#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -P "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
ROOT_DIR="$(cd -P "${SCRIPT_DIR}/.." && pwd -P)"
BUILD_DIR="${ROOT_DIR}/build-clang-libcxx-ci"
GTEST_BUILD_DIR="${ROOT_DIR}/build-clang-libcxx-ci-gtest"
GTEST_INSTALL_DIR="${ROOT_DIR}/build-clang-libcxx-ci-gtest-install"
GTEST_SRC="${GTEST_SRC:-}"
LIBCXX_SYSROOT="${LIBCXX_SYSROOT:-}"
LIBCXX_INCLUDE_DIR="${LIBCXX_INCLUDE_DIR:-}"
LIBCXX_LIB_DIR="${LIBCXX_LIB_DIR:-}"
ALEPH_CLANG_LIBCXX_CLEAN="${ALEPH_CLANG_LIBCXX_CLEAN:-0}"
CTEST_ARGS=("$@")

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "[clang-libcxx-ci] Missing required command: $1" >&2
    exit 1
  fi
}

find_gtest_src() {
  if [ -n "${GTEST_SRC}" ]; then
    if [ ! -d "${GTEST_SRC}" ]; then
      echo "[clang-libcxx-ci] GTEST_SRC does not exist: ${GTEST_SRC}" >&2
      exit 1
    fi
    return
  fi

  for candidate in /usr/src/googletest /usr/src/gtest; do
    if [ -d "${candidate}" ]; then
      GTEST_SRC="${candidate}"
      return
    fi
  done

  echo "[clang-libcxx-ci] Could not find GoogleTest sources in /usr/src." >&2
  echo "[clang-libcxx-ci] Install libgtest-dev or set GTEST_SRC." >&2
  exit 1
}

configure_libcxx_paths() {
  if [ -n "${LIBCXX_SYSROOT}" ]; then
    if [ -z "${LIBCXX_INCLUDE_DIR}" ]; then
      LIBCXX_INCLUDE_DIR="${LIBCXX_SYSROOT}/usr/include/c++/v1"
    fi

    if [ -z "${LIBCXX_LIB_DIR}" ]; then
      for candidate in \
        "${LIBCXX_SYSROOT}/usr/lib/x86_64-linux-gnu" \
        "${LIBCXX_SYSROOT}/usr/lib/llvm-18/lib"; do
        if [ -e "${candidate}/libc++.so" ]; then
          LIBCXX_LIB_DIR="${candidate}"
          break
        fi
      done
    fi
  fi
}

check_libcxx_toolchain() {
  local tmp_src
  local tmp_bin
  local -a cmd

  tmp_src="$(mktemp "${TMPDIR:-/tmp}/aleph-libcxx-check.XXXXXX.cc")"
  tmp_bin="${tmp_src%.cc}"
  cat > "${tmp_src}" <<'EOF'
#include <vector>
int main()
{
  std::vector<int> v{1, 2, 3};
  return static_cast<int>(v.size());
}
EOF

  cmd=(clang++ -std=c++20 -stdlib=libc++)
  if [ -n "${LIBCXX_INCLUDE_DIR}" ]; then
    cmd+=(-isystem "${LIBCXX_INCLUDE_DIR}")
  fi
  if [ -n "${LIBCXX_LIB_DIR}" ]; then
    cmd+=(-L "${LIBCXX_LIB_DIR}" "-Wl,-rpath,${LIBCXX_LIB_DIR}")
  fi
  cmd+=("${tmp_src}" -o "${tmp_bin}" -lc++abi)

  if ! "${cmd[@]}" >/dev/null 2>&1; then
    echo "[clang-libcxx-ci] Clang + libc++ is not usable on this machine." >&2
    echo "[clang-libcxx-ci] Install clang libc++-dev libc++abi-dev libgtest-dev, or set LIBCXX_SYSROOT." >&2
    rm -f "${tmp_src}" "${tmp_bin}"
    exit 1
  fi

  rm -f "${tmp_src}" "${tmp_bin}"
}

prepare_gtest() {
  local gtest_cxx_flags="-stdlib=libc++"
  local gtest_link_flags="-lc++abi"

  if [ -n "${LIBCXX_INCLUDE_DIR}" ]; then
    gtest_cxx_flags="${gtest_cxx_flags} -isystem ${LIBCXX_INCLUDE_DIR}"
  fi
  if [ -n "${LIBCXX_LIB_DIR}" ]; then
    gtest_link_flags="${gtest_link_flags} -L${LIBCXX_LIB_DIR} -Wl,-rpath,${LIBCXX_LIB_DIR}"
  fi

  if [ -f "${GTEST_INSTALL_DIR}/lib/cmake/GTest/GTestConfig.cmake" ]; then
    echo "[clang-libcxx-ci] Reusing GoogleTest in ${GTEST_INSTALL_DIR}"
    return
  fi

  echo "[clang-libcxx-ci] Configure GoogleTest from ${GTEST_SRC}"
  cmake -S "${GTEST_SRC}" -B "${GTEST_BUILD_DIR}" -G Ninja \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_CXX_FLAGS="${gtest_cxx_flags}" \
    -DCMAKE_EXE_LINKER_FLAGS="${gtest_link_flags}" \
    -DCMAKE_SHARED_LINKER_FLAGS="${gtest_link_flags}" \
    -DCMAKE_INSTALL_PREFIX="${GTEST_INSTALL_DIR}" \
    -DBUILD_GMOCK=ON

  echo "[clang-libcxx-ci] Build and install GoogleTest"
  cmake --build "${GTEST_BUILD_DIR}" --parallel
  cmake --install "${GTEST_BUILD_DIR}"
}

configure_project() {
  local -a cmake_args
  local extra_cxx_flags="-stdlib=libc++"
  local extra_link_flags="-lc++abi"

  cmake_args=(
    --preset clang-libcxx-ci
    "-DGTest_DIR=${GTEST_INSTALL_DIR}/lib/cmake/GTest"
  )

  if [ -n "${LIBCXX_INCLUDE_DIR}" ]; then
    extra_cxx_flags="${extra_cxx_flags} -isystem ${LIBCXX_INCLUDE_DIR}"
  fi
  if [ -n "${LIBCXX_LIB_DIR}" ]; then
    extra_link_flags="${extra_link_flags} -L${LIBCXX_LIB_DIR} -Wl,-rpath,${LIBCXX_LIB_DIR}"
  fi

  cmake_args+=("-DCMAKE_CXX_FLAGS=${extra_cxx_flags}")
  cmake_args+=("-DCMAKE_EXE_LINKER_FLAGS=${extra_link_flags}")
  cmake_args+=("-DCMAKE_SHARED_LINKER_FLAGS=${extra_link_flags}")

  echo "[clang-libcxx-ci] Configure Aleph-w in ${BUILD_DIR}"
  cmake "${cmake_args[@]}"
}

run_tests() {
  mkdir -p "${BUILD_DIR}/test_artifacts/golden_svg"
  echo "[clang-libcxx-ci] Run CTest in ${BUILD_DIR}"
  ALEPH_TEST_GOLDEN_DIR="${BUILD_DIR}/test_artifacts/golden_svg" \
    ctest --preset clang-libcxx-ci ${CTEST_ARGS[@]+"${CTEST_ARGS[@]}"}
}

require_cmd cmake
require_cmd ctest
require_cmd clang
require_cmd clang++
require_cmd ninja

configure_libcxx_paths
find_gtest_src
check_libcxx_toolchain

if [ "${ALEPH_CLANG_LIBCXX_CLEAN}" = "1" ]; then
  echo "[clang-libcxx-ci] Cleaning previous local CI reproduction build directories"
  rm -rf "${BUILD_DIR}" "${GTEST_BUILD_DIR}" "${GTEST_INSTALL_DIR}"
fi

cd "${ROOT_DIR}"
prepare_gtest
configure_project

echo "[clang-libcxx-ci] Build Aleph-w in ${BUILD_DIR}"
cmake --build --preset clang-libcxx-ci --parallel
run_tests
