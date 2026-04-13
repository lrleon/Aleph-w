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

/** @file Compiler_Token.H
 *  @brief Token model and parser-facing metadata for compiler front-ends.
 *
 *  This header provides a token kind enumeration, a token value object carrying
 *  raw lexeme and source span, keyword classification, and basic binary
 *  operator precedence metadata suitable for future Pratt or precedence-climbing
 *  parsers.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_TOKEN_H
#define COMPILER_TOKEN_H

#include <sstream>
#include <string>
#include <string_view>

#include <ah-source.H>

namespace Aleph {
/** @brief Associativity metadata for binary operators. 
 *
 *  @note Thread-safety: Thread-safe (POD).
 *  @note Complexity: O(1).
 */
enum class Compiler_Associativity
{
  None,
  Left,
  Right
};

/** @brief Token kinds supported by the compiler-support MVP lexer/parser.
 *
 *  Keywords use dedicated enumerators rather than a generic keyword token so
 *  parsers can dispatch on the token kind directly.
 *
 *  @note Thread-safety: Thread-safe (POD).
 *  @note Complexity: O(1).
 */
enum class Compiler_Token_Kind
{
  End_Of_File,
  Invalid,

  Identifier,
  Integer_Literal,
  String_Literal,
  Char_Literal,

  Line_Comment,
  Block_Comment,

  Kw_Fn,
  Kw_Let,
  Kw_If,
  Kw_Else,
  Kw_While,
  Kw_For,
  Kw_Return,
  Kw_True,
  Kw_False,
  Kw_Struct,
  Kw_Enum,
  Kw_Import,
  Kw_Break,
  Kw_Continue,

  LParen,
  RParen,
  LBrace,
  RBrace,
  LBracket,
  RBracket,
  Comma,
  Dot,
  Colon,
  Double_Colon,
  Semicolon,
  Question,
  Arrow,

  Assign,
  Plus,
  Minus,
  Star,
  Slash,
  Percent,
  Amp,
  Pipe,
  Caret,
  Bang,
  Tilde,
  Less,
  Greater,

  EqEq,
  NotEq,
  LessEq,
  GreaterEq,
  AndAnd,
  OrOr,
  PlusEq,
  MinusEq,
  StarEq,
  SlashEq,
  PercentEq,
  PlusPlus,
  MinusMinus
};

/** @brief Returns a stable string name for a token kind.
 *
 *  @param kind Token kind to format.
 *  @return String literal name.
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
const char *compiler_token_kind_name(Compiler_Token_Kind kind);

/** @brief Classifies a lexeme as a keyword or Identifier.
 *
 *  @param lexeme Raw lexeme text.
 *  @return Keyword kind or `Identifier`.
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1) (fixed number of keywords).
 */
Compiler_Token_Kind classify_compiler_keyword(std::string_view lexeme) noexcept;

/** @brief Returns whether a token kind is a keyword.
 *
 *  @param kind Token kind to check.
 *  @return `true` iff it is a keyword.
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
bool compiler_token_is_keyword(Compiler_Token_Kind kind) noexcept;

/** @brief Returns whether a token kind is a literal.
 *
 *  @param kind Token kind to check.
 *  @return `true` iff it is a literal (integer, string, char, or bool).
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
bool compiler_token_is_literal(Compiler_Token_Kind kind) noexcept;

/** @brief Returns whether a token kind is a comment.
 *
 *  @param kind Token kind to check.
 *  @return `true` iff it is a line or block comment.
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
bool compiler_token_is_comment(Compiler_Token_Kind kind) noexcept;

/** @brief Returns whether a token kind is an operator.
 *
 *  @param kind Token kind to check.
 *  @return `true` iff it is a recognized operator.
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
bool compiler_token_is_operator(Compiler_Token_Kind kind) noexcept;

/** @brief Returns binary precedence for an operator.
 *
 *  @param kind Token kind to check.
 *  @return Precedence level (higher is tighter), or 0 if not a binary op.
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
int compiler_binary_precedence(Compiler_Token_Kind kind) noexcept;

/** @brief Returns binary associativity for an operator.
 *
 *  @param kind Token kind to check.
 *  @return Associativity (Left, Right, or None).
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
Compiler_Associativity compiler_binary_associativity(Compiler_Token_Kind kind) noexcept;

/** @brief Lexical token with raw lexeme and source span. 
 *
 *  @note Thread-safety: Not thread-safe for concurrent mutation. Const methods 
 *        are safe for concurrent readers.
 *  @note Complexity: O(1) for construction/copy (plus string allocation).
 */
struct Compiler_Token
{
  Compiler_Token_Kind kind = Compiler_Token_Kind::Invalid;  ///< Token category.
  std::string lexeme;  ///< Raw lexeme spelling from the source slice.
  Source_Span span;    ///< Span covering the token text.

  /** @brief Returns whether the token is EOF. 
   *
   *  @note Thread-safety: Safe for concurrent readers.
   *  @note Complexity: O(1).
   */
  [[nodiscard]] bool is_eof() const noexcept
  {
    return kind == Compiler_Token_Kind::End_Of_File;
  }

  /** @brief Returns whether the token is invalid. 
   *
   *  @note Thread-safety: Safe for concurrent readers.
   *  @note Complexity: O(1).
   */
  [[nodiscard]] bool is_invalid() const noexcept
  {
    return kind == Compiler_Token_Kind::Invalid;
  }

  /** @brief Returns whether the token kind matches `expected`. 
   *
   *  @note Thread-safety: Safe for concurrent readers.
   *  @note Complexity: O(1).
   */
  [[nodiscard]] bool is(const Compiler_Token_Kind expected) const noexcept
  {
    return kind == expected;
  }

  /** @brief Formats the token for debugging. 
   *
   *  @note Thread-safety: Safe for concurrent readers.
   *  @note Complexity: O(N) where N is lexeme length.
   */
  [[nodiscard]] std::string to_string() const
  {
    auto escaped_lexeme = std::string();
    escaped_lexeme.reserve(lexeme.size());
    for (const char ch : lexeme)
      switch (ch)
        {
        case '\\':
          escaped_lexeme += "\\\\";
          break;
        case '"':
          escaped_lexeme += "\\\"";
          break;
        case '\n':
          escaped_lexeme += "\\n";
          break;
        case '\r':
          escaped_lexeme += "\\r";
          break;
        case '\t':
          escaped_lexeme += "\\t";
          break;
        default:
          escaped_lexeme.push_back(ch);
        }

    std::ostringstream out;
    out << compiler_token_kind_name(kind) << "(\"" << escaped_lexeme << "\")";
    if (span.is_valid())
      out << "@" << span.to_string();
    return out.str();
  }
};

/** @brief Returns a stable string name for a token kind. */
inline const char *compiler_token_kind_name(const Compiler_Token_Kind kind)
{
  switch (kind)
    {
    case Compiler_Token_Kind::End_Of_File:
      return "End_Of_File";
    case Compiler_Token_Kind::Invalid:
      return "Invalid";
    case Compiler_Token_Kind::Identifier:
      return "Identifier";
    case Compiler_Token_Kind::Integer_Literal:
      return "Integer_Literal";
    case Compiler_Token_Kind::String_Literal:
      return "String_Literal";
    case Compiler_Token_Kind::Char_Literal:
      return "Char_Literal";
    case Compiler_Token_Kind::Line_Comment:
      return "Line_Comment";
    case Compiler_Token_Kind::Block_Comment:
      return "Block_Comment";
    case Compiler_Token_Kind::Kw_Fn:
      return "Kw_Fn";
    case Compiler_Token_Kind::Kw_Let:
      return "Kw_Let";
    case Compiler_Token_Kind::Kw_If:
      return "Kw_If";
    case Compiler_Token_Kind::Kw_Else:
      return "Kw_Else";
    case Compiler_Token_Kind::Kw_While:
      return "Kw_While";
    case Compiler_Token_Kind::Kw_For:
      return "Kw_For";
    case Compiler_Token_Kind::Kw_Return:
      return "Kw_Return";
    case Compiler_Token_Kind::Kw_True:
      return "Kw_True";
    case Compiler_Token_Kind::Kw_False:
      return "Kw_False";
    case Compiler_Token_Kind::Kw_Struct:
      return "Kw_Struct";
    case Compiler_Token_Kind::Kw_Enum:
      return "Kw_Enum";
    case Compiler_Token_Kind::Kw_Import:
      return "Kw_Import";
    case Compiler_Token_Kind::Kw_Break:
      return "Kw_Break";
    case Compiler_Token_Kind::Kw_Continue:
      return "Kw_Continue";
    case Compiler_Token_Kind::LParen:
      return "LParen";
    case Compiler_Token_Kind::RParen:
      return "RParen";
    case Compiler_Token_Kind::LBrace:
      return "LBrace";
    case Compiler_Token_Kind::RBrace:
      return "RBrace";
    case Compiler_Token_Kind::LBracket:
      return "LBracket";
    case Compiler_Token_Kind::RBracket:
      return "RBracket";
    case Compiler_Token_Kind::Comma:
      return "Comma";
    case Compiler_Token_Kind::Dot:
      return "Dot";
    case Compiler_Token_Kind::Colon:
      return "Colon";
    case Compiler_Token_Kind::Double_Colon:
      return "Double_Colon";
    case Compiler_Token_Kind::Semicolon:
      return "Semicolon";
    case Compiler_Token_Kind::Question:
      return "Question";
    case Compiler_Token_Kind::Arrow:
      return "Arrow";
    case Compiler_Token_Kind::Assign:
      return "Assign";
    case Compiler_Token_Kind::Plus:
      return "Plus";
    case Compiler_Token_Kind::Minus:
      return "Minus";
    case Compiler_Token_Kind::Star:
      return "Star";
    case Compiler_Token_Kind::Slash:
      return "Slash";
    case Compiler_Token_Kind::Percent:
      return "Percent";
    case Compiler_Token_Kind::Amp:
      return "Amp";
    case Compiler_Token_Kind::Pipe:
      return "Pipe";
    case Compiler_Token_Kind::Caret:
      return "Caret";
    case Compiler_Token_Kind::Bang:
      return "Bang";
    case Compiler_Token_Kind::Tilde:
      return "Tilde";
    case Compiler_Token_Kind::Less:
      return "Less";
    case Compiler_Token_Kind::Greater:
      return "Greater";
    case Compiler_Token_Kind::EqEq:
      return "EqEq";
    case Compiler_Token_Kind::NotEq:
      return "NotEq";
    case Compiler_Token_Kind::LessEq:
      return "LessEq";
    case Compiler_Token_Kind::GreaterEq:
      return "GreaterEq";
    case Compiler_Token_Kind::AndAnd:
      return "AndAnd";
    case Compiler_Token_Kind::OrOr:
      return "OrOr";
    case Compiler_Token_Kind::PlusEq:
      return "PlusEq";
    case Compiler_Token_Kind::MinusEq:
      return "MinusEq";
    case Compiler_Token_Kind::StarEq:
      return "StarEq";
    case Compiler_Token_Kind::SlashEq:
      return "SlashEq";
    case Compiler_Token_Kind::PercentEq:
      return "PercentEq";
    case Compiler_Token_Kind::PlusPlus:
      return "PlusPlus";
    case Compiler_Token_Kind::MinusMinus:
      return "MinusMinus";
    }

  return "Unknown";
}

/** @brief Returns a reserved-keyword kind or `Identifier`. */
inline Compiler_Token_Kind classify_compiler_keyword(const std::string_view lexeme) noexcept
{
  if (lexeme == "fn")
    return Compiler_Token_Kind::Kw_Fn;
  if (lexeme == "let")
    return Compiler_Token_Kind::Kw_Let;
  if (lexeme == "if")
    return Compiler_Token_Kind::Kw_If;
  if (lexeme == "else")
    return Compiler_Token_Kind::Kw_Else;
  if (lexeme == "while")
    return Compiler_Token_Kind::Kw_While;
  if (lexeme == "for")
    return Compiler_Token_Kind::Kw_For;
  if (lexeme == "return")
    return Compiler_Token_Kind::Kw_Return;
  if (lexeme == "true")
    return Compiler_Token_Kind::Kw_True;
  if (lexeme == "false")
    return Compiler_Token_Kind::Kw_False;
  if (lexeme == "struct")
    return Compiler_Token_Kind::Kw_Struct;
  if (lexeme == "enum")
    return Compiler_Token_Kind::Kw_Enum;
  if (lexeme == "import")
    return Compiler_Token_Kind::Kw_Import;
  if (lexeme == "break")
    return Compiler_Token_Kind::Kw_Break;
  if (lexeme == "continue")
    return Compiler_Token_Kind::Kw_Continue;
  return Compiler_Token_Kind::Identifier;
}

/** @brief Returns whether `kind` is a dedicated keyword kind. */
inline bool compiler_token_is_keyword(const Compiler_Token_Kind kind) noexcept
{
  return kind >= Compiler_Token_Kind::Kw_Fn and kind <= Compiler_Token_Kind::Kw_Continue;
}

/** @brief Returns whether `kind` is a literal token. */
inline bool compiler_token_is_literal(const Compiler_Token_Kind kind) noexcept
{
  return kind == Compiler_Token_Kind::Integer_Literal or kind == Compiler_Token_Kind::String_Literal
      or kind == Compiler_Token_Kind::Char_Literal or kind == Compiler_Token_Kind::Kw_True
      or kind == Compiler_Token_Kind::Kw_False;
}

/** @brief Returns whether `kind` denotes a comment token. */
inline bool compiler_token_is_comment(const Compiler_Token_Kind kind) noexcept
{
  return kind == Compiler_Token_Kind::Line_Comment or kind == Compiler_Token_Kind::Block_Comment;
}

/** @brief Returns whether `kind` is classified as an operator token. */
inline bool compiler_token_is_operator(const Compiler_Token_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Token_Kind::Assign:
    case Compiler_Token_Kind::Plus:
    case Compiler_Token_Kind::Minus:
    case Compiler_Token_Kind::Star:
    case Compiler_Token_Kind::Slash:
    case Compiler_Token_Kind::Percent:
    case Compiler_Token_Kind::Amp:
    case Compiler_Token_Kind::Pipe:
    case Compiler_Token_Kind::Caret:
    case Compiler_Token_Kind::Bang:
    case Compiler_Token_Kind::Tilde:
    case Compiler_Token_Kind::Less:
    case Compiler_Token_Kind::Greater:
    case Compiler_Token_Kind::EqEq:
    case Compiler_Token_Kind::NotEq:
    case Compiler_Token_Kind::LessEq:
    case Compiler_Token_Kind::GreaterEq:
    case Compiler_Token_Kind::AndAnd:
    case Compiler_Token_Kind::OrOr:
    case Compiler_Token_Kind::PlusEq:
    case Compiler_Token_Kind::MinusEq:
    case Compiler_Token_Kind::StarEq:
    case Compiler_Token_Kind::SlashEq:
    case Compiler_Token_Kind::PercentEq:
    case Compiler_Token_Kind::PlusPlus:
    case Compiler_Token_Kind::MinusMinus:
    case Compiler_Token_Kind::Arrow:
      return true;
    default:
      return false;
    }
}

/** @brief Returns binary precedence metadata for `kind`, or `0` if none. */
inline int compiler_binary_precedence(const Compiler_Token_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Token_Kind::Assign:
    case Compiler_Token_Kind::PlusEq:
    case Compiler_Token_Kind::MinusEq:
    case Compiler_Token_Kind::StarEq:
    case Compiler_Token_Kind::SlashEq:
    case Compiler_Token_Kind::PercentEq:
      return 1;
    case Compiler_Token_Kind::OrOr:
      return 2;
    case Compiler_Token_Kind::AndAnd:
      return 3;
    case Compiler_Token_Kind::EqEq:
    case Compiler_Token_Kind::NotEq:
      return 4;
    case Compiler_Token_Kind::Less:
    case Compiler_Token_Kind::LessEq:
    case Compiler_Token_Kind::Greater:
    case Compiler_Token_Kind::GreaterEq:
      return 5;
    case Compiler_Token_Kind::Plus:
    case Compiler_Token_Kind::Minus:
    case Compiler_Token_Kind::Pipe:
    case Compiler_Token_Kind::Caret:
      return 6;
    case Compiler_Token_Kind::Star:
    case Compiler_Token_Kind::Slash:
    case Compiler_Token_Kind::Percent:
    case Compiler_Token_Kind::Amp:
      return 7;
    default:
      return 0;
    }
}

/** @brief Returns binary associativity metadata for `kind`. */
inline Compiler_Associativity compiler_binary_associativity(const Compiler_Token_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Token_Kind::Assign:
    case Compiler_Token_Kind::PlusEq:
    case Compiler_Token_Kind::MinusEq:
    case Compiler_Token_Kind::StarEq:
    case Compiler_Token_Kind::SlashEq:
    case Compiler_Token_Kind::PercentEq:
      return Compiler_Associativity::Right;
    default:
      return compiler_binary_precedence(kind) > 0 ? Compiler_Associativity::Left
                                                  : Compiler_Associativity::None;
    }
}
}  // namespace Aleph

#endif  // COMPILER_TOKEN_H
