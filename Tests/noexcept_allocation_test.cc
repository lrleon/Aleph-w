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
 * @file noexcept_allocation_test.cc
 * @brief What a container declares noexcept does not allocate
 */

// A function that is noexcept and allocates ends the program when there is
// no memory, where it should throw std::bad_alloc. The moves of several
// containers did: they built something for the container that they moved
// from, as the lists of a hash table of its capacity, the array of a
// FixedQueue of the default one, or a header node, whose item is built by
// default; and the hash functions of the maps were a std::bind, which a
// std::function keeps in the heap and copies there.
//
// These tests build, move, move assign and swap each container, with int
// and with an item whose default construction allocates, and count the
// allocations of what is noexcept: there must be none. And the container
// that was moved from must take items again, unless it is one of a fixed
// capacity, which a move leaves without array.
//
// The allocations are those of operator new, which allocation_failure.H
// replaces. Those of malloc() are not seen: the generator of random
// numbers of Treap, Treap_Rk and Rand_Tree, which a move of a DynSetTree
// on them makes for the set that it moves from, and the directory of the
// DynArray of a linear hash table, which a move of a DynSetLinHash makes
// for its new table.
// TODO (7.9.0): count them too, once those moves do not allocate.

#include <gtest/gtest.h>

#include "allocation_failure.H"

#include <ah-mapping.H>
#include <bitArray.H>
#include <htlist.H>
#include <tpl_array.H>
#include <tpl_arrayHeap.H>
#include <tpl_arrayQueue.H>
#include <tpl_arrayStack.H>
#include <tpl_avl.H>
#include <tpl_avlRk.H>
#include <tpl_binTree.H>
#include <tpl_dynArray.H>
#include <tpl_dynArrayHeap.H>
#include <tpl_dynBinHeap.H>
#include <tpl_dynDlist.H>
#include <tpl_dynLhash.H>
#include <tpl_dynListQueue.H>
#include <tpl_dynListStack.H>
#include <tpl_dynMapOhash.H>
#include <tpl_dynMapTree.H>
#include <tpl_dynMat.H>
#include <tpl_dynSetHash.H>
#include <tpl_dynSetTree.H>
#include <tpl_dynSkipList.H>
#include <tpl_flat_map.H>
#include <tpl_flat_set.H>
#include <tpl_graph.H>
#include <tpl_hRbTree.H>
#include <tpl_hRbTreeRk.H>
#include <tpl_hash.H>
#include <tpl_memArray.H>
#include <tpl_multi_polynomial.H>
#include <tpl_odhash.H>
#include <tpl_olhash.H>
#include <tpl_rand_tree.H>
#include <tpl_random_queue.H>
#include <tpl_rb_tree.H>
#include <tpl_rbRk.H>
#include <tpl_small_vector.H>
#include <tpl_splay_tree.H>
#include <tpl_splay_treeRk.H>
#include <tpl_tdRbTree.H>
#include <tpl_tdRbTreeRk.H>
#include <tpl_treap.H>
#include <tpl_treapRk.H>

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

using namespace Aleph;

namespace
{
  // An item whose default construction allocates, as that of an Array
  // does: it keeps its number in the heap. Its move takes the number, so
  // it does not allocate, and leaves the other item without one
  struct Allocating
  {
    std::unique_ptr<int> number;

    Allocating() : number(new int(-1)) {}

    Allocating(const int i) : number(new int(i)) {}

    Allocating(const Allocating & o) : number(new int(o.value())) {}

    Allocating(Allocating &&) noexcept = default;

    Allocating & operator = (const Allocating & o)
    {
      number.reset(new int(o.value()));
      return *this;
    }

    Allocating & operator = (Allocating &&) noexcept = default;

    int value() const noexcept { return number == nullptr ? -2 : *number; }

    bool operator < (const Allocating & o) const noexcept { return value() < o.value(); }

    bool operator == (const Allocating & o) const noexcept { return value() == o.value(); }
  };

  // The hash of the tables of Aleph, which they find by the namespace
  size_t aleph_hash_value(const Allocating & a) noexcept
  {
    return static_cast<size_t>(a.value()) * 40503u;
  }

  // Another hash function of the keys of a map
  size_t another_hash(const int & k) noexcept
  {
    return static_cast<size_t>(k) * 40503u + 7;
  }

  // What a check leaves out, where the code gives the reason
  struct Leaving_Out
  {
    bool move = false;            // the move construction allocates
    bool use_after_move = false;  // a fixed capacity: no array after a move
  };

  constexpr Leaving_Out nothing{};
  constexpr Leaving_Out its_move{true, false};
  constexpr Leaving_Out its_use_after_a_move{false, true};

  template <class C>
  constexpr bool nothrow_swap =
    requires (C & a, C & b) { { a.swap(b) } noexcept; };

  // Builds, moves, move assigns and swaps containers of type C, which
  // fill(c, from) fills with three items, from the number `from`; what is
  // noexcept among them must not allocate. Then the container that the
  // move left takes items again
  template <class C, class Fill>
  void expect_that_what_is_noexcept_does_not_allocate(const char * name, Fill fill,
                                                      const Leaving_Out out = nothing)
  {
    SCOPED_TRACE(name);

    if (std::is_nothrow_default_constructible_v<C>)
      {
        std::optional<C> built;
        const size_t before = allocations;
        built.emplace();
        EXPECT_EQ(allocations - before, 0u) << "its default construction";
      }

    C source;
    fill(source, 0);
    if (std::is_nothrow_move_constructible_v<C> and not out.move)
      {
        std::optional<C> target;
        const size_t before = allocations;
        target.emplace(std::move(source));
        EXPECT_EQ(allocations - before, 0u) << "its move construction";
        if (not out.use_after_move)
          {
            fill(source, 10); // the moved-from one takes items
            fill(*target, 20); // and so does the one that took them
          }
      }

    if (std::is_nothrow_move_assignable_v<C>)
      {
        C from, to;
        fill(from, 0);
        fill(to, 30);
        const size_t before = allocations;
        to = std::move(from);
        EXPECT_EQ(allocations - before, 0u) << "its move assignment";
        if (not out.use_after_move)
          {
            fill(from, 40); // the one that was moved from takes items
            fill(to, 50);   // and so does the one that took them
          }
      }

    if constexpr (nothrow_swap<C>)
      {
        C a, b;
        fill(a, 0);
        const size_t before = allocations;
        a.swap(b);
        EXPECT_EQ(allocations - before, 0u) << "its swap";
      }
  }

  // The ways of filling a container with the items 0, 1 and 2 from `from`
  template <class T>
  struct With
  {
    static T item(const int i) { return T(i); }

    static constexpr auto append = [] (auto & c, const int from)
      {
        for (int i = from; i < from + 3; ++i)
          (void) c.append(item(i));
      };

    static constexpr auto insert = [] (auto & c, const int from)
      {
        for (int i = from; i < from + 3; ++i)
          (void) c.insert(item(i));
      };

    static constexpr auto push = [] (auto & c, const int from)
      {
        for (int i = from; i < from + 3; ++i)
          (void) c.push(item(i));
      };

    static constexpr auto put = [] (auto & c, const int from)
      {
        for (int i = from; i < from + 3; ++i)
          (void) c.put(item(i));
      };

    static constexpr auto push_back = [] (auto & c, const int from)
      {
        for (int i = from; i < from + 3; ++i)
          c.push_back(item(i));
      };

    // a map from int to T
    static constexpr auto insert_value = [] (auto & c, const int from)
      {
        for (int i = from; i < from + 3; ++i)
          (void) c.insert(i, item(i));
      };

    // a map from T to int
    static constexpr auto insert_key = [] (auto & c, const int from)
      {
        for (int i = from; i < from + 3; ++i)
          (void) c.insert(item(i), i);
      };

    static constexpr auto insert_node = [] (auto & g, const int from)
      {
        for (int i = from; i < from + 3; ++i)
          (void) g.insert_node(item(i));
      };

    static constexpr auto write = [] (auto & m, const int from)
      {
        m.set_dimension(2, 2);
        m.write(0, 1, item(from));
      };
  };

  // The lists, the arrays, the stacks, the queues and the heaps of T
  template <class T>
  void check_the_sequences(const std::string & of)
  {
    auto name = [&of] (const char * container) { return container + ("<" + of + ">"); };
    const auto w = With<T>();
    // two macros, and not one with __VA_OPT__, which the preprocessor of
    // MSVC does not take
# define CHECK(Container, how)                                               \
    expect_that_what_is_noexcept_does_not_allocate<Container<T>>             \
      (name(#Container).c_str(), w.how)
# define CHECK_FIXED(Container, how)                                         \
    expect_that_what_is_noexcept_does_not_allocate<Container<T>>             \
      (name(#Container).c_str(), w.how, its_use_after_a_move)

    CHECK(DynList, append);
    CHECK(DynDlist, append);
    CHECK(DynListStack, push);
    CHECK(DynListQueue, put);
    CHECK(ArrayStack, push);
    CHECK(ArrayQueue, put);
    CHECK_FIXED(FixedStack, push);
    CHECK_FIXED(FixedQueue, put);
    CHECK(DynArray, append);
    CHECK(Array, append);
    CHECK(MemArray, append);
    CHECK(DynArrayHeap, insert);
    CHECK_FIXED(ArrayHeap, insert);
    CHECK(DynBinHeap, insert);
    CHECK(DynSkipList, insert);
    CHECK(FlatSet, insert);
    CHECK(Random_Set, put);
    CHECK(DynMatrix, write);
# undef CHECK
# undef CHECK_FIXED
    expect_that_what_is_noexcept_does_not_allocate<SmallVector<T, 4>>
      (name("SmallVector").c_str(), w.push_back);
    expect_that_what_is_noexcept_does_not_allocate<FlatMap<int, T>>
      (name("FlatMap<int, ").c_str(), w.insert_value);
  }

  // The sets and the maps of T on each tree, and the mapping, which keeps
  // a map. The move of a set builds a tree for the set that it moves
  // from, whose header nodes hold a key built by default: with a key whose
  // default construction allocates it allocates, and so it is not
  // noexcept, on every tree with such nodes. Without that condition in the
  // noexcept of the move, these checks fail
  template <class T>
  void check_the_trees(const std::string & of)
  {
    auto name = [&of] (const char * tree) { return "DynSetTree<" + of + ", " + tree + ">"; };
    const auto w = With<T>();
# define CHECK(Tree)                                                         \
    expect_that_what_is_noexcept_does_not_allocate<DynSetTree<T, Tree>>      \
      (name(#Tree).c_str(), w.insert);                                       \
    expect_that_what_is_noexcept_does_not_allocate<DynMapTree<int, T, Tree>> \
      (("DynMapTree<int, " + of + ", " #Tree ">").c_str(), w.insert_value)

    CHECK(BinTree);
    CHECK(Avl_Tree);
    CHECK(Avl_Tree_Rk);
    CHECK(Rb_Tree);
    CHECK(Rb_Tree_Rk);
    CHECK(HtdRbTree);
    CHECK(HtdRbTreeRk);
    CHECK(TdRbTree);
    CHECK(TdRbTreeRk);
    CHECK(Splay_Tree);
    CHECK(Splay_Tree_Rk);
    CHECK(Treap);
    CHECK(Treap_Rk);
    CHECK(Rand_Tree);
# undef CHECK
    expect_that_what_is_noexcept_does_not_allocate<DynMapTree<T, int>>
      (("DynMapTree<" + of + ", int>").c_str(), w.insert_key);
    expect_that_what_is_noexcept_does_not_allocate<AHMapping<int, T>>
      (("AHMapping<int, " + of + ">").c_str(), w.insert_value);
  }

  // The sets and the maps of T on each hash table. The move of a linear
  // one builds a table for the one that it moves from, whose DynArray
  // holds a default value: a list header, with a key built by default.
  // TODO (7.9.0): a DynArray that takes its directory and its default
  // value when it first needs them. Then `allocating_key` goes
  template <class T>
  void check_the_hash_tables(const std::string & of, const bool allocating_key)
  {
    const auto w = With<T>();
    const Leaving_Out linear = allocating_key ? its_move : nothing;
# define CHECK_SET(Set, out)                                                 \
    expect_that_what_is_noexcept_does_not_allocate<Set<T>>                   \
      ((std::string(#Set "<") + of + ">").c_str(), w.insert, out)
# define CHECK_MAP(Map, out)                                                 \
    expect_that_what_is_noexcept_does_not_allocate<Map<int, T>>              \
      ((std::string(#Map "<int, ") + of + ">").c_str(), w.insert_value, out);\
    expect_that_what_is_noexcept_does_not_allocate<Map<T, int>>              \
      ((std::string(#Map "<") + of + ", int>").c_str(), w.insert_key, out)

    CHECK_SET(DynSetLhash, nothing);
    CHECK_SET(DynSetHash, nothing);
    CHECK_SET(DynSetLinHash, linear);
    CHECK_SET(SetODhash, nothing);
    CHECK_SET(SetOLhash, nothing);
    CHECK_SET(HashSet, nothing);
    CHECK_MAP(DynMapHash, nothing);
    CHECK_MAP(DynMapLinHash, linear);
    CHECK_MAP(MapODhash, nothing);
    CHECK_MAP(MapOLhash, nothing);
    CHECK_MAP(HashMap, nothing);
    CHECK_MAP(DynLhashTable, nothing);
# undef CHECK_SET
# undef CHECK_MAP
  }
}

// The tests need the operator new of allocation_failure.H
struct NoexceptAllocation : public ::testing::Test
{
  void SetUp() override
  {
    if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
      GTEST_SKIP() << "ThreadSanitizer provides the allocation functions";
  }
};

TEST_F(NoexceptAllocation, TheItemAllocatesWhenItIsBuiltByDefaultAndNotWhenItMoves)
{
  ASSERT_FALSE(std::is_nothrow_default_constructible_v<Allocating>);
  ASSERT_TRUE(std::is_nothrow_move_constructible_v<Allocating>);

  size_t before = allocations;
  Allocating a;
  EXPECT_EQ(allocations - before, 1u);

  before = allocations;
  const Allocating b(std::move(a));
  EXPECT_EQ(allocations - before, 0u);
  EXPECT_EQ(b.value(), -1);
}

TEST_F(NoexceptAllocation, Sequences)
{
  check_the_sequences<int>("int");
  check_the_sequences<Allocating>("Allocating");
  expect_that_what_is_noexcept_does_not_allocate<BitArray>
    ("BitArray", [] (BitArray & b, const int) { b.push(1); b.push(0); });
}

TEST_F(NoexceptAllocation, SetsAndMapsOnTrees)
{
  check_the_trees<int>("int");
  check_the_trees<Allocating>("Allocating");

  // the keys of the map of a polynomial of several variables are arrays,
  // whose default construction allocates
  expect_that_what_is_noexcept_does_not_allocate<MultiPolynomial>
    ("MultiPolynomial", [] (MultiPolynomial & p, const int from)
     {
       p = MultiPolynomial(2, 1.0 + from);
     });
}

// The keys and the values that the checks above take are what they must
// be: the move of a set of int is noexcept, and so it is checked, and the
// one of a set of the item that allocates is not, on a tree with header
// nodes, and it is noexcept on the one without them
TEST_F(NoexceptAllocation, TheMovesOfTheSetsOnTreesThatAreChecked)
{
  EXPECT_TRUE(std::is_nothrow_move_constructible_v<DynSetTree<int>>);
  EXPECT_TRUE((std::is_nothrow_move_constructible_v<DynMapTree<int, int>>));
  EXPECT_TRUE((std::is_nothrow_move_constructible_v<AHMapping<int, int>>));
  EXPECT_FALSE(std::is_nothrow_move_constructible_v<DynSetTree<Allocating>>);
  EXPECT_FALSE((std::is_nothrow_move_constructible_v<DynMapTree<int, Allocating>>));
  EXPECT_FALSE((std::is_nothrow_move_constructible_v<DynMapTree<Allocating, int>>));
  EXPECT_FALSE((std::is_nothrow_move_constructible_v<AHMapping<int, Allocating>>));
  EXPECT_FALSE(std::is_nothrow_move_constructible_v<MultiPolynomial>);
  EXPECT_TRUE((std::is_nothrow_move_constructible_v<DynSetTree<Allocating, Splay_Tree_Rk>>));
}

TEST_F(NoexceptAllocation, SetsAndMapsOnHashTables)
{
  check_the_hash_tables<int>("int", false);
  check_the_hash_tables<Allocating>("Allocating", true);
}

// A map that took another hash function of the keys keeps it as the one
// that it was built with: in the buffer of the std::function of its table
TEST_F(NoexceptAllocation, AMapThatTookAnotherHashFunctionMovesWithoutAllocating)
{
  const auto fill = [] (auto & m, const int from)
    {
      m.set_hash_fct(another_hash);
      for (int i = from; i < from + 3; ++i)
        (void) m.insert(i, i);
    };
  expect_that_what_is_noexcept_does_not_allocate<DynMapHash<int, int>>
    ("DynMapHash<int, int> after set_hash_fct()", fill);
}

namespace
{
  // A hash function with 128 bytes of state: it does not fit in the buffer
  // of a std::function, which keeps it in the heap and allocates to copy it
  struct Large_Hash
  {
    std::array<size_t, 16> salt{};

    size_t operator () (const int & k) const noexcept
    {
      return static_cast<size_t>(k) * 40503u + salt[0];
    }
  };
}

// The move assignment of a table of chained lists copies no hash function:
// the two tables swap them. So it allocates nothing even with a hash
// function whose copy allocates. The move constructor copies it, so that
// the table that it moves from keeps it: with such a function it
// allocates, in a constructor that is noexcept
TEST_F(NoexceptAllocation, AMoveAssignmentOfATableOfChainedListsCopiesNoHashFunction)
{
  using Fct = std::function<size_t(const int &)>;
  {
    const Fct large{Large_Hash()};
    const size_t before = allocations;
    const Fct copy(large);
    ASSERT_GT(allocations - before, 0u) << "the hash function of this test must not fit";
  }

  LhashTable<int> from(101), to(101);
  from.set_hash_fct(Fct(Large_Hash()));
  to.set_hash_fct(Fct(Large_Hash()));
  for (int i = 0; i < 3; ++i)
    ASSERT_NE(from.insert(new LhashBucket<int>(i)), nullptr);
  size_t before = allocations;
  to = std::move(from);
  EXPECT_EQ(allocations - before, 0u) << "the move assignment of LhashTable";
  EXPECT_NE(to.search(1), nullptr);
  ASSERT_NE(from.insert(new LhashBucket<int>(9)), nullptr); // it goes on
  EXPECT_NE(from.search(9), nullptr);

  DynSetLhash<int> set_from, set_to;
  set_from.set_hash_fct(Fct(Large_Hash()));
  set_to.set_hash_fct(Fct(Large_Hash()));
  for (int i = 0; i < 3; ++i)
    ASSERT_NE(set_from.insert(i), nullptr);
  before = allocations;
  set_to = std::move(set_from);
  EXPECT_EQ(allocations - before, 0u) << "the move assignment of DynSetLhash";
  EXPECT_TRUE(set_to.contains(1));
  ASSERT_NE(set_from.insert(9), nullptr);
  EXPECT_TRUE(set_from.contains(9));
}

TEST_F(NoexceptAllocation, Graphs)
{
  using Node = Graph_Node<Allocating>;
  using Arc = Graph_Arc<int>;
  const auto w = With<Allocating>();
  expect_that_what_is_noexcept_does_not_allocate<List_Graph<Node, Arc>>("List_Graph", w.insert_node);
  expect_that_what_is_noexcept_does_not_allocate<List_Digraph<Node, Arc>>("List_Digraph", w.insert_node);
}
