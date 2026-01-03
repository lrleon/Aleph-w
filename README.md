# Aleph-w

<div align="center">

```
     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / /
   / ___ \| |  __/ |_) | | | |_____\ V  V /
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/
                 |_|
```

**A comprehensive C++ library of data structures and algorithms**

[![CI](https://github.com/lrleon/Aleph-w/actions/workflows/ci.yml/badge.svg)](https://github.com/lrleon/Aleph-w/actions/workflows/ci.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)

</div>

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Data Structures](#data-structures)
- [Algorithms](#algorithms)
- [Functional Programming](#functional-programming)
- [C++20 Ranges Support](#c20-ranges-support)
- [Examples](#examples)
- [Testing](#testing)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

Aleph-w is a production-ready C++ library providing a rich collection of data structures and algorithms, designed with both efficiency and usability in mind. Unlike the STL, Aleph-w exposes implementation details intentionally, allowing developers to make informed decisions about algorithmic complexity.

### Design Philosophy

- **Transparency**: Implementation details are explicit. `DynList<T>` is a singly-linked list; `DynDlist<T>` is doubly-linked. You always know the complexity.
- **Minimalism**: Following the end-to-end argument, components are designed with minimal assumptions.
- **Functional Programming**: All containers support functional primitives (`map`, `filter`, `fold`, `zip`, etc.).
- **Modern C++**: Full C++20 support including concepts, ranges, and move semantics.

---

## Features

### 🗂️ Containers

| Container | Description | Key Operations |
|-----------|-------------|----------------|
| `DynList<T>` | Singly-linked list | O(1) front insert/remove |
| `DynDlist<T>` | Doubly-linked list | O(1) insert/remove anywhere |
| `DynArray<T>` | Dynamic array with functional ops | O(1) amortized append |
| `DynSetTree<K,Tree>` | Balanced BST set | O(log n) insert/search/remove |
| `DynMapTree<K,V,Tree>` | Balanced BST map | O(log n) operations |
| `DynSetHash<K>` | Hash-based set | O(1) average operations |
| `DynMapHash<K,V>` | Hash-based map | O(1) average operations |

### 🌳 Trees

| Tree Type | Description | Height Guarantee |
|-----------|-------------|------------------|
| `Avl_Tree<K>` | AVL balanced tree | ≤ 1.44 log₂(n) |
| `Rb_Tree<K>` | Red-Black tree (bottom-up) | ≤ 2 log₂(n) |
| `Td_Rb_Tree<K>` | Red-Black tree (top-down) | ≤ 2 log₂(n) |
| `Treap<K>` | Randomized BST | O(log n) expected |
| `Splay_Tree<K>` | Self-adjusting BST | O(log n) amortized |
| `Rand_Tree<K>` | Randomized tree | O(log n) expected |

### 📊 Graphs

| Component | Description |
|-----------|-------------|
| `List_Graph<N,A>` | Adjacency list graph |
| `Array_Graph<N,A>` | Array-based graph (cache-friendly) |
| `List_Digraph<N,A>` | Directed graph |
| `Net_Graph<N,A>` | Network flow graph |

### 🔧 Algorithms

**Graph Algorithms:**
- Dijkstra, Bellman-Ford, Floyd-Warshall (shortest paths)
- Prim, Kruskal (minimum spanning trees)
- Tarjan (strongly connected components)
- Ford-Fulkerson (maximum flow)
- Network Simplex (minimum cost flow)

**Sorting:**
- Quicksort, Mergesort, Heapsort
- Insertion sort, Selection sort
- Shell sort (Ciura gaps)

**Other:**
- Union-Find with path compression
- Bloom filters
- LRU/MRU caches
- Quadtrees

---

## Requirements

### Compiler
- **GCC 12+** or **Clang 14+** (with libc++)
- C++20 support required

### Dependencies

| Library | Ubuntu Package | Purpose |
|---------|----------------|---------|
| GMP | `libgmp-dev` | Arbitrary precision arithmetic |
| MPFR | `libmpfr-dev` | Multi-precision floating point |
| GSL | `libgsl-dev` | Scientific computing |
| X11 | `libx11-dev` | Visualization (optional) |

### Build Tools
- CMake 3.16+
- Ninja (recommended) or Make

### Install Dependencies (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y \
    cmake ninja-build \
    g++ clang \
    libgmp-dev libmpfr-dev libgsl-dev \
    libx11-dev libpthread-stubs0-dev
```

---

## Installation

### Using CMake (Recommended)

```bash
# Clone the repository
git clone https://github.com/lrleon/Aleph-w.git
cd Aleph-w

# Configure and build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Install (optional)
sudo cmake --install build
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | ON | Build test suite |
| `BUILD_EXAMPLES` | OFF | Build example programs |
| `USE_SANITIZERS` | OFF | Enable ASan/UBSan |

### Using in Your Project

**CMake (FetchContent):**

```cmake
include(FetchContent)
FetchContent_Declare(
    aleph-w
    GIT_REPOSITORY https://github.com/lrleon/Aleph-w.git
    GIT_TAG v3
)
FetchContent_MakeAvailable(aleph-w)

target_link_libraries(your_target PRIVATE Aleph::Aleph)
```

**Manual Include:**

```bash
g++ -std=c++20 -I/path/to/Aleph-w your_code.cpp -lgmp -lmpfr -lgsl -lpthread
```

---

## Quick Start

### Dynamic List

```cpp
#include <tpl_dynDlist.H>

int main() {
    DynDlist<int> list = {1, 2, 3, 4, 5};
    
    // Functional operations
    auto doubled = list.map([](int x) { return x * 2; });
    auto evens = list.filter([](int x) { return x % 2 == 0; });
    int sum = list.foldl(0, [](int acc, int x) { return acc + x; });
    
    // Range-for iteration
    for (const auto& item : list)
        std::cout << item << " ";
}
```

### Balanced Tree Set

```cpp
#include <tpl_dynSetTree.H>

int main() {
    // AVL-based set
    DynSetTree<int, Avl_Tree> set;
    
    set.insert(10);
    set.insert(5);
    set.insert(15);
    
    if (auto* p = set.search(10))
        std::cout << "Found: " << *p << "\n";
    
    // In-order traversal
    set.for_each([](int x) { std::cout << x << " "; });
}
```

### Graph with Dijkstra

```cpp
#include <tpl_graph.H>
#include <Dijkstra.H>

struct NodeInfo { std::string name; };
struct ArcInfo { double weight; };

int main() {
    List_Graph<Graph_Node<NodeInfo>, Graph_Arc<ArcInfo>> g;
    
    auto* a = g.insert_node({"A"});
    auto* b = g.insert_node({"B"});
    auto* c = g.insert_node({"C"});
    
    g.insert_arc(a, b, {1.5});
    g.insert_arc(b, c, {2.0});
    g.insert_arc(a, c, {4.0});
    
    // Find shortest path
    Path<decltype(g)> path(g);
    Dijkstra_Min_Paths<decltype(g)> dijkstra;
    dijkstra(g, a, c, path);
    
    std::cout << "Path length: " << path.size() << "\n";
}
```

---

## Functional Programming

All Aleph-w containers support a rich set of functional operations:

| Method | Description | Example |
|--------|-------------|---------|
| `for_each(f)` | Apply f to each element | `list.for_each([](int x) { print(x); })` |
| `map(f)` | Transform elements | `list.map([](int x) { return x*2; })` |
| `filter(p)` | Select elements matching predicate | `list.filter([](int x) { return x > 0; })` |
| `foldl(init, f)` | Left fold (reduce) | `list.foldl(0, std::plus<>{})` |
| `all(p)` | Check if all match | `list.all([](int x) { return x > 0; })` |
| `exists(p)` | Check if any matches | `list.exists([](int x) { return x < 0; })` |
| `find_ptr(p)` | Find first matching | `list.find_ptr([](int x) { return x == 5; })` |
| `zip(other)` | Pair elements | `list.zip(other_list)` |
| `take(n)` | First n elements | `list.take(3)` |
| `drop(n)` | Skip n elements | `list.drop(2)` |
| `partition(p)` | Split by predicate | `auto [yes, no] = list.partition(pred)` |

---

## C++20 Ranges Support

Aleph-w containers work seamlessly with C++20 ranges:

```cpp
#include <ah-ranges.H>
#include <ranges>

DynList<int> list = {1, 2, 3, 4, 5};

// Use with std::views
auto result = list 
    | std::views::filter([](int x) { return x % 2 == 0; })
    | std::views::transform([](int x) { return x * 10; });

// Convert ranges to Aleph containers
auto vec = std::views::iota(1, 10) | to_dynlist;
auto arr = some_range | to_dynarray;
```

### Available Adaptors

- `to_dynlist` - Convert to DynList
- `to_dynarray` - Convert to DynArray  
- `to_dyndlist` - Convert to DynDlist
- `to<Container>()` - Generic conversion

> ⚠️ **Note**: For Clang, use `libc++` instead of `libstdc++` for full `std::ranges` compatibility.

---

## Examples

The `Examples/` directory contains demonstrations of various components:

```bash
# Build examples
cmake -S . -B build -DBUILD_EXAMPLES=ON
cmake --build build

# Run an example
./build/Examples/dijkstra_example
```

Notable examples:
- `dijkstra_example` - Shortest path computation
- `spanning_tree` - Minimum spanning tree
- `max_flow` - Network flow
- `btreepic` - Tree visualization

---

## Testing

Tests are located in `Tests/` and use Google Test:

```bash
# Build with tests
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure

# Run specific test
./build/Tests/dynlist_test
```

### Running with Sanitizers

```bash
cmake -S . -B build-asan -DUSE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-asan
ctest --test-dir build-asan
```

---

## Documentation

All headers include comprehensive Doxygen documentation:

```bash
# Generate documentation
doxygen Doxyfile

# Open in browser
open docs/html/index.html
```

### Key Header Files

| Header | Contents |
|--------|----------|
| `tpl_dynDlist.H` | Doubly-linked list |
| `tpl_dynArray.H` | Dynamic array |
| `tpl_dynSetTree.H` | Tree-based sets/maps |
| `tpl_dynSetHash.H` | Hash-based sets |
| `tpl_graph.H` | Graph data structures |
| `Dijkstra.H` | Shortest path algorithms |
| `Prim.H` | Minimum spanning tree |
| `ah-ranges.H` | C++20 ranges support |
| `ahFunctional.H` | Functional utilities |

---

## Project Structure

```
Aleph-w/
├── *.H                 # Header-only library files
├── *.C                 # Implementation files (compiled into libAleph.a)
├── Examples/           # Usage examples
├── Tests/              # Google Test-based tests
├── CMakeLists.txt      # CMake build configuration
├── .github/workflows/  # CI configuration
└── docs/               # Generated documentation
```

---

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Code Style

- Use descriptive names
- Document all public APIs with Doxygen
- Add tests for new functionality
- Ensure CI passes (both GCC and Clang)

---

## Authors

- **Leandro Rabindranath León** - *Creator and maintainer*
- **Alejandro Mujica** - *Contributor*

---

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

```
Copyright (c) 2002-2024 Leandro Rabindranath León

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
```

---

<div align="center">

**[⬆ Back to Top](#aleph-w)**

Made with ❤️ for the C++ community

</div>
