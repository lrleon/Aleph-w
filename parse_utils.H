
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
 * @file parse_utils.H
 * @brief Comprehensive parsing utilities for text processing and compiler construction
 * @ingroup Utilities
 * @author Leandro Rabindranath León
 *
 * This header provides a complete toolkit for parsing text files, suitable for
 * configuration files, domain-specific languages, and even compiler construction.
 *
 * ## Features
 *
 * ### Basic Parsing
 * - **Character-level reading** with automatic line/column tracking
 * - **Token scanning** with position bookmarking for error reporting
 * - **Whitespace and comment handling**
 *
 * ### Number Parsing
 * - Integer parsing (decimal, with sign)
 * - Floating-point parsing (double)
 * - Hexadecimal numbers (0xFF)
 * - Octal numbers (0755)
 * - Binary numbers (0b1010)
 *
 * ### String Parsing
 * - Quoted and unquoted strings
 * - Escape sequence processing (\\n, \\t, \\\\, \\\", etc.)
 * - Character literals ('a', '\n')
 *
 * ### Lexer/Tokenizer Support
 * - Identifier parsing
 * - Keyword recognition
 * - Token types and structures
 * - Lookahead (peek) operations
 * - Stream position marking and restoration (backtracking)
 *
 * ### Error Handling
 * - SourceLocation structure for precise error reporting
 * - ParseError exception with location information
 * - Warning and error reporting functions
 *
 * ### File Utilities
 * - Load entire file contents
 * - Load file as lines
 *
 * ## Usage Example
 *
 * ```cpp
 * #include <parse_utils.H>
 * 
 * void parse_program(const std::string& filename) {
 *   std::ifstream input(filename);
 *   reset_parse_state();
 *   
 *   while (!input.eof()) {
 *     skip_whitespace_and_comments(input);
 *     std::string id = load_identifier(input);
 *     
 *     if (is_keyword(id, {"if", "while", "for"})) {
 *       // Handle keyword...
 *     } else {
 *       expect_char(input, '=');
 *       double value = load_double(input);
 *     }
 *   }
 * }
 * ```
 *
 * ## Thread Safety
 *
 * @warning This module uses global state for tracking file position and
 * current token. It is **NOT** thread-safe. Each thread should use
 * separate parsing contexts or external synchronization.
 */

#ifndef PARSE_UTILS_H
#define PARSE_UTILS_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <aleph.H>
#include <ah-errors.H>

namespace Aleph
{
  // ============================================================================
  // Global State (inline for header-only)
  // ============================================================================

  /**
   * @brief Default buffer size for token parsing
   *
   * Maximum size for individual tokens (strings, numbers).
   * Tokens exceeding this size will cause an error.
   */
  inline constexpr size_t Buffer_Size = 512;

  /**
   * @brief Current line number in the input stream
   *
   * Updated automatically by read_char_from_stream().
   * Line numbering starts at 1.
   */
  inline int current_line_number = 1;

  /**
   * @brief Current column number in the input stream
   *
   * Updated automatically by read_char_from_stream().
   * Column numbering starts at 1, resets to 0 on newline.
   */
  inline int current_col_number = 1;

  /**
   * @brief Line number at the start of the current token
   *
   * Set by init_token_scanning() before reading a token.
   * Used for error reporting to show where the token began.
   */
  inline int previous_line_number = 1;

  /**
   * @brief Column number at the start of the current token
   *
   * Set by init_token_scanning() before reading a token.
   * Used for error reporting to show where the token began.
   */
  inline int previous_col_number = 1;

  /**
   * @brief The most recently scanned token
   *
   * Set by close_token_scanning() after completing a token.
   * Useful for error messages to show what was being parsed.
   */
  inline std::string token_instance;


  // ============================================================================
  // Buffer Management
  // ============================================================================

  /**
   * @brief Append a character to a buffer with bounds checking
   *
   * Safely adds a character to a buffer, checking that the buffer
   * has not been exceeded. If the buffer is full, an error is raised.
   *
   * @param start_addr Reference to current write position; incremented after write
   * @param end_addr   Pointer to one past the end of the buffer
   * @param c          Character to append
   *
   * @throws Terminates program if buffer overflow would occur
   *
   * @par Example
   * @code
   * char buffer[100];
   * char* ptr = buffer;
   * char* end = buffer + 100;
   *
   * put_char_in_buffer(ptr, end, 'H');
   * put_char_in_buffer(ptr, end, 'i');
   * put_char_in_buffer(ptr, end, '\0');
   * // buffer now contains "Hi"
   * @endcode
   */
  inline void put_char_in_buffer(char *& start_addr, char *end_addr, int c)
  {
    if (start_addr >= end_addr)
      AH_ERROR("String too long (buffer overflow in parse_utils)");

    *start_addr++ = static_cast<char>(c);
  }

  // ============================================================================
  // Token Scanning
  // ============================================================================

  /**
   * @brief Initialize token scanning by recording current position
   *
   * Call this before starting to read a new token. It saves the current
   * line and column numbers so that error messages can report where
   * the token began.
   *
   * @par Example
   * @code
   * init_token_scanning();
   * // ... read characters for the token ...
   * close_token_scanning(buffer, ptr, end);
   * @endcode
   *
   * @see close_token_scanning()
   */
  inline void init_token_scanning()
  {
    previous_line_number = current_line_number;
    previous_col_number = current_col_number;
  }

  /**
   * @brief Finalize token scanning by null-terminating and saving the token
   *
   * Call this after reading all characters of a token. It adds a null
   * terminator to the buffer and saves the token string for error reporting.
   *
   * @param buffer     Start of the buffer containing the token
   * @param start_addr Reference to current write position; used to add null terminator
   * @param end_addr   Pointer to one past the end of the buffer
   *
   * @see init_token_scanning()
   */
  inline void close_token_scanning(char *buffer, char *& start_addr, char *end_addr)
  {
    put_char_in_buffer(start_addr, end_addr, '\0');
    token_instance = buffer;
  }

  // ============================================================================
  // Stream Reading
  // ============================================================================

  /**
   * @brief Read a single character from an input stream with position tracking
   *
   * Reads one character from the stream and updates the current line
   * and column tracking. Handles newline characters by incrementing
   * the line number and resetting the column.
   *
   * @param input_stream The input file stream to read from
   * @return The character read (as int), or EOF on end of file
   *
   * @throws std::out_of_range if the stream is already at EOF before reading
   *
   * @par Example
   * @code
   * std::ifstream file("input.txt");
   * int ch = read_char_from_stream(file);
   * while (ch != EOF) {
   *   process(ch);
   *   ch = read_char_from_stream(file);
   * }
   * @endcode
   */
  inline int read_char_from_stream(std::ifstream & input_stream)
  {
    ah_out_of_range_error_if(input_stream.eof())
    << "end of file has been reached";

    int c = input_stream.get();

    if (c == '\n')
      {
        current_line_number++;
        current_col_number = 0;
      }
    else
      current_col_number++;

    return c;
  }

  /**
   * @brief Skip whitespace characters in the input stream
   *
   * Advances the stream position past any whitespace characters
   * (spaces, tabs, newlines, etc.). After this call, the next
   * character in the stream will be non-whitespace (or EOF).
   *
   * @param input_stream The input file stream
   *
   * @throws std::out_of_range if EOF is reached while skipping
   *
   * @par Example
   * @code
   * // Input: "   hello"
   * skip_white_spaces(input);
   * // Stream now positioned at 'h'
   * @endcode
   */
  inline void skip_white_spaces(std::ifstream & input_stream)
  {
    while (std::isspace(read_char_from_stream(input_stream))); // Skip whitespace
    input_stream.unget();
  }

  // ============================================================================
  // Token Loading
  // ============================================================================

  /**
   * @brief Load an integer number from the input stream
   *
   * Parses and returns a signed integer from the input stream.
   * Leading whitespace is automatically skipped. The number may
   * optionally be preceded by a minus sign for negative values.
   *
   * @param input_stream The input file stream
   * @return The parsed integer value
   *
   * @throws std::domain_error if the number format is invalid
   * @throws std::out_of_range if EOF is reached unexpectedly
   *
   * @par Accepted formats
   * - Positive integers: `123`, `0`, `42`
   * - Negative integers: `-123`, `-1`
   *
   * @par Example
   * @code
   * // Input: "  -42  hello"
   * long num = load_number(input);
   * // num == -42, stream positioned at space before "hello"
   * @endcode
   *
   * @note Numbers must be terminated by whitespace. Other terminators
   *       (like commas) will cause an error.
   */
  inline long load_number(std::ifstream & input_stream)
  {
    char buffer[Buffer_Size];
    char *start_addr = buffer;
    char *end_addr = buffer + Buffer_Size;

    init_token_scanning();

    try
      {
        skip_white_spaces(input_stream);
        int c = read_char_from_stream(input_stream);

        // Handle optional minus sign
        if (c == '-')
          {
            put_char_in_buffer(start_addr, end_addr, c);
            c = read_char_from_stream(input_stream);

            if (not std::isdigit(c))
              return std::atol(buffer);
          }

        while (true)
          {
            // Read digits
            while (std::isdigit(c))
              {
                put_char_in_buffer(start_addr, end_addr, c);
                c = read_char_from_stream(input_stream);
              }

            // Number ends at whitespace
            if (std::isspace(c))
              {
                close_token_scanning(buffer, start_addr, end_addr);
                return std::atol(buffer);
              }

            ah_domain_error_if(true) << "Invalid number format";
          }
      }
    catch (const std::out_of_range &)
      {
        // EOF reached - return what we have
        close_token_scanning(buffer, start_addr, end_addr);
        return std::atol(buffer);
      }
  }

  /**
   * @brief Load a string from the input stream
   *
   * Parses and returns a string from the input stream. The string may
   * be quoted with double quotes or unquoted.
   *
   * - **Quoted strings**: Start and end with a double quote character (\").
   *   and special characters. The quotes are not included in the result.
   * - **Unquoted strings**: End at whitespace or EOF. Cannot contain spaces.
   *
   * @param input_stream The input file stream
   * @return The parsed string (without quotes if quoted)
   *
   * @throws std::out_of_range if EOF is reached unexpectedly
   *
   * @par Example
   * @code
   * // Input: "hello world" more
   * std::string s1 = load_string(input);  // s1 == "hello world"
   * std::string s2 = load_string(input);  // s2 == "more"
   * @endcode
   *
   * @par Example (unquoted)
   * @code
   * // Input: hello world
   * std::string s1 = load_string(input);  // s1 == "hello"
   * std::string s2 = load_string(input);  // s2 == "world"
   * @endcode
   */
  inline std::string load_string(std::ifstream & input_stream)
  {
    char buffer[Buffer_Size];
    char *start_addr = buffer;
    char *end_addr = buffer + Buffer_Size;

    bool quoted = false;

    init_token_scanning();
    skip_white_spaces(input_stream);

    int c = read_char_from_stream(input_stream);

    if (c == '\"')
      quoted = true;
    else
      put_char_in_buffer(start_addr, end_addr, c);

    while (true)
      {
        c = read_char_from_stream(input_stream);

        // End of quoted string
        if (c == '\"' and quoted)
          break;

        // End of unquoted string
        if (not quoted and (std::isblank(c) or c == '\n' or c == EOF))
          break;

        put_char_in_buffer(start_addr, end_addr, c);
      }

    close_token_scanning(buffer, start_addr, end_addr);
    return std::string(buffer);
  }

  // ============================================================================
  // Error Reporting
  // ============================================================================

  /**
   * @brief Print a parse error message and terminate the program
   *
   * Outputs a formatted error message including the current file position
   * (line:column) and the last token that was scanned, then exits with
   * status code 1.
   *
   * @param str The error message to display
   *
   * @note This function does not return; it terminates the program.
   *
   * @par Example
   * @code
   * if (unexpected_token)
   *   print_parse_error_and_exit("Expected 'begin' keyword");
   * // Output: Parse error 15:23 Expected 'begin' keyword
   * //         Last token seen is: foo
   * @endcode
   */
  [[noreturn]]
  inline void print_parse_error_and_exit(const std::string & str)
  {
    std::cout << "Parse error "
        << previous_line_number << ":" << previous_col_number << " "
        << str << std::endl
        << "Last token seen is: " << token_instance << std::endl;
    std::exit(1);
  }

  /**
   * @brief Print a parse warning message
   *
   * Outputs a formatted warning message including the current file position
   * (line:column) and the last token that was scanned. Unlike
   * print_parse_error_and_exit(), this function returns normally.
   *
   * @param str The warning message to display
   *
   * @par Example
   * @code
   * if (deprecated_syntax)
   *   print_parse_warning("Deprecated syntax, use new format");
   * // Continues execution after printing warning
   * @endcode
   */
  inline void print_parse_warning(const std::string & str)
  {
    std::cout << "Parse warning "
        << previous_line_number << ":" << previous_col_number << " "
        << str << std::endl
        << "Last token seen is: " << token_instance << std::endl;
  }

  // ============================================================================
  // Utility Functions
  // ============================================================================

  /**
   * @brief Convert command line arguments to a single string
   *
   * Concatenates all command line arguments into a single space-separated
   * string. Useful for logging or displaying the command that was invoked.
   *
   * @param argc Argument count (from main())
   * @param argv Argument vector (from main())
   * @return Space-separated string of all arguments
   *
   * @par Example
   * @code
   * // Program invoked as: ./myapp --input file.txt --verbose
   * int main(int argc, char* argv[]) {
   *   std::string cmd = command_line_to_string(argc, argv);
   *   // cmd == " ./myapp --input file.txt --verbose"
   * }
   * @endcode
   *
   * @note The result has a leading space before the first argument.
   */
  inline std::string command_line_to_string(int argc, char *argv[])
  {
    std::string command_line;

    for (int i = 0; i < argc; ++i)
      {
        command_line += " ";
        command_line += argv[i];
      }

    return command_line;
  }

  /**
   * @brief Reset the parsing state to initial values
   *
   * Resets line/column counters and clears the token instance.
   * Call this before parsing a new file.
   *
   * @par Example
   * @code
   * reset_parse_state();
   * std::ifstream file("newfile.txt");
   * // Begin parsing fresh
   * @endcode
   */
  inline void reset_parse_state()
  {
    current_line_number = 1;
    current_col_number = 1;
    previous_line_number = 1;
    previous_col_number = 1;
    token_instance.clear();
  }

  // ============================================================================
  // Source Location and Parse Errors
  // ============================================================================

  /**
   * @brief Represents a location in source code
   *
   * Stores file name, line number, and column number for precise
   * error reporting in parsers and compilers.
   */
  struct SourceLocation
  {
    std::string filename; ///< Name of the source file
    int line = 1; ///< Line number (1-based)
    int column = 1; ///< Column number (1-based)

    /// Default constructor
    SourceLocation() = default;

    /// Construct with all fields
    SourceLocation(const std::string & file, int ln, int col)
      : filename(file), line(ln), column(col) {}

    /// Construct from current global parsing state
    static SourceLocation current(const std::string & file = "")
    {
      return SourceLocation(file, current_line_number, current_col_number);
    }

    /// Construct from previous (token start) global parsing state
    static SourceLocation previous(const std::string & file = "")
    {
      return SourceLocation(file, previous_line_number, previous_col_number);
    }

    /**
     * @brief Convert to human-readable string
     * @return String in format "filename:line:column" or "line:column"
     */
    std::string to_string() const
    {
      std::ostringstream oss;
      if (not filename.empty())
        oss << filename << ":";
      oss << line << ":" << column;
      return oss.str();
    }
  };

  /**
   * @brief Exception class for parsing errors with location information
   *
   * Extends std::runtime_error to include source location where the
   * error occurred, enabling precise error messages.
   *
   * @par Example
   * @code
   * if (unexpected_token)
   *   throw ParseError("Expected semicolon", SourceLocation::previous("input.txt"));
   * @endcode
   */
  class ParseError : public std::runtime_error
  {
    SourceLocation loc;

  public:
    /**
     * @brief Construct a parse error with message and location
     * @param message Error description
     * @param location Where the error occurred
     */
    ParseError(const std::string & message, const SourceLocation & location)
      : std::runtime_error(location.to_string() + ": " + message)
        , loc(location)
    {}

    /**
     * @brief Construct a parse error with message only
     * @param message Error description (uses current global position)
     */
    explicit ParseError(const std::string & message)
      : std::runtime_error(std::to_string(previous_line_number) + ":" +
                           std::to_string(previous_col_number) + ": " + message)
        , loc("", previous_line_number, previous_col_number)
    {}

    /// Get the source location of the error
    const SourceLocation &location() const { return loc; }
  };

  // ============================================================================
  // Lookahead and Backtracking
  // ============================================================================

  /**
   * @brief Structure to save stream position for backtracking
   */
  struct StreamPosition
  {
    std::streampos pos; ///< Stream position
    int line; ///< Line number at this position
    int column; ///< Column number at this position
  };

  /**
   * @brief Peek at the next character without consuming it
   *
   * Returns the next character in the stream without advancing
   * the stream position. Useful for lookahead decisions.
   *
   * @param input_stream The input file stream
   * @return The next character, or EOF if at end of file
   *
   * @par Example
   * @code
   * if (peek_char(input) == '(') {
   *   // Handle parenthesized expression
   * }
   * @endcode
   */
  inline int peek_char(std::ifstream & input_stream)
  {
    return input_stream.peek();
  }

  /**
   * @brief Mark current position for potential backtracking
   *
   * Saves the current stream position and line/column state so
   * that it can be restored later if needed.
   *
   * @param input_stream The input file stream
   * @return StreamPosition that can be passed to restore_position()
   *
   * @see restore_position()
   */
  inline StreamPosition mark_position(std::ifstream & input_stream)
  {
    StreamPosition sp;
    sp.pos = input_stream.tellg();
    sp.line = current_line_number;
    sp.column = current_col_number;
    return sp;
  }

  /**
   * @brief Restore a previously marked position
   *
   * Seeks the stream back to a saved position and restores the
   * line/column tracking state.
   *
   * @param input_stream The input file stream
   * @param pos Position previously returned by mark_position()
   *
   * @see mark_position()
   */
  inline void restore_position(std::ifstream & input_stream, const StreamPosition & pos)
  {
    input_stream.seekg(pos.pos);
    current_line_number = pos.line;
    current_col_number = pos.column;
  }

  // ============================================================================
  // Comment Handling
  // ============================================================================

  /**
   * @brief Skip a line comment (// style or # style)
   *
   * Assumes the comment marker has already been read. Advances the
   * stream to the end of the line (or EOF).
   *
   * @param input_stream The input file stream
   *
   * @par Example
   * @code
   * if (peek_char(input) == '/' && peek_next == '/') {
   *   read_char_from_stream(input);  // consume first /
   *   read_char_from_stream(input);  // consume second /
   *   skip_line_comment(input);
   * }
   * @endcode
   */
  inline void skip_line_comment(std::ifstream & input_stream)
  {
    int c;
    while (not input_stream.eof())
      {
        c = input_stream.get();
        if (c == '\n')
          {
            current_line_number++;
            current_col_number = 0;
            return;
          }
        current_col_number++;
      }
  }

  /**
   * @brief Skip a block comment (C-style)
   *
   * Reads and discards characters until the closing sequence is found.
   * Updates line/column tracking appropriately.
   *
   * @param input_stream The input file stream
   * @param open Opening sequence (default "/ *")
   * @param close Closing sequence (default "* /")
   *
   * @throws ParseError if EOF is reached before comment closes
   *
   * @note The open parameter is for documentation; this function assumes
   *       the opening sequence has already been consumed.
   */
  inline void skip_block_comment(std::ifstream & input_stream,
                                 const std::string & open = "/" "*",
                                 const std::string & close = "*" "/")
  {
    size_t match_pos = 0;

    while (not input_stream.eof())
      {
        if (const int c = input_stream.get(); c == '\n')
          {
            current_line_number++;
            current_col_number = 0;
            match_pos = 0;
          }
        else
          {
            current_col_number++;

            if (c == close[match_pos])
              {
                match_pos++;
                if (match_pos == close.size())
                  return; // Found closing sequence
              }
            else
              match_pos = (c == close[0]) ? 1 : 0;
          }
      }

    throw ParseError("Unterminated block comment");
  }

  /**
   * @brief Skip whitespace and comments (C/C++ style)
   *
   * Skips spaces, tabs, newlines, line comments (//), and block
   * comments. This is a common operation in lexers.
   *
   * @param input_stream The input file stream
   *
   * @par Example
   * @code
   * skip_whitespace_and_comments(input);
   * // Now positioned at next meaningful token
   * @endcode
   */
  inline void skip_whitespace_and_comments(std::ifstream & input_stream)
  {
    while (not input_stream.eof())
      {
        if (const int c = peek_char(input_stream); std::isspace(c))
          read_char_from_stream(input_stream);
        else if (c == '/')
          {
            // Peek ahead for // or /* ... */
            input_stream.get();
            current_col_number++;
            if (const int next = peek_char(input_stream); next == '/')
              {
                input_stream.get();
                current_col_number++;
                skip_line_comment(input_stream);
              }
            else if (next == '*')
              {
                input_stream.get();
                current_col_number++;
                skip_block_comment(input_stream);
              }
            else
              {
                // Not a comment, put back the '/'
                input_stream.unget();
                current_col_number--;
                return;
              }
          }
        else if (c == '#')
          {
            // Shell/Python style line comment
            read_char_from_stream(input_stream);
            skip_line_comment(input_stream);
          }
        else
          return; // Non-whitespace, non-comment found
      }
  }

  // ============================================================================
  // Numeric Parsing (Extended)
  // ============================================================================

  /**
   * @brief Load a floating-point number from the input stream
   *
   * Parses and returns a double-precision floating-point number.
   * Supports optional sign, decimal point, and scientific notation.
   *
   * @param input_stream The input file stream
   * @return The parsed double value
   *
   * @throws ParseError if the format is invalid
   *
   * @par Accepted formats
   * - `123.456`
   * - `-3.14`
   * - `1e10`, `1E10`
   * - `1.5e-3`
   * - `.5` (leading decimal)
   * - `5.` (trailing decimal)
   *
   * @par Example
   * @code
   * double pi = load_double(input);  // "3.14159"
   * @endcode
   */
  inline double load_double(std::ifstream & input_stream)
  {
    char buffer[Buffer_Size];
    char *ptr = buffer;
    char *end = buffer + Buffer_Size;

    init_token_scanning();
    skip_white_spaces(input_stream);

    int c = read_char_from_stream(input_stream);

    // Optional sign
    if (c == '-' or c == '+')
      {
        put_char_in_buffer(ptr, end, c);
        c = read_char_from_stream(input_stream);
      }

    bool has_digits = false;

    // Integer part
    while (std::isdigit(c))
      {
        has_digits = true;
        put_char_in_buffer(ptr, end, c);
        c = read_char_from_stream(input_stream);
      }

    // Decimal part
    if (c == '.')
      {
        put_char_in_buffer(ptr, end, c);
        c = read_char_from_stream(input_stream);

        while (std::isdigit(c))
          {
            has_digits = true;
            put_char_in_buffer(ptr, end, c);
            c = read_char_from_stream(input_stream);
          }
      }

    if (not has_digits)
      throw ParseError("Invalid floating-point number: no digits");

    // Exponent part
    if (c == 'e' or c == 'E')
      {
        put_char_in_buffer(ptr, end, c);
        c = read_char_from_stream(input_stream);

        if (c == '+' or c == '-')
          {
            put_char_in_buffer(ptr, end, c);
            c = read_char_from_stream(input_stream);
          }

        if (not std::isdigit(c))
          throw ParseError("Invalid exponent in floating-point number");

        while (std::isdigit(c))
          {
            put_char_in_buffer(ptr, end, c);
            c = read_char_from_stream(input_stream);
          }
      }

    // Put back the terminating character
    input_stream.unget();
    if (c != '\n')
      current_col_number--;

    close_token_scanning(buffer, ptr, end);
    return std::strtod(buffer, nullptr);
  }

  /**
   * @brief Load a hexadecimal number from the input stream
   *
   * Parses a number in hexadecimal format (0x or 0X prefix).
   *
   * @param input_stream The input file stream
   * @return The parsed value
   *
   * @throws ParseError if format is invalid
   *
   * @par Accepted formats
   * - `0xFF`, `0XFF`
   * - `0x1a2b`
   */
  inline long load_hex_number(std::ifstream & input_stream)
  {
    char buffer[Buffer_Size];
    char *ptr = buffer;
    char *end = buffer + Buffer_Size;

    init_token_scanning();
    skip_white_spaces(input_stream);

    int c = read_char_from_stream(input_stream);

    // Expect '0'
    if (c != '0')
      throw ParseError("Hexadecimal number must start with 0x");

    c = read_char_from_stream(input_stream);
    if (c != 'x' and c != 'X')
      throw ParseError("Hexadecimal number must start with 0x");

    c = read_char_from_stream(input_stream);

    if (not std::isxdigit(c))
      throw ParseError("Invalid hexadecimal number");

    while (std::isxdigit(c))
      {
        put_char_in_buffer(ptr, end, c);
        c = read_char_from_stream(input_stream);
      }

    input_stream.unget();
    if (c != '\n')
      current_col_number--;

    close_token_scanning(buffer, ptr, end);
    return std::strtol(buffer, nullptr, 16);
  }

  /**
   * @brief Load an octal number from the input stream
   *
   * Parses a number in octal format (leading 0).
   *
   * @param input_stream The input file stream
   * @return The parsed value
   *
   * @throws ParseError if format is invalid
   *
   * @par Accepted formats
   * - `0755`
   * - `0123`
   */
  inline long load_octal_number(std::ifstream & input_stream)
  {
    char buffer[Buffer_Size];
    char *ptr = buffer;
    char *end = buffer + Buffer_Size;

    init_token_scanning();
    skip_white_spaces(input_stream);

    int c = read_char_from_stream(input_stream);

    if (c != '0')
      throw ParseError("Octal number must start with 0");

    c = read_char_from_stream(input_stream);

    // Check it's not hex (0x)
    if (c == 'x' or c == 'X')
      throw ParseError("Expected octal number, got hexadecimal");

    while (c >= '0' and c <= '7')
      {
        put_char_in_buffer(ptr, end, c);
        c = read_char_from_stream(input_stream);
      }

    input_stream.unget();
    if (c != '\n')
      current_col_number--;

    close_token_scanning(buffer, ptr, end);

    if (ptr == buffer)
      return 0; // Just "0"

    return std::strtol(buffer, nullptr, 8);
  }

  /**
   * @brief Load a binary number from the input stream
   *
   * Parses a number in binary format (0b or 0B prefix).
   *
   * @param input_stream The input file stream
   * @return The parsed value
   *
   * @throws ParseError if format is invalid
   *
   * @par Accepted formats
   * - `0b1010`
   * - `0B11110000`
   */
  inline long load_binary_number(std::ifstream & input_stream)
  {
    char buffer[Buffer_Size];
    char *ptr = buffer;
    char *end = buffer + Buffer_Size;

    init_token_scanning();
    skip_white_spaces(input_stream);

    int c = read_char_from_stream(input_stream);

    if (c != '0')
      throw ParseError("Binary number must start with 0b");

    c = read_char_from_stream(input_stream);
    if (c != 'b' and c != 'B')
      throw ParseError("Binary number must start with 0b");

    c = read_char_from_stream(input_stream);

    if (c != '0' and c != '1')
      throw ParseError("Invalid binary number");

    while (c == '0' or c == '1')
      {
        put_char_in_buffer(ptr, end, c);
        c = read_char_from_stream(input_stream);
      }

    input_stream.unget();
    if (c != '\n')
      current_col_number--;

    close_token_scanning(buffer, ptr, end);
    return std::strtol(buffer, nullptr, 2);
  }

  // ============================================================================
  // Identifier and Keyword Parsing
  // ============================================================================

  /**
   * @brief Load an identifier from the input stream
   *
   * Parses an identifier (variable name, function name, etc.).
   * Identifiers start with a letter or underscore, followed by
   * letters, digits, or underscores.
   *
   * @param input_stream The input file stream
   * @return The parsed identifier
   *
   * @throws ParseError if no valid identifier found
   *
   * @par Valid identifiers
   * - `foo`, `_bar`, `camelCase`, `snake_case`, `var123`
   */
  inline std::string load_identifier(std::ifstream & input_stream)
  {
    char buffer[Buffer_Size];
    char *ptr = buffer;
    char *end = buffer + Buffer_Size;

    init_token_scanning();
    skip_white_spaces(input_stream);

    int c = read_char_from_stream(input_stream);

    // Must start with letter or underscore
    if (not std::isalpha(c) and c != '_')
      throw ParseError("Identifier must start with letter or underscore");

    put_char_in_buffer(ptr, end, c);

    while (true)
      {
        c = read_char_from_stream(input_stream);

        if (std::isalnum(c) or c == '_')
          put_char_in_buffer(ptr, end, c);
        else
          break;
      }

    input_stream.unget();
    if (c != '\n')
      current_col_number--;

    close_token_scanning(buffer, ptr, end);
    return {buffer};
  }

  /**
   * @brief Check if a string is in a list of keywords
   *
   * @param[in] s The string to check
   * @param[in] keywords Vector of keywords to match against
   * @return true if s matches any keyword
   *
   * @par Example
   * @code
   * std::vector<std::string> kw = {"if", "while", "for", "return"};
   * if (is_keyword(identifier, kw)) {
   *   // Handle keyword
   * }
   * @endcode
   */
  inline bool is_keyword(const std::string & s, const std::vector<std::string> & keywords)
  {
    return std::find(keywords.begin(), keywords.end(), s) != keywords.end();
  }

  // ============================================================================
  // Expectation Functions
  // ============================================================================

  /**
   * @brief Expect and consume a specific character
   *
   * Reads the next non-whitespace character and verifies it matches
   * the expected character. If not, throws an error.
   *
   * @param input_stream The input file stream
   * @param expected The character that must appear next
   *
   * @throws ParseError if the next character doesn't match
   *
   * @par Example
   * @code
   * expect_char(input, '(');  // Must find opening paren
   * // ... parse expression ...
   * expect_char(input, ')');  // Must find closing paren
   * @endcode
   */
  inline void expect_char(std::ifstream & input_stream, char expected)
  {
    skip_white_spaces(input_stream);
    if (const int c = read_char_from_stream(input_stream); c != expected)
      {
        std::string msg = "Expected '";
        msg += expected;
        msg += "' but found '";
        msg += static_cast<char>(c);
        msg += "'";
        throw ParseError(msg);
      }
  }

  /**
   * @brief Expect and consume a specific string/keyword
   *
   * Reads characters and verifies they match the expected string.
   * If not, throws an error.
   *
   * @param input_stream The input file stream
   * @param expected The string that must appear next
   *
   * @throws ParseError if the input doesn't match
   *
   * @par Example
   * @code
   * expect(input, "function");
   * std::string name = load_identifier(input);
   * @endcode
   */
  inline void expect(std::ifstream & input_stream, const std::string & expected)
  {
    skip_white_spaces(input_stream);
    init_token_scanning();

    for (const char ch: expected)
      if (const int c = read_char_from_stream(input_stream); c != ch)
        throw ParseError("Expected '" + expected + "'");

    token_instance = expected;
  }

  /**
   * @brief Try to match a character without throwing
   *
   * Checks if the next non-whitespace character matches. If it does,
   * consumes it and returns true. Otherwise, returns false without
   * consuming anything.
   *
   * @param input_stream The input file stream
   * @param ch The character to try to match
   * @return true if matched and consumed, false otherwise
   */
  inline bool try_char(std::ifstream & input_stream, char ch)
  {
    StreamPosition pos = mark_position(input_stream);
    skip_white_spaces(input_stream);

    if (const int c = read_char_from_stream(input_stream); c == ch)
      return true;

    restore_position(input_stream, pos);
    return false;
  }

  // ============================================================================
  // String Parsing (Extended)
  // ============================================================================

  /**
   * @brief Process an escape sequence
   *
   * Given a character after a backslash, returns the corresponding
   * escaped character.
   *
   * @param c The character after the backslash
   * @return The unescaped character
   *
   * @par Supported escapes
   * - `\\n` → newline
   * - `\\t` → tab
   * - `\\r` → carriage return
   * - `\\\\` → backslash
   * - `\\\"` → double quote
   * - `\\'` → single quote
   * - `\\0` → null
   */
  inline char process_escape(int c)
  {
    switch (c)
      {
      case 'n': return '\n';
      case 't': return '\t';
      case 'r': return '\r';
      case '\\': return '\\';
      case '"': return '"';
      case '\'': return '\'';
      case '0': return '\0';
      case 'a': return '\a';
      case 'b': return '\b';
      case 'f': return '\f';
      case 'v': return '\v';
      default: return static_cast<char>(c); // Unknown escape, keep as-is
      }
  }

  /**
   * @brief Load a string with escape sequence processing
   *
   * Like load_string() but processes escape sequences like \\n, \\t, etc.
   * Only works with quoted strings.
   *
   * @param input_stream The input file stream
   * @return The parsed string with escapes processed
   *
   * @throws ParseError if string is unterminated
   *
   * @par Example
   * @code
   * // Input: "hello\nworld"
   * std::string s = load_escaped_string(input);
   * // s contains "hello" + newline + "world"
   * @endcode
   */
  inline std::string load_escaped_string(std::ifstream & input_stream)
  {
    char buffer[Buffer_Size];
    char *ptr = buffer;
    char *end = buffer + Buffer_Size;

    init_token_scanning();
    skip_white_spaces(input_stream);

    int c = read_char_from_stream(input_stream);

    if (c != '"')
      throw ParseError("Expected '\"' to start string");

    while (true)
      {
        c = read_char_from_stream(input_stream);

        if (c == EOF)
          throw ParseError("Unterminated string literal");

        if (c == '"')
          break;

        if (c == '\\')
          {
            c = read_char_from_stream(input_stream);
            if (c == EOF)
              throw ParseError("Unterminated escape sequence");
            c = process_escape(c);
          }

        put_char_in_buffer(ptr, end, c);
      }

    close_token_scanning(buffer, ptr, end);
    return {buffer};
  }

  /**
   * @brief Load a character literal
   *
   * Parses a character literal enclosed in single quotes.
   * Supports escape sequences.
   *
   * @param input_stream The input file stream
   * @return The parsed character
   *
   * @throws ParseError if format is invalid
   *
   * @par Accepted formats
   * - `'a'`
   * - `'\\n'`
   * - `'\\t'`
   */
  inline char load_char_literal(std::ifstream & input_stream)
  {
    init_token_scanning();
    skip_white_spaces(input_stream);

    int c = read_char_from_stream(input_stream);

    if (c != '\'')
      throw ParseError("Expected single quote to start character literal");

    c = read_char_from_stream(input_stream);

    if (c == '\\')
      {
        c = read_char_from_stream(input_stream);
        c = process_escape(c);
      }

    char result = static_cast<char>(c);

    c = read_char_from_stream(input_stream);
    if (c != '\'')
      throw ParseError("Expected single quote to end character literal");

    token_instance = "'";
    token_instance += result;
    token_instance += "'";

    return result;
  }

  // ============================================================================
  // Token Types (for lexer support)
  // ============================================================================

  /**
   * @brief Enumeration of basic token types
   */
  enum class TokenType
  {
    END_OF_FILE, ///< End of input
    IDENTIFIER, ///< Variable/function name
    INTEGER, ///< Integer literal
    FLOAT, ///< Floating-point literal
    STRING, ///< String literal
    CHAR, ///< Character literal
    OPERATOR, ///< Operator (+, -, etc.)
    PUNCTUATION, ///< Punctuation (;, {, }, etc.)
    KEYWORD, ///< Reserved keyword
    COMMENT, ///< Comment (if preserving)
    UNKNOWN ///< Unknown/invalid token
  };

  /**
   * @brief Convert TokenType to string for debugging
   */
  inline std::string token_type_to_string(TokenType type)
  {
    switch (type)
      {
      case TokenType::END_OF_FILE: return "EOF";
      case TokenType::IDENTIFIER: return "IDENTIFIER";
      case TokenType::INTEGER: return "INTEGER";
      case TokenType::FLOAT: return "FLOAT";
      case TokenType::STRING: return "STRING";
      case TokenType::CHAR: return "CHAR";
      case TokenType::OPERATOR: return "OPERATOR";
      case TokenType::PUNCTUATION: return "PUNCTUATION";
      case TokenType::KEYWORD: return "KEYWORD";
      case TokenType::COMMENT: return "COMMENT";
      case TokenType::UNKNOWN: return "UNKNOWN";
      default: return "???";
      }
  }

  /**
   * @brief Structure representing a lexical token
   */
  struct Token
  {
    TokenType type = TokenType::UNKNOWN; ///< Type of the token
    std::string value; ///< String value/content
    SourceLocation location; ///< Where token appeared

    /// Default constructor
    Token() = default;

    /// Construct with all fields
    Token(TokenType t, const std::string & v, const SourceLocation & loc)
      : type(t), value(v), location(loc) {}

    /// Check if this is an end-of-file token
    [[nodiscard]] bool is_eof() const { return type == TokenType::END_OF_FILE; }

    /// String representation for debugging
    [[nodiscard]] std::string to_string() const
    {
      return token_type_to_string(type) + "(\"" + value + "\") at " +
             location.to_string();
    }
  };

  // ============================================================================
  // File Utilities
  // ============================================================================

  /**
   * @brief Load entire file contents into a string
   *
   * @param filename Path to the file
   * @return Contents of the file as a string
   *
   * @throws std::runtime_error if file cannot be opened
   */
  inline std::string load_file_contents(const std::string & filename)
  {
    std::ifstream file(filename);
    if (! file.is_open())
      throw std::runtime_error("Cannot open file: " + filename);

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
  }

  /**
   * @brief Load file as a vector of lines
   *
   * @param filename Path to the file
   * @return Vector where each element is one line (without newline)
   *
   * @throws std::runtime_error if file cannot be opened
   */
  inline std::vector<std::string> load_file_lines(const std::string & filename)
  {
    std::ifstream file(filename);
    if (not file.is_open())
      throw std::runtime_error("Cannot open file: " + filename);

    std::vector<std::string> lines;
    std::string line;

    while (std::getline(file, line))
      lines.push_back(line);

    return lines;
  }

  /**
   * @brief Split a string by a delimiter
   *
   * @param str The string to split
   * @param delimiter Character to split on
   * @return Vector of substrings
   */
  inline std::vector<std::string> split_string(const std::string & str, char delimiter)
  {
    std::vector<std::string> result;
    std::istringstream iss(str);
    std::string token;

    while (std::getline(iss, token, delimiter))
      result.push_back(token);

    return result;
  }

  /// @brief Trim whitespace from both ends of a string
  ///
  /// Removes leading and trailing whitespace characters from the input string
  /// and returns a new trimmed string.
  ///
  /// @param[in] text The string to trim
  /// @return Trimmed string with leading and trailing whitespace removed
  inline std::string trim(const std::string & text)
  {
    size_t start = 0;
    while (start < text.size() and std::isspace(text[start]))
      start++;

    if (start == text.size())
      return "";

    size_t end = text.size() - 1;
    while (end > start and std::isspace(text[end]))
      end--;

    return text.substr(start, end - start + 1);
  }
} // namespace Aleph

// ============================================================================
// Global namespace compatibility (for existing code)
// ============================================================================

// Note: input_file_name is NOT exported to global namespace because
// many applications define their own variable with this name.
// Use Aleph::input_file_name if needed.

// Global state
using Aleph::Buffer_Size;
using Aleph::current_line_number;
using Aleph::current_col_number;
using Aleph::previous_line_number;
using Aleph::previous_col_number;
using Aleph::token_instance;

// Core functions
using Aleph::put_char_in_buffer;
using Aleph::init_token_scanning;
using Aleph::close_token_scanning;
using Aleph::read_char_from_stream;
using Aleph::skip_white_spaces;
using Aleph::load_number;
using Aleph::load_string;
using Aleph::print_parse_error_and_exit;
using Aleph::print_parse_warning;
using Aleph::command_line_to_string;
using Aleph::reset_parse_state;

// Source location and errors
using Aleph::SourceLocation;
using Aleph::ParseError;

// Lookahead and backtracking
using Aleph::StreamPosition;
using Aleph::peek_char;
using Aleph::mark_position;
using Aleph::restore_position;

// Comments
using Aleph::skip_line_comment;
using Aleph::skip_block_comment;
using Aleph::skip_whitespace_and_comments;

// Extended numeric parsing
using Aleph::load_double;
using Aleph::load_hex_number;
using Aleph::load_octal_number;
using Aleph::load_binary_number;

// Identifiers and keywords
using Aleph::load_identifier;
using Aleph::is_keyword;

// Expectations
using Aleph::expect_char;
using Aleph::expect;
using Aleph::try_char;

// Extended string parsing
using Aleph::process_escape;
using Aleph::load_escaped_string;
using Aleph::load_char_literal;

// Tokens
using Aleph::TokenType;
using Aleph::token_type_to_string;
using Aleph::Token;

// File utilities
using Aleph::load_file_contents;
using Aleph::load_file_lines;
using Aleph::split_string;
using Aleph::trim;

// ============================================================================
// Legacy Macro (preserved for backward compatibility)
// ============================================================================

/**
 * @brief Macro for printing detailed parse errors
 * 
 * Prints the file name, position, and last token before calling
 * the AH_ERROR macro.
 *
 * @param str Error format string
 * @param args Optional format arguments
 *
 * @note Requires `input_file_name` (std::string) to be defined in the
 *       application's scope before use.
 *
 * @deprecated Prefer using print_parse_error_and_exit() or exceptions
 */
#define PRINT_ERROR(str, args...) (                                          \
  (std::cout << input_file_name << "("                                       \
             << Aleph::previous_line_number << ","                           \
             << Aleph::previous_col_number << "): " << '\n'),                \
  (std::cout << "Last token: " << Aleph::token_instance << '\n'),            \
  AH_ERROR(str, ##args))

#endif // PARSE_UTILS_H
