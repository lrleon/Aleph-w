cmake_minimum_required(VERSION 3.18)

project(Aleph-w VERSION 1.0.0 LANGUAGES C CXX)

# ------------------------------------------------------------------------------
# Compiler / standard configuration
# ------------------------------------------------------------------------------

set(ALEPH_SUPPORTED_CXX_STANDARDS 17 20 23)
set(_ALEPH_DEFAULT_CXX_STANDARD 20)
set(ALEPH_CXX_STANDARD ${_ALEPH_DEFAULT_CXX_STANDARD}
    CACHE STRING "C++ standard to use when building Aleph-w")
set_property(CACHE ALEPH_CXX_STANDARD PROPERTY STRINGS ${ALEPH_SUPPORTED_CXX_STANDARDS})

list(FIND ALEPH_SUPPORTED_CXX_STANDARDS "${ALEPH_CXX_STANDARD}" _ALEPH_STD_INDEX)
if(_ALEPH_STD_INDEX EQUAL -1)
    message(FATAL_ERROR
        "ALEPH_CXX_STANDARD='${ALEPH_CXX_STANDARD}' is not supported. "
        "Select one of: ${ALEPH_SUPPORTED_CXX_STANDARDS}")
endif()

# Respect a user-provided CMAKE_CXX_STANDARD if explicitly set; otherwise apply
# the Aleph default/value. This allows toolchains to inject their own choice.
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD ${ALEPH_CXX_STANDARD})
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_OPTIMIZED "Build optimized version (-Ofast -DNDEBUG)" OFF)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_TESTS "Build tests" ON)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    if(BUILD_OPTIMIZED)
        set(CMAKE_BUILD_TYPE Release)
    else()
        set(CMAKE_BUILD_TYPE Debug)
    endif()
endif()

# Compiler flags
set(WARN_FLAGS "-Wall -Wextra -Wcast-align -Wno-sign-compare -Wno-write-strings -Wno-parentheses")
set(COMMON_FLAGS "-DMESSAGES -D_GLIBCXX__PTHREADS -D_REENTRANT")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(COMMON_FLAGS "${COMMON_FLAGS} -D__extern_always_inline=\"extern __always_inline\"")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS} ${COMMON_FLAGS}")

# Debug flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -g3 -O0")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -g3 -O0")

# Release flags
# Note: Using -O2 instead of -O3 because -O3 can cause issues with certain
# template-heavy code patterns.
#
# IMPORTANT: -fno-strict-aliasing is REQUIRED!
# ==============================================
# htlist.H (Snodenc<T>::get_next()) uses reinterpret_cast to return a reference
# to the base class pointer (Slinknc*) as a derived class pointer reference
# (Snodenc<T>*&). This violates C++ strict aliasing rules. Without this flag,
# GCC's optimizer may generate incorrect code, causing segfaults in DynList,
# DynListStack, DynListQueue, and other data structures that use these lists.
#
# The original Makefile (2017) always included this flag. It was accidentally
# omitted when migrating to CMake. See htlist.H for a proposed CRTP refactoring
# that would eliminate this requirement.
#
# DO NOT REMOVE -fno-strict-aliasing without first refactoring htlist.H!
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O2 -fno-strict-aliasing -fPIE -fpic")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG -O2 -fno-strict-aliasing -fPIE -fpic")

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Header files
set(HLIST
    tpl_dynSetTree.H tpl_arrayHeap.H tpl_binHeap.H tpl_netcost.H
    tpl_dynBinHeap.H generate_tree.H generate_graph.H tpl_tree_node.H
    point.H tpl_treapRk.H Set.H Map.H array_it.H al-domain.H
    Multiset.H Multimap.H List.H Vector.H ahAlgo.H Stack.H
    Queue.H tpl_odhash.H tpl_nodePool.H random_graph.H
        tpl_olhash.H tpl_dynSetHash.H ahSort.H ahSearch.H
    tpl_random_queue.H tpl_net_sup_dem.H tpl_bipartite.H graph-traverse.H
    tpl_dynarray_set.H tpl_kgraph.H tpl_dynMat.H
    ahMem.H ahAssert.H ahDefs.H aleph.H euclidian-graph-common.H
    tpl_linearHash.H signal.H ahUtils.H ran_array.h tpl_dynDlist.H
    tpl_skipList.H tpl_tdRbTree.H tpl_splay_tree.H
    tpl_dynTreap.H tpl_union.H tpl_binNode.H tpl_splay_treeRk.H
    useMutex.H socket_wrappers.H tpl_dynListStack.H
    tpl_autoPtr.H trace.H tpl_cache.H format.H random_net.H
        log.H useCondVar.H
    tpl_dynListQueue.H al-vector.H ahSingleton.H comb.H
    treepic_utils.H driven_table.H pointer_table.H Karger.H al-matrix.H
    timeoutQueue.H parse_utils.H ah_stdc++_utils.H eulerian.H
    hamiltonian.H ahTypes.H tpl_arrayStack.H tpl_arrayQueue.H
    tpl_graph.H mat_path.H tpl_slist_modified.H ahFunction.H ahPair.H
    ahIterator.H grid.H tpl_path_utils.H mat_latex.H tpl_sort_utils.H
    polygon.H generate_df_tree.H generate_spanning_tree_picture.H
    archeap.H filter_iterator.H kosaraju.H htlist.H tpl_sgraph.H
    tpl_indexNode.H tpl_indexArc.H tpl_indexGraph.H io_graph.H
    tpl_slist.H tpl_snode.H tpl_lhash.H tpl_dynLhash.H
    tpl_odhash.H tpl_memArray.H tpl_dynArray.H ahFuntional.H hash-dry.H
    bloom-filter.H ah-dry.H graph-dry.H tpl_con_queue.H
    q-consumer-threads.H ah-string-utils.H ah-comb.H geom_algorithms.H
)

# C++ source files
set(LIBCCCSRCS
    ahAssert.C
    primes.C
    ahDefs.C
    useMutex.C
    ahDaemonize.C
    socket_wrappers.C
    timeoutQueue.C
    uid.C
    gmpfrxx.C
        eepicgeom.C
    ahUtils.C
    tpl_sort_utils.C
    hash-fct.C
    hash-dry.C
    aleph-graph.C
    ahNow.C
    point.C
    euclidian-graph-common.C
)

# C source files
set(LIBCSRCS
    ran_array.c
    mpfr_mul_d.c
)

# Create static library
add_library(Aleph STATIC ${LIBCCCSRCS} ${LIBCSRCS})

# Find required libraries
find_package(Threads REQUIRED)
find_package(X11 REQUIRED)

set(ALEPH_SYSTEM_LIBS
    gmpxx
    gmp
    pthread
    mpfr
    gsl
    gslcblas
    m
    ${X11_LIBRARIES}
    Threads::Threads
    CACHE INTERNAL "System libraries required by Aleph"
)

# Link system libraries
target_link_libraries(Aleph PUBLIC ${ALEPH_SYSTEM_LIBS})

# Set include directories for the library
target_include_directories(Aleph PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/aleph>
)

# Install library
install(TARGETS Aleph
    EXPORT AlephTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install headers
install(FILES ${HLIST}
    DESTINATION include/aleph
)

# Add subdirectories if enabled
if(BUILD_EXAMPLES)
    add_subdirectory(Examples)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()

# Print configuration summary
message(STATUS "Aleph-w Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")