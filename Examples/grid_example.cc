#include <iostream>
#include <numbers>
#include <grid.H>
#include <tpl_graph.H>

using namespace std;
using namespace Aleph;

// A simple structure to hold node coordinates
struct Coordinate {
  size_t row, col;
  Coordinate() : row(0), col(0) {}
  Coordinate(size_t r, size_t c) : row(r), col(c) {}
};

// Functor to initialize node coordinates
struct InitNode {
  template <class Graph>
  void operator()(Graph& /*g*/, typename Graph::Node* n, size_t r, size_t c) {
    n->get_info() = Coordinate(r, c);
  }
};

// Functor to initialize arc weights
// For a grid, we might set orthogonal moves to cost 1.0 and diagonal to 1.414 (sqrt(2))
struct InitArc {
  template <class Graph>
  void operator()(Graph& g, typename Graph::Arc* a, size_t /*r*/, size_t /*c*/) {
    // Determine if the connection is diagonal or orthogonal based on coordinates
    auto src_info = g.get_src_node(a)->get_info();
    auto tgt_info = g.get_tgt_node(a)->get_info();
    
    bool is_diagonal = (src_info.row != tgt_info.row) and (src_info.col != tgt_info.col);
    a->get_info() = is_diagonal ? std::numbers::sqrt2 : 1.0;
  }
};

int main() {
  cout << "========================================" << endl;
  cout << "        Grid Graph Generation Example" << endl;
  cout << "========================================" << endl;

  // We define a graph with Coordinate as node info and double as arc info
  List_Graph<Graph_Node<Coordinate>, Graph_Arc<double>> g;
  
  // We use our custom functors to initialize the nodes and arcs during generation
  Build_Grid<decltype(g), InitNode, InitArc> builder;
  
  const size_t rows = 4;
  const size_t cols = 4;
  
  cout << "\nBuilding a " << rows << " x " << cols << " grid..." << endl;
  // Note: parameters are (graph, width, height)
  builder(g, cols, rows);
  
  cout << "Grid successfully built!" << endl;
  cout << "Number of nodes: " << g.get_num_nodes() << endl;
  cout << "Number of arcs: " << g.get_num_arcs() << endl;
  
  cout << "\nSample of connections from the bottom-right node (" << rows-1 << ", " << cols-1 << "):" << endl;
  
  // Find the node at (rows-1, cols-1)
  List_Graph<Graph_Node<Coordinate>, Graph_Arc<double>>::Node* last_node = nullptr;
  for (auto it = g.get_node_it(); it.has_curr(); it.next()) {
    auto n = it.get_curr();
    if (n->get_info().row == rows - 1 and n->get_info().col == cols - 1) {
      last_node = n;
      break;
    }
  }
  
  if (last_node) {
    // In Aleph-w, arcs are directed from the node that created them. 
    // Build_Grid connects node (i,j) to its previously created neighbors (left, up, diags).
    // So the last node has outgoing arcs to its top, left, and upper-left neighbors.
    for (auto it = g.get_out_it(last_node); it.has_curr(); it.next()) {
      auto a = it.get_curr();
      auto tgt = g.get_tgt_node(a);
      cout << "  -> Connected to node (" << tgt->get_info().row << ", " << tgt->get_info().col 
           << ") with weight " << a->get_info() << endl;
    }
  }

  cout << "\n========================================" << endl;
  return 0;
}
