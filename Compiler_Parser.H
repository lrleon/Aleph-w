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

/** @file Compiler_Parser.H
 *  @brief Recursive-descent parser for the compiler-support MVP grammar.
 *
 *  The parser builds the arena-backed AST defined in `Compiler_AST.H` using the
 *  lexer from `Compiler_Lexer.H`. It supports function declarations, blocks,
 *  `let`, `return`, `if`, `while`, `break`, `continue`, expression statements,
 *  calls, unary operators, and precedence-climbing binary expressions.
 *
 *  Parse errors produce invalid nodes and, when available, diagnostics in the
 *  attached `Diagnostic_Engine`.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_PARSER_H
#define COMPILER_PARSER_H

#include <string>

#include <Compiler_AST.H>
#include <Compiler_Lexer.H>

namespace Aleph {
/** @brief Parser configuration knobs for the MVP grammar. */
struct Compiler_Parser_Options
{
  bool allow_top_level_statements = true;  ///< If false, only `fn` is accepted at top level.
};

/** @brief Recursive-descent parser that produces an AST in `Compiler_Ast_Context`. */
class Compiler_Parser
{
  Compiler_Ast_Context *ast = nullptr;
  Diagnostic_Engine *diagnostics = nullptr;
  Compiler_Lexer lexer;
  Compiler_Parser_Options options;

  static bool is_expression_start(const Compiler_Token_Kind kind) noexcept
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

  static bool is_statement_start(const Compiler_Token_Kind kind) noexcept
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
        return is_expression_start(kind);
      }
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

  Compiler_Token peek()
  {
    return lexer.peek();
  }

  Compiler_Token next()
  {
    return lexer.next();
  }

  bool match(const Compiler_Token_Kind kind)
  {
    if (peek().kind != kind)
      return false;
    (void) next();
    return true;
  }

  Compiler_Invalid_Expr *invalid_expr(const Source_Span &span = {})
  {
    return ast->make<Compiler_Invalid_Expr>(span);
  }

  Compiler_Invalid_Stmt *invalid_stmt(const Source_Span &span = {})
  {
    return ast->make<Compiler_Invalid_Stmt>(span);
  }

  Compiler_Token expect(const Compiler_Token_Kind kind,
                        const std::string &message,
                        const std::string &code)
  {
    const auto tok = peek();
    if (tok.kind == kind)
      return next();

    emit_error(tok.span, message, code);
    return {kind, "", tok.span};
  }

  Compiler_Token expect_identifier(const std::string &context, const std::string &code = "PAR002")
  {
    const auto tok = peek();
    if (tok.kind == Compiler_Token_Kind::Identifier)
      return next();

    emit_error(tok.span, "expected identifier " + context, code);
    return {Compiler_Token_Kind::Identifier, "", tok.span};
  }

  void synchronize_statement()
  {
    while (true)
      {
        const auto tok = peek();
        if (tok.is_eof() or tok.kind == Compiler_Token_Kind::RBrace)
          return;

        if (tok.kind == Compiler_Token_Kind::Semicolon)
          {
            (void) next();
            return;
          }

        if (tok.kind == Compiler_Token_Kind::Kw_Fn or tok.kind == Compiler_Token_Kind::Kw_Let
            or tok.kind == Compiler_Token_Kind::Kw_Return or tok.kind == Compiler_Token_Kind::Kw_If
            or tok.kind == Compiler_Token_Kind::Kw_While or tok.kind == Compiler_Token_Kind::Kw_Break
            or tok.kind == Compiler_Token_Kind::Kw_Continue)
          return;

        (void) next();
      }
  }

  Source_Span parse_statement_terminator(const Source_Span &anchor)
  {
    const auto tok = peek();
    if (tok.kind == Compiler_Token_Kind::Semicolon)
      return next().span;

    emit_error(tok.span.is_valid() ? tok.span : anchor, "expected ';' after statement", "PAR005");

    if (tok.is_eof() or tok.kind == Compiler_Token_Kind::RBrace or is_statement_start(tok.kind))
      return {};

    synchronize_statement();
    return {};
  }

  Compiler_Grouping_Expr *parse_grouping_expr()
  {
    const auto lparen = next();
    auto *inner = parse_expression();
    const auto rparen
      = expect(Compiler_Token_Kind::RParen, "expected ')' to close grouped expression", "PAR003");
    return ast->make<Compiler_Grouping_Expr>(inner, lparen.span, rparen.span);
  }

  Compiler_Expr *parse_primary()
  {
    const auto tok = peek();
    switch (tok.kind)
      {
        case Compiler_Token_Kind::Identifier: {
          const auto name = next();
          return ast->make<Compiler_Identifier_Expr>(name.lexeme, name.span);
        }
        case Compiler_Token_Kind::Integer_Literal: {
          const auto lit = next();
          return ast->make<Compiler_Literal_Expr>(Compiler_Expr_Kind::Integer_Literal,
                                                  lit.lexeme,
                                                  lit.span);
        }
        case Compiler_Token_Kind::String_Literal: {
          const auto lit = next();
          return ast->make<Compiler_Literal_Expr>(Compiler_Expr_Kind::String_Literal,
                                                  lit.lexeme,
                                                  lit.span);
        }
        case Compiler_Token_Kind::Char_Literal: {
          const auto lit = next();
          return ast->make<Compiler_Literal_Expr>(Compiler_Expr_Kind::Char_Literal,
                                                  lit.lexeme,
                                                  lit.span);
        }
        case Compiler_Token_Kind::Kw_True: {
          const auto lit = next();
          return ast->make<Compiler_Literal_Expr>(Compiler_Expr_Kind::Bool_Literal,
                                                  lit.lexeme,
                                                  lit.span,
                                                  true);
        }
        case Compiler_Token_Kind::Kw_False: {
          const auto lit = next();
          return ast->make<Compiler_Literal_Expr>(Compiler_Expr_Kind::Bool_Literal,
                                                  lit.lexeme,
                                                  lit.span,
                                                  false);
        }
      case Compiler_Token_Kind::LParen:
        return parse_grouping_expr();
      default:
        emit_error(tok.span, "expected expression", "PAR001");
        if (not tok.is_eof())
          (void) next();
        return invalid_expr(tok.span);
      }
  }

  Compiler_Expr *parse_call_suffix(Compiler_Expr *callee)
  {
    const auto lparen
      = expect(Compiler_Token_Kind::LParen, "expected '(' to start argument list", "PAR003");
    auto *call = ast->make<Compiler_Call_Expr>(callee, lparen.span, Source_Span());

    if (peek().kind != Compiler_Token_Kind::RParen)
      while (true)
        {
          call->arguments.append(parse_expression());
          if (match(Compiler_Token_Kind::Comma))
            continue;
          break;
        }

    const auto rparen
      = expect(Compiler_Token_Kind::RParen, "expected ')' after argument list", "PAR003");
    call->rparen_span = rparen.span;
    call->span = compiler_merge_spans(call->span, rparen.span);
    return call;
  }

  Compiler_Expr *parse_postfix()
  {
    auto *expr = parse_primary();
    while (peek().kind == Compiler_Token_Kind::LParen)
      expr = parse_call_suffix(expr);
    return expr;
  }

  Compiler_Expr *parse_unary()
  {
    const auto tok = peek();
    if (tok.kind == Compiler_Token_Kind::Bang or tok.kind == Compiler_Token_Kind::Minus
        or tok.kind == Compiler_Token_Kind::Plus or tok.kind == Compiler_Token_Kind::Tilde)
      {
        const auto op = next();
        auto *operand = parse_unary();
        return ast->make<Compiler_Unary_Expr>(op.kind, op.span, operand);
      }

    return parse_postfix();
  }

  Compiler_Expr *parse_expression(const int min_precedence = 1)
  {
    auto *lhs = parse_unary();

    while (true)
      {
        const auto op = peek();
        const int precedence = compiler_binary_precedence(op.kind);
        if (precedence < min_precedence)
          break;

        (void) next();
        const int rhs_min = compiler_binary_associativity(op.kind) == Compiler_Associativity::Right
                            ? precedence
                            : precedence + 1;
        auto *rhs = parse_expression(rhs_min);
        lhs = ast->make<Compiler_Binary_Expr>(lhs, op.kind, op.span, rhs);
      }

    return lhs;
  }

  Compiler_Block_Stmt *parse_block_statement()
  {
    const auto lbrace = expect(Compiler_Token_Kind::LBrace, "expected '{' to start block", "PAR003");
    auto *block = ast->make<Compiler_Block_Stmt>(lbrace.span, Source_Span());

    while (true)
      {
        const auto tok = peek();
        if (tok.is_eof() or tok.kind == Compiler_Token_Kind::RBrace)
          break;
        block->statements.append(parse_statement());
      }

    const auto rbrace = expect(Compiler_Token_Kind::RBrace, "expected '}' to close block", "PAR003");
    block->rbrace_span = rbrace.span;
    block->span = compiler_merge_spans(block->lbrace_span, rbrace.span);
    return block;
  }

  Compiler_Stmt *parse_let_statement()
  {
    const auto kw = next();
    const auto name = expect_identifier("after 'let'");

    Compiler_Expr *init = nullptr;
    if (match(Compiler_Token_Kind::Assign))
      init = parse_expression();

    const auto semi = parse_statement_terminator(name.span);
    return ast->make<Compiler_Let_Stmt>(kw.span, name.lexeme, name.span, init, semi);
  }

  Compiler_Stmt *parse_return_statement()
  {
    const auto kw = next();
    Compiler_Expr *value = nullptr;
    if (peek().kind != Compiler_Token_Kind::Semicolon and peek().kind != Compiler_Token_Kind::RBrace
        and not peek().is_eof())
      value = parse_expression();

    const auto semi = parse_statement_terminator(kw.span);
    return ast->make<Compiler_Return_Stmt>(kw.span, value, semi);
  }

  Compiler_Stmt *parse_break_statement()
  {
    const auto kw = next();
    const auto semi = parse_statement_terminator(kw.span);
    return ast->make<Compiler_Break_Stmt>(kw.span, semi);
  }

  Compiler_Stmt *parse_continue_statement()
  {
    const auto kw = next();
    const auto semi = parse_statement_terminator(kw.span);
    return ast->make<Compiler_Continue_Stmt>(kw.span, semi);
  }

  Compiler_Stmt *parse_if_statement()
  {
    const auto if_kw = next();
    auto *condition = parse_expression();
    auto *then_branch = parse_statement();

    Source_Span else_kw;
    Compiler_Stmt *else_branch = nullptr;
    if (peek().kind == Compiler_Token_Kind::Kw_Else)
      {
        else_kw = next().span;
        else_branch = parse_statement();
      }

    return ast->make<Compiler_If_Stmt>(if_kw.span, condition, then_branch, else_kw, else_branch);
  }

  Compiler_Stmt *parse_while_statement()
  {
    const auto kw = next();
    auto *condition = parse_expression();
    auto *body = parse_statement();
    return ast->make<Compiler_While_Stmt>(kw.span, condition, body);
  }

  Compiler_Stmt *parse_expression_statement()
  {
    auto *expr = parse_expression();
    const auto semi = parse_statement_terminator(expr != nullptr ? expr->span : Source_Span());
    return ast->make<Compiler_Expr_Stmt>(expr, semi);
  }

public:
  /** @brief Constructs a parser for one registered source file.
   *
   *  @param ctx AST context that will own the produced nodes.
   *  @param sm Source manager owning the file text.
   *  @param file_id Registered source file id to parse.
   *  @param dx Optional diagnostic sink shared with the lexer.
   *  @param opts Parser options for top-level grammar handling.
   */
  Compiler_Parser(Compiler_Ast_Context &ctx,
                  const Source_Manager &sm,
                  const Source_File_Id file_id,
                  Diagnostic_Engine *dx = nullptr,
                  const Compiler_Parser_Options &opts = {})
    : ast(&ctx), diagnostics(dx), lexer(sm, file_id, dx), options(opts)
  {}

  /** @brief Parses a full module until EOF.
   *
   *  @return Arena-backed module node containing functions and statements.
   */
  Compiler_Module *parse_module()
  {
    auto *module = ast->make<Compiler_Module>();
    Source_Span first_span;
    Source_Span last_span;

    while (true)
      {
        const auto tok = peek();
        if (tok.is_eof())
          {
            if (not first_span.is_valid())
              module->span = tok.span;
            else
              module->span = compiler_merge_spans(first_span, last_span);
            return module;
          }

        if (not first_span.is_valid())
          first_span = tok.span;

        if (tok.kind == Compiler_Token_Kind::Kw_Fn)
          {
            auto *fn = parse_function();
            module->functions.append(fn);
            last_span = fn->span;
            continue;
          }

        if (not options.allow_top_level_statements)
          emit_error(tok.span, "top-level statement is not allowed in this parser mode", "PAR004");

        auto *stmt = parse_statement();
        module->statements.append(stmt);
        last_span = stmt->span;
      }
  }

  /** @brief Parses a top-level function declaration.
   *
   *  @return Function declaration node. Recovery still returns a node on error.
   */
  Compiler_Function_Decl *parse_function()
  {
    const auto kw
      = expect(Compiler_Token_Kind::Kw_Fn, "expected 'fn' to start function declaration", "PAR003");
    const auto name = expect_identifier("after 'fn'");

    (void) expect(Compiler_Token_Kind::LParen, "expected '(' after function name", "PAR003");

    auto *fn = ast->make<Compiler_Function_Decl>(kw.span, name.lexeme, name.span, nullptr);

    if (peek().kind != Compiler_Token_Kind::RParen)
      while (true)
        {
          const auto param = expect_identifier("in parameter list");
          fn->parameters.append({param.lexeme, param.span});
          if (match(Compiler_Token_Kind::Comma))
            continue;
          break;
        }

    (void) expect(Compiler_Token_Kind::RParen, "expected ')' after parameter list", "PAR003");

    fn->body = parse_block_statement();
    fn->span = compiler_merge_spans(kw.span, fn->body != nullptr ? fn->body->span : name.span);
    return fn;
  }

  /** @brief Parses a single statement.
   *
   *  @return Statement node. Invalid input yields `Compiler_Invalid_Stmt`.
   */
  Compiler_Stmt *parse_statement()
  {
    const auto tok = peek();
    switch (tok.kind)
      {
      case Compiler_Token_Kind::LBrace:
        return parse_block_statement();
      case Compiler_Token_Kind::Kw_Let:
        return parse_let_statement();
      case Compiler_Token_Kind::Kw_Return:
        return parse_return_statement();
      case Compiler_Token_Kind::Kw_If:
        return parse_if_statement();
      case Compiler_Token_Kind::Kw_While:
        return parse_while_statement();
      case Compiler_Token_Kind::Kw_Break:
        return parse_break_statement();
      case Compiler_Token_Kind::Kw_Continue:
        return parse_continue_statement();
      default:
        if (is_expression_start(tok.kind))
          return parse_expression_statement();

        emit_error(tok.span, "expected statement", "PAR006");
        if (not tok.is_eof())
          (void) next();
        synchronize_statement();
        return invalid_stmt(tok.span);
      }
  }
};
}  // namespace Aleph

#endif  // COMPILER_PARSER_H
