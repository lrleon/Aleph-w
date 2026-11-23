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

    The Coverity Scan of 7.8.0 found the pattern one level up: the move of
    a DynSetTree, `noexcept`, builds an empty tree for the set that it moves
    from, so that it stays usable. Its `noexcept` follows that construction
    now, and so do those of a DynMapTree and of an AHMapping, which move
    their set; the tests of the sets and the maps below check it on every
    tree.
*/

# include <cstddef>
# include <memory>
# include <stdexcept>
# include <string>
# include <type_traits>
# include <utility>

# include <gtest/gtest.h>

# include <ah-mapping.H>
# include <htlist.H>
# include <tpl_array.H>
# include <tpl_avl.H>
# include <tpl_avlRk.H>
# include <tpl_binTree.H>
# include <tpl_dynArrayHeap.H>
# include <tpl_dynMapTree.H>
# include <tpl_dynSetTree.H>
# include <tpl_hRbTree.H>
# include <tpl_hRbTreeRk.H>
# include <tpl_odhash.H>
# include <tpl_olhash.H>
# include <tpl_rand_tree.H>
# include <tpl_rbRk.H>
# include <tpl_rb_tree.H>
# include <tpl_splay_tree.H>
# include <tpl_splay_treeRk.H>
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

  // A comparison whose default construction may throw; its copy, its
  // move, its swap and its call cannot
  template <typename Key>
  struct Less_Default_Throws
  {
    Less_Default_Throws() { maybe_fail(); }

    bool operator () (const Key & a, const Key & b) const noexcept { return a < b; }
  };

  // A comparison whose copy may throw; its default construction and its
  // move cannot, and so neither can its swap
  template <typename Key>
  struct Less_Copy_Alone_Throws
  {
    Less_Copy_Alone_Throws() noexcept = default;
    Less_Copy_Alone_Throws(const Less_Copy_Alone_Throws &) { maybe_fail(); }
    Less_Copy_Alone_Throws(Less_Copy_Alone_Throws &&) noexcept = default;

    Less_Copy_Alone_Throws & operator = (const Less_Copy_Alone_Throws &)
    {
      maybe_fail();
      return *this;
    }

    Less_Copy_Alone_Throws & operator = (Less_Copy_Alone_Throws &&) noexcept = default;

    bool operator () (const Key & a, const Key & b) const noexcept { return a < b; }
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
  // keeps header nodes, whose keys it builds; and the sets and the maps
  // on it
  template <template <typename, class> class T, bool Allocates, bool Movable, bool Header = true>
  struct Of
  {
    template <typename Key, class Compare>
    using Tree = T<Key, Compare>;

    template <typename Key, class Compare = Aleph::less<Key>>
    using Set = DynSetTree<Key, T, Compare>;

    template <typename Key, typename Data, class Compare = Aleph::less<Key>>
    using Map = DynMapTree<Key, Data, T, Compare>;

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
    Of<Splay_Tree, false, false>, Of<Splay_Tree_Rk, false, false, false>,
    Of<Treap, true, true>, Of<Treap_Rk, true, true>, Of<Rand_Tree, true, false, false>>;
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
// build the header nodes of a tree potentially throwing, but not its swap.
// A tree without header nodes builds no key
TYPED_TEST(Noexcept_Construction, FollowsTheDefaultConstructionOfTheKey)
{
  using Key = Key_Default_Throws;
  using Tree = typename TypeParam::template Tree<Key, Aleph::less<Key>>;
  EXPECT_EQ(std::is_nothrow_default_constructible_v<Tree>,
            not TypeParam::header and not TypeParam::allocates);
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

// With keys, values and a comparison that cannot throw, a set and a map
// move without throwing, and are built so on a tree that does not allocate.
// The key of the nodes of a map is a std::pair, whose default constructor
// libstdc++ does not declare noexcept: the nodes look into it
TYPED_TEST(Noexcept_Construction, ASetAndAMapAreNoexceptWithTypesThatDoNotThrow)
{
  using Set = typename TypeParam::template Set<int>;
  using Map = typename TypeParam::template Map<int, std::string>;
  using Map_Of_Pairs = typename TypeParam::template Map<std::string, std::pair<int, long>>;
  EXPECT_EQ(std::is_nothrow_default_constructible_v<Set>, not TypeParam::allocates);
  EXPECT_EQ(std::is_nothrow_default_constructible_v<Map>, not TypeParam::allocates);
  // and so is a set built with a comparison, as the copy of that one
  using Set_Whose_Comparison_Throws = typename TypeParam::template Set<int, Less_Copy_Throws>;
  EXPECT_EQ((std::is_nothrow_constructible_v<Set, const Aleph::less<int> &>),
            not TypeParam::allocates);
  EXPECT_FALSE((std::is_nothrow_constructible_v<Set_Whose_Comparison_Throws,
                                                 const Less_Copy_Throws &>));
  EXPECT_TRUE(std::is_nothrow_move_constructible_v<Set>);
  EXPECT_TRUE(std::is_nothrow_move_constructible_v<Map>);
  EXPECT_TRUE(std::is_nothrow_move_constructible_v<Map_Of_Pairs>);
  EXPECT_TRUE(std::is_nothrow_move_assignable_v<Set>);
  EXPECT_TRUE(std::is_nothrow_move_assignable_v<Map>);
}

// The move of a set builds an empty tree for the set that it moves from:
// with a key, or with a value of a map, whose default construction may
// throw, it is not noexcept, unless the tree has no header node. The move
// assignment builds nothing.
// TODO (7.9.0): once the move of a tree builds nothing, the moves of the
// sets and the maps are noexcept with every key: these expectations turn
TYPED_TEST(Noexcept_Construction, TheMoveOfASetFollowsTheDefaultConstructionOfTheKey)
{
  using Key = Key_Default_Throws;
  using Set = typename TypeParam::template Set<Key>;
  using Map_To_It = typename TypeParam::template Map<int, Key>;
  using Map_From_It = typename TypeParam::template Map<Key, int>;
  // a randomized tree is asked for its node, though one has no header
  const bool nothrow = not TypeParam::header and not TypeParam::allocates;
  EXPECT_EQ(std::is_nothrow_move_constructible_v<Set>, nothrow);
  EXPECT_EQ(std::is_nothrow_move_constructible_v<Map_To_It>, nothrow);
  EXPECT_EQ(std::is_nothrow_move_constructible_v<Map_From_It>, nothrow);
  EXPECT_TRUE(std::is_nothrow_move_assignable_v<Set>);
  EXPECT_TRUE(std::is_nothrow_move_assignable_v<Map_To_It>);
}

// The empty tree takes a comparison built by default, a default argument
// of its constructor: the move of a set follows that construction too
TYPED_TEST(Noexcept_Construction, TheMoveOfASetFollowsTheDefaultConstructionOfTheComparison)
{
  using Set = typename TypeParam::template Set<int, Less_Default_Throws<int>>;
  EXPECT_FALSE(std::is_nothrow_default_constructible_v<Set>);
  EXPECT_FALSE(std::is_nothrow_move_constructible_v<Set>);
  EXPECT_TRUE(std::is_nothrow_move_assignable_v<Set>);

  Set set;
  for (int i = 0; i < 5; ++i)
    (void) set.insert(i);
  {
    const Failing_At failing(0);
    EXPECT_THROW(Set taken(std::move(set)), Asked_Failure);
  }
  EXPECT_EQ(set.size(), 5u);
  for (int i = 0; i < 5; ++i)
    EXPECT_NE(set.search(i), nullptr) << i;
}

// The empty tree copies the comparison that it is given: the move of a
// set follows that copy too, on every tree, with a comparison whose swap
// cannot throw. What the copy throws reaches the caller, and the set is
// left as it was
TYPED_TEST(Noexcept_Construction, TheMoveOfASetFollowsTheCopyOfTheComparison)
{
  using Less = Less_Copy_Alone_Throws<int>;
  using Set = typename TypeParam::template Set<int, Less>;
  ASSERT_TRUE(std::is_nothrow_default_constructible_v<Less>);
  ASSERT_TRUE(std::is_nothrow_swappable_v<Less>);
  EXPECT_FALSE(std::is_nothrow_default_constructible_v<Set>);
  EXPECT_FALSE(std::is_nothrow_move_constructible_v<Set>);
  EXPECT_TRUE(std::is_nothrow_move_assignable_v<Set>);

  Set set;
  for (int i = 0; i < 5; ++i)
    (void) set.insert(i);
  {
    const Failing_At failing(0);
    EXPECT_THROW(Set taken(std::move(set)), Asked_Failure);
  }
  EXPECT_EQ(set.size(), 5u);
  for (int i = 0; i < 5; ++i)
    EXPECT_NE(set.search(i), nullptr) << i;
}

// What the default construction of the comparison throws reaches the
// caller when a set or a map is built by default, and when a class that
// inherits the constructors of a set is: GCC and Clang do not count a
// default argument in the noexcept of such an implicit constructor, so the
// set builds its comparison in a default constructor of its own
TYPED_TEST(Noexcept_Construction, ASetBuiltByDefaultLetsTheFailureOfTheComparisonThrough)
{
  using Set = typename TypeParam::template Set<int, Less_Default_Throws<int>>;
  using Map = typename TypeParam::template Map<int, int, Less_Default_Throws<int>>;
  struct Inheriting : Set
  {
    using Set::Set;
  };
  EXPECT_FALSE(std::is_nothrow_default_constructible_v<Inheriting>);
  EXPECT_FALSE(std::is_nothrow_default_constructible_v<Map>);

  const Failing_At failing(0);
  EXPECT_THROW(Set set, Asked_Failure);
  EXPECT_THROW(Inheriting inheriting, Asked_Failure);
  EXPECT_THROW(Map map, Asked_Failure);
}

// What the default construction of the key throws in the move of a set
// reaches the caller, instead of ending the program, and the set is left
// as it was: the empty tree is built before anything is taken
TYPED_TEST(Noexcept_Construction, AMoveOfASetThatThrowsLeavesItAsItWas)
{
  using Key = Key_Default_Throws;
  using Set = typename TypeParam::template Set<Key>;
  Set set;
  for (int i = 0; i < 5; ++i)
    (void) set.insert(Key(i));

  long failures = 0;
  for (bool moved = false; not moved; )
    {
      {
        const Failing_At failing(failures);
        try
          {
            Set taken(std::move(set));
            moved = true;
            EXPECT_EQ(taken.size(), 5u);
            EXPECT_TRUE(set.is_empty());
          }
        catch (const Asked_Failure &)
          {
            ++failures;
          }
      }
      if (not moved)
        {
          ASSERT_EQ(set.size(), 5u);
          for (int i = 0; i < 5; ++i)
            ASSERT_NE(set.search(Key(i)), nullptr) << i;
        }
    }

  if constexpr (TypeParam::header)
    EXPECT_GT(failures, 0);
  else
    EXPECT_EQ(failures, 0);
}

// And so does a map, whose nodes hold a pair of a key and a value
TYPED_TEST(Noexcept_Construction, AMoveOfAMapThatThrowsLeavesItAsItWas)
{
  using Map = typename TypeParam::template Map<int, Key_Default_Throws>;
  Map map;
  for (int i = 0; i < 5; ++i)
    (void) map.insert(i, Key_Default_Throws(10 * i));

  // the search of a map on a splay tree builds a value by default: the
  // map is looked at once the failures are over
  long failures = 0;
  for (bool moved = false; not moved; )
    {
      {
        const Failing_At failing(failures);
        try
          {
            Map taken(std::move(map));
            moved = true;
            EXPECT_EQ(taken.size(), 5u);
            EXPECT_TRUE(map.is_empty());
          }
        catch (const Asked_Failure &)
          {
            ++failures;
          }
      }
      if (not moved)
        {
          ASSERT_EQ(map.size(), 5u);
          for (int i = 0; i < 5; ++i)
            {
              const auto * pair = map.search(i);
              ASSERT_NE(pair, nullptr) << i;
              ASSERT_EQ(pair->second.value, 10 * i);
            }
        }
    }

  if constexpr (TypeParam::header)
    EXPECT_GT(failures, 0);
  else
    EXPECT_EQ(failures, 0);
}

// The node of a tree says whether the default construction of its key may
// throw; for a std::pair, the key of the nodes of a map, by its members
TEST(Noexcept_Construction_Nodes, TheKeyOfAMapIsSeenInItsMembers)
{
  using Key = Key_Default_Throws;
  EXPECT_TRUE(std::is_nothrow_default_constructible_v<AvlNode<int>>);
  EXPECT_FALSE(std::is_nothrow_default_constructible_v<AvlNode<Key>>);
  EXPECT_TRUE((std::is_nothrow_default_constructible_v<AvlNode<std::pair<int, std::string>>>));
  EXPECT_TRUE((std::is_nothrow_default_constructible_v<
                 AvlNode<std::pair<int, std::pair<long, std::string>>>>));
  EXPECT_FALSE((std::is_nothrow_default_constructible_v<AvlNode<std::pair<int, Key>>>));
  EXPECT_FALSE((std::is_nothrow_default_constructible_v<AvlNode<std::pair<Key, int>>>));
  EXPECT_FALSE((std::is_nothrow_default_constructible_v<
                  AvlNode<std::pair<int, std::pair<long, Key>>>>));
  // so does the constructor that takes the control data of the node
  EXPECT_TRUE((std::is_nothrow_constructible_v<AvlNode<std::pair<int, std::string>>,
                                                const AvlNode_Data &>));
  EXPECT_FALSE((std::is_nothrow_constructible_v<AvlNode<std::pair<int, Key>>,
                                                 const AvlNode_Data &>));
  // those of the trees that keep counters, and of the randomized ones
  EXPECT_TRUE((std::is_nothrow_default_constructible_v<BinNodeXt<std::pair<int, int>>>));
  EXPECT_TRUE((std::is_nothrow_default_constructible_v<TreapNode<std::pair<int, int>>>));
  EXPECT_FALSE((std::is_nothrow_default_constructible_v<TreapNode<std::pair<int, Key>>>));
}

// The tree without header nodes builds its comparison in a default
// constructor of its own, as a set does: what that construction throws
// reaches the caller, though the tree inherits its constructors
TEST(Noexcept_Construction_Trees, TheTreeWithoutHeaderNodesLetsTheFailureOfItsComparisonThrough)
{
  using Tree = Splay_Tree_Rk<int, Less_Default_Throws<int>>;
  EXPECT_FALSE(std::is_nothrow_default_constructible_v<Tree>);
  EXPECT_TRUE((std::is_nothrow_constructible_v<Tree, Less_Default_Throws<int>>));

  const Failing_At failing(0);
  EXPECT_THROW(Tree tree, Asked_Failure);
}

// The comparison of the pairs of a map is built as that of its keys, and
// in a default constructor of its own: a class that holds one lets what
// the construction of the comparison of the keys throws through
TEST(Noexcept_Construction_Maps, TheComparisonOfThePairsFollowsThatOfTheKeys)
{
  struct Holding
  {
    Dft_Pair_Cmp<int, std::string, Less_Default_Throws<int>> cmp;
  };
  EXPECT_FALSE(std::is_nothrow_default_constructible_v<Holding>);
  {
    const Failing_At failing(0);
    EXPECT_THROW(Holding holding, Asked_Failure);
  }
  EXPECT_TRUE((std::is_nothrow_constructible_v<
                 Dft_Pair_Cmp<int, std::string, Aleph::less<int>>, Aleph::less<int>>));
  EXPECT_FALSE((std::is_nothrow_constructible_v<
                  Dft_Pair_Cmp<int, std::string, Less_Copy_Throws>, const Less_Copy_Throws &>));
  // it copies the comparison that it takes, though it takes it by a move
  using Less = Less_Copy_Alone_Throws<int>;
  EXPECT_FALSE((std::is_nothrow_constructible_v<Dft_Pair_Cmp<int, std::string, Less>, Less>));
  {
    const Failing_At failing(0);
    EXPECT_THROW((Dft_Pair_Cmp<int, std::string, Less>(Less())), Asked_Failure);
  }

  EXPECT_TRUE((std::is_nothrow_default_constructible_v<
                 Dft_Pair_Cmp<int, std::string, Aleph::less<int>>>));
  EXPECT_FALSE((std::is_nothrow_default_constructible_v<
                  Dft_Pair_Cmp<int, std::string, Less_Default_Throws<int>>>));
  EXPECT_FALSE((std::is_nothrow_default_constructible_v<
                  Dft_Pair_Cmp<int, std::string, Less_Copy_Throws>>));
  EXPECT_FALSE((std::is_nothrow_default_constructible_v<
                  DynMapTree<int, int, Avl_Tree, Less_Default_Throws<int>>>));
  EXPECT_FALSE((std::is_nothrow_move_constructible_v<
                  DynMapTree<int, int, Avl_Tree, Less_Default_Throws<int>>>));
}

// A map moves without throwing when the default construction of its
// values cannot throw: so do the containers that allocate nothing when
// they are built, a list and a set or a map on a tree. With values that
// allocate, as an Array or a set on a randomized tree, whose generator of
// random numbers is in the heap, the move may throw
TEST(Noexcept_Construction_Maps, AMapOfContainersMovesAsTheyAreBuilt)
{
  EXPECT_TRUE(std::is_nothrow_default_constructible_v<DynList<int>>);
  EXPECT_TRUE(std::is_nothrow_default_constructible_v<DynSetTree<int>>);
  EXPECT_TRUE((std::is_nothrow_default_constructible_v<DynMapTree<int, int>>));
  EXPECT_TRUE((std::is_nothrow_move_constructible_v<DynMapTree<int, DynList<int>>>));
  EXPECT_TRUE((std::is_nothrow_move_constructible_v<DynMapTree<int, DynSetTree<int>>>));
  EXPECT_TRUE((std::is_nothrow_move_constructible_v<DynMapTree<int, DynMapTree<int, int>>>));

  EXPECT_FALSE(std::is_nothrow_default_constructible_v<Array<int>>);
  EXPECT_FALSE((std::is_nothrow_default_constructible_v<DynSetTree<int, Treap>>));
  EXPECT_FALSE((std::is_nothrow_move_constructible_v<DynMapTree<int, Array<int>>>));
  EXPECT_FALSE((std::is_nothrow_move_constructible_v<DynMapTree<int, DynSetTree<int, Treap>>>));
}

// A mapping moves its map: its move is noexcept exactly when that one is
TEST(Noexcept_Construction_Maps, AMappingMovesAsItsMap)
{
  using Key = Key_Default_Throws;
  EXPECT_TRUE((std::is_nothrow_move_constructible_v<AHMapping<int, std::string>>));
  EXPECT_FALSE((std::is_nothrow_move_constructible_v<AHMapping<int, Key>>));
  EXPECT_TRUE((std::is_nothrow_move_assignable_v<AHMapping<int, Key>>));

  AHMapping<int, Key> mapping;
  for (int i = 0; i < 5; ++i)
    mapping.insert(i, Key(10 * i));
  {
    const Failing_At failing(0);
    EXPECT_THROW((AHMapping<int, Key>(std::move(mapping))), Asked_Failure);
  }
  EXPECT_EQ(mapping.size(), 5u);
  for (int i = 0; i < 5; ++i)
    EXPECT_EQ(std::as_const(mapping)[i].value, 10 * i);
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
