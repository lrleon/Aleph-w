
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

#include <new>
#include <string>

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
