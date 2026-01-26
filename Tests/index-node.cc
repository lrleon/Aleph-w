
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
