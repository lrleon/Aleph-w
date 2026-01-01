# Install script for directory: /home/lrleon/Aleph-w

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/lrleon/Aleph-w/Tests/build/aleph/libAleph.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/aleph" TYPE FILE FILES
    "/home/lrleon/Aleph-w/tpl_dynSetTree.H"
    "/home/lrleon/Aleph-w/tpl_arrayHeap.H"
    "/home/lrleon/Aleph-w/tpl_binHeap.H"
    "/home/lrleon/Aleph-w/tpl_netcost.H"
    "/home/lrleon/Aleph-w/tpl_dynBinHeap.H"
    "/home/lrleon/Aleph-w/generate_tree.H"
    "/home/lrleon/Aleph-w/generate_graph.H"
    "/home/lrleon/Aleph-w/tpl_tree_node.H"
    "/home/lrleon/Aleph-w/point.H"
    "/home/lrleon/Aleph-w/tpl_treapRk.H"
    "/home/lrleon/Aleph-w/Set.H"
    "/home/lrleon/Aleph-w/Map.H"
    "/home/lrleon/Aleph-w/array_it.H"
    "/home/lrleon/Aleph-w/al-domain.H"
    "/home/lrleon/Aleph-w/Multiset.H"
    "/home/lrleon/Aleph-w/Multimap.H"
    "/home/lrleon/Aleph-w/List.H"
    "/home/lrleon/Aleph-w/Vector.H"
    "/home/lrleon/Aleph-w/ahAlgo.H"
    "/home/lrleon/Aleph-w/Stack.H"
    "/home/lrleon/Aleph-w/Queue.H"
    "/home/lrleon/Aleph-w/tpl_odhash.H"
    "/home/lrleon/Aleph-w/tpl_nodePool.H"
    "/home/lrleon/Aleph-w/random_graph.H"
    "/home/lrleon/Aleph-w/Priority_Queue.H"
    "/home/lrleon/Aleph-w/tpl_olhash.H"
    "/home/lrleon/Aleph-w/tpl_dynSetHash.H"
    "/home/lrleon/Aleph-w/ahSort.H"
    "/home/lrleon/Aleph-w/ahSearch.H"
    "/home/lrleon/Aleph-w/tpl_random_queue.H"
    "/home/lrleon/Aleph-w/tpl_net_sup_dem.H"
    "/home/lrleon/Aleph-w/tpl_bipartite.H"
    "/home/lrleon/Aleph-w/graph-traverse.H"
    "/home/lrleon/Aleph-w/tpl_dynarray_set.H"
    "/home/lrleon/Aleph-w/tpl_kgraph.H"
    "/home/lrleon/Aleph-w/tpl_dynMat.H"
    "/home/lrleon/Aleph-w/ahMem.H"
    "/home/lrleon/Aleph-w/ahAssert.H"
    "/home/lrleon/Aleph-w/ahDefs.H"
    "/home/lrleon/Aleph-w/aleph.H"
    "/home/lrleon/Aleph-w/euclidian-graph-common.H"
    "/home/lrleon/Aleph-w/tpl_linearHash.H"
    "/home/lrleon/Aleph-w/signal.H"
    "/home/lrleon/Aleph-w/ahUtils.H"
    "/home/lrleon/Aleph-w/ran_array.h"
    "/home/lrleon/Aleph-w/tpl_dynDlist.H"
    "/home/lrleon/Aleph-w/tpl_skipList.H"
    "/home/lrleon/Aleph-w/tpl_tdRbTree.H"
    "/home/lrleon/Aleph-w/tpl_splay_tree.H"
    "/home/lrleon/Aleph-w/tpl_dynTreap.H"
    "/home/lrleon/Aleph-w/tpl_union.H"
    "/home/lrleon/Aleph-w/tpl_binNode.H"
    "/home/lrleon/Aleph-w/tpl_splay_treeRk.H"
    "/home/lrleon/Aleph-w/useMutex.H"
    "/home/lrleon/Aleph-w/protected_cache.H"
    "/home/lrleon/Aleph-w/socket_wrappers.H"
    "/home/lrleon/Aleph-w/tpl_dynListStack.H"
    "/home/lrleon/Aleph-w/tpl_autoPtr.H"
    "/home/lrleon/Aleph-w/trace.H"
    "/home/lrleon/Aleph-w/tpl_cache.H"
    "/home/lrleon/Aleph-w/format.H"
    "/home/lrleon/Aleph-w/random_net.H"
    "/home/lrleon/Aleph-w/protected_lhash.H"
    "/home/lrleon/Aleph-w/protected_odhash.H"
    "/home/lrleon/Aleph-w/log.H"
    "/home/lrleon/Aleph-w/useCondVar.H"
    "/home/lrleon/Aleph-w/tpl_dynListQueue.H"
    "/home/lrleon/Aleph-w/al-vector.H"
    "/home/lrleon/Aleph-w/ahSingleton.H"
    "/home/lrleon/Aleph-w/comb.H"
    "/home/lrleon/Aleph-w/treepic_utils.H"
    "/home/lrleon/Aleph-w/driven_table.H"
    "/home/lrleon/Aleph-w/pointer_table.H"
    "/home/lrleon/Aleph-w/Karger.H"
    "/home/lrleon/Aleph-w/al-matrix.H"
    "/home/lrleon/Aleph-w/timeoutQueue.H"
    "/home/lrleon/Aleph-w/parse_utils.H"
    "/home/lrleon/Aleph-w/ah_stdc++_utils.H"
    "/home/lrleon/Aleph-w/eulerian.H"
    "/home/lrleon/Aleph-w/hamiltonian.H"
    "/home/lrleon/Aleph-w/ahTypes.H"
    "/home/lrleon/Aleph-w/tpl_arrayStack.H"
    "/home/lrleon/Aleph-w/tpl_arrayQueue.H"
    "/home/lrleon/Aleph-w/tpl_graph.H"
    "/home/lrleon/Aleph-w/mat_path.H"
    "/home/lrleon/Aleph-w/tpl_slist_modified.H"
    "/home/lrleon/Aleph-w/ahFunction.H"
    "/home/lrleon/Aleph-w/ahPair.H"
    "/home/lrleon/Aleph-w/ahIterator.H"
    "/home/lrleon/Aleph-w/grid.H"
    "/home/lrleon/Aleph-w/tpl_path_utils.H"
    "/home/lrleon/Aleph-w/mat_latex.H"
    "/home/lrleon/Aleph-w/tpl_sort_utils.H"
    "/home/lrleon/Aleph-w/polygon.H"
    "/home/lrleon/Aleph-w/generate_df_tree.H"
    "/home/lrleon/Aleph-w/generate_spanning_tree_picture.H"
    "/home/lrleon/Aleph-w/archeap.H"
    "/home/lrleon/Aleph-w/filter_iterator.H"
    "/home/lrleon/Aleph-w/kosaraju.H"
    "/home/lrleon/Aleph-w/htlist.H"
    "/home/lrleon/Aleph-w/tpl_sgraph.H"
    "/home/lrleon/Aleph-w/tpl_indexNode.H"
    "/home/lrleon/Aleph-w/tpl_indexArc.H"
    "/home/lrleon/Aleph-w/tpl_indexGraph.H"
    "/home/lrleon/Aleph-w/io_graph.H"
    "/home/lrleon/Aleph-w/tpl_slist.H"
    "/home/lrleon/Aleph-w/tpl_snode.H"
    "/home/lrleon/Aleph-w/tpl_lhash.H"
    "/home/lrleon/Aleph-w/tpl_dynLhash.H"
    "/home/lrleon/Aleph-w/tpl_odhash.H"
    "/home/lrleon/Aleph-w/tpl_memArray.H"
    "/home/lrleon/Aleph-w/tpl_dynArray.H"
    "/home/lrleon/Aleph-w/ahFuntional.H"
    "/home/lrleon/Aleph-w/hash-dry.H"
    "/home/lrleon/Aleph-w/bloom-filter.H"
    "/home/lrleon/Aleph-w/ah-dry.H"
    "/home/lrleon/Aleph-w/graph-dry.H"
    "/home/lrleon/Aleph-w/tpl_con_queue.H"
    "/home/lrleon/Aleph-w/q-consumer-threads.H"
    "/home/lrleon/Aleph-w/ah-string-utils.H"
    "/home/lrleon/Aleph-w/ah-comb.H"
    "/home/lrleon/Aleph-w/geom_algorithms.H"
    )
endif()

