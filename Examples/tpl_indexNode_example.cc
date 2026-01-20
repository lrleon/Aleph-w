/**
 * @file tpl_indexNode_example.cc
 * @brief Educational examples for indexed nodes (O(log n) node search)
 * 
 * WHAT IS NODE INDEXING?
 * ======================
 * Maintains a binary search tree of graph nodes indexed by their values
 * Enables O(log n) search instead of O(n) linear scan
 * Automatically prevents duplicate node values
 * 
 * WHEN TO USE?
 * ============
 * - Graphs with unique node identifiers (IDs, names, keys)
 * - Frequent node lookups by value
 * - Need to enforce uniqueness constraint
 * - Working with large graphs (n > 100 nodes)
 * 
 * COMPILE & RUN:
 * ==============
 * g++ -std=c++20 -I.. -o tpl_indexNode_example tpl_indexNode_example.cc
 * ./tpl_indexNode_example
 */

#include <iostream>
#include <tpl_indexNode.H>
#include <tpl_graph.H>

using namespace std;
using namespace Aleph;

int main()
{
  cout << "=== Node Indexing: Educational Examples ===\n\n";
  
  // =========================================================================
  // EXAMPLE 1: Basic Node Indexing
  // =========================================================================
  {
    cout << "--- Example 1: Fast Node Lookup ---\n\n";
    
    using GT = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
    GT g;
    
    // STEP 1: Create node index
    // IndexNode maintains a search tree of nodes by their value
    IndexNode<GT> node_idx(g);
    
    cout << "Creating social network with unique usernames...\n";
    
    // STEP 2: Insert nodes through index
    // This adds to BOTH graph and search tree
    auto alice = node_idx.insert_in_graph("Alice");
    auto bob = node_idx.insert_in_graph("Bob");
    auto charlie = node_idx.insert_in_graph("Charlie");
    auto diana = node_idx.insert_in_graph("Diana");
    (void)alice;
    (void)bob;
    (void)charlie;
    (void)diana;
    
    cout << "Added users: Alice, Bob, Charlie, Diana\n\n";
    
    // STEP 3: Fast O(log n) search by username
    cout << "SEARCH DEMONSTRATIONS:\n";
    
    auto found = node_idx.search("Charlie");
    if (found)
      cout << "  search(\"Charlie\"): FOUND (O(log n) time)\n";
    
    auto not_found = node_idx.search("Eve");
    if (not_found)
      cout << "  search(\"Eve\"): FOUND\n";
    else
      cout << "  search(\"Eve\"): NOT FOUND (O(log n) time)\n";
    
    cout << "\nWITHOUT INDEXING: Would scan all " << g.get_num_nodes() 
         << " nodes linearly\n";
    cout << "WITH INDEXING: Binary search tree lookup\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 2: Custom Comparator (Advanced)
  // =========================================================================
  {
    cout << "--- Example 2: Custom Node Comparison ---\n\n";
    
    // Define custom struct for nodes
    struct User {
      int id;
      string name;
      int age;
      
      bool operator<(const User& other) const {
        return id < other.id;  // Compare by ID
      }
    };
    
    using GT = List_Graph<Graph_Node<User>, Graph_Arc<int>>;
    GT g;
    
    // Custom comparator to index by user ID
    struct UserIdCmp {
      bool operator()(GT::Node* a, GT::Node* b) const {
        return a->get_info().id < b->get_info().id;
      }
    };
    
    // Create index with custom comparator
    IndexNode<GT, UserIdCmp> user_idx(g);
    
    cout << "User database indexed by ID:\n";
    
    // Insert users
    auto u1 = user_idx.insert_in_graph({1001, "Alice", 30});
    auto u2 = user_idx.insert_in_graph({1003, "Bob", 25});
    auto u3 = user_idx.insert_in_graph({1002, "Charlie", 35});
    (void)u1;
    (void)u2;
    (void)u3;
    
    cout << "  Added: Alice(1001), Bob(1003), Charlie(1002)\n";
    cout << "  Index maintains sorted order by ID\n\n";
    
    // Search by creating a probe node
    User probe{1002, "", 0};  // Only ID matters for search
    GT::Node probe_node(probe);
    
    auto found = user_idx.search(&probe_node);
    if (found)
      cout << "Found user 1002: " << found->get_info().name << "\n";
    
    cout << "\nKEY FEATURE: Can index by ANY field (ID, name, email, etc.)\n";
    cout << "             Just provide appropriate comparator\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 3: Duplicate Prevention
  // =========================================================================
  {
    cout << "--- Example 3: Enforcing Uniqueness ---\n\n";
    
    using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
    GT g;
    IndexNode<GT> node_idx(g);
    
    cout << "Inserting node with ID 100...\n";
    auto n1 = node_idx.insert_in_graph(100);
    (void)n1;
    cout << "  Success! Node count: " << g.get_num_nodes() << "\n\n";
    
    cout << "Attempting to insert duplicate ID 100...\n";
    // Check first to avoid duplicate
    if (node_idx.search(100))
    {
      cout << "  PREVENTED: ID 100 already exists\n";
      cout << "  Node count unchanged: " << g.get_num_nodes() << "\n";
    }
    
    cout << "\nREAL-WORLD APPLICATIONS:\n";
    cout << "  * Database primary keys\n";
    cout << "  * User registration (unique usernames/emails)\n";
    cout << "  * File systems (unique paths)\n";
    cout << "  * Network routing (unique IP addresses)\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 4: Performance Comparison
  // =========================================================================
  {
    cout << "--- Example 4: Performance Analysis ---\n\n";
    
    using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
    GT g;
    IndexNode<GT> idx(g);
    
    const int N = 100;
    
    cout << "Creating graph with " << N << " nodes...\n";
    for (int i = 0; i < N; ++i)
      idx.insert_in_graph(i);
    
    cout << "\nSEARCH COMPLEXITY COMPARISON:\n\n";
    
    cout << "Linear Search (without index):\n";
    cout << "  Best case:  O(1)   - found immediately\n";
    cout << "  Average:    O(n/2) - scan half the nodes\n";
    cout << "  Worst case: O(n)   - scan all nodes\n";
    cout << "  For n=100:  ~50 comparisons average\n\n";
    
    cout << "Indexed Search (with IndexNode):\n";
    cout << "  All cases: O(log n) - binary search tree\n";
    cout << "  For n=100: ~7 comparisons always\n\n";
    
    cout << "SPEEDUP: 50/7 = ~7x faster on average!\n";
    cout << "         Grows with graph size: O(n) vs O(log n)\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 5: Range Queries (Bonus)
  // =========================================================================
  {
    cout << "--- Example 5: Ordered Traversal ---\n\n";
    
    using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
    GT g;
    IndexNode<GT> idx(g);
    
    // Insert nodes in random order
    int values[] = {50, 30, 70, 20, 40, 60, 80};
    cout << "Inserting in order: ";
    for (int v : values)
    {
      idx.insert_in_graph(v);
      cout << v << " ";
    }
    cout << "\n\n";
    
    // Index maintains sorted order internally
    cout << "Internal index maintains BST order:\n";
    cout << "  Allows efficient range queries\n";
    cout << "  Enables sorted iteration\n";
    cout << "  Supports predecessor/successor queries\n\n";
    
    cout << "ADVANCED FEATURES (if needed):\n";
    cout << "  * Find all nodes in range [a, b]\n";
    cout << "  * Find k smallest/largest values\n";
    cout << "  * Iterate nodes in sorted order\n\n";
  }
  
  cout << "=== SUMMARY: IndexNode Best Practices ===\n";
  cout << "\n1. WHEN TO USE:\n";
  cout << "   ✓ Need fast node lookup by value\n";
  cout << "   ✓ Graph has unique node identifiers\n";
  cout << "   ✓ Frequent search operations\n";
  cout << "   ✓ Large graphs (n > 100)\n";
  cout << "\n2. DESIGN PATTERNS:\n";
  cout << "   - Social networks: Index by username\n";
  cout << "   - Databases: Index by primary key\n";
  cout << "   - IP networks: Index by IP address\n";
  cout << "   - File systems: Index by path\n";
  cout << "\n3. PERFORMANCE TIPS:\n";
  cout << "   - Always search before insert (check duplicates)\n";
  cout << "   - Use custom comparator for complex types\n";
  cout << "   - Index is self-balancing (treaps)\n";
  cout << "\n4. MEMORY OVERHEAD:\n";
  cout << "   - Extra O(n) space for search tree\n";
  cout << "   - Typical: ~24 bytes per node pointer\n";
  cout << "   - Negligible for graphs > 100 nodes\n";
  cout << "\n5. COMPLEXITY SUMMARY:\n";
  cout << "   Insert:  O(log n) - add to tree\n";
  cout << "   Search:  O(log n) - tree lookup\n";
  cout << "   Remove:  O(log n) - tree deletion\n";
  cout << "   Memory:  O(n)     - tree overhead\n";
  
  return 0;
}
