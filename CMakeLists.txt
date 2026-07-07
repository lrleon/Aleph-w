cmake_minimum_required(VERSION 3.18)

# Canonical version lives in the top-level VERSION.txt file (SemVer 2.0). CMake,
# packaging recipes and the release workflow all read it from there so the
# number is declared in exactly one place. The .txt extension avoids shadowing
# the standard C++20 <version> header on case-insensitive filesystems (macOS).
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt" ALEPH_VERSION LIMIT_COUNT 1)
string(STRIP "${ALEPH_VERSION}" ALEPH_VERSION)

project(Aleph-w VERSION ${ALEPH_VERSION} LANGUAGES C CXX)

# ------------------------------------------------------------------------------
# Compiler / standard configuration
# ------------------------------------------------------------------------------

# C++20 is the baseline: the library uses concepts/requires throughout, so it
# does not build under C++17 (despite earlier listings). C++23 is supported and
# guarded by the `cxx23-build` CI job.
set(ALEPH_SUPPORTED_CXX_STANDARDS 20 23)
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
option(BUILD_REPRODUCTIONS "Build weekly cellular-automata reproductions" OFF)
option(BUILD_BENCHMARKS "Build cellular-automata performance-gate benchmarks" OFF)
option(ALEPH_USE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
option(ALEPH_USE_LIBCXX "Use libc++ when compiling with Clang" OFF)
option(ALEPH_DISABLE_RANGES "Force C++20 std::ranges support off (portability escape hatch)" OFF)

# Optional POSIX-specific features. Default ON on Unix-like platforms,
# OFF on Windows so CI build jobs can opt-in without dragging X11 / xorg.
if(WIN32)
    set(_ALEPH_X11_DEFAULT OFF)
else()
    set(_ALEPH_X11_DEFAULT ON)
endif()
option(ALEPH_BUILD_X11_VIEWER
       "Build the X11 live viewer (ca-x11-viewer.H). Requires libX11."
       ${_ALEPH_X11_DEFAULT})

# Optional Python bindings (Phase 18 of the CA industrial roadmap).
# Opt-in: scikit-build-core sets it ON when building the wheel via
# `python/pyproject.toml`. Developers building from CMake directly can
# enable it with `-DALEPH_BUILD_PYTHON=ON`.
option(ALEPH_BUILD_PYTHON
       "Build the aleph_ca Python module (requires Python3 + pybind11)."
       OFF)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    if(BUILD_OPTIMIZED)
        set(CMAKE_BUILD_TYPE Release)
    else()
        set(CMAKE_BUILD_TYPE Debug)
    endif()
endif()

# Compiler flags
#
# MSVC (cl and clang-cl, both with MSVC == TRUE) rejects GNU-style flags
# such as -Wextra or -g3 with hard errors (D8021), so every flag below is
# split per frontend.
if(MSVC)
    set(WARN_FLAGS "/W3")
    # MSVC's <cmath> only exposes M_PI, M_PI_2 and friends when
    # _USE_MATH_DEFINES is set before the header is first included. Define it
    # on the command line so it applies to every translation unit (cl and
    # clang-cl both use the Microsoft STL headers).
    #
    # /FIah-compat.H force-includes the cross-compiler shim header into every
    # translation unit (it maps GCC/Clang builtins missing from cl). Doing it
    # here, rather than per-header, guarantees the shims are visible regardless
    # of each file's include graph. The repo root is on the include path
    # (include_directories below), so the relative name resolves.
    # NOMINMAX suppresses the Windows.h min/max macros that conflict with
    # std::numeric_limits<T>::max() and std::min/max (C2059/macro-arg errors).
    set(COMMON_FLAGS "-DMESSAGES -D_USE_MATH_DEFINES -DNOMINMAX /FIah-compat.H")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # clang-cl optimizes under strict-aliasing rules like clang;
        # htlist.H requires them disabled (see the Release flags note below).
        set(COMMON_FLAGS "${COMMON_FLAGS} -fno-strict-aliasing")
    endif()
else()
    set(WARN_FLAGS "-Wall -Wextra -Wcast-align -Wno-sign-compare -Wno-write-strings -Wno-parentheses")
    set(COMMON_FLAGS "-DMESSAGES -D_GLIBCXX__PTHREADS -D_REENTRANT")
endif()
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

# Portability escape hatch: force std::ranges support off. The library still
# auto-detects ranges by default (see ah-ranges.H); this option lets users on
# toolchains whose std::ranges is present but unreliable opt out globally.
if(ALEPH_DISABLE_RANGES)
    add_compile_definitions(ALEPH_NO_RANGES)
endif()

# Debug flags
# (MSVC keeps CMake's defaults: /Od /Zi /RTC1 for Debug, /O2 /Ob2 /DNDEBUG
# for Release. MSVC's cl performs no type-based aliasing optimizations, so
# the -fno-strict-aliasing requirement below does not apply to it; for
# clang-cl it is handled in COMMON_FLAGS above.)
if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")
else()
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
endif()

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
    blossom_weighted_mwmatching.H graph-traverse.H graph-traverse-generators.H
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
    q-consumer-threads.H ah-string-utils.H ah-unique.H ah-comb.H ah-comb-generators.H geom_algorithms.H
    ah-source.H
    ah-errors.H
    ah-cpp-compat.H
    ah-diagnostics.H
    ah-generator.H
    ah-mdspan.H
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
    tpl_binNodeGenerators.H
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
    tpl_flat_map.H
    tpl_flat_set.H
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
    tpl_ring_buffer.H
    tpl_small_vector.H
    tpl_sparse_table.H
    tpl_tdRbTreeRk.H
    tpl_treap.H
    treapNode.H
    ca-traits.H
    tpl_ca_concepts.H
    tpl_ca_storage.H
    tpl_ca_bit_storage.H
    tpl_ca_lattice.H
    tpl_ca_neighborhood.H
    tpl_ca_rule.H
    tpl_ca_engine.H
    ca-engine-utils.H
    ca-c-api.h
    tpl_ca_ghost_lattice.H
    ca-tiling.H
    ca-bench.H
    thread_pool.H
    tpl_mpsc_queue.H
    tpl_ca_parallel_engine.H
    tpl_ca_hex_lattice.H
    tpl_ca_triangular_lattice.H
    tpl_ca_graph_automaton.H
    ca-metrics.H
    ca-observer.H
    ca-rng.H
    tpl_ca_stochastic_rules.H
    ca-kernels.H
    tpl_ca_continuous_rules.H
    tpl_ca_hashlife.H
    tpl_ca_update_scheme.H
    tpl_ca_block_rule.H
    tpl_ca_async_engine.H
    tpl_ca_multi_field_lattice.H
    tpl_ca_multi_field_rule.H
    tpl_ca_multi_field_engine.H
    ca-checkpoint.H
    ca-checkpoint-compress.H
    ca-streaming-storage.H
    ca-io.H
    ca-tikz.H
    ca-frame-stream.H
    ca-png.H
    ca-gif.H
    ca-svg.H
    ca-npy.H
    ca-vtk.H
    ca-html-player.H
    ca-ffmpeg-sink.H
    ca-dot.H
    ca-ascii.H
    ca-x11-viewer.H
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
    ca-checkpoint-compress.C
)

# C source files
set(LIBCSRCS
    ran_array.c
    mpfr_mul_d.c
)

if(WIN32)
    # POSIX-only translation units: fork()/setsid() daemonization, BSD
    # sockets, pthread wrappers and the pthread-based timeout queue. They
    # stay out of the Windows build until ported.
    #
    # gmpfrxx.C is excluded too: it wraps the GMP/MPFR C++ interface (gmpxx),
    # whose C++ ABI is incompatible between the MSYS2/mingw libraries and the
    # MSVC/clang-cl toolchains. Its only consumers (k2tree) are already gated
    # to GCC, so dropping it here keeps the Windows library buildable.
    list(REMOVE_ITEM LIBCCCSRCS
        ahDaemonize.C
        socket_wrappers.C
        useMutex.C
        timeoutQueue.C
        gmpfrxx.C
    )
endif()

# ------------------------------------------------------------------------------
# Vendored third_party/miniz (DEFLATE backend for ca-checkpoint compression)
# ------------------------------------------------------------------------------
# miniz is a single-source, zlib-API-compatible DEFLATE implementation.
# We build it as a tiny static archive so the C symbols stay contained and
# never leak into headers consumed by downstream users of Aleph-w.
# License (zlib/MIT) recorded in THIRD_PARTY_NOTICES.md.
add_library(aleph_miniz STATIC third_party/miniz/miniz.c)
set_target_properties(aleph_miniz PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_STANDARD 99)
target_include_directories(aleph_miniz PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniz>
    $<INSTALL_INTERFACE:include/aleph/third_party/miniz>)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(aleph_miniz PRIVATE -w)
endif()

# Create static library
add_library(Aleph STATIC ${LIBCCCSRCS} ${LIBCSRCS})
target_link_libraries(Aleph PRIVATE aleph_miniz)

# Homebrew lives outside the default compiler search path on macOS — and on
# Apple Silicon it lives at `/opt/homebrew`, not `/usr/local`. The gmp /
# mpfr / gsl libraries are consumed via raw `-l` flags (see
# `ALEPH_SYSTEM_LIBS` below), so CMAKE_PREFIX_PATH alone doesn't propagate
# the include/lib directories to the compiler. Query `brew --prefix` once
# at configure time and add the resulting paths explicitly.
if(APPLE)
    execute_process(
        COMMAND brew --prefix
        OUTPUT_VARIABLE _ALEPH_BREW_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _ALEPH_BREW_RC
        ERROR_QUIET
    )
    if(_ALEPH_BREW_RC EQUAL 0 AND EXISTS "${_ALEPH_BREW_PREFIX}")
        message(STATUS "Aleph: Homebrew prefix detected at ${_ALEPH_BREW_PREFIX}")
        target_include_directories(Aleph PUBLIC "${_ALEPH_BREW_PREFIX}/include")
        target_link_directories(Aleph PUBLIC "${_ALEPH_BREW_PREFIX}/lib")
    endif()
endif()

# On Windows the gmp / mpfr / gsl headers and import libraries come from the
# MSYS2 mingw64 tree, passed to CMake via CMAKE_PREFIX_PATH. As with the
# Homebrew case above, those C libraries are consumed through raw -l names
# (see ALEPH_SYSTEM_LIBS), so CMAKE_PREFIX_PATH alone does not place their
# include / lib directories on the compiler command line. Add them explicitly.
if(WIN32)
    foreach(_aleph_prefix IN LISTS CMAKE_PREFIX_PATH)
        if(EXISTS "${_aleph_prefix}/include")
            target_include_directories(Aleph PUBLIC "${_aleph_prefix}/include")
        endif()
        if(EXISTS "${_aleph_prefix}/lib")
            target_link_directories(Aleph PUBLIC "${_aleph_prefix}/lib")
        endif()
    endforeach()
endif()

# Find required libraries
find_package(Threads REQUIRED)

# X11 is only required when the live X11 viewer is enabled. On Windows or
# when the user opts out, we skip X11 entirely and the viewer falls back
# to its no-op stub. The `__has_include(<X11/Xlib.h>)` guard in
# `ca-x11-viewer.H` is not enough on macOS: when XQuartz headers happen
# to be on the include path (e.g. brought in by `brew --prefix`), the
# header would activate the X11 code path even though we never link
# `-lX11`. Pass `ALEPH_DISABLE_X11_VIEWER=1` explicitly so the option
# wins over auto-detection.
set(_ALEPH_X11_LIBS "")
if(ALEPH_BUILD_X11_VIEWER)
    find_package(X11 REQUIRED)
    set(_ALEPH_X11_LIBS ${X11_LIBRARIES})
else()
    target_compile_definitions(Aleph PUBLIC ALEPH_DISABLE_X11_VIEWER=1)
endif()

# pthread and libm are POSIX-only conventions; on Windows the
# Threads::Threads target already pulls the correct synchronization
# runtime and the math functions live in the CRT.
if(WIN32)
    # GSL_DLL tells GSL headers to emit __declspec(dllimport) on global
    # data variables (e.g. gsl_rng_mt19937). Without it, lld-link fails
    # to resolve imported data symbols from the GNU import library (.dll.a).
    target_compile_definitions(Aleph PUBLIC GSL_DLL)
    set(_ALEPH_PTHREAD "")
    set(_ALEPH_LIBM "")
else()
    set(_ALEPH_PTHREAD pthread)
    set(_ALEPH_LIBM m)
endif()

if(WIN32)
    # MSYS2 mingw64 ships GNU import libraries (libXXX.dll.a), not the
    # MSVC-style name (XXX.lib) that CMake's XXX → XXX.lib translation
    # produces.  Both link.exe and lld-link can consume .dll.a when referenced
    # by full path, so resolve each library to its actual file now.
    set(_aleph_win_math_libs "")
    set(_aleph_win_found "")
    foreach(_aleph_prefix IN LISTS CMAKE_PREFIX_PATH)
        foreach(_lname gmpxx gmp mpfr gsl gslcblas)
            if(NOT "${_lname}" IN_LIST _aleph_win_found)
                # GMP and MPFR static archives (.a) contain standard COFF x64
                # objects and link cleanly with both link.exe and lld-link.
                # GSL's static archive pulls in MinGW-specific runtime symbols
                # (__mingw_vfprintf, __stack_chk_fail) that MSVC cannot resolve,
                # so for GSL/GSLCBLAS we use the GNU import library (.dll.a)
                # instead; GSL_DLL is defined below so headers emit
                # __declspec(dllimport) on data variables (e.g. gsl_rng_mt19937).
                # MSYS2 headers for GMP, MPFR, and GSL all use __declspec(dllimport)
                # because those packages were built as DLLs. The static .a archives
                # cannot satisfy __imp_* import symbols, so we must use the GNU
                # import libraries (.dll.a). Both MSVC link.exe and lld-link can
                # read .dll.a (COFF-format AR archives of import stubs).
                # An MSVC-style .lib takes priority if one happens to exist.
                if(EXISTS "${_aleph_prefix}/lib/${_lname}.lib")
                    list(APPEND _aleph_win_math_libs "${_aleph_prefix}/lib/${_lname}.lib")
                    list(APPEND _aleph_win_found "${_lname}")
                elseif(EXISTS "${_aleph_prefix}/lib/lib${_lname}.dll.a")
                    list(APPEND _aleph_win_math_libs "${_aleph_prefix}/lib/lib${_lname}.dll.a")
                    list(APPEND _aleph_win_found "${_lname}")
                elseif(EXISTS "${_aleph_prefix}/lib/lib${_lname}.a")
                    list(APPEND _aleph_win_math_libs "${_aleph_prefix}/lib/lib${_lname}.a")
                    list(APPEND _aleph_win_found "${_lname}")
                endif()
            endif()
        endforeach()
    endforeach()
    set(ALEPH_SYSTEM_LIBS
        ${_aleph_win_math_libs}
        ${_ALEPH_X11_LIBS}
        Threads::Threads
        CACHE INTERNAL "System libraries required by Aleph"
    )
else()
    set(ALEPH_SYSTEM_LIBS
        gmpxx
        gmp
        ${_ALEPH_PTHREAD}
        mpfr
        gsl
        gslcblas
        ${_ALEPH_LIBM}
        ${_ALEPH_X11_LIBS}
        Threads::Threads
        CACHE INTERNAL "System libraries required by Aleph"
    )
endif()

# Link system libraries
target_link_libraries(Aleph PUBLIC ${ALEPH_SYSTEM_LIBS})

# Set include directories for the library
target_include_directories(Aleph PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/aleph>
)

# ------------------------------------------------------------------------------
# Installation, export set and CMake package config (roadmap Phase 21)
# ------------------------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Header-only cellular-automata component. It carries no sources of its own;
# it simply links the core library and is exported under the Aleph::CA name so
# consumers can `find_package(Aleph COMPONENTS CA)`.
add_library(Aleph_CA INTERFACE)
target_link_libraries(Aleph_CA INTERFACE Aleph)
target_include_directories(Aleph_CA INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/aleph>)
# Export under the Aleph::CA name (the target itself is Aleph_CA to avoid a
# bare `CA` symbol in the build tree).
set_target_properties(Aleph_CA PROPERTIES EXPORT_NAME CA)

# Namespaced aliases so in-tree builds and installed consumers use identical
# target names.
add_library(Aleph::Aleph ALIAS Aleph)
add_library(Aleph::CA ALIAS Aleph_CA)

# Phase 25 — flat C ABI shim (libalephca_c). A small shared library exposing a
# pure-C surface (ca-c-api.h) so non-C++ languages (Rust, Julia, Go, .NET, ...)
# can drive a cellular automaton over a stable, versioned ABI. Internal C++
# symbols are hidden; only the `aleph_ca_*_v1` entry points are exported.
option(ALEPH_BUILD_C_API "Build the flat C ABI shared library (libalephca_c)" ON)
if(ALEPH_BUILD_C_API)
    add_library(alephca_c SHARED ca-c-api.cc)
    target_link_libraries(alephca_c PRIVATE Aleph)
    target_include_directories(alephca_c PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/aleph>)
    set_target_properties(alephca_c PROPERTIES
        OUTPUT_NAME alephca_c
        VERSION 1.0.0
        SOVERSION 1  # must match ALEPH_CA_API_VERSION_MAJOR in ca-c-api.h
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        POSITION_INDEPENDENT_CODE ON)
    add_library(Aleph::CApi ALIAS alephca_c)
endif()

# htlist.H type-puns through unions, so every consumer must also disable strict
# aliasing (this mirrors the library's own required Release flag). Propagated as
# an INTERFACE option so it reaches anyone linking Aleph::Aleph or Aleph::CA.
target_compile_options(Aleph INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-strict-aliasing>)

# Install the libraries. `aleph_miniz` is a PRIVATE dependency of the static
# `Aleph` archive, so it must travel in the same export set: a static archive
# does not absorb its dependencies and the consumer link line needs it.
install(TARGETS Aleph Aleph_CA aleph_miniz
    EXPORT AlephTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aleph
)

# Install headers (flat layout under include/aleph) plus the vendored miniz
# header so the exported include path is never dangling.
install(FILES ${HLIST}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aleph
)
install(FILES third_party/miniz/miniz.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aleph/third_party/miniz
)

# Phase 25 — install the C ABI shared library (kept out of the AlephTargets
# export set so it does not perturb the static-library package config).
# OPTIONAL: several packaging/consumer jobs build only `--target Aleph` and
# then run `cmake --install`; without OPTIONAL that install would fail because
# libalephca_c was never built. When a full build is done, the library is
# present and gets installed normally.
if(ALEPH_BUILD_C_API)
    install(TARGETS alephca_c
        OPTIONAL
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# Generate and install the package config files: AlephConfig.cmake,
# AlephConfigVersion.cmake and AlephTargets.cmake.
set(ALEPH_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/Aleph)

install(EXPORT AlephTargets
    FILE AlephTargets.cmake
    NAMESPACE Aleph::
    DESTINATION ${ALEPH_INSTALL_CMAKEDIR}
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/AlephConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/AlephConfig.cmake"
    INSTALL_DESTINATION ${ALEPH_INSTALL_CMAKEDIR}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/AlephConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/AlephConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/AlephConfigVersion.cmake"
    DESTINATION ${ALEPH_INSTALL_CMAKEDIR}
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

if(BUILD_REPRODUCTIONS)
    add_subdirectory(reproductions)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(ALEPH_BUILD_PYTHON)
    set(ALEPH_PYTHON_BINDINGS_INTERNAL ON)
    add_subdirectory(python)
endif()

# Print configuration summary
message(STATUS "Aleph-w Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Build reproductions: ${BUILD_REPRODUCTIONS}")
message(STATUS "  Build benchmarks: ${BUILD_BENCHMARKS}")
message(STATUS "  Build Python bindings: ${ALEPH_BUILD_PYTHON}")
message(STATUS "  Sanitizers: ${ALEPH_USE_SANITIZERS}")
message(STATUS "  Disable ranges: ${ALEPH_DISABLE_RANGES}")
message(STATUS "  X11 viewer: ${ALEPH_BUILD_X11_VIEWER}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")

# ------------------------------------------------------------------------------
# CPack: source tarball plus binary .deb / .rpm (roadmap Phase 21)
# ------------------------------------------------------------------------------
# Only drive packaging when Aleph-w is the top-level project, so a consumer
# pulling us in via FetchContent/add_subdirectory does not inherit CPack.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CPACK_PACKAGE_NAME "aleph-w")
    set(CPACK_PACKAGE_VENDOR "Leandro Rabindranath Leon")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
        "Aleph-w: header-dominant C++20 data structures, algorithms and cellular automata")
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/lrleon/Aleph-w")
    set(CPACK_PACKAGE_CONTACT "leandro.r.leon@gmail.com")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "aleph-w")

    # Binary generators and their runtime dependencies.
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "libgmp10, libgmpxx4ldbl, libmpfr6, libgsl27 | libgsl25 | libgsl23, libx11-6")
    set(CPACK_DEBIAN_PACKAGE_SECTION "libdevel")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_RPM_PACKAGE_REQUIRES "gmp, gmp-c++, mpfr, gsl, libX11")
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")

    # Source tarball: exclude VCS metadata, build trees, generated docs and
    # caches. Release artifacts are built with `git archive` (tracked files
    # only); this list keeps `make package_source` honest for local use.
    set(CPACK_SOURCE_GENERATOR "TGZ")
    set(CPACK_SOURCE_IGNORE_FILES
        "/\\.git/"
        "/build.*/"
        "/cmake-build-.*/"
        "/docs/html/"
        "/\\.cache/"
        "/__pycache__/"
        "\\.pyc$"
        "/benchmarks/trend-site/"
        "/reproductions/results/"
        "/reproductions/figures/generated/")

    include(CPack)
endif()
