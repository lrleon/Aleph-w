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
 *  @brief Lexer for the compiler-support MVP built on `Source_Manager`.
 *
 *  The lexer reads one registered source file and produces tokens with stable
 *  spans. It supports identifiers, reserved keywords, integer/string/character
 *  literals, punctuation, common operators, and line/block comments.
 *
 *  Whitespace is always skipped. Comment tokens are skipped by default but can
 *  be preserved with `Compiler_Lexer_Options::keep_comments`.
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
/** @brief Configures lexer trivia handling. */
struct Compiler_Lexer_Options
{
  bool keep_comments = false;        ///< If true, comments are returned as tokens.
  bool allow_block_comments = true;  ///< If false, `/*` becomes `/` then `*`.
};

/** @brief Single-file lexer with optional diagnostic emission. */
class Compiler_Lexer
{
  const Source_Manager *sources = nullptr;
  Source_File_Id file_id = 0;
  const std::string *input = nullptr;
  Diagnostic_Engine *diagnostics = nullptr;
  Compiler_Lexer_Options options;
  Source_Offset cursor = 0;
  bool has_lookahead = false;
  Compiler_Token lookahead;

  static bool is_ident_start(const unsigned char ch) noexcept
  {
    return std::isalpha(ch) or ch == '_';
  }

  static bool is_ident_continue(const unsigned char ch) noexcept
  {
    return std::isalnum(ch) or ch == '_';
  }

  [[nodiscard]] bool at_end() const noexcept
  {
    return cursor >= input->size();
  }

  [[nodiscard]] char char_at(const Source_Offset off) const noexcept
  {
    return off < input->size() ? (*input)[off] : '\0';
  }

  [[nodiscard]] char current_char() const noexcept
  {
    return char_at(cursor);
  }

  [[nodiscard]] char next_char() const noexcept
  {
    return char_at(cursor + 1);
  }

  [[nodiscard]] Source_Span make_span(const Source_Offset begin, const Source_Offset end) const
  {
    return sources->span(file_id, begin, end);
  }

  [[nodiscard]] Compiler_Token make_token(const Compiler_Token_Kind kind,
                                          const Source_Offset begin,
                                          const Source_Offset end) const
  {
    const auto sp = make_span(begin, end);
    return {kind, sources->slice(sp), sp};
  }

  [[nodiscard]] Compiler_Token make_eof_token() const
  {
    const auto sp = make_span(cursor, cursor);
    return {Compiler_Token_Kind::End_Of_File, "", sp};
  }

  void emit_error(const Source_Span &span,
                  const std::string &message,
                  const std::string &code,
                  const std::string &note = "",
                  const std::string &help = "") const
  {
    if (diagnostics == nullptr)
      return;

    auto builder = diagnostics->error(span, message).code(code);
    if (not note.empty())
      builder.note(note);
    if (not help.empty())
      builder.help(help);
    builder.emit();
  }

  void skip_whitespace()
  {
    while (not at_end() and std::isspace(static_cast<unsigned char>(current_char())))
      ++cursor;
  }

  Compiler_Token lex_line_comment()
  {
    const Source_Offset begin = cursor;
    cursor += 2;
    while (not at_end() and current_char() != '\n')
      ++cursor;
    return make_token(Compiler_Token_Kind::Line_Comment, begin, cursor);
  }

  Compiler_Token lex_block_comment()
  {
    const Source_Offset begin = cursor;
    cursor += 2;

    while (not at_end())
      {
        if (current_char() == '*' and next_char() == '/')
          {
            cursor += 2;
            return make_token(Compiler_Token_Kind::Block_Comment, begin, cursor);
          }
        ++cursor;
      }

    const auto sp = make_span(begin, cursor);
    emit_error(sp, "unterminated block comment", "LEX004", "block comments must end with '*/'");
    return {Compiler_Token_Kind::Invalid, sources->slice(sp), sp};
  }

  Compiler_Token lex_identifier_or_keyword()
  {
    const Source_Offset begin = cursor;
    ++cursor;
    while (not at_end() and is_ident_continue(static_cast<unsigned char>(current_char())))
      ++cursor;

    const auto sp = make_span(begin, cursor);
    const auto lexeme = sources->slice(sp);
    return {classify_compiler_keyword(lexeme), lexeme, sp};
  }

  Compiler_Token lex_number()
  {
    const Source_Offset begin = cursor;
    ++cursor;
    while (not at_end())
      {
        const auto ch = static_cast<unsigned char>(current_char());
        if (std::isdigit(ch))
          {
            ++cursor;
            continue;
          }

        if (ch == '_' and std::isdigit(static_cast<unsigned char>(next_char())))
          {
            ++cursor;
            continue;
          }

        break;
      }

    return make_token(Compiler_Token_Kind::Integer_Literal, begin, cursor);
  }

  Compiler_Token lex_string()
  {
    const Source_Offset begin = cursor;
    ++cursor;

    while (not at_end())
      {
        const char ch = current_char();
        if (ch == '"')
          {
            ++cursor;
            return make_token(Compiler_Token_Kind::String_Literal, begin, cursor);
          }

        if (ch == '\\')
          {
            ++cursor;
            if (at_end())
              break;
            ++cursor;
            continue;
          }

        if (ch == '\n' or ch == '\r')
          break;

        ++cursor;
      }

    const auto sp = make_span(begin, cursor);
    emit_error(sp,
               "unterminated string literal",
               "LEX002",
               "string literal started here",
               "close the string with a double quote");
    return {Compiler_Token_Kind::Invalid, sources->slice(sp), sp};
  }

  Compiler_Token lex_char_literal()
  {
    const Source_Offset begin = cursor;
    ++cursor;

    if (at_end() or current_char() == '\n' or current_char() == '\r')
      {
        const auto sp = make_span(begin, cursor);
        emit_error(sp, "unterminated character literal", "LEX003", "character literal started here");
        return {Compiler_Token_Kind::Invalid, sources->slice(sp), sp};
      }

    if (current_char() == '\\')
      {
        ++cursor;
        if (at_end() or current_char() == '\n' or current_char() == '\r')
          {
            const auto sp = make_span(begin, cursor);
            emit_error(sp, "unterminated escape sequence in character literal", "LEX003");
            return {Compiler_Token_Kind::Invalid, sources->slice(sp), sp};
          }
        ++cursor;
      }
    else
      ++cursor;

    if (not at_end() and current_char() == '\'')
      {
        ++cursor;
        return make_token(Compiler_Token_Kind::Char_Literal, begin, cursor);
      }

    while (not at_end() and current_char() != '\'' and current_char() != '\n'
           and current_char() != '\r')
      ++cursor;

    if (not at_end() and current_char() == '\'')
      ++cursor;

    const auto sp = make_span(begin, cursor);
    emit_error(sp,
               "malformed character literal",
               "LEX003",
               "character literals must contain exactly one character or escape");
    return {Compiler_Token_Kind::Invalid, sources->slice(sp), sp};
  }

  Compiler_Token lex_punctuation_or_operator()
  {
    const Source_Offset begin = cursor;
    const char a = current_char();
    const char b = next_char();

    if (a == ':' and b == ':')
      {
        cursor += 2;
        return make_token(Compiler_Token_Kind::Double_Colon, begin, cursor);
      }
    if (a == '-' and b == '>')
      {
        cursor += 2;
        return make_token(Compiler_Token_Kind::Arrow, begin, cursor);
      }
    if (a == '=' and b == '=')
      {
        cursor += 2;
        return make_token(Compiler_Token_Kind::EqEq, begin, cursor);
      }
    if (a == '!' and b == '=')
      {
        cursor += 2;
        return make_token(Compiler_Token_Kind::NotEq, begin, cursor);
      }
    if (a == '<' and b == '=')
      {
        cursor += 2;
        return make_token(Compiler_Token_Kind::LessEq, begin, cursor);
      }
    if (a == '>' and b == '=')
      {
        cursor += 2;
        return make_token(Compiler_Token_Kind::GreaterEq, begin, cursor);
      }
    if (a == '&' and b == '&')
      {
        cursor += 2;
        return make_token(Compiler_Token_Kind::AndAnd, begin, cursor);
      }
    if (a == '|' and b == '|')
      {
        cursor += 2;
        return make_token(Compiler_Token_Kind::OrOr, begin, cursor);
      }
    if (a == '+' and b == '=')
      {
        cursor += 2;
        return make_token(Compiler_Token_Kind::PlusEq, begin, cursor);
      }
    if (a == '-' and b == '=')
      {
        cursor += 2;
        return make_token(Compiler_Token_Kind::MinusEq, begin, cursor);
      }
    if (a == '*' and b == '=')
      {
        cursor += 2;
        return make_token(Compiler_Token_Kind::StarEq, begin, cursor);
      }
    if (a == '/' and b == '=')
      {
        cursor += 2;
        return make_token(Compiler_Token_Kind::SlashEq, begin, cursor);
      }
    if (a == '%' and b == '=')
      {
        cursor += 2;
        return make_token(Compiler_Token_Kind::PercentEq, begin, cursor);
      }
    if (a == '+' and b == '+')
      {
        cursor += 2;
        return make_token(Compiler_Token_Kind::PlusPlus, begin, cursor);
      }
    if (a == '-' and b == '-')
      {
        cursor += 2;
        return make_token(Compiler_Token_Kind::MinusMinus, begin, cursor);
      }

    ++cursor;
    switch (a)
      {
      case '(':
        return make_token(Compiler_Token_Kind::LParen, begin, cursor);
      case ')':
        return make_token(Compiler_Token_Kind::RParen, begin, cursor);
      case '{':
        return make_token(Compiler_Token_Kind::LBrace, begin, cursor);
      case '}':
        return make_token(Compiler_Token_Kind::RBrace, begin, cursor);
      case '[':
        return make_token(Compiler_Token_Kind::LBracket, begin, cursor);
      case ']':
        return make_token(Compiler_Token_Kind::RBracket, begin, cursor);
      case ',':
        return make_token(Compiler_Token_Kind::Comma, begin, cursor);
      case '.':
        return make_token(Compiler_Token_Kind::Dot, begin, cursor);
      case ':':
        return make_token(Compiler_Token_Kind::Colon, begin, cursor);
      case ';':
        return make_token(Compiler_Token_Kind::Semicolon, begin, cursor);
      case '?':
        return make_token(Compiler_Token_Kind::Question, begin, cursor);
      case '=':
        return make_token(Compiler_Token_Kind::Assign, begin, cursor);
      case '+':
        return make_token(Compiler_Token_Kind::Plus, begin, cursor);
      case '-':
        return make_token(Compiler_Token_Kind::Minus, begin, cursor);
      case '*':
        return make_token(Compiler_Token_Kind::Star, begin, cursor);
      case '/':
        return make_token(Compiler_Token_Kind::Slash, begin, cursor);
      case '%':
        return make_token(Compiler_Token_Kind::Percent, begin, cursor);
      case '&':
        return make_token(Compiler_Token_Kind::Amp, begin, cursor);
      case '|':
        return make_token(Compiler_Token_Kind::Pipe, begin, cursor);
      case '^':
        return make_token(Compiler_Token_Kind::Caret, begin, cursor);
      case '!':
        return make_token(Compiler_Token_Kind::Bang, begin, cursor);
      case '~':
        return make_token(Compiler_Token_Kind::Tilde, begin, cursor);
      case '<':
        return make_token(Compiler_Token_Kind::Less, begin, cursor);
      case '>':
        return make_token(Compiler_Token_Kind::Greater, begin, cursor);
        default: {
          const auto sp = make_span(begin, cursor);
          emit_error(sp, std::string("unexpected character '") + a + "'", "LEX001");
          return {Compiler_Token_Kind::Invalid, sources->slice(sp), sp};
        }
      }
  }

  Compiler_Token lex_token()
  {
    while (true)
      {
        skip_whitespace();
        if (at_end())
          return make_eof_token();

        if (current_char() == '/' and next_char() == '/')
          {
            auto tok = lex_line_comment();
            if (options.keep_comments or tok.is_invalid())
              return tok;
            continue;
          }

        if (options.allow_block_comments and current_char() == '/' and next_char() == '*')
          {
            auto tok = lex_block_comment();
            if (options.keep_comments or tok.is_invalid())
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
  /** @brief Constructs a lexer for a registered source file.
   *
   *  @param sm Source manager owning the file text.
   *  @param id Registered file id to tokenize.
   *  @param dx Optional diagnostic sink. Unterminated literals and unexpected
   *         characters are still reported through invalid tokens when this is
   *         null, but no human-readable diagnostics are accumulated.
   *  @param opts Trivia-handling options for comments.
   *  @throws std::out_of_range If `id` does not belong to `sm`.
   */
  Compiler_Lexer(const Source_Manager &sm,
                 const Source_File_Id id,
                 Diagnostic_Engine *dx = nullptr,
                 const Compiler_Lexer_Options &opts = {})
    : sources(&sm), file_id(id), input(&sm.file_text(id)), diagnostics(dx), options(opts)
  {}

  /** @brief Returns the file id currently being tokenized.
   *
   *  @return Stable file id associated with this lexer instance.
   */
  [[nodiscard]] Source_File_Id source_file_id() const noexcept
  {
    return file_id;
  }

  /** @brief Returns the current byte cursor.
   *
   *  @return Byte offset in the current source file.
   */
  [[nodiscard]] Source_Offset current_offset() const noexcept
  {
    return cursor;
  }

  /** @brief Resets the lexer to `offset` and clears cached lookahead.
   *
   *  @param offset New byte cursor.
   *  @throws std::out_of_range If `offset` is past the end of the file.
   */
  void reset(const Source_Offset offset = 0)
  {
    ah_out_of_range_error_unless(offset <= input->size())
      << "Compiler_Lexer::reset(): invalid offset " << offset;
    cursor = offset;
    has_lookahead = false;
    lookahead = Compiler_Token();
  }

  /** @brief Returns the next token without consuming it.
   *
   *  @return The same token that would be returned by the next call to
   *          `next()`.
   */
  Compiler_Token peek()
  {
    if (not has_lookahead)
      {
        lookahead = lex_token();
        has_lookahead = true;
      }
    return lookahead;
  }

  /** @brief Consumes and returns the next token.
   *
   *  @return Next token from the source stream, including invalid tokens for
   *          malformed lexemes.
   */
  Compiler_Token next()
  {
    if (has_lookahead)
      {
        has_lookahead = false;
        return lookahead;
      }
    return lex_token();
  }

  /** @brief Returns whether the next token is EOF.
   *
   *  @return `true` iff `peek()` would produce `End_Of_File`.
   */
  bool eof()
  {
    return peek().is_eof();
  }
};
}  // namespace Aleph

#endif  // COMPILER_LEXER_H
