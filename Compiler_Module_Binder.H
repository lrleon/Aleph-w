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

/** @file Compiler_Module_Binder.H
 *  @brief Reusable binding helpers for linked module surfaces.
 *
 *  This header turns one `Compiler_Module_Link_Result` into a deterministic
 *  binding view that answers which top-level names are actually bindable from
 *  each module after considering local exports, direct imports, and namespace
 *  conflicts. The binding layer intentionally stays shallow: it models visible
 *  top-level names, not full type-checking or member resolution.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_MODULE_BINDER_H
#define COMPILER_MODULE_BINDER_H

#include <sstream>
#include <string>

#include <tpl_dynMapTree.H>
#include <Compiler_Module_Linker.H>

namespace Aleph {
/** @brief One top-level name successfully bound in a module namespace. */
struct Compiler_Module_Bound_Name
{
  Compiler_Module_Export_Kind kind = Compiler_Module_Export_Kind::Global;
  std::string name;
  size_t provider_module_index = static_cast<size_t>(-1);
  std::string provider_module_name;
  Source_Span declaration_span;
  bool imported = false;  ///< `false` for local bindings, `true` for direct-imported bindings.
};

/** @brief Bound top-level names visible from one module. */
struct Compiler_Module_Bindings
{
  std::string module_name;
  size_t module_index = static_cast<size_t>(-1);
  size_t order_index = static_cast<size_t>(-1);
  DynArray<Compiler_Module_Bound_Name> names;
};

/** @brief Structural issues emitted while binding linked module surfaces. */
enum class Compiler_Module_Binding_Issue_Kind
{
  Missing_Metadata,
  Ambiguous_Value_Name,
  Ambiguous_Type_Name
};

/** @brief One structural issue reported by the reusable module binder. */
struct Compiler_Module_Binding_Issue
{
  Compiler_Module_Binding_Issue_Kind kind = Compiler_Module_Binding_Issue_Kind::Missing_Metadata;
  size_t module_index = static_cast<size_t>(-1);
  std::string module_name;
  std::string symbol_name;
  std::string referenced_module_name;
  std::string first_provider_module;
  std::string second_provider_module;
  Source_Span span;
};

/** @brief Binding result over one linked module-surface view. */
struct Compiler_Module_Binding_Result
{
  bool valid = true;
  DynArray<Compiler_Module_Bindings> modules;
  DynArray<Compiler_Module_Binding_Issue> issues;
};

/** @brief Returns the stable debug name for one module-binding issue kind. */
inline const char *compiler_module_binding_issue_name(const Compiler_Module_Binding_Issue_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Module_Binding_Issue_Kind::Missing_Metadata:
      return "Missing_Metadata";
    case Compiler_Module_Binding_Issue_Kind::Ambiguous_Value_Name:
      return "Ambiguous_Value_Name";
    case Compiler_Module_Binding_Issue_Kind::Ambiguous_Type_Name:
      return "Ambiguous_Type_Name";
    }

  return "Unknown";
}

namespace Compiler_Module_Binder_Detail {
inline int export_namespace_bucket(const Compiler_Module_Export_Kind kind) noexcept
{
  return kind == Compiler_Module_Export_Kind::Type ? 1 : 0;
}


inline Compiler_Module_Binding_Issue translate_issue(const Compiler_Module_Linkage_Issue &issue)
{
  Compiler_Module_Binding_Issue translated;
  translated.module_index = issue.module_index;
  translated.module_name = issue.module_name;
  translated.span = issue.span;

  switch (issue.kind)
    {
    case Compiler_Module_Linkage_Issue_Kind::Missing_Metadata:
      translated.kind = Compiler_Module_Binding_Issue_Kind::Missing_Metadata;
      translated.referenced_module_name = issue.second_provider_module;
      break;
    case Compiler_Module_Linkage_Issue_Kind::Duplicate_Value_Export:
      translated.kind = Compiler_Module_Binding_Issue_Kind::Ambiguous_Value_Name;
      translated.symbol_name = issue.export_name;
      translated.first_provider_module = issue.first_provider_module;
      translated.second_provider_module = issue.second_provider_module;
      break;
    case Compiler_Module_Linkage_Issue_Kind::Duplicate_Type_Export:
      translated.kind = Compiler_Module_Binding_Issue_Kind::Ambiguous_Type_Name;
      translated.symbol_name = issue.export_name;
      translated.first_provider_module = issue.first_provider_module;
      translated.second_provider_module = issue.second_provider_module;
      break;
    }

  return translated;
}
}  // namespace Compiler_Module_Binder_Detail

/** @brief Binds linked module surfaces into one deterministic name view.
 *
 *  Every visible top-level declaration with no namespace conflict becomes one
 *  binding entry in the corresponding module. Ambiguous names and missing
 *  metadata remain in `issues` and are omitted from the resulting bindings.
 *
 *  @param linkage Linked direct-import surface result.
 *  @return Binding result with deterministic per-module top-level names.
 */
inline Compiler_Module_Binding_Result compiler_bind_module_surfaces(
  const Compiler_Module_Link_Result &linkage)
{
  Compiler_Module_Binding_Result result;
  result.valid = linkage.valid;

  for (size_t i = 0; i < linkage.issues.size(); ++i)
    result.issues.append(Compiler_Module_Binder_Detail::translate_issue(linkage.issues.access(i)));

  for (size_t i = 0; i < linkage.modules.size(); ++i)
    {
      const auto &linked_module = linkage.modules.access(i);
      Compiler_Module_Bindings bindings;
      bindings.module_name = linked_module.module_name;
      bindings.module_index = linked_module.module_index;
      bindings.order_index = linked_module.order_index;

      // Precompute conflict set: keys with more than one distinct provider
      // in the same namespace bucket are ambiguous. O(N log N) single pass.
      DynMapTree<std::string, size_t> first_provider; // key -> source_module_index of first
      DynMapTree<std::string, bool> ambiguous_keys;
      for (size_t j = 0; j < linked_module.visible_exports.size(); ++j)
        {
          const auto &exp = linked_module.visible_exports.access(j);
          const std::string key =
            std::to_string(Compiler_Module_Binder_Detail::export_namespace_bucket(exp.kind))
            + ":" + exp.name;
          const auto *prev = first_provider.search(key);
          if (prev == nullptr)
            {
              first_provider[key] = exp.source_module_index;
            }
          else if (prev->second != exp.source_module_index)
            {
              ambiguous_keys[key] = true;
            }
        }

      // Binding pass: skip ambiguous names; deduplicate with a bound-names map.
      DynMapTree<std::string, bool> bound_keys;
      for (size_t j = 0; j < linked_module.visible_exports.size(); ++j)
        {
          const auto &item = linked_module.visible_exports.access(j);
          const std::string key =
            std::to_string(Compiler_Module_Binder_Detail::export_namespace_bucket(item.kind))
            + ":" + item.name;
          if (ambiguous_keys.search(key) != nullptr)
            continue;
          if (bound_keys.search(key) != nullptr)
            continue;
          bound_keys[key] = true;
          bindings.names.append({item.kind,
                                 item.name,
                                 item.source_module_index,
                                 item.source_module_name,
                                 item.declaration_span,
                                 item.imported});
        }

      result.modules.append(std::move(bindings));
    }

  return result;
}

/** @brief Renders one deterministic text artifact for bound module names.
 *
 *  @param result Binding result to render.
 *  @return Stable plain-text representation of the bound name view.
 */
inline std::string compiler_render_module_bindings(const Compiler_Module_Binding_Result &result)
{
  std::ostringstream out;
  out << "ModuleBindings\n";
  for (size_t i = 0; i < result.modules.size(); ++i)
    {
      const auto &module = result.modules.access(i);
      out << "  Module(" << module.module_name << ") order=" << module.order_index << '\n';
      for (size_t j = 0; j < module.names.size(); ++j)
        {
          const auto &item = module.names.access(j);
          out << "    Bind(" << compiler_module_export_kind_name(item.kind) << "(" << item.name
              << ")) -> " << item.provider_module_name << ' '
              << (item.imported ? "imported" : "local");
          if (item.declaration_span.is_valid())
            out << " @" << item.declaration_span.to_string();
          out << '\n';
        }
    }

  for (size_t i = 0; i < result.issues.size(); ++i)
    {
      const auto &issue = result.issues.access(i);
      out << "  Issue(" << compiler_module_binding_issue_name(issue.kind)
          << ") module=" << issue.module_name;
      if (not issue.symbol_name.empty())
        out << " name=" << issue.symbol_name;
      if (not issue.referenced_module_name.empty())
        out << " dependency=" << issue.referenced_module_name;
      if (not issue.first_provider_module.empty())
        out << " first=" << issue.first_provider_module;
      if (not issue.second_provider_module.empty())
        out << " second=" << issue.second_provider_module;
      if (issue.span.is_valid())
        out << " @" << issue.span.to_string();
      out << '\n';
    }

  return out.str();
}
}  // namespace Aleph

#endif
