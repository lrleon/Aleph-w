/* Aleph-w

   / \  | | ___ _ __ | |__      __      __
  / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
 / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
/_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
              |_|

This file is part of Aleph-w library

Copyright (c) 2002-2018 Leandro Rabindranath Leon

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
 * @file load_digraph_test.cc
 * @brief Tests for Load Digraph
 */

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

// ==================== Additional Tests ====================

TEST(Split, BasicSplitWithSingleSeparator)
{
  DynDlist<std::string> words;
  split("a|b|c", "|", words);
  ASSERT_EQ(words.size(), 3U);
  EXPECT_EQ(words[0], "a");
  EXPECT_EQ(words[1], "b");
  EXPECT_EQ(words[2], "c");
}

TEST(Split, SplitWithMultipleSeparators)
{
  DynDlist<std::string> words;
  split("a,b c d", " ,", words);
  ASSERT_EQ(words.size(), 4U);
  EXPECT_EQ(words[0], "a");
  EXPECT_EQ(words[1], "b");
  EXPECT_EQ(words[2], "c");
  EXPECT_EQ(words[3], "d");
}

TEST(Split, EmptyString)
{
  DynDlist<std::string> words;
  split("", "|", words);
  EXPECT_EQ(words.size(), 0U);
}

TEST(Split, OnlySeparators)
{
  DynDlist<std::string> words;
  split("|||", "|", words);
  EXPECT_EQ(words.size(), 0U);
}

TEST(Split, NoSeparatorsFound)
{
  DynDlist<std::string> words;
  split("hello", "|", words);
  ASSERT_EQ(words.size(), 1U);
  EXPECT_EQ(words[0], "hello");
}

TEST(Split, LeadingAndTrailingSeparators)
{
  DynDlist<std::string> words;
  split("|a|b|", "|", words);
  ASSERT_EQ(words.size(), 2U);
  EXPECT_EQ(words[0], "a");
  EXPECT_EQ(words[1], "b");
}

TEST(EqualNode, ComparesById)
{
  Equal_Node eq;
  DynDlist<std::string> fields1, fields2;
  fields1.append("x");
  fields2.append("y");

  Info_Nodo n1("A", fields1);
  Info_Nodo n2("A", fields2);
  Info_Nodo n3("B", fields1);

  EXPECT_TRUE(eq(n1, n2));  // Same ID, different fields
  EXPECT_FALSE(eq(n1, n3)); // Different IDs
}

TEST(LoadDigraph, ArcsWithCommaSeparator)
{
  FlagReset _;
  std::stringstream nodes;
  std::stringstream arcs;
  arcs << "X,Y\n"   // comma-separated
       << "Y Z\n";  // space-separated

  Digrafo g;
  load_digraph(g, nodes, arcs);

  EXPECT_EQ(g.get_num_nodes(), 3U);
  EXPECT_EQ(g.get_num_arcs(), 2U);
}

TEST(LoadDigraph, DuplicateNodeIdsInArcs)
{
  FlagReset _;
  std::stringstream nodes(make_nodes_csv());
  std::stringstream arcs;
  arcs << "A A\n";  // Self-loop

  Digrafo g;
  load_digraph(g, nodes, arcs);

  EXPECT_EQ(g.get_num_nodes(), 3U);
  EXPECT_EQ(g.get_num_arcs(), 1U);

  // Verify self-loop
  bool found_self_loop = false;
  for (Arc_Iterator<Digrafo> it(g); it.has_curr(); it.next_ne())
    {
      auto *a = it.get_curr();
      if (g.get_src_node(a) == g.get_tgt_node(a))
        found_self_loop = true;
    }
  EXPECT_TRUE(found_self_loop);
}

TEST(LoadDigraph, MultipleArcsWithSameEndpoints)
{
  FlagReset _;
  std::stringstream nodes;
  std::stringstream arcs;
  arcs << "A B\n"
       << "A B\n";  // Duplicate arc

  Digrafo g;
  load_digraph(g, nodes, arcs);

  EXPECT_EQ(g.get_num_nodes(), 2U);
  EXPECT_EQ(g.get_num_arcs(), 2U);  // Both arcs are inserted
}

TEST(GenerateDotFile, AllColorsAndShapes)
{
  FlagReset _;
  std::stringstream nodes;
  nodes << "id|plazo|class|f3|nes|power|label\n"
        << "A|cp|it|x|10|1|Node1\n"   // Green, box
        << "B|mp|ef|x|20|2|Node2\n"   // Yellow, ellipse
        << "C|lp|pp|x|30|3|Node3\n"   // Red, hexagon
        << "D|unknown|unknown|x|40|4|Node4\n";  // Unknown color/shape

  std::stringstream arcs;
  arcs << "A B\n"
       << "B C\n"
       << "C D\n";

  Digrafo g;
  load_digraph(g, nodes, arcs);

  with_power = true;
  with_nes = true;
  with_class = true;
  only_num = false;
  font_size = 12;

  std::stringstream out;
  generate_dot_file(g, out);
  const auto dot = out.str();

  // Check all colors
  EXPECT_NE(dot.find("A [color=Green"), std::string::npos);
  EXPECT_NE(dot.find("B [color=Yellow"), std::string::npos);
  EXPECT_NE(dot.find("C [color=Red"), std::string::npos);

  // Check all shapes
  EXPECT_NE(dot.find("shape = box"), std::string::npos);
  EXPECT_NE(dot.find("shape = ellipse"), std::string::npos);
  EXPECT_NE(dot.find("shape = hexagon"), std::string::npos);

  // Check power and NES
  EXPECT_NE(dot.find("P=1"), std::string::npos);
  EXPECT_NE(dot.find("P=2"), std::string::npos);
  EXPECT_NE(dot.find("10%"), std::string::npos);
  EXPECT_NE(dot.find("20%"), std::string::npos);
}

TEST(GenerateDotFile, SingleNodeGraph)
{
  FlagReset _;
  std::stringstream nodes;
  nodes << "id|plazo|class|f3|nes|power|label\n"
        << "X|cp|it|x|50|5|Single\n";
  std::stringstream arcs;

  Digrafo g;
  load_digraph(g, nodes, arcs);

  only_num = false;

  std::stringstream out;
  generate_dot_file(g, out);
  const auto dot = out.str();

  EXPECT_NE(dot.find("X [color=Green"), std::string::npos);
  EXPECT_NE(dot.find("Single"), std::string::npos);
  EXPECT_EQ(dot.find("->"), std::string::npos);  // No arcs
}

TEST(GenerateDotFile, DisconnectedComponents)
{
  FlagReset _;
  std::stringstream nodes;
  nodes << "id|plazo|class|f3|nes|power|label\n"
        << "A|cp|it|x|10|1|A\n"
        << "B|mp|ef|x|20|2|B\n"
        << "C|lp|pp|x|30|3|C\n"
        << "D|cp|it|x|40|4|D\n";
  std::stringstream arcs;
  arcs << "A B\n"
       << "C D\n";  // Two disconnected components

  Digrafo g;
  load_digraph(g, nodes, arcs);

  only_num = false;

  std::stringstream out;
  generate_dot_file(g, out);
  const auto dot = out.str();

  EXPECT_EQ(dot.find("WARNING: Cycle"), std::string::npos);
  EXPECT_NE(dot.find("A -> B"), std::string::npos);
  EXPECT_NE(dot.find("C -> D"), std::string::npos);
}

TEST(SearchNode, FindsExistingNode)
{
  Digrafo g;
  g.insert_node(Info_Nodo("existing", DynDlist<std::string>()));

  auto *found = search_node(g, "existing");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->get_info().first, "existing");

  // Verify no new nodes were created
  EXPECT_EQ(g.get_num_nodes(), 1U);
}

TEST(SearchNode, CreatesNewNodeIfNotFound)
{
  Digrafo g;

  auto *created = search_node(g, "new_node");
  ASSERT_NE(created, nullptr);
  EXPECT_EQ(created->get_info().first, "new_node");
  EXPECT_TRUE(created->get_info().second.is_empty());
  EXPECT_EQ(g.get_num_nodes(), 1U);
}

TEST(MinNodeFields, HasCorrectValue)
{
  EXPECT_EQ(MIN_NODE_FIELDS, 7U);
}

TEST(LoadDigraph, LargeGraph)
{
  FlagReset _;
  std::stringstream nodes;
  nodes << "id|plazo|class|f3|nes|power|label\n";

  constexpr size_t NUM_NODES = 100;
  for (size_t i = 0; i < NUM_NODES; ++i)
    {
      nodes << "N" << i << "|cp|it|x|10|5|Node" << i << "\n";
    }

  std::stringstream arcs;
  for (size_t i = 0; i < NUM_NODES - 1; ++i)
    {
      arcs << "N" << i << " N" << (i + 1) << "\n";
    }

  Digrafo g;
  load_digraph(g, nodes, arcs);

  EXPECT_EQ(g.get_num_nodes(), NUM_NODES);
  EXPECT_EQ(g.get_num_arcs(), NUM_NODES - 1);

  // Verify it's a chain (DAG)
  std::stringstream out;
  generate_dot_file(g, out);
  const auto dot = out.str();
  EXPECT_EQ(dot.find("WARNING: Cycle"), std::string::npos);
}
