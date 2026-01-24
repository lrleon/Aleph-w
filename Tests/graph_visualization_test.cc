
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
 * @file graph_visualization_test.cc
 * @brief Tests for graph visualization utilities
 *
 * Tests for:
 * - generate_graph.H (Graphviz DOT generation)
 * - graph_to_tree.H (Graph to Tree_Node conversion)
 * - generate_spanning_tree_picture.H (Spanning tree shading)
 */

#include <gtest/gtest.h>
#include <sstream>
#include <string>

#include <tpl_graph.H>
#include <tpl_graph_utils.H>
#include <generate_graph.H>
#include <graph_to_tree.H>
#include <generate_spanning_tree_picture.H>
#include <generate_tree.H>
#include <tpl_tree_node.H>

using namespace std;
using namespace testing;
using namespace Aleph;

// ============================================================================
// Test Fixtures
// ============================================================================

/// Simple graph for testing
class SimpleGraphTest : public Test
{
protected:
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using Node = Graph::Node;
  using Arc = Graph::Arc;

  Graph g;
  Node* n1;
  Node* n2;
  Node* n3;

  void SetUp() override
  {
    n1 = g.insert_node(1);
    n2 = g.insert_node(2);
    n3 = g.insert_node(3);
    g.insert_arc(n1, n2, 10);
    g.insert_arc(n2, n3, 20);
    g.insert_arc(n1, n3, 30);
  }
};

/// Simple digraph for testing
class SimpleDigraphTest : public Test
{
protected:
  using Digraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using Node = Digraph::Node;
  using Arc = Digraph::Arc;

  Digraph g;
  Node* n1;
  Node* n2;
  Node* n3;

  void SetUp() override
  {
    n1 = g.insert_node(1);
    n2 = g.insert_node(2);
    n3 = g.insert_node(3);
    g.insert_arc(n1, n2, 10);
    g.insert_arc(n2, n3, 20);
    g.insert_arc(n1, n3, 30);
  }
};

/// Tree graph for graph_to_tree conversion testing
class TreeGraphTest : public Test
{
protected:
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using Node = Graph::Node;
  using Arc = Graph::Arc;

  Graph tree;
  Node* root;
  Node* child1;
  Node* child2;
  Node* grandchild;

  void SetUp() override
  {
    // Create a simple tree:
    //       root(1)
    //      /       |
    //   child1(2)  child2(3)
    //      |
    //   grandchild(4)
    root = tree.insert_node(1);
    child1 = tree.insert_node(2);
    child2 = tree.insert_node(3);
    grandchild = tree.insert_node(4);

    tree.insert_arc(root, child1, 0);
    tree.insert_arc(root, child2, 0);
    tree.insert_arc(child1, grandchild, 0);
  }
};

// ============================================================================
// generate_graph.H Tests - Graphviz DOT generation
// ============================================================================

TEST_F(SimpleGraphTest, GenerateGraphvizContainsGraphKeyword)
{
  ostringstream out;
  To_Graphviz<Graph>()(g, out);
  string result = out.str();

  // Should contain "graph {" for undirected graphs
  EXPECT_NE(result.find("graph {"), string::npos);
  // Should NOT contain "digraph"
  EXPECT_EQ(result.find("digraph {"), string::npos);
}

TEST_F(SimpleDigraphTest, GenerateGraphvizContainsDigraphKeyword)
{
  ostringstream out;
  To_Graphviz<Digraph>()(g, out);
  string result = out.str();

  // Should contain "digraph {" for directed graphs
  EXPECT_NE(result.find("digraph {"), string::npos);
}

TEST_F(SimpleGraphTest, GenerateGraphvizContainsAllNodes)
{
  ostringstream out;
  To_Graphviz<Graph>()(g, out);
  string result = out.str();

  // Should contain labels for all nodes
  EXPECT_NE(result.find("label = \"1\""), string::npos);
  EXPECT_NE(result.find("label = \"2\""), string::npos);
  EXPECT_NE(result.find("label = \"3\""), string::npos);
}

TEST_F(SimpleGraphTest, GenerateGraphvizContainsArcs)
{
  ostringstream out;
  To_Graphviz<Graph>()(g, out);
  string result = out.str();

  // Should contain "--" for undirected graph arcs
  EXPECT_NE(result.find("--"), string::npos);
}

TEST_F(SimpleDigraphTest, GenerateGraphvizContainsDirectedArcs)
{
  ostringstream out;
  To_Graphviz<Digraph>()(g, out);
  string result = out.str();

  // Should contain "->" for directed graph arcs
  EXPECT_NE(result.find("->"), string::npos);
}

TEST_F(SimpleGraphTest, GenerateGraphvizRespectRankdir)
{
  ostringstream out;
  To_Graphviz<Graph>()(g, out, Dft_Node_Attr<Graph>(), Dft_Arc_Attr<Graph>(), "TB");
  string result = out.str();

  EXPECT_NE(result.find("rankdir = TB"), string::npos);
}

TEST_F(SimpleGraphTest, GenerateGraphvizClosesWithBrace)
{
  ostringstream out;
  To_Graphviz<Graph>()(g, out);
  string result = out.str();

  // Should end with closing brace
  size_t lastBrace = result.rfind('}');
  EXPECT_NE(lastBrace, string::npos);
}

// Test default attribute functors
TEST_F(SimpleGraphTest, DftNodeAttrOutputsLabel)
{
  ostringstream out;
  Dft_Node_Attr<Graph>()(g, n1, out);
  string result = out.str();

  EXPECT_NE(result.find("label"), string::npos);
  EXPECT_NE(result.find("1"), string::npos);
}

TEST_F(SimpleGraphTest, DftArcAttrOutputsLabel)
{
  auto arc = g.get_first_arc();
  ostringstream out;
  Dft_Arc_Attr<Graph>()(g, arc, out);
  string result = out.str();

  EXPECT_NE(result.find("label"), string::npos);
}

// Test Dummy_Attr always returns false
TEST_F(SimpleGraphTest, DummyAttrReturnsFalse)
{
  Dummy_Attr<Graph> dummy;
  EXPECT_FALSE(dummy(n1));
  EXPECT_FALSE(dummy(g.get_first_arc()));
}

// ============================================================================
// generate_spanning_tree_picture.H Tests
// ============================================================================

TEST_F(SimpleGraphTest, ShadeSpanNodeReturnsEmptyForNullCookie)
{
  NODE_COOKIE(n1) = nullptr;
  Shade_Span_Node<Graph> shader;
  EXPECT_EQ(shader(n1), "");
}

TEST_F(SimpleGraphTest, ShadeSpanNodeReturnsShadowForNonNullCookie)
{
  NODE_COOKIE(n1) = n2; // non-null cookie
  Shade_Span_Node<Graph> shader;
  EXPECT_EQ(shader(n1), "SHADOW-NODE");
}

TEST_F(SimpleGraphTest, ShadeSpanArcReturnsArcForNullCookie)
{
  auto arc = g.get_first_arc();
  ARC_COOKIE(arc) = nullptr;
  Shade_Span_Arc<Graph> shader;
  EXPECT_EQ(shader(arc), "ARC");
}

TEST_F(SimpleGraphTest, ShadeSpanArcReturnsShadowForNonNullCookie)
{
  auto arc = g.get_first_arc();
  ARC_COOKIE(arc) = n1; // non-null cookie
  Shade_Span_Arc<Graph> shader;
  EXPECT_EQ(shader(arc), "SHADOW-ARC");
}

// ============================================================================
// graph_to_tree.H Tests
// ============================================================================

/// Converter from Graph::Node to int for Tree_Node
struct IntConvert
{
  using GraphNode = List_Graph<Graph_Node<int>, Graph_Arc<int>>::Node;
  void operator()(GraphNode* gnode, Tree_Node<int>* tnode)
  {
    tnode->get_key() = gnode->get_info();
  }
};

TEST_F(TreeGraphTest, GraphToTreeNodeCreatesRoot)
{
  Tree_Node<int>* treeRoot = 
    Graph_To_Tree_Node<Graph, int, IntConvert>()(tree, root);

  ASSERT_NE(treeRoot, nullptr);
  EXPECT_EQ(treeRoot->get_key(), 1);

  destroy_tree(treeRoot);
}

TEST_F(TreeGraphTest, GraphToTreeNodeCreatesChildren)
{
  Tree_Node<int>* treeRoot = 
    Graph_To_Tree_Node<Graph, int, IntConvert>()(tree, root);

  // Root should have children
  auto* firstChild = treeRoot->get_left_child();
  ASSERT_NE(firstChild, nullptr);

  // Count children
  int childCount = 0;
  for (auto* child = firstChild; child != nullptr; 
       child = child->get_right_sibling())
    childCount++;

  EXPECT_EQ(childCount, 2); // child1 and child2

  destroy_tree(treeRoot);
}

TEST_F(TreeGraphTest, GraphToTreeNodePreservesStructure)
{
  Tree_Node<int>* treeRoot = 
    Graph_To_Tree_Node<Graph, int, IntConvert>()(tree, root);

  // Find all node values
  set<int> values;
  function<void(Tree_Node<int>*)> collectValues = [&](Tree_Node<int>* node) {
    if (node == nullptr) return;
    values.insert(node->get_key());
    for (auto* child = node->get_left_child(); child != nullptr;
         child = child->get_right_sibling())
      collectValues(child);
  };

  collectValues(treeRoot);

  // Should contain all original values
  EXPECT_TRUE(values.count(1) > 0);
  EXPECT_TRUE(values.count(2) > 0);
  EXPECT_TRUE(values.count(3) > 0);
  EXPECT_TRUE(values.count(4) > 0);
  EXPECT_EQ(values.size(), 4u);

  destroy_tree(treeRoot);
}

TEST_F(TreeGraphTest, GraphToTreeNodeThrowsOnCyclicGraph)
{
  // Add an arc to create a cycle
  tree.insert_arc(grandchild, root, 0);

  using Converter = Graph_To_Tree_Node<Graph, int, IntConvert>;
  EXPECT_THROW(Converter()(tree, root), std::domain_error);
}

// ============================================================================
// generate_tree.H Tests (additional to existing tests)
// ============================================================================

TEST_F(TreeGraphTest, GenerateTreeOutputsRoot)
{
  Tree_Node<int>* treeRoot = 
    Graph_To_Tree_Node<Graph, int, IntConvert>()(tree, root);

  ostringstream out;
  generate_tree(treeRoot, out);
  string result = out.str();

  // Should start with Root
  EXPECT_EQ(result.find("Root"), 0u);

  destroy_tree(treeRoot);
}

TEST_F(TreeGraphTest, GenerateTreeOutputsDeweyNotation)
{
  Tree_Node<int>* treeRoot = 
    Graph_To_Tree_Node<Graph, int, IntConvert>()(tree, root);

  ostringstream out;
  generate_tree(treeRoot, out);
  string result = out.str();

  // Should contain Node entries with Dewey notation
  EXPECT_NE(result.find("Node "), string::npos);

  destroy_tree(treeRoot);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(TreeGraphTest, FullPipelineGraphToTreeToOutput)
{
  // Convert graph to tree
  Tree_Node<int>* treeRoot = 
    Graph_To_Tree_Node<Graph, int, IntConvert>()(tree, root);

  // Generate tree output
  ostringstream out;
  generate_tree(treeRoot, out);
  string result = out.str();

  // Verify output is non-empty and has expected structure
  EXPECT_FALSE(result.empty());
  EXPECT_NE(result.find("Root"), string::npos);
  EXPECT_NE(result.find("\"1\""), string::npos); // Root value

  destroy_tree(treeRoot);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(EmptyGraphTest, GenerateGraphvizHandlesEmptyGraph)
{
  List_Graph<Graph_Node<int>, Graph_Arc<int>> emptyGraph;

  ostringstream out;
  To_Graphviz<List_Graph<Graph_Node<int>, Graph_Arc<int>>>()(emptyGraph, out);
  string result = out.str();

  // Should still produce valid DOT structure
  EXPECT_NE(result.find("graph {"), string::npos);
  EXPECT_NE(result.find("}"), string::npos);
}

TEST(SingleNodeGraphTest, GenerateGraphvizHandlesSingleNode)
{
  List_Graph<Graph_Node<int>, Graph_Arc<int>> g;
  g.insert_node(42);

  ostringstream out;
  To_Graphviz<List_Graph<Graph_Node<int>, Graph_Arc<int>>>()(g, out);
  string result = out.str();

  EXPECT_NE(result.find("42"), string::npos);
}

TEST(SingleNodeTreeTest, GraphToTreeNodeHandlesSingleNode)
{
  List_Graph<Graph_Node<int>, Graph_Arc<int>> g;
  auto* node = g.insert_node(42);

  struct Conv {
    void operator()(decltype(node) gn, Tree_Node<int>* tn) {
      tn->get_key() = gn->get_info();
    }
  };

  Tree_Node<int>* treeRoot = 
    Graph_To_Tree_Node<List_Graph<Graph_Node<int>, Graph_Arc<int>>, int, Conv>()(g, node);

  ASSERT_NE(treeRoot, nullptr);
  EXPECT_EQ(treeRoot->get_key(), 42);
  EXPECT_EQ(treeRoot->get_left_child(), nullptr);

  destroy_tree(treeRoot);
}

// ============================================================================
// Type Traits Tests
// ============================================================================

TEST(TypeTraitsTest, DftWriteConvertsToString)
{
  Tree_Node<int> node;
  node.get_key() = 123;

  Dft_Write<Tree_Node<int>> writer;
  string result = writer(&node);

  EXPECT_EQ(result, "123");
}

// ============================================================================
// Additional generate_graph.H Tests
// ============================================================================

// Test digraph_graphviz() - always outputs digraph format
TEST_F(SimpleGraphTest, DigraphGraphvizForcesDigraphFormat)
{
  // Custom functors for digraph_graphviz
  struct NodeAttr {
    void operator()(const Graph&, Node* n, ostream& out) {
      out << "label=\"" << n->get_info() << "\"";
    }
  };
  struct ArcAttr {
    void operator()(const Graph&, Arc* a, ostream& out) {
      out << "label=\"" << a->get_info() << "\"";
    }
  };

  ostringstream out;
  digraph_graphviz<Graph, NodeAttr, ArcAttr, Dft_Show_Node<Graph>, Dft_Show_Arc<Graph>>
    (g, out);
  string result = out.str();

  // Should contain "digraph {" even for undirected graph
  EXPECT_NE(result.find("digraph {"), string::npos);
  EXPECT_NE(result.find("->"), string::npos);
}

// Test Generate_Graphviz struct
TEST_F(SimpleGraphTest, GenerateGraphvizStructWorks)
{
  struct WriteNode {
    string operator()(Node* n) const { return to_string(n->get_info()); }
  };
  struct WriteArc {
    string operator()(Arc* a) const { return to_string(a->get_info()); }
  };

  ostringstream out;
  Generate_Graphviz<Graph, WriteNode, WriteArc>()(g, out);
  string result = out.str();

  EXPECT_NE(result.find("graph {"), string::npos);
  EXPECT_NE(result.find("rankdir = TB"), string::npos);
}

// Test custom rankdir values
TEST_F(SimpleGraphTest, GenerateGraphvizDifferentRankdirs)
{
  const vector<string> rankdirs = {"TB", "BT", "LR", "RL"};

  for (const auto& dir : rankdirs) {
    ostringstream out;
    To_Graphviz<Graph>()(g, out, Dft_Node_Attr<Graph>(), Dft_Arc_Attr<Graph>(), dir);
    string result = out.str();
    EXPECT_NE(result.find("rankdir = " + dir), string::npos) 
      << "Failed for rankdir: " << dir;
  }
}

// Test with string node info
TEST(StringNodeGraphTest, GenerateGraphvizWithStringNodes)
{
  using SGraph = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
  SGraph g;
  auto* a = g.insert_node("Alpha");
  auto* b = g.insert_node("Beta");
  g.insert_arc(a, b, 1);

  ostringstream out;
  To_Graphviz<SGraph>()(g, out);
  string result = out.str();

  EXPECT_NE(result.find("Alpha"), string::npos);
  EXPECT_NE(result.find("Beta"), string::npos);
}

// Test with double arc weights
TEST(DoubleArcGraphTest, GenerateGraphvizWithDoubleArcs)
{
  using DGraph = List_Graph<Graph_Node<int>, Graph_Arc<double>>;
  DGraph g;
  auto* a = g.insert_node(1);
  auto* b = g.insert_node(2);
  g.insert_arc(a, b, 3.14159);

  ostringstream out;
  To_Graphviz<DGraph>()(g, out);
  string result = out.str();

  EXPECT_NE(result.find("3.14"), string::npos);
}

// Test DAG with rank_graphviz
TEST(DAGTest, RankGraphvizCreatesSubgraphs)
{
  using DAG = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  DAG g;
  
  // Create a simple DAG: 1 -> 2 -> 3
  auto* n1 = g.insert_node(1);
  auto* n2 = g.insert_node(2);
  auto* n3 = g.insert_node(3);
  g.insert_arc(n1, n2, 0);
  g.insert_arc(n2, n3, 0);

  struct NodeAttr {
    void operator()(const DAG&, DAG::Node* n, ostream& out) {
      out << "label=\"" << n->get_info() << "\"";
    }
  };
  struct ArcAttr {
    void operator()(const DAG&, DAG::Arc*, ostream& out) {
      out << "";
    }
  };

  ostringstream out;
  size_t numRanks = rank_graphviz<DAG, NodeAttr, ArcAttr, 
                                   Dft_Show_Node<DAG>, Dft_Show_Arc<DAG>>(g, out);

  string result = out.str();

  // Should have multiple ranks
  EXPECT_GE(numRanks, 1u);
  // Should contain subgraph declarations
  EXPECT_NE(result.find("subgraph"), string::npos);
  EXPECT_NE(result.find("rank"), string::npos);
}

// ============================================================================
// Additional generate_tree.H Tests
// ============================================================================

// Test generate_forest with multiple trees
TEST(ForestTest, GenerateForestOutputsMultipleTrees)
{
  // Create a forest of 3 single-node trees as siblings
  Tree_Node<int>* tree1 = new Tree_Node<int>;
  Tree_Node<int>* tree2 = new Tree_Node<int>;
  Tree_Node<int>* tree3 = new Tree_Node<int>;

  tree1->get_key() = 100;
  tree2->get_key() = 200;
  tree3->get_key() = 300;

  // Link as siblings
  tree1->insert_right_sibling(tree2);
  tree2->insert_right_sibling(tree3);

  ostringstream out;
  generate_forest(tree1, out);
  string result = out.str();

  // Should contain all three roots
  EXPECT_NE(result.find("100"), string::npos);
  EXPECT_NE(result.find("200"), string::npos);
  EXPECT_NE(result.find("300"), string::npos);

  // Should have multiple "Root" entries
  size_t firstRoot = result.find("Root");
  size_t secondRoot = result.find("Root", firstRoot + 1);
  size_t thirdRoot = result.find("Root", secondRoot + 1);
  
  EXPECT_NE(firstRoot, string::npos);
  EXPECT_NE(secondRoot, string::npos);
  EXPECT_NE(thirdRoot, string::npos);

  // Clean up - siblings are not automatically deleted
  delete tree1;
  delete tree2;
  delete tree3;
}

// Test custom Write functor
TEST(CustomWriteTest, GenerateTreeWithCustomWriter)
{
  struct HexWriter {
    string operator()(Tree_Node<int>* p) {
      ostringstream ss;
      ss << "0x" << hex << p->get_key();
      return ss.str();
    }
  };

  Tree_Node<int>* root = new Tree_Node<int>;
  root->get_key() = 255;

  ostringstream out;
  generate_tree<Tree_Node<int>, HexWriter>(root, out);
  string result = out.str();

  EXPECT_NE(result.find("0xff"), string::npos);

  delete root;
}

// Test deep tree (many levels)
TEST(DeepTreeTest, GenerateTreeHandlesDeepTree)
{
  const int DEPTH = 10;
  Tree_Node<int>* root = new Tree_Node<int>;
  root->get_key() = 0;

  Tree_Node<int>* current = root;
  for (int i = 1; i < DEPTH; ++i) {
    Tree_Node<int>* child = new Tree_Node<int>;
    child->get_key() = i;
    current->insert_leftmost_child(child);
    current = child;
  }

  ostringstream out;
  generate_tree(root, out);
  string result = out.str();

  // Should contain all nodes
  for (int i = 0; i < DEPTH; ++i) {
    EXPECT_NE(result.find("\"" + to_string(i) + "\""), string::npos)
      << "Missing node " << i;
  }

  // Should have deep Dewey notation like "0.0.0.0..."
  // The deepest node would have 9 dots in its Dewey number
  EXPECT_NE(result.find("Node 0.0.0.0"), string::npos);

  destroy_tree(root);
}

// Test wide tree (many siblings)
TEST(WideTreeTest, GenerateTreeHandlesWideTree)
{
  const int WIDTH = 10;
  Tree_Node<int>* root = new Tree_Node<int>;
  root->get_key() = 0;

  for (int i = 1; i <= WIDTH; ++i) {
    Tree_Node<int>* child = new Tree_Node<int>;
    child->get_key() = i;
    root->insert_rightmost_child(child);
  }

  ostringstream out;
  generate_tree(root, out);
  string result = out.str();

  // Should contain all children
  for (int i = 1; i <= WIDTH; ++i) {
    EXPECT_NE(result.find("\"" + to_string(i) + "\""), string::npos)
      << "Missing child " << i;
  }

  destroy_tree(root);
}

// ============================================================================
// graph_to_tree.H Additional Tests  
// ============================================================================

// Test free function graph_to_tree_node (not just the class)
TEST_F(TreeGraphTest, FreeFunctionGraphToTreeNodeWorks)
{
  Tree_Node<int>* treeRoot = 
    graph_to_tree_node<Graph, int, IntConvert, Dft_Show_Arc<Graph>>(tree, root);

  ASSERT_NE(treeRoot, nullptr);
  EXPECT_EQ(treeRoot->get_key(), 1);

  destroy_tree(treeRoot);
}

// Test with different starting nodes
TEST_F(TreeGraphTest, GraphToTreeNodeFromDifferentRoot)
{
  // Start from child1 instead of root
  Tree_Node<int>* treeRoot = 
    Graph_To_Tree_Node<Graph, int, IntConvert>()(tree, child1);

  ASSERT_NE(treeRoot, nullptr);
  EXPECT_EQ(treeRoot->get_key(), 2); // child1's value

  // Should have grandchild as child (and root as parent via traversal)
  auto* firstChild = treeRoot->get_left_child();
  ASSERT_NE(firstChild, nullptr);

  destroy_tree(treeRoot);
}

// Test converter that transforms data
TEST_F(TreeGraphTest, GraphToTreeNodeWithTransformingConverter)
{
  struct DoubleConvert {
    using GNode = List_Graph<Graph_Node<int>, Graph_Arc<int>>::Node;
    void operator()(GNode* gnode, Tree_Node<int>* tnode) {
      tnode->get_key() = gnode->get_info() * 2; // Double the value
    }
  };

  Tree_Node<int>* treeRoot = 
    Graph_To_Tree_Node<Graph, int, DoubleConvert>()(tree, root);

  EXPECT_EQ(treeRoot->get_key(), 2); // 1 * 2 = 2

  destroy_tree(treeRoot);
}

// Test with string keys in tree
TEST_F(TreeGraphTest, GraphToTreeNodeWithStringKeys)
{
  struct StringConvert {
    using GNode = List_Graph<Graph_Node<int>, Graph_Arc<int>>::Node;
    void operator()(GNode* gnode, Tree_Node<string>* tnode) {
      tnode->get_key() = "Node_" + to_string(gnode->get_info());
    }
  };

  Tree_Node<string>* treeRoot = 
    Graph_To_Tree_Node<Graph, string, StringConvert>()(tree, root);

  EXPECT_EQ(treeRoot->get_key(), "Node_1");

  destroy_tree(treeRoot);
}

// ============================================================================
// Custom Filter Tests
// ============================================================================

// Custom arc filter that only shows arcs with weight > threshold
template <typename GT>
struct WeightFilter
{
  int threshold = 15;
  
  bool operator()(typename GT::Arc* a) const {
    return a->get_info() > threshold;
  }
};

TEST_F(SimpleGraphTest, GenerateGraphvizWithArcFilter)
{
  // Our graph has arcs with weights 10, 20, 30
  // Filter should only show arcs > 15 (so 20 and 30)
  
  struct NodeAttr {
    void operator()(const Graph&, Node* n, ostream& out) {
      out << "label=\"" << n->get_info() << "\"";
    }
  };
  struct ArcAttr {
    void operator()(const Graph&, Arc* a, ostream& out) {
      out << "label=\"" << a->get_info() << "\"";
    }
  };

  ostringstream out;
  generate_graphviz<Graph, NodeAttr, ArcAttr, Dft_Show_Node<Graph>, WeightFilter<Graph>>
    (g, out, NodeAttr(), ArcAttr(), "LR");
  string result = out.str();

  // Should contain arcs with weight 20 and 30
  EXPECT_NE(result.find("20"), string::npos);
  EXPECT_NE(result.find("30"), string::npos);
  // Should NOT contain arc with weight 10
  EXPECT_EQ(result.find("\"10\""), string::npos);
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST(LargeGraphTest, GenerateGraphvizHandlesLargeGraph)
{
  using LGraph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  LGraph g;
  
  const int NUM_NODES = 100;
  vector<LGraph::Node*> nodes;
  
  // Create nodes
  for (int i = 0; i < NUM_NODES; ++i) {
    nodes.push_back(g.insert_node(i));
  }
  
  // Create a path
  for (int i = 0; i < NUM_NODES - 1; ++i) {
    g.insert_arc(nodes[i], nodes[i + 1], i);
  }

  ostringstream out;
  To_Graphviz<LGraph>()(g, out);
  string result = out.str();

  // Should produce valid output
  EXPECT_NE(result.find("graph {"), string::npos);
  EXPECT_NE(result.find("}"), string::npos);
  
  // Should contain first and last nodes
  EXPECT_NE(result.find("\"0\""), string::npos);
  EXPECT_NE(result.find("\"99\""), string::npos);
}

TEST(LargeTreeTest, GraphToTreeNodeHandlesLargeTree)
{
  using LGraph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  LGraph tree;
  
  const int NUM_NODES = 100;
  vector<LGraph::Node*> nodes;
  
  // Create nodes
  for (int i = 0; i < NUM_NODES; ++i) {
    nodes.push_back(tree.insert_node(i));
  }
  
  // Create a binary-ish tree structure
  for (int i = 1; i < NUM_NODES; ++i) {
    int parent = (i - 1) / 2;
    tree.insert_arc(nodes[parent], nodes[i], 0);
  }

  struct Conv {
    void operator()(LGraph::Node* gn, Tree_Node<int>* tn) {
      tn->get_key() = gn->get_info();
    }
  };

  Tree_Node<int>* treeRoot = 
    Graph_To_Tree_Node<LGraph, int, Conv>()(tree, nodes[0]);

  ASSERT_NE(treeRoot, nullptr);
  EXPECT_EQ(treeRoot->get_key(), 0);

  // Count all nodes
  int count = 0;
  function<void(Tree_Node<int>*)> countNodes = [&](Tree_Node<int>* node) {
    if (node == nullptr) return;
    count++;
    for (auto* child = node->get_left_child(); child != nullptr;
         child = child->get_right_sibling())
      countNodes(child);
  };
  countNodes(treeRoot);

  EXPECT_EQ(count, NUM_NODES);

  destroy_tree(treeRoot);
}
