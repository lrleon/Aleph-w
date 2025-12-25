#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Define build directory
BUILD_DIR="build-asan"

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
  mkdir "$BUILD_DIR"
fi

# Navigate to build directory
cd "$BUILD_DIR"

# Configure CMake with sanitizers enabled
# We point to the Tests directory where the CMakeLists.txt for tests resides
echo "Configuring CMake with AddressSanitizer and UndefinedBehaviorSanitizer..."
cmake -DCMAKE_BUILD_TYPE=Debug -DUSE_SANITIZERS=ON ../Tests

# Build all tests
echo "Building tests..."
cmake --build . -j$(nproc)

# Run tests using CTest
echo "Running tests..."
ctest --output-on-failure

echo "All tests passed successfully with sanitizers!"
