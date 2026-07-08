
/*
                          Aleph_w

  Data structures & Algorithms
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
 * @file radix_tree_test.cc
 * @brief Tests for Aleph::RadixTree (tpl_radix_tree.H).
 */

#include <gtest/gtest.h>

#include <prefix-tree.H>
#include <tpl_radix_tree.H>

#include <algorithm>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Aleph;

namespace
{
  std::vector<std::string> sorted(std::vector<std::string> v)
  {
    std::sort(v.begin(), v.end());
    return v;
  }

  std::vector<std::string> to_vector(const Array<std::string> &a)
  {
    std::vector<std::string> v;
    for (const auto &s : a)
      v.push_back(s);
    return v;
  }

  std::vector<std::string> to_vector(const DynArray<std::string> &a)
  {
    std::vector<std::string> v;
    a.for_each([&v](const std::string &s) { v.push_back(s); });
    return v;
  }

  struct Throwing_Copy
  {
    static bool should_throw;
    int value;

    explicit Throwing_Copy(int v) : value(v) {}
    Throwing_Copy(const Throwing_Copy &other) : value(other.value)
    {
      if (should_throw)
        throw std::runtime_error("Throwing_Copy: copy failed");
    }
    Throwing_Copy(Throwing_Copy &&) = default;
    Throwing_Copy &operator=(const Throwing_Copy &) = default;
    Throwing_Copy &operator=(Throwing_Copy &&) = default;
  };

  bool Throwing_Copy::should_throw = false;
}  // namespace

TEST(RadixTree, DefaultConstructedTreeIsEmpty)
{
  RadixTree<int> t;
  EXPECT_TRUE(t.is_empty());
  EXPECT_EQ(t.size(), 0u);
  EXPECT_FALSE(t.contains("anything"));
  EXPECT_EQ(t.find("anything"), nullptr);
}

TEST(RadixTree, CheckInvariantsHoldsForEmptyTree)
{
  RadixTree<int> t;
  EXPECT_TRUE(t.check_invariants());
}

TEST(RadixTree, InsertAndFindSingleKey)
{
  RadixTree<int> t;
  EXPECT_TRUE(t.insert("apple", 1));
  EXPECT_EQ(t.size(), 1u);
  ASSERT_NE(t.find("apple"), nullptr);
  EXPECT_EQ(*t.find("apple"), 1);
  EXPECT_TRUE(t.contains("apple"));
  EXPECT_FALSE(t.contains("app"));
  EXPECT_FALSE(t.contains("applesauce"));
}

TEST(RadixTree, InsertRejectsDuplicateKeys)
{
  RadixTree<int> t;
  EXPECT_TRUE(t.insert("apple", 1));
  EXPECT_FALSE(t.insert("apple", 2));
  ASSERT_NE(t.find("apple"), nullptr);
  EXPECT_EQ(*t.find("apple"), 1);  // unchanged
  EXPECT_EQ(t.size(), 1u);
}

TEST(RadixTree, InsertMoveOverloadIsSupported)
{
  RadixTree<std::unique_ptr<int>> t;
  EXPECT_TRUE(t.insert("k", std::make_unique<int>(42)));
  ASSERT_NE(t.find("k"), nullptr);
  ASSERT_NE(*t.find("k"), nullptr);
  EXPECT_EQ(**t.find("k"), 42);
}

TEST(RadixTree, InsertOrAssignInsertsThenOverwrites)
{
  RadixTree<int> t;
  t.insert_or_assign("k", 1);
  ASSERT_NE(t.find("k"), nullptr);
  EXPECT_EQ(*t.find("k"), 1);
  EXPECT_EQ(t.size(), 1u);

  t.insert_or_assign("k", 2);
  ASSERT_NE(t.find("k"), nullptr);
  EXPECT_EQ(*t.find("k"), 2);
  EXPECT_EQ(t.size(), 1u);  // still one key
}

TEST(RadixTree, EmptyStringKeyIsSupported)
{
  RadixTree<int> t;
  EXPECT_TRUE(t.insert("", 7));
  EXPECT_TRUE(t.contains(""));
  ASSERT_NE(t.find(""), nullptr);
  EXPECT_EQ(*t.find(""), 7);
  EXPECT_EQ(t.size(), 1u);

  EXPECT_TRUE(t.insert("a", 1));
  EXPECT_EQ(t.size(), 2u);
  EXPECT_TRUE(t.contains(""));
  EXPECT_TRUE(t.contains("a"));

  EXPECT_TRUE(t.erase(""));
  EXPECT_FALSE(t.contains(""));
  EXPECT_TRUE(t.contains("a"));
}

TEST(RadixTree, EraseReturnsFalseForMissingKey)
{
  RadixTree<int> t;
  t.insert("apple", 1);
  EXPECT_FALSE(t.erase("banana"));
  EXPECT_FALSE(t.erase("app"));       // prefix of a key, not a key itself
  EXPECT_FALSE(t.erase("applesauce"));  // key extends past a stored one
  EXPECT_EQ(t.size(), 1u);
}

TEST(RadixTree, EraseRemovesKeyAndShrinksSize)
{
  RadixTree<int> t;
  t.insert("apple", 1);
  t.insert("app", 2);
  EXPECT_TRUE(t.erase("apple"));
  EXPECT_FALSE(t.contains("apple"));
  EXPECT_TRUE(t.contains("app"));
  EXPECT_EQ(t.size(), 1u);
  EXPECT_TRUE(t.check_invariants());
}

// --- Edge-splitting / compression-specific scenarios -----------------------

TEST(RadixTree, InsertingSharedPrefixSplitsEdgeCorrectly)
{
  RadixTree<int> t;
  ASSERT_TRUE(t.insert("romane", 1));
  ASSERT_TRUE(t.insert("romanus", 2));
  ASSERT_TRUE(t.insert("romulus", 3));
  ASSERT_TRUE(t.insert("rubens", 4));
  ASSERT_TRUE(t.insert("ruber", 5));
  ASSERT_TRUE(t.insert("rubicon", 6));
  ASSERT_TRUE(t.insert("rubicundus", 7));

  EXPECT_EQ(t.size(), 7u);
  for (const auto &[key, expected] :
       std::vector<std::pair<std::string, int>>{
           {"romane", 1}, {"romanus", 2}, {"romulus", 3}, {"rubens", 4},
           {"ruber", 5}, {"rubicon", 6}, {"rubicundus", 7}})
    {
      ASSERT_NE(t.find(key), nullptr) << "missing key: " << key;
      EXPECT_EQ(*t.find(key), expected) << "wrong value for key: " << key;
    }

  // Prefixes that were never inserted must not be considered present.
  EXPECT_FALSE(t.contains("rom"));
  EXPECT_FALSE(t.contains("rub"));
  EXPECT_FALSE(t.contains("ru"));
  EXPECT_TRUE(t.check_invariants());
}

TEST(RadixTree, InsertingKeyThatIsPrefixOfExistingKeySplitsWithoutExtraLeaf)
{
  RadixTree<int> t;
  ASSERT_TRUE(t.insert("romane", 1));
  ASSERT_TRUE(t.insert("roman", 2));  // prefix of "romane"

  EXPECT_EQ(t.size(), 2u);
  ASSERT_NE(t.find("roman"), nullptr);
  EXPECT_EQ(*t.find("roman"), 2);
  ASSERT_NE(t.find("romane"), nullptr);
  EXPECT_EQ(*t.find("romane"), 1);
  EXPECT_FALSE(t.contains("roma"));
  EXPECT_TRUE(t.check_invariants());
}

TEST(RadixTree, InsertingKeyThatExtendsExistingKeyAddsLeafUnderIt)
{
  RadixTree<int> t;
  ASSERT_TRUE(t.insert("roman", 1));
  ASSERT_TRUE(t.insert("romane", 2));  // extends "roman"

  EXPECT_EQ(t.size(), 2u);
  ASSERT_NE(t.find("roman"), nullptr);
  EXPECT_EQ(*t.find("roman"), 1);
  ASSERT_NE(t.find("romane"), nullptr);
  EXPECT_EQ(*t.find("romane"), 2);
  EXPECT_TRUE(t.check_invariants());
}

TEST(RadixTree, EraseMergesCompressedNodeBackTogether)
{
  RadixTree<int> t;
  ASSERT_TRUE(t.insert("test", 1));
  ASSERT_TRUE(t.insert("team", 2));   // splits "te" | "st"/"am"
  ASSERT_TRUE(t.insert("toast", 3));  // splits "t" | "e.."/"oast"

  ASSERT_EQ(t.size(), 3u);

  // Removing "team" should leave "test" reachable and merge any now-single-
  // child, valueless intermediate node the deletion exposes.
  EXPECT_TRUE(t.erase("team"));
  EXPECT_TRUE(t.check_invariants());
  EXPECT_FALSE(t.contains("team"));
  ASSERT_NE(t.find("test"), nullptr);
  EXPECT_EQ(*t.find("test"), 1);
  ASSERT_NE(t.find("toast"), nullptr);
  EXPECT_EQ(*t.find("toast"), 3);
  EXPECT_EQ(t.size(), 2u);

  // Continue removing until the tree is empty, confirming compression
  // merges never corrupt subsequent lookups.
  EXPECT_TRUE(t.erase("test"));
  EXPECT_TRUE(t.check_invariants());
  ASSERT_NE(t.find("toast"), nullptr);
  EXPECT_EQ(*t.find("toast"), 3);
  EXPECT_TRUE(t.erase("toast"));
  EXPECT_TRUE(t.check_invariants());
  EXPECT_TRUE(t.is_empty());
  EXPECT_EQ(t.size(), 0u);
}

TEST(RadixTree, EraseOfInternalNodeWithValueKeepsChildrenReachable)
{
  RadixTree<int> t;
  ASSERT_TRUE(t.insert("roman", 1));
  ASSERT_TRUE(t.insert("romane", 2));
  ASSERT_TRUE(t.insert("romanus", 3));

  // "roman" is an internal node (has children "e" and "us") that also
  // holds a value; erasing it must not disturb its children.
  EXPECT_TRUE(t.erase("roman"));
  EXPECT_TRUE(t.check_invariants());
  EXPECT_FALSE(t.contains("roman"));
  ASSERT_NE(t.find("romane"), nullptr);
  EXPECT_EQ(*t.find("romane"), 2);
  ASSERT_NE(t.find("romanus"), nullptr);
  EXPECT_EQ(*t.find("romanus"), 3);
  EXPECT_EQ(t.size(), 2u);
}

// --- longest_prefix ----------------------------------------------------

TEST(RadixTree, LongestPrefixFindsDeepestMatchingStoredKey)
{
  RadixTree<int> t;
  t.insert("a", 1);
  t.insert("ab", 2);
  t.insert("abc", 3);

  EXPECT_EQ(t.longest_prefix("abcd"), std::optional<std::string>("abc"));
  EXPECT_EQ(t.longest_prefix("abc"), std::optional<std::string>("abc"));
  EXPECT_EQ(t.longest_prefix("ab"), std::optional<std::string>("ab"));
  EXPECT_EQ(t.longest_prefix("a"), std::optional<std::string>("a"));
  EXPECT_EQ(t.longest_prefix(""), std::nullopt);
  EXPECT_EQ(t.longest_prefix("xyz"), std::nullopt);
}

TEST(RadixTree, LongestPrefixMatchesEmptyStringKeyAsFallback)
{
  RadixTree<int> t;
  t.insert("", 0);
  t.insert("ab", 2);

  EXPECT_EQ(t.longest_prefix("abc"), std::optional<std::string>("ab"));
  EXPECT_EQ(t.longest_prefix("xyz"), std::optional<std::string>(""));
  EXPECT_EQ(t.longest_prefix(""), std::optional<std::string>(""));
}

// --- keys_with_prefix ----------------------------------------------------

TEST(RadixTree, KeysWithPrefixReturnsAllMatchingKeys)
{
  RadixTree<int> t;
  for (const auto &k :
       {"romane", "romanus", "romulus", "rubens", "ruber", "rubicon"})
    t.insert(k, 0);

  EXPECT_EQ(sorted(to_vector(t.keys_with_prefix("rom"))),
            sorted({"romane", "romanus", "romulus"}));
  EXPECT_EQ(sorted(to_vector(t.keys_with_prefix("rub"))),
            sorted({"rubens", "ruber", "rubicon"}));
  EXPECT_EQ(sorted(to_vector(t.keys_with_prefix("r"))),
            sorted({"romane", "romanus", "romulus", "rubens", "ruber",
                    "rubicon"}));
  EXPECT_TRUE(to_vector(t.keys_with_prefix("z")).empty());
  EXPECT_TRUE(to_vector(t.keys_with_prefix("rom-nope")).empty());
}

TEST(RadixTree, KeysWithPrefixEmptyPrefixReturnsEverything)
{
  RadixTree<int> t;
  t.insert("a", 1);
  t.insert("b", 2);
  t.insert("", 3);

  EXPECT_EQ(sorted(to_vector(t.keys_with_prefix(""))), sorted({"", "a", "b"}));
}

TEST(RadixTree, KeysWithPrefixExactKeyIncludesItself)
{
  RadixTree<int> t;
  t.insert("roman", 1);
  t.insert("romane", 2);

  EXPECT_EQ(sorted(to_vector(t.keys_with_prefix("roman"))),
            sorted({"roman", "romane"}));
}

// --- Copy / move semantics ------------------------------------------------

TEST(RadixTree, MoveConstructorTransfersOwnershipLeavesSourceEmpty)
{
  RadixTree<int> a;
  a.insert("x", 1);
  a.insert("y", 2);

  RadixTree<int> b(std::move(a));
  EXPECT_EQ(b.size(), 2u);
  ASSERT_NE(b.find("x"), nullptr);
  EXPECT_EQ(*b.find("x"), 1);

  EXPECT_TRUE(a.is_empty());  // NOLINT(bugprone-use-after-move): documented
  EXPECT_EQ(a.size(), 0u);
  EXPECT_TRUE(a.insert("z", 3));  // moved-from tree stays usable
}

TEST(RadixTree, CopyConstructorProducesIndependentDeepCopy)
{
  RadixTree<int> a;
  a.insert("x", 1);
  a.insert("xy", 2);

  RadixTree<int> b(a);
  EXPECT_EQ(b.size(), a.size());
  ASSERT_NE(b.find("xy"), nullptr);
  EXPECT_EQ(*b.find("xy"), 2);

  // Mutating the copy must not affect the original, and vice versa.
  b.insert_or_assign("xy", 99);
  EXPECT_EQ(*b.find("xy"), 99);
  EXPECT_EQ(*a.find("xy"), 2);

  a.erase("x");
  EXPECT_FALSE(a.contains("x"));
  EXPECT_TRUE(b.contains("x"));
}

TEST(RadixTree, FailedInsertCopyLeavesTreeUnchanged)
{
  RadixTree<Throwing_Copy> t;
  t.insert("a", Throwing_Copy(1));
  EXPECT_EQ(t.size(), 1u);

  Throwing_Copy::should_throw = true;
  const Throwing_Copy value(2);
  EXPECT_THROW(t.insert("b", value), std::runtime_error);
  Throwing_Copy::should_throw = false;

  EXPECT_EQ(t.size(), 1u);
  EXPECT_FALSE(t.contains("b"));
  ASSERT_NE(t.find("a"), nullptr);
  EXPECT_EQ(t.find("a")->value, 1);
}

// --- Randomized parity tests -----------------------------------------------

TEST(RadixTree, RandomizedOperationTraceMatchesStdMapReferenceModel)
{
  std::mt19937 rng(0xC0FFEEu);
  std::uniform_int_distribution<int> op_dist(0, 2);  // insert / erase / find
  std::uniform_int_distribution<int> key_len_dist(0, 4);
  std::uniform_int_distribution<int> char_dist('a', 'd');  // small alphabet
                                                             // to force lots
                                                             // of shared
                                                             // prefixes.
  std::uniform_int_distribution<int> value_dist(0, 1'000'000);

  RadixTree<int> subject;
  std::map<std::string, int> reference;

  const auto random_key = [&]
  {
    std::string k;
    const int len = key_len_dist(rng);
    for (int i = 0; i < len; ++i)
      k.push_back(static_cast<char>(char_dist(rng)));
    return k;
  };

  for (int iter = 0; iter < 20000; ++iter)
    {
      const int op = op_dist(rng);
      const std::string key = random_key();

      if (op == 0)
        {
          const int value = value_dist(rng);
          const bool subject_inserted = subject.insert(key, value);
          const bool reference_inserted =
              reference.emplace(key, value).second;
          ASSERT_EQ(subject_inserted, reference_inserted)
              << "insert(\"" << key << "\") disagreement at iter " << iter;
        }
      else if (op == 1)
        {
          const bool subject_erased = subject.erase(key);
          const bool reference_erased = reference.erase(key) > 0;
          ASSERT_EQ(subject_erased, reference_erased)
              << "erase(\"" << key << "\") disagreement at iter " << iter;
        }
      else
        {
          const int * subject_value = subject.find(key);
          const auto reference_it = reference.find(key);
          ASSERT_EQ(subject_value != nullptr, reference_it != reference.end())
              << "find(\"" << key << "\") presence disagreement at iter "
              << iter;
          if (subject_value != nullptr)
            ASSERT_EQ(*subject_value, reference_it->second)
                << "find(\"" << key << "\") value disagreement at iter "
                << iter;
        }

      ASSERT_EQ(subject.size(), reference.size())
          << "size disagreement at iter " << iter;
      ASSERT_TRUE(subject.check_invariants())
          << "structural invariant violation at iter " << iter;
    }

  // Full final-state cross-check.
  ASSERT_EQ(subject.size(), reference.size());
  for (const auto & [key, value] : reference)
    {
      const int * found = subject.find(key);
      ASSERT_NE(found, nullptr) << "missing key in final check: " << key;
      EXPECT_EQ(*found, value) << "value mismatch in final check: " << key;
    }
}

TEST(RadixTree, RandomizedPrefixQueriesMatchBruteForceScan)
{
  std::mt19937 rng(0xBADC0FFEu);
  std::uniform_int_distribution<int> key_len_dist(1, 5);
  std::uniform_int_distribution<int> char_dist('a', 'c');  // tiny alphabet:
                                                             // forces heavy
                                                             // prefix sharing.

  RadixTree<int> subject;
  std::set<std::string> reference;

  const auto random_string = [&](const int len)
  {
    std::string s;
    for (int i = 0; i < len; ++i)
      s.push_back(static_cast<char>(char_dist(rng)));
    return s;
  };

  for (int i = 0; i < 500; ++i)
    {
      const std::string key = random_string(key_len_dist(rng));
      subject.insert(key, i);
      reference.insert(key);
      ASSERT_TRUE(subject.check_invariants())
          << "structural invariant violation after inserting: " << key;
    }

  for (int i = 0; i < 200; ++i)
    {
      const std::string prefix = random_string(key_len_dist(rng));

      std::vector<std::string> expected;
      for (const auto & key : reference)
        if (key.compare(0, prefix.size(), prefix) == 0)
          expected.push_back(key);

      const auto actual = sorted(to_vector(subject.keys_with_prefix(prefix)));
      ASSERT_EQ(actual, sorted(expected))
          << "prefix query disagreement for prefix: \"" << prefix << "\"";
    }
}

// --- Cross-implementation parity against Aleph::Prefix_Tree ---------------
//
// `Prefix_Tree` (prefix-tree.H) is Aleph's existing uncompressed
// character-per-edge trie. It has no `erase`, so this cannot cover
// removal, but insert/contains/prefix-query parity between two
// *independently implemented* tries is a materially stronger signal than
// parity against std::map alone: it cross-checks RadixTree's edge-
// splitting logic against a conceptually different (uncompressed)
// implementation of the same abstract structure, per the plan's own
// "compare prefix-tree.H vs RadixTree" documentation requirement.

TEST(RadixTree, MatchesPrefixTreeInsertContainsAndPrefixQueries)
{
  std::mt19937 rng(0xFACEFEEDu);
  std::uniform_int_distribution<int> word_len_dist(0, 6);
  std::uniform_int_distribution<int> char_dist('a', 'e');  // small alphabet:
                                                             // forces heavy
                                                             // prefix sharing,
                                                             // the scenario
                                                             // that most
                                                             // exercises
                                                             // RadixTree's
                                                             // edge-splitting
                                                             // and Prefix_
                                                             // Tree's node
                                                             // chains alike.

  RadixTree<char> subject;  // value type is irrelevant; used purely as a set
  Prefix_Tree reference;

  const auto random_word = [&]
  {
    std::string w;
    const int len = word_len_dist(rng);
    for (int i = 0; i < len; ++i)
      w.push_back(static_cast<char>(char_dist(rng)));
    return w;
  };

  for (int iter = 0; iter < 3000; ++iter)
    {
      const std::string word = random_word();
      const bool subject_inserted = subject.insert(word, 'x');
      const bool reference_inserted = reference.insert_word(word);
      ASSERT_EQ(subject_inserted, reference_inserted)
          << "insert(\"" << word << "\") disagreement at iter " << iter;
      ASSERT_EQ(subject.size(), reference.count())
          << "count disagreement at iter " << iter;
      ASSERT_TRUE(subject.check_invariants())
          << "structural invariant violation at iter " << iter;
    }

  // Presence parity, including words that were never inserted.
  for (int i = 0; i < 1000; ++i)
    {
      const std::string word = random_word();
      ASSERT_EQ(subject.contains(word), reference.contains(word))
          << "contains(\"" << word << "\") disagreement for: " << word;
    }

  // Full word-set parity (RadixTree's "all keys" is keys_with_prefix("")).
  EXPECT_EQ(sorted(to_vector(subject.keys_with_prefix(""))),
            sorted(to_vector(reference.words())));

  // Prefix-query parity across many random prefixes (including prefixes
  // that were never inserted as standalone words).
  for (int i = 0; i < 300; ++i)
    {
      const std::string prefix = random_word();
      const auto subject_matches = sorted(to_vector(subject.keys_with_prefix(prefix)));
      const auto reference_matches =
          sorted(to_vector(reference.words_with_prefix(prefix)));
      ASSERT_EQ(subject_matches, reference_matches)
          << "prefix query disagreement for prefix: \"" << prefix << "\"";
    }
}