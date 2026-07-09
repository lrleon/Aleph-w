
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
 * @file prefix_tree_test.cc
 * @brief Tests for Prefix Tree
 */

/**
 * @file prefix_tree_test.cc
 * @brief Comprehensive test suite for prefix-tree.H (Trie)
 *
 * Tests all aspects of the Cnode prefix tree including:
 * - Node construction and basic operations
 * - Word insertion and search
 * - Prefix search
 * - Tree traversal and word extraction
 * - Cloning
 * - Edge cases
 */

#include <gtest/gtest.h>
#include <prefix-tree.H>
#include <tpl_radix_tree.H>
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <map>
#include <memory>
#include <new>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// ThreadSanitizer ships its own strong global operator new/delete
// replacements (tsan_new_delete.cpp.o). Defining our own below -- needed to
// inject allocation failures for the tests exercising prefix-tree.H's
// exception-safety paths -- collides with those at link time ("multiple
// definition") under -fsanitize=thread. ASan/UBSan/plain Debug builds are
// unaffected; only skip the override (and the tests relying on it) for TSan.
#if defined(__SANITIZE_THREAD__)
#  define ALEPH_PREFIX_TREE_TEST_UNDER_TSAN 1
#elif defined(__has_feature)
#  if __has_feature(thread_sanitizer)
#    define ALEPH_PREFIX_TREE_TEST_UNDER_TSAN 1
#  endif
#endif
#ifndef ALEPH_PREFIX_TREE_TEST_UNDER_TSAN
#  define ALEPH_PREFIX_TREE_TEST_UNDER_TSAN 0
#endif

namespace
{
  std::atomic<int> allocation_countdown{-1};
  std::atomic<bool> track_allocations{false};
  std::atomic<int> tracked_balance{0};

#if !ALEPH_PREFIX_TREE_TEST_UNDER_TSAN
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

    void *ptr = std::malloc(size);
    if (ptr == nullptr)
      throw std::bad_alloc();

    if (track_allocations.load(std::memory_order_relaxed))
      tracked_balance.fetch_add(1, std::memory_order_relaxed);

    return ptr;
  }

  void release_allocation(void *ptr) noexcept
  {
    if (ptr == nullptr)
      return;

    if (track_allocations.load(std::memory_order_relaxed))
      tracked_balance.fetch_sub(1, std::memory_order_relaxed);

    std::free(ptr);
  }
#endif  // !ALEPH_PREFIX_TREE_TEST_UNDER_TSAN

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
}

#if !ALEPH_PREFIX_TREE_TEST_UNDER_TSAN

void *operator new(std::size_t size)
{
  return allocate_or_throw(size);
}

void *operator new[](std::size_t size)
{
  return allocate_or_throw(size);
}

void *operator new(std::size_t size, const std::nothrow_t &) noexcept
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

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept
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

void operator delete(void *ptr) noexcept
{
  release_allocation(ptr);
}

void operator delete[](void *ptr) noexcept
{
  release_allocation(ptr);
}

void operator delete(void *ptr, std::size_t) noexcept
{
  release_allocation(ptr);
}

void operator delete[](void *ptr, std::size_t) noexcept
{
  release_allocation(ptr);
}

void operator delete(void *ptr, const std::nothrow_t &) noexcept
{
  release_allocation(ptr);
}

void operator delete[](void *ptr, const std::nothrow_t &) noexcept
{
  release_allocation(ptr);
}

#endif  // !ALEPH_PREFIX_TREE_TEST_UNDER_TSAN

using namespace Aleph;

static_assert(std::is_same_v<decltype(std::declval<Cnode &>().search_child('a')),
                             Cnode *>);
static_assert(std::is_same_v<decltype(std::declval<const Cnode &>().search_child('a')),
                             const Cnode *>);
static_assert(std::is_same_v<decltype(std::declval<Cnode &>().greater_child('a')),
                             Cnode *>);
static_assert(std::is_same_v<decltype(std::declval<const Cnode &>().greater_child('a')),
                             const Cnode *>);
static_assert(std::is_same_v<decltype(std::declval<Cnode &>().search_prefix("a")),
                             std::tuple<Cnode *, const char *>>);
static_assert(std::is_same_v<decltype(std::declval<const Cnode &>().search_prefix("a")),
                             std::tuple<const Cnode *, const char *>>);
static_assert(std::is_same_v<decltype(std::declval<Prefix_Tree &>().root()),
                             Cnode *>);
static_assert(std::is_same_v<decltype(std::declval<const Prefix_Tree &>().root()),
                             const Cnode *>);
static_assert(std::is_move_constructible_v<Prefix_Tree_Map<int>>);
static_assert(std::is_copy_constructible_v<Prefix_Tree_Map<int>>);
static_assert(std::is_move_constructible_v<Prefix_Tree_Map<std::unique_ptr<int>>>);
static_assert(not std::is_copy_constructible_v<Prefix_Tree_Map<std::unique_ptr<int>>>);

static std::vector<std::string> to_sorted_vector(const DynArray<std::string> & words)
{
  std::vector<std::string> ret;
  words.for_each([&ret](const std::string & w) { ret.push_back(w); });
  std::sort(ret.begin(), ret.end());
  return ret;
}

static std::vector<std::string> to_sorted_vector(const Array<std::string> & words)
{
  std::vector<std::string> ret;
  ret.reserve(words.size());
  for (const auto & w : words)
    ret.push_back(w);
  std::sort(ret.begin(), ret.end());
  return ret;
}

//==============================================================================
// Test Fixture
//==============================================================================

class PrefixTreeTest : public ::testing::Test
{
protected:
  Cnode * root = nullptr;

  void SetUp() override
  {
    root = new Cnode('\0');  // Root with sentinel character
  }

  void TearDown() override
  {
    if (root)
      {
        root->destroy();
        delete root;
      }
  }
};

//==============================================================================
// Basic Node Tests
//==============================================================================

TEST_F(PrefixTreeTest, NodeConstruction)
{
  Cnode node('a');
  EXPECT_EQ(node.symbol(), 'a');
}

TEST_F(PrefixTreeTest, NodeSymbol)
{
  Cnode node('x');
  EXPECT_EQ(node.symbol(), 'x');
  
  Cnode node2('$');
  EXPECT_EQ(node2.symbol(), '$');
}

TEST_F(PrefixTreeTest, InitiallyNoChildren)
{
  Cnode node('a');
  EXPECT_TRUE(node.children().is_empty());
}

TEST_F(PrefixTreeTest, InitiallyNotEndWord)
{
  Cnode node('a');
  EXPECT_FALSE(node.is_end_word());
}

TEST_F(PrefixTreeTest, MarkEndWord)
{
  Cnode node('a');
  EXPECT_FALSE(node.is_end_word());
  
  node.mark_end_word();
  EXPECT_TRUE(node.is_end_word());
  EXPECT_TRUE(node.children().is_empty());
}

//==============================================================================
// Child Operations Tests
//==============================================================================

TEST_F(PrefixTreeTest, SearchChildNotFound)
{
  EXPECT_EQ(root->search_child('a'), nullptr);
}

TEST_F(PrefixTreeTest, InsertAndSearchChild)
{
  auto * child = new Cnode('a');
  root->insert_child(child);
  
  EXPECT_EQ(root->search_child('a'), child);
  EXPECT_EQ(root->search_child('b'), nullptr);
}

TEST_F(PrefixTreeTest, ChildrenSortedOrder)
{
  root->insert_child(new Cnode('c'));
  root->insert_child(new Cnode('a'));
  root->insert_child(new Cnode('b'));
  
  auto kids = root->children();
  ASSERT_EQ(kids.size(), 3);
  
  // Verify sorted order
  auto it = kids.get_it();
  EXPECT_EQ(it.get_curr()->symbol(), 'a');
  it.next();
  EXPECT_EQ(it.get_curr()->symbol(), 'b');
  it.next();
  EXPECT_EQ(it.get_curr()->symbol(), 'c');
}

TEST_F(PrefixTreeTest, GreaterChild)
{
  root->insert_child(new Cnode('a'));
  root->insert_child(new Cnode('c'));
  root->insert_child(new Cnode('e'));
  
  EXPECT_EQ(root->greater_child('b')->symbol(), 'c');
  EXPECT_EQ(root->greater_child('d')->symbol(), 'e');
  EXPECT_EQ(root->greater_child('e'), nullptr);
}

//==============================================================================
// Word Insertion Tests
//==============================================================================

TEST_F(PrefixTreeTest, InsertSingleWord)
{
  EXPECT_TRUE(root->insert_word("hello"));
  EXPECT_TRUE(root->contains("hello"));
}

TEST_F(PrefixTreeTest, InsertDuplicateWord)
{
  EXPECT_TRUE(root->insert_word("hello"));
  EXPECT_FALSE(root->insert_word("hello"));  // Already exists
}

TEST_F(PrefixTreeTest, InsertMultipleWords)
{
  EXPECT_TRUE(root->insert_word("hello"));
  EXPECT_TRUE(root->insert_word("help"));
  EXPECT_TRUE(root->insert_word("world"));
  
  EXPECT_TRUE(root->contains("hello"));
  EXPECT_TRUE(root->contains("help"));
  EXPECT_TRUE(root->contains("world"));
}

TEST_F(PrefixTreeTest, InsertWordsWithCommonPrefix)
{
  EXPECT_TRUE(root->insert_word("test"));
  EXPECT_TRUE(root->insert_word("testing"));
  EXPECT_TRUE(root->insert_word("tester"));
  
  EXPECT_TRUE(root->contains("test"));
  EXPECT_TRUE(root->contains("testing"));
  EXPECT_TRUE(root->contains("tester"));
}

TEST_F(PrefixTreeTest, InsertEmptyString)
{
  EXPECT_TRUE(root->insert_word(""));
  EXPECT_TRUE(root->contains(""));
  EXPECT_FALSE(root->insert_word(""));  // Already exists
}

TEST_F(PrefixTreeTest, InsertSingleCharacter)
{
  EXPECT_TRUE(root->insert_word("a"));
  EXPECT_TRUE(root->contains("a"));
}

TEST_F(PrefixTreeTest, WordEndSurvivesLowerSortedChild)
{
  EXPECT_TRUE(root->insert_word("a"));
  EXPECT_TRUE(root->insert_word("a!"));

  EXPECT_TRUE(root->contains("a"));
  EXPECT_TRUE(root->contains("a!"));
  EXPECT_EQ(root->count(), 2);

  EXPECT_EQ(to_sorted_vector(root->words()), (std::vector<std::string>{"a", "a!"}));
  EXPECT_EQ(to_sorted_vector(root->words_with_prefix("a")),
            (std::vector<std::string>{"a", "a!"}));
}

TEST_F(PrefixTreeTest, InsertWordCleansDetachedPathOnAllocationFailure)
{
#if ALEPH_PREFIX_TREE_TEST_UNDER_TSAN
  GTEST_SKIP() << "AllocationFailureScope needs a custom global operator "
                  "new/delete, which conflicts with TSan's own at link time.";
#endif
  EXPECT_TRUE(root->insert_word("mango"));
  const auto before = to_sorted_vector(root->words());

  bool threw = false;
  int balance = 0;
  {
    AllocationFailureScope fail_after_one_node(2);
    try
      {
        root->insert_word("abc");
      }
    catch (const std::bad_alloc &)
      {
        threw = true;
      }
    balance = fail_after_one_node.balance();
  }

  EXPECT_TRUE(threw);
  EXPECT_EQ(balance, 0);
  EXPECT_EQ(to_sorted_vector(root->words()), before);
  EXPECT_TRUE(root->contains("mango"));
  EXPECT_FALSE(root->contains("a"));
  EXPECT_FALSE(root->contains("ab"));
  EXPECT_FALSE(root->contains("abc"));
  EXPECT_EQ(root->count(), 1);
}

TEST_F(PrefixTreeTest, PrefixNotAWord)
{
  EXPECT_TRUE(root->insert_word("testing"));
  
  EXPECT_TRUE(root->contains("testing"));
  EXPECT_FALSE(root->contains("test"));
  EXPECT_FALSE(root->contains("tes"));
  EXPECT_FALSE(root->contains("te"));
  EXPECT_FALSE(root->contains("t"));
}

TEST_F(PrefixTreeTest, WordThenPrefix)
{
  EXPECT_TRUE(root->insert_word("test"));
  EXPECT_TRUE(root->insert_word("testing"));
  
  EXPECT_TRUE(root->contains("test"));
  EXPECT_TRUE(root->contains("testing"));
}

TEST_F(PrefixTreeTest, PrefixThenWord)
{
  EXPECT_TRUE(root->insert_word("testing"));
  EXPECT_TRUE(root->insert_word("test"));
  
  EXPECT_TRUE(root->contains("test"));
  EXPECT_TRUE(root->contains("testing"));
}

//==============================================================================
// Search Tests
//==============================================================================

TEST_F(PrefixTreeTest, SearchWordNotFound)
{
  root->insert_word("hello");
  
  EXPECT_EQ(root->search_word("world"), nullptr);
  EXPECT_EQ(root->search_word("hel"), nullptr);
}

TEST_F(PrefixTreeTest, SearchWordFound)
{
  root->insert_word("hello");
  
  auto result = root->search_word("hello");
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->symbol(), 'o');
}

TEST_F(PrefixTreeTest, ContainsNonExistent)
{
  root->insert_word("hello");
  
  EXPECT_FALSE(root->contains("world"));
  EXPECT_FALSE(root->contains("helloworld"));
  EXPECT_FALSE(root->contains("hell"));
}

//==============================================================================
// Prefix Search Tests
//==============================================================================

TEST_F(PrefixTreeTest, SearchPrefixEmpty)
{
  auto [node, remaining] = root->search_prefix("");
  
  EXPECT_EQ(node, root);
  EXPECT_STREQ(remaining, "");
}

TEST_F(PrefixTreeTest, SearchPrefixFullMatch)
{
  root->insert_word("hello");
  
  auto [node, remaining] = root->search_prefix("hel");
  
  EXPECT_EQ(node->symbol(), 'l');
  EXPECT_STREQ(remaining, "");
}

TEST_F(PrefixTreeTest, SearchPrefixPartialMatch)
{
  root->insert_word("hello");
  
  auto [node, remaining] = root->search_prefix("helping");
  
  EXPECT_EQ(node->symbol(), 'l');
  EXPECT_STREQ(remaining, "ping");
}

TEST_F(PrefixTreeTest, SearchPrefixNoMatch)
{
  root->insert_word("hello");
  
  auto [node, remaining] = root->search_prefix("world");
  
  EXPECT_EQ(node, root);
  EXPECT_STREQ(remaining, "world");
}

//==============================================================================
// Words Extraction Tests
//==============================================================================

TEST_F(PrefixTreeTest, WordsEmpty)
{
  auto words = root->words();
  EXPECT_TRUE(words.is_empty());
}

TEST_F(PrefixTreeTest, WordsSingle)
{
  root->insert_word("hello");
  
  auto words = root->words();
  ASSERT_EQ(words.size(), 1);
  EXPECT_EQ(std::string(words[0]), "hello");
}

TEST_F(PrefixTreeTest, WordsMultiple)
{
  root->insert_word("hello");
  root->insert_word("help");
  root->insert_word("world");
  
  auto words = root->words();
  ASSERT_EQ(words.size(), 3);
  
  // Convert to vector for easier checking
  std::vector<std::string> v;
  words.for_each([&v](const std::string& w) { v.push_back(w); });
  std::sort(v.begin(), v.end());
  
  EXPECT_EQ(v[0], std::string("hello"));
  EXPECT_EQ(v[1], std::string("help"));
  EXPECT_EQ(v[2], std::string("world"));
}

TEST_F(PrefixTreeTest, WordsWithCommonPrefixes)
{
  root->insert_word("a");
  root->insert_word("ab");
  root->insert_word("abc");
  root->insert_word("abcd");
  
  auto words = root->words();
  ASSERT_EQ(words.size(), 4);
}

TEST_F(PrefixTreeTest, WordsCanContainDollarCharacter)
{
  EXPECT_TRUE(root->insert_word("$"));
  EXPECT_TRUE(root->insert_word("a$"));
  EXPECT_TRUE(root->insert_word("a$b"));

  EXPECT_TRUE(root->contains("$"));
  EXPECT_TRUE(root->contains("a$"));
  EXPECT_TRUE(root->contains("a$b"));

  EXPECT_EQ(to_sorted_vector(root->words()),
            (std::vector<std::string>{"$", "a$", "a$b"}));
}

TEST_F(PrefixTreeTest, WordsCanContainEmptyString)
{
  EXPECT_TRUE(root->insert_word(""));
  EXPECT_TRUE(root->insert_word("!"));

  EXPECT_TRUE(root->contains(""));
  EXPECT_TRUE(root->contains("!"));
  EXPECT_EQ(to_sorted_vector(root->words()),
            (std::vector<std::string>{"", "!"}));
}

//==============================================================================
// Clone Tests
//==============================================================================

TEST_F(PrefixTreeTest, CloneEmpty)
{
  Cnode * cloned = root->clone();
  
  EXPECT_EQ(cloned->symbol(), root->symbol());
  EXPECT_TRUE(cloned->children().is_empty());
  
  cloned->destroy();
  delete cloned;
}

TEST_F(PrefixTreeTest, CloneWithWords)
{
  root->insert_word("hello");
  root->insert_word("help");
  root->insert_word("world");
  
  Cnode * cloned = root->clone();
  
  // Verify cloned tree has same words
  EXPECT_TRUE(cloned->contains("hello"));
  EXPECT_TRUE(cloned->contains("help"));
  EXPECT_TRUE(cloned->contains("world"));
  
  // Verify independence - modify original
  root->insert_word("test");
  EXPECT_TRUE(root->contains("test"));
  EXPECT_FALSE(cloned->contains("test"));
  
  cloned->destroy();
  delete cloned;
}

TEST_F(PrefixTreeTest, ClonePreservesWordEndFlags)
{
  root->insert_word("");
  root->insert_word("$");
  root->insert_word("a!");
  root->insert_word("a");

  Cnode * cloned = root->clone();

  EXPECT_TRUE(cloned->contains(""));
  EXPECT_TRUE(cloned->contains("$"));
  EXPECT_TRUE(cloned->contains("a!"));
  EXPECT_TRUE(cloned->contains("a"));
  EXPECT_EQ(to_sorted_vector(cloned->words()),
            (std::vector<std::string>{"", "$", "a", "a!"}));

  cloned->destroy();
  delete cloned;
}

TEST_F(PrefixTreeTest, CloneCleansPartialCopyOnAllocationFailure)
{
#if ALEPH_PREFIX_TREE_TEST_UNDER_TSAN
  GTEST_SKIP() << "AllocationFailureScope needs a custom global operator "
                  "new/delete, which conflicts with TSan's own at link time.";
#endif
  root->insert_word("alpha");
  root->insert_word("beta");
  root->insert_word("$");
  const auto before = to_sorted_vector(root->words());

  bool threw = false;
  int balance = 0;
  {
    AllocationFailureScope fail_after_two_nodes(2);
    try
      {
        Cnode *cloned = root->clone();
        cloned->destroy();
        delete cloned;
      }
    catch (const std::bad_alloc &)
      {
        threw = true;
      }
    balance = fail_after_two_nodes.balance();
  }

  EXPECT_TRUE(threw);
  EXPECT_EQ(balance, 0);
  EXPECT_EQ(to_sorted_vector(root->words()), before);
  EXPECT_TRUE(root->contains("alpha"));
  EXPECT_TRUE(root->contains("beta"));
  EXPECT_TRUE(root->contains("$"));
}

//==============================================================================
// To String Tests
//==============================================================================

TEST_F(PrefixTreeTest, ToStringEmpty)
{
  std::string str = root->to_str();
  // Root has null character
  EXPECT_FALSE(str.empty());
}

TEST_F(PrefixTreeTest, ToStringWithWord)
{
  root->insert_word("ab");
  std::string str = root->to_str();
  
  // Should contain 'a' and 'b' somewhere
  EXPECT_NE(str.find('a'), std::string::npos);
  EXPECT_NE(str.find('b'), std::string::npos);
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST_F(PrefixTreeTest, LongWord)
{
  std::string longWord(20, 'a');  // Keep short to avoid stack overflow in destroy_tree
  EXPECT_TRUE(root->insert_word(longWord));
  EXPECT_TRUE(root->contains(longWord));
}

TEST_F(PrefixTreeTest, ManyWords)
{
  const int N = 50;  // Reduced to avoid stack issues
  for (int i = 0; i < N; ++i)
    root->insert_word("w" + std::to_string(i));
  
  for (int i = 0; i < N; ++i)
    EXPECT_TRUE(root->contains("w" + std::to_string(i)));
  
  auto words = root->words();
  EXPECT_EQ(words.size(), N);
}

TEST_F(PrefixTreeTest, SpecialCharacters)
{
  EXPECT_TRUE(root->insert_word("hello-world"));
  EXPECT_TRUE(root->insert_word("test_case"));
  EXPECT_TRUE(root->insert_word("foo.bar"));
  
  EXPECT_TRUE(root->contains("hello-world"));
  EXPECT_TRUE(root->contains("test_case"));
  EXPECT_TRUE(root->contains("foo.bar"));
}

TEST_F(PrefixTreeTest, NumericStrings)
{
  EXPECT_TRUE(root->insert_word("123"));
  EXPECT_TRUE(root->insert_word("456"));
  
  EXPECT_TRUE(root->contains("123"));
  EXPECT_TRUE(root->contains("456"));
  EXPECT_FALSE(root->contains("789"));
}

//==============================================================================
// Count Tests
//==============================================================================

TEST_F(PrefixTreeTest, CountEmpty)
{
  EXPECT_EQ(root->count(), 0);
}

TEST_F(PrefixTreeTest, CountSingle)
{
  root->insert_word("hello");
  EXPECT_EQ(root->count(), 1);
}

TEST_F(PrefixTreeTest, CountMultiple)
{
  root->insert_word("hello");
  root->insert_word("help");
  root->insert_word("world");
  EXPECT_EQ(root->count(), 3);
}

TEST_F(PrefixTreeTest, CountWithPrefixes)
{
  root->insert_word("a");
  root->insert_word("ab");
  root->insert_word("abc");
  EXPECT_EQ(root->count(), 3);
}

//==============================================================================
// Words With Prefix Tests
//==============================================================================

TEST_F(PrefixTreeTest, WordsWithPrefixEmpty)
{
  root->insert_word("hello");
  auto words = root->words_with_prefix("xyz");
  EXPECT_TRUE(words.is_empty());
}

TEST_F(PrefixTreeTest, WordsWithPrefixMatch)
{
  root->insert_word("hello");
  root->insert_word("help");
  root->insert_word("helicopter");
  root->insert_word("world");
  
  auto words = root->words_with_prefix("hel");
  EXPECT_EQ(words.size(), 3);
}

TEST_F(PrefixTreeTest, WordsWithPrefixExactWord)
{
  root->insert_word("test");
  root->insert_word("testing");
  root->insert_word("tester");

  const auto words = root->words_with_prefix("test");
  EXPECT_EQ(words.size(), 3);  // test, testing, tester
}

TEST_F(PrefixTreeTest, WordsWithPrefixNoMatch)
{
  root->insert_word("apple");
  root->insert_word("application");
  
  auto words = root->words_with_prefix("ban");
  EXPECT_TRUE(words.is_empty());
}

TEST_F(PrefixTreeTest, WordsWithEmptyPrefixMatchesWords)
{
  root->insert_word("");
  root->insert_word("hello");
  root->insert_word("help");
  root->insert_word("world");

  EXPECT_EQ(to_sorted_vector(root->words_with_prefix("")),
            to_sorted_vector(root->words()));
}

TEST(PrefixTreeDestroyTest, DestroyWorks)
{
  // Use a separate test without the fixture to avoid double-free
  auto * tree = new Cnode('\0');
  tree->insert_word("hi");
  tree->insert_word("bye");
  
  EXPECT_TRUE(tree->contains("hi"));
  EXPECT_TRUE(tree->contains("bye"));
  
  // destroy() calls destroy_tree on children which deletes them
  tree->destroy();
  delete tree;
  // If we get here without crashing, destroy worked
}

//==============================================================================
// Owning Wrapper Tests
//==============================================================================

TEST(PrefixTreeWrapperTest, DelegatesWordOperations)
{
  Prefix_Tree tree;

  EXPECT_TRUE(tree.insert_word(""));
  EXPECT_TRUE(tree.insert_word("alpha"));
  EXPECT_TRUE(tree.insert_word("alphabet"));
  EXPECT_FALSE(tree.insert_word("alpha"));

  EXPECT_TRUE(tree.contains(""));
  EXPECT_TRUE(tree.contains("alpha"));
  EXPECT_TRUE(tree.contains("alphabet"));
  EXPECT_FALSE(tree.contains("alpine"));
  EXPECT_EQ(tree.count(), 3);
  EXPECT_EQ(tree.size(), 3);
  EXPECT_EQ(to_sorted_vector(tree.words()),
            (std::vector<std::string>{"", "alpha", "alphabet"}));
  EXPECT_EQ(to_sorted_vector(tree.words_with_prefix("alpha")),
            (std::vector<std::string>{"alpha", "alphabet"}));
}

TEST(PrefixTreeWrapperTest, CachedCountTracksWrapperInsertions)
{
  Prefix_Tree tree;

  EXPECT_EQ(tree.count(), 0);
  EXPECT_EQ(tree.size(), 0);
  EXPECT_TRUE(tree.insert_word("alpha"));
  EXPECT_TRUE(tree.insert_word("beta"));
  EXPECT_FALSE(tree.insert_word("alpha"));
  EXPECT_EQ(tree.count(), 2);
  EXPECT_EQ(tree.size(), 2);
}

TEST(PrefixTreeWrapperTest, CachedCountRecomputesAfterMutableRootAccess)
{
  Prefix_Tree tree;
  ASSERT_TRUE(tree.insert_word("alpha"));
  EXPECT_EQ(tree.count(), 1);

  Cnode *root = tree.root();
  ASSERT_NE(root, nullptr);
  EXPECT_TRUE(root->insert_word("beta"));
  EXPECT_TRUE(root->insert_word("gamma"));
  EXPECT_TRUE(tree.contains("beta"));
  EXPECT_TRUE(tree.contains("gamma"));

  EXPECT_EQ(tree.count(), 3);
  EXPECT_EQ(tree.size(), 3);
  EXPECT_TRUE(tree.insert_word("delta"));
  EXPECT_EQ(tree.count(), 4);
  EXPECT_TRUE(root->insert_word("epsilon"));
  EXPECT_EQ(tree.count(), 5);
  EXPECT_EQ(tree.size(), 5);
}

TEST(PrefixTreeWrapperTest, OwnsRootAndDestroysNodes)
{
#if ALEPH_PREFIX_TREE_TEST_UNDER_TSAN
  GTEST_SKIP() << "AllocationFailureScope needs a custom global operator "
                  "new/delete, which conflicts with TSan's own at link time.";
#endif
  size_t count = 0;
  int balance = 0;
  {
    AllocationFailureScope track_only(-1);
    {
      Prefix_Tree tree;
      tree.insert_word("alpha");
      tree.insert_word("beta");
      count = tree.count();
    }
    balance = track_only.balance();
  }

  EXPECT_EQ(count, 2);
  EXPECT_EQ(balance, 0);
}

TEST(PrefixTreeWrapperTest, RootExposesLowLevelAccessWithoutOwnershipTransfer)
{
  Prefix_Tree tree;
  const Prefix_Tree &const_tree = tree;

  ASSERT_NE(tree.root(), nullptr);
  EXPECT_EQ(tree.root()->symbol(), '\0');
  EXPECT_EQ(const_tree.root(), tree.root());

  EXPECT_TRUE(tree.root()->insert_word("raw"));
  EXPECT_TRUE(tree.contains("raw"));
}

TEST(PrefixTreeWrapperTest, CopyConstructorCreatesIndependentTree)
{
  Prefix_Tree tree;
  tree.insert_word("alpha");
  tree.insert_word("$");

  Prefix_Tree copy(tree);
  tree.insert_word("beta");
  copy.insert_word("gamma");

  EXPECT_TRUE(copy.contains("alpha"));
  EXPECT_TRUE(copy.contains("$"));
  EXPECT_TRUE(copy.contains("gamma"));
  EXPECT_FALSE(copy.contains("beta"));
  EXPECT_TRUE(tree.contains("beta"));
  EXPECT_FALSE(tree.contains("gamma"));
  EXPECT_EQ(tree.count(), 3);
  EXPECT_EQ(copy.count(), 3);
}

TEST(PrefixTreeWrapperTest, CopyAssignmentCreatesIndependentTree)
{
  Prefix_Tree source;
  source.insert_word("alpha");
  source.insert_word("beta");

  Prefix_Tree target;
  target.insert_word("old");

  target = source;
  source.insert_word("gamma");
  target.insert_word("delta");

  EXPECT_TRUE(target.contains("alpha"));
  EXPECT_TRUE(target.contains("beta"));
  EXPECT_TRUE(target.contains("delta"));
  EXPECT_FALSE(target.contains("old"));
  EXPECT_FALSE(target.contains("gamma"));
  EXPECT_TRUE(source.contains("gamma"));

  target = target;
  EXPECT_TRUE(target.contains("alpha"));
  EXPECT_TRUE(target.contains("delta"));
}

TEST(PrefixTreeWrapperTest, MoveConstructorTransfersOwnership)
{
  Prefix_Tree source;
  source.insert_word("alpha");
  source.insert_word("$");

  Prefix_Tree moved(std::move(source));

  EXPECT_TRUE(moved.contains("alpha"));
  EXPECT_TRUE(moved.contains("$"));
  EXPECT_EQ(moved.count(), 2);

  EXPECT_EQ(source.count(), 0);
  EXPECT_FALSE(source.contains("alpha"));
  EXPECT_TRUE(source.words().is_empty());
  EXPECT_TRUE(source.words_with_prefix("a").is_empty());
  EXPECT_TRUE(source.insert_word("reused"));
  EXPECT_TRUE(source.contains("reused"));
  EXPECT_EQ(source.count(), 1);
}

TEST(PrefixTreeWrapperTest, MoveAssignmentTransfersOwnershipAndFreesOldRoot)
{
  Prefix_Tree source;
  source.insert_word("alpha");
  source.insert_word("beta");

  Prefix_Tree target;
  target.insert_word("old");

  target = std::move(source);

  EXPECT_TRUE(target.contains("alpha"));
  EXPECT_TRUE(target.contains("beta"));
  EXPECT_FALSE(target.contains("old"));
  EXPECT_EQ(target.count(), 2);

  EXPECT_EQ(source.count(), 0);
  EXPECT_FALSE(source.contains("alpha"));
  EXPECT_TRUE(source.words().is_empty());
  EXPECT_TRUE(source.words_with_prefix("a").is_empty());
  EXPECT_TRUE(source.insert_word("reused"));
  EXPECT_TRUE(source.contains("reused"));
  EXPECT_EQ(source.count(), 1);
}

TEST(PrefixTreeWrapperTest, CopyAssignmentKeepsOldTreeOnAllocationFailure)
{
#if ALEPH_PREFIX_TREE_TEST_UNDER_TSAN
  GTEST_SKIP() << "AllocationFailureScope needs a custom global operator "
                  "new/delete, which conflicts with TSan's own at link time.";
#endif
  Prefix_Tree source;
  source.insert_word("alpha");
  source.insert_word("beta");
  source.insert_word("$");

  Prefix_Tree target;
  target.insert_word("old");
  const auto before = to_sorted_vector(target.words());

  bool threw = false;
  int balance = 0;
  {
    AllocationFailureScope fail_after_two_nodes(2);
    try
      {
        target = source;
      }
    catch (const std::bad_alloc &)
      {
        threw = true;
      }
    balance = fail_after_two_nodes.balance();
  }

  EXPECT_TRUE(threw);
  EXPECT_EQ(balance, 0);
  EXPECT_EQ(to_sorted_vector(target.words()), before);
  EXPECT_TRUE(target.contains("old"));
  EXPECT_FALSE(target.contains("alpha"));
  EXPECT_FALSE(target.contains("beta"));
  EXPECT_FALSE(target.contains("$"));
}

//==============================================================================
// Prefix_Tree_Map Tests
//==============================================================================

TEST(PrefixTreeMapTest, DefaultConstructedMapIsEmpty)
{
  Prefix_Tree_Map<int> map;

  EXPECT_TRUE(map.is_empty());
  EXPECT_EQ(map.size(), 0);
  EXPECT_EQ(map.count(), 0);
  EXPECT_FALSE(map.contains("alpha"));
  EXPECT_EQ(map.find("alpha"), nullptr);
  EXPECT_TRUE(map.words().is_empty());
}

TEST(PrefixTreeMapTest, InsertAndFindValues)
{
  Prefix_Tree_Map<int> map;

  EXPECT_TRUE(map.insert("", 0));
  EXPECT_TRUE(map.insert("alpha", 1));
  EXPECT_TRUE(map.insert("alphabet", 2));

  ASSERT_NE(map.find(""), nullptr);
  ASSERT_NE(map.find("alpha"), nullptr);
  ASSERT_NE(map.find("alphabet"), nullptr);
  EXPECT_EQ(*map.find(""), 0);
  EXPECT_EQ(*map.find("alpha"), 1);
  EXPECT_EQ(*map.find("alphabet"), 2);
  EXPECT_TRUE(map.contains(""));
  EXPECT_TRUE(map.contains("alpha"));
  EXPECT_FALSE(map.contains("alpine"));
  EXPECT_EQ(map.size(), 3);
}

TEST(PrefixTreeMapTest, DuplicateInsertKeepsOriginalValue)
{
  Prefix_Tree_Map<int> map;

  EXPECT_TRUE(map.insert("alpha", 1));
  EXPECT_FALSE(map.insert("alpha", 99));
  ASSERT_NE(map.find("alpha"), nullptr);
  EXPECT_EQ(*map.find("alpha"), 1);
  EXPECT_EQ(map.size(), 1);
}

TEST(PrefixTreeMapTest, DuplicateMoveInsertDoesNotConsumeValue)
{
  Prefix_Tree_Map<std::unique_ptr<int>> map;
  ASSERT_TRUE(map.insert("alpha", std::make_unique<int>(1)));

  auto duplicate = std::make_unique<int>(2);
  EXPECT_FALSE(map.insert("alpha", std::move(duplicate)));
  ASSERT_NE(duplicate, nullptr);
  EXPECT_EQ(*duplicate, 2);
  ASSERT_NE(map.find("alpha"), nullptr);
  ASSERT_NE(*map.find("alpha"), nullptr);
  EXPECT_EQ(**map.find("alpha"), 1);
}

TEST(PrefixTreeMapTest, InsertOrAssignInsertsThenOverwrites)
{
  Prefix_Tree_Map<std::string> map;

  map.insert_or_assign("alpha", "one");
  ASSERT_NE(map.find("alpha"), nullptr);
  EXPECT_EQ(*map.find("alpha"), "one");

  map.insert_or_assign("alpha", "uno");
  ASSERT_NE(map.find("alpha"), nullptr);
  EXPECT_EQ(*map.find("alpha"), "uno");

  map.insert_or_assign("beta", "two");
  ASSERT_NE(map.find("beta"), nullptr);
  EXPECT_EQ(*map.find("beta"), "two");
  EXPECT_EQ(map.size(), 2);
}

TEST(PrefixTreeMapTest, MutableFindAllowsInPlaceUpdate)
{
  Prefix_Tree_Map<int> map;
  ASSERT_TRUE(map.insert("alpha", 1));

  int *slot = map.find("alpha");
  ASSERT_NE(slot, nullptr);
  *slot = 42;

  const auto &const_map = map;
  ASSERT_NE(const_map.find("alpha"), nullptr);
  EXPECT_EQ(*const_map.find("alpha"), 42);
}

TEST(PrefixTreeMapTest, EraseRemovesLogicalKeyAndAllowsReinsert)
{
  Prefix_Tree_Map<int> map;
  ASSERT_TRUE(map.insert("alpha", 1));
  ASSERT_TRUE(map.insert("alphabet", 2));

  EXPECT_TRUE(map.erase("alpha"));
  EXPECT_FALSE(map.contains("alpha"));
  EXPECT_TRUE(map.contains("alphabet"));
  EXPECT_FALSE(map.erase("alpha"));
  EXPECT_EQ(map.size(), 1);

  EXPECT_TRUE(map.insert("alpha", 3));
  ASSERT_NE(map.find("alpha"), nullptr);
  EXPECT_EQ(*map.find("alpha"), 3);
  EXPECT_EQ(map.size(), 2);
}

TEST(PrefixTreeMapTest, WordsAndWordsWithPrefixReturnStoredKeys)
{
  Prefix_Tree_Map<int> map;
  ASSERT_TRUE(map.insert("", 0));
  ASSERT_TRUE(map.insert("app", 1));
  ASSERT_TRUE(map.insert("apple", 2));
  ASSERT_TRUE(map.insert("banana", 3));

  EXPECT_EQ(to_sorted_vector(map.words()),
            (std::vector<std::string>{"", "app", "apple", "banana"}));
  EXPECT_EQ(to_sorted_vector(map.words_with_prefix("app")),
            (std::vector<std::string>{"app", "apple"}));
  EXPECT_TRUE(map.words_with_prefix("cat").is_empty());
}

TEST(PrefixTreeMapTest, CopyConstructorCreatesIndependentClone)
{
  Prefix_Tree_Map<std::string> map;
  ASSERT_TRUE(map.insert("alpha", "one"));
  ASSERT_TRUE(map.insert("beta", "two"));

  Prefix_Tree_Map<std::string> copy(map);
  copy.insert_or_assign("alpha", "uno");
  copy.insert_or_assign("gamma", "three");

  ASSERT_NE(map.find("alpha"), nullptr);
  ASSERT_NE(copy.find("alpha"), nullptr);
  EXPECT_EQ(*map.find("alpha"), "one");
  EXPECT_EQ(*copy.find("alpha"), "uno");
  EXPECT_FALSE(map.contains("gamma"));
  EXPECT_TRUE(copy.contains("gamma"));
  EXPECT_EQ(map.size(), 2);
  EXPECT_EQ(copy.size(), 3);
}

TEST(PrefixTreeMapTest, MoveConstructorTransfersContents)
{
  Prefix_Tree_Map<int> source;
  ASSERT_TRUE(source.insert("alpha", 1));
  ASSERT_TRUE(source.insert("beta", 2));

  Prefix_Tree_Map<int> moved(std::move(source));

  EXPECT_TRUE(source.is_empty());
  EXPECT_TRUE(moved.contains("alpha"));
  EXPECT_TRUE(moved.contains("beta"));
  ASSERT_NE(moved.find("alpha"), nullptr);
  ASSERT_NE(moved.find("beta"), nullptr);
  EXPECT_EQ(*moved.find("alpha"), 1);
  EXPECT_EQ(*moved.find("beta"), 2);
}

TEST(PrefixTreeMapTest, ClearRemovesAllKeys)
{
  Prefix_Tree_Map<int> map;
  ASSERT_TRUE(map.insert("alpha", 1));
  ASSERT_TRUE(map.insert("beta", 2));

  map.clear();

  EXPECT_TRUE(map.is_empty());
  EXPECT_FALSE(map.contains("alpha"));
  EXPECT_FALSE(map.contains("beta"));
  EXPECT_TRUE(map.words().is_empty());
}

TEST(PrefixTreeMapTest, RandomizedOperationsMatchStdMap)
{
  const std::vector<std::string> keys =
  {
    "", "a", "app", "apple", "application", "banana", "band", "bandana",
    "car", "carbon", "cart", "dog", "door", "dorm"
  };

  Prefix_Tree_Map<int> subject;
  std::map<std::string, int> reference;

  for (size_t step = 0; step < 500; ++step)
    {
      const std::string &key = keys[(step * 7 + 3) % keys.size()];
      const int value = static_cast<int>(step);

      if ((step % 4) == 0)
        EXPECT_EQ(subject.insert(key, value),
                  reference.emplace(key, value).second);
      else if ((step % 4) == 1)
        {
          subject.insert_or_assign(key, value);
          reference[key] = value;
        }
      else if ((step % 4) == 2)
        EXPECT_EQ(subject.erase(key), reference.erase(key) != 0);
      else
        {
          const int *subject_value = subject.find(key);
          const auto it = reference.find(key);
          ASSERT_EQ(subject_value != nullptr, it != reference.end());
          if (subject_value != nullptr)
            EXPECT_EQ(*subject_value, it->second);
        }

      ASSERT_EQ(subject.size(), reference.size()) << "step " << step;
    }

  std::vector<std::string> expected;
  for (const auto &kv : reference)
    expected.push_back(kv.first);
  EXPECT_EQ(to_sorted_vector(subject.words()), expected);
  for (const auto &kv : reference)
    {
      ASSERT_NE(subject.find(kv.first), nullptr);
      EXPECT_EQ(*subject.find(kv.first), kv.second);
    }
}

TEST(PrefixTreeMapTest, MatchesPrefixTreeAndRadixTree)
{
  const std::vector<std::string> words =
  {
    "", "app", "apple", "application", "apt", "banana", "band",
    "bandana", "car", "carbon", "cart"
  };

  Prefix_Tree_Map<int> subject;
  Prefix_Tree reference_set;
  RadixTree<int> reference_map;

  for (size_t i = 0; i < words.size(); ++i)
    {
      const int value = static_cast<int>(i * 10);
      ASSERT_EQ(subject.insert(words[i], value), reference_set.insert_word(words[i]));
      ASSERT_TRUE(reference_map.insert(words[i], value));
    }

  for (size_t i = 0; i < words.size(); ++i)
    {
      ASSERT_TRUE(subject.contains(words[i]));
      ASSERT_TRUE(reference_set.contains(words[i]));
      ASSERT_NE(subject.find(words[i]), nullptr);
      ASSERT_NE(reference_map.find(words[i]), nullptr);
      EXPECT_EQ(*subject.find(words[i]), *reference_map.find(words[i]));
    }

  EXPECT_EQ(subject.size(), reference_set.size());
  EXPECT_EQ(to_sorted_vector(subject.words()), to_sorted_vector(reference_set.words()));
  EXPECT_EQ(to_sorted_vector(subject.words()), to_sorted_vector(reference_map.keys_with_prefix("")));

  for (const auto &prefix : {"", "app", "ban", "car", "z"})
    {
      EXPECT_EQ(to_sorted_vector(subject.words_with_prefix(prefix)),
                to_sorted_vector(reference_set.words_with_prefix(prefix)))
          << "Prefix_Tree disagreement for prefix " << prefix;
      EXPECT_EQ(to_sorted_vector(subject.words_with_prefix(prefix)),
                to_sorted_vector(reference_map.keys_with_prefix(prefix)))
          << "RadixTree disagreement for prefix " << prefix;
    }
}

//==============================================================================
// Main
//==============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
