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

/** @file Compiler_Module_Metadata.H
 *  @brief Reusable metadata for parsed or analyzed module surfaces.
 *
 *  This header models the user-visible top-level surface of one module so
 *  frontends, drivers, and external tools can inspect which declarations a
 *  source unit contributes without depending on one specific AST.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_MODULE_METADATA_H
#define COMPILER_MODULE_METADATA_H

#include <sstream>
#include <string>

#include <ah-source.H>
#include <tpl_dynArray.H>

namespace Aleph
{
  /** @brief Export kinds currently modeled by reusable module metadata. */
  enum class Compiler_Module_Export_Kind
  {
    Function,
    Global,
    Type
  };

  /** @brief Returns the stable debug name for one module-export kind. */
  inline const char *
  compiler_module_export_kind_name(const Compiler_Module_Export_Kind kind) noexcept
  {
    switch (kind)
      {
      case Compiler_Module_Export_Kind::Function:
        return "Function";
      case Compiler_Module_Export_Kind::Global:
        return "Global";
      case Compiler_Module_Export_Kind::Type:
        return "Type";
      }

    return "Unknown";
  }

  /** @brief One reusable top-level declaration exposed by a module surface. */
  struct Compiler_Module_Export
  {
    Compiler_Module_Export_Kind kind = Compiler_Module_Export_Kind::Global;
    std::string name;
    Source_Span declaration_span;
  };

  /** @brief Reusable metadata describing one parsed or analyzed module surface. */
  struct Compiler_Module_Metadata
  {
    std::string name;  ///< Stable module/source name.
    Source_Span span;  ///< Full module span when available.
    DynArray<Compiler_Module_Export> exports;  ///< Visible top-level declarations contributed by the module.
  };

  /** @brief Renders one deterministic text artifact for module metadata. */
  inline std::string
  compiler_render_module_metadata(const DynArray<Compiler_Module_Metadata> & modules)
  {
    std::ostringstream out;
    out << "ModuleMetadata\n";
    for (size_t i = 0; i < modules.size(); ++i)
      {
        const auto & module = modules.access(i);
        out << "  Module(" << module.name << ")";
        if (module.span.is_valid())
          out << " @" << module.span.to_string();
        out << '\n';

        for (size_t j = 0; j < module.exports.size(); ++j)
          {
            const auto & item = module.exports.access(j);
            out << "    " << compiler_module_export_kind_name(item.kind) << "(" << item.name
                << ")";
            if (item.declaration_span.is_valid())
              out << " @" << item.declaration_span.to_string();
            out << '\n';
          }
      }
    return out.str();
  }
}

#endif
