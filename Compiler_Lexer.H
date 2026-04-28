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

/** @file Compiler_Lexer.H
 *  @brief Lexical analyzer (Lexer) based on `Source_Manager`.
 *
 *  The lexer is the first component of the front-end. Its mission is to transform
 *  a stream of characters from a source file into a sequence of `Compiler_Token`
 *  objects with semantic meaning.
 *
 *  ### Responsibilities:
 *  - Skip whitespace and (optionally) comments.
 *  - Classify identifiers and keywords.
 *  - Parse numeric, string, and character literals, managing escape sequences.
 *  - Emit readable diagnostics for lexical errors (e.g., unterminated strings).
 *
 *  ### Typical Usage:
 *  @code
 *  Compiler_Lexer lexer(source_manager, file_id, diag_engine);
 *  while (!lexer.eof()) {
 *      Compiler_Token tok = lexer.next();
 *      // Process token...
 *  }
 *  @endcode
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_LEXER_H
#define COMPILER_LEXER_H

#include <cctype>
#include <string>

#include <Compiler_Token.H>
#include <ah-diagnostics.H>

namespace Aleph {

/** @brief Configuration options for lexer behavior.
 *
 *  Allows adjusting which parts of the source code are ignored and which
 *  comment rules apply.
 *
 *  @note Thread-safety: Thread-safe (POD).
 *  @note Complexity: O(1).
 */
struct Compiler_Lexer_Options
{
  bool keep_comments = false;        ///< If true, comments are returned as tokens.
  bool allow_block_comments = true;  ///< If false, `/*` is treated as `/` followed by `*`.
};

/** @brief Token generator for a single source file.
 *
 *  Maintains the reading state (cursor) and provides a "consumption" interface
 *  for logical symbols.
 *
 *  @note Thread-safety: Not thread-safe. Each file should have its own lexer
 *        instance if processed in parallel.
 */
class Compiler_Lexer
{
  const Source_Manager *sources_ = nullptr;
  Source_File_Id file_id_ = 0;
  const std::string *input_ = nullptr;
  Diagnostic_Engine *diagnostics_ = nullptr;
  Compiler_Lexer_Options options_;
  Source_Offset cursor_ = 0;
  bool has_lookahead_ = false;
  Compiler_Token lookahead_;

  /** @brief Checks if a character can start an identifier. */
  static bool is_ident_start(const unsigned char ch) noexcept
  {
    return std::isalpha(ch) or ch == '_';
  }

  /** @brief Checks if a character can continue an identifier. */
  static bool is_ident_continue(const unsigned char ch) noexcept
  {
    return std::isalnum(ch) or ch == '_';
  }

  /** @brief Returns true if the cursor has reached the end of the input. */
  [[nodiscard]] bool at_end() const noexcept
  {
    return cursor_ >= input_->size();
  }

  /** @brief Safe character access at a given offset. */
  [[nodiscard]] char char_at(const Source_Offset off) const noexcept
  {
    return off < input_->size() ? (*input_)[off] : '\0';
  }

  /** @brief Returns the character at the current cursor position. */
  [[nodiscard]] char current_char() const noexcept
  {
    return char_at(cursor_);
  }

  /** @brief Returns the character following the current cursor position. */
  [[nodiscard]] char next_char() const noexcept
  {
    return char_at(cursor_ + 1);
  }

  /** @brief Creates a source span for the given range in the current file. */
  [[nodiscard]] Source_Span make_span(const Source_Offset begin, const Source_Offset end) const
  {
    return sources_->span(file_id_, begin, end);
  }

  /** @brief Creates a token with the given kind and range. */
  [[nodiscard]] Compiler_Token make_token(const Compiler_Token_Kind kind,
                                          const Source_Offset begin,
                                          const Source_Offset end) const
  {
    const auto sp = make_span(begin, end);
    return {kind, sources_->slice(sp), sp};
  }

  /** @brief Creates an End_Of_File token at the current position. */
  [[nodiscard]] Compiler_Token make_eof_token() const
  {
    const auto sp = make_span(cursor_, cursor_);
    return {Compiler_Token_Kind::End_Of_File, "", sp};
  }

  /** @brief Emits a diagnostic error through the engine. */
  void emit_error(const Source_Span &span,
                  const std::string &message,
                  const std::string &code,
                  const std::string &note = "",
                  const std::string &help = "") const
  {
    if (diagnostics_ == nullptr)
      return;

    auto builder = diagnostics_->error(span, message).code(code);
    if (not note.empty())
      builder.note(note);
    if (not help.empty())
      builder.help(help);
    builder.emit();
  }

  /** @brief Consumes whitespace characters. */
  void skip_whitespace()
  {
    while (not at_end() and std::isspace(static_cast<unsigned char>(current_char())))
      ++cursor_;
  }

  /** @brief Lexes a single-line comment. */
  Compiler_Token lex_line_comment()
  {
    const Source_Offset begin = cursor_;
    cursor_ += 2;
    while (not at_end() and current_char() != '\n')
      ++cursor_;
    return make_token(Compiler_Token_Kind::Line_Comment, begin, cursor_);
  }

  /** @brief Lexes a multi-line block comment. */
  Compiler_Token lex_block_comment()
  {
    const Source_Offset begin = cursor_;
    cursor_ += 2;

    while (not at_end())
      {
        if (current_char() == '*' and next_char() == '/')
          {
            cursor_ += 2;
            return make_token(Compiler_Token_Kind::Block_Comment, begin, cursor_);
          }
        ++cursor_;
      }

    const auto sp = make_span(begin, cursor_);
    emit_error(sp, "unterminated block comment", "LEX004", "block comments must end with '*/'");
    return {Compiler_Token_Kind::Invalid, sources_->slice(sp), sp};
  }

  /** @brief Lexes an identifier or classifies it as a keyword. */
  Compiler_Token lex_identifier_or_keyword()
  {
    const Source_Offset begin = cursor_;
    ++cursor_;
    while (not at_end() and is_ident_continue(static_cast<unsigned char>(current_char())))
      ++cursor_;

    const auto sp = make_span(begin, cursor_);
    const auto lexeme = sources_->slice(sp);
    return {classify_compiler_keyword(lexeme), lexeme, sp};
  }

  /** @brief Lexes a numeric literal. */
  Compiler_Token lex_number()
  {
    const Source_Offset begin = cursor_;
    ++cursor_;
    while (not at_end())
      {
        const auto ch = static_cast<unsigned char>(current_char());
        if (std::isdigit(ch))
          {
            ++cursor_;
            continue;
          }

        if (ch == '_' and std::isdigit(static_cast<unsigned char>(next_char())))
          {
            ++cursor_;
            continue;
          }

        break;
      }

    return make_token(Compiler_Token_Kind::Integer_Literal, begin, cursor_);
  }

  /** @brief Lexes a string literal. */
  Compiler_Token lex_string()
  {
    const Source_Offset begin = cursor_;
    ++cursor_;

    while (not at_end())
      {
        const char ch = current_char();
        if (ch == '"')
          {
            ++cursor_;
            return make_token(Compiler_Token_Kind::String_Literal, begin, cursor_);
          }

        if (ch == '\\')
          {
            ++cursor_;
            if (at_end())
              break;
            ++cursor_;
            continue;
          }

        if (ch == '\n' or ch == '\r')
          break;

        ++cursor_;
      }

    const auto sp = make_span(begin, cursor_);
    emit_error(sp,
               "unterminated string literal",
               "LEX002",
               "string literal started here",
               "close the string with a double quote");
    return {Compiler_Token_Kind::Invalid, sources_->slice(sp), sp};
  }

  /** @brief Lexes a character literal. */
  Compiler_Token lex_char_literal()
  {
    const Source_Offset begin = cursor_;
    ++cursor_;

    if (at_end() or current_char() == '\n' or current_char() == '\r')
      {
        const auto sp = make_span(begin, cursor_);
        emit_error(sp, "unterminated character literal", "LEX003", "character literal started here");
        return {Compiler_Token_Kind::Invalid, sources_->slice(sp), sp};
      }

    if (current_char() == '\\')
      {
        ++cursor_;
        if (at_end() or current_char() == '\n' or current_char() == '\r')
          {
            const auto sp = make_span(begin, cursor_);
            emit_error(sp, "unterminated escape sequence in character literal", "LEX003");
            return {Compiler_Token_Kind::Invalid, sources_->slice(sp), sp};
          }
        ++cursor_;
      }
    else
      ++cursor_;

    if (not at_end() and current_char() == '\'')
      {
        ++cursor_;
        return make_token(Compiler_Token_Kind::Char_Literal, begin, cursor_);
      }

    while (not at_end() and current_char() != '\'' and current_char() != '\n'
           and current_char() != '\r')
      ++cursor_;

    if (not at_end() and current_char() == '\'')
      ++cursor_;

    const auto sp = make_span(begin, cursor_);
    emit_error(sp,
               "malformed character literal",
               "LEX003",
               "character literals must contain exactly one character or escape");
    return {Compiler_Token_Kind::Invalid, sources_->slice(sp), sp};
  }

  /** @brief Lexes punctuation or operator symbols. */
  Compiler_Token lex_punctuation_or_operator()
  {
    const Source_Offset begin = cursor_;
    const char a = current_char();
    const char b = next_char();

    // Multicharacter operators/punctuation
    if (a == ':' and b == ':')
      {
        cursor_ += 2;
        return make_token(Compiler_Token_Kind::Double_Colon, begin, cursor_);
      }
    if (a == '-' and b == '>')
      {
        cursor_ += 2;
        return make_token(Compiler_Token_Kind::Arrow, begin, cursor_);
      }
    if (a == '=' and b == '=')
      {
        cursor_ += 2;
        return make_token(Compiler_Token_Kind::EqEq, begin, cursor_);
      }
    if (a == '!' and b == '=')
      {
        cursor_ += 2;
        return make_token(Compiler_Token_Kind::NotEq, begin, cursor_);
      }
    if (a == '<' and b == '=')
      {
        cursor_ += 2;
        return make_token(Compiler_Token_Kind::LessEq, begin, cursor_);
      }
    if (a == '>' and b == '=')
      {
        cursor_ += 2;
        return make_token(Compiler_Token_Kind::GreaterEq, begin, cursor_);
      }
    if (a == '&' and b == '&')
      {
        cursor_ += 2;
        return make_token(Compiler_Token_Kind::AndAnd, begin, cursor_);
      }
    if (a == '|' and b == '|')
      {
        cursor_ += 2;
        return make_token(Compiler_Token_Kind::OrOr, begin, cursor_);
      }
    if (a == '+' and b == '=')
      {
        cursor_ += 2;
        return make_token(Compiler_Token_Kind::PlusEq, begin, cursor_);
      }
    if (a == '-' and b == '=')
      {
        cursor_ += 2;
        return make_token(Compiler_Token_Kind::MinusEq, begin, cursor_);
      }
    if (a == '*' and b == '=')
      {
        cursor_ += 2;
        return make_token(Compiler_Token_Kind::StarEq, begin, cursor_);
      }
    if (a == '/' and b == '=')
      {
        cursor_ += 2;
        return make_token(Compiler_Token_Kind::SlashEq, begin, cursor_);
      }
    if (a == '%' and b == '=')
      {
        cursor_ += 2;
        return make_token(Compiler_Token_Kind::PercentEq, begin, cursor_);
      }
    if (a == '+' and b == '+')
      {
        cursor_ += 2;
        return make_token(Compiler_Token_Kind::PlusPlus, begin, cursor_);
      }
    if (a == '-' and b == '-')
      {
        cursor_ += 2;
        return make_token(Compiler_Token_Kind::MinusMinus, begin, cursor_);
      }

    ++cursor_;
    switch (a)
      {
      case '(':
        return make_token(Compiler_Token_Kind::LParen, begin, cursor_);
      case ')':
        return make_token(Compiler_Token_Kind::RParen, begin, cursor_);
      case '{':
        return make_token(Compiler_Token_Kind::LBrace, begin, cursor_);
      case '}':
        return make_token(Compiler_Token_Kind::RBrace, begin, cursor_);
      case '[':
        return make_token(Compiler_Token_Kind::LBracket, begin, cursor_);
      case ']':
        return make_token(Compiler_Token_Kind::RBracket, begin, cursor_);
      case ',':
        return make_token(Compiler_Token_Kind::Comma, begin, cursor_);
      case '.':
        return make_token(Compiler_Token_Kind::Dot, begin, cursor_);
      case ':':
        return make_token(Compiler_Token_Kind::Colon, begin, cursor_);
      case ';':
        return make_token(Compiler_Token_Kind::Semicolon, begin, cursor_);
      case '?':
        return make_token(Compiler_Token_Kind::Question, begin, cursor_);
      case '=':
        return make_token(Compiler_Token_Kind::Assign, begin, cursor_);
      case '+':
        return make_token(Compiler_Token_Kind::Plus, begin, cursor_);
      case '-':
        return make_token(Compiler_Token_Kind::Minus, begin, cursor_);
      case '*':
        return make_token(Compiler_Token_Kind::Star, begin, cursor_);
      case '/':
        return make_token(Compiler_Token_Kind::Slash, begin, cursor_);
      case '%':
        return make_token(Compiler_Token_Kind::Percent, begin, cursor_);
      case '&':
        return make_token(Compiler_Token_Kind::Amp, begin, cursor_);
      case '|':
        return make_token(Compiler_Token_Kind::Pipe, begin, cursor_);
      case '^':
        return make_token(Compiler_Token_Kind::Caret, begin, cursor_);
      case '!':
        return make_token(Compiler_Token_Kind::Bang, begin, cursor_);
      case '~':
        return make_token(Compiler_Token_Kind::Tilde, begin, cursor_);
      case '<':
        return make_token(Compiler_Token_Kind::Less, begin, cursor_);
      case '>':
        return make_token(Compiler_Token_Kind::Greater, begin, cursor_);
      default:
        {
          const auto sp = make_span(begin, cursor_);
          emit_error(sp, std::string("unexpected character '") + a + "'", "LEX001");
          return {Compiler_Token_Kind::Invalid, sources_->slice(sp), sp};
        }
      }
  }

  /** @brief Dispatches the next token from the input stream. */
  Compiler_Token lex_token()
  {
    while (true)
      {
        skip_whitespace();
        if (at_end())
          return make_eof_token();

        if (current_char() == '/' and next_char() == '/')
          {
            if (auto tok = lex_line_comment(); options_.keep_comments or tok.is_invalid())
              return tok;
            continue;
          }

        if (options_.allow_block_comments and current_char() == '/' and next_char() == '*')
          {
            if (auto tok = lex_block_comment(); options_.keep_comments or tok.is_invalid())
              return tok;
            continue;
          }

        break;
      }

    const auto ch = static_cast<unsigned char>(current_char());
    if (is_ident_start(ch))
      return lex_identifier_or_keyword();
    if (std::isdigit(ch))
      return lex_number();
    if (current_char() == '"')
      return lex_string();
    if (current_char() == '\'')
      return lex_char_literal();
    return lex_punctuation_or_operator();
  }

public:
  /** @brief Initializes a lexer for a registered source file.
   *
   *  @param sm Source manager containing the file text.
   *  @param id Identifier of the file to tokenize.
   *  @param dx Diagnostic engine to report errors. If null, errors are only
   *         indicated via `Invalid` tokens.
   *  @param opts Trivia handling options (comments, etc.).
   *  @throws std::out_of_range If `id` is not valid in `sm`.
   *  @note Thread-safety: Not thread-safe.
   *  @note Complexity: O(1).
   */
  Compiler_Lexer(const Source_Manager &sm,
                 const Source_File_Id id,
                 Diagnostic_Engine *dx = nullptr,
                 const Compiler_Lexer_Options &opts = {})
    : sources_(&sm), file_id_(id), input_(&sm.file_text(id)), diagnostics_(dx), options_(opts)
  {}

  /** @brief Returns the file ID currently being analyzed.
   *
   *  @return The stable ID associated with this lexer.
   *  @note Thread-safety: Safe for concurrent readers.
   *  @note Complexity: O(1).
   */
  [[nodiscard]] Source_File_Id source_file_id() const noexcept
  {
    return file_id_;
  }

  /** @brief Returns the current byte offset in the file.
   *
   *  Useful for saving recovery states or for telemetry.
   *
   *  @return Cursor position in bytes.
   *  @note Thread-safety: Safe for concurrent readers.
   *  @note Complexity: O(1).
   */
  [[nodiscard]] Source_Offset current_offset() const noexcept
  {
    return cursor_;
  }

  /** @brief Repositions the reading cursor.
   *
   *  Discards any pre-read tokens (lookahead) and restarts the analysis
   *  from the indicated position.
   *
   *  @param offset New byte offset (default 0, the beginning).
   *  @throws std::out_of_range If the offset exceeds the file length.
   *  @note Thread-safety: Not thread-safe.
   *  @note Complexity: O(1).
   */
  void reset(const Source_Offset offset = 0)
  {
    ah_out_of_range_error_unless(offset <= input_->size())
      << "Compiler_Lexer::reset(): invalid offset " << offset;
    cursor_ = offset;
    has_lookahead_ = false;
    lookahead_ = Compiler_Token();
  }

  /** @brief Peeks at the next token without advancing the cursor.
   *
   *  Allows the parser to make syntactic decisions based on the next symbol
   *  without consuming it permanently. Successive calls return the same token.
   *
   *  @return The next available token in the stream.
   *  @note Thread-safety: Not thread-safe.
   *  @note Complexity: O(N) where N is lexeme length (amortized).
   */
  const Compiler_Token &peek()
  {
    if (not has_lookahead_)
      {
        lookahead_ = lex_token();
        has_lookahead_ = true;
      }
    return lookahead_;
  }

  /** @brief Consumes and returns the next token from the stream.
   *
   *  Advances the internal cursor. If there is a lexical error, returns an
   *  `Invalid` token but continues advancing to attempt recovery.
   *
   *  @return The consumed token.
   *  @note Thread-safety: Not thread-safe.
   *  @note Complexity: O(N) where N is lexeme length (amortized).
   */
  Compiler_Token next()
  {
    if (has_lookahead_)
      {
        has_lookahead_ = false;
        return lookahead_;
      }
    return lex_token();
  }

  /** @brief Indicates whether the end of the file has been reached.
   *
   *  @return `true` if the next call to `next()` would return `End_Of_File`.
   *  @note Thread-safety: Not thread-safe.
   *  @note Complexity: O(N) (calls peek internally).
   */
  bool eof()
  {
    return peek().is_eof();
  }
};
}  // namespace Aleph

#endif  // COMPILER_LEXER_H
