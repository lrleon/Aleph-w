cmake_minimum_required(VERSION 3.18)

project(Aleph-Tests CXX)

# Set C++23 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(WARN_FLAGS "-Wall -Wextra -Wcast-align -Wno-sign-compare -Wno-write-strings -Wno-parentheses")
set(COMMON_FLAGS "-DMESSAGES -D_GLIBCXX__PTHREADS -D_REENTRANT")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(COMMON_FLAGS "${COMMON_FLAGS} -D__extern_always_inline=\"extern __always_inline\"")
endif()

# Debug flags with sanitizers (optional)
option(USE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
if(USE_SANITIZERS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -O0 -g3")

# Include parent directory for Aleph headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

if(NOT TARGET Aleph)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/aleph)
endif()

# Link directory for Aleph library
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# Enable CTest - must be called before any add_test()
enable_testing()

# Include GoogleTest module for gtest_discover_tests()
include(GoogleTest)

# When this directory is configured standalone (without the root
# CMakeLists setting ALEPH_SYSTEM_LIBS), define the same defaults here so
# that tests depending on Aleph's transitive libraries link correctly.
if(NOT DEFINED ALEPH_SYSTEM_LIBS)
    set(ALEPH_SYSTEM_LIBS
        gmpxx
        gmp
        pthread
        mpfr
        gsl
        gslcblas
        m
        X11
        Threads::Threads)
endif()

# Allow automatically fetching GoogleTest if it is not present system-wide.
option(ALEPH_FETCH_GTEST "Download GoogleTest automatically when not found" ON)

# Find Google Test
find_package(GTest QUIET)

# If not found, try to fetch it via FetchContent (unless disabled).
if(NOT GTest_FOUND AND ALEPH_FETCH_GTEST)
    include(FetchContent)
    set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
    message(STATUS "GoogleTest not found locally; fetching from upstream (set ALEPH_FETCH_GTEST=OFF to disable).")
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # Prevent GoogleTest from overriding our global compiler/linker options.
    set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    find_package(GTest REQUIRED CONFIG) # Targets now exist within the build.
endif()

if(NOT GTest_FOUND)
    if(DEFINED ALEPH_ALLOW_TESTS_WITHOUT_GTEST AND ALEPH_ALLOW_TESTS_WITHOUT_GTEST)
        message(WARNING
            "GoogleTest was not found; skipping Aleph tests. "
            "Install GTest or set BUILD_TESTS=OFF to silence this warning.")
        return()
    else()
        message(FATAL_ERROR
            "GoogleTest is required to build Aleph tests. "
            "Install GTest (e.g. libgtest-dev), set GOOGLETEST to its path, "
            "or enable ALEPH_FETCH_GTEST.")
    endif()
endif()

# GoogleMock is not always installed alongside GTest. Some tests (e.g.
# ringfilecache.cc) require gmock headers. Detect availability and skip those
# targets if gmock is missing.
set(ALEPH_HAVE_GMOCK OFF)
if(TARGET GTest::gmock)
    set(ALEPH_HAVE_GMOCK ON)
elseif(EXISTS "/usr/include/gmock/gmock.h")
    set(ALEPH_HAVE_GMOCK ON)
elseif(DEFINED ENV{GOOGLETEST} AND EXISTS "$ENV{GOOGLETEST}/googlemock/include/gmock/gmock.h")
    set(ALEPH_HAVE_GMOCK ON)
endif()

# Note: You may need to set GOOGLETEST environment variable or specify the path
# export GOOGLETEST=/path/to/googletest
# or set it in CMake: cmake -DGOOGLETEST=/path/to/googletest ..

if(DEFINED ENV{GOOGLETEST})
    include_directories($ENV{GOOGLETEST}/googletest/include)
    include_directories($ENV{GOOGLETEST}/googlemock/include)
endif()

# List of all test programs from Imakefile
set(TEST_PROGRAMS
    array
    array_utils
    array-it
    bitarray
    bloom-filter
    dynarray
    floyd
    karger
    union
    sort_utils
    memarray
    slinknc
    snodenc
    htlist
    dynlist
    dlink
    dnode
    dyndlist
    arraystack
    fixedstack
    dynliststack
    arrayqueue
    dynlistqueue
    bin-node
    bin-node-utils
    bin-node-xt
    splay-tree
    splay-tree-rk
    rb-tree
    dynsettree
    dynsethash
    rand-tree
    lin-hash
    bintree
    avl
    avl-rb-rk
    binheap
    tree-node
    odhash
    olhash
    dynsetohash
    hash-it
    ah-zip
    ah-functional
    ah-dry
    ah-dry-mixin_test
    tree-it
    al-domain
    al-vector
    al-matrix
    ah-arena
    ah-string-utils
    ah-stl-utils
    ahAlgo
    ah_init_guard
    ah-utils
    index-node
    sort_arrays
    primes
    generate_tree
    graph_visualization_test
    latex_floyd_test
    tpl_matgraph_test
    parse_utils_test
    fixedqueue
    randomqueue
    slink
    snode
    slist
    dynslist
    arrayheap
    arrayheap_algos
    dynarrayheap
    ringfilecache
    warshall_test
    tpl_indexArc_test
    tpl_dynarray_set_test
    random_graph
    tarjan_test
    test_bellman_ford
    test_dijkstra
    test_kosaraju
    test_filter_iterator
    dynset_tree_test
    ah-comb
    ah-convert
    polygon_test
    parse_csv_test
    line_test
    mat_latex_path_test
    io_graph_test
    huffman_btreepic_test
    stat_utils_test
    ah_parallel_test
    topological_sort_test
    huffman
	    test_tpl_hash_macros
	    bipartite_test
	    dlink_test
	    ah_iterator_test
	    tpl_graph_utils_test
	    tpl_test_cycle
    tpl_spanning_tree_test
    tpl_components_test
    ah-stl-zip_test
    ah-zip-utils_test
    ah-stl-functional_test
    ah-uni-functional_test
    ah_date_test
    ah_mem_test
    ah_msg_test
    load_digraph_test
    test_al_vector
    archeap_test
    al_matrix_test
    cut_nodes_test
    graph_indexes_test
    graph_scenarios_test
    graph_stress_test
    tpl_net_test
    net_sup_dem_test
    tpl_netcost_test
    tpl_multicommodity_test
    tpl_kgraph_test
    tpl_agraph_test
    test_kruskal
    prim_test
    tpl_sgraph_test
    tpl_graph_test
    Simplex_test
    prefix_tree_test
    skiplist_test
    dynskiplist_test
    dynmat_test
    dynmapohash_test
    ah-mapping_test
    ahSingleton_test
    ah_signal_test
    ah_arena_test
    ah_stdc_utils_test
    tpl_dynMapTree_test
    euclidian_graph_common_test
    future_utils_test
    fibonacci_heap_test
    quadtree_test
    container_edge_cases_test
    netcapgraph_test
    tdrbtree_test
    tdrbtreerk_test
    htdrbtreerk_test
    hash_fct_test
    point_test
    segment_test
    ranges_iterator_test
    ah_ranges_test
    eepicgeom_test
    ahSort_test
    cookie_guard_test
    pointer_table_test
    graph_functional_test
    johnson_test
    graph_copy_test
    thread_pool_test
    timeoutQueue_test
    graph_traverse_test
    test_ah_errors
    dynliststack_test
    tpl_dynListQueue
    tpl_dynListStack
    astar_test
    tpl_maxflow_test
    net_utils_test
    tpl_mincost_test
    net_apps_test
    random_network_generator_test
    eulerian_test
    hamiltonian_test
    tpl_dynLhash_test
    tpl_random_queue_test
    rb_tree_test
    treap_test
    random_tree_test
    random_net_test
    euclidian_graph_test
    find_path_test
    index_graph_test
    test_acyclique_test
    test_connectivity_test
    test_path_test
    ah_mapping_test
    lfit_test
    dyn_treap_test
    geom_algorithms_test
    generate_df_tree_test
    index_graph_intensive_test
    driven_table_test
    uid_test
    fenwick_tree
    tpl_segment_tree_test
    tpl_cartesian_tree_test
    tpl_mo_algorithm_test
    tpl_mo_on_trees_test
    ah_concepts_test
    robust_predicates_test
)

# k2tree_test uses gmpfrxx which has ABI incompatibility with libc++
# (system GMP is compiled with libstdc++). Only include for GCC.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND TEST_PROGRAMS k2tree_test)
endif()

# Create executable for each test program
set(_ALEPH_TEST_EXECUTABLE_TARGETS "")
foreach(test_program ${TEST_PROGRAMS})
    # Check if source file exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${test_program}.cc")
        # Avoid collisions with targets created elsewhere in the project (e.g. Examples/).
        set(test_exe ${test_program})
        if(TARGET ${test_exe})
            set(test_exe tests_${test_exe})
        endif()

        if(test_program STREQUAL "geom_algorithms_test")
            add_executable(${test_exe}
                ${test_program}.cc
                geom_algorithms_test_triangulation_hulls_delaunay.cc
                geom_algorithms_test_edgecases_sweepline_minkowski_kdtree.cc
                geom_algorithms_test_decomp_rangetree_visibility_shortestpath_arrangement.cc
                geom_algorithms_test_boolean_3d_serializer_aabb_misc.cc
            )
        else()
            add_executable(${test_exe} ${test_program}.cc)
        endif()
        list(APPEND _ALEPH_TEST_EXECUTABLE_TARGETS ${test_exe})

        # Link with Aleph library and system libraries
        target_link_libraries(${test_exe} PRIVATE
            Aleph
            GTest::gtest_main
            GTest::gtest
            ${ALEPH_SYSTEM_LIBS}
        )

        # Tests intentionally discard some [[nodiscard]] results (e.g., when only
        # verifying side-effects or exception behavior). Keep this warning for
        # library/users, but silence it for test targets.
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${test_exe} PRIVATE -Wno-unused-result)
        endif()

        # Add test to CTest (gtest_discover_tests for IDE integration)
        gtest_discover_tests(${test_exe} DISCOVERY_MODE PRE_TEST)

        # Optional: Install test executable
        # install(TARGETS ${test_program} DESTINATION bin/tests)
    else()
        message(WARNING "Source file not found for test: ${test_program}.cc")
    endif()
endforeach()

# Create executable for each exhaustive test program (files ending in _test.cc)
# These tests are for header-only classes and need minimal dependencies
foreach(test_program ${EXHAUSTIVE_TESTS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${test_program}.cc")
        add_executable(${test_program} ${test_program}.cc)
        list(APPEND _ALEPH_TEST_EXECUTABLE_TARGETS ${test_program})

        # Most exhaustive tests are header-only and need minimal dependencies.
        # Some (like tpl_graph_utils_test and test_al_vector) exercise algorithms
        # that depend on symbols provided by libAleph.
        if(test_program STREQUAL "ah_iterator_test")
            target_link_libraries(${test_program} PRIVATE
                Aleph
                GTest::gtest_main
                GTest::gtest
                ${ALEPH_SYSTEM_LIBS}
            )
        elseif(test_program STREQUAL "bipartite_test" OR test_program STREQUAL "tpl_graph_utils_test" OR test_program STREQUAL "tpl_test_cycle" OR test_program STREQUAL "tpl_spanning_tree_test" OR test_program STREQUAL "tpl_components_test" OR test_program STREQUAL "ah-zip-utils_test" OR test_program STREQUAL "ah-uni-functional_test" OR test_program STREQUAL "ah_date_test" OR test_program STREQUAL "ah_mem_test" OR test_program STREQUAL "ah_msg_test" OR test_program STREQUAL "test_al_vector" OR test_program STREQUAL "archeap_test" OR test_program STREQUAL "al_matrix_test" OR test_program STREQUAL "cut_nodes_test" OR test_program STREQUAL "graph_indexes_test" OR test_program STREQUAL "graph_scenarios_test" OR test_program STREQUAL "tpl_net_test" OR test_program STREQUAL "net_sup_dem_test" OR test_program STREQUAL "tpl_netcost_test" OR test_program STREQUAL "tpl_multicommodity_test" OR test_program STREQUAL "tpl_kgraph_test" OR test_program STREQUAL "tpl_agraph_test" OR test_program STREQUAL "test_kruskal" OR test_program STREQUAL "prim_test" OR test_program STREQUAL "tpl_sgraph_test" OR test_program STREQUAL "tpl_graph_test" OR test_program STREQUAL "load_digraph_test" OR test_program STREQUAL "Simplex_test" OR test_program STREQUAL "prefix_tree_test" OR test_program STREQUAL "skiplist_test" OR test_program STREQUAL "dynskiplist_test" OR test_program STREQUAL "dynmat_test" OR test_program STREQUAL "dynmapohash_test" OR test_program STREQUAL "ah-mapping_test" OR test_program STREQUAL "tpl_dynMapTree_test" OR test_program STREQUAL "euclidian_graph_common_test" OR test_program STREQUAL "future_utils_test" OR test_program STREQUAL "quadtree_test" OR test_program STREQUAL "k2tree_test" OR test_program STREQUAL "container_edge_cases_test" OR test_program STREQUAL "netcapgraph_test" OR test_program STREQUAL "tdrbtree_test" OR test_program STREQUAL "tdrbtreerk_test" OR test_program STREQUAL "htdrbtreerk_test" OR test_program STREQUAL "hash_fct_test" OR test_program STREQUAL "point_test" OR test_program STREQUAL "segment_test" OR test_program STREQUAL "hash_cache_test" OR test_program STREQUAL "cache_test" OR test_program STREQUAL "ranges_iterator_test" OR test_program STREQUAL "ah_ranges_test" OR test_program STREQUAL "eepicgeom_test" OR test_program STREQUAL "pointer_table_test" OR test_program STREQUAL "graph_functional_test" OR test_program STREQUAL "cookie_guard_test" OR test_program STREQUAL "graph_stress_test" OR test_program STREQUAL "fibonacci_heap_test" OR test_program STREQUAL "ahSingleton_test" OR test_program STREQUAL "johnson_test" OR test_program STREQUAL "graph_copy_test" OR test_program STREQUAL "graph_traverse_test" OR test_program STREQUAL "test_ah_errors" OR test_program STREQUAL "dynliststack_test" OR test_program STREQUAL "tpl_dynListQueue" OR test_program STREQUAL "tpl_dynListStack" OR test_program STREQUAL "ahSort_test" OR test_program STREQUAL "astar_test" OR test_program STREQUAL "tpl_maxflow_test" OR test_program STREQUAL "net_utils_test" OR test_program STREQUAL "tpl_mincost_test" OR test_program STREQUAL "net_apps_test" OR test_program STREQUAL "random_network_generator_test" OR test_program STREQUAL "eulerian_test" OR test_program STREQUAL "hamiltonian_test" OR test_program STREQUAL "tpl_dynLhash_test" OR test_program STREQUAL "tpl_random_queue_test" OR test_program STREQUAL "rb_tree_test" OR test_program STREQUAL "treap_test" OR test_program STREQUAL "random_tree_test" OR test_program STREQUAL "random_net_test" OR test_program STREQUAL "euclidian_graph_test" OR test_program STREQUAL "find_path_test" OR test_program STREQUAL "index_graph_test" OR test_program STREQUAL "test_acyclique_test" OR test_program STREQUAL "test_connectivity_test" OR test_program STREQUAL "test_path_test" OR test_program STREQUAL "lfit_test" OR test_program STREQUAL "ah_mapping_test" OR test_program STREQUAL "dyn_treap_test" OR test_program STREQUAL "geom_algorithms_test" OR test_program STREQUAL "robust_predicates_test" OR test_program STREQUAL "generate_df_tree_test" OR test_program STREQUAL "index_graph_intensive_test" OR test_program STREQUAL "driven_table_test" OR test_program STREQUAL "uid_test")
            target_link_libraries(${test_program} PRIVATE
                Aleph
                GTest::gtest_main
                GTest::gtest
                gsl
                gslcblas
                ${ALEPH_SYSTEM_LIBS}
            )
        else()
            target_link_libraries(${test_program} PRIVATE
                GTest::gtest_main
                GTest::gtest
                Threads::Threads
            )
        endif()

        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${test_program} PRIVATE -Wno-unused-result)
        endif()

        gtest_discover_tests(${test_program} DISCOVERY_MODE PRE_TEST)
    else()
        message(WARNING "Source file not found for exhaustive test: ${test_program}.cc")
    endif()
endforeach()

# -----------------------------------------------------------------------------
# Auto-register any remaining test sources in this directory.
#
# Goal: if a new *.cc file is added under Tests/, it should automatically become
# a runnable target (and a CTest entry) so IDEs can discover it without having
# to update the lists above.
#
# We keep the explicit lists above as authoritative (they allow custom linking
# decisions). This block only creates targets for sources that were not already
# handled.
# -----------------------------------------------------------------------------

set(_ALEPH_KNOWN_TEST_TARGETS "${TEST_PROGRAMS};${EXHAUSTIVE_TESTS}")

file(GLOB _ALEPH_ALL_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")
foreach(_src ${_ALEPH_ALL_TEST_SOURCES})
    get_filename_component(_name "${_src}" NAME_WE)

    # Skip k2tree_test with clang (GMP/libc++ incompatibility)
    if(_name STREQUAL "k2tree_test" AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        continue()
    endif()

    # Skip files already registered by the lists above.
    list(FIND _ALEPH_KNOWN_TEST_TARGETS "${_name}" _known_idx)
    if(NOT _known_idx EQUAL -1)
        continue()
    endif()

    # Avoid collisions with targets created elsewhere in the project.
    set(_auto_target "${_name}")
    if(TARGET ${_auto_target})
        set(_auto_target "tests_${_auto_target}")
    endif()

    if(NOT TARGET ${_auto_target})
        add_executable(${_auto_target} "${_src}")
        list(APPEND _ALEPH_TEST_EXECUTABLE_TARGETS ${_auto_target})

        # Default linking: prefer the full Aleph link set so most tests "just work".
        # This is intentionally conservative (more deps) to maximize executability.
        target_link_libraries(${_auto_target} PRIVATE
            Aleph
            GTest::gtest_main
            GTest::gtest
            ${ALEPH_SYSTEM_LIBS}
        )

        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${_auto_target} PRIVATE -Wno-unused-result)
        endif()

        gtest_discover_tests(${_auto_target} DISCOVERY_MODE PRE_TEST)
    endif()
endforeach()

add_custom_target(aleph_build_tests DEPENDS ${_ALEPH_TEST_EXECUTABLE_TARGETS})
add_custom_target(aleph_check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS aleph_build_tests
)

message(STATUS "Configured ${CMAKE_PROJECT_NAME} with Google Test")
message(STATUS "  Standard test programs: ${TEST_PROGRAMS}")
message(STATUS "  Exhaustive test programs: ${EXHAUSTIVE_TESTS}")
message(STATUS "  Use 'make test' or 'ctest' to run all tests")
message(STATUS "  Use 'ctest -R _test' to run only exhaustive tests")
