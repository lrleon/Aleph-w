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

/** @file Compiler_MVP_Frontend.H
 *  @brief Reference frontend adapter that exposes the existing MVP language through `Compiler_Frontend`.
 *
 *  This adapter keeps the current parser, semantic analyzer, and lowering
 *  pipeline intact while making it consumable through the reusable frontend
 *  interface. It is meant both as the default frontend for Aleph's compiler
 *  experiments and as an example of how another language frontend can plug into
 *  the common HIR/IR-based pipeline.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_MVP_FRONTEND_H
#define COMPILER_MVP_FRONTEND_H

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <Compiler_Frontend.H>
#include <Compiler_HIR_Lowering_MVP.H>
#include <Compiler_IR_Lowering_MVP.H>
#include <Compiler_Module_Resolver.H>
#include <Compiler_Parser.H>

namespace Aleph
{
  /** @brief Options used by the MVP frontend adapter. */
  struct Compiler_MVP_Frontend_Options
  {
    Compiler_Parser_Options parser_options;  ///< Parser configuration used for every parsed file.
    Compiler_Typed_Semantic_Options typed_semantic_options;  ///< Typed semantic options shared by the frontend.
  };

  /** @brief `Compiler_Frontend` implementation backed by the current MVP language stack. */
  class Compiler_MVP_Frontend final : public Compiler_Frontend
  {
    Compiler_MVP_Frontend_Options config;
    Source_Manager sources;
    Diagnostic_Engine diagnostics;
    Compiler_Ast_Context ast_ctx;
    Compiler_HIR_Context hir_ctx;
    Compiler_IR_Context ir_ctx;
    Compiler_Typed_Semantic_Analyzer typed;
    size_t arena_bytes = 0;

    DynArray<std::string> source_names;
    DynArray<Source_File_Id> source_file_ids;
    DynArray<Compiler_Module_Descriptor> module_descriptors_;
    DynArray<Compiler_Module_Metadata> module_metadata_;
    DynArray<Compiler_Module *> parsed_modules;
    DynArray<size_t> merge_order_;
    DynArray<Compiler_Driver_Artifact> artifacts_;
    DynArray<std::string> errors_;
    DynArray<std::string> warnings_;

    Compiler_Module * merged_ast_ = nullptr;
    Compiler_HIR_Module * hir_module_ = nullptr;
    Compiler_IR_Module * ir_module_ = nullptr;

    void
    append_error(const std::string & text)
    {
      errors_.append(text);
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
      record_text_artifact("imports.order",
                           compiler_render_module_order(merge_order_, module_descriptors_));
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
    resolve_import_order()
    {
      merge_order_.clear();
      const auto result = compiler_resolve_module_order(module_descriptors_);
      for (size_t i = 0; i < result.issues.size(); ++i)
        {
          const auto & issue = result.issues.access(i);
          if (issue.kind == Compiler_Module_Resolution_Issue_Kind::Missing_Dependency)
            emit_driver_diagnostic(issue.span,
                                   "DRV001",
                                   "cannot resolve import '" + issue.dependency_name + "'",
                                   "known sources must match import names exactly");
          else
            emit_driver_diagnostic(issue.span,
                                   "DRV002",
                                   "cyclic import involving '" + issue.module_name + "' and '"
                                     + issue.dependency_name + "'",
                                   "imports currently form one acyclic source-order graph");
        }

      for (size_t i = 0; i < result.order.size(); ++i)
        merge_order_.append(result.order.access(i));

      return result.valid and not diagnostics.has_errors();
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

  public:
    /** @brief Constructs the reference MVP frontend.
     *
     *  @param options Parser and typed semantic configuration.
     *  @param arena_size Arena capacity used by AST/HIR/IR contexts.
     */
    explicit Compiler_MVP_Frontend(const Compiler_MVP_Frontend_Options & options = {},
                                   const size_t arena_size = 1 << 20)
      : config(options),
        diagnostics(sources),
        ast_ctx(arena_size),
        hir_ctx(arena_size),
        ir_ctx(arena_size),
        typed(&diagnostics, options.typed_semantic_options),
        arena_bytes(arena_size)
    {}

    /** @brief Returns the stable frontend name. */
    [[nodiscard]] const char *
    name() const noexcept override
    {
      return "mvp";
    }

    /** @brief Returns the configured shared arena size. */
    [[nodiscard]] size_t
    arena_size() const noexcept
    {
      return arena_bytes;
    }

    /** @brief Returns the immutable frontend configuration. */
    [[nodiscard]] const Compiler_MVP_Frontend_Options &
    options() const noexcept
    {
      return config;
    }

    void
    clear() noexcept override
    {
      parsed_modules.clear();
      module_descriptors_.clear();
      module_metadata_.clear();
      merge_order_.clear();
      source_names.clear();
      source_file_ids.clear();
      artifacts_.clear();
      errors_.clear();
      warnings_.clear();

      merged_ast_ = nullptr;
      hir_module_ = nullptr;
      ir_module_ = nullptr;

      typed.clear();
      diagnostics.clear();
      ast_ctx.reset();
      hir_ctx.reset();
      ir_ctx.reset();
      sources = Source_Manager();
    }

    bool
    load_sources(const DynArray<Compiler_Driver_Source> & inputs) override
    {
      if (inputs.is_empty())
        {
          append_error("frontend: no input sources were provided");
          return false;
        }

      for (size_t i = 0; i < inputs.size(); ++i)
        {
          const auto & input = inputs.access(i);
          if (find_source_index(input.name) != static_cast<size_t>(-1))
            {
              append_error("frontend: duplicate input source name '" + input.name + "'");
              return false;
            }
          source_names.append(input.name);
          source_file_ids.append(sources.add_virtual_file(input.name, input.text));
        }

      return true;
    }

    bool
    parse() override
    {
      parsed_modules.clear();
      module_descriptors_.clear();
      module_metadata_.clear();

      for (size_t i = 0; i < source_file_ids.size(); ++i)
        {
          Compiler_Parser parser(ast_ctx,
                                 sources,
                                 source_file_ids.access(i),
                                 &diagnostics,
                                 config.parser_options);
          auto * module = parser.parse_module();
          parsed_modules.append(module);

          Compiler_Module_Descriptor descriptor;
          descriptor.name = source_names.access(i);
          Compiler_Module_Metadata metadata;
          metadata.name = source_names.access(i);
          if (module != nullptr)
            metadata.span = module->span;
          if (module != nullptr)
            for (size_t j = 0; j < module->imports.size(); ++j)
              {
                const auto * decl = module->imports.access(j);
                if (decl == nullptr or decl->kind != Compiler_Import_Decl_Kind::Import)
                  continue;
                descriptor.imports.append({decl->module_name, decl->module_name_span});
              }

          if (module != nullptr)
            {
              for (size_t j = 0; j < module->type_declarations.size(); ++j)
                {
                  const auto * decl = module->type_declarations.access(j);
                  if (decl == nullptr)
                    continue;
                  metadata.exports.append(
                    {Compiler_Module_Export_Kind::Type, decl->name, decl->name_span});
                }

              for (size_t j = 0; j < module->functions.size(); ++j)
                {
                  const auto * fn = module->functions.access(j);
                  if (fn == nullptr)
                    continue;
                  metadata.exports.append(
                    {Compiler_Module_Export_Kind::Function, fn->name, fn->name_span});
                }

              for (size_t j = 0; j < module->statements.size(); ++j)
                {
                  const auto * stmt = module->statements.access(j);
                  if (stmt == nullptr or stmt->kind != Compiler_Stmt_Kind::Let)
                    continue;
                  const auto * let_stmt = static_cast<const Compiler_Let_Stmt *>(stmt);
                  metadata.exports.append(
                    {Compiler_Module_Export_Kind::Global, let_stmt->name, let_stmt->name_span});
                }
            }

          module_descriptors_.append(std::move(descriptor));
          module_metadata_.append(std::move(metadata));
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
    analyze() override
    {
      typed.analyze_module(merged_ast_);
      record_text_artifact("typed-sema", typed.dump_inference());
      return not diagnostics.has_errors();
    }

    bool
    lower_hir() override
    {
      Compiler_HIR_Lowering lowering(hir_ctx, typed);
      hir_module_ = lowering.lower_module(merged_ast_);
      record_text_artifact("hir", compiler_dump_hir_module(hir_module_, typed.type_context()));
      return hir_module_ != nullptr;
    }

    bool
    lower_ir() override
    {
      Compiler_IR_Lowering lowering(ir_ctx, &typed.type_context());
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

    [[nodiscard]] Diagnostic_Engine *
    diagnostic_engine() noexcept override
    {
      return &diagnostics;
    }

    [[nodiscard]] const Diagnostic_Engine *
    diagnostic_engine() const noexcept override
    {
      return &diagnostics;
    }

    [[nodiscard]] const Compiler_Type_Context *
    type_context() const noexcept override
    {
      return &typed.type_context();
    }

    [[nodiscard]] const Compiler_Module *
    merged_ast_module() const noexcept
    {
      return merged_ast_;
    }

    [[nodiscard]] const DynArray<Compiler_Module *> &
    parsed_ast_modules() const noexcept
    {
      return parsed_modules;
    }

    [[nodiscard]] const Compiler_HIR_Module *
    hir_module() const noexcept override
    {
      return hir_module_;
    }

    [[nodiscard]] const Compiler_IR_Module *
    ir_module() const noexcept override
    {
      return ir_module_;
    }

    [[nodiscard]] const DynArray<Compiler_Module_Descriptor> &
    module_descriptors() const noexcept override
    {
      return module_descriptors_;
    }

    [[nodiscard]] const DynArray<size_t> &
    module_merge_order() const noexcept override
    {
      return merge_order_;
    }

    [[nodiscard]] const DynArray<Compiler_Module_Metadata> &
    module_metadata() const noexcept override
    {
      return module_metadata_;
    }

    [[nodiscard]] const DynArray<Compiler_Driver_Artifact> &
    artifacts() const noexcept override
    {
      return artifacts_;
    }

    [[nodiscard]] const Compiler_Driver_Artifact *
    find_artifact(std::string_view label) const noexcept override
    {
      for (size_t i = artifacts_.size(); i > 0; --i)
        if (artifacts_.access(i - 1).label == label)
          return &artifacts_.access(i - 1);
      return nullptr;
    }

    [[nodiscard]] const DynArray<std::string> &
    errors() const noexcept override
    {
      return errors_;
    }

    [[nodiscard]] const DynArray<std::string> &
    warnings() const noexcept override
    {
      return warnings_;
    }

    /** @brief Renders accumulated diagnostics as plain text. */
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
