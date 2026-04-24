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

/** @file Compiler_Line_Frontend.H
 *  @brief Example frontend for a tiny line-oriented language built on reusable contracts.
 *
 *  This header provides a deliberately small second frontend that does not use
 *  the MVP lexer or parser. It exists to validate that the reusable driver,
 *  HIR builder, and IR builder can support another language frontend with its
 *  own surface syntax.
 *
 *  The accepted language is line-based and intentionally tiny:
 *
 *  - `import <source-name>`
 *  - `func <name> <x> <y> = <op> <lhs> <rhs>`
 *  - `let <name> = int <value>`
 *  - `let <name> = call <func> <arg1> <arg2>`
 *
 *  where `<op>` is one of `add`, `sub`, `mul`, or `div`. Function operands may
 *  refer to the two parameter names or to integer literals.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_LINE_FRONTEND_H
#define COMPILER_LINE_FRONTEND_H

#include <charconv>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <Compiler_Frontend.H>
#include <Compiler_HIR_Builder.H>
#include <Compiler_Module_Name_Diagnostics.H>
#include <Compiler_Module_Name_Resolver.H>
#include <Compiler_Module_Semantic_Environment.H>
#include <Compiler_Module_Resolver.H>
#include <Compiler_IR_Builder.H>
#include <Compiler_Symbol_Bindings.H>

namespace Aleph {
/** @brief Small example frontend for a line-oriented language.
 *
 *  The frontend is intentionally conservative and only supports integer
 *  values plus fixed-arity binary functions. Its main purpose is to show how
 *  a completely different frontend can plug into `Compiler_Generic_Driver`
 *  without depending on the MVP parser or typed AST.
 */
class Compiler_Line_Frontend final : public Compiler_Frontend
{
  enum class Operand_Kind
  {
    Integer,
    Variable
  };

  struct Operand
  {
    Operand_Kind kind = Operand_Kind::Integer;
    std::string text;
    long long integer_value = 0;
    Source_Span span;
  };

  struct Function_Definition
  {
    std::string module_name;
    std::string name;
    std::string left_param;
    std::string right_param;
    Compiler_Operator_Kind op = Compiler_Operator_Kind::Invalid;
    Operand lhs;
    Operand rhs;
    Source_Span span;
    Source_Span name_span;
  };

  struct Let_Definition
  {
    enum class Kind
    {
      Integer,
      Call
    };

    std::string module_name;
    Kind kind = Kind::Integer;
    std::string name;
    long long integer_value = 0;
    std::string callee;
    Operand arg1;
    Operand arg2;
    Source_Span span;
    Source_Span name_span;
  };

  struct Parsed_Source
  {
    std::string name;
    Source_File_Id file_id = 0;
    DynArray<Compiler_Module_Dependency> imports;
    DynArray<Function_Definition> functions;
    DynArray<Let_Definition> lets;
  };

  Source_Manager sources;
  Diagnostic_Engine diagnostics;
  Compiler_Type_Context types;
  Compiler_HIR_Context hir_ctx;
  Compiler_IR_Context ir_ctx;
  size_t arena_bytes = 0;

  DynArray<std::string> source_names;
  DynArray<Source_File_Id> source_ids;
  DynArray<Compiler_Module_Descriptor> module_descriptors_;
  DynArray<Compiler_Module_Metadata> module_metadata_;
  DynArray<Parsed_Source> parsed_sources_;
  DynArray<Function_Definition> functions_;
  DynArray<Let_Definition> lets_;
  DynArray<size_t> merge_order_;
  DynArray<Compiler_Driver_Artifact> artifacts_;
  DynArray<std::string> errors_;
  DynArray<std::string> warnings_;

  Compiler_HIR_Module *hir_module_ = nullptr;
  Compiler_IR_Module *ir_module_ = nullptr;

  static std::string trim_copy(std::string_view text)
  {
    size_t begin = 0;
    while (begin < text.size() and std::isspace(static_cast<unsigned char>(text[begin])))
      ++begin;

    size_t end = text.size();
    while (end > begin and std::isspace(static_cast<unsigned char>(text[end - 1])))
      --end;

    return std::string(text.substr(begin, end - begin));
  }

  static std::string trim_quotes_copy(std::string_view text)
  {
    if (text.size() >= 2 and text.front() == '"' and text.back() == '"')
      return std::string(text.substr(1, text.size() - 2));
    return std::string(text);
  }

  static DynArray<std::string> split_tokens(std::string_view text)
  {
    DynArray<std::string> tokens;
    std::istringstream in{std::string(text)};
    std::string token;
    while (in >> token)
      tokens.append(token);
    return tokens;
  }

  static bool parse_integer(std::string_view text, long long &value) noexcept
  {
    if (text.empty())
      return false;

    const char *const begin = text.data();
    const char *const finish = begin + text.size();
    const auto result = std::from_chars(begin, finish, value);
    return result.ec == std::errc{} and result.ptr == finish;
  }

  static bool is_identifier_start(const unsigned char ch) noexcept
  {
    return std::isalpha(ch) or ch == '_';
  }

  static bool is_identifier_continue(const unsigned char ch) noexcept
  {
    return std::isalnum(ch) or ch == '_';
  }

  static bool parse_identifier(std::string_view text) noexcept
  {
    if (text.empty() or not is_identifier_start(static_cast<unsigned char>(text.front())))
      return false;

    for (size_t i = 1; i < text.size(); ++i)
      if (not is_identifier_continue(static_cast<unsigned char>(text[i])))
        return false;

    return true;
  }

  static Compiler_Operator_Kind parse_operator_name(std::string_view text) noexcept
  {
    if (text == "add")
      return Compiler_Operator_Kind::Plus;
    if (text == "sub")
      return Compiler_Operator_Kind::Minus;
    if (text == "mul")
      return Compiler_Operator_Kind::Star;
    if (text == "div")
      return Compiler_Operator_Kind::Slash;
    return Compiler_Operator_Kind::Invalid;
  }

  void append_error(const std::string &text)
  {
    errors_.append(text);
  }

  void record_text_artifact(const std::string &label, std::string text)
  {
    Compiler_Driver_Detail::append_artifact(artifacts_, label, std::move(text));
  }

  void emit_error(const Source_Span &span, const std::string &code, const std::string &message)
  {
    diagnostics.error(span, message).code(code).emit();
  }

  size_t find_source_index(std::string_view name) const noexcept
  {
    for (size_t i = 0; i < source_names.size(); ++i)
      if (source_names.access(i) == name)
        return i;
    return static_cast<size_t>(-1);
  }

  static bool parse_operand(std::string_view text, const Source_Span &span, Operand &out)
  {
    if (long long value = 0; parse_integer(text, value))
      {
        out.kind = Operand_Kind::Integer;
        out.text = std::string(text);
        out.integer_value = value;
        out.span = span;
        return true;
      }

    if (not parse_identifier(text))
      return false;

    out.kind = Operand_Kind::Variable;
    out.text = std::string(text);
    out.integer_value = 0;
    out.span = span;
    return true;
  }

  bool parse_import_line(Parsed_Source &parsed,
                         const DynArray<std::string> &tokens,
                         const Source_Span &line_span)
  {
    if (tokens.size() != 2)
      {
        emit_error(line_span, "LIN016", "expected 'import <source-name>'");
        return false;
      }

    parsed.imports.append({trim_quotes_copy(tokens.access(1)), line_span});
    return true;
  }

  bool parse_function_line(Parsed_Source &parsed,
                           const DynArray<std::string> &tokens,
                           const Source_Span &line_span)
  {
    if (tokens.size() != 8 or tokens.access(4) != "=")
      {
        emit_error(line_span, "LIN001", "expected 'func <name> <x> <y> = <op> <lhs> <rhs>'");
        return false;
      }

    Function_Definition def;
    def.module_name = parsed.name;
    def.name = tokens.access(1);
    def.left_param = tokens.access(2);
    def.right_param = tokens.access(3);
    def.op = parse_operator_name(tokens.access(5));
    def.span = line_span;
    def.name_span = line_span;

    if (def.op == Compiler_Operator_Kind::Invalid)
      {
        emit_error(line_span, "LIN002", "unknown line-frontend operator '" + tokens.access(5) + "'");
        return false;
      }

    if (not parse_operand(tokens.access(6), line_span, def.lhs))
      {
        emit_error(line_span, "LIN018", "invalid function operand '" + tokens.access(6) + "'");
        return false;
      }

    if (not parse_operand(tokens.access(7), line_span, def.rhs))
      {
        emit_error(line_span, "LIN018", "invalid function operand '" + tokens.access(7) + "'");
        return false;
      }

    parsed.functions.append(std::move(def));
    return true;
  }

  bool parse_let_line(Parsed_Source &parsed,
                      const DynArray<std::string> &tokens,
                      const Source_Span &line_span)
  {
    if (tokens.size() < 5 or tokens.access(2) != "=")
      {
        emit_error(line_span,
                   "LIN003",
                   "expected 'let <name> = int <value>' or 'let <name> = call <func> <a> <b>'");
        return false;
      }

    Let_Definition def;
    def.module_name = parsed.name;
    def.name = tokens.access(1);
    def.span = line_span;
    def.name_span = line_span;

    if (tokens.access(3) == "int")
      {
        if (tokens.size() != 5)
          {
            emit_error(line_span, "LIN004", "expected 'let <name> = int <value>'");
            return false;
          }

        long long value = 0;
        if (not parse_integer(tokens.access(4), value))
          {
            emit_error(line_span, "LIN005", "invalid integer literal '" + tokens.access(4) + "'");
            return false;
          }

        def.kind = Let_Definition::Kind::Integer;
        def.integer_value = value;
        parsed.lets.append(std::move(def));
        return true;
      }

    if (tokens.access(3) == "call")
      {
        if (tokens.size() != 7)
          {
            emit_error(line_span, "LIN006", "expected 'let <name> = call <func> <a> <b>'");
            return false;
          }

        def.kind = Let_Definition::Kind::Call;
        def.callee = tokens.access(4);
        if (not parse_operand(tokens.access(5), line_span, def.arg1))
          {
            emit_error(line_span, "LIN019", "invalid call operand '" + tokens.access(5) + "'");
            return false;
          }

        if (not parse_operand(tokens.access(6), line_span, def.arg2))
          {
            emit_error(line_span, "LIN019", "invalid call operand '" + tokens.access(6) + "'");
            return false;
          }

        parsed.lets.append(std::move(def));
        return true;
      }

    emit_error(line_span, "LIN007", "unknown let initializer kind '" + tokens.access(3) + "'");
    return false;
  }

  bool parse_source_file(Parsed_Source &parsed)
  {
    const auto &text = sources.file_text(parsed.file_id);
    size_t line_begin = 0;
    while (line_begin <= text.size())
      {
        const auto newline = text.find('\n', line_begin);
        const auto raw_end = newline == std::string::npos ? text.size() : newline;
        size_t content_end = raw_end;
        if (content_end > line_begin and text[content_end - 1] == '\r')
          --content_end;

        std::string_view line(text.data() + line_begin, content_end - line_begin);
        if (const auto comment = line.find('#'); comment != std::string_view::npos)
          line = line.substr(0, comment);

        const auto trimmed = trim_copy(line);
        if (not trimmed.empty())
          {
            const Source_Span line_span{parsed.file_id, line_begin, content_end};
            const auto tokens = split_tokens(trimmed);
            if (not tokens.is_empty())
              {
                if (tokens.access(0) == "import")
                  (void) parse_import_line(parsed, tokens, line_span);
                else if (tokens.access(0) == "func")
                  (void) parse_function_line(parsed, tokens, line_span);
                else if (tokens.access(0) == "let")
                  (void) parse_let_line(parsed, tokens, line_span);
                else
                  emit_error(line_span,
                             "LIN008",
                             "unknown line-frontend declaration '" + tokens.access(0) + "'");
              }
          }

        if (newline == std::string::npos)
          break;
        line_begin = newline + 1;
      }

    return not diagnostics.has_errors();
  }

  void flatten_program_in_import_order()
  {
    functions_.clear();
    lets_.clear();

    for (size_t i = 0; i < merge_order_.size(); ++i)
      {
        const auto &parsed = parsed_sources_.access(merge_order_.access(i));
        for (size_t j = 0; j < parsed.functions.size(); ++j)
          functions_.append(parsed.functions.access(j));
        for (size_t j = 0; j < parsed.lets.size(); ++j)
          lets_.append(parsed.lets.access(j));
      }
  }

  void record_import_order_artifact()
  {
    record_text_artifact("imports.order",
                         compiler_render_module_order(merge_order_, module_descriptors_));
  }

  bool resolve_import_order()
  {
    merge_order_.clear();

    const auto result = compiler_resolve_module_order(module_descriptors_);
    for (size_t i = 0; i < result.issues.size(); ++i)
      {
        const auto &issue = result.issues.access(i);
        if (issue.kind == Compiler_Module_Resolution_Issue_Kind::Missing_Dependency)
          emit_error(issue.span,
                     "LIN017",
                     "cannot resolve line-frontend import '" + issue.dependency_name + "'");
        else
          emit_error(issue.span,
                     "LIN018",
                     "cyclic line-frontend import involving '" + issue.module_name + "' and '"
                       + issue.dependency_name + "'");
      }

    for (size_t i = 0; i < result.order.size(); ++i)
      merge_order_.append(result.order.access(i));

    if (not result.valid and merge_order_.is_empty())
      for (size_t i = 0; i < parsed_sources_.size(); ++i)
        merge_order_.append(i);

    return result.valid and not diagnostics.has_errors();
  }

  std::string dump_parsed_program() const
  {
    std::ostringstream out;
    out << "LineProgram\n";
    for (size_t i = 0; i < functions_.size(); ++i)
      {
        const auto &fn = functions_.access(i);
        out << "  func " << fn.name << " " << fn.left_param << " " << fn.right_param << " = "
            << compiler_operator_name(fn.op) << " " << fn.lhs.text << " " << fn.rhs.text << '\n';
      }
    for (size_t i = 0; i < lets_.size(); ++i)
      {
        const auto &let = lets_.access(i);
        out << "  let " << let.name << " = ";
        if (let.kind == Let_Definition::Kind::Integer)
          out << "int " << let.integer_value;
        else
          out << "call " << let.callee << " " << let.arg1.text << " " << let.arg2.text;
        out << '\n';
      }
    return out.str();
  }

  std::string dump_semantic_state() const
  {
    std::ostringstream out;
    out << "LineSema\n";
    out << "  Functions: " << functions_.size() << '\n';
    out << "  TopLevelLets: " << lets_.size() << '\n';
    return out.str();
  }

  bool validate_top_level_calls(const Compiler_Module_Name_Table &table,
                                const Compiler_Module_Semantic_Environment &env)
  {
    for (size_t i = 0; i < lets_.size(); ++i)
      {
        const auto &let = lets_.access(i);
        if (let.kind != Let_Definition::Kind::Call)
          continue;

        if (const auto *value = compiler_lookup_module_value(env, let.module_name, let.callee);
            value != nullptr and value->kind == Compiler_Module_Export_Kind::Function)
          {
            (void) validate_top_level_operand(let.arg1, let.span);
            (void) validate_top_level_operand(let.arg2, let.span);
            continue;
          }

        const auto resolution = compiler_resolve_module_name(table,
                                                             let.module_name,
                                                             let.callee,
                                                             Compiler_Module_Name_Filter::Function);
        if (resolution.status != Compiler_Module_Name_Resolution_Status::Resolved)
          (void) compiler_emit_module_name_resolution_diagnostic(
            diagnostics, let.span, resolution, "LIN015", "line-frontend callee");

        (void) validate_top_level_operand(let.arg1, let.span);
        (void) validate_top_level_operand(let.arg2, let.span);
      }

    return not diagnostics.has_errors();
  }

  bool validate_operand_in_function(const Operand &operand,
                                    const Function_Definition &function,
                                    const Compiler_Symbol_Bindings &bindings)
  {
    if (operand.kind == Operand_Kind::Integer)
      return true;

    const auto *symbol_id = bindings.lookup(operand.text);
    if (symbol_id != nullptr)
      return true;

    emit_error(operand.span,
               "LIN009",
               "unknown function-local name '" + operand.text + "' in '" + function.name + "'");
    return false;
  }

  bool validate_top_level_operand(const Operand &operand, const Source_Span &span)
  {
    if (operand.kind == Operand_Kind::Integer)
      return true;

    emit_error(span,
               "LIN010",
               "top-level line frontend currently accepts only integer call arguments");
    return false;
  }

  Compiler_HIR_Expr *build_hir_operand(Compiler_HIR_Builder &builder, const Operand &operand) const
  {
    if (operand.kind == Operand_Kind::Integer)
      return builder.make_integer_constant(operand.integer_value, types.integer_type(), operand.span);
    return builder.make_variable(operand.text, types.integer_type(), operand.span);
  }

  Compiler_IR_Value_Id build_ir_operand(Compiler_IR_Builder &builder,
                                        Compiler_IR_Function &function,
                                        const Compiler_IR_Block_Id block_id,
                                        const Operand &operand,
                                        const Compiler_IR_Local_Slot_Id left_slot,
                                        const Compiler_IR_Local_Slot_Id right_slot,
                                        const Function_Definition &definition) const
  {
    if (operand.kind == Operand_Kind::Integer)
      return builder.emit_constant(function,
                                   block_id,
                                   types.integer_type(),
                                   std::to_string(operand.integer_value),
                                   operand.span);

    if (operand.text == definition.left_param)
      return builder.emit_load_local(function, block_id, left_slot, types.integer_type(), operand.span);
    return builder.emit_load_local(function, block_id, right_slot, types.integer_type(), operand.span);
  }

public:
  /** @brief Builds one line-oriented frontend with reusable HIR/IR storage. */
  explicit Compiler_Line_Frontend(const size_t arena_size = 1 << 20)
    : diagnostics(sources), hir_ctx(arena_size), ir_ctx(arena_size), arena_bytes(arena_size)
  {}

  /** @brief Returns the stable frontend name. */
  [[nodiscard]] const char *name() const noexcept override
  {
    return "line";
  }

  /** @brief Clears all frontend-owned state from the previous run. */
  void clear() noexcept override
  {
    source_names.clear();
    source_ids.clear();
    module_descriptors_.clear();
    module_metadata_.clear();
    parsed_sources_.clear();
    functions_.clear();
    lets_.clear();
    merge_order_.clear();
    artifacts_.clear();
    errors_.clear();
    warnings_.clear();
    hir_module_ = nullptr;
    ir_module_ = nullptr;

    diagnostics.clear();
    types.clear();
    hir_ctx.reset();
    ir_ctx.reset();
    sources = Source_Manager();
  }

  /** @brief Registers ordered in-memory sources for the next run. */
  bool load_sources(const DynArray<Compiler_Driver_Source> &inputs) override
  {
    if (inputs.is_empty())
      {
        append_error("line frontend: no input sources were provided");
        return false;
      }

    for (size_t i = 0; i < inputs.size(); ++i)
      {
        const auto &input = inputs.access(i);
        if (find_source_index(input.name) != static_cast<size_t>(-1))
          {
            append_error("line frontend: duplicate input source name '" + input.name + "'");
            return false;
          }

        source_names.append(input.name);
        source_ids.append(sources.add_virtual_file(input.name, input.text));
      }

    return true;
  }

  /** @brief Parses all registered line-oriented sources. */
  bool parse() override
  {
    functions_.clear();
    lets_.clear();
    module_descriptors_.clear();
    module_metadata_.clear();
    parsed_sources_.clear();
    merge_order_.clear();

    for (size_t i = 0; i < source_ids.size(); ++i)
      {
        Parsed_Source parsed;
        parsed.name = source_names.access(i);
        parsed.file_id = source_ids.access(i);
        (void) parse_source_file(parsed);

        Compiler_Module_Descriptor descriptor;
        descriptor.name = parsed.name;
        for (size_t j = 0; j < parsed.imports.size(); ++j)
          descriptor.imports.append(parsed.imports.access(j));
        module_descriptors_.append(std::move(descriptor));

        Compiler_Module_Metadata metadata;
        metadata.name = parsed.name;
        for (size_t j = 0; j < parsed.functions.size(); ++j)
          metadata.exports.append({Compiler_Module_Export_Kind::Function,
                                   parsed.functions.access(j).name,
                                   parsed.functions.access(j).name_span});
        for (size_t j = 0; j < parsed.lets.size(); ++j)
          metadata.exports.append({Compiler_Module_Export_Kind::Global,
                                   parsed.lets.access(j).name,
                                   parsed.lets.access(j).name_span});
        module_metadata_.append(std::move(metadata));

        parsed_sources_.append(std::move(parsed));
      }

    (void) resolve_import_order();
    if (merge_order_.is_empty())
      for (size_t i = 0; i < parsed_sources_.size(); ++i)
        merge_order_.append(i);

    flatten_program_in_import_order();
    record_import_order_artifact();

    record_text_artifact("line.parse", dump_parsed_program());
    return not diagnostics.has_errors();
  }

  /** @brief Performs minimal semantic validation over parsed declarations. */
  bool analyze() override
  {
    for (size_t i = 0; i < parsed_sources_.size(); ++i)
      {
        const auto &module = parsed_sources_.access(i);
        Compiler_Symbol_Bindings locals;
        locals.enter_scope();

        for (size_t j = 0; j < module.functions.size(); ++j)
          {
            const auto &fn = module.functions.access(j);
            const auto result = locals.declare(Compiler_Symbol_Kind::Function, fn.name, fn.name_span);
            if (result.duplicate_local())
              emit_error(fn.name_span,
                         "LIN011",
                         "duplicate line-frontend function '" + fn.name + "'");
          }

        for (size_t j = 0; j < module.lets.size(); ++j)
          {
            const auto &let = module.lets.access(j);
            const auto result = locals.declare(Compiler_Symbol_Kind::Global, let.name, let.name_span);
            if (result.duplicate_local())
              {
                const auto &existing = locals.symbol(result.symbol_id);
                if (existing.kind == Compiler_Symbol_Kind::Function)
                  emit_error(let.name_span,
                             "LIN014",
                             "top-level let '" + let.name + "' conflicts with a function name");
                else
                  emit_error(let.name_span,
                             "LIN013",
                             "duplicate line-frontend top-level let '" + let.name + "'");
              }
          }
      }

    const auto linkage
      = compiler_link_module_surfaces(module_descriptors_, module_metadata_, merge_order_);
    const auto bindings = compiler_bind_module_surfaces(linkage);
    const auto table = compiler_build_module_name_table(bindings);
    const auto env = compiler_build_module_semantic_environment(table);
    (void) validate_top_level_calls(table, env);

    for (size_t i = 0; i < functions_.size(); ++i)
      {
        const auto &fn = functions_.access(i);
        Compiler_Symbol_Bindings locals;
        locals.enter_scope();
        (void) locals.declare(Compiler_Symbol_Kind::Parameter, fn.left_param, fn.span);
        (void) locals.declare(Compiler_Symbol_Kind::Parameter, fn.right_param, fn.span);
        (void) validate_operand_in_function(fn.lhs, fn, locals);
        (void) validate_operand_in_function(fn.rhs, fn, locals);
      }

    record_text_artifact("line.sema", dump_semantic_state());
    return not diagnostics.has_errors();
  }

  /** @brief Lowers the analyzed line program into reusable HIR. */
  bool lower_hir() override
  {
    Compiler_HIR_Builder builder(hir_ctx);
    hir_module_ = builder.make_module();
    const auto function_type
      = types.make_function_type({types.integer_type(), types.integer_type()}, types.integer_type());

    for (size_t i = 0; i < functions_.size(); ++i)
      {
        const auto &fn = functions_.access(i);
        auto *body = builder.make_block();
        auto *function = builder.make_function(fn.name, function_type, fn.name_span, body, fn.span);
        builder.add_parameter(*function, fn.left_param, types.integer_type(), fn.span);
        builder.add_parameter(*function, fn.right_param, types.integer_type(), fn.span);
        auto *result = builder.make_binary(build_hir_operand(builder, fn.lhs),
                                           fn.op,
                                           build_hir_operand(builder, fn.rhs),
                                           types.integer_type(),
                                           fn.span);
        builder.append_statement(*body, builder.make_return_stmt(result, fn.span));
        builder.append_function(*hir_module_, function);
      }

    for (size_t i = 0; i < lets_.size(); ++i)
      {
        const auto &let = lets_.access(i);
        if (let.kind == Let_Definition::Kind::Integer)
          builder.append_top_level_statement(
            *hir_module_,
            builder.make_let_stmt(
              let.name,
              types.integer_type(),
              let.name_span,
              builder.make_integer_constant(let.integer_value, types.integer_type(), let.span),
              let.span));
        else
          {
            auto *call = builder.make_call(builder.make_variable(let.callee, function_type, let.span),
                                           types.integer_type(),
                                           let.span,
                                           let.span);
            builder.append_argument(*call, build_hir_operand(builder, let.arg1));
            builder.append_argument(*call, build_hir_operand(builder, let.arg2));
            builder.append_top_level_statement(
              *hir_module_,
              builder.make_let_stmt(let.name, types.integer_type(), let.name_span, call, let.span));
          }
      }

    record_text_artifact("hir", compiler_dump_hir_module(hir_module_, types));
    return hir_module_ != nullptr;
  }

  /** @brief Lowers the analyzed line program into reusable IR. */
  bool lower_ir() override
  {
    Compiler_IR_Builder builder(ir_ctx);
    ir_module_ = builder.make_module();

    const auto function_type
      = types.make_function_type({types.integer_type(), types.integer_type()}, types.integer_type());

    for (size_t i = 0; i < lets_.size(); ++i)
      (void) builder.add_global_slot(*ir_module_,
                                     lets_.access(i).name,
                                     types.integer_type(),
                                     lets_.access(i).span);

    DynArray<std::pair<std::string, Compiler_IR_Function_Id>> function_ids;
    for (size_t i = 0; i < functions_.size(); ++i)
      {
        auto *function
          = builder.make_function(functions_.access(i).name, function_type, functions_.access(i).span);
        const auto id = builder.append_function(*ir_module_, function);
        function_ids.append({functions_.access(i).name, id});
      }

    auto find_function_id = [&function_ids](std::string_view name) noexcept
    {
      for (size_t i = 0; i < function_ids.size(); ++i)
        if (function_ids.access(i).first == name)
          return function_ids.access(i).second;
      return compiler_ir_invalid_id();
    };

    for (size_t i = 0; i < functions_.size(); ++i)
      {
        auto &function = *ir_module_->functions.access(i);
        const auto &definition = functions_.access(i);
        const auto left_slot = builder.add_local_slot(function,
                                                      Compiler_IR_Slot_Kind::Parameter,
                                                      definition.left_param,
                                                      types.integer_type(),
                                                      definition.span);
        const auto right_slot = builder.add_local_slot(function,
                                                       Compiler_IR_Slot_Kind::Parameter,
                                                       definition.right_param,
                                                       types.integer_type(),
                                                       definition.span);
        const auto entry = builder.create_block(function, "entry");
        const auto exit = builder.create_block(function, "exit");
        builder.set_entry_block(function, entry);
        builder.set_exit_block(function, exit);

        const auto lhs = build_ir_operand(
          builder, function, entry, definition.lhs, left_slot, right_slot, definition);
        const auto rhs = build_ir_operand(
          builder, function, entry, definition.rhs, left_slot, right_slot, definition);
        const auto result = builder.emit_binary(
          function, entry, definition.op, lhs, rhs, types.integer_type(), definition.span);
        builder.set_return(function, entry, result, definition.span);
        builder.set_exit(function, exit, definition.span);
      }

    if (not lets_.is_empty())
      {
        auto *top = builder.make_function("<top-level>");
        builder.set_top_level(*ir_module_, top);
        const auto entry = builder.create_block(*top, "entry");
        const auto exit = builder.create_block(*top, "exit");
        builder.set_entry_block(*top, entry);
        builder.set_exit_block(*top, exit);

        for (size_t i = 0; i < lets_.size(); ++i)
          {
            const auto &let = lets_.access(i);
            if (let.kind == Let_Definition::Kind::Integer)
              {
                const auto value = builder.emit_constant(
                  *top, entry, types.integer_type(), std::to_string(let.integer_value), let.span);
                builder.emit_store_global(*top, entry, i, value, types.integer_type(), let.span);
                continue;
              }

            DynArray<Compiler_IR_Value_Id> arguments;
            arguments.append(builder.emit_constant(*top,
                                                   entry,
                                                   types.integer_type(),
                                                   std::to_string(let.arg1.integer_value),
                                                   let.arg1.span));
            arguments.append(builder.emit_constant(*top,
                                                   entry,
                                                   types.integer_type(),
                                                   std::to_string(let.arg2.integer_value),
                                                   let.arg2.span));
            const auto callee = builder.emit_function_ref(
              *top, entry, find_function_id(let.callee), function_type, let.span);
            const auto value
              = builder.emit_call(*top, entry, callee, arguments, types.integer_type(), let.span);
            builder.emit_store_global(*top, entry, i, value, types.integer_type(), let.span);
          }

        builder.set_jump(*top, entry, exit);
        builder.set_exit(*top, exit);
      }

    record_text_artifact("ir", compiler_dump_ir_module(ir_module_, &types));
    if (ir_module_ == nullptr)
      {
        append_error("ir: null IR module produced by line frontend");
        return false;
      }

    const auto report = validate_ir_module(*ir_module_);
    return Compiler_Driver_Detail::merge_report("ir", report, errors_, warnings_);
  }

  /** @brief Returns the diagnostic engine populated by the line frontend. */
  [[nodiscard]] Diagnostic_Engine *diagnostic_engine() noexcept override
  {
    return &diagnostics;
  }

  /** @brief Returns the diagnostic engine populated by the line frontend. */
  [[nodiscard]] const Diagnostic_Engine *diagnostic_engine() const noexcept override
  {
    return &diagnostics;
  }

  /** @brief Returns the shared type context used by the line frontend. */
  [[nodiscard]] const Compiler_Type_Context *type_context() const noexcept override
  {
    return &types;
  }

  /** @brief Returns the lowered HIR module from the last run, when available. */
  [[nodiscard]] const Compiler_HIR_Module *hir_module() const noexcept override
  {
    return hir_module_;
  }

  /** @brief Returns the lowered IR module from the last run, when available. */
  [[nodiscard]] const Compiler_IR_Module *ir_module() const noexcept override
  {
    return ir_module_;
  }

  [[nodiscard]] const DynArray<Compiler_Module_Descriptor> &module_descriptors() const noexcept override
  {
    return module_descriptors_;
  }

  [[nodiscard]] const DynArray<size_t> &module_merge_order() const noexcept override
  {
    return merge_order_;
  }

  [[nodiscard]] const DynArray<Compiler_Module_Metadata> &module_metadata() const noexcept override
  {
    return module_metadata_;
  }

  /** @brief Returns all frontend-produced artifacts from the last run. */
  [[nodiscard]] const DynArray<Compiler_Driver_Artifact> &artifacts() const noexcept override
  {
    return artifacts_;
  }

  /** @brief Finds one frontend artifact by exact label. */
  [[nodiscard]] const Compiler_Driver_Artifact *find_artifact(std::string_view label) const noexcept override
  {
    for (size_t i = artifacts_.size(); i > 0; --i)
      if (artifacts_.access(i - 1).label == label)
        return &artifacts_.access(i - 1);
    return nullptr;
  }

  /** @brief Returns hard frontend-level errors outside the diagnostic engine. */
  [[nodiscard]] const DynArray<std::string> &errors() const noexcept override
  {
    return errors_;
  }

  /** @brief Returns non-fatal frontend-level warnings. */
  [[nodiscard]] const DynArray<std::string> &warnings() const noexcept override
  {
    return warnings_;
  }
};
}  // namespace Aleph

#endif
