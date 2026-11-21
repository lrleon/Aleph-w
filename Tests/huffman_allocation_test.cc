
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
 * @file huffman_allocation_test.cc
 * @brief Symbols that enter a Huffman encoder when memory runs out.
 *
 * This program replaces the global operator new, so that a test can make
 * one allocation fail. set_freq() and set_end_of_stream(), as read_input()
 * for each new symbol, make the leaf of the symbol and its node, and enter
 * the node in the map of the symbols and in the heap. The node entered the
 * heap first: without memory for the map, the leaf was freed while the
 * heap kept the node, so the tree had a freed leaf, and the destructor
 * freed it again.
 */

#include <gtest/gtest.h>

#include "allocation_failure.H"

#include <algorithm>
#include <iterator>
#include <new>
#include <string>
#include <utility>
#include <vector>

#include <Huffman.H>

using namespace Aleph;

namespace
{
  // The tests need the operator new of allocation_failure.H
  struct HuffmanWithoutMemory : public ::testing::Test
  {
    void SetUp() override
    {
      if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
        GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";
    }
  };

  // The leaves of the tree of root p
  size_t leaves(BinNode<std::string> *p)
  {
    if (p == nullptr)
      return 0;
    if (LLINK(p) == nullptr and RLINK(p) == nullptr)
      return 1;
    return leaves(LLINK(p)) + leaves(RLINK(p));
  }

  // The allocation k of add fails, for each k until add needs no more.
  // Then the encoder takes the symbol with memory, and its tree must have
  // two leaves, x and the symbol: a freed leaf left in the heap is a third
  template <class Add>
  void expect_two_leaves(Add add)
  {
    for (size_t k = 1; ; ++k)
      {
        SCOPED_TRACE(testing::Message() << "allocation " << k);
        Huffman_Encoder_Engine encoder;
        encoder.set_freq("x", 3);

        bool thrown = false;
        {
          Fail_At failing(k);
          try
            {
              add(encoder);
            }
          catch (const std::bad_alloc &)
            {
              thrown = true;
            }
        }

        if (thrown)
          add(encoder);
        BinNode<std::string> *root = encoder.generate_huffman_tree();
        EXPECT_EQ(leaves(root), 2u);
        destroyRec(root);  // the tree is of the caller
        if (not thrown)
          break;
      }
  }

  // Longer than the buffer of a short string: copying it allocates
  const std::string Symbol = "a symbol longer than a short string";
}

TEST_F(HuffmanWithoutMemory, SetFreqLeavesTheEncoderAsItWas)
{
  expect_two_leaves([](Huffman_Encoder_Engine & e) { e.set_freq(Symbol, 5); });
}

// The end symbol was copied after it entered the heap and the map: without
// memory for the copy, the encoder had the symbol, but not as its end, and
// it could not take it again
TEST_F(HuffmanWithoutMemory, SetEndOfStreamLeavesTheEncoderAsItWas)
{
  expect_two_leaves([](Huffman_Encoder_Engine & e) { e.set_end_of_stream(Symbol); });
}

namespace
{
  // Symbols longer than the buffer of a short string, some of them with
  // the same frequency
  const std::pair<std::string, size_t> Symbols[] = {
    {"the first symbol, longer than a short string", 5},
    {"the second symbol, longer than a short string", 2},
    {"the third symbol, longer than a short string", 2},
    {"the fourth symbol, longer than a short string", 9},
    {"the fifth symbol, longer than a short string", 1},
    {"x", 1},
    {"y", 7}
  };

  using One_More = std::pair<std::string, size_t>;

  void fill(Huffman_Encoder_Engine & encoder)
  {
    for (const auto & [symbol, freq] : Symbols)
      encoder.set_freq(symbol, freq);
  }

  // The bits that the tree of root p takes for the frequencies: the sum of
  // the frequency of each symbol times the depth of its leaf. `seen` takes
  // the symbol of each leaf
  size_t cost(BinNode<std::string> *p, const size_t depth, std::vector<std::string> & seen,
              const One_More & one_more = {})
  {
    if (LLINK(p) == nullptr and RLINK(p) == nullptr)
      {
        seen.push_back(p->get_key());
        if (one_more.second != 0 and one_more.first == p->get_key())
          return one_more.second * depth;
        for (const auto & [symbol, freq] : Symbols)
          if (symbol == p->get_key())
            return freq * depth;
        return 0; // not a symbol: the_symbols_once() says so
      }
    return cost(LLINK(p), depth + 1, seen, one_more) + cost(RLINK(p), depth + 1, seen, one_more);
  }

  // True if the leaves have each symbol once, and nothing else: a tree
  // with a symbol twice and without another one of the same frequency
  // would cost the same
  testing::AssertionResult the_symbols_once(std::vector<std::string> seen, const One_More & one_more = {})
  {
    std::vector<std::string> expected;
    for (const auto & [symbol, freq] : Symbols)
      expected.push_back(symbol);
    if (one_more.second != 0)
      expected.push_back(one_more.first);
    std::sort(expected.begin(), expected.end());
    std::sort(seen.begin(), seen.end());
    if (seen == expected)
      return testing::AssertionSuccess();
    auto failure = testing::AssertionFailure() << seen.size() << " leaves for " << expected.size()
                                               << " symbols:";
    for (const auto & symbol : seen)
      failure << " [" << symbol << "]";
    return failure;
  }

  // In a tree of frequencies, each node that is not a leaf has the sum of
  // those of its two children. It gives the frequency of p
  size_t frequency(Freq_Node *p)
  {
    if (LLINK(p) == nullptr and RLINK(p) == nullptr)
      return p->get_key().second;
    const size_t sum = frequency(LLINK(p)) + frequency(RLINK(p));
    EXPECT_EQ(p->get_key().second, sum) << "at the node " << p->get_key().first;
    return sum;
  }
}

// generate_huffman_tree() took two nodes out of the heap before it made
// the one that joins them, and built the map of the codes with the tree
// already out of the heap. Without memory for any of that, the nodes and
// the trees that were out were lost, and the map of the symbols kept nodes
// that the generation had freed. It now undoes what it did: the encoder
// has its symbols and their frequencies, and generates the tree again.
TEST_F(HuffmanWithoutMemory, GenerateLeavesTheEncoderAsItWas)
{
  size_t optimal = 0;
  size_t total = 0;
  for (const auto & [symbol, freq] : Symbols)
    total += freq;
  {
    Huffman_Encoder_Engine encoder;
    fill(encoder);
    encoder.generate_huffman_tree();
    std::vector<std::string> seen;
    optimal = cost(encoder.get_root(), 0, seen);
    EXPECT_TRUE(the_symbols_once(seen));
    destroyRec(encoder.get_root()); // the encoder does not free its tree
  }

  for (const bool with_freqs : {false, true})
    {
      size_t failed = 0;
      for (size_t k = 1; ; ++k)
        {
          SCOPED_TRACE(testing::Message() << "allocation " << k << ", with_freqs " << with_freqs);
          Huffman_Encoder_Engine encoder;
          fill(encoder);

          bool thrown = false;
          {
            Fail_At failing(k);
            try
              {
                encoder.generate_huffman_tree(with_freqs);
              }
            catch (const std::bad_alloc &)
              {
                thrown = true;
              }
          }

          if (thrown)
            {
              ++failed;
              // its symbols are there, and their nodes alive
              for (const auto & [symbol, freq] : Symbols)
                EXPECT_THROW(encoder.set_freq(symbol, freq), std::domain_error);
              EXPECT_THROW(encoder.get_root(), std::domain_error);
              encoder.generate_huffman_tree(with_freqs);
            }
          std::vector<std::string> seen;
          EXPECT_EQ(cost(encoder.get_root(), 0, seen), optimal);
          EXPECT_TRUE(the_symbols_once(seen));
          destroyRec(encoder.get_root());
          if (with_freqs)
            {
              EXPECT_EQ(frequency(encoder.get_freq_root()), total);
              destroyRec(encoder.get_freq_root());
            }
          if (not thrown)
            break;
        }
      EXPECT_GT(failed, 2 * std::size(Symbols)); // the nodes of each step, and the codes
    }
}

// After a generation that failed, the encoder takes one symbol more, of the
// least frequency, and its tree is the best one for them all: nothing of
// what the generation joined stays joined
TEST_F(HuffmanWithoutMemory, AFailedGenerationTakesMoreSymbols)
{
  const One_More one_more = {"one more symbol, longer than a short string", 1};

  size_t optimal = 0;
  {
    Huffman_Encoder_Engine encoder;
    fill(encoder);
    encoder.set_freq(one_more.first, one_more.second);
    encoder.generate_huffman_tree();
    std::vector<std::string> seen;
    optimal = cost(encoder.get_root(), 0, seen, one_more);
    EXPECT_TRUE(the_symbols_once(seen, one_more));
    destroyRec(encoder.get_root());
  }

  size_t failed = 0;
  for (size_t k = 1; ; ++k)
    {
      SCOPED_TRACE(testing::Message() << "allocation " << k);
      Huffman_Encoder_Engine encoder;
      fill(encoder);
      bool thrown = false;
      {
        Fail_At failing(k);
        try
          {
            encoder.generate_huffman_tree();
          }
        catch (const std::bad_alloc &)
          {
            thrown = true;
          }
      }
      if (not thrown)
        {
          destroyRec(encoder.get_root());
          break;
        }
      ++failed;
      encoder.set_freq(one_more.first, one_more.second);
      encoder.generate_huffman_tree();
      std::vector<std::string> seen;
      EXPECT_EQ(cost(encoder.get_root(), 0, seen, one_more), optimal);
      EXPECT_TRUE(the_symbols_once(seen, one_more));
      destroyRec(encoder.get_root());
    }
  EXPECT_GT(failed, 2 * std::size(Symbols));
}

// The tree of one symbol is its leaf: the node of the symbol is the last
// one of the heap, and it is freed once. Its tree of frequencies is a leaf
// too, which the encoder did not make: get_freq_root() threw
TEST_F(HuffmanWithoutMemory, OneSymbolGeneratesItsLeaf)
{
  for (const bool with_freqs : {false, true})
    {
      size_t failed = 0;
      for (size_t k = 1; ; ++k)
        {
          SCOPED_TRACE(testing::Message() << "allocation " << k << ", with_freqs " << with_freqs);
          Huffman_Encoder_Engine encoder;
          encoder.set_freq(Symbol, 3);
          bool thrown = false;
          {
            Fail_At failing(k);
            try
              {
                encoder.generate_huffman_tree(with_freqs);
              }
            catch (const std::bad_alloc &)
              {
                thrown = true;
              }
          }
          if (thrown)
            {
              ++failed;
              encoder.generate_huffman_tree(with_freqs);
            }
          BinNode<std::string> *root = encoder.get_root();
          EXPECT_EQ(leaves(root), 1u);
          EXPECT_EQ(root->get_key(), Symbol);
          destroyRec(encoder.get_root());
          if (with_freqs)
            {
              Freq_Node *freq_root = encoder.get_freq_root();
              EXPECT_EQ(LLINK(freq_root), nullptr);
              EXPECT_EQ(RLINK(freq_root), nullptr);
              EXPECT_EQ(freq_root->get_key().first, Symbol);
              EXPECT_EQ(freq_root->get_key().second, 3u);
              destroyRec(encoder.get_freq_root());
            }
          else
            EXPECT_THROW(encoder.get_freq_root(), std::domain_error);
          if (not thrown)
            break;
        }
      EXPECT_GT(failed, 0u);
    }
}

// A generation that fails leaves no node of frequencies in the symbols:
// one would keep the frequency of that moment, and read_input() counts on
char abc_input[] = "abracadabra, a cab";

TEST_F(HuffmanWithoutMemory, AFailedGenerationKeepsNoFrequencyOfItsOwn)
{
  const size_t length = std::string(abc_input).size();
  size_t failed = 0;
  size_t counted_twice = 0;
  for (size_t k = 1; ; ++k)
    {
      SCOPED_TRACE(testing::Message() << "allocation " << k);
      Huffman_Encoder_Engine encoder;
      bool thrown = false;
      {
        Fail_At failing(k);
        try
          {
            encoder.read_input(abc_input, true);
          }
        catch (const std::bad_alloc &)
          {
            thrown = true;
          }
      }
      // read_input() sets the end symbol once it has counted its input: a
      // failure after that is of the generation, and the retry counts the
      // whole input again
      const bool counted = thrown and encoder.get_end_of_stream() != "NO-END";
      if (thrown)
        {
          ++failed;
          encoder.read_input(abc_input, true); // it counts on what it had
        }
      const size_t total = frequency(encoder.get_freq_root());
      if (not thrown)
        EXPECT_EQ(total, length);
      else if (counted)
        {
          ++counted_twice;
          EXPECT_EQ(total, 2 * length);
        }
      else // it failed while it counted, or when the end symbol entered
        {
          EXPECT_GE(total, length);
          EXPECT_LE(total, 2 * length);
        }
      destroyRec(encoder.get_root());
      destroyRec(encoder.get_freq_root());
      if (not thrown)
        break;
    }
  EXPECT_GT(failed, 10u);
  EXPECT_GT(counted_twice, 10u); // failures of the generation itself
}
