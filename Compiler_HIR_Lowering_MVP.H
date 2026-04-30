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

/** @file Compiler_HIR_Lowering_MVP.H
 *  @brief Lowering from the current MVP typed AST into the reusable HIR model.
 *
 *  This header keeps the existing lowering strategy for the reference frontend,
 *  but isolates it from the reusable HIR node definitions so other languages
 *  can target HIR without depending on the MVP parser and typed semantics.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_HIR_LOWERING_MVP_H
#define COMPILER_HIR_LOWERING_MVP_H

#include <Compiler_HIR_Model.H>
#include <Compiler_Typed_Sema.H>

namespace Aleph {
/** @brief Lowers the MVP typed AST into HIR. */
class Compiler_HIR_Lowering
{
  Compiler_HIR_Context *hir = nullptr;
  const Compiler_Typed_Semantic_Analyzer *typed = nullptr;

  Compiler_Type_Id invalid_type() const noexcept
  {
    return typed != nullptr ? typed->type_context().invalid_type() : 0;
  }

  Compiler_Type_Id expr_type(const Compiler_Expr *expr) const noexcept
  {
    if (typed == nullptr or expr == nullptr)
      return invalid_type();

    const auto ty = typed->inferred_type(expr);
    return ty != 0 ? ty : invalid_type();
  }

  Compiler_Type_Id function_type(const Compiler_Function_Decl *function) const noexcept
  {
    if (typed == nullptr or function == nullptr)
      return invalid_type();

    const auto ty = typed->function_type(function);
    return ty != 0 ? ty : invalid_type();
  }

  Compiler_HIR_Expr *lower_expr(const Compiler_Expr *expr)
  {
    if (expr == nullptr)
      return hir->make<Compiler_HIR_Invalid_Expr>(invalid_type());

    switch (expr->kind)
      {
      case Compiler_Expr_Kind::Invalid:
        return hir->make<Compiler_HIR_Invalid_Expr>(expr_type(expr), expr->span);

      case Compiler_Expr_Kind::Grouping:
        return lower_expr(static_cast<const Compiler_Grouping_Expr *>(expr)->inner);

      case Compiler_Expr_Kind::Identifier:
        {
          const auto *node = static_cast<const Compiler_Identifier_Expr *>(expr);
          return hir->make<Compiler_HIR_Variable_Expr>(node->name, expr_type(expr), node->name_span);
        }

      case Compiler_Expr_Kind::Integer_Literal:
        {
          const auto *node = static_cast<const Compiler_Literal_Expr *>(expr);
          return hir->make<Compiler_HIR_Constant_Expr>(Compiler_HIR_Constant_Kind::Integer,
                                                       expr_type(expr),
                                                       node->lexeme,
                                                       node->span);
        }

      case Compiler_Expr_Kind::String_Literal:
        {
          const auto *node = static_cast<const Compiler_Literal_Expr *>(expr);
          return hir->make<Compiler_HIR_Constant_Expr>(Compiler_HIR_Constant_Kind::String,
                                                       expr_type(expr),
                                                       node->lexeme,
                                                       node->span);
        }

      case Compiler_Expr_Kind::Char_Literal:
        {
          const auto *node = static_cast<const Compiler_Literal_Expr *>(expr);
          return hir->make<Compiler_HIR_Constant_Expr>(Compiler_HIR_Constant_Kind::Character,
                                                       expr_type(expr),
                                                       node->lexeme,
                                                       node->span);
        }

      case Compiler_Expr_Kind::Bool_Literal:
        {
          const auto *node = static_cast<const Compiler_Literal_Expr *>(expr);
          return hir->make<Compiler_HIR_Constant_Expr>(Compiler_HIR_Constant_Kind::Bool,
                                                       expr_type(expr),
                                                       node->bool_value ? "true" : "false",
                                                       node->span,
                                                       node->bool_value);
        }

      case Compiler_Expr_Kind::Unary:
        {
          const auto *node = static_cast<const Compiler_Unary_Expr *>(expr);
          return hir->make<Compiler_HIR_Unary_Expr>(compiler_operator_from_token_kind(node->op),
                                                    expr_type(expr),
                                                    node->operator_span,
                                                    lower_expr(node->operand));
        }

      case Compiler_Expr_Kind::Binary:
        {
          const auto *node = static_cast<const Compiler_Binary_Expr *>(expr);
          return hir->make<Compiler_HIR_Binary_Expr>(lower_expr(node->left),
                                                     compiler_operator_from_token_kind(node->op),
                                                     expr_type(expr),
                                                     node->operator_span,
                                                     lower_expr(node->right));
        }

      case Compiler_Expr_Kind::Call:
        {
          const auto *node = static_cast<const Compiler_Call_Expr *>(expr);
          auto *call = hir->make<Compiler_HIR_Call_Expr>(lower_expr(node->callee),
                                                         expr_type(expr),
                                                         node->lparen_span,
                                                         node->rparen_span);
          for (size_t i = 0; i < node->arguments.size(); ++i)
            call->arguments.append(lower_expr(node->arguments.access(i)));
          return call;
        }
      }

    return hir->make<Compiler_HIR_Invalid_Expr>(invalid_type(), expr->span);
  }

  Compiler_HIR_Stmt *lower_stmt(const Compiler_Stmt *stmt)
  {
    if (stmt == nullptr)
      return hir->make<Compiler_HIR_Invalid_Stmt>();

    switch (stmt->kind)
      {
      case Compiler_Stmt_Kind::Invalid:
        return hir->make<Compiler_HIR_Invalid_Stmt>(stmt->span);

      case Compiler_Stmt_Kind::Expr:
        return hir->make<Compiler_HIR_Eval_Stmt>(
          lower_expr(static_cast<const Compiler_Expr_Stmt *>(stmt)->expr), stmt->span);

      case Compiler_Stmt_Kind::Let:
        {
          const auto *node = static_cast<const Compiler_Let_Stmt *>(stmt);
          Compiler_Type_Id binding_type = invalid_type();
          if (typed != nullptr)
            {
              const auto ty = typed->let_type(node);
              binding_type = ty != 0 ? ty : invalid_type();
            }
          return hir->make<Compiler_HIR_Let_Stmt>(node->name,
                                                  binding_type,
                                                  node->name_span,
                                                  node->initializer != nullptr
                                                    ? lower_expr(node->initializer)
                                                    : nullptr,
                                                  node->span);
        }

      case Compiler_Stmt_Kind::Return:
        {
          const auto *node = static_cast<const Compiler_Return_Stmt *>(stmt);
          return hir->make<Compiler_HIR_Return_Stmt>(node->value != nullptr ? lower_expr(node->value)
                                                                            : nullptr,
                                                     node->span);
        }

      case Compiler_Stmt_Kind::Block:
        {
          const auto *node = static_cast<const Compiler_Block_Stmt *>(stmt);
          auto *block = hir->make<Compiler_HIR_Block_Stmt>(node->lbrace_span, node->rbrace_span);
          for (size_t i = 0; i < node->statements.size(); ++i)
            block->statements.append(lower_stmt(node->statements.access(i)));
          block->span = node->span;
          return block;
        }

      case Compiler_Stmt_Kind::If:
        {
          const auto *node = static_cast<const Compiler_If_Stmt *>(stmt);
          return hir->make<Compiler_HIR_If_Stmt>(lower_expr(node->condition),
                                                 lower_stmt(node->then_branch),
                                                 lower_stmt(node->else_branch),
                                                 node->if_span,
                                                 node->else_span);
        }

      case Compiler_Stmt_Kind::While:
        {
          const auto *node = static_cast<const Compiler_While_Stmt *>(stmt);
          return hir->make<Compiler_HIR_While_Stmt>(lower_expr(node->condition),
                                                    lower_stmt(node->body),
                                                    node->keyword_span);
        }

      case Compiler_Stmt_Kind::Break:
        return hir->make<Compiler_HIR_Break_Stmt>(stmt->span);

      case Compiler_Stmt_Kind::Continue:
        return hir->make<Compiler_HIR_Continue_Stmt>(stmt->span);
      }

    return hir->make<Compiler_HIR_Invalid_Stmt>(stmt->span);
  }

public:
  /** @brief Constructs a lowering helper.
   *
   *  @param context HIR allocation context.
   *  @param analysis Typed semantic analysis results.
   */
  Compiler_HIR_Lowering(Compiler_HIR_Context &context,
                        const Compiler_Typed_Semantic_Analyzer &analysis) noexcept
    : hir(&context), typed(&analysis)
  {}

  /** @brief Lowers one function declaration into HIR. */
  Compiler_HIR_Function *lower_function(const Compiler_Function_Decl *function)
  {
    if (function == nullptr)
      return nullptr;

    auto *hir_function = hir->make<Compiler_HIR_Function>(
      function->name, function_type(function), function->name_span, nullptr, function->span);
    for (size_t i = 0; i < function->parameters.size(); ++i)
      {
        const auto &param = function->parameters.access(i);
        Compiler_Type_Id type_id = invalid_type();
        if (typed != nullptr)
          {
            const auto ty = typed->parameter_type(function, i);
            type_id = ty != 0 ? ty : invalid_type();
          }
        hir_function->parameters.append({param.name, param.span, type_id});
      }

    auto *lowered_body = lower_stmt(function->body);
    if (lowered_body != nullptr and lowered_body->kind == Compiler_HIR_Stmt_Kind::Block)
      hir_function->body = static_cast<Compiler_HIR_Block_Stmt *>(lowered_body);
    else
      hir_function->body = nullptr;
    hir_function->span = function->span;
    return hir_function;
  }

  /** @brief Lowers one module into HIR. */
  Compiler_HIR_Module *lower_module(const Compiler_Module *module)
  {
    if (module == nullptr)
      return nullptr;

    auto *hir_module = hir->make<Compiler_HIR_Module>(module->span);
    for (size_t i = 0; i < module->functions.size(); ++i)
      hir_module->functions.append(lower_function(module->functions.access(i)));
    for (size_t i = 0; i < module->statements.size(); ++i)
      hir_module->statements.append(lower_stmt(module->statements.access(i)));
    return hir_module;
  }
};
}  // namespace Aleph

#endif
