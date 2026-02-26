# Aleph-w

<div align="center">

Idioma: Español | [English](README.md)

```
     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / /
   / ___ \| |  __/ |_) | | | |_____\ V  V /
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/
                 |_|
```

**Una biblioteca C++20 completa de estructuras de datos y algoritmos**

[![CI](https://github.com/lrleon/Aleph-w/actions/workflows/ci.yml/badge.svg)](https://github.com/lrleon/Aleph-w/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![GCC](https://img.shields.io/badge/GCC-11%2B-orange.svg)](https://gcc.gnu.org/)
[![Clang](https://img.shields.io/badge/Clang-14%2B-orange.svg)](https://clang.llvm.org/)

</div>

---

<a id="readme-es-contenido"></a>
## Contenido

- [Resumen](#readme-es-resumen)
- [Requisitos](#readme-es-requisitos)
- [Instalación](#readme-es-instalacion)
- [Uso rápido](#readme-es-uso-rapido)
- [Pruebas](#readme-es-pruebas)
- [Contribuir](#readme-es-contribuir)
- [Licencia](#readme-es-licencia)
- [Agradecimientos](#readme-es-agradecimientos)

> Nota: `README.md` (en inglés) contiene el detalle completo (listados extensos de estructuras, algoritmos, tablas y ejemplos), incluyendo emparejamiento máximo en grafos generales con Edmonds-Blossom (`Blossom.H`), emparejamiento de máximo peso en grafos generales (`Blossom_Weighted.H`, validado en `List_Graph`, `List_SGraph` y `Array_Graph`), emparejamiento de costo minimo en grafos generales (`Min_Cost_Matching.H`, con variante de matching perfecto factible/no factible), el algoritmo Húngaro/Munkres para asignación óptima (`Hungarian.H`), LCA en árboles sobre grafos de Aleph (`LCA.H`, binary lifting y Euler+RMQ), descomposiciones de árbol avanzadas (`Tree_Decomposition.H`: Heavy-Light + Centroid Decomposition), la suite completa de strings (`String_Search.H`, `Aho_Corasick.H`, `Suffix_Structures.H`, `String_Palindromes.H`, `String_DP.H`, `String_Algorithms.H`), el flujo completo de Delaunay/Voronoi y los ejemplos:
> - `Examples/blossom_example.cc`
> - `Examples/weighted_blossom_example.cc`
> - `Examples/min_cost_matching_example.cc`
> - `Examples/hungarian_example.cc`
> - `Examples/lca_example.cc`
> - `Examples/heavy_light_decomposition_example.cc`
> - `Examples/centroid_decomposition_example.cc`
> - `Examples/geom_example.C` (incluye selector `-s advanced`)
> - `Examples/voronoi_clipped_cells_example.cc`
> - `Examples/delaunay_voronoi_example.cc`
> - `Examples/point_in_polygon_example.cc`
> - `Examples/polygon_intersection_example.cc`
> - `Examples/halfplane_intersection_example.cc`
> - `Examples/convex_hull_comparison_example.cc`
> - `Examples/closest_pair_example.cc`
> - `Examples/rotating_calipers_example.cc`
> - `Examples/tikz_polygons_example.cc`
> - `Examples/tikz_convex_hull_example.cc`
> - `Examples/tikz_intersection_example.cc`
> - `Examples/tikz_voronoi_power_example.cc`
> - `Examples/tikz_advanced_algorithms_example.cc`
> - `Examples/tikz_funnel_animation_example.cc`
> - `Examples/tikz_funnel_beamer_example.cc`
> - `Examples/tikz_funnel_beamer_twocol_example.cc`
> - `Examples/tikz_funnel_beamer_handout_example.cc`
> - `Examples/tikz_scene_example.cc`
> - `Examples/tikz_scene_beamer_example.cc`
> - `Examples/tikz_scene_overlays_example.cc`
> - `Examples/kmp_example.cc`
> - `Examples/z_algorithm_example.cc`
> - `Examples/horspool_example.cc`
> - `Examples/rabin_karp_example.cc`
> - `Examples/aho_corasick_example.cc`
> - `Examples/suffix_array_lcp_example.cc`
> - `Examples/suffix_tree_example.cc`
> - `Examples/suffix_automaton_example.cc`
> - `Examples/manacher_example.cc`
> - `Examples/edit_distance_example.cc`
> - `Examples/damerau_levenshtein_example.cc`
> - `Examples/lcs_longest_common_substring_example.cc`

---

<a id="readme-es-resumen"></a>
## Resumen

**Aleph-w** es una librería C++ (C++20) orientada a enseñanza y uso práctico: incluye implementaciones de estructuras de datos, algoritmos clásicos (especialmente de grafos) y una suite de pruebas amplia.

- **238 archivos de cabecera** cubriendo algoritmos clásicos y modernos.
- **90+ estructuras de datos** con múltiples variantes.
- **50+ algoritmos de grafos** incluyendo flujos, cortes y caminos.
- **80+ programas de ejemplo** con comentarios detallados.

### Características de un vistazo

#### Estructuras de Datos

```
┌────────────────────────────────────────────────────────────────────────────┐
│                  RESUMEN DE ESTRUCTURAS DE DATOS                           │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  ÁRBOLES                  TABLAS HASH              HEAPS                   │
│  ├─ Árbol AVL            ├─ Chaining              ├─ Binary Heap           │
│  ├─ Árbol Rojo-Negro     ├─ Open Addressing       ├─ Fibonacci Heap        │
│  ├─ Árbol Splay          ├─ Linear Probing        └─ Array Heap            │
│  ├─ Treap                └─ Linear Hashing                                 │
│  ├─ Rand Tree                                                              │
│  ├─ Skip List                                                              │
│  └─ Variantes con Rank                                                     │
│                                                                            │
│  LISTAS                   ARREGLOS                 GRAFOS                  │
│  ├─ Simplemente ligada   ├─ Arreglo dinámico      ├─ Lista de adyacencia   │
│  ├─ Doblemente ligada    ├─ Arreglo fijo          ├─ Matriz de adyacencia  │
│  ├─ Circular             ├─ Matriz 2D             ├─ Redes (flujos)        │
│  └─ Skip List            └─ BitArray              └─ Euclidianos           │
│                                                                            │
│  ESPECIALES               ESPACIALES               PROBABILÍSTICAS         │
│  ├─ Union-Find           ├─ Quadtree              ├─ Bloom Filter          │
│  ├─ LRU Cache            ├─ 2D-Tree               └─ Skip List             │
│  └─ Prefix Tree (Trie)   └─ K-d Tree                                       │
│                                                                            │
│  GEOMETRÍA                                                                 │
│  ├─ Primitivas (Punto, Segmento, Polígono, Elipse)                         │
│  ├─ Predicados Exactos (orientación, intersección, in_circle)              │
│  ├─ Convex Hull (Andrew, Graham, QuickHull)                                │
│  ├─ Triangulación (Ear-Cutting, Delaunay, CDT)                             │
│  ├─ Proximidad (Closest Pair, MEC, Rotating Calipers)                      │
│  ├─ Diagramas (Voronoi, Power Diagram)                                     │
│  └─ Visualización (TikZ/PGF backend)                                       │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

#### Algoritmos

```
┌────────────────────────────────────────────────────────────────────────────┐
│                          RESUMEN DE ALGORITMOS                             │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  CAMINOS MÍNIMOS          ÁRBOLES EXPANSORES       CONECTIVIDAD            │
│  ├─ Dijkstra             ├─ Kruskal               ├─ DFS / BFS             │
│  ├─ Bellman-Ford         └─ Prim                  ├─ Componentes Conexas   │
│  ├─ Floyd-Warshall                                ├─ Tarjan / Kosaraju     │
│  ├─ Johnson                                       ├─ Puntos de Articulación│
│  └─ A* Search                                     └─ Biconectividad        │
│                                                                            │
│  FLUJOS EN REDES          CORTE MÍNIMO             EMPAREJAMIENTO (MATCH)  │
│  ├─ Ford-Fulkerson       ├─ Karger                ├─ Hopcroft-Karp         │
│  ├─ Edmonds-Karp         ├─ Karger-Stein          ├─ Edmonds-Blossom       │
│  ├─ Push-Relabel         └─ Stoer-Wagner          └─ Húngaro (Asignación)  │
│  ├─ Dinic                                                                  │
│  └─ Min-Cost Max-Flow                                                      │
│                                                                            │
│  ORDENAMIENTO             BÚSQUEDA                 STRINGS                 │
│  ├─ Quicksort            ├─ Búsqueda Binaria      ├─ Pattern Matching      │
│  ├─ Mergesort            └─ Interpolación         ├─ Suffix Structures     │
│  ├─ Heapsort                                      ├─ Edit Distance / LCS   │
│  ├─ Introsort                                     └─ Palíndromos (Manacher)│
│  └─ Shell Sort                                                             │
│                                                                            │
│  OTROS                                                                     │
│  ├─ Union-Find           ├─ Huffman Coding        ├─ Simplex (LP)          │
│  └─ RMQ/LCA/HLD/Centroid └─ BitArray                                       │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

---

<a id="readme-es-requisitos"></a>
## Requisitos

### Compiladores

- **GCC** 11+ (recomendado 13+)
- **Clang** 14+ (recomendado 16+)
- **MSVC**: no soportado oficialmente

### Dependencias (esta rama)

Esta rama enlaza contra estas librerías:

| Librería | Paquete (Ubuntu/Debian) | Uso | Requerida |
|---------|--------------------------|-----|----------|
| GMP + gmpxx | `libgmp-dev` | enteros/racionales multiprecisión | Sí |
| MPFR | `libmpfr-dev` | punto flotante multiprecisión | Sí |
| GSL (+ gslcblas) | `libgsl-dev` | RNG/estadística usada por estructuras/algoritmos aleatorizados | Sí |
| X11 | `libx11-dev` | dependencia de enlace | Sí |
| GoogleTest | `libgtest-dev` | tests | Solo si compilas tests (auto-fetch opcional) |

### Herramientas

- **CMake** 3.18+
- **CMake presets**: `CMakePresets.json` requiere CMake 3.21+ (opcional)
- **Ninja** (recomendado) o **Make**

### Instalación rápida (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build g++ clang \
  libgmp-dev libmpfr-dev libgsl-dev libx11-dev

# Opcional (si quieres tests sin auto-fetch, o si estás offline)
sudo apt-get install -y libgtest-dev
```

---

<a id="readme-es-instalacion"></a>
## Instalación

Guía detallada: `INSTALL.es.md`.

### Método 1: CMake (recomendado)

```bash
git clone https://github.com/lrleon/Aleph-w.git
cd Aleph-w

cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_TESTS=ON

cmake --build build --parallel
ctest --test-dir build --output-on-failure

# Instalación (opcional)
sudo cmake --install build
```

Si GoogleTest no está instalado y no tienes red, desactiva el auto-fetch (`-DALEPH_FETCH_GTEST=OFF`) o desactiva tests (`-DBUILD_TESTS=OFF`), y/o instala `libgtest-dev`.

### Método 2: Compilación directa (sin instalar)

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF
cmake --build build --parallel

g++ -std=c++20 -I/path/to/Aleph-w your_code.cpp /path/to/Aleph-w/build/libAleph.a \
  -lgmpxx -lgmp -lmpfr -lgsl -lgslcblas -lpthread -lX11 -lm \
  -o your_program
```

### Método 3: CMake FetchContent (subproyecto)

```cmake
include(FetchContent)
FetchContent_Declare(
    aleph-w
    GIT_REPOSITORY https://github.com/lrleon/Aleph-w.git
    GIT_TAG v3
)
FetchContent_MakeAvailable(aleph-w)

target_link_libraries(your_target PRIVATE Aleph)
```

---

<a id="readme-es-uso-rapido"></a>
## Uso rápido

Los headers están en el raíz del repo (p.ej. `tpl_dynSetTree.H`, `tpl_graph.H`, etc.).

### Ejemplo 1: Set con árbol balanceado

```cpp
#include <tpl_dynSetTree.H>
#include <iostream>

int main() {
    // Soporta Avl_Tree, Rb_Tree, Splay_Tree, Treap, Rand_Tree y variantes *Rk
    DynSetTree<int, Avl_Tree> numbers;

    numbers.insert(50);
    numbers.insert(30);
    numbers.insert(70);
    numbers.insert(20);
    numbers.insert(40);

    if (numbers.contains(30))
        std::cout << "Found 30!\n";

    std::cout << "Sorted: ";
    numbers.for_each([](int x) { std::cout << x << " "; });

    return 0;
}
```

### Ejemplo 2: Grafo + Dijkstra

```cpp
#include <tpl_graph.H>
#include <Dijkstra.H>
#include <iostream>

int main() {
    using Node = Graph_Node<std::string>;
    using Arc = Graph_Arc<double>;
    using Graph = List_Graph<Node, Arc>;

    Graph city_map;

    auto* ny = city_map.insert_node("New York");
    auto* la = city_map.insert_node("Los Angeles");
    auto* ch = city_map.insert_node("Chicago");

    city_map.insert_arc(ny, ch, 790.0);
    city_map.insert_arc(ch, la, 2015.0);
    city_map.insert_arc(ny, la, 2790.0);

    Path<Graph> path(city_map);
    double distance = dijkstra_min_path(city_map, ny, la, path);

    std::cout << "Shortest distance: " << distance << "\n";
    return 0;
}
```

### Ejemplo 3: Fenwick Tree (sumas de prefijo y rangos)

```cpp
#include <tpl_fenwick_tree.H>
#include <iostream>

int main() {
    // Fenwick clásico: actualización puntual + consulta de rango
    Fenwick_Tree<int> ft = {3, 1, 4, 1, 5, 9};
    std::cout << "Suma [0..3]: " << ft.prefix(3) << "\n";   // 9
    std::cout << "Suma [2..4]: " << ft.query(2, 4) << "\n";  // 10

    ft.update(2, 6);  // a[2] += 6
    std::cout << "Suma [2..4] tras update: " << ft.query(2, 4) << "\n"; // 16

    // find_kth: menor índice cuyo prefijo acumulado >= k
    size_t idx = ft.find_kth(10);  // útil para order statistics

    // Range Fenwick: actualización y consulta por rangos
    Range_Fenwick_Tree<int> rft(8);
    rft.update(1, 4, 3);   // a[1..4] += 3
    rft.update(2, 6, 5);   // a[2..6] += 5
    std::cout << "Valor en pos 3: " << rft.get(3) << "\n";  // 8

    // Genérico sobre cualquier grupo abeliano (e.g. XOR)
    struct Xor { int operator()(int a, int b) const { return a ^ b; } };
    Gen_Fenwick_Tree<int, Xor, Xor> xft(10);
    xft.update(3, 0b1010);

    return 0;
}
```

### Ejemplo 3b: Descomposiciones de árbol (HLD + Centroid)

```cpp
#include <Tree_Decomposition.H>
#include <tpl_graph.H>

using G = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

int main() {
    G g;
    auto * r = g.insert_node(10);
    auto * a = g.insert_node(4);
    auto * b = g.insert_node(7);
    g.insert_arc(r, a, 1);
    g.insert_arc(r, b, 1);

    // HLD + segment tree (suma en paths/subárboles)
    HLD_Path_Query<G, int, Aleph::plus<int>> hldq(g, r, 0);
    int path_sum = hldq.query_path(a, b);   // 4 + 10 + 7 = 21
    int sub_sum  = hldq.query_subtree(r);   // 21

    // Centroid decomposition (cadenas de ancestros centroid + distancias)
    Centroid_Decomposition<G> cd(g, r);
    cd.for_each_centroid_ancestor(a, [&](size_t c, size_t d, size_t k) {
        (void)c; (void)d; (void)k; // usar para consultas dinámicas por distancia
    });

    (void)path_sum; (void)sub_sum;
    return 0;
}
```

#### ¿Qué problema resuelve `Tree_Decomposition.H`?

Patrones típicos:
- **Consultas online sobre caminos** con actualizaciones puntuales por nodo.
  Ejemplo: \"suma de riesgo entre sucursal A y B\" muchas veces.
- **Consultas online por subárbol** con actualizaciones puntuales.
  Ejemplo: \"costo total de una región operativa\" donde cambian nodos individuales.
- **Nodo activo más cercano / más lejano** en árboles (distancia en número de aristas).
  Ejemplo: centros de atención, estaciones de emergencia, hubs activos.

Guía rápida:
- Si el núcleo es `path(u, v)` o `subtree(u)` + updates puntuales: usa **HLD** (`Gen_HLD_Path_Query`).
- Si el núcleo es distancia a un conjunto dinámico de nodos marcados: usa **Centroid Decomposition** (`Gen_Centroid_Decomposition`).
- Si solo necesitas ancestros/LCA/distancia estática: `LCA.H` es más simple.
- Si todo es offline y no monoidal: evalúa `tpl_mo_on_trees.H`.

Checklist de diagnóstico (30 segundos):
1. ¿El árbol es estático y solo necesitas `lca`, ancestros o distancia? Usa `LCA.H`.
2. ¿Necesitas consultas online `path(u,v)` o `subtree(u)` con operación asociativa y updates puntuales por nodo? Usa `Gen_HLD_Path_Query`.
3. ¿Mantienes un conjunto dinámico de nodos activos y consultas el más cercano/más lejano? Usa `Gen_Centroid_Decomposition`.
4. ¿Todo es offline y se resuelve con add/remove (no monoidal)? Usa `tpl_mo_on_trees.H`.
5. ¿Necesitas distancias con aristas ponderadas? `Gen_Centroid_Decomposition` actual trabaja con número de aristas; adapta el flujo antes de aplicar fórmulas de centroid chain.

Recetas rápidas:
- **Suma en camino con updates puntuales**: `HLD_Path_Query<..., Aleph::plus<int>>`, luego `query_path(u,v)`, `update_node(x,delta)`, `set_node(x,val)`.
- **Total por subárbol**: mismo `HLD_Path_Query`, usando `query_subtree(u)`.
- **Centro activo más cercano**: `Centroid_Decomposition`, mantener `best[centroid]` y combinar sobre `for_each_centroid_ancestor()`.
- Referencias listas para ejecutar: `Examples/heavy_light_decomposition_example.cc` y `Examples/centroid_decomposition_example.cc`.

### Ejemplo 4: Álgebra Lineal Sparse (matrices y vectores dispersos)

```cpp
#include <al-vector.H>
#include <al-matrix.H>
#include <iostream>

int main() {
    // Vector disperso con dominio de strings
    AlDomain<std::string> productos;
    productos.insert("Laptop");
    productos.insert("Teléfono");
    productos.insert("Tablet");

    Vector<std::string, double> inventario(productos);
    inventario.set_entry("Laptop", 150.0);
    inventario.set_entry("Teléfono", 450.0);
    // Tablet implícitamente 0 (no se almacena)

    std::cout << "Inventario Laptop: " << inventario["Laptop"] << "\n";

    // Matriz dispersa con indexación por dominio
    AlDomain<std::string> tiendas;
    tiendas.insert("Bogotá");
    tiendas.insert("Medellín");
    tiendas.insert("Cali");

    Matrix<std::string, std::string, int> ventas(productos, tiendas);
    ventas.set_entry("Laptop", "Bogotá", 25);
    ventas.set_entry("Teléfono", "Medellín", 80);
    // Otras entradas son cero implícitamente (no se almacenan)

    // Multiplicación matriz-vector
    Vector<std::string, int> total_por_producto = ventas * inventario;

    // Transpuesta
    auto ventas_t = ventas.transpose();

    // Ventaja: matriz 1000×1000 con 1000 no-ceros:
    //   Densa:  1,000,000 doubles = 8 MB
    //   Sparse:     1,000 entries = 8 KB (ahorro 1000×)

    return 0;
}
```

### Ejemplo 5: Voronoi recortado (celdas indexadas por sitio)

El ejemplo `Examples/voronoi_clipped_cells_example.cc` muestra cómo:

- construir Voronoi desde un conjunto de puntos,
- recortar todas las celdas contra un polígono convexo, y
- exportar celdas `ClippedCell` a CSV/WKT.

```bash
cmake --build build --target voronoi_clipped_cells_example
./build/Examples/voronoi_clipped_cells_example /tmp/voronoi_clipped_cells.csv
```

### Ejemplo 6: Visualización TikZ/LaTeX (PGF)

Para generar figuras `.tex` con objetos geométricos y resultados de algoritmos:

```bash
cmake --build build --target tikz_polygons_example tikz_convex_hull_example tikz_intersection_example tikz_voronoi_power_example tikz_advanced_algorithms_example tikz_funnel_animation_example tikz_funnel_beamer_example tikz_funnel_beamer_twocol_example tikz_funnel_beamer_handout_example tikz_scene_example tikz_scene_beamer_example tikz_scene_overlays_example
./build/Examples/tikz_polygons_example /tmp/tikz_polygons_example.tex
./build/Examples/tikz_convex_hull_example /tmp/tikz_convex_hull_example.tex
./build/Examples/tikz_intersection_example /tmp/tikz_intersection_example.tex
./build/Examples/tikz_voronoi_power_example /tmp/tikz_voronoi_power_example.tex
./build/Examples/tikz_advanced_algorithms_example /tmp/tikz_advanced_algorithms_example.tex
./build/Examples/tikz_funnel_animation_example /tmp/tikz_funnel_animation_example.tex
./build/Examples/tikz_funnel_beamer_example /tmp/tikz_funnel_beamer_example.tex
./build/Examples/tikz_funnel_beamer_twocol_example /tmp/tikz_funnel_beamer_twocol_example.tex
./build/Examples/tikz_funnel_beamer_handout_example /tmp/tikz_funnel_beamer_handout_example.tex
./build/Examples/tikz_scene_example /tmp/tikz_scene_example.tex
./build/Examples/tikz_scene_beamer_example /tmp/tikz_scene_beamer_example.tex
./build/Examples/tikz_scene_beamer_example /tmp/tikz_scene_handout_example.tex --handout
./build/Examples/tikz_scene_overlays_example /tmp/tikz_scene_overlays_example.tex
./build/Examples/tikz_scene_overlays_example /tmp/tikz_scene_overlays_handout.tex --handout
```

Headers relevantes:
- `tikzgeom.H`: `Tikz_Plane`, estilos y capas.
- `tikzgeom_scene.H`: `Tikz_Scene`, interfaz de composición para mezclar múltiples visualizaciones en una sola figura/export (standalone, beamer y handout), incluyendo secuencias multi-paso con overlays.
- `tikzgeom_algorithms.H`: helpers para convex hull, intersecciones, Voronoi/power diagram, arrangement, shortest path (con portales), trazas/frames del funnel, versiones beamer (incluyendo layout en dos columnas y handout), descomposición convexa y alpha shape.
- `docs/TIKZGEOM_GUIDE.md`: guia completa de interfaz y extension.

### Ejemplo 7: Algoritmos de strings clásicos

Cabeceras principales:
- `String_Search.H`: KMP, Z-algorithm, Boyer-Moore-Horspool, Rabin-Karp.
- `Aho_Corasick.H`: matching multi-patrón.
- `Suffix_Structures.H`: suffix array + LCP (Kasai), suffix tree didáctico, suffix automaton.
- `String_Palindromes.H`: Manacher.
- `String_DP.H`: Levenshtein, Damerau-Levenshtein, LCS, longest common substring.
- `String_Algorithms.H`: cabecera paraguas.

```cpp
#include <String_Algorithms.H>
#include <iostream>

int main() {
    auto kmp = Aleph::kmp_search("ababaabababa", "ababa");

    Aleph::Aho_Corasick ac;
    ac.add_pattern("he");
    ac.add_pattern("she");
    ac.build();
    auto multi = ac.search("ahishers");

    auto sa = Aleph::suffix_array("banana");
    auto lcp = Aleph::lcp_array_kasai("banana", sa);
    auto pal = Aleph::longest_palindromic_substring("forgeeksskeegfor");
    auto lev = Aleph::levenshtein_distance("kitten", "sitting");

    std::cout << kmp.size() << " " << multi.size() << " "
              << sa.size() << " " << lcp.size() << " "
              << pal << " " << lev << "\n";
}
```

Ejemplos dedicados en `Examples/`:
- `kmp_example.cc`
- `z_algorithm_example.cc`
- `horspool_example.cc`
- `rabin_karp_example.cc`
- `aho_corasick_example.cc`
- `suffix_array_lcp_example.cc`
- `suffix_tree_example.cc`
- `suffix_automaton_example.cc`
- `manacher_example.cc`
- `edit_distance_example.cc`
- `damerau_levenshtein_example.cc`
- `lcs_longest_common_substring_example.cc`

### Ejemplo 8: Programación dinámica clásica

Cabeceras principales:
- `Knapsack.H`: 0/1, unbounded y bounded knapsack (con reconstrucción).
- `LIS.H`: LIS/LNDS en `O(n log n)` con reconstrucción.
- `Matrix_Chain.H`: parentización óptima para producto encadenado de matrices.
- `Subset_Sum.H`: existencia/reconstrucción/conteo + MITM.
- `Tree_DP.H`: DP genérico en árboles y rerooting `O(n)`.

```cpp
#include <Knapsack.H>
#include <LIS.H>
#include <Matrix_Chain.H>
#include <Subset_Sum.H>

int main() {
    Aleph::Array<Aleph::Knapsack_Item<int, int>> items = {{2, 3}, {3, 4}, {4, 5}};
    auto k = Aleph::knapsack_01(items, 6);

    Aleph::Array<int> seq = {10, 9, 2, 5, 3, 7, 101, 18};
    auto lis = Aleph::longest_increasing_subsequence(seq);

    Aleph::Array<size_t> dims = {30, 35, 15, 5, 10, 20, 25};
    auto mc = Aleph::matrix_chain_order(dims);

    Aleph::Array<int> vals = {3, 34, 4, 12, 5, 2};
    auto ss = Aleph::subset_sum(vals, 9);

    return int(k.optimal_value + lis.length + mc.min_multiplications + ss.exists);
}
```

Ejemplos dedicados en `Examples/`:
- `knapsack_example.cc`
- `lis_example.cc`
- `matrix_chain_example.cc`
- `subset_sum_example.cc`
- `tree_dp_example.cc`

---

<a id="readme-es-pruebas"></a>
## Pruebas

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure

# Ejecutar un binario específico
./build/Tests/dynlist
./build/Tests/test_dijkstra
./build/Tests/latex_floyd_test
```

Nota: algunos tests largos/de performance están marcados `Disabled/Skipped` intencionalmente; `ctest` los lista al final.

---

<a id="readme-es-contribuir"></a>
## Contribuir

PRs son bienvenidos.

- Mantén cambios pequeños y con tests cuando aplique.
- Si agregas algoritmos/estructuras, añade tests en `Tests/` y (idealmente) un ejemplo en `Examples/`.

---

<a id="readme-es-licencia"></a>
## Licencia

Aleph-w se distribuye bajo **licencia MIT**. Ver `LICENSE`.

---

<a id="readme-es-agradecimientos"></a>
## Agradecimientos

- **Universidad de Los Andes** (Mérida, Venezuela) - Cuna de Aleph-w
- **Miles de estudiantes** que aprendieron algoritmos con esta biblioteca
- **Comunidad de código abierto** por retroalimentación continua
- **[SYMYL RESEARCH](https://simylresearch.com/en/)** por apoyar el desarrollo y avance de Aleph-w

---
