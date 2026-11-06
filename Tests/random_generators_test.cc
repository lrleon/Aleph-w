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
 * @file random_generators_test.cc
 * @brief The random generators make, for a seed, what they made with GSL.
 *
 * The random graphs and digraphs, the random networks, the random trees, the
 * cuts of Karger, the seeds of a Bloom filter, the samples of a reservoir,
 * the Euclidean graphs and the table of jsw_hash() took their numbers from
 * GSL, and take them now from a Gsl_Compatible_Rng. For six seeds, what each
 * one makes must be what it made before: each line of output is compared, by
 * its FNV-1a hash, with the one of GSL. The real numbers are written with six
 * digits, so that a last bit of the logarithm of another C library does not
 * change a line.
 */

#include <gtest/gtest.h>
#include <Karger.H>
#include <ah-random.H>
#include <bloom-filter.H>
#include <euclidian-graph-common.H>
#include <hash-fct.H>
#include <random_graph.H>
#include <random_net.H>
#include <random_tree.H>
#include <reservoir-sampling.H>
#include <tpl_graph.H>
#include <tpl_net.H>
#include <tpl_sgraph.H>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace Aleph;

namespace
{
  using UGraph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using DGraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using Net = Net_Graph<Net_Node<Empty_Class>, Net_Arc<Empty_Class, double>>;
  using EGraph = List_SGraph<Graph_Snode<My_P>, Graph_Sarc<double>>;

  void num(std::string & out, const unsigned long long x)
  {
    out += ' ';
    out += std::to_string(x);
  }

  void real(std::string & out, const double x)
  {
    char buf[40];
    std::snprintf(buf, sizeof buf, " %.6g", x);
    out += buf;
  }

  // The nodes are numbered in the order of the graph; then each arc is
  // written by the numbers of its nodes, in the order of the graph too, with
  // what arc_info writes of it
  template <class GT, class Node_Info, class Arc_Info>
  std::string describe(const GT & g, Node_Info node_info, Arc_Info arc_info)
  {
    std::string out;
    std::unordered_map<const void *, size_t> number;
    num(out, g.get_num_nodes());
    num(out, g.get_num_arcs());
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr();
        number.emplace(p, number.size());
        node_info(out, p);
      }
    out += " |";
    for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
      {
        auto a = it.get_curr();
        num(out, number.at(g.get_src_node(a)));
        num(out, number.at(g.get_tgt_node(a)));
        arc_info(out, a);
      }
    return out;
  }

  template <class GT>
  std::string describe(const GT & g)
  {
    return describe(g, [] (std::string &, auto) {}, [] (std::string &, auto) {});
  }

  // The nodes and the arcs of a List_SGraph go in the order of their
  // addresses, which changes from a run to another: the nodes are written by
  // their positions, which are distinct, in their order, and then each arc by
  // the positions of its ends, the least first, in their order, with its
  // weight
  std::string describe_euclidian(const EGraph & g)
  {
    using Point = std::pair<long, long>;
    std::vector<Point> nodes;
    for (EGraph::Node_Iterator it(g); it.has_curr(); it.next_ne())
      nodes.emplace_back(it.get_curr()->get_info().x, it.get_curr()->get_info().y);
    std::sort(nodes.begin(), nodes.end());

    std::vector<std::tuple<Point, Point, double>> arcs;
    for (EGraph::Arc_Iterator it(g); it.has_curr(); it.next_ne())
      {
        auto a = it.get_curr();
        const My_P & p = g.get_src_node(a)->get_info();
        const My_P & q = g.get_tgt_node(a)->get_info();
        Point u(p.x, p.y), v(q.x, q.y);
        if (v < u)
          std::swap(u, v);
        arcs.emplace_back(u, v, a->get_info());
      }
    std::sort(arcs.begin(), arcs.end());

    std::string out;
    num(out, g.get_num_nodes());
    num(out, g.get_num_arcs());
    for (const auto & [x, y] : nodes)
      {
        num(out, x);
        num(out, y);
      }
    out += " |";
    for (const auto & [u, v, w] : arcs)
      {
        num(out, u.first);
        num(out, u.second);
        num(out, v.first);
        num(out, v.second);
        real(out, w);
      }
    return out;
  }

  void tree_pre(Tree_Node<int> * p, std::string & out)
  {
    size_t children = 0;
    for (auto c = p->get_left_child(); c != nullptr; c = c->get_right_sibling())
      ++children;
    num(out, children);
    for (auto c = p->get_left_child(); c != nullptr; c = c->get_right_sibling())
      tree_pre(c, out);
  }

  // Two cliques of six nodes, joined by three arcs, with three more arcs
  // inside the first one
  UGraph karger_graph()
  {
    UGraph g;
    std::vector<UGraph::Node *> n;
    for (int i = 0; i < 12; ++i)
      n.push_back(g.insert_node(i));
    for (int c = 0; c < 12; c += 6)
      for (int i = c; i < c + 6; ++i)
        for (int j = i + 1; j < c + 6; ++j)
          g.insert_arc(n[i], n[j]);
    g.insert_arc(n[0], n[6]);
    g.insert_arc(n[2], n[9]);
    g.insert_arc(n[5], n[11]);
    g.insert_arc(n[1], n[3]);
    g.insert_arc(n[2], n[4]);
    g.insert_arc(n[0], n[5]);
    return g;
  }

  // A cut: its size, and the numbers of the nodes of each side and of the
  // arcs that cross it, in the order of their lists
  std::string describe_cut(UGraph & g, const size_t size,
                           const DynList<UGraph::Node *> & vs,
                           const DynList<UGraph::Node *> & vt,
                           const DynList<UGraph::Arc *> & cut)
  {
    std::unordered_map<const void *, size_t> number;
    for (UGraph::Node_Iterator it(g); it.has_curr(); it.next_ne())
      number.emplace(it.get_curr(), number.size());
    std::unordered_map<const void *, size_t> arc_number;
    for (UGraph::Arc_Iterator it(g); it.has_curr(); it.next_ne())
      arc_number.emplace(it.get_curr(), arc_number.size());

    std::string out;
    num(out, size);
    out += " S";
    for (auto it = vs.get_it(); it.has_curr(); it.next_ne())
      num(out, number.at(it.get_curr()));
    out += " T";
    for (auto it = vt.get_it(); it.has_curr(); it.next_ne())
      num(out, number.at(it.get_curr()));
    out += " C";
    for (auto it = cut.get_it(); it.has_curr(); it.next_ne())
      num(out, arc_number.at(it.get_curr()));
    return out;
  }

  std::uint64_t fnv1a(const std::string & s)
  {
    std::uint64_t h = 0xcbf29ce484222325ULL;
    for (const unsigned char c : s)
      {
        h ^= c;
        h *= 0x100000001b3ULL;
      }
    return h;
  }

  // The lines of output for a seed: the seed, and then each generator
  std::vector<std::string> lines_for(const unsigned long s)
  {
    std::vector<std::string> lines;
    std::string out = "== seed";
    num(out, s);
    lines.push_back(out);

    {
      Random_Graph<UGraph> gen(s);
      lines.push_back("graph sparse:" + describe(gen(30, size_t(60))));
      lines.push_back("graph p:" + describe(gen(25, 0.2)));
      lines.push_back("graph sparse, not connected:" + describe(gen(30, size_t(20), false)));
      lines.push_back("graph eulerian:" + describe(gen.eulerian(20, size_t(40))));
      lines.push_back("graph eulerian p:" + describe(gen.eulerian(16, 0.3)));
      lines.push_back("graph hamiltonian:" + describe(gen.sufficient_hamiltonian(14, 0.3)));
    }
    {
      Random_Digraph<DGraph> gen(s);
      lines.push_back("digraph sparse:" + describe(gen(30, size_t(70))));
      lines.push_back("digraph p:" + describe(gen(20, 0.15)));
      lines.push_back("digraph eulerian:" + describe(gen.eulerian(16, size_t(40))));
      lines.push_back("digraph eulerian p:" + describe(gen.eulerian(12, 0.3)));
      lines.push_back("digraph hamiltonian:" + describe(gen.sufficient_hamiltonian(12, 0.3)));
    }
    {
      Random_Network_Flow<Net> gen(static_cast<unsigned int>(s));
      Net * net = gen(4, 6);
      lines.push_back("network:" + describe(*net, [] (std::string &, auto) {},
                                            [] (std::string & o, auto a) { real(o, a->cap); }));
      delete net;
      net = gen(3, 5, 0.3, 50, 10, 0.5, 0.1);
      lines.push_back("network 2:" + describe(*net, [] (std::string &, auto) {},
                                              [] (std::string & o, auto a) { real(o, a->cap); }));
      delete net;
    }
    {
      RandTree<int> gen(s);
      for (const size_t n : {1, 2, 9, 60})
        {
          Tree_Node<int> * t = gen(n);
          out = "tree " + std::to_string(n) + ":";
          tree_pre(t, out);
          lines.push_back(out);
          destroy_tree(t);
        }
    }
    {
      UGraph g = karger_graph();
      Karger_Min_Cut<UGraph> karger(s);
      DynList<UGraph::Node *> vs, vt;
      DynList<UGraph::Arc *> cut;
      size_t size = karger(g, vs, vt, cut, 3);
      out = "karger:" + describe_cut(g, size, vs, vt, cut);
      vs.empty();
      vt.empty();
      cut.empty();
      size = karger.fast(g, vs, vt, cut, 2);
      out += " fast:" + describe_cut(g, size, vs, vt, cut);
      num(out, karger.compute_min_cut_size(g, 4));
      // s + 1 modulo the maximum of the generator, as reseed() takes it, so
      // that it is the same where unsigned long has 32 bits, and s + 1 would
      // wrap to 0
      karger.reseed(static_cast<unsigned long>((s + 1ULL) % Gsl_Compatible_Rng::max()));
      num(out, karger.compute_min_cut_size(g, 1));
      lines.push_back(out);
    }
    {
      Bloom_Filter<int> f(1024, 7, dft_seed_hash_ptr_fct<int>, s);
      const DynList<size_t> seeds = f.hash_seeds();
      out = "bloom:";
      for (auto it = seeds.get_it(); it.has_curr(); it.next_ne())
        num(out, it.get_curr());
      lines.push_back(out);
    }
    {
      Reservoir_Sampler<int> sampler(10, s);
      for (int i = 0; i < 1000; ++i)
        sampler.update(i);
      out = "reservoir:";
      for (const int x : sampler.get_sample())
        num(out, x);
      sampler.clear();
      for (int i = 0; i < 300; ++i)
        sampler.update(i);
      out += " |";
      for (const int x : sampler.get_sample())
        num(out, x);
      std::vector<int> v(500);
      for (int i = 0; i < 500; ++i)
        v[i] = 3 * i;
      out += " |";
      for (const int x : reservoir_sample(v.begin(), v.end(), 7, s))
        num(out, x);
      lines.push_back(out);
    }
    {
      const EGraph g = gen_random_euclidian_graph<EGraph>(25, 50, 40, 30,
                                                          static_cast<unsigned int>(s));
      lines.push_back("euclidian:" + describe_euclidian(g));
    }
    {
      init_jsw(static_cast<std::uint32_t>(s));
      out = "jsw:";
      for (int b = 0; b < 256; ++b)
        {
          const unsigned char c = static_cast<unsigned char>(b);
          num(out, jsw_hash(&c, 1));
        }
      num(out, jsw_hash("Aleph-w"));
      lines.push_back(out);
      init_jsw(Default_Hash_Seed);
    }
    return lines;
  }
} // namespace

TEST(RandomGenerators, WhatTheyMadeWithGsl)
{
  // The hashes of the lines that the generators gave with GSL. For each
  // seed: the line of the seed; the graphs, sparse, of a probability,
  // sparse and not connected, Eulerian, Eulerian of a probability and
  // Hamiltonian; the digraphs, all of these but the one not connected; two
  // networks; the trees of 1, 2, 9 and 60 nodes; the cuts of Karger; the
  // seeds of a Bloom filter; the samples of a reservoir; a Euclidean graph;
  // and the table of jsw_hash()
  static const std::uint64_t golden[] = {
    // seed 0
    0xbf6aeb12f149fed4ULL, 0x6f511526ebaf569dULL, 0x4deba50a2eb8f7bfULL,
    0x381da76af356cab3ULL, 0x7d7424f2d8d0b8e1ULL, 0xc50c17f5f47b752bULL,
    0x1f0f26144038e7afULL, 0xb31eb3736eb3f411ULL, 0x6e272aeee69e1748ULL,
    0x8003699f11cf9091ULL, 0xdd2be74b369a4f2cULL, 0xe4856f3b289dccc4ULL,
    0x879b4e00418acd95ULL, 0x604a5935b83cf980ULL, 0x869a8495dc14f410ULL,
    0x1fd3b68b81e0134cULL, 0x7deb970b96ba0f48ULL, 0xc021367a91012c46ULL,
    0x43a0c494702d1e9dULL, 0x3d624cbf1b43cc29ULL, 0x023cff92af896d37ULL,
    0xcc42b61139e1f130ULL, 0xc231bc7683da6e00ULL,
    // seed 1
    0xbf6aec12f14a0087ULL, 0x56d0bbcf5af437ceULL, 0x3ab41b2c09e37704ULL,
    0x573c642fbf2f41e0ULL, 0x070de05da19320c0ULL, 0x264d306783592285ULL,
    0x6cedd2e2e32a2cb3ULL, 0xdc9175f4ec78fecdULL, 0x54041d6fd6ca1e58ULL,
    0x4a3a05c605ca3a0eULL, 0x52525b04fd596d26ULL, 0x16797da1dd32516eULL,
    0x03ca174932a6cec9ULL, 0x8a6f04069cf466d3ULL, 0x869a8495dc14f410ULL,
    0x1fd3b68b81e0134cULL, 0x6e42745bebcf360aULL, 0xf94ee3ba7def82ccULL,
    0x961132833c4dbbfbULL, 0x08460e090b658b25ULL, 0xfc3619b02331dd7eULL,
    0xc8efa1228c3f4cfbULL, 0xd011d4ff65e89969ULL,
    // seed 12345
    0x118e2e24d31a4c4fULL, 0x47a3a754887a996bULL, 0x54361a13cda8abb7ULL,
    0x175dd2f949705fd8ULL, 0x87fdc70c5167a565ULL, 0x6faffbcd27687fedULL,
    0xcce4ecf6e78fe225ULL, 0x3965f88a369d62a9ULL, 0x3435461ab9f4aa48ULL,
    0xa5ec352947df983cULL, 0x755a5e0527a471bfULL, 0x53b99ddd4a1ef6b0ULL,
    0x4dc48e12ede388d7ULL, 0xdcb58dc54b6f1424ULL, 0x869a8495dc14f410ULL,
    0x1fd3b68b81e0134cULL, 0x01436d933b157c12ULL, 0xaa77eac337f88546ULL,
    0x5b422700949e4ed5ULL, 0x2e3c29003279ca09ULL, 0xa38ea345c62a0ffcULL,
    0xdf00ab511e4a950fULL, 0x7e1acb6a3066d4d8ULL,
    // seed 4294967294
    0x0d24555385f3f048ULL, 0x126d1c47f46d7d7bULL, 0x6ba1fbd423966727ULL,
    0x4eb4a207cb87715dULL, 0x21656c559ce34bbdULL, 0x594ba60185f79232ULL,
    0x62fec0c48f08de63ULL, 0x691ac06dd47e445aULL, 0x83f445956ca9ed11ULL,
    0x2745874268d4fa70ULL, 0x902e749f5766405dULL, 0x5693c2ea7bab028bULL,
    0xb136634f8e41d264ULL, 0x296a197a405ebf8eULL, 0x869a8495dc14f410ULL,
    0x1fd3b68b81e0134cULL, 0x0ae4cc656280e88cULL, 0xc5223f7e67c1b05aULL,
    0x791ea0482fe7868fULL, 0x1cbed8d3f9ac9a59ULL, 0xec3276c7d3d4d486ULL,
    0x21bc3e31556dc712ULL, 0x11b74001e3c3be38ULL,
    // seed 4294967295
    0x0d24565385f3f1fbULL, 0x6f511526ebaf569dULL, 0x4deba50a2eb8f7bfULL,
    0x381da76af356cab3ULL, 0x7d7424f2d8d0b8e1ULL, 0xc50c17f5f47b752bULL,
    0x1f0f26144038e7afULL, 0xb31eb3736eb3f411ULL, 0x6e272aeee69e1748ULL,
    0x8003699f11cf9091ULL, 0xdd2be74b369a4f2cULL, 0xe4856f3b289dccc4ULL,
    0x879b4e00418acd95ULL, 0x604a5935b83cf980ULL, 0x869a8495dc14f410ULL,
    0x1fd3b68b81e0134cULL, 0x7deb970b96ba0f48ULL, 0xc021367a91012c46ULL,
    0x43a0c494702d1e9dULL, 0x3d624cbf1b43cc29ULL, 0x6ba91bf1c4cdde62ULL,
    0xcc42b61139e1f130ULL, 0xc231bc7683da6e00ULL,
    // seed 4294967301
    0x042cb55380c9916dULL, 0xbb3a9f9aadeda92fULL, 0x72d4029847eef99cULL,
    0x0072c6a658d68841ULL, 0x342bea24d3ed1ff0ULL, 0x4db7bd2097f438f1ULL,
    0xafa2dc2d2911b783ULL, 0x16c2089df454e958ULL, 0x582699ac9686606bULL,
    0x6d8d2143b2a23eaeULL, 0x05a6eb57a52c50f8ULL, 0x885da9241efdf674ULL,
    0x559d5766aa658ddeULL, 0xad574524c68180abULL, 0x869a8495dc14f410ULL,
    0x1fd3b68b81e0134cULL, 0xab1cf6530f406d02ULL, 0xdfacce1ca0dc16e0ULL,
    0x1dbaf9068def4d74ULL, 0xfe5efd0c084ef1d7ULL, 0xe3914214f007e432ULL,
    0xcd587691155ba501ULL, 0x7b22f892cc2c2d90ULL,
  };

  // The table of jsw_hash() is of long: where long has 32 bits and size_t
  // 64, as on Windows, its numbers above 2^31 - 1 are negative, and the
  // hashes of the line of the table, for the seeds below 2^32, are these,
  // which GSL gave there too. Where size_t has 32 bits too, jsw_hash()
  // gives other hashes, which the test does not have: it does not compare
  // the line there
  static const std::uint64_t golden_jsw_long32[] = {
    0x3ff9424109e6654dULL, 0x49f945dd5ec91e54ULL, 0x3521995e48d2e088ULL,
    0x575c8e62a68e5ce6ULL, 0x3ff9424109e6654dULL,
  };

  // The last seed, above 32 bits, exists only where unsigned long has 64:
  // where it has 32, its lines are skipped
  const unsigned long long seeds[] = {0, 1, 12345, 0xfffffffeULL, 0xffffffffULL,
                                      0x100000005ULL};
  const size_t lines_per_seed = 23;
  size_t i = 0;
  for (size_t k = 0; k < std::size(seeds); ++k)
    {
      const unsigned long long s = seeds[k];
      if (s > std::numeric_limits<unsigned long>::max())
        {
          i += lines_per_seed;
          continue;
        }

      const std::vector<std::string> lines = lines_for(static_cast<unsigned long>(s));
      ASSERT_EQ(lines.size(), lines_per_seed);
      for (size_t j = 0; j < lines_per_seed; ++j, ++i)
        {
          ASSERT_LT(i, std::size(golden));
          std::uint64_t expected = golden[i];
          if (sizeof(long) == 4 and j == lines_per_seed - 1) // the table of jsw_hash()
            {
              if (sizeof(std::size_t) != 8)
                continue;
              ASSERT_LT(k, std::size(golden_jsw_long32));
              expected = golden_jsw_long32[k];
            }
          EXPECT_EQ(fnv1a(lines[j]), expected) << lines[j];
        }
    }
  EXPECT_EQ(i, std::size(golden));
}

TEST(RandomGenerators, TheGeneratorsAreNotCopied)
{
  // A copy of a RandTree compiled, and both freed the same generator
  EXPECT_FALSE(std::is_copy_constructible_v<RandTree<int>>);
  EXPECT_FALSE(std::is_copy_assignable_v<RandTree<int>>);

  // The others keep what they had: a network generator is neither copied
  // nor moved, Karger and a reservoir are moved
  EXPECT_FALSE(std::is_copy_constructible_v<Random_Network_Flow<Net>>);
  EXPECT_FALSE(std::is_move_constructible_v<Random_Network_Flow<Net>>);
  EXPECT_FALSE(std::is_copy_constructible_v<Karger_Min_Cut<UGraph>>);
  EXPECT_TRUE(std::is_nothrow_move_constructible_v<Karger_Min_Cut<UGraph>>);
  EXPECT_TRUE(std::is_move_assignable_v<Karger_Min_Cut<UGraph>>);
  EXPECT_FALSE(std::is_copy_constructible_v<Reservoir_Sampler<int>>);
  EXPECT_TRUE(std::is_move_constructible_v<Reservoir_Sampler<int>>);
  EXPECT_FALSE((std::is_copy_constructible_v<Random_Graph<UGraph>>));
}

namespace
{
  // An arc filter whose moves throw while it is armed
  struct Move_Throwing_Filter
  {
    static inline bool armed = false;

    Move_Throwing_Filter() = default;

    Move_Throwing_Filter(const Move_Throwing_Filter &) = default;

    Move_Throwing_Filter(Move_Throwing_Filter &&)
    {
      if (armed)
        throw std::runtime_error("the move of the filter");
    }

    Move_Throwing_Filter & operator = (const Move_Throwing_Filter &) = default;

    Move_Throwing_Filter & operator = (Move_Throwing_Filter &&)
    {
      if (armed)
        throw std::runtime_error("the move of the filter");
      return *this;
    }

    bool operator () (UGraph::Arc *) const noexcept { return true; }
  };
} // namespace

TEST(RandomGenerators, AKargerKeepsItsGeneratorWhenTheMoveOfItsFilterThrows)
{
  using Karger = Karger_Min_Cut<UGraph, Move_Throwing_Filter>;
  UGraph g = karger_graph();
  Karger k(7);
  Karger other(8);
  Move_Throwing_Filter::armed = true;
  EXPECT_THROW((void) Karger(std::move(k)), std::runtime_error);
  EXPECT_THROW(other = std::move(k), std::runtime_error);
  Move_Throwing_Filter::armed = false;

  // Each one makes the cuts of a new solver of its seed
  const std::pair<Karger *, unsigned long> cases[] = {{&k, 7}, {&other, 8}};
  for (const auto & [karger, seed] : cases)
    {
      Karger twin(seed);
      DynList<UGraph::Node *> vs1, vt1, vs2, vt2;
      DynList<UGraph::Arc *> cut1, cut2;
      const size_t c1 = (*karger)(g, vs1, vt1, cut1, 5);
      const size_t c2 = twin(g, vs2, vt2, cut2, 5);
      EXPECT_EQ(describe_cut(g, c1, vs1, vt1, cut1), describe_cut(g, c2, vs2, vt2, cut2))
        << "seed " << seed;
    }
}

TEST(RandomGenerators, TheEuclideanGeneratorClearsItsGenerator)
{
  (void) gen_random_euclidian_graph<EGraph>(10, 20, 10, 10, 3u);
  EXPECT_EQ(rand_gen, nullptr);

  // No nodes: the random graph throws, after rand_gen is set
  EXPECT_THROW((void) gen_random_euclidian_graph<EGraph>(0, 1, 10, 10, 3u), std::domain_error);
  EXPECT_EQ(rand_gen, nullptr);
}

TEST(RandomGenerators, AReservoirRefusesTheElementsThatItsGeneratorCannotPlace)
{
  // The element after n seen is drawn among n + 1 positions, and the
  // generator draws among 2^32 - 1 at most
  Reservoir_Sampler<int> s(3, 42);
  for (int i = 0; i < 3; ++i)
    s.update(i);
  s.set_n_seen_for_testing(static_cast<size_t>(s.rng_range() - 2));
  EXPECT_NO_THROW(s.update(7));
  EXPECT_EQ(s.total_seen(), static_cast<size_t>(s.rng_range() - 1));
  EXPECT_THROW(s.update(8), std::runtime_error);
}

#if ALEPH_WITH_GSL
TEST(RandomGenerators, TheSeedsOfABloomFilterAreTheNumbersOfGsl)
{
  for (const unsigned long s : {0UL, 7UL, 0xfffffffeUL, 0xffffffffUL})
    {
      gsl_rng * r = gsl_rng_alloc(gsl_rng_mt19937);
      gsl_rng_set(r, s % gsl_rng_max(r));
      Bloom_Filter<int> f(512, 9, dft_seed_hash_ptr_fct<int>, s);
      const DynList<size_t> seeds = f.hash_seeds();
      ASSERT_EQ(seeds.size(), 9u);
      for (auto it = seeds.get_it(); it.has_curr(); it.next_ne())
        EXPECT_EQ(it.get_curr(), gsl_rng_get(r)) << "seed " << s;
      gsl_rng_free(r);
    }
}

TEST(RandomGenerators, TheTableOfJswHashHasTheNumbersOfGsl)
{
  // For a key of one byte b, jsw_hash() gives its initial value, 16777551,
  // rotated by one bit, xor the entry b of its table, which keeps a number
  // of GSL as a long
  const size_t rotated = (size_t(16777551) << 1) | (size_t(16777551) >> 31);
  for (const std::uint32_t s : {0u, 1u, 52679987u, 0xfffffffeu, 0xffffffffu})
    {
      init_jsw(s);
      gsl_rng * r = gsl_rng_alloc(gsl_rng_mt19937);
      gsl_rng_set(r, s % gsl_rng_max(r));
      for (int b = 0; b < 256; ++b)
        {
          const unsigned char c = static_cast<unsigned char>(b);
          const long entry = static_cast<long>(gsl_rng_get(r));
          EXPECT_EQ(jsw_hash(&c, 1), rotated ^ static_cast<size_t>(entry))
            << "seed " << s << ", byte " << b;
        }
      gsl_rng_free(r);
    }
  init_jsw(Default_Hash_Seed);
}

TEST(RandomGenerators, TheSampleOfAReservoirIsThatOfGsl)
{
  // Algorithm R, with the positions of gsl_rng_uniform_int(), seeded as the
  // sampler is seeded
  for (const unsigned long s : {0UL, 3UL, 0xffffffffUL})
    {
      gsl_rng * r = gsl_rng_alloc(gsl_rng_mt19937);
      gsl_rng_set(r, s);
      std::vector<int> expected;
      for (int i = 0; i < 2000; ++i)
        if (i < 16)
          expected.push_back(i);
        else if (const unsigned long j = gsl_rng_uniform_int(r, i + 1); j < 16)
          expected[j] = i;
      gsl_rng_free(r);

      Reservoir_Sampler<int> sampler(16, s);
      for (int i = 0; i < 2000; ++i)
        sampler.update(i);
      std::vector<int> got;
      for (const int x : sampler.get_sample())
        got.push_back(x);
      EXPECT_EQ(got, expected) << "seed " << s;
    }
}
#endif
