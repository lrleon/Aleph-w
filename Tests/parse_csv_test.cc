/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon

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
 * @file parse_csv_test.cc
 * @brief Comprehensive test suite for parse-csv.H - CSV parsing utilities
 *
 * Tests cover:
 * - csv_read_row: Basic parsing, quoted fields, escaped quotes, delimiters
 * - csv_read_all: Multiple rows, empty rows, different line endings
 * - csv_escape: Quoting rules, delimiter escaping, quote escaping
 * - csv_write_row: Output formatting, proper escaping
 * - csv_write_all: Multiple rows output
 * - csv_read_file / csv_write_file: File I/O operations
 * - csv_get_column: Column extraction
 * - csv_to_number: Numeric conversions
 * - csv_is_rectangular: Shape validation
 * - Edge cases: Empty input, single field, trailing newlines
 */

#include <gtest/gtest.h>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <parse-csv.H>

using namespace Aleph;

//============================================================================
// csv_read_row Tests - Stream Input
//============================================================================

class CsvReadRowStreamTest : public ::testing::Test {};

TEST_F(CsvReadRowStreamTest, SimpleRow)
{
  std::istringstream ss("a,b,c\n");
  Array<std::string> row = csv_read_row(ss);
  
  ASSERT_EQ(row.size(), 3u);
  EXPECT_EQ(row(0), "a");
  EXPECT_EQ(row(1), "b");
  EXPECT_EQ(row(2), "c");
}

TEST_F(CsvReadRowStreamTest, SingleField)
{
  std::istringstream ss("hello\n");
  Array<std::string> row = csv_read_row(ss);
  
  ASSERT_EQ(row.size(), 1u);
  EXPECT_EQ(row(0), "hello");
}

TEST_F(CsvReadRowStreamTest, EmptyFields)
{
  std::istringstream ss(",b,,d\n");
  Array<std::string> row = csv_read_row(ss);
  
  ASSERT_EQ(row.size(), 4u);
  EXPECT_EQ(row(0), "");
  EXPECT_EQ(row(1), "b");
  EXPECT_EQ(row(2), "");
  EXPECT_EQ(row(3), "d");
}

TEST_F(CsvReadRowStreamTest, QuotedField)
{
  std::istringstream ss("a,\"hello world\",c\n");
  Array<std::string> row = csv_read_row(ss);
  
  ASSERT_EQ(row.size(), 3u);
  EXPECT_EQ(row(0), "a");
  EXPECT_EQ(row(1), "hello world");
  EXPECT_EQ(row(2), "c");
}

TEST_F(CsvReadRowStreamTest, QuotedFieldWithDelimiter)
{
  std::istringstream ss("a,\"hello, world\",c\n");
  Array<std::string> row = csv_read_row(ss);
  
  ASSERT_EQ(row.size(), 3u);
  EXPECT_EQ(row(0), "a");
  EXPECT_EQ(row(1), "hello, world");
  EXPECT_EQ(row(2), "c");
}

TEST_F(CsvReadRowStreamTest, EscapedQuotes)
{
  std::istringstream ss("a,\"say \"\"hi\"\"\",c\n");
  Array<std::string> row = csv_read_row(ss);
  
  ASSERT_EQ(row.size(), 3u);
  EXPECT_EQ(row(0), "a");
  EXPECT_EQ(row(1), "say \"hi\"");
  EXPECT_EQ(row(2), "c");
}

TEST_F(CsvReadRowStreamTest, QuotedFieldWithNewline)
{
  std::istringstream ss("a,\"line1\nline2\",c\n");
  Array<std::string> row = csv_read_row(ss);
  
  ASSERT_EQ(row.size(), 3u);
  EXPECT_EQ(row(0), "a");
  EXPECT_EQ(row(1), "line1\nline2");
  EXPECT_EQ(row(2), "c");
}

TEST_F(CsvReadRowStreamTest, CRLFLineEnding)
{
  std::istringstream ss("a,b,c\r\n");
  Array<std::string> row = csv_read_row(ss);
  
  ASSERT_EQ(row.size(), 3u);
  EXPECT_EQ(row(0), "a");
  EXPECT_EQ(row(1), "b");
  EXPECT_EQ(row(2), "c");
}

TEST_F(CsvReadRowStreamTest, NoTrailingNewline)
{
  std::istringstream ss("a,b,c");
  Array<std::string> row = csv_read_row(ss);
  
  ASSERT_EQ(row.size(), 3u);
  EXPECT_EQ(row(0), "a");
  EXPECT_EQ(row(1), "b");
  EXPECT_EQ(row(2), "c");
}

TEST_F(CsvReadRowStreamTest, CustomDelimiter)
{
  std::istringstream ss("a;b;c\n");
  Array<std::string> row = csv_read_row(ss, ';');
  
  ASSERT_EQ(row.size(), 3u);
  EXPECT_EQ(row(0), "a");
  EXPECT_EQ(row(1), "b");
  EXPECT_EQ(row(2), "c");
}

TEST_F(CsvReadRowStreamTest, TabDelimiter)
{
  std::istringstream ss("a\tb\tc\n");
  Array<std::string> row = csv_read_row(ss, '\t');
  
  ASSERT_EQ(row.size(), 3u);
  EXPECT_EQ(row(0), "a");
  EXPECT_EQ(row(1), "b");
  EXPECT_EQ(row(2), "c");
}

TEST_F(CsvReadRowStreamTest, MultipleRows)
{
  std::istringstream ss("a,b,c\n1,2,3\nx,y,z\n");
  
  Array<std::string> row1 = csv_read_row(ss);
  Array<std::string> row2 = csv_read_row(ss);
  Array<std::string> row3 = csv_read_row(ss);
  
  ASSERT_EQ(row1.size(), 3u);
  EXPECT_EQ(row1(0), "a");
  
  ASSERT_EQ(row2.size(), 3u);
  EXPECT_EQ(row2(0), "1");
  
  ASSERT_EQ(row3.size(), 3u);
  EXPECT_EQ(row3(0), "x");
}

TEST_F(CsvReadRowStreamTest, EmptyStream)
{
  std::istringstream ss("");
  Array<std::string> row = csv_read_row(ss);
  
  // Empty stream produces one empty field (consistent with CSV parsers)
  // An empty string "" still has one field (empty)
  EXPECT_LE(row.size(), 1u);
}

TEST_F(CsvReadRowStreamTest, WhitespacePreserved)
{
  std::istringstream ss("  a  ,  b  ,  c  \n");
  Array<std::string> row = csv_read_row(ss);
  
  ASSERT_EQ(row.size(), 3u);
  EXPECT_EQ(row(0), "  a  ");
  EXPECT_EQ(row(1), "  b  ");
  EXPECT_EQ(row(2), "  c  ");
}

//============================================================================
// csv_read_row Tests - String Input
//============================================================================

class CsvReadRowStringTest : public ::testing::Test {};

TEST_F(CsvReadRowStringTest, SimpleRow)
{
  std::string line = "a,b,c";
  Array<std::string> row = csv_read_row(line);
  
  ASSERT_EQ(row.size(), 3u);
  EXPECT_EQ(row(0), "a");
  EXPECT_EQ(row(1), "b");
  EXPECT_EQ(row(2), "c");
}

TEST_F(CsvReadRowStringTest, QuotedFieldInString)
{
  std::string line = "a,\"hello, world\",c";
  Array<std::string> row = csv_read_row(line);
  
  ASSERT_EQ(row.size(), 3u);
  EXPECT_EQ(row(1), "hello, world");
}

TEST_F(CsvReadRowStringTest, EscapedQuotesInString)
{
  std::string line = "a,\"say \"\"hello\"\"\",c";
  Array<std::string> row = csv_read_row(line);
  
  ASSERT_EQ(row.size(), 3u);
  EXPECT_EQ(row(1), "say \"hello\"");
}

//============================================================================
// csv_read_all Tests
//============================================================================

class CsvReadAllTest : public ::testing::Test {};

TEST_F(CsvReadAllTest, MultipleRows)
{
  std::istringstream ss("a,b,c\n1,2,3\nx,y,z\n");
  Array<Array<std::string>> data = csv_read_all(ss);
  
  ASSERT_EQ(data.size(), 3u);
  EXPECT_EQ(data(0)(0), "a");
  EXPECT_EQ(data(1)(0), "1");
  EXPECT_EQ(data(2)(0), "x");
}

TEST_F(CsvReadAllTest, SingleRow)
{
  std::istringstream ss("a,b,c\n");
  Array<Array<std::string>> data = csv_read_all(ss);
  
  ASSERT_EQ(data.size(), 1u);
  EXPECT_EQ(data(0).size(), 3u);
}

TEST_F(CsvReadAllTest, NoTrailingNewline)
{
  std::istringstream ss("a,b,c\n1,2,3");
  Array<Array<std::string>> data = csv_read_all(ss);
  
  ASSERT_EQ(data.size(), 2u);
  EXPECT_EQ(data(1)(2), "3");
}

TEST_F(CsvReadAllTest, EmptyStream)
{
  std::istringstream ss("");
  Array<Array<std::string>> data = csv_read_all(ss);
  
  // Empty stream may produce 0 or 1 empty rows depending on implementation
  EXPECT_LE(data.size(), 1u);
}

TEST_F(CsvReadAllTest, RowsWithDifferentLengths)
{
  std::istringstream ss("a,b,c\n1,2\nx,y,z,w\n");
  Array<Array<std::string>> data = csv_read_all(ss);
  
  ASSERT_EQ(data.size(), 3u);
  EXPECT_EQ(data(0).size(), 3u);
  EXPECT_EQ(data(1).size(), 2u);
  EXPECT_EQ(data(2).size(), 4u);
}

TEST_F(CsvReadAllTest, CustomDelimiter)
{
  std::istringstream ss("a;b;c\n1;2;3\n");
  Array<Array<std::string>> data = csv_read_all(ss, ';');
  
  ASSERT_EQ(data.size(), 2u);
  EXPECT_EQ(data(0)(1), "b");
  EXPECT_EQ(data(1)(1), "2");
}

//============================================================================
// csv_escape Tests
//============================================================================

class CsvEscapeTest : public ::testing::Test {};

TEST_F(CsvEscapeTest, NoEscapingNeeded)
{
  EXPECT_EQ(csv_escape("hello"), "hello");
  EXPECT_EQ(csv_escape("world123"), "world123");
  EXPECT_EQ(csv_escape(""), "");
}

TEST_F(CsvEscapeTest, EscapeDelimiter)
{
  EXPECT_EQ(csv_escape("hello, world"), "\"hello, world\"");
}

TEST_F(CsvEscapeTest, EscapeQuotes)
{
  EXPECT_EQ(csv_escape("say \"hi\""), "\"say \"\"hi\"\"\"");
}

TEST_F(CsvEscapeTest, EscapeNewline)
{
  EXPECT_EQ(csv_escape("line1\nline2"), "\"line1\nline2\"");
}

TEST_F(CsvEscapeTest, EscapeCarriageReturn)
{
  EXPECT_EQ(csv_escape("line1\rline2"), "\"line1\rline2\"");
}

TEST_F(CsvEscapeTest, EscapeMultipleSpecialChars)
{
  EXPECT_EQ(csv_escape("a,\"b\"\nc"), "\"a,\"\"b\"\"\nc\"");
}

TEST_F(CsvEscapeTest, CustomDelimiter)
{
  EXPECT_EQ(csv_escape("a;b", ';'), "\"a;b\"");
  EXPECT_EQ(csv_escape("a,b", ';'), "a,b");  // Comma not special with semicolon delimiter
}

//============================================================================
// csv_write_row Tests
//============================================================================

class CsvWriteRowTest : public ::testing::Test {};

TEST_F(CsvWriteRowTest, SimpleRow)
{
  Array<std::string> row;
  row.append("a");
  row.append("b");
  row.append("c");
  
  std::ostringstream out;
  csv_write_row(out, row);
  
  EXPECT_EQ(out.str(), "a,b,c\n");
}

TEST_F(CsvWriteRowTest, RowWithQuotedFields)
{
  Array<std::string> row;
  row.append("name");
  row.append("hello, world");  // Contains comma
  row.append("value");
  
  std::ostringstream out;
  csv_write_row(out, row);
  
  EXPECT_EQ(out.str(), "name,\"hello, world\",value\n");
}

TEST_F(CsvWriteRowTest, RowWithQuotesInField)
{
  Array<std::string> row;
  row.append("say \"hi\"");
  
  std::ostringstream out;
  csv_write_row(out, row);
  
  EXPECT_EQ(out.str(), "\"say \"\"hi\"\"\"\n");
}

TEST_F(CsvWriteRowTest, EmptyRow)
{
  Array<std::string> row;
  
  std::ostringstream out;
  csv_write_row(out, row);
  
  EXPECT_EQ(out.str(), "\n");
}

TEST_F(CsvWriteRowTest, SingleField)
{
  Array<std::string> row;
  row.append("single");
  
  std::ostringstream out;
  csv_write_row(out, row);
  
  EXPECT_EQ(out.str(), "single\n");
}

TEST_F(CsvWriteRowTest, CustomDelimiter)
{
  Array<std::string> row;
  row.append("a");
  row.append("b");
  row.append("c");
  
  std::ostringstream out;
  csv_write_row(out, row, ';');
  
  EXPECT_EQ(out.str(), "a;b;c\n");
}

TEST_F(CsvWriteRowTest, CustomLineEnding)
{
  Array<std::string> row;
  row.append("a");
  row.append("b");
  
  std::ostringstream out;
  csv_write_row(out, row, ',', "\r\n");
  
  EXPECT_EQ(out.str(), "a,b\r\n");
}

//============================================================================
// csv_write_all Tests
//============================================================================

class CsvWriteAllTest : public ::testing::Test {};

TEST_F(CsvWriteAllTest, MultipleRows)
{
  Array<Array<std::string>> data;
  
  Array<std::string> row1;
  row1.append("a");
  row1.append("b");
  data.append(row1);
  
  Array<std::string> row2;
  row2.append("1");
  row2.append("2");
  data.append(row2);
  
  std::ostringstream out;
  csv_write_all(out, data);
  
  EXPECT_EQ(out.str(), "a,b\n1,2\n");
}

TEST_F(CsvWriteAllTest, EmptyData)
{
  Array<Array<std::string>> data;
  
  std::ostringstream out;
  csv_write_all(out, data);
  
  EXPECT_EQ(out.str(), "");
}

//============================================================================
// csv_read_file / csv_write_file Tests
//============================================================================

class CsvFileTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    test_filename = "/tmp/aleph_csv_test_" + std::to_string(rand()) + ".csv";
  }
  
  void TearDown() override
  {
    std::remove(test_filename.c_str());
  }
  
  std::string test_filename;
};

TEST_F(CsvFileTest, WriteAndReadFile)
{
  // Create test data
  Array<Array<std::string>> data;
  
  Array<std::string> header;
  header.append("name");
  header.append("value");
  data.append(header);
  
  Array<std::string> row1;
  row1.append("item1");
  row1.append("100");
  data.append(row1);
  
  Array<std::string> row2;
  row2.append("item2");
  row2.append("200");
  data.append(row2);
  
  // Write to file
  ASSERT_NO_THROW(csv_write_file(test_filename, data));
  
  // Read back
  Array<Array<std::string>> read_data;
  ASSERT_NO_THROW(read_data = csv_read_file(test_filename));
  
  // Verify
  ASSERT_EQ(read_data.size(), 3u);
  EXPECT_EQ(read_data(0)(0), "name");
  EXPECT_EQ(read_data(0)(1), "value");
  EXPECT_EQ(read_data(1)(0), "item1");
  EXPECT_EQ(read_data(1)(1), "100");
  EXPECT_EQ(read_data(2)(0), "item2");
  EXPECT_EQ(read_data(2)(1), "200");
}

TEST_F(CsvFileTest, ReadNonExistentFile)
{
  EXPECT_THROW(csv_read_file("/nonexistent/path/file.csv"), std::runtime_error);
}

TEST_F(CsvFileTest, WriteToInvalidPath)
{
  Array<Array<std::string>> data;
  Array<std::string> row;
  row.append("test");
  data.append(row);
  
  EXPECT_THROW(csv_write_file("/nonexistent/path/file.csv", data), std::runtime_error);
}

TEST_F(CsvFileTest, WriteAndReadWithSpecialCharacters)
{
  Array<Array<std::string>> data;
  
  Array<std::string> row;
  row.append("field with, comma");
  row.append("field with \"quotes\"");
  row.append("field with\nnewline");
  data.append(row);
  
  csv_write_file(test_filename, data);
  Array<Array<std::string>> read_data = csv_read_file(test_filename);
  
  ASSERT_EQ(read_data.size(), 1u);
  ASSERT_EQ(read_data(0).size(), 3u);
  EXPECT_EQ(read_data(0)(0), "field with, comma");
  EXPECT_EQ(read_data(0)(1), "field with \"quotes\"");
  EXPECT_EQ(read_data(0)(2), "field with\nnewline");
}

//============================================================================
// csv_num_columns Tests
//============================================================================

class CsvNumColumnsTest : public ::testing::Test {};

TEST_F(CsvNumColumnsTest, BasicUsage)
{
  std::string line = "a,b,c,d,e";
  Array<std::string> row = csv_read_row(line);
  
  EXPECT_EQ(csv_num_columns(row), 5u);
}

TEST_F(CsvNumColumnsTest, EmptyRow)
{
  Array<std::string> row;
  EXPECT_EQ(csv_num_columns(row), 0u);
}

TEST_F(CsvNumColumnsTest, SingleColumn)
{
  std::string line = "single";
  Array<std::string> row = csv_read_row(line);
  
  EXPECT_EQ(csv_num_columns(row), 1u);
}

//============================================================================
// csv_is_rectangular Tests
//============================================================================

class CsvIsRectangularTest : public ::testing::Test {};

TEST_F(CsvIsRectangularTest, EmptyData)
{
  Array<Array<std::string>> data;
  EXPECT_TRUE(csv_is_rectangular(data));
}

TEST_F(CsvIsRectangularTest, RectangularData)
{
  std::istringstream ss("a,b,c\n1,2,3\nx,y,z\n");
  Array<Array<std::string>> data = csv_read_all(ss);
  
  EXPECT_TRUE(csv_is_rectangular(data));
}

TEST_F(CsvIsRectangularTest, NonRectangularData)
{
  std::istringstream ss("a,b,c\n1,2\nx,y,z,w\n");
  Array<Array<std::string>> data = csv_read_all(ss);
  
  EXPECT_FALSE(csv_is_rectangular(data));
}

TEST_F(CsvIsRectangularTest, SingleRow)
{
  std::istringstream ss("a,b,c\n");
  Array<Array<std::string>> data = csv_read_all(ss);
  
  EXPECT_TRUE(csv_is_rectangular(data));
}

//============================================================================
// csv_get_column Tests
//============================================================================

class CsvGetColumnTest : public ::testing::Test {};

TEST_F(CsvGetColumnTest, BasicColumn)
{
  std::istringstream ss("name,age\nAlice,30\nBob,25\n");
  Array<Array<std::string>> data = csv_read_all(ss);
  
  Array<std::string> names = csv_get_column(data, 0);
  
  ASSERT_EQ(names.size(), 3u);
  EXPECT_EQ(names(0), "name");
  EXPECT_EQ(names(1), "Alice");
  EXPECT_EQ(names(2), "Bob");
}

TEST_F(CsvGetColumnTest, SecondColumn)
{
  std::istringstream ss("name,age\nAlice,30\nBob,25\n");
  Array<Array<std::string>> data = csv_read_all(ss);
  
  Array<std::string> ages = csv_get_column(data, 1);
  
  ASSERT_EQ(ages.size(), 3u);
  EXPECT_EQ(ages(0), "age");
  EXPECT_EQ(ages(1), "30");
  EXPECT_EQ(ages(2), "25");
}

TEST_F(CsvGetColumnTest, OutOfRangeColumn)
{
  std::istringstream ss("a,b\n1,2\n");
  Array<Array<std::string>> data = csv_read_all(ss);
  
  EXPECT_THROW(csv_get_column(data, 5), std::out_of_range);
}

TEST_F(CsvGetColumnTest, EmptyData)
{
  Array<Array<std::string>> data;
  Array<std::string> column = csv_get_column(data, 0);
  
  EXPECT_EQ(column.size(), 0u);
}

//============================================================================
// csv_to_number Tests
//============================================================================

class CsvToNumberTest : public ::testing::Test {};

TEST_F(CsvToNumberTest, ConvertToInt)
{
  EXPECT_EQ(csv_to_number<int>("42"), 42);
  EXPECT_EQ(csv_to_number<int>("-17"), -17);
  EXPECT_EQ(csv_to_number<int>("0"), 0);
}

TEST_F(CsvToNumberTest, ConvertToLong)
{
  EXPECT_EQ(csv_to_number<long>("1000000000"), 1000000000L);
  EXPECT_EQ(csv_to_number<long>("-999999999"), -999999999L);
}

TEST_F(CsvToNumberTest, ConvertToDouble)
{
  EXPECT_DOUBLE_EQ(csv_to_number<double>("3.14159"), 3.14159);
  EXPECT_DOUBLE_EQ(csv_to_number<double>("-2.5"), -2.5);
  EXPECT_DOUBLE_EQ(csv_to_number<double>("1e10"), 1e10);
}

TEST_F(CsvToNumberTest, ConvertToFloat)
{
  EXPECT_FLOAT_EQ(csv_to_number<float>("3.14f"), 3.14f);
  EXPECT_FLOAT_EQ(csv_to_number<float>("-1.5"), -1.5f);
}

TEST_F(CsvToNumberTest, InvalidIntConversion)
{
  EXPECT_THROW((void)csv_to_number<int>("not_a_number"), std::invalid_argument);
  EXPECT_THROW((void)csv_to_number<int>(""), std::invalid_argument);
}

TEST_F(CsvToNumberTest, InvalidDoubleConversion)
{
  EXPECT_THROW((void)csv_to_number<double>("abc"), std::invalid_argument);
}

TEST_F(CsvToNumberTest, ConvertWithWhitespace)
{
  // std::stoi handles leading whitespace
  EXPECT_EQ(csv_to_number<int>("  42"), 42);
}

//============================================================================
// Round-trip Tests
//============================================================================

class CsvRoundTripTest : public ::testing::Test {};

TEST_F(CsvRoundTripTest, SimpleDataRoundTrip)
{
  // Original data
  std::istringstream input("name,value\ntest,100\n");
  Array<Array<std::string>> original = csv_read_all(input);
  
  // Write to string
  std::ostringstream output;
  csv_write_all(output, original);
  
  // Parse back
  std::istringstream input2(output.str());
  Array<Array<std::string>> parsed = csv_read_all(input2);
  
  // Compare
  ASSERT_EQ(parsed.size(), original.size());
  for (size_t i = 0; i < original.size(); ++i)
  {
    ASSERT_EQ(parsed(i).size(), original(i).size());
    for (size_t j = 0; j < original(i).size(); ++j)
      EXPECT_EQ(parsed(i)(j), original(i)(j));
  }
}

TEST_F(CsvRoundTripTest, ComplexDataRoundTrip)
{
  // Create data with special characters
  Array<Array<std::string>> original;
  
  Array<std::string> row1;
  row1.append("simple");
  row1.append("with, comma");
  row1.append("with \"quotes\"");
  row1.append("with\nnewline");
  original.append(row1);
  
  Array<std::string> row2;
  row2.append("\"\"\"");  // Just quotes
  row2.append(",,,");    // Just commas
  row2.append("\n\r\n"); // Newlines
  row2.append("");       // Empty
  original.append(row2);
  
  // Round trip
  std::ostringstream output;
  csv_write_all(output, original);
  
  std::istringstream input(output.str());
  Array<Array<std::string>> parsed = csv_read_all(input);
  
  // Verify
  ASSERT_EQ(parsed.size(), original.size());
  for (size_t i = 0; i < original.size(); ++i)
  {
    ASSERT_EQ(parsed(i).size(), original(i).size()) 
      << "Row " << i << " size mismatch";
    for (size_t j = 0; j < original(i).size(); ++j)
      EXPECT_EQ(parsed(i)(j), original(i)(j))
        << "Mismatch at row " << i << ", col " << j;
  }
}

//============================================================================
// Edge Cases Tests
//============================================================================

class CsvEdgeCasesTest : public ::testing::Test {};

TEST_F(CsvEdgeCasesTest, OnlyNewlines)
{
  std::istringstream ss("\n\n\n");
  Array<Array<std::string>> data = csv_read_all(ss);
  
  // Each newline produces an empty row with one empty field
  EXPECT_GE(data.size(), 1u);
}

TEST_F(CsvEdgeCasesTest, OnlyDelimiters)
{
  std::string line = ",,,";
  Array<std::string> row = csv_read_row(line);
  
  // Four fields (three delimiters = four empty fields)
  ASSERT_EQ(row.size(), 4u);
  for (size_t i = 0; i < row.size(); ++i)
    EXPECT_EQ(row(i), "");
}

TEST_F(CsvEdgeCasesTest, EmptyQuotedField)
{
  std::string line = "a,\"\",b";
  Array<std::string> row = csv_read_row(line);
  
  ASSERT_EQ(row.size(), 3u);
  EXPECT_EQ(row(0), "a");
  EXPECT_EQ(row(1), "");
  EXPECT_EQ(row(2), "b");
}

TEST_F(CsvEdgeCasesTest, QuotedFieldWithOnlyQuotes)
{
  std::string line = "\"\"\"\"";  // Field containing single quote
  Array<std::string> row = csv_read_row(line);
  
  ASSERT_EQ(row.size(), 1u);
  EXPECT_EQ(row(0), "\"");
}

TEST_F(CsvEdgeCasesTest, VeryLongField)
{
  std::string long_string(10000, 'x');
  std::string line = "a," + long_string + ",b";
  Array<std::string> row = csv_read_row(line);
  
  ASSERT_EQ(row.size(), 3u);
  EXPECT_EQ(row(1), long_string);
}

TEST_F(CsvEdgeCasesTest, ManyColumns)
{
  std::string line;
  for (int i = 0; i < 100; ++i)
  {
    if (i > 0) line += ",";
    line += std::to_string(i);
  }
  
  Array<std::string> row = csv_read_row(line);
  
  ASSERT_EQ(row.size(), 100u);
  EXPECT_EQ(row(0), "0");
  EXPECT_EQ(row(99), "99");
}

TEST_F(CsvEdgeCasesTest, UnicodeContent)
{
  std::string line = "日本語,中文,한국어";
  Array<std::string> row = csv_read_row(line);
  
  ASSERT_EQ(row.size(), 3u);
  EXPECT_EQ(row(0), "日本語");
  EXPECT_EQ(row(1), "中文");
  EXPECT_EQ(row(2), "한국어");
}

//============================================================================
// Integration Tests
//============================================================================

TEST(CsvIntegrationTest, TypicalWorkflow)
{
  // 1. Parse CSV header and data
  std::istringstream input(
    "id,name,price,quantity\n"
    "1,Widget,9.99,100\n"
    "2,\"Gadget, Pro\",19.99,50\n"
    "3,\"Thing \"\"Deluxe\"\"\",29.99,25\n"
  );
  
  Array<Array<std::string>> data = csv_read_all(input);
  
  // 2. Verify structure
  ASSERT_EQ(data.size(), 4u);
  EXPECT_TRUE(csv_is_rectangular(data));
  
  // 3. Access header
  EXPECT_EQ(data(0)(0), "id");
  EXPECT_EQ(data(0)(1), "name");
  EXPECT_EQ(data(0)(2), "price");
  EXPECT_EQ(data(0)(3), "quantity");
  
  // 4. Parse numeric data
  EXPECT_EQ(csv_to_number<int>(data(1)(0)), 1);
  EXPECT_DOUBLE_EQ(csv_to_number<double>(data(1)(2)), 9.99);
  EXPECT_EQ(csv_to_number<int>(data(1)(3)), 100);
  
  // 5. Handle special characters
  EXPECT_EQ(data(2)(1), "Gadget, Pro");
  EXPECT_EQ(data(3)(1), "Thing \"Deluxe\"");
  
  // 6. Extract a column
  Array<std::string> names = csv_get_column(data, 1);
  EXPECT_EQ(names(1), "Widget");
  EXPECT_EQ(names(2), "Gadget, Pro");
}

//============================================================================
// Type Traits Tests
//============================================================================

TEST(CsvTypeTraits, NoDiscardAttribute)
{
  // Verify [[nodiscard]] attribute is respected
  EXPECT_TRUE((std::is_same<decltype(csv_num_columns(std::declval<Array<std::string>&>())), size_t>::value));
  EXPECT_TRUE((std::is_same<decltype(csv_is_rectangular(std::declval<Array<Array<std::string>>&>())), bool>::value));
}

//============================================================================
// CsvRow Tests
//============================================================================

class CsvRowTest : public ::testing::Test {};

TEST_F(CsvRowTest, IndexAccess)
{
  Array<std::string> fields;
  fields.append("a");
  fields.append("b");
  fields.append("c");
  
  CsvRow row(fields);
  
  EXPECT_EQ(row[0], "a");
  EXPECT_EQ(row[1], "b");
  EXPECT_EQ(row[2], "c");
}

TEST_F(CsvRowTest, HeaderAccess)
{
  Array<std::string> header;
  header.append("name");
  header.append("age");
  header.append("city");
  
  Array<std::string> fields;
  fields.append("Alice");
  fields.append("30");
  fields.append("NYC");
  
  CsvRow row(fields, header);
  
  EXPECT_EQ(row["name"], "Alice");
  EXPECT_EQ(row["age"], "30");
  EXPECT_EQ(row["city"], "NYC");
}

TEST_F(CsvRowTest, HeaderAccessThrowsWithoutHeader)
{
  Array<std::string> fields;
  fields.append("value");
  
  CsvRow row(fields);
  
  EXPECT_THROW((void)row["column"], std::runtime_error);
}

TEST_F(CsvRowTest, HeaderAccessThrowsForUnknownColumn)
{
  Array<std::string> header;
  header.append("name");
  
  Array<std::string> fields;
  fields.append("Alice");
  
  CsvRow row(fields, header);
  
  EXPECT_THROW((void)row["unknown"], std::out_of_range);
}

TEST_F(CsvRowTest, HasColumn)
{
  Array<std::string> header;
  header.append("name");
  header.append("age");
  
  Array<std::string> fields;
  fields.append("Alice");
  fields.append("30");
  
  CsvRow row(fields, header);
  
  EXPECT_TRUE(row.has_column("name"));
  EXPECT_TRUE(row.has_column("age"));
  EXPECT_FALSE(row.has_column("unknown"));
}

TEST_F(CsvRowTest, GetNumeric)
{
  Array<std::string> header;
  header.append("name");
  header.append("value");
  
  Array<std::string> fields;
  fields.append("test");
  fields.append("42");
  
  CsvRow row(fields, header);
  
  EXPECT_EQ(row.get<int>(1), 42);
  EXPECT_EQ(row.get<int>("value"), 42);
}

TEST_F(CsvRowTest, SizeAndEmpty)
{
  Array<std::string> fields;
  CsvRow empty_row(fields);
  EXPECT_TRUE(empty_row.empty());
  EXPECT_EQ(empty_row.size(), 0u);
  
  fields.append("a");
  CsvRow non_empty(fields);
  EXPECT_FALSE(non_empty.empty());
  EXPECT_EQ(non_empty.size(), 1u);
}

//============================================================================
// CsvReader Tests
//============================================================================

class CsvReaderTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    test_filename = "/tmp/aleph_csv_reader_test_" + std::to_string(rand()) + ".csv";
    
    // Create test file
    std::ofstream file(test_filename);
    file << "name,age,city\n";
    file << "Alice,30,NYC\n";
    file << "Bob,25,LA\n";
    file << "Charlie,35,Chicago\n";
    file.close();
  }
  
  void TearDown() override
  {
    std::remove(test_filename.c_str());
  }
  
  std::string test_filename;
};

TEST_F(CsvReaderTest, ReadWithHeader)
{
  CsvReader reader(test_filename);
  reader.read_header();
  
  EXPECT_TRUE(reader.has_header());
  EXPECT_EQ(reader.header().size(), 3u);
  EXPECT_EQ(reader.header()(0), "name");
}

TEST_F(CsvReaderTest, ReadRows)
{
  CsvReader reader(test_filename);
  reader.read_header();
  
  EXPECT_TRUE(reader.has_next());
  
  CsvRow row1 = reader.next_row();
  EXPECT_EQ(row1["name"], "Alice");
  EXPECT_EQ(row1["age"], "30");
  
  CsvRow row2 = reader.next_row();
  EXPECT_EQ(row2["name"], "Bob");
  
  CsvRow row3 = reader.next_row();
  EXPECT_EQ(row3["name"], "Charlie");
  
  EXPECT_FALSE(reader.has_next());
}

TEST_F(CsvReaderTest, Skip)
{
  CsvReader reader(test_filename);
  reader.read_header();
  reader.skip(1);  // Skip Alice
  
  CsvRow row = reader.next_row();
  EXPECT_EQ(row["name"], "Bob");
}

TEST_F(CsvReaderTest, RowsRead)
{
  CsvReader reader(test_filename);
  reader.read_header();
  
  EXPECT_EQ(reader.rows_read(), 0u);
  reader.next();
  EXPECT_EQ(reader.rows_read(), 1u);
  reader.next();
  EXPECT_EQ(reader.rows_read(), 2u);
}

TEST_F(CsvReaderTest, RangeBasedFor)
{
  CsvReader reader(test_filename);
  reader.read_header();
  
  size_t count = 0;
  for (const auto & row : reader)
  {
    (void)row;
    ++count;
  }
  
  EXPECT_EQ(count, 3u);
}

TEST_F(CsvReaderTest, FromStream)
{
  std::istringstream ss("a,b\n1,2\n");
  CsvReader reader(ss);
  
  Array<std::string> row1 = reader.next();
  EXPECT_EQ(row1(0), "a");
  
  Array<std::string> row2 = reader.next();
  EXPECT_EQ(row2(0), "1");
}

//============================================================================
// Filter and Selection Tests
//============================================================================

class CsvFilterTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    std::istringstream ss("name,status\nAlice,active\nBob,inactive\nCharlie,active\n");
    data = csv_read_all(ss);
  }
  
  Array<Array<std::string>> data;
};

TEST_F(CsvFilterTest, FilterByPredicate)
{
  auto result = csv_filter(data, [](const Array<std::string> & row) {
    return row.size() > 1 and row(1) == "active";
  });
  
  EXPECT_EQ(result.size(), 2u);  // Alice and Charlie
}

TEST_F(CsvFilterTest, FilterByValue)
{
  auto result = csv_filter_by_value(data, 1, "active");
  
  EXPECT_EQ(result.size(), 2u);
}

TEST_F(CsvFilterTest, SelectColumns)
{
  Array<size_t> cols;
  cols.append(0);  // name only
  
  auto result = csv_select_columns(data, cols);
  
  EXPECT_EQ(result.size(), data.size());
  EXPECT_EQ(result(0).size(), 1u);
  EXPECT_EQ(result(0)(0), "name");
}

TEST_F(CsvFilterTest, SkipRows)
{
  auto result = csv_skip_rows(data, 1);  // Skip header
  
  EXPECT_EQ(result.size(), 3u);
  EXPECT_EQ(result(0)(0), "Alice");
}

TEST_F(CsvFilterTest, TakeRows)
{
  auto result = csv_take_rows(data, 2);  // Header + Alice
  
  EXPECT_EQ(result.size(), 2u);
  EXPECT_EQ(result(1)(0), "Alice");
}

//============================================================================
// Statistics Tests
//============================================================================

class CsvStatisticsTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    std::istringstream ss("a,b,c\n1,,3\n4,5,\n7,8,9\n");
    data = csv_read_all(ss);
  }
  
  Array<Array<std::string>> data;
};

TEST_F(CsvStatisticsTest, CountRows)
{
  EXPECT_EQ(csv_count_rows(data), 4u);
}

TEST_F(CsvStatisticsTest, CountEmpty)
{
  EXPECT_EQ(csv_count_empty(data), 2u);  // Two empty fields
}

TEST_F(CsvStatisticsTest, CountIf)
{
  auto count = csv_count_if(data, [](const Array<std::string> & row) {
    return row.size() >= 3 and row(2) == "";
  });
  
  EXPECT_EQ(count, 1u);  // Row "4,5,"
}

TEST_F(CsvStatisticsTest, FindRow)
{
  auto idx = csv_find_row(data, [](const Array<std::string> & row) {
    return row.size() > 0 and row(0) == "4";
  });
  
  EXPECT_EQ(idx, 2u);
}

TEST_F(CsvStatisticsTest, FindByValue)
{
  auto idx = csv_find_by_value(data, 0, "7");
  EXPECT_EQ(idx, 3u);
}

TEST_F(CsvStatisticsTest, FindNotFound)
{
  auto idx = csv_find_by_value(data, 0, "xyz");
  EXPECT_EQ(idx, data.size());  // Not found
}

TEST_F(CsvStatisticsTest, Distinct)
{
  std::istringstream ss("cat\ndog\ncat\nbird\ndog\n");
  auto animals = csv_read_all(ss);
  
  auto unique = csv_distinct(animals, 0);
  EXPECT_EQ(unique.size(), 3u);  // cat, dog, bird
}

//============================================================================
// Transformation Tests
//============================================================================

class CsvTransformationTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    std::istringstream ss("a,b,c\n1,2,3\n4,5,6\n");
    data = csv_read_all(ss);
  }
  
  Array<Array<std::string>> data;
};

TEST_F(CsvTransformationTest, Transpose)
{
  auto result = csv_transpose(data);
  
  EXPECT_EQ(result.size(), 3u);   // 3 columns become 3 rows
  EXPECT_EQ(result(0).size(), 3u);  // 3 rows become 3 columns
  EXPECT_EQ(result(0)(0), "a");
  EXPECT_EQ(result(0)(1), "1");
  EXPECT_EQ(result(0)(2), "4");
}

TEST_F(CsvTransformationTest, SortByColumn)
{
  std::istringstream ss("charlie,3\nalice,1\nbob,2\n");
  auto unsorted = csv_read_all(ss);
  
  auto sorted = csv_sort_by_column(unsorted, 0);  // Sort by name
  
  EXPECT_EQ(sorted(0)(0), "alice");
  EXPECT_EQ(sorted(1)(0), "bob");
  EXPECT_EQ(sorted(2)(0), "charlie");
}

TEST_F(CsvTransformationTest, SortByColumnNumeric)
{
  std::istringstream ss("charlie,30\nalice,10\nbob,20\n");
  auto unsorted = csv_read_all(ss);
  
  auto sorted = csv_sort_by_column_numeric<int>(unsorted, 1);  // Sort by value
  
  EXPECT_EQ(sorted(0)(0), "alice");   // 10
  EXPECT_EQ(sorted(1)(0), "bob");     // 20
  EXPECT_EQ(sorted(2)(0), "charlie"); // 30
}

TEST_F(CsvTransformationTest, Unique)
{
  std::istringstream ss("a,b\n1,2\n3,4\n1,2\n5,6\n1,2\n");
  auto with_dupes = csv_read_all(ss);
  
  auto unique = csv_unique(with_dupes);
  
  EXPECT_EQ(unique.size(), 4u);  // header + 3 unique data rows
}

TEST_F(CsvTransformationTest, Transform)
{
  auto upper = csv_transform(data, [](const std::string & s) {
    std::string result = s;
    for (char & c : result)
      c = std::toupper(static_cast<unsigned char>(c));
    return result;
  });
  
  EXPECT_EQ(upper(0)(0), "A");
  EXPECT_EQ(upper(0)(1), "B");
}

TEST_F(CsvTransformationTest, AddColumn)
{
  auto with_sum = csv_add_column(data, [](const Array<std::string> & row) {
    if (row.size() < 3) return std::string("N/A");
    try {
      int sum = std::stoi(row(0)) + std::stoi(row(1)) + std::stoi(row(2));
      return std::to_string(sum);
    } catch (...) {
      return std::string("N/A");
    }
  });
  
  EXPECT_EQ(with_sum(0).size(), 4u);  // Original 3 + new column
  EXPECT_EQ(with_sum(1)(3), "6");     // 1+2+3
  EXPECT_EQ(with_sum(2)(3), "15");    // 4+5+6
}

TEST_F(CsvTransformationTest, RenameColumn)
{
  auto renamed = csv_rename_column(data, "a", "first_col");
  
  EXPECT_EQ(renamed(0)(0), "first_col");
  EXPECT_EQ(renamed(0)(1), "b");  // Unchanged
}

//============================================================================
// Utility Tests
//============================================================================

class CsvUtilityTest : public ::testing::Test {};

TEST_F(CsvUtilityTest, SkipBom)
{
  // Create stream with UTF-8 BOM (3 bytes: EF BB BF)
  std::string bom_str;
  bom_str += '\xEF';
  bom_str += '\xBB';
  bom_str += '\xBF';
  bom_str += "a,b,c\n";
  std::istringstream ss(bom_str);
  
  EXPECT_TRUE(csv_skip_bom(ss));
  
  auto row = csv_read_row(ss);
  EXPECT_EQ(row(0), "a");
}

TEST_F(CsvUtilityTest, SkipBomNotPresent)
{
  std::istringstream ss("a,b,c\n");
  
  EXPECT_FALSE(csv_skip_bom(ss));
  
  auto row = csv_read_row(ss);
  EXPECT_EQ(row(0), "a");  // Still reads correctly
}

TEST_F(CsvUtilityTest, TrimFields)
{
  std::istringstream ss("  a  ,  b  ,  c  \n");
  auto data = csv_read_all(ss);
  
  auto trimmed = csv_trim_fields(data);
  
  EXPECT_EQ(trimmed(0)(0), "a");
  EXPECT_EQ(trimmed(0)(1), "b");
  EXPECT_EQ(trimmed(0)(2), "c");
}

TEST_F(CsvUtilityTest, FillEmpty)
{
  std::istringstream ss("a,,c\n");
  auto data = csv_read_all(ss);
  
  auto filled = csv_fill_empty(data, "N/A");
  
  EXPECT_EQ(filled(0)(0), "a");
  EXPECT_EQ(filled(0)(1), "N/A");
  EXPECT_EQ(filled(0)(2), "c");
}

//============================================================================
// Join Tests
//============================================================================

class CsvJoinTest : public ::testing::Test {};

TEST_F(CsvJoinTest, JoinHorizontal)
{
  std::istringstream ss1("a,b\n1,2\n");
  std::istringstream ss2("c,d\n3,4\n");
  
  auto left = csv_read_all(ss1);
  auto right = csv_read_all(ss2);
  
  auto joined = csv_join_horizontal(left, right);
  
  EXPECT_EQ(joined.size(), 2u);
  EXPECT_EQ(joined(0).size(), 4u);
  EXPECT_EQ(joined(0)(0), "a");
  EXPECT_EQ(joined(0)(2), "c");
}

TEST_F(CsvJoinTest, JoinVertical)
{
  std::istringstream ss1("a,b\n1,2\n");
  std::istringstream ss2("3,4\n5,6\n");
  
  auto top = csv_read_all(ss1);
  auto bottom = csv_read_all(ss2);
  
  auto joined = csv_join_vertical(top, bottom);
  
  EXPECT_EQ(joined.size(), 4u);
  EXPECT_EQ(joined(2)(0), "3");
}

TEST_F(CsvJoinTest, InnerJoin)
{
  std::istringstream ss1("id,name\n1,Alice\n2,Bob\n3,Charlie\n");
  std::istringstream ss2("id,dept\n1,Sales\n3,Engineering\n4,HR\n");
  
  auto left = csv_read_all(ss1);
  auto right = csv_read_all(ss2);
  
  // Skip headers for join
  auto left_data = csv_skip_rows(left, 1);
  auto right_data = csv_skip_rows(right, 1);
  
  auto joined = csv_inner_join(left_data, 0, right_data, 0);
  
  EXPECT_EQ(joined.size(), 2u);  // Only 1 and 3 match
  EXPECT_EQ(joined(0)(1), "Alice");
  EXPECT_EQ(joined(0)(2), "Sales");
}

TEST_F(CsvJoinTest, GroupBy)
{
  std::istringstream ss("dept,name\nSales,Alice\nEng,Bob\nSales,Charlie\nEng,Dave\n");
  auto data = csv_read_all(ss);
  auto data_rows = csv_skip_rows(data, 1);
  
  auto groups = csv_group_by(data_rows, 0);
  
  EXPECT_EQ(groups.size(), 2u);  // Sales and Eng
  
  // Each group should have 2 members
  EXPECT_EQ(groups(0).size(), 2u);
  EXPECT_EQ(groups(1).size(), 2u);
}

//============================================================================
// Main
//============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
