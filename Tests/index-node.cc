
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/


/**
 * @file index-node.cc
 * @brief Tests for Index Node
 */

#include <gtest/gtest.h>
#include "../tpl_indexNode.H"

#include "../tpl_graph.H"

// A simple node data structure for our tests
struct TestNodeData {
    int id;
    std::string payload;

    bool operator<(const TestNodeData& other) const {
        return id < other.id;
    }
};

// Define the graph type we'll use for testing
using TestGraph = Aleph::List_Graph<Aleph::Graph_Node<TestNodeData>, Aleph::Graph_Arc<int>>;

// Custom comparator for nodes based on their ID
struct TestNodeCmp {
    bool operator()(const TestGraph::Node* p1, const TestGraph::Node* p2) const {
        return p1->get_info().id < p2->get_info().id;
    }
};

class IndexNodeTest : public ::testing::Test {
protected:
    TestGraph g;
    Aleph::IndexNode<TestGraph, TestNodeCmp> index{g};

    void SetUp() override {
        // The index is initialized with an empty graph
    }
};

TEST_F(IndexNodeTest, InitialState) {
    EXPECT_EQ(index.size(), 0);
}

TEST_F(IndexNodeTest, InsertInGraphAndSearch) {
    auto* node1 = index.insert_in_graph({1, "A"});
    EXPECT_EQ(index.size(), 1);
    EXPECT_EQ(g.get_num_nodes(), 1);

    auto* foundNode = index.search({1, ""});
    ASSERT_NE(foundNode, nullptr);
    EXPECT_EQ(foundNode, node1);
    EXPECT_EQ(foundNode->get_info().payload, "A");
}

TEST_F(IndexNodeTest, SearchNonExistent) {
    index.insert_in_graph({1, "A"});
    auto* foundNode = index.search({2, ""});
    EXPECT_EQ(foundNode, nullptr);
}

TEST_F(IndexNodeTest, Remove) {
    auto* node1 = index.insert_in_graph({1, "A"});
    index.insert_in_graph({2, "B"});
    EXPECT_EQ(index.size(), 2);

    index.remove(node1);
    EXPECT_EQ(index.size(), 1);
    EXPECT_EQ(g.get_num_nodes(), 2); // Still in graph
    EXPECT_EQ(index.search({1, ""}), nullptr);
    EXPECT_NE(index.search({2, ""}), nullptr);
}

TEST_F(IndexNodeTest, RemoveFromGraph) {
    auto* node1 = index.insert_in_graph({1, "A"});
    index.insert_in_graph({2, "B"});
    EXPECT_EQ(index.size(), 2);
    EXPECT_EQ(g.get_num_nodes(), 2);

    index.remove_from_graph(node1);
    EXPECT_EQ(index.size(), 1);
    EXPECT_EQ(g.get_num_nodes(), 1);
    EXPECT_EQ(index.search({1, ""}), nullptr);
}

TEST_F(IndexNodeTest, RemoveFromGraphNonExistent) {
    auto* node1 = g.insert_node({1, "A"});
    // Node is in graph but not in index
    EXPECT_THROW(index.remove_from_graph(node1), std::domain_error);
}

TEST_F(IndexNodeTest, BuildIndex) {
    g.insert_node({10, "X"});
    g.insert_node({20, "Y"});

    Aleph::IndexNode<TestGraph, TestNodeCmp> new_index(g);
    // The constructor should call build_index implicitly via init()
    EXPECT_EQ(new_index.size(), 2);
    EXPECT_NE(new_index.search({10, ""}), nullptr);
    EXPECT_NE(new_index.search({20, ""}), nullptr);

    // Manually add a node and rebuild
    g.insert_node({30, "Z"});
    new_index.build_index();
    EXPECT_EQ(new_index.size(), 3);
    EXPECT_NE(new_index.search({30, ""}), nullptr);
}

TEST_F(IndexNodeTest, ClearIndex) {
    index.insert_in_graph({1, "A"});
    index.insert_in_graph({2, "B"});
    EXPECT_EQ(index.size(), 2);
    EXPECT_EQ(g.get_num_nodes(), 2);

    index.clear_index();
    EXPECT_EQ(index.size(), 0);
    EXPECT_EQ(g.get_num_nodes(), 2); // Graph is untouched
}

TEST_F(IndexNodeTest, ClearGraph) {
    index.insert_in_graph({1, "A"});
    index.insert_in_graph({2, "B"});
    EXPECT_EQ(index.size(), 2);
    EXPECT_EQ(g.get_num_nodes(), 2);

    index.clear_graph();
    EXPECT_EQ(index.size(), 0);
    EXPECT_EQ(g.get_num_nodes(), 0);
}
