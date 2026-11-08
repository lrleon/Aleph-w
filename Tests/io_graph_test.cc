
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


/** @file io_graph_test.cc
 *  @brief Comprehensive unit tests for io_graph.H
 */

#include <gtest/gtest.h>
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <filesystem>
#include <limits>
#include <string>
#include <io_graph.H>
#include <tpl_graph.H>
#if defined(_WIN32)
#  include <process.h>
#else
#  include <unistd.h>
#endif

using namespace Aleph;

namespace
{
/// Process ID of the current process, portable across POSIX and Windows.
/// Shared by `IOGraphTest` and `IODigraphTest` below to build a
/// unique-per-process temp file name in their `SetUp()`: each `TEST_F`
/// runs as its own process (`gtest_discover_tests`), and CI runs ctest
/// with `--parallel`, so a name that is only unique *within* a process
/// (e.g. a plain, unseeded `rand()`, which returns the same value in
/// every freshly-launched process) is not enough to keep two
/// concurrently-running test cases from racing on the same file.
long long process_id() noexcept
{
#if defined(_WIN32)
  return static_cast<long long>(_getpid());
#else
  return static_cast<long long>(getpid());
#endif
}
}  // namespace

//============================================================================
// Test Fixtures
//============================================================================

class IOGraphTest : public ::testing::Test
{
protected:
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<double>>;
  using Node = Graph::Node;
  using Arc = Graph::Arc;
  
  void SetUp() override
  {
    // Create a simple graph:
    // 0 --1.5--> 1 --2.5--> 2
    //            |
    //           3.5
    //            v
    //            3
    
    n0 = g.insert_node(100);
    n1 = g.insert_node(200);
    n2 = g.insert_node(300);
    n3 = g.insert_node(400);
    
    g.insert_arc(n0, n1, 1.5);
    g.insert_arc(n1, n2, 2.5);
    g.insert_arc(n1, n3, 3.5);
    
    // Unique per test case and process (see the `process_id()` comment
    // above for why a plain rand() is not enough).
    const auto *test_info = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string base = std::string("aleph_io_graph_test_") + test_info->name() + "_" +
                        std::to_string(process_id());
    for (auto &ch : base)
      if (ch == '/' or ch == '\\' or ch == ' ')
        ch = '_';

    binary_file = (std::filesystem::temp_directory_path() / (base + ".bin")).string();
    text_file = (std::filesystem::temp_directory_path() / (base + ".txt")).string();
  }
  
  void TearDown() override
  {
    std::remove(binary_file.c_str());
    std::remove(text_file.c_str());
  }
  
  // Helper to collect node values
  std::vector<int> get_node_values(Graph & graph)
  {
    std::vector<int> values;
    for (typename Graph::Node_Iterator it(graph); it.has_curr(); it.next())
      values.push_back(it.get_curr()->get_info());
    std::sort(values.begin(), values.end());
    return values;
  }
  
  // Helper to collect arc values
  std::vector<double> get_arc_values(Graph & graph)
  {
    std::vector<double> values;
    for (typename Graph::Arc_Iterator it(graph); it.has_curr(); it.next())
      values.push_back(it.get_curr()->get_info());
    std::sort(values.begin(), values.end());
    return values;
  }
  
  Graph g;
  Node *n0, *n1, *n2, *n3;
  std::string binary_file;
  std::string text_file;
};

//============================================================================
// Constructor Tests
//============================================================================

TEST_F(IOGraphTest, ConstructFromReference)
{
  IO_Graph<Graph> io(g);
  EXPECT_FALSE(io.is_verbose());
}

TEST_F(IOGraphTest, ConstructFromPointer)
{
  IO_Graph<Graph> io(&g);
  EXPECT_FALSE(io.is_verbose());
}

TEST_F(IOGraphTest, VerboseMode)
{
  IO_Graph<Graph> io(g);
  
  EXPECT_FALSE(io.is_verbose());
  
  io.set_verbose(true);
  EXPECT_TRUE(io.is_verbose());
  
  io.set_verbose(false);
  EXPECT_FALSE(io.is_verbose());
}

//============================================================================
// Binary Mode Tests
//============================================================================

TEST_F(IOGraphTest, SaveAndLoadBinary)
{
  IO_Graph<Graph> io(g);
  
  // Save
  {
    std::ofstream out(binary_file, std::ios::binary);
    ASSERT_TRUE(out.is_open());
    io.save(out);
  }
  
  // Load into new graph
  Graph g2;
  IO_Graph<Graph> io2(g2);
  
  {
    std::ifstream in(binary_file, std::ios::binary);
    ASSERT_TRUE(in.is_open());
    io2.load(in);
  }
  
  // Verify
  EXPECT_EQ(g2.get_num_nodes(), 4u);
  EXPECT_EQ(g2.get_num_arcs(), 3u);
}

TEST_F(IOGraphTest, BinaryPreservesNodeData)
{
  IO_Graph<Graph> io(g);
  
  // Save
  {
    std::ofstream out(binary_file, std::ios::binary);
    io.save(out);
  }
  
  // Load
  Graph g2;
  IO_Graph<Graph> io2(g2);
  
  {
    std::ifstream in(binary_file, std::ios::binary);
    io2.load(in);
  }
  
  EXPECT_EQ(get_node_values(g), get_node_values(g2));
}

TEST_F(IOGraphTest, BinaryPreservesArcData)
{
  IO_Graph<Graph> io(g);
  
  // Save
  {
    std::ofstream out(binary_file, std::ios::binary);
    io.save(out);
  }
  
  // Load
  Graph g2;
  IO_Graph<Graph> io2(g2);
  
  {
    std::ifstream in(binary_file, std::ios::binary);
    io2.load(in);
  }
  
  EXPECT_EQ(get_arc_values(g), get_arc_values(g2));
}

//============================================================================
// Text Mode Tests
//============================================================================

TEST_F(IOGraphTest, SaveAndLoadText)
{
  IO_Graph<Graph> io(g);
  
  // Save
  {
    std::ofstream out(text_file);
    ASSERT_TRUE(out.is_open());
    io.save_in_text_mode(out);
  }
  
  // Load into new graph
  Graph g2;
  IO_Graph<Graph> io2(g2);
  
  {
    std::ifstream in(text_file);
    ASSERT_TRUE(in.is_open());
    io2.load_in_text_mode(in);
  }
  
  // Verify
  EXPECT_EQ(g2.get_num_nodes(), 4u);
  EXPECT_EQ(g2.get_num_arcs(), 3u);
}

TEST_F(IOGraphTest, TextPreservesNodeData)
{
  IO_Graph<Graph> io(g);
  
  // Save
  {
    std::ofstream out(text_file);
    io.save_in_text_mode(out);
  }
  
  // Load
  Graph g2;
  IO_Graph<Graph> io2(g2);
  
  {
    std::ifstream in(text_file);
    io2.load_in_text_mode(in);
  }
  
  EXPECT_EQ(get_node_values(g), get_node_values(g2));
}

TEST_F(IOGraphTest, TextModeToStringStream)
{
  IO_Graph<Graph> io(g);
  
  std::ostringstream out;
  io.save_in_text_mode(out);
  
  std::string content = out.str();
  
  // Should start with node count and arc count
  EXPECT_TRUE(content.find("4") != std::string::npos);  // 4 nodes
  EXPECT_TRUE(content.find("3") != std::string::npos);  // 3 arcs
}

//============================================================================
// Empty Graph Tests
//============================================================================

TEST_F(IOGraphTest, SaveAndLoadEmptyGraphBinary)
{
  Graph empty_g;
  IO_Graph<Graph> io(empty_g);
  
  // Save
  {
    std::ofstream out(binary_file, std::ios::binary);
    io.save(out);
  }
  
  // Load
  Graph g2;
  IO_Graph<Graph> io2(g2);
  
  {
    std::ifstream in(binary_file, std::ios::binary);
    io2.load(in);
  }
  
  EXPECT_EQ(g2.get_num_nodes(), 0u);
  EXPECT_EQ(g2.get_num_arcs(), 0u);
}

TEST_F(IOGraphTest, SaveAndLoadEmptyGraphText)
{
  Graph empty_g;
  IO_Graph<Graph> io(empty_g);
  
  // Save
  {
    std::ofstream out(text_file);
    io.save_in_text_mode(out);
  }
  
  // Load
  Graph g2;
  IO_Graph<Graph> io2(g2);
  
  {
    std::ifstream in(text_file);
    io2.load_in_text_mode(in);
  }
  
  EXPECT_EQ(g2.get_num_nodes(), 0u);
  EXPECT_EQ(g2.get_num_arcs(), 0u);
}

//============================================================================
// Single Node Graph Tests
//============================================================================

TEST_F(IOGraphTest, SaveAndLoadSingleNodeBinary)
{
  Graph single_g;
  single_g.insert_node(42);
  
  IO_Graph<Graph> io(single_g);
  
  // Save
  {
    std::ofstream out(binary_file, std::ios::binary);
    io.save(out);
  }
  
  // Load
  Graph g2;
  IO_Graph<Graph> io2(g2);
  
  {
    std::ifstream in(binary_file, std::ios::binary);
    io2.load(in);
  }
  
  EXPECT_EQ(g2.get_num_nodes(), 1u);
  EXPECT_EQ(g2.get_num_arcs(), 0u);
  
  typename Graph::Node_Iterator it(g2);
  EXPECT_EQ(it.get_curr()->get_info(), 42);
}

//============================================================================
// Digraph Tests
//============================================================================

class IODigraphTest : public ::testing::Test
{
protected:
  using Digraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using Node = Digraph::Node;
  using Arc = Digraph::Arc;
  
  void SetUp() override
  {
    n0 = dg.insert_node(10);
    n1 = dg.insert_node(20);
    n2 = dg.insert_node(30);
    
    dg.insert_arc(n0, n1, 1);
    dg.insert_arc(n1, n2, 2);
    dg.insert_arc(n2, n0, 3);  // Cycle
    
    // Unique per test case and process (see the `process_id()` comment
    // near the top of the file for why a plain rand() is not enough).
    const auto *test_info = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string base = std::string("aleph_io_digraph_test_") + test_info->name() + "_" +
                        std::to_string(process_id());
    for (auto &ch : base)
      if (ch == '/' or ch == '\\' or ch == ' ')
        ch = '_';

    binary_file = (std::filesystem::temp_directory_path() / (base + ".bin")).string();
    text_file = (std::filesystem::temp_directory_path() / (base + ".txt")).string();
  }
  
  void TearDown() override
  {
    std::remove(binary_file.c_str());
    std::remove(text_file.c_str());
  }
  
  Digraph dg;
  Node *n0, *n1, *n2;
  std::string binary_file;
  std::string text_file;
};

TEST_F(IODigraphTest, SaveAndLoadBinary)
{
  IO_Graph<Digraph> io(dg);
  
  // Save
  {
    std::ofstream out(binary_file, std::ios::binary);
    io.save(out);
  }
  
  // Load
  Digraph dg2;
  IO_Graph<Digraph> io2(dg2);
  
  {
    std::ifstream in(binary_file, std::ios::binary);
    io2.load(in);
  }
  
  EXPECT_EQ(dg2.get_num_nodes(), 3u);
  EXPECT_EQ(dg2.get_num_arcs(), 3u);
}

TEST_F(IODigraphTest, SaveAndLoadText)
{
  IO_Graph<Digraph> io(dg);
  
  // Save
  {
    std::ofstream out(text_file);
    io.save_in_text_mode(out);
  }
  
  // Load
  Digraph dg2;
  IO_Graph<Digraph> io2(dg2);
  
  {
    std::ifstream in(text_file);
    io2.load_in_text_mode(in);
  }
  
  EXPECT_EQ(dg2.get_num_nodes(), 3u);
  EXPECT_EQ(dg2.get_num_arcs(), 3u);
}

//============================================================================
// Default Functor Tests
//============================================================================

TEST(DftStoreNodeTest, TextMode)
{
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  Graph g;
  auto n = g.insert_node(12345);
  
  Dft_Store_Node<Graph> store;
  
  std::ostringstream ss;
  store(ss, g, n);
  
  EXPECT_TRUE(ss.str().find("12345") != std::string::npos);
}

TEST(DftStoreArcTest, TextMode)
{
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<double>>;
  Graph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto a = g.insert_arc(n1, n2, 3.14159);
  
  Dft_Store_Arc<Graph> store;
  
  std::ostringstream ss;
  store(ss, g, a);
  
  EXPECT_TRUE(ss.str().find("3.14") != std::string::npos);
}

TEST(DftLoadNodeTest, TextMode)
{
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  Graph g;
  auto n = g.insert_node(0);
  
  Dft_Load_Node<Graph> load;
  
  std::istringstream ss("999");
  load(ss, g, n);
  
  EXPECT_EQ(n->get_info(), 999);
}

TEST(DftLoadArcTest, TextMode)
{
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<double>>;
  Graph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto a = g.insert_arc(n1, n2, 0.0);
  
  Dft_Load_Arc<Graph> load;
  
  std::istringstream ss("2.718");
  load(ss, g, a);
  
  EXPECT_NEAR(a->get_info(), 2.718, 0.001);
}

//============================================================================
// Setter Tests
//============================================================================

TEST_F(IOGraphTest, SetLoadNode)
{
  IO_Graph<Graph> io(g);
  Dft_Load_Node<Graph> ln;
  io.set_load_node(ln);  // Should not throw
  SUCCEED();
}

TEST_F(IOGraphTest, SetStoreNode)
{
  IO_Graph<Graph> io(g);
  Dft_Store_Node<Graph> sn;
  io.set_store_node(sn);  // Should not throw
  SUCCEED();
}

TEST_F(IOGraphTest, SetLoadArc)
{
  IO_Graph<Graph> io(g);
  Dft_Load_Arc<Graph> la;
  io.set_load_arc(la);  // Should not throw
  SUCCEED();
}

TEST_F(IOGraphTest, SetStoreArc)
{
  IO_Graph<Graph> io(g);
  Dft_Store_Arc<Graph> sa;
  io.set_store_arc(sa);  // Should not throw
  SUCCEED();
}

//============================================================================
// Error Handling Tests
//============================================================================

TEST_F(IOGraphTest, LoadFromBadFile)
{
  Graph g2;
  IO_Graph<Graph> io(g2);
  
  std::ifstream bad_file("/nonexistent_file_12345.bin", std::ios::binary);
  
  // Loading from a bad stream should throw
  EXPECT_THROW(io.load(bad_file), std::runtime_error);
}

TEST_F(IOGraphTest, LoadTextFromBadFile)
{
  Graph g2;
  IO_Graph<Graph> io(g2);
  
  std::ifstream bad_file("/nonexistent_file_12345.txt");
  
  EXPECT_THROW(io.load_in_text_mode(bad_file), std::runtime_error);
}

//============================================================================
// Round-Trip Tests
//============================================================================

TEST_F(IOGraphTest, MultipleRoundTripsBinary)
{
  // First save
  {
    IO_Graph<Graph> io(g);
    std::ofstream out(binary_file, std::ios::binary);
    io.save(out);
  }
  
  // First load
  Graph g2;
  {
    IO_Graph<Graph> io(g2);
    std::ifstream in(binary_file, std::ios::binary);
    io.load(in);
  }
  
  EXPECT_EQ(g2.get_num_nodes(), 4u);
  
  // Second save (from loaded graph)
  std::string second_file = binary_file + ".2";
  {
    IO_Graph<Graph> io(g2);
    std::ofstream out(second_file, std::ios::binary);
    io.save(out);
  }
  
  // Second load
  Graph g3;
  {
    IO_Graph<Graph> io(g3);
    std::ifstream in(second_file, std::ios::binary);
    io.load(in);
  }
  
  EXPECT_EQ(g3.get_num_nodes(), 4u);
  EXPECT_EQ(g3.get_num_arcs(), 3u);
  
  std::remove(second_file.c_str());
}

TEST_F(IOGraphTest, MultipleRoundTripsText)
{
  // First save
  {
    IO_Graph<Graph> io(g);
    std::ofstream out(text_file);
    io.save_in_text_mode(out);
  }
  
  // First load
  Graph g2;
  {
    IO_Graph<Graph> io(g2);
    std::ifstream in(text_file);
    io.load_in_text_mode(in);
  }
  
  EXPECT_EQ(g2.get_num_nodes(), 4u);
  EXPECT_EQ(g2.get_num_arcs(), 3u);
}

//============================================================================
// Main
//============================================================================

//============================================================================
// Corrupted streams: the load validates the indices and undoes itself
//============================================================================

namespace
{
  // A binary stream of the nodes `infos` and of one arc from `src` to `tgt`
  void write_binary_graph(const std::string & file, const std::vector<int> & infos,
                          int src, int tgt, bool with_arc_info = true)
  {
    std::ofstream out(file, std::ios::binary);
    const size_t num_nodes = infos.size();
    out.write(reinterpret_cast<const char *>(&num_nodes), sizeof(num_nodes));
    for (int info : infos)
      out.write(reinterpret_cast<const char *>(&info), sizeof(info));
    const size_t num_arcs = 1;
    out.write(reinterpret_cast<const char *>(&num_arcs), sizeof(num_arcs));
    out.write(reinterpret_cast<const char *>(&src), sizeof(src));
    out.write(reinterpret_cast<const char *>(&tgt), sizeof(tgt));
    if (with_arc_info)
      {
        const double w = 2.5;
        out.write(reinterpret_cast<const char *>(&w), sizeof(w));
      }
  }
}

TEST_F(IOGraphTest, LoadRejectsAnArcToANodeThatIsNotThere)
{
  // The indices of the arcs were not validated: an index beyond the nodes
  // of the stream read an entry of the table that holds no node
  for (int tgt : {7, 3, -1})
    {
      write_binary_graph(binary_file, {10, 20, 30}, 0, tgt);
      Graph g2;
      g2.insert_node(99);
      IO_Graph<Graph> io(g2);
      std::ifstream in(binary_file, std::ios::binary);
      EXPECT_THROW(io.load(in), std::runtime_error) << tgt;
      // The load undid itself: the graph is as it was
      EXPECT_EQ(g2.vsize(), 1u) << tgt;
      EXPECT_EQ(g2.esize(), 0u) << tgt;
      EXPECT_EQ(g2.get_first_node()->get_info(), 99) << tgt;
    }
}

TEST_F(IOGraphTest, LoadInTextModeRejectsAnArcToANodeThatIsNotThere)
{
  for (const char * text : {"3\n1\n10\n20\n30\n0 5 2.5\n", "2\n1\n10\n20\n-1 0 1.0\n"})
    {
      std::istringstream in(text);
      Graph g2;
      g2.insert_node(99);
      IO_Graph<Graph> io(g2);
      EXPECT_THROW(io.load_in_text_mode(in), std::runtime_error) << text;
      EXPECT_EQ(g2.vsize(), 1u) << text;
      EXPECT_EQ(g2.esize(), 0u) << text;
    }
}

TEST_F(IOGraphTest, AFailedLoadLeavesTheGraphAsItWas)
{
  // The nodes and arcs read before the failure stayed in the graph
  write_binary_graph(binary_file, {10, 20, 30}, 0, 2, false);  // no arc info
  Graph g2;
  g2.insert_node(99);
  IO_Graph<Graph> io(g2);
  std::ifstream in(binary_file, std::ios::binary);
  EXPECT_THROW(io.load(in), std::runtime_error);
  EXPECT_EQ(g2.vsize(), 1u);
  EXPECT_EQ(g2.esize(), 0u);

  std::istringstream text("3\n2\n10\n20\n30\n0 1 1.0\n1 2\n");  // the second arc has no info
  EXPECT_THROW(io.load_in_text_mode(text), std::runtime_error);
  EXPECT_EQ(g2.vsize(), 1u);
  EXPECT_EQ(g2.esize(), 0u);
}

TEST_F(IOGraphTest, ALoadAddsToTheGraph)
{
  // The documentation said that the load cleared the graph, which it does not
  {
    std::ofstream out(binary_file, std::ios::binary);
    IO_Graph<Graph>(g).save(out);
  }
  Graph g2;
  g2.insert_node(99);
  IO_Graph<Graph> io(g2);
  std::ifstream in(binary_file, std::ios::binary);
  io.load(in);
  EXPECT_EQ(g2.vsize(), 5u);
  EXPECT_EQ(g2.esize(), 3u);
}

TEST_F(IOGraphTest, ALoadDoesNotTrustTheCountOfTheStream)
{
  // The count of nodes of the stream reserved the table of the load: a
  // corrupted one, huge, asked for more memory than there is
  const size_t huge = std::numeric_limits<size_t>::max();
  {
    std::ofstream out(binary_file, std::ios::binary);
    out.write(reinterpret_cast<const char *>(&huge), sizeof(huge));
  }
  Graph g2;
  g2.insert_node(99);
  IO_Graph<Graph> io(g2);
  std::ifstream in(binary_file, std::ios::binary);
  EXPECT_THROW(io.load(in), std::runtime_error);
  EXPECT_EQ(g2.vsize(), 1u);
  EXPECT_EQ(g2.esize(), 0u);

  std::istringstream text(std::to_string(huge) + "\n0\n");
  EXPECT_THROW(io.load_in_text_mode(text), std::runtime_error);
  EXPECT_EQ(g2.vsize(), 1u);
  EXPECT_EQ(g2.esize(), 0u);
}

TEST_F(IOGraphTest, ALoadGrowsBeyondWhatItReserves)
{
  // A load reserves for 2^20 nodes at most, and grows as it reads more:
  // an arc from a node beyond that limit
  const size_t n = (size_t(1) << 20) + 2;
  {
    std::ofstream out(binary_file, std::ios::binary);
    out.write(reinterpret_cast<const char *>(&n), sizeof(n));
    for (size_t i = 0; i < n; ++i)
      {
        const int info = static_cast<int>(i);
        out.write(reinterpret_cast<const char *>(&info), sizeof(info));
      }
    const size_t num_arcs = 1;
    out.write(reinterpret_cast<const char *>(&num_arcs), sizeof(num_arcs));
    const int src = static_cast<int>(n - 1), tgt = 0;
    out.write(reinterpret_cast<const char *>(&src), sizeof(src));
    out.write(reinterpret_cast<const char *>(&tgt), sizeof(tgt));
    const double w = 2.5;
    out.write(reinterpret_cast<const char *>(&w), sizeof(w));
  }
  Graph g2;
  IO_Graph<Graph> io(g2);
  std::ifstream in(binary_file, std::ios::binary);
  io.load(in);
  EXPECT_EQ(g2.vsize(), n);
  ASSERT_EQ(g2.esize(), 1u);
  auto * a = g2.get_first_arc();
  EXPECT_EQ(g2.get_src_node(a)->get_info(), static_cast<int>(n - 1));
  EXPECT_EQ(g2.get_tgt_node(a)->get_info(), 0);
  EXPECT_DOUBLE_EQ(a->get_info(), 2.5);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
