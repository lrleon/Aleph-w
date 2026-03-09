# Aleph-w — Backlog de algoritmos faltantes para considerarla “bastante completa y versátil”

> Contexto: Aleph-w ya es fuerte en grafos (paths, MST, flows/cuts, SCC, topological, etc.) y tiene un núcleo de algoritmos clásicos (sorting, Huffman, Simplex, etc.). Este backlog lista áreas típicas que faltan en librerías generalistas tipo “CLRS+moderno”.

---

## 1) Algoritmos de strings (alta demanda, hoy casi imprescindibles)

### Búsqueda de patrones
- ~~KMP (Knuth–Morris–Pratt)~~ ✅ Implementado: `kmp_prefix_function()`, `kmp_search()` — ver `String_Search.H`
- ~~Z-algorithm~~ ✅ Implementado: `z_algorithm()`, `z_search()` — ver `String_Search.H`
- ~~Boyer–Moore / Horspool~~ ✅ Implementado: `boyer_moore_horspool_search()` — ver `String_Search.H`
- ~~Rabin–Karp (rolling hash)~~ ✅ Implementado: `rabin_karp_search()` — ver `String_Search.H`

### Multi-pattern
- ~~Aho–Corasick~~ ✅ Implementado: `Aho_Corasick` — ver `Aho_Corasick.H`

### Estructuras de sufijos
- ~~Suffix Array + LCP (Kasai)~~ ✅ Implementado: `suffix_array()`, `lcp_array_kasai()` — ver `Suffix_Structures.H`
- ~~Suffix Tree *(opcional si hay suffix array, pero gran valor didáctico)*~~ ✅ Implementado (didáctico): `Naive_Suffix_Tree` — ver `Suffix_Structures.H`
- ~~Suffix Automaton~~ ✅ Implementado: `Suffix_Automaton` — ver `Suffix_Structures.H`

### Otros clásicos
- ~~Manacher (palíndromos en O(n))~~ ✅ Implementado: `manacher()`, `longest_palindromic_substring()` — ver `String_Palindromes.H`
- ~~Edit distance (Levenshtein) y Damerau–Levenshtein~~ ✅ Implementado: `levenshtein_distance()`, `damerau_levenshtein_distance()` — ver `String_DP.H`
- ~~LCS / Longest Common Substring~~ ✅ Implementado: `longest_common_subsequence()`, `longest_common_substring()` — ver `String_DP.H`

Cobertura y ejemplos:
- ✅ Tests dedicados: `Tests/string_search_test.cc`, `Tests/aho_corasick_test.cc`, `Tests/suffix_structures_test.cc`, `Tests/string_palindromes_test.cc`, `Tests/string_dp_test.cc`, `Tests/string_algorithms_test.cc`
- ✅ Ejemplos dedicados: `Examples/kmp_example.cc`, `Examples/z_algorithm_example.cc`, `Examples/horspool_example.cc`, `Examples/rabin_karp_example.cc`, `Examples/aho_corasick_example.cc`, `Examples/suffix_array_lcp_example.cc`, `Examples/suffix_tree_example.cc`, `Examples/suffix_automaton_example.cc`, `Examples/manacher_example.cc`, `Examples/edit_distance_example.cc`, `Examples/damerau_levenshtein_example.cc`, `Examples/lcs_longest_common_substring_example.cc`

---

## 2) Range queries y estructuras “de uso diario”

- ~~Fenwick Tree (BIT)~~ ✅ Implementado: `Gen_Fenwick_Tree`, `Fenwick_Tree` (con `find_kth`), `Range_Fenwick_Tree` — ver `tpl_fenwick_tree.H`
- ~~Segment Tree (incl. lazy propagation)~~ ✅ Implementado: `Gen_Segment_Tree` (point update + range query, monoid genérico), `Gen_Lazy_Segment_Tree` (range update + range query, lazy propagation), `Segment_Tree_Beats` (Ji Driver's, chmin/chmax) — ver `tpl_segment_tree.H`
- ~~Sparse Table (RMQ estática)~~ ✅ Implementado: `Gen_Sparse_Table` (genérica, op idempotente), `Sparse_Table` (range min), `Max_Sparse_Table` (range max) — ver `tpl_sparse_table.H`
- ~~Disjoint Sparse Table (range queries, op asociativa no idempotente)~~ ✅ Implementado: `Gen_Disjoint_Sparse_Table` (genérica), `Sum_Disjoint_Sparse_Table` (range sum), `Product_Disjoint_Sparse_Table` (range product) — ver `tpl_disjoint_sparse_table.H`
- ~~Interval Tree / Segment tree de intervalos~~ ✅ Implementado: `Gen_Interval_Tree`, `Interval_Tree` (augmented treap con max endpoint) — ver `tpl_interval_tree.H`
- ~~Cartesian Tree + reducciones a RMQ/LCA *(bloque didáctico elegante)*~~ ✅ Implementado: `Gen_Cartesian_Tree` (árbol explícito O(n)), `Gen_Euler_Tour_LCA` (LCA O(1) via Euler Tour + Sparse Table), `Gen_Cartesian_Tree_RMQ` (RMQ O(1) via reducción a LCA) — ver `tpl_cartesian_tree.H`
- ~~Mo's algorithm (offline range queries)~~ ✅ Implementado: `Gen_Mo_Algorithm<T, Policy>` (snake-optimized sweep), `Distinct_Count_Mo`, `Powerful_Array_Mo`, `Range_Mode_Mo` — ver `tpl_mo_algorithm.H`

---

## 3) Programación dinámica clásica + optimizaciones que hacen la diferencia

### DP clásica
- ~~Knapsack (0/1, unbounded, bounded)~~ ✅ Implementado: `knapsack_01()`, `knapsack_01_value()`, `knapsack_unbounded()`, `knapsack_bounded()` — ver `Knapsack.H`
- ~~LIS (O(n log n))~~ ✅ Implementado: `longest_increasing_subsequence()`, `lis_length()`, `longest_nondecreasing_subsequence()` — ver `LIS.H`
- ~~Matrix-chain multiplication~~ ✅ Implementado: `matrix_chain_order()`, `matrix_chain_min_cost()` — ver `Matrix_Chain.H`
- ~~Subset sum / meet-in-the-middle~~ ✅ Implementado: `subset_sum()`, `subset_sum_exists()`, `subset_sum_count()`, `subset_sum_mitm()` — ver `Subset_Sum.H`
- ~~DP en árboles (tree DP patterns, rerooting DP)~~ ✅ Implementado: `Gen_Tree_DP`, `Gen_Reroot_DP`, `tree_subtree_sizes()`, `tree_max_distance()`, `tree_sum_of_distances()` — ver `Tree_DP.H`

Cobertura y ejemplos:
- ✅ Tests dedicados: `Tests/knapsack_test.cc`, `Tests/lis_test.cc`, `Tests/matrix_chain_test.cc`, `Tests/subset_sum_test.cc`, `Tests/tree_dp_test.cc`
- ✅ Ejemplos dedicados: `Examples/knapsack_example.cc`, `Examples/lis_example.cc`, `Examples/matrix_chain_example.cc`, `Examples/subset_sum_example.cc`, `Examples/tree_dp_example.cc`

### Optimizaciones
- ~~Divide & Conquer DP optimization~~ ✅ Implementado: `divide_and_conquer_partition_dp()` — ver `DP_Optimizations.H`
- ~~Knuth optimization~~ ✅ Implementado: `knuth_optimize_interval()`, `optimal_merge_knuth()` — ver `DP_Optimizations.H`
- ~~Convex Hull Trick (CHT) + Li Chao Tree~~ ✅ Implementado: `Convex_Hull_Trick<T>`, `Li_Chao_Tree<T>`, `min_weighted_squared_distance_1d()` — ver `DP_Optimizations.H`
- ~~Monotone queue optimization~~ ✅ Implementado: `monotone_queue_min_dp()` — ver `DP_Optimizations.H`

Cobertura y ejemplos:
- ✅ Tests dedicados: `Tests/dp_optimizations_test.cc`
- ✅ Ejemplo dedicado: `Examples/dp_optimizations_example.cc`

---

## 4) Teoría de números / álgebra discreta (núcleo duro)

- ~~GCD y extended GCD~~ ✅ (Se usa `std::gcd` de C++17/20, `ext_gcd` en `modular_arithmetic.H`)
- ~~Multiplicación modular segura y expmod~~ ✅ LISTO en `modular_arithmetic.H`
- ~~Inverso modular y Teorema Chino del Resto (CRT)~~ ✅ LISTO en `modular_arithmetic.H`
- ~~Test de Primalidad de Miller-Rabin (64-bit determinista)~~ ✅ LISTO en `primality.H`
- ~~Factorización de Pollard's Rho~~ ✅ LISTO en `pollard_rho.H`
- ~~Number Theoretic Transform (NTT)~~ ✅ LISTO en `ntt.H`
- ~~Combinatoria modular (nCk) y Teorema de Lucas~~ ✅ LISTO en `modular_combinatorics.H`
- ~~Álgebra lineal modular (Gauss, determinante, inversa)~~ ✅ LISTO en `modular_linalg.H`

---

## 5) Geometría computacional (Suite robusta implementada)

✅ **Implementado en `geom_algorithms.H`, `polygon.H`, `point.H`**:
- **Convex Hull**: Andrew, Graham, QuickHull, Gift Wrapping, Brute Force.
- **Triangulación**: Ear-cutting (simple polygons), Bowyer-Watson (Delaunay).
- **Voronoi**: Diagramas de Voronoi derivados de Delaunay.
- **Intersección**: Sutherland-Hodgman (convex-convex), Half-plane intersection (O(n log n)).
- **MEC**: Welzl's Minimum Enclosing Circle (O(n) esperado).
- **Rotating Calipers**: Diámetro y ancho mínimo de polígonos convexos.
- **Point-in-Polygon**: Winding number exacto.
- **Estructuras**: AABBTree, 2D-Tree (en `tpl_2dtree.H`).
- **Simplificación**: Douglas-Peucker, Visvalingam-Whyatt.
- **Otros**: Par de puntos más cercanos (D&C), Polygon Offset.

---

## 6) Grafos — para redondear una suite “muy completa”

### Matching / assignment
- ~~Hopcroft–Karp (matching bipartito)~~ ✅ LISTO: `hopcroft_karp_matching()` en `tpl_bipartite.H`
- ~~Hungarian / Munkres (assignment)~~ ✅ LISTO: `Hungarian_Assignment<Cost_Type>`, `hungarian_assignment()`, `hungarian_max_assignment()` -- ver `Hungarian.H`
- ~~Blossom (matching general)~~ ✅ LISTO: `Blossom_Matching`, `Weighted_Blossom` -- ver `Blossom.H` y `Blossom_Weighted.H`
- ~~Min-cost matching~~ ✅ LISTO: `compute_minimum_cost_general_matching()`, `blossom_minimum_cost_matching()` -- ver `Min_Cost_Matching.H`

### Conectividad y estructuras
- ~~Bridges + biconnected components~~ ✅ LISTO: `Compute_Bridges<GT,SA>`, `find_bridges()` — Tarjan O(V+E) low-link DFS, aristas paralelas manejadas — ver `tpl_cut_nodes.H`
- ~~LCA (binary lifting, Euler tour + RMQ)~~ ✅ LISTO: `LCA<GT>` en `LCA.H`
- ~~Heavy-Light Decomposition (queries en paths)~~ ✅ LISTO: `Gen_Heavy_Light_Decomposition`, `Gen_HLD_Path_Query` — ver `Tree_Decomposition.H`; también `Gen_HLD`, `HLD_Sum`, `HLD_Max`, `HLD_Min` — ver `HLD.H`
- ~~Centroid decomposition *(niche, potente)*~~ ✅ LISTO: `Gen_Centroid_Decomposition` — ver `Tree_Decomposition.H`

### Caminos y ciclos
- ~~K shortest paths (Yen / Eppstein)~~ ✅ LISTO: `yen_k_shortest_paths()`, `eppstein_k_shortest_paths()`, `Yen_K_Shortest_Paths`, `Eppstein_K_Shortest_Paths` — ver `K_Shortest_Paths.H`
- ~~Min mean cycle (Karp)~~ ✅ LISTO: `karp_minimum_mean_cycle()`, `minimum_mean_cycle()`, `Karp_Minimum_Mean_Cycle` — ver `Min_Mean_Cycle.H`

### Otros “pro”
- ~~Dominators (Lengauer–Tarjan)~~ ✅ LISTO: `compute_dominators()`, `build_dominator_tree()`, `compute_dominance_frontiers()`, `Lengauer_Tarjan_Dominators` — ver `Dominators.H`
- ~~Planarity testing~~ ✅ LISTO: `planarity_test()`, `is_planar_graph()`, `Planarity_Test`, `Is_Planar_Graph` + `Planarity_Test_Options` (embedding LR lineal con fallback opcional, certificados Kuratowski `K5/K3,3`, trazabilidad a arcos originales, y API de caras/dual) — ver `Planarity_Test.H`
  - Mejora aplicada: embedding LR estricto reforzado con reparación local determinística (reversos/swaps + multi-arranque por descenso coordenado), sin búsqueda combinatoria global en modo estricto.
  - Mejora aplicada: API de dibujo planar geométrico `planar_geometric_drawing()` (coordenadas 2D embedding-aware + validación de cruces).
  - Mejora aplicada: exportadores de certificado no planar `nonplanar_certificate_to_json()` y `nonplanar_certificate_to_dot()`.
  - Mejora aplicada: formalización de garantías en documentación (`README` + comentarios de `Planarity_Test_Options`), separando claramente decisión LR exacta vs construcción de embedding acotada en modo estricto.
  - Mejora aplicada: exportadores adicionales `nonplanar_certificate_to_graphml()` y `nonplanar_certificate_to_gexf()`, y validador estructural `validate_nonplanar_certificate()`.
  - Mejora aplicada: adaptador externo end-to-end `scripts/planarity_certificate_validator.rb` para `GraphML/GEXF`, con validación XML estructural y chequeo opcional vía `--networkx`.
  - Mejora aplicada: integración opcional con Gephi Toolkit/CLI vía `--gephi` + `--gephi-cmd "{input}"`, con tests portables (modo opcional/requerido).
  - Mejora aplicada: catálogo versionado de plantillas Gephi por SO (`scripts/planarity_gephi_templates.json`) + selección por id `--gephi-template`.
  - Mejora aplicada: batch runner reproducible para CI `scripts/planarity_certificate_ci_batch.rb` con reporte JSON determinista.
  - Mejora aplicada: perfiles de export/render Gephi (SVG/PDF) con catálogo dedicado `scripts/planarity_gephi_render_profiles.json`, validación de artefactos en `scripts/planarity_certificate_validator.rb` (`--render-gephi`, `--render-profile`) y cobertura CI batch.
  - Mejora aplicada: expansión de perfiles Gephi por SO y versión (Linux/macOS/Windows; 0.9.x/0.10.x) en `scripts/planarity_gephi_render_profiles.json`.
  - Mejora aplicada: comparación visual determinística con golden digest vía `scripts/planarity_certificate_ci_visual_diff.rb` + baseline `scripts/planarity_visual_golden_manifest.json` y job dedicado en CI.
  - Mejora aplicada: job nightly/manual con Gephi real descargado desde releases oficiales (`.github/workflows/planarity_gephi_nightly.yml`) validando el adapter sobre fixture Aleph (`scripts/fixtures/planarity_k33_certificate.graphml`) en Linux/macOS/Windows.
  - Mejora aplicada: expansión paralela del catálogo de templates Gephi (`scripts/planarity_gephi_templates.json`) para 0.9.x/0.10.x en Linux/macOS/Windows.
  - Mejora aplicada: workflow nightly ampliado para barrer múltiples tags (`0.9.x` + `0.10.x`) con matriz multiplataforma y reporte comparativo por run (`gephi-nightly-comparison-*`).
  - Mejora aplicada: gate automático de regresiones entre tags Gephi por SO en el job comparativo (`overall_valid` + códigos de salida), con artifact/sumario siempre preservados.
  - Mejora aplicada: frontends Ruby para validación externa y CI (`scripts/planarity_certificate_validator.rb`, `scripts/planarity_certificate_ci_batch.rb`, `scripts/planarity_certificate_ci_visual_diff.rb`) y comparador weekly nativo Ruby (`scripts/planarity_gephi_weekly_comparison.rb`).
  - Mejora aplicada: notificación opcional de degradaciones vía webhook (`scripts/planarity_gephi_regression_notify.rb`) integrada en el nightly con secreto `ALEPH_PLANARITY_ALERT_WEBHOOK`.
  - Mejora pendiente (opcional): añadir adaptadores nativos para destinos específicos (Slack block-kit, Teams, email SMTP) sobre el notificador genérico por webhook.

---

## 7) Algoritmos utilitarios que suelen esperarse

- ~~Selection: quickselect + median of medians~~ ✅ LISTO: `random_select` (iterativo + 3-way) en `tpl_sort_utils.H`
- Probabilísticos: 
  - ~~Count-Min Sketch~~ ✅ LISTO: `Count_Min_Sketch` en `count-min-sketch.H`
  - ~~HyperLogLog~~ ✅ LISTO: `HyperLogLog` en `hyperloglog.H`
  - ~~Reservoir sampling~~ ✅ LISTO: `Reservoir_Sampler` en `reservoir-sampling.H`
  - ~~MinHash / SimHash~~ ✅ LISTO: `MinHash` y `SimHash` en sus respectivos headers.
- Sorting adicional: counting sort, radix sort, bucket sort, (opcional) timsort (LISTO)
- Combinatoria: ~~next_permutation extendido, combinaciones~~ ✅ Implementado en `ah-comb.H`: `next_permutation()`, `next_combination_indices()`, `next_combination_mask()`, `for_each_combination()`, `build_combinations()`, `combination_count()`; ~~Gray codes~~ ✅ Implementado: `bin_to_gray()`, `gray_to_bin()`, `build_gray_code()` — ver `ah-comb.H`
- Búsqueda: IDA*, bidirectional BFS
- BFS 0-1
- Dial’s algorithm (pesos enteros pequeños)

---

## 8) Probabilísticos / streaming (para versatilidad moderna)

- ~~Bloom filter~~ ✅ LISTO: `Bloom_Filter` en `bloom-filter.H`
- Count-Min Sketch LISTO
- HyperLogLog (cardinalidad aproximada) LISTO
- Reservoir sampling LISTO
- MinHash / SimHash (similaridad) LISTO

---

## Priorización sugerida (actualizada)

1) **Strings** (KMP/Z/Aho–Corasick + suffix array/LCP + rolling hash)
2) ~~**Teoría de números avanzada**~~ ✅ LISTO (Miller–Rabin + Pollard Rho + CRT + NTT)
3) **Programación Dinámica clásica** + LIS + CHT/Li Chao
4) **Estructuras de Grafos** (HLD + Centroid Decomposition)
