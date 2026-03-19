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

<a id="readme-es-tabla-de-contenido"></a>
## Tabla de contenido

- [Resumen](#readme-es-resumen)
- [Historia y filosofía](#readme-es-historia-y-filosofia)
- [Características de un vistazo](#readme-es-caracteristicas-de-un-vistazo)
- [Requisitos](#readme-es-requisitos)
- [Instalación](#readme-es-instalacion)
- [Uso rápido](#readme-es-uso-rapido)
- [Estructuras de datos](#readme-es-estructuras-de-datos)
  - [Árboles de búsqueda balanceados](#readme-es-arboles-balanceados)
  - [Tablas hash](#readme-es-tablas-hash)
  - [Heaps y colas de prioridad](#readme-es-heaps)
  - [Listas y estructuras secuenciales](#readme-es-estructuras-lineales)
  - [Estructuras para consultas por rango](#readme-es-consultas-por-rango)
  - [Grafos](#readme-es-grafos)
  - [Geometría computacional](#readme-es-geometria)
  - [Álgebra lineal (estructuras sparse)](#readme-es-algebra-lineal)
- [Algoritmos](#readme-es-algoritmos)
  - [Algoritmos de caminos mínimos](#readme-es-caminos-minimos)
  - [Árboles de expansión mínima](#readme-es-mst)
  - [Flujos en redes](#readme-es-flujos)
  - [Conectividad en grafos](#readme-es-conectividad)
  - [2-SAT (Satisfacibilidad)](#readme-es-two-sat)
  - [Emparejamiento (matching)](#readme-es-emparejamiento)
  - [Algoritmos de strings](#readme-es-algoritmos-de-cadenas)
  - [Algoritmos de ordenamiento](#readme-es-algoritmos-de-ordenamiento)
  - [Algoritmos de programación dinámica](#readme-es-programacion-dinamica)
  - [Procesamiento de señales (FFT)](#readme-es-procesamiento-de-senales)
  - [Transformada teórico-numérica (NTT)](#readme-es-ntt)
  - [Aritmética de polinomios](#readme-es-aritmetica-de-polinomios)
- [Gestión de memoria](#readme-es-gestion-de-memoria)
  - [Allocators tipo arena](#readme-es-asignadores-arena)
- [Cómputo paralelo](#readme-es-computacion-paralela)
- [Programación funcional](#readme-es-programacion-funcional)
- [Tutorial](#readme-es-tutorial)
- [Referencia de API](#readme-es-referencia-api)
- [Benchmarks](#readme-es-benchmarks)
- [Ejemplos](#readme-es-ejemplos)
- [Pruebas](#readme-es-testing)
- [Contribuir](#readme-es-contribuir)
- [Licencia](#readme-es-licencia)
- [Agradecimientos](#readme-es-agradecimientos)

---

<a id="readme-es-resumen"></a>
## Resumen

**Aleph-w** es una biblioteca C++20 lista para producción que ofrece más de **90 estructuras de datos** y **50+ algoritmos** para ingenieros de software, investigadores y estudiantes. Con **238 archivos de cabecera** y **80+ ejemplos**, es una de las bibliotecas de algoritmos más completas disponibles.

### ¿Por qué Aleph-w?

| Característica | STL | Boost | Aleph-w |
|---------|-----|-------|---------|
| Árboles balanceados (AVL, RB, Splay, Treap) | Limitado | Parcial | **8 variantes** |
| Estadística de orden (k-ésimo, rank) | Ninguna | Ninguna | **Todos los árboles** |
| Algoritmos de grafos | Ninguno | BGL | **50+ algoritmos** |
| Flujos en redes (max-flow, min-cost) | Ninguno | Ninguno | **Suite completa** |
| Allocators tipo arena | Ninguno | Ninguno | **Incluido** |
| map/filter/fold paralelo | Limitado | Limitado | **Soporte nativo** |
| Programación funcional | Limitado | Parcial | **Completa** |
| Ejemplos educativos | Ninguno | Pocos | **80+ programas** |

### Estadísticas clave

- **238 archivos de cabecera** cubriendo algoritmos clásicos y modernos
- **90+ estructuras de datos** con múltiples variantes de implementación
- **50+ algoritmos de grafos** incluyendo flujos, cortes y caminos
- **80+ programas de ejemplo** con comentarios detallados
- **7,700+ casos de prueba** con 99.98% de éxito
- **Soporte multi-compilador**: GCC 11+ y Clang 14+

---

<a id="readme-es-historia-y-filosofia"></a>
## Historia y filosofía

### Orígenes (2002)

Aleph-w nació en 2002 en la **Universidad de Los Andes** en Mérida, Venezuela, creado por **Leandro Rabindranath León** como herramienta de enseñanza para cursos de algoritmos y estructuras de datos.

El nombre "**Aleph**" (א) referencia el cuento de Jorge Luis Borges "[El Aleph](https://en.wikipedia.org/wiki/The_Aleph_(short_story))" — un punto en el espacio que contiene todos los demás puntos. Esto simboliza la meta de la biblioteca: ser un punto único que contiene todos los algoritmos fundamentales.

El sufijo "**-w**" significa "**with**" (con) — enfatizando que esta biblioteca viene *con* todo lo que necesitas: implementaciones, ejemplos, pruebas y documentación.

### Filosofía de diseño

```
"La mejor forma de aprender algoritmos es implementarlos,
 y la mejor forma de enseñarlos es mostrar implementaciones limpias."
                                        — Leandro R. León
```

**Principios centrales:**

1. **Transparencia sobre abstracción**
   - Los detalles de implementación son explícitos; no están ocultos
   - `DynList<T>` es simplemente ligada; `DynDlist<T>` es doblemente ligada
   - Siempre sabes la complejidad de cada operación

2. **Completitud**
   - Múltiples implementaciones del mismo concepto (AVL vs RB vs Splay)
   - Compara enfoques y entiende los trade-offs
   - Todos los clásicos, no solo lo “más común”

3. **Valor educativo**
   - Código escrito para leerse y entenderse
   - Ejemplos que enseñan, no solo demuestran

4. **Listo para producción**
   - Ampliamente probado (7,700+ tests)
   - Usado en aplicaciones reales
   - Rendimiento competitivo

5. **Estilo funcional**
   - Todos los contenedores soportan `map`, `filter`, `fold`
   - Operaciones composables
   - Inmutabilidad cuando es práctico

### Línea de tiempo de evolución

```
2002 ──────── Implementación original C++ para enseñanza en la ULA
     │        Enfoque: árboles, listas y algoritmos básicos de grafos
     │
2008 ──────── Expansión importante de algoritmos de grafos
     │        Añadido: flujos en redes, min-cost flows, cortes
     │
2012 ──────── Soporte de programación funcional
     │        Añadido: map, filter, fold para todos los contenedores
     │
2015 ──────── Modernización C++11/14
     │        Añadido: move semantics, lambdas, auto
     │
2020 ──────── C++17 y expansión de testing
     │        Añadido: structured bindings, if-init, GoogleTest
     │
2025 ──────── Migración C++20, allocators tipo arena
     │        Añadido: concepts, ranges, constexpr, memoria arena
     │
2026 ──────── Compatibilidad completa GCC/Clang (actual)
             Corregido: 200+ warnings, issues de templates, ABI
             Añadido: CI/CD, sanitizers, documentación
```

### Impacto académico

Aleph-w ha sido usado para enseñar a **miles de estudiantes** en Latinoamérica. Su diseño educativo incluye:

- **Implementaciones lado a lado**: compara Kruskal vs Prim, Dijkstra vs Bellman-Ford
- **Exportación de visualizaciones**: salida LaTeX/EEPIC para papers y presentaciones
- **Análisis de complejidad**: cada operación documenta su Big-O
- **Ejemplos extensos**: 80+ programas demostrando uso real

---

<a id="readme-es-caracteristicas-de-un-vistazo"></a>
## Características de un vistazo

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
│  ├─ LRU Cache            ├─ 2D-Tree               ├─ Cuckoo Filter         │
│  └─ Prefix Tree (Trie)   └─ K-d Tree               └─ HyperLogLog/MinHash/SimHash/Reservoir/CMS │
│                                                                            │
│  CONSULTAS POR RANGO                                                       │
│  ├─ Fenwick Tree (BIT)                                                     │
│  ├─ Sparse Table (RMQ)                                                     │
│  ├─ Disjoint Sparse Table                                                  │
│  ├─ Segment Tree / Lazy / Beats                                            │
│  ├─ Cartesian Tree RMQ (via LCA)                                           │
│  ├─ Interval Tree (overlap/stabbing queries)                               │
│  ├─ Heavy-Light Decomposition (path/subtree)                               │
│  ├─ Centroid Decomposition (distance tricks)                               │
│  ├─ Link-Cut Tree (bosque dinámico + consultas de camino)                  │
│  └─ Mo's Algorithm (Offline)                                               │
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
│  ├─ A* Search                                     ├─ Biconectividad        │
│  └─ 0-1 BFS / Dial / Bidirectional / IDA*         │                        │
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
│  ├─ RMQ/LCA/HLD/Centroid ├─ BitArray                                       │
│  └─ Sketches streaming    └─ (HLL/CMS/MinHash/SimHash/Reservoir)           │
│                           └─ Aritmética de polinomios dispersos            │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

---

<a id="readme-es-requisitos"></a>
## Requisitos

### Soporte de compiladores

| Compilador | Versión mínima | Recomendado | Estado |
|----------|-----------------|-------------|--------|
| GCC | 11 | 13 | Totalmente probado |
| Clang | 14 | 16 | Totalmente probado |
| MSVC | No soportado | - | Planeado |

### Dependencias

Esta rama construye una biblioteca estática (`libAleph.a`) más los headers.

| Librería | Paquete (Ubuntu/Debian) | Propósito | Requerida |
|---------|--------------------------|---------|----------|
| GMP + gmpxx | `libgmp-dev` | Enteros/racionales multiprecisión (y bindings C++) | Sí |
| MPFR | `libmpfr-dev` | Punto flotante multiprecisión | Sí |
| GSL (+ gslcblas) | `libgsl-dev` | RNG/estadística usada por estructuras/algoritmos aleatorizados | Sí |
| X11 | `libx11-dev` | Dependencia de enlace | Sí |
| GoogleTest | `libgtest-dev` | Unit tests | Solo para `BUILD_TESTS` (auto-fetch soportado) |

### Herramientas de build

- **CMake** 3.18+
- **CMake presets**: `CMakePresets.json` requiere CMake 3.21+ (opcional)
- **Ninja** (recomendado) o **Make**

### Instalación rápida (Ubuntu/Debian)

```bash
# Essential
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build

# Compilers
sudo apt-get install -y g++ clang

# Required libraries (this branch)
sudo apt-get install -y libgmp-dev libmpfr-dev libgsl-dev libx11-dev

# Optional (only needed if you want to build tests without auto-fetch)
sudo apt-get install -y libgtest-dev
```

---

<a id="readme-es-instalacion"></a>
## Instalación

Para instrucciones detalladas por plataforma y troubleshooting, ver `INSTALL.md`.

### Método 1: build con CMake (recomendado)

```bash
# Clone the repository
git clone https://github.com/lrleon/Aleph-w.git
cd Aleph-w

# Configure (Release build, library + tests; disable examples for faster builds)
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_TESTS=ON

# Build
cmake --build build --parallel

# Run tests
ctest --test-dir build --output-on-failure

# Install system-wide (optional)
sudo cmake --install build
```

Nota: si GoogleTest no está instalado y estás offline, configura con `-DALEPH_FETCH_GTEST=OFF` (o desactiva tests con `-DBUILD_TESTS=OFF`) y/o instala `libgtest-dev` desde tu distro.

### Método 2: compilación directa (sin instalar)

Si prefieres no instalar, puedes compilar Aleph una vez y luego enlazar directamente:

```bash
# Build the library
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF
cmake --build build --parallel

# Compile and link against the static library
g++ -std=c++20 -I/path/to/Aleph-w your_code.cpp /path/to/Aleph-w/build/libAleph.a \
  -lgmpxx -lgmp -lmpfr -lgsl -lgslcblas -lpthread -lX11 -lm \
  -o your_program
```

### Método 3: CMake FetchContent

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

### Opciones de CMake

| Opción | Default | Descripción |
|--------|---------|-------------|
| `ALEPH_CXX_STANDARD` | 20 | Estándar C++ para la biblioteca core (`17`, `20`, `23`) |
| `BUILD_TESTS` | ON | Compilar la suite de tests (`Tests/`) |
| `BUILD_EXAMPLES` | ON | Compilar los ejemplos (`Examples/`) |
| `BUILD_OPTIMIZED` | OFF | Si está en ON y `CMAKE_BUILD_TYPE` no se define, default a `Release` |
| `ALEPH_FETCH_GTEST` | ON | (Tests) Auto-fetch GoogleTest si falta |
| `USE_SANITIZERS` | OFF | (Tests) Habilitar ASan/UBSan para los binarios de test |

### CMake Presets (opcional)

Si tienes CMake 3.21+, puedes usar los presets provistos:

```bash
cmake --preset default
cmake --build --preset default
ctest --preset default
```

---

<a id="readme-es-uso-rapido"></a>
## Uso rápido

### Ejemplos en 5 minutos

#### Ejemplo 1: set con árbol balanceado

```cpp
#include <tpl_dynSetTree.H>
#include <iostream>

int main() {
    // Create an AVL tree-based set
    DynSetTree<int, Avl_Tree> numbers;

    // Insert elements - O(log n) each
    numbers.insert(50);
    numbers.insert(30);
    numbers.insert(70);
    numbers.insert(20);
    numbers.insert(40);

    // Search - O(log n)
    if (numbers.contains(30))
        std::cout << "Found 30!\n";

    // Iterate in sorted order
    std::cout << "Sorted: ";
    numbers.for_each([](int x) {
        std::cout << x << " ";
    });
    // Output: 20 30 40 50 70

    // Functional operations
    auto doubled = numbers.map<DynList>([](int x) { return x * 2; });
    auto big = numbers.filter([](int x) { return x > 35; });

    return 0;
}
```

#### Ejemplo 2: grafo con camino mínimo

```cpp
#include <tpl_graph.H>
#include <Dijkstra.H>
#include <iostream>

int main() {
    // Define graph types
    using Node = Graph_Node<std::string>;
    using Arc = Graph_Arc<double>;
    using Graph = List_Graph<Node, Arc>;

    Graph city_map;

    // Add cities (nodes)
    auto* ny = city_map.insert_node("New York");
    auto* la = city_map.insert_node("Los Angeles");
    auto* ch = city_map.insert_node("Chicago");

    // Add roads with distances (edges)
    city_map.insert_arc(ny, ch, 790.0);   // NY - Chicago
    city_map.insert_arc(ch, la, 2015.0);  // Chicago - LA
    city_map.insert_arc(ny, la, 2790.0);  // NY - LA (direct)

    // Find shortest path NY → LA
    Path<Graph> path(city_map);
    double distance = dijkstra_min_path(city_map, ny, la, path);

    std::cout << "Shortest distance: " << distance << "\n";

    // Print path
    std::cout << "Path: ";
    path.for_each_node([](auto* node) {
        std::cout << node->get_info() << " → ";
    });
    // Output: New York → Chicago → Los Angeles
    return 0;
}
```

#### Ejemplo 3: asignación tipo arena

```cpp
#include <tpl_dynSetTree.H>

int main() {
    // Allocate arena on stack (zero heap allocations!)
    char buffer[64 * 1024];  // 64 KB

    // Tree uses arena for all node allocations
    DynSetTree<int> tree(buffer, sizeof(buffer));

    // Insert until arena is exhausted
    int count = 0;
    for (int i = 0; ; ++i) {
        if (tree.insert(i) == nullptr) {
            count = i;
            break;
        }
    }

    std::cout << "Inserted " << count << " elements\n";
    std::cout << "Arena used: " << tree.arena_allocated_size() << " bytes\n";
    std::cout << "Arena free: " << tree.arena_available_size() << " bytes\n";

    return 0;
}  // All memory freed instantly when buffer goes out of scope
```

---

<a id="readme-es-estructuras-de-datos"></a>
## Estructuras de datos

<a id="readme-es-arboles-balanceados"></a>
### Árboles de búsqueda balanceados

Aleph-w ofrece **8 implementaciones diferentes de árboles balanceados**, cada una optimizada para casos de uso específicos.

#### Comparación de árboles

```
┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│                                BALANCED TREE COMPARISON                                     │
├──────────────┬──────────────┬──────────────┬──────────────┬──────────────┬──────────────────┤
│   Operation  │   AVL Tree   │  Red-Black   │  Splay Tree  │     Treap    │    Rand Tree     │
├──────────────┼──────────────┼──────────────┼──────────────┼──────────────┼──────────────────┤
│   Insert     │  O(log n)    │  O(log n)    │  O(log n)*   │  O(log n)**  │    O(log n)**    │
│   Search     │  O(log n)    │  O(log n)    │  O(log n)*   │  O(log n)**  │    O(log n)**    │
│   Delete     │  O(log n)    │  O(log n)    │  O(log n)*   │  O(log n)**  │    O(log n)**    │
│   Min/Max    │  O(log n)    │  O(log n)    │  O(log n)*   │  O(log n)**  │    O(log n)**    │
├──────────────┼──────────────┼──────────────┼──────────────┼──────────────┼──────────────────┤
│   Height     │ ≤1.44 log n  │  ≤2 log n    │ Unbounded    │  O(log n)**  │    O(log n)**    │
│   Balance    │   Strict     │   Relaxed    │    None      │  Randomized  │    Randomized    │
├──────────────┼──────────────┼──────────────┼──────────────┼──────────────┼──────────────────┤
│  Best For    │ Read-heavy   │  General     │  Locality    │ Simplicity   │ Fast split/join  │
│              │  workloads   │  purpose     │  patterns    │ randomized   │ randomized       │
└──────────────┴──────────────┴──────────────┴──────────────┴──────────────┴──────────────────┘

* Amortized complexity.
** Expected complexity (randomized).
```

#### Ejemplos de uso

```cpp
#include <tpl_dynSetTree.H>

// AVL Tree - Strictest balance, fastest lookups
DynSetTree<int, Avl_Tree> avl;

// Red-Black Tree - Good all-around (default)
DynSetTree<int, Rb_Tree> rb;

// Splay Tree - Self-adjusting, great for temporal locality
DynSetTree<int, Splay_Tree> splay;

// Treap - Randomized priorities, simple implementation
DynSetTree<int, Treap> treap;

// Randomized BST - Randomized insertion, excellent split/join performance
DynSetTree<int, Rand_Tree> rand_tree;

// With Rank support (order statistics)
DynSetTree<int, Avl_Tree_Rk> avl_rank;
int third = avl_rank.select(2);      // Get 3rd smallest (0-indexed)
size_t pos = avl_rank.position(42);  // Get rank of element 42
```

#### Estadísticas de orden con árboles Rank

Todos los tipos de árboles tienen variantes `*Rk` que soportan estadísticas de orden en O(log n):

```cpp
#include <tpl_dynSetTree.H>

int main() {
    DynSetTree<int, Avl_Tree_Rk> set;

    // Insert elements
    for (int x : {50, 30, 70, 20, 40, 60, 80})
        set.insert(x);

    // Find k-th smallest element (0-indexed)
    int first = set.select(0);   // 20 (smallest)
    int third = set.select(2);   // 40
    int last = set.select(6);    // 80 (largest)

    // Find rank of element
    size_t rank_of_50 = set.position(50);  // 3 (4th smallest)

    // Range iteration
    set.for_each_in_range(25, 65, [](int x) {
        std::cout << x << " ";  // Output: 30 40 50 60
    });

    return 0;
}
```

#### Diagrama de estructura

```
                    AVL Tree (height-balanced)
                    ========================

                           [50]                    Height: 3
                          /    \                   Balance factors
                       [30]    [70]                shown in ()
                       /  \    /  \
                    [20] [40][60] [80]

                    After inserting: 20, 30, 40, 50, 60, 70, 80
                    Rotations performed: 2


                    Splay Tree (self-adjusting)
                    ===========================

                    After accessing 20:

                           [20]                    Recently accessed
                              \                    elements move to
                              [30]                 the root
                                 \
                                 [50]
                                /    \
                             [40]    [70]
                                     /  \
                                [60]  [80]
```

### Árboles Fenwick (Binary Indexed Trees)

Aleph-w ofrece **tres variantes de árbol Fenwick** para sumas de prefijo y consultas por rango:

```text
┌───────────────────────────────────────────────────────────────────────────────┐
│                         FENWICK TREE VARIANTS                                 │
├──────────────────┬───────────────┬───────────────┬────────────────────────────┤
│     Operation    │ Gen (Abelian) │ Fenwick_Tree  │  Range_Fen                 │
├──────────────────┼───────────────┼───────────────┼────────────────────────────┤
│ update(i,delta)  │     O(log n)  │   O(log n)    │   O(log n)                 │
│ prefix / get     │     O(log n)  │   O(log n)    │   O(log n)                 │
│ range_query      │     O(log n)  │   O(log n)    │   O(log n)                 │
│ range_update     │       N/A     │     N/A       │   O(log n)                 │
│ find_kth         │       N/A     │   O(log n)    │     N/A                    │
├──────────────────┼───────────────┼───────────────┼────────────────────────────┤
│ Group Operand    │  Arbitrary    │  operator+    │  operator+                 │
│                  │  (XOR, +mod)  │  operator-    │  operator-                 │
├──────────────────┼───────────────┼───────────────┼────────────────────────────┤
│ Best For         │ Custom ops    │ Order stats   │ Promotions                 │
│                  │ (XOR, etc)    │ Find k-th     │ Dividends                  │
└──────────────────┴───────────────┴───────────────┴────────────────────────────┘
```

#### Ejemplos de uso

```cpp
#include <tpl_fenwick_tree.H>

int main() {
    // Point update + Range query (classic Fenwick tree)
    Fenwick_Tree<int> ft = {3, 1, 4, 1, 5};
    ft.update(2, 7);               // a[2] += 7
    int sum = ft.query(1, 4);      // sum of a[1..4]
    auto kth = ft.find_kth(5);     // smallest i with prefix(i) >= 5

    // Range update + Range query
    Range_Fenwick_Tree<int> rft(10);
    rft.update(2, 5, 10);          // add 10 to a[2..5]
    int range_sum = rft.query(0, 7); // sum of a[0..7]

    // Generic Fenwick over XOR group
    struct Xor {
        int operator()(int a, int b) const { return a ^ b; }
    };
    Gen_Fenwick_Tree<int, Xor, Xor> xor_ft(8);
    xor_ft.update(3, 0b1010);      // a[3] ^= 0b1010
    int prefix_xor = xor_ft.prefix(5);

    return 0;
}
```

#### Aplicación en el mundo real: profundidad del libro de órdenes

```cpp
// Stock exchange: find worst price for market order of K shares
Fenwick_Tree<int> ask_book(20);  // 20 price ticks
ask_book.update(0, 120);         // 120 shares at tick 0
ask_book.update(3, 200);         // 200 shares at tick 3
ask_book.update(7, 300);         // 300 shares at tick 7

size_t worst_tick = ask_book.find_kth(250); // Answer: fill 250 shares
// Result: tick 3 ($100.03) — pay worst price of $100.03
```

### Segment Trees

Aleph-w ofrece **tres variantes de segment tree** para consultas por rango dinámicas:

```text
┌───────────────────────────────────────────────────────────────────────────┐
│                        SEGMENT TREE VARIANTS                              │
├──────────────────┬───────────────┬────────────────┬───────────────────────┤
│     Operation    │ Gen_Segment   │ Gen_Lazy_Seg   │  Seg_Tree_Beats       │
├──────────────────┼───────────────┼────────────────┼───────────────────────┤
│ point_update     │   O(log n)    │   O(log n)     │       —               │
│ range_update     │      —        │   O(log n)     │  O(log n) amort.      │
│ range_query      │   O(log n)    │   O(log n)     │  O(log n)             │
│ chmin / chmax    │      —        │      —         │  O(log n) amort.      │
│ build            │     O(n)      │     O(n)       │    O(n)               │
├──────────────────┼───────────────┼────────────────┼───────────────────────┤
│ Requirements     │ Associative   │ Policy-based   │ Signed arithmetic     │
│                  │ monoid        │ (lazy tags)    │                       │
├──────────────────┼───────────────┼────────────────┼───────────────────────┤
│ Best For         │ Sum/min/max   │ Range add/     │ Clamping with         │
│                  │ + point ops   │ assign queries │ sum/min/max queries   │
└──────────────────┴───────────────┴────────────────┴───────────────────────┘
```

**Comparación de estructuras para consultas por rango:**

| Structure | Build | Update | Query | Space | Requirements |
|-----------|-------|--------|-------|-------|-------------|
| Sparse Table | O(n lg n) | — | **O(1)** | O(n lg n) | Idempotent |
| Disjoint Sparse Table | O(n lg n) | — | **O(1)** | O(n lg n) | Associative |
| Fenwick Tree | O(n) | O(lg n) | O(lg n) | O(n) | Invertible (group) |
| **Segment Tree** | O(n) | O(lg n) | O(lg n) | O(n) | Associative (monoid) |
| **Lazy Segment Tree** | O(n) | O(lg n) range | O(lg n) | O(n) | Policy-based |
| **Segment Tree Beats** | O(n) | O(lg n) amort. | O(lg n) | O(n) | Signed arithmetic |
| **Cartesian Tree RMQ** | O(n lg n) | — | **O(1)** | O(n lg n) | Totally ordered |
| **Mo's Algorithm** | O(Q lg Q) | — | O((N+Q)√N) total | O(N+Q) | Offline, decomposable |

#### Ejemplos de uso

```cpp
#include <tpl_segment_tree.H>

int main() {
    // Point update + Range query (sum)
    Sum_Segment_Tree<int> st = {3, 1, 4, 1, 5};
    st.update(2, 10);               // a[2] += 10
    int sum = st.query(1, 3);       // sum of a[1..3]

    // Min/Max variants
    Min_Segment_Tree<int> mn = {5, 2, 4, 7, 1};
    int m = mn.query(0, 3);          // min(5, 2, 4, 7) = 2

    // Range update + Range query (lazy propagation)
    Lazy_Sum_Segment_Tree<int> lazy = {1, 2, 3, 4, 5};
    lazy.update(1, 3, 10);           // a[1..3] += 10
    int s = lazy.query(0, 4);        // sum of entire array

    // Segment Tree Beats (chmin/chmax)
    Segment_Tree_Beats<int> beats = {75, 90, 45, 60};
    beats.chmin(0, 3, 70);           // cap all at 70
    beats.chmax(0, 3, 50);           // floor at 50
    int total = beats.query_sum(0, 3);

    return 0;
}
```

#### Aplicación en el mundo real: ajustes de salario (lazy propagation)

```cpp
// HR system: 8 departments, range salary raises
Lazy_Sum_Segment_Tree<int> payroll = {50, 45, 60, 55, 70, 48, 52, 65};
payroll.update(2, 5, 10);          // departments 2-5 get +$10K raise
int cost = payroll.query(0, 7);    // total payroll after raise
```

### Cartesian Trees & LCA

Aleph-w implementa la cadena clásica de reducciones **RMQ ↔ LCA ↔ Cartesian Tree**, confirmando que Range Minimum Query y Lowest Common Ancestor son problemas equivalentes:

```text
┌─────────────────────────────────────────────────────────────────────┐
│                  RMQ  ←→  LCA  ←→  Cartesian Tree                   │
├──────────────────────┬─────────────┬──────────┬─────────────────────┤
│       Class          │    Build    │  Query   │  Space              │
├──────────────────────┼─────────────┼──────────┼─────────────────────┤
│ Gen_Cartesian_Tree   │   O(n)      │    —     │  O(n)               │
│ Gen_Euler_Tour_LCA   │ O(n log n)  │  O(1)    │  O(n log n)         │
│ Gen_Cartesian_Tree_RMQ│ O(n log n) │  O(1)    │  O(n log n)         │
└──────────────────────┴─────────────┴──────────┴─────────────────────┘
```

- **`Gen_Cartesian_Tree<T, Comp>`** — Árbol cartesiano explícito construido en O(n) con un stack monótono. Satisface la propiedad de heap bajo `Comp` e inorder = arreglo original.
- **`Gen_Euler_Tour_LCA<T, Comp>`** — LCA en O(1) vía Euler Tour + Sparse Table sobre el árbol cartesiano.
- **`Gen_Cartesian_Tree_RMQ<T, Comp>`** — Consultas estáticas en O(1) vía la reducción RMQ → LCA → Cartesian Tree.

#### Ejemplos de uso

```cpp
#include <tpl_cartesian_tree.H>

int main() {
    // Build a Cartesian Tree (min-heap)
    Cartesian_Tree<int> ct = {3, 2, 6, 1, 9};
    size_t r = ct.root();          // index 3 (value 1 = minimum)
    auto io = ct.inorder();        // {0, 1, 2, 3, 4}

    // LCA queries in O(1)
    Euler_Tour_LCA<int> lca = {3, 2, 6, 1, 9};
    size_t a = lca.lca(0, 2);     // = 1 (common ancestor)
    size_t d = lca.distance(0, 4); // tree distance

    // O(1) RMQ via Cartesian Tree reduction
    Cartesian_Tree_RMQ<int> rmq = {5, 2, 4, 7, 1, 3, 6};
    int m = rmq.query(0, 3);      // min(5, 2, 4, 7) = 2
    size_t idx = rmq.query_idx(2, 6); // index of min in [2,6]

    // Max variants
    Max_Cartesian_Tree<int> max_ct = {3, 2, 6, 1, 9};
    Cartesian_Tree_RMaxQ<int> rmaxq = {5, 2, 4, 7, 1};

    return 0;
}
```

### LCA de árboles sobre backends de grafos

`LCA.H` provides two LCA engines for rooted trees represented as Aleph graphs:

| Class | Build | Query (`lca`) | Space |
|-------|-------|----------------|-------|
| `Gen_Binary_Lifting_LCA<GT, SA>` | O(n log n) | O(log n) | O(n log n) |
| `Gen_Euler_RMQ_LCA<GT, SA>` | O(n log n) | O(1) | O(n log n) |

Highlights:
- Works with `List_Graph`, `List_SGraph`, and `Array_Graph`.
- Supports arc filtering via `SA`.
- Validates tree shape on the filtered graph (connected, acyclic, simple).
- Exposes `lca`, `distance`, `is_ancestor`, `depth_of`, and id-based APIs.

```cpp
#include <LCA.H>
#include <tpl_graph.H>

using G = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

int main() {
    G g;
    auto * n0 = g.insert_node(0);
    auto * n1 = g.insert_node(1);
    auto * n2 = g.insert_node(2);
    auto * n3 = g.insert_node(3);
    g.insert_arc(n0, n1);
    g.insert_arc(n0, n2);
    g.insert_arc(n1, n3);

    Binary_Lifting_LCA<G> bl(g, n0);
    Euler_RMQ_LCA<G> er(g, n0);

    auto * a = bl.lca(n3, n2);           // n0
    auto * b = er.lca(n3, n2);           // n0
    size_t d = bl.distance(n3, n2);      // 3 edges
    bool ok = bl.is_ancestor(n0, n3);    // true
    (void)a; (void)b; (void)d; (void)ok;
}
```

Backend-specific constructors:

```cpp
#include <LCA.H>
#include <tpl_graph.H>

using G = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
Binary_Lifting_LCA<G> bl(g, root_node);
Euler_RMQ_LCA<G> er(g, root_node);
```

```cpp
#include <LCA.H>
#include <tpl_sgraph.H>

using G = List_SGraph<Graph_Snode<int>, Graph_Sarc<int>>;
Binary_Lifting_LCA<G> bl(g, root_node);
Euler_RMQ_LCA<G> er(g, root_node);
```

```cpp
#include <LCA.H>
#include <tpl_agraph.H>

using G = Array_Graph<Graph_Anode<int>, Graph_Aarc<int>>;
Binary_Lifting_LCA<G> bl(g, root_node);
Euler_RMQ_LCA<G> er(g, root_node);
```

### Descomposiciones de árbol (Heavy-Light + Centroid)

`Tree_Decomposition.H` adds two advanced tree engines on Aleph graph backends:

| Class | Build | Typical Query/Update | Use case |
|-------|-------|-----------------------|----------|
| `Gen_Heavy_Light_Decomposition<GT, SA>` | O(n) | path split in O(log n) segments | Path/subtree decomposition |
| `Gen_HLD_Path_Query<GT, T, Op, SA>` | O(n) + segment tree | O(log² n) path, O(log n) subtree | Dynamic path/subtree range queries |
| `Gen_Centroid_Decomposition<GT, SA>` | O(n log n) | O(log n) ancestor chain scans | Dynamic nearest/farthest marked-node tricks |

Highlights:
- Works with `List_Graph`, `List_SGraph`, and `Array_Graph`.
- Supports arc filters `SA`.
- Exposes centroid-ancestor chains with distances (ideal for online distance queries).
- Integrates directly with segment trees for HLD path/subtree queries.

Problem patterns solved:
- **Dynamic path aggregates**: `sum/min/max/xor` on `path(u, v)` with node point updates.
- **Dynamic subtree aggregates**: full subtree query `subtree(u)` with node point updates.
- **Dynamic nearest active node** (unweighted distance in edges): mark nodes as active and query `min dist(u, active)`.
- **Dynamic farthest/threshold-style distance queries** by scanning centroid ancestor chains.

Quick decision guide:

| If your problem looks like... | Prefer | Why |
|---|---|---|
| Many online path queries + point updates | `Gen_HLD_Path_Query` | Path becomes O(log n) base-array segments |
| Many online subtree queries + point updates | `Gen_HLD_Path_Query` | Subtree is one contiguous base-array range |
| Nearest/farthest active node on a tree | `Gen_Centroid_Decomposition` | Query/update naturally map to centroid ancestor chain |
| Only LCA/ancestor/distance (no segment aggregate) | `LCA.H` | Simpler API, lower conceptual overhead |
| Offline custom path/subtree statistics (non-monoid) | `tpl_mo_on_trees.H` | Better fit for add/remove offline policies |

30-second diagnostic checklist:
1. Is the tree static and you only need `lca`, `is_ancestor`, or `distance`? Use `LCA.H`.
2. Do you need online `path(u, v)` or `subtree(u)` aggregates with an associative op and node point updates? Use `Gen_HLD_Path_Query`.
3. Do you maintain a dynamic set of marked nodes and query nearest/farthest by tree distance? Use `Gen_Centroid_Decomposition`.
4. Are queries offline and maintained via add/remove (not monoid-friendly)? Use `tpl_mo_on_trees.H`.
5. Do you require weighted-edge distances? `Gen_Centroid_Decomposition` is edge-count based today; adapt your workflow (or preprocess distances) before applying centroid-chain formulas.

Practical notes:
- The current implementations assume an **unweighted tree topology** for distance-based centroid workflows (`distance = #edges`).
- They validate the filtered graph is a simple tree; construction fails fast on invalid topology.
- For **range updates** (not only point updates), combine HLD decomposition with a lazy segment tree policy.

#### Recetas listas para usar

Receta 1: Suma en camino con updates puntuales por nodo
Cuándo usar:
Muchas consultas online como “sum on path(u, v)” y cambios ocasionales de valores por nodo.

```cpp
#include <Tree_Decomposition.H>
#include <tpl_graph.H>

using G = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

// Build once.
HLD_Path_Query<G, int, Aleph::plus<int>> q(g, root, 0); // identity for sum

// Online operations.
q.update_node(sensor_a, +3);            // value[a] += 3
q.set_node(sensor_b, 42);               // value[b] = 42
int risk = q.query_path(city_u, city_v); // sum on unique tree path
```

Receta 2: Agregado por subárbol (área de servicio completa)
Cuándo usar:
Totales en jerarquías enraizadas donde cada update toca un solo nodo.

```cpp
#include <Tree_Decomposition.H>
#include <tpl_graph.H>

using G = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

HLD_Path_Query<G, int, Aleph::plus<int>> q(g, ceo, 0);

int area_cost = q.query_subtree(region_head); // sum over full rooted subtree
q.update_node(team_node, -5);                 // adjust one team budget
int updated = q.query_subtree(region_head);
```

Receta 3: Nodo activo más cercano (centros dinámicos)
Cuándo usar:
Activar/marcar nodos con el tiempo y consultar repetidamente la distancia al nodo activo más cercano.

```cpp
#include <Tree_Decomposition.H>
#include <tpl_graph.H>
#include <algorithm>
#include <limits>

using G = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
Centroid_Decomposition<G> cd(g, root);

const size_t INF = std::numeric_limits<size_t>::max() / 4;
auto best = Array<size_t>::create(cd.size());
for (size_t i = 0; i < cd.size(); ++i) best(i) = INF;

auto activate = [&](G::Node * x) {
  cd.for_each_centroid_ancestor(x, [&](size_t c, size_t d, size_t) {
    best(c) = std::min(best(c), d);
  });
};

auto nearest = [&](G::Node * x) {
  size_t ans = INF;
  cd.for_each_centroid_ancestor(x, [&](size_t c, size_t d, size_t) {
    if (best(c) != INF) ans = std::min(ans, best(c) + d);
  });
  return ans; // #edges to nearest active node (or INF if none active)
};
```

```cpp
#include <Tree_Decomposition.H>
#include <tpl_graph.H>

using G = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

// Assume g is a rooted tree and root is a node in g.
Heavy_Light_Decomposition<G> hld(g, root);
HLD_Path_Query<G, int, Aleph::plus<int>> q(g, root, 0); // identity = 0

int path_sum = q.query_path(u, v);
int subtree_sum = q.query_subtree(u);
q.update_node(u, +5);
q.set_node(v, 42);

Centroid_Decomposition<G> cd(g, root);
// For node x, iterate centroid ancestors with distances:
cd.for_each_centroid_ancestor(x, [&](size_t c, size_t d, size_t k) {
    // c: centroid ancestor id, d: distance(x, c), k: index in centroid chain
});
```

### Link-Cut Trees (Bosques Dinámicos)

Un **Link-Cut Tree** mantiene un bosque de árboles enraizados bajo operaciones dinámicas `link` y `cut`, cada una en tiempo amortizado O(log n). Es la estructura estándar cuando se necesita:

- **Conectividad dinámica** — unir y separar árboles al vuelo
- **Agregación sobre caminos** — consultar cualquier monoide (suma, min, max, xor, ...) sobre caminos del árbol
- **Re-enraizamiento** — elegir cualquier vértice como raíz representada
- **LCA** — ancestro común más bajo bajo el enraizamiento actual
- **Actualizaciones perezosas** — actualizaciones diferidas sobre caminos

```cpp
#include <tpl_link_cut_tree.H>

// Solo conectividad
Link_Cut_Tree lct;
auto *a = lct.make_vertex(0), *b = lct.make_vertex(1);
lct.link(a, b);
bool ok = lct.connected(a, b);  // true
lct.cut(a, b);

// Consultas de suma sobre caminos
Gen_Link_Cut_Tree<int, SumMonoid<int>> sum_lct;
auto *u = sum_lct.make_vertex(10), *v = sum_lct.make_vertex(20);
sum_lct.link(u, v);
int s = sum_lct.path_query(u, v);  // 30

// Actualizaciones perezosas (sumar delta a cada nodo del camino)
Gen_Link_Cut_Tree<long long, SumMonoid<long long>,
                  AddLazyTag<long long>> lazy_lct;
```

### Mo's Algorithm (Offline Range Queries)

Mo's algorithm responde Q consultas offline de rango sobre un arreglo estático de N elementos en tiempo O((N+Q)√N). Es útil cuando la función de consulta es “decomposable” (mantenible vía add/remove de un elemento) pero no tiene estructura algebraica (no es un monoid, no es invertible, no es idempotente).

La implementación usa un **snake-optimized sweep** (alternando la dirección del puntero derecho en bloques pares/impares) para mejor comportamiento de caché. El algoritmo se parametriza con una **policy** (concept `MoPolicy`), así que el usuario puede conectar lógica arbitraria de add/remove.

#### Policies incluidas

| Typedef | Policy | Description |
|---------|--------|-------------|
| `Distinct_Count_Mo<T>` | `Distinct_Count_Policy<T>` | Count distinct elements in a range |
| `Powerful_Array_Mo<T>` | `Powerful_Array_Policy<T>` | sum(cnt[x]² · x) for competitive programming |
| `Range_Mode_Mo<T>` | `Range_Mode_Policy<T>` | Most frequent element (frequency, value) |

#### Ejemplos de uso

```cpp
#include <tpl_mo_algorithm.H>

int main() {
    // Count distinct elements in ranges
    Distinct_Count_Mo<int> mo = {1, 2, 1, 3, 2, 1};
    auto ans = mo.solve({{0, 2}, {1, 4}, {0, 5}});
    // ans(0) == 2, ans(1) == 3, ans(2) == 3

    // Powerful array queries
    Powerful_Array_Mo<int> pa = {1, 2, 1, 1, 2};
    auto pa_ans = pa.solve({{0, 4}});
    // pa_ans(0) == 17  (9*1 + 4*2 = 17)

    // Range mode queries
    Range_Mode_Mo<int> mode = {3, 1, 3, 3, 1, 2};
    auto m = mode.solve({{0, 5}});
    // m(0) == {3, 3}  (element 3 appears 3 times)

    return 0;
}
```

#### Policies personalizadas

```cpp
// Policy: sum of elements in the current window
struct Sum_Policy {
    using answer_type = long long;
    long long sum = 0;

    void init(const Array<int> &, size_t) { sum = 0; }
    void add(const Array<int> & data, size_t idx) { sum += data(idx); }
    void remove(const Array<int> & data, size_t idx) { sum -= data(idx); }
    answer_type answer() const { return sum; }
};

Gen_Mo_Algorithm<int, Sum_Policy> mo = {3, 1, 4, 1, 5};
auto ans = mo.solve({{0, 4}, {1, 3}});
// ans(0) == 14, ans(1) == 6
```

<a id="readme-es-geometria"></a>
### Geometría computacional

Aleph-w ofrece una suite robusta para geometría computacional 2D y 3D, construida sobre **aritmética racional exacta** (`Geom_Number` = `mpq_class`) para evitar errores de punto flotante en predicados geométricos.

**Características clave:**

- **Primitivas geométricas (`point.H`, `polygon.H`)**: un conjunto completo de clases para puntos (2D/3D), segmentos, polígonos (simples y regulares), triángulos, rectángulos y elipses (axis-aligned y rotadas).
- **Predicados exactos**: funciones núcleo como `orientation()`, `segments_intersect()` e `in_circle()` son exactas, asegurando comportamiento robusto y correcto para algoritmos de mayor nivel.

**Algoritmos (`geom_algorithms.H`):**

| Categoría | Algoritmos | Complejidad |
|---|---|---|
| **Convex Hull** | Andrew's Monotonic Chain, Graham Scan, QuickHull, Gift Wrapping (Jarvis March), Brute-Force | O(n log n) / O(nh) |
| **Triangulation** | Ear-Cutting, Monotone Polygon, Delaunay (Bowyer-Watson, Randomized Inc.), Constrained Delaunay (CDT) | O(n²), O(n log n) |
| **Proximity** | Closest Pair (Divide & Conquer), Minimum Enclosing Circle (Welzl), Rotating Calipers (Diameter/Width) | O(n log n), O(n) |
| **Diagrams** | Voronoi Diagram (from Delaunay), Power Diagram (Weighted Voronoi) | O(n log n) |
| **Intersections** | Segment Sweep (Bentley-Ottmann), Half-Plane Intersection, Convex Polygon Clipping, Boolean Polygon Ops (Greiner-Hormann) | O((n+k)log n), O(n log n) |
| **Simplification** | Douglas-Peucker, Visvalingam-Whyatt, Chaikin Smoothing | O(n log n), O(n*2^k) |
| **Pathfinding** | Shortest Path in Simple Polygon (Funnel Algorithm) | O(n) |
| **Spatial Indexing** | AABB Tree, KD-Tree | O(log n) queries |

**Visualización (`tikzgeom.H`, `eepicgeom.H`):**

- **TikZ Backend**: un backend moderno y flexible (`Tikz_Plane`) para generar diagramas PGF/TikZ de alta calidad. Soporta capas, estilos y curvas Bézier nativas.
- **Visualizadores de algoritmos**: una suite de funciones en `tikzgeom_algorithms.H` para renderizar salidas de algoritmos como Voronoi, convex hull y arrangements.
- **Exportación de documentos**: el helper `Tikz_Scene` en `tikzgeom_scene.H` compone figuras complejas y las exporta como documentos LaTeX standalone, Beamer o handouts, incluyendo animaciones multi-paso usando overlays.
- **Backend legado EEPIC**: para compatibilidad con workflows LaTeX antiguos.

Ver `Examples/` para más de una docena de programas específicos de geometría, incluyendo `tikz_funnel_beamer_twocol_example.cc` que genera slides Beamer animados del algoritmo funnel de camino más corto.

**Header:** `point.H`

```cpp
#include <point.H>

// --- Orientation ---
Point a(0, 0), b(4, 0), c(2, 3);
Orientation o = orientation(a, b, c);  // Orientation::CCW

// --- Segment intersection detection ---
Segment s1(Point(0, 0), Point(2, 2));
Segment s2(Point(0, 2), Point(2, 0));
bool cross = segments_intersect(s1, s2);  // true

// --- Exact intersection point (mpq_class, no rounding) ---
Point p = segment_intersection_point(s1, s2);  // exactly (1, 1)

// --- Works with vertical / horizontal / any configuration ---
Segment v(Point(3, 0), Point(3, 6));
Segment d(Point(0, 0), Point(6, 6));
Point q = segment_intersection_point(v, d);  // exactly (3, 3)

// --- Triangle area (exact rational) ---
Geom_Number area = area_of_triangle(a, b, c);  // exact
```

<a id="readme-es-algebra-lineal"></a>
### Álgebra lineal (estructuras sparse)

Aleph-w ofrece **clases de vector y matriz dispersas** con indexación basada en dominio, optimizadas para datos con muchos ceros:

```text
┌────────────────────────────────────────────────────────────────────────────┐
│                   SPARSE LINEAR ALGEBRA STRUCTURES                         │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  SPARSE VECTOR (al-vector.H)                                               │
│  ────────────────────────────                                              │
│  Vector<T, NumType>                                                        │
│                                                                            │
│  • Domain-based indexing (any type T)                                      │
│  • Stores only non-zero entries (hash-based)                               │
│  • Epsilon tolerance for near-zero removal                                 │
│  • Operations: +, -, *, dot product, norms                                 │
│                                                                            │
│  Memory: O(nonzeros) instead of O(dimension)                               │
│                                                                            │
│  SPARSE MATRIX (al-matrix.H)                                               │
│  ────────────────────────────                                              │
│  Matrix<Trow, Tcol, NumType>                                               │
│                                                                            │
│  • Arbitrary row/column domains                                            │
│  • Stores only non-zero entries                                            │
│  • Operations: +, -, *, transpose, row/col vectors                         │
│  • Integration with Vector class                                           │
│                                                                            │
│  Memory: O(nonzeros) instead of O(rows × cols)                             │
│                                                                            │
│  WHEN TO USE SPARSE?                                                       │
│  • Sparsity > 90% (most entries are zero)                                  │
│  • Large dimensions with few non-zeros                                     │
│  • Need domain-based indexing (named rows/columns)                         │
│                                                                            │
│  Example: 1000×1000 matrix with 1000 non-zeros:                            │
│    Dense:  1,000,000 doubles = 8 MB                                        │
│    Sparse:     1,000 entries = 8 KB  (1000× savings!)                      │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

#### Ejemplos de uso

```cpp
#include <al-vector.H>
#include <al-matrix.H>

int main() {
    // Sparse vector with string domain
    AlDomain<std::string> products;
    products.insert("Laptop");
    products.insert("Phone");
    products.insert("Tablet");

    Vector<std::string, double> inventory(products);
    inventory.set_entry("Laptop", 150.0);
    inventory.set_entry("Phone", 450.0);
    // Tablet implicitly 0 (not stored)

    std::cout << "Laptop inventory: " << inventory["Laptop"] << "\n";

    // Sparse matrix with domain-based indexing
    AlDomain<std::string> stores;
    stores.insert("NYC");
    stores.insert("LA");
    stores.insert("CHI");

    Matrix<std::string, std::string, double> sales(products, stores);
    sales.set_entry("Laptop", "NYC", 25);
    sales.set_entry("Phone", "LA", 80);

    // Matrix-vector multiplication
    Vector<std::string, double> total_by_product = sales * inventory;

    // Transpose
    auto sales_t = sales.transpose();

    return 0;
}
```

#### Aplicación en el mundo real: análisis de ventas

```cpp
// Track sales across products × stores (mostly zeros for sparse data)
AlDomain<std::string> products = {"Laptop", "Phone", "Tablet", "Monitor"};
AlDomain<std::string> stores = {"NYC", "LA", "CHI", "MIA", "SEA"};

Matrix<std::string, std::string, int> sales(products, stores);

// Only record actual sales (most product-store pairs have zero sales)
sales.set_entry("Laptop", "NYC", 12);
sales.set_entry("Phone", "LA", 35);
sales.set_entry("Phone", "NYC", 28);
// Other entries implicitly zero (not stored)

// Query total sales for a product across all stores
auto laptop_sales = sales.get_row("Laptop");  // Vector<string, int>
int total_laptops = laptop_sales.sum();
```

<a id="readme-es-tablas-hash"></a>
### Tablas hash

Aleph-w ofrece múltiples implementaciones de tablas hash optimizadas para distintos escenarios:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        HASH TABLE IMPLEMENTATIONS                           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  SEPARATE CHAINING              OPEN ADDRESSING           LINEAR HASHING    │
│  (tpl_hash.H)                   (tpl_odhash.H)            (tpl_lhash.H)     │
│                                                                             │
│  ┌───┬───┬───┬───┐             ┌───┬───┬───┬───┐        Grows incrementally │
│  │ 0 │ 1 │ 2 │ 3 │             │ A │ B │   │ C │        one slot at a time  │
│  └─┬─┴─┬─┴───┴─┬─┘             └───┴───┴───┴───┘                            │
│    │   │       │                                                            │
│    ▼   ▼       ▼               On collision:                                │
│  ┌───┐┌───┐  ┌───┐             probe next slot           Best for:          │
│  │ A ││ B │  │ E │                                       • Gradual growth   │
│  └─┬─┘└─┬─┘  └───┘             Best for:                 • Memory-mapped    │
│    │    │                      • Cache locality          • Persistent       │
│    ▼    ▼                      • Small elements                             │
│  ┌───┐┌───┐                    • Read-heavy                                 │
│  │ C ││ D │                                                                 │
│  └───┘└───┘                                                                 │
│                                                                             │
│  Best for:                                                                  │
│  • Variable-size elements                                                   │
│  • High load factors                                                        │
│  • Easy deletion                                                            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### Uso básico

```cpp
#include <tpl_hash.H>

int main() {
    // Hash set
    DynHashSet<std::string> words;
    words.insert("algorithm");
    words.insert("data");
    words.insert("structure");

    if (words.contains("algorithm"))
        std::cout << "Found!\n";

    // Hash map
    DynHashMap<std::string, int> ages;
    ages.insert("Alice", 30);
    ages["Bob"] = 25;  // Alternative syntax

    int* age = ages.search("Alice");
    if (age)
        std::cout << "Alice is " << *age << "\n";

    return 0;
}
```

#### Funciones hash personalizadas

```cpp
#include <tpl_dynSetHash.H>

struct Point {
    int x, y;
    bool operator==(const Point& o) const {
        return x == o.x && y == o.y;
    }
};

struct PointHash {
    size_t operator()(const Point& p) const {
        // Combine hashes using XOR and bit shifting
        return std::hash<int>{}(p.x) ^ (std::hash<int>{}(p.y) << 1);
    }
};

int main() {
    DynHashSet<Point, PointHash> points;
    points.insert({1, 2});
    points.insert({3, 4});

    std::cout << "Size: " << points.size() << "\n";
    return 0;
}
```

<a id="readme-es-heaps"></a>
### Heaps y colas de prioridad

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         HEAP IMPLEMENTATIONS                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  BINARY HEAP                         FIBONACCI HEAP                         │
│  (tpl_binHeap.H)                     (tpl_fibonacci_heap.H)                 │
│                                                                             │
│        [10]                          Collection of heap-ordered trees       │
│       /    \                                                                │
│    [15]    [20]                      ○───○───○                              │
│    /  \    /  \                      │   │                                  │
│  [25][30][35][40]                    ○   ○───○                              │
│                                          │                                  │
│  Array representation:                   ○                                  │
│  [10|15|20|25|30|35|40]                                                     │
│                                                                             │
├───────────────────────────────────────┬─────────────────────────────────────┤
│  Operation     │ Binary   │ Fibonacci │  Notes                              │
├────────────────┼──────────┼───────────┼─────────────────────────────────────┤
│  Insert        │ O(log n) │ O(1)*     │  * Amortized                        │
│  Find-Min      │ O(1)     │ O(1)      │                                     │
│  Delete-Min    │ O(log n) │ O(log n)* │                                     │
│  Decrease-Key  │ O(log n) │ O(1)*     │  Key advantage of Fibonacci         │
│  Merge         │ O(n)     │ O(1)      │                                     │
└────────────────┴──────────┴───────────┴─────────────────────────────────────┘
```

```cpp
#include <tpl_binHeap.H>

int main() {
    // Min-heap (default)
    BinHeap<int> min_heap;
    min_heap.insert(30);
    min_heap.insert(10);
    min_heap.insert(20);

    while (!min_heap.is_empty())
        std::cout << min_heap.getMin() << " ";  // 10 20 30

    // Max-heap
    BinHeap<int, std::greater<int>> max_heap;
    max_heap.insert(30);
    max_heap.insert(10);
    max_heap.insert(20);

    while (!max_heap.is_empty())
        std::cout << max_heap.getMin() << " ";  // 30 20 10

    return 0;
}
```

<a id="readme-es-estructuras-lineales"></a>
### Listas y estructuras secuenciales

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       LINEAR DATA STRUCTURES                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  SINGLY-LINKED LIST (DynList)        DOUBLY-LINKED LIST (DynDlist)          │
│                                                                             │
│  ┌───┐   ┌───┐   ┌───┐   ┌───┐      ┌───┐   ┌───┐   ┌───┐   ┌───┐           │
│  │ A │──▶│ B │──▶│ C │──▶│ D │      │ A │◀─▶│ B │◀─▶│ C │◀─▶│ D │           │
│  └───┘   └───┘   └───┘   └───┘      └───┘   └───┘   └───┘   └───┘           │
│                                                                             │
│  Insert front: O(1)                  Insert anywhere: O(1)                  │
│  Insert back:  O(n)                  Insert back: O(1)                      │
│  Remove front: O(1)                  Remove anywhere: O(1)                  │
│  Search:       O(n)                  Search: O(n)                           │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  DYNAMIC ARRAY (DynArray)            QUEUE (ArrayQueue)                     │
│                                                                             │
│  ┌────┐  ┌────┐  ┌────┐  ┌────┐  ┌────┐                                     │
│  │ A  │  │free│  │ B  │  │free│  │ C  │   Fragmented, scattered             │
│  └────┘  └────┘  └────┘  └────┘  └────┘                                     │
│    0   1   2   3   4                  ▲               ▲                     │
│                                      front           back                   │
│  Access:  O(1)                                                              │
│  Append:  O(1) amortized             Enqueue: O(1)                          │
│  Insert:  O(n)                       Dequeue: O(1)                          │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

```cpp
#include <tpl_dynList.H>
#include <tpl_dynDlist.H>
#include <tpl_dynArray.H>

int main() {
    // Singly-linked list
    DynList<int> slist = {1, 2, 3, 4, 5};
    slist.insert(0);  // Insert at front: O(1)

    // Doubly-linked list
    DynDlist<int> dlist = {1, 2, 3, 4, 5};
    dlist.append(6);  // Insert at back: O(1)

    // Dynamic array
    DynArray<int> arr = {1, 2, 3, 4, 5};
    arr.append(6);    // Amortized O(1)
    int x = arr[2];   // Random access: O(1)

    // All support functional operations!
    auto doubled = slist.map([](int x) { return x * 2; });
    auto evens = dlist.filter([](int x) { return x % 2 == 0; });
    int sum = arr.foldl(0, [](int acc, int x) { return acc + x; });

    return 0;
}
```

<a id="readme-es-consultas-por-rango"></a>
### Estructuras para consultas por rango

#### Sparse Table — consultas O(1) sobre arreglos estáticos

Sparse Table ofrece **consultas por rango en O(1)** después de **preprocesamiento O(n log n)** para cualquier operación idempotente (min, max, gcd, bitwise AND/OR).

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                         SPARSE TABLE (RMQ)                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Original Array: [5, 2, 8, 1, 9, 3, 6, 4, 7]                                │
│                                                                             │
│  Level 0 (2^0=1): [5, 2, 8, 1, 9, 3, 6, 4, 7]                               │
│  Level 1 (2^1=2): [2, 2, 1, 1, 3, 3, 4, 4]                                  │
│  Level 2 (2^2=4): [1, 1, 1, 1, 3, 3]                                        │
│  Level 3 (2^3=8): [1, 1]                                                    │
│                                                                             │
│  Query [2, 7] (range length 6):                                             │
│    k = floor(log2(6)) = 2, 2^k = 4                                          │
│    Result = min(table[2][2], table[2][4]) = min(1, 3) = 1                   │
│                                                                             │
│  Build:  O(n log n)    Query: O(1)    Space: O(n log n)                     │
│  Use case: Static arrays with many queries                                  │
└─────────────────────────────────────────────────────────────────────────────┘
```

```cpp
#include <tpl_sparse_table.H>

int main() {
    // Range Minimum Queries
    Sparse_Table<int> rmq = {5, 2, 8, 1, 9, 3, 6, 4, 7};
    int min_val = rmq.query(2, 7);  // O(1) -> 1
    
    // Range Maximum Queries  
    Max_Sparse_Table<int> max_rmq = {5, 2, 8, 1, 9, 3, 6, 4, 7};
    int max_val = max_rmq.query(0, 4);  // O(1) -> 9
    
    // Custom operation: Range GCD
    struct Gcd_Op {
        int operator()(int a, int b) const noexcept { return std::gcd(a, b); }
    };
    Gen_Sparse_Table<int, Gcd_Op> gcd_rmq = {12, 18, 24, 36, 60};
    int g = gcd_rmq.query(0, 3);  // O(1) -> 6
    
    return 0;
}
```

#### Fenwick Tree — consultas dinámicas de prefijo

Para arreglos **dinámicos** con actualizaciones puntuales, usa Fenwick Tree (Binary Indexed Tree):

```cpp
#include <tpl_fenwick_tree.H>

Fenwick_Tree<int> ft(10);  // Size 10, all zeros
ft.update(3, 5);           // a[3] += 5
int sum = ft.prefix(7);    // sum a[0..7] = O(log n)
ft.update(2, -3);          // a[2] -= 3
```

#### Mo's Algorithm — consultas offline por rango

Mo's algorithm responde **Q consultas offline** de rango sobre un arreglo estático de N elementos en tiempo **O((N+Q)√N)**. Es ideal cuando tienes muchas consultas y puedes procesarlas todas juntas.

```text
┌───────────────────────────────────────────────────────────────────────────┐
│                        MO'S ALGORITHM OVERVIEW                            │
├──────────────────┬───────────────┬───────────────┬────────────────────────┤
│     Phase        │    Time       │    Space      │     Description        │
├──────────────────┼───────────────┼───────────────┼────────────────────────┤
│ Sort queries     │   O(Q log Q)  │     O(Q)      │ Block + snake ordering │
│ Sweep window     │ O((N+Q)√N)    │    O(N+Q)     │ Add/remove operations  │
│ Total            │ O((N+Q)√N)    │    O(N+Q)     │ All queries answered   │
└──────────────────┴───────────────┴───────────────┴────────────────────────┘
```

**Ventajas clave:**
- **Procesamiento por lotes**: responde muchas consultas más rápido que operaciones individuales O(log n).
- **Consultas descomponibles**: cualquier operación en la que puedas agregar/remover elementos.
- **Diseño basado en policies**: el mismo algoritmo funciona para distintos tipos de consulta.

**Policies incluidas:**
- `Distinct_Count_Policy<T>` — Cuenta elementos distintos en el rango
- `Powerful_Array_Policy<T>` — Suma de (count² × value)
- `Range_Mode_Policy<T>` — Elemento más frecuente

##### Ejemplos de uso

```cpp
#include <tpl_mo_algorithm.H>

// Distinct element queries on static array
Array<int> arr = {1, 2, 1, 3, 2, 4, 1, 5};
Distinct_Count_Mo<int> mo(arr);  // Preprocess

// Query distinct elements in ranges
Array<std::pair<size_t, size_t>> queries = {
  {0, 3}, {2, 6}, {1, 7}  // [1,2,1,3], [1,3,2,4,1], [2,1,3,2,4,1,5]
};
auto answers = mo.solve(queries);
// answers = {3, 4, 5} distinct elements respectively
```

```cpp
// Range mode queries
Array<int> data = {5, 2, 5, 1, 3, 2, 5, 1};
Range_Mode_Mo<int> mode_mo(data);

Array<std::pair<size_t, size_t>> ranges = {{0, 4}, {2, 7}};
auto modes = mode_mo.solve(ranges);
// modes[0] = {3, 5}  // frequency 3, value 5 (most frequent in [0,4])
// modes[1] = {2, 5}  // frequency 2, value 5 (most frequent in [2,7])
```

##### Mo's Algorithm sobre árboles

Aleph-w extiende Mo's algorithm a **estructuras de árbol** para consultas de subárbol y de paths:

```cpp
#include <tpl_mo_on_trees.H>

// Tree represented as Aleph graph
List_Graph<Graph_Node<int>, Graph_Arc<Empty_Class>> g;
auto * root_node = build_tree(g);  // Your tree construction

// Subtree distinct count
Distinct_Count_Mo_On_Trees<decltype(g)> mot(g, root_node);
auto subtree_answers = mot.subtree_solve({root_node, child1, child2});

// Path distinct count between nodes
auto path_answers = mot.path_solve({{node_a, node_b}, {node_c, node_d}});
```

```cpp
// Direct Tree_Node support (no graph needed)
Tree_Node<int> * r = new Tree_Node<int>(1);
// ... build tree with insert_rightmost_child()

Distinct_Count_Mo_On_Tree_Node<int> mot(r);
auto answers = mot.subtree_solve({r});  // Distinct values in entire tree
```

**Escenarios del mundo real:**
- **Analítica de bases de datos**: contar categorías distintas en rangos de fechas
- **Genómica**: encontrar k-mers más frecuentes en segmentos de ADN
- **Monitoreo de redes**: moda de clases de latencia en ventanas de tiempo
- **Sistemas de archivos**: tipos de archivo distintos en subárboles de directorios
- **Redes sociales**: intereses comunes a lo largo de paths entre usuarios

<a id="readme-es-grafos"></a>
### Grafos

Aleph-w ofrece múltiples representaciones de grafos optimizadas para distintos casos de uso:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        GRAPH REPRESENTATIONS                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ADJACENCY LIST (List_Graph)         ADJACENCY MATRIX (Array_Graph)         │
│                                                                             │
│  ┌───┐   ┌───┐   ┌───┐   ┌───┐      ┌───┬───┬───┬───┐                       │
│  │ A │───│ B │───│ C │───│ D │      │ A │ B │ C │ D │                       │
│  └───┘   └───┘   └───┘   └───┘      └───┴───┴───┴───┘                       │
│                                                                             │
│  Space: O(V + E)                   Space: O(V²)                             │
│  Add edge: O(1)                   Add edge: O(1)                            │
│  Check edge: O(degree)            Check edge: O(1)                          │
│  Best for: Sparse graphs          Best for: Dense graphs                    │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  NETWORK GRAPH (Net_Graph)           DIGRAPH (List_Digraph)                 │
│                                                                             │
│       10                             A ──▶ B ──▶ C                          │
│    A ────▶ B                         │           │                          │
│    │  5    │ 15                      │           ▼                          │
│    ▼       ▼                         └─────────▶ D                          │
│    C ────▶ D                                                                │
│       20                             Directed edges                         │
│                                      In-degree / Out-degree                 │
│  Capacities on edges                                                        │
│  For max-flow algorithms                                                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### Creación y uso de grafos

```cpp
#include <tpl_graph.H>

// Define node and arc types
using CityNode = Graph_Node<std::string>;
using RoadArc = Graph_Arc<double>;  // weight = distance
using RoadMap = List_Graph<CityNode, RoadArc>;

int main() {
    RoadMap map;

    // Add cities (nodes)
    auto* paris = map.insert_node("Paris");
    auto* london = map.insert_node("London");
    auto* berlin = map.insert_node("Berlin");
    auto* rome = map.insert_node("Rome");

    // Add roads (undirected edges with distances)
    map.insert_arc(paris, london, 344);   // Paris - London
    map.insert_arc(paris, berlin, 878);   // Paris - Berlin
    map.insert_arc(london, berlin, 932);  // London - Berlin
    map.insert_arc(berlin, rome, 1181);   // Berlin - Rome
    map.insert_arc(paris, rome, 1106);    // Paris - Rome

    // Graph statistics
    std::cout << "Cities: " << map.get_num_nodes() << "\n";  // 4
    std::cout << "Roads: " << map.get_num_arcs() << "\n";    // 5

    // Iterate over neighbors
    std::cout << "Cities connected to Paris:\n";
    for (auto it = map.get_first_arc(paris);
         it.has_curr(); it.next()) {
        auto* arc = it.get_curr();
        auto* neighbor = map.get_connected_node(arc, paris);
        std::cout << "  " << neighbor->get_info()
                  << " (" << arc->get_info() << " km)\n";
    }

    return 0;
}
```

#### Grafos dirigidos

```cpp
#include <tpl_graph.H>

// Directed graph for task dependencies
using Task = Graph_Node<std::string>;
using Dep = Graph_Arc<int>;  // weight = days
using TaskGraph = List_Digraph<Task, Dep>;

int main() {
    TaskGraph project;

    auto* design = project.insert_node("Design");
    auto* implement = project.insert_node("Implement");
    auto* test = project.insert_node("Test");
    auto* deploy = project.insert_node("Deploy");

    // Directed dependencies
    project.insert_arc(design, implement, 5);     // Design → Implement
    project.insert_arc(implement, test, 10);      // Implement → Test
    project.insert_arc(test, deploy, 2);          // Test → Deploy
    project.insert_arc(design, test, 3);          // Design → Test (parallel)

    return 0;
}
```

---
<a id="readme-es-algoritmos"></a>
## Algoritmos

<a id="readme-es-caminos-minimos"></a>
### Algoritmos de caminos mínimos

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      SHORTEST PATH ALGORITHMS                               │
├────────────────┬────────────┬────────────┬──────────────────────────────────┤
│   Algorithm    │ Complexity │  Negative  │  Best For                        │
├────────────────┼────────────┼────────────┼──────────────────────────────────┤
│ Dijkstra       │O((V+E)logV)│     No     │ Single source, non-negative      │
│ Bellman-Ford   │   O(VE)    │    Yes     │ Negative weights, cycle detection│
│ Floyd-Warshall │   O(V³)    │    Yes     │ All pairs, dense graphs          │
│ Johnson        │O(V²logV+VE)│    Yes     │ All pairs, sparse graphs         │
│ A*             │ O(E) best  │     No     │ Pathfinding with heuristic       │
│ K-shortest     │O(K*V*(E+VlogV))│  No     │ Top-K alternatives (simple/general) s→t │
└────────────────┴────────────┴────────────┴──────────────────────────────────┘
```

`K_Shortest_Paths.H` ofrece dos modos distintos:
- `yen_k_shortest_paths()`: alternativas simples/sin ciclos (loopless).
- `eppstein_k_shortest_paths()`: alternativas generales (pueden incluir ciclos).

#### Algoritmo de Dijkstra

```cpp
#include <tpl_graph.H>
#include <Dijkstra.H>

int main() {
    // ... build graph ...

    // Find shortest path from source to target
    Path<Graph> path(graph);
    double distance = dijkstra_min_path(graph, source, target, path);

    std::cout << "Shortest distance: " << distance << "\n";

    // Print path
    std::cout << "Path: ";
    path.for_each_node([](auto* node) {
        std::cout << node->get_info() << " → ";
    });
    std::cout << "\n";

    // Alternatively: compute all shortest paths from source
    Graph tree;
    dijkstra_min_spanning_tree(graph, source, tree);

    return 0;
}
```

#### Bellman-Ford (con aristas negativas)

```cpp
#include <tpl_graph.H>
#include <Bellman_Ford.H>

int main() {
    // Graph may have negative weights
    Graph graph;
    // ... build graph with potentially negative weights ...

    // Compute minimum path
    Path<Graph> path(graph);
    bool has_negative_cycle = false;

    double dist = bellman_ford_min_path(
        graph, source, target, path, has_negative_cycle
    );

    if (has_negative_cycle) {
        std::cerr << "Warning: Negative cycle detected!\n";
    } else {
        std::cout << "Distance: " << dist << "\n";
    }

    return 0;
}
```

#### Búsqueda A* (pathfinding heurístico)

```cpp
#include <tpl_graph.H>
#include <AStar.H>

// Grid-based pathfinding example
struct Coord { int x, y; };

// Heuristic: Euclidean distance to goal
double heuristic(Node* current, Node* goal) {
    auto [x1, y1] = current->get_info();
    auto [x2, y2] = goal->get_info();
    return std::sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
}

int main() {
    GridGraph grid;
    // ... build grid graph ...

    Path<GridGraph> path(grid);
    double dist = astar_search(grid, start, goal, path, heuristic);

    std::cout << "Path length: " << dist << "\n";
    return 0;
}
```

#### K-caminos más cortos (Yen sin ciclos + API general estilo Eppstein)

Úsalo cuando un solo camino más corto no es suficiente:
- planificación de rutas con `k` alternativas rankeadas de `s` a `t`
- failover resiliente (si la mejor ruta está bloqueada, pasar a la siguiente)
- generación de candidatos casi óptimos para restricciones posteriores

`K_Shortest_Paths.H` soporta ambos contratos:
- `yen_k_shortest_paths()`: hasta `k` caminos **simples** (sin ciclos).
- `eppstein_k_shortest_paths()`: hasta `k` caminos generales, donde pueden aparecer ciclos.
- `Yen_K_Shortest_Paths<...>` y `Eppstein_K_Shortest_Paths<...>`: wrappers tipo functor
  para solvers reutilizables configurados (distancia + filtro de arcos).

Ambos se retornan en costo total no decreciente. Los pesos de arcos deben ser no negativos.
Detalle de implementación:
- Yen hace recomputación iterativa de spur-path.
- El estilo Eppstein precomputa un árbol de caminos mínimos inverso y expande sidetracks.

Por qué mantener ambas APIs separadas:
- Usa Yen cuando la simplicidad del camino es un requisito estricto (ruteo sin vértices repetidos).
- Usa estilo Eppstein cuando necesitas alternativas rankeadas más amplias y quieres reusar más rápido
  la misma estructura source/target en espacios densos de desviaciones.

```cpp
#include <tpl_graph.H>
#include <K_Shortest_Paths.H>

using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<long long>>;

int main() {
    Graph g;
    auto* s = g.insert_node(0);
    auto* a = g.insert_node(1);
    auto* b = g.insert_node(2);
    auto* t = g.insert_node(3);

    g.insert_arc(s, a, 1);
    g.insert_arc(s, b, 2);
    g.insert_arc(a, b, 1);
    g.insert_arc(a, t, 2);
    g.insert_arc(b, t, 1);

    const size_t k = 4;
    auto simple = yen_k_shortest_paths<Graph>(g, s, t, k);
    auto general = eppstein_k_shortest_paths<Graph>(g, s, t, k);

    for (decltype(simple)::Iterator it(simple); it.has_curr(); it.next_ne()) {
      const auto& item = it.get_curr();
      std::cout << "[simple] cost=" << item.total_cost << "\n";
    }

    for (decltype(general)::Iterator it(general); it.has_curr(); it.next_ne()) {
      const auto& item = it.get_curr();
      std::cout << "[general] cost=" << item.total_cost << "\n";
    }

    return 0;
}
```

#### Ciclo de media mínima (Karp)

Úsalo cuando tu sistema tiene bucles repetidos y te importa
el **mejor costo promedio a largo plazo por paso**, no solo un camino mínimo.
Escenarios típicos:
- análisis de performance/latencia de pipelines cíclicos
- loops de pricing o recursos en sistemas en red
- optimización de estado estable en autómatas/máquinas de estados

`Min_Mean_Cycle.H` ofrece:
- `karp_minimum_mean_cycle()`: Karp `O(VE)` para la media mínima de ciclos en grafos dirigidos.
- `minimum_mean_cycle()`: alias con el mismo comportamiento.
- `karp_minimum_mean_cycle_value()` / `minimum_mean_cycle_value()`: variante solo-valor
  (sin extracción de witness).
- `Karp_Minimum_Mean_Cycle<...>`: wrapper tipo functor reutilizable.
- `Karp_Minimum_Mean_Cycle_Value<...>`: wrapper reutilizable solo-valor.

Ambas variantes mantienen la complejidad de Karp `O(VE)`. La variante solo-valor evita metadatos
del witness y usualmente usa menos memoria en la práctica.

El resultado reporta:
- `has_cycle`: si existe al menos un ciclo dirigido
- `minimum_mean`: media mínima óptima
- información del witness walk (`cycle_nodes`, `cycle_arcs`, `cycle_total_cost`, `cycle_length`)

Semántica del witness:
- `cycle_nodes` es un recorrido cerrado (el primer nodo se repite al final).
- En grafos con muchos empates puede contener vértices internos repetidos; es un witness válido
  del valor mínimo de la media, no necesariamente un ciclo simple canónico.

```cpp
#include <tpl_graph.H>
#include <Min_Mean_Cycle.H>

using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<long long>>;

int main() {
    Graph g;
    auto* a = g.insert_node(0);
    auto* b = g.insert_node(1);
    auto* c = g.insert_node(2);

    g.insert_arc(a, b, 4);
    g.insert_arc(b, c, 1);
    g.insert_arc(c, a, 1); // cycle mean = 2

    auto result = karp_minimum_mean_cycle(g);
    if (result.has_cycle) {
      std::cout << "minimum mean: " << result.minimum_mean << "\n";
    }
}
```

<a id="readme-es-mst"></a>
### Árboles de expansión mínima

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    MINIMUM SPANNING TREE ALGORITHMS                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  KRUSKAL'S ALGORITHM                 PRIM'S ALGORITHM                       │
│  (Edge-based)                        (Vertex-based)                         │
│                                                                             │
│  1. Sort all edges by weight         1. Start from any vertex               │
│  2. For each edge (u,v):             2. Add cheapest edge to tree           │
│     If u,v in different sets:        3. Repeat until all vertices           │
│        Add edge to MST                  are in tree                         │
│        Union the sets                                                       │
│                                                                             │
│  Uses: Union-Find                    Uses: Priority Queue                   │
│  Complexity: O(E log E)              Complexity: O(E log V)                 │
│  Best for: Sparse graphs             Best for: Dense graphs                 │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Example MST:                                                               │
│                                                                             │
│      [A]───2───[B]                   MST edges: A-B (2)                     │
│       │ \     / │                               A-C (3)                     │
│       3   4  5  6                               B-D (4)                     │
│       │     X   │                                                           │
│      [C]───7───[D]                   Total weight: 9                        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

```cpp
#include <tpl_graph.H>
#include <Kruskal.H>
#include <Prim.H>

int main() {
    Graph graph;
    // ... build weighted graph ...

    // Kruskal's algorithm
    Graph mst_kruskal;
    kruskal_min_spanning_tree(graph, mst_kruskal);

    // Prim's algorithm
    Graph mst_prim;
    prim_min_spanning_tree(graph, mst_prim);

    // Both produce the same MST (or one with equal weight)
    double total_weight = 0;
    mst_kruskal.for_each_arc([&](auto* arc) {
        total_weight += arc->get_info();
    });

    std::cout << "MST weight: " << total_weight << "\n";

    return 0;
}
```

<a id="readme-es-flujos"></a>
### Flujos en redes

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       NETWORK FLOW ALGORITHMS                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  MAXIMUM FLOW PROBLEM                                                       │
│                                                                             │
│              ┌─────10────▶[B]─────8─────┐                                   │
│              │             │            │                                   │
│           [Source]        2│         [Sink]                                 │
│              │             ▼            │                                   │
│              └─────5─────▶[C]─────7─────┘                                   │
│                                                                             │
│  Maximum flow = 15  (10 through B, 5 through C)                             │
│                                                                             │
├──────────────────────┬────────────────┬─────────────────────────────────────┤
│  Algorithm           │  Complexity    │  Description                        │
├──────────────────────┼────────────────┼─────────────────────────────────────┤
│  Ford-Fulkerson      │  O(E × maxflow)│  DFS for augmenting paths           │
│  Edmonds-Karp        │  O(VE²)        │  BFS for shortest augmenting paths  │
│  Push-Relabel        │  O(V²E)        │  Local push operations              │
│  Dinic               │  O(V²E)        │  Blocking flows + level graph       │
├──────────────────────┴────────────────┴─────────────────────────────────────┤
│                                                                             │
│  MIN-COST MAX-FLOW                                                          │
│                                                                             │
│  Each edge has: capacity AND cost                                           │
│  Goal: Find max flow with minimum total cost                                │
│                                                                             │
│  Algorithms: Successive Shortest Path, Network Simplex                      │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### Flujo máximo

```cpp
#include <tpl_net.H>
#include <tpl_maxflow.H>

int main() {
    // Network with capacities
    Net_Graph<long, long> network;

    auto* source = network.insert_node();
    auto* sink = network.insert_node();
    auto* a = network.insert_node();
    auto* b = network.insert_node();

    // Add arcs with capacities
    network.insert_arc(source, a, 10);  // capacity = 10
    network.insert_arc(source, b, 5);   // capacity = 5
    network.insert_arc(a, b, 15);
    network.insert_arc(a, sink, 10);
    network.insert_arc(b, sink, 10);

    // Compute maximum flow
    long max_flow = edmonds_karp_maximum_flow(network, source, sink);

    std::cout << "Maximum flow: " << max_flow << "\n";

    // Examine flow on each arc
    network.for_each_arc([](auto* arc) {
        std::cout << "Flow: " << arc->flow << "/" << arc->cap << "\n";
    });

    return 0;
}
```

#### Flujo máximo de costo mínimo

```cpp
#include <tpl_netcost.H>
#include <tpl_mincost.H>

int main() {
    // Network with capacities AND costs
    Net_Cost_Graph<long, long, long> network;

    // Build network with costs...
    // insert_arc(src, dst, capacity, cost)

    auto [flow, cost] = min_cost_max_flow(network, source, sink);

    std::cout << "Max flow: " << flow << "\n";
    std::cout << "Min cost: " << cost << "\n";

    return 0;
}
```

<a id="readme-es-conectividad"></a>
### Conectividad en grafos

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     CONNECTIVITY ALGORITHMS                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  CONNECTED COMPONENTS              STRONGLY CONNECTED COMPONENTS (SCC)      │
│  (Undirected graphs)               (Directed graphs)                        │
│                                                                             │
│  ┌───┐   ┌───┐                     A ──▶ B ──▶ C                            │
│  │ A │───│ B │                     ▲         │                              │
│  └───┘   └───┘                     │    ┌────┘                              │
│                                    │    ▼                                   │
│  ┌───┐   ┌───┐                     └─── D ◀── E                             │
│  │ C │───│ D │                                                              │
│  └───┘   └───┘                     SCCs: {A,B,C,D}, {E}                     │
│                                                                             │
│  2 components                      Tarjan's O(V+E)                          │
│  DFS/BFS O(V+E)                    Kosaraju's O(V+E)                        │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ARTICULATION POINTS               BRIDGES                                  │
│  (Cut vertices)                    (Cut edges)                              │
│                                                                             │
│  A ─── B ─── C                     A ─── B ═══ C                            │
│        │                                 ║                                  │
│        │                           Bridge: edge whose removal               │
│        D                           disconnects the graph                    │
│                                                                             │
│  B is articulation point:                                                   │
│  removing B disconnects graph                                               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

```cpp
#include <tpl_graph.H>
#include <tpl_components.H>
#include <Tarjan.H>
#include <tpl_cut_nodes.H>

int main() {
    Graph g;
    // ... build graph ...

    // Find connected components (undirected)
    DynList<Graph> components;
    size_t num_components = connected_components(g, components);

    std::cout << "Found " << num_components << " components\n";

    // Find strongly connected components (directed)
    DynList<DynList<Node*>> sccs;
    tarjan_scc(digraph, sccs);

    // Find articulation points
    auto cut_nodes = compute_cut_nodes(g);

    std::cout << "Articulation points:\n";
    cut_nodes.for_each([](auto* node) {
        std::cout << "  " << node->get_info() << "\n";
    });

    // Find bridges (cut edges) — Tarjan O(V+E)
    auto bridges = find_bridges(g);

    std::cout << "Bridges:\n";
    bridges.for_each([&g](auto* arc) {
        std::cout << "  " << g.get_src_node(arc)->get_info()
                  << " -- " << g.get_tgt_node(arc)->get_info() << "\n";
    });

    // Class form: cheaper when calling find_bridges() repeatedly
    Compute_Bridges<Graph> cb(g);
    auto b2 = cb.find_bridges(nodes[2]); // start from a specific node

    return 0;
}
```

<a id="readme-es-two-sat"></a>
### 2-SAT (Satisfacibilidad)

El problema de **2-satisfacibilidad** (2-SAT) determina si una fórmula booleana en CNF, con dos literales por cláusula, es satisfacible. Aleph-w ofrece un resolvedor en tiempo lineal `O(V+E)` basado en el grafo de implicación y el algoritmo de componentes fuertemente conexas (SCC) de Tarjan.

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                       RESUMEN DEL RESOLVEDOR 2-SAT                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  FÓRMULA: (x0 v x1) ∧ (~x0 v x2) ∧ (~x1 v ~x2)                              │
│                                                                             │
│  GRAFO DE IMPLICACIÓN:                                                      │
│  ~x0 ──▶ x1       x0 ──▶ x2       x1 ──▶ ~x2                                │
│  ~x1 ──▶ x0      ~x2 ──▶ ~x0      x2 ──▶ ~x1                                │
│                                                                             │
│  ALGORITMO:                                                                 │
│  1. Construir el grafo de implicación con 2N nodos.                         │
│  2. Calcular Componentes Fuertemente Conexas (SCC).                         │
│  3. La fórmula es SAT sii ningún par x y ~x están en la misma SCC.          │
│  4. Si es SAT, el orden topológico de las SCC da una asignación válida.      │
│                                                                             │
│  Complejidad: O(n + m) para n variables y m cláusulas                       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### Ejemplos de uso

```cpp
#include <Two_Sat.H>

int main() {
    // Resolver: (x0 OR x1) AND (~x0 OR x2) AND (~x1 OR ~x2)
    Two_Sat<> sat(3);

    // Usando índices de literales
    sat.add_clause(sat.pos_lit(0), sat.pos_lit(1));  // x0 v x1
    sat.add_clause(sat.neg_lit(0), sat.pos_lit(2));  // ~x0 v x2
    sat.add_clause(sat.neg_lit(1), sat.neg_lit(2));  // ~x1 v ~x2

    // Verificar satisfacibilidad y obtener asignación
    auto [ok, assignment] = sat.solve();

    if (ok) {
        bool x0 = assignment(0);
        bool x1 = assignment(1);
        bool x2 = assignment(2);
    }

    // Usando la API de variables signadas 1-based (más conciso)
    Two_Sat<> sat2(3);
    sat2.add_clause_signed(1, 2);    // x0 v x1
    sat2.add_clause_signed(-1, 3);   // ~x0 v x2
    sat2.add_clause_signed(-2, -3);  // ~x1 v ~x2

    return 0;
}
```

<a id="readme-es-planaridad"></a>
### Prueba de planaridad

Úsalo cuando tu problema depende de si un grafo puede embebirse en el plano
sin cruces de aristas.

Escenarios típicos:
- decidir si una topología es físicamente dibujable sin cruces
- validar restricciones de grafos antes de usar algoritmos solo planares
- extraer evidencia verificable por máquina de no-planaridad
- obtener un embedding combinatorio para razonar sobre caras/rotaciones

`Planarity_Test.H` ofrece:
- `planarity_test()`: resultado detallado (`is_planar` + metadata de normalización)
- `is_planar_graph()`: API booleana de conveniencia
- wrappers `Planarity_Test<...>` y `Is_Planar_Graph<...>`

La prueba se hace sobre el grafo simple no dirigido subyacente:
- se ignora la orientación de digrafos
- se ignoran self-loops
- se colapsan arcos paralelos

Las salidas avanzadas son opt-in mediante `Planarity_Test_Options`:
- `compute_embedding`: embedding combinatorio (rotation system + caras)
  - modo primario: construcción LR-first (`embedding_is_lr_linear = true`)
    con reparación LR-local determinística (candidatos reversal/swap,
    multi-start coordinate descent)
  - fallback opcional: búsqueda exacta acotada si el embedding LR falla validación
  - perfil estricto (`embedding_allow_bruteforce_fallback = false`) evita
    fallback exhaustivo y típicamente es más rápido (sin backtracking exponencial),
    pero puede retornar `is_planar = true` sin embedding
    (`has_combinatorial_embedding = false`, `embedding_search_truncated = true`)
  - `embedding_max_combinations` es el presupuesto de evaluación de LR local-repair
    en modo estricto (y el presupuesto de búsqueda exacta cuando se usa fallback)
  - perfil robusto (recomendado para workflows de cara/dual): mantener fallback habilitado
- `compute_nonplanar_certificate`: witness de no-planaridad
  - primero reduce a una obstrucción mínima por pasadas de borrado de aristas/vértices
  - luego busca patrones de Kuratowski en el branch-core
  - retorna `K5_Subdivision`, `K33_Subdivision`, o
    `Minimal_NonPlanar_Obstruction` si la clasificación no es posible bajo límites
  - cada arista witness mantiene trazabilidad a los arcos de entrada originales
    (`representative_input_arc`, `input_arcs`)
  - cada camino witness incluye tanto nodos como secuencia de aristas trazable
    (`certificate_paths[i].nodes`, `certificate_paths[i].edges`)

Modelo de garantía del embedding:
- la decisión de planaridad (`is_planar`) viene de la prueba LR y se mantiene exacta.
- el modo estricto de embedding LR (`embedding_allow_bruteforce_fallback = false`) es una
  estrategia constructiva acotada; si llega al presupuesto, el embedding puede faltar
  (`has_combinatorial_embedding = false`, `embedding_search_truncated = true`)
  incluso para grafos planares.
- si necesitas completitud del embedding, mantén el fallback habilitado y provee suficiente
  presupuesto `embedding_max_combinations`.

APIs de cara y dual:
- `planar_dual_metadata(result)`:
  - límites de caras como dart walks
  - adyacencia entre caras
  - relación arista-primal ↔ arista-dual
- `build_planar_dual_graph(result)` / `build_planar_dual_graph(metadata)`:
  - construye un grafo dual de Aleph (`List_Graph` por defecto)
  - info de nodo dual: face id
  - info de arco dual: `Planar_Dual_Edge_Info<GT>`

API de dibujo geométrico consciente del embedding:
- `planar_geometric_drawing(result, drawing_options)`:
  - computa coordenadas 2D para nodos del embedding
  - usa relajación armónica sobre el embedding combinatorio
  - puede validar cruces de segmentos (`drawing_validated_no_crossings`)

APIs de exportación de certificados:
- `nonplanar_certificate_to_json(result)`:
  - witness estructurado y legible por máquina (nodos, aristas de obstrucción, caminos)
- `nonplanar_certificate_to_dot(result)`:
  - GraphViz DOT con obstrucción/caminos resaltados
- `nonplanar_certificate_to_graphml(result)`:
  - formato GraphML de intercambio para herramientas/pipelines
- `nonplanar_certificate_to_gexf(result)`:
  - formato GEXF de intercambio para tooling estilo Gephi
- `validate_nonplanar_certificate(result)` / `nonplanar_certificate_is_valid(result)`:
  - validación de consistencia estructural para witnesses exportados

Adaptador de validación externa (artefactos end-to-end):
- `scripts/planarity_certificate_validator.rb`
  - valida archivos `GraphML` / `GEXF` exportados directamente
  - chequea consistencia XML, referencias de endpoints y presencia de aristas de obstrucción
  - pass opcional `--networkx` para chequeos de loadability a nivel de herramienta
  - pass opcional `--gephi` para chequeos del adaptador Gephi CLI/toolkit
    (`--gephi-cmd` soporta un template de comando custom con `{input}`)
  - pass opcional `--render-gephi` para validación de render/export
    (`--render-profile` + `--render-output-dir`, con chequeos de artefactos SVG/PDF)
- `scripts/planarity_gephi_templates.json`
  - catálogo de templates de comando Gephi por OS/versión (más template CI portable)
- `scripts/planarity_gephi_render_profiles.json`
  - catálogo de perfiles de render/export Gephi (SVG/PDF) por OS/versión
    (más perfiles CI portables determinísticos)
- `scripts/planarity_certificate_ci_batch.rb`
  - wrapper reproducible de batch para CI que emite un reporte JSON determinístico
- `scripts/planarity_visual_golden_manifest.json`
  - baseline determinístico de golden digest para perfiles de render portables
- `scripts/planarity_certificate_ci_visual_diff.rb`
  - runner CI dedicado a visual-golden (render + SHA256 diff vs manifest)
- `scripts/planarity_gephi_weekly_comparison.rb`
  - agregador de runs para artefactos nocturnos + detección de regresiones (`overall_valid` y deltas de exit-code)
- `scripts/planarity_gephi_regression_notify.rb`
  - notificador opcional para regresiones nocturnas (alerta Markdown + webhook dispatch)
- `scripts/fixtures/planarity_k33_certificate.graphml`
  - fixture de certificado no-planar generado por Aleph usado por CI/adapter probes
- `.github/workflows/planarity_gephi_nightly.yml`
  - probe real de empaquetado Gephi semanal + manual (Linux/macOS/Windows)
  - auto-selecciona tags `0.9.x` y `0.10.x` más recientes (o override manual)
  - descarga binarios oficiales de Gephi y valida integración del adaptador
  - emite artefacto de reporte comparativo por run a través de matriz tags/OS
  - aplica gate de regresión cuando tags nuevos regresan vs tags previos por OS
  - notificaciones opcionales por webhook vía secret
    `ALEPH_PLANARITY_ALERT_WEBHOOK`

```bash
ruby scripts/planarity_certificate_validator.rb \
  --input /tmp/planarity_k33_certificate.graphml \
  --input /tmp/planarity_k33_certificate.gexf

# Optional: also verify loadability through NetworkX
ruby scripts/planarity_certificate_validator.rb \
  --input /tmp/planarity_k33_certificate.graphml \
  --networkx

# Optional: Gephi adapter mode (portable; command template is user-configurable)
ruby scripts/planarity_certificate_validator.rb \
  --input /tmp/planarity_k33_certificate.graphml \
  --gephi \
  --gephi-cmd "gephi --headless --import {input}"

# List template catalog (filterable by OS)
ruby scripts/planarity_certificate_validator.rb \
  --list-gephi-templates --template-os linux --json

# Use template id from catalog
ruby scripts/planarity_certificate_validator.rb \
  --input /tmp/planarity_k33_certificate.graphml \
  --gephi --require-gephi \
  --gephi-template portable.python-file-exists

# List render profile catalog (filterable by OS)
ruby scripts/planarity_certificate_validator.rb \
  --list-gephi-render-profiles --render-os linux --json

# Run render profile and validate produced artifact
ruby scripts/planarity_certificate_validator.rb \
  --input /tmp/planarity_k33_certificate.graphml \
  --render-gephi --require-render \
  --render-profile portable.python-render-svg \
  --render-output-dir /tmp/aleph_planarity_renders

# CI batch report (deterministic JSON artifact)
ruby scripts/planarity_certificate_ci_batch.rb \
  --input /tmp/planarity_k33_certificate.graphml \
  --input /tmp/planarity_k33_certificate.gexf \
  --gephi --require-gephi \
  --gephi-template portable.python-file-exists \
  --report /tmp/aleph_planarity_ci_report.json --print-summary

# CI batch report with render validation
ruby scripts/planarity_certificate_ci_batch.rb \
  --input /tmp/planarity_k33_certificate.graphml \
  --render-gephi --require-render \
  --render-profile portable.python-render-svg \
  --render-output-dir /tmp/aleph_planarity_renders \
  --report /tmp/aleph_planarity_ci_render_report.json --print-summary

# Dedicated visual golden-diff runner (deterministic)
ruby scripts/planarity_certificate_ci_visual_diff.rb \
  --input /tmp/planarity_k33_certificate.graphml \
  --profile portable.python-render-svg \
  --profile portable.python-render-pdf \
  --render-output-dir /tmp/aleph_planarity_visual_renders \
  --report /tmp/aleph_planarity_visual_diff_report.json --print-summary

# Real-Gephi local smoke check (without portable profiles)
ruby scripts/planarity_certificate_validator.rb \
  --input scripts/fixtures/planarity_k33_certificate.graphml \
  --gephi --require-gephi \
  --gephi-cmd "\"/path/to/gephi\" --version"

# Nightly workflow manual override example:
# workflow_dispatch input gephi_tags="v0.9.7,v0.10.1"

# Local nightly artifact comparison/regression check (Ruby implementation)
ruby scripts/planarity_gephi_weekly_comparison.rb \
  --artifacts-root /tmp/gephi-nightly-artifacts \
  --resolved-tags v0.9.7,v0.10.1 \
  --run-id local --run-attempt 1 --git-sha local \
  --report-json /tmp/gephi_weekly_comparison.json \
  --report-md /tmp/gephi_weekly_comparison.md \
  --print-summary

# Optional regression notification (webhook)
ALEPH_PLANARITY_ALERT_WEBHOOK="https://example.invalid/webhook" \
ruby scripts/planarity_gephi_regression_notify.rb \
  --report-json /tmp/gephi_weekly_comparison.json \
  --output-md /tmp/gephi_nightly_alert.md \
  --repository lrleon/Aleph-w \
  --run-url https://github.com/lrleon/Aleph-w/actions/runs/123 \
  --webhook-env ALEPH_PLANARITY_ALERT_WEBHOOK \
  --print-summary
```

```cpp
#include <tpl_graph.H>
#include <Planarity_Test.H>

using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

int main() {
    Graph g;
    auto* u0 = g.insert_node(0);
    auto* u1 = g.insert_node(1);
    auto* u2 = g.insert_node(2);
    auto* v0 = g.insert_node(3);
    auto* v1 = g.insert_node(4);
    auto* v2 = g.insert_node(5);

    // K3,3
    g.insert_arc(u0, v0); g.insert_arc(u0, v1); g.insert_arc(u0, v2);
    g.insert_arc(u1, v0); g.insert_arc(u1, v1); g.insert_arc(u1, v2);
    g.insert_arc(u2, v0); g.insert_arc(u2, v1); g.insert_arc(u2, v2);

    Planarity_Test_Options opts;
    opts.compute_embedding = true;
    opts.compute_nonplanar_certificate = true;
    opts.embedding_prefer_lr_linear = true;
    opts.embedding_allow_bruteforce_fallback = true;

    auto result = planarity_test(g, opts);
    std::cout << "Planar: " << result.is_planar << "\n";  // false
    std::cout << "Certificate: " << to_string(result.certificate_type) << "\n";
    if (!result.certificate_obstruction_edges.is_empty())
        std::cout << "First witness edge multiplicity: "
                  << result.certificate_obstruction_edges[0].input_arcs.size()
                  << "\n";

    if (result.is_planar && result.has_combinatorial_embedding) {
        auto md = planar_dual_metadata(result);
        auto dual = build_planar_dual_graph<Graph>(md);
        auto drawing = planar_geometric_drawing(result);
        std::cout << "Dual faces: " << dual.get_num_nodes() << "\n";
        std::cout << "Crossings: " << drawing.crossing_count << "\n";
    } else if (result.has_nonplanar_certificate) {
        auto vr = validate_nonplanar_certificate(result);
        auto json = nonplanar_certificate_to_json(result);
        auto dot = nonplanar_certificate_to_dot(result);
        auto graphml = nonplanar_certificate_to_graphml(result);
        auto gexf = nonplanar_certificate_to_gexf(result);
        std::cout << "Certificate valid: " << vr.is_valid << "\n";
        std::cout << "JSON bytes: " << json.size() << "\n";
        std::cout << "DOT bytes: " << dot.size() << "\n";
        std::cout << "GraphML bytes: " << graphml.size() << "\n";
        std::cout << "GEXF bytes: " << gexf.size() << "\n";
    }
}
```

<a id="readme-es-emparejamiento"></a>
### Emparejamiento

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                           MATCHING ALGORITHMS                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  BIPARTITE MATCHING                                                         │
│                                                                             │
│  Hopcroft-Karp: O(E√V)                                                      │
│  Max-Flow Reduction: O(VE)                                                  │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  GENERAL GRAPH MATCHING (Edmonds-Blossom)                                  │
│                                                                             │
│  Handles odd cycles (blossoms) in non-bipartite graphs                     │
│  Time: O(V³)                                                                │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  GENERAL GRAPH WEIGHTED MATCHING (Weighted Blossom)                        │
│                                                                             │
│  Maximizes total matching weight in non-bipartite graphs                   │
│  Optional: maximize cardinality first, then weight                          │
│  Backends: List_Graph, List_SGraph, Array_Graph                             │
│  Time: O(V³)                                                                │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  GENERAL GRAPH MINIMUM-COST MATCHING (Dedicated API)                       │
│                                                                             │
│  Minimizes total matching cost in non-bipartite graphs                     │
│  Optional: maximize cardinality first, then minimize cost                   │
│  Also supports minimum-cost perfect matching (feasibility-aware)            │
│  API: blossom_minimum_cost_matching(),                                       │
│       blossom_minimum_cost_perfect_matching()                               │
│  Time: O(V³)                                                                │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ASSIGNMENT PROBLEM (Hungarian / Munkres)                                   │
│                                                                             │
│  Given an m×n cost matrix, find the minimum-cost perfect matching           │
│  between rows and columns.                                                  │
│                                                                             │
│  Cost matrix:          Optimal assignment:                                  │
│  ┌──────────────┐      Worker 0 → Task 2  (cost  69)                        │
│  │  82  83  69  │      Worker 1 → Task 1  (cost  37)                        │
│  │  77  37  49  │      Worker 2 → Task 0  (cost  11)                        │
│  │  11  69   5  │      Worker 3 → Task 3  (cost  23)                        │
│  │   8   9  98  │                                                           │
│  └──────────────┘      Total cost: 140                                      │
│                                                                             │
│  Algorithm: Shortest augmenting paths + dual variables (potentials)         │
│  Time:  O(n³)   Space: O(n²)   Handles: rectangular, negative costs        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### Emparejamiento en grafos generales (Edmonds-Blossom)

```cpp
#include <Blossom.H>
#include <tpl_graph.H>

using namespace Aleph;

int main() {
    using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
    Graph g;

    auto* n0 = g.insert_node(0);
    auto* n1 = g.insert_node(1);
    auto* n2 = g.insert_node(2);
    auto* n3 = g.insert_node(3);
    auto* n4 = g.insert_node(4);

    // Odd cycle + stem arcs
    g.insert_arc(n0, n1);
    g.insert_arc(n1, n2);
    g.insert_arc(n2, n3);
    g.insert_arc(n3, n4);
    g.insert_arc(n4, n0);
    g.insert_arc(n1, n4);

    DynDlist<Graph::Arc*> matching;
    size_t cardinality = blossom_maximum_cardinality_matching(g, matching);

    std::cout << "Maximum matching size: " << cardinality << "\n";
    return 0;
}
```

#### Emparejamiento ponderado en grafos generales (Weighted Blossom)

`Blossom_Weighted.H` es independiente del backend en Aleph. Se prueba en:
- `List_Graph<Graph_Node<int>, Graph_Arc<long long>>`
- `List_SGraph<Graph_Snode<int>, Graph_Sarc<long long>>`
- `Array_Graph<Graph_Anode<int>, Graph_Aarc<long long>>`

Ver `Tests/weighted_blossom_test.cc` y `Examples/weighted_blossom_example.cc`.

```cpp
#include <Blossom_Weighted.H>
#include <tpl_graph.H>
#include <tpl_sgraph.H>
#include <tpl_agraph.H>

using namespace Aleph;

int main() {
    using ListGraph = List_Graph<Graph_Node<int>, Graph_Arc<long long>>;
    using SGraph = List_SGraph<Graph_Snode<int>, Graph_Sarc<long long>>;
    using AGraph = Array_Graph<Graph_Anode<int>, Graph_Aarc<long long>>;

    ListGraph g;

    auto* n0 = g.insert_node(0);
    auto* n1 = g.insert_node(1);
    auto* n2 = g.insert_node(2);
    auto* n3 = g.insert_node(3);

    g.insert_arc(n0, n1, 9);
    g.insert_arc(n1, n2, 10);
    g.insert_arc(n2, n3, 8);
    g.insert_arc(n0, n3, 7);

    DynDlist<ListGraph::Arc*> matching;
    auto result = blossom_maximum_weight_matching(g, matching);
    // Optional lexicographic objective:
    // auto result = blossom_maximum_weight_matching(
    //     g, matching, Dft_Dist<ListGraph>(), Dft_Show_Arc<ListGraph>(), true);

    std::cout << "Matching size: " << result.cardinality << "\n";
    std::cout << "Total weight: " << result.total_weight << "\n";
    return 0;
}
```

#### Emparejamiento de costo mínimo en grafos generales (API dedicada)

`Min_Cost_Matching.H` provee una API dedicada para emparejamiento de costo mínimo no bipartito
sobre backends de grafo Aleph (`List_Graph`, `List_SGraph`, `Array_Graph`).

También provee una variante dedicada de emparejamiento perfecto:
`blossom_minimum_cost_perfect_matching()`, que reporta factibilidad
y retorna el costo mínimo de emparejamiento perfecto cuando es factible.

```cpp
#include <Min_Cost_Matching.H>
#include <tpl_graph.H>

using namespace Aleph;

int main() {
    using Graph = List_Graph<Graph_Node<int>, Graph_Arc<long long>>;
    Graph g;

    auto* n0 = g.insert_node(0);
    auto* n1 = g.insert_node(1);
    auto* n2 = g.insert_node(2);
    auto* n3 = g.insert_node(3);

    g.insert_arc(n0, n1, 8);
    g.insert_arc(n0, n2, -5);
    g.insert_arc(n1, n3, 6);
    g.insert_arc(n2, n3, 2);

    DynDlist<Graph::Arc*> matching;

    // Pure minimum-cost objective
    auto pure = blossom_minimum_cost_matching(g, matching);

    // Lexicographic objective:
    // maximum-cardinality first, then minimum-cost
    auto card_first = blossom_minimum_cost_matching(
        g, matching, Dft_Dist<Graph>(), Dft_Show_Arc<Graph>(), true);

    std::cout << "Pure   -> card: " << pure.cardinality
              << ", cost: " << pure.total_cost << "\n";
    std::cout << "Card+  -> card: " << card_first.cardinality
              << ", cost: " << card_first.total_cost << "\n";

    // Perfect matching variant (feasibility-aware)
    auto perfect = blossom_minimum_cost_perfect_matching(g, matching);
    if (perfect.feasible)
        std::cout << "Perfect -> card: " << perfect.cardinality
                  << ", cost: " << perfect.total_cost << "\n";
    else
        std::cout << "Perfect -> infeasible\n";
    return 0;
}
```

#### Asignación húngara (Munkres)

```cpp
#include <Hungarian.H>
#include <tpl_dynMat.H>

using namespace Aleph;

int main() {
    // Construct directly from initializer list
    Hungarian_Assignment<int> ha({
        {82, 83, 69, 92},
        {77, 37, 49, 92},
        {11, 69,  5, 86},
        { 8,  9, 98, 23}
    });

    std::cout << "Optimal cost: " << ha.get_total_cost() << "\n";  // 140

    for (auto [worker, task] : ha.get_assignments())
        std::cout << "Worker " << worker << " -> Task " << task << "\n";

    // Maximization: negate costs automatically
    DynMatrix<int> profit(3, 3, 0);
    profit.allocate();
    // ... fill profit matrix ...
    auto result = hungarian_max_assignment(profit);
    std::cout << "Max profit: " << result.total_cost << "\n";

    return 0;
}
```

<a id="readme-es-algoritmos-de-cadenas"></a>
### Algoritmos de cadenas

Aleph-w ahora incluye un toolkit clásico completo de cadenas dividido por propósito:

| Header | Alcance |
|---|---|
| `String_Search.H` | KMP, algoritmo Z, Boyer-Moore-Horspool, Rabin-Karp |
| `Aho_Corasick.H` | Emparejamiento multi-patrón |
| `Suffix_Structures.H` | Suffix Array, LCP (Kasai), Suffix Tree comprimido ingenuo, Suffix Automaton |
| `String_Palindromes.H` | Manacher (subcadena palíndroma más larga en O(n)) |
| `String_DP.H` | Levenshtein, Damerau-Levenshtein, LCS, subcadena común más larga |
| `String_Algorithms.H` | include paraguas para todos los algoritmos de cadenas |

#### Algoritmos cubiertos

- Búsqueda de un solo patrón: `kmp_search`, `z_search`, `boyer_moore_horspool_search`, `rabin_karp_search`
- Búsqueda multi-patrón: `Aho_Corasick`
- Estructuras de sufijos: `suffix_array`, `lcp_array_kasai`, `Naive_Suffix_Tree`, `Suffix_Automaton`
- Palíndromos: `manacher`, `longest_palindromic_substring`
- Edición/similitud: `levenshtein_distance`, `damerau_levenshtein_distance`
- Similitud de secuencias: `longest_common_subsequence`, `longest_common_substring`

#### Ejemplo de uso

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

    auto pal = Aleph::manacher("forgeeksskeegfor");
    auto lcs = Aleph::longest_common_subsequence("AGGTAB", "GXTXAYB");
    auto dist = Aleph::damerau_levenshtein_distance("ca", "ac");

    std::cout << "kmp matches: " << kmp.size() << "\n";
    std::cout << "aho matches: " << multi.size() << "\n";
    std::cout << "sa size: " << sa.size() << ", lcp size: " << lcp.size() << "\n";
    std::cout << "longest palindrome: " << pal.longest_palindrome << "\n";
    std::cout << "lcs length: " << lcs.length << ", damerau: " << dist << "\n";
}
```

#### Ejemplos de cadenas

Cada algoritmo tiene un ejemplo enfocado bajo `Examples/`:

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

<a id="readme-es-programacion-dinamica"></a>
### Algoritmos de programación dinámica

Aleph-w también incluye un toolkit práctico de programación dinámica:

| Header | Alcance |
|---|---|
| `Knapsack.H` | mochila 0/1, ilimitada y acotada (con reconstrucción) |
| `LIS.H` | LIS y LNDS en O(n log n), con reconstrucción de la secuencia |
| `Matrix_Chain.H` | parentización óptima de cadena de matrices (DP por intervalos) |
| `Subset_Sum.H` | subset sum (existencia/reconstrucción/conteo) + MITM |
| `Tree_DP.H` | DP genérica en árboles + rerooting DP (todas las raíces en O(n)) |
| `DP_Optimizations.H` | D&C DP, Knuth, Convex Hull Trick, Li Chao, monotone queue |

#### APIs de DP cubiertas

- Mochila: `knapsack_01`, `knapsack_01_value`, `knapsack_unbounded`, `knapsack_bounded`
- Subsecuencias: `longest_increasing_subsequence`, `lis_length`, `longest_nondecreasing_subsequence`
- Cadena de matrices: `matrix_chain_order`, `matrix_chain_min_cost`
- Subset sum: `subset_sum`, `subset_sum_exists`, `subset_sum_count`, `subset_sum_mitm`
- Árboles: `Gen_Tree_DP`, `Gen_Reroot_DP`, `tree_subtree_sizes`, `tree_max_distance`, `tree_sum_of_distances`
- Optimizaciones de DP: `divide_and_conquer_partition_dp`, `knuth_optimize_interval`, `optimal_merge_knuth`, `Convex_Hull_Trick`, `Li_Chao_Tree`, `monotone_queue_min_dp`, `min_weighted_squared_distance_1d`

#### Ejemplo de uso de DP

```cpp
#include <Knapsack.H>
#include <LIS.H>
#include <Matrix_Chain.H>
#include <Subset_Sum.H>
#include <DP_Optimizations.H>

int main() {
    Aleph::Array<Aleph::Knapsack_Item<int, int>> items = {{2, 3}, {3, 4}, {4, 5}};
    auto k = Aleph::knapsack_01(items, 6);

    Aleph::Array<int> seq = {10, 9, 2, 5, 3, 7, 101, 18};
    auto lis = Aleph::longest_increasing_subsequence(seq);

    Aleph::Array<size_t> dims = {30, 35, 15, 5, 10, 20, 25};
    auto mc = Aleph::matrix_chain_order(dims);

    Aleph::Array<int> vals = {3, 34, 4, 12, 5, 2};
    auto ss = Aleph::subset_sum(vals, 9);

    Aleph::Array<long long> xs = {-3, 0, 2, 7};
    Aleph::Array<long long> ws = {4, 1, 9, 2};
    auto geo = Aleph::min_weighted_squared_distance_1d(xs, ws);

    return int(k.optimal_value + lis.length + mc.min_multiplications + ss.exists + geo.size());
}
```

#### Ejemplos de DP

- `knapsack_example.cc`
- `lis_example.cc`
- `matrix_chain_example.cc`
- `subset_sum_example.cc`
- `tree_dp_example.cc`
- `dp_optimizations_example.cc`

<a id="readme-es-algoritmos-de-ordenamiento"></a>
### Algoritmos de ordenamiento

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        SORTING ALGORITHMS                                   │
├─────────────────┬──────────────┬──────────────┬──────────────┬──────────────┤
│   Algorithm     │    Best      │   Average    │    Worst     │   Stable?    │
├─────────────────┼──────────────┼──────────────┼──────────────┼──────────────┤
│ Quicksort       │  O(n log n)  │  O(n log n)  │    O(n²)     │     No       │
│ Mergesort       │  O(n log n)  │  O(n log n)  │  O(n log n)  │    Yes       │
│ Heapsort        │  O(n log n)  │  O(n log n)  │  O(n log n)  │     No       │
│ Introsort       │  O(n log n)  │  O(n log n)  │  O(n log n)  │     No       │
│ Insertion Sort  │    O(n)      │    O(n²)     │    O(n²)     │    Yes       │
│ Shell Sort      │  O(n log n)  │   O(n^1.3)   │    O(n²)     │     No       │
└─────────────────┴──────────────┴──────────────┴──────────────┴──────────────┘

  Introsort: Hybrid algorithm
  - Starts with Quicksort
  - Switches to Heapsort if recursion too deep
  - Uses Insertion Sort for small subarrays
  - Guarantees O(n log n) worst case
```

```cpp
#include <ahSort.H>
#include <tpl_dynArray.H>

int main() {
    DynArray<int> arr = {5, 2, 8, 1, 9, 3, 7, 4, 6};

    // Sort using default (introsort for arrays)
    quicksort(arr);

    // Sort with custom comparator
    quicksort(arr, [](int a, int b) { return a > b; });  // Descending

    // For lists, mergesort is used automatically
    DynList<int> list = {5, 2, 8, 1, 9};
    mergesort(list);

    return 0;
}
```

---

<a id="readme-es-procesamiento-de-senales"></a>
### Procesamiento de señales (FFT)

Aleph-w incluye un completo toolkit de **Transformada Rápida de Fourier** y procesamiento digital de señales en [`fft.H`](fft.H). Capacidades principales:

- **FFT/IFFT** para señales reales y complejas (Cooley-Tukey radix-2/4, Bluestein para tamaños arbitrarios)
- **STFT/ISTFT** con procesadores streaming para análisis tiempo-frecuencia
- **Diseño de filtros FIR**: método de la ventana (`firwin`), mínimos cuadrados (`firls`) y Remez
- **Diseño de filtros IIR**: familias Butterworth, Chebyshev I/II, Bessel y Elíptico (Cauer)
- **Estimación espectral**: PSD de Welch, densidad espectral cruzada, coherencia
- **Convolución eficiente**: Overlap-Add, Overlap-Save y convolución particionada para streaming de baja latencia
- **Resampleo**: resampleo racional polifásico (`resample_poly`, `upfirdn`)
- **Transformadas N-dimensionales** (2-D, 3-D)
- **Raíces de polinomios** via eigenvalores de la matriz compañera
- **Variantes paralelas** de todos los métodos via `ThreadPool` (prefijados con `p`)
- **Despacho SIMD**: selección automática AVX2/NEON en tiempo de ejecución

Para un recorrido detallado con ejemplos de código, ver el **[Tutorial FFT y Procesamiento Digital de Señales](docs/fft-tutorial.md)** ([English](docs/fft-tutorial.en.md)).

---
<a id="readme-es-ntt"></a>
### Transformada teórico-numérica (NTT)

Aleph-w también incluye una implementación industrial de la **Number
Theoretic Transform** en [`ntt.H`](ntt.H). Capacidades principales:

- **Transformadas y productos modulares exactos** bajo primos NTT-friendly
- **Planes reutilizables** via `NTT<MOD, ROOT>::Plan`
- **Soporte Bluestein** para tamaños no potencia de dos cuando el módulo admite las raíces necesarias
- **Butterflies Montgomery** para el hot loop modular
- **Variantes paralelas con `ThreadPool`** para transformadas, productos y lotes
- **Dispatch SIMD** con selección AVX2/NEON en tiempo de ejecución sobre el camino potencia-de-dos
- **Reconstrucción CRT de tres primos** en `__uint128_t` mediante `NTTExact`
- **Álgebra formal de polinomios**: inversa, división, log, exp, sqrt, potencia, evaluación multipunto e interpolación
- **Multiplicación de enteros grandes** sobre dígitos en base `B` con propagación de carry
- **Convolución negacíclica** módulo `x^N + 1`

Para un recorrido detallado con ejemplos de código, consulta el **[Tutorial
NTT](docs/ntt-tutorial.md)** ([English](docs/ntt-tutorial.en.md)).

---

<a id="readme-es-aritmetica-de-polinomios"></a>
### Aritmética de polinomios

Aleph-w proporciona una clase genérica de **polinomios univariados dispersos** en
[`tpl_polynomial.H`](tpl_polynomial.H). El tipo de coeficiente es un
parámetro de plantilla (por defecto `double`). Solo los coeficientes no nulos se
almacenan en un `DynMapTree<size_t, Coefficient>`, haciendo las operaciones
eficientes en polinomios dispersos de alto grado como `x^1000 + 1`.

**Aritmética**: `+`, `-`, `*`, `/`, `%`, `divmod`, `*` y `/` escalar,
negación unaria, variantes in-place (`+=`, `-=`, `*=`, `/=`, `%=`).

**Cálculo**: `derivative`, `nth_derivative`, `integral` (indefinida),
`definite_integral(a, b)`.

**Evaluación**: `eval(x)` adaptativa (Horner para densos, potencia directa
para ultra-dispersos), `horner_eval`, `sparse_eval`, `multi_eval(points)`,
`operator()(x)`.

**Composición y potencia**: `compose(q)` (Horner sparse-aware), `pow(n)`
(cuadratura repetida).

**Fábricas**: `zero()`, `one()`, `x_to(n)`, `from_roots(lista)`,
`interpolate(puntos)` (diferencias divididas de Newton).

**Familia GCD**: `gcd(a, b)`, `xgcd(a, b)` (extendido, retorna coeficientes
de Bezout), `lcm(a, b)`, `pseudo_divmod(d)` para coeficientes enteros exactos.

**Transformaciones algebraicas**: `to_monic()`, `square_free()`,
`reverse()`, `negate_arg()`, `shift(k)` (desplazamiento de Taylor p(x+k)),
`shift_up(k)`, `shift_down(k)`, `truncate(n)`, `to_dense()`.

**Análisis de raíces** (punto flotante): `cauchy_bound()`,
`sign_variations()` (Descartes), `sturm_chain()`,
`count_real_roots(a, b)`, `count_all_real_roots()`,
`bisect_root(a, b)`, `newton_root(x0)`.

**Alias de tipo**: `Polynomial` (`double`), `PolynomialF` (`float`),
`PolynomialLD` (`long double`).

```cpp
using namespace Aleph;

// Denso: 2 + 3x + x^2
Polynomial p({2, 3, 1});

// Desde raíces: (x-1)(x-2)(x-3) = x^3 - 6x^2 + 11x - 6
auto q = Polynomial::from_roots({1.0, 2.0, 3.0});

std::cout << "p(5) = " << p(5.0) << "\n";            // 42
std::cout << "p'(x) = " << p.derivative() << "\n";    // 2x + 3
std::cout << "raíces de q: " << q.count_all_real_roots() << "\n"; // 3

double raiz = q.newton_root(0.5);                     // 1.0
auto [cociente, resto] = q.divmod(Polynomial({-1, 1})); // dividir por (x-1)
std::cout << cociente << "\n";                         // x^2 - 5x + 6
```

---
<a id="readme-es-gestion-de-memoria"></a>
## Gestión de memoria

<a id="readme-es-asignadores-arena"></a>
### Asignadores arena

La asignación arena proporciona **gestión de memoria ultrarrápida** para estructuras de datos temporales.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         ARENA MEMORY LAYOUT                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Traditional Allocation (malloc):                                           │
│                                                                             │
│  ┌────┐  ┌────┐  ┌────┐  ┌────┐  ┌────┐                                     │
│  │ A  │  │free│  │ B  │  │free│  │ C  │   Fragmented, scattered             │
│  └────┘  └────┘  └────┘  └────┘  └────┘                                     │
│    0   1   2   3   4                  ▲               ▲                     │
│                                      front           back                   │
│  Access:  O(1)                                                              │
│  Append:  O(1) amortized             Enqueue: O(1)                          │
│  Insert:  O(n)                       Dequeue: O(1)                          │
│                                                                             │
│  Arena Allocation:                                                          │
│                                                                             │
│  ┌──────────────────────────────────────────────────────────────────────────┤
│  │  A   │  B   │  C   │  D   │  E   │         available space               │
│  └──────────────────────────────────────────────────────────────────────────┤
│  ▲                              ▲                                      ▲    │
│  base                        current                                 end    │
│                                                                             │
│  Allocation:   Just increment `current` pointer ────── O(1)                 │
│  Deallocation: Reset `current` to `base` ────────────  O(1) bulk            │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  COMPARISON                                                                 │
│                                                                             │
│  ┌────────────────┬─────────────┬──────────────────┐                        │
│  │   Operation    │     malloc  │      Arena       │                        │
│  ├────────────────┼─────────────┼──────────────────┤                        │
│  │   Allocate     │   O(log n)* │      O(1)        │                        │
│  │   Deallocate   │   O(log n)* │  O(1) bulk only  │                        │
│  │   Fragmentation│     High    │      None        │                        │
│  │   Cache perf   │     Poor    │    Excellent     │                        │
│  │   Overhead     │   16+ bytes │    0 bytes       │                        │
│  └────────────────┴─────────────┴──────────────────┘                        │
│  * Depending on allocator implementation                                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### Uso básico de arena

```cpp
#include <ah-arena.H>

int main() {
    // Method 1: Stack-allocated buffer (no heap at all!)
    char buffer[1024 * 1024];  // 1 MB on stack
    AhArenaAllocator arena(buffer, sizeof(buffer));

    // Method 2: Heap-allocated arena
    AhArenaAllocator heap_arena(1024 * 1024);  // 1 MB on heap

    // Allocate raw memory - O(1)
    int* numbers = static_cast<int*>(arena.alloc(100 * sizeof(int)));

    // Allocate aligned memory
    void* aligned = arena.alloc_aligned(1024, 64);  // 64-byte alignment

    // Check memory usage
    std::cout << "Used: " << arena.allocated_size() << " bytes\n";
    std::cout << "Free: " << arena.available_size() << " bytes\n";

    // Construct objects in arena memory
    struct MyClass {
        std::string name;
        int value;
        MyClass(std::string n, int v) : name(std::move(n)), value(v) {}
    };

    MyClass* obj = allocate<MyClass>(arena, "test", 42);

    // Destroy object (calls destructor, reclaims memory if LIFO order)
    dealloc<MyClass>(arena, obj);

    return 0;
}  // Everything freed when buffer goes out of scope
```

#### Árboles con asignación arena

```cpp
#include <tpl_dynSetTree.H>

int main() {
    // Create tree that uses arena for all node allocations
    char buffer[64 * 1024];  // 64 KB
    DynSetTree<int> tree(buffer, sizeof(buffer));

    // Or with dynamic arena size
    DynSetTree<int> tree2(64 * 1024);  // 64 KB heap-allocated arena

    // Insert until arena is exhausted
    for (int i = 0; ; ++i) {
        int* result = tree.insert(i);
        if (result == nullptr) {
            std::cout << "Arena full after " << i << " insertions\n";
            break;
        }
    }

    // Query arena state
    if (tree.uses_arena()) {
        std::cout << "Allocated: " << tree.arena_allocated_size() << "\n";
        std::cout << "Available: " << tree.arena_available_size() << "\n";
    }

    return 0;
}  // All nodes freed in O(1) when tree is destroyed
```

#### Cuándo usar asignación arena

| Caso de uso | Tradicional | Arena | Recomendación |
|----------|-------------|-------|----------------|
| Computaciones temporales | Muchos mallocs | Un solo buffer | **Arena** |
| Tiempo de vida conocido | Rastrear cada objeto | Liberación en bloque | **Arena** |
| Sistemas en tiempo real | Latencia impredecible | alloc O(1) | **Arena** |
| Objetos de larga vida | Bien | No apto | **Tradicional** |
| Objetos con tiempos de vida variables | Bien | Difícil | **Tradicional** |

---

<a id="readme-es-computacion-paralela"></a>
## Computación paralela

### Pool de hilos

```cpp
#include <thread_pool.H>
#include <future>
#include <vector>

int main() {
    // Create pool with hardware concurrency
    ThreadPool pool(std::thread::hardware_concurrency());

    // Submit tasks and get futures
    std::vector<std::future<int>> futures;

    for (int i = 0; i < 100; ++i) {
        futures.push_back(
            pool.enqueue([i] {
                // Expensive computation
                return compute_something(i);
            })
        );
    }

    // Collect results
    int total = 0;
    for (auto& future : futures) {
        total += future.get();  // Blocks until result ready
    }

    std::cout << "Total: " << total << "\n";

    return 0;
}  // Pool automatically joins all threads
```

### Operaciones funcionales paralelas

```cpp
#include <ah-parallel.H>
#include <tpl_dynList.H>

int main() {
    DynList<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // Parallel map: transform all elements concurrently
    auto squared = pmap(numbers, [](int x) {
        return x * x;
    });

    // Parallel filter: filter elements concurrently
    auto evens = pfilter(numbers, [](int x) {
        return x % 2 == 0;
    });

    // Parallel fold: reduce with parallel partial results
    int sum = pfold(numbers, 0, [](int a, int b) {
        return a + b;
    });

    // Parallel for_each: apply operation to all elements
    pfor_each(numbers, [](int& x) {
        x *= 2;
    });

    return 0;
}
```

---
<a id="readme-es-programacion-funcional"></a>
## Programación funcional

Todos los contenedores de Aleph-w soportan un conjunto rico de operaciones funcionales:

### Operaciones centrales

| Operación | Descripción | Complejidad |
|-----------|-------------|------------|
| `for_each(f)` | Aplicar f a cada elemento | O(n) |
| `map(f)` | Transformar elementos | O(n) |
| `filter(p)` | Seleccionar elementos que cumplen el predicado | O(n) |
| `foldl(init, f)` | Fold izquierdo (reduce) | O(n) |
| `foldr(init, f)` | Fold derecho | O(n) |
| `all(p)` | Verificar si todos cumplen | O(n) |
| `exists(p)` | Verificar si alguno cumple | O(n) |
| `find_ptr(p)` | Encontrar el primero que cumple | O(n) |
| `zip(other)` | Emparejar elementos de dos contenedores | O(n) |
| `take(n)` | Primeros n elementos | O(n) |
| `drop(n)` | Omitir los primeros n elementos | O(n) |
| `partition(p)` | Particionar por predicado | O(n) |

<a id="readme-es-ejemplos-funcional"></a>
### Ejemplos

```cpp
#include <tpl_dynList.H>

int main() {
    DynList<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // Transform: square each number
    auto squared = numbers.map([](int x) { return x * x; });
    // [1, 4, 9, 16, 25, 36, 49, 64, 81, 100]

    // Filter: keep only evens
    auto evens = numbers.filter([](int x) { return x % 2 == 0; });
    // [2, 4, 6, 8, 10]

    // Reduce: sum all elements
    int sum = numbers.foldl(0, [](int acc, int x) { return acc + x; });
    // 55

    // Check predicates
    bool all_positive = numbers.all([](int x) { return x > 0; });
    bool has_negative = numbers.exists([](int x) { return x < 0; });

    // Find element
    int* found = numbers.find_ptr([](int x) { return x > 5; });
    if (found)
        std::cout << "First > 5: " << *found << "\n";  // 6

    // Partition
    auto [small, large] = numbers.partition([](int x) { return x <= 5; });
    // small: [1,2,3,4,5], large: [6,7,8,9,10]

    // Zip two lists
    DynList<std::string> names = {"Alice", "Bob", "Charlie"};
    DynList<int> ages = {30, 25, 35};
    auto pairs = names.zip(ages);
    // [("Alice", 30), ("Bob", 25), ("Charlie", 35)]

    return 0;
}
```

### Integración con ranges de C++20

```cpp
#include <ah-ranges.H>
#include <ranges>

int main() {
    DynList<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // Use with std::views
    auto result = numbers
        | std::views::filter([](int x) { return x % 2 == 0; })
        | std::views::transform([](int x) { return x * 10; });

    for (int x : result)
        std::cout << x << " ";  // 20 40 60 80 100

    // Convert ranges to Aleph containers
    auto list = std::views::iota(1, 10) | to_dynlist;
    auto arr = std::views::iota(1, 10) | to_dynarray;

    return 0;
}
```

---
<a id="readme-es-tutorial"></a>
## Tutorial

### Ejemplo completo: construir una herramienta de análisis de redes sociales

Este tutorial demuestra múltiples características de Aleph-w trabajando juntas.

```cpp
#include <tpl_graph.H>
#include <tpl_dynSetHash.H>
#include <Dijkstra.H>
#include <tpl_components.H>
#include <ah-parallel.H>
#include <iostream>

// Define our social network types
struct Person {
    std::string name;
    int age;
};

struct Connection {
    int strength;  // 1-10 friendship strength
};

using SocialNode = Graph_Node<Person>;
using SocialArc = Graph_Arc<Connection>;
using SocialNetwork = List_Graph<SocialNode, SocialArc>;

// Find the "closest" friend (by path strength)
SocialNode* find_closest_friend(
    SocialNetwork& network,
    SocialNode* person,
    const std::string& target_name
) {
    // Find target node
    SocialNode* target = nullptr;
    network.for_each_node([&](auto* node) {
        if (node->get_info().name == target_name)
            target = node;
    });

    if (!target) return nullptr;

    Path<SocialNetwork> path(network);
    dijkstra_min_path(network, person, target, path);

    return target;
}

// Find communities (connected components)
DynList<DynList<std::string>> find_communities(SocialNetwork& network) {
    DynList<SocialNetwork> components;
    connected_components(network, components);

    DynList<DynList<std::string>> communities;

    for (auto& component : components) {
        DynList<std::string> community;
        component.for_each_node([&](auto* node) {
            community.append(node->get_info().name);
        });
        communities.append(std::move(community));
    }

    return communities;
}

// Calculate influence score (number of connections)
DynHashMap<std::string, int> calculate_influence(SocialNetwork& network) {
    DynHashMap<std::string, int> influence;

    network.for_each_node([&](auto* node) {
        int connections = 0;
        // Count edges connected to this node
        for (auto it = network.get_first_arc(node);
             it.has_curr(); it.next()) {
            connections++;
        }
        influence.insert(node->get_info().name, connections);
    });

    return influence;
}

int main() {
    SocialNetwork network;

    // Add people
    auto* alice = network.insert_node({"Alice", 30});
    auto* bob = network.insert_node({"Bob", 25});
    auto* charlie = network.insert_node({"Charlie", 35});
    auto* david = network.insert_node({"David", 28});
    auto* eve = network.insert_node({"Eve", 32});

    // Add connections with friendship strength
    network.insert_arc(alice, bob, {8});      // Close friends
    network.insert_arc(bob, charlie, {6});
    network.insert_arc(charlie, david, {9});
    network.insert_arc(alice, eve, {4});
    network.insert_arc(eve, david, {7});

    // Find communities
    auto communities = find_communities(network);
    std::cout << "Communities found: " << communities.size() << "\n";

    // Calculate influence
    auto influence = calculate_influence(network);
    std::cout << "\nInfluence scores:\n";
    influence.for_each([](const std::string& name, int score) {
        std::cout << "  " << name << ": " << score << "\n";
    });

    // Parallel analysis of connection strengths
    DynList<int> strengths;
    network.for_each_arc([&](auto* arc) {
        strengths.append(arc->get_info().strength);
    });

    int total_strength = pfold(strengths, 0,
        [](int a, int b) { return a + b; });

    std::cout << "\nAverage connection strength: "
              << (double)total_strength / strengths.size() << "\n";

    return 0;
}
```

---

<a id="readme-es-referencia-api"></a>
## Referencia de API

### Referencia rápida por categoría

#### Contenedores

| Header | Tipo | Descripción |
|--------|------|-------------|
| `tpl_dynList.H` | `DynList<T>` | Singly-linked list |
| `tpl_dynDlist.H` | `DynDlist<T>` | Doubly-linked list |
| `tpl_dynArray.H` | `DynArray<T>` | Dynamic array |
| `tpl_dynSetTree.H` | `DynSetTree<T, Tree>` | Tree-based set |
| `tpl_dynMapTree.H` | `DynMapTree<K, V, Tree>` | Tree-based map |
| `tpl_hash.H` | `DynHashSet<T>` | Hash set |
| `tpl_hash.H` | `DynHashMap<K, V>` | Hash map |
| `tpl_binHeap.H` | `BinHeap<T, Cmp>` | Binary heap |
| `tpl_arrayQueue.H` | `ArrayQueue<T>` | Queue (array-based) |
| `tpl_arrayStack.H` | `ArrayStack<T>` | Stack (array-based) |
| `tpl_fenwick_tree.H` | `Fenwick_Tree<T>` | Fenwick tree (BIT) |
| `tpl_fenwick_tree.H` | `Gen_Fenwick_Tree<T, Plus, Minus>` | Fenwick over abelian groups |
| `tpl_fenwick_tree.H` | `Range_Fenwick_Tree<T>` | Range update/query Fenwick |
| `tpl_sparse_table.H` | `Sparse_Table<T>` | Static range min in O(1) |
| `tpl_sparse_table.H` | `Gen_Sparse_Table<T, Op>` | Static range query (idempotent op) |
| `tpl_disjoint_sparse_table.H` | `Sum_Disjoint_Sparse_Table<T>` | Static range sum in O(1) |
| `tpl_disjoint_sparse_table.H` | `Product_Disjoint_Sparse_Table<T>` | Static range product in O(1) |
| `tpl_disjoint_sparse_table.H` | `Gen_Disjoint_Sparse_Table<T, Op>` | Static range query (associative op) |
| `tpl_segment_tree.H` | `Sum_Segment_Tree<T>` | Range sum with point updates |
| `tpl_segment_tree.H` | `Min_Segment_Tree<T>` | Range min with point updates |
| `tpl_segment_tree.H` | `Max_Segment_Tree<T>` | Range max with point updates |
| `tpl_segment_tree.H` | `Gen_Segment_Tree<T, Op>` | Range query (associative monoid) |
| `tpl_segment_tree.H` | `Lazy_Sum_Segment_Tree<T>` | Range add + range sum (lazy) |
| `tpl_segment_tree.H` | `Lazy_Min_Segment_Tree<T>` | Range add + range min (lazy) |
| `tpl_segment_tree.H` | `Lazy_Max_Segment_Tree<T>` | Range add + range max (lazy) |
| `tpl_segment_tree.H` | `Gen_Lazy_Segment_Tree<Policy>` | Lazy segment tree (custom policy) |
| `tpl_segment_tree.H` | `Segment_Tree_Beats<T>` | Ji Driver's segment tree (chmin/chmax) |
| `tpl_mo_algorithm.H` | `Gen_Mo_Algorithm<T, Policy>` | Mo's offline range queries (snake opt.) |
| `tpl_mo_algorithm.H` | `Distinct_Count_Mo<T>` | Count distinct elements in ranges |
| `tpl_mo_algorithm.H` | `Powerful_Array_Mo<T>` | Powerful array query (sum cnt²·x) |
| `tpl_mo_algorithm.H` | `Range_Mode_Mo<T>` | Range mode (most frequent element) |
| `tpl_interval_tree.H` | `DynIntervalTree<T>` | Interval tree (overlap/stabbing queries) |
| `tpl_interval_tree.H` | `Interval_Tree<T>` | Low-level interval treap (raw nodes) |
| `tpl_link_cut_tree.H` | `Gen_Link_Cut_Tree<T, Monoid, LazyTag>` | Link-Cut Tree (bosque dinámico + consultas de camino) |
| `tpl_link_cut_tree.H` | `Link_Cut_Tree` | Link-Cut Tree solo conectividad (valores int) |
| `al-vector.H` | `Vector<T, NumType>` | Sparse vector with domain indexing |
| `al-matrix.H` | `Matrix<Trow, Tcol, NumType>` | Sparse matrix with domain indexing |
| `al-domain.H` | `AlDomain<T>` | Domain for vector/matrix indices |

#### Tipos de árboles

| Selector | Header | Description |
|----------|--------|-------------|
| `Avl_Tree` | `tpl_avl.H` | AVL tree |
| `Avl_Tree_Rk` | `tpl_avlRk.H` | AVL with rank |
| `Rb_Tree` | `tpl_rb_tree.H` | Red-Black tree |
| `Rb_Tree_Rk` | `tpl_rbRk.H` | Red-Black with rank |
| `Splay_Tree` | `tpl_splay_tree.H` | Splay tree |
| `Splay_Tree_Rk` | `tpl_splay_treeRk.H` | Splay with rank |
| `Treap` | `tpl_treap.H` | Treap |
| `Treap_Rk` | `tpl_treapRk.H` | Treap with rank |
| `Rand_Tree` | `tpl_rand_tree.H` | Randomized search tree |
| `Rand_Tree_Rk` | `tpl_rand_tree.H` | Randomized tree with rank |

#### Algoritmos de cadenas

| Header | Functions / Classes | Description |
|--------|---------------------|-------------|
| `String_Search.H` | `kmp_search()`, `z_search()`, `boyer_moore_horspool_search()`, `rabin_karp_search()` | Pattern matching algorithms |
| `Aho_Corasick.H` | `Aho_Corasick` | Multi-pattern search |
| `Suffix_Structures.H` | `suffix_array()`, `lcp_array_kasai()`, `Naive_Suffix_Tree`, `Suffix_Automaton` | Text indexing structures |
| `String_Palindromes.H` | `manacher()`, `longest_palindromic_substring()` | Palindrome detection in O(n) |
| `String_DP.H` | `levenshtein_distance()`, `damerau_levenshtein_distance()`, `longest_common_subsequence()`, `longest_common_substring()` | Sequence similarity and edit distance |
| `String_Algorithms.H` | *(all above)* | Umbrella include for string classical toolkit |

#### Programación dinámica

Por favor, consulta la sección canónica de [Algoritmos de programación dinámica](#readme-es-programacion-dinamica) para una lista completa de headers, funciones y ejemplos.

#### Combinatoria y enumeración

| Header | Functions / Classes | Description |
|--------|---------------------|-------------|
| `ah-comb.H` | `next_permutation()`, `next_combination_indices()`, `next_combination_mask()`, `for_each_combination()`, `build_combinations()`, `combination_count()`, `bin_to_gray()`, `gray_to_bin()`, `build_gray_code()` | Lexicographic permutation, k-combination enumeration, and Gray code utilities |

#### Matemática y teoría de números

| Header | Functions / Classes | Description |
|--------|---------------------|-------------|
| `fft.H` | `FFT<Real>` | Transformada rápida de Fourier (FFT): Cooley-Tukey radix-2/4 y Bluestein para tamaños arbitrarios, STFT/ISTFT, diseño de filtros FIR/IIR (Butterworth, Chebyshev, Bessel, Elíptico), PSD de Welch, resampleo, raíces de polinomios, transformadas N-D y APIs paralelas con `ThreadPool`. Ver el [Tutorial FFT y DSP](docs/fft-tutorial.md) |
| `modular_arithmetic.H` | `mod_mul()`, `mod_exp()`, `ext_gcd()`, `mod_inv()`, `crt()` | Safe 64-bit modular arithmetic, extended GCD, modular inverse, and Chinese Remainder Theorem |
| `primality.H` | `miller_rabin()` | Deterministic 64-bit Miller-Rabin primality testing |
| `pollard_rho.H` | `pollard_rho()` | Integer factorization using Pollard's rho with random fallback |
| `ntt.H` | `NTT`, `NTTExact` | Number Theoretic Transform para multiplicacion exacta de polinomios modulares, con planes reutilizables, soporte Bluestein para tamanos no potencia de dos cuando el modulo lo permite, dispatch SIMD AVX2/NEON en tiempo de ejecucion sobre el butterfly potencia-de-dos, lotes, butterflies basados en Montgomery, APIs paralelas con `ThreadPool`, algebra formal de polinomios (`poly_inverse`, `poly_divmod`, `poly_log`, `poly_exp`, `poly_sqrt`, `poly_power`, evaluacion multipunto e interpolacion), multiplicacion exacta de enteros grandes sobre digitos en base `B`, convolucion negaciclica modulo `x^N + 1` y reconstruccion CRT de tres primos en `__uint128_t` cuando la cota de coeficientes cabe. Ver el [Tutorial NTT](docs/ntt-tutorial.md) |
| `tpl_polynomial.H` | `Gen_Polynomial<C>`, `Polynomial` | Anillo de polinomios univariados dispersos: aritmética (+, -, *, /, %), cálculo (derivada, integral), evaluación (Horner/dispersa adaptativa), composición, GCD/XGCD/LCM, factorización libre de cuadrados, conteo de raíces (Sturm), búsqueda de raíces (bisección/Newton), interpolación polinómica (diferencias divididas de Newton) |
| `modular_combinatorics.H` | `ModularCombinatorics` | $nCk \pmod p$ with precomputed factorials and Lucas Theorem |
| `modular_linalg.H` | `ModularMatrix` | Gaussian elimination, determinant, and inverse modulo a prime |

#### Algoritmos de grafos

| Header | Function | Description |
|--------|----------|-------------|
| `Dijkstra.H` | `dijkstra_min_path()` | Single-source shortest path |
| `Bellman_Ford.H` | `bellman_ford_min_path()` | Shortest path (negative edges) |
| `Floyd_Warshall.H` | `floyd_all_shortest_paths()` | All-pairs shortest paths |
| `Johnson.H` | `johnson_all_pairs()` | All-pairs (sparse graphs) |
| `AStar.H` | `astar_search()` | Heuristic pathfinding |
| `K_Shortest_Paths.H` | `yen_k_shortest_paths()`, `eppstein_k_shortest_paths()` | K shortest alternatives (loopless/simple and general) between source and target |
| `Kruskal.H` | `kruskal_min_spanning_tree()` | MST (edge-based) |
| `Prim.H` | `prim_min_spanning_tree()` | MST (vertex-based) |
| `Blossom.H` | `blossom_maximum_cardinality_matching()` | Maximum matching (general graph) |
| `Blossom_Weighted.H` | `blossom_maximum_weight_matching()` | Maximum-weight matching (general graph; List/SGraph/Array backends) |
| `Min_Cost_Matching.H` | `blossom_minimum_cost_matching()`, `blossom_minimum_cost_perfect_matching()` | Minimum-cost matching in general graphs (including perfect-matching feasibility/cost) |
| `Hungarian.H` | `hungarian_assignment()`, `hungarian_max_assignment()` | Assignment problem (min-cost / max-profit) |
| `LCA.H` | `Gen_Binary_Lifting_LCA`, `Gen_Euler_RMQ_LCA` | Lowest common ancestor on rooted trees (binary lifting / Euler+RMQ) |
| `Tree_Decomposition.H` | `Gen_Heavy_Light_Decomposition`, `Gen_HLD_Path_Query` | Heavy-Light decomposition + path/subtree dynamic queries |
| `Tree_Decomposition.H` | `Gen_Centroid_Decomposition` | Centroid decomposition with centroid-ancestor distance chains |
| `HLD.H` | `Gen_HLD`, `HLD_Sum`, `HLD_Max`, `HLD_Min` | Self-contained HLD with typed convenience wrappers + edge-weighted path queries |
| `tpl_maxflow.H` | `*_maximum_flow()` | Maximum flow algorithms |
| `tpl_mincost.H` | `min_cost_max_flow()` | Min-cost max-flow |
| `Tarjan.H` | `tarjan_scc()` | Strongly connected components |
| `tpl_components.H` | `connected_components()` | Connected components |
| `tpl_cut_nodes.H` | `compute_cut_nodes()`, `Compute_Cut_Nodes` | Articulation points and biconnected components |
| `tpl_cut_nodes.H` | `find_bridges()`, `Compute_Bridges` | Bridge edges (cut edges) — Tarjan O(V+E) |
| `Planarity_Test.H` | `planarity_test()`, `planar_dual_metadata()`, `planar_geometric_drawing()`, `validate_nonplanar_certificate()`, `nonplanar_certificate_to_json()`, `nonplanar_certificate_to_dot()`, `nonplanar_certificate_to_graphml()`, `nonplanar_certificate_to_gexf()` | LR planarity test + embedding/dual metadata + geometric drawing + validated non-planar witness export |
| `topological_sort.H` | `topological_sort()` | DAG ordering |
| `Karger.H` | `karger_min_cut()` | Probabilistic min-cut |
| `Dominators.H` | `compute_dominators()`, `build_dominator_tree()`, `compute_dominance_frontiers()`, `compute_post_dominators()`, `build_post_dominator_tree()`, `compute_post_dominance_frontiers()` | Dominator & post-dominator trees (Lengauer-Tarjan) + dominance/post-dominance frontiers |
| `Stoer_Wagner.H` | `stoer_wagner_min_cut()` | Deterministic min-cut |

#### Gestión de memoria

| Header | Type/Function | Description |
|--------|---------------|-------------|
| `ah-arena.H` | `AhArenaAllocator` | Arena allocator |
| `ah-arena.H` | `allocate<T>()` | Construct object in arena |
| `ah-arena.H` | `dealloc<T>()` | Destroy object from arena |

#### Computación paralela

| Header | Type/Function | Description |
|--------|---------------|-------------|
| `thread_pool.H` | `ThreadPool` | Thread pool with futures |
| `concurrency_utils.H` | `bounded_channel<T>`, `synchronized<T>` | Canales modernos y wrappers para estado compartido |
| `ah-parallel.H` | `pmap()` | Parallel map |
| `ah-parallel.H` | `pfilter()` | Parallel filter |
| `ah-parallel.H` | `pfold()` | Parallel reduce |
| `ah-parallel.H` | `pfor_each()` | Parallel iteration |

#### Programación funcional

| Header | Functions | Description |
|--------|-----------|-------------|
| `ahFunctional.H` | `map`, `filter`, `fold`, ... | Core FP operations |
| `ah-ranges.H` | `to_dynlist`, `to_dynarray` | Range adaptors |
| `ah-zip.H` | `zip`, `unzip` | Tuple operations |

---

<a id="readme-es-benchmarks"></a>
## Benchmarks

### Rendimiento de árboles (1 millón de operaciones)

```
┌────────────────────────────────────────────────────────────────────────────┐
│                    INSERT PERFORMANCE (ns/operation)                       │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  AVL Tree       ████████████████████████ 156 ns                            │
│  Red-Black      ██████████████████████████ 178 ns                          │
│  std::set       ██████████████████████████ 183 ns                          │
│  Treap          ██████████████████████████████ 212 ns                      │
│  Splay Tree     ████████████████████████████████ 234 ns                    │
│                                                                            │
├────────────────────────────────────────────────────────────────────────────┤
│                    SEARCH PERFORMANCE (ns/operation)                       │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  Splay (warm)   ████████████████ 67 ns   (after repeated access)           │
│  AVL Tree       ████████████████████ 89 ns                                 │
│  std::set       ██████████████████████ 95 ns                               │
│  Red-Black      ██████████████████████ 98 ns                               │
│  Treap          ██████████████████████████ 112 ns                          │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

### Arena vs asignación tradicional

```
┌────────────────────────────────────────────────────────────────────────────┐
│                 ALLOCATION PERFORMANCE (1M allocations)                    │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  Arena          ██ 12 ns/alloc                                             │
│  malloc         ████████████████████████████████████████ 156 ns/alloc      │
│  new            ████████████████████████████████████████ 168 ns/alloc      │
│                                                                            │
│  Speedup: ~13x faster than malloc                                          │
│                                                                            │
├────────────────────────────────────────────────────────────────────────────┤
│                 TREE WITH ARENA (10K insertions)                           │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  With Arena     ████████████████ 1.2 ms                                    │
│  Without Arena  ████████████████████████████████████████ 3.8 ms            │
│                                                                            │
│  Speedup: 3.2x faster                                                      │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘

```

### Escalado de Dijkstra

```
┌────────────────────────────────────────────────────────────────────────────┐
│              DIJKSTRA PERFORMANCE vs GRAPH SIZE                            │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  V=1K, E=10K    ████ 2.3 ms                                                │
│  V=10K, E=100K  ████████████████████ 28 ms                                 │
│  V=100K, E=1M   ████████████████████████████████████████ 380 ms            │
│                                                                            │
│  Confirmed: O((V+E) log V) scaling                                         │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

### Comparación de algoritmos de flujo máximo

```
┌────────────────────────────────────────────────────────────────────────────┐
│            MAX FLOW PERFORMANCE (V=1000, E=5000)                           │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  Ford-Fulkerson ████████████████████████████████████████ 180 ms            │
│  Edmonds-Karp   ██████████████████████████ 89 ms                           │
│  Push-Relabel   ██████████████████ 45 ms                                   │
│  Dinic          ████████████████ 38 ms                                     │
│                                                                            │
│  Recommendation: Use Dinic for large graphs                                │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

### Aceleración de operaciones paralelas

```
┌────────────────────────────────────────────────────────────────────────────┐
│              pmap() SPEEDUP (8-core system)                                │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  1 thread       ████████████████████████████████████████ 100% (baseline)   │
│  2 threads      ████████████████████ 52% (1.9x speedup)                    │
│  4 threads      ██████████████ 28% (3.6x speedup)                          │
│  8 threads      ████████ 15% (6.7x speedup)                                │
│                                                                            │
│  Parallel efficiency: 84%                                                  │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

---

<a id="readme-es-ejemplos"></a>
## Ejemplos

El directorio `Examples/` contiene **80+ programas** que demuestran el uso de la biblioteca:

### Compilar ejemplos

```bash
cmake -S . -B build -DBUILD_EXAMPLES=ON
cmake --build build
```

### Ejemplos clave por categoría

| Ejemplo | Archivo | Descripción |
|---------|---------|-------------|
| **Estructuras de datos** | | |
| Listas dinámicas | `linear_structures_example.C` | Listas, colas, pilas |
| Tablas hash | `hash_tables_example.C` | Conjuntos y mapas |
| Árboles | `dynset_trees.C` | Todas las variantes de árboles |
| Montículos | `heap_example.C` | Colas de prioridad |
| Consultas de rango | `sparse_table_example.cc` | Sparse Table (RMQ) |
| Suma/producto en rango | `disjoint_sparse_table_example.cc` | Disjoint Sparse Table |
| Árboles de segmentos | `segment_tree_example.cc` | Actualizaciones puntuales/en rango, propagación lazy, Beats |
| Árbol de intervalos | `interval_tree_example.cc` | Scheduler de reuniones, intersección de segmentos 1D |
| Árbol cartesiano/LCA/RMQ | `cartesian_tree_example.cc` | Árbol cartesiano, LCA, reducciones RMQ |
| Descomposición Heavy-Light | `heavy_light_decomposition_example.cc` | Consultas de camino/subárbol con actualizaciones dinámicas puntuales |
| Descomposición por centroides | `centroid_decomposition_example.cc` | Consultas dinámicas de centro activo más cercano en árboles |
| Link-Cut Trees | `link_cut_tree_example.cc` | Conectividad dinámica, re-enraizamiento, LCA, agregados de camino, actualizaciones perezosas |
| Algoritmo de Mo | `mo_algorithm_example.cc` | Consultas offline de rango (distinct count, powerful array, mode) |
| Caja de herramientas de combinatoria | `comb_example.C` | Recorrido de producto cartesiano, transposición y auxiliares de combinatoria |
| Utilidades de código Gray | `gray_code_example.cc` | Conversión binario a Gray y generación de secuencias |
| Transformada rápida de Fourier | `fft_example.cc` | Análisis espectral de señales reales, convolución real/compleja secuencial optimizada, concurrencia explícita con `ThreadPool` y uso directo con contenedores iterables compatibles como `std::vector`. Tutorial completo: [Tutorial FFT y DSP](docs/fft-tutorial.md) |
| Number Theoretic Transform | `ntt_example.cc` | Transformadas modulares exactas, reporte del backend SIMD activo, planes reutilizables, tamanos arbitrarios soportados via Bluestein, multiplicacion exacta CRT de tres primos en `__uint128_t`, algebra formal de polinomios, multiplicacion de enteros grandes en base 10, convolucion negaciclica modulo `x^N + 1`, ejecucion por lotes y multiplicacion paralela explicita de polinomios. Tutorial completo: [Tutorial NTT](docs/ntt-tutorial.md) |
| Aritmética de polinomios | `polynomial_example.cc` | Álgebra de polinomios dispersos, cálculo simbólico, análisis de raíces (Sturm/bisección/Newton), interpolación de Newton, funciones de transferencia y operaciones dispersas de alto grado |
| Caja de herramientas de teoría de números | `math_nt_example.cc` | Multiplicación modular segura, Miller-Rabin, Pollard's Rho, NTT, combinatoria modular y álgebra lineal |
| Algoritmos de streaming | `streaming_demo.cc` | Reservoir Sampling, Count-Min Sketch, HyperLogLog, MinHash |
| Enumeración lexicográfica de permutaciones/combinaciones | `combinatorics_enumeration_example.cc` | `next_permutation` extendida, k-combinaciones por índices/bitmask y enumeración materializada |
| **Básicos de grafos** | | |
| BFS/DFS | `bfs_dfs_example.C` | Algoritmos de recorrido |
| Componentes | `graph_components_example.C` | Encontrar componentes |
| Grafos aleatorios | `random_graph_example.C` | Generación de grafos |
| **Caminos mínimos** | | |
| Dijkstra | `dijkstra_example.cc` | Fuente única |
| Bellman-Ford | `bellman_ford_example.cc` | Pesos negativos |
| Johnson | `johnson_example.cc` | Todos los pares sparse |
| A* | `astar_example.cc` | Búsqueda heurística |
| K caminos más cortos | `k_shortest_paths_example.cc` | Yen (sin ciclos) vs alternativas generales estilo Eppstein |
| **Algoritmos de cadenas** | | |
| KMP | `kmp_example.cc` | Búsqueda de un solo patrón en tiempo lineal basada en función prefijo |
| Z-algorithm | `z_algorithm_example.cc` | Array de similitud de prefijos y búsqueda de patrón |
| Boyer-Moore-Horspool | `horspool_example.cc` | Búsqueda práctica rápida con desplazamiento por mal carácter |
| Rabin-Karp | `rabin_karp_example.cc` | Búsqueda con rolling-hash con verificación exacta |
| Aho-Corasick | `aho_corasick_example.cc` | Matching con autómata multi-patrón |
| Suffix Array + LCP | `suffix_array_lcp_example.cc` | Orden lexicográfico de sufijos y LCP de Kasai |
| Suffix Tree | `suffix_tree_example.cc` | Búsquedas en árbol de sufijos comprimido naive |
| Suffix Automaton | `suffix_automaton_example.cc` | Autómata de substrings, conteo de substrings distintos, LCS |
| Manacher | `manacher_example.cc` | Substring palíndromo más largo en tiempo lineal |
| Levenshtein | `edit_distance_example.cc` | Distancia de edición vía programación dinámica |
| Damerau-Levenshtein | `damerau_levenshtein_example.cc` | Distancia de edición con transposiciones adyacentes |
| LCS / Longest Common Substring | `lcs_longest_common_substring_example.cc` | Extracción de subsecuencia y superposición contigua |
| **Programación dinámica** | (ver [Algoritmos de programación dinámica](#readme-es-programacion-dinamica)) | Toolkit completo y ejemplos de optimización |
| **Flujos en redes** | | |
| Flujo máximo | `network_flow_example.C` | Flujo máximo básico |
| Flujo de costo mínimo | `mincost_flow_example.cc` | Optimización de costo |
| Corte mínimo | `min_cut_example.cc` | Karger, Stoer-Wagner |
| **Algoritmos especiales** | | |
| MST | `mst_example.C` | Kruskal, Prim |
| Matching general | `blossom_example.cc` | Edmonds-Blossom + exportaciones TikZ |
| Matching general ponderado | `weighted_blossom_example.cc` | Blossom ponderado + comparación de objetivo + exportaciones TikZ |
| Matching general de costo mínimo | `min_cost_matching_example.cc` | API dedicada de costo mínimo + factibilidad de matching perfecto + comparación de backends |
| Asignación (Húngaro) | `hungarian_example.cc` | Asignación Húngara/Munkres de costo mínimo y de máxima ganancia |
| LCA en árboles | `lca_example.cc` | Binary lifting + Euler/RMQ LCA con paridad cross-backend (List/SGraph/Array) |
| Descomposición de árboles | `heavy_light_decomposition_example.cc`, `centroid_decomposition_example.cc` | Consultas Heavy-Light de camino/subárbol y consultas dinámicas de distancia a centroides |
| Conveniencia HLD | `hld_example.cc` | Consultas de camino HLD_Sum/Max/Min, consultas de subárbol, actualizaciones puntuales, consultas con pesos en aristas |
| Link-Cut Tree | `link_cut_tree_example.cc` | Bosque dinámico: link, cut, reroot, LCA, path sum/min/max, actualizaciones perezosas |
| Planaridad + certificados | `planarity_test_example.cc` | Planaridad LR, metadatos del dual, dibujo geométrico, exportación de certificados JSON/DOT/GraphML/GEXF + validación estructural |
| SCC | `tarjan_example.C` | Fuertemente conexas |
| Ordenamiento topológico | `topological_sort_example.C` | Ordenamiento de DAG |

| **Geometría** | | |
| Predicados robustos | `robust_predicates_example.cc` | Orientación, intersección, aritmética exacta |
| Intersección de segmentos dedicada | `segment_segment_intersection_example.cc` | Intersección de segmentos por pares en O(1) (`none`/`point`/`overlap`) |
| Algoritmos de geometría | `geom_example.C` | Convex hull, triangulación y `-s advanced` (Delaunay/Voronoi/PIP/HPI) |
| Celdas de Voronoi recortadas | `voronoi_clipped_cells_example.cc` | Celdas de Voronoi recortadas indexadas por sitio con exportación CSV/WKT |
| Delaunay + Voronoi | `delaunay_voronoi_example.cc` | Triangulación de Delaunay, dual de Voronoi, celdas recortadas |
| Point-in-polygon | `point_in_polygon_example.cc` | Winding-number inside/boundary/outside |
| Intersección de polígonos | `polygon_intersection_example.cc` | Intersección convexo-convexo (Sutherland-Hodgman) |
| Intersección de semiplanos | `halfplane_intersection_example.cc` | Región factible acotada para restricciones lineales 2D |
| Comparación de hull | `convex_hull_comparison_example.cc` | Comparar 5 algoritmos de hull con el mismo dataset |
| Par más cercano | `closest_pair_example.cc` | Divide-and-conquer del par más cercano + verificación |
| Rotating calipers | `rotating_calipers_example.cc` | Diámetro y ancho mínimo de un polígono convexo |
| Polígonos TikZ | `tikz_polygons_example.cc` | Visualización estilizada de polígonos en PGF/TikZ |
| Convex hull TikZ | `tikz_convex_hull_example.cc` | Nube de entrada + overlay del hull |
| Intersecciones TikZ | `tikz_intersection_example.cc` | Overlays de intersección convexa y booleana |
| Voronoi/power TikZ | `tikz_voronoi_power_example.cc` | Overlay Voronoi+Delaunay y power diagram |
| Algoritmos avanzados TikZ | `tikz_advanced_algorithms_example.cc` | Arreglo de segmentos (caras coloreadas), camino mínimo con funnel portals, descomposición convexa y alpha shape |
| Animación de funnel TikZ | `tikz_funnel_animation_example.cc` | Funnel multipágina (SSFA) con frames paso a paso para camino mínimo |
| Funnel beamer TikZ | `tikz_funnel_beamer_example.cc` | Una diapositiva Beamer con overlays SSFA vía `Tikz_Scene::draw_beamer_overlays()` |
| Funnel beamer TikZ (2-col) | `tikz_funnel_beamer_twocol_example.cc` | Overlays Beamer con frame visual izquierdo + panel de estado derecho (`apex/left/right/event`) |
| Funnel handout TikZ | `tikz_funnel_beamer_handout_example.cc` | Salida `beamer[handout]` con un frame imprimible por paso del funnel (layout 2-columnas) |
| Compositor de escenas TikZ | `tikz_scene_example.cc` | Composición `Tikz_Scene` de arreglo, hull y overlays de camino mínimo en una exportación |
| Escena beamer/handout TikZ | `tikz_scene_beamer_example.cc` | Exportaciones single-frame `Tikz_Scene::draw_beamer()` y `draw_handout()` |
| Overlays de escena TikZ | `tikz_scene_overlays_example.cc` | `Tikz_Scene::draw_beamer_overlays()` / `draw_handout_overlays()` para slides de polígonos paso a paso |
| **Paralelo** | | |
| Thread pool | `thread_pool_example.cc` | Tareas concurrentes |
| Operaciones paralelas | `ah_parallel_example.cc` | pmap, pfilter |
| **Memoria** | | |
| Arena | `map_arena_example.C` | Arena allocator |

---

<a id="readme-es-algoritmos-streaming"></a>
### Algoritmos probabilísticos de streaming

Aleph-w proporciona un conjunto de algoritmos modernos para procesar flujos de datos con baja huella de memoria:

| Header | Class | Purpose |
|---|---|---|
| `reservoir-sampling.H` | `Reservoir_Sampler` | Muestreo aleatorio de `k` elementos en un flujo de datos |
| `bloom-filter.H` | `Bloom_Filter` | Membresía probabilística estándar (sin eliminación) |
| `cuckoo-filter.H` | `Cuckoo_Filter` | Membresía de alto rendimiento con soporte de eliminación |
| `count-min-sketch.H` | `Count_Min_Sketch` | Estimación de frecuencia aproximada (O(1)) |
| `hyperloglog.H` | `HyperLogLog` | Estimación de cardinalidad (conteo de elementos únicos) con bajo error |
| `minhash.H` | `MinHash` | Estimación de similitud de Jaccard entre conjuntos |
| `simhash.H` | `SimHash` | Estimación de similitud de coseno usando huellas digitales de bits |

#### Ejemplo de uso de streaming

```cpp
#include <count-min-sketch.H>
#include <hyperloglog.H>

int main() {
    // Frequency estimation with 1% relative error and 99% confidence
    auto cms = Aleph::Count_Min_Sketch<std::string>::from_error_bounds(0.01, 0.01);
    cms.update("event_type_a");
    size_t freq = cms.estimate("event_type_a");

    // Cardinality estimation (unique elements)
    Aleph::HyperLogLog<int> hll(12); // 2^12 registers
    for (int i = 0; i < 1000000; ++i) hll.update(i % 1000);
    double unique = hll.estimate(); // ~1000.0
}
```

---

<a id="readme-es-testing"></a>
## Testing

### Ejecutar tests

```bash
# Build with tests (default)
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure

# Run specific test
./build/Tests/dynlist
./build/Tests/test_dijkstra
./build/Tests/latex_floyd_test

# Run exhaustive / performance-heavy example tests (marked with _test suffix)
# NOTE: ./build/Examples/sparse_table_test includes large performance benchmarks that
# can take a long time and use significant memory, especially on constrained machines.
# Consider running it only on suitable hardware if you need to evaluate its performance-related behavior.
#
./build/Examples/sparse_table_test
./build/Examples/disjoint_sparse_table_test
./build/Examples/min_cut_test

# Verbose output
ctest --test-dir build -V
```

### Ejecutar con sanitizers

```bash
# AddressSanitizer + UndefinedBehaviorSanitizer
cmake -S . -B build-asan \
    -DBUILD_TESTS=ON \
    -DUSE_SANITIZERS=ON \
    -DCMAKE_BUILD_TYPE=Debug

cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

### Cobertura de tests

- **7,700+ casos de test**
- **99.98% tasa de aprobación**
- Tests para todas las estructuras de datos y algoritmos
- Cobertura de casos borde
- Tests de regresión de rendimiento
- Algunos tests de larga duración/rendimiento se marcan intencionalmente como Disabled/Skipped y serán reportados por `ctest`.

---

<a id="readme-es-contribuir"></a>
## Contribuir

¡Las contribuciones son bienvenidas! Así es como puedes empezar:

### Configuración de desarrollo

```bash
git clone https://github.com/lrleon/Aleph-w.git
cd Aleph-w

# Build with debug symbols
cmake -S . -B build-debug \
    -DBUILD_TESTS=ON \
    -DCMAKE_BUILD_TYPE=Debug \
    -DUSE_SANITIZERS=ON

cmake --build build-debug
ctest --test-dir build-debug
```

### Guías de estilo de código

- **Indentación**: 2 espacios
- **Nombres**: `snake_case` para funciones, `PascalCase` para clases
- **Documentación**: comentarios Doxygen para todas las APIs públicas
- **Testing**: agregar tests para nuevas características

### Proceso de Pull Request

1. Fork del repositorio
2. Crear rama de feature: `git checkout -b feature/amazing-feature`
3. Hacer cambios con tests
4. Asegurar que CI pase: `ctest --output-on-failure`
5. Enviar Pull Request

---

## Reportes de bugs

Si encuentras un bug, ¡por favor repórtalo!

- **Email**: `leandro.r.leon@gmail.com`
- **GitHub Issues**: [Abrir un nuevo issue](https://github.com/lrleon/Aleph-w/issues)

Si es posible, por favor incluye un ejemplo mínimo reproducible.

---

## Únete a la comunidad

Imagina una biblioteca que crece con tus necesidades. **Aleph-w** está construida por desarrolladores, para desarrolladores.

Tu contribución importa. Ya sea arreglar un typo, optimizar un algoritmo o agregar una estructura de datos completamente nueva, tienes el poder de dar forma al futuro de este proyecto.

- **¿Tienes una idea?** Compártela en [Discussions](https://github.com/lrleon/Aleph-w/discussions).
- **¿Encontraste una mejor forma?** Envía un Pull Request.
- **¿Quieres aprender?** Explora el código y haz preguntas.

Juntos, podemos construir la biblioteca de algoritmos en C++ más completa del mundo. **¡Únete!**

---

<a id="readme-es-licencia"></a>
## Licencia

Aleph-w está licenciado bajo la **Licencia MIT**.

```text
Copyright (C) 2002-2026 Leandro Rabindranath León

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## Autores

- **Leandro Rabindranath León** - *Creador y mantenedor* - Universidad de Los Andes, Venezuela
- **Alejandro Mujica** - *Colaborador*

---

<a id="readme-es-agradecimientos"></a>
## Agradecimientos

- **Universidad de Los Andes** (Mérida, Venezuela) - Lugar de nacimiento de Aleph-w
- **Miles de estudiantes** que aprendieron algoritmos con esta biblioteca
- **Comunidad de código abierto** por retroalimentación continua
- **[SYMYL RESEARCH](https://simylresearch.com/en/)** por apoyar el desarrollo y avance de Aleph-w

---

<div align="center">

**[Volver arriba](#readme-es-tabla-de-contenido)**

Hecho con pasión por los algoritmos y las estructuras de datos

[GitHub](https://github.com/lrleon/Aleph-w) •
[Issues](https://github.com/lrleon/Aleph-w/issues) •
[Discussions](https://github.com/lrleon/Aleph-w/discussions)

</div>
