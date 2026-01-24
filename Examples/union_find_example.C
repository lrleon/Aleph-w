
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


/**
 * @file union_find_example.C
 * @brief Disjoint Set Union (Union-Find) data structure demonstration
 *
 * This example demonstrates the Union-Find (also called Disjoint Set Union or DSU)
 * data structure, one of the most elegant and efficient data structures in computer
 * science. Despite its simplicity, it achieves near-constant-time operations through
 * clever optimizations.
 *
 * ## What is Union-Find?
 *
 * Union-Find maintains a collection of disjoint (non-overlapping) sets and supports
 * two main operations:
 * - **Union**: Merge two sets into one
 * - **Find**: Determine which set an element belongs to
 *
 * It's perfect for tracking connectivity and equivalence relationships.
 *
 * ## Key Operations
 *
 * ### make_set(x)
 * - Create a new set containing only element x
 * - **Time**: O(1)
 * - **Use**: Initialize the data structure
 *
 * ### find(x)
 * - Find the representative (root) of x's set
 * - Uses **path compression** for efficiency
 * - **Time**: O(α(n)) amortized (effectively O(1))
 * - **Use**: Check which set an element belongs to
 *
 * ### union(x, y)
 * - Merge the sets containing x and y
 * - Uses **union by rank** to keep trees shallow
 * - **Time**: O(α(n)) amortized (effectively O(1))
 * - **Use**: Connect two elements/sets
 *
 * ### same_set(x, y)
 * - Check if x and y are in the same set
 * - **Time**: O(α(n)) amortized
 * - **Use**: Test connectivity/equivalence
 *
 * ## Optimizations
 *
 * ### Union by Rank
 * - Always attach smaller tree under larger tree
 * - Keeps tree height logarithmic
 * - Prevents degenerate linear structures
 *
 * ### Path Compression
 * - During find, make all nodes point directly to root
 * - Flattens tree structure
 * - Future finds become faster
 *
 * ### Combined Effect
 * - Together, these optimizations achieve O(α(n)) per operation
 * - α(n) is the inverse Ackermann function
 * - For all practical purposes, α(n) ≤ 5 (effectively constant!)
 *
 * ## Implementation Variants
 *
 * ### Relation (Node-Based)
 * - Stores elements as nodes in a tree
 * - More flexible, can store additional data per element
 * - **Best for**: Variable number of elements, need element metadata
 *
 * ### Fixed_Relation (Array-Based)
 * - Uses array indexed by element ID
 * - More memory-efficient, faster access
 * - **Best for**: Fixed set of elements (0 to n-1), maximum performance
 *
 * ## Applications
 *
 * ### Graph Algorithms
 * - **Kruskal's MST**: Track connected components while adding edges
 * - **Connected components**: Find all components in a graph
 * - **Cycle detection**: Detect cycles in undirected graphs
 *
 * ### Network Analysis
 * - **Network connectivity**: Determine if nodes are connected
 * - **Social networks**: Find friend groups, communities
 * - **Infrastructure**: Check if cities are connected by roads
 *
 * ### Image Processing
 * - **Image segmentation**: Group connected pixels
 * - **Component labeling**: Label connected regions
 * - **Blob detection**: Find connected blobs in images
 *
 * ### Equivalence Relations
 * - **Equivalence classes**: Group equivalent elements
 * - **Partition problems**: Divide elements into groups
 * - **Constraint satisfaction**: Track variable equivalences
 *
 * ### Physical Systems
 * - **Percolation theory**: Model phase transitions
 * - **Particle systems**: Track particle clusters
 * - **Crystal growth**: Model crystal formation
 *
 * ## Complexity Analysis
 *
 * | Operation | Naive | Optimized |
 * |-----------|-------|-----------|
 * | make_set | O(1) | O(1) |
 * | find | O(n) | O(α(n)) |
 * | union | O(n) | O(α(n)) |
 *
 * **Space**: O(n) - one entry per element
 *
 * **Amortized complexity**: O(α(n)) per operation, where α(n) is the inverse
 * Ackermann function. For all practical values of n, α(n) ≤ 5, making it
 * effectively constant time!
 *
 * ## Example: Kruskal's Algorithm
 *
 * Union-Find is essential for Kruskal's MST algorithm:
 * ```
 * 1. Sort edges by weight
 * 2. For each edge (u, v):
 *    if find(u) != find(v):  // Not in same component
 *      add edge to MST
 *      union(u, v)            // Merge components
 * ```
 *
 * ## Usage Example
 *
 * ```cpp
 * Fixed_Relation uf(10);
 *
 * uf.join(0, 1);
 * uf.join(2, 3);
 * uf.join(1, 2);
 *
 * if (uf.are_connected(0, 3))
 *   cout << "0 and 3 are connected!\n";
 * ```
 *
 * ## Usage
 *
 * ```bash
 * ./union_find_example
 * ```
 *
 * This example has no command-line options; it runs all demos.
 *
 * @see tpl_union.H Union-Find implementation
 * @see percolation_example.C Application: Percolation simulation
 * @see mst_example.C Application: Kruskal's MST algorithm
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <string>
#include <vector>
#include <tpl_union.H>

using namespace std;
using namespace Aleph;

// =============================================================================
// Example 1: Basic Union-Find Operations
// =============================================================================

void demo_basic_operations()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║          EXAMPLE 1: Basic Union-Find Operations                  ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  // Fixed_Relation: For elements identified by integers 0..n-1
  const size_t n = 10;
  Fixed_Relation uf(n);

  cout << "Created Union-Find with " << n << " elements (0 to 9)\n";
  cout << "Initially, each element is in its own singleton set.\n\n";

  // Show initial state - demonstrate which elements are connected
  cout << "Initial state: Each element is isolated in its own set.\n";
  cout << "Number of disjoint sets: " << uf.get_num_blocks() << "\n";

  cout << "\n--- Performing unions ---\n\n";

  // Union some sets
  cout << "join(0, 1): Merge sets containing 0 and 1\n";
  uf.join(0, 1);

  cout << "join(2, 3): Merge sets containing 2 and 3\n";
  uf.join(2, 3);

  cout << "join(4, 5): Merge sets containing 4 and 5\n";
  uf.join(4, 5);

  cout << "join(0, 2): Merge sets {0,1} and {2,3} into {0,1,2,3}\n";
  uf.join(0, 2);

  cout << "\n--- Checking connectivity ---\n\n";

  // Check if elements are in the same set
  auto check = [&uf](size_t a, size_t b) {
    bool same = uf.are_connected(a, b);
    cout << "  " << a << " and " << b << " are " 
         << (same ? "in the SAME set" : "in DIFFERENT sets") << "\n";
  };

  cout << "Connectivity queries:\n";
  check(0, 3);  // Should be same (both in {0,1,2,3})
  check(4, 5);  // Should be same (both in {4,5})
  check(1, 4);  // Should be different
  check(6, 7);  // Should be different (both singletons)

  cout << "\n--- Final state ---\n\n";
  
  // Show connectivity relationships
  cout << "Elements in same set as 0: ";
  for (size_t i = 0; i < n; ++i)
    if (uf.are_connected(0, i))
      cout << i << " ";
  cout << "\n";

  cout << "Elements in same set as 4: ";
  for (size_t i = 0; i < n; ++i)
    if (uf.are_connected(4, i))
      cout << i << " ";
  cout << "\n";

  cout << "\nNumber of disjoint sets: " << uf.get_num_blocks() << "\n";
}

// =============================================================================
// Example 2: Network Connectivity Problem
// =============================================================================

void demo_network_connectivity()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║          EXAMPLE 2: Network Connectivity Problem                 ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  cout << "Problem: A network has 8 computers. Given a list of direct\n";
  cout << "connections, determine which computers can communicate.\n\n";

  const size_t num_computers = 8;
  Fixed_Relation network(num_computers);

  // Network topology (direct connections)
  vector<pair<size_t, size_t>> connections = {
    {0, 1}, {1, 2},           // Computers 0-1-2 connected
    {3, 4}, {4, 5}, {5, 3},   // Computers 3-4-5 connected (triangle)
    {6, 7}                     // Computers 6-7 connected
  };

  cout << "Network topology (direct connections):\n";
  cout << "  Cluster A: 0 — 1 — 2\n";
  cout << "  Cluster B: 3 — 4 — 5 (triangle)\n";
  cout << "  Cluster C: 6 — 7\n";
  cout << "  Isolated:  (none)\n\n";

  // Add connections
  cout << "Adding connections:\n";
  for (auto& [a, b] : connections) {
    cout << "  Connect " << a << " ↔ " << b << "\n";
    network.join(a, b);
  }

  cout << "\n--- Communication queries ---\n\n";

  auto can_communicate = [&](size_t a, size_t b) {
    bool can = network.are_connected(a, b);
    cout << "  Computer " << a << " and " << b << ": "
         << (can ? "✓ CAN communicate" : "✗ CANNOT communicate") << "\n";
  };

  cout << "Testing if computers can communicate:\n";
  can_communicate(0, 2);   // Yes (same cluster)
  can_communicate(3, 5);   // Yes (same cluster)
  can_communicate(0, 3);   // No (different clusters)
  can_communicate(6, 7);   // Yes (same cluster)
  can_communicate(2, 6);   // No (different clusters)

  cout << "\n--- Adding a bridge connection ---\n\n";

  cout << "Adding connection 2 ↔ 3 (bridges Cluster A and B)\n";
  network.join(2, 3);

  cout << "\nUpdated connectivity:\n";
  can_communicate(0, 5);   // Now yes!
  can_communicate(1, 4);   // Now yes!

  cout << "\nTotal isolated networks: " << network.size() << "\n";
}

// =============================================================================
// Example 3: Kruskal's MST (Simplified)
// =============================================================================

struct Edge {
  size_t u, v;
  double weight;
  
  bool operator<(const Edge& other) const {
    return weight < other.weight;
  }
};

void demo_kruskal_simulation()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║          EXAMPLE 3: Kruskal's MST Algorithm Simulation           ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  cout << "Kruskal's algorithm finds the Minimum Spanning Tree by:\n";
  cout << "  1. Sort edges by weight\n";
  cout << "  2. For each edge (u,v): add to MST if u,v in different sets\n";
  cout << "  3. Union-Find tracks which vertices are connected\n\n";

  // Graph with 6 vertices
  const size_t V = 6;
  
  // Edges: (u, v, weight)
  vector<Edge> edges = {
    {0, 1, 4.0}, {0, 2, 2.0}, {1, 2, 1.0}, {1, 3, 5.0},
    {2, 3, 8.0}, {2, 4, 10.0}, {3, 4, 2.0}, {3, 5, 6.0}, {4, 5, 3.0}
  };

  cout << "Graph edges:\n";
  for (const auto& e : edges)
    cout << "  " << e.u << " —(" << e.weight << ")— " << e.v << "\n";

  // Sort edges by weight
  sort(edges.begin(), edges.end());

  cout << "\nSorted edges by weight:\n";
  for (const auto& e : edges)
    cout << "  " << e.u << " —(" << e.weight << ")— " << e.v << "\n";

  // Kruskal's algorithm using Union-Find
  Fixed_Relation uf(V);
  vector<Edge> mst;
  double total_weight = 0;

  cout << "\n--- Running Kruskal's algorithm ---\n\n";

  for (const auto& e : edges) {
    if (!uf.are_connected(e.u, e.v)) {
      cout << "  ADD edge " << e.u << " —(" << e.weight << ")— " << e.v
           << " (connects different components)\n";
      uf.join(e.u, e.v);
      mst.push_back(e);
      total_weight += e.weight;
    } else {
      cout << "  SKIP edge " << e.u << " —(" << e.weight << ")— " << e.v
           << " (would create cycle)\n";
    }

    // MST has V-1 edges
    if (mst.size() == V - 1)
      break;
  }

  cout << "\n--- Minimum Spanning Tree ---\n\n";
  cout << "MST edges:\n";
  for (const auto& e : mst)
    cout << "  " << e.u << " —(" << e.weight << ")— " << e.v << "\n";
  cout << "\nTotal MST weight: " << total_weight << "\n";
}

// =============================================================================
// Example 4: Path Compression Visualization
// =============================================================================

void demo_path_compression()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║          EXAMPLE 4: Path Compression Effect                      ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  cout << "Path compression flattens the tree during find operations,\n";
  cout << "making subsequent finds much faster.\n\n";

  const size_t n = 8;
  Fixed_Relation uf(n);

  // Create a chain structure by sequential unions
  cout << "Creating connected components by sequential unions:\n";
  for (size_t i = 0; i < n - 1; ++i) {
    cout << "  join(" << i << ", " << (i + 1) << ")\n";
    uf.join(i, i + 1);
  }

  cout << "\nAfter all joins:\n";
  cout << "  All elements 0-" << (n-1) << " are now in the same set.\n";
  cout << "  Number of sets: " << uf.get_num_blocks() << "\n";

  cout << "\nVerifying all elements are connected:\n";
  for (size_t i = 1; i < n; ++i) {
    bool conn = uf.are_connected(0, i);
    cout << "  0 connected to " << i << ": " << (conn ? "YES" : "NO") << "\n";
  }

  cout << "\nPath compression happens automatically during are_connected().\n";
  cout << "The internal tree structure is flattened, making future\n";
  cout << "operations nearly O(1) - this is the key to Union-Find's speed!\n";
}

// =============================================================================
// Main
// =============================================================================

int main()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║     UNION-FIND (Disjoint Set Union) Data Structure Demo          ║\n";
  cout << "║                                                                  ║\n";
  cout << "║     Aleph-w Library - https://github.com/lrleon/Aleph-w          ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n";

  demo_basic_operations();
  demo_network_connectivity();
  demo_kruskal_simulation();
  demo_path_compression();

  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║                         Summary                                  ║\n";
  cout << "╠══════════════════════════════════════════════════════════════════╣\n";
  cout << "║  Union-Find is one of the most elegant data structures:          ║\n";
  cout << "║                                                                  ║\n";
  cout << "║  • Nearly O(1) operations via union-by-rank & path compression  ║\n";
  cout << "║  • Essential for Kruskal's MST algorithm                        ║\n";
  cout << "║  • Used in network connectivity, image processing, and more     ║\n";
  cout << "║                                                                  ║\n";
  cout << "║  Aleph-w provides Fixed_Relation for integer elements and       ║\n";
  cout << "║  Relation for arbitrary types with hash-based element lookup.   ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  return 0;
}
