#include <gtest/gtest.h>
#include <sstream>
#include <string>

#include "load_digraph.H"

using namespace Aleph;

namespace
{
std::string make_nodes_csv()
{
  return
    "id|plazo|class|f3|nes|power|label\n"  // header (ignored)
    "A|cp|it|x|12|5|Alpha\n"
    "B|mp|ef|x|34|7|Beta\n"
    "C|lp|pp|x|56|9|Gamma\n";
}

std::string make_arcs_csv()
{
  return
    "A B\n"
    "B C\n"
    "C A\n"; // completes a cycle
}
} // namespace

struct FlagReset
{
  bool wp = with_power;
  bool wn = with_nes;
  bool on = only_num;
  bool wc = with_class;
  size_t fs = font_size;

  FlagReset()
  {
    with_power = false;
    with_nes = false;
    only_num = false;
    with_class = false;
    font_size = 6;
  }

  ~FlagReset()
  {
    with_power = wp;
    with_nes = wn;
    only_num = on;
    with_class = wc;
    font_size = fs;
  }
};

TEST(LoadDigraph, LoadsNodesAndArcsAndSkipsShortRows)
{
  FlagReset _;
  std::stringstream nodes(make_nodes_csv());
  nodes << "Z|cp|it\n"; // malformed, should be skipped
  std::stringstream arcs(make_arcs_csv());

  Digrafo g;
  load_digraph(g, nodes, arcs);

  EXPECT_EQ(g.get_num_nodes(), 3U);
  EXPECT_EQ(g.get_num_arcs(), 3U);

  auto *a = search_node(g, "A");
  ASSERT_NE(a, nullptr);
  EXPECT_EQ(a->get_info().first, "A");
  EXPECT_EQ(a->get_info().second[6], "Alpha");
}

TEST(LoadDigraph, CreatesMissingNodesWhenParsingArcs)
{
  FlagReset _;
  std::stringstream nodes;  // empty -> no nodes added
  std::stringstream arcs;
  arcs << "X Y\n";

  Digrafo g;
  load_digraph(g, nodes, arcs);

  EXPECT_EQ(g.get_num_nodes(), 2U);
  EXPECT_EQ(g.get_num_arcs(), 1U);

  // Nodes created from arcs have empty fields
  auto *x = search_node(g, "X");
  ASSERT_NE(x, nullptr);
  EXPECT_TRUE(x->get_info().second.is_empty());
}

TEST(LoadDigraph, EmptyInputsProduceEmptyGraph)
{
  FlagReset _;
  std::stringstream nodes;
  std::stringstream arcs;

  Digrafo g;
  load_digraph(g, nodes, arcs);

  EXPECT_EQ(g.get_num_nodes(), 0U);
  EXPECT_EQ(g.get_num_arcs(), 0U);
}

TEST(LoadDigraph, LinesWithExtraSeparatorsAreIgnored)
{
  FlagReset _;
  std::stringstream nodes;
  nodes << "id|plazo|class|f3|nes|power|label\n"; // header
  nodes << "D|||\n"; // too few fields
  nodes << "E|cp|it|x|44|11|Echo\n"; // valid

  std::stringstream arcs;
  arcs << "D E\n"; // arc creates D with empty fields

  Digrafo g;
  load_digraph(g, nodes, arcs);

  EXPECT_EQ(g.get_num_nodes(), 2U);
  EXPECT_EQ(g.get_num_arcs(), 1U);

  auto *d = search_node(g, "D");
  ASSERT_NE(d, nullptr);
  EXPECT_TRUE(d->get_info().second.is_empty()); // created from arc, ignored bad row

  auto *e = search_node(g, "E");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->get_info().second.size(), 7U); // valid row stored
}

TEST(GenerateDotFile, ProducesWarningWhenCycleExists)
{
  FlagReset _;
  std::stringstream nodes(make_nodes_csv());
  std::stringstream arcs(make_arcs_csv()); // cycle
  Digrafo g;
  load_digraph(g, nodes, arcs);

  // Configure flags
  with_power = true;
  with_nes = true;
  only_num = false;
  with_class = true;
  font_size = 8;

  std::stringstream out;
  generate_dot_file(g, out);
  const auto dot = out.str();

  EXPECT_NE(dot.find("WARNING: Cycle detected"), std::string::npos);
  EXPECT_NE(dot.find("A [color=Green"), std::string::npos);
  EXPECT_NE(dot.find("\\nP=5"), std::string::npos);
  EXPECT_NE(dot.find("\\n12%"), std::string::npos);
  EXPECT_NE(dot.find("shape = box"), std::string::npos);
}

TEST(GenerateDotFile, EmitsTopologicalRanksWhenAcyclic)
{
  FlagReset _;
  std::stringstream nodes(make_nodes_csv());
  std::stringstream arcs;
  arcs << "A B\n"
       << "A C\n"; // DAG
  Digrafo g;
  load_digraph(g, nodes, arcs);

  with_power = false;
  with_nes = false;
  only_num = true;
  with_class = false;
  font_size = 6;

  std::stringstream out;
  generate_dot_file(g, out);
  const auto dot = out.str();

  EXPECT_EQ(dot.find("WARNING: Cycle detected"), std::string::npos);
  EXPECT_NE(dot.find("{ rank = same;"), std::string::npos);
  EXPECT_NE(dot.find("A -> B"), std::string::npos);
  EXPECT_NE(dot.find("A -> C"), std::string::npos);
}

TEST(GenerateDotFile, RespectsOnlyNumFlagWhenTrue)
{
  FlagReset _;
  std::stringstream nodes(make_nodes_csv());
  std::stringstream arcs;
  arcs << "A B\n"; // simple DAG
  Digrafo g;
  load_digraph(g, nodes, arcs);

  only_num = true;
  with_power = false;
  with_nes = false;
  with_class = false;
  font_size = 10;

  std::stringstream out;
  generate_dot_file(g, out);
  const auto dot = out.str();

  // Should not include labels or power/nes when only_num is true
  EXPECT_EQ(dot.find("Alpha"), std::string::npos);
  EXPECT_EQ(dot.find("P="), std::string::npos);
  EXPECT_EQ(dot.find("%"), std::string::npos);
  EXPECT_NE(dot.find("fontsize = 10"), std::string::npos);
}
