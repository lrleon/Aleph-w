# Aleph-w

<div align="center">

Language: English | [Español](README.es.md)

```
     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / /
   / ___ \| |  __/ |_) | | | |_____\ V  V /
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/
                 |_|
```

**A Comprehensive C++20 Library for Data Structures and Algorithms**

[![CI](https://github.com/lrleon/Aleph-w/actions/workflows/ci.yml/badge.svg)](https://github.com/lrleon/Aleph-w/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![GCC](https://img.shields.io/badge/GCC-11%2B-orange.svg)](https://gcc.gnu.org/)
[![Clang](https://img.shields.io/badge/Clang-14%2B-orange.svg)](https://clang.llvm.org/)
[![Tests](https://img.shields.io/badge/Tests-99.98%25-brightgreen.svg)](https://github.com/lrleon/Aleph-w/actions)
[![Documentation](https://img.shields.io/badge/docs-GitHub%20Pages-blue)](https://lrleon.github.io/Aleph-w/)

</div>

---

<a id="readme-table-of-contents"></a>
## Table of Contents

- [Overview](#readme-overview)
- [History and Philosophy](#readme-history-and-philosophy)
- [Features at a Glance](#readme-features-at-a-glance)
- [Requirements](#readme-requirements)
- [Installation](#readme-installation)
- [Quick Start](#readme-quick-start)
- [Data Structures](#readme-data-structures-main)
  - [Balanced Search Trees](#readme-balanced-search-trees)
  - [Hash Tables](#readme-hash-tables)
  - [Heaps and Priority Queues](#readme-heaps-and-priority-queues)
  - [Lists and Sequential Structures](#readme-lists-and-sequential-structures)
  - [Range Query Structures](#readme-range-query-structures)
  - [Graphs](#readme-graphs)
  - [Computational Geometry](#readme-computational-geometry)
  - [Linear Algebra (Sparse Structures)](#readme-linear-algebra-sparse-structures)
- [Algorithms](#readme-algorithms-main)
  - [Shortest Path Algorithms](#readme-shortest-path-algorithms)
  - [Minimum Spanning Trees](#readme-minimum-spanning-trees)
  - [Network Flows](#readme-network-flows)
  - [Graph Connectivity](#readme-graph-connectivity)
  - [Matching](#readme-matching)
  - [String Algorithms](#readme-string-algorithms)
  - [Sorting Algorithms](#readme-sorting-algorithms)
  - [Dynamic Programming Algorithms](#readme-dp-algorithms)
- [Memory Management](#readme-memory-management)
  - [Arena Allocators](#readme-arena-allocators)
- [Parallel Computing](#readme-parallel-computing)
- [Functional Programming](#readme-functional-programming)
- [Tutorial](#readme-tutorial)
- [API Reference](#readme-api-reference)
- [Benchmarks](#readme-benchmarks)
- [Examples](#readme-examples)
- [Testing](#readme-testing)
- [Contributing](#readme-contributing)
- [License](#readme-license)
- [Acknowledgments](#readme-acknowledgments)

---

<a id="readme-overview"></a>
## Overview

**Aleph-w** is a production-ready C++20 library providing over **90 data structures** and **50+ algorithms** for software engineers, researchers, and students. With **238 header files** and **80+ examples**, it is one of the most comprehensive algorithm libraries available.

### Why Aleph-w?

| Feature | STL | Boost | Aleph-w |
|---------|-----|-------|---------|
| Balanced Trees (AVL, RB, Splay, Treap) | Limited | Partial | **8 variants** |
| Order Statistics (k-th element, rank) | None | None | **All trees** |
| Graph Algorithms | None | BGL | **50+ algorithms** |
| Network Flows (max-flow, min-cost) | None | None | **Complete suite** |
| Arena Memory Allocators | None | None | **Built-in** |
| Parallel map/filter/fold | Limited | Limited | **Native support** |
| Functional Programming | Limited | Partial | **Comprehensive** |
| Educational Examples | None | Few | **80+ programs** |

### Key Statistics

- **238 header files** covering classic and modern algorithms
- **90+ data structures** with multiple implementation variants
- **50+ graph algorithms** including flows, cuts, and paths
- **80+ example programs** with detailed comments
- **7,700+ test cases** with 99.98% pass rate
- **Multi-compiler support**: GCC 11+ and Clang 14+

---

<a id="readme-history-and-philosophy"></a>
## History and Philosophy

### Origins (2002)

Aleph-w was born in 2002 at **Universidad de Los Andes** in Mérida, Venezuela, created by **Leandro Rabindranath León** as a teaching tool for algorithms and data structures courses.

The name "**Aleph**" (א) references Jorge Luis Borges' short story "[The Aleph](https://en.wikipedia.org/wiki/The_Aleph_(short_story))" — a point in space containing all other points. This symbolizes the library's goal: to be a single point containing all fundamental algorithms.

The "**-w**" suffix stands for "**with**" — emphasizing that this library comes *with* everything you need: implementations, examples, tests, and documentation.

### Design Philosophy

```
"The best way to learn algorithms is to implement them,
 and the best way to teach them is to show clean implementations."
                                        — Leandro R. León
```

**Core Principles:**

1. **Transparency over Abstraction**
   - Implementation details are explicit, not hidden
   - `DynList<T>` is singly-linked; `DynDlist<T>` is doubly-linked
   - You always know the complexity of every operation

2. **Completeness**
   - Multiple implementations of the same concept (AVL vs RB vs Splay)
   - Compare approaches and understand trade-offs
   - All classic algorithms, not just the "most common"

3. **Educational Value**
   - Code is written to be read and understood
   - Extensive comments explaining the "why"
   - Examples that teach, not just demonstrate

4. **Production Ready**
   - Extensively tested (7,700+ test cases)
   - Used in real-world applications
   - Performance competitive with specialized libraries

5. **Functional Style**
   - All containers support `map`, `filter`, `fold`
   - Composable operations
   - Immutability where practical

### Evolution Timeline

```
2002 ──────── Original C++ implementation for teaching at ULA
     │        Focus: Trees, lists, and basic graph algorithms
     │
2008 ──────── Major expansion of graph algorithms
     │        Added: Network flows, min-cost flows, cuts
     │
2012 ──────── Functional programming support
     │        Added: map, filter, fold for all containers
     │
2015 ──────── C++11/14 modernization
     │        Added: Move semantics, lambdas, auto
     │
2020 ──────── C++17 features and expanded testing
     │        Added: Structured bindings, if-init, GoogleTest
     │
2025 ──────── C++20 migration, arena allocators
     │        Added: Concepts, ranges, constexpr, arena memory
     │
2026 ──────── Full GCC/Clang compatibility (current)
             Fixed: 200+ warnings, template issues, ABI compatibility
             Added: CI/CD, sanitizers, comprehensive documentation
```

### Academic Impact

Aleph-w has been used to teach **thousands of students** across Latin America. Its educational design includes:

- **Side-by-side implementations**: Compare Kruskal vs Prim, Dijkstra vs Bellman-Ford
- **Visualization exports**: LaTeX/EEPIC output for papers and presentations
- **Complexity analysis**: Every operation documents its Big-O complexity
- **Extensive examples**: 80+ programs demonstrating real usage

---

<a id="readme-features-at-a-glance"></a>
## Features at a Glance

<a id="readme-data-structures"></a>
### Data Structures

```
┌────────────────────────────────────────────────────────────────────────────┐
│                         DATA STRUCTURES OVERVIEW                           │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  TREES                    HASH TABLES              HEAPS                   │
│  ├─ AVL Tree             ├─ Separate Chaining     ├─ Binary Heap           │
│  ├─ Red-Black Tree       ├─ Open Addressing       ├─ Fibonacci Heap        │
│  ├─ Splay Tree           ├─ Linear Probing        └─ Array Heap            │
│  ├─ Treap                └─ Linear Hashing                                 │
│  ├─ Skip List                                                              │
│  └─ All with Rank (Rk)                                                     │
│     variants for order                                                     │
│     statistics                                                             │
│                                                                            │
│  LISTS                    ARRAYS                   GRAPHS                  │
│  ├─ Singly-linked        ├─ Dynamic Array         ├─ Adjacency List        │
│  ├─ Doubly-linked        ├─ Fixed Array           ├─ Adjacency Matrix      │
│  ├─ Circular             ├─ 2D Matrix             ├─ Network (flows)       │
│  └─ Skip List            └─ BitArray              └─ Euclidean             │
│                                                                            │
│  SPECIAL                  SPATIAL                  PROBABILISTIC           │
│  ├─ Union-Find           ├─ Quadtree              ├─ Bloom Filter          │
│  ├─ LRU Cache            ├─ 2D-Tree               └─ Skip List             │
│  └─ Prefix Tree (Trie)   └─ K-d Tree                                       │
│                                                                            │
│  RANGE QUERIES                                                             │
│  ├─ Fenwick Tree (BIT)                                                     │
│  ├─ Sparse Table (RMQ)                                                     │
│  ├─ Disjoint Sparse Table                                                  │
│  ├─ Segment Tree / Lazy / Beats                                            │
│  ├─ Cartesian Tree RMQ (via LCA)                                           │
│  ├─ Heavy-Light Decomposition (path/subtree)                               │
│  ├─ Centroid Decomposition (distance tricks)                               │
│  └─ Mo's Algorithm (Offline)                                               │
│                                                                            │
│  LINEAR ALGEBRA                                                            │
│  ├─ Sparse Vector                                                          │
│  ├─ Sparse Matrix                                                          │
│  └─ Domain Indexing                                                        │
│                                                                            │
│  GEOMETRY                                                                  │
│  ├─ Primitives (Point, Segment, Polygon, Ellipse)                          │
│  ├─ Exact Predicates (orientation, intersection, in_circle)                │
│  ├─ Convex Hull (Andrew, Graham, QuickHull)                                │
│  ├─ Triangulation (Ear-Cutting, Monotone, Delaunay, CDT)                   │
│  ├─ Proximity (Closest Pair, MEC, Rotating Calipers)                       │
│  ├─ Diagrams (Voronoi, Power Diagram)                                      │
│  ├─ Intersections (Bentley-Ottmann, Sutherland-Hodgman)                    │
│  ├─ Simplification (Douglas-Peucker, Visvalingam-Whyatt)                   │
│  └─ Visualization (TikZ/PGF backend, algorithm renderers)                  │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

<a id="readme-algorithms"></a>
### Algorithms

```
┌────────────────────────────────────────────────────────────────────────────┐
│                           ALGORITHMS OVERVIEW                              │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  SHORTEST PATHS           SPANNING TREES           CONNECTIVITY            │
│  ├─ Dijkstra             ├─ Kruskal               ├─ DFS / BFS             │
│  ├─ Bellman-Ford         └─ Prim                  ├─ Connected Components  │
│  ├─ Floyd-Warshall                                ├─ Tarjan (SCC)          │
│  ├─ Johnson                                       ├─ Kosaraju (SCC)        │
│  └─ A* Search                                     ├─ Cut Nodes/Bridges     │
│                                                   └─ Biconnected           │
│                                                                            │
│  NETWORK FLOWS            MINIMUM CUT              SPECIAL                 │
│  ├─ Ford-Fulkerson       ├─ Karger                ├─ Topological Sort      │
│  ├─ Edmonds-Karp         ├─ Karger-Stein          ├─ Eulerian Path         │
│  ├─ Push-Relabel         └─ Stoer-Wagner          ├─ Hamiltonian Path      │
│  ├─ Dinic                                         └─ Cycle Detection       │
│  ├─ Min-Cost Max-Flow                                                      │
│  └─ Multicommodity                                                         │
│                                                                            │
│  SORTING                  SEARCHING                MATCHING                │
│  ├─ Quicksort            ├─ Binary Search         ├─ Hopcroft-Karp         │
│  ├─ Mergesort            ├─ Interpolation         ├─ Edmonds-Blossom       │
│  ├─ Heapsort             └─ Pattern Matching (KMP/Z/Aho)                   │
│  ├─ Introsort                                     └─ Hungarian (Munkres)   │
│  └─ Shell Sort                                                             │
│                                                                            │
│  GEOMETRY                 OTHER                                            │
│  ├─ Convex Hull          ├─ Union-Find                                     │
│  ├─ Triangulation        ├─ Huffman Coding                                 │
│  ├─ Voronoi / Delaunay   ├─ Simplex (LP)                                   │
│  └─ Intersections        └─ RMQ/LCA/HLD/Centroid decompositions            │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

---

<a id="readme-requirements"></a>
## Requirements

### Compiler Support

| Compiler | Minimum Version | Recommended | Status |
|----------|-----------------|-------------|--------|
| GCC | 11 | 13 | Fully tested |
| Clang | 14 | 16 | Fully tested |
| MSVC | Not supported | - | Planned |

### Dependencies

This branch builds a static library (`libAleph.a`) plus headers.

| Library | Package (Ubuntu/Debian) | Purpose | Required |
|---------|--------------------------|---------|----------|
| GMP + gmpxx | `libgmp-dev` | Multi-precision integers/rationals (and C++ bindings) | Yes |
| MPFR | `libmpfr-dev` | Multi-precision floating point | Yes |
| GSL (+ gslcblas) | `libgsl-dev` | RNG/statistics used by randomized structures/algorithms | Yes |
| X11 | `libx11-dev` | Link dependency | Yes |
| GoogleTest | `libgtest-dev` | Unit tests | Only for `BUILD_TESTS` (auto-fetch supported) |

### Build Tools

- **CMake** 3.18+
- **CMake presets**: `CMakePresets.json` requires CMake 3.21+ (optional)
- **Ninja** (recommended) or **Make**

### Quick Install (Ubuntu/Debian)

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

<a id="readme-installation"></a>
## Installation

For detailed, platform-specific instructions and troubleshooting, see `INSTALL.md`.

### Method 1: CMake Build (Recommended)

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

Note: if GoogleTest is not installed and you are offline, configure with `-DALEPH_FETCH_GTEST=OFF` (or disable tests with `-DBUILD_TESTS=OFF`) and/or install `libgtest-dev` from your distro.

### Method 2: Direct Compilation (No Install)

If you prefer not to install, you can build Aleph once and then link directly:

```bash
# Build the library
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF
cmake --build build --parallel

# Compile and link against the static library
g++ -std=c++20 -I/path/to/Aleph-w your_code.cpp /path/to/Aleph-w/build/libAleph.a \
  -lgmpxx -lgmp -lmpfr -lgsl -lgslcblas -lpthread -lX11 -lm \
  -o your_program
```

### Method 3: CMake FetchContent

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

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `ALEPH_CXX_STANDARD` | 20 | C++ standard for the core library (`17`, `20`, `23`) |
| `BUILD_TESTS` | ON | Build the test suite (`Tests/`) |
| `BUILD_EXAMPLES` | ON | Build the example programs (`Examples/`) |
| `BUILD_OPTIMIZED` | OFF | If ON and `CMAKE_BUILD_TYPE` is unset, default to `Release` |
| `ALEPH_FETCH_GTEST` | ON | (Tests) Auto-fetch GoogleTest if missing |
| `USE_SANITIZERS` | OFF | (Tests) Enable ASan/UBSan for test binaries |

### CMake Presets (Optional)

If you have CMake 3.21+, you can use the provided presets:

```bash
cmake --preset default
cmake --build --preset default
ctest --preset default
```

---

<a id="readme-quick-start"></a>
## Quick Start

### 5-Minute Examples

#### Example 1: Balanced Tree Set

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

#### Example 2: Graph with Shortest Path

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

#### Example 3: Arena Allocation

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

<a id="readme-data-structures-main"></a>
## Data Structures

<a id="readme-balanced-search-trees"></a>
### Balanced Search Trees

Aleph-w provides **8 different balanced tree implementations**, each optimized for specific use cases.

#### Tree Comparison

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

#### Usage Examples

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

#### Order Statistics with Rank Trees

All tree types have `*Rk` variants that support order statistics in O(log n):

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

#### Tree Structure Diagram

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

### Fenwick Trees (Binary Indexed Trees)

Aleph-w provides **three Fenwick tree variants** for efficient prefix sums and range queries:

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

#### Usage Examples

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

#### Real-World Application: Order Book Depth

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

Aleph-w provides **three segment tree variants** for dynamic range queries:

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

**Range Query Structure Comparison:**

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

#### Usage Examples

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

#### Real-World Application: Salary Adjustments (Lazy Propagation)

```cpp
// HR system: 8 departments, range salary raises
Lazy_Sum_Segment_Tree<int> payroll = {50, 45, 60, 55, 70, 48, 52, 65};
payroll.update(2, 5, 10);          // departments 2-5 get +$10K raise
int cost = payroll.query(0, 7);    // total payroll after raise
```

### Cartesian Trees & LCA

Aleph-w implements the classic chain of reductions **RMQ ↔ LCA ↔ Cartesian Tree**, confirming that Range Minimum Queries and Lowest Common Ancestor are equivalent problems:

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

- **`Gen_Cartesian_Tree<T, Comp>`** — Explicit Cartesian Tree built in O(n) with a monotonic stack.  Satisfies heap property under `Comp` and inorder = original array.
- **`Gen_Euler_Tour_LCA<T, Comp>`** — O(1) Lowest Common Ancestor via Euler Tour + Sparse Table on the Cartesian Tree.
- **`Gen_Cartesian_Tree_RMQ<T, Comp>`** — O(1) static range queries via the reduction RMQ → LCA → Cartesian Tree.

#### Usage Examples

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

### Tree LCA on Graph Backends

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

### Tree Decompositions (Heavy-Light + Centroid)

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

#### Ready-to-use Recipes

Recipe 1: Path sum with node point updates
When to use:
Many online queries like “sum on path(u, v)” and occasional node value changes.

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

Recipe 2: Subtree aggregate (full service area)
When to use:
Rooted-hierarchy totals where each update touches a single node.

```cpp
#include <Tree_Decomposition.H>
#include <tpl_graph.H>

using G = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

HLD_Path_Query<G, int, Aleph::plus<int>> q(g, ceo, 0);

int area_cost = q.query_subtree(region_head); // sum over full rooted subtree
q.update_node(team_node, -5);                 // adjust one team budget
int updated = q.query_subtree(region_head);
```

Recipe 3: Nearest active node (dynamic centers)
When to use:
Activate/mark nodes over time and repeatedly query nearest active node distance.

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

### Mo's Algorithm (Offline Range Queries)

Mo's algorithm answers Q offline range queries on a static array of N elements in O((N+Q)√N) time.  It is useful when the query function is "decomposable" (maintainable via add/remove of single elements) but has no algebraic structure (not a monoid, not invertible, not idempotent).

The implementation uses a **snake-optimized sweep** (alternating direction of the right pointer in even/odd blocks) for better cache behaviour.  The algorithm is parameterised by a **policy** (concept `MoPolicy`), so users can plug in arbitrary add/remove logic.

#### Built-in Policies

| Typedef | Policy | Description |
|---------|--------|-------------|
| `Distinct_Count_Mo<T>` | `Distinct_Count_Policy<T>` | Count distinct elements in a range |
| `Powerful_Array_Mo<T>` | `Powerful_Array_Policy<T>` | sum(cnt[x]² · x) for competitive programming |
| `Range_Mode_Mo<T>` | `Range_Mode_Policy<T>` | Most frequent element (frequency, value) |

#### Usage Examples

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

#### Custom Policies

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

<a id="readme-computational-geometry"></a>
### Computational Geometry

Aleph-w provides a robust suite for 2D and 3D computational geometry, built on **exact rational arithmetic** (`Geom_Number` = `mpq_class`) to prevent floating-point errors in geometric predicates.

**Key Features:**

- **Geometric Primitives (`point.H`, `polygon.H`)**: A comprehensive set of classes for points (2D/3D), segments, polygons (simple and regular), triangles, rectangles, and ellipses (axis-aligned and rotated).
- **Exact Predicates**: Core functions like `orientation()`, `segments_intersect()`, and `in_circle()` are exact, ensuring robust and correct behavior for higher-level algorithms.

**Algorithms (`geom_algorithms.H`):**

| Category | Algorithms | Complexity |
|---|---|---|
| **Convex Hull** | Andrew's Monotonic Chain, Graham Scan, QuickHull, Gift Wrapping (Jarvis March), Brute-Force | O(n log n) / O(nh) |
| **Triangulation** | Ear-Cutting, Monotone Polygon, Delaunay (Bowyer-Watson, Randomized Inc.), Constrained Delaunay (CDT) | O(n²), O(n log n) |
| **Proximity** | Closest Pair (Divide & Conquer), Minimum Enclosing Circle (Welzl), Rotating Calipers (Diameter/Width) | O(n log n), O(n) |
| **Diagrams** | Voronoi Diagram (from Delaunay), Power Diagram (Weighted Voronoi) | O(n log n) |
| **Intersections** | Segment Sweep (Bentley-Ottmann), Half-Plane Intersection, Convex Polygon Clipping, Boolean Polygon Ops (Greiner-Hormann) | O((n+k)log n), O(n log n) |
| **Simplification** | Douglas-Peucker, Visvalingam-Whyatt, Chaikin Smoothing | O(n log n), O(n*2^k) |
| **Pathfinding** | Shortest Path in Simple Polygon (Funnel Algorithm) | O(n) |
| **Spatial Indexing** | AABB Tree, KD-Tree | O(log n) queries |

**Visualization (`tikzgeom.H`, `eepicgeom.H`):**

- **TikZ Backend**: A modern, flexible backend (`Tikz_Plane`) for generating high-quality PGF/TikZ diagrams. Supports layers, styling, and native Bézier curves.
- **Algorithm Visualizers**: A suite of functions in `tikzgeom_algorithms.H` to render the output of algorithms like Voronoi diagrams, convex hulls, and arrangements.
- **Document Export**: The `Tikz_Scene` helper in `tikzgeom_scene.H` composes complex figures and exports them as standalone LaTeX documents, Beamer slides, or handouts, including multi-step animations using overlays.
- **Legacy EEPIC Backend**: For compatibility with older LaTeX workflows.

See `Examples/` for over a dozen geometry-specific programs, including `tikz_funnel_beamer_twocol_example.cc` which generates animated Beamer slides of the shortest-path funnel algorithm.

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

<a id="readme-linear-algebra-sparse-structures"></a>
### Linear Algebra (Sparse Structures)

Aleph-w provides **sparse vector and matrix classes** with domain-based indexing, optimized for data with many zeros:

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

#### Usage Examples

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

#### Real-World Application: Sales Analysis

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

<a id="readme-hash-tables"></a>
### Hash Tables

Aleph-w provides multiple hash table implementations optimized for different scenarios:

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

#### Basic Usage

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

#### Custom Hash Functions

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

<a id="readme-heaps-and-priority-queues"></a>
### Heaps and Priority Queues

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

<a id="readme-lists-and-sequential-structures"></a>
### Lists and Sequential Structures

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

<a id="readme-range-query-structures"></a>
### Range Query Structures

#### Sparse Table — O(1) Range Queries on Static Arrays

Sparse Table provides **O(1) range queries** after **O(n log n) preprocessing** for any idempotent operation (min, max, gcd, bitwise AND/OR).

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

#### Disjoint Sparse Table — O(1) Range Queries for Any Associative Op

Unlike the classical Sparse Table (which requires idempotency), the **Disjoint Sparse Table** works for **any associative** operation: sum, product, XOR, matrix multiplication, etc.

```cpp
#include <tpl_disjoint_sparse_table.H>

int main() {
    // Range Sum — NOT possible with classical Sparse Table!
    Sum_Disjoint_Sparse_Table<int> sum_tbl = {3, 1, 4, 1, 5, 9};
    int total = sum_tbl.query(0, 5);  // O(1) -> 23
    int sub   = sum_tbl.query(2, 4);  // O(1) -> 10
    
    // Range Product
    Product_Disjoint_Sparse_Table<long long> prod = {2, 3, 5, 7, 11};
    long long p = prod.query(0, 4);   // O(1) -> 2310
    
    // Custom: Range XOR
    struct Xor_Op {
        unsigned operator()(unsigned a, unsigned b) const { return a ^ b; }
    };
    Gen_Disjoint_Sparse_Table<unsigned, Xor_Op> xor_tbl = {0xA3, 0x5F, 0x12};
    unsigned x = xor_tbl.query(0, 2);  // O(1) -> 0xEE
    
    return 0;
}
```

#### Fenwick Tree — Dynamic Prefix Queries

For **dynamic** arrays with point updates, use Fenwick Tree (Binary Indexed Tree):

```cpp
#include <tpl_fenwick_tree.H>

Fenwick_Tree<int> ft(10);  // Size 10, all zeros
ft.update(3, 5);           // a[3] += 5
int sum = ft.prefix(7);    // sum a[0..7] = O(log n)
ft.update(2, -3);          // a[2] -= 3
```

#### Mo's Algorithm — Offline Range Queries

Mo's algorithm answers **Q offline range queries** on a static array of N elements in **O((N+Q)√N)** time. It's ideal when you have many queries and can process them all at once.

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

**Key advantages:**
- **Batch processing**: Answers many queries faster than individual O(log n) operations
- **Decomposable queries**: Any operation where you can add/remove elements
- **Policy-based design**: Same algorithm works for different query types

**Built-in policies:**
- `Distinct_Count_Policy<T>` — Count distinct elements in range
- `Powerful_Array_Policy<T>` — Sum of (count² × value)
- `Range_Mode_Policy<T>` — Most frequent element

##### Usage Examples

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

##### Mo's Algorithm on Trees

Aleph-w extends Mo's algorithm to **tree structures** for subtree and path queries:

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

**Real-world scenarios:**
- **Database analytics**: Count distinct categories in date ranges
- **Genomics**: Find most frequent k-mers in DNA segments  
- **Network monitoring**: Mode of latency classes in time windows
- **File systems**: Distinct file types in directory subtrees
- **Social networks**: Common interests between user paths

<a id="readme-graphs"></a>
### Graphs

Aleph-w provides multiple graph representations optimized for different use cases:

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

#### Creating and Using Graphs

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

#### Directed Graphs

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

<a id="readme-algorithms-main"></a>
## Algorithms

<a id="readme-shortest-path-algorithms"></a>
### Shortest Path Algorithms

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

`K_Shortest_Paths.H` provides two distinct modes:
- `yen_k_shortest_paths()`: loopless/simple alternatives.
- `eppstein_k_shortest_paths()`: general alternatives (may include cycles).

#### Dijkstra's Algorithm

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

#### Bellman-Ford (with negative edges)

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

#### A* Search (heuristic pathfinding)

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

#### K-Shortest Paths (Yen Loopless + Eppstein-Style General API)

Use this when one shortest path is not enough:
- route planning with `k` ranked alternatives from `s` to `t`
- resilient failover (if best route is blocked, move to next)
- near-optimal candidate generation for downstream constraints

`K_Shortest_Paths.H` supports both contracts:
- `yen_k_shortest_paths()`: up to `k` **simple** (cycle-free) paths.
- `eppstein_k_shortest_paths()`: up to `k` general paths, where cycles may appear.
- `Yen_K_Shortest_Paths<...>` and `Eppstein_K_Shortest_Paths<...>`: functor wrappers
  for reusable configured solvers (distance + arc filter).

Both are returned in non-decreasing total cost. Arc weights must be non-negative.
Implementation detail:
- Yen performs iterative spur-path recomputation.
- Eppstein-style precomputes one reverse shortest-path tree and expands sidetracks.

Why keep both APIs separated:
- Use Yen when path simplicity is a hard requirement (routing without repeated vertices).
- Use Eppstein-style when you need broader ranked alternatives and want faster reuse of
  the same source/target structure in dense deviation spaces.

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

#### Minimum Mean Cycle (Karp)

Use this when your system has repeated loops and you care about
the **best long-run average cost per step**, not just one shortest path.
Typical scenarios:
- performance/latency analysis of cyclic pipelines
- pricing or resource loops in networked systems
- steady-state optimization in automata/state machines

`Min_Mean_Cycle.H` provides:
- `karp_minimum_mean_cycle()`: Karp `O(VE)` minimum cycle mean on directed graphs.
- `minimum_mean_cycle()`: alias with the same behavior.
- `karp_minimum_mean_cycle_value()` / `minimum_mean_cycle_value()`: value-only
  variant (no witness extraction).
- `Karp_Minimum_Mean_Cycle<...>`: reusable functor wrapper.
- `Karp_Minimum_Mean_Cycle_Value<...>`: reusable value-only functor wrapper.

Both variants keep Karp complexity `O(VE)`. Value-only avoids witness metadata and
usually uses less memory in practice.

The result reports:
- `has_cycle`: whether at least one directed cycle exists
- `minimum_mean`: optimal cycle mean
- witness walk information (`cycle_nodes`, `cycle_arcs`, `cycle_total_cost`, `cycle_length`)

Witness semantics:
- `cycle_nodes` is a closed walk (first node repeated at the end).
- In tie-heavy graphs it may contain repeated internal vertices; it is a valid
  witness of the minimum mean value, not necessarily a canonical simple cycle.

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

<a id="readme-minimum-spanning-trees"></a>
### Minimum Spanning Trees

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

<a id="readme-network-flows"></a>
### Network Flows

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

#### Maximum Flow

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

#### Minimum Cost Maximum Flow

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

<a id="readme-graph-connectivity"></a>
### Graph Connectivity

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

<a id="readme-planarity"></a>
### Planarity Testing

Use this when your problem depends on whether a graph can be embedded in the plane
without edge crossings.

Typical scenarios:
- deciding if a topology is physically drawable without crossings
- validating graph constraints before planar-only algorithms
- extracting machine-checkable evidence for non-planarity
- obtaining a combinatorial embedding to reason about faces/rotations

`Planarity_Test.H` provides:
- `planarity_test()`: detailed result (`is_planar` + normalization metadata)
- `is_planar_graph()`: boolean convenience API
- `Planarity_Test<...>` and `Is_Planar_Graph<...>` wrappers

The test is performed on the underlying undirected simple graph:
- digraph orientation is ignored
- self-loops are ignored
- parallel arcs are collapsed

Advanced outputs are opt-in through `Planarity_Test_Options`:
- `compute_embedding`: combinatorial embedding (rotation system + faces)
  - primary mode: LR-first construction (`embedding_is_lr_linear = true`)
    with deterministic LR-local repair (reversal/swap candidates,
    multi-start coordinate descent)
  - optional fallback: bounded exact search if LR embedding fails validation
  - strict profile (`embedding_allow_bruteforce_fallback = false`) avoids
    exhaustive fallback and is typically faster (no exponential backtracking),
    but may return `is_planar = true` with no embedding
    (`has_combinatorial_embedding = false`, `embedding_search_truncated = true`)
  - `embedding_max_combinations` is the LR local-repair evaluation budget
    in strict mode (and the exact-search budget when fallback is used)
  - robust profile (recommended for face/dual workflows): keep fallback enabled
- `compute_nonplanar_certificate`: non-planarity witness
  - first reduces to a minimal obstruction by edge/vertex deletion passes
  - then searches Kuratowski patterns on the branch-core
  - returns `K5_Subdivision`, `K33_Subdivision`, or
    `Minimal_NonPlanar_Obstruction` if classification is not possible under limits
  - each witness edge keeps traceability to original input arcs
    (`representative_input_arc`, `input_arcs`)
  - each witness path includes both nodes and traceable edge sequence
    (`certificate_paths[i].nodes`, `certificate_paths[i].edges`)

Embedding guarantee model:
- planarity decision itself (`is_planar`) comes from the LR test and remains exact.
- strict LR embedding mode (`embedding_allow_bruteforce_fallback = false`) is a
  bounded constructive strategy; if it reaches budget, embedding can be missing
  (`has_combinatorial_embedding = false`, `embedding_search_truncated = true`)
  even for planar graphs.
- if embedding completeness is required, keep fallback enabled and provide enough
  `embedding_max_combinations` budget.

Face and dual APIs:
- `planar_dual_metadata(result)`:
  - face boundaries as dart walks
  - face adjacency
  - primal-edge ↔ dual-edge relation
- `build_planar_dual_graph(result)` / `build_planar_dual_graph(metadata)`:
  - builds an Aleph dual graph (`List_Graph` by default)
  - dual node info: face id
  - dual arc info: `Planar_Dual_Edge_Info<GT>`

Embedding-aware geometric drawing API:
- `planar_geometric_drawing(result, drawing_options)`:
  - computes 2D coordinates for embedding nodes
  - uses harmonic relaxation on top of the combinatorial embedding
  - can validate straight-edge crossings (`drawing_validated_no_crossings`)

Certificate export APIs:
- `nonplanar_certificate_to_json(result)`:
  - structured machine-readable witness (nodes, obstruction edges, paths)
- `nonplanar_certificate_to_dot(result)`:
  - GraphViz DOT with highlighted obstruction/path edges
- `nonplanar_certificate_to_graphml(result)`:
  - GraphML exchange format for graph tools/pipelines
- `nonplanar_certificate_to_gexf(result)`:
  - GEXF exchange format for Gephi-like tooling
- `validate_nonplanar_certificate(result)` / `nonplanar_certificate_is_valid(result)`:
  - structural consistency validation for exported witnesses

External validation adapter (end-to-end artifacts):
- `scripts/planarity_certificate_validator.rb`
  - validates exported `GraphML` / `GEXF` files directly
  - checks XML consistency, endpoint references and obstruction-edge presence
  - optional `--networkx` pass for tool-level loadability checks
  - optional `--gephi` pass for Gephi CLI/toolkit adapter checks
    (`--gephi-cmd` supports a custom command template with `{input}`)
  - optional `--render-gephi` pass for render/export validation
    (`--render-profile` + `--render-output-dir`, with SVG/PDF artifact checks)
- `scripts/planarity_gephi_templates.json`
  - catalog of Gephi command templates by OS/version (plus portable CI template)
- `scripts/planarity_gephi_render_profiles.json`
  - catalog of Gephi render/export profiles (SVG/PDF) by OS/version
    (plus portable deterministic CI profiles)
- `scripts/planarity_certificate_ci_batch.rb`
  - reproducible batch wrapper for CI that emits a deterministic JSON report
- `scripts/planarity_visual_golden_manifest.json`
  - deterministic golden digest baseline for portable render profiles
- `scripts/planarity_certificate_ci_visual_diff.rb`
  - dedicated visual-golden CI runner (render + SHA256 diff against manifest)
- `scripts/planarity_gephi_weekly_comparison.rb`
  - run-level aggregator for nightly artifacts + regression detection (`overall_valid` and exit-code deltas)
- `scripts/planarity_gephi_regression_notify.rb`
  - optional notifier for nightly regressions (Markdown alert + webhook dispatch)
- `scripts/fixtures/planarity_k33_certificate.graphml`
  - Aleph-generated non-planar certificate fixture used by CI/adapter probes
- `.github/workflows/planarity_gephi_nightly.yml`
  - weekly + manual real-Gephi packaging probe (Linux/macOS/Windows)
  - auto-selects latest `0.9.x` and `0.10.x` Gephi tags (or manual override)
  - downloads official Gephi release binaries and validates adapter integration
  - emits per-run comparative report artifact across tags/OS matrix
  - enforces a regression gate when newer tags regress against previous tags per OS
  - optional webhook notifications for regressions via secret
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

<a id="readme-matching"></a>
### Matching

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

#### General Graph Matching (Edmonds-Blossom)

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

#### General Graph Weighted Matching (Weighted Blossom)

`Blossom_Weighted.H` is backend-generic in Aleph. It is tested on:
- `List_Graph<Graph_Node<int>, Graph_Arc<long long>>`
- `List_SGraph<Graph_Snode<int>, Graph_Sarc<long long>>`
- `Array_Graph<Graph_Anode<int>, Graph_Aarc<long long>>`

See `Tests/weighted_blossom_test.cc` and `Examples/weighted_blossom_example.cc`.

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

#### General Graph Minimum-Cost Matching (Dedicated API)

`Min_Cost_Matching.H` provides a dedicated API for non-bipartite
minimum-cost matching over Aleph graph backends (`List_Graph`,
`List_SGraph`, `Array_Graph`).

It also provides a dedicated perfect-matching variant:
`blossom_minimum_cost_perfect_matching()`, which reports feasibility
and returns the minimum perfect-matching cost when feasible.

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

#### Hungarian Assignment (Munkres)

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

<a id="readme-string-algorithms"></a>
### String Algorithms

Aleph-w now includes a complete classical string toolkit split by purpose:

| Header | Scope |
|---|---|
| `String_Search.H` | KMP, Z-algorithm, Boyer-Moore-Horspool, Rabin-Karp |
| `Aho_Corasick.H` | Multi-pattern matching |
| `Suffix_Structures.H` | Suffix Array, LCP (Kasai), naive compressed Suffix Tree, Suffix Automaton |
| `String_Palindromes.H` | Manacher (longest palindromic substring in O(n)) |
| `String_DP.H` | Levenshtein, Damerau-Levenshtein, LCS, Longest Common Substring |
| `String_Algorithms.H` | Umbrella include for all string algorithms |

#### Covered Algorithms

- Single-pattern search: `kmp_search`, `z_search`, `boyer_moore_horspool_search`, `rabin_karp_search`
- Multi-pattern search: `Aho_Corasick`
- Suffix structures: `suffix_array`, `lcp_array_kasai`, `Naive_Suffix_Tree`, `Suffix_Automaton`
- Palindromes: `manacher`, `longest_palindromic_substring`
- Edit/similarity: `levenshtein_distance`, `damerau_levenshtein_distance`
- Sequence similarity: `longest_common_subsequence`, `longest_common_substring`

#### Usage Example

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

#### String Examples

Each algorithm has a focused example under `Examples/`:

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

<a id="readme-dp-algorithms"></a>
### Dynamic Programming Algorithms

Aleph-w also includes a practical dynamic-programming toolkit:

| Header | Scope |
|---|---|
| `Knapsack.H` | 0/1, unbounded, and bounded knapsack (with reconstruction) |
| `LIS.H` | LIS and LNDS in O(n log n), with sequence reconstruction |
| `Matrix_Chain.H` | Optimal matrix-chain parenthesization (interval DP) |
| `Subset_Sum.H` | Subset sum (existence/reconstruction/count) + MITM |
| `Tree_DP.H` | Generic tree DP + rerooting DP (all roots in O(n)) |
| `DP_Optimizations.H` | D&C DP, Knuth, Convex Hull Trick, Li Chao, monotone queue |

#### Covered DP APIs

- Knapsack: `knapsack_01`, `knapsack_01_value`, `knapsack_unbounded`, `knapsack_bounded`
- Subsequences: `longest_increasing_subsequence`, `lis_length`, `longest_nondecreasing_subsequence`
- Matrix chain: `matrix_chain_order`, `matrix_chain_min_cost`
- Subset sum: `subset_sum`, `subset_sum_exists`, `subset_sum_count`, `subset_sum_mitm`
- Trees: `Gen_Tree_DP`, `Gen_Reroot_DP`, `tree_subtree_sizes`, `tree_max_distance`, `tree_sum_of_distances`
- DP optimizations: `divide_and_conquer_partition_dp`, `knuth_optimize_interval`, `optimal_merge_knuth`, `Convex_Hull_Trick`, `Li_Chao_Tree`, `monotone_queue_min_dp`, `min_weighted_squared_distance_1d`

#### DP Usage Example

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

#### DP Examples

- `knapsack_example.cc`
- `lis_example.cc`
- `matrix_chain_example.cc`
- `subset_sum_example.cc`
- `tree_dp_example.cc`
- `dp_optimizations_example.cc`

<a id="readme-sorting-algorithms"></a>
### Sorting Algorithms

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

<a id="readme-memory-management"></a>
## Memory Management

<a id="readme-arena-allocators"></a>
### Arena Allocators

Arena allocation provides **ultra-fast memory management** for temporary data structures.

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

#### Basic Arena Usage

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

#### Trees with Arena Allocation

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

#### When to Use Arena Allocation

| Use Case | Traditional | Arena | Recommendation |
|----------|-------------|-------|----------------|
| Temporary computations | Many mallocs | Single buffer | **Arena** |
| Known lifetime | Track each object | Bulk free | **Arena** |
| Real-time systems | Unpredictable latency | O(1) alloc | **Arena** |
| Long-lived objects | Fine | Not suitable | **Traditional** |
| Objects with varying lifetimes | Fine | Difficult | **Traditional** |

---

<a id="readme-parallel-computing"></a>
## Parallel Computing

### Thread Pool

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

### Parallel Functional Operations

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

<a id="readme-functional-programming"></a>
## Functional Programming

All Aleph-w containers support a rich set of functional operations:

### Core Operations

| Operation | Description | Complexity |
|-----------|-------------|------------|
| `for_each(f)` | Apply f to each element | O(n) |
| `map(f)` | Transform elements | O(n) |
| `filter(p)` | Select elements matching predicate | O(n) |
| `foldl(init, f)` | Left fold (reduce) | O(n) |
| `foldr(init, f)` | Right fold | O(n) |
| `all(p)` | Check if all match | O(n) |
| `exists(p)` | Check if any matches | O(n) |
| `find_ptr(p)` | Find first matching element | O(n) |
| `zip(other)` | Pair elements from two containers | O(n) |
| `take(n)` | First n elements | O(n) |
| `drop(n)` | Skip first n elements | O(n) |
| `partition(p)` | Split by predicate | O(n) |

<a id="readme-examples"></a>
### Examples

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

### C++20 Ranges Integration

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

<a id="readme-tutorial"></a>
## Tutorial

### Complete Example: Building a Social Network Analysis Tool

This tutorial demonstrates multiple Aleph-w features working together.

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

<a id="readme-api-reference"></a>
## API Reference

### Quick Reference by Category

#### Containers

| Header | Type | Description |
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
| `al-vector.H` | `Vector<T, NumType>` | Sparse vector with domain indexing |
| `al-matrix.H` | `Matrix<Trow, Tcol, NumType>` | Sparse matrix with domain indexing |
| `al-domain.H` | `AlDomain<T>` | Domain for vector/matrix indices |

#### Tree Types

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

#### String Algorithms

| Header | Functions / Classes | Description |
|--------|---------------------|-------------|
| `String_Search.H` | `kmp_search()`, `z_search()`, `boyer_moore_horspool_search()`, `rabin_karp_search()` | Pattern matching algorithms |
| `Aho_Corasick.H` | `Aho_Corasick` | Multi-pattern search |
| `Suffix_Structures.H` | `suffix_array()`, `lcp_array_kasai()`, `Naive_Suffix_Tree`, `Suffix_Automaton` | Text indexing structures |
| `String_Palindromes.H` | `manacher()`, `longest_palindromic_substring()` | Palindrome detection in O(n) |
| `String_DP.H` | `levenshtein_distance()`, `damerau_levenshtein_distance()`, `longest_common_subsequence()`, `longest_common_substring()` | Sequence similarity and edit distance |
| `String_Algorithms.H` | *(all above)* | Umbrella include for string classical toolkit |

#### Dynamic Programming

| Header | Functions / Classes | Description |
|--------|---------------------|-------------|
| `Knapsack.H` | `knapsack_01()`, `knapsack_01_value()`, `knapsack_unbounded()`, `knapsack_bounded()` | Classical knapsack variants with reconstruction |
| `LIS.H` | `longest_increasing_subsequence()`, `lis_length()`, `longest_nondecreasing_subsequence()` | O(n log n) subsequence optimization |
| `Matrix_Chain.H` | `matrix_chain_order()`, `matrix_chain_min_cost()` | Optimal parenthesization for matrix products |
| `Subset_Sum.H` | `subset_sum()`, `subset_sum_exists()`, `subset_sum_count()`, `subset_sum_mitm()` | Exact subset-sum toolkit (DP + MITM) |
| `Tree_DP.H` | `Gen_Tree_DP`, `Gen_Reroot_DP`, `tree_subtree_sizes()`, `tree_max_distance()`, `tree_sum_of_distances()` | Generic tree DP and rerooting patterns |
| `DP_Optimizations.H` | `divide_and_conquer_partition_dp()`, `knuth_optimize_interval()`, `optimal_merge_knuth()`, `Convex_Hull_Trick`, `Li_Chao_Tree`, `monotone_queue_min_dp()`, `min_weighted_squared_distance_1d()` | High-impact DP optimizations + geometric lower-envelope application |

#### Graph Algorithms

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

#### Memory Management

| Header | Type/Function | Description |
|--------|---------------|-------------|
| `ah-arena.H` | `AhArenaAllocator` | Arena allocator |
| `ah-arena.H` | `allocate<T>()` | Construct object in arena |
| `ah-arena.H` | `dealloc<T>()` | Destroy object from arena |

#### Parallel Computing

| Header | Type/Function | Description |
|--------|---------------|-------------|
| `thread_pool.H` | `ThreadPool` | Thread pool with futures |
| `ah-parallel.H` | `pmap()` | Parallel map |
| `ah-parallel.H` | `pfilter()` | Parallel filter |
| `ah-parallel.H` | `pfold()` | Parallel reduce |
| `ah-parallel.H` | `pfor_each()` | Parallel iteration |

#### Functional Programming

| Header | Functions | Description |
|--------|-----------|-------------|
| `ahFunctional.H` | `map`, `filter`, `fold`, ... | Core FP operations |
| `ah-ranges.H` | `to_dynlist`, `to_dynarray` | Range adaptors |
| `ah-zip.H` | `zip`, `unzip` | Tuple operations |

---

<a id="readme-benchmarks"></a>
## Benchmarks

### Tree Performance (1 Million Operations)

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

### Arena vs Traditional Allocation

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

### Dijkstra Scaling

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

### Max Flow Algorithm Comparison

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

### Parallel Operations Speedup

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

## Examples

The `Examples/` directory contains **80+ programs** demonstrating library usage:

### Building Examples

```bash
cmake -S . -B build -DBUILD_EXAMPLES=ON
cmake --build build
```

### Key Examples by Category

| Example | File | Description |
|---------|------|-------------|
| **Data Structures** | | |
| Dynamic lists | `linear_structures_example.C` | Lists, queues, stacks |
| Hash tables | `hash_tables_example.C` | Sets and maps |
| Trees | `dynset_trees.C` | All tree variants |
| Heaps | `heap_example.C` | Priority queues |
| Range queries | `sparse_table_example.cc` | Sparse Table (RMQ) |
| Range sum/product | `disjoint_sparse_table_example.cc` | Disjoint Sparse Table |
| Segment trees | `segment_tree_example.cc` | Point/range updates, lazy propagation, Beats |
| Cartesian Tree/LCA/RMQ | `cartesian_tree_example.cc` | Cartesian Tree, LCA, RMQ reductions |
| Heavy-Light decomposition | `heavy_light_decomposition_example.cc` | Path/subtree queries with dynamic point updates |
| Centroid decomposition | `centroid_decomposition_example.cc` | Dynamic nearest active center queries on trees |
| Mo's algorithm | `mo_algorithm_example.cc` | Offline range queries (distinct count, powerful array, mode) |
| **Graph Basics** | | |
| BFS/DFS | `bfs_dfs_example.C` | Traversal algorithms |
| Components | `graph_components_example.C` | Finding components |
| Random graphs | `random_graph_example.C` | Graph generation |
| **Shortest Paths** | | |
| Dijkstra | `dijkstra_example.cc` | Single-source |
| Bellman-Ford | `bellman_ford_example.cc` | Negative weights |
| Johnson | `johnson_example.cc` | All-pairs sparse |
| A* | `astar_example.cc` | Heuristic search |
| K shortest paths | `k_shortest_paths_example.cc` | Yen (loopless) vs Eppstein-style general alternatives |
| **String Algorithms** | | |
| KMP | `kmp_example.cc` | Prefix-function based linear-time single-pattern search |
| Z-algorithm | `z_algorithm_example.cc` | Prefix-similarity array and pattern search |
| Boyer-Moore-Horspool | `horspool_example.cc` | Fast practical bad-character shift search |
| Rabin-Karp | `rabin_karp_example.cc` | Rolling-hash search with exact verification |
| Aho-Corasick | `aho_corasick_example.cc` | Multi-pattern automaton matching |
| Suffix Array + LCP | `suffix_array_lcp_example.cc` | Lexicographic suffix ordering and Kasai LCP |
| Suffix Tree | `suffix_tree_example.cc` | Naive compressed suffix tree lookups |
| Suffix Automaton | `suffix_automaton_example.cc` | Substring automaton, distinct substring count, LCS |
| Manacher | `manacher_example.cc` | Longest palindromic substring in linear time |
| Levenshtein | `edit_distance_example.cc` | Edit distance via dynamic programming |
| Damerau-Levenshtein | `damerau_levenshtein_example.cc` | Edit distance with adjacent transpositions |
| LCS / Longest Common Substring | `lcs_longest_common_substring_example.cc` | Subsequence and contiguous overlap extraction |
| **Dynamic Programming** | | |
| Knapsack (0/1, unbounded, bounded) | `knapsack_example.cc` | Packing optimization with reconstruction and variant comparison |
| LIS / LNDS | `lis_example.cc` | O(n log n) subsequence optimization with comparator variants |
| Matrix-chain multiplication | `matrix_chain_example.cc` | Parenthesization minimization for chained products |
| Subset sum (DP + MITM) | `subset_sum_example.cc` | Existence, counting, reconstruction, and MITM contrast |
| Tree DP / Rerooting | `tree_dp_example.cc` | Subtree metrics and all-root rerooting in linear time |
| DP optimizations (D&C / Knuth / CHT / Li Chao / monotone queue) | `dp_optimizations_example.cc` | Complexity-reduction patterns with a geometric lower-envelope use case |
| **Network Flows** | | |
| Max flow | `network_flow_example.C` | Basic max flow |
| Min-cost flow | `mincost_flow_example.cc` | Cost optimization |
| Min cut | `min_cut_example.cc` | Karger, Stoer-Wagner |
| **Special Algorithms** | | |
| MST | `mst_example.C` | Kruskal, Prim |
| General matching | `blossom_example.cc` | Edmonds-Blossom + TikZ exports |
| General weighted matching | `weighted_blossom_example.cc` | Weighted blossom + objective comparison + TikZ exports |
| General minimum-cost matching | `min_cost_matching_example.cc` | Dedicated minimum-cost API + perfect-matching feasibility + backend comparison |
| Assignment (Hungarian) | `hungarian_example.cc` | Hungarian/Munkres minimum-cost and maximum-profit assignment |
| Tree LCA | `lca_example.cc` | Binary lifting + Euler/RMQ LCA with cross-backend parity (List/SGraph/Array) |
| Tree Decomposition | `heavy_light_decomposition_example.cc`, `centroid_decomposition_example.cc` | Heavy-Light path/subtree queries and centroid-distance dynamic queries |
| HLD convenience | `hld_example.cc` | HLD_Sum/Max/Min path queries, subtree queries, point updates, edge-weighted queries |
| Planarity + certificates | `planarity_test_example.cc` | LR planarity, dual metadata, geometric drawing, JSON/DOT/GraphML/GEXF certificate export + structural validation |
| SCC | `tarjan_example.C` | Strongly connected |
| Topological | `topological_sort_example.C` | DAG ordering |
| **Geometry** | | |
| Robust predicates | `robust_predicates_example.cc` | Orientation, intersection, exact arithmetic |
| Dedicated segment intersection | `segment_segment_intersection_example.cc` | O(1) pairwise segment intersection (`none`/`point`/`overlap`) |
| Geometry algorithms | `geom_example.C` | Convex hull, triangulation, and `-s advanced` (Delaunay/Voronoi/PIP/HPI) |
| Voronoi clipped cells | `voronoi_clipped_cells_example.cc` | Site-indexed clipped Voronoi cells with CSV/WKT export |
| Delaunay + Voronoi | `delaunay_voronoi_example.cc` | Delaunay triangulation, Voronoi dual, clipped cells |
| Point-in-polygon | `point_in_polygon_example.cc` | Winding-number inside/boundary/outside |
| Polygon intersection | `polygon_intersection_example.cc` | Convex-convex intersection (Sutherland-Hodgman) |
| Half-plane intersection | `halfplane_intersection_example.cc` | Bounded feasible region for 2D linear constraints |
| Hull comparison | `convex_hull_comparison_example.cc` | Compare 5 hull algorithms on the same dataset |
| Closest pair | `closest_pair_example.cc` | Divide-and-conquer closest pair + verification |
| Rotating calipers | `rotating_calipers_example.cc` | Diameter and minimum width of a convex polygon |
| TikZ polygons | `tikz_polygons_example.cc` | Styled polygon visualization in PGF/TikZ |
| TikZ convex hull | `tikz_convex_hull_example.cc` | Input cloud + hull overlay |
| TikZ intersections | `tikz_intersection_example.cc` | Convex and boolean intersection overlays |
| TikZ Voronoi/power | `tikz_voronoi_power_example.cc` | Voronoi+Delaunay overlay and power diagram |
| TikZ advanced algorithms | `tikz_advanced_algorithms_example.cc` | Segment arrangement (colored faces), shortest path with funnel portals, convex decomposition, and alpha shape |
| TikZ funnel animation | `tikz_funnel_animation_example.cc` | Multi-page funnel (SSFA) step-by-step frames for shortest path |
| TikZ funnel beamer | `tikz_funnel_beamer_example.cc` | Single Beamer slide with SSFA overlays via `Tikz_Scene::draw_beamer_overlays()` |
| TikZ funnel beamer (2-col) | `tikz_funnel_beamer_twocol_example.cc` | Beamer overlays with left visual frame + right state panel (`apex/left/right/event`) |
| TikZ funnel handout | `tikz_funnel_beamer_handout_example.cc` | `beamer[handout]` output with one printable frame per funnel step (2-column layout) |
| TikZ scene composer | `tikz_scene_example.cc` | `Tikz_Scene` composition of arrangement, hull, and shortest-path overlays in one export |
| TikZ scene beamer/handout | `tikz_scene_beamer_example.cc` | `Tikz_Scene::draw_beamer()` and `draw_handout()` single-frame exports |
| TikZ scene overlays | `tikz_scene_overlays_example.cc` | `Tikz_Scene::draw_beamer_overlays()` / `draw_handout_overlays()` for step-by-step polygon slides |
| **Parallel** | | |
| Thread pool | `thread_pool_example.cc` | Concurrent tasks |
| Parallel ops | `ah_parallel_example.cc` | pmap, pfilter |
| **Memory** | | |
| Arena | `map_arena_example.C` | Arena allocator |

---

<a id="readme-testing"></a>
## Testing

### Running Tests

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

### Running with Sanitizers

```bash
# AddressSanitizer + UndefinedBehaviorSanitizer
cmake -S . -B build-asan \
    -DBUILD_TESTS=ON \
    -DUSE_SANITIZERS=ON \
    -DCMAKE_BUILD_TYPE=Debug

cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

### Test Coverage

- **7,700+ test cases**
- **99.98% pass rate**
- Tests for all data structures and algorithms
- Edge case coverage
- Performance regression tests
- Some long-running/performance tests are intentionally marked Disabled/Skipped and will be reported by `ctest`.

---

<a id="readme-contributing"></a>
## Contributing

Contributions are welcome! Here's how to get started:

### Development Setup

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

### Code Style Guidelines

- **Indentation**: 2 spaces
- **Naming**: `snake_case` for functions, `PascalCase` for classes
- **Documentation**: Doxygen comments for all public APIs
- **Testing**: Add tests for new features

### Pull Request Process

1. Fork the repository
2. Create feature branch: `git checkout -b feature/amazing-feature`
3. Make changes with tests
4. Ensure CI passes: `ctest --output-on-failure`
5. Submit Pull Request

---

## Bug Reports

If you find a bug, please report it!

- **Email**: `leandro.r.leon@gmail.com`
- **GitHub Issues**: [Open a new issue](https://github.com/lrleon/Aleph-w/issues)

Please include a minimal reproducible example if possible.

---

## Join the Community

Imagine a library that grows with your needs. **Aleph-w** is built by developers, for developers.

Your contribution matters. Whether it's fixing a typo, optimizing an algorithm, or adding a brand-new data structure, you have the power to shape the future of this project.

- **Have an idea?** Share it in [Discussions](https://github.com/lrleon/Aleph-w/discussions).
- **Found a better way?** Submit a Pull Request.
- **Want to learn?** Explore the code and ask questions.

Together, we can build the most comprehensive C++ algorithm library in the world. **Join us!**

---

<a id="readme-license"></a>
## License

Aleph-w is licensed under the **MIT License**.

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

## Authors

- **Leandro Rabindranath León** - *Creator and maintainer* - Universidad de Los Andes, Venezuela
- **Alejandro Mujica** - *Contributor*

---

<a id="readme-acknowledgments"></a>
## Acknowledgments

- **Universidad de Los Andes** (Mérida, Venezuela) - Birthplace of Aleph-w
- **Thousands of students** who learned algorithms with this library
- **Open source community** for continuous feedback
- **[SYMYL RESEARCH](https://simylresearch.com/en/)** for supporting the development and advancement of Aleph-w

---

<div align="center">

**[Back to Top](#readme-table-of-contents)**

Made with passion for algorithms and data structures

[GitHub](https://github.com/lrleon/Aleph-w) •
[Issues](https://github.com/lrleon/Aleph-w/issues) •
[Discussions](https://github.com/lrleon/Aleph-w/discussions)

</div>
