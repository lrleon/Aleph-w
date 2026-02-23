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
 * @file planarity_test.cc
 * @brief Tests for Planarity_Test.H.
 */

# include <gtest/gtest.h>

# include <algorithm>
# include <cstdlib>
# include <cstdint>
# include <filesystem>
# include <functional>
# include <fstream>
# include <limits>
# include <optional>
# include <random>
# include <sstream>
# include <string>
# include <tuple>
# include <unordered_map>
# include <utility>
# include <vector>

# include <Planarity_Test.H>
# include <tpl_agraph.H>
# include <tpl_graph.H>

using namespace Aleph;

namespace
{
  using UGraph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using DGraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using ArrayUGraph = Array_Graph<Graph_Anode<int>, Graph_Aarc<int>>;

  using UNode = UGraph::Node;
  using UArc = UGraph::Arc;

  using Edge = std::pair<size_t, size_t>;

  template <class GT>
  struct Built_Graph
  {
    GT g;
    std::vector<typename GT::Node *> nodes;
    std::vector<typename GT::Arc *> arcs;
  };


  Built_Graph<UGraph>
  build_ugraph(const size_t n,
               const std::vector<Edge> & edges)
  {
    Built_Graph<UGraph> built;
    built.nodes.reserve(n);
    built.arcs.reserve(edges.size());

    for (size_t i = 0; i < n; ++i)
      built.nodes.push_back(built.g.insert_node(static_cast<int>(i)));

    for (const auto & [u, v] : edges)
      if (u < n and v < n)
        built.arcs.push_back(built.g.insert_arc(built.nodes[u], built.nodes[v], 1));

    return built;
  }


  Built_Graph<DGraph>
  build_dgraph(const size_t n,
               const std::vector<Edge> & arcs)
  {
    Built_Graph<DGraph> built;
    built.nodes.reserve(n);
    built.arcs.reserve(arcs.size());

    for (size_t i = 0; i < n; ++i)
      built.nodes.push_back(built.g.insert_node(static_cast<int>(i)));

    for (const auto & [u, v] : arcs)
      if (u < n and v < n)
        built.arcs.push_back(built.g.insert_arc(built.nodes[u], built.nodes[v], 1));

    return built;
  }


  Built_Graph<ArrayUGraph>
  build_array_ugraph(const size_t n,
                     const std::vector<Edge> & edges)
  {
    Built_Graph<ArrayUGraph> built;
    built.nodes.reserve(n);
    built.arcs.reserve(edges.size());

    for (size_t i = 0; i < n; ++i)
      built.nodes.push_back(built.g.insert_node(static_cast<int>(i)));

    for (const auto & [u, v] : edges)
      if (u < n and v < n)
        built.arcs.push_back(built.g.insert_arc(built.nodes[u], built.nodes[v], 1));

    return built;
  }


  std::vector<Edge> complete_graph_edges(const size_t n)
  {
    std::vector<Edge> edges;
    for (size_t u = 0; u < n; ++u)
      for (size_t v = u + 1; v < n; ++v)
        edges.emplace_back(u, v);
    return edges;
  }


  std::vector<Edge> complete_bipartite_edges(const size_t left,
                                             const size_t right)
  {
    std::vector<Edge> edges;
    for (size_t u = 0; u < left; ++u)
      for (size_t v = 0; v < right; ++v)
        edges.emplace_back(u, left + v);
    return edges;
  }


  uint64_t dart_key(const int u, const int v)
  {
    return (static_cast<uint64_t>(static_cast<uint32_t>(u)) << 32)
           | static_cast<uint32_t>(v);
  }


  size_t component_count(const size_t n, const std::vector<Edge> & edges)
  {
    std::vector<std::vector<int>> adj(n);
    for (const auto & [u, v] : edges)
      {
        adj[u].push_back(static_cast<int>(v));
        adj[v].push_back(static_cast<int>(u));
      }

    std::vector<int> vis(n, 0);
    size_t comps = 0;

    for (size_t s = 0; s < n; ++s)
      {
        if (vis[s])
          continue;

        ++comps;
        std::vector<int> stack = {static_cast<int>(s)};
        vis[s] = 1;

        while (not stack.empty())
          {
            const int v = stack.back();
            stack.pop_back();
            for (const int w : adj[static_cast<size_t>(v)])
              if (not vis[static_cast<size_t>(w)])
                {
                  vis[static_cast<size_t>(w)] = 1;
                  stack.push_back(w);
                }
          }
      }

    return comps;
  }


  std::optional<bool>
  brute_force_planarity_oracle(const size_t n,
                               const std::vector<Edge> & edges,
                               const size_t max_combinations = 200000)
  {
    if (n <= 4)
      return true;

    if (edges.empty())
      return true;

    std::vector<std::vector<int>> adj(n);
    for (const auto & [u, v] : edges)
      {
        adj[u].push_back(static_cast<int>(v));
        adj[v].push_back(static_cast<int>(u));
      }

    const size_t m = edges.size();
    const size_t c = component_count(n, edges);

    // This bound is valid only for connected simple planar graphs.
    if (c == 1 and n >= 3 and m > 3 * n - 6)
      return false;

    std::vector<std::vector<std::vector<int>>> order_options(n);
    size_t combinations = 1;

    for (size_t v = 0; v < n; ++v)
      {
        auto neigh = adj[v];
        std::sort(neigh.begin(), neigh.end());

        const size_t d = neigh.size();
        if (d <= 2)
          {
            order_options[v].push_back(std::move(neigh));
            continue;
          }

        const int first = neigh.front();
        std::vector<int> tail(neigh.begin() + 1, neigh.end());
        std::sort(tail.begin(), tail.end());

        do
          {
            std::vector<int> order;
            order.reserve(d);
            order.push_back(first);
            order.insert(order.end(), tail.begin(), tail.end());
            order_options[v].push_back(std::move(order));
          }
        while (std::next_permutation(tail.begin(), tail.end()));

        combinations *= order_options[v].size();
        if (combinations > max_combinations)
          return std::nullopt;
      }

    std::vector<std::pair<int, int>> darts;
    darts.reserve(2 * m);
    std::unordered_map<uint64_t, int> dart_id;

    for (const auto & [u, v] : edges)
      {
        const int duv = static_cast<int>(darts.size());
        darts.emplace_back(static_cast<int>(u), static_cast<int>(v));
        dart_id.emplace(dart_key(static_cast<int>(u), static_cast<int>(v)), duv);

        const int dvu = static_cast<int>(darts.size());
        darts.emplace_back(static_cast<int>(v), static_cast<int>(u));
        dart_id.emplace(dart_key(static_cast<int>(v), static_cast<int>(u)), dvu);
      }

    std::vector<int> alpha(darts.size(), -1);
    for (size_t i = 0; i < darts.size(); ++i)
      {
        const auto [u, v] = darts[i];
        alpha[i] = dart_id.at(dart_key(v, u));
      }

    std::vector<size_t> selected(n, 0);
    std::vector<size_t> vars;
    vars.reserve(n);
    for (size_t v = 0; v < n; ++v)
      if (order_options[v].size() > 1)
        vars.push_back(v);

    std::sort(vars.begin(), vars.end(),
              [&order_options](const size_t a, const size_t b)
    {
      return order_options[a].size() > order_options[b].size();
    });

    std::function<bool()> evaluate = [&]() -> bool
    {
      std::vector<int> sigma(darts.size(), -1);

      for (size_t v = 0; v < n; ++v)
        {
          const auto & ord = order_options[v][selected[v]];
          if (ord.empty())
            continue;

          for (size_t i = 0; i < ord.size(); ++i)
            {
              const int to = ord[i];
              const int next = ord[(i + 1) % ord.size()];

              const int a = dart_id.at(dart_key(static_cast<int>(v), to));
              const int b = dart_id.at(dart_key(static_cast<int>(v), next));
              sigma[a] = b;
            }
        }

      for (const int s : sigma)
        if (s < 0)
          return false;

      std::vector<int> vis(darts.size(), 0);
      size_t faces = 0;
      for (size_t d = 0; d < darts.size(); ++d)
        {
          if (vis[d])
            continue;

          ++faces;
          size_t x = d;
          while (not vis[x])
            {
              vis[x] = 1;
              x = static_cast<size_t>(sigma[static_cast<size_t>(alpha[x])]);
            }
        }

      size_t isolated_vertices = 0;
      for (size_t v = 0; v < n; ++v)
        if (adj[v].empty())
          ++isolated_vertices;
      const size_t component_faces = faces + isolated_vertices;
      const size_t global_faces = component_faces - (c == 0 ? 0 : (c - 1));

      return static_cast<long long>(n)
             - static_cast<long long>(m)
             + static_cast<long long>(global_faces)
             == static_cast<long long>(c) + 1;
    };

    std::function<bool(size_t)> dfs = [&](const size_t pos) -> bool
    {
      if (pos == vars.size())
        return evaluate();

      const size_t v = vars[pos];
      for (size_t i = 0; i < order_options[v].size(); ++i)
        {
          selected[v] = i;
          if (dfs(pos + 1))
            return true;
        }

      return false;
    };

    return dfs(0);
  }


  std::string
  shell_quote(const std::string & s)
  {
    std::string q = "'";
    for (size_t i = 0; i < s.size(); ++i)
      {
        const char c = s[i];
        if (c == '\'')
          q += "'\\''";
        else
          q += c;
      }
    q += "'";
    return q;
  }


  std::optional<std::filesystem::path>
  find_repo_root()
  {
    namespace fs = std::filesystem;
    fs::path p = fs::current_path();

    for (size_t i = 0; i < 12; ++i)
      {
        if (fs::exists(p / "Planarity_Test.H") and fs::exists(p / "scripts"))
          return p;

        if (p == p.root_path())
          break;

        p = p.parent_path();
      }

    return std::nullopt;
  }


  std::string
  make_tmp_path(const std::string & stem,
                const std::string & ext)
  {
    static size_t seq = 0;
    namespace fs = std::filesystem;
    std::ostringstream out;
    out << (fs::temp_directory_path() / "aleph_planarity").string()
        << "_" << stem << "_" << ++seq << ext;
    return out.str();
  }


  void
  write_text_file(const std::string & path,
                  const std::string & content)
  {
    std::ofstream out(path);
    out << content;
  }


  std::string
  read_text_file(const std::string & path)
  {
    std::ifstream in(path);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
  }


  // Run a shell command, redirecting only stdout.  Stderr passes through
  // to the test output so that Ruby script errors are visible in CI logs.
  // Clears GITHUB_OUTPUT so scripts don't try to write to the CI output
  // file when invoked as test sub-processes.
  int
  run_script_cmd(const std::string & cmd_str, const std::string & label)
  {
    namespace fs = std::filesystem;
    const std::string stdout_path =
        (fs::temp_directory_path() / ("aleph_planarity_" + label + ".stdout")).string();
    const std::string full =
        "GITHUB_OUTPUT='' " + cmd_str + " > " + shell_quote(stdout_path);
    return std::system(full.c_str());
  }


  int
  run_external_certificate_validator(const std::vector<std::string> & inputs,
                                     const bool use_networkx = false,
                                     const bool require_networkx = false,
                                     const bool use_gephi = false,
                                     const bool require_gephi = false,
                                     const std::string & gephi_cmd = "",
                                     const std::string & extra_args = "")
  {
    namespace fs = std::filesystem;
    const auto root = find_repo_root();
    if (not root.has_value())
      return -999;

    const fs::path script = *root / "scripts" / "planarity_certificate_validator.rb";
    if (not fs::exists(script))
      return -998;

    std::ostringstream cmd;
    cmd << "ruby " << shell_quote(script.string());

    for (size_t i = 0; i < inputs.size(); ++i)
      cmd << " --input " << shell_quote(inputs[i]);

    if (use_networkx)
      cmd << " --networkx";
    if (require_networkx)
      cmd << " --require-networkx";
    if (use_gephi)
      cmd << " --gephi";
    if (require_gephi)
      cmd << " --require-gephi";
    if (not gephi_cmd.empty())
      cmd << " --gephi-cmd " << shell_quote(gephi_cmd);
    if (not extra_args.empty())
      cmd << " " << extra_args;

    return run_script_cmd(cmd.str(), "validator");
  }


  int
  run_external_certificate_batch(const std::vector<std::string> & inputs,
                                 const std::string & report_path,
                                 const std::string & extra_args = "")
  {
    namespace fs = std::filesystem;
    const auto root = find_repo_root();
    if (not root.has_value())
      return -997;

    const fs::path script = *root / "scripts" / "planarity_certificate_ci_batch.rb";
    if (not fs::exists(script))
      return -996;

    std::ostringstream cmd;
    cmd << "ruby " << shell_quote(script.string());
    for (size_t i = 0; i < inputs.size(); ++i)
      cmd << " --input " << shell_quote(inputs[i]);
    cmd << " --report " << shell_quote(report_path);

    if (not extra_args.empty())
      cmd << " " << extra_args;

    return run_script_cmd(cmd.str(), "batch");
  }


  int
  run_external_certificate_visual_diff(const std::vector<std::string> & inputs,
                                       const std::vector<std::string> & profiles,
                                       const std::string & report_path,
                                       const std::string & extra_args = "")
  {
    namespace fs = std::filesystem;
    const auto root = find_repo_root();
    if (not root.has_value())
      return -995;

    const fs::path script = *root / "scripts" / "planarity_certificate_ci_visual_diff.rb";
    if (not fs::exists(script))
      return -994;

    std::ostringstream cmd;
    cmd << "ruby " << shell_quote(script.string());

    for (size_t i = 0; i < inputs.size(); ++i)
      cmd << " --input " << shell_quote(inputs[i]);

    for (size_t i = 0; i < profiles.size(); ++i)
      cmd << " --profile " << shell_quote(profiles[i]);

    cmd << " --report " << shell_quote(report_path);
    if (not extra_args.empty())
      cmd << " " << extra_args;

    return run_script_cmd(cmd.str(), "visual");
  }


  int
  run_gephi_nightly_comparison(const std::string & artifacts_root,
                               const std::string & report_json,
                               const std::string & report_md,
                               const std::string & extra_args = "")
  {
    namespace fs = std::filesystem;
    const auto root = find_repo_root();
    if (not root.has_value())
      return -993;

    const fs::path script = *root / "scripts" / "planarity_gephi_weekly_comparison.rb";
    if (not fs::exists(script))
      return -992;

    std::ostringstream cmd;
    cmd << "ruby " << shell_quote(script.string())
        << " --artifacts-root " << shell_quote(artifacts_root)
        << " --resolved-tags " << shell_quote("v0.9.7,v0.10.1")
        << " --run-id " << shell_quote("test-run")
        << " --run-attempt " << shell_quote("1")
        << " --git-sha " << shell_quote("deadbeef")
        << " --report-json " << shell_quote(report_json)
        << " --report-md " << shell_quote(report_md);

    if (not extra_args.empty())
      cmd << " " << extra_args;

    return run_script_cmd(cmd.str(), "gephi_comparison");
  }


  int
  run_gephi_regression_notify(const std::string & report_json,
                              const std::string & output_md,
                              const std::string & extra_args = "")
  {
    namespace fs = std::filesystem;
    const auto root = find_repo_root();
    if (not root.has_value())
      return -991;

    const fs::path script = *root / "scripts" / "planarity_gephi_regression_notify.rb";
    if (not fs::exists(script))
      return -990;

    std::ostringstream cmd;
    cmd << "ruby " << shell_quote(script.string())
        << " --report-json " << shell_quote(report_json)
        << " --output-md " << shell_quote(output_md)
        << " --repository " << shell_quote("lrleon/Aleph-w")
        << " --run-url " << shell_quote("https://example.invalid/run/123");

    if (not extra_args.empty())
      cmd << " " << extra_args;

    return run_script_cmd(cmd.str(), "gephi_notify");
  }
  // Returns the path where run_external_certificate_validator writes stdout.
  std::string
  validator_stdout_path()
  {
    namespace fs = std::filesystem;
    return (fs::temp_directory_path() / "aleph_planarity_validator.stdout").string();
  }


  // Returns true when the planarity_certificate_validator.rb script can be
  // found in the repository and Ruby is executable on this machine.
  bool
  has_ruby_and_validator_script()
  {
    namespace fs = std::filesystem;
    const auto root = find_repo_root();
    if (not root.has_value())
      return false;
    if (not fs::exists(*root / "scripts" / "planarity_certificate_validator.rb"))
      return false;
    return std::system("ruby --version > /dev/null 2>&1") == 0;
  }


  // Returns true when python3 is executable on this machine.
  bool
  has_python3()
  {
    return std::system("python3 --version > /dev/null 2>&1") == 0;
  }

} // namespace


TEST(PlanarityTest, EmptyGraphIsPlanar)
{
  UGraph g;
  const auto r = planarity_test(g);

  EXPECT_TRUE(r.is_planar);
  EXPECT_EQ(r.num_nodes, 0u);
  EXPECT_EQ(r.simplified_num_edges, 0u);
}


TEST(PlanarityTest, TreeIsPlanar)
{
  const std::vector<Edge> edges = {
      {0, 1}, {1, 2}, {1, 3}, {3, 4}, {3, 5}
  };

  auto built = build_ugraph(6, edges);
  EXPECT_TRUE(is_planar_graph(built.g));
}


TEST(PlanarityTest, K5IsNonPlanar)
{
  auto built = build_ugraph(5, complete_graph_edges(5));
  const auto r = planarity_test(built.g);

  EXPECT_FALSE(r.is_planar);
  EXPECT_TRUE(r.failed_euler_bound);
}


TEST(PlanarityTest, K33IsNonPlanar)
{
  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));
  const auto r = planarity_test(built.g);

  EXPECT_FALSE(r.is_planar);
}


TEST(PlanarityTest, K5CertificateIsExtractedWhenEnabled)
{
  auto built = build_ugraph(5, complete_graph_edges(5));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;

  const auto r = planarity_test(built.g, opts);

  EXPECT_FALSE(r.is_planar);
  EXPECT_TRUE(r.has_nonplanar_certificate);
  EXPECT_EQ(r.certificate_type, Planarity_Certificate_Type::K5_Subdivision);
  EXPECT_EQ(r.certificate_branch_nodes.size(), 5u);
  EXPECT_EQ(r.certificate_paths.size(), 10u);
}


TEST(PlanarityTest, K33CertificateIsExtractedWhenEnabled)
{
  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;

  const auto r = planarity_test(built.g, opts);

  EXPECT_FALSE(r.is_planar);
  EXPECT_TRUE(r.has_nonplanar_certificate);
  EXPECT_EQ(r.certificate_type, Planarity_Certificate_Type::K33_Subdivision);
  EXPECT_EQ(r.certificate_branch_nodes.size(), 6u);
  EXPECT_EQ(r.certificate_paths.size(), 9u);
}


TEST(PlanarityTest, CertificateObstructionEdgesMapToOriginalInputArcs)
{
  auto built = build_ugraph(5, complete_graph_edges(5));

  // Add extra parallel inputs for edge {0,1}.
  built.arcs.push_back(built.g.insert_arc(built.nodes[0], built.nodes[1], 1));
  built.arcs.push_back(built.g.insert_arc(built.nodes[1], built.nodes[0], 1));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;

  const auto r = planarity_test(built.g, opts);

  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);
  ASSERT_EQ(r.certificate_type, Planarity_Certificate_Type::K5_Subdivision);
  ASSERT_EQ(r.certificate_obstruction_edges.size(), 10u);

  bool found_01 = false;
  for (typename Array<Planarity_Test_Result<UGraph>::Edge_Witness>::Iterator
       it(r.certificate_obstruction_edges); it.has_curr(); it.next_ne())
    {
      const auto & w = it.get_curr_ne();
      EXPECT_NE(w.representative_input_arc, nullptr);
      EXPECT_GE(w.input_arcs.size(), 1u);

      const bool is_01 = (w.src == built.nodes[0] and w.tgt == built.nodes[1])
                         or (w.src == built.nodes[1] and w.tgt == built.nodes[0]);
      if (is_01)
        {
          found_01 = true;
          EXPECT_GE(w.input_arcs.size(), 3u);
        }
    }

  EXPECT_TRUE(found_01);
}


TEST(PlanarityTest, CertificatePathsExposeTraceableOriginalEdges)
{
  auto edges = complete_bipartite_edges(3, 3);

  // Replace edge (0,3) by 0-6-3 to force at least one subdivided witness path.
  auto it = std::find(edges.begin(), edges.end(), Edge{0, 3});
  ASSERT_NE(it, edges.end());
  edges.erase(it);
  edges.emplace_back(0, 6);
  edges.emplace_back(6, 3);

  auto built = build_ugraph(7, edges);

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;

  const auto r = planarity_test(built.g, opts);

  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);
  ASSERT_EQ(r.certificate_type, Planarity_Certificate_Type::K33_Subdivision);
  ASSERT_FALSE(r.certificate_paths.is_empty());

  bool found_subdivided_path = false;
  for (typename Array<Planarity_Test_Result<UGraph>::Path_Witness>::Iterator
       pit(r.certificate_paths); pit.has_curr(); pit.next_ne())
    {
      const auto & p = pit.get_curr_ne();
      ASSERT_GE(p.nodes.size(), 2u);
      EXPECT_EQ(p.edges.size(), p.nodes.size() - 1);

      if (p.nodes.size() > 2)
        found_subdivided_path = true;

      for (typename Array<Planarity_Test_Result<UGraph>::Edge_Witness>::Iterator
           eit(p.edges); eit.has_curr(); eit.next_ne())
        {
          const auto & e = eit.get_curr_ne();
          EXPECT_NE(e.representative_input_arc, nullptr);
          EXPECT_GE(e.input_arcs.size(), 1u);
        }
    }

  EXPECT_TRUE(found_subdivided_path);
}


TEST(PlanarityTest, K5MinusEdgeIsPlanar)
{
  auto edges = complete_graph_edges(5);
  edges.erase(edges.begin());

  auto built = build_ugraph(5, edges);
  const auto r = planarity_test(built.g);

  EXPECT_TRUE(r.is_planar);
}


TEST(PlanarityTest, PlanarEmbeddingIsExtractedWhenEnabled)
{
  auto edges = complete_graph_edges(5);
  edges.erase(edges.begin()); // K5 - e, planar

  auto built = build_ugraph(5, edges);

  Planarity_Test_Options opts;
  opts.compute_embedding = true;

  const auto r = planarity_test(built.g, opts);

  EXPECT_TRUE(r.is_planar);
  EXPECT_TRUE(r.has_combinatorial_embedding);
  EXPECT_FALSE(r.embedding_search_truncated);
  EXPECT_EQ(r.embedding_rotation.size(), 5u);

  const size_t expected_faces = edges.size() - 5 + 2; // connected Euler
  EXPECT_EQ(r.embedding_num_faces, expected_faces);
}


TEST(PlanarityTest, PlanarDualMetadataConnectedGraph)
{
  auto built = build_ugraph(5, {
      {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0}, {0, 2}
  });

  Planarity_Test_Options opts;
  opts.compute_embedding = true;
  opts.embedding_allow_bruteforce_fallback = false;

  const auto r = planarity_test(built.g, opts);
  ASSERT_TRUE(r.is_planar);
  ASSERT_TRUE(r.has_combinatorial_embedding);

  const auto md = planar_dual_metadata(r);
  EXPECT_TRUE(md.has_embedding);
  EXPECT_EQ(md.num_components, 1u);
  EXPECT_EQ(md.num_faces_local, r.embedding_num_faces);
  EXPECT_EQ(md.num_faces_global, r.embedding_num_faces);
  EXPECT_FALSE(md.faces_are_component_local);
  EXPECT_EQ(md.dual_edges.size(), r.simplified_num_edges);
  EXPECT_EQ(md.face_adjacency.size(), md.num_faces_local);
}


TEST(PlanarityTest, PlanarDualMetadataTreeHasSingleFaceAndLoops)
{
  auto built = build_ugraph(4, {
      {0, 1}, {1, 2}, {1, 3}
  });

  Planarity_Test_Options opts;
  opts.compute_embedding = true;
  opts.embedding_allow_bruteforce_fallback = false;

  const auto r = planarity_test(built.g, opts);
  ASSERT_TRUE(r.is_planar);
  ASSERT_TRUE(r.has_combinatorial_embedding);

  const auto md = planar_dual_metadata(r);
  EXPECT_EQ(md.num_faces_local, 1u);
  EXPECT_EQ(md.num_faces_global, 1u);
  EXPECT_EQ(md.dual_edges.size(), 3u);

  for (typename Array<Planar_Dual_Edge_Info<UGraph>>::Iterator
       it(md.dual_edges); it.has_curr(); it.next_ne())
    {
      const auto & e = it.get_curr_ne();
      EXPECT_EQ(e.face_a, 0u);
      EXPECT_EQ(e.face_b, 0u);
    }

  auto dual = build_planar_dual_graph<UGraph>(md);
  EXPECT_EQ(dual.get_num_nodes(), 1u);
  EXPECT_EQ(dual.get_num_arcs(), 3u);
}


TEST(PlanarityTest, PlanarDualMetadataDisconnectedUsesComponentLocalFaces)
{
  auto built = build_ugraph(3, {
      {0, 1}
  });

  Planarity_Test_Options opts;
  opts.compute_embedding = true;
  opts.embedding_allow_bruteforce_fallback = false;

  const auto r = planarity_test(built.g, opts);
  ASSERT_TRUE(r.is_planar);
  ASSERT_TRUE(r.has_combinatorial_embedding);

  const auto md = planar_dual_metadata(r);
  EXPECT_EQ(md.num_components, 2u);
  EXPECT_EQ(md.num_faces_local, 2u);
  EXPECT_EQ(md.num_faces_global, 1u);
  EXPECT_TRUE(md.faces_are_component_local);
}


TEST(PlanarityTest, PlanarDualMetadataRequiresEmbedding)
{
  auto built = build_ugraph(4, {
      {0, 1}, {1, 2}, {2, 3}
  });

  const auto r = planarity_test(built.g); // no embedding requested
  EXPECT_TRUE(r.is_planar);
  EXPECT_FALSE(r.has_combinatorial_embedding);

  EXPECT_ANY_THROW((void) planar_dual_metadata(r));
}


TEST(PlanarityTest, LinearLREmbeddingWorksOnSimplePlanarSample)
{
  auto built = build_ugraph(5, {
      {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0}, {0, 2}
  });

  Planarity_Test_Options opts;
  opts.compute_embedding = true;
  opts.embedding_allow_bruteforce_fallback = false;

  const auto r = planarity_test(built.g, opts);

  EXPECT_TRUE(r.is_planar);
  EXPECT_TRUE(r.has_combinatorial_embedding);
  EXPECT_TRUE(r.embedding_is_lr_linear);
}


TEST(PlanarityTest, K5MinusEdgeHasStrictLREmbeddingWithoutFallback)
{
  auto edges = complete_graph_edges(5);
  edges.erase(edges.begin()); // K5 - e, planar
  auto built = build_ugraph(5, edges);

  Planarity_Test_Options strict_opts;
  strict_opts.compute_embedding = true;
  strict_opts.embedding_allow_bruteforce_fallback = false;

  const auto r_strict = planarity_test(built.g, strict_opts);

  EXPECT_TRUE(r_strict.is_planar);
  EXPECT_TRUE(r_strict.has_combinatorial_embedding);
  EXPECT_TRUE(r_strict.embedding_is_lr_linear);
  EXPECT_FALSE(r_strict.embedding_search_truncated);

  Planarity_Test_Options robust_opts = strict_opts;
  robust_opts.embedding_allow_bruteforce_fallback = true;

  const auto r_robust = planarity_test(built.g, robust_opts);
  EXPECT_TRUE(r_robust.is_planar);
  EXPECT_TRUE(r_robust.has_combinatorial_embedding);
}


TEST(PlanarityTest, StrictLREmbeddingCanBeBudgetTruncated)
{
  auto edges = complete_graph_edges(5);
  edges.erase(edges.begin()); // K5 - e, planar
  auto built = build_ugraph(5, edges);

  Planarity_Test_Options opts;
  opts.compute_embedding = true;
  opts.embedding_allow_bruteforce_fallback = false;
  opts.embedding_max_combinations = 1; // very low LR local-repair budget

  const auto r = planarity_test(built.g, opts);

  EXPECT_TRUE(r.is_planar);
  EXPECT_FALSE(r.has_combinatorial_embedding);
  EXPECT_TRUE(r.embedding_search_truncated);
}


TEST(PlanarityTest, EmbeddingCanBeTruncatedByCombinationBound)
{
  auto edges = complete_graph_edges(5);
  edges.erase(edges.begin()); // still enough rotation combinations
  auto built = build_ugraph(5, edges);

  Planarity_Test_Options opts;
  opts.compute_embedding = true;
  opts.embedding_prefer_lr_linear = false;
  opts.embedding_max_combinations = 1;

  const auto r = planarity_test(built.g, opts);

  EXPECT_TRUE(r.is_planar);
  EXPECT_FALSE(r.has_combinatorial_embedding);
  EXPECT_TRUE(r.embedding_search_truncated);
}


TEST(PlanarityTest, K33MinusEdgeIsPlanar)
{
  auto edges = complete_bipartite_edges(3, 3);
  edges.erase(edges.begin());

  auto built = build_ugraph(6, edges);
  const auto r = planarity_test(built.g);

  EXPECT_TRUE(r.is_planar);
}


TEST(PlanarityTest, SubdivisionOfK33IsNonPlanar)
{
  auto edges = complete_bipartite_edges(3, 3);

  // Replace edge (0,3) by 0-6-3
  auto it = std::find(edges.begin(), edges.end(), Edge{0, 3});
  ASSERT_NE(it, edges.end());
  edges.erase(it);
  edges.emplace_back(0, 6);
  edges.emplace_back(6, 3);

  auto built = build_ugraph(7, edges);
  const auto r = planarity_test(built.g);

  EXPECT_FALSE(r.is_planar);
}


TEST(PlanarityTest, CertificateCanBeTruncatedByEdgeBudget)
{
  auto built = build_ugraph(5, complete_graph_edges(5));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  opts.certificate_max_edges = 5; // K5 has 10 simplified edges

  const auto r = planarity_test(built.g, opts);

  EXPECT_FALSE(r.is_planar);
  EXPECT_FALSE(r.has_nonplanar_certificate);
  EXPECT_TRUE(r.certificate_search_truncated);
}


TEST(PlanarityTest, CertificateDetectsK5InsideBiggerGraph)
{
  auto built = build_ugraph(8, {
      {0, 1}, {0, 2}, {0, 3}, {0, 4},
      {1, 2}, {1, 3}, {1, 4},
      {2, 3}, {2, 4}, {3, 4}, // K5 on 0..4
      {2, 5}, {5, 6}, {6, 7} // extra planar tail
  });

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;

  const auto r = planarity_test(built.g, opts);

  EXPECT_FALSE(r.is_planar);
  EXPECT_TRUE(r.has_nonplanar_certificate);
  EXPECT_EQ(r.certificate_type, Planarity_Certificate_Type::K5_Subdivision);
  EXPECT_EQ(r.certificate_branch_nodes.size(), 5u);
  EXPECT_EQ(r.certificate_paths.size(), 10u);
}


TEST(PlanarityTest, CertificateDetectsK33InsideBiggerGraph)
{
  auto built = build_ugraph(9, {
      {0, 3}, {0, 4}, {0, 5},
      {1, 3}, {1, 4}, {1, 5},
      {2, 3}, {2, 4}, {2, 5}, // K3,3 on {0,1,2} x {3,4,5}
      {0, 8}, {8, 7}, {7, 6} // extra planar tail
  });

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;

  const auto r = planarity_test(built.g, opts);

  EXPECT_FALSE(r.is_planar);
  EXPECT_TRUE(r.has_nonplanar_certificate);
  EXPECT_EQ(r.certificate_type, Planarity_Certificate_Type::K33_Subdivision);
  EXPECT_EQ(r.certificate_branch_nodes.size(), 6u);
  EXPECT_EQ(r.certificate_paths.size(), 9u);
}


TEST(PlanarityTest, CertificateBranchSearchCanBeTruncated)
{
  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  opts.certificate_max_branch_nodes_search = 4;

  const auto r = planarity_test(built.g, opts);

  EXPECT_FALSE(r.is_planar);
  EXPECT_TRUE(r.has_nonplanar_certificate);
  EXPECT_EQ(r.certificate_type,
            Planarity_Certificate_Type::Minimal_NonPlanar_Obstruction);
  EXPECT_TRUE(r.certificate_search_truncated);
}


TEST(PlanarityTest, DisconnectedWithNonPlanarComponentIsNonPlanar)
{
  std::vector<Edge> edges = complete_graph_edges(5); // non-planar component
  edges.emplace_back(6, 7); // independent planar component

  auto built = build_ugraph(8, edges);
  const auto r = planarity_test(built.g);

  EXPECT_FALSE(r.is_planar);
}


TEST(PlanarityTest, DisconnectedPlanarGraphIsNotRejectedByGlobalEulerBound)
{
  // Connected component {0,1,2,4} has n=4, m=4 and is planar.
  // Node 3 is isolated, so a naive global bound m <= 3n - 6c would fail here.
  auto built = build_ugraph(5, {
      {0, 2}, {1, 2}, {1, 4}, {2, 4}
  });

  const auto r = planarity_test(built.g);

  EXPECT_TRUE(r.is_planar);
  EXPECT_FALSE(r.failed_euler_bound);
}


TEST(PlanarityTest, LoopsAndParallelArcsAreIgnoredInNormalization)
{
  auto built = build_ugraph(3, {{0, 1}, {1, 2}, {2, 0}});

  // Add loops
  built.arcs.push_back(built.g.insert_arc(built.nodes[0], built.nodes[0], 1));
  built.arcs.push_back(built.g.insert_arc(built.nodes[2], built.nodes[2], 1));

  // Add parallel edges
  built.arcs.push_back(built.g.insert_arc(built.nodes[0], built.nodes[1], 1));
  built.arcs.push_back(built.g.insert_arc(built.nodes[1], built.nodes[0], 1));

  const auto r = planarity_test(built.g);

  EXPECT_TRUE(r.is_planar);
  EXPECT_GE(r.ignored_loops, 2u);
  EXPECT_GE(r.ignored_parallel_arcs, 2u);
}


TEST(PlanarityTest, DirectedInputUsesUnderlyingUndirectedGraph)
{
  // Oriented K3,3
  auto built = build_dgraph(6, {
      {0, 3}, {0, 4}, {0, 5},
      {1, 3}, {1, 4}, {1, 5},
      {2, 3}, {2, 4}, {2, 5}
  });

  const auto r = planarity_test(built.g);
  EXPECT_TRUE(r.input_is_digraph);
  EXPECT_FALSE(r.is_planar);
}


TEST(PlanarityTest, ArrayGraphBackendWorks)
{
  auto planar = build_array_ugraph(5, {
      {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0}, {0, 2}
  });
  auto non_planar = build_array_ugraph(5, complete_graph_edges(5));

  EXPECT_TRUE(is_planar_graph(planar.g));
  EXPECT_FALSE(is_planar_graph(non_planar.g));
}


TEST(PlanarityTest, FunctorApisMatchFreeFunctions)
{
  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  const bool b1 = is_planar_graph(built.g);
  const bool b2 = Is_Planar_Graph<UGraph>()(built.g);
  const auto r1 = planarity_test(built.g);
  const auto r2 = Planarity_Test<UGraph>()(built.g);

  EXPECT_EQ(b1, b2);
  EXPECT_EQ(r1.is_planar, r2.is_planar);
}


TEST(PlanarityTest, RandomSmallGraphsMatchBruteforceOracle)
{
  std::mt19937_64 rng(0xA11CE123456789ULL);
  std::uniform_int_distribution<int> n_dist(1, 7);
  std::bernoulli_distribution has_edge(0.35);

  size_t checked = 0;
  for (size_t trial = 0; trial < 3000 and checked < 70; ++trial)
    {
      const size_t n = static_cast<size_t>(n_dist(rng));
      std::vector<Edge> edges;

      for (size_t u = 0; u < n; ++u)
        for (size_t v = u + 1; v < n; ++v)
          if (has_edge(rng))
            edges.emplace_back(u, v);

      const auto oracle = brute_force_planarity_oracle(n, edges);
      if (not oracle.has_value())
        continue;

      auto built = build_ugraph(n, edges);
      const auto got = is_planar_graph(built.g);
      EXPECT_EQ(got, *oracle) << "trial=" << trial;
      ++checked;
    }

  EXPECT_GE(checked, 50u);
}


TEST(PlanarityTest, RandomSmallPlanarGraphsHaveStrictLREmbedding)
{
  std::mt19937_64 rng(0xD00D1234A11CEULL);
  std::uniform_int_distribution<int> n_dist(1, 7);
  std::bernoulli_distribution has_edge(0.36);

  Planarity_Test_Options strict_opts;
  strict_opts.compute_embedding = true;
  strict_opts.embedding_allow_bruteforce_fallback = false;
  Planarity_Test_Options robust_opts = strict_opts;
  robust_opts.embedding_allow_bruteforce_fallback = true;

  size_t checked = 0;
  for (size_t trial = 0; trial < 5000 and checked < 70; ++trial)
    {
      const size_t n = static_cast<size_t>(n_dist(rng));
      std::vector<Edge> edges;

      for (size_t u = 0; u < n; ++u)
        for (size_t v = u + 1; v < n; ++v)
          if (has_edge(rng))
            edges.emplace_back(u, v);

      const auto oracle = brute_force_planarity_oracle(n, edges);
      if (not oracle.has_value() or not *oracle)
        continue;

      auto built = build_ugraph(n, edges);
      const auto r_strict = planarity_test(built.g, strict_opts);
      const auto r_robust = planarity_test(built.g, robust_opts);

      SCOPED_TRACE("trial=" + std::to_string(trial)
                   + " n=" + std::to_string(n)
                   + " m=" + std::to_string(edges.size()));

      EXPECT_TRUE(r_strict.is_planar);
      EXPECT_TRUE(r_strict.has_combinatorial_embedding);
      EXPECT_TRUE(r_strict.embedding_is_lr_linear);
      EXPECT_FALSE(r_strict.embedding_search_truncated);

      EXPECT_TRUE(r_robust.is_planar);
      EXPECT_TRUE(r_robust.has_combinatorial_embedding);

      ++checked;
    }

  EXPECT_GE(checked, 50u);
}


TEST(PlanarityTest, PlanarGeometricDrawingRequiresEmbedding)
{
  auto built = build_ugraph(3, {
      {0, 1}, {1, 2}, {2, 0}
  });

  const auto r = planarity_test(built.g); // no embedding requested
  EXPECT_TRUE(r.is_planar);
  EXPECT_FALSE(r.has_combinatorial_embedding);
  EXPECT_ANY_THROW((void) planar_geometric_drawing(r));
}


TEST(PlanarityTest, PlanarGeometricDrawingConnectedIsCrossingFree)
{
  auto built = build_ugraph(5, {
      {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0}, {0, 2}
  });

  Planarity_Test_Options opts;
  opts.compute_embedding = true;
  opts.embedding_allow_bruteforce_fallback = false;

  const auto r = planarity_test(built.g, opts);
  ASSERT_TRUE(r.is_planar);
  ASSERT_TRUE(r.has_combinatorial_embedding);

  const auto d = planar_geometric_drawing(r);
  EXPECT_TRUE(d.drawing_available);
  EXPECT_TRUE(d.drawing_validated_no_crossings);
  EXPECT_EQ(d.crossing_count, 0u);
  EXPECT_EQ(d.node_positions.size(), r.simplified_num_nodes);

  bool distinct = false;
  for (size_t i = 1; i < d.node_positions.size(); ++i)
    if (std::abs(d.node_positions[i].x - d.node_positions[0].x) > 1e-9
        or std::abs(d.node_positions[i].y - d.node_positions[0].y) > 1e-9)
      {
        distinct = true;
        break;
      }
  EXPECT_TRUE(distinct);
}


TEST(PlanarityTest, PlanarGeometricDrawingDensePlanarSampleWorksInStrictMode)
{
  auto edges = complete_graph_edges(5);
  edges.erase(edges.begin()); // K5 - e
  auto built = build_ugraph(5, edges);

  Planarity_Test_Options opts;
  opts.compute_embedding = true;
  opts.embedding_allow_bruteforce_fallback = false;

  const auto r = planarity_test(built.g, opts);
  ASSERT_TRUE(r.is_planar);
  ASSERT_TRUE(r.has_combinatorial_embedding);

  const auto d = planar_geometric_drawing(r);
  EXPECT_TRUE(d.drawing_available);
  EXPECT_TRUE(d.drawing_validated_no_crossings);
  EXPECT_EQ(d.crossing_count, 0u);
  EXPECT_NE(d.chosen_outer_face, std::numeric_limits<size_t>::max());
}


TEST(PlanarityTest, PlanarGeometricDrawingDisconnectedComponentsAreSeparated)
{
  auto built = build_ugraph(6, {
      {0, 1}, {1, 2}, {2, 0}, // component 1
      {3, 4}, {4, 5}, {5, 3}  // component 2
  });

  Planarity_Test_Options opts;
  opts.compute_embedding = true;
  opts.embedding_allow_bruteforce_fallback = false;
  const auto r = planarity_test(built.g, opts);
  ASSERT_TRUE(r.is_planar);
  ASSERT_TRUE(r.has_combinatorial_embedding);

  const auto d = planar_geometric_drawing(r);
  EXPECT_TRUE(d.drawing_available);
  EXPECT_TRUE(d.drawing_validated_no_crossings);
  EXPECT_EQ(d.num_components, 2u);

  double min_x_a = std::numeric_limits<double>::max();
  double max_x_a = -std::numeric_limits<double>::max();
  double min_x_b = std::numeric_limits<double>::max();
  double max_x_b = -std::numeric_limits<double>::max();

  for (typename Array<typename Planar_Geometric_Drawing<UGraph>::Node_Position>::Iterator
       it(d.node_positions); it.has_curr(); it.next_ne())
    {
      const auto & p = it.get_curr_ne();
      if (p.node == built.nodes[0] or p.node == built.nodes[1] or p.node == built.nodes[2])
        {
          min_x_a = std::min(min_x_a, p.x);
          max_x_a = std::max(max_x_a, p.x);
        }
      else
        {
          min_x_b = std::min(min_x_b, p.x);
          max_x_b = std::max(max_x_b, p.x);
        }
    }

  EXPECT_GT(min_x_b - max_x_a, 0.5);
}


TEST(PlanarityTest, PlanarGeometricDrawingCanBeTruncatedByFaceBudget)
{
  auto built = build_ugraph(5, {
      {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0}, {0, 2}
  });

  Planarity_Test_Options opts;
  opts.compute_embedding = true;
  opts.embedding_allow_bruteforce_fallback = false;
  const auto r = planarity_test(built.g, opts);
  ASSERT_TRUE(r.is_planar);
  ASSERT_TRUE(r.has_combinatorial_embedding);

  Planar_Geometric_Drawing_Options dopt;
  dopt.max_outer_faces_to_try = 0;

  const auto d = planar_geometric_drawing(r, dopt);
  EXPECT_FALSE(d.drawing_available);
  EXPECT_TRUE(d.drawing_search_truncated);
}


TEST(PlanarityTest, NonPlanarCertificateExportsToJsonAndDot)
{
  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;

  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  const auto json = nonplanar_certificate_to_json(r);
  const auto dot = nonplanar_certificate_to_dot(r);

  EXPECT_NE(json.find("\"certificate_type\": \"K33_Subdivision\""), std::string::npos);
  EXPECT_NE(json.find("\"obstruction_edges\""), std::string::npos);
  EXPECT_NE(dot.find("graph NonPlanarCertificate"), std::string::npos);
  EXPECT_NE(dot.find("obs x"), std::string::npos);
}


TEST(PlanarityTest, NonPlanarCertificateExportSupportsCustomLabelsAndOptions)
{
  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;

  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  NonPlanar_Certificate_Export_Options eopt;
  eopt.pretty_json = false;
  eopt.dot_highlight_paths = false;

  const auto labeler = [](UGraph::Node *) -> std::string
    {
      return "q\"\\\n";
    };

  const auto json = nonplanar_certificate_to_json(r, labeler, eopt);
  const auto dot = nonplanar_certificate_to_dot(r, labeler, eopt);

  EXPECT_EQ(json.find('\n'), std::string::npos);
  EXPECT_NE(json.find("q\\\"\\\\\\n"), std::string::npos);
  EXPECT_NE(dot.find("q\\\"\\\\\\n"), std::string::npos);
  EXPECT_EQ(dot.find("label=\"p"), std::string::npos);
  EXPECT_NE(dot.find("obs x"), std::string::npos);
}


TEST(PlanarityTest, NonPlanarCertificateExportsToGraphmlAndGexf)
{
  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  const auto graphml = nonplanar_certificate_to_graphml(r);
  const auto gexf = nonplanar_certificate_to_gexf(r);

  EXPECT_NE(graphml.find("<graphml"), std::string::npos);
  EXPECT_NE(graphml.find("kind"), std::string::npos);
  EXPECT_NE(graphml.find("obstruction"), std::string::npos);

  EXPECT_NE(gexf.find("<gexf"), std::string::npos);
  EXPECT_NE(gexf.find("defaultedgetype=\"undirected\""), std::string::npos);
  EXPECT_NE(gexf.find("obstruction"), std::string::npos);
}


TEST(PlanarityTest, NonPlanarCertificateExportExchangeFormatsCanDisablePathOverlay)
{
  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  NonPlanar_Certificate_Export_Options eopt;
  eopt.graphml_include_paths = false;
  eopt.gexf_include_paths = false;

  const auto graphml = nonplanar_certificate_to_graphml(r, Dft_Certificate_Node_Label<UGraph>(), eopt);
  const auto gexf = nonplanar_certificate_to_gexf(r, Dft_Certificate_Node_Label<UGraph>(), eopt);

  EXPECT_EQ(graphml.find(">path<"), std::string::npos);
  EXPECT_EQ(gexf.find("value=\"path\""), std::string::npos);
}


TEST(PlanarityTest, NonPlanarCertificateValidationAcceptsGeneratedCertificate)
{
  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  const auto vr = validate_nonplanar_certificate(r);
  EXPECT_TRUE(vr.has_certificate);
  EXPECT_TRUE(vr.is_valid);
  EXPECT_EQ(vr.num_obstruction_edges, r.certificate_obstruction_edges.size());
  EXPECT_EQ(vr.num_paths, r.certificate_paths.size());
  EXPECT_EQ(vr.path_edge_not_in_obstruction, 0u);
}


TEST(PlanarityTest, NonPlanarCertificateValidationDetectsTamperedPathEdge)
{
  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);
  ASSERT_FALSE(r.certificate_paths.is_empty());
  ASSERT_FALSE(r.certificate_paths[0].edges.is_empty());

  r.certificate_paths[0].edges[0].src = nullptr;

  const auto vr = validate_nonplanar_certificate(r);
  EXPECT_TRUE(vr.has_certificate);
  EXPECT_FALSE(vr.is_valid);
  EXPECT_GT(vr.null_path_edge_endpoints, 0u);
  EXPECT_FALSE(nonplanar_certificate_is_valid(r));
}


TEST(PlanarityTest, ExternalCertificateValidatorAcceptsGraphmlAndGexf)
{
  if (not has_ruby_and_validator_script())
    {
      GTEST_SKIP() << "Skipped: Ruby or planarity_certificate_validator.rb not found";
      return;
    }

  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  const std::string graphml_path = make_tmp_path("external_valid", ".graphml");
  const std::string gexf_path = make_tmp_path("external_valid", ".gexf");

  write_text_file(graphml_path, nonplanar_certificate_to_graphml(r));
  write_text_file(gexf_path, nonplanar_certificate_to_gexf(r));

  const int rc = run_external_certificate_validator({graphml_path, gexf_path});
  EXPECT_EQ(rc, 0);
}


TEST(PlanarityTest, ExternalCertificateValidatorRejectsMissingObstruction)
{
  if (not has_ruby_and_validator_script())
    {
      GTEST_SKIP() << "Skipped: Ruby or planarity_certificate_validator.rb not found";
      return;
    }

  const std::string broken_graphml = make_tmp_path("external_invalid", ".graphml");
  write_text_file(broken_graphml, R"(<?xml version="1.0" encoding="UTF-8"?>
<graphml xmlns="http://graphml.graphdrawing.org/xmlns">
  <graph id="Broken" edgedefault="undirected">
    <node id="n0"/>
    <node id="n1"/>
    <edge id="e0" source="n0" target="n1">
      <data key="k_edge_kind">path</data>
      <data key="k_edge_path_id">0</data>
    </edge>
  </graph>
</graphml>
)");

  const int rc = run_external_certificate_validator({broken_graphml});
  EXPECT_NE(rc, 0);
}


TEST(PlanarityTest, ExternalCertificateValidatorNetworkxModeIsPortable)
{

    if (not has_ruby_and_validator_script() or not has_python3())
    {
      GTEST_SKIP() << "Skipped: Ruby, validator script, or python3 not found";
      return;
    }

  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  const std::string graphml_path = make_tmp_path("external_nx", ".graphml");
  write_text_file(graphml_path, nonplanar_certificate_to_graphml(r));

  const int rc_optional = run_external_certificate_validator({graphml_path}, true, false);
  EXPECT_EQ(rc_optional, 0);

  const int has_nx = std::system(
      "python3 -c \"import importlib.util,sys; sys.exit(0 if importlib.util.find_spec('networkx') else 1)\"");
  const int rc_required = run_external_certificate_validator({graphml_path}, true, true);

  if (has_nx == 0)
    EXPECT_EQ(rc_required, 0);
  else
    EXPECT_NE(rc_required, 0);
}


TEST(PlanarityTest, ExternalCertificateValidatorGephiModeIsPortable)
{
  if (not has_ruby_and_validator_script())
    {
      GTEST_SKIP() << "Skipped: Ruby or planarity_certificate_validator.rb not found";
      return;
    }

  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  const std::string graphml_path = make_tmp_path("external_gephi", ".graphml");
  write_text_file(graphml_path, nonplanar_certificate_to_graphml(r));

  const int rc_optional = run_external_certificate_validator(
      {graphml_path}, false, false, true, false);
  EXPECT_EQ(rc_optional, 0);

  const int has_gephi_usable =
      std::system("gephi --version > /dev/null 2>&1");
  const int rc_required = run_external_certificate_validator(
      {graphml_path}, false, false, true, true);

  if (has_gephi_usable == 0)
    EXPECT_EQ(rc_required, 0);
  else
    EXPECT_NE(rc_required, 0);
}


TEST(PlanarityTest, ExternalCertificateValidatorGephiCustomTemplateWorks)
{
  if (not has_ruby_and_validator_script())
    {
      GTEST_SKIP() << "Skipped: Ruby or planarity_certificate_validator.rb not found";
      return;
    }

  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  const std::string graphml_path = make_tmp_path("external_gephi_tpl", ".graphml");
  write_text_file(graphml_path, nonplanar_certificate_to_graphml(r));

  const std::string cmd_template =
      "ruby -e \"File.stat(ARGV[0])\" {input}";

  const int rc = run_external_certificate_validator(
      {graphml_path}, false, false, true, true, cmd_template);
  EXPECT_EQ(rc, 0);
}


TEST(PlanarityTest, ExternalCertificateValidatorGephiTemplateSupportsInputPathWithSpaces)
{
  namespace fs = std::filesystem;

  if (not has_ruby_and_validator_script())
    {
      GTEST_SKIP() << "Skipped: Ruby or planarity_certificate_validator.rb not found";
      return;
    }

  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  const fs::path dir = fs::temp_directory_path() / "aleph planarity gephi spaces";
  fs::create_directories(dir);
  const std::string graphml_path = (dir / "k33 certificate.graphml").string();
  write_text_file(graphml_path, nonplanar_certificate_to_graphml(r));

  const std::string cmd_template =
      "ruby -e \"File.stat(ARGV[0])\" {input}";

  const int rc = run_external_certificate_validator(
      {graphml_path}, false, false, true, true, cmd_template);
  EXPECT_EQ(rc, 0);
}


TEST(PlanarityTest, ExternalCertificateValidatorCanListGephiTemplates)
{
  if (not has_ruby_and_validator_script())
    {
      GTEST_SKIP() << "Skipped: Ruby or planarity_certificate_validator.rb not found";
      return;
    }

  const int rc = run_external_certificate_validator(
      {}, false, false, false, false, "", "--list-gephi-templates --json");
  EXPECT_EQ(rc, 0);

  const std::string out = read_text_file(validator_stdout_path());
  EXPECT_NE(out.find("\"templates\""), std::string::npos);
  EXPECT_NE(out.find("portable.ruby-file-exists"), std::string::npos);
  EXPECT_NE(out.find("linux.gephi-0.10.smoke"), std::string::npos);
  EXPECT_NE(out.find("macos.gephi-0.10.smoke"), std::string::npos);
  EXPECT_NE(out.find("windows.gephi-0.10.smoke"), std::string::npos);
}


TEST(PlanarityTest, ExternalCertificateValidatorCanFilterGephiTemplatesByOs)
{
  if (not has_ruby_and_validator_script())
    {
      GTEST_SKIP() << "Skipped: Ruby or planarity_certificate_validator.rb not found";
      return;
    }

  const int rc = run_external_certificate_validator(
      {}, false, false, false, false, "",
      "--list-gephi-templates --template-os windows --json");
  EXPECT_EQ(rc, 0);

  const std::string out = read_text_file(validator_stdout_path());
  EXPECT_NE(out.find("windows.gephi-0.10.smoke"), std::string::npos);
  EXPECT_EQ(out.find("linux.gephi-0.10.smoke"), std::string::npos);
}


TEST(PlanarityTest, ExternalCertificateValidatorCanListGephiRenderProfiles)
{
  if (not has_ruby_and_validator_script())
    {
      GTEST_SKIP() << "Skipped: Ruby or planarity_certificate_validator.rb not found";
      return;
    }

  const int rc = run_external_certificate_validator(
      {}, false, false, false, false, "",
      "--list-gephi-render-profiles --json");
  EXPECT_EQ(rc, 0);

  const std::string out = read_text_file(validator_stdout_path());
  EXPECT_NE(out.find("\"profiles\""), std::string::npos);
  EXPECT_NE(out.find("portable.ruby-render-svg"), std::string::npos);
  EXPECT_NE(out.find("linux.gephi-0.10.render-svg"), std::string::npos);
  EXPECT_NE(out.find("macos.gephi-0.10.render-svg"), std::string::npos);
  EXPECT_NE(out.find("windows.gephi-0.10.render-svg"), std::string::npos);
}


TEST(PlanarityTest, ExternalCertificateValidatorCanFilterRenderProfilesByOs)
{
  if (not has_ruby_and_validator_script())
    {
      GTEST_SKIP() << "Skipped: Ruby or planarity_certificate_validator.rb not found";
      return;
    }

  const int rc = run_external_certificate_validator(
      {}, false, false, false, false, "",
      "--list-gephi-render-profiles --render-os windows --json");
  EXPECT_EQ(rc, 0);

  const std::string out = read_text_file(validator_stdout_path());
  EXPECT_NE(out.find("windows.gephi-0.10.render-svg"), std::string::npos);
  EXPECT_EQ(out.find("linux.gephi-0.10.render-svg"), std::string::npos);
}


TEST(PlanarityTest, ExternalCertificateValidatorGephiCatalogTemplateWorks)
{
  if (not has_ruby_and_validator_script())
    {
      GTEST_SKIP() << "Skipped: Ruby or planarity_certificate_validator.rb not found";
      return;
    }

  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  const std::string graphml_path = make_tmp_path("external_gephi_catalog", ".graphml");
  write_text_file(graphml_path, nonplanar_certificate_to_graphml(r));

  const int rc = run_external_certificate_validator(
      {graphml_path}, false, false, true, true, "",
      "--gephi-template portable.ruby-file-exists");
  EXPECT_EQ(rc, 0);
}


TEST(PlanarityTest, ExternalCertificateValidatorRenderSvgProfileProducesArtifact)
{
  namespace fs = std::filesystem;

  if (not has_ruby_and_validator_script())
    {
      GTEST_SKIP() << "Skipped: Ruby or planarity_certificate_validator.rb not found";
      return;
    }

  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  const std::string graphml_path = make_tmp_path("external_render_svg", ".graphml");
  const std::string output_dir = make_tmp_path("external_render_svg_dir", "");
  write_text_file(graphml_path, nonplanar_certificate_to_graphml(r));

  const int rc = run_external_certificate_validator(
      {graphml_path}, false, false, false, false, "",
      "--render-gephi --require-render "
      "--render-profile portable.ruby-render-svg "
      "--render-output-dir " + shell_quote(output_dir));
  EXPECT_EQ(rc, 0);

  const fs::path out_path = fs::path(output_dir) /
                            (fs::path(graphml_path).stem().string() + ".svg");
  EXPECT_TRUE(fs::exists(out_path));
  EXPECT_GT(fs::file_size(out_path), 0u);

  const std::string out = read_text_file(validator_stdout_path());
  EXPECT_NE(out.find("render_output_kind"), std::string::npos);
  EXPECT_NE(out.find("svg"), std::string::npos);
}


TEST(PlanarityTest, ExternalCertificateValidatorRenderPdfProfileProducesArtifact)
{
  namespace fs = std::filesystem;

  if (not has_ruby_and_validator_script())
    {
      GTEST_SKIP() << "Skipped: Ruby or planarity_certificate_validator.rb not found";
      return;
    }

  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  const std::string graphml_path = make_tmp_path("external_render_pdf", ".graphml");
  const std::string output_dir = make_tmp_path("external_render_pdf_dir", "");
  write_text_file(graphml_path, nonplanar_certificate_to_graphml(r));

  const int rc = run_external_certificate_validator(
      {graphml_path}, false, false, false, false, "",
      "--render-gephi --require-render "
      "--render-profile portable.ruby-render-pdf "
      "--render-output-dir " + shell_quote(output_dir));
  EXPECT_EQ(rc, 0);

  const fs::path out_path = fs::path(output_dir) /
                            (fs::path(graphml_path).stem().string() + ".pdf");
  EXPECT_TRUE(fs::exists(out_path));
  EXPECT_GT(fs::file_size(out_path), 0u);
}


TEST(PlanarityTest, ExternalCertificateValidatorRenderSupportsInputPathWithSpaces)
{
  namespace fs = std::filesystem;

  if (not has_ruby_and_validator_script())
    {
      GTEST_SKIP() << "Skipped: Ruby or planarity_certificate_validator.rb not found";
      return;
    }

  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  const fs::path dir = fs::temp_directory_path() / "aleph planarity render spaces";
  fs::create_directories(dir);
  const std::string graphml_path = (dir / "k33 certificate.graphml").string();
  const std::string output_dir = (dir / "render output").string();
  write_text_file(graphml_path, nonplanar_certificate_to_graphml(r));

  const int rc = run_external_certificate_validator(
      {graphml_path}, false, false, false, false, "",
      "--render-gephi --require-render "
      "--render-profile portable.ruby-render-svg "
      "--render-output-dir " + shell_quote(output_dir));
  EXPECT_EQ(rc, 0);

  const fs::path out_path = fs::path(output_dir) /
                            (fs::path(graphml_path).stem().string() + ".svg");
  EXPECT_TRUE(fs::exists(out_path));
  EXPECT_GT(fs::file_size(out_path), 0u);
}


TEST(PlanarityTest, ExternalCertificateValidatorRenderFailsOnUnknownProfile)
{
  if (not has_ruby_and_validator_script())
    {
      GTEST_SKIP() << "Skipped: Ruby or planarity_certificate_validator.rb not found";
      return;
    }

  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  const std::string graphml_path = make_tmp_path("external_render_bad", ".graphml");
  write_text_file(graphml_path, nonplanar_certificate_to_graphml(r));

  const int rc = run_external_certificate_validator(
      {graphml_path}, false, false, false, false, "",
      "--render-gephi --require-render --render-profile missing.profile");
  EXPECT_NE(rc, 0);
}


TEST(PlanarityTest, ExternalCertificateBatchRunnerProducesReport)
{
  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  const std::string graphml_path = make_tmp_path("external_batch", ".graphml");
  const std::string gexf_path = make_tmp_path("external_batch", ".gexf");
  const std::string report_path = make_tmp_path("external_batch_report", ".json");

  write_text_file(graphml_path, nonplanar_certificate_to_graphml(r));
  write_text_file(gexf_path, nonplanar_certificate_to_gexf(r));

  const int rc = run_external_certificate_batch(
      {graphml_path, gexf_path}, report_path,
      "--gephi --require-gephi --gephi-template portable.ruby-file-exists --print-summary");
  EXPECT_EQ(rc, 0);

  const std::string report = read_text_file(report_path);
  EXPECT_NE(report.find("\"num_inputs\": 2"), std::string::npos);
  EXPECT_NE(report.find("\"overall_valid\": true"), std::string::npos);
  EXPECT_NE(report.find("\"gephi_template\": \"portable.ruby-file-exists\""),
            std::string::npos);
}


TEST(PlanarityTest, ExternalCertificateBatchRunnerSupportsRenderProfiles)
{
  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  const std::string graphml_path = make_tmp_path("external_batch_render", ".graphml");
  const std::string report_path = make_tmp_path("external_batch_render_report", ".json");
  const std::string output_dir = make_tmp_path("external_batch_render_dir", "");
  write_text_file(graphml_path, nonplanar_certificate_to_graphml(r));

  const int rc = run_external_certificate_batch(
      {graphml_path}, report_path,
      "--render-gephi --require-render "
      "--render-profile portable.ruby-render-svg "
      "--render-output-dir " + shell_quote(output_dir));
  EXPECT_EQ(rc, 0);

  const std::string report = read_text_file(report_path);
  EXPECT_NE(report.find("\"render_profile\": \"portable.ruby-render-svg\""),
            std::string::npos);
  EXPECT_NE(report.find("\"overall_valid\": true"), std::string::npos);
}


TEST(PlanarityTest, ExternalCertificateVisualDiffRunnerPassesWithGoldenManifest)
{
  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  const std::string graphml_path = make_tmp_path("external_visual", ".graphml");
  const std::string report_path = make_tmp_path("external_visual_report", ".json");
  const std::string output_dir = make_tmp_path("external_visual_render_dir", "");
  write_text_file(graphml_path, nonplanar_certificate_to_graphml(r));

  const int rc = run_external_certificate_visual_diff(
      {graphml_path},
      {"portable.ruby-render-svg", "portable.ruby-render-pdf"},
      report_path,
      "--render-output-dir " + shell_quote(output_dir) + " --print-summary");
  EXPECT_EQ(rc, 0);

  const std::string report = read_text_file(report_path);
  EXPECT_NE(report.find("\"overall_passed\": true"), std::string::npos);
  EXPECT_NE(report.find("\"profile_id\": \"portable.ruby-render-svg\""),
            std::string::npos);
  EXPECT_NE(report.find("\"profile_id\": \"portable.ruby-render-pdf\""),
            std::string::npos);
}


TEST(PlanarityTest, ExternalCertificateVisualDiffRunnerDetectsGoldenMismatch)
{
  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  const std::string graphml_path = make_tmp_path("external_visual_bad", ".graphml");
  const std::string report_path = make_tmp_path("external_visual_bad_report", ".json");
  const std::string output_dir = make_tmp_path("external_visual_bad_render_dir", "");
  const std::string manifest_path = make_tmp_path("external_visual_bad_manifest", ".json");
  write_text_file(graphml_path, nonplanar_certificate_to_graphml(r));
  write_text_file(
      manifest_path,
      R"({
  "schema_version": 1,
  "entries": [
    {
      "profile_id": "portable.ruby-render-svg",
      "output_ext": "svg",
      "sha256": "0000000000000000000000000000000000000000000000000000000000000000",
      "bytes": 1
    }
  ]
}
)");

  const int rc = run_external_certificate_visual_diff(
      {graphml_path},
      {"portable.ruby-render-svg"},
      report_path,
      "--render-output-dir " + shell_quote(output_dir)
      + " --golden-manifest " + shell_quote(manifest_path));
  EXPECT_NE(rc, 0);

  const std::string report = read_text_file(report_path);
  EXPECT_NE(report.find("\"overall_passed\": false"), std::string::npos);
  EXPECT_NE(report.find("Golden hash mismatch"), std::string::npos);
}


TEST(PlanarityTest, ExternalCertificateVisualDiffRunnerCanUpdateGoldenManifest)
{
  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  Planarity_Test_Options opts;
  opts.compute_nonplanar_certificate = true;
  const auto r = planarity_test(built.g, opts);
  ASSERT_FALSE(r.is_planar);
  ASSERT_TRUE(r.has_nonplanar_certificate);

  const std::string graphml_path = make_tmp_path("external_visual_upd", ".graphml");
  const std::string report_path = make_tmp_path("external_visual_upd_report", ".json");
  const std::string output_dir = make_tmp_path("external_visual_upd_render_dir", "");
  const std::string manifest_path = make_tmp_path("external_visual_upd_manifest", ".json");
  write_text_file(graphml_path, nonplanar_certificate_to_graphml(r));

  const int rc = run_external_certificate_visual_diff(
      {graphml_path},
      {"portable.ruby-render-svg"},
      report_path,
      "--render-output-dir " + shell_quote(output_dir)
      + " --golden-manifest " + shell_quote(manifest_path)
      + " --update-golden");
  EXPECT_EQ(rc, 0);

  const std::string report = read_text_file(report_path);
  const std::string manifest = read_text_file(manifest_path);

  if (manifest.find("portable.ruby-render-svg") == std::string::npos)
    {
      std::cerr << "[diag] manifest_path=" << manifest_path << "\n"
                << "[diag] manifest content (" << manifest.size() << " bytes):\n"
                << manifest << "\n"
                << "[diag] report content (" << report.size() << " bytes):\n"
                << report << "\n";
    }

  EXPECT_NE(manifest.find("\"profile_id\": \"portable.ruby-render-svg\""),
            std::string::npos);
  EXPECT_NE(manifest.find("0459f595d6268e7bdf9e8e273d4394fa50d23a89ec3d2449d10b7b47941b1327"),
            std::string::npos);
}


TEST(PlanarityTest, ExternalGephiNightlyComparisonReportHasNoRegressions)
{
  namespace fs = std::filesystem;

  const fs::path artifacts_root =
      fs::path(make_tmp_path("gephi_nightly_artifacts_ok", ""));
  fs::create_directories(artifacts_root);

  const auto write_case = [&](const std::string & dir_name,
                              const std::string & os_name,
                              const std::string & tag,
                              const int validator_exit_code,
                              const bool overall_valid)
  {
    const fs::path dir = artifacts_root / dir_name;
    fs::create_directories(dir);

    write_text_file((dir / "downloaded_gephi_asset.txt").string(),
                    "tag=" + tag + "\n"
                    "runner_os=" + os_name + "\n"
                    "asset_name=gephi-package-" + tag + ".zip\n"
                    "gephi_executable=/tmp/gephi\n");

    std::ostringstream summary;
    summary << "{\n"
            << "  \"os\": \"" << os_name << "\",\n"
            << "  \"gephi_tag\": \"" << tag << "\",\n"
            << "  \"asset_name\": \"gephi-package-" << tag << ".zip\",\n"
            << "  \"validator_exit_code\": " << validator_exit_code << ",\n"
            << "  \"overall_valid\": " << (overall_valid ? "true" : "false") << "\n"
            << "}\n";
    write_text_file((dir / "nightly_case_summary.json").string(), summary.str());
  };

  write_case("gephi-nightly-ubuntu-24.04-v0.9.7", "ubuntu-24.04", "v0.9.7", 0, true);
  write_case("gephi-nightly-ubuntu-24.04-v0.10.1", "ubuntu-24.04", "v0.10.1", 0, true);
  write_case("gephi-nightly-windows-2022-v0.9.7", "windows-2022", "v0.9.7", 0, true);
  write_case("gephi-nightly-windows-2022-v0.10.1", "windows-2022", "v0.10.1", 0, true);

  const std::string report_json = make_tmp_path("gephi_nightly_report_ok", ".json");
  const std::string report_md = make_tmp_path("gephi_nightly_report_ok", ".md");

  const int rc = run_gephi_nightly_comparison(
      artifacts_root.string(), report_json, report_md, "--print-summary");
  EXPECT_EQ(rc, 0);

  const std::string json = read_text_file(report_json);
  EXPECT_NE(json.find("\"num_entries\": 4"), std::string::npos);
  EXPECT_NE(json.find("\"num_regressions\": 0"), std::string::npos);
  EXPECT_NE(json.find("\"has_regressions\": false"), std::string::npos);

  const std::string markdown = read_text_file(report_md);
  EXPECT_NE(markdown.find("### Regression Alerts"), std::string::npos);
  EXPECT_NE(markdown.find("- none"), std::string::npos);
}


TEST(PlanarityTest, ExternalGephiNightlyComparisonDetectsRegressionAndCanFail)
{
  namespace fs = std::filesystem;

  const fs::path artifacts_root =
      fs::path(make_tmp_path("gephi_nightly_artifacts_bad", ""));
  fs::create_directories(artifacts_root);

  const auto write_case = [&](const std::string & dir_name,
                              const std::string & os_name,
                              const std::string & tag,
                              const int validator_exit_code,
                              const bool overall_valid)
  {
    const fs::path dir = artifacts_root / dir_name;
    fs::create_directories(dir);

    write_text_file((dir / "downloaded_gephi_asset.txt").string(),
                    "tag=" + tag + "\n"
                    "runner_os=" + os_name + "\n"
                    "asset_name=gephi-package-" + tag + ".zip\n"
                    "gephi_executable=/tmp/gephi\n");

    std::ostringstream summary;
    summary << "{\n"
            << "  \"os\": \"" << os_name << "\",\n"
            << "  \"gephi_tag\": \"" << tag << "\",\n"
            << "  \"asset_name\": \"gephi-package-" << tag << ".zip\",\n"
            << "  \"validator_exit_code\": " << validator_exit_code << ",\n"
            << "  \"overall_valid\": " << (overall_valid ? "true" : "false") << "\n"
            << "}\n";
    write_text_file((dir / "nightly_case_summary.json").string(), summary.str());
  };

  write_case("gephi-nightly-ubuntu-24.04-v0.9.7", "ubuntu-24.04", "v0.9.7", 0, true);
  write_case("gephi-nightly-ubuntu-24.04-v0.10.1", "ubuntu-24.04", "v0.10.1", 2, false);

  const std::string report_json = make_tmp_path("gephi_nightly_report_bad", ".json");
  const std::string report_md = make_tmp_path("gephi_nightly_report_bad", ".md");

  const int rc = run_gephi_nightly_comparison(
      artifacts_root.string(), report_json, report_md, "--print-summary");
  EXPECT_EQ(rc, 0);

  const std::string json = read_text_file(report_json);
  EXPECT_NE(json.find("\"num_regressions\": 1"), std::string::npos);
  EXPECT_NE(json.find("\"has_regressions\": true"), std::string::npos);
  EXPECT_NE(json.find("overall_valid_regressed"), std::string::npos);
  EXPECT_NE(json.find("exit_code_regressed"), std::string::npos);

  const int rc_fail = run_gephi_nightly_comparison(
      artifacts_root.string(), report_json, report_md, "--fail-on-regressions");
  EXPECT_NE(rc_fail, 0);
}


TEST(PlanarityTest, ExternalGephiRegressionNotifierNoRegressionProducesSummary)
{
  const std::string report_json = make_tmp_path("gephi_notify_ok_report", ".json");
  const std::string output_md = make_tmp_path("gephi_notify_ok_alert", ".md");

  write_text_file(
      report_json,
      R"({
  "schema_version": 1,
  "run_id": "r1",
  "run_attempt": "1",
  "git_sha": "abc123",
  "resolved_tags": "v0.9.7,v0.10.1",
  "num_entries": 4,
  "entries": [],
  "num_regressions": 0,
  "has_regressions": false,
  "regressions": []
}
)");

  const int rc = run_gephi_regression_notify(report_json, output_md, "--print-summary");
  EXPECT_EQ(rc, 0);

  const std::string markdown = read_text_file(output_md);
  EXPECT_NE(markdown.find("Gephi Nightly Regression Alert"), std::string::npos);
  EXPECT_NE(markdown.find("No regressions detected."), std::string::npos);
}


TEST(PlanarityTest, ExternalGephiRegressionNotifierRegressionAndRequiredWebhookFailsWithoutWebhook)
{
  const std::string report_json = make_tmp_path("gephi_notify_bad_report", ".json");
  const std::string output_md = make_tmp_path("gephi_notify_bad_alert", ".md");

  write_text_file(
      report_json,
      R"({
  "schema_version": 1,
  "run_id": "r2",
  "run_attempt": "1",
  "git_sha": "def456",
  "resolved_tags": "v0.9.7,v0.10.1",
  "num_entries": 2,
  "entries": [],
  "num_regressions": 1,
  "has_regressions": true,
  "regressions": [
    {
      "os": "ubuntu-24.04",
      "from_tag": "v0.9.7",
      "to_tag": "v0.10.1",
      "from_overall_valid": true,
      "to_overall_valid": false,
      "from_exit_code": 0,
      "to_exit_code": 2,
      "reason_codes": ["overall_valid_regressed", "exit_code_regressed"]
    }
  ]
}
)");

  const int rc_optional = run_gephi_regression_notify(
      report_json, output_md, "--print-summary");
  EXPECT_EQ(rc_optional, 0);

  const std::string markdown = read_text_file(output_md);
  EXPECT_NE(markdown.find("Detected regression(s):"), std::string::npos);
  EXPECT_NE(markdown.find("overall_valid_regressed"), std::string::npos);
  EXPECT_NE(markdown.find("exit_code_regressed"), std::string::npos);
  EXPECT_NE(markdown.find("https://example.invalid/run/123"), std::string::npos);

  const int rc_required = run_gephi_regression_notify(
      report_json, output_md, "--require-webhook-success");
  EXPECT_NE(rc_required, 0);
}


TEST(PlanarityTest, ExternalCertificateBatchRunnerFailsOnInvalidInput)
{
  const std::string broken_graphml = make_tmp_path("external_batch_invalid", ".graphml");
  const std::string report_path = make_tmp_path("external_batch_invalid_report", ".json");

  write_text_file(broken_graphml, R"(<?xml version="1.0" encoding="UTF-8"?>
<graphml xmlns="http://graphml.graphdrawing.org/xmlns">
  <graph id="Broken" edgedefault="undirected">
    <node id="n0"/>
    <node id="n1"/>
    <edge id="e0" source="n0" target="n1">
      <data key="k_edge_kind">path</data>
    </edge>
  </graph>
</graphml>
)");

  const int rc = run_external_certificate_batch({broken_graphml}, report_path);
  EXPECT_NE(rc, 0);

  const std::string report = read_text_file(report_path);
  EXPECT_NE(report.find("\"num_inputs\": 1"), std::string::npos);
  EXPECT_NE(report.find("\"overall_valid\": false"), std::string::npos);
}


TEST(PlanarityTest, ExternalCertificateFixtureGraphmlIsValid)
{
  namespace fs = std::filesystem;

  if (not has_ruby_and_validator_script())
    {
      GTEST_SKIP() << "Skipped: Ruby or planarity_certificate_validator.rb not found";
      return;
    }

  const auto root = find_repo_root();
  ASSERT_TRUE(root.has_value());

  const fs::path fixture =
      *root / "scripts" / "fixtures" / "planarity_k33_certificate.graphml";
  ASSERT_TRUE(fs::exists(fixture));

  const int rc = run_external_certificate_validator({fixture.string()});
  EXPECT_EQ(rc, 0);
}


TEST(PlanarityTest, NonPlanarCertificateExportRequiresCertificate)
{
  auto built = build_ugraph(6, complete_bipartite_edges(3, 3));

  const auto r = planarity_test(built.g); // no certificate requested
  ASSERT_FALSE(r.is_planar);
  ASSERT_FALSE(r.has_nonplanar_certificate);

  EXPECT_ANY_THROW((void) nonplanar_certificate_to_json(r));
  EXPECT_ANY_THROW((void) nonplanar_certificate_to_dot(r));
  EXPECT_ANY_THROW((void) nonplanar_certificate_to_graphml(r));
  EXPECT_ANY_THROW((void) nonplanar_certificate_to_gexf(r));

  const auto vr = validate_nonplanar_certificate(r);
  EXPECT_FALSE(vr.has_certificate);
  EXPECT_FALSE(vr.is_valid);
  EXPECT_FALSE(nonplanar_certificate_is_valid(r));
}
