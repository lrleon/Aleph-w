cmake_minimum_required(VERSION 3.18)

# External consumer that checks an installed Aleph-w package, with nothing
# from the source tree on its include path:
#
#   - its identity (release number, source revision, sanitizer configuration)
#     is the same in the CMake package, in the Aleph::Aleph target and in the
#     header aleph-version.H;
#   - a package built with sanitizers gives its consumers the instrumentation
#     and the MemArray annotations through Aleph::Aleph alone, in any build
#     type, and an access outside the logical size of an array is reported;
#   - every installed header compiles by itself.
#
# Usage, against an install tree:
#
#   cmake -S consumer/package-check -B build-package-check \
#         -DCMAKE_BUILD_TYPE=Release \
#         -DCMAKE_PREFIX_PATH=/path/to/install/prefix
#   cmake --build build-package-check --parallel
#   ctest --test-dir build-package-check --output-on-failure
#
# Optional expectations, to assert which package was found:
#
#   -DALEPH_EXPECT_SOURCE_REVISION=<id>
#   -DALEPH_EXPECT_SANITIZERS=ON|OFF
#
# -DALEPH_CHECK_EVERY_HEADER=OFF skips the compilation of every header, which
# takes most of the time.
project(aleph_package_check LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ALEPH_CHECK_EVERY_HEADER "Compile every installed header by itself" ON)

# No version is requested: this project checks whatever package it is given.
find_package(Aleph REQUIRED)

enable_testing()

# ------------------------------------------------------------------------------
# Identity
# ------------------------------------------------------------------------------
foreach(_var IN ITEMS Aleph_VERSION Aleph_SOURCE_REVISION)
    if(NOT DEFINED ${_var} OR "${${_var}}" STREQUAL "")
        message(FATAL_ERROR "find_package(Aleph) did not set ${_var}")
    endif()
endforeach()
foreach(_var IN ITEMS Aleph_SANITIZERS Aleph_CONTAINER_ANNOTATIONS)
    if(NOT DEFINED ${_var})
        message(FATAL_ERROR "find_package(Aleph) did not set ${_var}")
    endif()
endforeach()

# The target says what the package says.
foreach(_item IN ITEMS SOURCE_REVISION SANITIZERS CONTAINER_ANNOTATIONS)
    get_target_property(_from_target Aleph::Aleph ALEPH_${_item})
    if(NOT "${_from_target}" STREQUAL "${Aleph_${_item}}")
        message(FATAL_ERROR
            "Aleph::Aleph says ALEPH_${_item}='${_from_target}' but the package "
            "says Aleph_${_item}='${Aleph_${_item}}'")
    endif()
endforeach()

if(DEFINED ALEPH_EXPECT_SOURCE_REVISION
   AND NOT "${ALEPH_EXPECT_SOURCE_REVISION}" STREQUAL "${Aleph_SOURCE_REVISION}")
    message(FATAL_ERROR
        "Expected the Aleph-w revision '${ALEPH_EXPECT_SOURCE_REVISION}' but "
        "${Aleph_DIR} is '${Aleph_SOURCE_REVISION}'")
endif()

if(DEFINED ALEPH_EXPECT_SANITIZERS)
    if((ALEPH_EXPECT_SANITIZERS AND NOT Aleph_SANITIZERS)
       OR (NOT ALEPH_EXPECT_SANITIZERS AND Aleph_SANITIZERS))
        message(FATAL_ERROR
            "Expected an Aleph-w package with sanitizers ${ALEPH_EXPECT_SANITIZERS} "
            "but ${Aleph_DIR} has Aleph_SANITIZERS=${Aleph_SANITIZERS}")
    endif()
endif()

if(Aleph_CONTAINER_ANNOTATIONS AND NOT Aleph_SANITIZERS)
    message(FATAL_ERROR "Container annotations without sanitizers in ${Aleph_DIR}")
endif()

message(STATUS "Aleph-w ${Aleph_VERSION}, revision ${Aleph_SOURCE_REVISION}, "
               "sanitizers ${Aleph_SANITIZERS}, "
               "container annotations ${Aleph_CONTAINER_ANNOTATIONS} (${Aleph_DIR})")

set(_expect_sanitizers 0)
if(Aleph_SANITIZERS)
    set(_expect_sanitizers 1)
endif()
set(_expect_annotations 0)
if(Aleph_CONTAINER_ANNOTATIONS)
    set(_expect_annotations 1)
endif()

# The header says what the package says.
add_executable(package_identity package_identity.cc)
target_link_libraries(package_identity PRIVATE Aleph::Aleph)
target_compile_definitions(package_identity PRIVATE
    EXPECT_VERSION="${Aleph_VERSION}"
    EXPECT_VERSION_MAJOR=${Aleph_VERSION_MAJOR}
    EXPECT_VERSION_MINOR=${Aleph_VERSION_MINOR}
    EXPECT_VERSION_PATCH=${Aleph_VERSION_PATCH}
    EXPECT_SOURCE_REVISION="${Aleph_SOURCE_REVISION}"
    EXPECT_SANITIZERS=${_expect_sanitizers}
    EXPECT_CONTAINER_ANNOTATIONS=${_expect_annotations})
add_test(NAME package_identity COMMAND package_identity)

# ------------------------------------------------------------------------------
# Logical bounds of the arrays under the configuration of the package
# ------------------------------------------------------------------------------
add_executable(logical_bounds logical_bounds.cc)
target_link_libraries(logical_bounds PRIVATE Aleph::Aleph)
target_compile_definitions(logical_bounds PRIVATE
    EXPECT_CONTAINER_ANNOTATIONS=${_expect_annotations})

# Every legitimate access, with the poisoning checked after each operation.
add_test(NAME logical_bounds.valid_accesses COMMAND logical_bounds)

if(Aleph_CONTAINER_ANNOTATIONS)
    # A read inside the capacity and outside the logical size must be reported
    # in every build type: these do not rely on assertions.
    foreach(_case IN ITEMS after_put after_growth after_shrink after_move after_swap after_reuse)
        add_test(NAME logical_bounds.overflow_${_case} COMMAND logical_bounds ${_case})
        set_tests_properties(logical_bounds.overflow_${_case} PROPERTIES
            PASS_REGULAR_EXPRESSION "AddressSanitizer: container-overflow"
            FAIL_REGULAR_EXPRESSION "the read was not reported")
    endforeach()
endif()

# ------------------------------------------------------------------------------
# Every installed header compiles by itself, against the installation alone
# ------------------------------------------------------------------------------
if(ALEPH_CHECK_EVERY_HEADER)
    get_target_property(_aleph_includes Aleph::Aleph INTERFACE_INCLUDE_DIRECTORIES)
    set(_aleph_header_dir "")
    foreach(_dir IN LISTS _aleph_includes)
        if(EXISTS "${_dir}/aleph-version.H")
            set(_aleph_header_dir "${_dir}")
            break()
        endif()
    endforeach()
    if(_aleph_header_dir STREQUAL "")
        message(FATAL_ERROR "aleph-version.H is not in the include directories of Aleph::Aleph")
    endif()

    file(GLOB _aleph_headers RELATIVE "${_aleph_header_dir}"
         "${_aleph_header_dir}/*.H" "${_aleph_header_dir}/*.h")
    list(SORT _aleph_headers)

    # Installed headers that are not expected to compile by themselves.
    set(_not_self_contained
        # Fragments included by other headers once their context is declared.
        graph-dry.H          # by the graph classes, after Dlink and the node classes
        hashDry.H            # by the hash tables, after Primes
        single_graph.H       # by the algorithms, after the filtered iterators
        generate_df_tree.H   # by the tree drawing programs
        # Known debt: they need an include that their users happen to provide.
        ah-time.H            # <cassert>
        ahTypes.H            # <utility>
        mat_latex.H          # <sstream>
        # Needs libxml++-2.6, which is not a dependency of the package.
        xml_graph.H
    )
    foreach(_header IN LISTS _not_self_contained)
        list(FIND _aleph_headers "${_header}" _index)
        if(_index EQUAL -1)
            message(FATAL_ERROR
                "${_header} is listed as not self-contained but is not installed: "
                "update consumer/package-check/CMakeLists.txt")
        endif()
    endforeach()
    list(REMOVE_ITEM _aleph_headers ${_not_self_contained})

    list(LENGTH _aleph_headers _count)
    message(STATUS "Compiling ${_count} installed headers by themselves from ${_aleph_header_dir}")

    set(_probe_sources "")
    foreach(_header IN LISTS _aleph_headers)
        string(MAKE_C_IDENTIFIER "${_header}" _id)
        set(_source "${CMAKE_CURRENT_BINARY_DIR}/header-probes/${_id}.cc")
        file(GENERATE OUTPUT "${_source}" CONTENT "#include <${_header}>\n")
        list(APPEND _probe_sources "${_source}")
    endforeach()

    # An object library: the headers only have to compile.
    add_library(every_header OBJECT ${_probe_sources})
    target_link_libraries(every_header PRIVATE Aleph::Aleph)
endif()
