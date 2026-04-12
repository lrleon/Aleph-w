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


/** @file Compiler_Types.H
 *  @brief Stable type graph for the compiler-support MVP.
 *
 *  This header introduces a compact type representation that can be shared by
 *  parsers, semantic analyzers, and future inference passes. Types are owned by
 *  `Compiler_Type_Context` and referenced through stable integer identifiers.
 *
 *  The current model is intentionally small:
 *
 *  - built-in scalar types
 *  - tuple types
 *  - function types
 *  - type variables, including optional rigid variables
 *
 *  This layer is independent from the AST on purpose. It can be reused by a
 *  hand-written parser, a yacc-generated parser, or a future typed IR.
 *
 *  @ingroup Utilities
 */

# ifndef COMPILER_TYPES_H
# define COMPILER_TYPES_H

# include <initializer_list>
# include <sstream>
# include <string>
# include <utility>

# include <ah-errors.H>
# include <tpl_dynArray.H>

namespace Aleph
{
  using Compiler_Type_Id = size_t;

  /** @brief Category of one stored type node. */
  enum class Compiler_Type_Kind
  {
    Builtin,
    Tuple,
    Function,
    Type_Variable
  };

  /** @brief Built-in scalar types available in the MVP type graph. */
  enum class Compiler_Builtin_Type
  {
    Invalid,
    Unit,
    Bool,
    Integer,
    String,
    Character
  };

  /** @brief Returns a stable debug name for a type kind. */
  inline const char *
  compiler_type_kind_name(const Compiler_Type_Kind kind) noexcept
  {
    switch (kind)
      {
      case Compiler_Type_Kind::Builtin: return "Builtin";
      case Compiler_Type_Kind::Tuple: return "Tuple";
      case Compiler_Type_Kind::Function: return "Function";
      case Compiler_Type_Kind::Type_Variable: return "Type_Variable";
      }

    return "Unknown";
  }

  /** @brief Returns a stable debug name for a built-in type. */
  inline const char *
  compiler_builtin_type_name(const Compiler_Builtin_Type builtin) noexcept
  {
    switch (builtin)
      {
      case Compiler_Builtin_Type::Invalid: return "Invalid";
      case Compiler_Builtin_Type::Unit: return "Unit";
      case Compiler_Builtin_Type::Bool: return "Bool";
      case Compiler_Builtin_Type::Integer: return "Int";
      case Compiler_Builtin_Type::String: return "String";
      case Compiler_Builtin_Type::Character: return "Char";
      }

    return "Unknown";
  }

  /** @brief One node inside the type graph.
   *
   *  For `Tuple`, `components` stores the tuple members.
   *
   *  For `Function`, `components` stores the parameter types and `result_type`
   *  stores the return type.
   *
   *  For `Type_Variable`, `label` is the debug/display name and `rigid`
   *  controls whether a unifier is allowed to bind the variable.
   */
  struct Compiler_Type
  {
    Compiler_Type_Id id = 0; ///< Stable 1-based identifier.
    Compiler_Type_Kind kind = Compiler_Type_Kind::Builtin; ///< Node category.
    Compiler_Builtin_Type builtin = Compiler_Builtin_Type::Invalid; ///< Built-in tag.
    DynArray<Compiler_Type_Id> components; ///< Tuple members or function parameters.
    Compiler_Type_Id result_type = 0; ///< Function return type, or `0`.
    std::string label; ///< Display label for type variables.
    bool rigid = false; ///< Whether the type variable is rigid.

    /** @brief Returns whether this node represents a built-in type. */
    bool is_builtin() const noexcept
    {
      return kind == Compiler_Type_Kind::Builtin;
    }

    /** @brief Returns whether this node represents a type variable. */
    bool is_type_variable() const noexcept
    {
      return kind == Compiler_Type_Kind::Type_Variable;
    }
  };

  /** @brief Context owning all compiler type nodes.
   *
   *  Built-in types are created eagerly and remain stable for the life of the
   *  context. User-defined nodes can be discarded with `clear()`, which keeps
   *  the built-ins and resets the fresh type-variable counter.
   */
  class Compiler_Type_Context
  {
    DynArray<Compiler_Type> types;
    size_t builtin_type_count = 0;
    size_t next_type_variable_index = 0;

    Compiler_Type_Id invalid_id = 0;
    Compiler_Type_Id unit_id = 0;
    Compiler_Type_Id bool_id = 0;
    Compiler_Type_Id integer_id = 0;
    Compiler_Type_Id string_id = 0;
    Compiler_Type_Id character_id = 0;

    Compiler_Type_Id
    append_type(Compiler_Type type)
    {
      type.id = types.size() + 1;
      types.append(std::move(type));
      return types.size();
    }

    void
    initialize_builtins()
    {
      auto add_builtin = [this](const Compiler_Builtin_Type builtin)
      {
        Compiler_Type type;
        type.kind = Compiler_Type_Kind::Builtin;
        type.builtin = builtin;
        return append_type(std::move(type));
      };

      invalid_id = add_builtin(Compiler_Builtin_Type::Invalid);
      unit_id = add_builtin(Compiler_Builtin_Type::Unit);
      bool_id = add_builtin(Compiler_Builtin_Type::Bool);
      integer_id = add_builtin(Compiler_Builtin_Type::Integer);
      string_id = add_builtin(Compiler_Builtin_Type::String);
      character_id = add_builtin(Compiler_Builtin_Type::Character);
      builtin_type_count = types.size();
    }

    static DynArray<Compiler_Type_Id>
    list_to_array(const std::initializer_list<Compiler_Type_Id> ids)
    {
      DynArray<Compiler_Type_Id> result;
      for (const auto id : ids)
        result.append(id);
      return result;
    }

    void
    append_type_text(std::ostream & out, const Compiler_Type_Id id) const
    {
      const auto & ty = type(id);
      switch (ty.kind)
        {
        case Compiler_Type_Kind::Builtin:
          out << compiler_builtin_type_name(ty.builtin);
          return;

        case Compiler_Type_Kind::Type_Variable:
          out << ty.label;
          if (ty.rigid)
            out << '!';
          return;

        case Compiler_Type_Kind::Tuple:
          out << '(';
          for (size_t i = 0; i < ty.components.size(); ++i)
            {
              if (i > 0)
                out << ", ";
              append_type_text(out, ty.components.access(i));
            }
          out << ')';
          return;

        case Compiler_Type_Kind::Function:
          out << "fn(";
          for (size_t i = 0; i < ty.components.size(); ++i)
            {
              if (i > 0)
                out << ", ";
              append_type_text(out, ty.components.access(i));
            }
          out << ") -> ";
          append_type_text(out, ty.result_type);
          return;
        }
    }

  public:
    /** @brief Builds a fresh type context with the built-ins preloaded. */
    Compiler_Type_Context()
    {
      initialize_builtins();
    }

    /** @brief Drops user-created types and keeps the built-ins.
     *
     *  Fresh type-variable names restart from `T0`.
     */
    void clear() noexcept
    {
      types.clear();
      builtin_type_count = 0;
      next_type_variable_index = 0;
      invalid_id = 0;
      unit_id = 0;
      bool_id = 0;
      integer_id = 0;
      string_id = 0;
      character_id = 0;
      initialize_builtins();
    }

    /** @brief Returns the total number of stored types. */
    size_t size() const noexcept
    {
      return types.size();
    }

    /** @brief Returns the number of eager built-in types. */
    size_t builtin_count() const noexcept
    {
      return builtin_type_count;
    }

    /** @brief Returns the preloaded `Invalid` type id. */
    Compiler_Type_Id invalid_type() const noexcept
    {
      return invalid_id;
    }

    /** @brief Returns the preloaded `Unit` type id. */
    Compiler_Type_Id unit_type() const noexcept
    {
      return unit_id;
    }

    /** @brief Returns the preloaded `Bool` type id. */
    Compiler_Type_Id bool_type() const noexcept
    {
      return bool_id;
    }

    /** @brief Returns the preloaded `Int` type id. */
    Compiler_Type_Id integer_type() const noexcept
    {
      return integer_id;
    }

    /** @brief Returns the preloaded `String` type id. */
    Compiler_Type_Id string_type() const noexcept
    {
      return string_id;
    }

    /** @brief Returns the preloaded `Char` type id. */
    Compiler_Type_Id character_type() const noexcept
    {
      return character_id;
    }

    /** @brief Returns type `id`.
     *
     *  @param id 1-based type identifier.
     *  @return Stored type node.
     *  @throws std::out_of_range If `id` is invalid.
     */
    const Compiler_Type &
    type(const Compiler_Type_Id id) const
    {
      ah_out_of_range_error_unless(id > 0 and id <= types.size())
        << "Compiler_Type_Context::type(): invalid id " << id;
      return types.access(id - 1);
    }

    /** @brief Returns whether `id` names a type variable. */
    bool is_type_variable(const Compiler_Type_Id id) const
    {
      return type(id).kind == Compiler_Type_Kind::Type_Variable;
    }

    /** @brief Returns whether `id` names a built-in type. */
    bool is_builtin(const Compiler_Type_Id id) const
    {
      return type(id).kind == Compiler_Type_Kind::Builtin;
    }

    /** @brief Creates a fresh type variable.
     *
     *  @param label Optional display name. If empty, a fresh `T<n>` label is
     *         generated.
     *  @param rigid If true, unifiers should treat the variable as rigid.
     *  @return Stable type id for the new variable.
     */
    Compiler_Type_Id make_type_variable(std::string label = "",
                                        const bool rigid = false)
    {
      Compiler_Type ty;
      ty.kind = Compiler_Type_Kind::Type_Variable;
      ty.rigid = rigid;
      ty.label = label.empty() ?
          ("T" + std::to_string(next_type_variable_index++)) :
          std::move(label);
      return append_type(std::move(ty));
    }

    /** @brief Creates a tuple type.
     *
     *  @param members Member types in source order.
     *  @return Stable type id for the tuple.
     */
    Compiler_Type_Id
    make_tuple_type(const DynArray<Compiler_Type_Id> & members)
    {
      Compiler_Type ty;
      ty.kind = Compiler_Type_Kind::Tuple;
      ty.components = members;
      return append_type(std::move(ty));
    }

    /** @brief Creates a tuple type from a small literal list.
     *
     *  @param members Member types in source order.
     *  @return Stable type id for the tuple.
     */
    Compiler_Type_Id
    make_tuple_type(const std::initializer_list<Compiler_Type_Id> members)
    {
      return make_tuple_type(list_to_array(members));
    }

    /** @brief Creates a function type.
     *
     *  @param parameters Parameter types in declaration order.
     *  @param result Return type.
     *  @return Stable type id for the function type.
     */
    Compiler_Type_Id
    make_function_type(const DynArray<Compiler_Type_Id> & parameters,
                       const Compiler_Type_Id result)
    {
      Compiler_Type ty;
      ty.kind = Compiler_Type_Kind::Function;
      ty.components = parameters;
      ty.result_type = result;
      return append_type(std::move(ty));
    }

    /** @brief Creates a function type from a small literal list.
     *
     *  @param parameters Parameter types in declaration order.
     *  @param result Return type.
     *  @return Stable type id for the function type.
     */
    Compiler_Type_Id
    make_function_type(const std::initializer_list<Compiler_Type_Id> parameters,
                       const Compiler_Type_Id result)
    {
      return make_function_type(list_to_array(parameters), result);
    }

    /** @brief Renders one type to a deterministic human-readable string.
     *
     *  @param id Type to render.
     *  @return Textual representation such as `Int` or `fn(Int) -> Bool`.
     */
    std::string to_string(const Compiler_Type_Id id) const
    {
      std::ostringstream out;
      append_type_text(out, id);
      return out.str();
    }

    /** @brief Dumps all known types in allocation order. */
    std::string dump_types() const
    {
      std::ostringstream out;
      out << "Types\n";
      for (size_t i = 0; i < types.size(); ++i)
        {
          const auto & ty = types.access(i);
          out << "  #" << ty.id
              << " " << compiler_type_kind_name(ty.kind)
              << " " << to_string(ty.id)
              << '\n';
        }
      return out.str();
    }
  };
}

# endif // COMPILER_TYPES_H
