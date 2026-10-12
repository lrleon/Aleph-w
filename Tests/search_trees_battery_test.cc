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
 * @file search_trees_battery_test.cc
 * @brief One battery of tests for every binary search tree of the library.
 *
 * Each tree runs next to a `std::multiset` that says what it must hold.
 * After every operation the tree must pass its own `verify()` (the order of
 * the keys and the invariant of its type: AVL balance, red-black colors,
 * heap of priorities, subtree counters) and its keys in order must be the
 * ones of the multiset. Where it applies, `size()`, `search()`, `select()`
 * and `position()` are compared too, and the height is bounded by what the
 * invariant guarantees, which does not depend on `verify()` being right.
 *
 * The sequences of operations are:
 *
 *  - every order of insertion of up to 8 keys (46 233 sequences), and of
 *    up to 7 keys with search_or_insert();
 *  - every order of removal of up to 6 keys, from each tree that some order
 *    of insertion builds;
 *  - every order of removal of 7 keys from a few given trees, and every
 *    sequence of removals by position in the trees that have remove_pos();
 *  - long random mixes of insertions, removals and searches, with and
 *    without repeated keys;
 *  - the orders that usually break a tree: ascending, descending,
 *    alternating between both ends, from the middle outwards, and repeated
 *    keys.
 */

#include "search_trees_test_common.H"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

using namespace Aleph;
using namespace search_trees_test;

using ::testing::AssertionFailure;
using ::testing::AssertionResult;
using ::testing::AssertionSuccess;

namespace
{
  template <class Tree>
  concept Sized_Tree = requires(const Tree & tree) { tree.size(); };

  template <class Tree>
  concept Ranked_Tree = requires(Tree & tree, int key, size_t i)
    {
      tree.select(i);
      tree.position(key);
    };

  template <class Tree>
  concept Removes_By_Position = requires(Tree & tree, size_t i) { tree.remove_pos(i); };

  std::string to_string(const std::vector<int> & keys)
  {
    std::ostringstream out;
    out << '{';
    for (size_t i = 0; i < keys.size(); ++i)
      out << (i == 0 ? "" : ", ") << keys[i];
    out << '}';
    return out.str();
  }

  // The least number of nodes of an AVL tree of height `h`.
  size_t min_avl_nodes(const int h) noexcept
  {
    size_t below = 0, at = 1; // heights 0 and 1
    if (h == 0)
      return 0;
    for (int i = 1; i < h; ++i)
      {
        const size_t next = at + below + 1;
        below = at;
        at = next;
      }
    return at;
  }

  // A tree and the multiset that says what the tree must hold.
  template <class Tree>
  class Checked_Tree
  {
  public:
    using Node = typename Tree::Node;

  private:
    std::optional<Tree> tree;
    std::multiset<int> model;
    std::vector<std::unique_ptr<Node>> nodes; // every node ever made
    std::vector<Node *> spare;                // the ones out of the tree
    std::vector<int> keys;                    // the last traversal
    int height = 0;                           // and the height it saw

    Node * take_node(const int key)
    {
      if (spare.empty())
        {
          nodes.push_back(std::make_unique<Node>(key));
          return nodes.back().get();
        }

      Node * p = spare.back();
      spare.pop_back();
      p->reset();
      KEY(p) = key;
      return p;
    }

    void walk(Node * p, const int depth)
    {
      if (p == Node::NullPtr)
        return;
      if (depth > height)
        height = depth;
      walk(LLINK(p), depth + 1);
      keys.push_back(KEY(p));
      walk(RLINK(p), depth + 1);
    }

    AssertionResult check_ranks()
    {
      if constexpr (Ranked_Tree<Tree>)
        {
          size_t i = 0;
          for (const int key : model)
            {
              Node * p = tree->select(i);
              if (p == nullptr or p == Node::NullPtr or KEY(p) != key)
                return AssertionFailure() << "select(" << i << ") is not the node of key " << key;

              const auto [pos, q] = tree->position(key);
              if (pos < 0 or static_cast<size_t>(pos) >= keys.size() or keys[pos] != key)
                return AssertionFailure() << "position(" << key << ") is " << pos;
              if (q == nullptr or q == Node::NullPtr or KEY(q) != key)
                return AssertionFailure() << "position(" << key << ") did not give its node";
              ++i;
            }

          const int absent = model.empty() ? 0 : *model.rbegin() + 1;
          if (tree->position(absent).first != -1)
            return AssertionFailure() << "position() of the absent key " << absent << " is not -1";
        }

      return AssertionSuccess();
    }

  public:
    explicit Checked_Tree(const unsigned long seed = 1) { restart(seed); }

    // Leaves the tree empty, as if just made with the given seed.
    void restart([[maybe_unused]] const unsigned long seed)
    {
      spare.clear();
      for (auto & p : nodes)
        spare.push_back(p.get());
      model.clear();
      tree.emplace();
      if constexpr (Seeded_Tree<Tree>)
        tree->set_seed(seed);
    }

    [[nodiscard]] size_t size() const noexcept { return model.size(); }

    [[nodiscard]] bool has(const int key) const { return model.count(key) > 0; }

    [[nodiscard]] const std::multiset<int> & expected() const noexcept { return model; }

    // The shape of the tree, and what its nodes say of its balance.
    [[nodiscard]] std::string signature()
    {
      std::string out;
      write_signature(tree->getRoot(), out);
      return out;
    }

    AssertionResult insert(const int key)
    {
      const bool is_new = not has(key);
      Node * p = take_node(key);
      Node * q = tree->insert(p);

      if (not is_new)
        {
          spare.push_back(p);
          if (q != nullptr)
            return AssertionFailure() << "insert(" << key << ") accepted a key that was in the tree";
          return AssertionSuccess();
        }

      if (q != p)
        return AssertionFailure() << "insert(" << key << ") did not return the node of the new key";
      model.insert(key);
      return AssertionSuccess();
    }

    // The insertion that the sets and maps of the library use.
    AssertionResult search_or_insert(const int key)
    {
      const bool is_new = not has(key);
      Node * p = take_node(key);
      Node * q = tree->search_or_insert(p);

      if (not is_new)
        {
          spare.push_back(p);
          if (q == p)
            return AssertionFailure() << "search_or_insert(" << key
                                      << ") inserted a key that was in the tree";
          if (q == nullptr or q == Node::NullPtr or KEY(q) != key)
            return AssertionFailure() << "search_or_insert(" << key
                                      << ") did not return the node that holds the key";
          return AssertionSuccess();
        }

      if (q != p)
        return AssertionFailure() << "search_or_insert(" << key
                                  << ") did not return the node of the new key";
      model.insert(key);
      return AssertionSuccess();
    }

    AssertionResult insert_dup(const int key)
    {
      Node * p = take_node(key);
      if (tree->insert_dup(p) != p)
        return AssertionFailure() << "insert_dup(" << key << ") did not return the node inserted";
      model.insert(key);
      return AssertionSuccess();
    }

    AssertionResult remove(const int key)
    {
      const auto it = model.find(key);
      Node * p = tree->remove(key);

      if (it == model.end())
        {
          if (p != nullptr)
            return AssertionFailure() << "remove(" << key << ") gave a node for an absent key";
          return AssertionSuccess();
        }

      if (p == nullptr or p == Node::NullPtr)
        return AssertionFailure() << "remove(" << key << ") did not find a key of the tree";
      if (KEY(p) != key)
        return AssertionFailure() << "remove(" << key << ") gave the node of key " << KEY(p);

      model.erase(it);
      spare.push_back(p);
      return AssertionSuccess();
    }

    // Removes the key at the position `i` of the order, in the trees that
    // can do it.
    AssertionResult remove_pos(const size_t i)
    {
      if constexpr (Removes_By_Position<Tree>)
        {
          const auto it = std::next(model.begin(), static_cast<std::ptrdiff_t>(i));
          Node * p = tree->remove_pos(i);

          if (p == nullptr or p == Node::NullPtr)
            return AssertionFailure() << "remove_pos(" << i << ") gave no node";
          if (KEY(p) != *it)
            return AssertionFailure() << "remove_pos(" << i << ") gave the node of key " << KEY(p)
                                      << " and the key there is " << *it;

          model.erase(it);
          spare.push_back(p);
          return AssertionSuccess();
        }
      else
        return AssertionFailure() << "the tree has no remove_pos(" << i << ")";
    }

    AssertionResult search(const int key)
    {
      Node * p = tree->search(key);

      if (not has(key))
        {
          if (p != nullptr)
            return AssertionFailure() << "search(" << key << ") gave a node for an absent key";
          return AssertionSuccess();
        }

      if (p == nullptr or p == Node::NullPtr or KEY(p) != key)
        return AssertionFailure() << "search(" << key << ") did not find a key of the tree";
      return AssertionSuccess();
    }

    // The invariants of the tree and its keys in order.
    AssertionResult check()
    {
      Node * root = tree->getRoot();

      if (not tree->verify())
        return AssertionFailure() << "verify() is false";

      keys.clear();
      height = 0;
      walk(root, 1);
      if (keys.size() != model.size() or not std::equal(keys.begin(), keys.end(), model.begin()))
        return AssertionFailure() << "the tree holds " << to_string(keys) << " and it must hold "
                                  << to_string(std::vector<int>(model.begin(), model.end()));

      if constexpr (Sized_Tree<Tree>)
        if (tree->size() != model.size())
          return AssertionFailure() << "size() is " << tree->size() << " with " << model.size()
                                    << " keys";

      if constexpr (Avl_Balanced_Node<Node>)
        if (keys.size() < min_avl_nodes(height))
          return AssertionFailure() << "height " << height << " is too much for an AVL tree of "
                                    << keys.size() << " nodes";

      if constexpr (Colored_Node<Node>)
        {
          // A path has no two red nodes in a row, so half of the height is
          // black, and a black height of b takes 2^b - 1 nodes at least.
          // The root can be red: these trees do not force it to be black,
          // which does not change the bound
          if (keys.size() + 1 < (size_t(1) << (height / 2)))
            return AssertionFailure() << "height " << height
                                      << " is too much for a red-black tree of " << keys.size()
                                      << " nodes";
        }

      return AssertionSuccess();
    }

    // check(), and then every key is found where it must be. Searching
    // restructures a self-adjusting tree, so the invariants are checked
    // again at the end.
    AssertionResult deep_check()
    {
      if (auto ok = check(); not ok)
        return ok;

      if (not model.empty())
        if (auto ok = search(*model.begin() - 1); not ok)
          return ok;

      for (auto it = model.begin(); it != model.end(); it = model.upper_bound(*it))
        {
          if (auto ok = search(*it); not ok)
            return ok;
          if (not has(*it + 1))
            if (auto ok = search(*it + 1); not ok)
              return ok;
        }

      if (auto ok = check_ranks(); not ok)
        return ok;

      return check();
    }

  private:
    static void write_signature(Node * p, std::string & out)
    {
      if (p == Node::NullPtr)
        {
          out += '.';
          return;
        }

      out += std::to_string(KEY(p));
      if constexpr (Colored_Node<Node>)
        out += COLOR(p) == RED ? 'r' : 'b';
      if constexpr (Avl_Balanced_Node<Node>)
        out += DIFF(p) < 0 ? '-' : DIFF(p) > 0 ? '+' : '=';
      out += '(';
      write_signature(LLINK(p), out);
      write_signature(RLINK(p), out);
      out += ')';
    }
  };

  // What is checked after each operation of a sequence: the invariants and
  // the keys in order, or also the search of every key.
  enum class Checks { invariants, searches_too };

  template <class Tree>
  AssertionResult checked(Checked_Tree<Tree> & tree, const Checks checks)
  {
    return checks == Checks::searches_too ? tree.deep_check() : tree.check();
  }

  // Inserts the keys, with a check after each one.
  template <class Tree>
  AssertionResult insert_all(Checked_Tree<Tree> & tree, const std::vector<int> & order,
                             const Checks checks)
  {
    for (const int key : order)
      {
        if (auto ok = tree.insert(key); not ok)
          return ok;
        if (auto ok = checked(tree, checks); not ok)
          return ok << " after inserting " << key;
      }
    return AssertionSuccess();
  }

  // Removes the keys, with a check after each one; a key just removed is no
  // longer there to be removed.
  template <class Tree>
  AssertionResult remove_all(Checked_Tree<Tree> & tree, const std::vector<int> & order,
                             const Checks checks)
  {
    for (const int key : order)
      {
        if (auto ok = tree.remove(key); not ok)
          return ok;
        if (auto ok = checked(tree, checks); not ok)
          return ok << " after removing " << key;
        if (tree.has(key))
          continue; // another node with the same key remains
        if (auto ok = tree.remove(key); not ok)
          return ok << " the second time";
        if (auto ok = tree.check(); not ok)
          return ok << " after removing " << key << " again";
      }
    return AssertionSuccess();
  }

  // The keys 1 ... n in the orders that degrade a tree that does not
  // balance itself, or that push a balanced one always to the same side.
  std::vector<std::pair<std::string, std::vector<int>>> adverse_orders(const int n)
  {
    std::vector<int> ascending(n);
    std::iota(ascending.begin(), ascending.end(), 1);

    std::vector<int> descending(ascending.rbegin(), ascending.rend());

    std::vector<int> from_both_ends; // 1, n, 2, n - 1, ...
    for (int lo = 1, hi = n; lo <= hi; ++lo, --hi)
      {
        from_both_ends.push_back(lo);
        if (lo != hi)
          from_both_ends.push_back(hi);
      }

    // n/2, n/2 + 1, n/2 - 1, ...
    std::vector<int> from_the_middle(from_both_ends.rbegin(), from_both_ends.rend());

    std::vector<int> by_halves; // the root of a perfect tree, then its children, ...
    int top = 1;
    while (2 * top <= n)
      top *= 2;
    std::vector<bool> used(n + 1, false);
    for (int step = top; step >= 1; step /= 2)
      for (int key = step; key <= n; key += step)
        if (not used[key])
          {
            used[key] = true;
            by_halves.push_back(key);
          }

    std::vector<int> shuffled(ascending);
    std::mt19937 rng(20260710);
    for (size_t i = shuffled.size(); i > 1; --i)
      std::swap(shuffled[i - 1], shuffled[rng() % i]);

    return {
      {"ascending", ascending},
      {"descending", descending},
      {"from both ends", from_both_ends},
      {"from the middle", from_the_middle},
      {"by halves", by_halves},
      {"shuffled", shuffled}};
  }

  template <class Tree>
  class SearchTreeBattery : public ::testing::Test
  {
  protected:
    Checked_Tree<Tree> tree;
  };

  TYPED_TEST_SUITE(SearchTreeBattery, All_Trees, Tree_Names);

  template <class Tree>
  class RankedTreeBattery : public ::testing::Test {};

  TYPED_TEST_SUITE(RankedTreeBattery, Ranked_Trees, Tree_Names);

  template <class Tree>
  class PositionalRemovalBattery : public ::testing::Test
  {
  protected:
    Checked_Tree<Tree> tree;
  };

  // The trees with remove_pos().
  using Positional_Trees =
    ::testing::Types<TdRbTreeRk<int>, HtdRbTreeRk<int>, Treap_Rk<int>, Rand_Tree<int>>;

  TYPED_TEST_SUITE(PositionalRemovalBattery, Positional_Trees, Tree_Names);
}

// ---------------------------------------------------------------------------
// Every sequence, in small
// ---------------------------------------------------------------------------

TYPED_TEST(SearchTreeBattery, EveryInsertionOrderOfUpToEightKeys)
{
  auto & tree = this->tree;
  unsigned long sequences = 0;

  for (int n = 1; n <= 8; ++n)
    {
      std::vector<int> order(n);
      std::iota(order.begin(), order.end(), 1);

      do
        {
          tree.restart(++sequences);
          ASSERT_TRUE(insert_all(tree, order, Checks::invariants))
            << "inserting " << to_string(order);
          ASSERT_TRUE(tree.deep_check()) << "after inserting " << to_string(order);

          // A key that is in the tree is refused, and nothing changes
          ASSERT_TRUE(tree.insert(order.front())) << "after " << to_string(order);
          ASSERT_TRUE(tree.insert(order.back())) << "after " << to_string(order);
          ASSERT_TRUE(tree.check()) << "after " << to_string(order);
        }
      while (std::next_permutation(order.begin(), order.end()));
    }

  EXPECT_EQ(sequences, 46233ul); // 1! + 2! + ... + 8!
}

// search_or_insert() is how DynSetTree and DynMapTree insert, and each
// tree has its own code for it.
TYPED_TEST(SearchTreeBattery, EveryOrderOfSearchOrInsertOfUpToSevenKeys)
{
  auto & tree = this->tree;
  unsigned long sequences = 0;

  for (int n = 1; n <= 7; ++n)
    {
      std::vector<int> order(n);
      std::iota(order.begin(), order.end(), 1);

      do
        {
          tree.restart(++sequences);
          for (const int key : order)
            {
              ASSERT_TRUE(tree.search_or_insert(key)) << "inserting " << to_string(order);
              ASSERT_TRUE(tree.check())
                << "after inserting " << key << " of " << to_string(order);
            }
          ASSERT_TRUE(tree.deep_check()) << "after inserting " << to_string(order);

          // A key that is in the tree is found, and nothing is inserted
          ASSERT_TRUE(tree.search_or_insert(order.front())) << "after " << to_string(order);
          ASSERT_TRUE(tree.search_or_insert(order.back())) << "after " << to_string(order);
          ASSERT_TRUE(tree.check()) << "after " << to_string(order);
        }
      while (std::next_permutation(order.begin(), order.end()));
    }

  EXPECT_EQ(sequences, 5913ul); // 1! + 2! + ... + 7!
}

TYPED_TEST(SearchTreeBattery, EveryRemovalOrderFromEveryTreeOfUpToSixKeys)
{
  auto & tree = this->tree;
  unsigned long seed = 0;
  size_t shapes = 0;

  for (int n = 1; n <= 6; ++n)
    {
      std::set<std::string> seen;
      std::vector<int> insertion(n);
      std::iota(insertion.begin(), insertion.end(), 1);

      do
        {
          // The same seed builds the same tree again for each removal order
          tree.restart(++seed);
          for (const int key : insertion)
            ASSERT_TRUE(tree.insert(key));
          if (not seen.insert(tree.signature()).second)
            continue; // these removals were already done from this tree

          std::vector<int> removal(n);
          std::iota(removal.begin(), removal.end(), 1);
          do
            {
              tree.restart(seed);
              for (const int key : insertion)
                ASSERT_TRUE(tree.insert(key));

              ASSERT_TRUE(remove_all(tree, removal, Checks::invariants))
                << "after inserting " << to_string(insertion) << ", removing "
                << to_string(removal);
              ASSERT_EQ(tree.size(), 0u);
            }
          while (std::next_permutation(removal.begin(), removal.end()));
        }
      while (std::next_permutation(insertion.begin(), insertion.end()));

      shapes += seen.size();
    }

  EXPECT_GE(shapes, 6u);
  this->RecordProperty("trees", static_cast<int>(shapes));
}

TYPED_TEST(SearchTreeBattery, EveryRemovalOrderOfSevenKeysFromGivenTrees)
{
  auto & tree = this->tree;
  const int n = 7;
  unsigned long seed = 0;

  for (const auto & [name, insertion] : adverse_orders(n))
    {
      std::vector<int> removal(n);
      std::iota(removal.begin(), removal.end(), 1);
      do
        {
          tree.restart(++seed);
          for (const int key : insertion)
            ASSERT_TRUE(tree.insert(key));

          ASSERT_TRUE(remove_all(tree, removal, Checks::invariants))
            << "after inserting " << to_string(insertion) << " (" << name << "), removing "
            << to_string(removal);
        }
      while (std::next_permutation(removal.begin(), removal.end()));
    }
}

// ---------------------------------------------------------------------------
// Long random sequences
// ---------------------------------------------------------------------------

namespace
{
  // `num_ops` random operations on keys of [0, universe): first mostly
  // insertions, then as many insertions as removals, then mostly removals.
  // Then the tree is emptied.
  template <class Tree>
  AssertionResult random_operations(Checked_Tree<Tree> & tree, const unsigned seed,
                                    const int universe, const int num_ops,
                                    const bool repeated_keys)
  {
    std::mt19937 rng(seed);
    tree.restart(seed);

    for (int i = 0; i < num_ops; ++i)
      {
        const int key = static_cast<int>(rng() % universe);
        const unsigned dice = rng() % 100;
        const unsigned inserts = i < num_ops / 3 ? 60 : i < 2 * num_ops / 3 ? 40 : 20;

        AssertionResult done = AssertionSuccess();
        const char * what = "searching";
        if (dice < inserts)
          {
            what = "inserting";
            const unsigned how = rng() % 4;
            done = repeated_keys and how < 2 ? tree.insert_dup(key) :
                   how == 3 ? tree.search_or_insert(key) : tree.insert(key);
          }
        else if (dice < 80)
          {
            what = "removing";
            if (Removes_By_Position<Tree> and tree.size() > 0 and rng() % 4 == 0)
              {
                what = "removing the key at position";
                done = tree.remove_pos(key % tree.size());
              }
            else
              done = tree.remove(key);
          }
        else
          done = tree.search(key);

        if (not done)
          return done << " (operation " << i << ", seed " << seed << ")";

        if (auto ok = i % 61 == 0 ? tree.deep_check() : tree.check(); not ok)
          return ok << " after " << what << ' ' << key << " (operation " << i << ", seed "
                    << seed << ")";
      }

    std::vector<int> rest(tree.expected().begin(), tree.expected().end());
    for (size_t i = rest.size(); i > 1; --i)
      std::swap(rest[i - 1], rest[rng() % i]);

    for (const int key : rest)
      {
        if (auto ok = tree.remove(key); not ok)
          return ok << " (emptying, seed " << seed << ")";
        if (auto ok = tree.check(); not ok)
          return ok << " after removing " << key << " (emptying, seed " << seed << ")";
      }

    if (tree.size() != 0)
      return AssertionFailure() << "the tree was not emptied";
    return tree.deep_check();
  }
}

// Few keys: most insertions find the key and many removals do not.
TYPED_TEST(SearchTreeBattery, RandomOperationsOnFewKeys)
{
  for (const unsigned seed : {1u, 2u, 3u, 4u})
    ASSERT_TRUE(random_operations(this->tree, seed, 24, 6000, false));
}

TYPED_TEST(SearchTreeBattery, RandomOperationsOnManyKeys)
{
  for (const unsigned seed : {11u, 12u})
    ASSERT_TRUE(random_operations(this->tree, seed, 400, 5000, false));
  ASSERT_TRUE(random_operations(this->tree, 13u, 100000, 2500, false));
}

TYPED_TEST(SearchTreeBattery, RandomOperationsWithRepeatedKeys)
{
  for (const unsigned seed : {21u, 22u})
    ASSERT_TRUE(random_operations(this->tree, seed, 12, 4000, true));
  ASSERT_TRUE(random_operations(this->tree, 23u, 300, 4000, true));
}

// ---------------------------------------------------------------------------
// The orders that usually break a tree
// ---------------------------------------------------------------------------

TYPED_TEST(SearchTreeBattery, AdverseInsertionAndRemovalOrders)
{
  auto & tree = this->tree;
  const auto orders = adverse_orders(96);
  unsigned long seed = 0;

  for (const auto & [inserting, insertion] : orders)
    for (const auto & [removing, removal] : orders)
      {
        tree.restart(++seed);
        ASSERT_TRUE(insert_all(tree, insertion, Checks::searches_too))
          << "inserting " << inserting;
        ASSERT_TRUE(remove_all(tree, removal, Checks::searches_too))
          << "removing " << removing << " after inserting " << inserting;
        ASSERT_EQ(tree.size(), 0u);
      }
}

TYPED_TEST(SearchTreeBattery, LongAscendingAndDescendingRuns)
{
  auto & tree = this->tree;
  const int n = 1000;

  for (const bool ascending : {true, false})
    {
      tree.restart(ascending ? 1 : 2);
      for (int i = 1; i <= n; ++i)
        {
          ASSERT_TRUE(tree.insert(ascending ? i : n + 1 - i));
          if (i % 50 == 0)
            {
              ASSERT_TRUE(tree.check()) << "after " << i << " insertions";
            }
        }
      ASSERT_TRUE(tree.deep_check());

      for (int i = 1; i <= n; ++i)
        {
          ASSERT_TRUE(tree.remove(ascending ? i : n + 1 - i));
          if (i % 50 == 0)
            {
              ASSERT_TRUE(tree.check()) << "after " << i << " removals";
            }
        }
      ASSERT_EQ(tree.size(), 0u);
    }
}

TYPED_TEST(SearchTreeBattery, ManyNodesWithTheSameKey)
{
  auto & tree = this->tree;
  const int n = 64;

  for (int i = 0; i < n; ++i)
    {
      ASSERT_TRUE(tree.insert_dup(7));
      ASSERT_TRUE(tree.deep_check()) << "after " << i + 1 << " insertions of the same key";
    }
  ASSERT_TRUE(tree.insert(7)); // refused: the key is there
  ASSERT_TRUE(tree.check());

  for (int i = 0; i < n; ++i)
    {
      ASSERT_TRUE(tree.remove(7));
      ASSERT_TRUE(tree.deep_check()) << "after " << i + 1 << " removals of the same key";
    }
  ASSERT_EQ(tree.size(), 0u);
  ASSERT_TRUE(tree.remove(7)); // nothing left to remove
}

TYPED_TEST(SearchTreeBattery, FewKeysRepeatedManyTimes)
{
  auto & tree = this->tree;
  const auto orders = adverse_orders(60);
  unsigned long seed = 0;

  for (const auto & [inserting, insertion] : orders)
    for (const auto & [removing, removal] : orders)
      {
        tree.restart(++seed);
        for (const int i : insertion)
          {
            ASSERT_TRUE(tree.insert_dup(i % 5));
            ASSERT_TRUE(tree.deep_check()) << "inserting " << inserting;
          }
        for (const int i : removal)
          {
            ASSERT_TRUE(tree.remove(i % 5));
            ASSERT_TRUE(tree.deep_check())
              << "removing " << removing << " after inserting " << inserting;
          }
        ASSERT_EQ(tree.size(), 0u);
      }
}

// ---------------------------------------------------------------------------
// Access by position
// ---------------------------------------------------------------------------

// Every way of emptying a tree of up to 7 keys by positions: the first
// position is any of the n, the second any of the n - 1 left, and so on.
TYPED_TEST(PositionalRemovalBattery, EverySequenceOfPositionsOfUpToSevenKeys)
{
  auto & tree = this->tree;
  unsigned long seed = 0;

  for (int n = 1; n <= 7; ++n)
    for (const auto & [name, insertion] : adverse_orders(n))
      {
        std::vector<size_t> positions(n, 0); // the i-th one is less than n - i
        while (true)
          {
            tree.restart(++seed);
            for (const int key : insertion)
              ASSERT_TRUE(tree.insert(key));

            for (const size_t i : positions)
              {
                ASSERT_TRUE(tree.remove_pos(i)) << "inserting " << name << ", " << n << " keys";
                ASSERT_TRUE(tree.check())
                  << "after removing the position " << i << ", inserting " << name << ", " << n
                  << " keys";
              }
            ASSERT_EQ(tree.size(), 0u);

            // The next sequence, as a counter whose digit i runs up to n - i
            int digit = n - 1;
            while (digit >= 0 and positions[digit] + 1 >= static_cast<size_t>(n - digit))
              positions[digit--] = 0;
            if (digit < 0)
              break;
            ++positions[digit];
          }
      }
}

TYPED_TEST(PositionalRemovalBattery, RemovePosBeyondTheLastKeyThrows)
{
  TypeParam tree;
  Node_Pool<typename TypeParam::Node> pool;

  for (const int key : {5, 3, 8, 1, 4})
    ASSERT_NE(tree.insert(pool.make(key)), nullptr);

  EXPECT_THROW(tree.remove_pos(5), std::out_of_range);
  EXPECT_TRUE(tree.verify());
  EXPECT_EQ(tree.size(), 5u);

  auto * removed = tree.remove_pos(4);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(KEY(removed), 8);
  EXPECT_TRUE(tree.verify());
  EXPECT_EQ(tree.size(), 4u);
}

TYPED_TEST(RankedTreeBattery, SelectBeyondTheLastKeyThrows)
{
  TypeParam tree;
  Node_Pool<typename TypeParam::Node> pool;

  EXPECT_THROW(tree.select(0), std::out_of_range);

  for (const int key : {5, 3, 8, 1, 4})
    ASSERT_NE(tree.insert(pool.make(key)), nullptr);

  EXPECT_EQ(KEY(tree.select(4)), 8);
  EXPECT_THROW(tree.select(5), std::out_of_range);
  EXPECT_THROW(tree.select(100), std::out_of_range);
  EXPECT_TRUE(tree.verify());
}
