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

/** @file Compiler_Module_Name_Table.H
 *  @brief Reusable per-module cache for top-level name resolution.
 *
 *  This header precomputes one deterministic name table from
 *  `Compiler_Module_Binding_Result`. The resulting structure is suitable for
 *  repeated semantic queries without rescanning the full binding result and its
 *  issues every time.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_MODULE_NAME_TABLE_H
#define COMPILER_MODULE_NAME_TABLE_H

#include <sstream>
#include <string>

#include <Compiler_Module_Binder.H>

namespace Aleph
{
  /** @brief One ambiguous name tracked in a per-module name table. */
  struct Compiler_Module_Ambiguous_Name
  {
    std::string name;
    Compiler_Module_Export_Kind namespace_kind = Compiler_Module_Export_Kind::Global;
  };

  /** @brief Cached top-level names for one module. */
  struct Compiler_Module_Name_Table_Entry
  {
    std::string module_name;
    size_t module_index = static_cast<size_t>(-1);
    size_t order_index = static_cast<size_t>(-1);
    DynArray<Compiler_Module_Bound_Name> names;
    DynArray<Compiler_Module_Ambiguous_Name> ambiguous_names;
  };

  /** @brief Cached top-level name table over all visible modules. */
  struct Compiler_Module_Name_Table
  {
    DynArray<Compiler_Module_Name_Table_Entry> modules;
  };

  namespace Compiler_Module_Name_Table_Detail
  {
    inline Compiler_Module_Export_Kind
    issue_namespace_kind(const Compiler_Module_Binding_Issue_Kind kind) noexcept
    {
      return kind == Compiler_Module_Binding_Issue_Kind::Ambiguous_Type_Name
               ? Compiler_Module_Export_Kind::Type
               : Compiler_Module_Export_Kind::Global;
    }
  }

  /** @brief Builds one reusable per-module name table from bindings.
   *
   *  @param bindings Bound module surfaces.
   *  @return Deterministic cache for repeated top-level name queries.
   */
  inline Compiler_Module_Name_Table
  compiler_build_module_name_table(const Compiler_Module_Binding_Result & bindings)
  {
    Compiler_Module_Name_Table table;

    for (size_t i = 0; i < bindings.modules.size(); ++i)
      {
        const auto & source = bindings.modules.access(i);
        Compiler_Module_Name_Table_Entry entry;
        entry.module_name = source.module_name;
        entry.module_index = source.module_index;
        entry.order_index = source.order_index;

        for (size_t j = 0; j < source.names.size(); ++j)
          entry.names.append(source.names.access(j));

        for (size_t j = 0; j < bindings.issues.size(); ++j)
          {
            const auto & issue = bindings.issues.access(j);
            if (issue.module_name != source.module_name)
              continue;
            if (issue.kind != Compiler_Module_Binding_Issue_Kind::Ambiguous_Value_Name
                and issue.kind != Compiler_Module_Binding_Issue_Kind::Ambiguous_Type_Name)
              continue;

            bool duplicate = false;
            for (size_t k = 0; k < entry.ambiguous_names.size(); ++k)
              {
                const auto & current = entry.ambiguous_names.access(k);
                if (current.name == issue.symbol_name
                    and current.namespace_kind
                         == Compiler_Module_Name_Table_Detail::issue_namespace_kind(issue.kind))
                  {
                    duplicate = true;
                    break;
                  }
              }
            if (duplicate)
              continue;

            entry.ambiguous_names.append(
              {issue.symbol_name,
               Compiler_Module_Name_Table_Detail::issue_namespace_kind(issue.kind)});
          }

        table.modules.append(std::move(entry));
      }

    return table;
  }

  /** @brief Renders one deterministic text artifact for cached module names.
   *
   *  @param table Cached table to render.
   *  @return Stable plain-text representation of the per-module name cache.
   */
  inline std::string
  compiler_render_module_name_table(const Compiler_Module_Name_Table & table)
  {
    std::ostringstream out;
    out << "ModuleNameTable\n";
    for (size_t i = 0; i < table.modules.size(); ++i)
      {
        const auto & module = table.modules.access(i);
        out << "  Module(" << module.module_name << ") order=" << module.order_index << '\n';
        for (size_t j = 0; j < module.names.size(); ++j)
          {
            const auto & item = module.names.access(j);
            out << "    Name(" << compiler_module_export_kind_name(item.kind)
                << "(" << item.name << ")) -> " << item.provider_module_name << ' '
                << (item.imported ? "imported" : "local") << '\n';
          }
        for (size_t j = 0; j < module.ambiguous_names.size(); ++j)
          {
            const auto & item = module.ambiguous_names.access(j);
            out << "    Ambiguous(" << compiler_module_export_kind_name(item.namespace_kind)
                << "(" << item.name << "))\n";
          }
      }
    return out.str();
  }
}

#endif
