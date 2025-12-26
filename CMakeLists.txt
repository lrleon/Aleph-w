cmake_minimum_required(VERSION 3.18)

project(Aleph-w VERSION 1.0.0 LANGUAGES C CXX)

# Set C++23 standard
set(CMAKE_CXX_STANDARD 20)
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
set(COMMON_FLAGS "-DMESSAGES -D_GLIBCXX__PTHREADS -D_REENTRANT -D__extern_always_inline=\"extern __always_inline\"")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS} ${COMMON_FLAGS}")

# Debug flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -g3 -O0")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -g3 -O0")

# Release flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -Ofast -fPIE -fpic")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG -Ofast -fPIE -fpic")

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Header files
set(HLIST
    tpl_dynSetTree.H tpl_arrayHeap.H tpl_binHeap.H tpl_netcost.H
    tpl_dynBinHeap.H generate_tree.H generate_graph.H tpl_tree_node.H
    point.H tpl_treapRk.H Set.H Map.H array_it.H al-domain.H
    Multiset.H Multimap.H List.H Vector.H ahAlgo.H Stack.H
    Queue.H tpl_odhash.H tpl_nodePool.H random_graph.H Priority_Queue.H
    tpl_olhash.H tpl_dynSetHash.H ahSort.H ahSearch.H
    tpl_random_queue.H tpl_net_sup_dem.H tpl_bipartite.H graph-traverse.H
    tpl_dynarray_set.H tpl_kgraph.H tpl_dynMat.H
    ahMem.H ahAssert.H ahDefs.H aleph.H euclidian-graph-common.H
    tpl_linearHash.H signal.H ahUtils.H ran_array.h tpl_dynDlist.H
    tpl_skipList.H tpl_tdRbTree.H tpl_splay_tree.H
    tpl_dynTreap.H tpl_union.H tpl_binNode.H tpl_splay_treeRk.H
    useMutex.H protected_cache.H socket_wrappers.H tpl_dynListStack.H
    tpl_autoPtr.H trace.H tpl_cache.H format.H random_net.H
    protected_lhash.H protected_odhash.H log.H useCondVar.H
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
    parse_utils.C
    timeoutQueue.C
    uid.C
    gmpfrxx.C
    euclidian-graph-common.C
    eepicgeom.C
    ahUtils.C
    tpl_sort_utils.C
    hash-fct.C
    hash-dry.C
    aleph-graph.C
    ahNow.C
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