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

/** @file Compiler_HIR_Model.H
 *  @brief Reusable typed high-level IR model independent from any concrete frontend.
 *
 *  This header defines the stable HIR node model, ownership context, and
 *  deterministic dump helpers. It is the reusable boundary that language
 *  frontends can target before later execution or lowering stages.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_HIR_MODEL_H
#define COMPILER_HIR_MODEL_H

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <Compiler_Operator.H>
#include <Compiler_Types.H>
#include <ah-arena.H>
#include <ah-errors.H>
#include <tpl_dynArray.H>

namespace Aleph {
/** @brief Merges two spans without depending on the parser-oriented AST layer. */
inline Source_Span compiler_hir_merge_spans(const Source_Span &lhs, const Source_Span &rhs) noexcept
{
  if (not lhs.is_valid())
    return rhs;
  if (not rhs.is_valid())
    return lhs;
  return lhs.file_id == rhs.file_id ? Source_Span{lhs.file_id, lhs.begin, rhs.end} : lhs;
}

struct Compiler_HIR_Expr;
struct Compiler_HIR_Stmt;
struct Compiler_HIR_Block_Stmt;
struct Compiler_HIR_Function;
struct Compiler_HIR_Module;

/** @brief Kinds of HIR expressions. */
enum class Compiler_HIR_Expr_Kind
{
  Invalid,
  Constant,
  Variable,
  Unary,
  Binary,
  Call
};

/** @brief Kinds of HIR statements. */
enum class Compiler_HIR_Stmt_Kind
{
  Invalid,
  Eval,
  Let,
  Return,
  Block,
  If,
  While,
  Break,
  Continue
};

/** @brief Kinds of HIR constants. */
enum class Compiler_HIR_Constant_Kind
{
  Integer,
  String,
  Character,
  Bool,
  Unit
};

/** @brief Returns a stable debug name for an HIR expression kind. */
inline const char *compiler_hir_expr_kind_name(const Compiler_HIR_Expr_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_HIR_Expr_Kind::Invalid:
      return "Invalid";
    case Compiler_HIR_Expr_Kind::Constant:
      return "Constant";
    case Compiler_HIR_Expr_Kind::Variable:
      return "Variable";
    case Compiler_HIR_Expr_Kind::Unary:
      return "Unary";
    case Compiler_HIR_Expr_Kind::Binary:
      return "Binary";
    case Compiler_HIR_Expr_Kind::Call:
      return "Call";
    }

  return "Unknown";
}

/** @brief Returns a stable debug name for an HIR statement kind. */
inline const char *compiler_hir_stmt_kind_name(const Compiler_HIR_Stmt_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_HIR_Stmt_Kind::Invalid:
      return "Invalid";
    case Compiler_HIR_Stmt_Kind::Eval:
      return "Eval";
    case Compiler_HIR_Stmt_Kind::Let:
      return "Let";
    case Compiler_HIR_Stmt_Kind::Return:
      return "Return";
    case Compiler_HIR_Stmt_Kind::Block:
      return "Block";
    case Compiler_HIR_Stmt_Kind::If:
      return "If";
    case Compiler_HIR_Stmt_Kind::While:
      return "While";
    case Compiler_HIR_Stmt_Kind::Break:
      return "Break";
    case Compiler_HIR_Stmt_Kind::Continue:
      return "Continue";
    }

  return "Unknown";
}

/** @brief Returns a stable debug name for an HIR constant kind. */
inline const char *compiler_hir_constant_kind_name(const Compiler_HIR_Constant_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_HIR_Constant_Kind::Integer:
      return "Integer";
    case Compiler_HIR_Constant_Kind::String:
      return "String";
    case Compiler_HIR_Constant_Kind::Character:
      return "Char";
    case Compiler_HIR_Constant_Kind::Bool:
      return "Bool";
    case Compiler_HIR_Constant_Kind::Unit:
      return "Unit";
    }

  return "Unknown";
}

/** @brief Base node carrying a source span. */
struct Compiler_HIR_Node
{
  Source_Span span;  ///< Source region associated with the HIR node.

  explicit Compiler_HIR_Node(const Source_Span &sp = {}) noexcept : span(sp) {}
};

/** @brief Arena-backed ownership context for HIR nodes. */
class Compiler_HIR_Context
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
  explicit Compiler_HIR_Context(const size_t arena_size = AhArenaAllocator::DEFAULT_SIZE)
    : arena(arena_size)
  {}

  Compiler_HIR_Context(const Compiler_HIR_Context &) = delete;
  Compiler_HIR_Context &operator=(const Compiler_HIR_Context &) = delete;

  ~Compiler_HIR_Context() noexcept
  {
    reset();
  }

  /** @brief Allocates and constructs one HIR object.
   *
   *  @tparam T Object type to create.
   *  @param args Constructor arguments forwarded to `T`.
   *  @return Pointer stable until `reset()` or destruction.
   *  @throws std::runtime_error If arena allocation fails.
   */
  template <typename T, typename... Args>
  T *make(Args &&...args)
  {
    T *ptr = allocate<T>(arena, std::forward<Args>(args)...);
    ah_runtime_error_unless(ptr != nullptr) << "Compiler_HIR_Context: arena allocation failed";

    owned.append({ptr,
                  [](void *raw) noexcept
    {
      static_cast<T *>(raw)->~T();
    }});
    return ptr;
  }

  /** @brief Destroys all tracked objects and rewinds the arena. */
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

/** @brief Base class for HIR expressions. */
struct Compiler_HIR_Expr : public Compiler_HIR_Node
{
  Compiler_HIR_Expr_Kind kind = Compiler_HIR_Expr_Kind::Invalid;  ///< Runtime node kind.
  Compiler_Type_Id type_id = 0;                                   ///< Inferred or resolved type.

  Compiler_HIR_Expr(const Compiler_HIR_Expr_Kind k = Compiler_HIR_Expr_Kind::Invalid,
                    const Compiler_Type_Id ty = 0,
                    const Source_Span &sp = {}) noexcept
    : Compiler_HIR_Node(sp), kind(k), type_id(ty)
  {}
};

/** @brief Base class for HIR statements. */
struct Compiler_HIR_Stmt : public Compiler_HIR_Node
{
  Compiler_HIR_Stmt_Kind kind = Compiler_HIR_Stmt_Kind::Invalid;  ///< Runtime node kind.

  Compiler_HIR_Stmt(const Compiler_HIR_Stmt_Kind k = Compiler_HIR_Stmt_Kind::Invalid,
                    const Source_Span &sp = {}) noexcept
    : Compiler_HIR_Node(sp), kind(k)
  {}
};

/** @brief Invalid HIR expression placeholder. */
struct Compiler_HIR_Invalid_Expr final : public Compiler_HIR_Expr
{
  explicit Compiler_HIR_Invalid_Expr(const Compiler_Type_Id ty = 0, const Source_Span &sp = {}) noexcept
    : Compiler_HIR_Expr(Compiler_HIR_Expr_Kind::Invalid, ty, sp)
  {}
};

/** @brief Typed constant expression. */
struct Compiler_HIR_Constant_Expr final : public Compiler_HIR_Expr
{
  Compiler_HIR_Constant_Kind constant_kind
    = Compiler_HIR_Constant_Kind::Unit;  ///< Constant category.
  std::string text;                      ///< Original or normalized constant spelling.
  bool bool_value = false;               ///< Decoded boolean value for Bool constants.

  Compiler_HIR_Constant_Expr(const Compiler_HIR_Constant_Kind k = Compiler_HIR_Constant_Kind::Unit,
                             const Compiler_Type_Id ty = 0,
                             std::string value = "",
                             const Source_Span &sp = {},
                             const bool b = false) noexcept
    : Compiler_HIR_Expr(Compiler_HIR_Expr_Kind::Constant, ty, sp), constant_kind(k),
      text(std::move(value)), bool_value(b)
  {}
};

/** @brief Typed variable reference expression. */
struct Compiler_HIR_Variable_Expr final : public Compiler_HIR_Expr
{
  std::string name;       ///< Variable or function name.
  Source_Span name_span;  ///< Span of the original identifier.

  Compiler_HIR_Variable_Expr(std::string value = "",
                             const Compiler_Type_Id ty = 0,
                             const Source_Span &sp = {}) noexcept
    : Compiler_HIR_Expr(Compiler_HIR_Expr_Kind::Variable, ty, sp), name(std::move(value)),
      name_span(sp)
  {}
};

/** @brief Typed unary expression. */
struct Compiler_HIR_Unary_Expr final : public Compiler_HIR_Expr
{
  Compiler_Operator_Kind op = Compiler_Operator_Kind::Invalid;  ///< Unary operator kind.
  Source_Span operator_span;                                    ///< Span of the operator token.
  Compiler_HIR_Expr *operand = nullptr;                         ///< Operand expression.

  Compiler_HIR_Unary_Expr(const Compiler_Operator_Kind k = Compiler_Operator_Kind::Invalid,
                          const Compiler_Type_Id ty = 0,
                          const Source_Span &op_sp = {},
                          Compiler_HIR_Expr *expr = nullptr) noexcept
    : Compiler_HIR_Expr(Compiler_HIR_Expr_Kind::Unary,
                        ty,
                        compiler_hir_merge_spans(op_sp, expr != nullptr ? expr->span : Source_Span())),
      op(k), operator_span(op_sp), operand(expr)
  {}
};

/** @brief Typed binary expression. */
struct Compiler_HIR_Binary_Expr final : public Compiler_HIR_Expr
{
  Compiler_Operator_Kind op = Compiler_Operator_Kind::Invalid;  ///< Binary operator kind.
  Source_Span operator_span;                                    ///< Span of the operator token.
  Compiler_HIR_Expr *left = nullptr;                            ///< Left operand.
  Compiler_HIR_Expr *right = nullptr;                           ///< Right operand.

  Compiler_HIR_Binary_Expr(Compiler_HIR_Expr *lhs = nullptr,
                           const Compiler_Operator_Kind k = Compiler_Operator_Kind::Invalid,
                           const Compiler_Type_Id ty = 0,
                           const Source_Span &op_sp = {},
                           Compiler_HIR_Expr *rhs = nullptr) noexcept
    : Compiler_HIR_Expr(Compiler_HIR_Expr_Kind::Binary,
                        ty,
                        lhs != nullptr
                          ? compiler_hir_merge_spans(lhs->span, rhs != nullptr ? rhs->span : op_sp)
                          : compiler_hir_merge_spans(op_sp,
                                                     rhs != nullptr ? rhs->span : Source_Span())),
      op(k), operator_span(op_sp), left(lhs), right(rhs)
  {}
};

/** @brief Typed function call expression. */
struct Compiler_HIR_Call_Expr final : public Compiler_HIR_Expr
{
  Compiler_HIR_Expr *callee = nullptr;      ///< Called expression.
  DynArray<Compiler_HIR_Expr *> arguments;  ///< Call arguments.
  Source_Span lparen_span;                  ///< Span of `'('`.
  Source_Span rparen_span;                  ///< Span of `')'`.

  Compiler_HIR_Call_Expr(Compiler_HIR_Expr *func = nullptr,
                         const Compiler_Type_Id ty = 0,
                         const Source_Span &lparen = {},
                         const Source_Span &rparen = {}) noexcept
    : Compiler_HIR_Expr(Compiler_HIR_Expr_Kind::Call,
                        ty,
                        func != nullptr ? compiler_hir_merge_spans(func->span, rparen)
                                        : compiler_hir_merge_spans(lparen, rparen)),
      callee(func), lparen_span(lparen), rparen_span(rparen)
  {}
};

/** @brief Invalid HIR statement placeholder. */
struct Compiler_HIR_Invalid_Stmt final : public Compiler_HIR_Stmt
{
  explicit Compiler_HIR_Invalid_Stmt(const Source_Span &sp = {}) noexcept
    : Compiler_HIR_Stmt(Compiler_HIR_Stmt_Kind::Invalid, sp)
  {}
};

/** @brief Expression statement in HIR. */
struct Compiler_HIR_Eval_Stmt final : public Compiler_HIR_Stmt
{
  Compiler_HIR_Expr *expr = nullptr;  ///< Evaluated expression.

  explicit Compiler_HIR_Eval_Stmt(Compiler_HIR_Expr *value = nullptr,
                                  const Source_Span &sp = {}) noexcept
    : Compiler_HIR_Stmt(Compiler_HIR_Stmt_Kind::Eval, value != nullptr ? value->span : sp),
      expr(value)
  {}
};

/** @brief Typed lexical binding statement. */
struct Compiler_HIR_Let_Stmt final : public Compiler_HIR_Stmt
{
  std::string name;                          ///< Bound identifier.
  Source_Span name_span;                     ///< Span of the binding name.
  Compiler_Type_Id binding_type = 0;         ///< Inferred binding type.
  Compiler_HIR_Expr *initializer = nullptr;  ///< Optional initializer.

  Compiler_HIR_Let_Stmt(std::string value = "",
                        const Compiler_Type_Id ty = 0,
                        const Source_Span &name_sp = {},
                        Compiler_HIR_Expr *init = nullptr,
                        const Source_Span &sp = {}) noexcept
    : Compiler_HIR_Stmt(Compiler_HIR_Stmt_Kind::Let,
                        sp.is_valid()
                          ? sp
                          : (init != nullptr ? compiler_hir_merge_spans(name_sp, init->span)
                                             : name_sp)),
      name(std::move(value)), name_span(name_sp), binding_type(ty), initializer(init)
  {}
};

/** @brief Return statement in HIR. */
struct Compiler_HIR_Return_Stmt final : public Compiler_HIR_Stmt
{
  Compiler_HIR_Expr *value = nullptr;  ///< Returned value, or `nullptr`.

  explicit Compiler_HIR_Return_Stmt(Compiler_HIR_Expr *expr = nullptr,
                                    const Source_Span &sp = {}) noexcept
    : Compiler_HIR_Stmt(Compiler_HIR_Stmt_Kind::Return,
                        sp.is_valid() ? sp : (expr != nullptr ? expr->span : Source_Span())),
      value(expr)
  {}
};

/** @brief Structured block statement. */
struct Compiler_HIR_Block_Stmt final : public Compiler_HIR_Stmt
{
  DynArray<Compiler_HIR_Stmt *> statements;  ///< Nested statements.
  Source_Span lbrace_span;                   ///< Span of `'{'`.
  Source_Span rbrace_span;                   ///< Span of `'}'`.

  Compiler_HIR_Block_Stmt(const Source_Span &lbrace = {}, const Source_Span &rbrace = {}) noexcept
    : Compiler_HIR_Stmt(Compiler_HIR_Stmt_Kind::Block, compiler_hir_merge_spans(lbrace, rbrace)),
      lbrace_span(lbrace), rbrace_span(rbrace)
  {}
};

/** @brief Structured conditional statement. */
struct Compiler_HIR_If_Stmt final : public Compiler_HIR_Stmt
{
  Compiler_HIR_Expr *condition = nullptr;    ///< Condition expression.
  Compiler_HIR_Stmt *then_branch = nullptr;  ///< True branch.
  Compiler_HIR_Stmt *else_branch = nullptr;  ///< Optional false branch.
  Source_Span if_span;                       ///< Span of `if`.
  Source_Span else_span;                     ///< Span of `else`, if present.

  Compiler_HIR_If_Stmt(Compiler_HIR_Expr *cond = nullptr,
                       Compiler_HIR_Stmt *then_stmt = nullptr,
                       Compiler_HIR_Stmt *else_stmt = nullptr,
                       const Source_Span &if_sp = {},
                       const Source_Span &else_sp = {}) noexcept
    : Compiler_HIR_Stmt(Compiler_HIR_Stmt_Kind::If,
                        compiler_hir_merge_spans(if_sp,
                                                 else_stmt != nullptr
                                                   ? else_stmt->span
                                                   : (then_stmt != nullptr
                                                        ? then_stmt->span
                                                        : (cond != nullptr ? cond->span : else_sp)))),
      condition(cond), then_branch(then_stmt), else_branch(else_stmt), if_span(if_sp),
      else_span(else_sp)
  {}
};

/** @brief Structured while-loop statement. */
struct Compiler_HIR_While_Stmt final : public Compiler_HIR_Stmt
{
  Compiler_HIR_Expr *condition = nullptr;  ///< Loop condition.
  Compiler_HIR_Stmt *body = nullptr;       ///< Loop body.
  Source_Span keyword_span;                ///< Span of `while`.

  Compiler_HIR_While_Stmt(Compiler_HIR_Expr *cond = nullptr,
                          Compiler_HIR_Stmt *stmt = nullptr,
                          const Source_Span &kw = {}) noexcept
    : Compiler_HIR_Stmt(Compiler_HIR_Stmt_Kind::While,
                        compiler_hir_merge_spans(kw,
                                                 stmt != nullptr
                                                   ? stmt->span
                                                   : (cond != nullptr ? cond->span : Source_Span()))),
      condition(cond), body(stmt), keyword_span(kw)
  {}
};

/** @brief `break` statement in HIR. */
struct Compiler_HIR_Break_Stmt final : public Compiler_HIR_Stmt
{
  explicit Compiler_HIR_Break_Stmt(const Source_Span &sp = {}) noexcept
    : Compiler_HIR_Stmt(Compiler_HIR_Stmt_Kind::Break, sp)
  {}
};

/** @brief `continue` statement in HIR. */
struct Compiler_HIR_Continue_Stmt final : public Compiler_HIR_Stmt
{
  explicit Compiler_HIR_Continue_Stmt(const Source_Span &sp = {}) noexcept
    : Compiler_HIR_Stmt(Compiler_HIR_Stmt_Kind::Continue, sp)
  {}
};

/** @brief Parameter metadata for one HIR function. */
struct Compiler_HIR_Param
{
  std::string name;              ///< Parameter name.
  Source_Span span;              ///< Parameter source span.
  Compiler_Type_Id type_id = 0;  ///< Inferred or declared parameter type.
};

/** @brief Typed HIR function. */
struct Compiler_HIR_Function : public Compiler_HIR_Node
{
  std::string name;                         ///< Function name.
  Source_Span name_span;                    ///< Span of the function name.
  Compiler_Type_Id type_id = 0;             ///< Full function type.
  DynArray<Compiler_HIR_Param> parameters;  ///< Parameters in declaration order.
  Compiler_HIR_Block_Stmt *body = nullptr;  ///< Function body.

  Compiler_HIR_Function(std::string value = "",
                        const Compiler_Type_Id ty = 0,
                        const Source_Span &name_sp = {},
                        Compiler_HIR_Block_Stmt *block = nullptr,
                        const Source_Span &sp = {}) noexcept
    : Compiler_HIR_Node(
        sp.is_valid()
          ? sp
          : compiler_hir_merge_spans(name_sp, block != nullptr ? block->span : Source_Span())),
      name(std::move(value)), name_span(name_sp), type_id(ty), body(block)
  {}
};

/** @brief Top-level HIR module. */
struct Compiler_HIR_Module : public Compiler_HIR_Node
{
  DynArray<Compiler_HIR_Function *> functions;  ///< Lowered top-level functions.
  DynArray<Compiler_HIR_Stmt *> statements;     ///< Lowered top-level statements.

  explicit Compiler_HIR_Module(const Source_Span &sp = {}) noexcept : Compiler_HIR_Node(sp) {}
};

namespace Compiler_HIR_Detail {
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

inline std::string type_text(const Compiler_Type_Context *types, const Compiler_Type_Id id)
{
  if (types == nullptr)
    return "<no-types>";
  if (id == 0)
    return "<unknown-type>";
  return types->to_string(id);
}

inline void dump_expr(const Compiler_HIR_Expr *expr,
                      std::ostream &out,
                      const size_t indent,
                      const Compiler_Type_Context *types);

inline void dump_stmt(const Compiler_HIR_Stmt *stmt,
                      std::ostream &out,
                      const size_t indent,
                      const Compiler_Type_Context *types);

inline void dump_expr(const Compiler_HIR_Expr *expr,
                      std::ostream &out,
                      const size_t indent,
                      const Compiler_Type_Context *types)
{
  append_indent(out, indent);
  if (expr == nullptr)
    {
      out << "<null-hir-expr>\n";
      return;
    }

  switch (expr->kind)
    {
    case Compiler_HIR_Expr_Kind::Invalid:
      out << "InvalidExpr: " << type_text(types, expr->type_id) << '\n';
      break;

    case Compiler_HIR_Expr_Kind::Constant:
      {
        const auto *node = static_cast<const Compiler_HIR_Constant_Expr *>(expr);
        out << "Constant(" << compiler_hir_constant_kind_name(node->constant_kind);
        if (node->constant_kind == Compiler_HIR_Constant_Kind::Bool)
          out << ", " << (node->bool_value ? "true" : "false");
        else if (node->constant_kind != Compiler_HIR_Constant_Kind::Unit)
          out << ", " << node->text;
        out << "): " << type_text(types, node->type_id) << '\n';
        break;
      }

    case Compiler_HIR_Expr_Kind::Variable:
      {
        const auto *node = static_cast<const Compiler_HIR_Variable_Expr *>(expr);
        out << "Variable(" << node->name << "): " << type_text(types, node->type_id) << '\n';
        break;
      }

    case Compiler_HIR_Expr_Kind::Unary:
      {
        const auto *node = static_cast<const Compiler_HIR_Unary_Expr *>(expr);
        out << "Unary(" << compiler_operator_name(node->op)
            << "): " << type_text(types, node->type_id) << '\n';
        dump_expr(node->operand, out, indent + 2, types);
        break;
      }

    case Compiler_HIR_Expr_Kind::Binary:
      {
        const auto *node = static_cast<const Compiler_HIR_Binary_Expr *>(expr);
        out << "Binary(" << compiler_operator_name(node->op)
            << "): " << type_text(types, node->type_id) << '\n';
        dump_expr(node->left, out, indent + 2, types);
        dump_expr(node->right, out, indent + 2, types);
        break;
      }

    case Compiler_HIR_Expr_Kind::Call:
      {
        const auto *node = static_cast<const Compiler_HIR_Call_Expr *>(expr);
        out << "Call: " << type_text(types, node->type_id) << '\n';
        append_indent(out, indent + 2);
        out << "Callee:\n";
        dump_expr(node->callee, out, indent + 4, types);
        append_indent(out, indent + 2);
        out << "Args:\n";
        for (size_t i = 0; i < node->arguments.size(); ++i)
          dump_expr(node->arguments.access(i), out, indent + 4, types);
        break;
      }
    }
}

inline void dump_stmt(const Compiler_HIR_Stmt *stmt,
                      std::ostream &out,
                      const size_t indent,
                      const Compiler_Type_Context *types)
{
  append_indent(out, indent);
  if (stmt == nullptr)
    {
      out << "<null-hir-stmt>\n";
      return;
    }

  switch (stmt->kind)
    {
    case Compiler_HIR_Stmt_Kind::Invalid:
      out << "InvalidStmt\n";
      break;

    case Compiler_HIR_Stmt_Kind::Eval:
      {
        const auto *node = static_cast<const Compiler_HIR_Eval_Stmt *>(stmt);
        out << "Eval\n";
        dump_expr(node->expr, out, indent + 2, types);
        break;
      }

    case Compiler_HIR_Stmt_Kind::Let:
      {
        const auto *node = static_cast<const Compiler_HIR_Let_Stmt *>(stmt);
        out << "Let(" << node->name << "): " << type_text(types, node->binding_type) << '\n';
        if (node->initializer != nullptr)
          dump_expr(node->initializer, out, indent + 2, types);
        break;
      }

    case Compiler_HIR_Stmt_Kind::Return:
      {
        const auto *node = static_cast<const Compiler_HIR_Return_Stmt *>(stmt);
        out << "Return\n";
        if (node->value != nullptr)
          dump_expr(node->value, out, indent + 2, types);
        break;
      }

    case Compiler_HIR_Stmt_Kind::Block:
      {
        const auto *node = static_cast<const Compiler_HIR_Block_Stmt *>(stmt);
        out << "Block\n";
        for (size_t i = 0; i < node->statements.size(); ++i)
          dump_stmt(node->statements.access(i), out, indent + 2, types);
        break;
      }

    case Compiler_HIR_Stmt_Kind::If:
      {
        const auto *node = static_cast<const Compiler_HIR_If_Stmt *>(stmt);
        out << "If\n";
        append_indent(out, indent + 2);
        out << "Condition:\n";
        dump_expr(node->condition, out, indent + 4, types);
        append_indent(out, indent + 2);
        out << "Then:\n";
        dump_stmt(node->then_branch, out, indent + 4, types);
        if (node->else_branch != nullptr)
          {
            append_indent(out, indent + 2);
            out << "Else:\n";
            dump_stmt(node->else_branch, out, indent + 4, types);
          }
        break;
      }

    case Compiler_HIR_Stmt_Kind::While:
      {
        const auto *node = static_cast<const Compiler_HIR_While_Stmt *>(stmt);
        out << "While\n";
        append_indent(out, indent + 2);
        out << "Condition:\n";
        dump_expr(node->condition, out, indent + 4, types);
        append_indent(out, indent + 2);
        out << "Body:\n";
        dump_stmt(node->body, out, indent + 4, types);
        break;
      }

    case Compiler_HIR_Stmt_Kind::Break:
      out << "Break\n";
      break;

    case Compiler_HIR_Stmt_Kind::Continue:
      out << "Continue\n";
      break;
    }
}
}  // namespace Compiler_HIR_Detail

/** @brief Dumps one HIR expression in a deterministic text format. */
inline std::string compiler_dump_hir_expr(const Compiler_HIR_Expr *expr,
                                          const Compiler_Type_Context &types)
{
  std::ostringstream out;
  Compiler_HIR_Detail::dump_expr(expr, out, 0, &types);
  return out.str();
}

/** @brief Dumps one HIR statement in a deterministic text format. */
inline std::string compiler_dump_hir_stmt(const Compiler_HIR_Stmt *stmt,
                                          const Compiler_Type_Context &types)
{
  std::ostringstream out;
  Compiler_HIR_Detail::dump_stmt(stmt, out, 0, &types);
  return out.str();
}

/** @brief Dumps one HIR function in a deterministic text format. */
inline std::string compiler_dump_hir_function(const Compiler_HIR_Function *function,
                                              const Compiler_Type_Context &types)
{
  std::ostringstream out;
  if (function == nullptr)
    {
      out << "<null-hir-function>\n";
      return out.str();
    }

  out << "Function(" << function->name << "): " << types.to_string(function->type_id) << '\n';
  if (function->parameters.size() > 0)
    {
      out << "  Params:";
      for (size_t i = 0; i < function->parameters.size(); ++i)
        {
          const auto &param = function->parameters.access(i);
          if (i > 0)
            out << ",";
          out << " " << param.name << ": " << types.to_string(param.type_id);
        }
      out << '\n';
    }
  out << "  Body:\n";
  Compiler_HIR_Detail::dump_stmt(function->body, out, 4, &types);
  return out.str();
}

/** @brief Dumps one HIR module in a deterministic text format. */
inline std::string compiler_dump_hir_module(const Compiler_HIR_Module *module,
                                            const Compiler_Type_Context &types)
{
  std::ostringstream out;
  if (module == nullptr)
    {
      out << "<null-hir-module>\n";
      return out.str();
    }

  out << "HIRModule\n";
  for (size_t i = 0; i < module->functions.size(); ++i)
    {
      Compiler_HIR_Detail::append_indent(out, 2);
      const auto *function = module->functions.access(i);
      if (function == nullptr)
        {
          out << "Function: <null>\n";
          continue;
        }
      out << "Function(" << function->name << "): " << types.to_string(function->type_id) << '\n';
      if (function->parameters.size() > 0)
        {
          Compiler_HIR_Detail::append_indent(out, 4);
          out << "Params:";
          for (size_t j = 0; j < function->parameters.size(); ++j)
            {
              const auto &param = function->parameters.access(j);
              if (j > 0)
                out << ",";
              out << " " << param.name << ": " << types.to_string(param.type_id);
            }
          out << '\n';
        }
      Compiler_HIR_Detail::append_indent(out, 4);
      out << "Body:\n";
      Compiler_HIR_Detail::dump_stmt(function->body, out, 6, &types);
    }

  for (size_t i = 0; i < module->statements.size(); ++i)
    Compiler_HIR_Detail::dump_stmt(module->statements.access(i), out, 2, &types);
  return out.str();
}
}  // namespace Aleph

#endif
