
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/


/**
 * @file parse_utils_test.cc
 * @brief Comprehensive tests for parse_utils.H
 */

#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <parse_utils.H>

using namespace std;
using namespace testing;
using namespace Aleph;

// ============================================================================
// Test Fixture
// ============================================================================

class ParseUtilsTest : public Test
{
protected:
  string temp_filename;
  
  void SetUp() override
  {
    // Create a unique temp file name
    temp_filename = "/tmp/parse_utils_test_" + to_string(getpid()) + ".txt";
    reset_parse_state();
  }
  
  void TearDown() override
  {
    // Remove temp file if it exists
    remove(temp_filename.c_str());
  }
  
  // Helper to create temp file with content
  void create_temp_file(const string& content)
  {
    ofstream out(temp_filename);
    out << content;
    out.close();
  }
};

// ============================================================================
// put_char_in_buffer Tests
// ============================================================================

TEST(PutCharInBufferTest, BasicAppend)
{
  char buffer[10];
  char* ptr = buffer;
  char* end = buffer + 10;
  
  put_char_in_buffer(ptr, end, 'H');
  put_char_in_buffer(ptr, end, 'i');
  put_char_in_buffer(ptr, end, '\0');
  
  EXPECT_STREQ(buffer, "Hi");
}

TEST(PutCharInBufferTest, PointerAdvances)
{
  char buffer[10];
  char* ptr = buffer;
  char* end = buffer + 10;
  
  EXPECT_EQ(ptr, buffer);
  put_char_in_buffer(ptr, end, 'A');
  EXPECT_EQ(ptr, buffer + 1);
  put_char_in_buffer(ptr, end, 'B');
  EXPECT_EQ(ptr, buffer + 2);
}

TEST(PutCharInBufferTest, FillsToCapacity)
{
  char buffer[5];
  char* ptr = buffer;
  char* end = buffer + 5;
  
  for (int i = 0; i < 5; ++i)
    put_char_in_buffer(ptr, end, 'X');
  
  EXPECT_EQ(ptr, end);
}

// Note: Buffer overflow causes AH_ERROR which terminates - can't easily test

// ============================================================================
// init_token_scanning / close_token_scanning Tests
// ============================================================================

TEST(TokenScanningTest, InitSavesPosition)
{
  Aleph::current_line_number = 42;
  Aleph::current_col_number = 15;
  
  init_token_scanning();
  
  EXPECT_EQ(Aleph::previous_line_number, 42);
  EXPECT_EQ(Aleph::previous_col_number, 15);
}

TEST(TokenScanningTest, CloseSavesToken)
{
  char buffer[20] = "hello";
  char* ptr = buffer + 5;
  char* end = buffer + 20;
  
  close_token_scanning(buffer, ptr, end);
  
  EXPECT_EQ(Aleph::token_instance, "hello");
}

TEST(TokenScanningTest, CloseAddsNullTerminator)
{
  char buffer[20] = "test";
  char* ptr = buffer + 4;
  char* end = buffer + 20;
  
  // Overwrite the implicit null
  buffer[4] = 'X';
  buffer[5] = 'Y';
  
  close_token_scanning(buffer, ptr, end);
  
  // Should have added null terminator at position 4
  EXPECT_EQ(buffer[4], '\0');
  EXPECT_EQ(Aleph::token_instance, "test");
}

// ============================================================================
// read_char_from_stream Tests
// ============================================================================

TEST_F(ParseUtilsTest, ReadCharBasic)
{
  create_temp_file("ABC");
  ifstream input(temp_filename);
  
  EXPECT_EQ(read_char_from_stream(input), 'A');
  EXPECT_EQ(read_char_from_stream(input), 'B');
  EXPECT_EQ(read_char_from_stream(input), 'C');
}

TEST_F(ParseUtilsTest, ReadCharUpdatesColumn)
{
  create_temp_file("ABC");
  ifstream input(temp_filename);
  
  EXPECT_EQ(Aleph::current_col_number, 1);
  read_char_from_stream(input);
  EXPECT_EQ(Aleph::current_col_number, 2);
  read_char_from_stream(input);
  EXPECT_EQ(Aleph::current_col_number, 3);
}

TEST_F(ParseUtilsTest, ReadCharNewlineUpdatesLine)
{
  create_temp_file("A\nB");
  ifstream input(temp_filename);
  
  EXPECT_EQ(Aleph::current_line_number, 1);
  read_char_from_stream(input);  // 'A'
  EXPECT_EQ(Aleph::current_line_number, 1);
  read_char_from_stream(input);  // '\n'
  EXPECT_EQ(Aleph::current_line_number, 2);
  EXPECT_EQ(Aleph::current_col_number, 0);  // Reset on newline
}

TEST_F(ParseUtilsTest, ReadCharThrowsOnEOF)
{
  create_temp_file("");
  ifstream input(temp_filename);
  
  // Force EOF
  input.get();  // This will set EOF flag
  
  EXPECT_THROW(read_char_from_stream(input), out_of_range);
}

// ============================================================================
// skip_white_spaces Tests
// ============================================================================

TEST_F(ParseUtilsTest, SkipSpaces)
{
  create_temp_file("   hello");
  ifstream input(temp_filename);
  
  skip_white_spaces(input);
  
  EXPECT_EQ(input.peek(), 'h');
}

TEST_F(ParseUtilsTest, SkipTabs)
{
  create_temp_file("\t\thello");
  ifstream input(temp_filename);
  
  skip_white_spaces(input);
  
  EXPECT_EQ(input.peek(), 'h');
}

TEST_F(ParseUtilsTest, SkipNewlines)
{
  create_temp_file("\n\nhello");
  ifstream input(temp_filename);
  
  skip_white_spaces(input);
  
  EXPECT_EQ(input.peek(), 'h');
}

TEST_F(ParseUtilsTest, SkipMixedWhitespace)
{
  create_temp_file("  \t\n  \t\nhello");
  ifstream input(temp_filename);
  
  skip_white_spaces(input);
  
  EXPECT_EQ(input.peek(), 'h');
}

TEST_F(ParseUtilsTest, SkipNothingIfNoWhitespace)
{
  create_temp_file("hello");
  ifstream input(temp_filename);
  
  skip_white_spaces(input);
  
  EXPECT_EQ(input.peek(), 'h');
}

// ============================================================================
// load_number Tests
// ============================================================================

TEST_F(ParseUtilsTest, LoadPositiveNumber)
{
  create_temp_file("42 ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_number(input), 42);
}

TEST_F(ParseUtilsTest, LoadNegativeNumber)
{
  create_temp_file("-123 ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_number(input), -123);
}

TEST_F(ParseUtilsTest, LoadZero)
{
  create_temp_file("0 ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_number(input), 0);
}

TEST_F(ParseUtilsTest, LoadNumberWithLeadingSpaces)
{
  create_temp_file("   100 ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_number(input), 100);
}

TEST_F(ParseUtilsTest, LoadMultipleNumbers)
{
  create_temp_file("10 20 30 ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_number(input), 10);
  EXPECT_EQ(load_number(input), 20);
  EXPECT_EQ(load_number(input), 30);
}

TEST_F(ParseUtilsTest, LoadNumberAtEOF)
{
  // Number without trailing whitespace at EOF throws exception
  // because the parser expects whitespace to terminate numbers
  create_temp_file("42");
  ifstream input(temp_filename);
  
  // The implementation throws when number doesn't end in whitespace
  EXPECT_THROW(load_number(input), std::domain_error);
}

TEST_F(ParseUtilsTest, LoadNumberAtEOFWithNewline)
{
  // Number with trailing newline works fine
  create_temp_file("42\n");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_number(input), 42);
}

TEST_F(ParseUtilsTest, LoadLargeNumber)
{
  create_temp_file("1234567890 ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_number(input), 1234567890L);
}

TEST_F(ParseUtilsTest, LoadNegativeLargeNumber)
{
  create_temp_file("-987654321 ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_number(input), -987654321L);
}

TEST_F(ParseUtilsTest, LoadNumberSetsToken)
{
  create_temp_file("999 ");
  ifstream input(temp_filename);
  
  load_number(input);
  
  EXPECT_EQ(Aleph::token_instance, "999");
}

// ============================================================================
// load_string Tests
// ============================================================================

TEST_F(ParseUtilsTest, LoadUnquotedString)
{
  create_temp_file("hello ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_string(input), "hello");
}

TEST_F(ParseUtilsTest, LoadQuotedString)
{
  create_temp_file("\"hello world\" ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_string(input), "hello world");
}

TEST_F(ParseUtilsTest, LoadQuotedStringWithSpaces)
{
  create_temp_file("\"hello   world\" ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_string(input), "hello   world");
}

TEST_F(ParseUtilsTest, LoadStringWithLeadingSpaces)
{
  create_temp_file("   hello ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_string(input), "hello");
}

TEST_F(ParseUtilsTest, LoadMultipleStrings)
{
  create_temp_file("hello world test ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_string(input), "hello");
  EXPECT_EQ(load_string(input), "world");
  EXPECT_EQ(load_string(input), "test");
}

TEST_F(ParseUtilsTest, LoadMixedQuotedUnquoted)
{
  create_temp_file("hello \"world test\" foo ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_string(input), "hello");
  EXPECT_EQ(load_string(input), "world test");
  EXPECT_EQ(load_string(input), "foo");
}

TEST_F(ParseUtilsTest, LoadEmptyQuotedString)
{
  create_temp_file("\"\" ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_string(input), "");
}

TEST_F(ParseUtilsTest, LoadStringSetsToken)
{
  create_temp_file("test ");
  ifstream input(temp_filename);
  
  load_string(input);
  
  EXPECT_EQ(Aleph::token_instance, "test");
}

TEST_F(ParseUtilsTest, LoadStringWithNumbers)
{
  create_temp_file("abc123 ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_string(input), "abc123");
}

TEST_F(ParseUtilsTest, LoadQuotedStringWithNewline)
{
  create_temp_file("\"line1\nline2\" ");
  ifstream input(temp_filename);
  
  string result = load_string(input);
  EXPECT_EQ(result, "line1\nline2");
}

// ============================================================================
// command_line_to_string Tests
// ============================================================================

TEST(CommandLineTest, SingleArgument)
{
  const char* argv[] = {"program"};
  string result = command_line_to_string(1, const_cast<char**>(argv));
  
  EXPECT_EQ(result, " program");
}

TEST(CommandLineTest, MultipleArguments)
{
  const char* argv[] = {"program", "--input", "file.txt", "-v"};
  string result = command_line_to_string(4, const_cast<char**>(argv));
  
  EXPECT_EQ(result, " program --input file.txt -v");
}

TEST(CommandLineTest, EmptyArguments)
{
  string result = command_line_to_string(0, nullptr);
  
  EXPECT_EQ(result, "");
}

// ============================================================================
// reset_parse_state Tests
// ============================================================================

TEST(ResetParseStateTest, ResetsLineNumbers)
{
  Aleph::current_line_number = 100;
  Aleph::current_col_number = 50;
  Aleph::previous_line_number = 99;
  Aleph::previous_col_number = 45;
  
  reset_parse_state();
  
  EXPECT_EQ(Aleph::current_line_number, 1);
  EXPECT_EQ(Aleph::current_col_number, 1);
  EXPECT_EQ(Aleph::previous_line_number, 1);
  EXPECT_EQ(Aleph::previous_col_number, 1);
}

TEST(ResetParseStateTest, ClearsToken)
{
  Aleph::token_instance = "some token";
  
  reset_parse_state();
  
  EXPECT_TRUE(Aleph::token_instance.empty());
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(ParseUtilsTest, ParseSimpleConfigFile)
{
  // Simulate a simple config file format:
  // name value
  // ...
  create_temp_file("count 10\nname \"test file\"\nsize 42\n");
  ifstream input(temp_filename);
  
  string key1 = load_string(input);
  long val1 = load_number(input);
  EXPECT_EQ(key1, "count");
  EXPECT_EQ(val1, 10);
  
  string key2 = load_string(input);
  string val2 = load_string(input);
  EXPECT_EQ(key2, "name");
  EXPECT_EQ(val2, "test file");
  
  string key3 = load_string(input);
  long val3 = load_number(input);
  EXPECT_EQ(key3, "size");
  EXPECT_EQ(val3, 42);
}

TEST_F(ParseUtilsTest, ParseMixedContent)
{
  create_temp_file("   123   \"hello world\"   -456   test   ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_number(input), 123);
  EXPECT_EQ(load_string(input), "hello world");
  EXPECT_EQ(load_number(input), -456);
  EXPECT_EQ(load_string(input), "test");
}

TEST_F(ParseUtilsTest, ParseMultilineFile)
{
  create_temp_file("line1\nline2\nline3\n");
  ifstream input(temp_filename);
  
  // Note: current_line_number updates AFTER reading newline,
  // so after reading "line1\n", we're at line 2
  EXPECT_EQ(load_string(input), "line1");
  EXPECT_EQ(Aleph::current_line_number, 2);  // After reading newline
  
  EXPECT_EQ(load_string(input), "line2");
  EXPECT_EQ(Aleph::current_line_number, 3);
  
  EXPECT_EQ(load_string(input), "line3");
  EXPECT_EQ(Aleph::current_line_number, 4);
}

// ============================================================================
// Position Tracking Tests
// ============================================================================

TEST_F(ParseUtilsTest, PositionTrackingAccurate)
{
  create_temp_file("abc def\nghi jkl\n");
  ifstream input(temp_filename);
  
  // Read first token
  load_string(input);
  
  // Previous position is recorded by init_token_scanning() inside load_string()
  // It records position BEFORE skipping whitespace, which happens after token
  EXPECT_EQ(Aleph::previous_line_number, 1);
  EXPECT_EQ(Aleph::previous_col_number, 1);
  
  // Read second token - previous_col is where skip_white_spaces left off
  load_string(input);
  
  EXPECT_EQ(Aleph::previous_line_number, 1);
  // Column 6 because: skip reads space (col 5), then init captures col 6
  EXPECT_EQ(Aleph::previous_col_number, 6);
  
  // Read third token (on new line)
  load_string(input);
  
  EXPECT_EQ(Aleph::previous_line_number, 2);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(ParseUtilsTest, NumberOnlyMinus)
{
  create_temp_file("- 5 ");
  ifstream input(temp_filename);
  
  // A lone minus followed by space should return 0 (atol(""))
  long result = load_number(input);
  EXPECT_EQ(result, 0);  // atol("-") or empty
}

TEST_F(ParseUtilsTest, StringWithSpecialChars)
{
  create_temp_file("\"hello@world#test!\" ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_string(input), "hello@world#test!");
}

TEST_F(ParseUtilsTest, StringWithBackslash)
{
  create_temp_file("\"path\\to\\file\" ");
  ifstream input(temp_filename);
  
  // Backslashes are preserved (no escape processing)
  EXPECT_EQ(load_string(input), "path\\to\\file");
}

TEST_F(ParseUtilsTest, VeryLongString)
{
  string longstr(400, 'X');  // 400 X's, within Buffer_Size
  create_temp_file("\"" + longstr + "\" ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_string(input), longstr);
}

// ============================================================================
// Buffer Size Constant Test
// ============================================================================

TEST(BufferSizeTest, HasReasonableValue)
{
  EXPECT_GE(Buffer_Size, 256u);
  EXPECT_LE(Buffer_Size, 4096u);
}

// ============================================================================
// SourceLocation Tests
// ============================================================================

TEST(SourceLocationTest, DefaultConstruction)
{
  SourceLocation loc;
  EXPECT_TRUE(loc.filename.empty());
  EXPECT_EQ(loc.line, 1);
  EXPECT_EQ(loc.column, 1);
}

TEST(SourceLocationTest, FullConstruction)
{
  SourceLocation loc("test.cpp", 42, 15);
  EXPECT_EQ(loc.filename, "test.cpp");
  EXPECT_EQ(loc.line, 42);
  EXPECT_EQ(loc.column, 15);
}

TEST(SourceLocationTest, ToStringWithFilename)
{
  SourceLocation loc("test.cpp", 10, 5);
  EXPECT_EQ(loc.to_string(), "test.cpp:10:5");
}

TEST(SourceLocationTest, ToStringWithoutFilename)
{
  SourceLocation loc("", 10, 5);
  EXPECT_EQ(loc.to_string(), "10:5");
}

TEST(SourceLocationTest, CurrentFactory)
{
  Aleph::current_line_number = 100;
  Aleph::current_col_number = 50;
  
  auto loc = SourceLocation::current("file.txt");
  
  EXPECT_EQ(loc.filename, "file.txt");
  EXPECT_EQ(loc.line, 100);
  EXPECT_EQ(loc.column, 50);
  
  reset_parse_state();
}

// ============================================================================
// ParseError Tests
// ============================================================================

TEST(ParseErrorTest, WithLocation)
{
  SourceLocation loc("test.cpp", 5, 10);
  ParseError err("unexpected token", loc);
  
  string msg = err.what();
  EXPECT_NE(msg.find("test.cpp"), string::npos);
  EXPECT_NE(msg.find("5"), string::npos);
  EXPECT_NE(msg.find("unexpected token"), string::npos);
}

TEST(ParseErrorTest, LocationAccessor)
{
  SourceLocation loc("file.c", 20, 30);
  ParseError err("error", loc);
  
  EXPECT_EQ(err.location().line, 20);
  EXPECT_EQ(err.location().column, 30);
}

// ============================================================================
// Lookahead and Backtracking Tests
// ============================================================================

TEST_F(ParseUtilsTest, PeekCharDoesNotConsume)
{
  create_temp_file("ABC");
  ifstream input(temp_filename);
  
  EXPECT_EQ(peek_char(input), 'A');
  EXPECT_EQ(peek_char(input), 'A');  // Still 'A'
  EXPECT_EQ(input.get(), 'A');       // Now consumed
  EXPECT_EQ(peek_char(input), 'B');
}

TEST_F(ParseUtilsTest, MarkAndRestorePosition)
{
  create_temp_file("hello world");
  ifstream input(temp_filename);
  
  // Read "hello"
  for (int i = 0; i < 5; ++i)
    read_char_from_stream(input);
  
  StreamPosition pos = mark_position(input);
  
  // Read " world"
  for (int i = 0; i < 6; ++i)
    read_char_from_stream(input);
  
  // Restore
  restore_position(input, pos);
  
  EXPECT_EQ(input.get(), ' ');
  EXPECT_EQ(input.get(), 'w');
}

// ============================================================================
// Comment Handling Tests
// ============================================================================

TEST_F(ParseUtilsTest, SkipLineComment)
{
  create_temp_file("// this is a comment\nhello");
  ifstream input(temp_filename);
  
  input.get();  // '/'
  input.get();  // '/'
  current_col_number = 3;
  
  skip_line_comment(input);
  
  EXPECT_EQ(input.peek(), 'h');
  EXPECT_EQ(Aleph::current_line_number, 2);
}

TEST_F(ParseUtilsTest, SkipBlockComment)
{
  create_temp_file("/* comment */hello");
  ifstream input(temp_filename);
  
  input.get();  // '/'
  input.get();  // '*'
  current_col_number = 3;
  
  skip_block_comment(input);
  
  EXPECT_EQ(input.peek(), 'h');
}

TEST_F(ParseUtilsTest, SkipBlockCommentMultiline)
{
  create_temp_file("/* line 1\n   line 2\n   line 3 */hello");
  ifstream input(temp_filename);
  
  input.get();  // '/'
  input.get();  // '*'
  current_col_number = 3;
  
  skip_block_comment(input);
  
  EXPECT_EQ(input.peek(), 'h');
  EXPECT_EQ(Aleph::current_line_number, 3);
}

TEST_F(ParseUtilsTest, SkipWhitespaceAndComments)
{
  create_temp_file("  // comment\n  /* block */  hello");
  ifstream input(temp_filename);
  
  skip_whitespace_and_comments(input);
  
  EXPECT_EQ(input.peek(), 'h');
}

TEST_F(ParseUtilsTest, SkipHashComment)
{
  create_temp_file("  # python style comment\nhello");
  ifstream input(temp_filename);
  
  skip_whitespace_and_comments(input);
  
  EXPECT_EQ(input.peek(), 'h');
}

// ============================================================================
// Extended Numeric Parsing Tests
// ============================================================================

TEST_F(ParseUtilsTest, LoadDoubleBasic)
{
  create_temp_file("3.14159 ");
  ifstream input(temp_filename);
  
  EXPECT_NEAR(load_double(input), 3.14159, 0.00001);
}

TEST_F(ParseUtilsTest, LoadDoubleNegative)
{
  create_temp_file("-2.5 ");
  ifstream input(temp_filename);
  
  EXPECT_NEAR(load_double(input), -2.5, 0.001);
}

TEST_F(ParseUtilsTest, LoadDoubleScientific)
{
  create_temp_file("1.5e10 ");
  ifstream input(temp_filename);
  
  EXPECT_NEAR(load_double(input), 1.5e10, 1e5);
}

TEST_F(ParseUtilsTest, LoadDoubleNegativeExponent)
{
  create_temp_file("2.5e-3 ");
  ifstream input(temp_filename);
  
  EXPECT_NEAR(load_double(input), 0.0025, 0.00001);
}

TEST_F(ParseUtilsTest, LoadDoubleLeadingDecimal)
{
  create_temp_file(".5 ");
  ifstream input(temp_filename);
  
  EXPECT_NEAR(load_double(input), 0.5, 0.001);
}

TEST_F(ParseUtilsTest, LoadDoubleTrailingDecimal)
{
  create_temp_file("5. ");
  ifstream input(temp_filename);
  
  EXPECT_NEAR(load_double(input), 5.0, 0.001);
}

TEST_F(ParseUtilsTest, LoadHexNumber)
{
  create_temp_file("0xFF ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_hex_number(input), 255);
}

TEST_F(ParseUtilsTest, LoadHexNumberLowercase)
{
  create_temp_file("0x1a2b ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_hex_number(input), 0x1a2b);
}

TEST_F(ParseUtilsTest, LoadOctalNumber)
{
  create_temp_file("0755 ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_octal_number(input), 0755);
}

TEST_F(ParseUtilsTest, LoadOctalZero)
{
  create_temp_file("0 ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_octal_number(input), 0);
}

TEST_F(ParseUtilsTest, LoadBinaryNumber)
{
  create_temp_file("0b1010 ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_binary_number(input), 10);
}

TEST_F(ParseUtilsTest, LoadBinaryNumberUpper)
{
  create_temp_file("0B11110000 ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_binary_number(input), 0xF0);
}

// ============================================================================
// Identifier Tests
// ============================================================================

TEST_F(ParseUtilsTest, LoadIdentifierBasic)
{
  create_temp_file("myVariable ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_identifier(input), "myVariable");
}

TEST_F(ParseUtilsTest, LoadIdentifierWithUnderscore)
{
  create_temp_file("_private_var ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_identifier(input), "_private_var");
}

TEST_F(ParseUtilsTest, LoadIdentifierWithNumbers)
{
  create_temp_file("var123 ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_identifier(input), "var123");
}

TEST_F(ParseUtilsTest, LoadIdentifierCamelCase)
{
  create_temp_file("camelCaseIdentifier ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_identifier(input), "camelCaseIdentifier");
}

TEST_F(ParseUtilsTest, LoadIdentifierInvalid)
{
  create_temp_file("123invalid ");
  ifstream input(temp_filename);
  
  EXPECT_THROW(load_identifier(input), ParseError);
}

// ============================================================================
// Keyword Tests
// ============================================================================

TEST(KeywordTest, IsKeywordTrue)
{
  vector<string> keywords = {"if", "while", "for", "return"};
  
  EXPECT_TRUE(is_keyword("if", keywords));
  EXPECT_TRUE(is_keyword("while", keywords));
  EXPECT_TRUE(is_keyword("return", keywords));
}

TEST(KeywordTest, IsKeywordFalse)
{
  vector<string> keywords = {"if", "while", "for", "return"};
  
  EXPECT_FALSE(is_keyword("IF", keywords));  // Case sensitive
  EXPECT_FALSE(is_keyword("unless", keywords));
  EXPECT_FALSE(is_keyword("", keywords));
}

// ============================================================================
// Expectation Tests
// ============================================================================

TEST_F(ParseUtilsTest, ExpectCharSuccess)
{
  create_temp_file("  ( hello");
  ifstream input(temp_filename);
  
  EXPECT_NO_THROW(expect_char(input, '('));
}

TEST_F(ParseUtilsTest, ExpectCharFailure)
{
  create_temp_file("  [ hello");
  ifstream input(temp_filename);
  
  EXPECT_THROW(expect_char(input, '('), ParseError);
}

TEST_F(ParseUtilsTest, ExpectSuccess)
{
  create_temp_file("  function foo");
  ifstream input(temp_filename);
  
  EXPECT_NO_THROW(expect(input, "function"));
}

TEST_F(ParseUtilsTest, ExpectFailure)
{
  create_temp_file("  procedure foo");
  ifstream input(temp_filename);
  
  EXPECT_THROW(expect(input, "function"), ParseError);
}

TEST_F(ParseUtilsTest, TryCharSuccess)
{
  create_temp_file("  ; next");
  ifstream input(temp_filename);
  
  EXPECT_TRUE(try_char(input, ';'));
  // Should have consumed the ';'
  skip_white_spaces(input);
  EXPECT_EQ(input.peek(), 'n');
}

TEST_F(ParseUtilsTest, TryCharFailure)
{
  create_temp_file("  , next");
  ifstream input(temp_filename);
  
  EXPECT_FALSE(try_char(input, ';'));
  // Should NOT have consumed anything
  skip_white_spaces(input);
  EXPECT_EQ(input.peek(), ',');
}

// ============================================================================
// Escape Processing Tests
// ============================================================================

TEST(EscapeTest, ProcessBasicEscapes)
{
  EXPECT_EQ(process_escape('n'), '\n');
  EXPECT_EQ(process_escape('t'), '\t');
  EXPECT_EQ(process_escape('r'), '\r');
  EXPECT_EQ(process_escape('\\'), '\\');
  EXPECT_EQ(process_escape('"'), '"');
  EXPECT_EQ(process_escape('\''), '\'');
  EXPECT_EQ(process_escape('0'), '\0');
}

TEST(EscapeTest, ProcessUnknownEscape)
{
  // Unknown escapes return the character as-is
  EXPECT_EQ(process_escape('x'), 'x');
  EXPECT_EQ(process_escape('q'), 'q');
}

// ============================================================================
// Escaped String Tests
// ============================================================================

TEST_F(ParseUtilsTest, LoadEscapedStringBasic)
{
  create_temp_file("\"hello\\nworld\" ");
  ifstream input(temp_filename);
  
  string result = load_escaped_string(input);
  EXPECT_EQ(result, "hello\nworld");
}

TEST_F(ParseUtilsTest, LoadEscapedStringMultipleEscapes)
{
  create_temp_file("\"line1\\tline2\\r\\nline3\" ");
  ifstream input(temp_filename);
  
  string result = load_escaped_string(input);
  EXPECT_EQ(result, "line1\tline2\r\nline3");
}

TEST_F(ParseUtilsTest, LoadEscapedStringQuotes)
{
  create_temp_file("\"he said \\\"hello\\\"\" ");
  ifstream input(temp_filename);
  
  string result = load_escaped_string(input);
  EXPECT_EQ(result, "he said \"hello\"");
}

TEST_F(ParseUtilsTest, LoadEscapedStringBackslash)
{
  create_temp_file("\"path\\\\to\\\\file\" ");
  ifstream input(temp_filename);
  
  string result = load_escaped_string(input);
  EXPECT_EQ(result, "path\\to\\file");
}

TEST_F(ParseUtilsTest, LoadEscapedStringUnterminated)
{
  create_temp_file("\"no closing quote");
  ifstream input(temp_filename);
  
  EXPECT_THROW(load_escaped_string(input), ParseError);
}

// ============================================================================
// Character Literal Tests
// ============================================================================

TEST_F(ParseUtilsTest, LoadCharLiteralBasic)
{
  create_temp_file("'a' ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_char_literal(input), 'a');
}

TEST_F(ParseUtilsTest, LoadCharLiteralEscaped)
{
  create_temp_file("'\\n' ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_char_literal(input), '\n');
}

TEST_F(ParseUtilsTest, LoadCharLiteralTab)
{
  create_temp_file("'\\t' ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_char_literal(input), '\t');
}

TEST_F(ParseUtilsTest, LoadCharLiteralQuote)
{
  create_temp_file("'\\'' ");
  ifstream input(temp_filename);
  
  EXPECT_EQ(load_char_literal(input), '\'');
}

// ============================================================================
// Token Type Tests
// ============================================================================

TEST(TokenTypeTest, ToString)
{
  EXPECT_EQ(token_type_to_string(TokenType::END_OF_FILE), "EOF");
  EXPECT_EQ(token_type_to_string(TokenType::IDENTIFIER), "IDENTIFIER");
  EXPECT_EQ(token_type_to_string(TokenType::INTEGER), "INTEGER");
  EXPECT_EQ(token_type_to_string(TokenType::STRING), "STRING");
}

TEST(TokenTest, Construction)
{
  SourceLocation loc("file.c", 10, 5);
  Token tok(TokenType::IDENTIFIER, "myVar", loc);
  
  EXPECT_EQ(tok.type, TokenType::IDENTIFIER);
  EXPECT_EQ(tok.value, "myVar");
  EXPECT_EQ(tok.location.line, 10);
}

TEST(TokenTest, IsEof)
{
  Token eof(TokenType::END_OF_FILE, "", SourceLocation());
  Token id(TokenType::IDENTIFIER, "x", SourceLocation());
  
  EXPECT_TRUE(eof.is_eof());
  EXPECT_FALSE(id.is_eof());
}

// ============================================================================
// File Utility Tests
// ============================================================================

TEST_F(ParseUtilsTest, LoadFileContents)
{
  create_temp_file("line1\nline2\nline3");
  
  string contents = load_file_contents(temp_filename);
  EXPECT_EQ(contents, "line1\nline2\nline3");
}

TEST_F(ParseUtilsTest, LoadFileLines)
{
  create_temp_file("line1\nline2\nline3");
  
  vector<string> lines = load_file_lines(temp_filename);
  ASSERT_EQ(lines.size(), 3u);
  EXPECT_EQ(lines[0], "line1");
  EXPECT_EQ(lines[1], "line2");
  EXPECT_EQ(lines[2], "line3");
}

TEST(FileUtilsTest, LoadFileContentsNotFound)
{
  EXPECT_THROW(load_file_contents("/nonexistent/file.txt"), runtime_error);
}

// ============================================================================
// String Utility Tests
// ============================================================================

TEST(StringUtilsTest, SplitString)
{
  vector<string> parts = split_string("a,b,c,d", ',');
  ASSERT_EQ(parts.size(), 4u);
  EXPECT_EQ(parts[0], "a");
  EXPECT_EQ(parts[1], "b");
  EXPECT_EQ(parts[2], "c");
  EXPECT_EQ(parts[3], "d");
}

TEST(StringUtilsTest, SplitStringEmpty)
{
  // std::getline produces no tokens for empty string
  vector<string> parts = split_string("", ',');
  EXPECT_EQ(parts.size(), 0u);
}

TEST(StringUtilsTest, TrimSpaces)
{
  EXPECT_EQ(trim("  hello  "), "hello");
  EXPECT_EQ(trim("hello"), "hello");
  EXPECT_EQ(trim("  hello"), "hello");
  EXPECT_EQ(trim("hello  "), "hello");
}

TEST(StringUtilsTest, TrimTabs)
{
  EXPECT_EQ(trim("\t\thello\t\t"), "hello");
}

TEST(StringUtilsTest, TrimEmpty)
{
  EXPECT_EQ(trim(""), "");
  EXPECT_EQ(trim("   "), "");
}

// ============================================================================
// Integration Test: Simple Expression Parser
// ============================================================================

TEST_F(ParseUtilsTest, ParseSimpleExpression)
{
  // Parse: x = 42 + 3.14;
  create_temp_file("x = 42 + 3.14;");
  ifstream input(temp_filename);
  
  string id = load_identifier(input);
  EXPECT_EQ(id, "x");
  
  expect_char(input, '=');
  
  long intVal = load_number(input);
  EXPECT_EQ(intVal, 42);
  
  expect_char(input, '+');
  
  double dblVal = load_double(input);
  EXPECT_NEAR(dblVal, 3.14, 0.01);
  
  expect_char(input, ';');
}

TEST_F(ParseUtilsTest, ParseWithComments)
{
  // Note: load_number expects whitespace after number, so use spaces before ';'
  create_temp_file("// comment\nx = 10 ; /* another */ y = 20 ;");
  ifstream input(temp_filename);
  
  skip_whitespace_and_comments(input);
  EXPECT_EQ(load_identifier(input), "x");
  skip_white_spaces(input);
  expect_char(input, '=');
  EXPECT_EQ(load_number(input), 10);
  expect_char(input, ';');
  
  skip_whitespace_and_comments(input);
  EXPECT_EQ(load_identifier(input), "y");
  skip_white_spaces(input);
  expect_char(input, '=');
  EXPECT_EQ(load_number(input), 20);
}
