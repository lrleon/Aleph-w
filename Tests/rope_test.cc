
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

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <new>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>

// `Rope<Char>::View` is `std::basic_string_view<Char>`, which the standard
// library requires `Char` to be a trivial, standard-layout type for (see
// `<string_view>`'s own static_assert) -- so, unlike e.g. `RadixTree<T>`'s
// mapped value `T`, `Rope`'s `Char` can never be a type whose copy
// constructor throws: a trivial copy constructor cannot run user code at
// all. The only realistic exception source `Rope`'s methods have is
// allocation failure (`std::bad_alloc`, already documented on every
// `@throws`), so exception-safety here is tested via allocation-failure
// injection instead of a throwing element type. This reuses the same
// global operator new/delete override pattern already established in
// `Tests/prefix_tree_test.cc` for the same purpose.
#if defined(__SANITIZE_THREAD__)
#  define ALEPH_ROPE_TEST_UNDER_TSAN 1
#elif defined(__has_feature)
#  if __has_feature(thread_sanitizer)
#    define ALEPH_ROPE_TEST_UNDER_TSAN 1
#  endif
#endif
#ifndef ALEPH_ROPE_TEST_UNDER_TSAN
#  define ALEPH_ROPE_TEST_UNDER_TSAN 0
#endif

namespace
{
  std::atomic<int> allocation_countdown{-1};
  std::atomic<bool> track_allocations{false};
  std::atomic<int> tracked_balance{0};

#if !ALEPH_ROPE_TEST_UNDER_TSAN
  bool should_fail_allocation() noexcept
  {
    int remaining = allocation_countdown.load(std::memory_order_relaxed);
    while (remaining >= 0)
      {
        if (remaining == 0)
          return true;
        if (allocation_countdown.compare_exchange_weak(
                remaining, remaining - 1, std::memory_order_relaxed))
          return false;
      }
    return false;
  }

  void * allocate_or_throw(std::size_t size)
  {
    if (should_fail_allocation())
      throw std::bad_alloc();

    if (size == 0)
      size = 1;

    void * ptr = std::malloc(size);
    if (ptr == nullptr)
      throw std::bad_alloc();

    if (track_allocations.load(std::memory_order_relaxed))
      tracked_balance.fetch_add(1, std::memory_order_relaxed);

    return ptr;
  }

  void release_allocation(void * ptr) noexcept
  {
    if (ptr == nullptr)
      return;

    if (track_allocations.load(std::memory_order_relaxed))
      tracked_balance.fetch_sub(1, std::memory_order_relaxed);

    std::free(ptr);
  }
#endif  // !ALEPH_ROPE_TEST_UNDER_TSAN

  /// RAII scope that makes the `N`-th allocation after construction throw
  /// `std::bad_alloc` (or, for negative `N`, only tracks the net
  /// allocation/deallocation balance without ever failing). `balance()`
  /// after the scope's operation should be 0: every allocation made while
  /// armed was matched by a deallocation, proving nothing leaked when the
  /// operation under test threw partway through.
  class AllocationFailureScope
  {
  public:
    explicit AllocationFailureScope(const int successful_allocations_before_failure)
    {
      tracked_balance.store(0, std::memory_order_relaxed);
      track_allocations.store(true, std::memory_order_relaxed);
      allocation_countdown.store(successful_allocations_before_failure,
                                 std::memory_order_relaxed);
    }

    ~AllocationFailureScope()
    {
      allocation_countdown.store(-1, std::memory_order_relaxed);
      track_allocations.store(false, std::memory_order_relaxed);
    }

    [[nodiscard]] int balance() const noexcept
    {
      return tracked_balance.load(std::memory_order_relaxed);
    }
  };
}  // namespace

#if !ALEPH_ROPE_TEST_UNDER_TSAN

void * operator new(std::size_t size)
{
  return allocate_or_throw(size);
}

void * operator new[](std::size_t size)
{
  return allocate_or_throw(size);
}

void * operator new(std::size_t size, const std::nothrow_t &) noexcept
{
  try
    {
      return allocate_or_throw(size);
    }
  catch (...)
    {
      return nullptr;
    }
}

void * operator new[](std::size_t size, const std::nothrow_t &) noexcept
{
  try
    {
      return allocate_or_throw(size);
    }
  catch (...)
    {
      return nullptr;
    }
}

void operator delete(void * ptr) noexcept
{
  release_allocation(ptr);
}

void operator delete[](void * ptr) noexcept
{
  release_allocation(ptr);
}

void operator delete(void * ptr, std::size_t) noexcept
{
  release_allocation(ptr);
}

void operator delete[](void * ptr, std::size_t) noexcept
{
  release_allocation(ptr);
}

void operator delete(void * ptr, const std::nothrow_t &) noexcept
{
  release_allocation(ptr);
}

void operator delete[](void * ptr, const std::nothrow_t &) noexcept
{
  release_allocation(ptr);
}

#endif  // !ALEPH_ROPE_TEST_UNDER_TSAN

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

TEST(Rope, IdentityEditsDoNotAllocate)
{
#if ALEPH_ROPE_TEST_UNDER_TSAN
  GTEST_SKIP() << "AllocationFailureScope needs a custom global operator "
                  "new/delete, which conflicts with TSan's own at link time.";
#else
  const Rope<char> r{std::string_view("abcdef")};
  const Rope<char> empty;

  Rope<char> inserted;
  int insert_balance = 0;
  {
    AllocationFailureScope fail_immediately(0);
    EXPECT_NO_THROW(inserted = r.insert(3, empty));
    insert_balance = fail_immediately.balance();
  }
  EXPECT_EQ(insert_balance, 0);
  EXPECT_EQ(inserted.to_string(), "abcdef");

  Rope<char> erased;
  int erase_balance = 0;
  {
    AllocationFailureScope fail_immediately(0);
    EXPECT_NO_THROW(erased = r.erase(3, 0));
    erase_balance = fail_immediately.balance();
  }
  EXPECT_EQ(erase_balance, 0);
  EXPECT_EQ(erased.to_string(), "abcdef");
#endif  // !ALEPH_ROPE_TEST_UNDER_TSAN
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

TEST(Rope, EqualityHandlesMismatchedLeafBoundariesAndSharedSubtrees)
{
  // Same content, but built two different ways so the two trees have
  // different shapes (misaligned leaf boundaries): the lock-step
  // leaf-cursor comparison in operator== must handle a leaf pair that
  // only partially overlaps, not just leaf-for-leaf-aligned trees like
  // EqualityComparesContentNotIdentity above.
  TinyRope whole{std::string_view("abcdefghijkl")};  // one build_from_view split
  TinyRope pieced = TinyRope{std::string_view("ab")}
                        .concat(TinyRope{std::string_view("cde")})
                        .concat(TinyRope{std::string_view("fghij")})
                        .concat(TinyRope{std::string_view("kl")});
  ASSERT_EQ(whole.to_string(), "abcdefghijkl");
  ASSERT_EQ(pieced.to_string(), "abcdefghijkl");
  EXPECT_TRUE(whole == pieced);
  EXPECT_TRUE(pieced == whole);

  // A one-character difference near the end must still be caught even
  // though most of the content (and many leaf boundaries) matches.
  TinyRope pieced_diff = TinyRope{std::string_view("ab")}
                              .concat(TinyRope{std::string_view("cde")})
                              .concat(TinyRope{std::string_view("fghij")})
                              .concat(TinyRope{std::string_view("kX")});
  EXPECT_FALSE(whole == pieced_diff);

  // Structural sharing partially aligned: a rope built by concatenating a
  // shared sub-rope with different tails should still compare correctly
  // whether or not the shared part lines up on leaf boundaries with the
  // other side.
  TinyRope shared_prefix{std::string_view("abcdef")};
  TinyRope left = shared_prefix.concat(TinyRope{std::string_view("ghijkl")});
  TinyRope right = shared_prefix.concat(TinyRope{std::string_view("ghijkl")});
  EXPECT_TRUE(left == right);
  EXPECT_TRUE(left.verify());
  EXPECT_TRUE(right.verify());
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
  // Timing-based assertions are inherently flaky under CI conditions
  // (debug builds, sanitizers, shared/throttled runners), so -- matching
  // the ENABLE_PERF_TESTS convention already used across this repo (see
  // e.g. Tests/math_nt_test.cc, Tests/ntt_test.cc) -- this is opt-in
  // rather than run unconditionally.
  if (not std::getenv("ENABLE_PERF_TESTS"))
    GTEST_SKIP() << "Skipping Rope performance regression (set "
                    "ENABLE_PERF_TESTS=1 to enable)";

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
  // losing the ability to fail loudly on a real regression. Overridable
  // via ROPE_ABSORPTION_MAX_MS for machines where even that isn't enough
  // (matches the ad-hoc TIMSORT_MAX_MS pattern in Tests/sort_utils.cc).
  long max_ms = 400;
  if (const char * env_ms = std::getenv("ROPE_ABSORPTION_MAX_MS"))
    max_ms = std::atol(env_ms);
  EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), max_ms)
      << N << " single-character concats took too long -- the leaf-absorption "
         "fast path may have regressed (see tpl_rope.H's \"Leaf absorption\" note).";
}

// --- Structural sharing (copy is O(1), independent of source size) -------

TEST(Rope, CopyAndSmallConcatStayFastRegardlessOfSourceSize)
{
  // Opt-in only: see the matching comment on
  // RepeatedSmallConcatStaysFastEnoughToProveAbsorptionFired above.
  if (not std::getenv("ENABLE_PERF_TESTS"))
    GTEST_SKIP() << "Skipping Rope performance regression (set "
                    "ENABLE_PERF_TESTS=1 to enable)";

  // Structural-sharing proof by timing: content-only tests (like
  // CopyIsIndependentOfLaterOperationsOnTheOriginalVariable above) prove
  // a copy is *independent*, but not that it is *cheap* -- a rope whose
  // copy constructor accidentally started deep-cloning the tree would
  // still pass every content-based test, just far more slowly. Building
  // one large rope once, then doing many independent copy+small-concat
  // derivations from it, should cost roughly the same per derivation
  // (shared_ptr refcount bump + a short leaf-absorption/wrap) regardless
  // of how large the shared source tree is -- an O(size) copy would blow
  // the budget below by orders of magnitude.
  using R = Rope<char, 256>;
  const std::string big_text(200000, 'x');
  R big{std::string_view(big_text)};
  ASSERT_TRUE(big.verify());

  constexpr int N = 5000;
  const auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < N; ++i)
    {
      R copy = big;                                       // O(1): shares the tree
      R derived = copy.concat(R{std::string_view("!")});  // small, cheap addition
      ASSERT_EQ(derived.size(), big.size() + 1);
    }
  const auto elapsed = std::chrono::steady_clock::now() - start;

  // Overridable via ROPE_SHARING_MAX_MS for slow/loaded machines (see the
  // matching override on the absorption regression test above).
  long max_ms = 500;
  if (const char * env_ms = std::getenv("ROPE_SHARING_MAX_MS"))
    max_ms = std::atol(env_ms);
  EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), max_ms)
      << N << " copy+small-concat derivations from a " << big_text.size()
      << "-character rope took too long -- copy() or concat() may no longer be "
         "O(1)/structurally sharing (see tpl_rope.H's \"Structure\" note).";
}

// --- Exception safety on allocation failure -------------------------------

TEST(Rope, FailedAllocationDuringConcatLeavesExistingRopesUnchanged)
{
#if ALEPH_ROPE_TEST_UNDER_TSAN
  GTEST_SKIP() << "AllocationFailureScope needs a custom global operator "
                  "new/delete, which conflicts with TSan's own at link time.";
#else
  // Build an internal-rooted `left` whose rightmost leaf ("ef") has spare
  // room: "abcd" (4 chars) is already a full leaf at LeafSize=4, so
  // concat-ing "ef" can't absorb and falls back to make_internal,
  // producing one internal node over two leaves.
  const TinyRope left =
      TinyRope{std::string_view("abcd")}.concat(TinyRope{std::string_view("ef")});
  const TinyRope right{std::string_view("g")};
  ASSERT_TRUE(left.verify());
  ASSERT_EQ(left.to_string(), "abcdef");

  // Absorbing "g" into `left` walks one level of the right spine: it
  // allocates the new merged leaf "efg" (try_absorb_right's base case,
  // via make_leaf -- one allocation for the Node, one for its
  // heap-indirected leaf_data, see the Node struct's own comment on why
  // leaf storage isn't embedded inline), then allocates one new internal
  // node wrapping the *unchanged* "abcd" leaf and the new "efg" leaf
  // (try_absorb_right's recursive case) -- three allocations total.
  // Failing after 0, 1, and 2 successful allocations exercises every
  // point along that chain: no work done yet, the leaf's Node built but
  // not its data, and the whole leaf built but not the wrapper.
  for (int allowed = 0; allowed <= 2; ++allowed)
    {
      bool threw = false;
      int balance = 0;
      {
        AllocationFailureScope fail_after(allowed);
        try
          {
            [[maybe_unused]] const TinyRope result = left.concat(right);
          }
        catch (const std::bad_alloc &)
          {
            threw = true;
          }
        balance = fail_after.balance();
      }
      EXPECT_TRUE(threw) << "allowed=" << allowed;
      EXPECT_EQ(balance, 0) << "allocation leaked with allowed=" << allowed;

      // Neither operand was mutated by the failed attempt: concat() only
      // ever builds brand-new nodes and reaches existing ones via
      // shared_ptr copies, so a mid-construction throw has nothing to
      // unwind on either side.
      EXPECT_TRUE(left.verify());
      EXPECT_TRUE(right.verify());
      EXPECT_EQ(left.to_string(), "abcdef");
      EXPECT_EQ(right.to_string(), "g");
    }

  // With enough budget, the same operation succeeds normally afterwards --
  // no leftover poisoned state from the earlier failed attempts.
  const TinyRope combined = left.concat(right);
  EXPECT_TRUE(combined.verify());
  EXPECT_EQ(combined.to_string(), "abcdefg");
#endif  // !ALEPH_ROPE_TEST_UNDER_TSAN
}

// --- substr() on ranges spanning many leaves ------------------------------

TEST(Rope, SubstrOfManyLeafRangeStaysCorrectAndFastRegardlessOfTotalSize)
{
  // Opt-in only: see the matching comment on
  // RepeatedSmallConcatStaysFastEnoughToProveAbsorptionFired above. This
  // test also builds multi-megabyte ropes, which is itself costly to run
  // unconditionally in every default CI run; the underlying correctness
  // property (substr() on multi-leaf ranges) is already covered by
  // RandomizedEditScriptMatchesStdString and SubstrAtExactLeafBoundaries.
  if (not std::getenv("ENABLE_PERF_TESTS"))
    GTEST_SKIP() << "Skipping Rope performance regression (set "
                    "ENABLE_PERF_TESTS=1 to enable)";

  // substr()'s straddle path (see tpl_rope.H's own @note on the method)
  // can, in the worst case, cost up to O(range's leaf count) rather than
  // a strict O(log size()) if the rejoin triggers a rebalance. Extracting
  // a fixed-size range that spans many leaves (~40 at LeafSize=256)
  // should still be correct and should not scale with the *total* rope
  // size -- only with the extracted range itself.
  using R = Rope<char, 256>;
  constexpr size_t extract_len = 10000;  // spans ~40 leaves

  for (const int total : {100000, 1000000, 5000000})
    {
      std::string text(static_cast<size_t>(total), 'x');
      std::mt19937 rng(42);
      for (auto & c : text)
        c = static_cast<char>('a' + (rng() % 26));
      R r{std::string_view(text)};
      ASSERT_TRUE(r.verify());

      const size_t pos = static_cast<size_t>(total) / 2;
      const auto start = std::chrono::steady_clock::now();
      for (int i = 0; i < 200; ++i)
        {
          R sub = r.substr(pos, extract_len);
          ASSERT_EQ(sub.size(), extract_len);
          ASSERT_EQ(sub.to_string(), text.substr(pos, extract_len));
        }
      const double ms = std::chrono::duration<double, std::milli>(
                             std::chrono::steady_clock::now() - start)
                             .count();
      // Loose bound: 200 extractions of a many-leaf range should never
      // take anywhere close to a second, regardless of how large the
      // source rope is. Overridable for slow/loaded machines.
      long max_ms = 2000;
      if (const char * env_ms = std::getenv("ROPE_SLICE_MAX_MS"))
        max_ms = std::atol(env_ms);
      EXPECT_LT(ms, static_cast<double>(max_ms))
          << "200x substr(len=" << extract_len << ") from a " << total
          << "-character rope took too long -- see tpl_rope.H substr()'s "
             "@note on the straddle-rebalance worst case.";
    }
}

// --- Length overflow via structural sharing -------------------------------

TEST(Rope, RepeatedSelfConcatThrowsOverflowErrorInsteadOfWrappingOrHanging)
{
  // Structural sharing makes an astronomically large logical length
  // reachable with very little real work: `r = r.concat(r)` doubles
  // size() each call at O(1) real cost (both sides already share the same
  // subtree), so this reaches SIZE_MAX in ~64 fast iterations -- unlike a
  // naive "you'd need exabytes of real data" argument, which only holds
  // without sharing. Left unchecked, the wrapped length can desync from
  // the real depth and make a later maybe_rebalance() call collect_leaves
  // on an exponentially-shared structure (2^depth root-to-leaf paths over
  // O(depth) real nodes) instead of throwing promptly here.
  Rope<char> r{std::string_view("x")};
  size_t iterations = 0;
  EXPECT_THROW(
      {
        for (; iterations < 100; ++iterations)
          r = r.concat(r);
      },
      std::overflow_error);
  // Reached the limit well before the loop's own safety cap, and the last
  // successful doubling left size() consistent with that many iterations.
  //
  // Deliberately NOT calling r.verify() here: verify_rec(), like
  // collect_leaves(), does not deduplicate shared nodes either, so it
  // would walk this self-shared tree's ~2^60 root-to-leaf paths instead
  // of its ~60 distinct nodes -- the same exponential-blowup shape this
  // test's own overflow guard exists to prevent one level up, just not
  // (yet) guarded against inside verify()/collect_leaves() themselves.
  EXPECT_LT(iterations, 100u);
  EXPECT_EQ(r.size(), size_t{1} << iterations);
}
