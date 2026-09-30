
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

/** @file xml_graph_test.cc
 *  @brief Tests for xml_graph.H (built only when libxml++ 2.6 is available).
 */

# include <gtest/gtest.h>

# include <cstdio>
# include <filesystem>
# include <random>
# include <string>

# include <ahSort.H>
# include <xml_graph.H>

using namespace Aleph;

namespace
{
  using G = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

  struct NotAGraph
  {
    using Node = int;
  };

  template <class T>
  concept can_xml_graph = requires { typename Xml_Graph<T>; };

  static_assert(can_xml_graph<G>);
  static_assert(not can_xml_graph<NotAGraph> and not can_xml_graph<DynList<int>>);

  // Stateful writer: counts its calls so the tests can see whose copy ran.
  struct NodeWriter
  {
    int calls = 0;

    void operator()(G &, G::Node * p, DynArray<Attr> & attrs)
    {
      ++calls;
      attrs.append(Attr{"info", std::to_string(p->get_info())});
    }
  };

  struct NodeReader
  {
    void operator()(G &, G::Node * p, DynArray<Attr> & attrs)
    {
      for (size_t i = 0; i < attrs.size(); ++i)
        if (attrs(i).name == "info")
          p->get_info() = std::stoi(attrs(i).value);
    }
  };

  struct ArcWriter
  {
    void operator()(G &, G::Arc * a, DynArray<Attr> & attrs)
    {
      attrs.append(Attr{"w", std::to_string(a->get_info())});
    }
  };

  struct ArcReader
  {
    void operator()(G &, G::Arc * a, DynArray<Attr> & attrs)
    {
      for (size_t i = 0; i < attrs.size(); ++i)
        if (attrs(i).name == "w")
          a->get_info() = std::stoi(attrs(i).value);
    }
  };

  using XG = Xml_Graph<G, NodeReader, ArcReader, NodeWriter, ArcWriter>;

  // Order-independent description of a graph: node infos and
  // "src info -> tgt info : weight" for every arc.
  DynList<std::string> describe(G & g)
  {
    DynList<std::string> items;
    for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
      items.append("n" + std::to_string(it.get_curr()->get_info()));
    for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
      {
        auto * a = it.get_curr();
        items.append(std::to_string(g.get_src_node(a)->get_info()) + "->" +
                     std::to_string(g.get_tgt_node(a)->get_info()) + ":" +
                     std::to_string(a->get_info()));
      }
    return sort(items);
  }

  G sample_graph()
  {
    G g;
    G::Node * n[5];
    for (int i = 0; i < 5; ++i)
      n[i] = g.insert_node(10 * (i + 1));
    g.insert_arc(n[0], n[1], 1);
    g.insert_arc(n[1], n[2], 2);
    g.insert_arc(n[2], n[3], 3);
    g.insert_arc(n[3], n[4], 4);
    g.insert_arc(n[4], n[0], 5);
    g.insert_arc(n[0], n[2], 6);
    return g;
  }

  // ctest runs each case in its own process, possibly in parallel: every
  // case writes to files named after itself plus a random token.
  class XmlGraphTest : public ::testing::Test
  {
    std::string prefix;

  protected:
    void SetUp() override
    {
      prefix = std::string("aleph_xml_graph_test_") +
               ::testing::UnitTest::GetInstance()->current_test_info()->name() +
               "_" + std::to_string(std::random_device{}()) + "_";
    }

    std::string file(const std::string & name) const
    {
      return (std::filesystem::temp_directory_path() / (prefix + name)).string();
    }

    void TearDown() override
    {
      for (const char * name : {"owned.xml", "shared.xml", "copy.xml", "names.xml"})
        std::remove(file(name).c_str());
    }
  };
} // namespace

// The default constructor used to keep references to temporary functors,
// which dangled as soon as it returned.
TEST_F(XmlGraphTest, RoundTripWithOwnedFunctors)
{
  G g = sample_graph();
  XG xml;
  xml(g, file("owned.xml"));
  G h = xml(file("owned.xml"));

  EXPECT_EQ(h.get_num_nodes(), g.get_num_nodes());
  EXPECT_EQ(h.get_num_arcs(), g.get_num_arcs());
  EXPECT_EQ(describe(h), describe(g));
}

// Functors passed by lvalue are shared: their state is visible to the caller,
// also through a copy of the reader/writer.
TEST_F(XmlGraphTest, SharedFunctorsStaySharedAcrossCopies)
{
  G g = sample_graph();
  NodeReader nr;
  ArcReader ar;
  NodeWriter nw;
  ArcWriter aw;
  XG shared(nr, ar, nw, aw);
  shared(g, file("shared.xml"));
  EXPECT_EQ(nw.calls, 5);

  XG copy = shared;
  G h = copy(file("shared.xml"));
  copy(h, file("copy.xml"));
  EXPECT_EQ(nw.calls, 10);
  EXPECT_EQ(describe(h), describe(g));
}

// A copy of an owning reader/writer owns its own functors.
TEST_F(XmlGraphTest, CopyOfOwnerOutlivesTheOriginal)
{
  G g = sample_graph();
  auto * original = new XG;
  XG copy = *original;
  delete original;

  copy(g, file("owned.xml"));
  G h = copy(file("owned.xml"));
  EXPECT_EQ(describe(h), describe(g));
}

TEST_F(XmlGraphTest, CustomElementNamesRoundTrip)
{
  G g = sample_graph();
  XG xml;
  xml.set_graph_name("network");
  xml.set_node_name("vertex");
  xml.set_arc_name("edge");
  EXPECT_EQ(xml.get_node_name(), "vertex");
  xml(g, file("names.xml"));

  G h = xml(file("names.xml"));
  EXPECT_EQ(describe(h), describe(g));

  // With the default names nothing in the file is recognized.
  XG defaults;
  G empty = defaults(file("names.xml"));
  EXPECT_EQ(empty.get_num_nodes(), 0u);
}
