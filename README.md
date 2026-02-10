# Aleph-w {#aleph-w}

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

## Table of Contents

- [Overview](#overview)
- [History and Philosophy](#history-and-philosophy)
- [Features at a Glance](#features-at-a-glance)
- [Requirements](#requirements)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Data Structures](#data-structures)
  - [Balanced Search Trees](#balanced-search-trees)
  - [Hash Tables](#hash-tables)
  - [Heaps and Priority Queues](#heaps-and-priority-queues)
  - [Lists and Sequential Structures](#lists-and-sequential-structures)
  - [Range Query Structures](#range-query-structures)
  - [Graphs](#graphs)
  - [Linear Algebra (Sparse Structures)](#linear-algebra-sparse-structures)
- [Algorithms](#algorithms)
  - [Shortest Path Algorithms](#shortest-path-algorithms)
  - [Minimum Spanning Trees](#minimum-spanning-trees)
  - [Network Flows](#network-flows)
  - [Graph Connectivity](#graph-connectivity)
  - [Matching](#matching)
  - [Sorting Algorithms](#sorting-algorithms)
- [Memory Management](#memory-management)
  - [Arena Allocators](#arena-allocators)
- [Parallel Computing](#parallel-computing)
- [Functional Programming](#functional-programming)
- [Tutorial](#tutorial)
- [API Reference](#api-reference)
- [Benchmarks](#benchmarks)
- [Examples](#examples)
- [Testing](#testing)
- [Contributing](#contributing)
- [License](#license)
- [Acknowledgments](#acknowledgments)

---

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

## Features at a Glance

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
│  LINEAR ALGEBRA                                                            │
│  ├─ Sparse Vector                                                          │
│  ├─ Sparse Matrix                                                          │
│  └─ Domain Indexing                                                        │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

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
│  SORTING                  SEARCHING                OTHER                   │
│  ├─ Quicksort            ├─ Binary Search         ├─ Union-Find            │
│  ├─ Mergesort            ├─ Interpolation         ├─ Huffman Coding        │
│  ├─ Heapsort             └─ Pattern Matching      ├─ Simplex (LP)          │
│  ├─ Introsort                                     └─ Geometric             │
│  └─ Shell Sort                                                             │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

---

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

## Data Structures

### Balanced Search Trees

Aleph-w provides **8 different balanced tree implementations**, each optimized for specific use cases.

#### Tree Comparison

```
┌────────────────────────────────────────────────────────────────────────────┐
│                      BALANCED TREE COMPARISON                              │
├──────────────┬──────────────┬──────────────┬──────────────┬────────────────┤
│   Operation  │   AVL Tree   │  Red-Black   │  Splay Tree  │     Treap      │
├──────────────┼──────────────┼──────────────┼──────────────┼────────────────┤
│   Insert     │  O(log n)    │  O(log n)    │  O(log n)*   │  O(log n)**    │
│   Search     │  O(log n)    │  O(log n)    │  O(log n)*   │  O(log n)**    │
│   Delete     │  O(log n)    │  O(log n)    │  O(log n)*   │  O(log n)**    │
│   Min/Max    │  O(log n)    │  O(log n)    │  O(log n)*   │  O(log n)**    │
├──────────────┼──────────────┼──────────────┼──────────────┼────────────────┤
│   Height     │ ≤1.44 log n  │  ≤2 log n    │ Unbounded    │  O(log n)**    │
│   Balance    │   Strict     │   Relaxed    │    None      │   Randomized   │
├──────────────┼──────────────┼──────────────┼──────────────┼────────────────┤
│  Best For    │ Read-heavy   │  General     │  Locality    │   Simplicity   │
│              │  workloads   │  purpose     │  patterns    │   randomized   │
└──────────────┴──────────────┴──────────────┴──────────────┴────────────────┘
  * Amortized    ** Expected (randomized)
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
┌────────────────────────────────────────────────────────────────────────────┐
│                         FENWICK TREE VARIANTS                              │
├──────────────────┬──────────────┬──────────────┬──────────────┬────────────┤
│     Operation    │ Gen (Abelian)│ Fenwick_Tree │  Range_Fen   │  Complexity │
├──────────────────┼──────────────┼──────────────┼──────────────┼────────────┤
│ update(i,delta)  │     O(log n) │   O(log n)   │   O(log n)   │            │
│ prefix / get     │     O(log n) │   O(log n)   │   O(log n)   │            │
│ range_query      │     O(log n) │   O(log n)   │   O(log n)   │            │
│ range_update     │       N/A    │     N/A      │   O(log n)   │            │
│ find_kth         │       N/A    │   O(log n)   │     N/A      │            │
├──────────────────┼──────────────┼──────────────┼──────────────┼────────────┤
│ Group Operand    │  Arbitrary   │  operator+   │  operator+   │            │
│                  │  (XOR, +mod) │  operator-   │  operator-   │            │
├──────────────────┼──────────────┼──────────────┼──────────────┼────────────┤
│ Best For         │ Custom ops   │ Order stats  │ Promotions   │            │
│                  │ (XOR, etc)   │ Find k-th    │ Dividends    │            │
└──────────────────┴──────────────┴──────────────┴──────────────┴────────────┘
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
┌────────────────────────────────────────────────────────────────────────────┐
│                        SEGMENT TREE VARIANTS                               │
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
┌──────────────────────────────────────────────────────────────────────┐
│                  RMQ  ←→  LCA  ←→  Cartesian Tree                  │
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

### Linear Algebra (Sparse Structures)

Aleph-w provides **sparse vector and matrix classes** with domain-based indexing, optimized for data with many zeros:

```text
┌────────────────────────────────────────────────────────────────────────────┐
│                   SPARSE LINEAR ALGEBRA STRUCTURES                         │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  SPARSE VECTOR (al-vector.H)                                              │
│  ────────────────────────────                                             │
│  Vector<T, NumType>                                                        │
│                                                                            │
│  • Domain-based indexing (any type T)                                     │
│  • Stores only non-zero entries (hash-based)                              │
│  • Epsilon tolerance for near-zero removal                                │
│  • Operations: +, -, *, dot product, norms                                │
│                                                                            │
│  Memory: O(nonzeros) instead of O(dimension)                              │
│                                                                            │
│  SPARSE MATRIX (al-matrix.H)                                              │
│  ────────────────────────────                                             │
│  Matrix<Trow, Tcol, NumType>                                              │
│                                                                            │
│  • Arbitrary row/column domains                                           │
│  • Stores only non-zero entries                                           │
│  • Operations: +, -, *, transpose, row/col vectors                        │
│  • Integration with Vector class                                          │
│                                                                            │
│  Memory: O(nonzeros) instead of O(rows × cols)                            │
│                                                                            │
│  WHEN TO USE SPARSE?                                                      │
│  • Sparsity > 90% (most entries are zero)                                │
│  • Large dimensions with few non-zeros                                    │
│  • Need domain-based indexing (named rows/columns)                        │
│                                                                            │
│  Example: 1000×1000 matrix with 1000 non-zeros:                          │
│    Dense:  1,000,000 doubles = 8 MB                                       │
│    Sparse:     1,000 entries = 8 KB  (1000× savings!)                     │
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
├───────────────────────────────────────┼─────────────────────────────────────┤
│  Insert        │ O(log n) │ O(1)*     │  * Amortized                        │
│  Find-Min      │ O(1)     │ O(1)      │                                     │
│  Delete-Min    │ O(log n) │ O(log n)* │                                     │
│  Decrease-Key  │ O(log n) │ O(1)*     │  Key advantage of Fibonacci         │
│  Merge         │ O(n)     │ O(1)      │                                     │
└───────────────────────────────────────┴─────────────────────────────────────┘
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
│  ┌───┬───┬───┬───┬───┬─────────┐    ┌───┬───┬───┬───┬───┐                   │
│  │ A │ B │ C │ D │ E │ (space) │    │ A │ B │ C │ D │ E │                   │
│  └───┴───┴───┴───┴───┴─────────┘    └───┴───┴───┴───┴───┘                   │
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

### Graphs

Aleph-w provides multiple graph representations optimized for different use cases:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        GRAPH REPRESENTATIONS                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ADJACENCY LIST (List_Graph)         ADJACENCY MATRIX (Array_Graph)         │
│                                                                             │
│  A: [B, C, D]                        │ A │ B │ C │ D │                      │
│  B: [A, D]                         ──┼───┼───┼───┼───│                      │
│  C: [A, D]                         A │ 0 │ 1 │ 1 │ 1 │                      │
│  D: [A, B, C]                      B │ 1 │ 0 │ 0 │ 1 │                      │
│                                    C │ 1 │ 0 │ 0 │ 1 │                      │
│  Space: O(V + E)                   D │ 1 │ 1 │ 1 │ 0 │                      │
│  Add edge: O(1)                                                             │
│  Check edge: O(degree)              Space: O(V²)                            │
│  Best for: Sparse graphs            Add edge: O(1)                          │
│                                     Check edge: O(1)                        │
│                                     Best for: Dense graphs                  │
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

## Algorithms

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
└────────────────┴────────────┴────────────┴──────────────────────────────────┘
```

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
    DynList<Node*> cut_nodes;
    compute_cut_nodes(g, cut_nodes);

    std::cout << "Articulation points:\n";
    cut_nodes.for_each([](auto* node) {
        std::cout << "  " << node->get_info() << "\n";
    });

    return 0;
}
```

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
└─────────────────────────────────────────────────────────────────────────────┘
```

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

## Memory Management

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
│  ┌────────────────┬─────────────────┬──────────────────┐                    │
│  │   Operation    │     malloc      │      Arena       │                    │
│  ├────────────────┼─────────────────┼──────────────────┤                    │
│  │   Allocate     │   O(log n)*     │      O(1)        │                    │
│  │   Deallocate   │   O(log n)*     │  O(1) bulk only  │                    │
│  │   Fragmentation│     High        │      None        │                    │
│  │   Cache perf   │     Poor        │    Excellent     │                    │
│  │   Overhead     │   16+ bytes     │    0 bytes       │                    │
│  └────────────────┴─────────────────┴──────────────────┘                    │
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
    pfor_each(items, [](Item& item) {
        item.expensive_process();
    });

    return 0;
}
```

---

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

#### Graph Algorithms

| Header | Function | Description |
|--------|----------|-------------|
| `Dijkstra.H` | `dijkstra_min_path()` | Single-source shortest path |
| `Bellman_Ford.H` | `bellman_ford_min_path()` | Shortest path (negative edges) |
| `Floyd_Warshall.H` | `floyd_all_shortest_paths()` | All-pairs shortest paths |
| `Johnson.H` | `johnson_all_pairs()` | All-pairs (sparse graphs) |
| `AStar.H` | `astar_search()` | Heuristic pathfinding |
| `Kruskal.H` | `kruskal_min_spanning_tree()` | MST (edge-based) |
| `Prim.H` | `prim_min_spanning_tree()` | MST (vertex-based) |
| `tpl_maxflow.H` | `*_maximum_flow()` | Maximum flow algorithms |
| `tpl_mincost.H` | `min_cost_max_flow()` | Min-cost max-flow |
| `Tarjan.H` | `tarjan_scc()` | Strongly connected components |
| `tpl_components.H` | `connected_components()` | Connected components |
| `tpl_cut_nodes.H` | `compute_cut_nodes()` | Articulation points |
| `topological_sort.H` | `topological_sort()` | DAG ordering |
| `Karger.H` | `karger_min_cut()` | Probabilistic min-cut |
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
│  malloc         ██████████████████████████████████████ 156 ns/alloc        │
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
| **Graph Basics** | | |
| BFS/DFS | `bfs_dfs_example.C` | Traversal algorithms |
| Components | `graph_components_example.C` | Finding components |
| Random graphs | `random_graph_example.C` | Graph generation |
| **Shortest Paths** | | |
| Dijkstra | `dijkstra_example.cc` | Single-source |
| Bellman-Ford | `bellman_ford_example.cc` | Negative weights |
| Johnson | `johnson_example.cc` | All-pairs sparse |
| A* | `astar_example.cc` | Heuristic search |
| **Network Flows** | | |
| Max flow | `network_flow_example.C` | Basic max flow |
| Min-cost flow | `mincost_flow_example.cc` | Cost optimization |
| Min cut | `min_cut_example.cc` | Karger, Stoer-Wagner |
| **Special Algorithms** | | |
| MST | `mst_example.C` | Kruskal, Prim |
| SCC | `tarjan_example.C` | Strongly connected |
| Topological | `topological_sort_example.C` | DAG ordering |
| **Parallel** | | |
| Thread pool | `thread_pool_example.cc` | Concurrent tasks |
| Parallel ops | `ah_parallel_example.cc` | pmap, pfilter |
| **Memory** | | |
| Arena | `map_arena_example.C` | Arena allocator |

---

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

## Acknowledgments

- **Universidad de Los Andes** (Mérida, Venezuela) - Birthplace of Aleph-w
- **Thousands of students** who learned algorithms with this library
- **Open source community** for continuous feedback
- **[SYMYL RESEARCH](https://simylresearch.com/en/)** for supporting the development and advancement of Aleph-w

---

<div align="center">

**[Back to Top](#aleph-w)**

Made with passion for algorithms and data structures

[GitHub](https://github.com/lrleon/Aleph-w) •
[Issues](https://github.com/lrleon/Aleph-w/issues) •
[Discussions](https://github.com/lrleon/Aleph-w/discussions)

</div>
