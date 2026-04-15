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

/** @file Compiler_Typed_Sema.H
 *  @brief Typed semantic pass connecting the AST, the name pass, and type constraints.
 *
 *  `Compiler_Typed_Semantic_Analyzer` performs a small inference-oriented pass
 *  over the MVP AST. It uses `Compiler_Sema.H` as an optional precheck and
 *  generates constraints over `Compiler_Types.H` with the unifier from
 *  `tpl_constraints.H`.
 *
 *  The current typed pass covers:
 *
 *  - literal typing
 *  - lexical binding of inferred local and global types
 *  - fresh parameter and result types for functions
 *  - operator typing for arithmetic, comparison, logical, and assignment-like operators
 *  - boolean conditions for `if` and `while`
 *  - call constraints of the form `callee ~ fn(args...) -> result`
 *  - return-value constraints against the inferred function result type
 *
 *  This is still intentionally modest: there are no explicit type annotations,
 *  no polymorphic generalization, and no trait or overload solving yet.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_TYPED_SEMA_H
#define COMPILER_TYPED_SEMA_H

#include <sstream>
#include <string>

#include <Compiler_Sema.H>
#include <Compiler_Types.H>
#include <tpl_constraints.H>
#include <tpl_scope.H>

namespace Aleph {
/** @brief Recorded inferred type for one expression node. */
struct Compiler_Expr_Type_Assignment
{
  const Compiler_Expr *expr = nullptr;  ///< Expression node.
  Compiler_Type_Id type_id = 0;         ///< Inferred or constrained type.
};

/** @brief Recorded inferred type for one function declaration. */
struct Compiler_Function_Type_Assignment
{
  const Compiler_Function_Decl *function = nullptr;  ///< Function declaration.
  Compiler_Type_Id type_id = 0;                      ///< Inferred function type.
};

/** @brief Recorded inferred type for one `let` binding. */
struct Compiler_Let_Type_Assignment
{
  const Compiler_Let_Stmt *stmt = nullptr;  ///< Let statement node.
  Compiler_Type_Id type_id = 0;             ///< Binding type.
};

/** @brief Recorded inferred type for one function parameter. */
struct Compiler_Param_Type_Assignment
{
  const Compiler_Function_Decl *function = nullptr;  ///< Owner function.
  size_t index = 0;                                  ///< Parameter index inside the function.
  Compiler_Type_Id type_id = 0;                      ///< Parameter type.
};

/** @brief Options controlling the typed semantic pass. */
struct Compiler_Typed_Semantic_Options
{
  bool run_base_semantic_checks = true;  ///< Run `Compiler_Sema.H` before typing.
  bool infer_unit_from_missing_return
    = true;  ///< Constrain functions with no explicit `return` to `Unit`.
  Compiler_Semantic_Options semantic_options;  ///< Options forwarded to the base semantic pass.
};

/** @brief Inference-oriented semantic pass for the MVP compiler front-end. */
class Compiler_Typed_Semantic_Analyzer
{
  Diagnostic_Engine *diagnostics = nullptr;
  Compiler_Typed_Semantic_Options options;
  Compiler_Semantic_Analyzer base_semantic;
  Compiler_Type_Context types;
  Compiler_Type_Unifier unifier;
  Scope<std::string, Compiler_Type_Id> scopes;
  DynArray<Compiler_Expr_Type_Assignment> expr_types;
  DynArray<Compiler_Function_Type_Assignment> function_types;
  DynArray<Compiler_Let_Type_Assignment> let_types;
  DynArray<Compiler_Param_Type_Assignment> param_types;

  void emit_error(const Source_Span &span,
                  const std::string &code,
                  const std::string &message,
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

  bool is_invalid_type(const Compiler_Type_Id id) const noexcept
  {
    return id == 0 or id == types.invalid_type();
  }

  Compiler_Type_Id fresh_type()
  {
    return types.make_type_variable();
  }

  void record_expr_type(const Compiler_Expr *expr, const Compiler_Type_Id type_id)
  {
    if (expr == nullptr)
      return;

    for (size_t i = expr_types.size(); i > 0; --i)
      if (expr_types.access(i - 1).expr == expr)
        {
          expr_types.access(i - 1).type_id = type_id;
          return;
        }

    expr_types.append({expr, type_id});
  }

  void record_function_type(const Compiler_Function_Decl *function, const Compiler_Type_Id type_id)
  {
    if (function == nullptr)
      return;

    for (size_t i = function_types.size(); i > 0; --i)
      if (function_types.access(i - 1).function == function)
        {
          function_types.access(i - 1).type_id = type_id;
          return;
        }

    function_types.append({function, type_id});
  }

  void record_let_type(const Compiler_Let_Stmt *stmt, const Compiler_Type_Id type_id)
  {
    if (stmt == nullptr)
      return;

    for (size_t i = let_types.size(); i > 0; --i)
      if (let_types.access(i - 1).stmt == stmt)
        {
          let_types.access(i - 1).type_id = type_id;
          return;
        }

    let_types.append({stmt, type_id});
  }

  void record_param_type(const Compiler_Function_Decl *function,
                         const size_t index,
                         const Compiler_Type_Id type_id)
  {
    for (size_t i = param_types.size(); i > 0; --i)
      if (param_types.access(i - 1).function == function and param_types.access(i - 1).index == index)
        {
          param_types.access(i - 1).type_id = type_id;
          return;
        }

    param_types.append({function, index, type_id});
  }

  Compiler_Type_Id lookup_name_type(const std::string &name, const Source_Span &span)
  {
    if (const auto *type_id = scopes.lookup(name); type_id != nullptr)
      return *type_id;

    if (not options.run_base_semantic_checks)
      emit_error(span, "TYP001", "use of undeclared identifier '" + name + "'");
    return types.invalid_type();
  }

  bool require_unify(const Compiler_Type_Id lhs,
                     const Compiler_Type_Id rhs,
                     const Source_Span &span,
                     const std::string &code,
                     const std::string &prefix)
  {
    if (is_invalid_type(lhs) or is_invalid_type(rhs))
      return false;

    const auto result = unifier.unify(lhs, rhs);
    if (result.ok())
      return true;

    emit_error(span, code, prefix + ": " + result.message);
    return false;
  }

  Compiler_Type_Id predeclare_function(const Compiler_Function_Decl *function)
  {
    DynArray<Compiler_Type_Id> params;
    for (size_t i = 0; i < function->parameters.size(); ++i)
      {
        const auto param_type = fresh_type();
        params.append(param_type);
        record_param_type(function, i, param_type);
      }

    const auto result_type = fresh_type();
    const auto function_type = types.make_function_type(params, result_type);
    record_function_type(function, function_type);
    (void) scopes.insert(function->name, function_type);
    return function_type;
  }

  Compiler_Type_Id find_function_type(const Compiler_Function_Decl *function) const
  {
    for (size_t i = function_types.size(); i > 0; --i)
      if (function_types.access(i - 1).function == function)
        return function_types.access(i - 1).type_id;
    return 0;
  }

  Compiler_Type_Id analyze_expr(const Compiler_Expr *expr)
  {
    if (expr == nullptr)
      return types.invalid_type();

    Compiler_Type_Id result = types.invalid_type();
    switch (expr->kind)
      {
      case Compiler_Expr_Kind::Invalid:
        result = types.invalid_type();
        break;

      case Compiler_Expr_Kind::Integer_Literal:
        result = types.integer_type();
        break;

      case Compiler_Expr_Kind::String_Literal:
        result = types.string_type();
        break;

      case Compiler_Expr_Kind::Char_Literal:
        result = types.character_type();
        break;

      case Compiler_Expr_Kind::Bool_Literal:
        result = types.bool_type();
        break;

      case Compiler_Expr_Kind::Identifier:
        {
          const auto *node = static_cast<const Compiler_Identifier_Expr *>(expr);
          result = lookup_name_type(node->name, node->name_span);
          break;
        }

      case Compiler_Expr_Kind::Grouping:
        {
          const auto *node = static_cast<const Compiler_Grouping_Expr *>(expr);
          result = analyze_expr(node->inner);
          break;
        }

      case Compiler_Expr_Kind::Unary:
        {
          const auto *node = static_cast<const Compiler_Unary_Expr *>(expr);
          const auto operand_type = analyze_expr(node->operand);

          switch (node->op)
            {
            case Compiler_Token_Kind::Plus:
            case Compiler_Token_Kind::Minus:
            case Compiler_Token_Kind::Tilde:
              require_unify(operand_type,
                            types.integer_type(),
                            node->operator_span,
                            "TYP003",
                            "unary operator '" + std::string(compiler_token_kind_name(node->op))
                              + "' requires an Int operand");
              result = types.integer_type();
              break;

            case Compiler_Token_Kind::Bang:
              require_unify(operand_type,
                            types.bool_type(),
                            node->operator_span,
                            "TYP003",
                            "unary operator '!' requires a Bool operand");
              result = types.bool_type();
              break;

            default:
              result = fresh_type();
              break;
            }
          break;
        }

      case Compiler_Expr_Kind::Binary:
        {
          const auto *node = static_cast<const Compiler_Binary_Expr *>(expr);
          const auto left_type = analyze_expr(node->left);
          const auto right_type = analyze_expr(node->right);

          switch (node->op)
            {
            case Compiler_Token_Kind::Plus:
            case Compiler_Token_Kind::Minus:
            case Compiler_Token_Kind::Star:
            case Compiler_Token_Kind::Slash:
            case Compiler_Token_Kind::Percent:
            case Compiler_Token_Kind::Amp:
            case Compiler_Token_Kind::Pipe:
            case Compiler_Token_Kind::Caret:
            case Compiler_Token_Kind::PlusEq:
            case Compiler_Token_Kind::MinusEq:
            case Compiler_Token_Kind::StarEq:
            case Compiler_Token_Kind::SlashEq:
            case Compiler_Token_Kind::PercentEq:
              require_unify(left_type,
                            types.integer_type(),
                            node->operator_span,
                            "TYP004",
                            "binary operator '" + std::string(compiler_token_kind_name(node->op))
                              + "' requires Int operands");
              require_unify(right_type,
                            types.integer_type(),
                            node->operator_span,
                            "TYP004",
                            "binary operator '" + std::string(compiler_token_kind_name(node->op))
                              + "' requires Int operands");
              result = types.integer_type();
              break;

            case Compiler_Token_Kind::Less:
            case Compiler_Token_Kind::LessEq:
            case Compiler_Token_Kind::Greater:
            case Compiler_Token_Kind::GreaterEq:
              require_unify(left_type,
                            types.integer_type(),
                            node->operator_span,
                            "TYP004",
                            "comparison operator '" + std::string(compiler_token_kind_name(node->op))
                              + "' requires Int operands");
              require_unify(right_type,
                            types.integer_type(),
                            node->operator_span,
                            "TYP004",
                            "comparison operator '" + std::string(compiler_token_kind_name(node->op))
                              + "' requires Int operands");
              result = types.bool_type();
              break;

            case Compiler_Token_Kind::AndAnd:
            case Compiler_Token_Kind::OrOr:
              require_unify(left_type,
                            types.bool_type(),
                            node->operator_span,
                            "TYP004",
                            "logical operator '" + std::string(compiler_token_kind_name(node->op))
                              + "' requires Bool operands");
              require_unify(right_type,
                            types.bool_type(),
                            node->operator_span,
                            "TYP004",
                            "logical operator '" + std::string(compiler_token_kind_name(node->op))
                              + "' requires Bool operands");
              result = types.bool_type();
              break;

            case Compiler_Token_Kind::EqEq:
            case Compiler_Token_Kind::NotEq:
            case Compiler_Token_Kind::Assign:
              require_unify(left_type,
                            right_type,
                            node->operator_span,
                            "TYP004",
                            "binary operator '" + std::string(compiler_token_kind_name(node->op))
                              + "' requires compatible operand types");
              result = node->op == Compiler_Token_Kind::Assign ? left_type : types.bool_type();
              break;

            default:
              result = fresh_type();
              break;
            }
          break;
        }

      case Compiler_Expr_Kind::Call:
        {
          const auto *node = static_cast<const Compiler_Call_Expr *>(expr);
          const auto callee_type = analyze_expr(node->callee);
          if (is_invalid_type(callee_type))
            {
              for (size_t i = 0; i < node->arguments.size(); ++i)
                (void) analyze_expr(node->arguments.access(i));
              result = types.invalid_type();
              break;
            }

          DynArray<Compiler_Type_Id> argument_types;
          for (size_t i = 0; i < node->arguments.size(); ++i)
            argument_types.append(analyze_expr(node->arguments.access(i)));

          result = fresh_type();
          const auto callable_type = types.make_function_type(argument_types, result);
          require_unify(
            callee_type, callable_type, node->span, "TYP005", "call expression constraint failed");
          break;
        }
      }

    record_expr_type(expr, result);
    return result;
  }

  void analyze_block(const Compiler_Block_Stmt *block,
                     const Compiler_Type_Id current_return_type,
                     bool &saw_return)
  {
    if (block == nullptr)
      return;

    scopes.enter_scope();
    for (size_t i = 0; i < block->statements.size(); ++i)
      analyze_stmt(block->statements.access(i), current_return_type, saw_return);
    scopes.leave_scope();
  }

  void analyze_stmt(const Compiler_Stmt *stmt,
                    const Compiler_Type_Id current_return_type,
                    bool &saw_return)
  {
    if (stmt == nullptr)
      return;

    switch (stmt->kind)
      {
      case Compiler_Stmt_Kind::Invalid:
        return;

      case Compiler_Stmt_Kind::Expr:
        (void) analyze_expr(static_cast<const Compiler_Expr_Stmt *>(stmt)->expr);
        return;

      case Compiler_Stmt_Kind::Let:
        {
          const auto *node = static_cast<const Compiler_Let_Stmt *>(stmt);
          const auto type_id
            = node->initializer != nullptr ? analyze_expr(node->initializer) : fresh_type();
          (void) scopes.insert(node->name, type_id);
          record_let_type(node, type_id);
          return;
        }

      case Compiler_Stmt_Kind::Return:
        {
          const auto *node = static_cast<const Compiler_Return_Stmt *>(stmt);
          saw_return = true;
          const auto value_type
            = node->value != nullptr ? analyze_expr(node->value) : types.unit_type();
          if (current_return_type != 0)
            require_unify(
              current_return_type,
              value_type,
              node->keyword_span,
              "TYP006",
              "return expression is incompatible with the inferred function result type");
          return;
        }

      case Compiler_Stmt_Kind::Block:
        analyze_block(static_cast<const Compiler_Block_Stmt *>(stmt), current_return_type, saw_return);
        return;

      case Compiler_Stmt_Kind::If:
        {
          const auto *node = static_cast<const Compiler_If_Stmt *>(stmt);
          const auto condition_type = analyze_expr(node->condition);
          require_unify(condition_type,
                        types.bool_type(),
                        node->if_span,
                        "TYP002",
                        "if condition must have type Bool");
          analyze_stmt(node->then_branch, current_return_type, saw_return);
          analyze_stmt(node->else_branch, current_return_type, saw_return);
          return;
        }

      case Compiler_Stmt_Kind::While:
        {
          const auto *node = static_cast<const Compiler_While_Stmt *>(stmt);
          const auto condition_type = analyze_expr(node->condition);
          require_unify(condition_type,
                        types.bool_type(),
                        node->keyword_span,
                        "TYP002",
                        "while condition must have type Bool");
          analyze_stmt(node->body, current_return_type, saw_return);
          return;
        }

      case Compiler_Stmt_Kind::Break:
      case Compiler_Stmt_Kind::Continue:
        return;
      }
  }

  void analyze_function(const Compiler_Function_Decl *function)
  {
    if (function == nullptr)
      return;

    const auto function_type = find_function_type(function);
    if (function_type == 0)
      return;

    const auto &type_node = types.type(function_type);
    const auto return_type = type_node.result_type;

    scopes.enter_scope();
    for (size_t i = 0; i < function->parameters.size(); ++i)
      {
        const auto param_type = type_node.components.access(i);
        (void) scopes.insert(function->parameters.access(i).name, param_type);
      }

    bool saw_return = false;
    analyze_block(function->body, return_type, saw_return);

    if (options.infer_unit_from_missing_return and not saw_return)
      require_unify(return_type,
                    types.unit_type(),
                    function->name_span,
                    "TYP006",
                    "function without explicit returns defaults to Unit");

    scopes.leave_scope();
  }

  void normalize_recorded_types()
  {
    for (size_t i = 0; i < expr_types.size(); ++i)
      expr_types.access(i).type_id = unifier.apply(expr_types.access(i).type_id);
    for (size_t i = 0; i < function_types.size(); ++i)
      function_types.access(i).type_id = unifier.apply(function_types.access(i).type_id);
    for (size_t i = 0; i < let_types.size(); ++i)
      let_types.access(i).type_id = unifier.apply(let_types.access(i).type_id);
    for (size_t i = 0; i < param_types.size(); ++i)
      param_types.access(i).type_id = unifier.apply(param_types.access(i).type_id);
  }

public:
  /** @brief Constructs a typed semantic analyzer.
   *
   *  @param dx Optional diagnostic sink.
   *  @param opts Options controlling base checks and type inference behavior.
   */
  explicit Compiler_Typed_Semantic_Analyzer(Diagnostic_Engine *dx = nullptr,
                                            const Compiler_Typed_Semantic_Options &opts = {})
    : diagnostics(dx), options(opts), base_semantic(dx, opts.semantic_options), unifier(types)
  {}

  /** @brief Clears all inferred state and substitutions. */
  void clear() noexcept
  {
    base_semantic.clear();
    types.clear();
    unifier.clear();
    scopes.clear();
    expr_types.clear();
    function_types.clear();
    let_types.clear();
    param_types.clear();
  }

  /** @brief Runs the typed semantic analysis for `module`.
   *
   *  @param module Parsed module to analyze.
   */
  void analyze_module(const Compiler_Module *module)
  {
    clear();
    if (module == nullptr)
      return;

    if (options.run_base_semantic_checks)
      base_semantic.analyze_module(module);

    scopes.enter_scope();

    for (size_t i = 0; i < module->functions.size(); ++i)
      (void) predeclare_function(module->functions.access(i));

    bool ignored_return = false;
    for (size_t i = 0; i < module->statements.size(); ++i)
      analyze_stmt(module->statements.access(i), 0, ignored_return);

    for (size_t i = 0; i < module->functions.size(); ++i)
      analyze_function(module->functions.access(i));

    scopes.leave_scope();
    normalize_recorded_types();
  }

  /** @brief Returns the base name-resolution pass used by the analyzer. */
  const Compiler_Semantic_Analyzer &semantic_analysis() const noexcept
  {
    return base_semantic;
  }

  /** @brief Returns the internal type context. */
  const Compiler_Type_Context &type_context() const noexcept
  {
    return types;
  }

  /** @brief Returns the internal unifier state. */
  const Compiler_Type_Unifier &type_unifier() const noexcept
  {
    return unifier;
  }

  /** @brief Returns the inferred type of one expression, or `0` if unknown. */
  Compiler_Type_Id inferred_type(const Compiler_Expr *expr) const noexcept
  {
    for (size_t i = expr_types.size(); i > 0; --i)
      if (expr_types.access(i - 1).expr == expr)
        return expr_types.access(i - 1).type_id;
    return 0;
  }

  /** @brief Returns the inferred type of one function, or `0` if unknown. */
  Compiler_Type_Id function_type(const Compiler_Function_Decl *function) const noexcept
  {
    for (size_t i = function_types.size(); i > 0; --i)
      if (function_types.access(i - 1).function == function)
        return function_types.access(i - 1).type_id;
    return 0;
  }

  /** @brief Returns the inferred type of one `let` binding, or `0` if unknown. */
  Compiler_Type_Id let_type(const Compiler_Let_Stmt *stmt) const noexcept
  {
    for (size_t i = let_types.size(); i > 0; --i)
      if (let_types.access(i - 1).stmt == stmt)
        return let_types.access(i - 1).type_id;
    return 0;
  }

  /** @brief Returns the inferred type of one parameter, or `0` if unknown. */
  Compiler_Type_Id parameter_type(const Compiler_Function_Decl *function,
                                  const size_t index) const noexcept
  {
    for (size_t i = param_types.size(); i > 0; --i)
      if (param_types.access(i - 1).function == function and param_types.access(i - 1).index == index)
        return param_types.access(i - 1).type_id;
    return 0;
  }

  /** @brief Dumps inferred function, parameter, and binding types. */
  std::string dump_inference() const
  {
    std::ostringstream out;
    out << "Typed Semantic Analysis\n";

    if (function_types.size() > 0)
      {
        out << "Functions\n";
        for (size_t i = 0; i < function_types.size(); ++i)
          {
            const auto &entry = function_types.access(i);
            out << "  " << entry.function->name << ": " << types.to_string(entry.type_id) << '\n';
          }
      }

    if (param_types.size() > 0)
      {
        out << "Parameters\n";
        for (size_t i = 0; i < param_types.size(); ++i)
          {
            const auto &entry = param_types.access(i);
            out << "  " << entry.function->name << "."
                << entry.function->parameters.access(entry.index).name << ": "
                << types.to_string(entry.type_id) << '\n';
          }
      }

    if (let_types.size() > 0)
      {
        out << "Bindings\n";
        for (size_t i = 0; i < let_types.size(); ++i)
          {
            const auto &entry = let_types.access(i);
            out << "  " << entry.stmt->name << ": " << types.to_string(entry.type_id) << " @"
                << entry.stmt->name_span.to_string() << '\n';
          }
      }

    return out.str();
  }
};
}  // namespace Aleph

#endif  // COMPILER_TYPED_SEMA_H
