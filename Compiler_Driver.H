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

/** @file Compiler_Driver.H
 *  @brief End-to-end multi-file driver for the compiler-support pipeline.
 *
 *  This header stitches together the reusable compiler components already
 *  present in Aleph-w:
 *
 *  - parsing from one or more source files
 *  - typed semantic analysis
 *  - lowering to HIR and explicit IR
 *  - optional lowering to bytecode
 *  - execution through the HIR interpreter runtime
 *  - emission of portable C through `Compiler_Backend_C.H`
 *
 *  The driver does not introduce a CLI by itself. Instead, it provides a
 *  reusable orchestration layer that higher-level tools can call to implement
 *  commands such as `parse-only`, `sema-only`, `hir`, `ir`, `run`, `emit-c`,
 *  and `emit-bytecode`.
 *
 *  Multi-file support is still intentionally conservative:
 *
 *  - each input file is parsed independently
 *  - top-level `import "name";` declarations are resolved by exact source name
 *  - the resulting modules are merged in dependency order
 *  - top-level functions and statements still share one merged namespace
 *  - per-node spans still point back to the original input file
 *
 *  This provides a reusable import-aware orchestration layer without yet
 *  claiming a full module system with namespaces or exports.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_DRIVER_H
#define COMPILER_DRIVER_H

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <Bytecode.H>
#include <Compiler_Backend_C.H>
#include <Compiler_Parser.H>
#include <Interpreter_Runtime.H>
#include <tpl_dynArray.H>

namespace Aleph
{
  /** @brief High-level actions supported by the compiler driver. */
  enum class Compiler_Driver_Action
  {
    Parse_Only,    ///< Stop after parsing and publish AST artifacts.
    Sema_Only,     ///< Stop after typed semantic analysis.
    HIR,           ///< Stop after lowering to typed HIR.
    IR,            ///< Stop after lowering to explicit IR.
    Run,           ///< Execute the merged HIR module with `Interpreter_Runtime`.
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

  /** @brief Parses a canonical driver action spelling.
   *
   *  Accepted spellings include `parse-only`, `sema-only`, `hir`, `ir`,
   *  `run`, `emit-c`, and `emit-bytecode`. A few short aliases such as
   *  `parse`, `sema`, and `bytecode` are also accepted for convenience.
   *
   *  @param text User-facing action spelling.
   *  @param action Output action updated on success.
   *  @return `true` when `text` could be recognized.
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

  /** @brief One logical input source handled by the driver.
   *
   *  The driver preserves the order of these sources as the stable tie-breaker
   *  when multiple modules are otherwise independent. Import resolution matches
   *  `import "name";` declarations against these exact names.
   */
  struct Compiler_Driver_Source
  {
    std::string name;  ///< User-facing file name used in diagnostics and artifacts.
    std::string text;  ///< Full source text.
  };

  /** @brief Stable text artifact produced by one driver run. */
  struct Compiler_Driver_Artifact
  {
    std::string label;  ///< Logical artifact identifier such as `hir` or `run.globals`.
    std::string text;   ///< Deterministic plain-text artifact payload.
  };

  /** @brief Structured output of a `run` action.
   *
   *  The current driver executes HIR through `Interpreter_Runtime.H`. The raw
   *  runtime result is preserved, while `value_text` and `globals_text` provide
   *  immediately printable, deterministic summaries.
   */
  struct Compiler_Driver_Run_Output
  {
    bool executed = false;  ///< Whether the current driver action actually executed code.
    Interpreter_Execution_Result result;  ///< Raw HIR-runtime result.
    std::string value_text;  ///< Stable rendering of the produced value.
    std::string globals_text;  ///< Stable dump of the final globals environment.

    /** @brief Returns whether execution succeeded or never ran. */
    [[nodiscard]] bool
    ok() const noexcept
    {
      return not executed or result.ok();
    }
  };

  /** @brief Immutable configuration shared by all runs of one driver object.
   *
   *  Construction-time configuration is deliberate: several underlying
   *  components keep internal references to shared contexts, so keeping the
   *  configuration stable makes the orchestration API much easier to reason
   *  about.
   */
  struct Compiler_Driver_Options
  {
    Compiler_Parser_Options parser_options;  ///< Parser configuration used for every parsed file.
    Compiler_Typed_Semantic_Options typed_semantic_options;  ///< Typed semantic options shared by the driver.
    Interpreter_Runtime_Options runtime_options;  ///< Runtime limits used by `run`.
    Compiler_C_Backend_Options c_backend_options;  ///< Backend-C emission options used by `emit-c`.
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

  /** @brief Reusable orchestration object for compiler pipeline experiments.
   *
   *  A driver object owns all contexts required by the current pipeline stages
   *  so that pointers returned by getters remain valid until `clear()` or
   *  destruction. The object can be reused across multiple runs; host functions
   *  bound into the embedded `Interpreter_Runtime` prelude remain installed.
   */
  class Compiler_Driver
  {
    Compiler_Driver_Options config;
    Source_Manager sources;
    Diagnostic_Engine diagnostics;
    Compiler_Ast_Context ast_ctx;
    Compiler_HIR_Context hir_ctx;
    Compiler_IR_Context ir_ctx;
    Compiler_Bytecode_Context bytecode_ctx;
    Compiler_Typed_Semantic_Analyzer typed;
    Interpreter_Runtime runtime;
    size_t arena_bytes = 0;

    DynArray<std::string> source_names;
    DynArray<Source_File_Id> source_file_ids;
    DynArray<Compiler_Module *> parsed_modules;
    DynArray<size_t> merge_order_;
    DynArray<Compiler_Driver_Artifact> artifacts_;
    DynArray<std::string> errors_;
    DynArray<std::string> warnings_;

    Compiler_Module * merged_ast_ = nullptr;
    Compiler_HIR_Module * hir_module_ = nullptr;
    Compiler_IR_Module * ir_module_ = nullptr;
    Compiler_Bytecode_Module * bytecode_module_ = nullptr;
    Compiler_C_Backend_Emission c_emission_;
    Compiler_Driver_Run_Output run_output_;
    Compiler_Driver_Action last_action_ = Compiler_Driver_Action::Parse_Only;

    void
    append_error(const std::string & text)
    {
      errors_.append(text);
    }

    void
    append_warning(const std::string & text)
    {
      warnings_.append(text);
    }

    void
    emit_driver_diagnostic(const Source_Span & span,
                           const std::string & code,
                           const std::string & message,
                           const std::string & note = "",
                           const std::string & help = "")
    {
      auto builder = diagnostics.error(span, message).code(code);
      if (not note.empty())
        builder.note(note);
      if (not help.empty())
        builder.help(help);
      builder.emit();
    }

    void
    record_text_artifact(const std::string & label,
                         std::string text)
    {
      Compiler_Driver_Detail::append_artifact(artifacts_, label, std::move(text));
    }

    size_t
    find_source_index(std::string_view name) const noexcept
    {
      for (size_t i = 0; i < source_names.size(); ++i)
        if (source_names.access(i) == name)
          return i;
      return static_cast<size_t>(-1);
    }

    void
    record_import_order_artifact()
    {
      std::ostringstream out;
      out << "ImportOrder\n";
      for (size_t i = 0; i < merge_order_.size(); ++i)
        {
          const auto index = merge_order_.access(i);
          out << "  [" << i << "] " << source_names.access(index) << '\n';
        }
      record_text_artifact("imports.order", out.str());
    }

    void
    record_ast_artifacts()
    {
      for (size_t i = 0; i < parsed_modules.size(); ++i)
        record_text_artifact(
          Compiler_Driver_Detail::artifact_label("ast", i, source_names.access(i)),
          compiler_dump_module(parsed_modules.access(i)));

      record_import_order_artifact();
      record_text_artifact("ast", compiler_dump_module(merged_ast_));
    }

    bool
    resolve_import_order_visit(const size_t index,
                               DynArray<unsigned char> & states)
    {
      const auto state = states.access(index);
      if (state == 2)
        return true;
      if (state == 1)
        return false;

      states.access(index) = 1;
      const auto * module = parsed_modules.access(index);
      if (module != nullptr)
        for (size_t i = 0; i < module->imports.size(); ++i)
          {
            const auto * decl = module->imports.access(i);
            if (decl == nullptr or decl->kind != Compiler_Import_Decl_Kind::Import)
              continue;

            const auto dep_index = find_source_index(decl->module_name);
            if (dep_index == static_cast<size_t>(-1))
              {
                emit_driver_diagnostic(decl->module_name_span,
                                       "DRV001",
                                       "cannot resolve import '" + decl->module_name + "'",
                                       "known sources must match import names exactly");
                continue;
              }

            if (dep_index == index)
              continue;

            if (states.access(dep_index) == 1)
              {
                emit_driver_diagnostic(decl->module_name_span,
                                       "DRV002",
                                       "cyclic import involving '" + source_names.access(index)
                                       + "' and '" + decl->module_name + "'",
                                       "imports currently form one acyclic source-order graph");
                return false;
              }

            if (states.access(dep_index) == 0
                and not resolve_import_order_visit(dep_index, states))
              return false;
          }

      states.access(index) = 2;
      merge_order_.append(index);
      return true;
    }

    bool
    resolve_import_order()
    {
      merge_order_.clear();
      DynArray<unsigned char> states;
      for (size_t i = 0; i < parsed_modules.size(); ++i)
        states.append(0);

      for (size_t i = 0; i < parsed_modules.size(); ++i)
        if (states.access(i) == 0 and not resolve_import_order_visit(i, states))
          return false;

      return not diagnostics.has_errors();
    }

    void
    merge_modules_from_order()
    {
      merged_ast_ = ast_ctx.make<Compiler_Module>();
      merged_ast_->source_name = "<merged>";

      Source_Span first_span;
      Source_Span last_span;

      for (size_t i = 0; i < merge_order_.size(); ++i)
        {
          const auto module_index = merge_order_.access(i);
          const auto * module = parsed_modules.access(module_index);
          if (module == nullptr)
            continue;

          if (not first_span.is_valid() and module->span.is_valid())
            first_span = module->span;
          if (module->span.is_valid())
            last_span = module->span;

          for (size_t j = 0; j < module->type_declarations.size(); ++j)
            merged_ast_->type_declarations.append(module->type_declarations.access(j));
          for (size_t j = 0; j < module->functions.size(); ++j)
            merged_ast_->functions.append(module->functions.access(j));
          for (size_t j = 0; j < module->statements.size(); ++j)
            merged_ast_->statements.append(module->statements.access(j));
        }

      if (first_span.is_valid())
        merged_ast_->span = compiler_merge_spans(first_span, last_span);
    }

    void
    record_diagnostics_artifact()
    {
      std::ostringstream out;
      diagnostics.render_plain(out);
      record_text_artifact("diagnostics", out.str());
    }

    void
    record_driver_messages_artifact()
    {
      record_text_artifact("driver.messages",
                           Compiler_Driver_Detail::render_messages(errors_, warnings_));
    }

    bool
    parse_registered_sources()
    {
      parsed_modules.clear();

      for (size_t i = 0; i < source_file_ids.size(); ++i)
        {
          Compiler_Parser parser(ast_ctx,
                                 sources,
                                 source_file_ids.access(i),
                                 &diagnostics,
                                 config.parser_options);
          parsed_modules.append(parser.parse_module());
        }

      if (not resolve_import_order())
        {
          if (merge_order_.is_empty())
            for (size_t i = 0; i < parsed_modules.size(); ++i)
              merge_order_.append(i);
        }

      merge_modules_from_order();

      record_ast_artifacts();
      return not diagnostics.has_errors();
    }

    bool
    run_typed_semantics()
    {
      typed.analyze_module(merged_ast_);
      record_text_artifact("typed-sema", typed.dump_inference());
      return not diagnostics.has_errors();
    }

    bool
    lower_hir()
    {
      Compiler_HIR_Lowering lowering(hir_ctx, typed);
      hir_module_ = lowering.lower_module(merged_ast_);
      record_text_artifact("hir", compiler_dump_hir_module(hir_module_, typed.type_context()));
      return hir_module_ != nullptr;
    }

    bool
    lower_ir()
    {
      Compiler_IR_Lowering lowering(ir_ctx);
      ir_module_ = lowering.lower_module(hir_module_);
      record_text_artifact("ir", compiler_dump_ir_module(ir_module_, &typed.type_context()));

      if (ir_module_ == nullptr)
        {
          append_error("ir: null IR module produced by lowering");
          return false;
        }

      const auto report = validate_ir_module(*ir_module_);
      return Compiler_Driver_Detail::merge_report("ir", report, errors_, warnings_);
    }

    bool
    lower_bytecode()
    {
      Compiler_Bytecode_Lowering lowering(bytecode_ctx);
      bytecode_module_ = lowering.lower_module(ir_module_);
      record_text_artifact("bytecode",
                           compiler_dump_bytecode_module(bytecode_module_,
                                                         &typed.type_context()));

      if (bytecode_module_ == nullptr)
        {
          append_error("bytecode: null bytecode module produced by lowering");
          return false;
        }

      const auto report = validate_bytecode_module(*bytecode_module_);
      return Compiler_Driver_Detail::merge_report("bytecode", report, errors_, warnings_);
    }

    bool
    emit_c()
    {
      c_emission_ = compiler_emit_c_module(ir_module_, config.c_backend_options);
      record_text_artifact("emit-c", c_emission_.source);
      return Compiler_Driver_Detail::merge_report("emit-c", c_emission_, errors_, warnings_);
    }

    bool
    execute_hir_runtime()
    {
      run_output_ = {};
      run_output_.executed = true;
      run_output_.result = runtime.evaluate_module(hir_module_);
      if (run_output_.result.ok())
        run_output_.value_text = interpreter_value_to_string(run_output_.result.value);
      else
        run_output_.value_text = "";
      run_output_.globals_text = interpreter_dump_globals(runtime);

      record_text_artifact("run.result",
                           Compiler_Driver_Detail::render_run_result(run_output_));
      record_text_artifact("run.globals", run_output_.globals_text);
      return run_output_.result.ok();
    }

    bool
    finalize(const Compiler_Driver_Action action)
    {
      last_action_ = action;
      record_driver_messages_artifact();
      record_diagnostics_artifact();
      return ok();
    }

    bool
    prepare_inputs(const DynArray<Compiler_Driver_Source> & inputs)
    {
      if (inputs.is_empty())
        {
          append_error("driver: no input sources were provided");
          return false;
        }

      for (size_t i = 0; i < inputs.size(); ++i)
        {
          const auto & input = inputs.access(i);
          if (find_source_index(input.name) != static_cast<size_t>(-1))
            {
              append_error("driver: duplicate input source name '" + input.name + "'");
              return false;
            }
          source_names.append(input.name);
          source_file_ids.append(sources.add_virtual_file(input.name, input.text));
        }

      return true;
    }

  public:
    /** @brief Constructs a driver with one stable set of pipeline options.
     *
     *  @param options Shared parser/sema/runtime/backend configuration.
     *  @param arena_size Arena capacity used by AST/HIR/IR/bytecode contexts.
     */
    explicit Compiler_Driver(const Compiler_Driver_Options & options = {},
                             const size_t arena_size = 1 << 20)
      : config(options),
        diagnostics(sources),
        ast_ctx(arena_size),
        hir_ctx(arena_size),
        ir_ctx(arena_size),
        bytecode_ctx(arena_size),
        typed(&diagnostics, options.typed_semantic_options),
        runtime(&diagnostics, options.runtime_options),
        arena_bytes(arena_size)
    {}

    Compiler_Driver(const Compiler_Driver &) = delete;
    Compiler_Driver & operator=(const Compiler_Driver &) = delete;

    /** @brief Returns the immutable construction-time configuration. */
    [[nodiscard]] const Compiler_Driver_Options &
    options() const noexcept
    {
      return config;
    }

    /** @brief Returns the arena size shared by the internal contexts. */
    [[nodiscard]] size_t
    arena_size() const noexcept
    {
      return arena_bytes;
    }

    /** @brief Clears all stage outputs while preserving host prelude bindings. */
    void
    clear() noexcept
    {
      parsed_modules.clear();
      merge_order_.clear();
      source_names.clear();
      source_file_ids.clear();
      artifacts_.clear();
      errors_.clear();
      warnings_.clear();

      merged_ast_ = nullptr;
      hir_module_ = nullptr;
      ir_module_ = nullptr;
      bytecode_module_ = nullptr;
      c_emission_ = {};
      run_output_ = {};
      last_action_ = Compiler_Driver_Action::Parse_Only;

      runtime.reset();
      runtime.set_diagnostics(&diagnostics);
      typed.clear();
      diagnostics.clear();
      ast_ctx.reset();
      hir_ctx.reset();
      ir_ctx.reset();
      bytecode_ctx.reset();
      sources = Source_Manager();
    }

    /** @brief Binds one host function into the persistent runtime prelude. */
    bool
    bind_host_function(const std::string & name,
                       const Interpreter_Host_Function callback,
                       void * user_data = nullptr)
    {
      return runtime.bind_host_function(name, callback, user_data);
    }

    /** @brief Runs the pipeline over in-memory sources.
     *
     *  @param inputs Ordered source units to parse and merge.
     *  @param action Final stage requested by the caller.
     *  @return `true` when the requested action finished without diagnostics or
     *          driver-level hard errors.
     */
    bool
    execute_sources(const DynArray<Compiler_Driver_Source> & inputs,
                    const Compiler_Driver_Action action = Compiler_Driver_Action::Run)
    {
      clear();
      if (not prepare_inputs(inputs))
        return finalize(action);
      if (not parse_registered_sources())
        return finalize(action);
      if (action == Compiler_Driver_Action::Parse_Only)
        return finalize(action);

      if (not run_typed_semantics())
        return finalize(action);
      if (action == Compiler_Driver_Action::Sema_Only)
        return finalize(action);

      if (not lower_hir())
        {
          append_error("hir: failed to lower merged AST to HIR");
          return finalize(action);
        }
      if (action == Compiler_Driver_Action::HIR)
        return finalize(action);

      if (action == Compiler_Driver_Action::Run)
        {
          (void) execute_hir_runtime();
          return finalize(action);
        }

      if (not lower_ir())
        return finalize(action);
      if (action == Compiler_Driver_Action::IR)
        return finalize(action);

      switch (action)
        {
        case Compiler_Driver_Action::Emit_Bytecode:
          (void) lower_bytecode();
          return finalize(action);

        case Compiler_Driver_Action::Emit_C:
          (void) emit_c();
          return finalize(action);

        case Compiler_Driver_Action::Parse_Only:
        case Compiler_Driver_Action::Sema_Only:
        case Compiler_Driver_Action::HIR:
        case Compiler_Driver_Action::IR:
        case Compiler_Driver_Action::Run:
          return finalize(action);
        }

      append_error("driver: reached unreachable action dispatch");
      return finalize(action);
    }

    /** @brief Runs the pipeline over source files loaded from disk.
     *
     *  @param paths Ordered file paths to register in the `Source_Manager`.
     *  @param action Final stage requested by the caller.
     *  @return `true` when the requested action finished successfully.
     */
    bool
    execute_files(const DynArray<std::string> & paths,
                  const Compiler_Driver_Action action = Compiler_Driver_Action::Run)
    {
      clear();
      if (paths.is_empty())
        {
          append_error("driver: no input file paths were provided");
          return finalize(action);
        }

      for (size_t i = 0; i < paths.size(); ++i)
        {
          const auto & path = paths.access(i);
          if (find_source_index(path) != static_cast<size_t>(-1))
            {
              append_error("driver: duplicate input file path '" + path + "'");
              return finalize(action);
            }
          source_names.append(path);
          source_file_ids.append(sources.load_file(path));
        }

      if (not parse_registered_sources())
        return finalize(action);
      if (action == Compiler_Driver_Action::Parse_Only)
        return finalize(action);

      if (not run_typed_semantics())
        return finalize(action);
      if (action == Compiler_Driver_Action::Sema_Only)
        return finalize(action);

      if (not lower_hir())
        {
          append_error("hir: failed to lower merged AST to HIR");
          return finalize(action);
        }
      if (action == Compiler_Driver_Action::HIR)
        return finalize(action);

      if (action == Compiler_Driver_Action::Run)
        {
          (void) execute_hir_runtime();
          return finalize(action);
        }

      if (not lower_ir())
        return finalize(action);
      if (action == Compiler_Driver_Action::IR)
        return finalize(action);

      if (action == Compiler_Driver_Action::Emit_Bytecode)
        {
          (void) lower_bytecode();
          return finalize(action);
        }

      if (action == Compiler_Driver_Action::Emit_C)
        {
          (void) emit_c();
          return finalize(action);
        }

      append_error("driver: reached unreachable file-action dispatch");
      return finalize(action);
    }

    /** @brief Convenience overload for a single in-memory source. */
    bool
    execute_source(const std::string & name,
                   const std::string & text,
                   const Compiler_Driver_Action action = Compiler_Driver_Action::Run)
    {
      DynArray<Compiler_Driver_Source> inputs;
      inputs.append({name, text});
      return execute_sources(inputs, action);
    }

    /** @brief Returns whether the last driver action finished successfully. */
    [[nodiscard]] bool
    ok() const noexcept
    {
      return errors_.is_empty() and not diagnostics.has_errors();
    }

    /** @brief Returns the last requested action. */
    [[nodiscard]] Compiler_Driver_Action
    last_action() const noexcept
    {
      return last_action_;
    }

    /** @brief Returns the shared source manager for the last run. */
    [[nodiscard]] const Source_Manager &
    source_manager() const noexcept
    {
      return sources;
    }

    /** @brief Returns the diagnostic engine populated by the last run. */
    [[nodiscard]] const Diagnostic_Engine &
    diagnostic_engine() const noexcept
    {
      return diagnostics;
    }

    /** @brief Returns the typed semantic analyzer state from the last run. */
    [[nodiscard]] const Compiler_Typed_Semantic_Analyzer &
    typed_analysis() const noexcept
    {
      return typed;
    }

    /** @brief Returns the parsed per-file AST modules from the last run. */
    [[nodiscard]] const DynArray<Compiler_Module *> &
    parsed_ast_modules() const noexcept
    {
      return parsed_modules;
    }

    /** @brief Returns the merged AST module from the last run. */
    [[nodiscard]] const Compiler_Module *
    merged_ast_module() const noexcept
    {
      return merged_ast_;
    }

    /** @brief Returns the lowered HIR module from the last run, when available. */
    [[nodiscard]] const Compiler_HIR_Module *
    hir_module() const noexcept
    {
      return hir_module_;
    }

    /** @brief Returns the lowered IR module from the last run, when available. */
    [[nodiscard]] const Compiler_IR_Module *
    ir_module() const noexcept
    {
      return ir_module_;
    }

    /** @brief Returns the lowered bytecode module from the last run, when available. */
    [[nodiscard]] const Compiler_Bytecode_Module *
    bytecode_module() const noexcept
    {
      return bytecode_module_;
    }

    /** @brief Returns the C emission payload from the last `emit-c` action. */
    [[nodiscard]] const Compiler_C_Backend_Emission &
    c_emission() const noexcept
    {
      return c_emission_;
    }

    /** @brief Returns the run summary from the last `run` action. */
    [[nodiscard]] const Compiler_Driver_Run_Output &
    run_output() const noexcept
    {
      return run_output_;
    }

    /** @brief Returns non-diagnostic driver-level hard errors. */
    [[nodiscard]] const DynArray<std::string> &
    errors() const noexcept
    {
      return errors_;
    }

    /** @brief Returns non-fatal driver-level warnings. */
    [[nodiscard]] const DynArray<std::string> &
    warnings() const noexcept
    {
      return warnings_;
    }

    /** @brief Returns all recorded artifacts from the last run. */
    [[nodiscard]] const DynArray<Compiler_Driver_Artifact> &
    artifacts() const noexcept
    {
      return artifacts_;
    }

    /** @brief Finds one artifact by label.
     *
     *  @param label Exact artifact label to look up.
     *  @return Matching artifact pointer, or `nullptr` if absent.
     */
    [[nodiscard]] const Compiler_Driver_Artifact *
    find_artifact(std::string_view label) const noexcept
    {
      for (size_t i = artifacts_.size(); i > 0; --i)
        if (artifacts_.access(i - 1).label == label)
          return &artifacts_.access(i - 1);
      return nullptr;
    }

    /** @brief Renders diagnostics from the last run as plain text. */
    [[nodiscard]] std::string
    diagnostics_text() const
    {
      std::ostringstream out;
      diagnostics.render_plain(out);
      return out.str();
    }
  };
}

#endif
