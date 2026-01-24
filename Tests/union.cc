
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
 * @file union.cc
 * @brief Tests for Union
 */

#include <gtest/gtest.h>

#include <tpl_union.H>

#include <random>
#include <string>
#include <vector>

using namespace std;

namespace {

class FixedRelationInspector : public Fixed_Relation
{
public:
  using Fixed_Relation::Fixed_Relation;
  using Fixed_Relation::depth;
  using Fixed_Relation::id;
  using Fixed_Relation::root;
};

struct RefDsu
{
  vector<size_t> parent;
  vector<size_t> sz;
  size_t blocks = 0;

  explicit RefDsu(size_t n) : parent(n), sz(n, 1), blocks(n)
  {
    for (size_t i = 0; i < n; ++i)
      parent[i] = i;
  }

  void ensure(size_t n)
  {
    if (n <= parent.size())
      return;

    const size_t old = parent.size();
    parent.resize(n);
    sz.resize(n, 1);
    for (size_t i = old; i < n; ++i)
      parent[i] = i;
    blocks += n - old;
  }

  size_t find(size_t x)
  {
    while (x != parent[x])
      {
        parent[x] = parent[parent[x]];
        x = parent[x];
      }
    return x;
  }

  bool connected(size_t a, size_t b)
  {
    return find(a) == find(b);
  }

  void unite(size_t a, size_t b)
  {
    a = find(a);
    b = find(b);
    if (a == b)
      return;

    if (sz[a] < sz[b])
      {
        parent[a] = b;
        sz[b] += sz[a];
      }
    else
      {
        parent[b] = a;
        sz[a] += sz[b];
      }

    --blocks;
  }
};

TEST(UnionFixedRelation, empty_and_out_of_range)
{
  Fixed_Relation rel;
  EXPECT_EQ(rel.size(), 0u);
  EXPECT_EQ(rel.get_num_blocks(), 0u);
  EXPECT_THROW(rel.are_connected(0, 0), std::out_of_range);
  EXPECT_THROW(rel.join(0, 0), std::out_of_range);
}

TEST(UnionFixedRelation, basic_connectivity_and_blocks)
{
  Fixed_Relation rel(5);
  EXPECT_EQ(rel.size(), 5u);
  EXPECT_EQ(rel.get_num_blocks(), 5u);

  EXPECT_TRUE(rel.are_connected(0, 0));
  EXPECT_FALSE(rel.are_connected(0, 1));

  rel.join(0, 1);
  EXPECT_TRUE(rel.are_connected(0, 1));
  EXPECT_EQ(rel.get_num_blocks(), 4u);

  rel.join(1, 2);
  EXPECT_TRUE(rel.are_connected(0, 2));
  EXPECT_EQ(rel.get_num_blocks(), 3u);

  const size_t before = rel.get_num_blocks();
  rel.join(0, 2);
  EXPECT_EQ(rel.get_num_blocks(), before);

  EXPECT_THROW(rel.are_connected(0, 5), std::out_of_range);
  EXPECT_THROW(rel.join(4, 5), std::out_of_range);
}

TEST(UnionFixedRelation, set_n_resets_state)
{
  Fixed_Relation rel(4);
  rel.join(0, 1);
  EXPECT_TRUE(rel.are_connected(0, 1));
  EXPECT_EQ(rel.get_num_blocks(), 3u);

  rel.set_n(2);
  EXPECT_EQ(rel.size(), 2u);
  EXPECT_EQ(rel.get_num_blocks(), 2u);
  EXPECT_FALSE(rel.are_connected(0, 1));
  EXPECT_THROW(rel.are_connected(0, 2), std::out_of_range);
}

TEST(UnionFixedRelation, depth_cycle_and_invalid_parent_detection)
{
  {
    FixedRelationInspector rel(3);
    rel.id(0) = 1;
    rel.id(1) = 0;
    rel.id(2) = 2;
    EXPECT_THROW(rel.depth(0), std::logic_error);
  }

  {
    FixedRelationInspector rel(3);
    rel.id(0) = 10;
    EXPECT_THROW(rel.depth(0), std::out_of_range);
  }
}

TEST(UnionFixedRelation, randomized_operations_match_reference)
{
  constexpr size_t n = 50;
  Fixed_Relation rel(n);
  RefDsu ref(n);

  std::mt19937 rng(1);
  std::uniform_int_distribution<size_t> pick(0, n - 1);

  for (int step = 0; step < 2000; ++step)
    {
      const size_t a = pick(rng);
      const size_t b = pick(rng);

      if (step % 3 == 0)
        {
          EXPECT_EQ(rel.are_connected(a, b), ref.connected(a, b));
        }
      else
        {
          rel.join(a, b);
          ref.unite(a, b);
          EXPECT_EQ(rel.get_num_blocks(), ref.blocks);
        }
    }
}

TEST(UnionRelation, grows_dynamically_and_updates_blocks)
{
  Relation rel;
  EXPECT_EQ(rel.size(), 0u);
  EXPECT_EQ(rel.get_num_blocks(), 0u);

  EXPECT_TRUE(rel.are_connected(0, 0));
  EXPECT_EQ(rel.size(), 1u);
  EXPECT_EQ(rel.get_num_blocks(), 1u);

  EXPECT_FALSE(rel.are_connected(0, 1));
  EXPECT_EQ(rel.size(), 2u);
  EXPECT_EQ(rel.get_num_blocks(), 2u);

  rel.join(0, 1);
  EXPECT_TRUE(rel.are_connected(0, 1));
  EXPECT_EQ(rel.size(), 2u);
  EXPECT_EQ(rel.get_num_blocks(), 1u);

  EXPECT_TRUE(rel.are_connected(100, 100));
  EXPECT_EQ(rel.size(), 101u);
  EXPECT_EQ(rel.get_num_blocks(), 100u);

  EXPECT_FALSE(rel.are_connected(100, 0));
}

TEST(UnionRelation, randomized_growth_matches_reference)
{
  Relation rel;
  RefDsu ref(0);

  std::mt19937 rng(2);
  std::uniform_int_distribution<size_t> pick(0, 200);

  for (int step = 0; step < 1500; ++step)
    {
      const size_t a = pick(rng);
      const size_t b = pick(rng);

      ref.ensure(std::max(a, b) + 1);

      if (step % 4 == 0)
        {
          EXPECT_EQ(rel.are_connected(a, b), ref.connected(a, b));
          EXPECT_EQ(rel.size(), ref.parent.size());
          EXPECT_EQ(rel.get_num_blocks(), ref.blocks);
        }
      else
        {
          rel.join(a, b);
          ref.unite(a, b);
          EXPECT_EQ(rel.size(), ref.parent.size());
          EXPECT_EQ(rel.get_num_blocks(), ref.blocks);
        }
    }
}

TEST(UnionRelationT, inserts_items_on_query_and_preserves_unique_ids)
{
  Relation_T<int> rel;

  EXPECT_TRUE(rel.are_connected(1, 1));
  EXPECT_EQ(rel.size(), 1u);
  EXPECT_EQ(rel.get_num_blocks(), 1u);

  EXPECT_FALSE(rel.are_connected(1, 2));
  EXPECT_EQ(rel.size(), 2u);
  EXPECT_EQ(rel.get_num_blocks(), 2u);

  rel.join(1, 2);
  EXPECT_TRUE(rel.are_connected(1, 2));
  EXPECT_EQ(rel.get_num_blocks(), 1u);

  const size_t size_before = rel.size();
  const size_t blocks_before = rel.get_num_blocks();
  EXPECT_TRUE(rel.are_connected(2, 1));
  EXPECT_EQ(rel.size(), size_before);
  EXPECT_EQ(rel.get_num_blocks(), blocks_before);
}

TEST(UnionRelationT, supports_multiple_item_types)
{
  Relation_T<std::string> rel;

  EXPECT_FALSE(rel.are_connected(std::string("a"), std::string("b")));
  EXPECT_EQ(rel.size(), 2u);
  EXPECT_EQ(rel.get_num_blocks(), 2u);

  rel.join(std::string("a"), std::string("b"));
  EXPECT_TRUE(rel.are_connected(std::string("a"), std::string("b")));
  EXPECT_EQ(rel.get_num_blocks(), 1u);

  EXPECT_FALSE(rel.are_connected(std::string("c"), std::string("a")));
  EXPECT_EQ(rel.size(), 3u);
  EXPECT_EQ(rel.get_num_blocks(), 2u);
}

} // namespace
