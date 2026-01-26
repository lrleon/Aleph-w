
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
 * @file huffman.cc
 * @brief Tests for Huffman
 */

#include <gtest/gtest.h>

#include <Huffman.H>

#include <tpl_binNodeUtils.H>

#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace Aleph;
using namespace std;

namespace {

vector<char> to_c_buffer(const string &s)
{
  vector<char> buf;
  buf.reserve(s.size() + 1);
  buf.insert(buf.end(), s.begin(), s.end());
  buf.push_back('\0');
  return buf;
}

string decode_bits(BinNode<string> *root, const string &end_symbol, BitArray &bits)
{
  const Huffman_Decoder_Engine decoder(root, end_symbol);
  ostringstream out;
  decoder.decode(bits, out);
  return out.str();
}

TEST(HuffmanCharAPI, roundtrip_with_whitespace_and_freq_tree)
{
  const string input = "hello world\nthis\tis a test\n";
  auto buf = to_c_buffer(input);

  Huffman_Encoder_Engine encoder;
  BitArray bits;

  encoder.read_input(buf.data(), true);
  encoder.encode(buf.data(), bits);

  EXPECT_EQ(encoder.get_end_of_stream(), string(""));
  EXPECT_GT(bits.size(), 0u);

  EXPECT_EQ(decode_bits(encoder.get_root(), encoder.get_end_of_stream(), bits), input);

  destroyRec(encoder.get_root());
  destroyRec(encoder.get_freq_root());
}

TEST(HuffmanStreamAPI, no_eof_off_by_one_on_read_and_encode)
{
  const string input = "A";
  istringstream in1(input);

  Huffman_Encoder_Engine encoder;
  encoder.read_input(in1, false);

  istringstream in2(input);
  BitArray bits;
  encoder.encode(in2, bits);

  EXPECT_EQ(decode_bits(encoder.get_root(), encoder.get_end_of_stream(), bits), input);

  destroyRec(encoder.get_root());
}

TEST(HuffmanPersistence, save_load_tree_and_encode_roundtrip)
{
  const string input = "a b\nc\tdd\n";
  auto buf = to_c_buffer(input);

  Huffman_Encoder_Engine encoder;
  encoder.read_input(buf.data(), false);

  BitArray bits1;
  encoder.encode(buf.data(), bits1);

  ostringstream out;
  encoder.save_tree(out);

  Huffman_Encoder_Engine loaded;
  istringstream in(out.str());
  loaded.load_tree(in);

  EXPECT_TRUE(areEquivalents(encoder.get_root(), loaded.get_root()));
  EXPECT_EQ(loaded.get_end_of_stream(), encoder.get_end_of_stream());

  BitArray bits2;
  loaded.encode(buf.data(), bits2);
  EXPECT_TRUE(bits1 == bits2);
  EXPECT_EQ(decode_bits(loaded.get_root(), loaded.get_end_of_stream(), bits2), input);

  destroyRec(encoder.get_root());
  destroyRec(loaded.get_root());
}

TEST(HuffmanDecoder, stops_at_end_symbol_and_ignores_trailing_bits)
{
  const string input = "aba";
  auto buf = to_c_buffer(input);

  Huffman_Encoder_Engine encoder;
  encoder.read_input(buf.data(), false);

  BitArray bits;
  encoder.encode(buf.data(), bits);

  BitArray extended = bits;
  extended.push(1);
  extended.push(0);
  extended.push(1);

  EXPECT_EQ(decode_bits(encoder.get_root(), encoder.get_end_of_stream(), extended), input);

  destroyRec(encoder.get_root());
}

TEST(HuffmanRandomized, matches_reference_roundtrip)
{
  mt19937 rng(123456u);
  uniform_int_distribution<int> len_dist(0, 200);
  const string alphabet = "abcde fghij\n\t";
  uniform_int_distribution<size_t> pick(0, alphabet.size() - 1);

  for (int iter = 0; iter < 50; ++iter)
    {
      const int len = len_dist(rng);
      string input;
      input.reserve(static_cast<size_t>(len));
      for (int i = 0; i < len; ++i)
        input.push_back(alphabet[pick(rng)]);

      auto buf = to_c_buffer(input);

      Huffman_Encoder_Engine encoder;
      encoder.read_input(buf.data(), false);

      BitArray bits;
      encoder.encode(buf.data(), bits);

      EXPECT_EQ(decode_bits(encoder.get_root(), encoder.get_end_of_stream(), bits), input);

      destroyRec(encoder.get_root());
    }
}

TEST(HuffmanErrors, encode_without_tree_throws)
{
  Huffman_Encoder_Engine encoder;
  BitArray bits;
  char input[] = "x";
  EXPECT_THROW(encoder.encode(input, bits), std::domain_error);
}

TEST(HuffmanErrors, set_end_of_stream_twice_throws)
{
  Huffman_Encoder_Engine encoder;
  encoder.set_end_of_stream("END");
  EXPECT_THROW(encoder.set_end_of_stream("OTHER"), std::domain_error);
}

TEST(HuffmanErrors, generate_without_symbols_throws)
{
  Huffman_Encoder_Engine encoder;
  EXPECT_THROW((void) encoder.generate_huffman_tree(false), std::domain_error);
}

} // namespace

