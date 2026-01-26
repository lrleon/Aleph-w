
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
 * @file huffman_btreepic_test.cc
 * @brief Comprehensive tests for huffman_btreepic.H
 */

#include <gtest/gtest.h>
#include <sstream>
#include <Huffman.H>
#include <huffman_btreepic.H>

using namespace std;

// =============================================================================
// Test Fixtures
// =============================================================================

class HuffmanBtreepicTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    reset_huffman_btreepic_state();
    output_ptr = nullptr;
  }

  void TearDown() override
  {
    reset_huffman_btreepic_state();
    output_ptr = nullptr;
  }

  // Helper to create a simple Huffman tree from a string
  Huffman_Encoder_Engine create_encoder(const string & text)
  {
    Huffman_Encoder_Engine encoder;
    char * str = const_cast<char*>(text.c_str());
    encoder.read_input(str, true);
    return encoder;
  }
};

// =============================================================================
// Infix_Desc Tests
// =============================================================================

TEST_F(HuffmanBtreepicTest, InfixDescDefaultConstruction)
{
  Infix_Desc desc;
  EXPECT_EQ(desc.pos, 0);
  EXPECT_EQ(desc.level, 0);
  EXPECT_EQ(desc.offset, NO_OFFSET);
}

TEST_F(HuffmanBtreepicTest, InfixDescParameterizedConstruction)
{
  Infix_Desc desc(5, 3);
  EXPECT_EQ(desc.pos, 5);
  EXPECT_EQ(desc.level, 3);
  EXPECT_EQ(desc.offset, NO_OFFSET);
}

// =============================================================================
// Level_Desc Tests
// =============================================================================

TEST_F(HuffmanBtreepicTest, LevelDescDefaultConstruction)
{
  Level_Desc desc;
  EXPECT_FALSE(desc.is_left);
  EXPECT_EQ(desc.level_succ, nullptr);
}

TEST_F(HuffmanBtreepicTest, LevelDescParameterizedConstruction)
{
  Freq_Node node;
  Level_Desc desc(true, &node);
  EXPECT_TRUE(desc.is_left);
  EXPECT_EQ(desc.level_succ, &node);
}

// =============================================================================
// Offset Enum Tests
// =============================================================================

TEST_F(HuffmanBtreepicTest, OffsetEnumValues)
{
  EXPECT_EQ(NO_OFFSET, 0);
  EXPECT_EQ(LEFT, 1);
  EXPECT_EQ(RIGHT, 2);
}

// =============================================================================
// get_offset Tests
// =============================================================================

TEST_F(HuffmanBtreepicTest, GetOffsetValidIndices)
{
  EXPECT_DOUBLE_EQ(get_offset(0), 10);
  EXPECT_DOUBLE_EQ(get_offset(1), 15);
  EXPECT_DOUBLE_EQ(get_offset(2), 25);
  EXPECT_DOUBLE_EQ(get_offset(7), 90);
}

TEST_F(HuffmanBtreepicTest, GetOffsetClampedIndices)
{
  // Indices beyond MAX_OFFSET_INDEX should be clamped
  EXPECT_DOUBLE_EQ(get_offset(8), 90);
  EXPECT_DOUBLE_EQ(get_offset(100), 90);
  EXPECT_DOUBLE_EQ(get_offset(1000), 90);
}

// =============================================================================
// num_digits Tests
// =============================================================================

TEST_F(HuffmanBtreepicTest, NumDigitsSingleDigit)
{
  EXPECT_EQ(num_digits(0), 1u);
  EXPECT_EQ(num_digits(5), 1u);
  EXPECT_EQ(num_digits(9), 1u);
}

TEST_F(HuffmanBtreepicTest, NumDigitsMultipleDigits)
{
  EXPECT_EQ(num_digits(10), 2u);
  EXPECT_EQ(num_digits(99), 2u);
  EXPECT_EQ(num_digits(100), 3u);
  EXPECT_EQ(num_digits(12345), 5u);
}

// =============================================================================
// reset_huffman_btreepic_state Tests
// =============================================================================

TEST_F(HuffmanBtreepicTest, ResetStateClearsInfixTable)
{
  // Add something to the infix table
  Freq_Node node;
  node.get_key().first = "a";
  node.get_key().second = 5;
  infix_table.insert(&node, Infix_Desc(1, 2));
  EXPECT_EQ(infix_table.size(), 1u);

  reset_huffman_btreepic_state();
  EXPECT_EQ(infix_table.size(), 0u);
}

TEST_F(HuffmanBtreepicTest, ResetStateClearsLevelTable)
{
  Freq_Node node;
  level_table.insert(&node, Level_Desc(true));
  EXPECT_EQ(level_table.size(), 1u);

  reset_huffman_btreepic_state();
  EXPECT_EQ(level_table.size(), 0u);
}

TEST_F(HuffmanBtreepicTest, ResetStateClearsPred)
{
  Freq_Node node;
  pred = &node;
  EXPECT_NE(pred, nullptr);

  reset_huffman_btreepic_state();
  EXPECT_EQ(pred, nullptr);
}

// =============================================================================
// huffman_to_btreepic Basic Tests
// =============================================================================

TEST_F(HuffmanBtreepicTest, NullptrRootProducesNoOutput)
{
  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(nullptr, false);
  EXPECT_TRUE(ss.str().empty());
}

TEST_F(HuffmanBtreepicTest, NullOutputPtrDoesNotCrash)
{
  Freq_Node node;
  node.get_key().first = "a";
  node.get_key().second = 1;
  LLINK(&node) = nullptr;
  RLINK(&node) = nullptr;

  output_ptr = nullptr;
  // Should not crash
  EXPECT_NO_THROW(huffman_to_btreepic(&node, false));
}

TEST_F(HuffmanBtreepicTest, SingleNodeTree)
{
  Freq_Node node;
  node.get_key().first = "a";
  node.get_key().second = 5;
  LLINK(&node) = nullptr;
  RLINK(&node) = nullptr;

  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(&node, false);

  string output = ss.str();
  EXPECT_TRUE(output.find("start-prefix") != string::npos);
  EXPECT_TRUE(output.find("start-key") != string::npos);
  EXPECT_TRUE(output.find("\"5\"") != string::npos);
  EXPECT_TRUE(output.find("TAG 0") != string::npos);
}

TEST_F(HuffmanBtreepicTest, StreamOverload)
{
  Freq_Node node;
  node.get_key().first = "a";
  node.get_key().second = 5;
  LLINK(&node) = nullptr;
  RLINK(&node) = nullptr;

  stringstream ss;
  huffman_to_btreepic(&node, ss, false);

  string output = ss.str();
  EXPECT_TRUE(output.find("start-prefix") != string::npos);
  EXPECT_TRUE(output.find("start-key") != string::npos);
}

// =============================================================================
// LaTeX Character Escaping Tests
// =============================================================================

TEST_F(HuffmanBtreepicTest, EscapeNewline)
{
  Freq_Node node;
  node.get_key().first = "\n";
  node.get_key().second = 1;
  LLINK(&node) = nullptr;
  RLINK(&node) = nullptr;

  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(&node, false);

  EXPECT_TRUE(ss.str().find("$\\backslash$n") != string::npos);
}

TEST_F(HuffmanBtreepicTest, EscapeDollar)
{
  Freq_Node node;
  node.get_key().first = "$";
  node.get_key().second = 1;
  LLINK(&node) = nullptr;
  RLINK(&node) = nullptr;

  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(&node, false);

  EXPECT_TRUE(ss.str().find("\\$") != string::npos);
}

TEST_F(HuffmanBtreepicTest, EscapeAmpersand)
{
  Freq_Node node;
  node.get_key().first = "&";
  node.get_key().second = 1;
  LLINK(&node) = nullptr;
  RLINK(&node) = nullptr;

  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(&node, false);

  EXPECT_TRUE(ss.str().find("\\&") != string::npos);
}

TEST_F(HuffmanBtreepicTest, EscapeHash)
{
  Freq_Node node;
  node.get_key().first = "#";
  node.get_key().second = 1;
  LLINK(&node) = nullptr;
  RLINK(&node) = nullptr;

  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(&node, false);

  EXPECT_TRUE(ss.str().find("\\#") != string::npos);
}

TEST_F(HuffmanBtreepicTest, EscapePercent)
{
  Freq_Node node;
  node.get_key().first = "%";
  node.get_key().second = 1;
  LLINK(&node) = nullptr;
  RLINK(&node) = nullptr;

  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(&node, false);

  EXPECT_TRUE(ss.str().find("\\%") != string::npos);
}

TEST_F(HuffmanBtreepicTest, EscapeSpace)
{
  Freq_Node node;
  node.get_key().first = " ";
  node.get_key().second = 1;
  LLINK(&node) = nullptr;
  RLINK(&node) = nullptr;

  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(&node, false);

  EXPECT_TRUE(ss.str().find("$\\square$") != string::npos);
}

TEST_F(HuffmanBtreepicTest, EscapeBackslash)
{
  Freq_Node node;
  node.get_key().first = "\\";
  node.get_key().second = 1;
  LLINK(&node) = nullptr;
  RLINK(&node) = nullptr;

  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(&node, false);

  EXPECT_TRUE(ss.str().find("$\\backslash$") != string::npos);
}

TEST_F(HuffmanBtreepicTest, EscapeQuote)
{
  Freq_Node node;
  node.get_key().first = "\"";
  node.get_key().second = 1;
  LLINK(&node) = nullptr;
  RLINK(&node) = nullptr;

  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(&node, false);

  EXPECT_TRUE(ss.str().find("$\\prime\\prime$") != string::npos);
}

TEST_F(HuffmanBtreepicTest, EscapeEmptyString)
{
  Freq_Node node;
  node.get_key().first = "";
  node.get_key().second = 1;
  LLINK(&node) = nullptr;
  RLINK(&node) = nullptr;

  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(&node, false);

  EXPECT_TRUE(ss.str().find("$\\neg$") != string::npos);
}

TEST_F(HuffmanBtreepicTest, EscapeBraces)
{
  Freq_Node node1, node2;
  node1.get_key().first = "{";
  node1.get_key().second = 1;
  node2.get_key().first = "}";
  node2.get_key().second = 1;
  LLINK(&node1) = nullptr;
  RLINK(&node1) = nullptr;
  LLINK(&node2) = nullptr;
  RLINK(&node2) = nullptr;

  stringstream ss1, ss2;
  output_ptr = &ss1;
  huffman_to_btreepic(&node1, false);
  output_ptr = &ss2;
  huffman_to_btreepic(&node2, false);

  EXPECT_TRUE(ss1.str().find("$\\{$") != string::npos);
  EXPECT_TRUE(ss2.str().find("$\\}$") != string::npos);
}

// =============================================================================
// Full Huffman Tree Tests
// =============================================================================

TEST_F(HuffmanBtreepicTest, SimpleTextEncoding)
{
  Huffman_Encoder_Engine encoder = create_encoder("aab");
  Freq_Node * root = encoder.get_freq_root();

  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(root, false);

  string output = ss.str();

  // Should have start-prefix line
  EXPECT_TRUE(output.find("start-prefix") != string::npos);

  // Should have start-key line
  EXPECT_TRUE(output.find("start-key") != string::npos);

  // Should have TAG commands for leaves
  EXPECT_TRUE(output.find("TAG") != string::npos);

  // Clean up
  destroyRec(encoder.get_root());
  destroyRec(encoder.get_freq_root());
}

TEST_F(HuffmanBtreepicTest, WithLevelAdjustment)
{
  Huffman_Encoder_Engine encoder = create_encoder("abcdefgh");
  Freq_Node * root = encoder.get_freq_root();

  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(root, true);

  string output = ss.str();

  // Should have all the standard elements
  EXPECT_TRUE(output.find("start-prefix") != string::npos);
  EXPECT_TRUE(output.find("start-key") != string::npos);

  // Clean up
  destroyRec(encoder.get_root());
  destroyRec(encoder.get_freq_root());
}

TEST_F(HuffmanBtreepicTest, RepeatedCallsResetState)
{
  Huffman_Encoder_Engine encoder1 = create_encoder("aab");
  Huffman_Encoder_Engine encoder2 = create_encoder("xyz");

  stringstream ss1, ss2;

  output_ptr = &ss1;
  huffman_to_btreepic(encoder1.get_freq_root(), false);

  output_ptr = &ss2;
  huffman_to_btreepic(encoder2.get_freq_root(), false);

  // Both outputs should be valid and independent
  EXPECT_TRUE(ss1.str().find("start-prefix") != string::npos);
  EXPECT_TRUE(ss2.str().find("start-prefix") != string::npos);

  // They should be different
  EXPECT_NE(ss1.str(), ss2.str());

  // Clean up
  destroyRec(encoder1.get_root());
  destroyRec(encoder1.get_freq_root());
  destroyRec(encoder2.get_root());
  destroyRec(encoder2.get_freq_root());
}

// =============================================================================
// Output Format Tests
// =============================================================================

TEST_F(HuffmanBtreepicTest, OutputStartsWithStartPrefix)
{
  Huffman_Encoder_Engine encoder = create_encoder("abc");
  Freq_Node * root = encoder.get_freq_root();

  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(root, false);

  string output = ss.str();
  EXPECT_EQ(output.substr(0, 12), "start-prefix");

  destroyRec(encoder.get_root());
  destroyRec(encoder.get_freq_root());
}

TEST_F(HuffmanBtreepicTest, OutputContainsKeyLine)
{
  Huffman_Encoder_Engine encoder = create_encoder("abc");
  Freq_Node * root = encoder.get_freq_root();

  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(root, false);

  string output = ss.str();
  EXPECT_TRUE(output.find("\nstart-key ") != string::npos);

  destroyRec(encoder.get_root());
  destroyRec(encoder.get_freq_root());
}

TEST_F(HuffmanBtreepicTest, TagCommandFormat)
{
  Freq_Node node;
  node.get_key().first = "x";
  node.get_key().second = 7;
  LLINK(&node) = nullptr;
  RLINK(&node) = nullptr;

  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(&node, false);

  string output = ss.str();
  // TAG format: TAG <pos> "<symbol>" S 0 -20
  EXPECT_TRUE(output.find("TAG 0 \"x\" S 0 -20") != string::npos);
}

// =============================================================================
// minimal_gap Tests
// =============================================================================

TEST_F(HuffmanBtreepicTest, MinimalGapDefaultValue)
{
  EXPECT_EQ(minimal_gap, 4);
}

TEST_F(HuffmanBtreepicTest, MinimalGapModification)
{
  int original = minimal_gap;
  minimal_gap = 10;
  EXPECT_EQ(minimal_gap, 10);
  minimal_gap = original;  // Restore
}

// =============================================================================
// Binary Tree Structure Tests
// =============================================================================

TEST_F(HuffmanBtreepicTest, ThreeNodeTree)
{
  /*
   * Create a simple 3-node tree:
   *       root (3)
   *      /    \
   *    left(1) right(2)
   */
  Freq_Node root, left, right;

  root.get_key().first = "";
  root.get_key().second = 3;
  left.get_key().first = "a";
  left.get_key().second = 1;
  right.get_key().first = "b";
  right.get_key().second = 2;

  LLINK(&root) = &left;
  RLINK(&root) = &right;
  LLINK(&left) = nullptr;
  RLINK(&left) = nullptr;
  LLINK(&right) = nullptr;
  RLINK(&right) = nullptr;

  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(&root, false);

  string output = ss.str();

  // Should have TAGs for both leaves
  EXPECT_TRUE(output.find("TAG") != string::npos);
  EXPECT_TRUE(output.find("\"a\"") != string::npos);
  EXPECT_TRUE(output.find("\"b\"") != string::npos);
}

// =============================================================================
// Backward Compatibility Tests
// =============================================================================

TEST_F(HuffmanBtreepicTest, GlobalVariablesExist)
{
  // These should compile and be accessible
  EXPECT_EQ(&infix_table, &infix_table);
  EXPECT_EQ(&level_table, &level_table);
  EXPECT_EQ(&pred, &pred);
  EXPECT_EQ(&minimal_gap, &minimal_gap);
}

TEST_F(HuffmanBtreepicTest, LegacyApiWorks)
{
  Freq_Node node;
  node.get_key().first = "a";
  node.get_key().second = 1;
  LLINK(&node) = nullptr;
  RLINK(&node) = nullptr;

  stringstream ss;
  output_ptr = &ss;  // Legacy: set global output pointer

  // Legacy API call
  huffman_to_btreepic(&node);

  EXPECT_FALSE(ss.str().empty());
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(HuffmanBtreepicTest, VeryLongSymbol)
{
  Freq_Node node;
  node.get_key().first = "verylongsymbolname";
  node.get_key().second = 1;
  LLINK(&node) = nullptr;
  RLINK(&node) = nullptr;

  stringstream ss;
  output_ptr = &ss;

  // Should not crash due to offset array bounds
  EXPECT_NO_THROW(huffman_to_btreepic(&node, true));
}

TEST_F(HuffmanBtreepicTest, ZeroFrequency)
{
  Freq_Node node;
  node.get_key().first = "a";
  node.get_key().second = 0;
  LLINK(&node) = nullptr;
  RLINK(&node) = nullptr;

  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(&node, false);

  EXPECT_TRUE(ss.str().find("\"0\"") != string::npos);
}

TEST_F(HuffmanBtreepicTest, LargeFrequency)
{
  Freq_Node node;
  node.get_key().first = "a";
  node.get_key().second = 1000000;
  LLINK(&node) = nullptr;
  RLINK(&node) = nullptr;

  stringstream ss;
  output_ptr = &ss;
  huffman_to_btreepic(&node, false);

  EXPECT_TRUE(ss.str().find("\"1000000\"") != string::npos);
}

// =============================================================================
// Integration Test with Real Huffman Encoding
// =============================================================================

TEST_F(HuffmanBtreepicTest, IntegrationWithHuffmanEncoder)
{
  const char * text = "hello world";
  Huffman_Encoder_Engine encoder;
  char * str = const_cast<char*>(text);
  encoder.read_input(str, true);

  stringstream ss;
  huffman_to_btreepic(encoder.get_freq_root(), ss, false);

  string output = ss.str();

  // Verify output structure
  EXPECT_TRUE(output.find("start-prefix") != string::npos);
  EXPECT_TRUE(output.find("start-key") != string::npos);

  // Should have TAGs for leaf nodes
  size_t tag_count = 0;
  size_t pos = 0;
  while ((pos = output.find("TAG", pos)) != string::npos)
    {
      tag_count++;
      pos += 3;
    }

  // "hello world" has unique chars: h, e, l, o, ' ', w, r, d
  // But l appears 3 times, o 2 times, so we have fewer unique symbols
  EXPECT_GE(tag_count, 1u);

  // Clean up
  destroyRec(encoder.get_root());
  destroyRec(encoder.get_freq_root());
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
