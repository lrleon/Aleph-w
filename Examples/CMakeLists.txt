# Examples share configuration from the parent project; only add targets here

set(EXAMPLES_WARN_FLAGS "-Wall -Wextra -Wcast-align -Wno-sign-compare -Wno-write-strings -Wno-parentheses")
set(EXAMPLES_COMMON_FLAGS "-DMESSAGES -D_GLIBCXX__PTHREADS -D_REENTRANT -D__extern_always_inline=\"extern __always_inline\"")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXAMPLES_WARN_FLAGS} ${EXAMPLES_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -O0 -g3")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# List of all example programs present in the directory
set(EXAMPLE_PROGRAMS
    bfs_dfs_example
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
