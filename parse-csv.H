
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


/** @file parse-csv.H
 *  @brief Comprehensive CSV (Comma-Separated Values) parsing and manipulation utilities.
 *
 *  This file provides a complete toolkit for parsing, generating, and manipulating
 *  CSV data following RFC 4180 conventions:
 *
 *  - Fields are separated by a delimiter (default: comma).
 *  - Fields containing delimiters, quotes, or newlines are enclosed in double quotes.
 *  - Double quotes inside quoted fields are escaped by doubling them ("").
 *  - Lines end with CRLF or LF.
 *
 *  ## Features
 *
 *  ### Reading & Writing
 *  - `csv_read_row()` - Parse single row from stream or string
 *  - `csv_read_all()` - Read all rows into memory
 *  - `csv_read_file()` - Read from file
 *  - `csv_write_row()` / `csv_write_all()` / `csv_write_file()` - Write CSV output
 *  - `CsvReader` - Iterator-based lazy reading for large files
 *
 *  ### Header-Based Access
 *  - `CsvRow` - Access fields by column name: `row["name"]`
 *
 *  ### Filtering & Selection
 *  - `csv_filter()` - Filter rows by predicate
 *  - `csv_filter_by_value()` - Filter by column value
 *  - `csv_select_columns()` - Select specific columns
 *  - `csv_skip_rows()` / `csv_take_rows()` - Slice rows
 *
 *  ### Statistics & Search
 *  - `csv_count_rows()` / `csv_count_empty()` / `csv_count_if()`
 *  - `csv_find_row()` / `csv_find_by_value()`
 *  - `csv_distinct()` - Get unique values in column
 *
 *  ### Transformations
 *  - `csv_transpose()` - Swap rows and columns
 *  - `csv_sort_by_column()` / `csv_sort_by_column_numeric()`
 *  - `csv_unique()` - Remove duplicates
 *  - `csv_transform()` - Apply function to all fields
 *  - `csv_add_column()` - Add computed column
 *  - `csv_rename_column()` - Rename header column
 *
 *  ### Join & Combine
 *  - `csv_join_horizontal()` - Add columns side by side
 *  - `csv_join_vertical()` - Concatenate rows
 *  - `csv_inner_join()` - SQL-like join on key column
 *  - `csv_group_by()` - Group rows by column value
 *
 *  ### Utilities
 *  - `csv_skip_bom()` - Handle UTF-8 BOM
 *  - `csv_trim_fields()` - Remove whitespace
 *  - `csv_fill_empty()` - Replace empty fields
 *  - `csv_escape()` - Escape field for output
 *  - `csv_to_number<T>()` - Parse numeric fields
 *
 *  ## Usage Examples
 *
 *  @code{.cpp}
 *  #include <parse-csv.H>
 *  
 *  // Simple reading
 *  auto data = csv_read_file("data.csv");
 *  
 *  // Lazy reading for large files
 *  CsvReader reader("large.csv");
 *  reader.read_header();
 *  while (reader.has_next())
 *  {
 *    CsvRow row = reader.next_row();
 *    std::cout << row["name"] << ": " << row.get<int>("value") << "\n";
 *  }
 *  
 *  // Filter and transform
 *  auto active = csv_filter_by_value(data, 2, "active");
 *  auto sorted = csv_sort_by_column_numeric<int>(active, 1);
 *  csv_write_file("output.csv", sorted);
 *  @endcode
 *
 *  ## Thread Safety
 *
 *  All functions are thread-safe as long as different threads operate on
 *  different streams. Concurrent access to the same stream requires external
 *  synchronization.
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

#ifndef PARSE_CSV_H
#define PARSE_CSV_H

#include <sstream>
#include <istream>
#include <ostream>
#include <fstream>
#include <string>
#include <tpl_array.H>

namespace Aleph
{

/** @brief Read a single CSV row from an input stream.
 *
 *  Parses one row of CSV data from the given input stream. The function
 *  handles:
 *
 *  - Quoted fields (enclosed in double quotes).
 *  - Escaped quotes (double double-quotes: "" becomes ").
 *  - Fields containing the delimiter character.
 *  - Both CRLF and LF line endings.
 *
 *  @param in The input stream to read from.
 *  @param delimiter The field delimiter character (default: comma).
 *  @return An Array of strings, one for each field in the row.
 *          Returns an empty array if the stream is at EOF.
 *
 *  @note The function advances the stream position past the end of the row
 *        (including the newline character).
 *
 *  Example: reading two CSV rows from a stream.
 *
 *  @see csv_read_row(std::string &, char)
 *  @see csv_read_all()
 *  @ingroup Utilities
 */
inline Array<std::string> csv_read_row(std::istream & in, char delimiter = ',')
{
  std::stringstream ss;
  bool inquotes = false;
  Array<std::string> row;  // Relying on RVO
  
  while (in.good())
    {
      int ch = in.get();
      
      // Check for EOF
      if (ch == std::char_traits<char>::eof())
        break;
      
      char c = static_cast<char>(ch);
      
      if (not inquotes and c == '"')  // Begin quoted field
        inquotes = true;
      else if (inquotes and c == '"')  // Quote character inside quoted field
        {
          if (in.peek() == '"')  // Two consecutive quotes resolve to one
            ss << static_cast<char>(in.get());
          else  // End of quoted field
            inquotes = false;
        }
      else if (not inquotes and c == delimiter)  // End of field
        {
          row.append(ss.str());
          ss.str("");
        }
      else if (not inquotes and (c == '\r' or c == '\n'))  // End of row
        {
          if (in.peek() == '\n')  // Handle CRLF
            in.get();
          row.append(ss.str());
          return row;
        }
      else
        ss << c;
    }
  
  // Handle last field if stream ended without newline
  if (ss.str().size() > 0 or row.size() > 0)
    row.append(ss.str());
    
  return row;
}

/** @brief Read a single CSV row from a string.
 *
 *  Convenience overload that parses a CSV row from a string instead of
 *  a stream.
 *
 *  @param line The string containing the CSV row.
 *  @param delimiter The field delimiter character.
 *  @return An Array of strings, one for each field in the row.
 *
 *  @code{.cpp}
 *  std::string line = "apple,banana,\"cherry, fresh\"";
 *  Array<std::string> fields = csv_read_row(line, ',');
 *  // fields = ["apple", "banana", "cherry, fresh"]
 *  @endcode
 *
 *  @see csv_read_row(std::istream &, char)
 *  @ingroup Utilities
 */
inline Array<std::string> csv_read_row(const std::string & line, 
                                       char delimiter = ',')
{
  std::stringstream ss(line);
  return csv_read_row(ss, delimiter);
}

/** @brief Read all rows from a CSV input stream.
 *
 *  Reads all remaining rows from the input stream and returns them as
 *  an array of arrays.
 *
 *  @param in The input stream to read from.
 *  @param delimiter The field delimiter character (default: comma).
 *  @return An Array of row Arrays, where each inner Array contains
 *          the fields of one row.
 *
 *  @code{.cpp}
 *  std::istringstream ss("a,b,c\n1,2,3\n4,5,6");
 *  Array<Array<std::string>> data = csv_read_all(ss);
 *  // data.size() == 3
 *  // data(0) = ["a", "b", "c"]
 *  // data(1) = ["1", "2", "3"]
 *  // data(2) = ["4", "5", "6"]
 *  @endcode
 *
 *  @see csv_read_row()
 *  @ingroup Utilities
 */
inline Array<Array<std::string>> csv_read_all(std::istream & in, 
                                              char delimiter = ',')
{
  Array<Array<std::string>> result;
  
  while (in.good())
    {
      Array<std::string> row = csv_read_row(in, delimiter);
      if (row.size() == 0 and not in.good())
        break;
      result.append(std::move(row));
    }
  
  return result;
}

/** @brief Read all rows from a CSV file.
 *
 *  Opens the specified file and reads all CSV rows from it.
 *
 *  @param filename Path to the CSV file.
 *  @param delimiter The field delimiter character (default: comma).
 *  @return An Array of row Arrays.
 *  @throws std::runtime_error If the file cannot be opened.
 *
 *  @code{.cpp}
 *  Array<Array<std::string>> data = csv_read_file("data.csv");
 *  for (size_t i = 0; i < data.size(); ++i)
 *  {
 *    const Array<std::string> & row = data(i);
 *    // Process row...
 *  }
 *  @endcode
 *
 *  @see csv_read_all()
 *  @ingroup Utilities
 */
inline Array<Array<std::string>> csv_read_file(const std::string & filename,
                                               char delimiter = ',')
{
  std::ifstream file(filename);
  if (not file.is_open())
    throw std::runtime_error("Cannot open file: " + filename);
  
  return csv_read_all(file, delimiter);
}

/** @brief Escape a string for CSV output.
 *
 *  If the input string contains the delimiter, double quotes, or newlines,
 *  the string is enclosed in double quotes and internal quotes are escaped
 *  by doubling them.
 *
 *  @param field The string to escape.
 *  @param delimiter The field delimiter character (default: comma).
 *  @return The escaped string, ready for CSV output.
 *
 *  @code{.cpp}
 *  csv_escape("hello")           // "hello" (no change)
 *  csv_escape("hello, world")    // "\"hello, world\"" (quoted due to comma)
 *  csv_escape("say \"hi\"")      // "\"say \"\"hi\"\"\""  (quotes escaped)
 *  csv_escape("line1\nline2")    // "\"line1\nline2\"" (quoted due to newline)
 *  @endcode
 *
 *  @see csv_write_row()
 *  @ingroup Utilities
 */
inline std::string csv_escape(const std::string & field, char delimiter = ',')
{
  bool needs_quoting = false;
  
  // Check if quoting is needed
  for (char c : field)
    {
      if (c == delimiter or c == '"' or c == '\n' or c == '\r')
        {
          needs_quoting = true;
          break;
        }
    }
  
  if (not needs_quoting)
    return field;
  
  // Escape by quoting and doubling internal quotes
  std::string result = "\"";
  for (char c : field)
    {
      if (c == '"')
        result += "\"\"";
      else
        result += c;
    }
  result += "\"";
  
  return result;
}

/** @brief Write a CSV row to an output stream.
 *
 *  Writes a row of fields to the output stream, properly escaping fields
 *  that need quoting.
 *
 *  @param out The output stream to write to.
 *  @param row An Array of strings representing the fields.
 *  @param delimiter The field delimiter character (default: comma).
 *  @param line_ending The line ending string (default: "\n").
 *
 *  @code{.cpp}
 *  Array<std::string> row;
 *  row.append("Name");
 *  row.append("Description");
 *  row.append("Price");
 *  
 *  std::ostringstream out;
 *  csv_write_row(out, row);
 *  // Output: "Name,Description,Price\n"
 *  @endcode
 *
 *  @see csv_escape()
 *  @see csv_write_all()
 *  @ingroup Utilities
 */
inline void csv_write_row(std::ostream & out, 
                          const Array<std::string> & row,
                          char delimiter = ',',
                          const std::string & line_ending = "\n")
{
  for (size_t i = 0; i < row.size(); ++i)
    {
      if (i > 0)
        out << delimiter;
      out << csv_escape(row(i), delimiter);
    }
  out << line_ending;
}

/** @brief Write multiple CSV rows to an output stream.
 *
 *  Writes all given rows to the output stream.
 *
 *  @param out The output stream to write to.
 *  @param rows An Array of row Arrays.
 *  @param delimiter The field delimiter character (default: comma).
 *  @param line_ending The line ending string (default: "\n").
 *
 *  @see csv_write_row()
 *  @ingroup Utilities
 */
inline void csv_write_all(std::ostream & out,
                          const Array<Array<std::string>> & rows,
                          char delimiter = ',',
                          const std::string & line_ending = "\n")
{
  for (size_t i = 0; i < rows.size(); ++i)
    csv_write_row(out, rows(i), delimiter, line_ending);
}

/** @brief Write CSV data to a file.
 *
 *  Opens the specified file and writes all rows to it.
 *
 *  @param filename Path to the output file.
 *  @param rows An Array of row Arrays.
 *  @param delimiter The field delimiter character (default: comma).
 *  @param line_ending The line ending string (default: "\n").
 *  @throws std::runtime_error If the file cannot be opened.
 *
 *  @see csv_write_all()
 *  @ingroup Utilities
 */
inline void csv_write_file(const std::string & filename,
                           const Array<Array<std::string>> & rows,
                           char delimiter = ',',
                           const std::string & line_ending = "\n")
{
  std::ofstream file(filename);
  if (not file.is_open())
    throw std::runtime_error("Cannot open file for writing: " + filename);
  
  csv_write_all(file, rows, delimiter, line_ending);
}

/** @brief Get the number of columns in a CSV row.
 *
 *  Returns the number of fields in the given row.
 *
 *  @param row A CSV row.
 *  @return Number of fields.
 *  @ingroup Utilities
 */
[[nodiscard]] inline size_t csv_num_columns(const Array<std::string> & row)
{
  return row.size();
}

/** @brief Check if all rows have the same number of columns.
 *
 *  Validates that a CSV dataset is rectangular (all rows have the same
 *  number of columns).
 *
 *  @param rows An Array of row Arrays.
 *  @return True if all rows have the same number of columns, or if
 *          the array is empty.
 *
 *  @code{.cpp}
 *  Array<Array<std::string>> data = csv_read_file("data.csv");
 *  if (not csv_is_rectangular(data))
 *    std::cerr << "Warning: CSV has inconsistent row lengths\n";
 *  @endcode
 *
 *  @ingroup Utilities
 */
[[nodiscard]] inline bool csv_is_rectangular(const Array<Array<std::string>> & rows)
{
  if (rows.size() == 0)
    return true;
  
  size_t expected_cols = rows(0).size();
  for (size_t i = 1; i < rows.size(); ++i)
    if (rows(i).size() != expected_cols)
      return false;
  
  return true;
}

/** @brief Get a column from CSV data.
 *
 *  Extracts all values from a specific column across all rows.
 *
 *  @param rows An Array of row Arrays.
 *  @param col_index The column index (0-based).
 *  @return An Array containing the values of the specified column.
 *  @throws std::out_of_range If col_index is out of bounds for any row.
 *
 *  @code{.cpp}
 *  // CSV: "name,age\nAlice,30\nBob,25"
 *  Array<Array<std::string>> data = csv_read_all(ss);
 *  Array<std::string> names = csv_get_column(data, 0);
 *  // names = ["name", "Alice", "Bob"]
 *  @endcode
 *
 *  @ingroup Utilities
 */
inline Array<std::string> csv_get_column(const Array<Array<std::string>> & rows,
                                         size_t col_index)
{
  Array<std::string> column;
  
  for (size_t i = 0; i < rows.size(); ++i)
    {
      if (col_index >= rows(i).size())
        throw std::out_of_range("Column index " + std::to_string(col_index) +
                                " out of range for row " + std::to_string(i));
      column.append(rows(i)(col_index));
    }
  
  return column;
}

/** @brief Convert a CSV field to a numeric type.
 *
 *  Parses a string field as a numeric value.
 *
 *  @tparam T The numeric type (int, long, double, etc.).
 *  @param field The string to convert.
 *  @return The parsed numeric value.
 *  @throws std::invalid_argument If the string cannot be parsed.
 *  @throws std::out_of_range If the value is out of range for the type.
 *
 *  @code{.cpp}
 *  std::string s = "42";
 *  int value = csv_to_number<int>(s);  // 42
 *  
 *  std::string d = "3.14159";
 *  double pi = csv_to_number<double>(d);  // 3.14159
 *  @endcode
 *
 *  @ingroup Utilities
 */
template <typename T>
[[nodiscard]] inline T csv_to_number(const std::string & field)
{
  std::istringstream ss(field);
  T value;
  ss >> value;
  
  if (ss.fail())
    throw std::invalid_argument("Cannot convert '" + field + "' to number");
  
  return value;
}

// Template specialization for int (uses std::stoi for better error handling)
template <>
[[nodiscard]] inline int csv_to_number<int>(const std::string & field)
{
  return std::stoi(field);
}

// Template specialization for long (uses std::stol for better error handling)
template <>
[[nodiscard]] inline long csv_to_number<long>(const std::string & field)
{
  return std::stol(field);
}

// Template specialization for double (uses std::stod for better error handling)
template <>
[[nodiscard]] inline double csv_to_number<double>(const std::string & field)
{
  return std::stod(field);
}

// Template specialization for float (uses std::stof for better error handling)
template <>
[[nodiscard]] inline float csv_to_number<float>(const std::string & field)
{
  return std::stof(field);
}

//============================================================================
// CsvRow - Row with header-based access
//============================================================================

/** @brief A CSV row with header-based field access.
 *
 *  CsvRow wraps an Array of strings and provides access to fields
 *  by column name in addition to index-based access.
 *
 *  @code{.cpp}
 *  // Parse CSV with header
 *  std::istringstream ss("name,age,city\nAlice,30,NYC\nBob,25,LA");
 *  Array<std::string> header_row = csv_read_row(ss);
 *  
 *  while (ss.good())
 *  {
 *    CsvRow row(csv_read_row(ss), header_row);
 *    if (row.size() == 0) break;
 *    
 *    std::cout << row["name"] << " is " << row["age"] << " years old\n";
 *    // Or use index: row[0], row[1], row[2]
 *  }
 *  @endcode
 *
 *  @ingroup Utilities
 */
class CsvRow
{
  Array<std::string> fields_;
  const Array<std::string> * header_;
  
public:
  /// @brief Construct from fields only (no header access).
  explicit CsvRow(Array<std::string> fields)
    : fields_(std::move(fields)), header_(nullptr) {}
  
  /// @brief Construct with header reference for name-based access.
  CsvRow(Array<std::string> fields, const Array<std::string> & header)
    : fields_(std::move(fields)), header_(&header) {}
  
  /// @brief Get field by index.
  [[nodiscard]] const std::string & operator[](size_t index) const
  {
    return fields_(index);
  }
  
  /// @brief Get field by column name.
  /// @throws std::runtime_error If no header is set.
  /// @throws std::out_of_range If column name not found.
  [[nodiscard]] const std::string & operator[](const std::string & column_name) const
  {
    if (header_ == nullptr)
      throw std::runtime_error("No header set for CsvRow");
    
    for (size_t i = 0; i < header_->size(); ++i)
      if ((*header_)(i) == column_name)
        return fields_(i);
    
    throw std::out_of_range("Column not found: " + column_name);
  }
  
  /// @brief Get field by index with bounds checking.
  [[nodiscard]] const std::string & at(size_t index) const
  {
    if (index >= fields_.size())
      throw std::out_of_range("Index out of range: " + std::to_string(index));
    return fields_(index);
  }
  
  /// @brief Check if column exists.
  [[nodiscard]] bool has_column(const std::string & column_name) const
  {
    if (header_ == nullptr) return false;
    for (size_t i = 0; i < header_->size(); ++i)
      if ((*header_)(i) == column_name)
        return i < fields_.size();
    return false;
  }
  
  /// @brief Get field as numeric type.
  template <typename T>
  [[nodiscard]] T get(size_t index) const
  {
    return csv_to_number<T>(fields_(index));
  }
  
  /// @brief Get field as numeric type by column name.
  template <typename T>
  [[nodiscard]] T get(const std::string & column_name) const
  {
    return csv_to_number<T>((*this)[column_name]);
  }
  
  /// @brief Get number of fields.
  [[nodiscard]] size_t size() const { return fields_.size(); }
  
  /// @brief Check if row is empty.
  [[nodiscard]] bool empty() const { return fields_.size() == 0; }
  
  /// @brief Get underlying array.
  [[nodiscard]] const Array<std::string> & fields() const { return fields_; }
  
  /// @brief Get column names (if header set).
  [[nodiscard]] const Array<std::string> * header() const { return header_; }
};

//============================================================================
// CsvReader - Iterator-based reader for large files
//============================================================================

/** @brief Lazy CSV reader for large files.
 *
 *  CsvReader provides an iterator-based interface for reading CSV files
 *  row by row without loading the entire file into memory.
 *
 *  @code{.cpp}
 *  // Read large CSV file efficiently
 *  CsvReader reader("large_file.csv");
 *  
 *  // Skip header and use it for column names
 *  reader.read_header();
 *  
 *  while (reader.has_next())
 *  {
 *    CsvRow row = reader.next_row();
 *    process(row["id"], row["value"]);
 *  }
 *  
 *  // Or use range-based for (without header)
 *  CsvReader reader2("data.csv");
 *  for (const auto & row : reader2)
 *    std::cout << row[0] << "\n";
 *  @endcode
 *
 *  @ingroup Utilities
 */
class CsvReader
{
  std::ifstream file_;
  std::istream * stream_;
  bool owns_stream_;
  char delimiter_;
  Array<std::string> header_;
  bool has_header_;
  size_t row_count_;
  
public:
  /// @brief Construct reader from file path.
  /// @throws std::runtime_error If file cannot be opened.
  explicit CsvReader(const std::string & filename, char delimiter = ',')
    : file_(filename), stream_(&file_), owns_stream_(true), 
      delimiter_(delimiter), has_header_(false), row_count_(0)
  {
    if (not file_.is_open())
      throw std::runtime_error("Cannot open file: " + filename);
  }
  
  /// @brief Construct reader from stream.
  explicit CsvReader(std::istream & stream, char delimiter = ',')
    : stream_(&stream), owns_stream_(false), 
      delimiter_(delimiter), has_header_(false), row_count_(0) {}
  
  /// @brief Read and store the header row.
  /// @return The header row.
  Array<std::string> read_header()
  {
    header_ = csv_read_row(*stream_, delimiter_);
    has_header_ = true;
    return header_;
  }
  
  /// @brief Check if more rows are available.
  [[nodiscard]] bool has_next() const
  {
    return stream_->good() and stream_->peek() != std::char_traits<char>::eof();
  }
  
  /// @brief Read the next row as Array.
  Array<std::string> next()
  {
    ++row_count_;
    return csv_read_row(*stream_, delimiter_);
  }
  
  /// @brief Read the next row with header access.
  CsvRow next_row()
  {
    ++row_count_;
    auto fields = csv_read_row(*stream_, delimiter_);
    if (has_header_)
      return CsvRow(std::move(fields), header_);
    return CsvRow(std::move(fields));
  }
  
  /// @brief Get number of rows read so far.
  [[nodiscard]] size_t rows_read() const { return row_count_; }
  
  /// @brief Get the header (if read).
  [[nodiscard]] const Array<std::string> & header() const { return header_; }
  
  /// @brief Check if header was read.
  [[nodiscard]] bool has_header() const { return has_header_; }
  
  /// @brief Skip N rows.
  void skip(size_t n)
  {
    for (size_t i = 0; i < n and has_next(); ++i)
      next();
  }
  
  /// @brief Reset to beginning (only works for file-based readers).
  void reset()
  {
    if (owns_stream_)
      {
        file_.clear();
        file_.seekg(0);
        row_count_ = 0;
        if (has_header_)
          read_header();  // Re-read header
      }
  }
  
  // Iterator support for range-based for loops
  class Iterator
  {
    CsvReader * reader_;
    Array<std::string> current_;
    bool at_end_;
    
  public:
    Iterator(CsvReader * reader, bool at_end)
      : reader_(reader), at_end_(at_end)
    {
      if (not at_end_ and reader_->has_next())
        current_ = reader_->next();
      else
        at_end_ = true;
    }
    
    const Array<std::string> & operator*() const { return current_; }
    
    Iterator & operator++()
    {
      if (reader_->has_next())
        current_ = reader_->next();
      else
        at_end_ = true;
      return *this;
    }
    
    bool operator!=(const Iterator & other) const
    {
      return at_end_ != other.at_end_;
    }
  };
  
  Iterator begin() { return Iterator(this, false); }
  Iterator end() { return Iterator(this, true); }
};

//============================================================================
// Filtering and Selection Functions
//============================================================================

/** @brief Filter CSV rows by a predicate.
 *
 *  Returns only rows for which the predicate returns true.
 *
 *  @tparam Pred Predicate type (callable taking const Array<std::string>&).
 *  @param rows The CSV data.
 *  @param predicate Function returning true for rows to keep.
 *  @return Filtered rows.
 *
 *  @code{.cpp}
 *  auto data = csv_read_file("data.csv");
 *  // Keep only rows where column 1 == "active"
 *  auto active = csv_filter(data, [](const auto& row) {
 *    return row.size() > 1 && row(1) == "active";
 *  });
 *  @endcode
 *
 *  @ingroup Utilities
 */
template <typename Pred>
inline Array<Array<std::string>> csv_filter(const Array<Array<std::string>> & rows,
                                            Pred predicate)
{
  Array<Array<std::string>> result;
  for (size_t i = 0; i < rows.size(); ++i)
    if (predicate(rows(i)))
      result.append(rows(i));
  return result;
}

/** @brief Filter CSV rows by column value.
 *
 *  Returns rows where the specified column equals the given value.
 *
 *  @param rows The CSV data.
 *  @param col_index Column index to check.
 *  @param value Value to match.
 *  @return Filtered rows.
 *
 *  @ingroup Utilities
 */
inline Array<Array<std::string>> csv_filter_by_value(
    const Array<Array<std::string>> & rows,
    size_t col_index,
    const std::string & value)
{
  return csv_filter(rows, [col_index, &value](const Array<std::string> & row) {
    return col_index < row.size() and row(col_index) == value;
  });
}

/** @brief Select specific columns from CSV data.
 *
 *  @param rows The CSV data.
 *  @param col_indices Indices of columns to select.
 *  @return Data with only the selected columns.
 *
 *  @code{.cpp}
 *  auto data = csv_read_file("data.csv");
 *  Array<size_t> cols;
 *  cols.append(0);  // First column
 *  cols.append(2);  // Third column
 *  auto selected = csv_select_columns(data, cols);
 *  @endcode
 *
 *  @ingroup Utilities
 */
inline Array<Array<std::string>> csv_select_columns(
    const Array<Array<std::string>> & rows,
    const Array<size_t> & col_indices)
{
  Array<Array<std::string>> result;
  
  for (size_t i = 0; i < rows.size(); ++i)
    {
      Array<std::string> new_row;
      for (size_t j = 0; j < col_indices.size(); ++j)
        {
          size_t idx = col_indices(j);
          if (idx < rows(i).size())
            new_row.append(rows(i)(idx));
          else
            new_row.append("");  // Empty if column doesn't exist
        }
      result.append(std::move(new_row));
    }
  
  return result;
}

/** @brief Skip the first N rows of CSV data.
 *
 *  @param rows The CSV data.
 *  @param n Number of rows to skip.
 *  @return Data without the first N rows.
 *
 *  @ingroup Utilities
 */
inline Array<Array<std::string>> csv_skip_rows(
    const Array<Array<std::string>> & rows,
    size_t n)
{
  Array<Array<std::string>> result;
  for (size_t i = n; i < rows.size(); ++i)
    result.append(rows(i));
  return result;
}

/** @brief Take only the first N rows of CSV data.
 *
 *  @param rows The CSV data.
 *  @param n Number of rows to take.
 *  @return The first N rows.
 *
 *  @ingroup Utilities
 */
inline Array<Array<std::string>> csv_take_rows(
    const Array<Array<std::string>> & rows,
    size_t n)
{
  Array<Array<std::string>> result;
  size_t limit = (n < rows.size()) ? n : rows.size();
  for (size_t i = 0; i < limit; ++i)
    result.append(rows(i));
  return result;
}

//============================================================================
// Statistics Functions
//============================================================================

/** @brief Count total number of rows.
 *  @ingroup Utilities
 */
[[nodiscard]] inline size_t csv_count_rows(const Array<Array<std::string>> & rows)
{
  return rows.size();
}

/** @brief Count empty fields in CSV data.
 *  @ingroup Utilities
 */
[[nodiscard]] inline size_t csv_count_empty(const Array<Array<std::string>> & rows)
{
  size_t count = 0;
  for (size_t i = 0; i < rows.size(); ++i)
    for (size_t j = 0; j < rows(i).size(); ++j)
      if (rows(i)(j).empty())
        ++count;
  return count;
}

/** @brief Count rows matching a predicate.
 *  @ingroup Utilities
 */
template <typename Pred>
[[nodiscard]] inline size_t csv_count_if(const Array<Array<std::string>> & rows,
                                         Pred predicate)
{
  size_t count = 0;
  for (size_t i = 0; i < rows.size(); ++i)
    if (predicate(rows(i)))
      ++count;
  return count;
}

/** @brief Find first row matching a predicate.
 *  @return Index of first matching row, or rows.size() if not found.
 *  @ingroup Utilities
 */
template <typename Pred>
[[nodiscard]] inline size_t csv_find_row(const Array<Array<std::string>> & rows,
                                         Pred predicate)
{
  for (size_t i = 0; i < rows.size(); ++i)
    if (predicate(rows(i)))
      return i;
  return rows.size();
}

/** @brief Find row where column equals value.
 *  @return Index of first matching row, or rows.size() if not found.
 *  @ingroup Utilities
 */
[[nodiscard]] inline size_t csv_find_by_value(
    const Array<Array<std::string>> & rows,
    size_t col_index,
    const std::string & value)
{
  return csv_find_row(rows, [col_index, &value](const Array<std::string> & row) {
    return col_index < row.size() and row(col_index) == value;
  });
}

/** @brief Get distinct values in a column.
 *  @ingroup Utilities
 */
inline Array<std::string> csv_distinct(const Array<Array<std::string>> & rows,
                                       size_t col_index)
{
  Array<std::string> result;
  for (size_t i = 0; i < rows.size(); ++i)
    {
      if (col_index >= rows(i).size()) continue;
      
      const std::string & val = rows(i)(col_index);
      bool found = false;
      for (size_t j = 0; j < result.size(); ++j)
        if (result(j) == val) { found = true; break; }
      
      if (not found)
        result.append(val);
    }
  return result;
}

//============================================================================
// Transformation Functions
//============================================================================

/** @brief Transpose CSV data (swap rows and columns).
 *
 *  @param rows The CSV data (must be rectangular).
 *  @return Transposed data.
 *  @throws std::runtime_error If data is not rectangular.
 *
 *  @ingroup Utilities
 */
inline Array<Array<std::string>> csv_transpose(
    const Array<Array<std::string>> & rows)
{
  if (rows.size() == 0)
    return Array<Array<std::string>>();
  
  if (not csv_is_rectangular(rows))
    throw std::runtime_error("Cannot transpose non-rectangular CSV data");
  
  size_t num_rows = rows.size();
  size_t num_cols = rows(0).size();
  
  Array<Array<std::string>> result;
  for (size_t j = 0; j < num_cols; ++j)
    {
      Array<std::string> new_row;
      for (size_t i = 0; i < num_rows; ++i)
        new_row.append(rows(i)(j));
      result.append(std::move(new_row));
    }
  
  return result;
}

/** @brief Sort CSV data by a column.
 *
 *  @param rows The CSV data.
 *  @param col_index Column to sort by.
 *  @param ascending Sort in ascending order (default true).
 *  @return Sorted data.
 *
 *  @ingroup Utilities
 */
inline Array<Array<std::string>> csv_sort_by_column(
    const Array<Array<std::string>> & rows,
    size_t col_index,
    bool ascending = true)
{
  // Create index array
  Array<size_t> indices;
  for (size_t i = 0; i < rows.size(); ++i)
    indices.append(i);
  
  // Sort indices by column value
  for (size_t i = 0; i < indices.size(); ++i)
    for (size_t j = i + 1; j < indices.size(); ++j)
      {
        const std::string & a = (col_index < rows(indices(i)).size()) 
                                 ? rows(indices(i))(col_index) : "";
        const std::string & b = (col_index < rows(indices(j)).size()) 
                                 ? rows(indices(j))(col_index) : "";
        
        bool should_swap = ascending ? (a > b) : (a < b);
        if (should_swap)
          std::swap(indices(i), indices(j));
      }
  
  // Build sorted result
  Array<Array<std::string>> result;
  for (size_t i = 0; i < indices.size(); ++i)
    result.append(rows(indices(i)));
  
  return result;
}

/** @brief Sort CSV data by a column with numeric comparison.
 *
 *  @tparam T Numeric type for comparison.
 *  @param rows The CSV data.
 *  @param col_index Column to sort by.
 *  @param ascending Sort in ascending order (default true).
 *  @return Sorted data.
 *
 *  @ingroup Utilities
 */
template <typename T>
inline Array<Array<std::string>> csv_sort_by_column_numeric(
    const Array<Array<std::string>> & rows,
    size_t col_index,
    bool ascending = true)
{
  // Create pairs of (value, index)
  Array<std::pair<T, size_t>> pairs;
  for (size_t i = 0; i < rows.size(); ++i)
    {
      T val = (col_index < rows(i).size()) 
              ? csv_to_number<T>(rows(i)(col_index)) : T{};
      pairs.append({val, i});
    }
  
  // Sort pairs
  for (size_t i = 0; i < pairs.size(); ++i)
    for (size_t j = i + 1; j < pairs.size(); ++j)
      {
        bool should_swap = ascending 
                           ? (pairs(i).first > pairs(j).first)
                           : (pairs(i).first < pairs(j).first);
        if (should_swap)
          std::swap(pairs(i), pairs(j));
      }
  
  // Build sorted result
  Array<Array<std::string>> result;
  for (size_t i = 0; i < pairs.size(); ++i)
    result.append(rows(pairs(i).second));
  
  return result;
}

/** @brief Remove duplicate rows.
 *
 *  @param rows The CSV data.
 *  @return Data with duplicates removed.
 *
 *  @ingroup Utilities
 */
inline Array<Array<std::string>> csv_unique(
    const Array<Array<std::string>> & rows)
{
  Array<Array<std::string>> result;
  
  for (size_t i = 0; i < rows.size(); ++i)
    {
      bool is_duplicate = false;
      for (size_t j = 0; j < result.size(); ++j)
        {
          if (result(j).size() != rows(i).size()) continue;
          
          bool equal = true;
          for (size_t k = 0; k < rows(i).size(); ++k)
            if (result(j)(k) != rows(i)(k)) { equal = false; break; }
          
          if (equal) { is_duplicate = true; break; }
        }
      
      if (not is_duplicate)
        result.append(rows(i));
    }
  
  return result;
}

/** @brief Apply a transformation to each field.
 *
 *  @tparam Func Transformation function type.
 *  @param rows The CSV data.
 *  @param func Function taking string and returning string.
 *  @return Transformed data.
 *
 *  @ingroup Utilities
 */
template <typename Func>
inline Array<Array<std::string>> csv_transform(
    const Array<Array<std::string>> & rows,
    Func func)
{
  Array<Array<std::string>> result;
  
  for (size_t i = 0; i < rows.size(); ++i)
    {
      Array<std::string> new_row;
      for (size_t j = 0; j < rows(i).size(); ++j)
        new_row.append(func(rows(i)(j)));
      result.append(std::move(new_row));
    }
  
  return result;
}

//============================================================================
// Utility Functions
//============================================================================

/** @brief Skip UTF-8 BOM if present.
 *
 *  Checks for and skips the UTF-8 Byte Order Mark (EF BB BF) at the
 *  beginning of a stream.
 *
 *  @param in The input stream.
 *  @return True if BOM was found and skipped.
 *
 *  @ingroup Utilities
 */
inline bool csv_skip_bom(std::istream & in)
{
  char c1 = in.get();
  char c2 = in.get();
  char c3 = in.get();
  
  if (c1 == '\xEF' and c2 == '\xBB' and c3 == '\xBF')
    return true;  // BOM skipped
  
  // Not a BOM, put characters back
  in.putback(c3);
  in.putback(c2);
  in.putback(c1);
  return false;
}

/** @brief Trim whitespace from all fields.
 *
 *  @param rows The CSV data.
 *  @return Data with trimmed fields.
 *
 *  @ingroup Utilities
 */
inline Array<Array<std::string>> csv_trim_fields(
    const Array<Array<std::string>> & rows)
{
  auto trim = [](const std::string & s) -> std::string {
    size_t start = 0;
    while (start < s.size() and std::isspace(static_cast<unsigned char>(s[start])))
      ++start;
    
    size_t end = s.size();
    while (end > start and std::isspace(static_cast<unsigned char>(s[end - 1])))
      --end;
    
    return s.substr(start, end - start);
  };
  
  return csv_transform(rows, trim);
}

/** @brief Replace empty fields with a default value.
 *
 *  @param rows The CSV data.
 *  @param default_value Value to use for empty fields.
 *  @return Data with empty fields replaced.
 *
 *  @ingroup Utilities
 */
inline Array<Array<std::string>> csv_fill_empty(
    const Array<Array<std::string>> & rows,
    const std::string & default_value)
{
  return csv_transform(rows, [&default_value](const std::string & s) {
    return s.empty() ? default_value : s;
  });
}

/** @brief Join two CSV datasets horizontally (add columns).
 *
 *  @param left First dataset.
 *  @param right Second dataset.
 *  @return Combined dataset with columns from both.
 *
 *  @note Rows are matched by index. If datasets have different numbers
 *        of rows, the result has the maximum number of rows.
 *
 *  @ingroup Utilities
 */
inline Array<Array<std::string>> csv_join_horizontal(
    const Array<Array<std::string>> & left,
    const Array<Array<std::string>> & right)
{
  Array<Array<std::string>> result;
  size_t max_rows = (left.size() > right.size()) ? left.size() : right.size();
  
  for (size_t i = 0; i < max_rows; ++i)
    {
      Array<std::string> new_row;
      
      // Add fields from left
      if (i < left.size())
        for (size_t j = 0; j < left(i).size(); ++j)
          new_row.append(left(i)(j));
      
      // Add fields from right
      if (i < right.size())
        for (size_t j = 0; j < right(i).size(); ++j)
          new_row.append(right(i)(j));
      
      result.append(std::move(new_row));
    }
  
  return result;
}

/** @brief Join two CSV datasets vertically (add rows).
 *
 *  @param top First dataset.
 *  @param bottom Second dataset.
 *  @return Combined dataset with rows from both.
 *
 *  @ingroup Utilities
 */
inline Array<Array<std::string>> csv_join_vertical(
    const Array<Array<std::string>> & top,
    const Array<Array<std::string>> & bottom)
{
  Array<Array<std::string>> result;
  
  for (size_t i = 0; i < top.size(); ++i)
    result.append(top(i));
  
  for (size_t i = 0; i < bottom.size(); ++i)
    result.append(bottom(i));
  
  return result;
}

/** @brief Inner join two CSV datasets by a key column.
 *
 *  @param left First dataset.
 *  @param left_key_col Key column index in left dataset.
 *  @param right Second dataset.
 *  @param right_key_col Key column index in right dataset.
 *  @return Joined dataset (only matching rows).
 *
 *  @ingroup Utilities
 */
inline Array<Array<std::string>> csv_inner_join(
    const Array<Array<std::string>> & left,
    size_t left_key_col,
    const Array<Array<std::string>> & right,
    size_t right_key_col)
{
  Array<Array<std::string>> result;
  
  for (size_t i = 0; i < left.size(); ++i)
    {
      if (left_key_col >= left(i).size()) continue;
      const std::string & left_key = left(i)(left_key_col);
      
      for (size_t j = 0; j < right.size(); ++j)
        {
          if (right_key_col >= right(j).size()) continue;
          
          if (left_key == right(j)(right_key_col))
            {
              // Combine rows (excluding duplicate key from right)
              Array<std::string> new_row;
              
              for (size_t k = 0; k < left(i).size(); ++k)
                new_row.append(left(i)(k));
              
              for (size_t k = 0; k < right(j).size(); ++k)
                if (k != right_key_col)
                  new_row.append(right(j)(k));
              
              result.append(std::move(new_row));
            }
        }
    }
  
  return result;
}

/** @brief Group rows by a column value.
 *
 *  @param rows The CSV data.
 *  @param col_index Column to group by.
 *  @return Array of groups, each containing rows with the same value.
 *
 *  @ingroup Utilities
 */
inline Array<Array<Array<std::string>>> csv_group_by(
    const Array<Array<std::string>> & rows,
    size_t col_index)
{
  Array<std::string> keys = csv_distinct(rows, col_index);
  Array<Array<Array<std::string>>> result;
  
  for (size_t k = 0; k < keys.size(); ++k)
    {
      Array<Array<std::string>> group;
      for (size_t i = 0; i < rows.size(); ++i)
        if (col_index < rows(i).size() and rows(i)(col_index) == keys(k))
          group.append(rows(i));
      result.append(std::move(group));
    }
  
  return result;
}

/** @brief Add a new column with computed values.
 *
 *  @tparam Func Function type (takes row, returns string).
 *  @param rows The CSV data.
 *  @param func Function to compute new column value.
 *  @return Data with new column appended.
 *
 *  @code{.cpp}
 *  // Add a "total" column that sums columns 1 and 2
 *  auto with_total = csv_add_column(data, [](const auto& row) {
 *    if (row.size() < 3) return std::string("0");
 *    int a = std::stoi(row(1));
 *    int b = std::stoi(row(2));
 *    return std::to_string(a + b);
 *  });
 *  @endcode
 *
 *  @ingroup Utilities
 */
template <typename Func>
inline Array<Array<std::string>> csv_add_column(
    const Array<Array<std::string>> & rows,
    Func func)
{
  Array<Array<std::string>> result;
  
  for (size_t i = 0; i < rows.size(); ++i)
    {
      Array<std::string> new_row;
      for (size_t j = 0; j < rows(i).size(); ++j)
        new_row.append(rows(i)(j));
      new_row.append(func(rows(i)));
      result.append(std::move(new_row));
    }
  
  return result;
}

/** @brief Rename a column (in the header row).
 *
 *  @param rows The CSV data (first row is header).
 *  @param old_name Current column name.
 *  @param new_name New column name.
 *  @return Data with renamed column.
 *
 *  @ingroup Utilities
 */
inline Array<Array<std::string>> csv_rename_column(
    const Array<Array<std::string>> & rows,
    const std::string & old_name,
    const std::string & new_name)
{
  if (rows.size() == 0)
    return rows;
  
  Array<Array<std::string>> result;
  
  // Process header
  Array<std::string> new_header;
  for (size_t j = 0; j < rows(0).size(); ++j)
    {
      if (rows(0)(j) == old_name)
        new_header.append(new_name);
      else
        new_header.append(rows(0)(j));
    }
  result.append(std::move(new_header));
  
  // Copy remaining rows
  for (size_t i = 1; i < rows.size(); ++i)
    result.append(rows(i));
  
  return result;
}

} // namespace Aleph

// Global namespace compatibility: export commonly used functions and classes
using Aleph::CsvRow;
using Aleph::CsvReader;
using Aleph::csv_read_row;
using Aleph::csv_read_all;
using Aleph::csv_read_file;
using Aleph::csv_escape;
using Aleph::csv_write_row;
using Aleph::csv_write_all;
using Aleph::csv_write_file;
using Aleph::csv_num_columns;
using Aleph::csv_is_rectangular;
using Aleph::csv_get_column;
using Aleph::csv_to_number;
using Aleph::csv_filter;
using Aleph::csv_filter_by_value;
using Aleph::csv_select_columns;
using Aleph::csv_skip_rows;
using Aleph::csv_take_rows;
using Aleph::csv_count_rows;
using Aleph::csv_count_empty;
using Aleph::csv_count_if;
using Aleph::csv_find_row;
using Aleph::csv_find_by_value;
using Aleph::csv_distinct;
using Aleph::csv_transpose;
using Aleph::csv_sort_by_column;
using Aleph::csv_sort_by_column_numeric;
using Aleph::csv_unique;
using Aleph::csv_transform;
using Aleph::csv_skip_bom;
using Aleph::csv_trim_fields;
using Aleph::csv_fill_empty;
using Aleph::csv_join_horizontal;
using Aleph::csv_join_vertical;
using Aleph::csv_inner_join;
using Aleph::csv_group_by;
using Aleph::csv_add_column;
using Aleph::csv_rename_column;

#endif // PARSE_CSV_H
