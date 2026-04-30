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
option(ALEPH_USE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
option(ALEPH_USE_LIBCXX "Use libc++ when compiling with Clang" OFF)

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
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND ALEPH_USE_LIBCXX)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
    add_link_options($<$<LINK_LANGUAGE:CXX>:-stdlib=libc++>)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS} ${COMMON_FLAGS}")

if(ALEPH_USE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    else()
        message(WARNING "ALEPH_USE_SANITIZERS is only supported with GCC or Clang")
    endif()
endif()

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
    point.H tpl_treapRk.H array_it.H al-domain.H
    ahAlgo.H
        tpl_odhash.H tpl_nodePool.H random_graph.H
        tpl_olhash.H tpl_dynSetHash.H ahSort.H
        tpl_random_queue.H tpl_net_sup_dem.H tpl_bipartite.H Blossom.H Blossom_Weighted.H
    Min_Cost_Matching.H
    blossom_weighted_mwmatching.H graph-traverse.H
    tpl_dynarray_set.H tpl_kgraph.H tpl_dynMat.H
    ahMem.H ahAssert.H ahDefs.H aleph.H euclidian-graph-common.H
    ahUtils.H ran_array.h tpl_dynDlist.H
    tpl_skipList.H tpl_tdRbTree.H tpl_splay_tree.H
    tpl_dynTreap.H tpl_union.H tpl_binNode.H tpl_splay_treeRk.H
    tpl_b_tree.H tpl_bplus_tree.H tpl_tree_snapshot.H
    tpl_paged_tree_durability.H tpl_paged_value_codec.H
    tpl_file_b_tree.H tpl_file_bplus_tree.H
    tpl_file_b_map.H tpl_file_bplus_map.H
    useMutex.H socket_wrappers.H tpl_dynListStack.H
    trace.H random_net.H
        log.H useCondVar.H
    tpl_dynListQueue.H al-vector.H ahSingleton.H
    treepic_utils.H driven_table.H pointer_table.H Karger.H al-matrix.H
    timeoutQueue.H parse_utils.H ah_stdc++_utils.H eulerian.H
    hamiltonian.H ahTypes.H tpl_arrayStack.H tpl_arrayQueue.H
    tpl_graph.H mat_path.H ahFunction.H ahPair.H
    ahIterator.H grid.H mat_latex.H tpl_sort_utils.H
    polygon.H generate_df_tree.H generate_spanning_tree_picture.H
    archeap.H filter_iterator.H kosaraju.H htlist.H tpl_sgraph.H
    tpl_indexNode.H tpl_indexArc.H tpl_indexGraph.H io_graph.H
    tpl_slist.H tpl_snode.H tpl_lhash.H tpl_dynLhash.H
    tpl_memArray.H tpl_dynArray.H hash-dry.H
    bloom-filter.H ah-dry.H graph-dry.H
    q-consumer-threads.H ah-string-utils.H ah-unique.H ah-comb.H geom_algorithms.H
    ah-source.H
    ah-errors.H
    ah-diagnostics.H
    Compiler_Token.H
    Compiler_Lexer.H
    Compiler_Parser_Utils.H
    Compiler_AST.H
    Compiler_Parser.H
    Compiler_Operator.H
    Compiler_Driver_Contracts.H
    Compiler_Module_Resolver.H
    Compiler_Module_Metadata.H
    Compiler_Module_Linker.H
    Compiler_Module_Binder.H
    Compiler_Module_Name_Table.H
    Compiler_Module_Name_Resolver.H
    Compiler_Module_Name_Diagnostics.H
    Compiler_Module_Semantic_Environment.H
    Compiler_Frontend.H
    Compiler_MVP_Frontend.H
    Compiler_Generic_Driver.H
    Compiler_Line_Frontend.H
    tpl_scope.H
    Compiler_Symbol_Bindings.H
    Compiler_Sema.H
    Compiler_Types.H
    tpl_constraints.H
    Compiler_Typed_Sema.H
    Compiler_HIR_Model.H
    Compiler_HIR_Builder.H
    Compiler_HIR_Lowering_MVP.H
    Compiler_HIR.H
    Interpreter_Runtime.H
    Compiler_CFG.H
    Compiler_IR_Model.H
    Compiler_IR_Builder.H
    Compiler_IR_Lowering_MVP.H
    Compiler_IR.H
    Compiler_Dataflow.H
    SSA.H
    Bytecode.H
    Bytecode_Interpreter.H
    Compiler_Backend_C.H
    Compiler_Driver.H
    String_Search.H
    Aho_Corasick.H
    Suffix_Structures.H
    String_Palindromes.H
    String_DP.H
    String_Algorithms.H
    Knapsack.H
    LIS.H
    DP_Optimizations.H
    Matrix_Chain.H
    Subset_Sum.H
    Tree_DP.H
    tikzgeom.H
    tikzgeom_algorithms.H
    tikzgeom_scene.H
    tpl_fenwick_tree.H
    tpl_segment_tree.H
    tpl_cartesian_tree.H
    K_Shortest_Paths.H
    Min_Mean_Cycle.H
    Planarity_Test.H
    Tree_Decomposition.H
    ah-concepts.H
    LCA.H
    HLD.H
    Dominators.H
    Hungarian.H
    tpl_interval_tree.H
    search_move_ordering.H
    state_search_common.H
    Transposition_Table.H
    Backtracking.H
    Branch_And_Bound.H
    Negamax.H
    Alpha_Beta.H
    State_Search.H
    State_Search_IDA_Star.H
    Bellman_Ford.H
    Dijkstra.H
    Tarjan.H
    ah-arena.H
    ah-args-ctor.H
    ah-dry-mixin.H
    ah-iterator.H
    ah-ranges.H
    ah-time.H
    ah-zip.H
    ahDry.H
    ahFunctional.H
    ah_init_guard.H
    aleph-graph.H
    array_utils.H
    avlNode.H
    avlNodeRk.H
    bitArray.H
    cookie_guard.H
    dlink.H
    gmpfrxx.h
    line.H
    hash-fct.H
    hashDry.H
    mpfr_mul_d.h
    primes.H
    rbNode.H
    rbNodeRk.H
    shortest_path_common.H
    single_graph.H
    slink.H
    topological_sort.H
    tpl_2dtree.H
    tpl_agraph.H
    tpl_array.H
    tpl_avl.H
    tpl_avlRk.H
    tpl_binNodeUtils.H
    tpl_binNodeXt.H
    tpl_binTree.H
    tpl_binTreeOps.H
    tpl_components.H
    tpl_cut_nodes.H
    tpl_dnode.H
    tpl_dynList.H
    tpl_dynMapOhash.H
    tpl_dynMapTree.H
    tpl_dynSkipList.H
    tpl_fibonacci_heap.H
    tpl_find_path.H
    tpl_graph_utils.H
    tpl_hRbTree.H
    tpl_hRbTreeRk.H
    tpl_hash.H
    tpl_linHash.H
    tpl_matgraph.H
    tpl_net.H
    tpl_randNode.H
    tpl_rand_tree.H
    tpl_rbRk.H
    tpl_rb_tree.H
    tpl_sparse_table.H
    tpl_tdRbTreeRk.H
    tpl_treap.H
    treapNode.H
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

if(BUILD_TESTS)
    enable_testing()
endif()

# Add subdirectories if enabled
if(BUILD_EXAMPLES)
    add_subdirectory(Examples)
endif()

if(BUILD_TESTS)
    add_subdirectory(Tests)
endif()

# Print configuration summary
message(STATUS "Aleph-w Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Sanitizers: ${ALEPH_USE_SANITIZERS}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
