/**
 * @file tpl_indexArc_example.cc
 * @brief Educational examples for arc indexing (fast arc lookup)
 * 
 * WHAT IS ARC INDEXING?
 * =====================
 * Maintains a binary search tree of arcs indexed by (source, target) pair
 * Enables O(log m) arc search instead of O(degree) iteration
 * Essential for dense graphs where degree is large
 * 
 * TYPICAL PROBLEM:
 * ================
 * "Does arc exist from node A to node B?"
 * Without index: Iterate through all arcs of A - O(degree)
 * With index: Binary search tree lookup - O(log m)
 * 
 * WHEN TO USE?
 * ============
 * - Dense graphs (many arcs per node)
 * - Frequent arc existence queries
 * - Need fast arc weight/data lookup
 * - Building adjacency-matrix-like behavior
 * 
 * COMPILE & RUN:
 * ==============
 * g++ -std=c++20 -I.. -o tpl_indexArc_example tpl_indexArc_example.cc
 * ./tpl_indexArc_example
 */

#include <iostream>
#include <tpl_indexArc.H>
#include <tpl_graph.H>

using namespace std;
using namespace Aleph;

int main()
{
  cout << "=== Arc Indexing: Educational Examples ===\n\n";
  
  // =========================================================================
  // EXAMPLE 1: Basic Arc Indexing
  // =========================================================================
  {
    cout << "--- Example 1: Fast Arc Lookup ---\n\n";
    
    using GT = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
    GT g;
    
    // STEP 1: Create nodes
    auto nyc = g.insert_node("NYC");
    auto boston = g.insert_node("Boston");
    auto dc = g.insert_node("DC");
    auto philly = g.insert_node("Philadelphia");
    
    // STEP 2: Create arc index for fast lookup
    IndexArc<GT> arc_idx(g);
    
    cout << "Road network: NYC, Boston, DC, Philadelphia\n\n";
    
    // STEP 3: Insert arcs with distances (directed graph)
    cout << "Adding roads (directed):\n";
    arc_idx.insert_in_graph(nyc, boston, 215);      // miles
    arc_idx.insert_in_graph(nyc, philly, 95);
    arc_idx.insert_in_graph(nyc, dc, 225);
    arc_idx.insert_in_graph(philly, dc, 140);
    arc_idx.insert_in_graph(boston, philly, 100);
    arc_idx.insert_in_graph(dc, boston, 440);
    
    cout << "  NYC <-> Boston: 215 miles\n";
    cout << "  NYC <-> Philadelphia: 95 miles\n";
    cout << "  NYC <-> DC: 225 miles\n";
    cout << "  Philadelphia <-> DC: 140 miles\n\n";
    
    // STEP 4: Fast arc queries
    cout << "QUERY: Is there a direct road from NYC to Boston?\n";
    auto road = arc_idx.search(nyc, boston);
    if (road)
      cout << "  YES! Distance: " << road->get_info() << " miles (O(log m) lookup)\n";
    else
      cout << "  NO direct road\n";
    
    cout << "\nQUERY: Is there a direct road from Boston to DC?\n";
    auto no_road = arc_idx.search(boston, dc);
    if (no_road)
      cout << "  YES! Distance: " << no_road->get_info() << " miles\n";
    else
      cout << "  NO direct road (O(log m) lookup)\n";
    
    cout << "\nKEY BENEFIT: Without index, would iterate through all arcs of source node\n";
    cout << "             With index: Direct O(log m) binary search tree lookup\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 2: Weighted Graph Queries
  // =========================================================================
  {
    cout << "--- Example 2: Querying Arc Weights ---\n\n";
    
    using GT = List_Graph<Graph_Node<string>, Graph_Arc<double>>;
    GT g;
    IndexArc<GT> arc_idx(g);
    
    // Build network topology
    auto server1 = g.insert_node("Server1");
    auto server2 = g.insert_node("Server2");
    auto server3 = g.insert_node("Server3");
    auto router = g.insert_node("Router");
    
    cout << "Network topology with bandwidth (Gbps):\n";
    
    // Add connections with bandwidth (directed)
    arc_idx.insert_in_graph(server1, router, 10.0);
    arc_idx.insert_in_graph(server2, router, 10.0);
    arc_idx.insert_in_graph(server3, router, 10.0);
    
    cout << "  All servers connected to router at 10 Gbps\n\n";
    
    // Query specific connections
    cout << "Checking connection bandwidth:\n";
    auto link1 = arc_idx.search(server1, router);
    if (link1)
      cout << "  Server1 -> Router: " << link1->get_info() << " Gbps\n";
    
    auto link2 = arc_idx.search(server2, router);
    if (link2)
      cout << "  Server2 -> Router: " << link2->get_info() << " Gbps\n";
    
    // Check if direct server-to-server link exists
    auto direct = arc_idx.search(server1, server2);
    if (direct)
      cout << "  Server1 -> Server2: Direct link\n";
    else
      cout << "  Server1 -> Server2: No direct link (must route through router)\n";
    
    cout << "\nAPPLICATION: Network topology queries, routing decisions\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 3: Detecting Parallel Arcs
  // =========================================================================
  {
    cout << "--- Example 3: Parallel Arc Detection ---\n\n";
    
    using GT = List_Graph<Graph_Node<string>, Graph_Arc<string>>;
    GT g;
    IndexArc<GT> arc_idx(g);
    
    auto cityA = g.insert_node("City A");
    auto cityB = g.insert_node("City B");
    
    cout << "Transportation network between two cities:\n";
    
    // Insert first arc
    arc_idx.insert_in_graph(cityA, cityB, "Highway");
    cout << "  Added: Highway\n";
    
    // Check before adding another
    if (arc_idx.search(cityA, cityB))
    {
      cout << "  WARNING: Arc A->B already exists!\n";
      cout << "  Cannot add parallel arc with IndexArc (simple graph assumption)\n";
    }
    
    cout << "\nIMPORTANT: IndexArc assumes SIMPLE GRAPH (no parallel arcs)\n";
    cout << "           One arc per (source, target) pair\n";
    cout << "           Use multi-graph if parallel arcs needed\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 4: Performance Comparison
  // =========================================================================
  {
    cout << "--- Example 4: Performance Analysis ---\n\n";
    
    using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
    GT g;
    IndexArc<GT> arc_idx(g);
    
    const int N = 20;  // nodes
    cout << "Creating dense graph: " << N << " nodes...\n";
    
    // Create nodes
    DynList<GT::Node*> nodes;
    for (int i = 0; i < N; ++i)
      nodes.append(g.insert_node(i));
    
    // Create dense connections (directed, avoid duplicates)
    int arc_count = 0;
    for (int i = 0; i < N; ++i)
      for (int j = i + 1; j < N; ++j)  // j > i to avoid duplicates
        if ((i + j) % 2 == 0)
        {
          arc_idx.insert_in_graph(nodes[i], nodes[j], i * 10 + j);
          arc_count++;
        }
    
    cout << "  Nodes: " << N << "\n";
    cout << "  Arcs: " << arc_count << "\n";
    cout << "  Average degree: " << (arc_count / N) << "\n\n";
    
    cout << "SEARCH COMPLEXITY:\n\n";
    
    cout << "Without Index (iterate outgoing arcs):\n";
    cout << "  Best:    O(1)       - arc is first\n";
    cout << "  Average: O(deg/2)   - scan half of arcs\n";
    cout << "  Worst:   O(deg)     - scan all arcs\n";
    cout << "  For degree=" << (arc_count/N) << ": ~" << (arc_count/N/2) << " comparisons\n\n";
    
    cout << "With IndexArc:\n";
    cout << "  All cases: O(log m) - binary search tree\n";
    cout << "  For m=" << arc_count << ": ~" << (int)log2(arc_count) << " comparisons\n\n";
    
    double speedup = (arc_count / N / 2.0) / log2(arc_count);
    cout << "SPEEDUP: ~" << (int)speedup << "x faster for arc queries!\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 5: Dynamic Updates
  // =========================================================================
  {
    cout << "--- Example 5: Dynamic Arc Management ---\n\n";
    
    using GT = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
    GT g;
    IndexArc<GT> arc_idx(g);
    
    auto a = g.insert_node("A");
    auto b = g.insert_node("B");
    auto c = g.insert_node("C");
    
    cout << "Building graph dynamically...\n";
    
    // Add arcs
    arc_idx.insert_in_graph(a, b, 10);
    arc_idx.insert_in_graph(b, c, 20);
    cout << "  Added: A->B(10), B->C(20)\n";
    
    // Query
    auto arc_ab = arc_idx.search(a, b);
    if (arc_ab)
      cout << "  Found arc A->B with weight " << arc_ab->get_info() << "\n";
    
    // Add more arcs
    arc_idx.insert_in_graph(a, c, 30);
    cout << "  Added: A->C(30)\n";
    
    // All arcs remain searchable in O(log m)
    cout << "\nIndex automatically maintains balance\n";
    cout << "All arc queries remain O(log m) after updates\n\n";
  }
  
  cout << "=== SUMMARY: IndexArc Best Practices ===\n";
  cout << "\n1. WHEN TO USE:\n";
  cout << "   ✓ Dense graphs (high average degree)\n";
  cout << "   ✓ Frequent 'does arc exist?' queries\n";
  cout << "   ✓ Need arc weight/data lookup\n";
  cout << "   ✓ Adjacency matrix-like access pattern\n";
  cout << "\n2. DESIGN PATTERNS:\n";
  cout << "   - Road networks: Query route existence\n";
  cout << "   - Social graphs: Check friendship status\n";
  cout << "   - Network topology: Verify link existence\n";
  cout << "   - Dependency graphs: Check direct dependency\n";
  cout << "\n3. LIMITATIONS:\n";
  cout << "   ✗ Assumes SIMPLE GRAPH (no parallel arcs)\n";
  cout << "   ✗ Extra memory for index: O(m)\n";
  cout << "   ✗ Slightly slower insertions: O(log m) vs O(1)\n";
  cout << "\n4. PERFORMANCE:\n";
  cout << "   Without index: O(degree) arc iteration\n";
  cout << "   With index:    O(log m) tree lookup\n";
  cout << "   Speedup:       degree / log(m) times faster\n";
  cout << "\n5. COMPLEXITY SUMMARY:\n";
  cout << "   Insert arc:  O(log m) - add to tree\n";
  cout << "   Search arc:  O(log m) - tree lookup\n";
  cout << "   Remove arc:  O(log m) - tree deletion\n";
  cout << "   Memory:      O(m)     - tree overhead\n";
  cout << "\n6. BEST PRACTICES:\n";
  cout << "   - Use with IndexNode for complete indexing\n";
  cout << "   - Check arc existence before insert\n";
  cout << "   - Ideal for graphs with many arcs per node\n";
  cout << "   - Not needed for sparse graphs (low degree)\n";
  
  return 0;
}
