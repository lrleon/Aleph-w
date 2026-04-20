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

/** @file Compiler_Driver_Contracts.H
 *  @brief Shared contracts used by reusable compiler frontends and drivers.
 *
 *  This header centralizes the user-facing action enum and the stable artifact
 *  and input records consumed by orchestration code. It intentionally avoids
 *  committing to one specific frontend implementation so language-specific
 *  adapters can reuse the same contracts.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_DRIVER_CONTRACTS_H
#define COMPILER_DRIVER_CONTRACTS_H

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <Interpreter_Runtime.H>
#include <tpl_dynArray.H>

namespace Aleph
{
  /** @brief High-level actions supported by compiler pipeline drivers. */
  enum class Compiler_Driver_Action
  {
    Parse_Only,    ///< Stop after parsing and publish AST/frontend artifacts.
    Sema_Only,     ///< Stop after semantic analysis.
    HIR,           ///< Stop after lowering to typed HIR.
    IR,            ///< Stop after lowering to explicit IR.
    Run,           ///< Execute the lowered HIR module with `Interpreter_Runtime`.
    Emit_C,        ///< Lower to IR and emit portable C.
    Emit_Bytecode  ///< Lower to IR and emit reusable bytecode.
  };

  /** @brief Returns the canonical external spelling for one driver action. */
  inline const char *
  compiler_driver_action_name(const Compiler_Driver_Action action) noexcept
  {
    switch (action)
      {
      case Compiler_Driver_Action::Parse_Only:
        return "parse-only";
      case Compiler_Driver_Action::Sema_Only:
        return "sema-only";
      case Compiler_Driver_Action::HIR:
        return "hir";
      case Compiler_Driver_Action::IR:
        return "ir";
      case Compiler_Driver_Action::Run:
        return "run";
      case Compiler_Driver_Action::Emit_C:
        return "emit-c";
      case Compiler_Driver_Action::Emit_Bytecode:
        return "emit-bytecode";
      }

    return "unknown";
  }

  /** @brief Parses one canonical driver action spelling.
   *
   *  Accepted spellings include `parse-only`, `sema-only`, `hir`, `ir`,
   *  `run`, `emit-c`, and `emit-bytecode`. Short aliases such as `parse`,
   *  `sema`, `c`, and `bytecode` are accepted as convenience spellings.
   *
   *  @param text User-facing action spelling.
   *  @param action Output action updated on success.
   *  @return `true` if `text` is recognized.
   */
  inline bool
  compiler_parse_driver_action(std::string_view text,
                               Compiler_Driver_Action & action) noexcept
  {
    if (text == "parse-only" or text == "parse")
      {
        action = Compiler_Driver_Action::Parse_Only;
        return true;
      }
    if (text == "sema-only" or text == "sema")
      {
        action = Compiler_Driver_Action::Sema_Only;
        return true;
      }
    if (text == "hir")
      {
        action = Compiler_Driver_Action::HIR;
        return true;
      }
    if (text == "ir")
      {
        action = Compiler_Driver_Action::IR;
        return true;
      }
    if (text == "run")
      {
        action = Compiler_Driver_Action::Run;
        return true;
      }
    if (text == "emit-c" or text == "c")
      {
        action = Compiler_Driver_Action::Emit_C;
        return true;
      }
    if (text == "emit-bytecode" or text == "bytecode")
      {
        action = Compiler_Driver_Action::Emit_Bytecode;
        return true;
      }

    return false;
  }

  /** @brief One logical input source handled by a compiler frontend. */
  struct Compiler_Driver_Source
  {
    std::string name;  ///< User-facing file name used in diagnostics and artifacts.
    std::string text;  ///< Full source text.
  };

  /** @brief Stable text artifact produced during one pipeline run. */
  struct Compiler_Driver_Artifact
  {
    std::string label;  ///< Logical artifact identifier such as `hir` or `run.globals`.
    std::string text;   ///< Deterministic plain-text artifact payload.
  };

  /** @brief Structured output of a `run` action. */
  struct Compiler_Driver_Run_Output
  {
    bool executed = false;  ///< Whether the current driver action actually executed code.
    Interpreter_Execution_Result result;  ///< Raw runtime result.
    std::string value_text;  ///< Stable rendering of the produced value.
    std::string globals_text;  ///< Stable dump of the final globals environment.

    /** @brief Returns whether execution succeeded or never ran. */
    [[nodiscard]] bool
    ok() const noexcept
    {
      return not executed or result.ok();
    }
  };

  namespace Compiler_Driver_Detail
  {
    inline std::string
    artifact_label(const std::string & prefix,
                   const size_t index,
                   const std::string & name)
    {
      std::ostringstream out;
      out << prefix << '[' << index << "]:" << name;
      return out.str();
    }

    inline void
    append_artifact(DynArray<Compiler_Driver_Artifact> & artifacts,
                    const std::string & label,
                    std::string text)
    {
      artifacts.append({label, std::move(text)});
    }

    template <typename Report>
    inline bool
    merge_report(const std::string & stage,
                 const Report & report,
                 DynArray<std::string> & errors,
                 DynArray<std::string> & warnings)
    {
      for (size_t i = 0; i < report.errors.size(); ++i)
        errors.append(stage + ": " + report.errors.access(i));
      for (size_t i = 0; i < report.warnings.size(); ++i)
        warnings.append(stage + ": " + report.warnings.access(i));
      return report.valid;
    }

    inline std::string
    render_messages(const DynArray<std::string> & errors,
                    const DynArray<std::string> & warnings)
    {
      std::ostringstream out;
      out << "DriverMessages\n";

      if (errors.is_empty() and warnings.is_empty())
        {
          out << "  <none>\n";
          return out.str();
        }

      for (size_t i = 0; i < errors.size(); ++i)
        out << "  error: " << errors.access(i) << '\n';
      for (size_t i = 0; i < warnings.size(); ++i)
        out << "  warning: " << warnings.access(i) << '\n';
      return out.str();
    }

    inline std::string
    render_run_result(const Compiler_Driver_Run_Output & output)
    {
      std::ostringstream out;
      out << "HIRRun\n";
      out << "  Executed: " << (output.executed ? "true" : "false") << '\n';
      if (not output.executed)
        return out.str();

      out << "  Flow: " << interpreter_control_flow_name(output.result.flow) << '\n';
      if (output.result.ok())
        out << "  Value: " << output.value_text << '\n';
      else
        {
          out << "  Error: " << output.result.error.code << " "
              << output.result.error.message << '\n';
          if (output.result.error.span.is_valid())
            out << "  Span: " << output.result.error.span.to_string() << '\n';
        }
      return out.str();
    }
  }
}

#endif
