/*
  This file is part of Aleph-w system

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
 *  Determines how operators of the same precedence are grouped (e.g., `a - b - c`).
 *
 *  @note Thread-safety: Thread-safe (POD).
 *  @note Complexity: O(1).
 */
enum class Compiler_Associativity
{
  None,  ///< The operator is not associative (e.g., chained comparators prohibited).
  Left,  ///< Left-associative: `(a - b) - c`.
  Right  ///< Right-associative: `a = (b = c)`.
};

/** @brief Token kinds supported by the compiler MVP.
 *
 *  Includes keywords, literals, operators, and punctuation. Keywords have
 *  their own enumerators to avoid string comparisons in the parser.
 *
 *  @note Thread-safety: Thread-safe (POD).
 *  @note Complexity: O(1).
 */
enum class Compiler_Token_Kind
{
  Invalid,      ///< Sentinel for lexer errors or uninitialized tokens.
  End_Of_File,  ///< End of the source stream.

  // Identifiers and Literals
  Identifier,       ///< Named identifier (e.g., variable or function name).
  Integer_Literal,  ///< Integer constant.
  String_Literal,   ///< String constant (with escape sequences).
  Char_Literal,     ///< Single character constant.

  // Trivia
  Line_Comment,   ///< Single-line comment (`// ...`).
  Block_Comment,  ///< Multi-line comment (`/* ... */`).

  // Keywords
  Kw_If,
  Kw_Else,
  Kw_While,
  Kw_For,
  Kw_Return,
  Kw_Break,
  Kw_Continue,
  Kw_Func,
  Kw_Fn = Kw_Func,
  Kw_Var,
  Kw_Let,
  Kw_Const,
  Kw_Type,
  Kw_Struct,
  Kw_Enum,
  Kw_Class,
  Kw_Import,
  Kw_Export,
  Kw_Namespace,
  Kw_Try,
  Kw_Catch,
  Kw_Throw,
  Kw_True,
  Kw_False,

  // Punctuation
  LParen,
  RParen,
  LBrace,
  RBrace,
  LBracket,
  RBracket,
  Comma,
  Semicolon,
  Colon,
  Dot,
  Arrow,
  Double_Colon,
  Question,

  // Operators
  Assign,
  Plus,
  Minus,
  Star,
  Slash,
  Percent,
  EqEq,
  NotEq,
  Less,
  LessEq,
  Greater,
  GreaterEq,
  AndAnd,
  OrOr,
  Bang,
  Amp,
  Pipe,
  Caret,
  Tilde,
  PlusEq,
  MinusEq,
  StarEq,
  SlashEq,
  PercentEq,
  PlusPlus,
  MinusMinus
};

/** @brief Returns a human-readable name for a token kind.
 *
 *  Useful for error messages and debugging (e.g., "expected RParen, found Semicolon").
 *
 *  @param kind Token kind to format.
 *  @return String literal containing the kind name.
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
const char *compiler_token_kind_name(Compiler_Token_Kind kind);

/** @brief Classifies a lexeme as a keyword or Identifier.
 *
 *  The lexer uses this function to decide if "if" is a `Kw_If` or an `Identifier`.
 *
 *  @param lexeme Raw lexeme text.
 *  @return The corresponding keyword kind or `Identifier` if no match.
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1) (fixed number of keywords).
 */
Compiler_Token_Kind classify_compiler_keyword(std::string_view lexeme) noexcept;

/** @brief Predicate: Is the kind a reserved keyword?
 *
 *  @param kind Token kind.
 *  @return `true` if it is a keyword.
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
bool compiler_token_is_keyword(Compiler_Token_Kind kind) noexcept;

/** @brief Predicate: Is the kind a literal (number, string, bool, etc.)?
 *
 *  @param kind Token kind.
 *  @return `true` if it is a literal.
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
bool compiler_token_is_literal(Compiler_Token_Kind kind) noexcept;

/** @brief Predicate: Is the kind a comment?
 *
 *  @param kind Token kind.
 *  @return `true` if it is a comment (line or block).
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
bool compiler_token_is_comment(Compiler_Token_Kind kind) noexcept;

/** @brief Predicate: Is the kind an operator (arithmetic, logical, etc.)?
 *
 *  @param kind Token kind.
 *  @return `true` if it is a recognized operator.
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
bool compiler_token_is_operator(Compiler_Token_Kind kind) noexcept;

/** @brief Returns binary precedence for an operator.
 *
 *  Higher values indicate tighter binding (e.g., `*` has more precedence than `+`).
 *  Returns 0 if the token is not a valid binary operator.
 *
 *  @param kind Token kind.
 *  @return Precedence level (>= 1).
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
int compiler_binary_precedence(Compiler_Token_Kind kind) noexcept;

/** @brief Returns binary associativity for an operator.
 *
 *  @param kind Token kind.
 *  @return Associativity (Left/Right/None).
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
Compiler_Associativity compiler_binary_associativity(Compiler_Token_Kind kind) noexcept;

/** @brief Lexical token representation with its value and location.
 *
 *  It is the primary object that the Lexer delivers to the Parser.
 *  It combines the "what" (kind), the "how" (lexeme), and the "where" (span).
 *
 *  @note Thread-safety: Not thread-safe for concurrent mutation. Const methods
 *        are safe for concurrent readers.
 *  @note Complexity: O(1) for construction/copy (plus std::string management).
 */
struct Compiler_Token
{
  Compiler_Token_Kind kind = Compiler_Token_Kind::Invalid;  ///< Logical category (e.g., Kw_If).
  std::string lexeme;                                       ///< Exact text found in the source.
  Source_Span span;  ///< Precise location in the source code.

  /** @brief Returns whether the token marks the end of the file.
   *
   *  @note Thread-safety: Safe for concurrent readers.
   *  @note Complexity: O(1).
   */
  [[nodiscard]] bool is_eof() const noexcept
  {
    return kind == Compiler_Token_Kind::End_Of_File;
  }

  /** @brief Returns whether the token represents a lexical error.
   *
   *  @note Thread-safety: Safe for concurrent readers.
   *  @note Complexity: O(1).
   */
  [[nodiscard]] bool is_invalid() const noexcept
  {
    return kind == Compiler_Token_Kind::Invalid;
  }

  /** @brief Fast kind check.
   *
   *  @param expected The kind to verify.
   *  @return `true` if the token matches the expected kind.
   *  @note Thread-safety: Safe for concurrent readers.
   *  @note Complexity: O(1).
   */
  [[nodiscard]] bool is(const Compiler_Token_Kind expected) const noexcept
  {
    return kind == expected;
  }

  /** @brief Formats the token as a descriptive string for debugging.
   *
   *  Escapes special characters in the lexeme to ensure readable output.
   *  Useful for logging and diagnostics.
   *
   *  @note Thread-safety: Safe for concurrent readers.
   *  @note Complexity: O(N) where N is lexeme length.
   */
  [[nodiscard]] std::string to_string() const;
};

namespace Compiler_Token_Detail {
inline std::string escape_token_lexeme(std::string_view text)
{
  std::string escaped;
  escaped.reserve(text.size() + 8);

  for (const char ch : text)
    switch (ch)
      {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped.push_back(ch);
      }

  return escaped;
}
}  // namespace Compiler_Token_Detail

/** @brief Returns a stable debug name for one token kind. */
inline const char *compiler_token_kind_name(const Compiler_Token_Kind kind)
{
  switch (kind)
    {
    case Compiler_Token_Kind::Invalid:
      return "Invalid";
    case Compiler_Token_Kind::End_Of_File:
      return "End_Of_File";
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
    case Compiler_Token_Kind::Kw_Break:
      return "Kw_Break";
    case Compiler_Token_Kind::Kw_Continue:
      return "Kw_Continue";
    case Compiler_Token_Kind::Kw_Fn:
      return "Kw_Fn";
    case Compiler_Token_Kind::Kw_Var:
      return "Kw_Var";
    case Compiler_Token_Kind::Kw_Let:
      return "Kw_Let";
    case Compiler_Token_Kind::Kw_Const:
      return "Kw_Const";
    case Compiler_Token_Kind::Kw_Type:
      return "Kw_Type";
    case Compiler_Token_Kind::Kw_Struct:
      return "Kw_Struct";
    case Compiler_Token_Kind::Kw_Enum:
      return "Kw_Enum";
    case Compiler_Token_Kind::Kw_Class:
      return "Kw_Class";
    case Compiler_Token_Kind::Kw_Import:
      return "Kw_Import";
    case Compiler_Token_Kind::Kw_Export:
      return "Kw_Export";
    case Compiler_Token_Kind::Kw_Namespace:
      return "Kw_Namespace";
    case Compiler_Token_Kind::Kw_Try:
      return "Kw_Try";
    case Compiler_Token_Kind::Kw_Catch:
      return "Kw_Catch";
    case Compiler_Token_Kind::Kw_Throw:
      return "Kw_Throw";
    case Compiler_Token_Kind::Kw_True:
      return "Kw_True";
    case Compiler_Token_Kind::Kw_False:
      return "Kw_False";
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
    case Compiler_Token_Kind::Semicolon:
      return "Semicolon";
    case Compiler_Token_Kind::Colon:
      return "Colon";
    case Compiler_Token_Kind::Dot:
      return "Dot";
    case Compiler_Token_Kind::Arrow:
      return "Arrow";
    case Compiler_Token_Kind::Double_Colon:
      return "Double_Colon";
    case Compiler_Token_Kind::Question:
      return "Question";
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
    case Compiler_Token_Kind::EqEq:
      return "EqEq";
    case Compiler_Token_Kind::NotEq:
      return "NotEq";
    case Compiler_Token_Kind::Less:
      return "Less";
    case Compiler_Token_Kind::LessEq:
      return "LessEq";
    case Compiler_Token_Kind::Greater:
      return "Greater";
    case Compiler_Token_Kind::GreaterEq:
      return "GreaterEq";
    case Compiler_Token_Kind::AndAnd:
      return "AndAnd";
    case Compiler_Token_Kind::OrOr:
      return "OrOr";
    case Compiler_Token_Kind::Bang:
      return "Bang";
    case Compiler_Token_Kind::Amp:
      return "Amp";
    case Compiler_Token_Kind::Pipe:
      return "Pipe";
    case Compiler_Token_Kind::Caret:
      return "Caret";
    case Compiler_Token_Kind::Tilde:
      return "Tilde";
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

/** @brief Classifies one identifier lexeme as a keyword when applicable. */
inline Compiler_Token_Kind classify_compiler_keyword(std::string_view lexeme) noexcept
{
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
  if (lexeme == "break")
    return Compiler_Token_Kind::Kw_Break;
  if (lexeme == "continue")
    return Compiler_Token_Kind::Kw_Continue;
  if (lexeme == "fn")
    return Compiler_Token_Kind::Kw_Fn;
  if (lexeme == "var")
    return Compiler_Token_Kind::Kw_Var;
  if (lexeme == "let")
    return Compiler_Token_Kind::Kw_Let;
  if (lexeme == "const")
    return Compiler_Token_Kind::Kw_Const;
  if (lexeme == "type")
    return Compiler_Token_Kind::Kw_Type;
  if (lexeme == "struct")
    return Compiler_Token_Kind::Kw_Struct;
  if (lexeme == "enum")
    return Compiler_Token_Kind::Kw_Enum;
  if (lexeme == "class")
    return Compiler_Token_Kind::Kw_Class;
  if (lexeme == "import")
    return Compiler_Token_Kind::Kw_Import;
  if (lexeme == "export")
    return Compiler_Token_Kind::Kw_Export;
  if (lexeme == "namespace")
    return Compiler_Token_Kind::Kw_Namespace;
  if (lexeme == "try")
    return Compiler_Token_Kind::Kw_Try;
  if (lexeme == "catch")
    return Compiler_Token_Kind::Kw_Catch;
  if (lexeme == "throw")
    return Compiler_Token_Kind::Kw_Throw;
  if (lexeme == "true")
    return Compiler_Token_Kind::Kw_True;
  if (lexeme == "false")
    return Compiler_Token_Kind::Kw_False;

  return Compiler_Token_Kind::Identifier;
}

/** @brief Returns whether `kind` is one reserved keyword. */
inline bool compiler_token_is_keyword(const Compiler_Token_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Token_Kind::Kw_If:
    case Compiler_Token_Kind::Kw_Else:
    case Compiler_Token_Kind::Kw_While:
    case Compiler_Token_Kind::Kw_For:
    case Compiler_Token_Kind::Kw_Return:
    case Compiler_Token_Kind::Kw_Break:
    case Compiler_Token_Kind::Kw_Continue:
    case Compiler_Token_Kind::Kw_Fn:
    case Compiler_Token_Kind::Kw_Var:
    case Compiler_Token_Kind::Kw_Let:
    case Compiler_Token_Kind::Kw_Const:
    case Compiler_Token_Kind::Kw_Type:
    case Compiler_Token_Kind::Kw_Struct:
    case Compiler_Token_Kind::Kw_Enum:
    case Compiler_Token_Kind::Kw_Class:
    case Compiler_Token_Kind::Kw_Import:
    case Compiler_Token_Kind::Kw_Export:
    case Compiler_Token_Kind::Kw_Namespace:
    case Compiler_Token_Kind::Kw_Try:
    case Compiler_Token_Kind::Kw_Catch:
    case Compiler_Token_Kind::Kw_Throw:
    case Compiler_Token_Kind::Kw_True:
    case Compiler_Token_Kind::Kw_False:
      return true;
    default:
      return false;
    }
}

/** @brief Returns whether `kind` is one literal token. */
inline bool compiler_token_is_literal(const Compiler_Token_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Token_Kind::Integer_Literal:
    case Compiler_Token_Kind::String_Literal:
    case Compiler_Token_Kind::Char_Literal:
    case Compiler_Token_Kind::Kw_True:
    case Compiler_Token_Kind::Kw_False:
      return true;
    default:
      return false;
    }
}

/** @brief Returns whether `kind` denotes comment trivia. */
inline bool compiler_token_is_comment(const Compiler_Token_Kind kind) noexcept
{
  return kind == Compiler_Token_Kind::Line_Comment or kind == Compiler_Token_Kind::Block_Comment;
}

/** @brief Returns whether `kind` participates in the operator surface syntax. */
inline bool compiler_token_is_operator(const Compiler_Token_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Token_Kind::Arrow:
    case Compiler_Token_Kind::Assign:
    case Compiler_Token_Kind::Plus:
    case Compiler_Token_Kind::Minus:
    case Compiler_Token_Kind::Star:
    case Compiler_Token_Kind::Slash:
    case Compiler_Token_Kind::Percent:
    case Compiler_Token_Kind::EqEq:
    case Compiler_Token_Kind::NotEq:
    case Compiler_Token_Kind::Less:
    case Compiler_Token_Kind::LessEq:
    case Compiler_Token_Kind::Greater:
    case Compiler_Token_Kind::GreaterEq:
    case Compiler_Token_Kind::AndAnd:
    case Compiler_Token_Kind::OrOr:
    case Compiler_Token_Kind::Bang:
    case Compiler_Token_Kind::Amp:
    case Compiler_Token_Kind::Pipe:
    case Compiler_Token_Kind::Caret:
    case Compiler_Token_Kind::Tilde:
    case Compiler_Token_Kind::PlusEq:
    case Compiler_Token_Kind::MinusEq:
    case Compiler_Token_Kind::StarEq:
    case Compiler_Token_Kind::SlashEq:
    case Compiler_Token_Kind::PercentEq:
    case Compiler_Token_Kind::PlusPlus:
    case Compiler_Token_Kind::MinusMinus:
      return true;
    default:
      return false;
    }
}

/** @brief Returns the precedence used by the precedence-climbing parser. */
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
      return 6;
    case Compiler_Token_Kind::Star:
    case Compiler_Token_Kind::Slash:
    case Compiler_Token_Kind::Percent:
      return 7;
    default:
      return 0;
    }
}

/** @brief Returns associativity metadata for binary operators. */
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

    case Compiler_Token_Kind::OrOr:
    case Compiler_Token_Kind::AndAnd:
    case Compiler_Token_Kind::EqEq:
    case Compiler_Token_Kind::NotEq:
    case Compiler_Token_Kind::Less:
    case Compiler_Token_Kind::LessEq:
    case Compiler_Token_Kind::Greater:
    case Compiler_Token_Kind::GreaterEq:
    case Compiler_Token_Kind::Plus:
    case Compiler_Token_Kind::Minus:
    case Compiler_Token_Kind::Star:
    case Compiler_Token_Kind::Slash:
    case Compiler_Token_Kind::Percent:
      return Compiler_Associativity::Left;

    default:
      return Compiler_Associativity::None;
    }
}

/** @brief Formats the token for deterministic debugging and testing. */
inline std::string Compiler_Token::to_string() const
{
  std::ostringstream out;
  out << compiler_token_kind_name(kind) << "(\""
      << Compiler_Token_Detail::escape_token_lexeme(lexeme) << "\")@";
  if (span.is_valid())
    out << span.to_string();
  else
    out << "<invalid-span>";
  return out.str();
}

}  // namespace Aleph

#endif  // COMPILER_TOKEN_H
