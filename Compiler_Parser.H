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
 *  lexer from `Compiler_Lexer.H`. It supports import declarations, function
 *  declarations, explicit type annotations, blocks, `let`, `return`, `if`,
 *  `while`, `break`, `continue`, expression statements, calls, unary
 *  operators, and precedence-climbing binary expressions.
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
#include <Compiler_Parser_Utils.H>

namespace Aleph {

/** @brief Parser configuration knobs for the MVP grammar.
 *
 *  @note Thread-safety: Thread-safe (POD).
 *  @note Complexity: O(1).
 */
struct Compiler_Parser_Options
{
  bool allow_top_level_statements = true;  ///< If false, only top-level declarations are accepted.
};

/** @brief Recursive-descent parser that produces an AST in `Compiler_Ast_Context`.
 *
 *  Implements a hand-written parser for a C-like language. It uses
 *  precedence-climbing for expressions and recursive descent for statements
 *  and declarations, including top-level `import`, `fn`, `type`, `struct`,
 *  and `enum`.
 *
 *  @note Thread-safety: Not thread-safe.
 */
class Compiler_Parser
{
  Compiler_Ast_Context *ast = nullptr;       ///< AST owner.
  Diagnostic_Engine *diagnostics = nullptr;  ///< diagnostic sink.
  Compiler_Lexer lexer;                      ///< Token source.
  Compiler_Parser_Stream tokens;             ///< Reusable parser-oriented token facade.
  Compiler_Parser_Options options;           ///< Parser behavior configuration.
  std::string source_name;                   ///< User-facing source name being parsed.

  /** @brief Peeks at the next token without consuming it. */
  Compiler_Token peek()
  {
    return tokens.peek();
  }

  /** @brief Consumes and returns the next token. */
  Compiler_Token next()
  {
    return tokens.next();
  }

  /** @brief Consumes the next token if it matches `kind`.
   *  @return `true` if matched and consumed.
   */
  bool match(const Compiler_Token_Kind kind)
  {
    return tokens.match(kind);
  }

  /** @brief Internal helper to report a diagnostic error. */
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

  /** @brief Creates an invalid expression node in the current arena. */
  [[nodiscard]] Compiler_Invalid_Expr *invalid_expr(const Source_Span &span = {}) const
  {
    return ast->make<Compiler_Invalid_Expr>(span);
  }

  /** @brief Creates an invalid statement node in the current arena. */
  [[nodiscard]] Compiler_Invalid_Stmt *invalid_stmt(const Source_Span &span = {}) const
  {
    return ast->make<Compiler_Invalid_Stmt>(span);
  }

  /** @brief Creates an invalid type-expression node in the current arena. */
  [[nodiscard]] Compiler_Invalid_Type_Expr *invalid_type_expr(const Source_Span &span = {}) const
  {
    return ast->make<Compiler_Invalid_Type_Expr>(span);
  }

  /** @brief Creates an invalid import-declaration node in the current arena. */
  [[nodiscard]] Compiler_Invalid_Import_Decl *invalid_import_decl(const Source_Span &span = {}) const
  {
    return ast->make<Compiler_Invalid_Import_Decl>(span);
  }

  /** @brief Creates an invalid type-declaration node in the current arena. */
  [[nodiscard]] Compiler_Invalid_Type_Decl *invalid_type_decl(const Source_Span &span = {}) const
  {
    return ast->make<Compiler_Invalid_Type_Decl>(span);
  }

  /** @brief Consumes a token of kind `kind` or emits an error.
   *  @return The matched token, or a synthetic token on error.
   */
  Compiler_Token expect(const Compiler_Token_Kind kind,
                        const std::string &message,
                        const std::string &code)
  {
    return tokens.expect(kind, message, code);
  }

  /** @brief Consumes an identifier token or emits an error. */
  Compiler_Token expect_identifier(const std::string &context, const std::string &code = "PAR002")
  {
    return tokens.expect_identifier(context, code);
  }

  /** @brief Error recovery helper: skips tokens until a statement boundary. */
  void synchronize_statement()
  {
    tokens.synchronize_statement();
  }

  /** @brief Ensures a statement is terminated by a semicolon or reports an error. */
  Source_Span parse_statement_terminator(const Source_Span &anchor)
  {
    const auto tok = peek();
    if (tok.kind == Compiler_Token_Kind::Semicolon)
      return next().span;

    emit_error(tok.span.is_valid() ? tok.span : anchor, "expected ';' after statement", "PAR005");

    if (tok.is_eof() or tok.kind == Compiler_Token_Kind::RBrace
        or compiler_is_statement_start(tok.kind))
      return {};

    synchronize_statement();
    return {};
  }

  /** @brief Parses a parenthesized expression: `(` expression `)`. */
  Compiler_Grouping_Expr *parse_grouping_expr()
  {
    const auto lparen = next();
    auto *inner = parse_expression();
    const auto rparen
      = expect(Compiler_Token_Kind::RParen, "expected ')' to close grouped expression", "PAR003");
    return ast->make<Compiler_Grouping_Expr>(inner, lparen.span, rparen.span);
  }

  /** @brief Parses primary expressions: literals, identifiers, and groupings. */
  Compiler_Expr *parse_primary()
  {
    const auto tok = peek();
    switch (tok.kind)
      {
      case Compiler_Token_Kind::Identifier:
        {
          const auto name = next();
          return ast->make<Compiler_Identifier_Expr>(name.lexeme, name.span);
        }
      case Compiler_Token_Kind::Integer_Literal:
        {
          const auto lit = next();
          return ast->make<Compiler_Literal_Expr>(Compiler_Expr_Kind::Integer_Literal,
                                                  lit.lexeme,
                                                  lit.span);
        }
      case Compiler_Token_Kind::String_Literal:
        {
          const auto lit = next();
          return ast->make<Compiler_Literal_Expr>(Compiler_Expr_Kind::String_Literal,
                                                  lit.lexeme,
                                                  lit.span);
        }
      case Compiler_Token_Kind::Char_Literal:
        {
          const auto lit = next();
          return ast->make<Compiler_Literal_Expr>(Compiler_Expr_Kind::Char_Literal,
                                                  lit.lexeme,
                                                  lit.span);
        }
      case Compiler_Token_Kind::Kw_True:
        {
          const auto lit = next();
          return ast->make<Compiler_Literal_Expr>(Compiler_Expr_Kind::Bool_Literal,
                                                  lit.lexeme,
                                                  lit.span,
                                                  true);
        }
      case Compiler_Token_Kind::Kw_False:
        {
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

  /** @brief Parses function call argument list suffix: `(` args? `)`. */
  Compiler_Expr *parse_call_suffix(Compiler_Expr *callee)
  {
    const auto lparen
      = expect(Compiler_Token_Kind::LParen, "expected '(' to start argument list", "PAR003");
    auto *call = ast->make<Compiler_Call_Expr>(callee, lparen.span, Source_Span());

    (void) tokens.parse_separated_list(Compiler_Token_Kind::Comma,
                                       Compiler_Token_Kind::RParen,
                                       [&]()
    {
      call->arguments.append(parse_expression());
      return true;
    });

    const auto rparen
      = expect(Compiler_Token_Kind::RParen, "expected ')' after argument list", "PAR003");
    call->rparen_span = rparen.span;
    call->span = compiler_merge_spans(call->span, rparen.span);
    return call;
  }

  /** @brief Parses postfix expressions such as calls. */
  Compiler_Expr *parse_postfix()
  {
    auto *expr = parse_primary();
    while (peek().kind == Compiler_Token_Kind::LParen)
      expr = parse_call_suffix(expr);
    return expr;
  }

  /** @brief Parses prefix unary expressions: `!`, `-`, `+`, `~`. */
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

  /** @brief Parses binary expressions using precedence-climbing. */
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

  /** @brief Parses a named type such as `Int` or `T`. */
  Compiler_Type_Expr *parse_named_type()
  {
    const auto tok = peek();
    if (tok.kind != Compiler_Token_Kind::Identifier)
      {
        emit_error(tok.span, "expected type name", "PAR007");
        if (not tok.is_eof())
          (void) next();
        return invalid_type_expr(tok.span);
      }

    const auto name = next();
    return ast->make<Compiler_Named_Type_Expr>(name.lexeme, name.span);
  }

  /** @brief Parses parenthesized tuple-type syntax.
   *
   *  A single member such as `(Int)` is normalized to `Int`, while two or more
   *  members produce one tuple type-expression node.
   */
  Compiler_Type_Expr *parse_parenthesized_type()
  {
    const auto lparen = expect(Compiler_Token_Kind::LParen, "expected '(' to start type", "PAR003");

    auto *first = parse_type_expression();
    if (peek().kind != Compiler_Token_Kind::Comma)
      {
        const auto rparen = expect(Compiler_Token_Kind::RParen, "expected ')' after type", "PAR003");
        if (first != nullptr and rparen.span.is_valid())
          first->span = compiler_merge_spans(lparen.span, rparen.span);
        return first;
      }

    auto *tuple = ast->make<Compiler_Tuple_Type_Expr>(lparen.span, Source_Span());
    tuple->members.append(first);
    (void) expect(Compiler_Token_Kind::Comma, "expected ',' in tuple type", "PAR003");
    tuple->members.append(parse_type_expression());
    (void) tokens.parse_separated_list(Compiler_Token_Kind::Comma,
                                       Compiler_Token_Kind::RParen,
                                       [&]()
    {
      tuple->members.append(parse_type_expression());
      return true;
    });

    const auto rparen
      = expect(Compiler_Token_Kind::RParen, "expected ')' after tuple type", "PAR003");
    tuple->rparen_span = rparen.span;
    tuple->span = compiler_merge_spans(lparen.span, rparen.span);
    return tuple;
  }

  /** @brief Parses a function type such as `fn(Int) -> Bool`. */
  Compiler_Type_Expr *parse_function_type()
  {
    const auto kw
      = expect(Compiler_Token_Kind::Kw_Fn, "expected 'fn' to start function type", "PAR003");
    const auto lparen
      = expect(Compiler_Token_Kind::LParen, "expected '(' after 'fn' in type", "PAR003");

    auto *fn_type = ast->make<Compiler_Function_Type_Expr>(
      kw.span, lparen.span, Source_Span(), Source_Span(), nullptr);

    (void) tokens.parse_separated_list(Compiler_Token_Kind::Comma,
                                       Compiler_Token_Kind::RParen,
                                       [&]()
    {
      fn_type->parameters.append(parse_type_expression());
      return true;
    });

    const auto rparen
      = expect(Compiler_Token_Kind::RParen, "expected ')' after function type parameters", "PAR003");
    fn_type->rparen_span = rparen.span;
    const auto arrow
      = expect(Compiler_Token_Kind::Arrow, "expected '->' after function type parameters", "PAR003");
    fn_type->arrow_span = arrow.span;
    fn_type->result = parse_type_expression();
    fn_type->span
      = compiler_merge_spans(kw.span,
                             fn_type->result != nullptr ? fn_type->result->span : arrow.span);
    return fn_type;
  }

  /** @brief Parses one primary type-expression form. */
  Compiler_Type_Expr *parse_type_primary()
  {
    switch (const auto tok = peek(); tok.kind)
      {
      case Compiler_Token_Kind::Identifier:
        return parse_named_type();
      case Compiler_Token_Kind::LParen:
        return parse_parenthesized_type();
      case Compiler_Token_Kind::Kw_Fn:
        return parse_function_type();
      default:
        emit_error(tok.span, "expected type expression", "PAR007");
        if (not tok.is_eof())
          (void) next();
        return invalid_type_expr(tok.span);
      }
  }

  /** @brief Parses a type expression used by explicit annotations. */
  Compiler_Type_Expr *parse_type_expression()
  {
    return parse_type_primary();
  }

  /** @brief Parses one top-level import declaration: `import` string `;`. */
  Compiler_Import_Decl *parse_import_declaration()
  {
    const auto kw = next();
    const auto path_tok = peek();
    const auto path = expect(Compiler_Token_Kind::String_Literal,
                             "expected string literal after 'import'",
                             "PAR003");
    const auto semi = parse_statement_terminator(path.span.is_valid() ? path.span : kw.span);
    if (path_tok.kind != Compiler_Token_Kind::String_Literal)
      return invalid_import_decl(
        compiler_merge_spans(kw.span, semi.is_valid() ? semi : path_tok.span));
    return ast->make<Compiler_Source_Import_Decl>(kw.span,
                                                  compiler_unquote_string_token(path),
                                                  path.span,
                                                  semi);
  }

  /** @brief Parses one top-level type alias: `type` ident `=` type `;`. */
  Compiler_Type_Decl *parse_type_alias_declaration()
  {
    const auto kw = next();
    const auto name = expect_identifier("after 'type'");
    const auto equal
      = expect(Compiler_Token_Kind::Assign, "expected '=' in type alias declaration", "PAR003");
    auto *target = parse_type_expression();
    const auto semi = parse_statement_terminator(name.span);
    return ast->make<Compiler_Type_Alias_Decl>(kw.span, name.lexeme, name.span, equal.span, target, semi);
  }

  /** @brief Parses one top-level `struct` declaration. */
  Compiler_Type_Decl *parse_struct_declaration()
  {
    const auto kw = next();
    const auto name = expect_identifier("after 'struct'");
    const auto lbrace
      = expect(Compiler_Token_Kind::LBrace, "expected '{' after struct name", "PAR003");

    auto *decl
      = ast->make<Compiler_Struct_Decl>(kw.span, name.lexeme, name.span, lbrace.span, Source_Span());

    while (true)
      {
        const auto tok = peek();
        if (tok.is_eof() or tok.kind == Compiler_Token_Kind::RBrace)
          break;

        const auto field = expect_identifier("in struct field list");
        const auto colon
          = expect(Compiler_Token_Kind::Colon, "expected ':' after struct field name", "PAR003");
        auto *annotation = parse_type_expression();
        const auto semi = expect(Compiler_Token_Kind::Semicolon,
                                 "expected ';' after struct field declaration",
                                 "PAR003");
        decl->fields.append({field.lexeme, field.span, colon.span, annotation, semi.span});
      }

    const auto rbrace
      = expect(Compiler_Token_Kind::RBrace, "expected '}' after struct declaration", "PAR003");
    decl->rbrace_span = rbrace.span;
    decl->span = compiler_merge_spans(kw.span, rbrace.span);
    return decl;
  }

  /** @brief Parses one top-level unit-variant `enum` declaration. */
  Compiler_Type_Decl *parse_enum_declaration()
  {
    const auto kw = next();
    const auto name = expect_identifier("after 'enum'");
    const auto lbrace
      = expect(Compiler_Token_Kind::LBrace, "expected '{' after enum name", "PAR003");

    auto *decl
      = ast->make<Compiler_Enum_Decl>(kw.span, name.lexeme, name.span, lbrace.span, Source_Span());

    (void) tokens.parse_separated_list(Compiler_Token_Kind::Comma,
                                       Compiler_Token_Kind::RBrace,
                                       [&]()
    {
      const auto variant = expect_identifier("in enum variant list");
      decl->variants.append({variant.lexeme, variant.span});
      return true;
    },
                                       true);

    const auto rbrace
      = expect(Compiler_Token_Kind::RBrace, "expected '}' after enum declaration", "PAR003");
    decl->rbrace_span = rbrace.span;
    decl->span = compiler_merge_spans(kw.span, rbrace.span);
    return decl;
  }

  /** @brief Parses a block statement: `{` statements* `}`. */
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

  /** @brief Parses a variable declaration: `let` ident (`:` type)? (`=` expr)? `;`. */
  Compiler_Stmt *parse_let_statement()
  {
    const auto kw = next();
    const auto name = expect_identifier("after 'let'");
    Source_Span colon;
    Compiler_Type_Expr *annotation = nullptr;

    if (peek().kind == Compiler_Token_Kind::Colon)
      {
        colon = next().span;
        annotation = parse_type_expression();
      }

    Compiler_Expr *init = nullptr;
    if (match(Compiler_Token_Kind::Assign))
      init = parse_expression();

    const auto semi = parse_statement_terminator(name.span);
    return ast->make<Compiler_Let_Stmt>(kw.span, name.lexeme, name.span, colon, annotation, init, semi);
  }

  /** @brief Parses a return statement: `return` expr? `;`. */
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

  /** @brief Parses a break statement: `break;`. */
  Compiler_Stmt *parse_break_statement()
  {
    const auto kw = next();
    const auto semi = parse_statement_terminator(kw.span);
    return ast->make<Compiler_Break_Stmt>(kw.span, semi);
  }

  /** @brief Parses a continue statement: `continue;`. */
  Compiler_Stmt *parse_continue_statement()
  {
    const auto kw = next();
    const auto semi = parse_statement_terminator(kw.span);
    return ast->make<Compiler_Continue_Stmt>(kw.span, semi);
  }

  /** @brief Parses an if statement: `if` expr stmt (`else` stmt)?. */
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

  /** @brief Parses a while loop: `while` expr stmt. */
  Compiler_Stmt *parse_while_statement()
  {
    const auto kw = next();
    auto *condition = parse_expression();
    auto *body = parse_statement();
    return ast->make<Compiler_While_Stmt>(kw.span, condition, body);
  }

  /** @brief Parses an expression statement: expr `;`. */
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
   *  @note Thread-safety: Not thread-safe.
   *  @note Complexity: O(1).
   */
  Compiler_Parser(Compiler_Ast_Context &ctx,
                  const Source_Manager &sm,
                  const Source_File_Id file_id,
                  Diagnostic_Engine *dx = nullptr,
                  const Compiler_Parser_Options &opts = {})
    : ast(&ctx), diagnostics(dx), lexer(sm, file_id, dx), tokens(lexer, dx), options(opts),
      source_name(sm.file_name(file_id))
  {}

  /** @brief Parses a full module until EOF.
   *
   *  Processes top-level imports, type declarations, functions, and optional
   *  top-level statements depending on `options`.
   *
   *  @return Arena-backed module node containing imports, type declarations,
   *          functions, and statements.
   *  @note Thread-safety: Not thread-safe.
   *  @note Complexity: O(N) where N is source length.
   */
  Compiler_Module *parse_module()
  {
    auto *module = ast->make<Compiler_Module>();
    module->source_name = source_name;
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

        if (tok.kind == Compiler_Token_Kind::Kw_Import)
          {
            auto *decl = parse_import_declaration();
            module->imports.append(decl);
            last_span = decl != nullptr ? decl->span : tok.span;
            continue;
          }

        if (tok.kind == Compiler_Token_Kind::Kw_Type)
          {
            auto *decl = parse_type_alias_declaration();
            module->type_declarations.append(decl);
            last_span = decl != nullptr ? decl->span : tok.span;
            continue;
          }

        if (tok.kind == Compiler_Token_Kind::Kw_Struct)
          {
            auto *decl = parse_struct_declaration();
            module->type_declarations.append(decl);
            last_span = decl != nullptr ? decl->span : tok.span;
            continue;
          }

        if (tok.kind == Compiler_Token_Kind::Kw_Enum)
          {
            auto *decl = parse_enum_declaration();
            module->type_declarations.append(decl);
            last_span = decl != nullptr ? decl->span : tok.span;
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
   *  Syntax: `fn` ident `(` params? `)` (`->` type)? block.
   *
   *  @return Function declaration node. Recovery still returns a node on error.
   *  @note Thread-safety: Not thread-safe.
   *  @note Complexity: O(N) where N is source length.
   */
  Compiler_Function_Decl *parse_function()
  {
    const auto kw
      = expect(Compiler_Token_Kind::Kw_Fn, "expected 'fn' to start function declaration", "PAR003");
    const auto name = expect_identifier("after 'fn'");

    (void) expect(Compiler_Token_Kind::LParen, "expected '(' after function name", "PAR003");

    auto *fn = ast->make<Compiler_Function_Decl>(
      kw.span, name.lexeme, name.span, Source_Span(), nullptr, nullptr);

    (void) tokens.parse_separated_list(Compiler_Token_Kind::Comma,
                                       Compiler_Token_Kind::RParen,
                                       [&]()
    {
      const auto param = expect_identifier("in parameter list");
      Source_Span colon;
      Compiler_Type_Expr *annotation = nullptr;
      if (peek().kind == Compiler_Token_Kind::Colon)
        {
          colon = next().span;
          annotation = parse_type_expression();
        }
      fn->parameters.append({param.lexeme, param.span, colon, annotation});
      return true;
    });

    (void) expect(Compiler_Token_Kind::RParen, "expected ')' after parameter list", "PAR003");

    if (peek().kind == Compiler_Token_Kind::Arrow)
      {
        fn->arrow_span = next().span;
        fn->return_annotation = parse_type_expression();
      }

    fn->body = parse_block_statement();
    fn->span = compiler_merge_spans(kw.span, fn->body != nullptr ? fn->body->span : name.span);
    return fn;
  }

  /** @brief Parses a single statement.
   *
   *  Dispatches to specific statement parsers based on the next token.
   *
   *  @return Statement node. Invalid input yields `Compiler_Invalid_Stmt`.
   *  @note Thread-safety: Not thread-safe.
   *  @note Complexity: O(N) where N is source length.
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
        if (compiler_is_expression_start(tok.kind))
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
