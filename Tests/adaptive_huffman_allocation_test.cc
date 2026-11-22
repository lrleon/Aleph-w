
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
 * @file adaptive_huffman_allocation_test.cc
 * @brief The adaptive Huffman decoder when its output cannot grow.
 */

#include <gtest/gtest.h>

#include "allocation_failure.H"

#include <cstddef>
#include <new>
#include <random>
#include <stdexcept>
#include <string>

#include <Adaptive_Huffman.H>

using namespace Aleph;

namespace
{
  // The tests need the operator new of allocation_failure.H
  struct AdaptiveHuffmanWithoutMemory : public ::testing::Test
  {
    void SetUp() override
    {
      if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
        GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";
    }
  };

  // A message of n bytes: a few frequent ones and the rest at random, so
  // that the codewords have many lengths
  std::string message(const size_t n)
  {
    std::mt19937 rng(213);
    std::string text;
    for (size_t i = 0; i < n; ++i)
      text.push_back(static_cast<char>(rng() % 5 != 0 ? 'a' + rng() % 6 : rng() % 256));
    return text;
  }
} // namespace

// decode() counts a byte in its model before it appends it to the output.
// When the output could not grow, the decoder was left one byte ahead of
// it, and nothing said so: finish() accepted the state, going on gave the
// message with that byte missing, and decoding the same chunk again
// appended bytes that are not in the message. Now the decoder is left
// failed, as after malformed input: every later call says so until reset().
TEST_F(AdaptiveHuffmanWithoutMemory, ALostByteLeavesTheDecoderFailed)
{
  const std::string text = message(3000);
  Adaptive_Huffman_Encoder encoder;
  BitArray bits;
  encoder.encode(text, bits);

  const std::string prefix = "kept:";
  size_t failures = 0;
  for (size_t k = 1; ; ++k) // the growth of the output that fails
    {
      SCOPED_TRACE(testing::Message() << "growth " << k);
      Adaptive_Huffman_Decoder decoder;
      std::string output = prefix;
      bool thrown = false;
      {
        Fail_At fail(k);
        try
          {
            decoder.decode(bits, output);
          }
        catch (const std::bad_alloc &)
          {
            thrown = true;
          }
      }
      if (not thrown)
        {
          EXPECT_EQ(output, prefix + text);
          EXPECT_NO_THROW(decoder.finish());
          break;
        }
      ++failures;

      // The bytes before the lost one are in the output, and the decoder
      // counted one more
      const size_t kept = output.size() - prefix.size();
      ASSERT_LT(kept, text.size());
      EXPECT_EQ(output, prefix + text.substr(0, kept));
      EXPECT_EQ(decoder.symbols_processed(), kept + 1);

      EXPECT_THROW(decoder.finish(), std::domain_error);
      unsigned char byte = 99;
      EXPECT_THROW(decoder.decode_bit(0, byte), std::domain_error);
      EXPECT_THROW(decoder.decode_bit(1, byte), std::domain_error);
      EXPECT_EQ(byte, 99);
      EXPECT_THROW(decoder.decode(bits, output), std::domain_error); // the same chunk again
      EXPECT_EQ(output.size(), prefix.size() + kept);
      EXPECT_EQ(decoder.symbols_processed(), kept + 1);

      // After reset() the decoder starts the message again
      decoder.reset();
      output = prefix;
      EXPECT_EQ(decoder.decode(bits, output), text.size());
      EXPECT_EQ(output, prefix + text);
      EXPECT_NO_THROW(decoder.finish());
    }
  EXPECT_GE(failures, 5u); // the output grew that many times
}

// decode_bit() allocates nothing, so a decoder that the caller feeds bit by
// bit, appending to an output that it grows itself, can go on after its
// output fails: the byte is in the caller's hands when it is decoded
TEST_F(AdaptiveHuffmanWithoutMemory, DecodeBitLetsTheCallerGoOn)
{
  const std::string text = message(500);
  Adaptive_Huffman_Encoder encoder;
  BitArray bits;
  encoder.encode(text, bits);

  Adaptive_Huffman_Decoder decoder;
  std::string output;
  size_t refused = 0;
  for (size_t i = 0; i < bits.size(); ++i)
    {
      unsigned char byte;
      {
        No_Memory none; // decode_bit() takes no memory
        if (not decoder.decode_bit(static_cast<unsigned int>(bits[i]), byte))
          continue;
      }
      // Each growth of the output fails once, and the caller tries again
      for (bool refused_once = false; ;)
        try
          {
            const bool grows = output.size() == output.capacity();
            Fail_At fail(grows and not refused_once ? 1 : 0);
            output.push_back(static_cast<char>(byte));
            break;
          }
        catch (const std::bad_alloc &)
          {
            refused_once = true;
            ++refused;
          }
    }
  EXPECT_EQ(output, text);
  EXPECT_NO_THROW(decoder.finish());
  EXPECT_GE(refused, 4u); // the output grew that many times
}
