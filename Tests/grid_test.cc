#include <gtest/gtest.h>
#include <grid.H>
#include <tpl_graph.H>

using namespace Aleph;

TEST(GridTest, BasicCreation) {
  List_Graph<Graph_Node<int>, Graph_Arc<int>> g;
  Build_Grid<decltype(g)> build;
  build(g, 10, 10);
  
  EXPECT_EQ(g.get_num_nodes(), 100);
  
  // horizontal: 10 * 9 = 90
  // vertical: 10 * 9 = 90
  // upper-left diag: 9 * 9 = 81
  // upper-right diag: 9 * 9 = 81
  // Total arcs = 342
  EXPECT_EQ(g.get_num_arcs(), 342);
}

TEST(GridTest, InvalidDimensions) {
  List_Graph<Graph_Node<int>, Graph_Arc<int>> g;
  Build_Grid<decltype(g)> build;
  
  EXPECT_THROW(build(g, 1, 10), std::length_error);
  EXPECT_THROW(build(g, 10, 1), std::length_error);
  
  EXPECT_NO_THROW(build(g, 2, 2));
  EXPECT_EQ(g.get_num_nodes(), 4);
}

TEST(GridTest, CustomOperations) {
  List_Graph<Graph_Node<int>, Graph_Arc<int>> g;
  
  struct NodeOp {
    void operator()(decltype(g)& /*g*/, decltype(g)::Node* n, size_t row, size_t col) {
      n->get_info() = row * 100 + col;
    }
  };
  
  struct ArcOp {
    void operator()(decltype(g)& /*g*/, decltype(g)::Arc* a, size_t /*row*/, size_t /*col*/) {
      a->get_info() = 1;
    }
  };

  Build_Grid<decltype(g), NodeOp, ArcOp> build;
  build(g, 3, 3);
  
  EXPECT_EQ(g.get_num_nodes(), 9);
  
  size_t sum_info = 0;
  for (auto it = g.get_node_it(); it.has_curr(); it.next()) {
    sum_info += it.get_curr()->get_info();
  }
  
  // row 0: 0, 1, 2 = 3
  // row 1: 100, 101, 102 = 303
  // row 2: 200, 201, 202 = 603
  // sum = 909
  EXPECT_EQ(sum_info, 909);
  
  size_t arc_sum = 0;
  for (auto it = g.get_arc_it(); it.has_curr(); it.next()) {
    arc_sum += it.get_curr()->get_info();
  }
  
  // 3x3 grid:
  // horiz: 3 * 2 = 6
  // vert: 3 * 2 = 6
  // ul: 2 * 2 = 4
  // ur: 2 * 2 = 4
  // total = 20
  EXPECT_EQ(arc_sum, 20);
}
