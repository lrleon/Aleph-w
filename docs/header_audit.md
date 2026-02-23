## Auditoría de Headers (documentación, tests y ejemplos)

Este reporte se genera automáticamente (ver `scripts/header_audit.py`).

- **Headers analizados**: 239
- **Con `@file` + `@brief`**: 239
- **Sin tests directos (`#include`) en `Tests/`**: 81
- **Sin ejemplos directos (`#include`) en `Examples/`**: 135

### Resumen de faltantes (por heurística)

- **Sin tests directos**:
  - `List.H`
  - `Multimap.H`
  - `Queue.H`
  - `Stack.H`
  - `ah-args-ctor.H`
  - `ah-chronos-utils.H`
  - `ah-dry.H`
  - `ah-iterator.H`
  - `ah-time.H`
  - `ah-trace.H`
  - `ahAssert.H`
  - `ahDaemonize.H`
  - `ahDefs.H`
  - `ahDry.H`
  - `ahField.H`
  - `ahFunction.H`
  - `ahIterator.H`
  - `ahNow.H`
  - `ahPair.H`
  - `ahTypes.H`
  - `aleph-exceptions.H`
  - `aleph-graph.H`
  - `aleph-it.H`
  - `avlNode.H`
  - `avlNodeRk.H`
  - `btreepic_avl.H`
  - `deque.H`
  - `doxygen_groups.H`
  - `driven_table.H`
  - `fmmd.H`
  - `generate_df_tree.H`
  - `graph-dry.H`
  - `grid.H`
  - `hash-dry.H`
  - `hashDry.H`
  - `hashPars.H`
  - `lfit.H`
  - `log.H`
  - `mat_path.H`
  - `opBinTree.H`
  - `point_utils.H`
  - `q-consumer-threads.H`
  - `quadnode.H`
  - `random_net.H`
  - `random_tree.H`
  - `rbNode.H`
  - `rbNodeRk.H`
  - `shortest_path_common.H`
  - `single_graph.H`
  - `slink_nc.H`
  - …(+31)

- **Sin ejemplos directos**:
  - `List.H`
  - `Multimap.H`
  - `Queue.H`
  - `Stack.H`
  - `ah-args-ctor.H`
  - `ah-chronos-utils.H`
  - `ah-dry.H`
  - `ah-iterator.H`
  - `ah-time.H`
  - `ah-trace.H`
  - `ahAssert.H`
  - `ahDaemonize.H`
  - `ahDefs.H`
  - `ahDry.H`
  - `ahField.H`
  - `ahFunction.H`
  - `ahIterator.H`
  - `ahNow.H`
  - `ahPair.H`
  - `ahTypes.H`
  - `aleph-exceptions.H`
  - `aleph-graph.H`
  - `aleph-it.H`
  - `avlNode.H`
  - `avlNodeRk.H`
  - `btreepic_avl.H`
  - `deque.H`
  - `doxygen_groups.H`
  - `driven_table.H`
  - `fmmd.H`
  - `generate_df_tree.H`
  - `graph-dry.H`
  - `grid.H`
  - `hash-dry.H`
  - `hashDry.H`
  - `hashPars.H`
  - `lfit.H`
  - `log.H`
  - `mat_path.H`
  - `opBinTree.H`
  - `point_utils.H`
  - `q-consumer-threads.H`
  - `quadnode.H`
  - `random_net.H`
  - `random_tree.H`
  - `rbNode.H`
  - `rbNodeRk.H`
  - `shortest_path_common.H`
  - `single_graph.H`
  - `slink_nc.H`
  - …(+85)

### Matriz header por header

| Header | `@file/@brief` | Clases doc (aprox) | Funciones libres doc (aprox) | Tests que lo incluyen | Ejemplos que lo incluyen |
|---|---:|---:|---:|---|---|
| `List.H` | ✅ | 0/2 (0%) | n/a | — | — |
| `Multimap.H` | ✅ | 0/9 (0%) | n/a | — | — |
| `Queue.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `Stack.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `ah-args-ctor.H` | ✅ | n/a | n/a | — | — |
| `ah-chronos-utils.H` | ✅ | n/a | n/a | — | — |
| `ah-dry.H` | ✅ | 0/14 (0%) | n/a | — | — |
| `ah-iterator.H` | ✅ | 0/3 (0%) | n/a | — | — |
| `ah-time.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `ah-trace.H` | ✅ | n/a | n/a | — | — |
| `ahAssert.H` | ✅ | n/a | 0/2 (0%) | — | — |
| `ahDaemonize.H` | ✅ | n/a | 0/1 (0%) | — | — |
| `ahDefs.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `ahDry.H` | ✅ | 0/4 (0%) | n/a | — | — |
| `ahField.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `ahFunction.H` | ✅ | 0/44 (0%) | n/a | — | — |
| `ahIterator.H` | ✅ | 0/3 (0%) | n/a | — | — |
| `ahNow.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `ahPair.H` | ✅ | n/a | n/a | — | — |
| `ahTypes.H` | ✅ | n/a | n/a | — | — |
| `aleph-exceptions.H` | ✅ | 0/2 (0%) | n/a | — | — |
| `aleph-graph.H` | ✅ | 0/2 (0%) | n/a | — | — |
| `aleph-it.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `avlNode.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `avlNodeRk.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `btreepic_avl.H` | ✅ | 0/4 (0%) | n/a | — | — |
| `deque.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `doxygen_groups.H` | ✅ | n/a | n/a | — | — |
| `driven_table.H` | ✅ | 0/3 (0%) | n/a | — | — |
| `fmmd.H` | ✅ | 0/2 (0%) | n/a | — | — |
| `generate_df_tree.H` | ✅ | 0/6 (0%) | n/a | — | — |
| `graph-dry.H` | ✅ | 0/17 (0%) | n/a | — | — |
| `grid.H` | ✅ | 0/6 (0%) | n/a | — | — |
| `hash-dry.H` | ✅ | n/a | n/a | — | — |
| `hashDry.H` | ✅ | 0/5 (0%) | n/a | — | — |
| `hashPars.H` | ✅ | n/a | n/a | — | — |
| `lfit.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `log.H` | ✅ | 0/3 (0%) | n/a | — | — |
| `mat_path.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `opBinTree.H` | ✅ | n/a | n/a | — | — |
| `point_utils.H` | ✅ | n/a | n/a | — | — |
| `q-consumer-threads.H` | ✅ | 0/3 (0%) | n/a | — | — |
| `quadnode.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `random_net.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `random_tree.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `rbNode.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `rbNodeRk.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `shortest_path_common.H` | ✅ | 11/20 (55%) | n/a | — | — |
| `single_graph.H` | ✅ | 0/3 (0%) | n/a | — | — |
| `slink_nc.H` | ✅ | 0/2 (0%) | n/a | — | — |
| `socket_wrappers.H` | ✅ | n/a | n/a | — | — |
| `timeoutQueue.H` | ✅ | 0/2 (0%) | n/a | — | — |
| `tpl_binHeap.H` | ✅ | 0/8 (0%) | n/a | — | — |
| `tpl_binNodeAux.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `tpl_binTreeOps.H` | ✅ | 0/3 (0%) | n/a | — | — |
| `tpl_dyn_slist_nc.H` | ✅ | 0/2 (0%) | n/a | — | — |
| `tpl_euclidian_graph.H` | ✅ | 0/5 (0%) | n/a | — | — |
| `tpl_indexGraph.H` | ✅ | 0/3 (0%) | n/a | — | — |
| `tpl_nodePool.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `tpl_randNode.H` | ✅ | n/a | n/a | — | — |
| `tpl_rbNode.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `tpl_snode_nc.H` | ✅ | 0/2 (0%) | n/a | — | — |
| `tpl_test_acyclique.H` | ✅ | 0/2 (0%) | n/a | — | — |
| `tpl_test_path.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `trace.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `treapNode.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `uid.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `useCondVar.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `useMutex.H` | ✅ | 0/1 (0%) | 4/4 (100%) | — | — |
| `worker_pool.H` | ✅ | 0/1 (0%) | n/a | — | — |
| `xml_graph.H` | ✅ | 0/10 (0%) | n/a | — | — |
| `ah-dispatcher.H` | ✅ | 0/3 (0%) | n/a | — | `Examples/dispatcher_example.C` |
| `ah-mapping.H` | ✅ | 0/1 (0%) | n/a | — | `Examples/mapping_example.C` |
| `aleph.H` | ✅ | n/a | n/a | — | `Examples/btreepic.C`<br/>`Examples/writeBalance.C`<br/>`Examples/writeInsertRoot.C`<br/>`Examples/writeJoin.C`<br/>`Examples/writeRankTree.C`<br/>`Examples/writeSplit.C`<br/>…(+1) |
| `geom_algorithms.H` | ✅ | 0/5 (0%) | n/a | — | `Examples/geom_example.C` |
| `tpl_balanceXt.H` | ✅ | n/a | n/a | — | `Examples/writeBalance.C` |
| `tpl_dynTreap.H` | ✅ | 0/1 (0%) | n/a | — | `Examples/graphpic.C` |
| `tpl_find_path.H` | ✅ | 0/11 (0%) | n/a | — | `Examples/bfs_dfs_example.C` |
| `tpl_test_connectivity.H` | ✅ | 0/1 (0%) | n/a | — | `Examples/graph_components_example.C` |
| `tpl_treapRk.H` | ✅ | 0/6 (0%) | n/a | — | `Examples/timeAllTree.C` |
| `treepic_utils.H` | ✅ | n/a | n/a | — | `Examples/btreepic.C`<br/>`Examples/graphpic.C`<br/>`Examples/ntreepic.C` |
| `Karger.H` | ✅ | 1/2 (50%) | n/a | `Tests/karger.cc` | — |
| `ah-arena.H` | ✅ | 0/1 (0%) | n/a | `Tests/ah-arena.cc`<br/>`Tests/ah_arena_test.cc` | — |
| `ah-convert.H` | ✅ | 0/19 (0%) | n/a | `Tests/ah-convert.cc` | — |
| `ah-date.H` | ✅ | n/a | n/a | `Tests/ah_date_test.cc` | — |
| `ah-dry-mixin.H` | ✅ | 3/22 (13%) | n/a | `Tests/ah-dry-mixin_test.cc` | — |
| `ah-msg.H` | ✅ | 0/3 (0%) | n/a | `Tests/ah_msg_test.cc` | — |
| `ah-signal.H` | ✅ | 0/8 (0%) | n/a | `Tests/ah_signal_test.cc` | — |
| `ah-stl-functional.H` | ✅ | n/a | n/a | `Tests/ah-stl-functional_test.cc` | — |
| `ah-stl-zip.H` | ✅ | 0/6 (0%) | n/a | `Tests/ah-stl-zip_test.cc` | — |
| `ahAlgo.H` | ✅ | 1/12 (8%) | n/a | `Tests/ahAlgo.cc` | — |
| `ahMem.H` | ✅ | n/a | n/a | `Tests/ah_mem_test.cc` | — |
| `ahSingleton.H` | ✅ | 0/1 (0%) | n/a | `Tests/ahSingleton_test.cc` | — |
| `ahUtils.H` | ✅ | 0/2 (0%) | n/a | `Tests/ah-utils.cc` | — |
| `ah_init_guard.H` | ✅ | 0/1 (0%) | n/a | `Tests/ah_init_guard.cc` | — |
| `ah_stdc++_utils.H` | ✅ | n/a | n/a | `Tests/ah_stdc_utils_test.cc` | — |
| `archeap.H` | ✅ | 0/6 (0%) | n/a | `Tests/archeap_test.cc` | — |
| `array_it.H` | ✅ | 1/4 (25%) | n/a | `Tests/array-it.cc` | — |
| `array_utils.H` | ✅ | n/a | n/a | `Tests/array_utils.cc` | — |
| `cookie_guard.H` | ✅ | 0/3 (0%) | n/a | `Tests/cookie_guard_test.cc` | — |
| `dlink.H` | ✅ | 0/6 (0%) | n/a | `Tests/dlink.cc`<br/>`Tests/dlink_test.cc` | — |
| `filter_iterator.H` | ✅ | 0/3 (0%) | n/a | `Tests/test_filter_iterator.cc` | — |
| `future_utils.H` | ✅ | n/a | n/a | `Tests/future_utils_test.cc` | — |
| `generate_graph.H` | ✅ | 0/49 (0%) | n/a | `Tests/graph_visualization_test.cc` | — |
| `generate_spanning_tree_picture.H` | ✅ | 0/2 (0%) | n/a | `Tests/graph_visualization_test.cc` | — |
| `graph_to_tree.H` | ✅ | 0/4 (0%) | n/a | `Tests/graph_visualization_test.cc` | — |
| `hash-fct.H` | ✅ | 0/1 (0%) | n/a | `Tests/hash_fct_test.cc` | — |
| `line.H` | ✅ | 0/1 (0%) | n/a | `Tests/line_test.cc` | — |
| `load_digraph.H` | ✅ | 1/1 (100%) | n/a | `Tests/load_digraph_test.cc` | — |
| `parse-csv.H` | ✅ | 0/3 (0%) | n/a | `Tests/parse_csv_test.cc` | — |
| `point.H` | ✅ | 0/13 (0%) | n/a | `Tests/point_test.cc`<br/>`Tests/segment_test.cc` | — |
| `pointer_table.H` | ✅ | 0/1 (0%) | n/a | `Tests/pointer_table_test.cc` | — |
| `polygon.H` | ✅ | 0/8 (0%) | n/a | `Tests/polygon_test.cc` | — |
| `primes.H` | ✅ | n/a | n/a | `Tests/primes.cc` | — |
| `ringfilecache.H` | ✅ | 0/4 (0%) | n/a | `Tests/ringfilecache.cc` | — |
| `slink.H` | ✅ | 0/2 (0%) | n/a | `Tests/slink.cc` | — |
| `stat_utils.H` | ✅ | 0/1 (0%) | n/a | `Tests/stat_utils_test.cc` | — |
| `tpl_2dtree.H` | ✅ | 1/2 (50%) | 0/2 (0%) | `Tests/k2tree_test.cc` | — |
| `tpl_array.H` | ✅ | 0/2 (0%) | n/a | `Tests/ah-zip.cc`<br/>`Tests/ahAlgo.cc`<br/>`Tests/array.cc`<br/>`Tests/container_edge_cases_test.cc` | — |
| `tpl_dnode.H` | ✅ | 0/2 (0%) | n/a | `Tests/dnode.cc`<br/>`Tests/sort_utils.cc` | — |
| `tpl_dynArrayHeap.H` | ✅ | 1/2 (50%) | n/a | `Tests/ah-dry.cc`<br/>`Tests/dynarrayheap.cc` | — |
| `tpl_dynLhash.H` | ✅ | 0/3 (0%) | n/a | `Tests/tpl_dynLhash_test.cc` | — |
| `tpl_dynMat.H` | ✅ | 0/2 (0%) | n/a | `Tests/dynmat_test.cc` | — |
| `tpl_dynSlist.H` | ✅ | 0/2 (0%) | n/a | `Tests/dynslist.cc` | — |
| `tpl_dynarray_set.H` | ✅ | 0/1 (0%) | n/a | `Tests/ah-dry.cc`<br/>`Tests/tpl_dynarray_set_test.cc` | — |
| `tpl_graph.H` | ✅ | 0/2 (0%) | n/a | `Tests/graph_copy_test.cc` | — |
| `tpl_graph_indexes.H` | ✅ | 0/8 (0%) | n/a | `Tests/graph_indexes_test.cc` | — |
| `tpl_hRbTree.H` | ✅ | 1/3 (33%) | n/a | `Tests/rb-tree.cc` | — |
| `tpl_hRbTreeRk.H` | ✅ | 2/3 (66%) | n/a | `Tests/htdrbtreerk_test.cc` | — |
| `tpl_hash.H` | ✅ | 0/7 (0%) | n/a | `Tests/ah-dry.cc`<br/>`Tests/test_tpl_hash_macros.cc` | — |
| `tpl_indexArc.H` | ✅ | 0/5 (0%) | n/a | `Tests/tpl_indexArc_test.cc` | — |
| `tpl_indexNode.H` | ✅ | 0/4 (0%) | n/a | `Tests/index-node.cc` | — |
| `tpl_kgraph.H` | ✅ | 0/11 (0%) | n/a | `Tests/tpl_kgraph_test.cc` | — |
| `tpl_lhash.H` | ✅ | 0/5 (0%) | n/a | `Tests/hash-it.cc` | — |
| `tpl_linHash.H` | ✅ | 0/6 (0%) | n/a | `Tests/lin-hash.cc` | — |
| `tpl_memArray.H` | ✅ | 0/2 (0%) | n/a | `Tests/memarray.cc` | — |
| `tpl_multicommodity.H` | ✅ | 0/6 (0%) | n/a | `Tests/tpl_multicommodity_test.cc` | — |
| `tpl_net_sup_dem.H` | ✅ | 0/3 (0%) | n/a | `Tests/net_sup_dem_test.cc` | — |
| `tpl_netcapgraph.H` | ✅ | 0/3 (0%) | n/a | `Tests/netcapgraph_test.cc` | — |
| `tpl_random_queue.H` | ✅ | 0/4 (0%) | n/a | `Tests/ah-dry.cc`<br/>`Tests/ah_ranges_test.cc`<br/>`Tests/randomqueue.cc`<br/>`Tests/tpl_random_queue_test.cc` | — |
| `tpl_skipList.H` | ✅ | 1/4 (25%) | n/a | `Tests/skiplist_test.cc` | — |
| `tpl_slist.H` | ✅ | 0/2 (0%) | n/a | `Tests/slist.cc` | — |
| `tpl_snode.H` | ✅ | 0/1 (0%) | n/a | `Tests/snode.cc` | — |
| `tpl_test_cycle.H` | ✅ | 0/1 (0%) | n/a | `Tests/tpl_test_cycle.cc` | — |
| `warshall.H` | ✅ | 0/1 (0%) | n/a | `Tests/warshall_test.cc` | — |
| `AStar.H` | ✅ | 0/9 (0%) | n/a | `Tests/astar_test.cc` | `Examples/astar_example.cc` |
| `Bellman_Ford.H` | ✅ | 0/13 (0%) | n/a | `Tests/graph_scenarios_test.cc`<br/>`Tests/johnson_test.cc`<br/>`Tests/test_bellman_ford.cc` | `Examples/bellman_ford_example.cc` |
| `Dijkstra.H` | ✅ | 0/5 (0%) | n/a | `Tests/astar_test.cc`<br/>`Tests/graph_scenarios_test.cc`<br/>`Tests/graph_stress_test.cc`<br/>`Tests/johnson_test.cc`<br/>`Tests/test_dijkstra.cc` | `Examples/astar_example.cc`<br/>`Examples/dijkstra_example.cc` |
| `Floyd_Warshall.H` | ✅ | 0/3 (0%) | n/a | `Tests/floyd.cc` | `Examples/johnson_example.cc` |
| `Huffman.H` | ✅ | 0/6 (0%) | n/a | `Tests/huffman.cc`<br/>`Tests/huffman_btreepic_test.cc` | `Examples/write_huffman.C` |
| `Johnson.H` | ✅ | 0/5 (0%) | n/a | `Tests/johnson_test.cc` | `Examples/johnson_example.cc` |
| `Kruskal.H` | ✅ | 2/5 (40%) | n/a | `Tests/prim_test.cc`<br/>`Tests/test_kruskal.cc` | `Examples/mst_example.C` |
| `Prim.H` | ✅ | 0/8 (0%) | n/a | `Tests/prim_test.cc` | `Examples/mst_example.C` |
| `Simplex.H` | ✅ | 0/5 (0%) | n/a | `Tests/Simplex_test.cc`<br/>`Tests/tpl_netcost_test.cc` | `Examples/simplex_example.C` |
| `Tarjan.H` | ✅ | 0/7 (0%) | n/a | `Tests/graph_scenarios_test.cc`<br/>`Tests/tarjan_test.cc`<br/>`Tests/test_kosaraju.cc` | `Examples/graph_components_example.C`<br/>`Examples/kosaraju_example.cc`<br/>`Examples/tarjan_example.C` |
| `ah-comb.H` | ✅ | 0/1 (0%) | n/a | `Tests/ah-comb.cc` | `Examples/comb_example.C` |
| `ah-errors.H` | ✅ | 0/1 (0%) | n/a | `Tests/test_ah_errors.cc`<br/>`Tests/test_al_vector.cc` | `Examples/deway.C`<br/>`Examples/fibonacci.C`<br/>`Examples/ntreepic.C` |
| `ah-map-arena.H` | ✅ | 0/1 (0%) | n/a | `Tests/ah-mapping_test.cc` | `Examples/map_arena_example.C` |
| `ah-parallel.H` | ✅ | 1/1 (100%) | 0/1 (0%) | `Tests/ah_parallel_test.cc` | `Examples/ah_parallel_example.cc` |
| `ah-ranges.H` | ✅ | 3/18 (16%) | n/a | `Tests/ah_iterator_test.cc`<br/>`Tests/ah_ranges_test.cc`<br/>`Tests/ranges_iterator_test.cc` | `Examples/ranges_example.C` |
| `ah-stl-utils.H` | ✅ | 1/3 (33%) | n/a | `Tests/ah-stl-utils.cc`<br/>`Tests/sort_arrays.cc` | `Examples/stl_utils_example.C` |
| `ah-string-utils.H` | ✅ | 0/1 (0%) | n/a | `Tests/ah-dry.cc`<br/>`Tests/ah-functional.cc`<br/>`Tests/ah-string-utils.cc`<br/>`Tests/ah-zip.cc`<br/>`Tests/al-matrix.cc`<br/>`Tests/al-vector.cc`<br/>…(+3) | `Examples/string_utils_example.C` |
| `ah-uni-functional.H` | ✅ | 0/9 (0%) | n/a | `Tests/ah-uni-functional_test.cc` | `Examples/uni_functional_example.C` |
| `ah-zip-utils.H` | ✅ | 0/14 (0%) | n/a | `Tests/ah-zip-utils_test.cc` | `Examples/zip_utils_example.C` |
| `ah-zip.H` | ✅ | 0/4 (0%) | n/a | `Tests/ah-dry.cc`<br/>`Tests/ah-functional.cc`<br/>`Tests/ah-zip.cc`<br/>`Tests/tpl_dynListQueue.cc`<br/>`Tests/tree-node.cc` | `Examples/zip_example.C` |
| `ahFunctional.H` | ✅ | 18/65 (27%) | n/a | `Tests/ah-comb.cc`<br/>`Tests/ah-dry.cc`<br/>`Tests/ah-functional.cc`<br/>`Tests/ah-stl-utils.cc`<br/>`Tests/ah-zip.cc`<br/>`Tests/arrayqueue.cc`<br/>…(+9) | `Examples/bitarray_example.C`<br/>`Examples/functional_example.C`<br/>`Examples/matrix_example.C`<br/>`Examples/zip_example.C` |
| `ahSort.H` | ✅ | 2/6 (33%) | n/a | `Tests/ah-dry.cc`<br/>`Tests/ahSort_test.cc`<br/>`Tests/al-matrix.cc`<br/>`Tests/al-vector.cc`<br/>`Tests/hash-it.cc`<br/>`Tests/sort_arrays.cc`<br/>…(+1) | `Examples/functional_example.C`<br/>`Examples/sort_benchmark.C` |
| `al-domain.H` | ✅ | 0/2 (0%) | n/a | `Tests/al-domain.cc`<br/>`Tests/al_matrix_test.cc`<br/>`Tests/test_al_vector.cc` | `Examples/matrix_example.C` |
| `al-matrix.H` | ✅ | 0/1 (0%) | n/a | `Tests/al-matrix.cc`<br/>`Tests/al_matrix_test.cc` | `Examples/matrix_example.C` |
| `al-vector.H` | ✅ | 0/6 (0%) | n/a | `Tests/al-vector.cc`<br/>`Tests/al_matrix_test.cc`<br/>`Tests/test_al_vector.cc` | `Examples/matrix_example.C` |
| `bitArray.H` | ✅ | 0/4 (0%) | n/a | `Tests/bitarray.cc`<br/>`Tests/container_edge_cases_test.cc` | `Examples/bitarray_example.C` |
| `bloom-filter.H` | ✅ | 0/1 (0%) | n/a | `Tests/bloom-filter.cc` | `Examples/bloom_filter_example.C` |
| `eepicgeom.H` | ✅ | 0/10 (0%) | n/a | `Tests/eepicgeom_test.cc` | `Examples/graphpic.C` |
| `euclidian-graph-common.H` | ✅ | 1/7 (14%) | n/a | `Tests/euclidian_graph_common_test.cc` | `Examples/gen_rand_graph.C` |
| `eulerian.H` | ✅ | 1/7 (14%) | n/a | `Tests/eulerian_test.cc` | `Examples/eulerian_example.C`<br/>`Examples/random_graph_example.C` |
| `generate_tree.H` | ✅ | 0/1 (0%) | n/a | `Tests/generate_tree.cc`<br/>`Tests/graph_visualization_test.cc` | `Examples/deway.C`<br/>`Examples/generate_forest.C` |
| `graph-traverse.H` | ✅ | 0/5 (0%) | n/a | `Tests/graph_traverse_test.cc` | `Examples/bfs_dfs_example.C` |
| `hamiltonian.H` | ✅ | 0/6 (0%) | n/a | `Tests/hamiltonian_test.cc` | `Examples/hamiltonian_example.C` |
| `htlist.H` | ✅ | 0/10 (0%) | n/a | `Tests/ah-dry-mixin_test.cc`<br/>`Tests/ah-dry.cc`<br/>`Tests/ah-uni-functional_test.cc`<br/>`Tests/ah-zip-utils_test.cc`<br/>`Tests/ah_ranges_test.cc`<br/>`Tests/bin-node.cc`<br/>…(+7) | `Examples/bitarray_example.C`<br/>`Examples/comb_example.C`<br/>`Examples/functional_example.C`<br/>`Examples/matrix_example.C`<br/>`Examples/ranges_example.C`<br/>`Examples/sort_benchmark.C`<br/>…(+5) |
| `huffman_btreepic.H` | ✅ | 0/2 (0%) | n/a | `Tests/huffman_btreepic_test.cc` | `Examples/write_huffman.C` |
| `io_graph.H` | ✅ | 0/11 (0%) | n/a | `Tests/io_graph_test.cc` | `Examples/gen_rand_graph.C` |
| `kosaraju.H` | ✅ | 0/1 (0%) | n/a | `Tests/test_kosaraju.cc` | `Examples/kosaraju_example.cc` |
| `latex_floyd.H` | ✅ | 2/7 (28%) | n/a | `Tests/latex_floyd_test.cc` | `Examples/write_floyd.C` |
| `mat_latex.H` | ✅ | 0/9 (0%) | n/a | `Tests/mat_latex_path_test.cc` | `Examples/write_floyd.C` |
| `net_apps.H` | ✅ | 6/10 (60%) | n/a | `Tests/net_apps_test.cc` | `Examples/net_apps_example.cc` |
| `net_utils.H` | ✅ | 2/3 (66%) | n/a | `Tests/net_utils_test.cc` | `Examples/net_utils_example.cc` |
| `parse_utils.H` | ✅ | 2/4 (50%) | n/a | `Tests/parse_utils_test.cc` | `Examples/btreepic.C`<br/>`Examples/graphpic.C`<br/>`Examples/ntreepic.C` |
| `prefix-tree.H` | ✅ | 0/1 (0%) | n/a | `Tests/prefix_tree_test.cc` | `Examples/trie_example.C` |
| `quadtree.H` | ✅ | 0/1 (0%) | 0/1 (0%) | `Tests/quadtree_test.cc` | `Examples/quadtree_example.C` |
| `random_graph.H` | ✅ | 0/11 (0%) | n/a | `Tests/graph_scenarios_test.cc`<br/>`Tests/random_graph.cc` | `Examples/gen_rand_graph.C`<br/>`Examples/random_graph_example.C` |
| `thread_pool.H` | ✅ | 2/5 (40%) | n/a | `Tests/thread_pool_test.cc` | `Examples/thread_pool_example.cc` |
| `topological_sort.H` | ✅ | 1/11 (9%) | n/a | `Tests/topological_sort_test.cc` | `Examples/topological_sort_example.C` |
| `tpl_agraph.H` | ✅ | 1/8 (12%) | n/a | `Tests/graph_functional_test.cc`<br/>`Tests/test_kruskal.cc`<br/>`Tests/tpl_agraph_test.cc`<br/>`Tests/tpl_components_test.cc`<br/>`Tests/tpl_spanning_tree_test.cc`<br/>`Tests/tpl_test_cycle.cc` | `Examples/gen_rand_graph.C` |
| `tpl_arrayHeap.H` | ✅ | 0/2 (0%) | n/a | `Tests/ah-dry.cc`<br/>`Tests/arrayheap.cc`<br/>`Tests/arrayheap_algos.cc` | `Examples/writeHeap.C` |
| `tpl_arrayQueue.H` | ✅ | 0/4 (0%) | n/a | `Tests/ah-dry.cc`<br/>`Tests/ah-zip-utils_test.cc`<br/>`Tests/ah_ranges_test.cc`<br/>`Tests/arrayqueue.cc`<br/>`Tests/container_edge_cases_test.cc`<br/>`Tests/fixedqueue.cc` | `Examples/linear_structures_example.C` |
| `tpl_arrayStack.H` | ✅ | 0/4 (0%) | n/a | `Tests/ah-zip-utils_test.cc`<br/>`Tests/ah_ranges_test.cc`<br/>`Tests/arraystack.cc`<br/>`Tests/container_edge_cases_test.cc`<br/>`Tests/fixedstack.cc` | `Examples/evalExp.C`<br/>`Examples/fib.C`<br/>`Examples/fibonacci.C`<br/>`Examples/linear_structures_example.C` |
| `tpl_avl.H` | ✅ | 0/5 (0%) | n/a | `Tests/avl.cc` | `Examples/timeAllTree.C`<br/>`Examples/write_tree.C` |
| `tpl_avlRk.H` | ✅ | 0/5 (0%) | n/a | `Tests/avl-rb-rk.cc` | `Examples/timeAllTree.C` |
| `tpl_binNode.H` | ✅ | 0/3 (0%) | n/a | `Tests/bin-node-utils.cc`<br/>`Tests/bin-node.cc` | `Examples/btreepic.C`<br/>`Examples/writeInsertRoot.C` |
| `tpl_binNodeUtils.H` | ✅ | 0/27 (0%) | n/a | `Tests/bin-node-utils.cc`<br/>`Tests/bin-node-xt.cc`<br/>`Tests/huffman.cc`<br/>`Tests/rand-tree.cc` | `Examples/btreepic.C`<br/>`Examples/deway.C`<br/>`Examples/timeAllTree.C`<br/>`Examples/writeInsertRoot.C`<br/>`Examples/writeJoin.C`<br/>`Examples/writeRankTree.C`<br/>…(+2) |
| `tpl_binNodeXt.H` | ✅ | 8/13 (61%) | n/a | `Tests/bin-node-xt.cc` | `Examples/writeJoin.C`<br/>`Examples/writeRankTree.C`<br/>`Examples/writeSplit.C` |
| `tpl_binTree.H` | ✅ | 0/5 (0%) | n/a | `Tests/bintree.cc` | `Examples/generate_forest.C`<br/>`Examples/timeAllTree.C`<br/>`Examples/writeRankTree.C`<br/>`Examples/writeSplit.C`<br/>`Examples/write_tree.C` |
| `tpl_bipartite.H` | ✅ | 0/6 (0%) | n/a | `Tests/bipartite_test.cc` | `Examples/bipartite_example.C` |
| `tpl_components.H` | ✅ | 0/5 (0%) | n/a | `Tests/tpl_components_test.cc` | `Examples/graph_components_example.C`<br/>`Examples/random_graph_example.C` |
| `tpl_cut_nodes.H` | ✅ | 0/1 (0%) | n/a | `Tests/cut_nodes_test.cc` | `Examples/cut_nodes_example.C` |
| `tpl_dynArray.H` | ✅ | 0/4 (0%) | n/a | `Tests/ah-dry.cc`<br/>`Tests/ah-functional.cc`<br/>`Tests/ah-uni-functional_test.cc`<br/>`Tests/ah-zip-utils_test.cc`<br/>`Tests/ah-zip.cc`<br/>`Tests/ahSort_test.cc`<br/>…(+7) | `Examples/btreepic.C`<br/>`Examples/dispatcher_example.C`<br/>`Examples/functional_example.C`<br/>`Examples/geom_example.C`<br/>`Examples/graphpic.C`<br/>`Examples/linear_structures_example.C`<br/>…(+10) |
| `tpl_dynBinHeap.H` | ✅ | 0/2 (0%) | n/a | `Tests/ah-dry.cc`<br/>`Tests/binheap.cc` | `Examples/heap_example.C` |
| `tpl_dynDlist.H` | ✅ | 0/2 (0%) | n/a | `Tests/ah-dry.cc`<br/>`Tests/ah-uni-functional_test.cc`<br/>`Tests/ah-zip-utils_test.cc`<br/>`Tests/ahAlgo.cc`<br/>`Tests/ahSort_test.cc`<br/>`Tests/ah_iterator_test.cc`<br/>…(+8) | `Examples/btreepic.C`<br/>`Examples/functional_example.C`<br/>`Examples/joseph.C`<br/>`Examples/linear_structures_example.C`<br/>`Examples/ntreepic.C`<br/>`Examples/polinom.C`<br/>…(+3) |
| `tpl_dynList.H` | ✅ | n/a | n/a | `Tests/ahSort_test.cc`<br/>`Tests/sort_utils.cc` | `Examples/linear_structures_example.C` |
| `tpl_dynListQueue.H` | ✅ | 0/2 (0%) | n/a | `Tests/ah-zip-utils_test.cc`<br/>`Tests/ah_ranges_test.cc`<br/>`Tests/dynlistqueue.cc`<br/>`Tests/tpl_dynListQueue.cc` | `Examples/linear_structures_example.C` |
| `tpl_dynListStack.H` | ✅ | 0/2 (0%) | n/a | `Tests/ah-dry.cc`<br/>`Tests/ah-zip-utils_test.cc`<br/>`Tests/ah_ranges_test.cc`<br/>`Tests/dynliststack.cc`<br/>`Tests/dynliststack_test.cc`<br/>`Tests/tpl_dynListStack.cc` | `Examples/linear_structures_example.C` |
| `tpl_dynMapOhash.H` | ✅ | 0/5 (0%) | n/a | `Tests/dynmapohash_test.cc`<br/>`Tests/dynsetohash.cc`<br/>`Tests/hash-it.cc` | `Examples/hash_resize.C` |
| `tpl_dynMapTree.H` | ✅ | 0/10 (0%) | n/a | `Tests/tpl_dynMapTree_test.cc` | `Examples/dynmap_example.C`<br/>`Examples/timeAllTree.C` |
| `tpl_dynSetHash.H` | ✅ | 0/16 (0%) | n/a | `Tests/ah-dry.cc`<br/>`Tests/ah-zip-utils_test.cc`<br/>`Tests/ah-zip.cc`<br/>`Tests/dynsethash.cc`<br/>`Tests/dynsetohash.cc`<br/>`Tests/hash-it.cc` | `Examples/hash_tables_example.C`<br/>`Examples/set_structures_benchmark.cc` |
| `tpl_dynSetTree.H` | ✅ | 0/29 (0%) | n/a | `Tests/ah-arena.cc`<br/>`Tests/ah-dry.cc`<br/>`Tests/ah-functional.cc`<br/>`Tests/ah-zip-utils_test.cc`<br/>`Tests/ah-zip.cc`<br/>`Tests/ah_iterator_test.cc`<br/>…(+6) | `Examples/dynset_trees.C`<br/>`Examples/set_structures_benchmark.cc` |
| `tpl_dynSkipList.H` | ✅ | 1/3 (33%) | n/a | `Tests/dynskiplist_test.cc` | `Examples/set_structures_benchmark.cc`<br/>`Examples/skiplist_example.C` |
| `tpl_fibonacci_heap.H` | ✅ | 0/2 (0%) | n/a | `Tests/fibonacci_heap_test.cc` | `Examples/heap_example.C` |
| `tpl_graph.H` | ✅ | 0/44 (0%) | n/a | `Tests/archeap_test.cc`<br/>`Tests/astar_test.cc`<br/>`Tests/bipartite_test.cc`<br/>`Tests/cookie_guard_test.cc`<br/>`Tests/cut_nodes_test.cc`<br/>`Tests/euclidian_graph_common_test.cc`<br/>…(+30) | `Examples/astar_example.cc`<br/>`Examples/bellman_ford_example.cc`<br/>`Examples/bfs_dfs_example.C`<br/>`Examples/bipartite_example.C`<br/>`Examples/cut_nodes_example.C`<br/>`Examples/dijkstra_example.cc`<br/>…(+10) |
| `tpl_graph_utils.H` | ✅ | 0/14 (0%) | n/a | `Tests/astar_test.cc`<br/>`Tests/cut_nodes_test.cc`<br/>`Tests/floyd.cc`<br/>`Tests/graph_visualization_test.cc`<br/>`Tests/johnson_test.cc`<br/>`Tests/prim_test.cc`<br/>…(+3) | `Examples/write_floyd.C` |
| `tpl_matgraph.H` | ✅ | 1/6 (16%) | n/a | `Tests/latex_floyd_test.cc`<br/>`Tests/tpl_matgraph_test.cc` | `Examples/write_floyd.C` |
| `tpl_maxflow.H` | ✅ | 0/9 (0%) | n/a | `Tests/net_utils_test.cc`<br/>`Tests/random_network_generator_test.cc`<br/>`Tests/tpl_maxflow_test.cc` | `Examples/maxflow_advanced_example.cc`<br/>`Examples/net_utils_example.cc` |
| `tpl_mincost.H` | ✅ | 0/7 (0%) | n/a | `Tests/random_network_generator_test.cc`<br/>`Tests/tpl_mincost_test.cc` | `Examples/mincost_flow_example.cc` |
| `tpl_net.H` | ✅ | 3/23 (13%) | n/a | `Tests/netcapgraph_test.cc`<br/>`Tests/random_network_generator_test.cc`<br/>`Tests/tpl_maxflow_test.cc`<br/>`Tests/tpl_net_test.cc` | `Examples/maxflow_advanced_example.cc`<br/>`Examples/net_utils_example.cc`<br/>`Examples/network_flow_example.C` |
| `tpl_netcost.H` | ✅ | 0/21 (0%) | n/a | `Tests/random_network_generator_test.cc`<br/>`Tests/tpl_mincost_test.cc`<br/>`Tests/tpl_netcost_test.cc` | `Examples/mincost_flow_example.cc` |
| `tpl_odhash.H` | ✅ | 0/2 (0%) | n/a | `Tests/ah-functional.cc`<br/>`Tests/container_edge_cases_test.cc`<br/>`Tests/hash-it.cc`<br/>`Tests/odhash.cc` | `Examples/hash_tables_example.C`<br/>`Examples/set_structures_benchmark.cc` |
| `tpl_olhash.H` | ✅ | 0/2 (0%) | n/a | `Tests/container_edge_cases_test.cc`<br/>`Tests/hash-it.cc`<br/>`Tests/olhash.cc` | `Examples/set_structures_benchmark.cc` |
| `tpl_rand_tree.H` | ✅ | 0/5 (0%) | n/a | `Tests/rand-tree.cc` | `Examples/timeAllTree.C`<br/>`Examples/write_tree.C` |
| `tpl_rbRk.H` | ✅ | 0/7 (0%) | n/a | `Tests/avl-rb-rk.cc`<br/>`Tests/htdrbtreerk_test.cc`<br/>`Tests/tdrbtreerk_test.cc` | `Examples/timeAllTree.C` |
| `tpl_rb_tree.H` | ✅ | 0/5 (0%) | n/a | `Tests/rb-tree.cc`<br/>`Tests/rb_tree_test.cc`<br/>`Tests/tdrbtree_test.cc` | `Examples/timeAllTree.C`<br/>`Examples/write_tree.C` |
| `tpl_sgraph.H` | ✅ | 0/6 (0%) | n/a | `Tests/euclidian_graph_common_test.cc`<br/>`Tests/graph_functional_test.cc`<br/>`Tests/karger.cc`<br/>`Tests/test_kruskal.cc`<br/>`Tests/tpl_components_test.cc`<br/>`Tests/tpl_sgraph_test.cc`<br/>…(+2) | `Examples/gen_rand_graph.C` |
| `tpl_sort_utils.H` | ✅ | 0/52 (0%) | n/a | `Tests/sort_utils.cc` | `Examples/btreepic.C`<br/>`Examples/graphpic.C`<br/>`Examples/sort_benchmark.C`<br/>`Examples/timeAllTree.C` |
| `tpl_spanning_tree.H` | ✅ | 0/3 (0%) | n/a | `Tests/tpl_spanning_tree_test.cc` | `Examples/graph_components_example.C` |
| `tpl_splay_tree.H` | ✅ | 0/5 (0%) | n/a | `Tests/splay-tree.cc` | `Examples/timeAllTree.C`<br/>`Examples/write_tree.C` |
| `tpl_splay_treeRk.H` | ✅ | 0/5 (0%) | n/a | `Tests/splay-tree-rk.cc` | `Examples/timeAllTree.C` |
| `tpl_tdRbTree.H` | ✅ | 0/6 (0%) | n/a | `Tests/tdrbtree_test.cc` | `Examples/timeAllTree.C` |
| `tpl_tdRbTreeRk.H` | ✅ | 1/6 (16%) | n/a | `Tests/tdrbtreerk_test.cc` | `Examples/timeAllTree.C` |
| `tpl_treap.H` | ✅ | 0/5 (0%) | n/a | `Tests/treap_test.cc` | `Examples/timeAllTree.C`<br/>`Examples/write_tree.C` |
| `tpl_tree_node.H` | ✅ | 4/13 (30%) | n/a | `Tests/graph_visualization_test.cc`<br/>`Tests/tree-node.cc` | `Examples/deway.C`<br/>`Examples/generate_forest.C`<br/>`Examples/ntreepic.C` |
| `tpl_union.H` | ✅ | 0/5 (0%) | n/a | `Tests/union.cc` | `Examples/percolation_example.C`<br/>`Examples/union_find_example.C` |

### Notas importantes

- **Los checks de documentación son heurísticos**: cuentan `class/struct` por regex y buscan `///` o `/**` cerca. Las funciones se detectan con una heurística muy conservadora (solo firmas en una línea a nivel 0 de llaves).
- **La cobertura de tests/ejemplos es por `#include` directo**. Un header puede estar cubierto indirectamente por otro include, y aquí no aparecer.