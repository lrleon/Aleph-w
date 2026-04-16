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
 *  - explicit type annotations for parameters, `let` bindings, and returns
 *  - nominal type declarations for `struct`, `enum`, and transparent aliases
 *  - basic polymorphic schemes for explicitly generic declarations and generalized `let`
 *  - operator typing for arithmetic, comparison, logical, and assignment-like operators
 *  - boolean conditions for `if` and `while`
 *  - call constraints of the form `callee ~ fn(args...) -> result`
 *  - return-value constraints against the inferred function result type
 *
 *  This is still intentionally modest: it does not yet solve traits,
 *  overloads, advanced module-level typing, or nominal value operations such as
 *  field access and enum construction.
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
  struct Compiler_Type_Scheme
  {
    Compiler_Type_Id type_id = 0;
    DynArray<Compiler_Type_Id> quantified_variables;
  };

  struct Compiler_Value_Binding
  {
    Compiler_Type_Id type_id = 0;
    bool polymorphic = false;
    Compiler_Type_Scheme scheme;
  };

  struct Compiler_Type_Name_Binding
  {
    std::string name;
    Compiler_Type_Id type_id = 0;
  };

  struct Compiler_Function_Annotation_Type_Variable
  {
    const Compiler_Function_Decl *function = nullptr;
    std::string name;
    Compiler_Type_Id type_id = 0;
  };

  struct Compiler_Active_Binding
  {
    std::string name;
    Compiler_Value_Binding binding;
  };

  struct Compiler_Module_Type_Binding
  {
    std::string name;
    Compiler_Type_Id type_id = 0;
    const Compiler_Type_Alias_Decl *alias_decl = nullptr;
    bool resolving = false;
    bool resolved = false;
  };

  Diagnostic_Engine *diagnostics = nullptr;
  Compiler_Typed_Semantic_Options options;
  Compiler_Semantic_Analyzer base_semantic;
  Compiler_Type_Context types;
  Compiler_Type_Unifier unifier;
  Scope<std::string, Compiler_Value_Binding> scopes;
  DynArray<Compiler_Expr_Type_Assignment> expr_types;
  DynArray<Compiler_Function_Type_Assignment> function_types;
  DynArray<Compiler_Let_Type_Assignment> let_types;
  DynArray<Compiler_Param_Type_Assignment> param_types;
  DynArray<Compiler_Function_Annotation_Type_Variable> function_annotation_type_variables;
  DynArray<Compiler_Active_Binding> active_bindings;
  DynArray<size_t> scope_markers;
  DynArray<Compiler_Module_Type_Binding> module_type_bindings;

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

  static bool contains_type_id(const DynArray<Compiler_Type_Id> &values,
                               const Compiler_Type_Id target) noexcept
  {
    for (size_t i = 0; i < values.size(); ++i)
      if (values.access(i) == target)
        return true;
    return false;
  }

  static void append_unique_type_id(DynArray<Compiler_Type_Id> &values, const Compiler_Type_Id target)
  {
    if (target != 0 and not contains_type_id(values, target))
      values.append(target);
  }

  static bool contains_name(const DynArray<std::string> &names, const std::string &target) noexcept
  {
    for (size_t i = 0; i < names.size(); ++i)
      if (names.access(i) == target)
        return true;
    return false;
  }

  void enter_scope()
  {
    scopes.enter_scope();
    scope_markers.append(active_bindings.size());
  }

  void leave_scope()
  {
    scopes.leave_scope();
    const auto start = scope_markers.size() > 0 ? scope_markers.pop() : 0;
    active_bindings.cut(start);
  }

  bool insert_binding(const std::string &name, const Compiler_Value_Binding &binding)
  {
    const auto inserted = scopes.insert(name, binding);
    if (inserted)
      active_bindings.append({name, binding});
    return inserted;
  }

  Compiler_Module_Type_Binding *find_module_type_binding(const std::string &name) noexcept
  {
    for (size_t i = module_type_bindings.size(); i > 0; --i)
      if (module_type_bindings.access(i - 1).name == name)
        return &module_type_bindings.access(i - 1);
    return nullptr;
  }

  const Compiler_Module_Type_Binding *find_module_type_binding(const std::string &name) const noexcept
  {
    for (size_t i = module_type_bindings.size(); i > 0; --i)
      if (module_type_bindings.access(i - 1).name == name)
        return &module_type_bindings.access(i - 1);
    return nullptr;
  }

  Compiler_Type_Id lookup_type_name(const std::string &name,
                                    const DynArray<Compiler_Type_Name_Binding> &bindings) const noexcept
  {
    for (size_t i = bindings.size(); i > 0; --i)
      if (bindings.access(i - 1).name == name)
        return bindings.access(i - 1).type_id;
    return 0;
  }

  Compiler_Type_Id resolve_builtin_type_name(const std::string &name) const noexcept
  {
    if (name == "Invalid")
      return types.invalid_type();
    if (name == "Unit")
      return types.unit_type();
    if (name == "Bool")
      return types.bool_type();
    if (name == "Int" or name == "Integer")
      return types.integer_type();
    if (name == "String")
      return types.string_type();
    if (name == "Char" or name == "Character")
      return types.character_type();
    return 0;
  }

  Compiler_Type_Id resolve_alias_binding(Compiler_Module_Type_Binding &binding,
                                         const Source_Span &span)
  {
    if (binding.resolved)
      return binding.type_id;

    if (binding.alias_decl == nullptr)
      {
        binding.resolved = true;
        return binding.type_id;
      }

    if (binding.resolving)
      {
        emit_error(span,
                   "TYP010",
                   "cyclic type alias involving '" + binding.name + "'");
        binding.type_id = types.invalid_type();
        binding.resolved = true;
        return binding.type_id;
      }

    binding.resolving = true;
    DynArray<Compiler_Type_Name_Binding> no_type_variables;
    binding.type_id = resolve_type_annotation(binding.alias_decl->aliased_type,
                                              no_type_variables,
                                              false);
    binding.resolving = false;
    binding.resolved = true;
    return binding.type_id;
  }

  Compiler_Type_Id resolve_module_type_name(const std::string &name, const Source_Span &span)
  {
    auto *binding = find_module_type_binding(name);
    if (binding == nullptr)
      return 0;

    if (binding->alias_decl != nullptr)
      return resolve_alias_binding(*binding, span);

    binding->resolved = true;
    return binding->type_id;
  }

  bool validate_unique_member_name(const DynArray<std::string> &names,
                                   const std::string &candidate,
                                   const Source_Span &span,
                                   const std::string &what,
                                   const std::string &owner)
  {
    if (contains_name(names, candidate))
      {
        emit_error(span,
                   "TYP011",
                   "duplicate " + what + " '" + candidate + "' in '" + owner + "'");
        return false;
      }
    return true;
  }

  void predeclare_module_type_declarations(const Compiler_Module *module)
  {
    if (module == nullptr)
      return;

    for (size_t i = 0; i < module->type_declarations.size(); ++i)
      {
        const auto *decl = module->type_declarations.access(i);
        if (decl == nullptr or decl->kind == Compiler_Type_Decl_Kind::Invalid)
          continue;

        if (find_module_type_binding(decl->name) != nullptr)
          {
            emit_error(decl->name_span,
                       "TYP008",
                       "duplicate type declaration of '" + decl->name + "'");
            continue;
          }

        Compiler_Module_Type_Binding binding;
        binding.name = decl->name;

        switch (decl->kind)
          {
          case Compiler_Type_Decl_Kind::Alias:
            binding.alias_decl = static_cast<const Compiler_Type_Alias_Decl *>(decl);
            break;

          case Compiler_Type_Decl_Kind::Struct:
            binding.type_id = types.make_struct_type(decl->name);
            binding.resolved = false;
            break;

          case Compiler_Type_Decl_Kind::Enum:
            binding.type_id = types.make_enum_type(decl->name);
            binding.resolved = false;
            break;

          case Compiler_Type_Decl_Kind::Invalid:
            break;
          }

        module_type_bindings.append(std::move(binding));
      }
  }

  void resolve_module_type_declaration(const Compiler_Type_Decl *decl)
  {
    if (decl == nullptr)
      return;

    auto *binding = find_module_type_binding(decl->name);
    if (binding == nullptr or binding->resolved)
      return;

    switch (decl->kind)
      {
      case Compiler_Type_Decl_Kind::Invalid:
        return;

      case Compiler_Type_Decl_Kind::Alias:
        (void) resolve_alias_binding(*binding, decl->name_span);
        return;

      case Compiler_Type_Decl_Kind::Struct:
        {
          const auto *node = static_cast<const Compiler_Struct_Decl *>(decl);
          DynArray<std::string> field_names;
          DynArray<Compiler_Type_Id> field_types;
          for (size_t i = 0; i < node->fields.size(); ++i)
            {
              const auto &field = node->fields.access(i);
              if (not validate_unique_member_name(field_names,
                                                  field.name,
                                                  field.name_span,
                                                  "field",
                                                  node->name))
                continue;

              DynArray<Compiler_Type_Name_Binding> no_type_variables;
              field_names.append(field.name);
              field_types.append(resolve_type_annotation(field.annotation,
                                                         no_type_variables,
                                                         false));
            }

          types.set_struct_fields(binding->type_id, field_names, field_types);
          binding->resolved = true;
          return;
        }

      case Compiler_Type_Decl_Kind::Enum:
        {
          const auto *node = static_cast<const Compiler_Enum_Decl *>(decl);
          DynArray<std::string> variant_names;
          for (size_t i = 0; i < node->variants.size(); ++i)
            {
              const auto &variant = node->variants.access(i);
              if (validate_unique_member_name(variant_names,
                                              variant.name,
                                              variant.name_span,
                                              "enum variant",
                                              node->name))
                variant_names.append(variant.name);
            }

          types.set_enum_variants(binding->type_id, variant_names);
          binding->resolved = true;
          return;
        }
      }
  }

  void resolve_module_type_declarations(const Compiler_Module *module)
  {
    if (module == nullptr)
      return;

    for (size_t i = 0; i < module->type_declarations.size(); ++i)
      resolve_module_type_declaration(module->type_declarations.access(i));
  }

  Compiler_Type_Id resolve_type_annotation(const Compiler_Type_Expr *type_expr,
                                           DynArray<Compiler_Type_Name_Binding> &type_names,
                                           const bool allow_fresh_type_variables = true)
  {
    if (type_expr == nullptr)
      return types.invalid_type();

    switch (type_expr->kind)
      {
      case Compiler_Type_Expr_Kind::Invalid:
        return types.invalid_type();

      case Compiler_Type_Expr_Kind::Named:
        {
          const auto *node = static_cast<const Compiler_Named_Type_Expr *>(type_expr);
          if (const auto builtin = resolve_builtin_type_name(node->name); builtin != 0)
            return builtin;

          if (const auto known = lookup_type_name(node->name, type_names); known != 0)
            return known;

          if (const auto declared = resolve_module_type_name(node->name, node->name_span); declared != 0)
            return declared;

          if (not allow_fresh_type_variables)
            {
              emit_error(node->name_span,
                         "TYP009",
                         "unknown type name '" + node->name + "'");
              return types.invalid_type();
            }

          const auto variable = types.make_type_variable(node->name, true);
          type_names.append({node->name, variable});
          return variable;
        }

      case Compiler_Type_Expr_Kind::Tuple:
        {
          const auto *node = static_cast<const Compiler_Tuple_Type_Expr *>(type_expr);
          DynArray<Compiler_Type_Id> members;
          for (size_t i = 0; i < node->members.size(); ++i)
            members.append(
              resolve_type_annotation(node->members.access(i), type_names, allow_fresh_type_variables));
          return types.make_tuple_type(members);
        }

      case Compiler_Type_Expr_Kind::Function:
        {
          const auto *node = static_cast<const Compiler_Function_Type_Expr *>(type_expr);
          DynArray<Compiler_Type_Id> params;
          for (size_t i = 0; i < node->parameters.size(); ++i)
            params.append(resolve_type_annotation(node->parameters.access(i),
                                                 type_names,
                                                 allow_fresh_type_variables));
          return types.make_function_type(
            params, resolve_type_annotation(node->result, type_names, allow_fresh_type_variables));
        }
      }

    return types.invalid_type();
  }

  void remember_function_annotation_type_variables(
    const Compiler_Function_Decl *function,
    const DynArray<Compiler_Type_Name_Binding> &type_names)
  {
    if (function == nullptr)
      return;

    for (size_t i = 0; i < type_names.size(); ++i)
      function_annotation_type_variables.append(
        {function, type_names.access(i).name, type_names.access(i).type_id});
  }

  void seed_function_annotation_type_variables(
    const Compiler_Function_Decl *function,
    DynArray<Compiler_Type_Name_Binding> &type_names) const
  {
    if (function == nullptr)
      return;

    for (size_t i = 0; i < function_annotation_type_variables.size(); ++i)
      if (function_annotation_type_variables.access(i).function == function)
        type_names.append({function_annotation_type_variables.access(i).name,
                           function_annotation_type_variables.access(i).type_id});
  }

  Compiler_Type_Id instantiate_type(
    const Compiler_Type_Id type_id,
    const DynArray<Compiler_Type_Id> &quantified_variables,
    DynArray<Compiler_Type_Id> &originals,
    DynArray<Compiler_Type_Id> &replacements)
  {
    const auto resolved = unifier.apply(type_id);
    if (is_invalid_type(resolved))
      return resolved;

    if (types.is_builtin(resolved))
      return resolved;

    const auto &type = types.type(resolved);
    if (type.kind == Compiler_Type_Kind::Type_Variable)
      {
        for (size_t i = 0; i < quantified_variables.size(); ++i)
          if (quantified_variables.access(i) == resolved)
            {
              for (size_t j = 0; j < originals.size() and j < replacements.size(); ++j)
                if (originals.access(j) == resolved)
                  return replacements.access(j);

              const auto fresh = fresh_type();
              originals.append(resolved);
              replacements.append(fresh);
              return fresh;
            }

        return resolved;
      }

    if (type.kind == Compiler_Type_Kind::Tuple)
      {
        DynArray<Compiler_Type_Id> members;
        for (size_t i = 0; i < type.components.size(); ++i)
          members.append(instantiate_type(type.components.access(i),
                                          quantified_variables,
                                          originals,
                                          replacements));
        return types.make_tuple_type(members);
      }

    if (type.kind == Compiler_Type_Kind::Function)
      {
        DynArray<Compiler_Type_Id> params;
        for (size_t i = 0; i < type.components.size(); ++i)
          params.append(instantiate_type(type.components.access(i),
                                         quantified_variables,
                                         originals,
                                         replacements));
        const auto result
          = instantiate_type(type.result_type, quantified_variables, originals, replacements);
        return types.make_function_type(params, result);
      }

    if (type.kind == Compiler_Type_Kind::Struct or type.kind == Compiler_Type_Kind::Enum)
      return resolved;

    return resolved;
  }

  Compiler_Type_Id instantiate_binding(const Compiler_Value_Binding &binding)
  {
    if (not binding.polymorphic)
      return unifier.apply(binding.type_id);

    DynArray<Compiler_Type_Id> originals;
    DynArray<Compiler_Type_Id> replacements;
    return instantiate_type(binding.scheme.type_id,
                            binding.scheme.quantified_variables,
                            originals,
                            replacements);
  }

  void collect_free_type_variables(const Compiler_Type_Id type_id,
                                   DynArray<Compiler_Type_Id> &free_variables)
  {
    const auto resolved = unifier.apply(type_id);
    if (is_invalid_type(resolved) or types.is_builtin(resolved))
      return;

    const auto &type = types.type(resolved);
    switch (type.kind)
      {
      case Compiler_Type_Kind::Builtin:
      case Compiler_Type_Kind::Struct:
      case Compiler_Type_Kind::Enum:
        return;

      case Compiler_Type_Kind::Type_Variable:
        append_unique_type_id(free_variables, resolved);
        return;

      case Compiler_Type_Kind::Tuple:
        for (size_t i = 0; i < type.components.size(); ++i)
          collect_free_type_variables(type.components.access(i), free_variables);
        return;

      case Compiler_Type_Kind::Function:
        for (size_t i = 0; i < type.components.size(); ++i)
          collect_free_type_variables(type.components.access(i), free_variables);
        collect_free_type_variables(type.result_type, free_variables);
        return;
      }
  }

  void collect_binding_free_type_variables(const Compiler_Value_Binding &binding,
                                           DynArray<Compiler_Type_Id> &free_variables)
  {
    if (not binding.polymorphic)
      {
        collect_free_type_variables(binding.type_id, free_variables);
        return;
      }

    DynArray<Compiler_Type_Id> scheme_free_variables;
    collect_free_type_variables(binding.scheme.type_id, scheme_free_variables);
    for (size_t i = 0; i < scheme_free_variables.size(); ++i)
      if (not contains_type_id(binding.scheme.quantified_variables, scheme_free_variables.access(i)))
        append_unique_type_id(free_variables, scheme_free_variables.access(i));
  }

  void collect_environment_free_type_variables(DynArray<Compiler_Type_Id> &free_variables)
  {
    DynArray<std::string> seen_names;
    for (size_t i = active_bindings.size(); i > 0; --i)
      {
        const auto &entry = active_bindings.access(i - 1);
        if (contains_name(seen_names, entry.name))
          continue;

        seen_names.append(entry.name);
        collect_binding_free_type_variables(entry.binding, free_variables);
      }
  }

  Compiler_Type_Scheme generalize_type(
    const Compiler_Type_Id type_id,
    const DynArray<Compiler_Type_Id> &explicit_quantified_variables = {})
  {
    Compiler_Type_Scheme scheme;
    scheme.type_id = unifier.apply(type_id);

    for (size_t i = 0; i < explicit_quantified_variables.size(); ++i)
      append_unique_type_id(scheme.quantified_variables, explicit_quantified_variables.access(i));

    DynArray<Compiler_Type_Id> free_type_variables;
    collect_free_type_variables(scheme.type_id, free_type_variables);

    DynArray<Compiler_Type_Id> environment_free_variables;
    collect_environment_free_type_variables(environment_free_variables);

    for (size_t i = 0; i < free_type_variables.size(); ++i)
      {
        const auto variable = free_type_variables.access(i);
        const auto &type = types.type(variable);
        if (type.kind != Compiler_Type_Kind::Type_Variable)
          continue;

        if (type.rigid)
          continue;

        if (contains_type_id(environment_free_variables, variable))
          continue;

        append_unique_type_id(scheme.quantified_variables, variable);
      }

    return scheme;
  }

  Compiler_Type_Id lookup_name_type(const std::string &name, const Source_Span &span)
  {
    if (const auto *binding = scopes.lookup(name); binding != nullptr)
      return instantiate_binding(*binding);

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
    DynArray<Compiler_Type_Name_Binding> type_names;
    for (size_t i = 0; i < function->parameters.size(); ++i)
      {
        const auto &param = function->parameters.access(i);
        const auto param_type = param.annotation != nullptr
                                  ? resolve_type_annotation(param.annotation, type_names)
                                  : fresh_type();
        params.append(param_type);
        record_param_type(function, i, param_type);
      }

    const auto result_type = function->return_annotation != nullptr
                               ? resolve_type_annotation(function->return_annotation, type_names)
                               : fresh_type();
    const auto function_type = types.make_function_type(params, result_type);
    record_function_type(function, function_type);
    remember_function_annotation_type_variables(function, type_names);

    Compiler_Value_Binding binding;
    binding.type_id = function_type;
    if (type_names.size() > 0)
      {
        binding.polymorphic = true;
        binding.scheme.type_id = function_type;
        for (size_t i = 0; i < type_names.size(); ++i)
          append_unique_type_id(binding.scheme.quantified_variables, type_names.access(i).type_id);
      }

    (void) insert_binding(function->name, binding);
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
                     const Compiler_Function_Decl *current_function,
                     bool &saw_return)
  {
    if (block == nullptr)
      return;

    enter_scope();
    for (size_t i = 0; i < block->statements.size(); ++i)
      analyze_stmt(block->statements.access(i), current_return_type, current_function, saw_return);
    leave_scope();
  }

  void analyze_stmt(const Compiler_Stmt *stmt,
                    const Compiler_Type_Id current_return_type,
                    const Compiler_Function_Decl *current_function,
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
          DynArray<Compiler_Type_Name_Binding> type_names;
          seed_function_annotation_type_variables(current_function, type_names);

          const auto annotation_type = node->annotation != nullptr
                                         ? resolve_type_annotation(node->annotation, type_names)
                                         : Compiler_Type_Id(0);
          const auto initializer_type
            = node->initializer != nullptr ? analyze_expr(node->initializer) : Compiler_Type_Id(0);

          Compiler_Type_Id type_id = types.invalid_type();
          if (annotation_type != 0 and initializer_type != 0)
            {
              (void) require_unify(annotation_type,
                                   initializer_type,
                                   node->name_span,
                                   "TYP007",
                                   "initializer is incompatible with the declared binding type");
              type_id = annotation_type;
            }
          else if (annotation_type != 0)
            type_id = annotation_type;
          else if (initializer_type != 0)
            type_id = initializer_type;
          else
            type_id = fresh_type();

          const auto scheme = generalize_type(
            type_id,
            [&type_names]()
            {
              DynArray<Compiler_Type_Id> quantified;
              for (size_t i = 0; i < type_names.size(); ++i)
                quantified.append(type_names.access(i).type_id);
              return quantified;
            }());

          Compiler_Value_Binding binding;
          binding.type_id = scheme.type_id;
          binding.polymorphic = scheme.quantified_variables.size() > 0;
          binding.scheme = scheme;

          (void) insert_binding(node->name, binding);
          record_let_type(node, binding.type_id);
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
        analyze_block(static_cast<const Compiler_Block_Stmt *>(stmt),
                      current_return_type,
                      current_function,
                      saw_return);
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
          analyze_stmt(node->then_branch, current_return_type, current_function, saw_return);
          analyze_stmt(node->else_branch, current_return_type, current_function, saw_return);
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
          analyze_stmt(node->body, current_return_type, current_function, saw_return);
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

    enter_scope();
    for (size_t i = 0; i < function->parameters.size(); ++i)
      {
        const auto param_type = type_node.components.access(i);
        Compiler_Value_Binding binding;
        binding.type_id = param_type;
        (void) insert_binding(function->parameters.access(i).name, binding);
      }

    bool saw_return = false;
    analyze_block(function->body, return_type, function, saw_return);

    if (options.infer_unit_from_missing_return and not saw_return)
      require_unify(return_type,
                    types.unit_type(),
                    function->name_span,
                    "TYP006",
                    "function without explicit returns defaults to Unit");

    leave_scope();
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
    function_annotation_type_variables.clear();
    active_bindings.clear();
    scope_markers.clear();
    module_type_bindings.clear();
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

    predeclare_module_type_declarations(module);
    resolve_module_type_declarations(module);

    enter_scope();

    for (size_t i = 0; i < module->functions.size(); ++i)
      (void) predeclare_function(module->functions.access(i));

    bool ignored_return = false;
    for (size_t i = 0; i < module->statements.size(); ++i)
      analyze_stmt(module->statements.access(i), 0, nullptr, ignored_return);

    for (size_t i = 0; i < module->functions.size(); ++i)
      analyze_function(module->functions.access(i));

    leave_scope();
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

  /** @brief Returns one declared module type by name, or `0` if unknown.
   *
   *  Transparent aliases resolve to their aliased target type id.
   *
   *  @param name Declared type name.
   *  @return Resolved type id or `0` when the name is not known.
   */
  Compiler_Type_Id declared_type(const std::string &name) const noexcept
  {
    if (const auto *binding = find_module_type_binding(name); binding != nullptr)
      return binding->type_id;
    return 0;
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

    if (module_type_bindings.size() > 0)
      {
        out << "Types\n";
        for (size_t i = 0; i < module_type_bindings.size(); ++i)
          {
            const auto &entry = module_type_bindings.access(i);
            out << "  " << entry.name << ": ";
            if (entry.type_id != 0)
              out << types.to_string(entry.type_id);
            else
              out << "<unresolved>";
            out << '\n';
          }
      }

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
