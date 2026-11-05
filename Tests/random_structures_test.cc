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
 * @file random_structures_test.cc
 * @brief The randomized structures make, for a seed, what they made with GSL.
 *
 * The treaps, the randomized trees, the skip lists, the random queue and the
 * interval trees took their numbers from GSL, and take them now from a
 * Gsl_Compatible_Rng. For six seeds, the shapes of the trees with their
 * priorities, the levels of the skip lists, the order of a random queue and
 * the next number of each generator after the operations must be those of
 * before: each line of output is compared, by its FNV-1a hash, with the one
 * of GSL. The deprecated accessors to a `gsl_rng` are checked too.
 */

#include <gtest/gtest.h>
#include <ah-random.H>
#include <tpl_dynSkipList.H>
#include <tpl_interval_tree.H>
#include <tpl_rand_tree.H>
#include <tpl_random_queue.H>
#include <tpl_skipList.H>
#include <tpl_treap.H>
#include <tpl_treapRk.H>

#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <utility>
#include <vector>

using namespace Aleph;

// The sentinel key of a skip list of int, which its user gives
template <>
int SkipList<int, int>::Node::computeMaxKey() noexcept
{
  return std::numeric_limits<int>::max();
}

namespace
{
  // a fixed scramble of 0 .. n - 1
  std::vector<int> keys(const int n)
  {
    std::vector<int> k(n);
    for (int i = 0; i < n; ++i)
      k[i] = (i * 37 + 11) % n;
    return k;
  }

  void add(std::string & out, const char * fmt, unsigned long a)
  {
    char buf[64];
    std::snprintf(buf, sizeof buf, fmt, a);
    out += buf;
  }

  template <bool Prio, class Node>
  void pre(Node * p, std::string & out)
  {
    if (p == Node::NullPtr)
      return;
    add(out, " %d", static_cast<unsigned long>(KEY(p)));
    if constexpr (Prio)
      add(out, "/%lu", static_cast<unsigned long>(PRIO(p)));
    pre<Prio>(LLINK(p), out);
    pre<Prio>(RLINK(p), out);
  }

  void interval_pre(Interval_Tree<int>::Node * p, std::string & out)
  {
    if (p == Interval_Tree<int>::Node::NullPtr)
      return;
    char buf[64];
    std::snprintf(buf, sizeof buf, " [%d,%d]/%lu", KEY(p).low, KEY(p).high,
                  static_cast<unsigned long>(PRIO(p)));
    out += buf;
    interval_pre(LLINK(p), out);
    interval_pre(RLINK(p), out);
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

  // The lines of output for a seed: the seed, and then each structure
  std::vector<std::string> lines_for(const unsigned long s)
  {
    std::vector<std::string> lines;
    std::string out = "== seed ";
    add(out, "%lu", s);
    lines.push_back(out);

    {
      Treap<int> t(s);
      for (int k : keys(40))
        t.insert(new Treap<int>::Node(k));
      out = "treap:";
      pre<true>(t.getRoot(), out);
      add(out, " next %lu", t.rng().get());
      lines.push_back(out);
      destroyRec(t.getRoot());
    }
    {
      Treap_Rk<int> t(s);
      for (int k : keys(40))
        t.insert(new Treap_Rk<int>::Node(k));
      out = "treap_rk:";
      pre<true>(t.getRoot(), out);
      lines.push_back(out);
      destroyRec(t.getRoot());
    }
    {
      Rand_Tree<int> t(static_cast<unsigned int>(s));
      for (int k : keys(60))
        t.insert(new Rand_Tree<int>::Node(k));
      for (int k = 0; k < 60; k += 7)
        delete t.remove(k);
      out = "rand_tree:";
      pre<false>(t.getRoot(), out);
      add(out, " next %lu", t.rng().get());
      lines.push_back(out);
      destroyRec(t.getRoot());
    }
    {
      SkipList<int, int> l(s);
      out = "skiplist:";
      for (int i = 0; i < 64; ++i)
        add(out, " %d", static_cast<unsigned long>(l.generateRandomLevel()));
      l.set_seed(s + 777);
      for (int i = 0; i < 64; ++i)
        add(out, " %d", static_cast<unsigned long>(l.generateRandomLevel()));
      lines.push_back(out);
    }
    {
      DynSkipList<int> l(s);
      for (int k : keys(200))
        l.insert(k);
      out = "dynskiplist: next ";
      add(out, "%lu", l.rng().get());
      l.set_seed(s + 5);
      for (int k = 200; k < 260; ++k)
        l.insert(k);
      add(out, " next %lu", l.rng().get());
      lines.push_back(out);
    }
    {
      Random_Set<int> q;
      q.set_seed(s);
      for (int k = 0; k < 50; ++k)
        q.append(k);
      out = "random_set:";
      for (int k = 0; k < 20; ++k)
        add(out, " %d", static_cast<unsigned long>(q.get()));
      for (int k = 50; k < 60; ++k)
        q.put(k);
      while (not q.is_empty())
        add(out, " %d", static_cast<unsigned long>(q.get()));
      add(out, " next %lu", q.rng().get());
      lines.push_back(out);
    }
    {
      Interval_Tree<int> t(s);
      for (int k : keys(30))
        t.insert(new Interval_Tree<int>::Node(Interval<int>(k, k + (k % 5))));
      out = "interval_tree:";
      interval_pre(t.getRoot(), out);
      add(out, " next %lu", t.rng().get());
      lines.push_back(out);
      destroyRec(t.getRoot());
    }
    return lines;
  }
} // namespace

TEST(RandomStructures, WhatTheyMadeWithGsl)
{
  // The hashes of the lines that the structures gave with GSL
  static const std::uint64_t golden[] = {
    // seed 0: the line of the seed, treap, treap_rk, rand_tree, skiplist,
    // dynskiplist, random_set and interval_tree
    0xbf6aeb12f149fed4ULL, 0xaf0b5ce48c61a393ULL, 0x1d6615e62a03143dULL,
    0x6a2166086b45a98cULL, 0x6c63d3b271929409ULL, 0xd12483814d06e78eULL,
    0xb24574a6225d43afULL, 0x7cf954f63b5b566cULL,
    // seed 1: the line of the seed, treap, treap_rk, rand_tree, skiplist,
    // dynskiplist, random_set and interval_tree
    0xbf6aec12f14a0087ULL, 0x9278d4c463049723ULL, 0x4a4d7bcbd6401c76ULL,
    0xc799efa20f1ddcc9ULL, 0x7d0232dce7b87affULL, 0xe9fc8e3d9141e44bULL,
    0x348d2efb0da28b7dULL, 0xe155284b0cd406abULL,
    // seed 12345: the line of the seed, treap, treap_rk, rand_tree, skiplist,
    // dynskiplist, random_set and interval_tree
    0x118e2e24d31a4c4fULL, 0x9371ee5b6011fe82ULL, 0x023bbd9db1128c32ULL,
    0x73f6f6e38ba2c668ULL, 0x99c9ec1e34beceadULL, 0x9a7271d69b2bb3f2ULL,
    0xc2282e94fa57e861ULL, 0x279881ccaeee8bb8ULL,
    // seed 4294967294: the line of the seed, treap, treap_rk, rand_tree, skiplist,
    // dynskiplist, random_set and interval_tree
    0x0d24555385f3f048ULL, 0x4f9361527a9e7f0bULL, 0xd30abfc5d0c391a5ULL,
    0xbd37d372779f3bccULL, 0x799d90ae07e107c6ULL, 0xc197f84291991cd4ULL,
    0xb18163959293eb53ULL, 0x25c23e87534c6092ULL,
    // seed 4294967295: the line of the seed, treap, treap_rk, rand_tree, skiplist,
    // dynskiplist, random_set and interval_tree
    0x0d24565385f3f1fbULL, 0xaf0b5ce48c61a393ULL, 0x1d6615e62a03143dULL,
    0x6a2166086b45a98cULL, 0x653c1a017fdf92a8ULL, 0x098538cb93321526ULL,
    0x57f5924f1a57c31bULL, 0x7cf954f63b5b566cULL,
    // seed 4294967301: the line of the seed, treap, treap_rk, rand_tree, skiplist,
    // dynskiplist, random_set and interval_tree
    0x042cb55380c9916dULL, 0x521310421689f20dULL, 0x4f50ad59636c17e3ULL,
    0xdcef84f3309fdf82ULL, 0xfe21fd5f50f37f28ULL, 0xbe5611ca7fbc90b9ULL,
    0xf18df4da3be551c4ULL, 0x491cadffd8e29cefULL,
  };

  // The last seed, above 32 bits, exists only where unsigned long has 64:
  // where it has 32, its lines are skipped
  const unsigned long long seeds[] = {0, 1, 12345, 0xfffffffeULL, 0xffffffffULL,
                                      0x100000005ULL};
  const size_t lines_per_seed = 8;
  size_t i = 0;
  for (const unsigned long long s : seeds)
    {
      if (s > std::numeric_limits<unsigned long>::max())
        {
          i += lines_per_seed;
          continue;
        }

      const std::vector<std::string> lines = lines_for(static_cast<unsigned long>(s));
      ASSERT_EQ(lines.size(), lines_per_seed);
      for (const std::string & line : lines)
        {
          ASSERT_LT(i, std::size(golden));
          EXPECT_EQ(fnv1a(line), golden[i]) << line;
          ++i;
        }
    }
  EXPECT_EQ(i, std::size(golden));
}

#if ALEPH_WITH_GSL
TEST(RandomStructures, TheTreapPrioritiesAreTheNumbersOfGsl)
{
  // A treap seeded with s took its priorities, one per insertion, from a
  // gsl_rng_mt19937 seeded with s, through an unsigned int, modulo 2^32 - 1
  for (const unsigned long s : {0UL, 7UL, 0xfffffffeUL, 0xffffffffUL})
    {
      gsl_rng * r = gsl_rng_alloc(gsl_rng_mt19937);
      gsl_rng_set(r, static_cast<unsigned int>(s) % gsl_rng_max(r));
      Treap<int> t(s);
      std::vector<Treap<int>::Node *> nodes;
      for (int k : keys(100))
        {
          nodes.push_back(new Treap<int>::Node(k));
          t.insert(nodes.back());
        }
      for (auto * p : nodes)
        EXPECT_EQ(PRIO(p), gsl_rng_get(r)) << "seed " << s << ", key " << KEY(p);
      destroyRec(t.getRoot());
      gsl_rng_free(r);
    }
}

#  if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#  elif defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning(disable: 4996)
#  endif

namespace
{
  // The gsl_rng of a structure acts on its generator: a seed set through it
  // reseeds the generator, and the draws through it are those of a twin
  template <class S>
  void check_adapter(S & s, gsl_rng * r)
  {
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(gsl_rng_min(r), 0UL);
    EXPECT_EQ(gsl_rng_max(r), 0xffffffffUL);
    gsl_rng_set(r, 99);
    EXPECT_EQ(s.rng(), Gsl_Compatible_Rng(99));
    Gsl_Compatible_Rng twin(99);
    for (int i = 0; i < 100; ++i)
      {
        EXPECT_EQ(gsl_rng_uniform_int(r, 1000), twin.uniform_int(1000));
        EXPECT_EQ(gsl_rng_get(r), twin.get());
        EXPECT_EQ(gsl_rng_uniform(r), twin.uniform());
      }
    EXPECT_EQ(s.rng(), twin);
  }
}

TEST(RandomStructures, TheDeprecatedGslRngActsOnTheGenerator)
{
  Treap<int> treap(1);
  check_adapter(treap, treap.gsl_rng_object());
  EXPECT_EQ(treap.gsl_rng_object(), treap.gsl_rng_object()); // made once

  Rand_Tree<int> rand_tree(1);
  check_adapter(rand_tree, rand_tree.gsl_rng_object());

  SkipList<int, int> skip_list(1UL);
  check_adapter(skip_list, skip_list.gsl_rng_object());

  DynSkipList<int> dyn_skip_list(1UL);
  check_adapter(dyn_skip_list, dyn_skip_list.gsl_rng_object());

  Random_Set<int> random_set;
  check_adapter(random_set, random_set.get_rng());

  Interval_Tree<int> interval_tree(1);
  check_adapter(interval_tree, interval_tree.gsl_rng_object());
}

TEST(RandomStructures, TheDeprecatedGslRngFollowsItsGenerator)
{
  // after a swap, the gsl_rng of a treap reads from the generator that went
  // to the other one, as the gsl_rng itself went before
  Treap<int> a(1), b(2);
  gsl_rng * ra = a.gsl_rng_object();
  a.swap(b);
  Gsl_Compatible_Rng twin = b.rng();
  EXPECT_EQ(twin, Gsl_Compatible_Rng(1));
  EXPECT_EQ(gsl_rng_get(ra), twin.get());
  EXPECT_EQ(b.rng(), twin);
}

#  if defined(__GNUC__)
#    pragma GCC diagnostic pop
#  elif defined(_MSC_VER)
#    pragma warning(pop)
#  endif
#endif // ALEPH_WITH_GSL

TEST(RandomStructures, MovesAndSwapsKeepTheGenerators)
{
  Rand_Tree<int> a(5);
  const Gsl_Compatible_Rng twin = a.rng();
  Rand_Tree<int> b(std::move(a));
  EXPECT_EQ(b.rng(), twin);

  Rand_Tree<int> c(6);
  c = std::move(b);
  EXPECT_EQ(c.rng(), twin);

  DynSkipList<int> d(7UL), e(8UL);
  d.swap(e);
  EXPECT_EQ(d.rng(), Gsl_Compatible_Rng(8));
  EXPECT_EQ(e.rng(), Gsl_Compatible_Rng(7));

  // a copy of a DynSkipList makes its own generator; a copy assignment
  // keeps the one of the target
  DynSkipList<int> f(9UL);
  f.insert(1);
  const Gsl_Compatible_Rng before = f.rng();
  DynSkipList<int> g(10UL);
  f = g;
  EXPECT_EQ(f.rng(), before);

  Random_Set<int> q, r;
  q.set_seed(3);
  r.set_seed(4);
  q.swap(r);
  EXPECT_EQ(q.rng(), Gsl_Compatible_Rng(4));
  EXPECT_EQ(r.rng(), Gsl_Compatible_Rng(3));
  Random_Set<int> moved(std::move(q));
  EXPECT_EQ(moved.rng(), Gsl_Compatible_Rng(4));
}

namespace
{
  // One of two treaps, by value: the copy cannot be elided, so that the one
  // returned is moved
  template <class Tree>
  Tree one_of_two(const bool first, typename Tree::Node * p, Gsl_Compatible_Rng & twin)
  {
    Tree a(12), b(13);
    Tree & t = first ? a : b;
    t.insert(p);
    twin = t.rng();
    if (first)
      return a;
    return b;
  }

  // Whether the root of a treap is in the treap, and not in the one that it
  // was moved from
  template <class Tree>
  bool owns_its_root(Tree & t)
  {
    const auto self = reinterpret_cast<std::uintptr_t>(&t);
    const auto root = reinterpret_cast<std::uintptr_t>(&t.getRoot());
    return root >= self and root < self + sizeof(t);
  }

  template <class Tree>
  void check_moves()
  {
    for (const bool first : {true, false})
      {
        typename Tree::Node n1(1), n2(2);
        Gsl_Compatible_Rng twin;
        Tree t = one_of_two<Tree>(first, &n1, twin);
        EXPECT_TRUE(owns_its_root(t));
        EXPECT_EQ(t.rng(), twin);
        EXPECT_EQ(t.search(1), &n1);
        t.insert(&n2);
        EXPECT_EQ(t.search(2), &n2);

        Tree u(std::move(t));
        EXPECT_TRUE(owns_its_root(u));
        EXPECT_EQ(u.search(1), &n1);
        EXPECT_EQ(u.search(2), &n2);
        EXPECT_EQ(t.getRoot(), Tree::Node::NullPtr);

        // the moved one takes nodes and a generator again by a swap
        Tree v(14);
        t.swap(v);
        EXPECT_EQ(t.rng(), Gsl_Compatible_Rng(14));
        EXPECT_EQ(v.getRoot(), Tree::Node::NullPtr);
      }
  }
} // namespace

TEST(RandomStructures, AMovedTreapTakesItsNodesAndItsGenerator)
{
  // before, a move was a copy: the treap kept the head of the one that it
  // came from, and both freed the same gsl_rng
  check_moves<Treap<int>>();
  check_moves<Treap_Rk<int>>();
}

TEST(RandomStructures, TheSeedsAreTakenAsForGsl)
{
  // through an unsigned int, modulo 2^32 - 1, in the constructors of the
  // treaps and of the interval tree; set_seed() takes the seed as is
  Treap<int> t(0xffffffffUL);
  EXPECT_EQ(t.rng(), Gsl_Compatible_Rng(0)); // 2^32 - 1 modulo itself
  t.set_seed(0xffffffffUL);
  EXPECT_EQ(t.rng(), Gsl_Compatible_Rng(0xffffffffUL));

  Interval_Tree<int> it(0xffffffffUL);
  EXPECT_EQ(it.rng(), Gsl_Compatible_Rng(0));

  // the randomized tree and the skip lists, modulo 2^32 - 1 also in set_seed()
  // of the randomized tree
  Rand_Tree<int> rt(0xffffffffU);
  EXPECT_EQ(rt.rng(), Gsl_Compatible_Rng(0));
  rt.set_seed(0xffffffffUL);
  EXPECT_EQ(rt.rng(), Gsl_Compatible_Rng(0));

  SkipList<int, int> sl(0xffffffffUL);
  EXPECT_EQ(sl.rng(), Gsl_Compatible_Rng(0));
  sl.set_seed(0xffffffffUL);
  EXPECT_EQ(sl.rng(), Gsl_Compatible_Rng(0xffffffffUL));
}
