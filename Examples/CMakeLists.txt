# Examples share configuration from the parent project; only add targets here

set(EXAMPLES_WARN_FLAGS "-Wall -Wextra -Wcast-align -Wno-sign-compare -Wno-write-strings -Wno-parentheses")
set(EXAMPLES_COMMON_FLAGS "-DMESSAGES -D_GLIBCXX__PTHREADS -D_REENTRANT -D__extern_always_inline=\"extern __always_inline\"")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXAMPLES_WARN_FLAGS} ${EXAMPLES_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -O0 -g3")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# List of all example programs from Imakefile
set(EXAMPLE_PROGRAMS
    btreepic
    deway
    evalExp
    fibonacci
    generate_forest
    gen_rand_graph
    graphpic
    joseph
    ntreepic
    polinom
    example_2dtree
    example_agraph
    exampleAlgo
    exampleAllTree
    exampleArrayHeap
    exampleArrayQueue
    exampleAvl
    exampleBalance
    example_bellman
    exampleBinHeap
    exampleBinHeapPtr
    exampleBinNode
    exampleBinNodeUtils
    exampleBinNodeXt
    example_bin_to_tree
    exampleBinTree
    example_bipartite
    exampleBitArray
    example-bloom
    example-bsearch
    exampleCache
    example-comb
    example-con-queue
    example-convexhull-time
    example_copy_graph
    example_cut
    example_cut_nodes
    example-digraph
    exampleDijkstra
    example-dispatch
    exampleDlink
    exampleDnode
    example-dry
    exampleDynArray
    exampleDynBinHeap
    exampleDynDlist
    exampleDynHash
    exampleDynListQueue
    exampleDynListStack
    exampleDynMapTree
    exampleDynMat
    exampleDynSetHash
    example_dynSetTree
    exampleDynSetTree
    exampleDynSlist
    example_dyn_sort_utils
    example_eepicgeom
    example_ellipse
    example-emplace
    example-filter
    example_find_path
    example-flatten
    example_floyd
    example-forward
    exampleFunctional
    example_generate_graph
    example_generate_spanning
    example_gen_tree
    example_graph
    example-graph-dry
    example_graph_to_tree
    exampleHash
    example_hash_cache
    example_htlist
    example_huffman
    example_in_graph
    example-in-out
    exampleInsertRoot
    example_io_graph
    example-itor
    example-join
    exampleJoin
    example_karger
    example_kgraph
    example_kosaraju
    exampleLinHash
    exampleList
    example_load_eugraph
    exampleLuka
    example-map
    exampleMap
    example_matgraph
    example-matrix
    example-memarray
    example-odhash
    exampleMerge
    example_min_path
    example_min_spanning
    exampleMultimap
    exampleMultiset
    example-net
    example-netcost
    exampleOhash
    exampleOpBinTree
    example-pair-it
    example_point
    example_polygon
    exampleProtectedCache
    example-quadtree
    exampleQueue
    example_random_graph
    example_random_net
    example_random_search
    example_rand_tree
    example_rb_tree
    example_rk
    example-rvalues
    example_search
    example_search_deway
    exampleSet
    example_sgraph
    example_simplex
    exampleSkipList
    exampleSlink
    exampleSlist
    exampleSnode
    example_sort_lists
    example_splayRk
    example_splay_tree
    example-splice
    exampleSplit
    example-sq
    example-stl-utils
    example-string
    example_tarjan
    example_top_sort
    example-traverse
    exampleTreap
    exampleTreapRk
    example-tree-itor
    example_tree_node
    exampleUid
    example_union
    example-vector
    exampleVector
    timeAllTree
    writeAllTrees
    writeAvl
    writeBalance
    writeBinTree
    write_floyd
    writeHeap
    write_huffman
    writeInsertRoot
    writeJoin
    writeRandTree
    writeRankTree
    writeRb
    writeSplay
    writeSplit
    writeTreap
    exampleSplayRk1
    exampleSplayTree
    exampleWorkersTest
    example-zip-it
    hash_resize
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

        # Optional: Install the example executable
        # install(TARGETS ${program} DESTINATION bin/examples)
    else()
        message(WARNING "Source file not found for program: ${program}")
    endif()
endforeach()

message(STATUS "Configured ${CMAKE_PROJECT_NAME} examples")