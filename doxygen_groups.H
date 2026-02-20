/** @file doxygen_groups.H
 *  @brief Doxygen group definitions for Aleph-w library organization
 *
 *  This file centralizes all module/group definitions (\@defgroup) used
 *  throughout the Aleph-w library. Each header file uses @ingroup to
 *  place itself within one or more of these logical groupings.
 *
 *  @author Leandro Rabindranath León
 */

/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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
*/

#ifndef DOXYGEN_GROUPS_H
#define DOXYGEN_GROUPS_H

// This file only contains documentation, no actual code
#ifdef DOXYGEN_ONLY

//==============================================================================
// PRIMARY MODULES
//==============================================================================

/**
 * @defgroup Algorithms Algorithms
 * @brief General-purpose algorithm implementations
 *
 * This module contains a wide variety of algorithms including:
 * - Sorting and searching algorithms
 * - Combinatorial algorithms (permutations, combinations)
 * - String algorithms
 * - Numerical algorithms
 * - Generic algorithms operating on data structures
 *
 * Many of these algorithms are parameterized by comparison functors
 * and can work with various data structures.
 */

/**
 * @defgroup Graphs Graph Data Structures and Algorithms
 * @brief Graph representations and graph algorithms
 *
 * Comprehensive support for graph theory including:
 * - Multiple graph representations (adjacency lists, matrices)
 * - Directed and undirected graphs
 * - Weighted and unweighted graphs
 * - Graph traversals (DFS, BFS)
 * - Shortest path algorithms (Dijkstra, Bellman-Ford, Floyd-Warshall, A*)
 * - Minimum spanning trees (Prim, Kruskal)
 * - Connectivity algorithms (strongly connected components, bridges, cuts)
 * - Topological sorting
 * - Graph coloring
 * - Eulerian and Hamiltonian paths
 * - Graph I/O and visualization
 *
 * All graph structures use template-based design for maximum flexibility.
 */

/**
 * @defgroup Trees Tree Data Structures
 * @brief Binary search trees, balanced trees, heaps, and tree algorithms
 *
 * Extensive collection of tree-based data structures:
 *
 * **Balanced Search Trees:**
 * - AVL trees (height-balanced)
 * - Red-Black trees (color-balanced)
 * - Splay trees (self-adjusting)
 * - Treaps (randomized)
 * - Skip lists
 * - All with rank/order statistics support (k-th element, rank queries)
 *
 * **Heaps:**
 * - Binary heaps
 * - Fibonacci heaps
 * - Array-based heaps
 *
 * **Tree Utilities:**
 * - Tree traversals (preorder, inorder, postorder)
 * - Tree operations (rotations, balancing)
 * - Tree visualization and printing
 * - Conversion between tree types
 *
 * All tree structures maintain logarithmic height for efficient operations.
 */

/**
 * @defgroup Hash Hash Tables and Hash Functions
 * @brief Hash table implementations and hashing utilities
 *
 * Multiple hash table implementations with different collision resolution:
 * - Linear probing (open addressing)
 * - Separate chaining (closed addressing)
 * - Cuckoo hashing
 * - Linear hashing (dynamic)
 * - Open-addressed double hashing
 *
 * Also includes:
 * - Various hash functions for common types
 * - Universal hashing support
 * - Hash table iterators
 * - Dynamic resizing with configurable load factors
 * - Hash sets and hash maps
 *
 * All hash tables provide expected O(1) operations under good hashing.
 */

/**
 * @defgroup Sequences Lists, Arrays, and Sequential Structures
 * @brief Linear data structures and sequential containers
 *
 * Various sequential data structures:
 * - Singly-linked lists
 * - Doubly-linked lists
 * - Circular lists
 * - Dynamic arrays (resizable)
 * - Fixed-size arrays
 * - Deques (double-ended queues)
 * - Stacks (LIFO)
 * - Queues (FIFO)
 * - Priority queues
 *
 * Includes both intrusive and non-intrusive variants.
 */

/**
 * @defgroup Networks Network Flow Algorithms and Applications
 * @brief Network optimization, flows, and graph-based networks
 *
 * Specialized algorithms for network problems:
 *
 * **Flow Algorithms:**
 * - Maximum flow (Ford-Fulkerson, Edmonds-Karp, Push-Relabel)
 * - Minimum cost flow
 * - Multi-commodity flow
 * - Flow with capacity constraints
 *
 * **Network Structures:**
 * - Network graphs with capacities and costs
 * - Residual networks
 * - Flow augmenting path utilities
 *
 * **Applications:**
 * - Supply and demand networks
 * - Network simplex algorithm
 * - Minimum cut (Stoer-Wagner, Karger)
 * - Bipartite matching via flows
 *
 * Essential for optimization problems in logistics, routing, and resource allocation.
 */

/**
 * @defgroup Geometry Computational Geometry
 * @brief Geometric structures, algorithms, and spatial data structures
 *
 * Support for computational geometry:
 * - Points (2D, 3D)
 * - Lines and line segments
 * - Polygons
 * - Convex hulls
 * - Triangulations (cutting ears, monotone)
 * - Delaunay triangulation (Bowyer-Watson, randomized incremental)
 * - Voronoi diagrams (dual of Delaunay; not Fortune sweep-line)
 * - Power diagram / regular triangulation (weighted Voronoi)
 * - Alpha shapes
 * - Segment arrangements (planar subdivision induced by segments)
 * - Visibility polygon
 * - Shortest path in polygon (funnel)
 * - Half-plane intersection / convex clipping utilities
 * - Spatial data structures (quadtrees, 2D-trees, grids)
 * - Spatial data structures (AABB tree, range trees, KD trees)
 * - Geometric algorithms (intersection, distance, containment)
 * - Euclidean graphs
 * - Geometric utilities and transformations
 *
 * Useful for graphics, GIS applications, and spatial analysis.
 */

/**
 * @defgroup Utilities General-Purpose Utilities
 * @brief Helper classes, utilities, and support code
 *
 * Wide range of utility components (mostly prefixed with `ah-`):
 *
 * **Functional Programming:**
 * - Functional utilities (map, filter, fold, zip)
 * - Function composition and currying
 * - Lazy evaluation
 * - Range-based utilities
 *
 * **Error Handling:**
 * - Exception classes
 * - Assertion macros
 * - Error reporting
 *
 * **Iterators:**
 * - Generic iterators
 * - Filter iterators
 * - Transform iterators
 * - Zip iterators
 *
 * **Type Utilities:**
 * - Type traits
 * - Type conversions
 * - Pair utilities
 *
 * **Time and Date:**
 * - Date handling
 * - Time measurements
 * - Chronometer utilities
 *
 * **String Utilities:**
 * - String manipulation
 * - String conversions
 * - Parsing utilities
 *
 * **Memory:**
 * - Arena allocators
 * - Memory management utilities
 * - Smart pointer helpers
 *
 * **Miscellaneous:**
 * - Logging
 * - Tracing
 * - Unique identifiers
 * - Singleton pattern
 * - Signals and event dispatch
 */

/**
 * @defgroup ThreadsAndConcurrency Threads and Concurrency
 * @brief Parallel computing, thread pools, and synchronization
 *
 * Support for multi-threaded and parallel programming:
 * - Thread pools with work stealing
 * - Worker pools
 * - Parallel map/reduce operations
 * - Mutex wrappers (RAII)
 * - Condition variable utilities
 * - Lock-free data structures
 * - Consumer-producer queues
 * - Parallel algorithms on containers
 *
 * Built on C++20 threading facilities with convenient abstractions.
 */

/**
 * @defgroup FunctionalProgramming Functional Programming
 * @brief Functional programming paradigms and utilities
 *
 * Comprehensive functional programming support:
 * - Higher-order functions (map, filter, reduce, fold)
 * - Function composition and pipelining
 * - Currying and partial application
 * - Lazy evaluation and generators
 * - Immutable data structure utilities
 * - Monadic operations
 * - Zip operations for combining sequences
 * - Range-based functional operations
 *
 * Integrates seamlessly with C++ STL and modern C++ features.
 */

/**
 * @defgroup Memory Memory Management
 * @brief Memory allocators and memory management utilities
 *
 * Advanced memory management:
 * - Arena allocators (bump pointer allocation)
 * - Memory pool allocators
 * - Map-based arenas for heterogeneous allocation
 * - Memory guards and leak detection
 * - Custom allocator adapters
 *
 * Designed for high-performance scenarios where standard allocators
 * are insufficient.
 */

//==============================================================================
// SECONDARY/SPECIALIZED MODULES
//==============================================================================

/**
 * @defgroup Sets Set Data Structures
 * @brief Mathematical sets implemented with various backing structures
 *
 * Set abstractions built on:
 * - Balanced trees (ordered sets)
 * - Hash tables (unordered sets)
 * - Arrays (static sets)
 *
 * Support standard set operations: union, intersection, difference.
 */

/**
 * @defgroup Maps Map and Dictionary Structures
 * @brief Key-value associative containers
 *
 * Map implementations based on:
 * - Balanced trees (ordered maps)
 * - Hash tables (unordered maps)
 *
 * Both support efficient insertion, deletion, and lookup by key.
 */

/**
 * @defgroup Heaps Heap Data Structures
 * @brief Priority queue implementations using heaps
 *
 * Various heap implementations:
 * - Binary heaps (array-based and node-based)
 * - Fibonacci heaps (for advanced algorithms)
 * - D-ary heaps
 *
 * All support O(log n) insertion and O(1) find-min operations.
 */

/**
 * @defgroup Matrices Matrix Operations
 * @brief Matrix data structures and operations
 *
 * Support for:
 * - Dense matrices
 * - Sparse matrices
 * - Matrix operations (addition, multiplication, transposition)
 * - LaTeX matrix output
 * - Matrix-based graph representations
 */

/**
 * @defgroup Iterators Iterators and Iterator Adaptors
 * @brief Generic iterators and iterator utilities
 *
 * Comprehensive iterator support:
 * - Forward, bidirectional, and random-access iterators
 * - Filter iterators (skip elements matching predicate)
 * - Transform iterators (apply function during iteration)
 * - Zip iterators (combine multiple sequences)
 * - Range-based iteration utilities
 *
 * Compatible with STL algorithms and C++ range-based for loops.
 */

/**
 * @defgroup IteratorFilters Iterator Filters
 * @brief Filtering iterators for conditional traversal
 *
 * Specialized iterators that skip elements not matching a predicate.
 * Useful for:
 * - Filtering graphs (e.g., only certain node types)
 * - Conditional tree traversal
 * - Selective container iteration
 */

/**
 * @defgroup Functors Function Objects
 * @brief Functors and function object utilities
 *
 * Predefined functors for common operations:
 * - Comparison functors (less, greater, equal)
 * - Arithmetic functors
 * - Logical functors
 * - Projection functors (extract fields)
 * - Adaptor functors (bind, negate, compose)
 */

/**
 * @defgroup Exceptions Exception Classes
 * @brief Exception hierarchy and error handling
 *
 * Structured exception hierarchy:
 * - Base exception classes
 * - Domain-specific exceptions (graph errors, memory errors, etc.)
 * - Exception utilities and formatting
 * - Stack trace support
 */

/**
 * @defgroup Containers Generic Containers
 * @brief General-purpose container abstractions
 *
 * High-level container interfaces that abstract underlying implementations.
 * Includes adaptors that provide unified interfaces to different
 * data structures (e.g., stack interface over list or array).
 */

#endif // DOXYGEN_ONLY

#endif // DOXYGEN_GROUPS_H
