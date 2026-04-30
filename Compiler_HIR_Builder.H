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

/** @file Compiler_HIR_Builder.H
 *  @brief Reusable builder helpers for constructing HIR directly.
 *
 *  Frontends that do not want to reuse the MVP parser or typed-AST lowering can
 *  use this builder to allocate HIR nodes safely inside a `Compiler_HIR_Context`
 *  while keeping container membership and aggregate spans consistent.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_HIR_BUILDER_H
#define COMPILER_HIR_BUILDER_H

#include <Compiler_HIR_Model.H>

namespace Aleph {
/** @brief Small reusable helper that allocates and connects HIR nodes.
 *
 *  The builder is intentionally lightweight: it owns no memory itself and
 *  only wraps a `Compiler_HIR_Context`. All nodes remain owned by the
 *  associated context.
 */
class Compiler_HIR_Builder
{
  Compiler_HIR_Context *hir = nullptr;

  static void merge_span(Source_Span &dst, const Source_Span &src) noexcept
  {
    if (not src.is_valid())
      return;
    if (not dst.is_valid())
      {
        dst = src;
        return;
      }
    dst = compiler_hir_merge_spans(dst, src);
  }

public:
  /** @brief Builds HIR nodes inside `context`. */
  explicit Compiler_HIR_Builder(Compiler_HIR_Context &context) noexcept : hir(&context) {}

  /** @brief Returns the underlying ownership context. */
  [[nodiscard]] Compiler_HIR_Context &context() const noexcept
  {
    return *hir;
  }

  /** @brief Allocates one empty module. */
  Compiler_HIR_Module *make_module(const Source_Span &span = {}) const
  {
    return hir->make<Compiler_HIR_Module>(span);
  }

  /** @brief Appends one function to `module` and updates the module span. */
  static void append_function(Compiler_HIR_Module &module, Compiler_HIR_Function *function)
  {
    ah_runtime_error_unless(function != nullptr)
      << "Compiler_HIR_Builder::append_function(): null function";
    module.functions.append(function);
    merge_span(module.span, function->span);
  }

  /** @brief Appends one top-level statement to `module` and updates the span. */
  static void append_top_level_statement(Compiler_HIR_Module &module, Compiler_HIR_Stmt *stmt)
  {
    ah_runtime_error_unless(stmt != nullptr)
      << "Compiler_HIR_Builder::append_top_level_statement(): null statement";
    module.statements.append(stmt);
    merge_span(module.span, stmt->span);
  }

  /** @brief Allocates one structured block statement. */
  Compiler_HIR_Block_Stmt *make_block(const Source_Span &lbrace = {},
                                      const Source_Span &rbrace = {}) const
  {
    return hir->make<Compiler_HIR_Block_Stmt>(lbrace, rbrace);
  }

  /** @brief Appends one nested statement to `block` and updates the span. */
  static void append_statement(Compiler_HIR_Block_Stmt &block, Compiler_HIR_Stmt *stmt)
  {
    ah_runtime_error_unless(stmt != nullptr)
      << "Compiler_HIR_Builder::append_statement(): null statement";
    block.statements.append(stmt);
    merge_span(block.span, stmt->span);
  }

  /** @brief Allocates one function node without inserting it into any module. */
  Compiler_HIR_Function *make_function(std::string name,
                                       const Compiler_Type_Id type_id = 0,
                                       const Source_Span &name_span = {},
                                       Compiler_HIR_Block_Stmt *body = nullptr,
                                       const Source_Span &span = {}) const
  {
    return hir->make<Compiler_HIR_Function>(std::move(name), type_id, name_span, body, span);
  }

  /** @brief Attaches `body` to `function` and refreshes the aggregate span. */
  static void set_body(Compiler_HIR_Function &function, Compiler_HIR_Block_Stmt *body) noexcept
  {
    function.body = body;
    function.span = function.name_span;
    if (body != nullptr)
      merge_span(function.span, body->span);
  }

  /** @brief Adds one parameter metadata entry to `function`. */
  static Compiler_HIR_Param &add_parameter(Compiler_HIR_Function &function,
                                           std::string name,
                                           const Compiler_Type_Id type_id,
                                           const Source_Span &span = {})
  {
    function.parameters.append({std::move(name), span, type_id});
    merge_span(function.span, span);
    return function.parameters.access(function.parameters.size() - 1);
  }

  /** @brief Allocates one invalid expression placeholder. */
  Compiler_HIR_Invalid_Expr *make_invalid_expr(const Compiler_Type_Id type_id = 0,
                                               const Source_Span &span = {}) const
  {
    return hir->make<Compiler_HIR_Invalid_Expr>(type_id, span);
  }

  /** @brief Allocates one generic constant expression. */
  Compiler_HIR_Constant_Expr *make_constant(const Compiler_HIR_Constant_Kind kind,
                                            const Compiler_Type_Id type_id,
                                            std::string text,
                                            const Source_Span &span = {},
                                            const bool bool_value = false) const
  {
    return hir->make<Compiler_HIR_Constant_Expr>(kind, type_id, std::move(text), span, bool_value);
  }

  /** @brief Allocates one `unit` constant expression. */
  Compiler_HIR_Constant_Expr *make_unit_constant(const Compiler_Type_Id type_id,
                                                 const Source_Span &span = {}) const
  {
    return make_constant(Compiler_HIR_Constant_Kind::Unit, type_id, "unit", span, false);
  }

  /** @brief Allocates one boolean constant expression. */
  Compiler_HIR_Constant_Expr *make_bool_constant(const bool value,
                                                 const Compiler_Type_Id type_id,
                                                 const Source_Span &span = {}) const
  {
    return make_constant(
      Compiler_HIR_Constant_Kind::Bool, type_id, value ? "true" : "false", span, value);
  }

  /** @brief Allocates one integer constant expression. */
  Compiler_HIR_Constant_Expr *make_integer_constant(const long long value,
                                                    const Compiler_Type_Id type_id,
                                                    const Source_Span &span = {}) const
  {
    return make_constant(
      Compiler_HIR_Constant_Kind::Integer, type_id, std::to_string(value), span, false);
  }

  /** @brief Allocates one string constant expression. */
  Compiler_HIR_Constant_Expr *make_string_constant(std::string text,
                                                   const Compiler_Type_Id type_id,
                                                   const Source_Span &span = {}) const
  {
    return make_constant(Compiler_HIR_Constant_Kind::String, type_id, std::move(text), span, false);
  }

  /** @brief Allocates one character constant expression. */
  Compiler_HIR_Constant_Expr *make_character_constant(std::string text,
                                                      const Compiler_Type_Id type_id,
                                                      const Source_Span &span = {}) const
  {
    return make_constant(Compiler_HIR_Constant_Kind::Character, type_id, std::move(text), span, false);
  }

  /** @brief Allocates one variable-reference expression. */
  Compiler_HIR_Variable_Expr *make_variable(std::string name,
                                            const Compiler_Type_Id type_id = 0,
                                            const Source_Span &span = {}) const
  {
    return hir->make<Compiler_HIR_Variable_Expr>(std::move(name), type_id, span);
  }

  /** @brief Allocates one unary expression. */
  Compiler_HIR_Unary_Expr *make_unary(const Compiler_Operator_Kind op,
                                      Compiler_HIR_Expr *operand,
                                      const Compiler_Type_Id type_id = 0,
                                      const Source_Span &operator_span = {}) const
  {
    return hir->make<Compiler_HIR_Unary_Expr>(op, type_id, operator_span, operand);
  }

  /** @brief Allocates one binary expression. */
  Compiler_HIR_Binary_Expr *make_binary(Compiler_HIR_Expr *lhs,
                                        const Compiler_Operator_Kind op,
                                        Compiler_HIR_Expr *rhs,
                                        const Compiler_Type_Id type_id = 0,
                                        const Source_Span &operator_span = {}) const
  {
    return hir->make<Compiler_HIR_Binary_Expr>(lhs, op, type_id, operator_span, rhs);
  }

  /** @brief Allocates one call expression without arguments. */
  Compiler_HIR_Call_Expr *make_call(Compiler_HIR_Expr *callee,
                                    const Compiler_Type_Id type_id = 0,
                                    const Source_Span &lparen_span = {},
                                    const Source_Span &rparen_span = {}) const
  {
    return hir->make<Compiler_HIR_Call_Expr>(callee, type_id, lparen_span, rparen_span);
  }

  /** @brief Appends one argument to `call` and updates the aggregate span. */
  static void append_argument(Compiler_HIR_Call_Expr &call, Compiler_HIR_Expr *argument)
  {
    ah_runtime_error_unless(argument != nullptr)
      << "Compiler_HIR_Builder::append_argument(): null argument";
    call.arguments.append(argument);
    merge_span(call.span, argument->span);
  }

  /** @brief Allocates one invalid statement placeholder. */
  Compiler_HIR_Invalid_Stmt *make_invalid_stmt(const Source_Span &span = {}) const
  {
    return hir->make<Compiler_HIR_Invalid_Stmt>(span);
  }

  /** @brief Allocates one expression statement. */
  Compiler_HIR_Eval_Stmt *make_eval_stmt(Compiler_HIR_Expr *expr, const Source_Span &span = {}) const
  {
    return hir->make<Compiler_HIR_Eval_Stmt>(expr, span);
  }

  /** @brief Allocates one lexical binding statement. */
  Compiler_HIR_Let_Stmt *make_let_stmt(std::string name,
                                       const Compiler_Type_Id type_id,
                                       const Source_Span &name_span = {},
                                       Compiler_HIR_Expr *initializer = nullptr,
                                       const Source_Span &span = {}) const
  {
    return hir->make<Compiler_HIR_Let_Stmt>(std::move(name), type_id, name_span, initializer, span);
  }

  /** @brief Allocates one return statement. */
  Compiler_HIR_Return_Stmt *make_return_stmt(Compiler_HIR_Expr *value = nullptr,
                                             const Source_Span &span = {}) const
  {
    return hir->make<Compiler_HIR_Return_Stmt>(value, span);
  }

  /** @brief Allocates one structured conditional statement. */
  Compiler_HIR_If_Stmt *make_if_stmt(Compiler_HIR_Expr *condition,
                                     Compiler_HIR_Stmt *then_branch,
                                     Compiler_HIR_Stmt *else_branch = nullptr,
                                     const Source_Span &if_span = {},
                                     const Source_Span &else_span = {}) const
  {
    return hir->make<Compiler_HIR_If_Stmt>(condition, then_branch, else_branch, if_span, else_span);
  }

  /** @brief Allocates one structured while-loop statement. */
  Compiler_HIR_While_Stmt *make_while_stmt(Compiler_HIR_Expr *condition,
                                           Compiler_HIR_Stmt *body,
                                           const Source_Span &keyword_span = {}) const
  {
    return hir->make<Compiler_HIR_While_Stmt>(condition, body, keyword_span);
  }

  /** @brief Allocates one `break` statement. */
  Compiler_HIR_Break_Stmt *make_break_stmt(const Source_Span &span = {}) const
  {
    return hir->make<Compiler_HIR_Break_Stmt>(span);
  }

  /** @brief Allocates one `continue` statement. */
  Compiler_HIR_Continue_Stmt *make_continue_stmt(const Source_Span &span = {}) const
  {
    return hir->make<Compiler_HIR_Continue_Stmt>(span);
  }
};
}  // namespace Aleph

#endif
