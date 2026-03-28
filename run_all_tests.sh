#!/bin/bash

# Script to compile and run all tests in Debug and Release modes.
# Stops at the first failure and reports the problem.

set -e # Exit immediately if a command exits with a non-zero status.

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

PROJECT_ROOT=$(pwd)
BUILD_DIR_DEBUG="$PROJECT_ROOT/build-debug"
BUILD_DIR_RELEASE="$PROJECT_ROOT/build-release"

# Compute number of jobs for parallel build
JOBS="$(command -v nproc >/dev/null 2>&1 && nproc || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"

report_failure() {
    echo -e "${RED}[FAIL] $1${NC}"
    exit 1
}

run_stage() {
    local mode=$1
    local build_dir=$2
    
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE} Starting $mode Build and Test...${NC}"
    echo -e "${BLUE}========================================${NC}"

    # 1. Configuration
    echo -e "${BLUE}[1/3] Configuring $mode...${NC}"
    cmake -S "$PROJECT_ROOT" -B "$build_dir" \
          -DCMAKE_BUILD_TYPE="$mode" \
          -DBUILD_TESTS=ON \
          -DBUILD_EXAMPLES=OFF || report_failure "Configuration failed for $mode mode."

    # 2. Compilation
    echo -e "${BLUE}[2/3] Compiling $mode...${NC}"
    cmake --build "$build_dir" -j"$JOBS" || report_failure "Compilation failed for $mode mode."

    # 3. Execution
    echo -e "${BLUE}[3/3] Running tests for $mode...${NC}"
    ctest --test-dir "$build_dir" --output-on-failure || report_failure "Test execution failed for $mode mode."

    echo -e "${GREEN}[SUCCESS] $mode stage completed.${NC}\n"
}

# Ensure we are in the project root
if [ ! -f "CMakeLists.txt" ]; then
    report_failure "CMakeLists.txt not found. Please run this script from the project root."
fi

# Run Debug stage
run_stage "Debug" "$BUILD_DIR_DEBUG"

# Run Release stage
run_stage "Release" "$BUILD_DIR_RELEASE"

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN} ALL TESTS PASSED (Debug & Release)     ${NC}"
echo -e "${GREEN}========================================${NC}"
