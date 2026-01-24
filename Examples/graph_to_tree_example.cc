/**
 * @file graph_to_tree_example.cc
 * @brief Educational examples for converting graphs to tree structures
 *
 * WHY CONVERT GRAPH TO TREE?
 * ===========================
 * - Trees are simpler than graphs (no cycles)
 * - Tree algorithms are often more efficient
 * - Tree visualization is easier (ntreepic, btreepic)
 * - Spanning trees capture connectivity
 *
 * IMPORTANT DISTINCTION:
 * ======================
 * - graph_to_tree.H: Converts an ACYCLIC graph (already a tree) to Tree_Node
 * - tpl_graph_utils.H: Extracts spanning trees from graphs WITH cycles
 *
 * If your graph has cycles, you must first extract a spanning tree,
 * then convert that spanning tree to Tree_Node format.
 *
 * COMPILE & RUN:
 * ==============
 * Built as part of Aleph-w examples, or manually with the library.
 */

#include <iostream>
#include <tpl_graph.H>
#include <tpl_tree_node.H>
#include <graph_to_tree.H>
#include <tpl_graph_utils.H>
#include <generate_tree.H>

using namespace std;
using namespace Aleph;

// =============================================================================
// Helper: Print Tree_Node structure recursively
// =============================================================================
template <typename T>
void print_tree(Tree_Node<T>* node, int depth = 0)
{
  if (node == nullptr)
    return;

  for (int i = 0; i < depth; ++i)
    cout << "  ";

  cout << node->get_key() << endl;

  auto* child = node->get_left_child();
  while (child != nullptr)
  {
    print_tree(child, depth + 1);
    child = child->get_right_sibling();
  }
}

// =============================================================================
// Helper: Delete Tree_Node structure recursively
// =============================================================================
template <typename T>
void delete_tree(Tree_Node<T>* node)
{
  if (node == nullptr)
    return;

  auto* child = node->get_left_child();
  while (child != nullptr)
  {
    auto* next = child->get_right_sibling();
    delete_tree(child);
    child = next;
  }

  delete node;
}

int main()
{
  cout << "=== Graph to Tree Conversion: Educational Examples ===\n\n";

  // =========================================================================
  // EXAMPLE 1: Convert an Acyclic Graph (Tree) Directly
  // =========================================================================
  {
    cout << "--- Example 1: Converting an Acyclic Graph (Tree) ---\n\n";

    using GT = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
    GT g;

    cout << "Building a tree-structured graph:\n";
    cout << "        A          <- root\n";
    cout << "       / \\\n";
    cout << "      B   C\n";
    cout << "     / \\   \\\n";
    cout << "    D   E   F\n\n";

    auto a = g.insert_node("A");
    auto b = g.insert_node("B");
    auto c = g.insert_node("C");
    auto d = g.insert_node("D");
    auto e = g.insert_node("E");
    auto f = g.insert_node("F");

    // Tree edges only - no cycles!
    g.insert_arc(a, b);
    g.insert_arc(a, c);
    g.insert_arc(b, d);
    g.insert_arc(b, e);
    g.insert_arc(c, f);

    cout << "Graph has " << g.get_num_nodes() << " nodes and "
         << g.get_num_arcs() << " arcs\n";
    cout << "This graph is acyclic (a tree with n-1 = 5 edges)\n\n";

    // Converter functor
    struct CopyString {
      void operator()(GT::Node* gnode, Tree_Node<string>* tnode) {
        tnode->get_key() = gnode->get_info();
      }
    };

    cout << "Converting to Tree_Node structure...\n";

    // Use the functor class to avoid ambiguity
    Graph_To_Tree_Node<GT, string, CopyString> converter;
    auto* tree_root = converter(g, a);

    cout << "Result (Tree_Node hierarchy):\n";
    print_tree(tree_root);

    cout << "\nKEY POINT: graph_to_tree_node() only works on acyclic graphs!\n";
    cout << "           If your graph has cycles, it will throw domain_error.\n\n";

    delete_tree(tree_root);
  }

  // =========================================================================
  // EXAMPLE 2: Graph with Cycles -> Spanning Tree -> Tree_Node
  // =========================================================================
  {
    cout << "--- Example 2: Graph with Cycles (Two-Step Process) ---\n\n";

    using GT = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
    GT g;

    cout << "Building a graph WITH cycles:\n";
    cout << "        A\n";
    cout << "       / \\\n";
    cout << "      B---C      <- B-C creates a cycle A-B-C-A\n";
    cout << "      |   |\n";
    cout << "      D---E      <- D-E creates another cycle\n\n";

    auto a = g.insert_node("A");
    auto b = g.insert_node("B");
    auto c = g.insert_node("C");
    auto d = g.insert_node("D");
    auto e = g.insert_node("E");

    g.insert_arc(a, b);
    g.insert_arc(a, c);
    g.insert_arc(b, c);  // Creates cycle!
    g.insert_arc(b, d);
    g.insert_arc(c, e);
    g.insert_arc(d, e);  // Creates another cycle!

    cout << "Graph has " << g.get_num_nodes() << " nodes and "
         << g.get_num_arcs() << " arcs\n";
    cout << "A tree would have only " << g.get_num_nodes() - 1 << " edges.\n";
    cout << "This graph has cycles (6 edges > 4 needed for tree)\n\n";

    // Step 1: Extract a spanning tree using DFS (returns a new graph)
    cout << "Step 1: Extract DFS spanning tree...\n";
    GT spanning_tree = find_depth_first_spanning_tree<GT>(g, a);

    cout << "Spanning tree has " << spanning_tree.get_num_nodes() << " nodes and "
         << spanning_tree.get_num_arcs() << " arcs\n";
    cout << "(Exactly n-1 = 4 edges, as expected for a tree)\n\n";

    // Step 2: Find the root in the spanning tree
    GT::Node* st_root = nullptr;
    for (GT::Node_Iterator it(spanning_tree); it.has_curr(); it.next())
    {
      if (it.get_curr()->get_info() == "A")
      {
        st_root = it.get_curr();
        break;
      }
    }

    // Step 3: Convert spanning tree to Tree_Node
    cout << "Step 2: Convert spanning tree to Tree_Node...\n";

    struct CopyString {
      void operator()(GT::Node* gnode, Tree_Node<string>* tnode) {
        tnode->get_key() = gnode->get_info();
      }
    };

    Graph_To_Tree_Node<GT, string, CopyString> converter;
    auto* tree_root = converter(spanning_tree, st_root);

    cout << "Result (Tree_Node hierarchy):\n";
    print_tree(tree_root);

    cout << "\nNOTE: The spanning tree removed the cycle-creating edges.\n\n";

    delete_tree(tree_root);
  }

  // =========================================================================
  // EXAMPLE 3: BFS vs DFS Spanning Trees
  // =========================================================================
  {
    cout << "--- Example 3: BFS vs DFS Spanning Trees ---\n\n";

    using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
    GT g;

    cout << "Building a grid graph:\n";
    cout << "    0---1---2\n";
    cout << "    |   |   |\n";
    cout << "    3---4---5\n\n";

    // Insert nodes and keep track of them
    GT::Node* n[6];
    for (int i = 0; i < 6; ++i)
      n[i] = g.insert_node(i);

    // Horizontal edges
    g.insert_arc(n[0], n[1]);
    g.insert_arc(n[1], n[2]);
    g.insert_arc(n[3], n[4]);
    g.insert_arc(n[4], n[5]);
    // Vertical edges
    g.insert_arc(n[0], n[3]);
    g.insert_arc(n[1], n[4]);
    g.insert_arc(n[2], n[5]);

    struct CopyInt {
      void operator()(GT::Node* gnode, Tree_Node<int>* tnode) {
        tnode->get_key() = gnode->get_info();
      }
    };

    // DFS spanning tree
    {
      GT dfs_tree = find_depth_first_spanning_tree<GT>(g, n[0]);

      GT::Node* root = nullptr;
      for (GT::Node_Iterator it(dfs_tree); it.has_curr(); it.next())
        if (it.get_curr()->get_info() == 0)
        {
          root = it.get_curr();
          break;
        }

      Graph_To_Tree_Node<GT, int, CopyInt> converter;
      auto* tree_root = converter(dfs_tree, root);

      cout << "DFS Spanning Tree (tends to be DEEP):\n";
      print_tree(tree_root);
      cout << endl;

      delete_tree(tree_root);
    }

    // BFS spanning tree
    {
      GT bfs_tree = find_breadth_first_spanning_tree<GT>(g, n[0]);

      GT::Node* root = nullptr;
      for (GT::Node_Iterator it(bfs_tree); it.has_curr(); it.next())
        if (it.get_curr()->get_info() == 0)
        {
          root = it.get_curr();
          break;
        }

      Graph_To_Tree_Node<GT, int, CopyInt> converter;
      auto* tree_root = converter(bfs_tree, root);

      cout << "BFS Spanning Tree (tends to be SHALLOW):\n";
      print_tree(tree_root);
      cout << endl;

      delete_tree(tree_root);
    }

    cout << "DFS: Explores one path deeply before backtracking.\n";
    cout << "BFS: Explores all neighbors at distance k before k+1.\n";
    cout << "     BFS tree gives shortest paths from root!\n\n";
  }

  // =========================================================================
  // EXAMPLE 4: Using Graph_To_Tree_Node Functor Class
  // =========================================================================
  {
    cout << "--- Example 4: Functor Class API ---\n\n";

    using GT = List_Graph<Graph_Node<char>, Graph_Arc<int>>;
    GT g;

    auto a = g.insert_node('A');
    auto b = g.insert_node('B');
    auto c = g.insert_node('C');

    g.insert_arc(a, b);
    g.insert_arc(a, c);

    cout << "Simple tree: A -> {B, C}\n\n";

    struct CopyChar {
      void operator()(GT::Node* gnode, Tree_Node<char>* tnode) {
        tnode->get_key() = gnode->get_info();
      }
    };

    Graph_To_Tree_Node<GT, char, CopyChar> converter;
    auto* tree_root = converter(g, a);

    cout << "Converted using Graph_To_Tree_Node functor:\n";
    print_tree(tree_root);

    cout << "\nThe functor class allows storing arc filters.\n\n";

    delete_tree(tree_root);
  }

  // =========================================================================
  // EXAMPLE 5: Generating Tree Specification for ntreepic
  // =========================================================================
  {
    cout << "--- Example 5: Generate ntreepic Specification ---\n\n";

    using GT = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
    GT g;

    auto root = g.insert_node("Root");
    auto l1 = g.insert_node("L1");
    auto l2 = g.insert_node("L2");
    auto l1a = g.insert_node("L1a");
    auto l1b = g.insert_node("L1b");

    g.insert_arc(root, l1);
    g.insert_arc(root, l2);
    g.insert_arc(l1, l1a);
    g.insert_arc(l1, l1b);

    struct CopyString {
      void operator()(GT::Node* gnode, Tree_Node<string>* tnode) {
        tnode->get_key() = gnode->get_info();
      }
    };

    Graph_To_Tree_Node<GT, string, CopyString> converter;
    auto* tree_root = converter(g, root);

    cout << "Tree structure:\n";
    print_tree(tree_root);

    cout << "\nntreepic specification (for visualization):\n";
    cout << "-------------------------------------------\n";
    generate_tree<Tree_Node<string>>(tree_root, cout);
    cout << "-------------------------------------------\n";
    cout << "\nThis output can be used with ntreepic to generate LaTeX.\n\n";

    delete_tree(tree_root);
  }

  cout << "=== SUMMARY ===\n\n";
  cout << "1. Graph_To_Tree_Node: Converts ACYCLIC graphs only\n";
  cout << "   - Input must be a tree (no cycles)\n";
  cout << "   - Throws domain_error if cycles detected\n\n";
  cout << "2. For graphs WITH cycles:\n";
  cout << "   a) Extract spanning tree: find_depth_first_spanning_tree()\n";
  cout << "                          or find_breadth_first_spanning_tree()\n";
  cout << "   b) Convert spanning tree: Graph_To_Tree_Node()(tree, root)\n\n";
  cout << "3. DFS vs BFS spanning trees:\n";
  cout << "   - DFS: Deep, narrow trees\n";
  cout << "   - BFS: Shallow, wide trees (shortest paths from root)\n\n";
  cout << "4. Tree_Node can be visualized with generate_tree() + ntreepic\n\n";

  return 0;
}