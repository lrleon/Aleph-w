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

/** @file Compiler_Sema.H
 *  @brief Name-resolution and basic semantic checks for the compiler-support MVP.
 *
 *  The semantic analyzer performs a deliberately small first pass over the AST:
 *
 *  - lexical scope management
 *  - symbol declaration and duplicate detection
 *  - top-level import/type declaration validation
 *  - identifier resolution
 *  - optional shadowing diagnostics
 *  - `return` outside function checks
 *  - `break` / `continue` outside loop checks
 *
 *  This is not a type checker. The goal is to provide a stable semantic layer
 *  immediately above parsing while future type and constraint systems remain
 *  separate components.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_SEMA_H
#define COMPILER_SEMA_H

#include <string>

#include <Compiler_AST.H>
#include <Compiler_Symbol_Bindings.H>
#include <ah-diagnostics.H>
#include <tpl_dynMapTree.H>

namespace Aleph {
/** @brief Mapping from an identifier expression to its resolved symbol. */
struct Compiler_Name_Resolution
{
  const Compiler_Expr *expr = nullptr;  ///< Identifier expression node.
  Compiler_Symbol_Id symbol_id = 0;     ///< Resolved symbol id.
};

/** @brief Options controlling semantic analyzer behavior. */
struct Compiler_Semantic_Options
{
  bool allow_shadowing = true;  ///< If false, nested redefinitions report `SEM007`.
};

/** @brief Name-resolution and basic semantic checker for the MVP AST. */
class Compiler_Semantic_Analyzer
{
  Diagnostic_Engine *diagnostics = nullptr;
  Compiler_Semantic_Options options;
  DynArray<Compiler_Name_Resolution> resolutions;
  DynMapTree<const Compiler_Expr *, Compiler_Symbol_Id> resolution_index;
  Compiler_Symbol_Bindings value_bindings;
  Compiler_Symbol_Bindings type_bindings;
  size_t function_depth = 0;
  size_t loop_depth = 0;

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

  Compiler_Symbol_Id declare_symbol(const Compiler_Symbol_Kind kind,
                                    const std::string &name,
                                    const Source_Span &span)
  {
    const auto result = value_bindings.declare(kind, name, span);
    if (result.duplicate_local())
      {
        emit_error(span, "duplicate declaration of '" + name + "'", "SEM001");
        return result.symbol_id;
      }

    if (result.shadowing() and not options.allow_shadowing)
      emit_error(span, "shadowing declaration of '" + name + "' is not allowed", "SEM007");

    return result.symbol_id;
  }

  void record_resolution(const Compiler_Expr *expr, const Compiler_Symbol_Id symbol_id)
  {
    if (expr == nullptr)
      return;

    for (size_t i = resolutions.size(); i > 0; --i)
      if (resolutions.access(i - 1).expr == expr)
        {
          resolutions.access(i - 1).symbol_id = symbol_id;
          resolution_index[expr] = symbol_id;
          return;
        }

    resolutions.append({expr, symbol_id});
    resolution_index[expr] = symbol_id;
  }

  void declare_type_name(const std::string &name, const Source_Span &span)
  {
    const auto result = type_bindings.declare(Compiler_Symbol_Kind::Type, name, span);
    if (result.duplicate_local())
      {
        emit_error(span, "duplicate type declaration of '" + name + "'", "SEM008");
        return;
      }
  }

  void analyze_imports(const Compiler_Module *module) const
  {
    if (module == nullptr)
      return;

    DynArray<std::string> imported_modules;
    for (size_t i = 0; i < module->imports.size(); ++i)
      {
        const auto *decl = module->imports.access(i);
        if (decl == nullptr or decl->kind != Compiler_Import_Decl_Kind::Import)
          continue;

        if (not module->source_name.empty() and decl->module_name == module->source_name)
          emit_error(decl->module_name_span,
                     "module '" + module->source_name + "' cannot import itself",
                     "SEM010");

        bool duplicate = false;
        for (size_t j = 0; j < imported_modules.size(); ++j)
          if (imported_modules.access(j) == decl->module_name)
            {
              duplicate = true;
              break;
            }

        if (duplicate)
          emit_error(decl->module_name_span,
                     "duplicate import of '" + decl->module_name + "'",
                     "SEM009");
        else
          imported_modules.append(decl->module_name);
      }
  }

  void analyze_expr(const Compiler_Expr *expr)
  {
    if (expr == nullptr)
      return;

    switch (expr->kind)
      {
      case Compiler_Expr_Kind::Invalid:
      case Compiler_Expr_Kind::Integer_Literal:
      case Compiler_Expr_Kind::String_Literal:
      case Compiler_Expr_Kind::Char_Literal:
      case Compiler_Expr_Kind::Bool_Literal:
        return;

      case Compiler_Expr_Kind::Identifier:
        {
          const auto *node = static_cast<const Compiler_Identifier_Expr *>(expr);
          const auto *symbol_id = value_bindings.lookup(node->name);
          if (symbol_id == nullptr)
            {
              emit_error(node->span, "use of undeclared identifier '" + node->name + "'", "SEM002");
              return;
            }

          record_resolution(expr, *symbol_id);
          return;
        }

      case Compiler_Expr_Kind::Grouping:
        analyze_expr(static_cast<const Compiler_Grouping_Expr *>(expr)->inner);
        return;

      case Compiler_Expr_Kind::Unary:
        analyze_expr(static_cast<const Compiler_Unary_Expr *>(expr)->operand);
        return;

      case Compiler_Expr_Kind::Binary:
        {
          const auto *node = static_cast<const Compiler_Binary_Expr *>(expr);
          analyze_expr(node->left);
          analyze_expr(node->right);
          return;
        }

      case Compiler_Expr_Kind::Call:
        {
          const auto *node = static_cast<const Compiler_Call_Expr *>(expr);
          analyze_expr(node->callee);
          for (size_t i = 0; i < node->arguments.size(); ++i)
            analyze_expr(node->arguments.access(i));
          return;
        }
      }
  }

  void analyze_block(const Compiler_Block_Stmt *block)
  {
    if (block == nullptr)
      return;

    value_bindings.enter_scope();
    for (size_t i = 0; i < block->statements.size(); ++i)
      analyze_stmt(block->statements.access(i));
    value_bindings.leave_scope();
  }

  void analyze_stmt(const Compiler_Stmt *stmt)
  {
    if (stmt == nullptr)
      return;

    switch (stmt->kind)
      {
      case Compiler_Stmt_Kind::Invalid:
        return;

      case Compiler_Stmt_Kind::Expr:
        analyze_expr(static_cast<const Compiler_Expr_Stmt *>(stmt)->expr);
        return;

      case Compiler_Stmt_Kind::Let:
        {
          const auto *node = static_cast<const Compiler_Let_Stmt *>(stmt);
          analyze_expr(node->initializer);
          const auto kind
            = function_depth > 0 ? Compiler_Symbol_Kind::Local : Compiler_Symbol_Kind::Global;
          (void) declare_symbol(kind, node->name, node->name_span);
          return;
        }

      case Compiler_Stmt_Kind::Return:
        {
          const auto *node = static_cast<const Compiler_Return_Stmt *>(stmt);
          if (function_depth == 0)
            emit_error(node->keyword_span, "'return' is only valid inside a function", "SEM006");
          analyze_expr(node->value);
          return;
        }

      case Compiler_Stmt_Kind::Block:
        analyze_block(static_cast<const Compiler_Block_Stmt *>(stmt));
        return;

      case Compiler_Stmt_Kind::If:
        {
          const auto *node = static_cast<const Compiler_If_Stmt *>(stmt);
          analyze_expr(node->condition);
          analyze_stmt(node->then_branch);
          analyze_stmt(node->else_branch);
          return;
        }

      case Compiler_Stmt_Kind::While:
        {
          const auto *node = static_cast<const Compiler_While_Stmt *>(stmt);
          analyze_expr(node->condition);
          ++loop_depth;
          analyze_stmt(node->body);
          --loop_depth;
          return;
        }

      case Compiler_Stmt_Kind::Break:
        {
          const auto *node = static_cast<const Compiler_Break_Stmt *>(stmt);
          if (loop_depth == 0)
            emit_error(node->keyword_span, "'break' is only valid inside a loop", "SEM003");
          return;
        }

      case Compiler_Stmt_Kind::Continue:
        {
          const auto *node = static_cast<const Compiler_Continue_Stmt *>(stmt);
          if (loop_depth == 0)
            emit_error(node->keyword_span, "'continue' is only valid inside a loop", "SEM004");
          return;
        }
      }
  }

  void analyze_function(const Compiler_Function_Decl *fn)
  {
    if (fn == nullptr)
      return;

    ++function_depth;
    value_bindings.enter_scope();
    for (size_t i = 0; i < fn->parameters.size(); ++i)
      {
        const auto &param = fn->parameters.access(i);
        (void) declare_symbol(Compiler_Symbol_Kind::Parameter, param.name, param.span);
      }
    analyze_block(fn->body);
    value_bindings.leave_scope();
    --function_depth;
  }

public:
  /** @brief Constructs a semantic analyzer.
   *
   *  @param dx Optional diagnostic sink to receive semantic errors.
   *  @param opts Analyzer behavior options.
   */
  explicit Compiler_Semantic_Analyzer(Diagnostic_Engine *dx = nullptr,
                                      const Compiler_Semantic_Options &opts = {})
    : diagnostics(dx), options(opts)
  {}

  /** @brief Clears previously collected semantic state. */
  void clear() noexcept
  {
    resolutions.clear();
    resolution_index.clear();
    value_bindings.clear();
    type_bindings.clear();
    function_depth = 0;
    loop_depth = 0;
  }

  /** @brief Analyzes a parsed module.
   *
   *  Existing semantic state is discarded before the new analysis.
   *
   *  @param module Parsed module to analyze.
   */
  void analyze_module(const Compiler_Module *module)
  {
    clear();
    if (module == nullptr)
      return;

    value_bindings.enter_scope();
    type_bindings.enter_scope();

    analyze_imports(module);

    for (size_t i = 0; i < module->type_declarations.size(); ++i)
      {
        const auto *decl = module->type_declarations.access(i);
        if (decl != nullptr)
          declare_type_name(decl->name, decl->name_span);
      }

    for (size_t i = 0; i < module->functions.size(); ++i)
      {
        const auto *fn = module->functions.access(i);
        if (fn != nullptr)
          (void) declare_symbol(Compiler_Symbol_Kind::Function, fn->name, fn->name_span);
      }

    for (size_t i = 0; i < module->statements.size(); ++i)
      analyze_stmt(module->statements.access(i));

    for (size_t i = 0; i < module->functions.size(); ++i)
      analyze_function(module->functions.access(i));

    value_bindings.leave_scope();
    type_bindings.leave_scope();
  }

  /** @brief Returns the number of collected symbols. */
  size_t symbol_count() const noexcept
  {
    return value_bindings.symbol_count();
  }

  /** @brief Returns the number of identifier resolutions. */
  size_t resolution_count() const noexcept
  {
    return resolutions.size();
  }

  /** @brief Returns symbol `id`.
   *
   *  @param id 1-based symbol id.
   *  @return Symbol record.
   *  @throws std::out_of_range If `id` is invalid.
   */
  const Compiler_Symbol &symbol(const Compiler_Symbol_Id id) const
  {
    return value_bindings.symbol(id);
  }

  /** @brief Returns resolution record `i`.
   *
   *  @param i Resolution index.
   *  @return Stored resolution.
   *  @throws std::out_of_range If `i` is invalid.
   */
  const Compiler_Name_Resolution &resolution(const size_t i) const
  {
    ah_out_of_range_error_unless(i < resolutions.size())
      << "Compiler_Semantic_Analyzer::resolution(): invalid index " << i;
    return resolutions.access(i);
  }

  /** @brief Returns the resolved symbol for an identifier expression.
   *
   *  @param expr Identifier expression node.
   *  @return Resolved symbol, or `nullptr` if unresolved/not recorded.
   */
  const Compiler_Symbol *resolved_symbol(const Compiler_Expr *expr) const noexcept
  {
    if (expr == nullptr)
      return nullptr;

    const auto *entry = resolution_index.search(expr);
    if (entry == nullptr)
      return nullptr;

    const auto id = entry->second;
    if (id == 0 or id > value_bindings.symbol_count())
      return nullptr;

    return &value_bindings.symbol(id);
  }

  /** @brief Dumps the current symbol table in a deterministic text format. */
  std::string dump_symbols() const
  {
    return value_bindings.dump_symbols();
  }
};
}  // namespace Aleph

#endif  // COMPILER_SEMA_H
