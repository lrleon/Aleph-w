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

/** @file patricia_trie_test.cc
 *  @brief Tests for Aleph::PatriciaSet and Aleph::PatriciaMap.
 */

#include <prefix-tree.H>
#include <tpl_patricia_trie.H>
#include <tpl_radix_tree.H>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

using namespace Aleph;

namespace
{
template <typename T>
std::vector<T> to_sorted_vector(const Array<T> & a)
{
  std::vector<T> out;
  out.reserve(a.size());
  for (const auto & item : a)
    out.push_back(item);
  std::sort(out.begin(), out.end());
  return out;
}

template <typename T>
std::vector<T> to_vector(const std::set<T> & s)
{
  return std::vector<T>(s.begin(), s.end());
}

template <typename K, typename V>
std::vector<K> keys_of(const std::map<K, V> & m)
{
  std::vector<K> out;
  out.reserve(m.size());
  for (const auto & kv : m)
    out.push_back(kv.first);
  return out;
}

std::vector<std::string> to_vector(const DynArray<std::string> & a)
{
  std::vector<std::string> out;
  a.for_each([&out] (const std::string & s) { out.push_back(s); });
  return out;
}

std::vector<std::string> sorted(std::vector<std::string> v)
{
  std::sort(v.begin(), v.end());
  return v;
}

std::string encode_bits(const std::uint16_t key)
{
  std::string out;
  out.reserve(PatriciaSet<std::uint16_t>::bit_width);
  for (size_t bit = 0; bit < PatriciaSet<std::uint16_t>::bit_width; ++bit)
    {
      const auto shift = PatriciaSet<std::uint16_t>::bit_width - 1 - bit;
      out.push_back(((key >> shift) & std::uint16_t{1}) ? '1' : '0');
    }
  return out;
}

std::vector<std::string> encoded_words(const std::set<std::uint16_t> & keys)
{
  std::vector<std::string> out;
  out.reserve(keys.size());
  for (const auto key : keys)
    out.push_back(encode_bits(key));
  std::sort(out.begin(), out.end());
  return out;
}

std::vector<std::string> encoded_words(const std::vector<std::uint16_t> & keys)
{
  std::vector<std::string> out;
  out.reserve(keys.size());
  for (const auto key : keys)
    out.push_back(encode_bits(key));
  std::sort(out.begin(), out.end());
  return out;
}

std::vector<std::string> encoded_words_with_prefix(const std::set<std::uint16_t> & keys,
                                                    const std::string & prefix)
{
  std::vector<std::string> out;
  for (const auto key : keys)
    {
      const std::string encoded = encode_bits(key);
      if (encoded.starts_with(prefix))
        out.push_back(encoded);
    }
  std::sort(out.begin(), out.end());
  return out;
}
} // namespace

static_assert(std::is_move_constructible_v<PatriciaSet<unsigned>>);
static_assert(std::is_copy_constructible_v<PatriciaSet<unsigned>>);
static_assert(std::is_move_constructible_v<PatriciaMap<unsigned, int>>);
static_assert(std::is_copy_constructible_v<PatriciaMap<unsigned, int>>);
static_assert(std::is_move_constructible_v<
              PatriciaMap<unsigned, std::unique_ptr<int>>>);
static_assert(not std::is_copy_constructible_v<
              PatriciaMap<unsigned, std::unique_ptr<int>>>);

TEST(PatriciaSet, DefaultConstructedSetIsEmpty)
{
  PatriciaSet<unsigned> s;
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0U);
  EXPECT_FALSE(s.contains(0));
  EXPECT_TRUE(s.keys().is_empty());
  EXPECT_TRUE(s.check_invariants());
}

TEST(PatriciaSet, InsertAndContainsSingleKey)
{
  PatriciaSet<unsigned> s;
  EXPECT_TRUE(s.insert(42));
  EXPECT_FALSE(s.is_empty());
  EXPECT_EQ(s.size(), 1U);
  EXPECT_TRUE(s.contains(42));
  EXPECT_FALSE(s.contains(41));
  EXPECT_TRUE(s.check_invariants());
}

TEST(PatriciaSet, InsertRejectsDuplicateKeys)
{
  PatriciaSet<unsigned> s;
  EXPECT_TRUE(s.insert(7));
  EXPECT_FALSE(s.insert(7));
  EXPECT_EQ(s.size(), 1U);
  EXPECT_TRUE(s.contains(7));
  EXPECT_TRUE(s.check_invariants());
}

TEST(PatriciaSet, HandlesZeroAndExtremes)
{
  PatriciaSet<unsigned> s;
  const unsigned max = std::numeric_limits<unsigned>::max();
  const unsigned high = 1U << (PatriciaSet<unsigned>::bit_width - 1);

  EXPECT_TRUE(s.insert(0));
  EXPECT_TRUE(s.insert(max));
  EXPECT_TRUE(s.insert(high));

  EXPECT_TRUE(s.contains(0));
  EXPECT_TRUE(s.contains(max));
  EXPECT_TRUE(s.contains(high));
  EXPECT_FALSE(s.contains(high - 1));
  EXPECT_EQ(s.size(), 3U);
  EXPECT_TRUE(s.check_invariants());
}

TEST(PatriciaSet, KeysReturnsAllStoredValues)
{
  PatriciaSet<unsigned> s;
  for (unsigned key : {9U, 1U, 17U, 3U, 0U})
    ASSERT_TRUE(s.insert(key));

  EXPECT_EQ(to_sorted_vector(s.keys()), (std::vector<unsigned>{0, 1, 3, 9, 17}));
  EXPECT_TRUE(s.check_invariants());
}

TEST(PatriciaSet, EraseMissingKeyReturnsFalse)
{
  PatriciaSet<unsigned> s;
  EXPECT_FALSE(s.erase(1));
  EXPECT_TRUE(s.insert(10));
  EXPECT_FALSE(s.erase(11));
  EXPECT_TRUE(s.contains(10));
  EXPECT_TRUE(s.check_invariants());
}

TEST(PatriciaSet, EraseOnlyKeyEmptiesSet)
{
  PatriciaSet<unsigned> s;
  ASSERT_TRUE(s.insert(10));
  EXPECT_TRUE(s.erase(10));
  EXPECT_TRUE(s.is_empty());
  EXPECT_FALSE(s.contains(10));
  EXPECT_TRUE(s.check_invariants());
}

TEST(PatriciaSet, EraseCollapsesInternalNodeAndPreservesSibling)
{
  PatriciaSet<unsigned> s;
  ASSERT_TRUE(s.insert(0b1000));
  ASSERT_TRUE(s.insert(0b1001));
  ASSERT_TRUE(s.insert(0b1100));
  ASSERT_TRUE(s.insert(0b1101));

  EXPECT_TRUE(s.erase(0b1001));
  EXPECT_FALSE(s.contains(0b1001));
  EXPECT_TRUE(s.contains(0b1000));
  EXPECT_TRUE(s.contains(0b1100));
  EXPECT_TRUE(s.contains(0b1101));
  EXPECT_EQ(s.size(), 3U);
  EXPECT_TRUE(s.check_invariants());
}

TEST(PatriciaSet, ClearRemovesAllKeys)
{
  PatriciaSet<unsigned> s;
  ASSERT_TRUE(s.insert(1));
  ASSERT_TRUE(s.insert(2));
  s.clear();

  EXPECT_TRUE(s.is_empty());
  EXPECT_FALSE(s.contains(1));
  EXPECT_FALSE(s.contains(2));
  EXPECT_TRUE(s.keys().is_empty());
  EXPECT_TRUE(s.check_invariants());
}

TEST(PatriciaSet, CopyConstructorProducesIndependentClone)
{
  PatriciaSet<unsigned> a;
  ASSERT_TRUE(a.insert(1));
  ASSERT_TRUE(a.insert(8));

  PatriciaSet<unsigned> b(a);
  EXPECT_TRUE(b.contains(1));
  EXPECT_TRUE(b.contains(8));
  EXPECT_TRUE(b.erase(1));

  EXPECT_TRUE(a.contains(1));
  EXPECT_FALSE(b.contains(1));
  EXPECT_TRUE(a.check_invariants());
  EXPECT_TRUE(b.check_invariants());
}

TEST(PatriciaSet, MoveConstructorTransfersKeysAndLeavesSourceEmpty)
{
  PatriciaSet<unsigned> a;
  ASSERT_TRUE(a.insert(5));
  ASSERT_TRUE(a.insert(6));

  PatriciaSet<unsigned> b(std::move(a));
  EXPECT_TRUE(a.is_empty());
  EXPECT_TRUE(a.check_invariants());
  EXPECT_TRUE(b.contains(5));
  EXPECT_TRUE(b.contains(6));
  EXPECT_TRUE(b.check_invariants());
}

TEST(PatriciaSet, CopyAssignmentProducesIndependentClone)
{
  PatriciaSet<unsigned> a;
  ASSERT_TRUE(a.insert(1));
  ASSERT_TRUE(a.insert(8));

  PatriciaSet<unsigned> b;
  ASSERT_TRUE(b.insert(99));

  b = a;
  EXPECT_TRUE(b.contains(1));
  EXPECT_TRUE(b.contains(8));
  EXPECT_FALSE(b.contains(99));
  EXPECT_TRUE(b.erase(1));

  EXPECT_TRUE(a.contains(1));
  EXPECT_FALSE(b.contains(1));
  EXPECT_TRUE(a.check_invariants());
  EXPECT_TRUE(b.check_invariants());
}

TEST(PatriciaSet, MoveAssignmentTransfersKeysAndLeavesSourceEmpty)
{
  PatriciaSet<unsigned> a;
  ASSERT_TRUE(a.insert(5));
  ASSERT_TRUE(a.insert(6));

  PatriciaSet<unsigned> b;
  ASSERT_TRUE(b.insert(99));

  b = std::move(a);
  EXPECT_TRUE(a.is_empty());  // NOLINT(bugprone-use-after-move): documented
  EXPECT_TRUE(a.check_invariants());
  EXPECT_FALSE(b.contains(99));
  EXPECT_TRUE(b.contains(5));
  EXPECT_TRUE(b.contains(6));
  EXPECT_TRUE(b.check_invariants());
}

TEST(PatriciaSet, SelfAssignmentDoesNotCorruptState)
{
  PatriciaSet<unsigned> s;
  ASSERT_TRUE(s.insert(5));
  ASSERT_TRUE(s.insert(6));

  PatriciaSet<unsigned> &alias = s;
  s = alias;
  EXPECT_TRUE(s.contains(5));
  EXPECT_TRUE(s.contains(6));
  EXPECT_EQ(s.size(), 2U);
  EXPECT_TRUE(s.check_invariants());

  s = std::move(alias);  // NOLINT(bugprone-use-after-move): deliberate self-move
  EXPECT_TRUE(s.contains(5));
  EXPECT_TRUE(s.contains(6));
  EXPECT_EQ(s.size(), 2U);
  EXPECT_TRUE(s.check_invariants());
}

TEST(PatriciaSet, RandomizedOperationsMatchStdSet)
{
  PatriciaSet<std::uint32_t> subject;
  std::set<std::uint32_t> reference;
  std::mt19937 rng(0x5eed1234U);
  std::uniform_int_distribution<std::uint32_t> key_dist(0, 4095);
  std::uniform_int_distribution<int> op_dist(0, 2);

  for (int step = 0; step < 20000; ++step)
    {
      const std::uint32_t key = key_dist(rng);
      const int op = op_dist(rng);
      if (op == 0)
        EXPECT_EQ(subject.insert(key), reference.insert(key).second);
      else if (op == 1)
        EXPECT_EQ(subject.erase(key), reference.erase(key) != 0);
      else
        EXPECT_EQ(subject.contains(key), reference.contains(key));

      ASSERT_EQ(subject.size(), reference.size()) << "step " << step;
      ASSERT_TRUE(subject.check_invariants()) << "step " << step;
    }

  EXPECT_EQ(to_sorted_vector(subject.keys()), to_vector(reference));
}

TEST(PatriciaSet, MatchesRadixTreeAndPrefixTreeOnEncodedIntegerKeys)
{
  PatriciaSet<std::uint16_t> patricia;
  RadixTree<char> radix;
  Prefix_Tree prefix_tree;
  std::set<std::uint16_t> reference;

  std::mt19937 rng(0xC017B175U);
  std::uniform_int_distribution<unsigned> key_dist(
      0, std::numeric_limits<std::uint16_t>::max());

  for (int iter = 0; iter < 5000; ++iter)
    {
      const auto key = static_cast<std::uint16_t>(key_dist(rng));
      const std::string encoded = encode_bits(key);
      const bool expected_inserted = reference.insert(key).second;

      ASSERT_EQ(patricia.insert(key), expected_inserted)
          << "Patricia disagreement inserting " << key;
      ASSERT_EQ(radix.insert(encoded, 'x'), expected_inserted)
          << "RadixTree disagreement inserting " << encoded;
      ASSERT_EQ(prefix_tree.insert_word(encoded), expected_inserted)
          << "Prefix_Tree disagreement inserting " << encoded;

      ASSERT_EQ(patricia.size(), reference.size());
      ASSERT_EQ(radix.size(), reference.size());
      ASSERT_EQ(prefix_tree.count(), reference.size());
      ASSERT_TRUE(patricia.check_invariants()) << "Patricia invariant at " << iter;
      ASSERT_TRUE(radix.verify()) << "RadixTree invariant at " << iter;
    }

  for (int iter = 0; iter < 1000; ++iter)
    {
      const auto key = static_cast<std::uint16_t>(key_dist(rng));
      const std::string encoded = encode_bits(key);
      const bool expected = reference.contains(key);
      ASSERT_EQ(patricia.contains(key), expected)
          << "Patricia contains disagreement for " << key;
      ASSERT_EQ(radix.contains(encoded), expected)
          << "RadixTree contains disagreement for " << encoded;
      ASSERT_EQ(prefix_tree.contains(encoded), expected)
          << "Prefix_Tree contains disagreement for " << encoded;
    }

  const auto expected_all = encoded_words(reference);
  EXPECT_EQ(to_sorted_vector(radix.keys_with_prefix("")), expected_all);
  EXPECT_EQ(sorted(to_vector(prefix_tree.words())), expected_all);

  std::vector<std::uint16_t> patricia_keys = to_sorted_vector(patricia.keys());
  EXPECT_EQ(patricia_keys, to_vector(reference));

  std::uniform_int_distribution<size_t> prefix_len_dist(
      0, PatriciaSet<std::uint16_t>::bit_width);
  for (int iter = 0; iter < 500; ++iter)
    {
      const auto key = static_cast<std::uint16_t>(key_dist(rng));
      const std::string probe = encode_bits(key);
      const std::string prefix = probe.substr(0, prefix_len_dist(rng));
      const auto expected = encoded_words_with_prefix(reference, prefix);

      EXPECT_EQ(to_sorted_vector(radix.keys_with_prefix(prefix)), expected)
          << "RadixTree prefix disagreement for " << prefix;
      EXPECT_EQ(sorted(to_vector(prefix_tree.words_with_prefix(prefix))), expected)
          << "Prefix_Tree prefix disagreement for " << prefix;

      std::set<std::uint16_t> patricia_reference;
      for (const auto stored : patricia.keys())
        if (encode_bits(stored).starts_with(prefix))
          patricia_reference.insert(stored);
      EXPECT_EQ(encoded_words(patricia_reference), expected)
          << "Patricia scan prefix disagreement for " << prefix;
    }
}

TEST(PatriciaMap, DefaultConstructedMapIsEmpty)
{
  PatriciaMap<unsigned, int> m;
  EXPECT_TRUE(m.is_empty());
  EXPECT_EQ(m.size(), 0U);
  EXPECT_FALSE(m.contains(0));
  EXPECT_EQ(m.find(0), nullptr);
  EXPECT_TRUE(m.keys().is_empty());
  EXPECT_TRUE(m.check_invariants());
}

TEST(PatriciaMap, InsertAndFindSingleValue)
{
  PatriciaMap<unsigned, int> m;
  EXPECT_TRUE(m.insert(42, 100));
  EXPECT_FALSE(m.is_empty());
  ASSERT_NE(m.find(42), nullptr);
  EXPECT_EQ(*m.find(42), 100);
  EXPECT_TRUE(m.contains(42));
  EXPECT_FALSE(m.contains(41));
  EXPECT_TRUE(m.check_invariants());
}

TEST(PatriciaMap, InsertRejectsDuplicateAndKeepsOriginalValue)
{
  PatriciaMap<unsigned, int> m;
  ASSERT_TRUE(m.insert(7, 1));
  EXPECT_FALSE(m.insert(7, 2));
  ASSERT_NE(m.find(7), nullptr);
  EXPECT_EQ(*m.find(7), 1);
  EXPECT_EQ(m.size(), 1U);
  EXPECT_TRUE(m.check_invariants());
}

TEST(PatriciaMap, DuplicateMoveInsertDoesNotConsumeValue)
{
  PatriciaMap<unsigned, std::unique_ptr<int>> m;
  ASSERT_TRUE(m.insert(7, std::make_unique<int>(1)));

  auto duplicate = std::make_unique<int>(2);
  EXPECT_FALSE(m.insert(7, std::move(duplicate)));
  ASSERT_NE(duplicate, nullptr);
  EXPECT_EQ(*duplicate, 2);
  ASSERT_NE(m.find(7), nullptr);
  ASSERT_NE(*m.find(7), nullptr);
  EXPECT_EQ(**m.find(7), 1);
  EXPECT_TRUE(m.check_invariants());
}

TEST(PatriciaMap, InsertOrAssignInsertsThenOverwrites)
{
  PatriciaMap<unsigned, std::string> m;
  m.insert_or_assign(10, "ten");
  ASSERT_NE(m.find(10), nullptr);
  EXPECT_EQ(*m.find(10), "ten");

  m.insert_or_assign(10, "diez");
  ASSERT_NE(m.find(10), nullptr);
  EXPECT_EQ(*m.find(10), "diez");

  m.insert_or_assign(11, "once");
  ASSERT_NE(m.find(11), nullptr);
  EXPECT_EQ(*m.find(11), "once");
  EXPECT_EQ(m.size(), 2U);
  EXPECT_TRUE(m.check_invariants());
}

TEST(PatriciaMap, MutableFindAllowsInPlaceUpdate)
{
  PatriciaMap<unsigned, int> m;
  ASSERT_TRUE(m.insert(1, 10));
  int * slot = m.find(1);
  ASSERT_NE(slot, nullptr);
  *slot = 20;

  const auto & cm = m;
  ASSERT_NE(cm.find(1), nullptr);
  EXPECT_EQ(*cm.find(1), 20);
  EXPECT_TRUE(m.check_invariants());
}

TEST(PatriciaMap, EraseRemovesValueAndPreservesSibling)
{
  PatriciaMap<unsigned, int> m;
  ASSERT_TRUE(m.insert(0b1000, 8));
  ASSERT_TRUE(m.insert(0b1001, 9));
  ASSERT_TRUE(m.insert(0b1100, 12));
  ASSERT_TRUE(m.insert(0b1101, 13));

  EXPECT_TRUE(m.erase(0b1001));
  EXPECT_FALSE(m.contains(0b1001));
  ASSERT_NE(m.find(0b1000), nullptr);
  ASSERT_NE(m.find(0b1100), nullptr);
  ASSERT_NE(m.find(0b1101), nullptr);
  EXPECT_EQ(*m.find(0b1000), 8);
  EXPECT_EQ(*m.find(0b1100), 12);
  EXPECT_EQ(*m.find(0b1101), 13);
  EXPECT_EQ(m.size(), 3U);
  EXPECT_TRUE(m.check_invariants());
}

TEST(PatriciaMap, ClearRemovesAllValues)
{
  PatriciaMap<unsigned, int> m;
  ASSERT_TRUE(m.insert(1, 10));
  ASSERT_TRUE(m.insert(2, 20));
  m.clear();

  EXPECT_TRUE(m.is_empty());
  EXPECT_FALSE(m.contains(1));
  EXPECT_FALSE(m.contains(2));
  EXPECT_TRUE(m.keys().is_empty());
  EXPECT_TRUE(m.check_invariants());
}

TEST(PatriciaMap, CopyConstructorProducesIndependentClone)
{
  PatriciaMap<unsigned, std::string> a;
  ASSERT_TRUE(a.insert(1, "one"));
  ASSERT_TRUE(a.insert(8, "eight"));

  PatriciaMap<unsigned, std::string> b(a);
  ASSERT_NE(b.find(1), nullptr);
  ASSERT_NE(b.find(8), nullptr);
  EXPECT_EQ(*b.find(1), "one");
  EXPECT_EQ(*b.find(8), "eight");

  b.insert_or_assign(1, "uno");
  EXPECT_EQ(*a.find(1), "one");
  EXPECT_EQ(*b.find(1), "uno");
  EXPECT_TRUE(a.check_invariants());
  EXPECT_TRUE(b.check_invariants());
}

TEST(PatriciaMap, MoveConstructorTransfersValuesAndLeavesSourceEmpty)
{
  PatriciaMap<unsigned, int> a;
  ASSERT_TRUE(a.insert(5, 50));
  ASSERT_TRUE(a.insert(6, 60));

  PatriciaMap<unsigned, int> b(std::move(a));
  EXPECT_TRUE(a.is_empty());
  EXPECT_TRUE(a.check_invariants());
  ASSERT_NE(b.find(5), nullptr);
  ASSERT_NE(b.find(6), nullptr);
  EXPECT_EQ(*b.find(5), 50);
  EXPECT_EQ(*b.find(6), 60);
  EXPECT_TRUE(b.check_invariants());
}

TEST(PatriciaMap, CopyAssignmentProducesIndependentClone)
{
  PatriciaMap<unsigned, std::string> a;
  ASSERT_TRUE(a.insert(1, "one"));
  ASSERT_TRUE(a.insert(8, "eight"));

  PatriciaMap<unsigned, std::string> b;
  ASSERT_TRUE(b.insert(99, "old"));

  b = a;
  ASSERT_NE(b.find(1), nullptr);
  ASSERT_NE(b.find(8), nullptr);
  EXPECT_EQ(*b.find(1), "one");
  EXPECT_EQ(*b.find(8), "eight");
  EXPECT_FALSE(b.contains(99));

  b.insert_or_assign(1, "uno");
  EXPECT_EQ(*a.find(1), "one");
  EXPECT_EQ(*b.find(1), "uno");
  EXPECT_TRUE(a.check_invariants());
  EXPECT_TRUE(b.check_invariants());
}

TEST(PatriciaMap, MoveAssignmentTransfersValuesAndLeavesSourceEmpty)
{
  PatriciaMap<unsigned, int> a;
  ASSERT_TRUE(a.insert(5, 50));
  ASSERT_TRUE(a.insert(6, 60));

  PatriciaMap<unsigned, int> b;
  ASSERT_TRUE(b.insert(99, 990));

  b = std::move(a);
  EXPECT_TRUE(a.is_empty());  // NOLINT(bugprone-use-after-move): documented
  EXPECT_TRUE(a.check_invariants());
  EXPECT_FALSE(b.contains(99));
  ASSERT_NE(b.find(5), nullptr);
  ASSERT_NE(b.find(6), nullptr);
  EXPECT_EQ(*b.find(5), 50);
  EXPECT_EQ(*b.find(6), 60);
  EXPECT_TRUE(b.check_invariants());
}

TEST(PatriciaMap, SelfAssignmentDoesNotCorruptState)
{
  PatriciaMap<unsigned, std::string> m;
  ASSERT_TRUE(m.insert(5, "five"));
  ASSERT_TRUE(m.insert(6, "six"));

  PatriciaMap<unsigned, std::string> &alias = m;
  m = alias;
  ASSERT_NE(m.find(5), nullptr);
  ASSERT_NE(m.find(6), nullptr);
  EXPECT_EQ(*m.find(5), "five");
  EXPECT_EQ(*m.find(6), "six");
  EXPECT_EQ(m.size(), 2U);
  EXPECT_TRUE(m.check_invariants());

  m = std::move(alias);  // NOLINT(bugprone-use-after-move): deliberate self-move
  ASSERT_NE(m.find(5), nullptr);
  ASSERT_NE(m.find(6), nullptr);
  EXPECT_EQ(*m.find(5), "five");
  EXPECT_EQ(*m.find(6), "six");
  EXPECT_EQ(m.size(), 2U);
  EXPECT_TRUE(m.check_invariants());
}

TEST(PatriciaMap, RandomizedOperationsMatchStdMap)
{
  PatriciaMap<std::uint32_t, int> subject;
  std::map<std::uint32_t, int> reference;
  std::mt19937 rng(0x0BADC0DEU);
  std::uniform_int_distribution<std::uint32_t> key_dist(0, 4095);
  std::uniform_int_distribution<int> value_dist(-10000, 10000);
  std::uniform_int_distribution<int> op_dist(0, 3);

  for (int step = 0; step < 10000; ++step)
    {
      const std::uint32_t key = key_dist(rng);
      const int value = value_dist(rng);
      const int op = op_dist(rng);

      if (op == 0)
        EXPECT_EQ(subject.insert(key, value),
                  reference.emplace(key, value).second);
      else if (op == 1)
        {
          subject.insert_or_assign(key, value);
          reference[key] = value;
        }
      else if (op == 2)
        EXPECT_EQ(subject.erase(key), reference.erase(key) != 0);
      else
        {
          const int * subject_value = subject.find(key);
          const auto reference_it = reference.find(key);
          ASSERT_EQ(subject_value != nullptr, reference_it != reference.end())
              << "find presence disagreement at step " << step;
          if (subject_value != nullptr)
            EXPECT_EQ(*subject_value, reference_it->second)
                << "find value disagreement at step " << step;
        }

      ASSERT_EQ(subject.size(), reference.size()) << "step " << step;
      ASSERT_TRUE(subject.check_invariants()) << "step " << step;
    }

  EXPECT_EQ(to_sorted_vector(subject.keys()), keys_of(reference));
  for (const auto & kv : reference)
    {
      ASSERT_NE(subject.find(kv.first), nullptr);
      EXPECT_EQ(*subject.find(kv.first), kv.second);
    }
}

TEST(PatriciaMap, MatchesRadixTreeValuesAndPrefixTreeMembership)
{
  PatriciaMap<std::uint16_t, int> patricia;
  RadixTree<int> radix;
  Prefix_Tree prefix_tree;
  std::map<std::uint16_t, int> reference;

  std::mt19937 rng(0xFACEB00CU);
  std::uniform_int_distribution<unsigned> key_dist(
      0, std::numeric_limits<std::uint16_t>::max());
  std::uniform_int_distribution<int> value_dist(-5000, 5000);

  for (int iter = 0; iter < 3000; ++iter)
    {
      const auto key = static_cast<std::uint16_t>(key_dist(rng));
      const std::string encoded = encode_bits(key);
      const int value = value_dist(rng);

      if ((iter % 3) == 0)
        {
          const bool expected_inserted = reference.emplace(key, value).second;
          ASSERT_EQ(patricia.insert(key, value), expected_inserted)
              << "PatriciaMap disagreement inserting " << key;
          ASSERT_EQ(radix.insert(encoded, value), expected_inserted)
              << "RadixTree disagreement inserting " << encoded;
          ASSERT_EQ(prefix_tree.insert_word(encoded), expected_inserted)
              << "Prefix_Tree disagreement inserting " << encoded;
        }
      else
        {
          const bool was_absent = not reference.contains(key);
          reference[key] = value;
          patricia.insert_or_assign(key, value);
          radix.insert_or_assign(encoded, value);
          if (was_absent)
            ASSERT_TRUE(prefix_tree.insert_word(encoded))
                << "Prefix_Tree missed new key " << encoded;
          else
            ASSERT_TRUE(prefix_tree.contains(encoded))
                << "Prefix_Tree lost existing key " << encoded;
        }

      ASSERT_EQ(patricia.size(), reference.size());
      ASSERT_EQ(radix.size(), reference.size());
      ASSERT_EQ(prefix_tree.count(), reference.size());
      ASSERT_TRUE(patricia.check_invariants()) << "PatriciaMap invariant at "
                                               << iter;
      ASSERT_TRUE(radix.verify()) << "RadixTree invariant at " << iter;
    }

  for (const auto & kv : reference)
    {
      const std::string encoded = encode_bits(kv.first);
      ASSERT_NE(patricia.find(kv.first), nullptr);
      ASSERT_NE(radix.find(encoded), nullptr);
      EXPECT_TRUE(prefix_tree.contains(encoded));
      EXPECT_EQ(*patricia.find(kv.first), kv.second);
      EXPECT_EQ(*radix.find(encoded), kv.second);
    }

  const auto expected_all = encoded_words(keys_of(reference));
  EXPECT_EQ(to_sorted_vector(radix.keys_with_prefix("")), expected_all);
  EXPECT_EQ(sorted(to_vector(prefix_tree.words())), expected_all);
  EXPECT_EQ(to_sorted_vector(patricia.keys()), keys_of(reference));

  std::uniform_int_distribution<size_t> prefix_len_dist(
      0, PatriciaMap<std::uint16_t, int>::bit_width);
  for (int iter = 0; iter < 300; ++iter)
    {
      const auto key = static_cast<std::uint16_t>(key_dist(rng));
      const std::string probe = encode_bits(key);
      const std::string prefix = probe.substr(0, prefix_len_dist(rng));
      std::set<std::uint16_t> reference_keys;
      for (const auto & kv : reference)
        if (encode_bits(kv.first).starts_with(prefix))
          reference_keys.insert(kv.first);
      const auto expected = encoded_words(reference_keys);

      EXPECT_EQ(to_sorted_vector(radix.keys_with_prefix(prefix)), expected)
          << "RadixTree prefix disagreement for " << prefix;
      EXPECT_EQ(sorted(to_vector(prefix_tree.words_with_prefix(prefix))), expected)
          << "Prefix_Tree prefix disagreement for " << prefix;
    }
}
