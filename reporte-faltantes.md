# Aleph-w — Reporte de elementos faltantes

Análisis comparativo contra lo que se espera de una librería general de
estructuras de datos y algoritmos "bastante completa y versátil".

Se compara contra: CLRS, Sedgewick, cp-algorithms.com, librerías de
competitive programming (Kactl, ACL), Boost Graph Library, CGAL, y
curricula estándar de CS.

---

## 1. Estructuras de datos

### 1.1 Árboles y estructuras de búsqueda

| Estructura | Estado | Prioridad | Notas |
|---|---|---|---|
| B-Tree / B+ Tree | **Falta** | Media | Necesario para almacenamiento en disco / bases de datos. Estructura clásica de CLRS. |
| Van Emde Boas tree | **Falta** | Baja | O(log log U) operaciones sobre universo entero. Teórico pero elegante. |
| Wavelet tree | **Falta** | Media | Consultas rank/select/quantile sobre secuencias. Muy útil en compresión y text indexing. |
| Link-Cut tree | **Falta** | Media-Alta | Conectividad dinámica en árboles. Necesario para flujo máximo O(VE log V) y dynamic MST. |
| Treap con clave implícita | **Falta** | Media | Secuencia con split/merge, reverse, inserciones arbitrarias en O(log n). `tpl_treapRk.H` tiene un comentario "Implicit keys possible" pero no está implementado. |
| Persistent segment tree | **Falta** | Media | Consultas sobre versiones históricas. Fundamental en CP y bases de datos funcionales. |
| Rope (cuerda) | **Falta** | Baja | Edición eficiente de strings largos. |

### 1.2 Heaps

| Estructura | Estado | Prioridad | Notas |
|---|---|---|---|
| Leftist heap | **Falta** | Baja | Heap mergeable en O(log n). Simple de implementar. |
| Skew heap | **Falta** | Baja | Heap mergeable amortizado. Más simple que leftist. |
| Pairing heap | **Falta** | Media | Mejor rendimiento práctico que Fibonacci heap, más simple. Usado en implementaciones modernas de Dijkstra. |

Ya existen: `BinHeap`, `ArrayHeap`, `DynArrayHeap`, `DynBinHeap`, `FibonacciHeap`.

### 1.3 Hash tables

| Estructura | Estado | Prioridad | Notas |
|---|---|---|---|
| Cuckoo hashing | **Falta** | Media | Worst-case O(1) lookup. Mencionado en documentación pero no implementado. |
| Robin Hood hashing | **Falta** | Baja | Variante de open addressing con mejor clustering. |

Ya existen: chaining (`tpl_lhash.H`, `tpl_linHash.H`), open addressing lineal (`tpl_olhash.H`), double hashing (`tpl_odhash.H`).

---

## 2. Algoritmos de grafos

### 2.1 Flujo en redes

| Algoritmo | Estado | Prioridad | Notas |
|---|---|---|---|
| Push-Relabel (Goldberg-Tarjan) | **Existe** (en `tpl_kgraph.H`) | — | `Heap_Preflow_Maximum_Flow`. Verificar accesibilidad pública. |

Ya existen: Ford-Fulkerson, Edmonds-Karp, Dinic, Capacity Scaling.

### 2.2 Problemas NP-duros (aproximaciones y heurísticas)

| Algoritmo | Estado | Prioridad | Notas |
|---|---|---|---|
| Graph coloring (greedy + backtracking) | **Falta** | Media | Coloración greedy, Welsh-Powell, DSatur. Común en scheduling y asignación de registros. |
| Maximum clique (Bron-Kerbosch) | **Falta** | Baja | Backtracking con pivoteo. Útil en bioinformática y redes sociales. |
| Maximum independent set | **Falta** | Baja | Complemento de vertex cover. Aproximación greedy. |
| Minimum vertex cover | **Falta** | Baja | 2-aproximación clásica (CLRS). |
| TSP (aproximaciones) | **Falta** | Media | Christofides (1.5-aprox), nearest neighbor, 2-opt. Problema icónico de optimización combinatoria. |

### 2.3 Satisfacibilidad y lógica

| Algoritmo | Estado | Prioridad | Notas |
|---|---|---|---|
| 2-SAT | **Falta** | Media-Alta | Resoluble en O(V+E) via SCC. Fundamental en CP y verificación. Usa Tarjan que ya existe. |

### 2.4 Otros algoritmos de grafos

| Algoritmo | Estado | Prioridad | Notas |
|---|---|---|---|
| Grafo de condensación (SCC → DAG) | **Parcial** | Media | Tarjan.H calcula SCCs. Falta una función explícita que construya el DAG condensado como un nuevo grafo. |
| Camino más largo en DAG | **Falta** | Media | O(V+E) con topological sort + DP. Trivial de implementar dado que `topological_sort.H` ya existe. |
| Graph isomorphism | **Falta** | Baja | Al menos tree isomorphism (hashing canónico). El caso general es cuasi-polinomial (Babai). |
| Stable matching (Gale-Shapley) | **Falta** | Baja | Algoritmo clásico de CLRS/Kleinberg-Tardos. |

---

## 3. Algoritmos numéricos y algebraicos

| Algoritmo | Estado | Prioridad | Notas |
|---|---|---|---|
| FFT (Fast Fourier Transform) | **Falta** | Alta | Multiplicación de polinomios en O(n log n). Existe NTT (`ntt.H`) para aritmética modular, pero falta FFT sobre `double`/`complex` para aplicaciones de señales, convolución real, y multiplicación de enteros grandes. |
| Matrix exponentiation | **Falta** | Alta | A^n en O(k³ log n). Esencial para resolver recurrencias lineales. Trivial de implementar sobre `tpl_dynMat.H` o `al-matrix.H`. |
| Eliminación de Gauss sobre reales | **Falta** | Media | Existe sobre campos finitos (`modular_linalg.H`), pero falta sobre `double` con pivoteo parcial. Necesario para resolver sistemas lineales generales. |
| Interpolación de Lagrange | **Falta** | Baja | Interpolación polinomial en O(n²) o O(n log² n). |
| Exponenciación rápida genérica | **Parcial** | Media | Existe `mod_pow` en `modular_arithmetic.H`. Falta versión genérica sobre monoides arbitrarios (matrices, etc.). |

---

## 4. Algoritmos de strings

| Algoritmo | Estado | Prioridad | Notas |
|---|---|---|---|
| Palindromic tree (Eertree) | **Falta** | Baja | Estructura online para palíndromos distintos. Existe Manacher en `String_Palindromes.H` pero Eertree permite consultas más ricas. |

La cobertura de strings es muy buena: KMP, Z, Boyer-Moore, Rabin-Karp, Aho-Corasick, Suffix Array/LCP, Suffix Tree, Suffix Automaton, Manacher, LCS, edit distance.

---

## 5. Técnicas algorítmicas generales

| Técnica | Estado | Prioridad | Notas |
|---|---|---|---|
| Sqrt decomposition | **Falta** | Media | Descomposición en bloques O(√n). Complementa a Mo's algorithm (que sí existe). |
| Fenwick tree: range update + range query | **Falta** | Media | Requiere dos Fenwick internos. La versión actual solo soporta point update + range query. (Ya documentado en `falta.md`.) |

---

## 6. Geometría computacional

La cobertura geométrica es excelente. Posibles adiciones menores:

| Algoritmo | Estado | Prioridad | Notas |
|---|---|---|---|
| Half-plane intersection | **Verificar** | Baja | Intersección de semiplanos en O(n log n). |
| 3D geometry primitives | **Falta** | Baja | Puntos, planos, y operaciones en 3D. La librería es puramente 2D. |
| Minkowski sum | **Falta** | Baja | Suma de Minkowski de polígonos convexos. |

---

## Resumen de prioridades

### Alta prioridad (gaps significativos para una librería "completa")

1. **FFT sobre reales/complejos** — complementa el NTT existente
2. **Matrix exponentiation** — algoritmo fundamental, trivial de implementar
3. **2-SAT** — usa Tarjan que ya existe, implementación directa
4. **Link-Cut tree** — habilita algoritmos dinámicos avanzados

### Media prioridad (valiosos pero no bloqueantes)

5. **B-Tree / B+ Tree** — estructura clásica de CLRS
6. **Wavelet tree** — consultas rank/select/quantile
7. **Persistent segment tree** — versiones históricas
8. **Treap con clave implícita** — secuencia dinámica con split/merge
9. **Graph coloring** — scheduling, asignación de registros
10. **TSP (aproximaciones)** — problema icónico
11. **Camino más largo en DAG** — trivial con topological sort
12. **Gaussian elimination sobre reales** — complementa versión modular
13. **Sqrt decomposition** — técnica complementaria
14. **Fenwick range-update/range-query** — ya en backlog
15. **Pairing heap** — mejor alternativa práctica a Fibonacci
16. **Cuckoo hashing** — O(1) worst-case lookup
17. **Condensación explícita de SCC** — construir el DAG como grafo

### Baja prioridad (nice-to-have)

18. Van Emde Boas tree
19. Leftist / Skew heap
20. Robin Hood hashing
21. Maximum clique (Bron-Kerbosch)
22. Maximum independent set / Vertex cover
23. Stable matching (Gale-Shapley)
24. Graph isomorphism
25. Rope
26. Eertree
27. Lagrange interpolation
28. 3D geometry
29. Minkowski sum

---

## Nota final

La librería ya tiene una cobertura **muy amplia** — 287 headers cubriendo
estructuras de datos clásicas y avanzadas, grafos, strings, geometría, DP,
number theory, y probabilistic sketches. Los gaps identificados son
principalmente: (1) estructuras dinámicas avanzadas (link-cut, persistent,
wavelet), (2) FFT/matrix exponentiation como building blocks algebraicos, y
(3) algunos problemas combinatorios NP-duros con aproximaciones clásicas.