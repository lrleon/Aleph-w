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

/** @file Compiler_Module_Name_Diagnostics.H
 *  @brief Reusable diagnostics for top-level inter-module name resolution.
 *
 *  This header translates one `Compiler_Module_Name_Resolution` into a
 *  structured diagnostic with stable wording, notes, and help messages. It is
 *  intended for frontends that reuse the module-surface resolver but still want
 *  frontend-specific error codes and subject names such as "callee" or
 *  "imported type".
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_MODULE_NAME_DIAGNOSTICS_H
#define COMPILER_MODULE_NAME_DIAGNOSTICS_H

#include <string>

#include <Compiler_Module_Name_Resolver.H>
#include <ah-diagnostics.H>

namespace Aleph {
/** @brief Emits one structured diagnostic for a module-name resolution failure.
 *
 *  When `resolution.ok()` is true, this helper emits nothing and returns `0`.
 *
 *  @param diagnostics Diagnostic engine receiving the error.
 *  @param span Source span of the failed use site.
 *  @param resolution Structured module-name resolution result.
 *  @param code Stable frontend-specific diagnostic code.
 *  @param subject User-facing noun phrase such as `"line-frontend callee"`.
 *  @return Diagnostic index, or `0` if no diagnostic was emitted.
 */
inline size_t compiler_emit_module_name_resolution_diagnostic(
  Diagnostic_Engine &diagnostics,
  const Source_Span &span,
  const Compiler_Module_Name_Resolution &resolution,
  const std::string &code,
  const std::string &subject)
{
  if (resolution.ok())
    return 0;

  switch (resolution.status)
    {
    case Compiler_Module_Name_Resolution_Status::Resolved:
      return 0;

    case Compiler_Module_Name_Resolution_Status::Missing_Module:
      return diagnostics
        .error(span,
               "cannot resolve " + subject + " '" + resolution.symbol_name + "' because module '"
                 + resolution.module_name + "' is not available")
        .code(code)
        .note("the current binding view does not contain that module")
        .help("ensure the source was loaded and its imports were resolved before semantic checks")
        .emit();

    case Compiler_Module_Name_Resolution_Status::Missing_Name:
      return diagnostics.error(span, "unknown " + subject + " '" + resolution.symbol_name + "'")
        .code(code)
        .note("module '" + resolution.module_name
              + "' does not expose that name through local declarations or direct imports")
        .help("declare the symbol locally or import a module that exports it")
        .emit();

    case Compiler_Module_Name_Resolution_Status::Wrong_Kind:
      return diagnostics.error(span, "invalid " + subject + " '" + resolution.symbol_name + "'")
        .code(code)
        .note("module '" + resolution.module_name + "' resolves that name as "
              + compiler_module_export_kind_name(resolution.binding.kind))
        .help("use a symbol exported with the expected kind")
        .emit();

    case Compiler_Module_Name_Resolution_Status::Ambiguous_Name:
      return diagnostics.error(span, "ambiguous " + subject + " '" + resolution.symbol_name + "'")
        .code(code)
        .note("module '" + resolution.module_name
              + "' sees multiple matching providers for that name")
        .help("rename one provider or reduce the set of direct imports")
        .emit();
    }

  return 0;
}
}  // namespace Aleph

#endif
