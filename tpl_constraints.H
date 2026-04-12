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


/** @file tpl_constraints.H
 *  @brief Equality constraints and type unification helpers for compiler work.
 *
 *  The generic half of this header is a small equality-constraint worklist.
 *  The compiler-oriented half builds on it with substitutions and a unifier for
 *  `Compiler_Type_Id`.
 *
 *  The current solver supports:
 *
 *  - substitutions for type variables
 *  - rigid variables
 *  - structural unification of tuples and functions
 *  - occurs-check failures
 *
 *  It intentionally does not emit diagnostics by itself. Callers can map
 *  `Compiler_Unify_Result` back to source-level diagnostics when needed.
 *
 *  @ingroup Utilities
 */

# ifndef TPL_CONSTRAINTS_H
# define TPL_CONSTRAINTS_H

# include <sstream>
# include <string>

# include <Compiler_Types.H>
# include <ah-errors.H>
# include <ah-source.H>
# include <tpl_dynArray.H>

namespace Aleph
{
  /** @brief Equality constraint between two values of type `T`. */
  template <typename T>
  struct Equality_Constraint
  {
    T lhs{}; ///< Left-hand side.
    T rhs{}; ///< Right-hand side.
    Source_Span span; ///< Optional source span attached by the caller.
    std::string message; ///< Optional caller-defined explanation.
  };

  /** @brief Ordered collection of equality constraints.
   *
   *  Constraints are processed in insertion order to keep solver behavior
   *  deterministic and easy to test.
   *
   *  @tparam T Item type compared by each equality constraint.
   */
  template <typename T>
  class Constraint_Set
  {
    DynArray<Equality_Constraint<T>> constraints;

  public:
    /** @brief Returns the number of stored constraints. */
    size_t size() const noexcept
    {
      return constraints.size();
    }

    /** @brief Returns whether the set has no constraints. */
    bool empty() const noexcept
    {
      return constraints.is_empty();
    }

    /** @brief Removes all stored constraints. */
    void clear() noexcept
    {
      constraints.clear();
    }

    /** @brief Appends an equality constraint.
     *
     *  @param lhs Left-hand side.
     *  @param rhs Right-hand side.
     *  @param span Optional source span.
     *  @param message Optional debug description.
     *  @return New size after insertion.
     */
    size_t add(const T & lhs,
               const T & rhs,
               const Source_Span & span = {},
               const std::string & message = "")
    {
      Equality_Constraint<T> c;
      c.lhs = lhs;
      c.rhs = rhs;
      c.span = span;
      c.message = message;
      constraints.append(std::move(c));
      return constraints.size();
    }

    /** @brief Returns one stored constraint.
     *
     *  @param i Constraint index.
     *  @return Stored equality constraint.
     *  @throws std::out_of_range If `i` is invalid.
     */
    const Equality_Constraint<T> & constraint(const size_t i) const
    {
      ah_out_of_range_error_unless(i < constraints.size())
        << "Constraint_Set::constraint(): invalid index " << i;
      return constraints.access(i);
    }
  };

  using Compiler_Type_Constraint = Equality_Constraint<Compiler_Type_Id>;
  using Compiler_Type_Constraint_Set = Constraint_Set<Compiler_Type_Id>;

  /** @brief Result category returned by type unification. */
  enum class Compiler_Unify_Result_Kind
  {
    Success,
    Invalid_Type,
    Kind_Mismatch,
    Arity_Mismatch,
    Occurs_Check_Failed,
    Rigid_Variable
  };

  /** @brief Returns a stable debug name for one unification result kind. */
  inline const char *
  compiler_unify_result_kind_name(const Compiler_Unify_Result_Kind kind) noexcept
  {
    switch (kind)
      {
      case Compiler_Unify_Result_Kind::Success: return "Success";
      case Compiler_Unify_Result_Kind::Invalid_Type: return "Invalid_Type";
      case Compiler_Unify_Result_Kind::Kind_Mismatch: return "Kind_Mismatch";
      case Compiler_Unify_Result_Kind::Arity_Mismatch: return "Arity_Mismatch";
      case Compiler_Unify_Result_Kind::Occurs_Check_Failed: return "Occurs_Check_Failed";
      case Compiler_Unify_Result_Kind::Rigid_Variable: return "Rigid_Variable";
      }

    return "Unknown";
  }

  /** @brief Outcome of one type-unification attempt. */
  struct Compiler_Unify_Result
  {
    Compiler_Unify_Result_Kind kind = Compiler_Unify_Result_Kind::Success;
    Compiler_Type_Id lhs = 0; ///< Left type involved in the result.
    Compiler_Type_Id rhs = 0; ///< Right type involved in the result.
    Source_Span span; ///< Constraint span, if solving a stored constraint.
    std::string message; ///< Human-readable explanation.

    /** @brief Returns whether the result denotes a successful unification. */
    bool ok() const noexcept
    {
      return kind == Compiler_Unify_Result_Kind::Success;
    }
  };

  /** @brief Mutable substitution from type variables to replacement types. */
  class Compiler_Type_Substitution
  {
  public:
    /** @brief One variable binding inside the substitution. */
    struct Binding
    {
      Compiler_Type_Id variable = 0; ///< Bound type-variable id.
      Compiler_Type_Id replacement = 0; ///< Replacing type id.
    };

  private:
    DynArray<Binding> bindings;

    Compiler_Type_Id
    resolve_alias(const Compiler_Type_Id id) const
    {
      auto current = id;
      size_t remaining = bindings.size() + 1;
      while (const auto * replacement = lookup(current))
        {
          current = *replacement;
          ah_logic_error_if(remaining == 0)
            << "Compiler_Type_Substitution::resolve_alias(): cyclic alias chain";
          --remaining;
        }
      return current;
    }

  public:
    /** @brief Returns the number of stored variable bindings. */
    size_t size() const noexcept
    {
      return bindings.size();
    }

    /** @brief Returns whether the substitution is empty. */
    bool empty() const noexcept
    {
      return bindings.is_empty();
    }

    /** @brief Removes every binding. */
    void clear() noexcept
    {
      bindings.clear();
    }

    /** @brief Returns binding `i`.
     *
     *  @param i Binding index.
     *  @return Stored binding.
     *  @throws std::out_of_range If `i` is invalid.
     */
    const Binding & binding(const size_t i) const
    {
      ah_out_of_range_error_unless(i < bindings.size())
        << "Compiler_Type_Substitution::binding(): invalid index " << i;
      return bindings.access(i);
    }

    /** @brief Looks up a bound replacement for `variable`.
     *
     *  @param variable Type variable id to search.
     *  @return Replacement pointer, or `nullptr` if unbound.
     */
    Compiler_Type_Id * lookup(const Compiler_Type_Id variable) noexcept
    {
      for (size_t i = bindings.size(); i > 0; --i)
        if (bindings.access(i - 1).variable == variable)
          return &bindings.access(i - 1).replacement;
      return nullptr;
    }

    /** @brief Looks up a bound replacement for `variable`.
     *
     *  @param variable Type variable id to search.
     *  @return Replacement pointer, or `nullptr` if unbound.
     */
    const Compiler_Type_Id * lookup(const Compiler_Type_Id variable) const noexcept
    {
      for (size_t i = bindings.size(); i > 0; --i)
        if (bindings.access(i - 1).variable == variable)
          return &bindings.access(i - 1).replacement;
      return nullptr;
    }

    /** @brief Binds or updates one type-variable replacement.
     *
     *  @param variable Type variable to bind.
     *  @param replacement Replacement type.
     */
    void bind(const Compiler_Type_Id variable,
              const Compiler_Type_Id replacement)
    {
      if (auto * existing = lookup(variable); existing != nullptr)
        {
          *existing = replacement;
          return;
        }

      bindings.append({variable, replacement});
    }

    /** @brief Applies the substitution to `id`.
     *
     *  Composite types are rebuilt only when one of their children changes.
     *  This may append new nodes to `ctx`.
     *
     *  @param ctx Type context used to inspect and rebuild types.
     *  @param id Type to normalize.
     *  @return Normalized type id.
     */
    Compiler_Type_Id apply(Compiler_Type_Context & ctx,
                           const Compiler_Type_Id id) const
    {
      if (id == 0)
        return 0;

      const auto resolved = resolve_alias(id);
      const auto & ty = ctx.type(resolved);
      switch (ty.kind)
        {
        case Compiler_Type_Kind::Builtin:
        case Compiler_Type_Kind::Type_Variable:
          return resolved;

        case Compiler_Type_Kind::Tuple:
          {
            DynArray<Compiler_Type_Id> members;
            bool changed = false;
            for (size_t i = 0; i < ty.components.size(); ++i)
              {
                const auto before = ty.components.access(i);
                const auto after = apply(ctx, before);
                members.append(after);
                if (after != before)
                  changed = true;
              }
            return changed ? ctx.make_tuple_type(members) : resolved;
          }

        case Compiler_Type_Kind::Function:
          {
            DynArray<Compiler_Type_Id> params;
            bool changed = false;
            for (size_t i = 0; i < ty.components.size(); ++i)
              {
                const auto before = ty.components.access(i);
                const auto after = apply(ctx, before);
                params.append(after);
                if (after != before)
                  changed = true;
              }
            const auto new_result = apply(ctx, ty.result_type);
            if (new_result != ty.result_type)
              changed = true;
            return changed ? ctx.make_function_type(params, new_result) : resolved;
          }
        }

      return resolved;
    }

    /** @brief Renders all bindings in insertion order.
     *
     *  @param ctx Type context used to pretty-print type ids.
     *  @return Deterministic text dump of the substitution.
     */
    std::string to_string(Compiler_Type_Context & ctx) const
    {
      std::ostringstream out;
      out << "Substitution\n";
      for (size_t i = 0; i < bindings.size(); ++i)
        {
          const auto & b = bindings.access(i);
          out << "  " << ctx.to_string(b.variable)
              << " := " << ctx.to_string(apply(ctx, b.replacement))
              << '\n';
        }
      return out.str();
    }
  };

  /** @brief Structural unifier for compiler types. */
  class Compiler_Type_Unifier
  {
    Compiler_Type_Context * context = nullptr;
    Compiler_Type_Substitution subst;
    Compiler_Unify_Result last;

    static Compiler_Unify_Result
    success() noexcept
    {
      return {};
    }

    Compiler_Unify_Result
    fail(const Compiler_Unify_Result_Kind kind,
         const Compiler_Type_Id lhs,
         const Compiler_Type_Id rhs,
         const std::string & message,
         const Source_Span & span = {}) noexcept
    {
      Compiler_Unify_Result result;
      result.kind = kind;
      result.lhs = lhs;
      result.rhs = rhs;
      result.span = span;
      result.message = message;
      return result;
    }

    Compiler_Type_Id
    resolve(const Compiler_Type_Id id) const
    {
      if (id == 0)
        return 0;

      auto current = id;
      size_t remaining = subst.size() + 1;
      while (const auto * replacement = subst.lookup(current))
        {
          current = *replacement;
          ah_logic_error_if(remaining == 0)
            << "Compiler_Type_Unifier::resolve(): cyclic substitution";
          --remaining;
        }
      return current;
    }

    bool
    occurs_in(const Compiler_Type_Id variable,
              const Compiler_Type_Id candidate)
    {
      const auto resolved = resolve(candidate);
      if (resolved == variable)
        return true;
      if (resolved == 0)
        return false;

      const auto & ty = context->type(resolved);
      switch (ty.kind)
        {
        case Compiler_Type_Kind::Builtin:
          return false;

        case Compiler_Type_Kind::Type_Variable:
          return resolved == variable;

        case Compiler_Type_Kind::Tuple:
          for (size_t i = 0; i < ty.components.size(); ++i)
            if (occurs_in(variable, ty.components.access(i)))
              return true;
          return false;

        case Compiler_Type_Kind::Function:
          for (size_t i = 0; i < ty.components.size(); ++i)
            if (occurs_in(variable, ty.components.access(i)))
              return true;
          return occurs_in(variable, ty.result_type);
        }

      return false;
    }

    Compiler_Unify_Result
    bind_variable(const Compiler_Type_Id variable,
                  const Compiler_Type_Id replacement)
    {
      const auto resolved_replacement = resolve(replacement);
      if (variable == resolved_replacement)
        return success();

      const auto & var_type = context->type(variable);
      if (var_type.rigid)
        return fail(Compiler_Unify_Result_Kind::Rigid_Variable,
                    variable,
                    resolved_replacement,
                    "cannot bind rigid type variable '"
                    + context->to_string(variable) + "'");

      if (occurs_in(variable, resolved_replacement))
        return fail(Compiler_Unify_Result_Kind::Occurs_Check_Failed,
                    variable,
                    resolved_replacement,
                    "occurs check failed while binding '"
                    + context->to_string(variable) + "' to '"
                    + context->to_string(resolved_replacement) + "'");

      subst.bind(variable, resolved_replacement);
      return success();
    }

  public:
    /** @brief Constructs a unifier operating over `ctx`. */
    explicit Compiler_Type_Unifier(Compiler_Type_Context & ctx) noexcept
      : context(&ctx)
    {}

    /** @brief Clears substitutions and the last stored result. */
    void clear() noexcept
    {
      subst.clear();
      last = {};
    }

    /** @brief Returns the last result produced by `unify()` or `solve()`. */
    const Compiler_Unify_Result & last_result() const noexcept
    {
      return last;
    }

    /** @brief Returns the accumulated substitution. */
    const Compiler_Type_Substitution & substitution() const noexcept
    {
      return subst;
    }

    /** @brief Applies the current substitution to `id`.
     *
     *  @param id Type to normalize.
     *  @return Normalized type id.
     */
    Compiler_Type_Id apply(const Compiler_Type_Id id)
    {
      return subst.apply(*context, id);
    }

    /** @brief Attempts to unify `lhs` and `rhs`.
     *
     *  On success, the internal substitution is updated.
     *
     *  @param lhs Left-hand type.
     *  @param rhs Right-hand type.
     *  @return Success or the first detected failure.
     */
    Compiler_Unify_Result unify(const Compiler_Type_Id lhs,
                                const Compiler_Type_Id rhs)
    {
      if (lhs == 0 or rhs == 0)
        return last = fail(Compiler_Unify_Result_Kind::Invalid_Type,
                           lhs,
                           rhs,
                           "cannot unify null type ids");

      const auto left = resolve(lhs);
      const auto right = resolve(rhs);
      if (left == right)
        return last = success();

      const auto & lhs_type = context->type(left);
      const auto & rhs_type = context->type(right);

      if (lhs_type.kind == Compiler_Type_Kind::Type_Variable)
        return last = bind_variable(left, right);

      if (rhs_type.kind == Compiler_Type_Kind::Type_Variable)
        return last = bind_variable(right, left);

      if (lhs_type.kind != rhs_type.kind)
        return last = fail(Compiler_Unify_Result_Kind::Kind_Mismatch,
                           left,
                           right,
                           "cannot unify '" + context->to_string(left)
                           + "' with '" + context->to_string(right) + "'");

      switch (lhs_type.kind)
        {
        case Compiler_Type_Kind::Builtin:
          if (lhs_type.builtin == rhs_type.builtin)
            return last = success();
          return last = fail(Compiler_Unify_Result_Kind::Kind_Mismatch,
                             left,
                             right,
                             "built-in type mismatch between '"
                             + context->to_string(left) + "' and '"
                             + context->to_string(right) + "'");

        case Compiler_Type_Kind::Tuple:
          if (lhs_type.components.size() != rhs_type.components.size())
            return last = fail(Compiler_Unify_Result_Kind::Arity_Mismatch,
                               left,
                               right,
                               "tuple arity mismatch between '"
                               + context->to_string(left) + "' and '"
                               + context->to_string(right) + "'");

          for (size_t i = 0; i < lhs_type.components.size(); ++i)
            if (auto result = unify(lhs_type.components.access(i),
                                    rhs_type.components.access(i));
                not result.ok())
              return last = result;
          return last = success();

        case Compiler_Type_Kind::Function:
          if (lhs_type.components.size() != rhs_type.components.size())
            return last = fail(Compiler_Unify_Result_Kind::Arity_Mismatch,
                               left,
                               right,
                               "function arity mismatch between '"
                               + context->to_string(left) + "' and '"
                               + context->to_string(right) + "'");

          for (size_t i = 0; i < lhs_type.components.size(); ++i)
            if (auto result = unify(lhs_type.components.access(i),
                                    rhs_type.components.access(i));
                not result.ok())
              return last = result;
          return last = unify(lhs_type.result_type, rhs_type.result_type);

        case Compiler_Type_Kind::Type_Variable:
          break;
        }

      return last = success();
    }

    /** @brief Solves a whole ordered set of type constraints.
     *
     *  Constraints are solved in insertion order and the first failing result
     *  is returned with the corresponding `span`.
     *
     *  @param constraints Ordered equality constraints.
     *  @return Final success result or the first failure encountered.
     */
    Compiler_Unify_Result solve(const Compiler_Type_Constraint_Set & constraints)
    {
      for (size_t i = 0; i < constraints.size(); ++i)
        {
          const auto & c = constraints.constraint(i);
          auto result = unify(c.lhs, c.rhs);
          if (not result.ok())
            {
              result.span = c.span;
              if (result.message.empty())
                result.message = c.message;
              else if (not c.message.empty())
                result.message += ": " + c.message;
              last = result;
              return last;
            }
        }
      return last = success();
    }

    /** @brief Dumps the current substitution in a deterministic format. */
    std::string dump_substitution()
    {
      return subst.to_string(*context);
    }
  };
}

# endif // TPL_CONSTRAINTS_H
