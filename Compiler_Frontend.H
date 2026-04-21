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

/** @file Compiler_Frontend.H
 *  @brief Reusable interface for language frontends that plug into common drivers.
 *
 *  A frontend implementation is responsible for language-specific concerns such
 *  as parsing, semantic analysis, and lowering into one of the common
 *  intermediate representations. The interface intentionally does not impose a
 *  shared AST: each language can own its own frontend surface while still
 *  participating in the reusable HIR/IR execution and backend pipeline.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_FRONTEND_H
#define COMPILER_FRONTEND_H

#include <string_view>

#include <Compiler_Driver_Contracts.H>
#include <Compiler_IR_Model.H>
#include <Compiler_Module_Metadata.H>
#include <Compiler_Module_Resolver.H>
#include <ah-diagnostics.H>

namespace Aleph
{
  /** @brief Abstract language frontend that can feed reusable drivers. */
  class Compiler_Frontend
  {
  public:
    virtual ~Compiler_Frontend() = default;

    /** @brief Returns a stable user-facing frontend name. */
    [[nodiscard]] virtual const char *
    name() const noexcept = 0;

    /** @brief Clears all frontend-owned state from the previous run. */
    virtual void
    clear() noexcept = 0;

    /** @brief Registers the ordered input sources for the next run.
     *
     *  @param inputs Input sources to register in frontend-specific storage.
     *  @return `true` if the inputs are acceptable.
     */
    virtual bool
    load_sources(const DynArray<Compiler_Driver_Source> & inputs) = 0;

    /** @brief Parses all previously loaded sources. */
    virtual bool
    parse() = 0;

    /** @brief Runs semantic analysis on the parsed program. */
    virtual bool
    analyze() = 0;

    /** @brief Lowers the analyzed program into common HIR. */
    virtual bool
    lower_hir() = 0;

    /** @brief Lowers the analyzed program into common IR.
     *
     *  Implementations may lower from HIR or directly from language-specific
     *  semantic state, as long as the resulting `Compiler_IR_Module` follows
     *  the public IR contracts.
     */
    virtual bool
    lower_ir() = 0;

    /** @brief Returns the diagnostic engine populated by the frontend. */
    [[nodiscard]] virtual Diagnostic_Engine *
    diagnostic_engine() noexcept = 0;

    /** @brief Returns the diagnostic engine populated by the frontend. */
    [[nodiscard]] virtual const Diagnostic_Engine *
    diagnostic_engine() const noexcept = 0;

    /** @brief Returns the optional shared type context for dumps and backends.
     *
     *  Frontends that do not use `Compiler_Types.H` may return `nullptr`.
     */
    [[nodiscard]] virtual const Compiler_Type_Context *
    type_context() const noexcept = 0;

    /** @brief Returns the lowered HIR module from the last run, when available. */
    [[nodiscard]] virtual const Compiler_HIR_Module *
    hir_module() const noexcept = 0;

    /** @brief Returns the lowered IR module from the last run, when available. */
    [[nodiscard]] virtual const Compiler_IR_Module *
    ir_module() const noexcept = 0;

    /** @brief Returns parsed module descriptors for reusable import orchestration.
     *
     *  Frontends that do not model imports may return an empty array.
     */
    [[nodiscard]] virtual const DynArray<Compiler_Module_Descriptor> &
    module_descriptors() const noexcept
    {
      static const DynArray<Compiler_Module_Descriptor> empty;
      return empty;
    }

    /** @brief Returns the dependency-first merge order from the last parse.
     *
     *  Frontends that do not reorder modules may return an empty array.
     */
    [[nodiscard]] virtual const DynArray<size_t> &
    module_merge_order() const noexcept
    {
      static const DynArray<size_t> empty;
      return empty;
    }

    /** @brief Returns reusable metadata for the top-level surface of each module.
     *
     *  Until the platform grows explicit export syntax, frontends typically map
     *  this to all top-level declarations that the module contributes.
     */
    [[nodiscard]] virtual const DynArray<Compiler_Module_Metadata> &
    module_metadata() const noexcept
    {
      static const DynArray<Compiler_Module_Metadata> empty;
      return empty;
    }

    /** @brief Returns all frontend-produced artifacts from the last run. */
    [[nodiscard]] virtual const DynArray<Compiler_Driver_Artifact> &
    artifacts() const noexcept = 0;

    /** @brief Finds one frontend artifact by exact label. */
    [[nodiscard]] virtual const Compiler_Driver_Artifact *
    find_artifact(std::string_view label) const noexcept = 0;

    /** @brief Returns hard frontend-level errors outside the diagnostic engine. */
    [[nodiscard]] virtual const DynArray<std::string> &
    errors() const noexcept = 0;

    /** @brief Returns non-fatal frontend-level warnings. */
    [[nodiscard]] virtual const DynArray<std::string> &
    warnings() const noexcept = 0;
  };
}

#endif
