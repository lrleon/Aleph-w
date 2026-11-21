/*
  Aleph-w: Data structures & Algorithms
  Copyright (c) 2026 Leandro Rabindranath Leon

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

/** @file adaptive_huffman_test.cc
 *  @brief Adaptive Huffman wire-format, streaming and tree-invariant tests.
 */

#include <Adaptive_Huffman.H>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <queue>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace Aleph;

namespace
{
  using Model = adaptive_huffman_detail::Model;
  using Counts = std::array<std::uint64_t, 256>;

  /** @brief Copy a half-open bit interval into an independent chunk.
   *  @param bits Source bit sequence.
   *  @param begin First included bit.
   *  @param end First excluded bit.
   *  @return A chunk containing exactly the selected bits.
   */
  BitArray slice(const BitArray &bits, const size_t begin, const size_t end)
  {
    BitArray chunk;
    for (size_t i = begin; i < end; ++i)
      chunk.push(bits[i]);
    return chunk;
  }

  /** @brief Build a bit array from a literal 0/1 sequence.
   *  @param text Literal bits.
   *  @return The corresponding bit array.
   */
  BitArray bit_literal(const std::string &text)
  {
    BitArray result;
    result.set_bit_str(text);
    return result;
  }

  /** @brief Check exact counts, topology and Gallager's sibling property.
   *
   *  Every check is fatal: the callers stop at the first one that fails, with
   *  ASSERT_NO_FATAL_FAILURE, so that a broken tree is reported once and is
   *  not walked further (a cycle of parents would never end).
   *  @param model Adaptive tree after a complete update.
   *  @param counts Independently accumulated byte frequencies.
   */
  void check_model(const Model &model, const Counts &counts)
  {
    std::uint64_t total = 0;
    size_t distinct = 0;
    for (size_t symbol = 0; symbol < counts.size(); ++symbol)
      {
        total += counts[symbol];
        if (counts[symbol] == 0)
          ASSERT_EQ(model.leaves[symbol], -1);
        else
          {
            ++distinct;
            const int leaf = model.leaves[symbol];
            ASSERT_GE(leaf, 0);
            ASSERT_LT(leaf, model.used);
            ASSERT_TRUE(model.nodes[leaf].is_leaf());
            ASSERT_EQ(model.nodes[leaf].symbol, symbol);
            ASSERT_EQ(model.nodes[leaf].weight, counts[symbol]);
          }
      }
    ASSERT_EQ(model.distinct, distinct);
    ASSERT_EQ(model.used, 2 * distinct + 1);
    ASSERT_EQ(model.nodes[Model::root].parent, -1);
    ASSERT_EQ(model.nodes[Model::root].weight, total);
    ASSERT_EQ(model.nodes[Model::root].order, Model::max_nodes - 1);
    ASSERT_GE(model.nyt, 0);
    ASSERT_LT(model.nyt, model.used);
    ASSERT_TRUE(model.nodes[model.nyt].is_leaf());
    ASSERT_EQ(model.nodes[model.nyt].weight, 0u);

    const int first = Model::max_nodes - model.used;
    for (int order = first; order < Model::max_nodes; ++order)
      {
        const int id = model.by_order[order];
        ASSERT_GE(id, 0);
        ASSERT_LT(id, model.used);
        const auto &node = model.nodes[id];
        ASSERT_EQ(node.order, order);
        if (order != first)
          ASSERT_LE(model.nodes[model.by_order[order - 1]].weight, node.weight);
        if (id != Model::root)
          {
            ASSERT_GE(node.parent, 0);
            ASSERT_LT(node.parent, model.used);
            const auto &parent = model.nodes[node.parent];
            ASSERT_GT(parent.order, order); // Also excludes cycles.
            ASSERT_TRUE(parent.left == id or parent.right == id);
          }
        if (not node.is_leaf())
          {
            ASSERT_GE(node.left, 0);
            ASSERT_LT(node.left, model.used);
            ASSERT_GE(node.right, 0);
            ASSERT_LT(node.right, model.used);
            const auto &left = model.nodes[node.left];
            const auto &right = model.nodes[node.right];
            ASSERT_EQ(left.parent, id);
            ASSERT_EQ(right.parent, id);
            ASSERT_EQ(left.order + 1, right.order);
            ASSERT_EQ(left.order % 2, 0);
            ASSERT_EQ(node.weight, left.weight + right.weight);
          }
      }
    for (int order = first; order + 1 < Model::max_nodes; order += 2)
      ASSERT_EQ(model.nodes[model.by_order[order]].parent,
                model.nodes[model.by_order[order + 1]].parent);
  }

  /** @brief Compute a static Huffman cost, independently of the FGK updates.
   *  @param counts Byte frequencies; the zero-weight NYT is included separately.
   *  @return Optimal weighted external path length for these leaves.
   */
  std::uint64_t optimal_cost(const Counts &counts)
  {
    std::priority_queue<std::uint64_t, std::vector<std::uint64_t>,
                        std::greater<std::uint64_t>> heap;
    heap.push(0); // The retained NYT leaf is part of the coding alphabet.
    for (const auto count : counts)
      if (count != 0)
        heap.push(count);
    std::uint64_t cost = 0;
    while (heap.size() > 1)
      {
        const auto a = heap.top();
        heap.pop();
        const auto b = heap.top();
        heap.pop();
        cost += a + b;
        heap.push(a + b);
      }
    return cost;
  }

  /** @brief Compute the adaptive tree's weighted external path length.
   *  @param model Tree whose cost is measured.
   *  @return Sum of each byte frequency times its depth.
   */
  std::uint64_t tree_cost(const Model &model)
  {
    std::uint64_t cost = 0;
    for (const int leaf : model.leaves)
      if (leaf != -1)
        for (int node = leaf; model.nodes[node].parent != -1;
             node = model.nodes[node].parent)
          cost += model.nodes[leaf].weight;
    return cost;
  }

  /** @brief Verify a complete binary-string round trip and its frequencies.
   *  @param input Bytes to encode and decode.
   */
  void check_round_trip(const std::string_view input)
  {
    Adaptive_Huffman_Encoder encoder;
    Adaptive_Huffman_Decoder decoder;
    BitArray bits;
    const auto count = encoder.encode(input, bits);
    EXPECT_EQ(count, bits.size());
    std::string output;
    EXPECT_EQ(decoder.decode(bits, output), input.size());
    EXPECT_NO_THROW(decoder.finish());
    EXPECT_EQ(output, input);
    EXPECT_EQ(encoder.symbols_processed(), input.size());
    EXPECT_EQ(decoder.symbols_processed(), input.size());
    Counts counts{};
    for (const unsigned char byte : input)
      ++counts[byte];
    for (size_t symbol = 0; symbol < counts.size(); ++symbol)
      {
        EXPECT_EQ(encoder.frequency(static_cast<unsigned char>(symbol)), counts[symbol]);
        EXPECT_EQ(decoder.frequency(static_cast<unsigned char>(symbol)), counts[symbol]);
      }
  }

  TEST(AdaptiveHuffman, EmptyInputAndExistingOutputPrefixes)
  {
    Adaptive_Huffman_Encoder encoder;
    Adaptive_Huffman_Decoder decoder;
    BitArray bits = bit_literal("101");
    EXPECT_EQ(encoder.encode(std::string_view{}, bits), 0u);
    EXPECT_EQ(bits.get_bit_str(), "101");
    std::string output = "prefix";
    EXPECT_EQ(decoder.decode(BitArray{}, output), 0u);
    EXPECT_EQ(output, "prefix");
    EXPECT_NO_THROW(decoder.finish());
    EXPECT_FALSE(decoder.has_pending_symbol());
    EXPECT_EQ(encoder.symbols_processed(), 0u);
    EXPECT_EQ(decoder.symbols_processed(), 0u);
    EXPECT_EQ(encoder.distinct_symbols(), 0u);
    EXPECT_EQ(decoder.distinct_symbols(), 0u);
    EXPECT_EQ(encoder.frequency('A'), 0u);
    EXPECT_EQ(decoder.frequency('A'), 0u);

    EXPECT_EQ(encoder.encode('A', bits), 8u);
    EXPECT_EQ(bits.get_bit_str(), "10101000001");
    EXPECT_EQ(decoder.decode(slice(bits, 3, bits.size()), output), 1u);
    EXPECT_EQ(output, "prefixA");
    EXPECT_NO_THROW(decoder.finish());
  }

  TEST(AdaptiveHuffman, GoldenBitSequences)
  {
    // These paths are determined manually from the NYT splits and FGK swaps.
    const std::pair<std::string, std::string> cases[] = {
      {"A", "01000001"},
      {"AAAA", "01000001" "1" "1" "1"},
      {"AB", "01000001" "0" "01000010"},
      {"ABBA", "01000001" "0" "01000010" "01" "01"},
      {"ABC", "01000001" "0" "01000010" "00" "01000011"},
      {std::string("\0\xff\0", 3), "00000000" "0" "11111111" "1"}
    };
    for (const auto &[input, expected] : cases)
      {
        SCOPED_TRACE(expected);
        Adaptive_Huffman_Encoder encoder;
        BitArray bits;
        encoder.encode(input, bits);
        EXPECT_EQ(bits.get_bit_str(), expected);

        // Decode the independent expected bits, not the encoder's output.
        Adaptive_Huffman_Decoder decoder;
        std::string output;
        decoder.decode(bit_literal(expected), output);
        EXPECT_NO_THROW(decoder.finish());
        EXPECT_EQ(output, input);
      }
  }

  TEST(AdaptiveHuffman, RepeatedSymbolNeedsOneBitAfterItsFirstOccurrence)
  {
    Adaptive_Huffman_Encoder encoder;
    BitArray bits;
    const std::string input(4096, '\0');
    encoder.encode(input, bits);
    EXPECT_EQ(bits.size(), 8 + input.size() - 1);
    EXPECT_EQ(encoder.distinct_symbols(), 1u);
    check_round_trip(input);
  }

  TEST(AdaptiveHuffman, AllByteValuesAndChangingDistributions)
  {
    std::string input;
    for (int round = 0; round < 3; ++round)
      for (int symbol = 0; symbol < 256; ++symbol)
        input.push_back(static_cast<char>(round % 2 == 0 ? symbol : 255 - symbol));
    input.append(5000, 'a');
    input.append(7000, static_cast<char>(0xff));
    input.append(9000, '\0');
    check_round_trip(input);
  }

  TEST(AdaptiveHuffman, EmitsEachByteOnItsLastBit)
  {
    Adaptive_Huffman_Encoder encoder;
    Adaptive_Huffman_Decoder decoder;
    const std::string input = "abcdefghiaaf abracadabra\n\t";
    for (const unsigned char byte : input)
      {
        BitArray bits;
        encoder.encode(byte, bits);
        unsigned char output = static_cast<unsigned char>(~byte);
        for (size_t i = 0; i < bits.size(); ++i)
          {
            EXPECT_EQ(decoder.decode_bit(bits[i], output), i + 1 == bits.size());
            EXPECT_EQ(output, i + 1 == bits.size() ? byte : static_cast<unsigned char>(~byte));
          }
        EXPECT_FALSE(decoder.has_pending_symbol());
        EXPECT_EQ(decoder.frequency(byte), encoder.frequency(byte));
        EXPECT_EQ(decoder.symbols_processed(), encoder.symbols_processed());
        EXPECT_NO_THROW(decoder.finish());
      }
  }

  TEST(AdaptiveHuffman, ArbitraryChunksMatchWholeMessage)
  {
    std::mt19937 rng(0xada971u);
    const unsigned int alphabets[] = {1, 2, 3, 16, 128, 256};
    for (const auto alphabet : alphabets)
      for (int round = 0; round < 8; ++round)
        {
          SCOPED_TRACE(alphabet);
          SCOPED_TRACE(round);
          const size_t length = rng() % 1025;
          std::string input;
          for (size_t i = 0; i < length; ++i)
            input.push_back(static_cast<char>(rng() % alphabet));

          Adaptive_Huffman_Encoder whole_encoder, chunk_encoder;
          BitArray whole_bits, chunk_bits;
          whole_encoder.encode(input, whole_bits);
          for (size_t i = 0; i < input.size();)
            {
              const size_t count = std::min<size_t>(1 + rng() % 23, input.size() - i);
              chunk_encoder.encode(std::string_view(input).substr(i, count), chunk_bits);
              i += count;
            }
          ASSERT_TRUE(chunk_bits == whole_bits);

          Adaptive_Huffman_Decoder decoder;
          std::string output;
          for (size_t i = 0; i < chunk_bits.size();)
            {
              const size_t end = std::min<size_t>(i + 1 + rng() % 37, chunk_bits.size());
              decoder.decode(slice(chunk_bits, i, end), output);
              i = end;
            }
          EXPECT_NO_THROW(decoder.finish());
          EXPECT_EQ(output, input);
        }
  }

  TEST(AdaptiveHuffman, EveryPartialCodewordCanBeResumed)
  {
    Adaptive_Huffman_Encoder encoder;
    Adaptive_Huffman_Decoder decoder;
    const std::string input = "abcdefghiaaf abracadabra";
    for (const unsigned char byte : input)
      {
        BitArray bits;
        encoder.encode(byte, bits);
        for (size_t cut = 1; cut < bits.size(); ++cut)
          {
            auto partial = decoder;
            std::string output;
            EXPECT_EQ(partial.decode(slice(bits, 0, cut), output), 0u);
            EXPECT_TRUE(partial.has_pending_symbol());
            EXPECT_THROW(partial.finish(), std::invalid_argument);
            EXPECT_EQ(partial.decode(slice(bits, cut, bits.size()), output), 1u);
            EXPECT_NO_THROW(partial.finish());
            EXPECT_EQ(output, std::string(1, static_cast<char>(byte)));
          }
        std::string output;
        decoder.decode(bits, output);
      }
  }

  TEST(AdaptiveHuffman, RejectsNonBinaryBitsWithoutChangingState)
  {
    Adaptive_Huffman_Decoder decoder;
    unsigned char output = 99;
    for (const unsigned int bit : {0u, 1u, 0u})
      EXPECT_FALSE(decoder.decode_bit(bit, output));
    EXPECT_THROW(decoder.decode_bit(2, output), std::out_of_range);
    EXPECT_EQ(output, 99);
    EXPECT_TRUE(decoder.has_pending_symbol());
    std::string text;
    decoder.decode(bit_literal("00001"), text);
    EXPECT_NO_THROW(decoder.finish());
    EXPECT_EQ(text, "A");
  }

  TEST(AdaptiveHuffman, RejectsKnownByteIntroducedThroughNYT)
  {
    Adaptive_Huffman_Decoder decoder;
    std::string output;
    decoder.decode(bit_literal("01000001"), output);
    EXPECT_THROW(decoder.decode(bit_literal("0" "01000001"), output), std::invalid_argument);
    EXPECT_EQ(output, "A");
    EXPECT_EQ(decoder.symbols_processed(), 1u);
    EXPECT_EQ(decoder.distinct_symbols(), 1u);
    EXPECT_EQ(decoder.frequency('A'), 1u);
    unsigned char byte = 99;
    EXPECT_THROW(decoder.decode_bit(1, byte), std::domain_error);
    EXPECT_THROW(decoder.finish(), std::domain_error);
    EXPECT_EQ(byte, 99);

    decoder.reset();
    output.clear();
    decoder.decode(bit_literal("01000001"), output);
    EXPECT_EQ(output, "A");
    EXPECT_NO_THROW(decoder.finish());
  }

  TEST(AdaptiveHuffman, RejectsNYTAfterTheWholeAlphabetWasSeen)
  {
    Adaptive_Huffman_Encoder encoder;
    Adaptive_Huffman_Decoder decoder;
    Model model;
    BitArray bits;
    for (int symbol = 0; symbol < 256; ++symbol)
      {
        encoder.encode(static_cast<unsigned char>(symbol), bits);
        model.update(static_cast<unsigned char>(symbol));
      }
    std::string output;
    decoder.decode(bits, output);
    ASSERT_EQ(decoder.distinct_symbols(), 256u);
    std::string path;
    for (int node = model.nyt; model.nodes[node].parent != -1;)
      {
        const int parent = model.nodes[node].parent;
        path.push_back(model.nodes[parent].left == node ? '0' : '1');
        node = parent;
      }
    std::reverse(path.begin(), path.end());
    EXPECT_THROW(decoder.decode(bit_literal(path), output), std::invalid_argument);
    EXPECT_EQ(output.size(), 256u);
    EXPECT_EQ(decoder.symbols_processed(), 256u);

    // The decoder asks for a reset, as after the escape of a known byte; a
    // bit that is not binary is refused first, as on any decoder
    unsigned char byte = 99;
    EXPECT_THROW(decoder.decode_bit(0, byte), std::domain_error);
    EXPECT_THROW(decoder.decode_bit(1, byte), std::domain_error);
    EXPECT_THROW(decoder.decode_bit(2, byte), std::out_of_range);
    EXPECT_THROW(decoder.finish(), std::domain_error);
    EXPECT_EQ(byte, 99);
    decoder.reset();
    EXPECT_NO_THROW(decoder.finish());
  }

  // decode() keeps in the output the bytes that it decoded before the bad
  // codeword of the same chunk, and the decoder counts exactly those
  TEST(AdaptiveHuffman, MalformedChunkKeepsTheBytesBeforeTheBadCodeword)
  {
    Adaptive_Huffman_Encoder encoder;
    BitArray chunk;
    encoder.encode("AB", chunk);
    for (const char c : std::string("00" "01000001")) // the NYT path, then A again
      chunk.push(c == '1');
    Adaptive_Huffman_Decoder decoder;
    std::string output = "kept:";
    EXPECT_THROW(decoder.decode(chunk, output), std::invalid_argument);
    EXPECT_EQ(output, "kept:AB");
    EXPECT_EQ(decoder.symbols_processed(), 2u);
    EXPECT_THROW(decoder.finish(), std::domain_error);
    EXPECT_EQ(decoder.decode(BitArray(), output), 0u); // an empty chunk consumes nothing
  }

  // encode(byte) returns the bits that it appended: for a known byte, and for
  // an unseen one behind an NYT path that is not empty
  TEST(AdaptiveHuffman, EncodeByteReturnsTheBitsItAppended)
  {
    Adaptive_Huffman_Encoder encoder;
    BitArray bits = bit_literal("101");
    size_t known = 0;
    size_t unseen_deep = 0;
    for (const unsigned char byte : std::string_view("abracadabra: every byte reports its own bits"))
      {
        const bool first = encoder.frequency(byte) == 0;
        const size_t before = bits.size();
        const size_t returned = encoder.encode(byte, bits);
        EXPECT_EQ(returned, bits.size() - before);
        if (first)
          {
            EXPECT_GE(returned, 8u);
            unseen_deep += returned > 8;
          }
        else
          {
            EXPECT_GE(returned, 1u);
            ++known;
          }
      }
    EXPECT_GT(known, 0u); // both kinds were exercised
    EXPECT_GT(unseen_deep, 0u);
  }

  // With Fibonacci counts the tree is a chain, and its codewords are as long
  // as the number of distinct bytes: 17,710 bytes of 20 values give 20 bits.
  // No other test goes deeper than 11 levels
  TEST(AdaptiveHuffman, FibonacciFrequenciesGiveCodewordsAsDeepAsTheAlphabet)
  {
    constexpr int distinct = 20;
    std::string input;
    std::uint64_t a = 1;
    std::uint64_t b = 1;
    for (int symbol = 0; symbol < distinct; ++symbol)
      {
        input.append(a, static_cast<char>(200 + symbol));
        const auto next = a + b;
        a = b;
        b = next;
      }
    Adaptive_Huffman_Encoder encoder;
    BitArray bits;
    encoder.encode(input, bits);
    size_t longest = 0;
    for (int symbol = 0; symbol < distinct; ++symbol)
      {
        auto probe = encoder; // the code of a known byte, without changing the encoder
        BitArray code;
        longest = std::max(longest, probe.encode(static_cast<unsigned char>(200 + symbol), code));
      }
    EXPECT_EQ(longest, static_cast<size_t>(distinct));
    Adaptive_Huffman_Decoder decoder;
    std::string output;
    EXPECT_EQ(decoder.decode(bits, output), input.size());
    EXPECT_NO_THROW(decoder.finish());
    EXPECT_EQ(output, input);
  }

  TEST(AdaptiveHuffman, ResetRestoresInitialWireFormatAndPendingState)
  {
    Adaptive_Huffman_Encoder encoder;
    Adaptive_Huffman_Decoder decoder;
    BitArray bits;
    encoder.encode("many different bytes", bits);
    std::string output;
    decoder.decode(bits, output);
    BitArray next_bits;
    encoder.encode('\0', next_bits);
    decoder.decode(slice(next_bits, 0, 1), output); // Begin the next codeword.
    ASSERT_TRUE(decoder.has_pending_symbol());
    encoder.reset();
    decoder.reset();
    EXPECT_EQ(encoder.symbols_processed(), 0u);
    EXPECT_EQ(decoder.symbols_processed(), 0u);
    EXPECT_EQ(encoder.distinct_symbols(), 0u);
    EXPECT_EQ(decoder.distinct_symbols(), 0u);
    EXPECT_EQ(encoder.frequency('m'), 0u);
    EXPECT_EQ(decoder.frequency('m'), 0u);
    EXPECT_FALSE(decoder.has_pending_symbol());
    EXPECT_NO_THROW(decoder.finish());
    bits.empty();
    encoder.encode('A', bits);
    EXPECT_EQ(bits.get_bit_str(), "01000001");
    output.clear();
    decoder.decode(bits, output);
    EXPECT_EQ(output, "A");
  }

  TEST(AdaptiveHuffman, CopiesOwnIndependentModelsAndPartialLiterals)
  {
    Adaptive_Huffman_Encoder encoder;
    BitArray prefix;
    encoder.encode("abcde", prefix);
    auto copy = encoder;
    BitArray original_suffix, copied_suffix;
    encoder.encode("aaaa", original_suffix);
    copy.encode("bcde", copied_suffix);
    EXPECT_EQ(encoder.frequency('a'), 5u);
    EXPECT_EQ(copy.frequency('a'), 1u);
    Adaptive_Huffman_Encoder reference;
    BitArray expected;
    reference.encode("abcdebcde", expected);
    EXPECT_TRUE(copied_suffix == slice(expected, prefix.size(), expected.size()));

    Adaptive_Huffman_Decoder decoder;
    std::string ignored;
    decoder.decode(bit_literal("010"), ignored);
    auto decoder_copy = decoder;
    std::string a, b;
    decoder.decode(bit_literal("00001"), a);
    decoder_copy.decode(bit_literal("00010"), b);
    EXPECT_EQ(a, "A");
    EXPECT_EQ(b, "B");
    EXPECT_EQ(decoder.frequency('B'), 0u);
    EXPECT_EQ(decoder_copy.frequency('A'), 0u);
    EXPECT_NO_THROW(decoder.finish());
    EXPECT_NO_THROW(decoder_copy.finish());
  }

  TEST(AdaptiveHuffman, BitArrayPersistencePreservesFraming)
  {
    const std::string input("\0\xff\0 binary\n\xff", 12);
    Adaptive_Huffman_Encoder encoder;
    BitArray bits;
    encoder.encode(input, bits);
    std::stringstream storage;
    bits.save(storage);
    BitArray loaded;
    loaded.load(storage);
    EXPECT_TRUE(loaded == bits);
    Adaptive_Huffman_Decoder decoder;
    std::string output;
    decoder.decode(loaded, output);
    EXPECT_NO_THROW(decoder.finish());
    EXPECT_EQ(output, input);
  }

  TEST(AdaptiveHuffmanTree, ExhaustiveShortTernarySequencesKeepSiblingProperty)
  {
    Counts counts{};
    Model model;
    check_model(model, counts);
    size_t combinations = 1;
    for (size_t length = 1; length <= 7; ++length)
      {
        combinations *= 3;
        for (size_t sequence = 0; sequence < combinations; ++sequence)
          {
            model.reset();
            counts.fill(0);
            size_t digits = sequence;
            for (size_t i = 0; i < length; ++i)
              {
                const auto byte = static_cast<unsigned char>(digits % 3);
                digits /= 3;
                ++counts[byte];
                model.update(byte);
                ASSERT_NO_FATAL_FAILURE(check_model(model, counts));
              }
            ASSERT_EQ(tree_cost(model), optimal_cost(counts));
          }
      }
  }

  TEST(AdaptiveHuffmanTree, RandomUpdatesRemainOptimalForCurrentFrequencies)
  {
    Model model;
    Counts counts{};
    std::mt19937 rng(0xf6c123u);
    // The first part exercises NYT siblings and many equal-weight swaps;
    // later parts force large changes in the distribution and internal swaps.
    for (size_t i = 0; i < 6000; ++i)
      {
        const unsigned int alphabet = i < 2000 ? 256 : (i < 4000 ? 3 : 64);
        const auto byte = static_cast<unsigned char>(rng() % alphabet);
        ++counts[byte];
        model.update(byte);
        ASSERT_NO_FATAL_FAILURE(check_model(model, counts));
        ASSERT_EQ(tree_cost(model), optimal_cost(counts)) << "Update " << i;
      }
  }

  TEST(AdaptiveHuffmanTree, CounterLimitDoesNotWrapAndResetRecovers)
  {
    Model model;
    constexpr unsigned char a = 'A';
    model.update(a);
    constexpr auto limit = std::numeric_limits<std::uint64_t>::max();
    model.nodes[model.leaves[a]].weight = limit - 1;
    model.nodes[Model::root].weight = limit - 1;
    ASSERT_NO_THROW(model.check_capacity());
    model.update(a);
    EXPECT_EQ(model.nodes[Model::root].weight, limit);
    EXPECT_EQ(model.nodes[model.leaves[a]].weight, limit);
    EXPECT_THROW(model.check_capacity(), std::overflow_error);
    model.reset();
    EXPECT_NO_THROW(model.check_capacity());
    EXPECT_EQ(model.nodes[Model::root].weight, 0u);
  }
}
