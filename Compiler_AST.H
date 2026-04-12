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

/** @file Compiler_AST.H
 *  @brief Arena-backed AST node model for the compiler-support MVP.
 *
 *  This header defines a small typed AST for expressions, statements, function
 *  declarations, and modules. Nodes are allocated through `Compiler_Ast_Context`
 *  and remain valid until the context is reset or destroyed.
 *
 *  The current grammar-oriented AST is intentionally compact: it covers the
 *  language surface parsed by `Compiler_Parser.H` without committing yet to a
 *  richer semantic layer.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_AST_H
#define COMPILER_AST_H

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <Compiler_Token.H>
#include <ah-arena.H>
#include <ah-errors.H>
#include <tpl_dynArray.H>

namespace Aleph {
struct Compiler_Expr;
struct Compiler_Stmt;
struct Compiler_Block_Stmt;
struct Compiler_Function_Decl;
struct Compiler_Module;

/** @brief Merges two spans from the same source file.
 *
 *  If either span is invalid, the other is returned unchanged.
 *
 *  @param lhs Left span.
 *  @param rhs Right span.
 *  @return Span covering the minimum begin and maximum end.
 */
inline Source_Span compiler_merge_spans(const Source_Span &lhs, const Source_Span &rhs) noexcept
{
  if (not lhs.is_valid())
    return rhs;
  if (not rhs.is_valid())
    return lhs;
  if (lhs.file_id != rhs.file_id)
    return lhs;
  return {lhs.file_id,
          lhs.begin < rhs.begin ? lhs.begin : rhs.begin,
          lhs.end > rhs.end ? lhs.end : rhs.end};
}

/** @brief Kinds of expression nodes produced by the MVP parser. */
enum class Compiler_Expr_Kind
{
  Invalid,
  Identifier,
  Integer_Literal,
  String_Literal,
  Char_Literal,
  Bool_Literal,
  Grouping,
  Unary,
  Binary,
  Call
};

/** @brief Kinds of statement nodes produced by the MVP parser. */
enum class Compiler_Stmt_Kind
{
  Invalid,
  Expr,
  Let,
  Return,
  Block,
  If,
  While,
  Break,
  Continue
};

/** @brief Returns a stable debug name for an expression kind. */
inline const char *compiler_expr_kind_name(const Compiler_Expr_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Expr_Kind::Invalid:
      return "Invalid";
    case Compiler_Expr_Kind::Identifier:
      return "Identifier";
    case Compiler_Expr_Kind::Integer_Literal:
      return "Integer_Literal";
    case Compiler_Expr_Kind::String_Literal:
      return "String_Literal";
    case Compiler_Expr_Kind::Char_Literal:
      return "Char_Literal";
    case Compiler_Expr_Kind::Bool_Literal:
      return "Bool_Literal";
    case Compiler_Expr_Kind::Grouping:
      return "Grouping";
    case Compiler_Expr_Kind::Unary:
      return "Unary";
    case Compiler_Expr_Kind::Binary:
      return "Binary";
    case Compiler_Expr_Kind::Call:
      return "Call";
    }

  return "Unknown";
}

/** @brief Returns a stable debug name for a statement kind. */
inline const char *compiler_stmt_kind_name(const Compiler_Stmt_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Stmt_Kind::Invalid:
      return "Invalid";
    case Compiler_Stmt_Kind::Expr:
      return "Expr";
    case Compiler_Stmt_Kind::Let:
      return "Let";
    case Compiler_Stmt_Kind::Return:
      return "Return";
    case Compiler_Stmt_Kind::Block:
      return "Block";
    case Compiler_Stmt_Kind::If:
      return "If";
    case Compiler_Stmt_Kind::While:
      return "While";
    case Compiler_Stmt_Kind::Break:
      return "Break";
    case Compiler_Stmt_Kind::Continue:
      return "Continue";
    }

  return "Unknown";
}

/** @brief Base node carrying a source span. */
struct Compiler_Ast_Node
{
  Source_Span span;  ///< Source region covered by the node.

  explicit Compiler_Ast_Node(const Source_Span &sp = {}) noexcept : span(sp) {}
};

/** @brief Arena-backed ownership context for AST nodes.
 *
 *  The context allocates nodes inside an `AhArenaAllocator` but also tracks
 *  exact destructors so `std::string` and `DynArray` members are properly
 *  destroyed on reset.
 */
class Compiler_Ast_Context
{
  struct Owned_Object
  {
    void *ptr = nullptr;
    void (*destroy)(void *) noexcept = nullptr;
  };

  AhArenaAllocator arena;
  DynArray<Owned_Object> owned;

public:
  /** @brief Constructs a context with `arena_size` bytes.
   *
   *  @param arena_size Arena capacity in bytes.
   *  @throws std::runtime_error If the arena cannot be created.
   */
  explicit Compiler_Ast_Context(const size_t arena_size = AhArenaAllocator::DEFAULT_SIZE)
    : arena(arena_size)
  {}

  Compiler_Ast_Context(const Compiler_Ast_Context &) = delete;
  Compiler_Ast_Context &operator=(const Compiler_Ast_Context &) = delete;

  ~Compiler_Ast_Context() noexcept
  {
    reset();
  }

  /** @brief Allocates and constructs an AST object of type `T`.
   *
   *  @tparam T Node type to allocate.
   *  @param args Constructor arguments forwarded to `T`.
   *  @return Pointer stable until `reset()` or destruction.
   *  @throws std::runtime_error If arena allocation fails.
   */
  template <typename T, typename... Args>
  T *make(Args &&...args)
  {
    T *ptr = allocate<T>(arena, std::forward<Args>(args)...);
    ah_runtime_error_unless(ptr != nullptr) << "Compiler_Ast_Context: arena allocation failed";

    owned.append({ptr,
                  [](void *raw) noexcept
    {
      static_cast<T *>(raw)->~T();
    }});
    return ptr;
  }

  /** @brief Destroys all tracked objects and rewinds the arena.
   *
   *  All pointers returned by `make()` become invalid after this call.
   */
  void reset() noexcept
  {
    for (size_t i = owned.size(); i > 0; --i)
      {
        auto &[ptr, destroy] = owned.access(i - 1);
        if (destroy != nullptr and ptr != nullptr)
          destroy(ptr);
      }
    owned.clear();
    arena.reset();
  }

  /** @brief Returns bytes currently consumed in the arena. */
  size_t allocated_size() const noexcept
  {
    return arena.allocated_size();
  }

  /** @brief Returns total arena capacity in bytes. */
  size_t capacity() const noexcept
  {
    return arena.capacity();
  }
};

/** @brief Base class for expression nodes. */
struct Compiler_Expr : public Compiler_Ast_Node
{
  Compiler_Expr_Kind kind = Compiler_Expr_Kind::Invalid;  ///< Runtime node kind.

  Compiler_Expr(const Compiler_Expr_Kind k = Compiler_Expr_Kind::Invalid,
                const Source_Span &sp = {}) noexcept
    : Compiler_Ast_Node(sp), kind(k)
  {}
};

/** @brief Base class for statement nodes. */
struct Compiler_Stmt : public Compiler_Ast_Node
{
  Compiler_Stmt_Kind kind = Compiler_Stmt_Kind::Invalid;  ///< Runtime node kind.

  Compiler_Stmt(const Compiler_Stmt_Kind k = Compiler_Stmt_Kind::Invalid,
                const Source_Span &sp = {}) noexcept
    : Compiler_Ast_Node(sp), kind(k)
  {}
};

/** @brief Invalid expression placeholder used after parse errors. */
struct Compiler_Invalid_Expr final : public Compiler_Expr
{
  explicit Compiler_Invalid_Expr(const Source_Span &sp = {}) noexcept
    : Compiler_Expr(Compiler_Expr_Kind::Invalid, sp)
  {}
};

/** @brief Identifier reference expression. */
struct Compiler_Identifier_Expr final : public Compiler_Expr
{
  std::string name;       ///< Identifier spelling.
  Source_Span name_span;  ///< Span of the identifier token.

  Compiler_Identifier_Expr(std::string value = "", const Source_Span &sp = {}) noexcept
    : Compiler_Expr(Compiler_Expr_Kind::Identifier, sp), name(std::move(value)), name_span(sp)
  {}
};

/** @brief Literal expression storing its original lexeme. */
struct Compiler_Literal_Expr final : public Compiler_Expr
{
  std::string lexeme;       ///< Original source spelling.
  bool bool_value = false;  ///< Decoded boolean value when `kind` is boolean.

  Compiler_Literal_Expr(const Compiler_Expr_Kind k = Compiler_Expr_Kind::Invalid,
                        std::string value = "",
                        const Source_Span &sp = {},
                        const bool bool_val = false) noexcept
    : Compiler_Expr(k, sp), lexeme(std::move(value)), bool_value(bool_val)
  {}
};

/** @brief Parenthesized expression preserving explicit grouping. */
struct Compiler_Grouping_Expr final : public Compiler_Expr
{
  Compiler_Expr *inner = nullptr;  ///< Expression inside the parentheses.
  Source_Span lparen_span;         ///< Span of `'('`.
  Source_Span rparen_span;         ///< Span of `')'`.

  Compiler_Grouping_Expr(Compiler_Expr *expr = nullptr,
                         const Source_Span &lparen = {},
                         const Source_Span &rparen = {}) noexcept
    : Compiler_Expr(Compiler_Expr_Kind::Grouping,
                    compiler_merge_spans(lparen,
                                         expr != nullptr ? compiler_merge_spans(expr->span, rparen)
                                                         : rparen)),
      inner(expr), lparen_span(lparen), rparen_span(rparen)
  {}
};

/** @brief Prefix unary expression. */
struct Compiler_Unary_Expr final : public Compiler_Expr
{
  Compiler_Token_Kind op = Compiler_Token_Kind::Invalid;  ///< Unary operator kind.
  Source_Span operator_span;                              ///< Span of the operator token.
  Compiler_Expr *operand = nullptr;                       ///< Operand expression.

  Compiler_Unary_Expr(const Compiler_Token_Kind op_kind = Compiler_Token_Kind::Invalid,
                      const Source_Span &op_sp = {},
                      Compiler_Expr *expr = nullptr) noexcept
    : Compiler_Expr(Compiler_Expr_Kind::Unary,
                    compiler_merge_spans(op_sp, expr != nullptr ? expr->span : Source_Span())),
      op(op_kind), operator_span(op_sp), operand(expr)
  {}
};

/** @brief Binary operator expression. */
struct Compiler_Binary_Expr final : public Compiler_Expr
{
  Compiler_Token_Kind op = Compiler_Token_Kind::Invalid;  ///< Operator kind.
  Source_Span operator_span;                              ///< Span of the operator token.
  Compiler_Expr *left = nullptr;                          ///< Left-hand operand.
  Compiler_Expr *right = nullptr;                         ///< Right-hand operand.

  Compiler_Binary_Expr(Compiler_Expr *lhs = nullptr,
                       const Compiler_Token_Kind op_kind = Compiler_Token_Kind::Invalid,
                       const Source_Span &op_sp = {},
                       Compiler_Expr *rhs = nullptr) noexcept
    : Compiler_Expr(Compiler_Expr_Kind::Binary,
                    lhs != nullptr
                      ? compiler_merge_spans(lhs->span, rhs != nullptr ? rhs->span : op_sp)
                      : compiler_merge_spans(op_sp, rhs != nullptr ? rhs->span : Source_Span())),
      op(op_kind), operator_span(op_sp), left(lhs), right(rhs)
  {}
};

/** @brief Function-call expression. */
struct Compiler_Call_Expr final : public Compiler_Expr
{
  Compiler_Expr *callee = nullptr;      ///< Called expression.
  DynArray<Compiler_Expr *> arguments;  ///< Call arguments in source order.
  Source_Span lparen_span;              ///< Span of `'('`.
  Source_Span rparen_span;              ///< Span of `')'`.

  Compiler_Call_Expr(Compiler_Expr *func = nullptr,
                     const Source_Span &lparen = {},
                     const Source_Span &rparen = {}) noexcept
    : Compiler_Expr(Compiler_Expr_Kind::Call,
                    func != nullptr ? compiler_merge_spans(func->span, rparen)
                                    : compiler_merge_spans(lparen, rparen)),
      callee(func), lparen_span(lparen), rparen_span(rparen)
  {}
};

/** @brief Invalid statement placeholder used after parse errors. */
struct Compiler_Invalid_Stmt final : public Compiler_Stmt
{
  explicit Compiler_Invalid_Stmt(const Source_Span &sp = {}) noexcept
    : Compiler_Stmt(Compiler_Stmt_Kind::Invalid, sp)
  {}
};

/** @brief Expression statement terminated by a semicolon. */
struct Compiler_Expr_Stmt final : public Compiler_Stmt
{
  Compiler_Expr *expr = nullptr;  ///< Expression value.
  Source_Span semicolon_span;     ///< Span of the terminating `';'`.

  Compiler_Expr_Stmt(Compiler_Expr *value = nullptr, const Source_Span &semi = {}) noexcept
    : Compiler_Stmt(Compiler_Stmt_Kind::Expr,
                    value != nullptr ? compiler_merge_spans(value->span, semi) : semi),
      expr(value), semicolon_span(semi)
  {}
};

/** @brief Variable-binding statement introduced by `let`. */
struct Compiler_Let_Stmt final : public Compiler_Stmt
{
  Source_Span keyword_span;              ///< Span of `let`.
  std::string name;                      ///< Bound identifier.
  Source_Span name_span;                 ///< Span of the binding name.
  Compiler_Expr *initializer = nullptr;  ///< Optional initializer.
  Source_Span semicolon_span;            ///< Span of the terminating semicolon.

  Compiler_Let_Stmt(const Source_Span &kw = {},
                    std::string value = "",
                    const Source_Span &name_sp = {},
                    Compiler_Expr *init = nullptr,
                    const Source_Span &semi = {}) noexcept
    : Compiler_Stmt(
        Compiler_Stmt_Kind::Let,
        compiler_merge_spans(kw, semi.is_valid() ? semi : (init != nullptr ? init->span : name_sp))),
      keyword_span(kw), name(std::move(value)), name_span(name_sp), initializer(init),
      semicolon_span(semi)
  {}
};

/** @brief Return statement with optional value. */
struct Compiler_Return_Stmt final : public Compiler_Stmt
{
  Source_Span keyword_span;        ///< Span of `return`.
  Compiler_Expr *value = nullptr;  ///< Optional return value.
  Source_Span semicolon_span;      ///< Span of the terminating semicolon.

  Compiler_Return_Stmt(const Source_Span &kw = {},
                       Compiler_Expr *expr = nullptr,
                       const Source_Span &semi = {}) noexcept
    : Compiler_Stmt(Compiler_Stmt_Kind::Return,
                    compiler_merge_spans(
                      kw, semi.is_valid() ? semi : (expr != nullptr ? expr->span : Source_Span()))),
      keyword_span(kw), value(expr), semicolon_span(semi)
  {}
};

/** @brief Statement sequence enclosed by braces. */
struct Compiler_Block_Stmt final : public Compiler_Stmt
{
  DynArray<Compiler_Stmt *> statements;  ///< Nested statements.
  Source_Span lbrace_span;               ///< Span of `'{'`.
  Source_Span rbrace_span;               ///< Span of `'}'`.

  Compiler_Block_Stmt(const Source_Span &lbrace = {}, const Source_Span &rbrace = {}) noexcept
    : Compiler_Stmt(Compiler_Stmt_Kind::Block, compiler_merge_spans(lbrace, rbrace)),
      lbrace_span(lbrace), rbrace_span(rbrace)
  {}
};

/** @brief Conditional statement with optional else-branch. */
struct Compiler_If_Stmt final : public Compiler_Stmt
{
  Source_Span if_span;                   ///< Span of `if`.
  Compiler_Expr *condition = nullptr;    ///< Branch condition.
  Compiler_Stmt *then_branch = nullptr;  ///< Statement executed on true.
  Source_Span else_span;                 ///< Span of `else`, if present.
  Compiler_Stmt *else_branch = nullptr;  ///< Optional false branch.

  Compiler_If_Stmt(const Source_Span &if_kw = {},
                   Compiler_Expr *cond = nullptr,
                   Compiler_Stmt *then_stmt = nullptr,
                   const Source_Span &else_kw = {},
                   Compiler_Stmt *else_stmt = nullptr) noexcept
    : Compiler_Stmt(Compiler_Stmt_Kind::If,
                    compiler_merge_spans(if_kw,
                                         else_stmt != nullptr
                                           ? else_stmt->span
                                           : (then_stmt != nullptr
                                                ? then_stmt->span
                                                : (cond != nullptr ? cond->span : else_kw)))),
      if_span(if_kw), condition(cond), then_branch(then_stmt), else_span(else_kw),
      else_branch(else_stmt)
  {}
};

/** @brief While-loop statement. */
struct Compiler_While_Stmt final : public Compiler_Stmt
{
  Source_Span keyword_span;            ///< Span of `while`.
  Compiler_Expr *condition = nullptr;  ///< Loop condition.
  Compiler_Stmt *body = nullptr;       ///< Loop body.

  Compiler_While_Stmt(const Source_Span &kw = {},
                      Compiler_Expr *cond = nullptr,
                      Compiler_Stmt *stmt = nullptr) noexcept
    : Compiler_Stmt(Compiler_Stmt_Kind::While,
                    compiler_merge_spans(kw,
                                         stmt != nullptr
                                           ? stmt->span
                                           : (cond != nullptr ? cond->span : Source_Span()))),
      keyword_span(kw), condition(cond), body(stmt)
  {}
};

/** @brief `break;` statement. */
struct Compiler_Break_Stmt final : public Compiler_Stmt
{
  Source_Span keyword_span;    ///< Span of `break`.
  Source_Span semicolon_span;  ///< Span of the semicolon.

  Compiler_Break_Stmt(const Source_Span &kw = {}, const Source_Span &semi = {}) noexcept
    : Compiler_Stmt(Compiler_Stmt_Kind::Break, compiler_merge_spans(kw, semi)), keyword_span(kw),
      semicolon_span(semi)
  {}
};

/** @brief `continue;` statement. */
struct Compiler_Continue_Stmt final : public Compiler_Stmt
{
  Source_Span keyword_span;    ///< Span of `continue`.
  Source_Span semicolon_span;  ///< Span of the semicolon.

  Compiler_Continue_Stmt(const Source_Span &kw = {}, const Source_Span &semi = {}) noexcept
    : Compiler_Stmt(Compiler_Stmt_Kind::Continue, compiler_merge_spans(kw, semi)), keyword_span(kw),
      semicolon_span(semi)
  {}
};

/** @brief Function parameter metadata. */
struct Compiler_Param
{
  std::string name;  ///< Parameter name.
  Source_Span span;  ///< Parameter source span.
};

/** @brief Top-level function declaration. */
struct Compiler_Function_Decl : public Compiler_Ast_Node
{
  Source_Span keyword_span;             ///< Span of `fn`.
  std::string name;                     ///< Function name.
  Source_Span name_span;                ///< Span of the function name.
  DynArray<Compiler_Param> parameters;  ///< Parameters in source order.
  Compiler_Block_Stmt *body = nullptr;  ///< Function body block.

  Compiler_Function_Decl(const Source_Span &kw = {},
                         std::string value = "",
                         const Source_Span &name_sp = {},
                         Compiler_Block_Stmt *block = nullptr) noexcept
    : Compiler_Ast_Node(compiler_merge_spans(kw, block != nullptr ? block->span : name_sp)),
      keyword_span(kw), name(std::move(value)), name_span(name_sp), body(block)
  {}
};

/** @brief Parsed translation unit. */
struct Compiler_Module : public Compiler_Ast_Node
{
  DynArray<Compiler_Function_Decl *> functions;  ///< Top-level functions.
  DynArray<Compiler_Stmt *> statements;          ///< Optional top-level statements.

  explicit Compiler_Module(const Source_Span &sp = {}) noexcept : Compiler_Ast_Node(sp) {}
};

namespace Compiler_Ast_Detail {
inline std::string escape_text(std::string_view text)
{
  std::string escaped;
  escaped.reserve(text.size());
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

inline void append_indent(std::ostream &out, const size_t indent)
{
  out << std::string(indent, ' ');
}

inline void dump_expr(const Compiler_Expr *expr, std::ostream &out, const size_t indent);

inline void dump_stmt(const Compiler_Stmt *stmt, std::ostream &out, const size_t indent);

inline void dump_expr(const Compiler_Expr *expr, std::ostream &out, const size_t indent)
{
  append_indent(out, indent);
  if (expr == nullptr)
    {
      out << "<null-expr>\n";
      return;
    }

  switch (expr->kind)
    {
    case Compiler_Expr_Kind::Invalid:
      out << "InvalidExpr\n";
      break;
    case Compiler_Expr_Kind::Identifier:
      {
        const auto *node = static_cast<const Compiler_Identifier_Expr *>(expr);
        out << "Identifier(" << node->name << ")\n";
        break;
      }
    case Compiler_Expr_Kind::Integer_Literal:
      {
        const auto *node = static_cast<const Compiler_Literal_Expr *>(expr);
        out << "Integer(" << node->lexeme << ")\n";
        break;
      }
    case Compiler_Expr_Kind::String_Literal:
      {
        const auto *node = static_cast<const Compiler_Literal_Expr *>(expr);
        out << "String(\"" << escape_text(node->lexeme) << "\")\n";
        break;
      }
    case Compiler_Expr_Kind::Char_Literal:
      {
        const auto *node = static_cast<const Compiler_Literal_Expr *>(expr);
        out << "Char(\"" << escape_text(node->lexeme) << "\")\n";
        break;
      }
    case Compiler_Expr_Kind::Bool_Literal:
      {
        const auto *node = static_cast<const Compiler_Literal_Expr *>(expr);
        out << "Bool(" << (node->bool_value ? "true" : "false") << ")\n";
        break;
      }
    case Compiler_Expr_Kind::Grouping:
      {
        const auto *node = static_cast<const Compiler_Grouping_Expr *>(expr);
        out << "Grouping\n";
        dump_expr(node->inner, out, indent + 2);
        break;
      }
    case Compiler_Expr_Kind::Unary:
      {
        const auto *node = static_cast<const Compiler_Unary_Expr *>(expr);
        out << "Unary(" << compiler_token_kind_name(node->op) << ")\n";
        dump_expr(node->operand, out, indent + 2);
        break;
      }
    case Compiler_Expr_Kind::Binary:
      {
        const auto *node = static_cast<const Compiler_Binary_Expr *>(expr);
        out << "Binary(" << compiler_token_kind_name(node->op) << ")\n";
        dump_expr(node->left, out, indent + 2);
        dump_expr(node->right, out, indent + 2);
        break;
      }
    case Compiler_Expr_Kind::Call:
      {
        const auto *node = static_cast<const Compiler_Call_Expr *>(expr);
        out << "Call\n";
        append_indent(out, indent + 2);
        out << "Callee:\n";
        dump_expr(node->callee, out, indent + 4);
        append_indent(out, indent + 2);
        out << "Args:\n";
        for (size_t i = 0; i < node->arguments.size(); ++i)
          dump_expr(node->arguments.access(i), out, indent + 4);
        break;
      }
    }
}

inline void dump_stmt(const Compiler_Stmt *stmt, std::ostream &out, const size_t indent)
{
  append_indent(out, indent);
  if (stmt == nullptr)
    {
      out << "<null-stmt>\n";
      return;
    }

  switch (stmt->kind)
    {
    case Compiler_Stmt_Kind::Invalid:
      out << "InvalidStmt\n";
      break;
    case Compiler_Stmt_Kind::Expr:
      {
        const auto *node = static_cast<const Compiler_Expr_Stmt *>(stmt);
        out << "ExprStmt\n";
        dump_expr(node->expr, out, indent + 2);
        break;
      }
    case Compiler_Stmt_Kind::Let:
      {
        const auto *node = static_cast<const Compiler_Let_Stmt *>(stmt);
        out << "Let(" << node->name << ")\n";
        if (node->initializer != nullptr)
          dump_expr(node->initializer, out, indent + 2);
        break;
      }
    case Compiler_Stmt_Kind::Return:
      {
        const auto *node = static_cast<const Compiler_Return_Stmt *>(stmt);
        out << "Return\n";
        if (node->value != nullptr)
          dump_expr(node->value, out, indent + 2);
        break;
      }
    case Compiler_Stmt_Kind::Block:
      {
        const auto *node = static_cast<const Compiler_Block_Stmt *>(stmt);
        out << "Block\n";
        for (size_t i = 0; i < node->statements.size(); ++i)
          dump_stmt(node->statements.access(i), out, indent + 2);
        break;
      }
    case Compiler_Stmt_Kind::If:
      {
        const auto *node = static_cast<const Compiler_If_Stmt *>(stmt);
        out << "If\n";
        append_indent(out, indent + 2);
        out << "Condition:\n";
        dump_expr(node->condition, out, indent + 4);
        append_indent(out, indent + 2);
        out << "Then:\n";
        dump_stmt(node->then_branch, out, indent + 4);
        if (node->else_branch != nullptr)
          {
            append_indent(out, indent + 2);
            out << "Else:\n";
            dump_stmt(node->else_branch, out, indent + 4);
          }
        break;
      }
    case Compiler_Stmt_Kind::While:
      {
        const auto *node = static_cast<const Compiler_While_Stmt *>(stmt);
        out << "While\n";
        append_indent(out, indent + 2);
        out << "Condition:\n";
        dump_expr(node->condition, out, indent + 4);
        append_indent(out, indent + 2);
        out << "Body:\n";
        dump_stmt(node->body, out, indent + 4);
        break;
      }
    case Compiler_Stmt_Kind::Break:
      out << "Break\n";
      break;
    case Compiler_Stmt_Kind::Continue:
      out << "Continue\n";
      break;
    }
}
}  // namespace Compiler_Ast_Detail

/** @brief Dumps an expression subtree in a deterministic text format. */
inline std::string compiler_dump_expr(const Compiler_Expr *expr)
{
  std::ostringstream out;
  Compiler_Ast_Detail::dump_expr(expr, out, 0);
  return out.str();
}

/** @brief Dumps a statement subtree in a deterministic text format. */
inline std::string compiler_dump_stmt(const Compiler_Stmt *stmt)
{
  std::ostringstream out;
  Compiler_Ast_Detail::dump_stmt(stmt, out, 0);
  return out.str();
}

/** @brief Dumps a function declaration in a deterministic text format. */
inline std::string compiler_dump_function(const Compiler_Function_Decl *fn)
{
  std::ostringstream out;
  if (fn == nullptr)
    return "<null-function>\n";

  out << "Function(" << fn->name << ")\n";
  if (fn->parameters.size() > 0)
    {
      out << "  Params:";
      for (size_t i = 0; i < fn->parameters.size(); ++i)
        {
          if (i > 0)
            out << ",";
          out << " " << fn->parameters.access(i).name;
        }
      out << '\n';
    }
  else
    out << "  Params:\n";

  out << "  Body:\n";
  Compiler_Ast_Detail::dump_stmt(fn->body, out, 4);
  return out.str();
}

/** @brief Dumps a parsed module in a deterministic text format. */
inline std::string compiler_dump_module(const Compiler_Module *module)
{
  std::ostringstream out;
  if (module == nullptr)
    return "<null-module>\n";

  out << "Module\n";
  for (size_t i = 0; i < module->functions.size(); ++i)
    {
      const auto *fn = module->functions.access(i);
      Compiler_Ast_Detail::append_indent(out, 2);
      out << "Function(" << fn->name << ")\n";
      if (fn->parameters.size() > 0)
        {
          Compiler_Ast_Detail::append_indent(out, 4);
          out << "Params:";
          for (size_t j = 0; j < fn->parameters.size(); ++j)
            {
              if (j > 0)
                out << ",";
              out << " " << fn->parameters.access(j).name;
            }
          out << '\n';
        }
      else
        {
          Compiler_Ast_Detail::append_indent(out, 4);
          out << "Params:\n";
        }

      Compiler_Ast_Detail::append_indent(out, 4);
      out << "Body:\n";
      Compiler_Ast_Detail::dump_stmt(fn->body, out, 6);
    }

  for (size_t i = 0; i < module->statements.size(); ++i)
    Compiler_Ast_Detail::dump_stmt(module->statements.access(i), out, 2);

  return out.str();
}
}  // namespace Aleph

#endif  // COMPILER_AST_H
