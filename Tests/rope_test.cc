
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
 * @file rope_test.cc
 * @brief Tests for Aleph::Rope (tpl_rope.H).
 */

#include <gtest/gtest.h>

#include <tpl_rope.H>

#include <chrono>
#include <random>
#include <stdexcept>
#include <string>

using namespace Aleph;

namespace
{
  // A tiny leaf size to force frequent splitting/rebalancing in tests
  // without needing enormous input strings.
  using TinyRope = Rope<char, 4>;
}  // namespace

TEST(Rope, DefaultConstructedRopeIsEmpty)
{
  Rope<char> r;
  EXPECT_TRUE(r.is_empty());
  EXPECT_EQ(r.size(), 0u);
  EXPECT_EQ(r.to_string(), "");
}

TEST(Rope, VerifyHoldsForEmptyAndTrivialRopes)
{
  EXPECT_TRUE(Rope<char>().verify());
  EXPECT_TRUE(Rope<char>{std::string_view("x")}.verify());
  EXPECT_TRUE(Rope<char>{std::string_view("hello world")}.verify());
}

TEST(Rope, ConstructFromStringView)
{
  Rope<char> r{std::string_view("hello world")};
  EXPECT_FALSE(r.is_empty());
  EXPECT_EQ(r.size(), 11u);
  EXPECT_EQ(r.to_string(), "hello world");
}

TEST(Rope, ConstructFromEmptyStringViewIsEmpty)
{
  Rope<char> r{std::string_view("")};
  EXPECT_TRUE(r.is_empty());
  EXPECT_EQ(r.size(), 0u);
}

TEST(Rope, AtReturnsCorrectCharacters)
{
  Rope<char> r{std::string_view("abcdef")};
  EXPECT_EQ(r.at(0), 'a');
  EXPECT_EQ(r.at(5), 'f');
  EXPECT_EQ(r.at(3), 'd');
}

TEST(Rope, AtThrowsOutOfRange)
{
  Rope<char> r{std::string_view("abc")};
  EXPECT_THROW(r.at(3), std::out_of_range);
  EXPECT_THROW(r.at(100), std::out_of_range);

  Rope<char> empty;
  EXPECT_THROW(empty.at(0), std::out_of_range);
}

TEST(Rope, ConcatCombinesTwoRopes)
{
  Rope<char> a{std::string_view("hello ")};
  Rope<char> b{std::string_view("world")};
  Rope<char> c = a.concat(b);

  EXPECT_EQ(c.size(), 11u);
  EXPECT_EQ(c.to_string(), "hello world");
  // Originals are untouched (immutability).
  EXPECT_EQ(a.to_string(), "hello ");
  EXPECT_EQ(b.to_string(), "world");
  EXPECT_TRUE(c.verify());
}

TEST(Rope, ConcatWithEmptyRopeIsIdentity)
{
  Rope<char> a{std::string_view("abc")};
  Rope<char> empty;

  EXPECT_EQ(a.concat(empty).to_string(), "abc");
  EXPECT_EQ(empty.concat(a).to_string(), "abc");
  EXPECT_EQ(Rope<char>().concat(Rope<char>()).size(), 0u);
}

TEST(Rope, SubstrExtractsCorrectRange)
{
  Rope<char> r{std::string_view("hello world")};
  EXPECT_EQ(r.substr(0, 5).to_string(), "hello");
  EXPECT_EQ(r.substr(6, 5).to_string(), "world");
  EXPECT_EQ(r.substr(0, 11).to_string(), "hello world");
  EXPECT_EQ(r.substr(3, 0).to_string(), "");
  EXPECT_EQ(r.substr(11, 0).to_string(), "");  // one-past-the-end, zero length
}

TEST(Rope, SubstrThrowsOutOfRange)
{
  Rope<char> r{std::string_view("abc")};
  EXPECT_THROW(r.substr(0, 4), std::out_of_range);
  EXPECT_THROW(r.substr(4, 0), std::out_of_range);
  EXPECT_THROW(r.substr(2, 5), std::out_of_range);
}

TEST(Rope, SubstrOfWholeRopeSharesRoot)
{
  // Use a multi-leaf (internal-rooted) rope: with a single-leaf rope this
  // test would pass even if `slice()` failed to take the whole-node
  // sharing fast path for internal nodes (content would still come out
  // right either way), which is exactly what happened before that fast
  // path covered internal nodes too, not just leaves.
  const std::string_view text = "the quick brown fox jumps over the lazy dog";
  TinyRope r{text};
  TinyRope whole = r.substr(0, r.size());
  EXPECT_EQ(whole.to_string(), text);
  EXPECT_TRUE(r.verify());
  EXPECT_TRUE(whole.verify());
}

TEST(Rope, InsertSplicesInAnotherRope)
{
  Rope<char> r{std::string_view("hello world")};
  Rope<char> ins{std::string_view("brave new ")};
  Rope<char> result = r.insert(6, ins);

  EXPECT_EQ(result.to_string(), "hello brave new world");
  EXPECT_EQ(r.to_string(), "hello world");  // original untouched
  EXPECT_TRUE(result.verify());
}

TEST(Rope, InsertAtBoundariesWorks)
{
  Rope<char> r{std::string_view("world")};
  EXPECT_EQ(r.insert(0, Rope<char>{std::string_view("hello ")}).to_string(),
            "hello world");
  EXPECT_EQ(r.insert(5, Rope<char>{std::string_view("!")}).to_string(),
            "world!");
}

TEST(Rope, InsertThrowsOutOfRange)
{
  Rope<char> r{std::string_view("abc")};
  EXPECT_THROW(r.insert(4, Rope<char>{std::string_view("x")}), std::out_of_range);
}

TEST(Rope, EraseRemovesRange)
{
  Rope<char> r{std::string_view("hello brave new world")};
  Rope<char> result = r.erase(6, 10);  // remove "brave new "

  EXPECT_EQ(result.to_string(), "hello world");
  EXPECT_EQ(r.to_string(), "hello brave new world");  // original untouched
  EXPECT_TRUE(result.verify());
}

TEST(Rope, EraseWholeRopeGivesEmpty)
{
  Rope<char> r{std::string_view("abc")};
  EXPECT_TRUE(r.erase(0, 3).is_empty());
}

TEST(Rope, EraseZeroLengthIsIdentity)
{
  Rope<char> r{std::string_view("abc")};
  EXPECT_EQ(r.erase(1, 0).to_string(), "abc");
}

TEST(Rope, EraseThrowsOutOfRange)
{
  Rope<char> r{std::string_view("abc")};
  EXPECT_THROW(r.erase(2, 5), std::out_of_range);
  EXPECT_THROW(r.erase(4, 0), std::out_of_range);
}

TEST(Rope, FlattenReturnsAllCharactersInOrder)
{
  Rope<char> r{std::string_view("abcdef")};
  auto arr = r.flatten();
  ASSERT_EQ(arr.size(), 6u);
  const char expected[] = {'a', 'b', 'c', 'd', 'e', 'f'};
  for (size_t i = 0; i < 6; ++i)
    EXPECT_EQ(arr[i], expected[i]);
}

TEST(Rope, EqualityComparesContentNotIdentity)
{
  Rope<char> a{std::string_view("hello")};
  Rope<char> b{std::string_view("hello")};
  Rope<char> c{std::string_view("world")};
  Rope<char> a_copy = a;  // shares the same tree

  EXPECT_TRUE(a == b);   // same content, different trees
  EXPECT_TRUE(a == a_copy);  // same content, same tree (fast path)
  EXPECT_FALSE(a == c);
  EXPECT_FALSE(Rope<char>() == a);
  EXPECT_TRUE(Rope<char>() == Rope<char>());
}

TEST(Rope, CopyIsIndependentOfLaterOperationsOnTheOriginalVariable)
{
  Rope<char> a{std::string_view("hello")};
  Rope<char> b = a;
  a = a.concat(Rope<char>{std::string_view(" world")});

  EXPECT_EQ(a.to_string(), "hello world");
  EXPECT_EQ(b.to_string(), "hello");  // b is unaffected by reassigning a
}

// --- Leaf-splitting / small-LeafSize scenarios ------------------------------

TEST(Rope, TinyLeafSizeStillProducesCorrectResults)
{
  TinyRope r{std::string_view("the quick brown fox jumps over the lazy dog")};
  EXPECT_EQ(r.size(), 43u);
  EXPECT_EQ(r.to_string(), "the quick brown fox jumps over the lazy dog");
  EXPECT_TRUE(r.verify());

  auto sub = r.substr(4, 5);  // "quick"
  EXPECT_EQ(sub.to_string(), "quick");
  EXPECT_TRUE(sub.verify());

  auto ins = r.insert(10, TinyRope{std::string_view("very ")});
  EXPECT_EQ(ins.to_string(),
            "the quick very brown fox jumps over the lazy dog");
  EXPECT_TRUE(ins.verify());

  auto er = r.erase(4, 6);  // remove "quick "
  EXPECT_EQ(er.to_string(), "the brown fox jumps over the lazy dog");
  EXPECT_TRUE(er.verify());
}

TEST(Rope, RepeatedSingleCharacterConcatStaysCorrect)
{
  // Pathological case for a naive (unbalanced) rope: N concats of a
  // single character each, worst case for depth. Confirms the explicit
  // rebalancing keeps every operation correct even if not asymptotically
  // optimal (see tpl_rope.H's "Rebalancing" note).
  TinyRope r;
  std::string expected;
  for (int i = 0; i < 500; ++i)
    {
      const char c = static_cast<char>('a' + (i % 26));
      r = r.concat(TinyRope{std::string_view(&c, 1)});
      expected.push_back(c);
    }

  ASSERT_EQ(r.size(), expected.size());
  EXPECT_EQ(r.to_string(), expected);
  EXPECT_TRUE(r.verify());
  for (size_t i = 0; i < expected.size(); ++i)
    ASSERT_EQ(r.at(i), expected[i]) << "mismatch at index " << i;
}

TEST(Rope, RepeatedSinglePrependStaysCorrect)
{
  // Mirror of RepeatedSingleCharacterConcatStaysCorrect, but prepending
  // instead of appending: exercises the leaf-absorption fast path on the
  // *left* side (try_absorb_left) instead of the right, repeatedly
  // crossing the LeafSize boundary and falling back to a normal concat.
  TinyRope r;
  std::string expected;
  for (int i = 0; i < 500; ++i)
    {
      const char c = static_cast<char>('a' + (i % 26));
      r = TinyRope{std::string_view(&c, 1)}.concat(r);
      expected.insert(expected.begin(), c);
    }

  ASSERT_EQ(r.size(), expected.size());
  EXPECT_EQ(r.to_string(), expected);
  EXPECT_TRUE(r.verify());
  for (size_t i = 0; i < expected.size(); ++i)
    ASSERT_EQ(r.at(i), expected[i]) << "mismatch at index " << i;
}

TEST(Rope, SubstrAtExactLeafBoundaries)
{
  // LeafSize=4: "abcd|efgh|ijkl" if split naively; exercise pos/len that
  // land exactly on those (implementation-internal) boundaries as well as
  // straddling them.
  TinyRope r{std::string_view("abcdefghijkl")};
  EXPECT_EQ(r.substr(0, 4).to_string(), "abcd");
  EXPECT_EQ(r.substr(4, 4).to_string(), "efgh");
  EXPECT_EQ(r.substr(2, 4).to_string(), "cdef");  // straddles a boundary
  EXPECT_EQ(r.substr(0, 12).to_string(), "abcdefghijkl");
  EXPECT_TRUE(r.verify());
}

// --- Randomized parity against std::string ----------------------------------

TEST(Rope, RandomizedEditScriptMatchesStdString)
{
  std::mt19937 rng(0xC0FFEEu);
  std::uniform_int_distribution<int> op_dist(0, 3);  // concat/insert/erase/substr-roundtrip
  std::uniform_int_distribution<int> char_dist('a', 'z');

  using SmallLeafRope = Rope<char, 8>;  // small leaf: forces frequent splits

  SmallLeafRope subject;
  std::string reference;

  const auto random_string = [&](const int len)
  {
    std::string s;
    for (int i = 0; i < len; ++i)
      s.push_back(static_cast<char>(char_dist(rng)));
    return s;
  };

  std::uniform_int_distribution<int> len_dist(1, 6);

  for (int iter = 0; iter < 2000; ++iter)
    {
      const int op = reference.empty() ? 0 : op_dist(rng);

      if (op == 0)
        {
          // concat a random chunk at the end.
          const std::string chunk = random_string(len_dist(rng));
          subject = subject.concat(SmallLeafRope{std::string_view(chunk)});
          reference += chunk;
        }
      else if (op == 1)
        {
          // insert a random chunk at a random position.
          std::uniform_int_distribution<size_t> pos_dist(0, reference.size());
          const size_t pos = pos_dist(rng);
          const std::string chunk = random_string(len_dist(rng));
          subject = subject.insert(pos, SmallLeafRope{std::string_view(chunk)});
          reference.insert(pos, chunk);
        }
      else if (op == 2)
        {
          // erase a random range.
          std::uniform_int_distribution<size_t> pos_dist(0, reference.size());
          const size_t pos = pos_dist(rng);
          std::uniform_int_distribution<size_t> len_d(0, reference.size() - pos);
          const size_t len = len_d(rng);
          subject = subject.erase(pos, len);
          reference.erase(pos, len);
        }
      else
        {
          // substr round-trip: extract, then rebuild reference the same
          // way, verifying substr() against std::string::substr().
          std::uniform_int_distribution<size_t> pos_dist(0, reference.size());
          const size_t pos = pos_dist(rng);
          std::uniform_int_distribution<size_t> len_d(0, reference.size() - pos);
          const size_t len = len_d(rng);
          ASSERT_EQ(subject.substr(pos, len).to_string(), reference.substr(pos, len))
              << "substr disagreement at iter " << iter;
        }

      ASSERT_EQ(subject.size(), reference.size()) << "size disagreement at iter " << iter;
      ASSERT_EQ(subject.to_string(), reference) << "content disagreement at iter " << iter;
      ASSERT_TRUE(subject.verify()) << "invariant violation at iter " << iter;
    }
}

// --- Leaf-absorption fast-path regression --------------------------------

TEST(Rope, RepeatedSmallConcatStaysFastEnoughToProveAbsorptionFired)
{
  // Correctness alone doesn't prove the leaf-absorption fast path
  // (try_absorb_right/try_absorb_left in concat_nodes) is actually being
  // taken: RepeatedSingleCharacterConcatStaysCorrect and
  // RepeatedSinglePrependStaysCorrect above would still pass, just much
  // more slowly, even if that fast path were silently deleted, since the
  // general concat+rebalance path is still correct on its own (see
  // tpl_rope.H's "Leaf absorption" note for the measured ~550x effect).
  // This test instead asserts a generous wall-clock time budget for a
  // pattern that is specifically pathological *without* the fast path
  // (many single-character concats), so a regression that silently
  // removes or breaks absorption is expected to blow well past it.
  using SmallLeafRope = Rope<char, 64>;
  constexpr int N = 20000;

  const auto start = std::chrono::steady_clock::now();
  SmallLeafRope r;
  for (int i = 0; i < N; ++i)
    {
      const char c = static_cast<char>('a' + (i % 26));
      r = r.concat(SmallLeafRope{std::string_view(&c, 1)});
    }
  const auto elapsed = std::chrono::steady_clock::now() - start;

  EXPECT_EQ(r.size(), static_cast<size_t>(N));
  EXPECT_TRUE(r.verify());
  // With absorption active this runs in ~10-20ms; measured ~1.7s for the
  // same N with absorption forcibly disabled during development of this
  // test (confirming it actually catches a regression). 400ms leaves
  // roughly 20x slack above the healthy case and roughly 4x margin below
  // the broken one -- generous enough for a loaded CI machine without
  // losing the ability to fail loudly on a real regression.
  EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 400)
      << N << " single-character concats took too long -- the leaf-absorption "
         "fast path may have regressed (see tpl_rope.H's \"Leaf absorption\" note).";
}