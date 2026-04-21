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

/** @file Compiler_Module_Linker.H
 *  @brief Reusable linking helpers for module surfaces and direct imports.
 *
 *  This header links `Compiler_Module_Descriptor` and `Compiler_Module_Metadata`
 *  into a direct-import view showing which top-level declarations are visible to
 *  each module from itself and its immediate imports. It also reports duplicate
 *  visible names across the current value or type namespace.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_MODULE_LINKER_H
#define COMPILER_MODULE_LINKER_H

#include <sstream>
#include <string>

#include <Compiler_Module_Metadata.H>
#include <Compiler_Module_Resolver.H>

namespace Aleph
{
  /** @brief One visible top-level declaration in a linked module surface. */
  struct Compiler_Module_Visible_Export
  {
    Compiler_Module_Export_Kind kind = Compiler_Module_Export_Kind::Global;
    std::string name;
    size_t source_module_index = static_cast<size_t>(-1);
    std::string source_module_name;
    Source_Span declaration_span;
    bool imported = false;  ///< `false` for local exports, `true` for direct-imported exports.
  };

  /** @brief One linked module view under direct-import surface rules. */
  struct Compiler_Module_Linkage
  {
    std::string module_name;
    size_t module_index = static_cast<size_t>(-1);
    size_t order_index = static_cast<size_t>(-1);
    DynArray<size_t> direct_import_indexes;
    DynArray<Compiler_Module_Visible_Export> visible_exports;
  };

  /** @brief Structural issues emitted while linking module surfaces. */
  enum class Compiler_Module_Linkage_Issue_Kind
  {
    Missing_Metadata,
    Duplicate_Value_Export,
    Duplicate_Type_Export
  };

  /** @brief One structural issue reported by the reusable module linker. */
  struct Compiler_Module_Linkage_Issue
  {
    Compiler_Module_Linkage_Issue_Kind kind
      = Compiler_Module_Linkage_Issue_Kind::Missing_Metadata;
    size_t module_index = static_cast<size_t>(-1);
    std::string module_name;
    std::string export_name;
    std::string first_provider_module;
    std::string second_provider_module;
    Source_Span span;
  };

  /** @brief Link result over module descriptors and module metadata. */
  struct Compiler_Module_Link_Result
  {
    bool valid = true;
    DynArray<Compiler_Module_Linkage> modules;
    DynArray<Compiler_Module_Linkage_Issue> issues;
  };

  /** @brief Returns the stable debug name for one linkage issue kind. */
  inline const char *
  compiler_module_linkage_issue_name(const Compiler_Module_Linkage_Issue_Kind kind) noexcept
  {
    switch (kind)
      {
      case Compiler_Module_Linkage_Issue_Kind::Missing_Metadata:
        return "Missing_Metadata";
      case Compiler_Module_Linkage_Issue_Kind::Duplicate_Value_Export:
        return "Duplicate_Value_Export";
      case Compiler_Module_Linkage_Issue_Kind::Duplicate_Type_Export:
        return "Duplicate_Type_Export";
      }

    return "Unknown";
  }

  namespace Compiler_Module_Linker_Detail
  {
    inline size_t
    find_metadata_index(const DynArray<Compiler_Module_Metadata> & metadata,
                        const std::string & name) noexcept
    {
      for (size_t i = 0; i < metadata.size(); ++i)
        if (metadata.access(i).name == name)
          return i;
      return static_cast<size_t>(-1);
    }

    inline int
    export_namespace_bucket(const Compiler_Module_Export_Kind kind) noexcept
    {
      return kind == Compiler_Module_Export_Kind::Type ? 1 : 0;
    }
  }

  /** @brief Links direct-import module surfaces into visible top-level views. */
  inline Compiler_Module_Link_Result
  compiler_link_module_surfaces(const DynArray<Compiler_Module_Descriptor> & descriptors,
                                const DynArray<Compiler_Module_Metadata> & metadata,
                                const DynArray<size_t> & order)
  {
    Compiler_Module_Link_Result result;

    for (size_t order_pos = 0; order_pos < order.size(); ++order_pos)
      {
        const auto module_index = order.access(order_pos);
        if (module_index >= descriptors.size())
          continue;

        const auto & descriptor = descriptors.access(module_index);
        Compiler_Module_Linkage linkage;
        linkage.module_name = descriptor.name;
        linkage.module_index = module_index;
        linkage.order_index = order_pos;

        const auto self_metadata_index
          = Compiler_Module_Linker_Detail::find_metadata_index(metadata, descriptor.name);
        if (self_metadata_index == static_cast<size_t>(-1))
          {
            result.valid = false;
            result.issues.append({Compiler_Module_Linkage_Issue_Kind::Missing_Metadata,
                                  module_index,
                                  descriptor.name,
                                  "",
                                  "",
                                  descriptor.name,
                                  {}});
            result.modules.append(std::move(linkage));
            continue;
          }

        const auto & self_metadata = metadata.access(self_metadata_index);
        for (size_t i = 0; i < self_metadata.exports.size(); ++i)
          {
            const auto & item = self_metadata.exports.access(i);
            linkage.visible_exports.append(
              {item.kind,
               item.name,
               self_metadata_index,
               self_metadata.name,
               item.declaration_span,
               false});
          }

        for (size_t i = 0; i < descriptor.imports.size(); ++i)
          {
            const auto & dep = descriptor.imports.access(i);
            const auto dep_index
              = Compiler_Module_Resolver_Detail::find_module_index(descriptors, dep.module_name);
            if (dep_index == static_cast<size_t>(-1))
              continue;
            linkage.direct_import_indexes.append(dep_index);

            const auto dep_metadata_index
              = Compiler_Module_Linker_Detail::find_metadata_index(metadata, dep.module_name);
            if (dep_metadata_index == static_cast<size_t>(-1))
              {
                result.valid = false;
                result.issues.append({Compiler_Module_Linkage_Issue_Kind::Missing_Metadata,
                                      module_index,
                                      descriptor.name,
                                      dep.module_name,
                                      "",
                                      dep.module_name,
                                      dep.span});
                continue;
              }

            const auto & dep_metadata = metadata.access(dep_metadata_index);
            for (size_t j = 0; j < dep_metadata.exports.size(); ++j)
              {
                const auto & item = dep_metadata.exports.access(j);
                linkage.visible_exports.append(
                  {item.kind,
                   item.name,
                   dep_metadata_index,
                   dep_metadata.name,
                   item.declaration_span,
                   true});
              }
          }

        for (size_t i = 0; i < linkage.visible_exports.size(); ++i)
          for (size_t j = i + 1; j < linkage.visible_exports.size(); ++j)
            {
              const auto & lhs = linkage.visible_exports.access(i);
              const auto & rhs = linkage.visible_exports.access(j);
              if (lhs.name != rhs.name)
                continue;
              if (Compiler_Module_Linker_Detail::export_namespace_bucket(lhs.kind)
                  != Compiler_Module_Linker_Detail::export_namespace_bucket(rhs.kind))
                continue;
              if (lhs.source_module_name == rhs.source_module_name and lhs.imported == rhs.imported)
                continue;

              result.valid = false;
              result.issues.append({lhs.kind == Compiler_Module_Export_Kind::Type
                                      ? Compiler_Module_Linkage_Issue_Kind::Duplicate_Type_Export
                                      : Compiler_Module_Linkage_Issue_Kind::Duplicate_Value_Export,
                                    module_index,
                                    descriptor.name,
                                    lhs.name,
                                    lhs.source_module_name,
                                    rhs.source_module_name,
                                    rhs.declaration_span});
            }

        result.modules.append(std::move(linkage));
      }

    return result;
  }

  /** @brief Renders one deterministic text artifact for linked module surfaces. */
  inline std::string
  compiler_render_module_linkage(const Compiler_Module_Link_Result & result)
  {
    std::ostringstream out;
    out << "ModuleLinkage\n";
    for (size_t i = 0; i < result.modules.size(); ++i)
      {
        const auto & module = result.modules.access(i);
        out << "  Module(" << module.module_name << ") order=" << module.order_index << '\n';
        for (size_t j = 0; j < module.visible_exports.size(); ++j)
          {
            const auto & item = module.visible_exports.access(j);
            out << "    Visible(" << compiler_module_export_kind_name(item.kind) << "("
                << item.name << ")) from " << item.source_module_name
                << (item.imported ? " imported" : " local") << '\n';
          }
      }

    if (not result.issues.is_empty())
      {
        out << "  Issues\n";
        for (size_t i = 0; i < result.issues.size(); ++i)
          {
            const auto & issue = result.issues.access(i);
            out << "    " << compiler_module_linkage_issue_name(issue.kind) << " module="
                << issue.module_name;
            if (not issue.export_name.empty())
              out << " export=" << issue.export_name;
            if (not issue.first_provider_module.empty())
              out << " first=" << issue.first_provider_module;
            if (not issue.second_provider_module.empty())
              out << " second=" << issue.second_provider_module;
            out << '\n';
          }
      }

    return out.str();
  }
}

#endif
