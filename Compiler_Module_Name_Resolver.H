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

/** @file Compiler_Module_Name_Resolver.H
 *  @brief Reusable top-level name lookup over bound module surfaces.
 *
 *  This header provides one small semantic query layer above
 *  `Compiler_Module_Binder.H`: given a module name and one symbol query, it
 *  answers whether that symbol resolves locally/imported, is missing, belongs
 *  to the wrong kind, or was omitted because the bound surface is ambiguous.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_MODULE_NAME_RESOLVER_H
#define COMPILER_MODULE_NAME_RESOLVER_H

#include <string>
#include <string_view>

#include <Compiler_Module_Binder.H>
#include <Compiler_Module_Name_Table.H>

namespace Aleph
{
  /** @brief Query filters supported by reusable module-name resolution. */
  enum class Compiler_Module_Name_Filter
  {
    Any,
    Value,
    Function,
    Global,
    Type
  };

  /** @brief Result status for one module-name query. */
  enum class Compiler_Module_Name_Resolution_Status
  {
    Resolved,
    Missing_Module,
    Missing_Name,
    Wrong_Kind,
    Ambiguous_Name
  };

  /** @brief Structured result returned by reusable module-name resolution. */
  struct Compiler_Module_Name_Resolution
  {
    Compiler_Module_Name_Resolution_Status status
      = Compiler_Module_Name_Resolution_Status::Missing_Name;
    std::string module_name;
    std::string symbol_name;
    Compiler_Module_Name_Filter filter = Compiler_Module_Name_Filter::Any;
    Compiler_Module_Bound_Name binding;

    /** @brief Returns whether the query resolved to one concrete bound name. */
    [[nodiscard]] bool
    ok() const noexcept
    {
      return status == Compiler_Module_Name_Resolution_Status::Resolved;
    }
  };

  /** @brief Returns the stable debug name for one module-name filter. */
  inline const char *
  compiler_module_name_filter_name(const Compiler_Module_Name_Filter filter) noexcept
  {
    switch (filter)
      {
      case Compiler_Module_Name_Filter::Any:
        return "Any";
      case Compiler_Module_Name_Filter::Value:
        return "Value";
      case Compiler_Module_Name_Filter::Function:
        return "Function";
      case Compiler_Module_Name_Filter::Global:
        return "Global";
      case Compiler_Module_Name_Filter::Type:
        return "Type";
      }

    return "Unknown";
  }

  /** @brief Returns the stable debug name for one module-name resolution status. */
  inline const char *
  compiler_module_name_resolution_status_name(
    const Compiler_Module_Name_Resolution_Status status) noexcept
  {
    switch (status)
      {
      case Compiler_Module_Name_Resolution_Status::Resolved:
        return "Resolved";
      case Compiler_Module_Name_Resolution_Status::Missing_Module:
        return "Missing_Module";
      case Compiler_Module_Name_Resolution_Status::Missing_Name:
        return "Missing_Name";
      case Compiler_Module_Name_Resolution_Status::Wrong_Kind:
        return "Wrong_Kind";
      case Compiler_Module_Name_Resolution_Status::Ambiguous_Name:
        return "Ambiguous_Name";
      }

    return "Unknown";
  }

  namespace Compiler_Module_Name_Resolver_Detail
  {
    inline bool
    matches_filter(const Compiler_Module_Bound_Name & binding,
                   const Compiler_Module_Name_Filter filter) noexcept
    {
      switch (filter)
        {
        case Compiler_Module_Name_Filter::Any:
          return true;
        case Compiler_Module_Name_Filter::Value:
          return binding.kind != Compiler_Module_Export_Kind::Type;
        case Compiler_Module_Name_Filter::Function:
          return binding.kind == Compiler_Module_Export_Kind::Function;
        case Compiler_Module_Name_Filter::Global:
          return binding.kind == Compiler_Module_Export_Kind::Global;
        case Compiler_Module_Name_Filter::Type:
          return binding.kind == Compiler_Module_Export_Kind::Type;
        }

      return false;
    }

    inline bool
    issue_matches_query(const Compiler_Module_Binding_Issue & issue,
                        const std::string_view module_name,
                        const std::string_view symbol_name,
                        const Compiler_Module_Name_Filter filter) noexcept
    {
      if (issue.module_name != module_name)
        return false;
      if (issue.symbol_name != symbol_name)
        return false;

      if (filter == Compiler_Module_Name_Filter::Type)
        return issue.kind == Compiler_Module_Binding_Issue_Kind::Ambiguous_Type_Name;
      if (filter == Compiler_Module_Name_Filter::Function
          or filter == Compiler_Module_Name_Filter::Global
          or filter == Compiler_Module_Name_Filter::Value
          or filter == Compiler_Module_Name_Filter::Any)
        return issue.kind == Compiler_Module_Binding_Issue_Kind::Ambiguous_Value_Name;
      return false;
    }
  }

  /** @brief Resolves one top-level name through reusable per-module bindings.
   *
   *  @param bindings Binding result produced by `compiler_bind_module_surfaces`.
   *  @param module_name Query module.
   *  @param symbol_name Query symbol.
   *  @param filter Optional kind filter for values/functions/globals/types.
   *  @return Structured resolution result for the requested symbol.
   */
  inline Compiler_Module_Name_Resolution
  compiler_resolve_module_name(const Compiler_Module_Binding_Result & bindings,
                               const std::string_view module_name,
                               const std::string_view symbol_name,
                               const Compiler_Module_Name_Filter filter
                               = Compiler_Module_Name_Filter::Any)
  {
    Compiler_Module_Name_Resolution result;
    result.module_name = std::string(module_name);
    result.symbol_name = std::string(symbol_name);
    result.filter = filter;

    const Compiler_Module_Bindings * bound_module = nullptr;
    for (size_t i = 0; i < bindings.modules.size(); ++i)
      if (bindings.modules.access(i).module_name == module_name)
        {
          bound_module = &bindings.modules.access(i);
          break;
        }

    if (bound_module == nullptr)
      {
        result.status = Compiler_Module_Name_Resolution_Status::Missing_Module;
        return result;
      }

    bool found_wrong_kind = false;
    for (size_t i = 0; i < bound_module->names.size(); ++i)
      {
        const auto & current = bound_module->names.access(i);
        if (current.name != symbol_name)
          continue;

        if (Compiler_Module_Name_Resolver_Detail::matches_filter(current, filter))
          {
            result.status = Compiler_Module_Name_Resolution_Status::Resolved;
            result.binding = current;
            return result;
          }

        result.binding = current;
        found_wrong_kind = true;
      }

    for (size_t i = 0; i < bindings.issues.size(); ++i)
      if (Compiler_Module_Name_Resolver_Detail::issue_matches_query(bindings.issues.access(i),
                                                                    module_name,
                                                                    symbol_name,
                                                                    filter))
        {
          result.status = Compiler_Module_Name_Resolution_Status::Ambiguous_Name;
          return result;
        }

    result.status = found_wrong_kind ? Compiler_Module_Name_Resolution_Status::Wrong_Kind
                                     : Compiler_Module_Name_Resolution_Status::Missing_Name;
    return result;
  }

  /** @brief Resolves one top-level name through a cached per-module name table.
   *
   *  @param table Cached name table built by `compiler_build_module_name_table`.
   *  @param module_name Query module.
   *  @param symbol_name Query symbol.
   *  @param filter Optional kind filter for values/functions/globals/types.
   *  @return Structured resolution result for the requested symbol.
   */
  inline Compiler_Module_Name_Resolution
  compiler_resolve_module_name(const Compiler_Module_Name_Table & table,
                               const std::string_view module_name,
                               const std::string_view symbol_name,
                               const Compiler_Module_Name_Filter filter
                               = Compiler_Module_Name_Filter::Any)
  {
    Compiler_Module_Name_Resolution result;
    result.module_name = std::string(module_name);
    result.symbol_name = std::string(symbol_name);
    result.filter = filter;

    const Compiler_Module_Name_Table_Entry * bound_module = nullptr;
    for (size_t i = 0; i < table.modules.size(); ++i)
      if (table.modules.access(i).module_name == module_name)
        {
          bound_module = &table.modules.access(i);
          break;
        }

    if (bound_module == nullptr)
      {
        result.status = Compiler_Module_Name_Resolution_Status::Missing_Module;
        return result;
      }

    bool found_wrong_kind = false;
    for (size_t i = 0; i < bound_module->names.size(); ++i)
      {
        const auto & current = bound_module->names.access(i);
        if (current.name != symbol_name)
          continue;

        if (Compiler_Module_Name_Resolver_Detail::matches_filter(current, filter))
          {
            result.status = Compiler_Module_Name_Resolution_Status::Resolved;
            result.binding = current;
            return result;
          }

        result.binding = current;
        found_wrong_kind = true;
      }

    for (size_t i = 0; i < bound_module->ambiguous_names.size(); ++i)
      {
        const auto & current = bound_module->ambiguous_names.access(i);
        if (current.name != symbol_name)
          continue;

        if (filter == Compiler_Module_Name_Filter::Type
            and current.namespace_kind == Compiler_Module_Export_Kind::Type)
          {
            result.status = Compiler_Module_Name_Resolution_Status::Ambiguous_Name;
            return result;
          }

        if ((filter == Compiler_Module_Name_Filter::Any
             or filter == Compiler_Module_Name_Filter::Value
             or filter == Compiler_Module_Name_Filter::Function
             or filter == Compiler_Module_Name_Filter::Global)
            and current.namespace_kind != Compiler_Module_Export_Kind::Type)
          {
            result.status = Compiler_Module_Name_Resolution_Status::Ambiguous_Name;
            return result;
          }
      }

    result.status = found_wrong_kind ? Compiler_Module_Name_Resolution_Status::Wrong_Kind
                                     : Compiler_Module_Name_Resolution_Status::Missing_Name;
    return result;
  }
}

#endif
