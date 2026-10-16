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
#
# or against the sources, which it adds with add_subdirectory:
#
#   cmake -S consumer -B build-consumer -DALEPH_SOURCE_DIR=/path/to/Aleph-w
#
# It chooses no C++ standard on purpose: Aleph::CA brings the C++20 that the
# headers need, in both ways.
project(aleph_consumer LANGUAGES CXX)

if(DEFINED ALEPH_SOURCE_DIR)
    # Only the library is needed, not its tests or examples.
    set(BUILD_TESTS OFF)
    set(BUILD_EXAMPLES OFF)
    add_subdirectory("${ALEPH_SOURCE_DIR}" aleph EXCLUDE_FROM_ALL)
else()
    # No explicit version: the installed package uses SameMajorVersion
    # compatibility, so pinning a major here (e.g. "4.0") would reject any new
    # major release. This smoke test only needs to prove find_package
    # integration, not a version floor.
    find_package(Aleph COMPONENTS CA REQUIRED)
endif()

add_executable(gol_consumer main.cc)
target_link_libraries(gol_consumer PRIVATE Aleph::CA)
