/**
 * @file karger_example.cc
 * @brief Comprehensive educational example for Karger's min-cut algorithm
 * 
 * WHAT IS A MIN-CUT?
 * ==================
 * A minimum cut of a graph is a partition of vertices into two non-empty sets
 * such that the number of edges crossing between the sets is minimized.
 * 
 * WHY IS IT IMPORTANT?
 * ====================
 * - Network reliability: Find critical connections (bridges)
 * - Graph clustering: Partition graphs into communities
 * - VLSI design: Minimize wire crossings
 * - Image segmentation: Partition image into regions
 * 
 * KARGER'S ALGORITHM:
 * ===================
 * Randomized algorithm that repeatedly contracts random edges until only
 * two "super-nodes" remain. The edges between them form a cut.
 * 
 * - Time: O(n^2 * m) per iteration
 * - Success probability: >= 1/n^2 per iteration
 * - Running O(n^2 log n) iterations gives high probability of success
 * - Fast variant (Karger-Stein): O(n^2 log^3 n)
 * 
 * COMPILE & RUN:
 * ==============
 * g++ -std=c++20 -I.. -o karger_example karger_example.cc -lgsl -lgslcblas
 * ./karger_example
 */

#include <iostream>
#include <Karger.H>
#include <tpl_graph.H>

using namespace std;
using namespace Aleph;

int main()
{
  cout << "=== Karger's Min-Cut Algorithm: Educational Examples ===\n\n";
  
  cout << "WHAT IS KARGER'S ALGORITHM?\n";
  cout << "===========================\n";
  cout << "Randomized algorithm to find minimum cut in undirected graphs.\n";
  cout << "Repeatedly contracts random edges until 2 super-nodes remain.\n\n";
  
  cout << "KEY CONCEPTS:\n";
  cout << "-------------\n";
  cout << "1. MIN-CUT: Partition vertices into 2 sets minimizing edges between them\n";
  cout << "2. EDGE CONTRACTION: Merge two nodes connected by an edge\n";
  cout << "3. RANDOMIZATION: Success probability ≥ 1/n² per iteration\n";
  cout << "4. REPETITION: O(n² log n) iterations give high success probability\n\n";
  
  cout << "APPLICATIONS:\n";
  cout << "-------------\n";
  cout << "* Network reliability: Find critical connections (bridges)\n";
  cout << "* Graph clustering: Partition graphs into communities\n";
  cout << "* VLSI design: Minimize wire crossings\n";
  cout << "* Image segmentation: Partition images into regions\n\n";
  
  cout << "EXAMPLE SCENARIO: Network Bottleneck Detection\n";
  cout << "===============================================\n\n";
  
  cout << "Consider a network with two clusters:\n";
  cout << "  Cluster 1: [A-B] (well-connected internally)\n";
  cout << "  Cluster 2: [C-D] (well-connected internally)\n";
  cout << "  Bridge:    B <--> C (single connection between clusters)\n\n";
  
  cout << "GRAPH STRUCTURE:\n";
  cout << "  Nodes: A, B, C, D\n";
  cout << "  Edges: A-B, B-A, C-D, D-C, B-C, C-B (6 arcs total)\n\n";
  
  cout << "EXPECTED MIN-CUT:\n";
  cout << "  Size: 1 (the bridge B-C)\n";
  cout << "  Partition 1: {A, B}\n";
  cout << "  Partition 2: {C, D}\n\n";
  
  cout << "ALGORITHM STEPS:\n";
  cout << "----------------\n";
  cout << "1. Start with original graph\n";
  cout << "2. Randomly select an edge and contract it\n";
  cout << "3. Repeat until only 2 super-nodes remain\n";
  cout << "4. Count edges between the 2 super-nodes (cut size)\n";
  cout << "5. Repeat entire process O(n² log n) times\n";
  cout << "6. Return minimum cut found\n\n";
  
  cout << "COMPLEXITY:\n";
  cout << "-----------\n";
  cout << "* Per iteration: O(n²)\n";
  cout << "* Total with repetition: O(n⁴ log n)\n";
  cout << "* Karger-Stein variant: O(n² log³ n)\n\n";
  
  cout << "ADVANTAGES:\n";
  cout << "-----------\n";
  cout << "+ Simple to implement\n";
  cout << "+ Works on general graphs\n";
  cout << "+ Provably correct with high probability\n\n";
  
  cout << "DISADVANTAGES:\n";
  cout << "--------------\n";
  cout << "- Randomized (may need multiple runs)\n";
  cout << "- Slower than deterministic algorithms for dense graphs\n\n";
  
  cout << "USAGE IN CODE:\n";
  cout << "==============\n";
  cout << "```cpp\n";
  cout << "// 1. Define graph type\n";
  cout << "using GT = List_Graph<Graph_Node<string>, Graph_Arc<int>>;\n";
  cout << "GT g;\n\n";
  cout << "// 2. Build graph (nodes and arcs)\n";
  cout << "auto a = g.insert_node(\"A\");\n";
  cout << "auto b = g.insert_node(\"B\");\n";
  cout << "g.insert_arc(a, b);\n\n";
  cout << "// 3. Create Karger solver\n";
  cout << "Karger_Min_Cut<GT> karger;\n\n";
  cout << "// 4. Run algorithm\n";
  cout << "DynList<GT::Node*> S, T;\n";
  cout << "DynList<GT::Arc*> cut_arcs;\n";
  cout << "size_t min_cut = karger(g, S, T, cut_arcs);\n\n";
  cout << "// 5. Analyze results\n";
  cout << "// S and T contain the two partitions\n";
  cout << "// cut_arcs contains edges crossing the cut\n";
  cout << "// min_cut is the size of the minimum cut\n";
  cout << "```\n\n";
  
  cout << "IMPROVING ACCURACY:\n";
  cout << "===================\n";
  cout << "Since Karger's algorithm is randomized:\n";
  cout << "* Run it MULTIPLE times\n";
  cout << "* Keep the MINIMUM cut found\n";
  cout << "* For n nodes, O(n² log n) runs give high probability\n\n";
  
  cout << "Example: Multiple iterations\n";
  cout << "```cpp\n";
  cout << "size_t best_cut = infinity;\n";
  cout << "for (int i = 0; i < n*n*log(n); ++i) {\n";
  cout << "  size_t cut = karger(g, S, T, cut_arcs);\n";
  cout << "  if (cut < best_cut)\n";
  cout << "    best_cut = cut;\n";
  cout << "}\n";
  cout << "```\n\n";
  
  {
    cout << "COMPARISON WITH OTHER MIN-CUT ALGORITHMS:\n";
    cout << "=========================================\n";
    cout << "\nKarger (this algorithm):\n";
    cout << "  Time:         O(n⁴ log n) with repetition\n";
    cout << "  Type:         Randomized\n";
    cout << "  Advantage:    Simple implementation\n";
    cout << "  Disadvantage: Slower for dense graphs\n\n";
    
    cout << "Karger-Stein (improved variant):\n";
    cout << "  Time:         O(n² log³ n)\n";
    cout << "  Type:         Randomized\n";
    cout << "  Advantage:    Much faster than basic Karger\n";
    cout << "  Disadvantage: More complex implementation\n\n";
    
    cout << "Stoer-Wagner:\n";
    cout << "  Time:         O(nm + n² log n)\n";
    cout << "  Type:         Deterministic\n";
    cout << "  Advantage:    Always correct, faster for sparse graphs\n";
    cout << "  Disadvantage: More complex\n\n";
    
    cout << "Max-Flow Min-Cut:\n";
    cout << "  Time:         O(n³) or better with advanced algorithms\n";
    cout << "  Type:         Deterministic\n";
    cout << "  Advantage:    Well-studied, many optimizations\n";
    cout << "  Disadvantage: Requires choosing source and sink\n\n";
  }
  
  cout << "\n=== SUMMARY: Key Takeaways ===\n";
  cout << "\n1. WHAT IS MIN-CUT?\n";
  cout << "   Minimum number of edges to remove to disconnect the graph\n";
  cout << "\n2. WHY USE KARGER'S ALGORITHM?\n";
  cout << "   - Simple randomized approach (contract random edges)\n";
  cout << "   - Works on any undirected graph\n";
  cout << "   - No need to specify source/sink like max-flow algorithms\n";
  cout << "\n3. PRACTICAL TIPS:\n";
  cout << "   - Run multiple iterations for higher accuracy\n";
  cout << "   - Use Karger_Min_Cut::fast() for large graphs (n > 100)\n";
  cout << "   - Min-cut = 1 means BRIDGE exists (critical connection)\n";
  cout << "\n4. REAL-WORLD APPLICATIONS:\n";
  cout << "   * Network reliability analysis (find weak points)\n";
  cout << "   * Community detection in social networks\n";
  cout << "   * Image segmentation (computer vision)\n";
  cout << "   * VLSI circuit design (minimize wire crossings)\n";
  cout << "\n5. COMPLEXITY SUMMARY:\n";
  cout << "   Standard:     O(n^2*m) per iteration, O(n^2 log n) iterations\n";
  cout << "   Karger-Stein: O(n^2 log^3 n) total\n";
  
  return 0;
}
