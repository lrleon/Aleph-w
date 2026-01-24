# Aleph-w Installation Guide

Language: English | [Español](INSTALL.es.md)

This guide covers the installation of Aleph-w from source using CMake.

---

## Table of Contents

- [Prerequisites](#prerequisites)
  - [Compiler Requirements](#compiler-requirements)
  - [Build Tools](#build-tools)
  - [Required Libraries](#required-libraries)
  - [Optional Libraries](#optional-libraries)
- [Installation on Linux](#installation-on-linux)
  - [Ubuntu/Debian](#ubuntudebian)
  - [Fedora/RHEL](#fedorarhel)
  - [Arch Linux](#arch-linux)
- [Building from Source](#building-from-source)
  - [Basic Build](#basic-build)
  - [Build Options](#build-options)
  - [Build Types](#build-types)
- [Advanced Configuration](#advanced-configuration)
- [Verification](#verification)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Compiler Requirements

Aleph-w requires a modern C++ compiler with full C++20 support (C++17 and C++23 are also supported):

| Compiler | Minimum Version | Tested Versions | Status |
|----------|-----------------|-----------------|--------|
| **GCC** | 11.0 | 11, 12, 13 | ✅ Fully supported |
| **Clang** | 14.0 | 14, 15, 16 | ✅ Fully supported |
| **MSVC** | - | - | ❌ Not supported |

### Build Tools

- **CMake** 3.18 or later (3.21+ if using `CMakePresets.json`)
- **Make** or **Ninja** (Ninja recommended for faster builds)

### Required Libraries

The following libraries are **required** to build Aleph-w:

| Library | Package Name (Ubuntu/Debian) | Purpose |
|---------|------------------------------|---------|
| **GMP** | `libgmp-dev` | Arbitrary precision arithmetic |
| **GMP C++** | `libgmp-dev` (includes C++ bindings) | C++ interface for GMP |
| **MPFR** | `libmpfr-dev` | Multiple precision floating-point |
| **GSL** | `libgsl-dev` | GNU Scientific Library (random numbers, statistics) |
| **X11** | `libx11-dev` | X Window System (for visualization) |

Threads (pthreads) are required and typically provided by the OS C library (no separate package needed on most Linux distros).

### Optional Libraries

| Library | Package Name | Purpose |
|---------|--------------|---------|
| **GoogleTest** | `libgtest-dev` | Unit testing (auto-fetched if not found) |

Auto-fetching GoogleTest requires network access; set `ALEPH_FETCH_GTEST=OFF` to disable it (and/or set `BUILD_TESTS=OFF`).

**Note:** The following libraries are **NO LONGER REQUIRED**:
- ~~`autosprintf` / `gettext`~~ (removed)
- ~~`nana`~~ (removed)
- ~~`m4`~~ (removed)
- ~~`xutils-dev`~~ (no longer using Imakefile/xmkmf)

---

## Installation on Linux

### Ubuntu/Debian

```bash
# Update package lists
sudo apt-get update

# Install compiler (choose one)
sudo apt-get install -y g++      # GCC
sudo apt-get install -y clang    # Clang (alternative)

# Install build tools
sudo apt-get install -y cmake ninja-build

# Install required libraries
sudo apt-get install -y \
    libgmp-dev \
    libmpfr-dev \
    libgsl-dev \
    libx11-dev

# Optional: Install GoogleTest (otherwise auto-fetched)
sudo apt-get install -y libgtest-dev
```

### Fedora/RHEL

```bash
# Install compiler
sudo dnf install -y gcc-c++      # GCC
# sudo dnf install -y clang       # Clang (alternative)

# Install build tools
sudo dnf install -y cmake ninja-build

# Install required libraries
sudo dnf install -y \
    gmp-devel \
    mpfr-devel \
    gsl-devel \
    libX11-devel

# Optional: Install GoogleTest
sudo dnf install -y gtest-devel
```

### Arch Linux

```bash
# Install compiler (usually already installed)
sudo pacman -S gcc               # GCC
# sudo pacman -S clang           # Clang (alternative)

# Install build tools
sudo pacman -S cmake ninja

# Install required libraries
sudo pacman -S \
    gmp \
    mpfr \
    gsl \
    libx11

# Optional: Install GoogleTest
sudo pacman -S gtest
```

---

## Building from Source

### Basic Build

```bash
# Clone the repository
git clone https://github.com/lrleon/Aleph-w.git
cd Aleph-w

# Configure the build
cmake -S . -B build -G Ninja

# Build the library
cmake --build build

# Run tests (optional but recommended)
ctest --test-dir build --output-on-failure

# Install system-wide (optional)
sudo cmake --install build
```

### Using CMake Presets (Optional)

If you have CMake 3.21+, you can use the provided `CMakePresets.json`:

```bash
cmake --preset default
cmake --build --preset default
ctest --preset default
```

**Alternative using Make:**

If you prefer Make over Ninja:

```bash
cmake -S . -B build            # Omit -G Ninja
cmake --build build -j$(nproc)
```

### Build Options

Aleph-w provides several CMake options to customize the build:

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | `Debug` | Build type: `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel` |
| `ALEPH_CXX_STANDARD` | `20` | C++ standard: `17`, `20`, or `23` |
| `BUILD_EXAMPLES` | `ON` | Build example programs |
| `BUILD_TESTS` | `ON` | Build test suite |
| `BUILD_OPTIMIZED` | `OFF` | Convenience switch: if `CMAKE_BUILD_TYPE` is unset, default to `Release` |

**Example with custom options:**

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DALEPH_CXX_STANDARD=20 \
    -DBUILD_EXAMPLES=ON \
    -DBUILD_TESTS=ON \
    -G Ninja

cmake --build build
```

### Build Types

#### Debug Build (Default)

Includes debug symbols, no optimizations, assertions enabled:

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
```

Compiler flags applied: `-DDEBUG -g3 -O0`

#### Release Build

Optimized for performance, no debug symbols, assertions disabled:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

Compiler flags applied: `-DNDEBUG -O2 -fno-strict-aliasing -fPIE -fpic`

**Important:** `-fno-strict-aliasing` is **required** due to type-punning in `htlist.H`. Do not remove this flag!

#### Release with Debug Info

Optimized but with debug symbols:

```bash
cmake -S . -B build-relwithdebinfo -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-relwithdebinfo
```

#### “Optimized” Convenience Build

`BUILD_OPTIMIZED` is mainly a convenience switch used by some presets: if you do not set `CMAKE_BUILD_TYPE`, it makes the default build type `Release`.

```bash
cmake -S . -B build-optimized -DBUILD_OPTIMIZED=ON
cmake --build build-optimized
```

If you want `-Ofast`, pass it explicitly via `CMAKE_CXX_FLAGS_RELEASE` (use with caution).

---

## Advanced Configuration

### Selecting a C++ Standard

```bash
# Build with C++17
cmake -S . -B build -DALEPH_CXX_STANDARD=17

# Build with C++20 (default)
cmake -S . -B build -DALEPH_CXX_STANDARD=20

# Build with C++23
cmake -S . -B build -DALEPH_CXX_STANDARD=23
```

### Selecting a Compiler

```bash
# Use GCC
cmake -S . -B build \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_C_COMPILER=gcc

# Use Clang
cmake -S . -B build \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_C_COMPILER=clang

# Use a specific version
cmake -S . -B build \
    -DCMAKE_CXX_COMPILER=g++-13 \
    -DCMAKE_C_COMPILER=gcc-13
```

### Custom Installation Prefix

```bash
# Install to /opt/aleph
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/opt/aleph
cmake --build build
sudo cmake --install build
```

### Building Only the Library

If you don't need examples or tests:

```bash
cmake -S . -B build \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_TESTS=OFF

cmake --build build
```

### Out-of-Source Build

Out-of-source builds are strongly recommended. Avoid running `cmake` directly in the source directory:

```bash
# ✅ Correct
cmake -S . -B build

# ❌ Wrong (pollutes source directory)
cd /path/to/Aleph-w
cmake .
```

---

## Verification

### Running Tests

```bash
# Run all tests
ctest --test-dir build --output-on-failure

# Run tests verbosely
ctest --test-dir build -V

# Run specific test
./build/Tests/dynlist
./build/Tests/test_dijkstra
./build/Tests/latex_floyd_test

# Run tests in parallel
ctest --test-dir build -j$(nproc)
```

Note: some long-running/performance/stress tests are intentionally marked Disabled/Skipped; `ctest` will list them at the end of the run.

### Testing with Sanitizers

To detect memory errors and undefined behavior:

```bash
# Build with AddressSanitizer and UndefinedBehaviorSanitizer
cmake -S . -B build-asan \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g" \
    -DCMAKE_C_FLAGS="-fsanitize=address,undefined -g"

cmake --build build-asan
ctest --test-dir build-asan
```

### Verifying Installation

After installing:

```bash
# Check if library is installed
ls /usr/local/lib/libAleph.a

# Check if headers are installed
ls /usr/local/include/aleph/

# Test with a simple program
cat > test.cpp << 'EOF'
#include <tpl_dynList.H>
#include <iostream>

int main() {
    Aleph::DynList<int> list = {1, 2, 3, 4, 5};
    list.for_each([](int x) {
        std::cout << x << " ";
    });
    std::cout << "\n";
    return 0;
}
EOF

g++ -std=c++20 -I/usr/local/include/aleph test.cpp \
    -L/usr/local/lib -lAleph -lgmp -lmpfr -lgsl -lgslcblas -lpthread -lX11 -lm \
    -o test

./test
# Output: 1 2 3 4 5
```

---

## Troubleshooting

### Common Issues

#### 1. CMake version too old

**Error:**
```
CMake Error at CMakeLists.txt:1 (cmake_minimum_required):
  CMake 3.18 or higher is required.
```

**Solution:**

Update CMake from [cmake.org](https://cmake.org/download/) or use a PPA:

```bash
# Ubuntu
sudo apt-get install -y software-properties-common
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | \
    gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt-get update
sudo apt-get install -y cmake
```

#### 2. GMP not found

**Error:**
```
/usr/bin/ld: cannot find -lgmp
```

**Solution:**

Install GMP development files:

```bash
sudo apt-get install libgmp-dev    # Ubuntu/Debian
sudo dnf install gmp-devel          # Fedora/RHEL
sudo pacman -S gmp                  # Arch
```

#### 3. X11 not found

**Error:**
```
Could not find X11
```

**Solution:**

Install X11 development files:

```bash
sudo apt-get install libx11-dev    # Ubuntu/Debian
sudo dnf install libX11-devel       # Fedora/RHEL
sudo pacman -S libx11               # Arch
```

#### 4. Tests failing

If tests fail, first ensure all dependencies are installed:

```bash
# Re-run cmake to check dependencies
cmake -S . -B build

# Rebuild completely
rm -rf build
cmake -S . -B build
cmake --build build

# Run tests with verbose output
ctest --test-dir build -V
```

#### 5. Compilation errors with C++20

**Error:**
```
error: 'concept' does not name a type
```

**Solution:**

Ensure your compiler supports C++20:

```bash
# Check GCC version (needs 11+)
g++ --version

# Check Clang version (needs 14+)
clang++ --version

# If version is too old, install newer compiler
sudo apt-get install g++-13    # Ubuntu
```

Alternatively, build with C++17:

```bash
cmake -S . -B build -DALEPH_CXX_STANDARD=17
```

#### 6. Linker errors with `-fno-strict-aliasing`

**Error:**
```
Segmentation fault in DynList operations
```

**Cause:** The `-fno-strict-aliasing` flag was removed from Release builds.

**Solution:**

Ensure Release builds include `-fno-strict-aliasing`. This is required due to type-punning in `htlist.H`:

```bash
# Check if flag is present
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
grep -R "fno-strict-aliasing" build/CMakeFiles/*/flags.make
```

If missing, the CMakeLists.txt needs to be updated (it should already include this flag).

---

## Building Examples

```bash
# Build with examples
cmake -S . -B build -DBUILD_EXAMPLES=ON
cmake --build build

# Run an example
./build/Examples/dijkstra_example
./build/Examples/mst_example
./build/Examples/network_flow_example
```

---

## Using Aleph-w in Your Project

### Method 1: System-wide Installation

```bash
# Install Aleph-w
sudo cmake --install build

# In your CMakeLists.txt
find_library(ALEPH_LIB Aleph)
find_path(ALEPH_INCLUDE_DIR tpl_dynList.H PATHS /usr/local/include/aleph)

find_package(Threads REQUIRED)
find_package(X11 REQUIRED)

target_include_directories(your_target PRIVATE ${ALEPH_INCLUDE_DIR})
target_link_libraries(your_target PRIVATE
  ${ALEPH_LIB}
  gmpxx gmp mpfr gsl gslcblas m
  Threads::Threads
  ${X11_LIBRARIES}
)
```

### Method 2: CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    aleph-w
    GIT_REPOSITORY https://github.com/lrleon/Aleph-w.git
    GIT_TAG v3
)
FetchContent_MakeAvailable(aleph-w)

target_link_libraries(your_target PRIVATE Aleph)
```

### Method 3: Add as Subdirectory

```cmake
add_subdirectory(path/to/Aleph-w)
target_link_libraries(your_target PRIVATE Aleph)
```

### Method 4: Direct Compilation

```bash
g++ -std=c++20 \
    -I/path/to/Aleph-w \
    your_code.cpp \
    /path/to/Aleph-w/build/libAleph.a \
    -lgmpxx -lgmp -lmpfr -lgsl -lgslcblas -lpthread -lX11 -lm \
    -o your_program
```

---

## Platform-Specific Notes

### Ubuntu 20.04 (Focal)

GCC 9 is the default compiler, which is too old. Install GCC 11+:

```bash
sudo apt-get install software-properties-common
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install g++-11

# Use GCC 11 explicitly
cmake -S . -B build -DCMAKE_CXX_COMPILER=g++-11
```

### WSL (Windows Subsystem for Linux)

Aleph-w builds successfully on WSL2 with Ubuntu. Follow the Ubuntu instructions above.

### macOS

Aleph-w is primarily developed for Linux. macOS is not officially supported, but it may work with Homebrew:

```bash
# Install dependencies (untested)
brew install gcc cmake ninja gmp mpfr gsl libx11

# Build with Homebrew GCC
cmake -S . -B build \
    -DCMAKE_CXX_COMPILER=/usr/local/bin/g++-13 \
    -DCMAKE_C_COMPILER=/usr/local/bin/gcc-13
```

---

## Getting Help

If you encounter issues not covered here:

1. Check the [GitHub Issues](https://github.com/lrleon/Aleph-w/issues)
2. Review the [README.md](README.md)
3. Open a new issue with:
   - Your OS and version
   - Compiler and version
   - CMake version
   - Full error output

---

## Summary

**Quick start for Ubuntu:**

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build \
    libgmp-dev libmpfr-dev libgsl-dev libx11-dev

# Build and install
git clone https://github.com/lrleon/Aleph-w.git
cd Aleph-w
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
sudo cmake --install build
```

**That's it!** You're now ready to use Aleph-w.
