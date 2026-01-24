/* Tests for net_utils.H - Network flow utilities
 *
 * Tests cover:
 * - Random network generation
 * - Grid network generation
 * - Bipartite network generation
 * - DOT export
 * - JSON export/import
 * - DIMACS export/import
 * - Benchmarking utilities
 */

#include <gtest/gtest.h>
#include <net_utils.H>
#include <tpl_maxflow.H>
#include <filesystem>
#include <fstream>
#include <string>
#if defined(_WIN32)
#  include <process.h>
#else
#  include <unistd.h>
#endif

using namespace Aleph;

// Network type for tests
using TestNet = Net_Graph<Net_Node<Empty_Class>,
                          Net_Arc<Empty_Class, double>>;

class NetUtilsTest : public ::testing::Test
{
protected:
  std::filesystem::path dot_file;
  std::filesystem::path json_file;
  std::filesystem::path dimacs_file;

  static long long process_id() noexcept
  {
#if defined(_WIN32)
    return static_cast<long long>(_getpid());
#else
    return static_cast<long long>(getpid());
#endif
  }

  void SetUp() override
  {
    const auto *test_info = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string base = std::string("alephw_") + test_info->test_suite_name() + "_" +
                       test_info->name() + "_" + std::to_string(process_id());

    for (auto &ch : base)
      if (ch == '/' || ch == '\\' || ch == ' ')
        ch = '_';

    auto dir = std::filesystem::temp_directory_path();
    dot_file = dir / (base + ".dot");
    json_file = dir / (base + ".json");
    dimacs_file = dir / (base + ".dimacs");
  }

  // Cleanup temp files
  void TearDown() override
  {
    std::error_code ec;
    std::filesystem::remove(dot_file, ec);
    std::filesystem::remove(json_file, ec);
    std::filesystem::remove(dimacs_file, ec);
  }
};


//==============================================================================
// Random Network Generator Tests
//==============================================================================

TEST_F(NetUtilsTest, GenerateRandomNetworkBasic)
{
  NetworkGenParams params;
  params.num_nodes = 10;
  params.num_arcs = 20;
  params.min_capacity = 1;
  params.max_capacity = 100;
  params.seed = 42;

  auto net = generate_random_network<TestNet>(params);

  EXPECT_EQ(net.vsize(), 10);
  EXPECT_GE(net.esize(), params.num_nodes - 1);  // At least path arcs
  // Note: Random networks may have multiple sources/sinks
  // Use make_super_source/make_super_sink if needed
}

TEST_F(NetUtilsTest, GenerateRandomNetworkConvenience)
{
  auto net = generate_random_network<TestNet>(50, 200, 10, 1000, 123);

  EXPECT_EQ(net.vsize(), 50);
  EXPECT_GT(net.esize(), 0);

  // Verify capacity bounds
  for (Arc_Iterator<TestNet> it(net); it.has_curr(); it.next_ne())
    {
      auto arc = it.get_curr();
      EXPECT_GE(arc->cap, 10.0);
      EXPECT_LE(arc->cap, 1000.0);
    }
}

TEST_F(NetUtilsTest, GenerateRandomNetworkConnected)
{
  NetworkGenParams params;
  params.num_nodes = 20;
  params.num_arcs = 50;
  params.ensure_connected = true;
  params.seed = 42;

  auto net = generate_random_network<TestNet>(params);

  // Verify connectivity was attempted
  EXPECT_GE(net.esize(), params.num_nodes - 1);
}


//==============================================================================
// Grid Network Generator Tests
//==============================================================================

TEST_F(NetUtilsTest, GenerateGridNetwork)
{
  // Use unidirectional to ensure single source/sink
  auto net = generate_grid_network<TestNet>(5, 5, 10.0, false);

  EXPECT_EQ(net.vsize(), 25);
  EXPECT_TRUE(net.is_single_source());
  EXPECT_TRUE(net.is_single_sink());

  // Unidirectional: 4*5 + 5*4 = 40 arcs
  EXPECT_EQ(net.esize(), 40);
}

TEST_F(NetUtilsTest, GenerateGridNetworkUnidirectional)
{
  auto net = generate_grid_network<TestNet>(3, 3, 5.0, false);

  EXPECT_EQ(net.vsize(), 9);

  // Unidirectional: 2*3 + 3*2 = 12 arcs
  EXPECT_EQ(net.esize(), 12);
}

TEST_F(NetUtilsTest, GenerateGridNetworkFlowComputation)
{
  // Use unidirectional to ensure single source/sink
  auto net = generate_grid_network<TestNet>(4, 4, 100.0, false);

  auto flow = dinic_maximum_flow(net);
  EXPECT_GT(flow, 0.0);
}


//==============================================================================
// Bipartite Network Generator Tests
//==============================================================================

TEST_F(NetUtilsTest, GenerateBipartiteNetwork)
{
  auto net = generate_bipartite_network<TestNet>(10, 10, 0.5, 42);

  // Source + sink + 10 left + 10 right = 22 nodes
  EXPECT_EQ(net.vsize(), 22);
  EXPECT_TRUE(net.is_single_source());
  EXPECT_TRUE(net.is_single_sink());
}

TEST_F(NetUtilsTest, GenerateBipartiteNetworkMatching)
{
  auto net = generate_bipartite_network<TestNet>(5, 5, 1.0, 42);

  // With probability 1.0, all edges exist
  auto flow = dinic_maximum_flow(net);

  // Max matching can be at most min(left, right) = 5
  EXPECT_LE(flow, 5.0);
}


//==============================================================================
// Layered Network Generator Tests
//==============================================================================

TEST_F(NetUtilsTest, GenerateLayeredNetwork)
{
  // Use probability 1.0 to ensure all nodes are connected
  std::vector<size_t> layers = {1, 5, 10, 5, 1};
  auto net = generate_layered_network<TestNet>(layers, 10.0, 1.0, 42);

  EXPECT_EQ(net.vsize(), 22);
  EXPECT_GT(net.esize(), 0);
}

TEST_F(NetUtilsTest, GenerateLayeredNetworkFlow)
{
  std::vector<size_t> layers = {1, 3, 3, 1};
  auto net = generate_layered_network<TestNet>(layers, 10.0, 1.0, 42);

  auto flow = dinic_maximum_flow(net);
  EXPECT_GT(flow, 0.0);
}


//==============================================================================
// DOT Export Tests
//==============================================================================

TEST_F(NetUtilsTest, ExportToDotBasic)
{
  auto net = generate_grid_network<TestNet>(3, 3, 10.0, false);  // unidirectional
  dinic_maximum_flow(net);

  export_network_to_dot(net, dot_file.string());

  std::ifstream file(dot_file);
  EXPECT_TRUE(file.good());

  std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

  EXPECT_TRUE(content.find("digraph") != std::string::npos);
  EXPECT_TRUE(content.find("->") != std::string::npos);
}

TEST_F(NetUtilsTest, ExportToDotWithOptions)
{
  auto net = generate_grid_network<TestNet>(2, 2, 10.0, false);  // unidirectional
  dinic_maximum_flow(net);

  DotExportOptions options;
  options.show_flow = true;
  options.show_capacity = true;
  options.highlight_saturated = true;

  export_network_to_dot(net, dot_file.string(), options);

  std::ifstream file(dot_file);
  EXPECT_TRUE(file.good());
}

TEST_F(NetUtilsTest, NetworkToDotString)
{
  auto net = generate_grid_network<TestNet>(2, 2, 10.0);

  std::string dot = network_to_dot_string(net);

  EXPECT_TRUE(dot.find("digraph") != std::string::npos);
  EXPECT_TRUE(dot.find("rankdir=LR") != std::string::npos);
}


//==============================================================================
// JSON Export Tests
//==============================================================================

TEST_F(NetUtilsTest, ExportToJsonBasic)
{
  auto net = generate_grid_network<TestNet>(2, 2, 10.0);

  export_network_to_json(net, json_file.string());

  std::ifstream file(json_file);
  EXPECT_TRUE(file.good());

  std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

  EXPECT_TRUE(content.find("\"num_nodes\"") != std::string::npos);
  EXPECT_TRUE(content.find("\"arcs\"") != std::string::npos);
  EXPECT_TRUE(content.find("\"cap\"") != std::string::npos);
}

TEST_F(NetUtilsTest, NetworkToJsonString)
{
  auto net = generate_grid_network<TestNet>(2, 2, 10.0, false);  // unidirectional

  std::string json = network_to_json_string(net);

  EXPECT_TRUE(json.find("\"num_nodes\": 4") != std::string::npos);
  EXPECT_TRUE(json.find("\"source\"") != std::string::npos);
  EXPECT_TRUE(json.find("\"sink\"") != std::string::npos);
}


//==============================================================================
// DIMACS Export/Import Tests
//==============================================================================

TEST_F(NetUtilsTest, ExportToDimacsBasic)
{
  auto net = generate_grid_network<TestNet>(3, 3, 10.0, false);

  export_network_to_dimacs(net, dimacs_file.string());

  std::ifstream file(dimacs_file);
  EXPECT_TRUE(file.good());

  std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

  EXPECT_TRUE(content.find("p max 9") != std::string::npos);
  EXPECT_TRUE(content.find("n") != std::string::npos);
  EXPECT_TRUE(content.find("a") != std::string::npos);
}

TEST_F(NetUtilsTest, DimacsRoundTrip)
{
  // Create and export
  auto net1 = generate_grid_network<TestNet>(3, 3, 10.0, false);
  export_network_to_dimacs(net1, dimacs_file.string());

  // Import
  auto net2 = import_network_from_dimacs<TestNet>(dimacs_file.string());

  EXPECT_EQ(net2.vsize(), net1.vsize());
  EXPECT_EQ(net2.esize(), net1.esize());
  EXPECT_TRUE(net2.is_single_source());
  EXPECT_TRUE(net2.is_single_sink());
}

TEST_F(NetUtilsTest, DimacsFlowEquivalence)
{
  // Create network, compute flow, export
  auto net1 = generate_grid_network<TestNet>(4, 4, 10.0, false);
  auto flow1 = dinic_maximum_flow(net1);

  export_network_to_dimacs(net1, dimacs_file.string());

  // Import and compute flow
  auto net2 = import_network_from_dimacs<TestNet>(dimacs_file.string());
  auto flow2 = dinic_maximum_flow(net2);

  // Flows should be equal
  EXPECT_DOUBLE_EQ(flow1, flow2);
}


//==============================================================================
// Benchmark Utilities Tests
//==============================================================================

TEST_F(NetUtilsTest, BenchmarkMaxFlow)
{
  auto net = generate_grid_network<TestNet>(5, 5, 10.0, false);  // unidirectional

  auto result = benchmark_maxflow(net, [](auto& n) {
    return dinic_maximum_flow(n);
  }, "Dinic");

  EXPECT_GT(result.flow_value, 0.0);
  EXPECT_GE(result.elapsed_ms, 0.0);
  EXPECT_EQ(result.algorithm_name, "Dinic");
}

TEST_F(NetUtilsTest, PrintBenchmarkResults)
{
  std::vector<MaxFlowBenchmarkResult<double>> results;

  results.push_back({100.0, 1.5, "Algorithm A"});
  results.push_back({100.0, 2.3, "Algorithm B"});

  // Just verify it doesn't crash
  testing::internal::CaptureStdout();
  print_benchmark_results(results);
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(output.find("Algorithm A") != std::string::npos);
  EXPECT_TRUE(output.find("Algorithm B") != std::string::npos);
}


//==============================================================================
// Stress Tests
//==============================================================================

TEST_F(NetUtilsTest, LargeRandomNetwork)
{
  NetworkGenParams params;
  params.num_nodes = 100;
  params.num_arcs = 500;
  params.seed = 42;

  auto net = generate_random_network<TestNet>(params);

  EXPECT_EQ(net.vsize(), 100);
  EXPECT_GE(net.esize(), 99);  // At least connected path
  // Don't run flow since multi-source/multi-sink
}

TEST_F(NetUtilsTest, LargeGridNetwork)
{
  auto net = generate_grid_network<TestNet>(20, 20, 100.0, false);  // unidirectional

  EXPECT_EQ(net.vsize(), 400);

  // Should complete in reasonable time
  auto flow = dinic_maximum_flow(net);
  EXPECT_GT(flow, 0.0);
}


int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
