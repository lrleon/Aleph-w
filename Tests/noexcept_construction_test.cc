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

/** @file noexcept_construction_test.cc
    @brief The `noexcept` of the constructions, moves and swaps that build,
    copy, move or swap a type of the user follows that type.

    The Coverity Scan of 7.7.3 found constructors of the trees, of the
    buckets of the open hash tables and the sifts of DynArrayHeap declared
    `noexcept` without a condition, though they build the key of a header
    node, copy the comparison or call it: an exception of the user there
    ended the program (`std::terminate()`). This test checks the pattern on
    every tree, so that a new tree, or a constructor added to one, that
    repeats it fails here: with keys and comparisons that cannot throw, the
    operations are `noexcept`; with a key whose default construction may
    throw, or a comparison whose copy, move or swap may throw, they are not,
    and what the user type throws reaches the caller.

    The splay trees keep their operations `noexcept` on purpose (see
    tpl_splay_tree.H); their constructors follow the pattern.
*/

# include <cstddef>
# include <memory>
# include <stdexcept>
# include <type_traits>
# include <utility>

# include <gtest/gtest.h>

# include <tpl_avl.H>
# include <tpl_avlRk.H>
# include <tpl_binTree.H>
# include <tpl_dynArrayHeap.H>
# include <tpl_hRbTree.H>
# include <tpl_hRbTreeRk.H>
# include <tpl_odhash.H>
# include <tpl_olhash.H>
# include <tpl_rand_tree.H>
# include <tpl_rbRk.H>
# include <tpl_rb_tree.H>
# include <tpl_splay_tree.H>
# include <tpl_tdRbTree.H>
# include <tpl_tdRbTreeRk.H>
# include <tpl_treap.H>
# include <tpl_treapRk.H>

using namespace Aleph;

namespace
{
  // What a type of the user throws when the test asks it to
  struct Asked_Failure : std::runtime_error
  {
    Asked_Failure() : std::runtime_error("an operation of the user failed") {}
  };

  // How many of the operations below that may throw still succeed before
  // one throws; negative, none throws
  long countdown = -1;

  void maybe_fail()
  {
    if (countdown == 0)
      throw Asked_Failure();
    if (countdown > 0)
      --countdown;
  }

  // Makes the operation k of the types below throw, for the life of the
  // object, and none after
  struct Failing_At
  {
    explicit Failing_At(const long k) noexcept { countdown = k; }
    ~Failing_At() { countdown = -1; }
    Failing_At(const Failing_At &) = delete;
    Failing_At & operator = (const Failing_At &) = delete;
  };

  // Builds with f, with each operation of the user failing in turn, until
  // one succeeds: f must throw Asked_Failure or succeed, never end the
  // program. Returns how many failures reached the caller
  template <class F>
  long failures_through(F && f)
  {
    for (long k = 0; ; ++k)
      {
        const Failing_At failing(k);
        try
          {
            f();
            return k;
          }
        catch (const Asked_Failure &)
          {
            // the next operation fails now
          }
      }
  }

  // A key whose default construction may throw; the rest cannot
  struct Key_Default_Throws
  {
    int value = 0;

    Key_Default_Throws() { maybe_fail(); }
    Key_Default_Throws(const int v) noexcept : value(v) {}

    friend bool operator < (const Key_Default_Throws & a, const Key_Default_Throws & b) noexcept
    {
      return a.value < b.value;
    }

    friend bool operator == (const Key_Default_Throws & a, const Key_Default_Throws & b) noexcept
    {
      return a.value == b.value;
    }
  };

  // A comparison whose construction, copy, move and assignments may throw,
  // and so its swap; its call cannot
  struct Less_Copy_Throws
  {
    Less_Copy_Throws() { maybe_fail(); }
    Less_Copy_Throws(const Less_Copy_Throws &) { maybe_fail(); }
    Less_Copy_Throws(Less_Copy_Throws &&) { maybe_fail(); }

    Less_Copy_Throws & operator = (const Less_Copy_Throws &)
    {
      maybe_fail();
      return *this;
    }

    Less_Copy_Throws & operator = (Less_Copy_Throws &&)
    {
      maybe_fail();
      return *this;
    }

    bool operator () (const int a, const int b) const noexcept { return a < b; }
  };

  // A comparison whose call may throw
  struct Less_Call_Throws
  {
    bool operator () (const int a, const int b) const
    {
      maybe_fail();
      return a < b;
    }
  };

  // A tree template, whether its default construction allocates (and so
  // is never noexcept), whether it has a move constructor, and whether it
  // keeps header nodes, whose keys it builds
  template <template <typename, class> class T, bool Allocates, bool Movable, bool Header = true>
  struct Of
  {
    template <typename Key, class Compare>
    using Tree = T<Key, Compare>;

    static constexpr bool allocates = Allocates;
    static constexpr bool movable = Movable;
    static constexpr bool header = Header;
  };

  template <class Kind>
  class Noexcept_Construction : public ::testing::Test {};

  using Tree_Kinds = ::testing::Types<
    Of<BinTree, false, false>, Of<Avl_Tree, false, false>, Of<Avl_Tree_Rk, false, false>,
    Of<Rb_Tree, false, true>, Of<Rb_Tree_Rk, false, true>, Of<HtdRbTree, false, true>,
    Of<HtdRbTreeRk, false, true>, Of<TdRbTree, false, true>, Of<TdRbTreeRk, false, true>,
    Of<Splay_Tree, false, false>, Of<Treap, true, true>, Of<Treap_Rk, true, true>,
    Of<Rand_Tree, true, false, false>>;
  TYPED_TEST_SUITE(Noexcept_Construction, Tree_Kinds);

  template <class Tree>
  constexpr bool nothrow_swap = noexcept(std::declval<Tree &>().swap(std::declval<Tree &>()));
} // namespace

// With a key and a comparison that cannot throw, a tree that does not
// allocate is built, moved and swapped without throwing
TYPED_TEST(Noexcept_Construction, IsNoexceptWithTypesThatDoNotThrow)
{
  using Tree = typename TypeParam::template Tree<int, Aleph::less<int>>;
  EXPECT_EQ(std::is_nothrow_default_constructible_v<Tree>, not TypeParam::allocates);
  EXPECT_TRUE(nothrow_swap<Tree>);
  if constexpr (TypeParam::movable)
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<Tree>);
}

// A key whose default construction may throw makes the constructions that
// build the header nodes of a tree potentially throwing, but not its swap
TYPED_TEST(Noexcept_Construction, FollowsTheDefaultConstructionOfTheKey)
{
  using Key = Key_Default_Throws;
  using Tree = typename TypeParam::template Tree<Key, Aleph::less<Key>>;
  EXPECT_FALSE(std::is_nothrow_default_constructible_v<Tree>);
  EXPECT_TRUE(nothrow_swap<Tree>);
  if constexpr (TypeParam::movable)
    EXPECT_FALSE(std::is_nothrow_move_constructible_v<Tree>);
}

// A comparison whose copy, move or swap may throw makes the constructions,
// the moves and the swap of a tree potentially throwing
TYPED_TEST(Noexcept_Construction, FollowsTheCopyMoveAndSwapOfTheComparison)
{
  using Tree = typename TypeParam::template Tree<int, Less_Copy_Throws>;
  EXPECT_FALSE(std::is_nothrow_default_constructible_v<Tree>);
  EXPECT_FALSE(nothrow_swap<Tree>);
  if constexpr (TypeParam::movable)
    EXPECT_FALSE(std::is_nothrow_move_constructible_v<Tree>);
}

// What the default construction of the key throws reaches the caller,
// instead of ending the program
TYPED_TEST(Noexcept_Construction, LetsTheFailureOfTheKeyThrough)
{
  using Key = Key_Default_Throws;
  using Tree = typename TypeParam::template Tree<Key, Aleph::less<Key>>;
  const long failures = failures_through([] { Tree tree; (void) tree; });
  if constexpr (TypeParam::header)
    EXPECT_GT(failures, 0);
  else
    EXPECT_EQ(failures, 0);
}

// What the copy of the comparison throws reaches the caller
TYPED_TEST(Noexcept_Construction, LetsTheFailureOfTheComparisonThrough)
{
  using Tree = typename TypeParam::template Tree<int, Less_Copy_Throws>;
  const Less_Copy_Throws cmp;
  EXPECT_GT(failures_through([&cmp] { Tree tree(cmp); (void) tree; }), 0);
}

// The buckets of the open hash tables build their key
TEST(Noexcept_Construction_Hash, TheBucketsFollowTheDefaultConstructionOfTheKey)
{
  EXPECT_TRUE(std::is_nothrow_default_constructible_v<OLhashTable<int>::Bucket>);
  EXPECT_TRUE(std::is_nothrow_default_constructible_v<ODhashTable<int>::Bucket>);
  EXPECT_FALSE(std::is_nothrow_default_constructible_v<OLhashTable<Key_Default_Throws>::Bucket>);
  EXPECT_FALSE(std::is_nothrow_default_constructible_v<ODhashTable<Key_Default_Throws>::Bucket>);

  const Failing_At failing(0);
  using OL_Bucket = OLhashTable<Key_Default_Throws>::Bucket;
  using OD_Bucket = ODhashTable<Key_Default_Throws>::Bucket;
  EXPECT_THROW((void) std::make_unique<OL_Bucket[]>(3), Asked_Failure);
  EXPECT_THROW((void) std::make_unique<OD_Bucket[]>(3), Asked_Failure);
}

// The sifts of DynArrayHeap call its comparison
TEST(Noexcept_Construction_Heap, TheSiftsFollowTheComparison)
{
  DynArray<int> a;
  Aleph::less<int> nothrow_less;
  Less_Call_Throws throwing_less;
  EXPECT_TRUE(noexcept(sift_up(a, 1, 1, nothrow_less)));
  EXPECT_TRUE(noexcept(sift_down(a, 1, 1, nothrow_less)));
  EXPECT_FALSE(noexcept(sift_up(a, 1, 1, throwing_less)));
  EXPECT_FALSE(noexcept(sift_down(a, 1, 1, throwing_less)));

  DynArrayHeap<int, Less_Call_Throws> heap;
  for (int i = 10; i > 0; --i)
    heap.insert(i);
  const Failing_At failing(0);
  EXPECT_THROW(heap.insert(0), Asked_Failure);
  EXPECT_THROW((void) heap.getMin(), Asked_Failure);
}
