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

/** @file Compiler_Generic_Driver.H
 *  @brief Reusable pipeline driver that operates on any `Compiler_Frontend`.
 *
 *  The generic driver owns only the reusable post-frontend stages:
 *
 *  - execution through `Interpreter_Runtime.H`
 *  - lowering from IR to bytecode
 *  - validation of bytecode artifacts
 *  - emission of portable C from IR
 *
 *  Language-specific concerns such as parsing, semantic analysis, and lowering
 *  into HIR/IR are delegated to a `Compiler_Frontend` implementation.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_GENERIC_DRIVER_H
#define COMPILER_GENERIC_DRIVER_H

#include <fstream>
#include <sstream>
#include <string>

#include <Bytecode.H>
#include <Compiler_Backend_C.H>
#include <Compiler_Frontend.H>
#include <Compiler_Module_Binder.H>
#include <Compiler_Module_Linker.H>
#include <Compiler_Module_Name_Table.H>
#include <Compiler_Module_Semantic_Environment.H>

namespace Aleph
{
  /** @brief Configuration shared by all runs of `Compiler_Generic_Driver`. */
  struct Compiler_Generic_Driver_Options
  {
    Interpreter_Runtime_Options runtime_options;  ///< Runtime limits used by `run`.
    Compiler_C_Backend_Options c_backend_options;  ///< Backend-C emission options used by `emit-c`.
  };

  /** @brief Reusable orchestration object built on top of `Compiler_Frontend`. */
  class Compiler_Generic_Driver
  {
    Compiler_Frontend * frontend = nullptr;
    Compiler_Generic_Driver_Options config;
    Compiler_Bytecode_Context bytecode_ctx;
    Interpreter_Runtime runtime;
    size_t arena_bytes = 0;

    DynArray<Compiler_Driver_Artifact> artifacts_;
    DynArray<std::string> errors_;
    DynArray<std::string> warnings_;

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
    record_text_artifact(const std::string & label,
                         std::string text)
    {
      Compiler_Driver_Detail::append_artifact(artifacts_, label, std::move(text));
    }

    void
    collect_frontend_state()
    {
      const auto & frontend_errors = frontend->errors();
      for (size_t i = 0; i < frontend_errors.size(); ++i)
        errors_.append(frontend_errors.access(i));

      const auto & frontend_warnings = frontend->warnings();
      for (size_t i = 0; i < frontend_warnings.size(); ++i)
        warnings_.append(frontend_warnings.access(i));

      const auto & frontend_artifacts = frontend->artifacts();
      for (size_t i = 0; i < frontend_artifacts.size(); ++i)
        artifacts_.append(frontend_artifacts.access(i));
    }

    void
    record_diagnostics_artifact()
    {
      const auto * dx = frontend->diagnostic_engine();
      if (dx == nullptr)
        return;

      std::ostringstream out;
      dx->render_plain(out);
      record_text_artifact("diagnostics", out.str());
    }

    void
    record_import_order_artifact()
    {
      if (frontend->find_artifact("imports.order") != nullptr)
        return;

      const auto & modules = frontend->module_descriptors();
      const auto & order = frontend->module_merge_order();
      if (modules.is_empty() or order.is_empty())
        return;

      record_text_artifact("imports.order", compiler_render_module_order(order, modules));
    }

    void
    record_module_metadata_artifact()
    {
      if (frontend->find_artifact("modules.surface") != nullptr)
        return;

      const auto & metadata = frontend->module_metadata();
      if (metadata.is_empty())
        return;

      record_text_artifact("modules.surface", compiler_render_module_metadata(metadata));
    }

    void
    record_module_linkage_artifact()
    {
      if (frontend->find_artifact("modules.linkage") != nullptr)
        return;

      const auto & descriptors = frontend->module_descriptors();
      const auto & metadata = frontend->module_metadata();
      const auto & order = frontend->module_merge_order();
      if (descriptors.is_empty() or metadata.is_empty() or order.is_empty())
        return;

      const auto linkage = compiler_link_module_surfaces(descriptors, metadata, order);
      record_text_artifact("modules.linkage", compiler_render_module_linkage(linkage));
    }

    void
    record_module_bindings_artifact()
    {
      if (frontend->find_artifact("modules.binding") != nullptr)
        return;

      const auto & descriptors = frontend->module_descriptors();
      const auto & metadata = frontend->module_metadata();
      const auto & order = frontend->module_merge_order();
      if (descriptors.is_empty() or metadata.is_empty() or order.is_empty())
        return;

      const auto linkage = compiler_link_module_surfaces(descriptors, metadata, order);
      const auto bindings = compiler_bind_module_surfaces(linkage);
      record_text_artifact("modules.binding", compiler_render_module_bindings(bindings));
    }

    void
    record_module_name_table_artifact()
    {
      if (frontend->find_artifact("modules.names") != nullptr)
        return;

      const auto & descriptors = frontend->module_descriptors();
      const auto & metadata = frontend->module_metadata();
      const auto & order = frontend->module_merge_order();
      if (descriptors.is_empty() or metadata.is_empty() or order.is_empty())
        return;

      const auto linkage = compiler_link_module_surfaces(descriptors, metadata, order);
      const auto bindings = compiler_bind_module_surfaces(linkage);
      const auto table = compiler_build_module_name_table(bindings);
      record_text_artifact("modules.names", compiler_render_module_name_table(table));
    }

    void
    record_module_semantic_environment_artifact()
    {
      if (frontend->find_artifact("modules.semantic") != nullptr)
        return;

      const auto & descriptors = frontend->module_descriptors();
      const auto & metadata = frontend->module_metadata();
      const auto & order = frontend->module_merge_order();
      if (descriptors.is_empty() or metadata.is_empty() or order.is_empty())
        return;

      const auto linkage = compiler_link_module_surfaces(descriptors, metadata, order);
      const auto bindings = compiler_bind_module_surfaces(linkage);
      const auto table = compiler_build_module_name_table(bindings);
      const auto env = compiler_build_module_semantic_environment(table);
      record_text_artifact("modules.semantic", compiler_render_module_semantic_environment(env));
    }

    void
    record_driver_messages_artifact()
    {
      record_text_artifact("driver.messages",
                           Compiler_Driver_Detail::render_messages(errors_, warnings_));
    }

    bool
    lower_bytecode()
    {
      Compiler_Bytecode_Lowering lowering(bytecode_ctx);
      bytecode_module_ = lowering.lower_module(frontend->ir_module());
      record_text_artifact("bytecode",
                           compiler_dump_bytecode_module(bytecode_module_,
                                                         frontend->type_context()));

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
      c_emission_ = compiler_emit_c_module(frontend->ir_module(), config.c_backend_options);
      record_text_artifact("emit-c", c_emission_.source);
      return Compiler_Driver_Detail::merge_report("emit-c", c_emission_, errors_, warnings_);
    }

    bool
    execute_hir_runtime()
    {
      run_output_ = {};
      run_output_.executed = true;
      run_output_.result = runtime.evaluate_module(frontend->hir_module());
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
      collect_frontend_state();
      record_import_order_artifact();
      record_module_metadata_artifact();
      record_module_linkage_artifact();
      record_module_bindings_artifact();
      record_module_name_table_artifact();
      record_module_semantic_environment_artifact();
      record_driver_messages_artifact();
      record_diagnostics_artifact();
      return ok();
    }

    static bool
    read_file_text(const std::string & path,
                   std::string & out_text)
    {
      std::ifstream in(path);
      if (not in)
        return false;

      std::ostringstream out;
      out << in.rdbuf();
      out_text = out.str();
      return true;
    }

  public:
    /** @brief Builds one generic driver over a concrete frontend implementation.
     *
     *  @param frontend_impl Frontend used for parse/sema/HIR/IR stages.
     *  @param options Runtime and backend configuration.
     *  @param arena_size Arena size used by bytecode lowering.
     */
    explicit Compiler_Generic_Driver(Compiler_Frontend & frontend_impl,
                                     const Compiler_Generic_Driver_Options & options = {},
                                     const size_t arena_size = 1 << 20)
      : frontend(&frontend_impl),
        config(options),
        bytecode_ctx(arena_size),
        runtime(frontend_impl.diagnostic_engine(), options.runtime_options),
        arena_bytes(arena_size)
    {}

    Compiler_Generic_Driver(const Compiler_Generic_Driver &) = delete;
    Compiler_Generic_Driver & operator=(const Compiler_Generic_Driver &) = delete;

    /** @brief Returns the immutable driver configuration. */
    [[nodiscard]] const Compiler_Generic_Driver_Options &
    options() const noexcept
    {
      return config;
    }

    /** @brief Returns the configured bytecode arena size. */
    [[nodiscard]] size_t
    arena_size() const noexcept
    {
      return arena_bytes;
    }

    /** @brief Clears all driver and frontend state while preserving host bindings. */
    void
    clear() noexcept
    {
      artifacts_.clear();
      errors_.clear();
      warnings_.clear();
      bytecode_module_ = nullptr;
      c_emission_ = {};
      run_output_ = {};
      last_action_ = Compiler_Driver_Action::Parse_Only;

      frontend->clear();
      runtime.reset();
      runtime.set_diagnostics(frontend->diagnostic_engine());
      bytecode_ctx.reset();
    }

    /** @brief Binds one host function into the persistent runtime prelude. */
    bool
    bind_host_function(const std::string & name,
                       const Interpreter_Host_Function callback,
                       void * user_data = nullptr)
    {
      return runtime.bind_host_function(name, callback, user_data);
    }

    /** @brief Runs the pipeline over in-memory sources. */
    bool
    execute_sources(const DynArray<Compiler_Driver_Source> & inputs,
                    const Compiler_Driver_Action action = Compiler_Driver_Action::Run)
    {
      clear();
      if (not frontend->load_sources(inputs))
        return finalize(action);
      if (not frontend->parse())
        return finalize(action);
      if (action == Compiler_Driver_Action::Parse_Only)
        return finalize(action);

      if (not frontend->analyze())
        return finalize(action);
      if (action == Compiler_Driver_Action::Sema_Only)
        return finalize(action);

      if (not frontend->lower_hir())
        {
          append_error("hir: failed to lower frontend program to HIR");
          return finalize(action);
        }
      if (action == Compiler_Driver_Action::HIR)
        return finalize(action);

      if (action == Compiler_Driver_Action::Run)
        {
          (void) execute_hir_runtime();
          return finalize(action);
        }

      if (not frontend->lower_ir())
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

    /** @brief Runs the pipeline over source files loaded from disk. */
    bool
    execute_files(const DynArray<std::string> & paths,
                  const Compiler_Driver_Action action = Compiler_Driver_Action::Run)
    {
      DynArray<Compiler_Driver_Source> inputs;
      for (size_t i = 0; i < paths.size(); ++i)
        {
          std::string text;
          if (not read_file_text(paths.access(i), text))
            {
              clear();
              append_error("driver: unable to read input file '" + paths.access(i) + "'");
              return finalize(action);
            }
          inputs.append({paths.access(i), std::move(text)});
        }
      return execute_sources(inputs, action);
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
      const auto * dx = frontend->diagnostic_engine();
      return errors_.is_empty() and (dx == nullptr or not dx->has_errors());
    }

    /** @brief Returns the frontend used by this driver. */
    [[nodiscard]] const Compiler_Frontend &
    frontend_impl() const noexcept
    {
      return *frontend;
    }

    /** @brief Returns the last requested action. */
    [[nodiscard]] Compiler_Driver_Action
    last_action() const noexcept
    {
      return last_action_;
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

    /** @brief Finds one artifact by exact label. */
    [[nodiscard]] const Compiler_Driver_Artifact *
    find_artifact(std::string_view label) const noexcept
    {
      for (size_t i = artifacts_.size(); i > 0; --i)
        if (artifacts_.access(i - 1).label == label)
          return &artifacts_.access(i - 1);
      return nullptr;
    }

    /** @brief Renders diagnostics from the frontend as plain text. */
    [[nodiscard]] std::string
    diagnostics_text() const
    {
      std::ostringstream out;
      const auto * dx = frontend->diagnostic_engine();
      if (dx != nullptr)
        dx->render_plain(out);
      return out.str();
    }
  };
}

#endif
