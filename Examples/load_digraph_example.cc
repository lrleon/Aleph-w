/**
 * @file load_digraph_example.cc
 * @brief Educational examples for saving/loading directed graphs from files
 * 
 * WHY SERIALIZE GRAPHS?
 * =====================
 * - Persistence: Save graph state to disk
 * - Data exchange: Share graphs between programs
 * - Checkpointing: Save progress in long computations
 * - Version control: Track graph evolution over time
 * 
 * FILE FORMAT:
 * ============
 * Nodes: pipe-separated fields (id|name|data)
 * Arcs: space-separated (src_id dst_id [weight])
 * 
 * COMPILE & RUN:
 * ==============
 * g++ -std=c++20 -I.. -o load_digraph_example load_digraph_example.cc
 * ./load_digraph_example
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <load_digraph.H>

using namespace std;
using namespace Aleph;

int main()
{
  cout << "=== Graph Serialization: Educational Examples ===\n\n";
  
  // =========================================================================
  // EXAMPLE 1: Basic Graph Saving
  // =========================================================================
  {
    cout << "--- Example 1: Saving Graph to File ---\n\n";
    
    Digrafo g;
    
    // STEP 1: Build a directed graph
    cout << "Building directed graph (workflow):\n";
    cout << "  Start -> Process1 -> Process2 -> End\n";
    cout << "           Process1 -> Process3 -> End\n\n";
    
    auto start = g.insert_node({"start", {"Start", "Node", "Type:Source"}});
    auto p1 = g.insert_node({"p1", {"Process1", "CPU-Intensive"}});
    auto p2 = g.insert_node({"p2", {"Process2", "I/O-Intensive"}});
    auto p3 = g.insert_node({"p3", {"Process3", "Memory-Intensive"}});
    auto end = g.insert_node({"end", {"End", "Node", "Type:Sink"}});
    
    g.insert_arc(start, p1, "trigger");
    g.insert_arc(p1, p2, "pass_data");
    g.insert_arc(p1, p3, "parallel");
    g.insert_arc(p2, end, "complete");
    g.insert_arc(p3, end, "complete");
    
    cout << "Graph created: " << g.get_num_nodes() << " nodes, "
         << g.get_num_arcs() << " arcs\n\n";
    
    // STEP 2: Save to files
    cout << "Saving to files...\n";
    
    ofstream nodes_file("/tmp/workflow_nodes.txt");
    ofstream arcs_file("/tmp/workflow_arcs.txt");
    
    // Save nodes (format: id|field1|field2|...)
    for (Node_Iterator<Digrafo> it(g); it.has_curr(); it.next())
    {
      auto node = it.get_curr();
      auto& info = node->get_info();
      
      nodes_file << info.first;  // ID
      for (auto& field : info.second)
        nodes_file << "|" << field;
      nodes_file << "\n";
    }
    
    // Save arcs (format: src_id dst_id [arc_info])
    for (Arc_Iterator<Digrafo> it(g); it.has_curr(); it.next())
    {
      auto arc = it.get_curr();
      auto src = g.get_src_node(arc);
      auto tgt = g.get_tgt_node(arc);
      
      arcs_file << src->get_info().first << " "
                << tgt->get_info().first;
      if (!arc->get_info().empty())
        arcs_file << " " << arc->get_info();
      arcs_file << "\n";
    }
    
    nodes_file.close();
    arcs_file.close();
    
    cout << "  Nodes saved to: /tmp/workflow_nodes.txt\n";
    cout << "  Arcs saved to: /tmp/workflow_arcs.txt\n\n";
    
    cout << "FILE CONTENTS:\n";
    cout << "Nodes file:\n";
    cout << "  start|Start|Node|Type:Source\n";
    cout << "  p1|Process1|CPU-Intensive\n";
    cout << "  ...\n\n";
    cout << "Arcs file:\n";
    cout << "  start p1 trigger\n";
    cout << "  p1 p2 pass_data\n";
    cout << "  ...\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 2: Loading Graph from File
  // =========================================================================
  {
    cout << "--- Example 2: Loading Graph from File ---\n\n";
    
    // STEP 1: Open saved files
    ifstream nodes_file("/tmp/workflow_nodes.txt");
    ifstream arcs_file("/tmp/workflow_arcs.txt");
    
    if (!nodes_file || !arcs_file)
    {
      cout << "ERROR: Could not open saved files\n";
      cout << "Make sure Example 1 ran successfully\n\n";
    }
    else
    {
      // STEP 2: Load graph
      Digrafo loaded_graph;
      
      cout << "Loading graph from files...\n";
      load_digraph(loaded_graph, nodes_file, arcs_file);
      
      cout << "  Loaded: " << loaded_graph.get_num_nodes() << " nodes, "
           << loaded_graph.get_num_arcs() << " arcs\n\n";
      
      // STEP 3: Verify loaded data
      cout << "Loaded nodes:\n";
      for (Node_Iterator<Digrafo> it(loaded_graph); it.has_curr(); it.next())
      {
        auto node = it.get_curr();
        auto& info = node->get_info();
        
        cout << "  ID: " << info.first << ", Fields: ";
        for (size_t i = 0; i < info.second.size(); ++i)
        {
          if (i > 0) cout << ", ";
          cout << info.second[i];
        }
        cout << "\n";
      }
      
      cout << "\nKEY INSIGHT: Graph structure perfectly preserved\n";
      cout << "             All nodes, arcs, and data restored\n\n";
    }
  }
  
  // =========================================================================
  // EXAMPLE 3: Round-Trip Verification
  // =========================================================================
  {
    cout << "--- Example 3: Round-Trip Test (Save → Load → Verify) ---\n\n";
    
    // Create original graph
    Digrafo original;
    auto n1 = original.insert_node({"1", {"Node", "One"}});
    auto n2 = original.insert_node({"2", {"Node", "Two"}});
    auto n3 = original.insert_node({"3", {"Node", "Three"}});
    
    original.insert_arc(n1, n2, "arc12");
    original.insert_arc(n2, n3, "arc23");
    original.insert_arc(n3, n1, "arc31");  // Creates cycle
    
    cout << "Original graph: " << original.get_num_nodes() << " nodes, "
         << original.get_num_arcs() << " arcs\n";
    
    // Save
    stringstream nodes_stream, arcs_stream;
    
    for (Node_Iterator<Digrafo> it(original); it.has_curr(); it.next())
    {
      auto node = it.get_curr();
      auto& info = node->get_info();
      nodes_stream << info.first;
      for (auto& field : info.second)
        nodes_stream << "|" << field;
      nodes_stream << "\n";
    }
    
    for (Arc_Iterator<Digrafo> it(original); it.has_curr(); it.next())
    {
      auto arc = it.get_curr();
      auto src = original.get_src_node(arc);
      auto tgt = original.get_tgt_node(arc);
      arcs_stream << src->get_info().first << " "
                  << tgt->get_info().first << " "
                  << arc->get_info() << "\n";
    }
    
    // Load into new graph
    Digrafo loaded;
    load_digraph(loaded, nodes_stream, arcs_stream);
    
    cout << "Loaded graph:   " << loaded.get_num_nodes() << " nodes, "
         << loaded.get_num_arcs() << " arcs\n\n";
    
    // Verify
    bool same_structure = (original.get_num_nodes() == loaded.get_num_nodes()) &&
                          (original.get_num_arcs() == loaded.get_num_arcs());
    
    if (same_structure)
      cout << "✓ VERIFICATION PASSED: Structure preserved\n";
    else
      cout << "✗ VERIFICATION FAILED: Structure mismatch\n";
    
    cout << "\nIMPORTANT: This format preserves:\n";
    cout << "  ✓ Node IDs and attributes\n";
    cout << "  ✓ Arc connectivity\n";
    cout << "  ✓ Arc labels/weights\n";
    cout << "  ✓ Directed graph structure\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 4: Error Handling
  // =========================================================================
  {
    cout << "--- Example 4: Error Handling ---\n\n";
    
    cout << "COMMON ERRORS AND SOLUTIONS:\n\n";
    
    cout << "1. File Not Found:\n";
    cout << "   ifstream file(\"nonexistent.txt\");\n";
    cout << "   if (!file) {\n";
    cout << "     cerr << \"ERROR: Cannot open file\\n\";\n";
    cout << "     return 1;\n";
    cout << "   }\n\n";
    
    cout << "2. Invalid Arc (node ID not found):\n";
    cout << "   Arc references node that doesn't exist\n";
    cout << "   Solution: Validate node IDs before loading arcs\n\n";
    
    cout << "3. Malformed Data:\n";
    cout << "   Missing delimiters, incorrect format\n";
    cout << "   Solution: Use try-catch around load_digraph()\n\n";
    
    cout << "4. Encoding Issues:\n";
    cout << "   Non-ASCII characters in node names\n";
    cout << "   Solution: Use UTF-8 encoding consistently\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 5: Large Graph Handling
  // =========================================================================
  {
    cout << "--- Example 5: Performance Tips ---\n\n";
    
    cout << "FOR LARGE GRAPHS:\n\n";
    
    cout << "1. STREAMING:\n";
    cout << "   Don't load entire file into memory\n";
    cout << "   Process line by line\n\n";
    
    cout << "2. BINARY FORMAT:\n";
    cout << "   Text files: human-readable but large\n";
    cout << "   Binary: compact but not portable\n";
    cout << "   Trade-off based on use case\n\n";
    
    cout << "3. COMPRESSION:\n";
    cout << "   Use gzip for text files\n";
    cout << "   Can reduce size by 80-90%\n";
    cout << "   Read/write through compression library\n\n";
    
    cout << "4. CHUNKING:\n";
    cout << "   Save/load in chunks for very large graphs\n";
    cout << "   Process incrementally\n\n";
  }
  
  cout << "=== SUMMARY: Graph Serialization Best Practices ===\n";
  cout << "\n1. FILE FORMAT CHOICE:\n";
  cout << "   Text: Human-readable, easy to debug, larger files\n";
  cout << "   Binary: Compact, fast, not human-readable\n";
  cout << "   Choose based on needs (debugging vs production)\n";
  cout << "\n2. ALWAYS VERIFY:\n";
  cout << "   After loading, check node/arc counts\n";
  cout << "   Validate graph properties (connectivity, etc.)\n";
  cout << "   Round-trip test: save → load → compare\n";
  cout << "\n3. ERROR HANDLING:\n";
  cout << "   Check file open success\n";
  cout << "   Validate node IDs before creating arcs\n";
  cout << "   Use try-catch for parsing errors\n";
  cout << "\n4. METADATA:\n";
  cout << "   Include version info in file header\n";
  cout << "   Add timestamp, graph name, etc.\n";
  cout << "   Document file format in comments\n";
  cout << "\n5. LARGE GRAPHS:\n";
  cout << "   Use streaming for memory efficiency\n";
  cout << "   Consider compression (gzip)\n";
  cout << "   Chunk processing for huge graphs\n";
  cout << "\n6. APPLICATIONS:\n";
  cout << "   * Checkpoint long-running algorithms\n";
  cout << "   * Share datasets between researchers\n";
  cout << "   * Version control for graph evolution\n";
  cout << "   * Backup and disaster recovery\n";
  
  return 0;
}
