/*
                          Aleph_w

                           / \  | | ___ _ __ | |__      __      __
                          / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / /
                         / ___ \| |  __/ |_) | | | |_____\ V  V /
                        /_/   \_\_|\___| .__/|_| |_|      \_/\_/
                                       |_|

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
 * @file throwing_compare_test.cc
 * @brief The search trees let through what their comparison throws, and
 *        keep their keys.
 *
 * The comparison of a tree is code of the user, and it may throw. The
 * operations of the trees that compare are `noexcept` only if it cannot:
 * a `noexcept` function that lets an exception out ends the program
 * (`std::terminate()`), where the user expects the exception.
 *
 * Here the comparison throws at its k-th call, for every k up to the
 * number of calls that an operation makes. Every search tree of the
 * library but the splay trees is tested alone, in a DynSetTree and in a
 * DynMapTree: each insertion, search, removal and split must let the
 * exception through and leave a tree that verifies, with the keys and the
 * size that it had. The trees that do not recolor or rotate on their way
 * down are left exactly as they were. The operations of BinTree_Operation
 * and BinTreeXt_Operation are tested in the same way.
 *
 * The splay trees keep their `noexcept`: a splay relinks the tree as it
 * compares, so their comparison must not throw.
 */

#include "search_trees_test_common.H"

#include <gtest/gtest-spi.h>

#include <climits>
#include <stdexcept>
#include <string>
#include <utility>

#include <ahFunction.H>
#include <tpl_array.H>
#include <tpl_binNodeXt.H>
#include <tpl_binTreeOps.H>
#include <tpl_dynMapTree.H>
#include <tpl_dynSetTree.H>

using namespace Aleph;
using namespace search_trees_test;

namespace
{
  struct Comparison_Error {};

  // Orders as less<int> does. Armed, it throws at its call number fail_at
  // (from 1); it counts in static members because the trees copy it.
  struct Countdown_Less
  {
    static inline long calls = 0;
    static inline long fail_at = 0;  // 0: it does not throw

    bool operator ()(const int a, const int b) const
    {
      if (fail_at != 0 and ++calls == fail_at)
        throw Comparison_Error();
      return a < b;
    }
  };

  // While one lives, Countdown_Less throws at its call k.
  struct Failing_At
  {
    explicit Failing_At(const long k) noexcept
    {
      Countdown_Less::calls = 0;
      Countdown_Less::fail_at = k;
    }

    ~Failing_At() { Countdown_Less::fail_at = 0; }
  };

  // More calls than any operation on these trees makes: past it, the
  // operation is taken as one that does not end.
  constexpr long Max_Calls = 1000;

  // The trees start with the keys 0, 2, ..., 2 (Num_Keys - 1); the odd
  // keys, and -1, are not in them.
  constexpr int Num_Keys = 24;

  // The order in which the keys are inserted: scrambled (7 is prime with
  // Num_Keys), so that BinTree does not degenerate into a list.
  Array<int> insertion_order()
  {
    Array<int> keys;
    for (int i = 0; i < Num_Keys; ++i)
      keys.append(2 * (i * 7 % Num_Keys));
    return keys;
  }

  // The initial keys, sorted, plus `added` and less `removed` when they
  // are not INT_MIN.
  Array<int> keys_with(const int added = INT_MIN, const int removed = INT_MIN)
  {
    Array<int> keys;
    bool pending = added != INT_MIN;  // `added`, after the keys not greater
    for (int k = 0; k < 2 * Num_Keys; k += 2)
      {
        if (pending and added < k)
          {
            keys.append(added);
            pending = false;
          }
        if (k != removed)
          keys.append(k);
      }
    if (pending)
      keys.append(added);
    return keys;
  }

  bool is_present(const int key) noexcept
  {
    return key >= 0 and key < 2 * Num_Keys and key % 2 == 0;
  }

  // The position that `key` has among the initial keys, if it is one.
  long rank_of(const int key) noexcept { return key / 2; }

  // The keys at which the operations are tried: every key of the tree,
  // and every gap around them.
  Array<int> target_keys()
  {
    Array<int> keys;
    for (int k = -1; k < 2 * Num_Keys; ++k)
      keys.append(k);
    return keys;
  }

  template <class Node>
  void collect_keys(Node * p, Array<int> & keys)
  {
    if (p == Node::NullPtr)
      return;
    collect_keys(LLINK(p), keys);
    keys.append(KEY(p));
    collect_keys(RLINK(p), keys);
  }

  // The keys of the tree of root `root`, in order.
  template <class Node>
  Array<int> keys_of(Node * root)
  {
    Array<int> keys;
    collect_keys(root, keys);
    return keys;
  }

  template <class Node>
  void collect_state(Node * p, Array<long> & state)
  {
    if (p == Node::NullPtr)
      {
        state.append(LONG_MIN);
        return;
      }
    state.append(KEY(p));
    if constexpr (Colored_Node<Node>)
      state.append(p->getColor());
    if constexpr (Avl_Balanced_Node<Node>)
      state.append(p->getDiff());
    if constexpr (Counted_Node<Node>)
      state.append(static_cast<long>(p->getCount()));
    if constexpr (Prioritized_Node<Node>)
      state.append(static_cast<long>(p->getPriority()));
    collect_state(LLINK(p), state);
    collect_state(RLINK(p), state);
  }

  // The shape of the tree of root `root` and the data of its nodes, in
  // preorder: equal states are the same tree.
  template <class Node>
  Array<long> state_of(Node * root)
  {
    Array<long> state;
    collect_state(root, state);
    return state;
  }

  // How the runs of the operations of a test that threw left their trees.
  struct Failures
  {
    long thrown = 0;        // runs that threw
    long restructured = 0;  // of them, those that left a different tree
  };

  /* Runs `op` on a fresh `Fixture` with the comparison failing at its call
     k, for k = 1, 2, ... until op makes all its calls, and counts in
     `failures` the runs that threw. After each one, the fixture must keep
     its initial keys (`expect_initial()`); after the run that does not
     throw, `done` checks its result. An exception other than the one of
     the comparison fails the test, and so does a run that does not throw
     after the comparison threw: op caught the exception. */
  template <class Fixture, class Op, class Done>
  void fail_at_every_call(Op op, Done done, Failures & failures)
  {
    for (long k = 1; k <= Max_Calls; ++k)
      {
        SCOPED_TRACE("the comparison throws at its call " + std::to_string(k));
        Fixture f;
        const auto before = f.state();
        bool thrown = false;
        {
          const Failing_At failing(k);
          try
            {
              op(f);
            }
          catch (const Comparison_Error &)
            {
              thrown = true;
            }
        }

        if (not thrown)
          {
            // k calls or more: the call k threw, and op caught it
            if (Countdown_Less::calls >= k)
              {
                ADD_FAILURE() << "the comparison threw, but the exception did not reach the caller";
                return;
              }
            done(f);
            return;
          }

        ++failures.thrown;
        f.expect_initial();
        if (f.state() != before)
          ++failures.restructured;
      }

    ADD_FAILURE() << "the operation made more than " << Max_Calls << " comparisons";
  }

  // A tree of type Tree with the initial keys.
  template <class Tree>
  class Tree_Fixture
  {
    Node_Pool<typename Tree::Node> pool;  // the tree does not own its nodes

  public:
    Tree tree;

    Tree_Fixture()
    {
      if constexpr (Seeded_Tree<Tree>)
        tree.set_seed(17);
      for (const int key : insertion_order())
        EXPECT_NE(tree.insert(pool.make(key)), nullptr);
    }

    typename Tree::Node * node(const int key) { return pool.make(key); }

    [[nodiscard]] Array<long> state() { return state_of(tree.getRoot()); }

    // `t` verifies and has `keys`, and as many as them if it counts them.
    static void expect_valid(Tree & t, const Array<int> & keys)
    {
      EXPECT_TRUE(t.verify());
      EXPECT_EQ(keys_of(t.getRoot()), keys);
      if constexpr (requires { t.size(); })
        {
          EXPECT_EQ(t.size(), keys.size());
        }
    }

    void expect_initial() { expect_valid(tree, keys_with()); }
  };

  // A DynSetTree with the initial keys.
  template <class Set>
  struct Set_Fixture
  {
    Set set;

    Set_Fixture()
    {
      for (const int key : insertion_order())
        EXPECT_NE(set.insert(key), nullptr);
    }

    [[nodiscard]] Array<long> state() { return state_of(set.get_root_node()); }

    static void expect_valid(const Set & s, const Array<int> & keys)
    {
      EXPECT_TRUE(s.verify());
      EXPECT_EQ(s.size(), keys.size());
      Array<int> got;
      s.for_each([&got] (const int key) { got.append(key); });
      EXPECT_EQ(got, keys);
    }

    void expect_initial() { expect_valid(set, keys_with()); }
  };

  // A DynMapTree with the initial keys, each one mapped to its opposite.
  template <class Map>
  struct Map_Fixture
  {
    Map map;

    Map_Fixture()
    {
      for (const int key : insertion_order())
        EXPECT_NE(map.insert(key, -key), nullptr);
    }

    [[nodiscard]] Array<long> state()
    {
      Array<long> state;
      map.for_each([&state] (const std::pair<int, int> & p)
                   {
                     state.append(p.first);
                     state.append(p.second);
                   });
      return state;
    }

    static void expect_valid(const Map & m, const Array<int> & keys)
    {
      EXPECT_TRUE(m.verify());
      EXPECT_EQ(m.size(), keys.size());
      Array<int> got;
      m.for_each([&got] (const std::pair<int, int> & p)
                 {
                   EXPECT_EQ(p.second, -p.first);
                   got.append(p.first);
                 });
      EXPECT_EQ(got, keys);
    }

    void expect_initial() { expect_valid(map, keys_with()); }
  };

  /* The trees under test. For each one, whether its insertions recolor
     or rotate on their way down, before their last comparison, and
     whether its operations are noexcept with a comparison that cannot
     throw (those of Rb_Tree_Rk never were). A new tree is one more line
     here. */
# define TREES_UNDER_TEST(X)      \
  X(BinTree, false, true)         \
  X(Avl_Tree, false, true)        \
  X(Avl_Tree_Rk, false, true)     \
  X(Rb_Tree, false, true)         \
  X(Rb_Tree_Rk, false, false)     \
  X(TdRbTree, true, true)         \
  X(TdRbTreeRk, true, true)       \
  X(HtdRbTree, true, true)        \
  X(HtdRbTreeRk, true, true)      \
  X(Treap, false, true)           \
  X(Treap_Rk, false, true)        \
  X(Rand_Tree, false, true)

  // A tree under test, alone, in a set and in a map.
# define DECLARE_TREE_KIND(T, Restructures, Noexcept)       \
  struct T##_Kind                                           \
  {                                                         \
    using Tree = T<int, Countdown_Less>;                    \
    using Nothrow_Tree = T<int>;                            \
    using Set = DynSetTree<int, T, Countdown_Less>;         \
    using Map = DynMapTree<int, int, T, Countdown_Less>;    \
    static constexpr bool restructures = Restructures;      \
    static constexpr bool noexcept_ops = Noexcept;          \
    static constexpr const char * name = #T;                \
  };

  TREES_UNDER_TEST(DECLARE_TREE_KIND)

  template <class First, class... Kinds>
  struct Without_First
  {
    using type = ::testing::Types<Kinds...>;
  };

# define TREE_KIND_ENTRY(T, Restructures, Noexcept) , T##_Kind

  using Tree_Kinds = Without_First<void TREES_UNDER_TEST(TREE_KIND_ENTRY)>::type;

# undef TREE_KIND_ENTRY
# undef DECLARE_TREE_KIND
# undef TREES_UNDER_TEST

  struct Kind_Names
  {
    template <class Kind>
    static std::string GetName(int)
    {
      return Kind::name;
    }
  };

  std::string trace_of(const char * op, const int key)
  {
    return std::string(op) + "(" + std::to_string(key) + ")";
  }
}

// The harness: an operation that catches what the comparison throws, and
// tries again, gives the right result, but the exception did not reach its
// caller. The run of fail_at_every_call() that takes it must fail, instead
// of taking it as the run where the comparison does not throw.
TEST(ThrowingCompareHarness, AnExceptionThatTheOperationCatchesFailsTheTest)
{
  using Fixture = Tree_Fixture<Avl_Tree<int, Countdown_Less>>;
  Failures failures;
  EXPECT_NONFATAL_FAILURE(fail_at_every_call<Fixture>(
                            [] (Fixture & f)
                            {
                              try
                                {
                                  (void) f.tree.search(4);
                                }
                              catch (const Comparison_Error &)  // the call k is past
                                {
                                  (void) f.tree.search(4);
                                }
                            },
                            [] (Fixture & f) { f.expect_initial(); }, failures),
                          "did not reach the caller");
  EXPECT_EQ(failures.thrown, 0);
}

template <class Kind>
struct ThrowingCompareTree : ::testing::Test {};

TYPED_TEST_SUITE(ThrowingCompareTree, Tree_Kinds, Kind_Names);

// With a comparison that cannot throw, the operations keep their
// noexcept; with one that can, they let through what it throws.
TYPED_TEST(ThrowingCompareTree, IsNoexceptOnlyWithANothrowComparison)
{
  using Nothrow = typename TypeParam::Nothrow_Tree;
  using Node = typename Nothrow::Node;
  constexpr bool expected = TypeParam::noexcept_ops;
  EXPECT_EQ(noexcept(std::declval<Nothrow &>().insert(std::declval<Node *>())), expected);
  EXPECT_EQ(noexcept(std::declval<Nothrow &>().insert_dup(std::declval<Node *>())), expected);
  EXPECT_EQ(noexcept(std::declval<Nothrow &>().search_or_insert(std::declval<Node *>())),
            expected);
  EXPECT_EQ(noexcept(std::declval<Nothrow &>().search(1)), expected);
  EXPECT_EQ(noexcept(std::declval<Nothrow &>().remove(1)), expected);

  using Tree = typename TypeParam::Tree;
  using Tree_Node = typename Tree::Node;
  EXPECT_FALSE(noexcept(std::declval<Tree &>().insert(std::declval<Tree_Node *>())));
  EXPECT_FALSE(noexcept(std::declval<Tree &>().insert_dup(std::declval<Tree_Node *>())));
  EXPECT_FALSE(noexcept(std::declval<Tree &>().search_or_insert(std::declval<Tree_Node *>())));
  EXPECT_FALSE(noexcept(std::declval<Tree &>().search(1)));
  EXPECT_FALSE(noexcept(std::declval<Tree &>().remove(1)));
}

TYPED_TEST(ThrowingCompareTree, InsertionLetsTheExceptionThrough)
{
  using Tree = typename TypeParam::Tree;
  using Fixture = Tree_Fixture<Tree>;
  using Node = typename Tree::Node;
  Failures failures;

  for (const int key : target_keys())
    {
      const bool present = is_present(key);
      Node * got = nullptr;
      Node * p = nullptr;
      {
        SCOPED_TRACE(trace_of("insert", key));
        fail_at_every_call<Fixture>(
          [&] (Fixture & f) { got = f.tree.insert(p = f.node(key)); },
          [&] (Fixture & f)
          {
            EXPECT_EQ(got, present ? nullptr : p);
            Fixture::expect_valid(f.tree, present ? keys_with() : keys_with(key));
          }, failures);
      }
      {
        SCOPED_TRACE(trace_of("insert_dup", key));
        fail_at_every_call<Fixture>(
          [&] (Fixture & f) { got = f.tree.insert_dup(p = f.node(key)); },
          [&] (Fixture & f)
          {
            EXPECT_EQ(got, p);
            Fixture::expect_valid(f.tree, keys_with(key));
          }, failures);
      }
      {
        SCOPED_TRACE(trace_of("search_or_insert", key));
        fail_at_every_call<Fixture>(
          [&] (Fixture & f) { got = f.tree.search_or_insert(p = f.node(key)); },
          [&] (Fixture & f)
          {
            ASSERT_NE(got, nullptr);
            EXPECT_EQ(KEY(got), key);
            EXPECT_EQ(got == p, not present);
            Fixture::expect_valid(f.tree, present ? keys_with() : keys_with(key));
          }, failures);
      }
    }

  EXPECT_GT(failures.thrown, 0);
  // The top-down trees recolor and rotate before they compare again: they
  // must be left valid. The others change only after their last
  // comparison: they must be left as they were.
  if constexpr (TypeParam::restructures)
    {
      EXPECT_GT(failures.restructured, 0);
    }
  else
    {
      EXPECT_EQ(failures.restructured, 0);
    }
}

TYPED_TEST(ThrowingCompareTree, SearchLetsTheExceptionThrough)
{
  using Tree = typename TypeParam::Tree;
  using Fixture = Tree_Fixture<Tree>;
  using Node = typename Tree::Node;
  Failures failures;

  for (const int key : target_keys())
    {
      const bool present = is_present(key);
      {
        SCOPED_TRACE(trace_of("search", key));
        Node * got = nullptr;
        fail_at_every_call<Fixture>(
          [&] (Fixture & f) { got = f.tree.search(key); },
          [&] (Fixture & f)
          {
            EXPECT_EQ(got != nullptr, present);
            if (got != nullptr)
              {
                EXPECT_EQ(KEY(got), key);
              }
            f.expect_initial();
          }, failures);
      }

      if constexpr (requires (Tree & t, const int k) { t.position(k); t.find_position(k); })
        {
          SCOPED_TRACE(trace_of("position", key));
          long pos = 0;
          fail_at_every_call<Fixture>(
            [&] (Fixture & f) { pos = f.tree.position(key).first; },
            [&] (Fixture & f)
            {
              EXPECT_EQ(pos, present ? rank_of(key) : -1);
              f.expect_initial();
            }, failures);
          fail_at_every_call<Fixture>(
            [&] (Fixture & f) { pos = f.tree.find_position(key).first; },
            [&] (Fixture & f)
            {
              if (present)
                {
                  EXPECT_EQ(pos, rank_of(key));
                }
              f.expect_initial();
            }, failures);
        }
    }

  EXPECT_GT(failures.thrown, 0);
  EXPECT_EQ(failures.restructured, 0);
}

TYPED_TEST(ThrowingCompareTree, RemovalLetsTheExceptionThrough)
{
  using Tree = typename TypeParam::Tree;
  using Fixture = Tree_Fixture<Tree>;
  using Node = typename Tree::Node;
  Failures failures;

  for (const int key : target_keys())
    {
      SCOPED_TRACE(trace_of("remove", key));
      Node * got = nullptr;
      fail_at_every_call<Fixture>(
        [&] (Fixture & f) { got = f.tree.remove(key); },
        [&] (Fixture & f)
        {
          EXPECT_EQ(got != nullptr, is_present(key));
          if (got != nullptr)
            {
              EXPECT_EQ(KEY(got), key);
            }
          Fixture::expect_valid(f.tree, keys_with(INT_MIN, key));
        }, failures);
    }

  EXPECT_GT(failures.thrown, 0);
  if constexpr (not TypeParam::restructures)
    {
      EXPECT_EQ(failures.restructured, 0);
    }
}

namespace
{
  // A split by key: it splits `t` by `key` into `l` and `r`, and returns
  // false if it did not split because `key` is in `t`.
  template <class Tree>
  struct Split
  {
    const char * name;
    bool (*split)(Tree & t, int key, Tree & l, Tree & r);
  };

  // The splits by key of Tree that let through what the comparison
  // throws. The others move the nodes one by one, and stay noexcept.
  template <class Tree>
  Array<Split<Tree>> splits_that_may_throw()
  {
    Array<Split<Tree>> splits;

    if constexpr (requires (Tree t, Tree l, Tree r) { t.split_key(1, l, r); })
      if constexpr (not noexcept(std::declval<Tree &>().split_key(1, std::declval<Tree &>(),
                                                                  std::declval<Tree &>())))
        {
          splits.append({"split_key", [] (Tree & t, int k, Tree & l, Tree & r)
                            {
                              return t.split_key(k, l, r);
                            }});
          splits.append({"split_key_dup", [] (Tree & t, int k, Tree & l, Tree & r)
                            {
                              t.split_key_dup(k, l, r);
                              return true;
                            }});
        }

    if constexpr (requires (Tree t, Tree l, Tree r) { t.split(1, l, r); })
      {
        splits.append({"split", [] (Tree & t, int k, Tree & l, Tree & r)
                          {
                            return t.split(k, l, r);
                          }});
        splits.append({"split_dup", [] (Tree & t, int k, Tree & l, Tree & r)
                          {
                            t.split_dup(k, l, r);
                            return true;
                          }});
      }

    return splits;
  }

  // A tree with the initial keys, and the two empty ones of its split.
  template <class Tree>
  struct Split_Fixture : Tree_Fixture<Tree>
  {
    Tree l, r;

    void expect_initial()
    {
      Tree_Fixture<Tree>::expect_initial();
      EXPECT_EQ(l.getRoot(), Tree::Node::NullPtr);
      EXPECT_EQ(r.getRoot(), Tree::Node::NullPtr);
    }
  };

  // `l` and `r` split the initial keys by `key`: those before it in `l`,
  // those after it in `r`, and `key` in either one.
  template <class Node>
  void expect_split_by(Node * l, Node * r, const int key)
  {
    Array<int> keys = keys_of(l);
    const Array<int> right = keys_of(r);
    for (const int k : keys)
      EXPECT_LE(k, key);
    for (const int k : right)
      EXPECT_GE(k, key);
    for (const int k : right)
      keys.append(k);
    EXPECT_EQ(keys, keys_with());
  }
}

TYPED_TEST(ThrowingCompareTree, SplitLetsTheExceptionThrough)
{
  using Tree = typename TypeParam::Tree;
  using Fixture = Split_Fixture<Tree>;
  Failures failures;

  for (const Split<Tree> & s : splits_that_may_throw<Tree>())
    for (const int key : target_keys())
      {
        SCOPED_TRACE(trace_of(s.name, key));
        bool split = false;
        fail_at_every_call<Fixture>(
          [&] (Fixture & f) { split = s.split(f.tree, key, f.l, f.r); },
          [&] (Fixture & f)
          {
            if (not split)  // key is in the tree, which stays as it was
              {
                EXPECT_TRUE(is_present(key));
                f.expect_initial();
                return;
              }
            EXPECT_EQ(f.tree.getRoot(), Tree::Node::NullPtr);
            EXPECT_TRUE(f.l.verify());
            EXPECT_TRUE(f.r.verify());
            expect_split_by(f.l.getRoot(), f.r.getRoot(), key);
          }, failures);
      }

  EXPECT_EQ(failures.restructured, 0);
}

template <class Kind>
struct ThrowingCompareSet : ::testing::Test {};

TYPED_TEST_SUITE(ThrowingCompareSet, Tree_Kinds, Kind_Names);

// The set frees the node of a key whose insertion throws: the sanitizers
// would report it otherwise.
TYPED_TEST(ThrowingCompareSet, LetsTheExceptionThrough)
{
  using Set = typename TypeParam::Set;
  using Fixture = Set_Fixture<Set>;
  using Tree = typename TypeParam::Tree;
  Failures failures;

  for (const int key : target_keys())
    {
      const bool present = is_present(key);
      int * got = nullptr;
      {
        SCOPED_TRACE(trace_of("insert", key));
        fail_at_every_call<Fixture>(
          [&] (Fixture & f) { got = f.set.insert(key); },
          [&] (Fixture & f)
          {
            EXPECT_EQ(got == nullptr, present);
            Fixture::expect_valid(f.set, present ? keys_with() : keys_with(key));
          }, failures);
      }
      {
        SCOPED_TRACE(trace_of("insert_dup", key));
        fail_at_every_call<Fixture>(
          [&] (Fixture & f) { got = f.set.insert_dup(key); },
          [&] (Fixture & f)
          {
            EXPECT_NE(got, nullptr);
            Fixture::expect_valid(f.set, keys_with(key));
          }, failures);
      }
      {
        SCOPED_TRACE(trace_of("search_or_insert", key));
        fail_at_every_call<Fixture>(
          [&] (Fixture & f) { got = f.set.search_or_insert(key); },
          [&] (Fixture & f)
          {
            ASSERT_NE(got, nullptr);
            EXPECT_EQ(*got, key);
            Fixture::expect_valid(f.set, present ? keys_with() : keys_with(key));
          }, failures);
      }
      {
        SCOPED_TRACE(trace_of("search", key));
        fail_at_every_call<Fixture>(
          [&] (Fixture & f) { got = f.set.search(key); },
          [&] (Fixture & f)
          {
            EXPECT_EQ(got != nullptr, present);
            f.expect_initial();
          }, failures);
      }
      {
        SCOPED_TRACE(trace_of("find", key));
        bool found = false;
        bool refused = false;  // it threw std::domain_error
        fail_at_every_call<Fixture>(
          [&] (Fixture & f)
          {
            refused = false;
            try
              {
                found = f.set.find(key) == key;
              }
            catch (const std::domain_error &)  // the key is not in the set
              {
                found = false;
                refused = true;
              }
          },
          [&] (Fixture & f)
          {
            EXPECT_EQ(found, present);
            EXPECT_EQ(refused, not present)
              << "find() throws std::domain_error for a key not in the set, alone";
            f.expect_initial();
          }, failures);
      }
      {
        SCOPED_TRACE(trace_of("remove", key));
        size_t size = 0;
        fail_at_every_call<Fixture>(
          [&] (Fixture & f) { size = f.set.remove(key); },
          [&] (Fixture & f)
          {
            EXPECT_EQ(size, present ? Num_Keys - 1 : Num_Keys);
            Fixture::expect_valid(f.set, keys_with(INT_MIN, key));
          }, failures);
      }

      if constexpr (requires (Tree & t, const int k) { t.position(k); })
        {
          SCOPED_TRACE(trace_of("position", key));
          long pos = 0;
          fail_at_every_call<Fixture>(
            [&] (Fixture & f) { pos = f.set.position(key); },
            [&] (Fixture & f)
            {
              EXPECT_EQ(pos, present ? rank_of(key) : -1);
              f.expect_initial();
            }, failures);
        }
    }

  EXPECT_GT(failures.thrown, 0);
}

template <class Kind>
struct ThrowingCompareMap : ::testing::Test {};

TYPED_TEST_SUITE(ThrowingCompareMap, Tree_Kinds, Kind_Names);

TYPED_TEST(ThrowingCompareMap, LetsTheExceptionThrough)
{
  using Map = typename TypeParam::Map;
  using Fixture = Map_Fixture<Map>;
  Failures failures;

  for (const int key : target_keys())
    {
      const bool present = is_present(key);
      {
        SCOPED_TRACE(trace_of("insert", key));
        std::pair<int, int> * got = nullptr;
        fail_at_every_call<Fixture>(
          [&] (Fixture & f) { got = f.map.insert(key, -key); },
          [&] (Fixture & f)
          {
            EXPECT_EQ(got == nullptr, present);
            Fixture::expect_valid(f.map, present ? keys_with() : keys_with(key));
          }, failures);
      }
      {
        SCOPED_TRACE(trace_of("operator[]", key));
        fail_at_every_call<Fixture>(
          [&] (Fixture & f) { f.map[key] = -key; },
          [&] (Fixture & f)
          {
            Fixture::expect_valid(f.map, present ? keys_with() : keys_with(key));
          }, failures);
      }
      {
        SCOPED_TRACE(trace_of("find", key));
        bool found = false;
        bool refused = false;  // it threw std::domain_error
        fail_at_every_call<Fixture>(
          [&] (Fixture & f)
          {
            refused = false;
            try
              {
                found = f.map.find(key) == -key;
              }
            catch (const std::domain_error &)  // the key is not in the map
              {
                found = false;
                refused = true;
              }
          },
          [&] (Fixture & f)
          {
            EXPECT_EQ(found, present);
            EXPECT_EQ(refused, not present)
              << "find() throws std::domain_error for a key not in the map, alone";
            f.expect_initial();
          }, failures);
      }
      {
        SCOPED_TRACE(trace_of("remove", key));
        bool removed = false;
        bool refused = false;  // it threw std::domain_error
        fail_at_every_call<Fixture>(
          [&] (Fixture & f)
          {
            refused = false;
            try
              {
                removed = f.map.remove(key) == -key;
              }
            catch (const std::domain_error &)  // the key is not in the map
              {
                removed = false;
                refused = true;
              }
          },
          [&] (Fixture & f)
          {
            EXPECT_EQ(removed, present);
            EXPECT_EQ(refused, not present)
              << "remove() throws std::domain_error for a key not in the map, alone";
            Fixture::expect_valid(f.map, keys_with(INT_MIN, key));
          }, failures);
      }

      // search() of DynMapTree lets through what the comparison throws:
      // a search that went back to noexcept fails here, instead of
      // skipping its test
      {
        SCOPED_TRACE(trace_of("search", key));
        EXPECT_FALSE(noexcept(std::declval<Map &>().search(key)))
          << "search() is noexcept, but the comparison may throw";
        if constexpr (not noexcept(std::declval<Map &>().search(key)))
          {
            bool found = false;
            fail_at_every_call<Fixture>(
              [&] (Fixture & f) { found = f.map.search(key) != nullptr; },
              [&] (Fixture & f)
              {
                EXPECT_EQ(found, present);
                f.expect_initial();
              }, failures);
          }
      }
    }

  EXPECT_GT(failures.thrown, 0);
}

namespace
{
  // A tree of nodes of type Node with the initial keys, built and searched
  // with the operations Ops.
  template <class Node, class Ops>
  struct Ops_Fixture
  {
    Node_Pool<Node> pool;
    Ops ops;
    Node * root = Node::NullPtr;

    Ops_Fixture()
    {
      for (const int key : insertion_order())
        EXPECT_NE(ops.insert(root, pool.make(key)), Node::NullPtr);
      if constexpr (Counted_Node<Node>)
        (void) count(root);  // insert() of BinTree_Operation does not count
    }

    static size_t count(Node * p)
    {
      if (p == Node::NullPtr)
        return 0;
      return p->getCount() = count(LLINK(p)) + count(RLINK(p)) + 1;
    }

    Node * node(const int key) { return pool.make(key); }

    [[nodiscard]] Array<long> state() { return state_of(root); }

    static void expect_valid(Node * r, const Array<int> & keys)
    {
      EXPECT_TRUE(check_bst(r, Countdown_Less()));
      if constexpr (Counted_Node<Node>)
        {
          EXPECT_TRUE(check_rank_tree(r));
        }
      EXPECT_EQ(keys_of(r), keys);
    }

    void expect_initial() { expect_valid(root, keys_with()); }
  };
}

// The operations of BinTree_Operation compare before they change the
// tree: what the comparison throws leaves it as it was.
TEST(ThrowingCompareOperations, BinTreeOperationLeavesTheTreeAsItWas)
{
  using Node = BinNode<int>;
  using Fixture = Ops_Fixture<Node, BinTree_Operation<Node, Countdown_Less>>;
  Failures failures;

  for (const int key : target_keys())
    {
      const bool present = is_present(key);
      SCOPED_TRACE(trace_of("the operations at", key));
      Node * got = nullptr;
      Node * p = nullptr;
      const Array<int> keys_after_insert = present ? keys_with() : keys_with(key);

      fail_at_every_call<Fixture>(
        [&] (Fixture & f) { got = f.ops.search(f.root, key); },
        [&] (Fixture & f)
        {
          EXPECT_EQ(got != Node::NullPtr, present);
          f.expect_initial();
        }, failures);

      fail_at_every_call<Fixture>(
        [&] (Fixture & f)
        {
          Node head;  // the parent of the root, as search_parent() asks
          RLINK(&head) = f.root;
          Node * parent = &head;
          got = f.ops.search_parent(f.root, key, parent);
        },
        [&] (Fixture & f)
        {
          EXPECT_EQ(KEY(got) == key, present);
          f.expect_initial();
        }, failures);

      fail_at_every_call<Fixture>(
        [&] (Fixture & f) { got = f.ops.search_rank_parent(f.root, key); },
        [&] (Fixture & f)
        {
          EXPECT_EQ(KEY(got) == key, present);
          f.expect_initial();
        }, failures);

      fail_at_every_call<Fixture>(
        [&] (Fixture & f) { got = f.ops.insert(f.root, p = f.node(key)); },
        [&] (Fixture & f)
        {
          EXPECT_EQ(got == p, not present);
          Fixture::expect_valid(f.root, keys_after_insert);
        }, failures);

      fail_at_every_call<Fixture>(
        [&] (Fixture & f) { got = f.ops.insert_dup(f.root, p = f.node(key)); },
        [&] (Fixture & f)
        {
          EXPECT_EQ(got, p);
          Fixture::expect_valid(f.root, keys_with(key));
        }, failures);

      fail_at_every_call<Fixture>(
        [&] (Fixture & f) { got = f.ops.search_or_insert(f.root, p = f.node(key)); },
        [&] (Fixture & f)
        {
          EXPECT_EQ(KEY(got), key);
          EXPECT_EQ(got == p, not present);
          Fixture::expect_valid(f.root, keys_after_insert);
        }, failures);

      fail_at_every_call<Fixture>(
        [&] (Fixture & f) { got = f.ops.remove(f.root, key); },
        [&] (Fixture & f)
        {
          EXPECT_EQ(got != Node::NullPtr, present);
          Fixture::expect_valid(f.root, keys_with(INT_MIN, key));
        }, failures);

      fail_at_every_call<Fixture>(
        [&] (Fixture & f) { got = f.ops.insert_root(f.root, p = f.node(key)); },
        [&] (Fixture & f)
        {
          EXPECT_EQ(got == p, not present);
          EXPECT_EQ(f.root == p, not present);
          Fixture::expect_valid(f.root, keys_after_insert);
        }, failures);

      fail_at_every_call<Fixture>(
        [&] (Fixture & f) { got = f.ops.insert_dup_root(f.root, p = f.node(key)); },
        [&] (Fixture & f)
        {
          EXPECT_EQ(f.root, p);
          Fixture::expect_valid(f.root, keys_with(key));
        }, failures);

      fail_at_every_call<Fixture>(
        [&] (Fixture & f)
        {
          got = f.ops.insert_root_rec(f.root, p = f.node(key));
          if (got != Node::NullPtr)
            f.root = got;
        },
        [&] (Fixture & f)
        {
          EXPECT_EQ(got == p, not present);
          Fixture::expect_valid(f.root, keys_after_insert);
        }, failures);

      fail_at_every_call<Fixture>(
        [&] (Fixture & f)
        {
          got = f.ops.search_or_insert_root_rec(f.root, p = f.node(key));
          if (got == p)
            f.root = got;
        },
        [&] (Fixture & f)
        {
          EXPECT_EQ(KEY(got), key);
          EXPECT_EQ(got == p, not present);
          Fixture::expect_valid(f.root, keys_after_insert);
        }, failures);

      Node * ts = Node::NullPtr;
      Node * tg = Node::NullPtr;
      bool split = false;
      fail_at_every_call<Fixture>(
        [&] (Fixture & f)
        {
          ts = tg = Node::NullPtr;
          split = f.ops.split_key_rec(f.root, key, ts, tg);
        },
        [&] (Fixture & f)
        {
          EXPECT_EQ(split, not present);
          if (not split)
            {
              f.expect_initial();
              return;
            }
          EXPECT_EQ(f.root, Node::NullPtr);
          expect_split_by(ts, tg, key);
        }, failures);

      // split_key_dup_rec() put no node in ts or tg, and lost the tree;
      // insert_dup_root(), above, kept only the node that it inserted
      fail_at_every_call<Fixture>(
        [&] (Fixture & f)
        {
          ts = tg = Node::NullPtr;
          f.ops.split_key_dup_rec(f.root, key, ts, tg);
        },
        [&] (Fixture & f)
        {
          EXPECT_EQ(f.root, Node::NullPtr);
          EXPECT_TRUE(check_bst(ts, Countdown_Less()));
          EXPECT_TRUE(check_bst(tg, Countdown_Less()));
          for (const int k : keys_of(ts))
            EXPECT_LT(k, key);
          expect_split_by(ts, tg, key);
        }, failures);
    }

  EXPECT_GT(failures.thrown, 0);
  EXPECT_EQ(failures.restructured, 0);
}

// The operations of BinTreeXt_Operation compare before they change the
// tree: what the comparison throws leaves it, and its counters, as they
// were.
TEST(ThrowingCompareOperations, BinTreeXtOperationLeavesTheTreeAsItWas)
{
  using Node = BinNodeXt<int>;
  using Fixture = Ops_Fixture<Node, BinTreeXt_Operation<Node, Countdown_Less>>;
  Failures failures;

  for (const int key : target_keys())
    {
      const bool present = is_present(key);
      SCOPED_TRACE(trace_of("the operations at", key));
      Node * got = nullptr;
      Node * p = nullptr;
      long pos = 0;

      fail_at_every_call<Fixture>(
        [&] (Fixture & f) { pos = f.ops.inorder_position(f.root, key, got); },
        [&] (Fixture & f)
        {
          EXPECT_EQ(pos, present ? rank_of(key) : -1);
          f.expect_initial();
        }, failures);

      fail_at_every_call<Fixture>(
        [&] (Fixture & f) { pos = f.ops.find_position(f.root, key, got); },
        [&] (Fixture & f)
        {
          if (present)
            {
              EXPECT_EQ(pos, rank_of(key));
            }
          f.expect_initial();
        }, failures);

      fail_at_every_call<Fixture>(
        [&] (Fixture & f) { got = f.ops.insert_root(f.root, p = f.node(key)); },
        [&] (Fixture & f)
        {
          EXPECT_EQ(got == p, not present);
          Fixture::expect_valid(f.root, present ? keys_with() : keys_with(key));
        }, failures);

      fail_at_every_call<Fixture>(
        [&] (Fixture & f) { got = f.ops.insert_dup_root(f.root, p = f.node(key)); },
        [&] (Fixture & f)
        {
          EXPECT_EQ(f.root, p);
          Fixture::expect_valid(f.root, keys_with(key));
        }, failures);

      Node * l = Node::NullPtr;
      Node * r = Node::NullPtr;
      bool split = false;
      fail_at_every_call<Fixture>(
        [&] (Fixture & f)
        {
          l = r = Node::NullPtr;
          split = f.ops.split_key_rec(f.root, key, l, r);
          if (split)
            f.root = Node::NullPtr;  // its nodes are now in l and r
        },
        [&] (Fixture & f)
        {
          EXPECT_EQ(split, not present);
          if (not split)
            {
              f.expect_initial();
              return;
            }
          EXPECT_TRUE(check_rank_tree(l));
          EXPECT_TRUE(check_rank_tree(r));
          expect_split_by(l, r, key);
        }, failures);

      fail_at_every_call<Fixture>(
        [&] (Fixture & f)
        {
          l = r = Node::NullPtr;
          f.ops.split_key_dup_rec(f.root, key, l, r);
          f.root = Node::NullPtr;  // its nodes are now in l and r
        },
        [&] (Fixture &)
        {
          EXPECT_TRUE(check_rank_tree(l));
          EXPECT_TRUE(check_rank_tree(r));
          expect_split_by(l, r, key);
        }, failures);
    }

  EXPECT_GT(failures.thrown, 0);
  EXPECT_EQ(failures.restructured, 0);
}

namespace
{
  // Splits a tree with the keys 0, 2, ..., 18, ordered from the greatest,
  // by 9: the keys before 9 in the order of the tree are those greater.
  template <class Tree>
  void expect_split_in_the_order_of_the_tree()
  {
    Node_Pool<typename Tree::Node> pool;
    Tree t, l, r;
    if constexpr (Seeded_Tree<Tree>)
      t.set_seed(5);
    for (int k = 0; k < 20; k += 2)
      ASSERT_NE(t.insert(pool.make(k)), nullptr);

    if constexpr (requires { t.split_key(9, l, r); })
      (void) t.split_key(9, l, r);
    else
      (void) t.split(9, l, r);

    EXPECT_EQ(keys_of(l.getRoot()), (Array<int>{18, 16, 14, 12, 10}));
    EXPECT_EQ(keys_of(r.getRoot()), (Array<int>{8, 6, 4, 2, 0}));
    EXPECT_TRUE(l.verify());
    EXPECT_TRUE(r.verify());
  }
}

// The splits of the treaps and of Rand_Tree split with the comparison of
// the tree: they compared with Aleph::less of the key, and with another
// order they lost keys or put them on the wrong side.
TEST(ThrowingCompareOperations, SplitsWithTheComparisonOfTheTree)
{
  expect_split_in_the_order_of_the_tree<Treap<int, Aleph::greater<int>>>();
  expect_split_in_the_order_of_the_tree<Treap_Rk<int, Aleph::greater<int>>>();
  expect_split_in_the_order_of_the_tree<Rand_Tree<int, Aleph::greater<int>>>();
  expect_split_in_the_order_of_the_tree<BinTree<int, Aleph::greater<int>>>();
}

// The splay trees keep their noexcept: a splay relinks the tree as it
// compares, so their comparison must not throw.
TEST(ThrowingCompareOperations, SplayTreesKeepTheirNoexcept)
{
  using Tree = Splay_Tree<int, Countdown_Less>;
  using Node = Tree::Node;
  static_assert(noexcept(std::declval<Tree &>().insert(std::declval<Node *>())));
  static_assert(noexcept(std::declval<Tree &>().insert_dup(std::declval<Node *>())));
  static_assert(noexcept(std::declval<Tree &>().search_or_insert(std::declval<Node *>())));
  static_assert(noexcept(std::declval<Tree &>().search(1)));
  static_assert(noexcept(std::declval<Tree &>().remove(1)));
  static_assert(noexcept(std::declval<Tree &>().splay(1)));
  static_assert(noexcept(std::declval<Splay_Tree_Rk<int, Countdown_Less> &>().splay(1)));
}

namespace
{
  // A comparison that does not throw, whose result throws when it is
  // converted to bool, while armed
  struct Proxy_Less
  {
    static inline bool armed = false;

    struct Result
    {
      bool value;

      operator bool () const
      {
        if (armed)
          throw Comparison_Error();
        return value;
      }
    };

    Result operator ()(const int & a, const int & b) const noexcept
    {
      return Result{a < b};
    }
  };

  // A comparison that does not throw, whose result converts to bool in two
  // ways: implicitly, as a return does, through an operator int() that
  // does not throw, and contextually, as the condition of an if does,
  // through an operator bool() that throws while armed
  struct Two_Conversions_Less
  {
    static inline bool armed = false;

    struct Result
    {
      bool value;

      operator int () const noexcept { return value; }

      explicit operator bool () const
      {
        if (armed)
          throw Comparison_Error();
        return value;
      }
    };

    Result operator ()(const int & a, const int & b) const noexcept
    {
      return Result{a < b};
    }
  };

  // Its overloads for keys that are not constant count their calls and
  // throw, while armed; the one for constant keys does not throw
  template <class Key>
  struct Mutable_Key_Less
  {
    static inline bool armed = false;
    static inline long mutable_calls = 0;

    static bool mutable_call(const bool result)
    {
      ++mutable_calls;
      if (armed)
        throw Comparison_Error();
      return result;
    }

    bool operator ()(const Key & a, const Key & b) const noexcept { return a < b; }
    bool operator ()(const Key & a, Key & b) const { return mutable_call(a < b); }
    bool operator ()(Key & a, const Key & b) const { return mutable_call(a < b); }
    bool operator ()(Key & a, Key & b) const { return mutable_call(a < b); }
  };

  // A comparison that does not throw, whose copy throws while armed
  struct Copy_Throw_Less
  {
    static inline bool armed = false;

    Copy_Throw_Less() = default;

    Copy_Throw_Less(const Copy_Throw_Less &)
    {
      if (armed)
        throw Comparison_Error();
    }

    Copy_Throw_Less & operator = (const Copy_Throw_Less &) = default;

    bool operator ()(const int & a, const int & b) const noexcept { return a < b; }
  };

  // While one lives, the comparison Cmp is armed
  template <class Cmp>
  struct Armed
  {
    Armed() noexcept { Cmp::armed = true; }
    ~Armed() { Cmp::armed = false; }
    Armed(const Armed &) = delete;
    Armed & operator = (const Armed &) = delete;
  };

  // A comparison that knows whether it is alive when it is called: each
  // one keeps its address among those of the live ones while it lives
  struct Tracked_Less
  {
    static constexpr size_t Max_Alive = 64;
    static inline const Tracked_Less * alive[Max_Alive] = {};
    static inline size_t num_alive = 0;
    static inline long calls_on_dead = 0;

    void born() noexcept
    {
      if (num_alive < Max_Alive)
        alive[num_alive++] = this;
    }

    Tracked_Less() noexcept { born(); }
    Tracked_Less(const Tracked_Less &) noexcept { born(); }
    Tracked_Less & operator = (const Tracked_Less &) noexcept { return *this; }

    ~Tracked_Less()
    {
      for (size_t i = 0; i < num_alive; ++i)
        if (alive[i] == this)
          {
            alive[i] = alive[--num_alive];
            return;
          }
    }

    bool is_alive() const noexcept
    {
      for (size_t i = 0; i < num_alive; ++i)
        if (alive[i] == this)
          return true;
      return false;
    }

    bool operator ()(const int & a, const int & b) const noexcept
    {
      if (not is_alive())
        ++calls_on_dead;
      return a < b;
    }
  };

  // A tree that gives a copy of its comparison, as the concept BSTPolicy
  // allows to a tree of the user
  template <typename Key, class Compare>
  struct By_Value_Avl : public Avl_Tree<Key, Compare>
  {
    using Base = Avl_Tree<Key, Compare>;
    using Base::Base;

    Compare get_compare() const
    {
      return const_cast<By_Value_Avl &>(*this).Base::get_compare();
    }
  };

  // A tree with the keys 1, 3, 4, 5 and 8
  template <class Tree>
  struct Small_Tree
  {
    Node_Pool<typename Tree::Node> pool;
    Tree tree;

    Small_Tree()
    {
      if constexpr (Seeded_Tree<Tree>)
        tree.set_seed(3);
      for (const int key : {5, 3, 8, 1, 4})
        EXPECT_NE(tree.insert(pool.make(key)), nullptr);
    }
  };

  // The search of Tree lets through what the condition on the result of
  // Two_Conversions_Less throws, and so does the position of a node that
  // is not the root, which takes comparisons, in the ranked trees
  template <class Tree>
  void expect_the_condition_lets_the_exception_through()
  {
    using Less = Two_Conversions_Less;
    Small_Tree<Tree> f;
    EXPECT_FALSE(noexcept(f.tree.search(3)));
    {
      const Armed<Less> armed;
      EXPECT_THROW((void) f.tree.search(4), Comparison_Error);
    }
    EXPECT_NE(f.tree.search(8), nullptr);

    if constexpr (requires (typename Tree::Iterator it, typename Tree::Node * p)
                  { it.reset_to_node(p); })
      {
        typename Tree::Iterator it(f.tree);
        it.reset_to_node(f.tree.search(KEY(f.tree.getRoot()) == 1 ? 3 : 1));
        EXPECT_FALSE(noexcept(it.has_curr()));
        {
          const Armed<Less> armed;
          EXPECT_THROW((void) it.has_curr(), Comparison_Error);
        }
        EXPECT_TRUE(it.has_curr());
      }
  }

  // The operations that copy the comparison into an operation object, as
  // those of BinTree and the positions of the ranked trees, let through
  // what that copy throws
  template <class Tree>
  void expect_the_copy_lets_the_exception_through()
  {
    Small_Tree<Tree> f;
    auto & tree = f.tree;
    const Armed<Copy_Throw_Less> armed;
    if constexpr (requires { tree.position(3); })
      {
        EXPECT_FALSE(noexcept(tree.position(3)));
        EXPECT_FALSE(noexcept(tree.find_position(3)));
        EXPECT_THROW((void) tree.position(3), Comparison_Error);
        EXPECT_THROW((void) tree.find_position(3), Comparison_Error);
      }
    else
      {
        EXPECT_FALSE(noexcept(tree.search(3)));
        EXPECT_THROW((void) tree.search(3), Comparison_Error);
        EXPECT_THROW((void) tree.insert(f.pool.make(7)), Comparison_Error);
        EXPECT_THROW((void) tree.remove(3), Comparison_Error);
      }
    Copy_Throw_Less::armed = false;
    EXPECT_TRUE(tree.verify());
    EXPECT_EQ(keys_of(tree.getRoot()), (Array<int>{1, 3, 4, 5, 8}));
  }

  // The iterators of the ranked trees compare to go to a key, and to find
  // the position of a node that they were put on: what the comparison
  // throws must reach the caller, with the iterator as it was
  template <class Tree>
  void expect_the_iterator_lets_the_exception_through()
  {
    Tree_Fixture<Tree> f;
    auto & tree = f.tree;
    using Iterator = typename Tree::Iterator;
    EXPECT_FALSE(noexcept(std::declval<Iterator &>().reset_to_key(1)));
    EXPECT_FALSE(noexcept(std::declval<const Iterator &>().has_curr()));
    EXPECT_FALSE(noexcept(std::declval<const Iterator &>() ==
                          std::declval<const Iterator &>()));

    Iterator it(tree, size_t{3});  // on the key 6
    {
      const Failing_At failing(1);
      EXPECT_THROW(it.reset_to_key(10), Comparison_Error);
    }
    EXPECT_EQ(it.get_pos(), 3u);
    EXPECT_EQ(KEY(it.get_curr()), 6);

    // a node that is not the root, so that its position takes comparisons
    const int key = KEY(tree.getRoot()) == 0 ? 2 : 0;
    typename Tree::Node * node = tree.search(key);
    ASSERT_NE(node, nullptr);
    Iterator by_node(tree);
    by_node.reset_to_node(node);
    const Iterator at_pos(tree, size_t{3});
    {
      const Failing_At failing(1);
      EXPECT_THROW((void) by_node.has_curr(), Comparison_Error);
    }
    {
      const Failing_At failing(1);
      EXPECT_THROW((void) (by_node == at_pos), Comparison_Error);
    }
    {
      const Failing_At failing(1);
      EXPECT_THROW((void) by_node.get_pos(), Comparison_Error);
    }
    ASSERT_TRUE(by_node.has_curr());
    EXPECT_EQ(by_node.get_curr(), node);
    EXPECT_EQ(by_node.get_pos(), static_cast<size_t>(rank_of(key)));
    f.expect_initial();
  }
}

// A comparison whose result throws when it is converted to bool: the
// operations converted it inside noexcept, and the program ended
TEST(ThrowingCompareOperations, TheConversionOfTheResultLetsTheExceptionThrough)
{
  Small_Tree<Avl_Tree<int, Proxy_Less>> f;
  EXPECT_FALSE(noexcept(f.tree.search(3)));
  {
    const Armed<Proxy_Less> armed;
    EXPECT_THROW((void) f.tree.search(4), Comparison_Error);
  }
  EXPECT_NE(f.tree.search(8), nullptr);

  DynMapTree<int, int, Avl_Tree, Proxy_Less> map;
  for (const int key : {5, 3, 8})
    map.insert(key, -key);
  EXPECT_FALSE(noexcept(map.search(3)));
  {
    const Armed<Proxy_Less> armed;
    EXPECT_THROW((void) map.search(4), Comparison_Error);
    EXPECT_THROW((void) map.contains(4), Comparison_Error);
  }
  EXPECT_EQ(map.find(8), -8);

  const Dft_Pair_Cmp<int, int, Proxy_Less> pair_cmp;
  const std::pair<int, int> p1{1, 10}, p2{2, 20};
  const int k = 2;
  EXPECT_FALSE(noexcept(pair_cmp(p1, p2)));
  EXPECT_FALSE(noexcept(pair_cmp(p1, k)));
  EXPECT_FALSE(noexcept(pair_cmp(k, p2)));
  {
    const Armed<Proxy_Less> armed;
    EXPECT_THROW((void) pair_cmp(p1, p2), Comparison_Error);
    EXPECT_THROW((void) pair_cmp(p1, k), Comparison_Error);
    EXPECT_THROW((void) pair_cmp(k, p2), Comparison_Error);
  }

  // a comparison that gives a bool and does not throw keeps the noexcept
  EXPECT_TRUE(noexcept(std::declval<Avl_Tree<int> &>().search(1)));
  EXPECT_TRUE(noexcept(std::declval<const DynMapTree<int, int> &>().search(1)));
  EXPECT_TRUE(noexcept(std::declval<const Dft_Pair_Cmp<int, int, Aleph::less<int>> &>()
                       (p1, p2)));
}

// A result whose conversion to bool as a condition throws, while its
// conversion as a returned value does not: the operations looked at the
// second one, and the first one threw inside noexcept in their ifs. The
// search of a map, which compared pairs, took the second one before
TEST(ThrowingCompareOperations, TheConditionOnTheResultLetsTheExceptionThrough)
{
  using Less = Two_Conversions_Less;
  expect_the_condition_lets_the_exception_through<Avl_Tree<int, Less>>();
  expect_the_condition_lets_the_exception_through<Treap<int, Less>>();
  expect_the_condition_lets_the_exception_through<Avl_Tree_Rk<int, Less>>();
  expect_the_condition_lets_the_exception_through<Treap_Rk<int, Less>>();

  DynMapTree<int, int, Avl_Tree, Less> map;
  for (const int key : {5, 3, 8})
    map.insert(key, -key);
  EXPECT_FALSE(noexcept(map.search(3)));
  {
    const Armed<Less> armed;
    EXPECT_THROW((void) map.search(4), Comparison_Error);
    EXPECT_THROW((void) map.contains(4), Comparison_Error);
  }
  EXPECT_EQ(map.find(8), -8);

  // a comparison that gives a bool and does not throw keeps the noexcept
  EXPECT_TRUE(noexcept(std::declval<const Avl_Tree_Rk<int>::Iterator &>().has_curr()));
  EXPECT_TRUE(noexcept(std::declval<const Treap_Rk<int>::Iterator &>().has_curr()));
}

// The trees compare the key that they search for with the key of a node,
// which is not constant: an overload of the comparison for keys that are
// not constant that throws was called inside noexcept
TEST(ThrowingCompareOperations, AnOverloadForKeysNotConstantLetsTheExceptionThrough)
{
  using Less = Mutable_Key_Less<int>;
  Small_Tree<Avl_Tree<int, Less>> f;
  EXPECT_FALSE(noexcept(f.tree.search(3)));
  {
    const Armed<Less> armed;
    EXPECT_THROW((void) f.tree.search(4), Comparison_Error);
  }
  EXPECT_NE(f.tree.search(8), nullptr);
}

// A map compares its key with those of its pairs as the pairs give them,
// constant: its search by the key called the overload for keys that are
// not constant, which the search by a pair never called
TEST(ThrowingCompareOperations, TheSearchOfAMapComparesConstantKeys)
{
  using Int_Less = Mutable_Key_Less<int>;
  DynMapTree<int, int, Avl_Tree, Int_Less> map;
  for (const int key : {5, 3, 8, 1, 4})
    map.insert(key, -key);
  Int_Less::mutable_calls = 0;
  for (const int key : {1, 3, 4, 5, 8})
    EXPECT_EQ(map.find(key), -key);
  EXPECT_FALSE(map.contains(7));
  EXPECT_EQ(Int_Less::mutable_calls, 0);

  using String_Less = Mutable_Key_Less<std::string>;
  DynMapTree<std::string, int, Avl_Tree, String_Less> names;
  for (const char * name : {"m", "d", "t", "a", "g"})
    names.insert(name, 1);
  String_Less::mutable_calls = 0;
  EXPECT_TRUE(names.contains("g"));
  EXPECT_FALSE(names.contains("h"));
  EXPECT_EQ(String_Less::mutable_calls, 0);
}

// BinTree and the positions of the ranked trees copy the comparison into
// an operation object before they compare: they made that copy inside
// noexcept, and what it threw ended the program
TEST(ThrowingCompareOperations, TheCopyOfTheComparisonLetsTheExceptionThrough)
{
  expect_the_copy_lets_the_exception_through<BinTree<int, Copy_Throw_Less>>();
  expect_the_copy_lets_the_exception_through<Avl_Tree_Rk<int, Copy_Throw_Less>>();
  expect_the_copy_lets_the_exception_through<Rb_Tree_Rk<int, Copy_Throw_Less>>();
  expect_the_copy_lets_the_exception_through<TdRbTreeRk<int, Copy_Throw_Less>>();
  expect_the_copy_lets_the_exception_through<HtdRbTreeRk<int, Copy_Throw_Less>>();
  expect_the_copy_lets_the_exception_through<Treap_Rk<int, Copy_Throw_Less>>();
  expect_the_copy_lets_the_exception_through<Rand_Tree<int, Copy_Throw_Less>>();

  // a comparison whose copy does not throw keeps the noexcept
  EXPECT_TRUE(noexcept(std::declval<BinTree<int> &>().search(1)));
  EXPECT_TRUE(noexcept(std::declval<const Avl_Tree_Rk<int> &>().position(1)));
}

// The iterators of the ranked trees: reset_to_key(), has_curr() and
// operator == were noexcept, and what the comparison threw ended the
// program
TEST(ThrowingCompareOperations, RankedIteratorsLetTheExceptionThrough)
{
  expect_the_iterator_lets_the_exception_through<Avl_Tree_Rk<int, Countdown_Less>>();
  expect_the_iterator_lets_the_exception_through<Rb_Tree_Rk<int, Countdown_Less>>();
  expect_the_iterator_lets_the_exception_through<Treap_Rk<int, Countdown_Less>>();

  // with a comparison that cannot throw, they keep the noexcept
  using Iterator = Avl_Tree_Rk<int>::Iterator;
  EXPECT_TRUE(noexcept(std::declval<const Iterator &>().has_curr()));
  EXPECT_TRUE(noexcept(std::declval<Iterator &>().reset_to_key(1)));
}

// A tree of the user may give a copy of its comparison, as BSTPolicy
// allows: the search of a map compared with a reference to that copy,
// which had died when the search took it
TEST(ThrowingCompareOperations, AMapOnATreeThatGivesACopyOfItsComparison)
{
  DynMapTree<int, int, By_Value_Avl, Tracked_Less> map;
  for (const int key : {5, 3, 8, 1, 4})
    map.insert(key, -key);
  Tracked_Less::calls_on_dead = 0;
  for (const int key : {1, 3, 4, 5, 8})
    {
      const auto * p = map.search(key);
      ASSERT_NE(p, nullptr) << "key " << key;
      EXPECT_EQ(p->second, -key);
    }
  EXPECT_EQ(map.search(7), nullptr);
  EXPECT_EQ(Tracked_Less::calls_on_dead, 0);
}
