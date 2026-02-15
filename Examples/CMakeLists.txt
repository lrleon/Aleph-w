# Examples share configuration from the parent project; only add targets here

set(EXAMPLES_WARN_FLAGS "-Wall -Wextra -Wcast-align -Wno-sign-compare -Wno-write-strings -Wno-parentheses")
set(EXAMPLES_COMMON_FLAGS "-DMESSAGES -D_GLIBCXX__PTHREADS -D_REENTRANT -D__extern_always_inline=\"extern __always_inline\"")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXAMPLES_WARN_FLAGS} ${EXAMPLES_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -O0 -g3")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# List of all example programs present in the directory
set(EXAMPLE_PROGRAMS
    ah_parallel_example
    astar_example
    bfs_dfs_example
    set_structures_benchmark
    bellman_ford_example
    kosaraju_example
    johnson_example
    btreepic
    cut_nodes_example
    deway
    dijkstra_example
    dynmap_example
    dynset_trees
    evalExp
    fibonacci
    functional_example
    gen_rand_graph
    bitarray_example
    matrix_example
    generate_forest
    graphpic
    hash_resize
    heap_example
    joseph
    mst_example
    network_flow_example
    ntreepic
    percolation_example
    polinom
    quadtree_example
    simplex_example
    skiplist_example
    sort_benchmark
    tarjan_example
    thread_pool_example
    timeAllTree
    topological_sort_example
    trie_example
    writeBalance
    writeHeap
    writeInsertRoot
    writeJoin
    writeRankTree
    writeSplit
    write_floyd
    write_huffman
    write_tree
    zip_example
    zip_utils_example
    uni_functional_example
    comb_example
    string_utils_example
    eulerian_example
    bloom_filter_example
    hamiltonian_example
    random_graph_example
    bipartite_example
    ranges_example
    dispatcher_example
    geom_example
    mapping_example
    stl_utils_example
    map_arena_example
    map_arena_persistence_example
    maxflow_advanced_example
    mincost_flow_example
    net_apps_example
    net_utils_example
    union_find_example
    hash_tables_example
    linear_structures_example
    graph_components_example
    array_utils_example
    tpl_indexGraph_example
    parse_csv_example
    tpl_indexNode_example
    tpl_indexArc_example
    mat_path_example
    tpl_dynMat_example
    tpl_net_sup_dem_example
    tpl_netcapgraph_example
    tpl_multicommodity_example
    tpl_kgraph_example
    graph_to_tree_example
    karger_example
    load_digraph_example
    tpl_euclidian_graph_example
    tpl_2dtree_example
    min_cut_test
    min_cut_example
    timeoutQueue_example
    ah_stl_functional_example
    driven_table_example
    fenwick_tree_example
    sparse_table_example
    sparse_table_test
    disjoint_sparse_table_example
    disjoint_sparse_table_test
    segment_tree_example
    cartesian_tree_example
    mo_algorithm_example
    mo_on_trees_example
    robust_predicates_example
    voronoi_clipped_cells_example
    delaunay_voronoi_example
    constrained_delaunay_example
    point_in_polygon_example
    polygon_intersection_example
    tikz_polygons_example
    tikz_convex_hull_example
    tikz_intersection_example
    tikz_voronoi_power_example
    tikz_advanced_algorithms_example
    tikz_funnel_animation_example
    tikz_funnel_beamer_example
    tikz_funnel_beamer_twocol_example
    tikz_funnel_beamer_handout_example
    tikz_scene_example
    tikz_scene_beamer_example
    tikz_scene_overlays_example
    halfplane_intersection_example
    convex_hull_comparison_example
    closest_pair_example
    rotating_calipers_example
)

# Function to get source file from program name
function(get_source_file program_name out_var)
    # First try with .C extension
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${program_name}.C")
        set(${out_var} "${program_name}.C" PARENT_SCOPE)
    # Then try with .cc extension
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${program_name}.cc")
        set(${out_var} "${program_name}.cc" PARENT_SCOPE)
    else()
        string(REPLACE "-" "_" alt_name ${program_name})
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${alt_name}.C")
            set(${out_var} "${alt_name}.C" PARENT_SCOPE)
        elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${alt_name}.cc")
            set(${out_var} "${alt_name}.cc" PARENT_SCOPE)
        else()
            set(${out_var} "" PARENT_SCOPE)
        endif()
    endif()
endfunction()

# Create executable for each example program
foreach(program ${EXAMPLE_PROGRAMS})
    get_source_file(${program} source_file)

    if(source_file)
        add_executable(${program} ${source_file})

        target_link_libraries(${program} PRIVATE Aleph ${ALEPH_SYSTEM_LIBS})
    else()
        message(WARNING "Source file not found for program: ${program}")
    endif()
endforeach()

message(STATUS "Configured ${CMAKE_PROJECT_NAME} examples")
