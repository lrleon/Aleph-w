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

/** @file Compiler_Parser_Utils.H
 *  @brief Reusable token-stream helpers for recursive-descent frontends.
 *
 *  This header provides lightweight utilities that sit between
 *  `Compiler_Lexer.H` and a language-specific parser. It intentionally avoids
 *  AST concerns so that alternate frontends can reuse lookahead, expectations,
 *  separated-list parsing, and statement recovery without adopting the MVP
 *  language surface.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_PARSER_UTILS_H
#define COMPILER_PARSER_UTILS_H

#include <string>
#include <string_view>

#include <Compiler_Lexer.H>

namespace Aleph
{
  /** @brief Returns whether a token kind can start an expression in the MVP grammar.
   *
   *  Frontends may reuse this predicate directly or use it as a reference when
   *  defining a custom expression-entry policy.
   */
  inline bool
  compiler_is_expression_start(const Compiler_Token_Kind kind) noexcept
  {
    switch (kind)
      {
      case Compiler_Token_Kind::Identifier:
      case Compiler_Token_Kind::Integer_Literal:
      case Compiler_Token_Kind::String_Literal:
      case Compiler_Token_Kind::Char_Literal:
      case Compiler_Token_Kind::Kw_True:
      case Compiler_Token_Kind::Kw_False:
      case Compiler_Token_Kind::LParen:
      case Compiler_Token_Kind::Bang:
      case Compiler_Token_Kind::Minus:
      case Compiler_Token_Kind::Plus:
      case Compiler_Token_Kind::Tilde:
        return true;
      default:
        return false;
      }
  }

  /** @brief Returns whether a token kind can start a statement in the MVP grammar. */
  inline bool
  compiler_is_statement_start(const Compiler_Token_Kind kind) noexcept
  {
    switch (kind)
      {
      case Compiler_Token_Kind::LBrace:
      case Compiler_Token_Kind::Kw_Let:
      case Compiler_Token_Kind::Kw_Return:
      case Compiler_Token_Kind::Kw_If:
      case Compiler_Token_Kind::Kw_While:
      case Compiler_Token_Kind::Kw_Break:
      case Compiler_Token_Kind::Kw_Continue:
        return true;
      default:
        return compiler_is_expression_start(kind);
      }
  }

  /** @brief Returns whether a token kind starts one top-level declaration. */
  inline bool
  compiler_is_top_level_declaration_start(const Compiler_Token_Kind kind) noexcept
  {
    switch (kind)
      {
      case Compiler_Token_Kind::Kw_Import:
      case Compiler_Token_Kind::Kw_Fn:
      case Compiler_Token_Kind::Kw_Type:
      case Compiler_Token_Kind::Kw_Struct:
      case Compiler_Token_Kind::Kw_Enum:
        return true;
      default:
        return false;
      }
  }

  /** @brief Removes surrounding double quotes from one string-literal token. */
  inline std::string
  compiler_unquote_string_token(const Compiler_Token & token)
  {
    if (token.kind != Compiler_Token_Kind::String_Literal)
      return token.lexeme;
    if (token.lexeme.size() >= 2 and token.lexeme.front() == '"'
        and token.lexeme.back() == '"')
      return token.lexeme.substr(1, token.lexeme.size() - 2);
    return token.lexeme;
  }

  /** @brief Lightweight token stream facade for recursive-descent parsers.
   *
   *  The stream wraps one `Compiler_Lexer` and exposes parser-oriented helpers:
   *  lookahead, `match`, `expect`, identifier expectations, separated-list
   *  parsing, and statement-oriented recovery.
   */
  class Compiler_Parser_Stream
  {
    Compiler_Lexer * lexer = nullptr;
    Diagnostic_Engine * diagnostics = nullptr;

    void
    emit_error(const Source_Span & span,
               const std::string & message,
               const std::string & code,
               const std::string & note = "",
               const std::string & help = "") const
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

  public:
    /** @brief Builds one parser stream over an existing lexer. */
    Compiler_Parser_Stream(Compiler_Lexer & token_source,
                           Diagnostic_Engine * dx = nullptr) noexcept
      : lexer(&token_source),
        diagnostics(dx)
    {}

    /** @brief Returns the next token without consuming it. */
    [[nodiscard]] Compiler_Token
    peek()
    {
      return lexer->peek();
    }

    /** @brief Consumes and returns the next token. */
    [[nodiscard]] Compiler_Token
    next()
    {
      return lexer->next();
    }

    /** @brief Returns whether the next token has the requested kind. */
    [[nodiscard]] bool
    check(const Compiler_Token_Kind kind)
    {
      return peek().kind == kind;
    }

    /** @brief Consumes the next token if it matches `kind`. */
    bool
    match(const Compiler_Token_Kind kind)
    {
      if (not check(kind))
        return false;
      (void) next();
      return true;
    }

    /** @brief Consumes a token of kind `kind` or emits one diagnostic error. */
    [[nodiscard]] Compiler_Token
    expect(const Compiler_Token_Kind kind,
           const std::string & message,
           const std::string & code,
           const std::string & note = "",
           const std::string & help = "")
    {
      const auto tok = peek();
      if (tok.kind == kind)
        return next();

      emit_error(tok.span, message, code, note, help);
      return {kind, "", tok.span};
    }

    /** @brief Consumes one identifier token or emits one diagnostic error. */
    [[nodiscard]] Compiler_Token
    expect_identifier(const std::string_view context,
                      const std::string & code = "PAR002")
    {
      const auto tok = peek();
      if (tok.kind == Compiler_Token_Kind::Identifier)
        return next();

      emit_error(tok.span, "expected identifier " + std::string(context), code);
      return {Compiler_Token_Kind::Identifier, "", tok.span};
    }

    /** @brief Parses one separated list until a terminator token is reached.
     *
     *  @param separator Separator token between items, such as `Comma`.
     *  @param terminator Closing token that ends the list.
     *  @param parse_item Callback that parses and records one item.
     *  @param allow_trailing_separator If `true`, `a, b, )` is accepted.
     *  @return `false` if `parse_item()` reports failure; `true` otherwise.
     */
    template <typename Parse_Item>
    bool
    parse_separated_list(const Compiler_Token_Kind separator,
                         const Compiler_Token_Kind terminator,
                         Parse_Item && parse_item,
                         const bool allow_trailing_separator = false)
    {
      if (check(terminator))
        return true;

      while (true)
        {
          if (not parse_item())
            return false;

          if (not match(separator))
            return true;

          if (allow_trailing_separator and check(terminator))
            return true;
        }
    }

    /** @brief Skips tokens until a statement boundary suitable for recovery.
     *
     *  The method stops at EOF, `}`, the next top-level declaration keyword, or
     *  the next structural statement starter (`{`, `let`, `return`, `if`,
     *  `while`, `break`, `continue`). If a semicolon is encountered, it is
     *  consumed before returning so that callers can resume after the failed
     *  statement.
     */
    void
    synchronize_statement()
    {
      while (true)
        {
          const auto tok = peek();
          if (tok.is_eof() or tok.kind == Compiler_Token_Kind::RBrace
              or compiler_is_top_level_declaration_start(tok.kind))
            return;

          switch (tok.kind)
            {
            case Compiler_Token_Kind::LBrace:
            case Compiler_Token_Kind::Kw_Let:
            case Compiler_Token_Kind::Kw_Return:
            case Compiler_Token_Kind::Kw_If:
            case Compiler_Token_Kind::Kw_While:
            case Compiler_Token_Kind::Kw_Break:
            case Compiler_Token_Kind::Kw_Continue:
              return;
            default:
              break;
            }

          if (tok.kind == Compiler_Token_Kind::Semicolon)
            {
              (void) next();
              return;
            }

          (void) next();
        }
    }
  };
}

#endif
