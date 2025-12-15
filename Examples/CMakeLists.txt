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
    test_2dtree
    test_agraph
    testAlgo
    testAllTree
    testArrayHeap
    testArrayQueue
    testAvl
    testBalance
    test_bellman
    testBinHeap
    testBinHeapPtr
    testBinNode
    testBinNodeUtils
    testBinNodeXt
    test_bin_to_tree
    testBinTree
    test_bipartite
    testBitArray
    test-bloom
    test-bsearch
    testCache
    test-comb
    test-con-queue
    test-convexhull-time
    test_copy_graph
    test_cut
    test_cut_nodes
    test-digraph
    test_dijkstra
    test-dispatch
    testDlink
    testDnode
    test-dry
    testDynArray
    testDynBinHeap
    testDynDlist
    testDynHash
    testDynListQueue
    testDynListStack
    testDynMapTree
    testDynMat
    testDynSetHash
    test_dynSetTree
    testDynSetTree
    testDynSlist
    test_dyn_sort_utils
    test_eepicgeom
    test_ellipse
    test-emplace
    test-filter
    test_find_path
    test-flatten
    test_floyd
    test-forward
    testFunctional
    test_generate_graph
    test_generate_spanning
    test_gen_tree
    test_graph
    test-graph-dry
    test_graph_to_tree
    testHash
    test_hash_cache
    test_htlist
    test_huffman
    test_in_graph
    test-in-out
    testInsertRoot
    test_io_graph
    test-itor
    test-join
    testJoin
    test_karger
    test_kgraph
    test_kosaraju
    testLinHash
    testList
    test_load_eugraph
    testLuka
    test-map
    testMap
    test_matgraph
    test-matrix
    test-memarray
    test-odhash
    testMerge
    test_min_path
    test_min_spanning
    testMultimap
    testMultiset
    test-net
    test-netcost
    testOhash
    testOpBinTree
    test-pair-it
    test_point
    test_polygon
    testProtectedCache
    test-quadtree
    testQueue
    test_random_graph
    test_random_net
    test_random_search
    test_rand_tree
    test_rb_tree
    test_rk
    test-rvalues
    test_search
    test_search_deway
    testSet
    test_sgraph
    test_simplex
    testSkipList
    testSlink
    testSlist
    testSnode
    test_sort_lists
    test_splayRk
    test_splay_tree
    test-splice
    testSplit
    test-sq
    test-stl-utils
    test-string
    test_tarjan
    test_top_sort
    test-traverse
    testTreap
    testTreapRk
    test-tree-itor
    test_tree_node
    testUid
    test_union
    test-vector
    testVector
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
    testSplayRk1
    testSplayTree
    workersTest
    test-zip-it
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