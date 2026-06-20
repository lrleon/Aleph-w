cmake_minimum_required(VERSION 3.18)

# Minimal external consumer used to prove that an installed Aleph-w can be
# integrated through find_package without cloning the repo (roadmap Phase 21).
#
# Build it against an install tree like this:
#
#   cmake -S consumer -B build-consumer \
#         -DCMAKE_PREFIX_PATH=/path/to/install/prefix
#   cmake --build build-consumer
#   ./build-consumer/gol_consumer
project(aleph_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# No explicit version: the installed package uses SameMajorVersion compatibility,
# so pinning a major here (e.g. "4.0") would reject any new major release. This
# smoke test only needs to prove find_package integration, not a version floor.
find_package(Aleph COMPONENTS CA REQUIRED)

add_executable(gol_consumer main.cc)
target_link_libraries(gol_consumer PRIVATE Aleph::CA)