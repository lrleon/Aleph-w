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

/** @file Compiler_Backend_C.H
 *  @brief Portable C backend that emits standalone C from `Compiler_IR.H`.
 *
 *  This header introduces the first source-emitting backend in the compiler
 *  platform. The goal is not to model final ABI/layout decisions yet, but to
 *  provide a readable, deterministic, and executable lowering target that can
 *  be used for:
 *
 *  - validating the explicit IR end-to-end
 *  - experimenting with `emit-c` workflows before a full driver exists
 *  - generating portable artifacts that can be compiled by a normal C toolchain
 *
 *  The emitted C intentionally stays close to the structure of `Compiler_IR.H`:
 *
 *  - each IR function becomes one C function
 *  - each IR block becomes one C label plus `goto`-based control flow
 *  - each IR value id becomes one local temporary
 *  - each local/global slot remains explicit
 *  - a tiny tagged-value runtime is embedded in the generated translation unit
 *
 *  The current MVP favors clarity and portability over performance. It is
 *  therefore a good bridge between the reusable IR and future lower-level
 *  backends such as LLVM IR or native assembly.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_BACKEND_C_H
#define COMPILER_BACKEND_C_H

#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <Compiler_IR.H>
#include <ah-errors.H>
#include <tpl_dynArray.H>

namespace Aleph
{
  /** @brief Options that control one C backend emission request. */
  struct Compiler_C_Backend_Options
  {
    std::string module_name = "module"; ///< Prefix used to derive stable C identifiers.
    bool emit_main = false; ///< Whether to emit a standalone `main()` when the IR has a top-level body.
    bool emit_comments = true; ///< Whether to annotate the generated C with readable IR-derived comments.
  };

  /** @brief Result of one C backend emission request. */
  struct Compiler_C_Backend_Emission
  {
    bool valid = true; ///< Whether emission completed successfully.
    DynArray<std::string> errors; ///< Hard emission failures.
    DynArray<std::string> warnings; ///< Non-fatal findings.
    std::string source; ///< Final generated C translation unit.

    std::string c_prefix; ///< Shared prefix used for runtime helpers and generated symbols.
    std::string value_type_name; ///< Generated C typedef name for runtime values.
    std::string module_struct_name; ///< Generated C struct name for module state.
    std::string module_init_symbol; ///< Generated C symbol that initializes module globals.
    std::string dump_globals_symbol; ///< Generated C symbol that prints current global state.
    std::string print_value_symbol; ///< Generated C symbol that prints one runtime value.
    std::string top_level_symbol; ///< Generated C symbol for the lowered top-level body, if any.
    DynArray<std::string> function_symbols; ///< Generated C function symbols aligned with `module.functions`.
    DynArray<std::string> function_descriptor_symbols; ///< Generated callable descriptor symbols aligned with `module.functions`.

    /** @brief Returns whether emission finished without hard errors. */
    [[nodiscard]] bool
    ok() const noexcept
    {
      return valid;
    }
  };

  namespace Compiler_Backend_C_Detail
  {
    inline std::string
    sanitize_identifier(std::string_view text)
    {
      std::string result;
      result.reserve(text.size() + 8);
      for (const unsigned char ch : text)
        {
          if (std::isalnum(ch))
            result.push_back(static_cast<char>(std::tolower(ch)));
          else
            result.push_back('_');
        }

      while (not result.empty() and result.front() == '_')
        result.erase(result.begin());
      while (not result.empty() and result.back() == '_')
        result.pop_back();
      while (result.find("__") != std::string::npos)
        result.replace(result.find("__"), 2, "_");

      if (result.empty())
        result = "item";
      if (std::isdigit(static_cast<unsigned char>(result.front())))
        result.insert(result.begin(), '_');
      return result;
    }

    inline std::string
    upper_identifier(std::string_view text)
    {
      std::string result = sanitize_identifier(text);
      for (char & ch : result)
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
      return result;
    }

    inline std::string
    c_string_literal(std::string_view text)
    {
      std::string result;
      result.reserve(text.size() + 2);
      result.push_back('"');
      for (const char ch : text)
        switch (ch)
          {
          case '\\':
            result += "\\\\";
            break;
          case '"':
            result += "\\\"";
            break;
          case '\n':
            result += "\\n";
            break;
          case '\r':
            result += "\\r";
            break;
          case '\t':
            result += "\\t";
            break;
          default:
            result.push_back(ch);
          }
      result.push_back('"');
      return result;
    }

    inline std::string
    safe_comment_text(std::string text)
    {
      while (text.find("*/") != std::string::npos)
        text.replace(text.find("*/"), 2, "* /");
      return text;
    }

    inline bool
    parse_integer_literal(const std::string & text,
                          long long & value) noexcept
    {
      if (text.empty())
        return false;

      char * end = nullptr;
      value = std::strtoll(text.c_str(), &end, 10);
      return end != nullptr and *end == '\0';
    }

    inline size_t
    count_parameters(const Compiler_IR_Function & function) noexcept
    {
      size_t count = 0;
      for (size_t i = 0; i < function.local_slots.size(); ++i)
        if (function.local_slots.access(i).kind == Compiler_IR_Slot_Kind::Parameter)
          ++count;
      return count;
    }

    inline std::string
    token_name(const Compiler_Token_Kind kind) noexcept
    {
      switch (kind)
        {
        case Compiler_Token_Kind::Invalid: return "Invalid";
        case Compiler_Token_Kind::Assign: return "Assign";
        case Compiler_Token_Kind::Plus: return "Plus";
        case Compiler_Token_Kind::Minus: return "Minus";
        case Compiler_Token_Kind::Star: return "Star";
        case Compiler_Token_Kind::Slash: return "Slash";
        case Compiler_Token_Kind::Percent: return "Percent";
        case Compiler_Token_Kind::EqEq: return "EqEq";
        case Compiler_Token_Kind::NotEq: return "NotEq";
        case Compiler_Token_Kind::Less: return "Less";
        case Compiler_Token_Kind::LessEq: return "LessEq";
        case Compiler_Token_Kind::Greater: return "Greater";
        case Compiler_Token_Kind::GreaterEq: return "GreaterEq";
        case Compiler_Token_Kind::AndAnd: return "AndAnd";
        case Compiler_Token_Kind::OrOr: return "OrOr";
        case Compiler_Token_Kind::Bang: return "Bang";
        case Compiler_Token_Kind::Amp: return "Amp";
        case Compiler_Token_Kind::Pipe: return "Pipe";
        case Compiler_Token_Kind::Caret: return "Caret";
        case Compiler_Token_Kind::Tilde: return "Tilde";
        case Compiler_Token_Kind::PlusEq: return "PlusEq";
        case Compiler_Token_Kind::MinusEq: return "MinusEq";
        case Compiler_Token_Kind::StarEq: return "StarEq";
        case Compiler_Token_Kind::SlashEq: return "SlashEq";
        case Compiler_Token_Kind::PercentEq: return "PercentEq";
        case Compiler_Token_Kind::PlusPlus: return "PlusPlus";
        case Compiler_Token_Kind::MinusMinus: return "MinusMinus";
        default:
          break;
        }

      return "Token";
    }

    inline std::string
    value_name(const Compiler_IR_Value_Id id)
    {
      return "v" + std::to_string(id);
    }

    inline std::string
    local_name(const Compiler_IR_Local_Slot_Id id)
    {
      return "s" + std::to_string(id);
    }

    inline std::string
    global_name(const Compiler_IR_Global_Slot_Id id)
    {
      return "g" + std::to_string(id);
    }

    inline std::string
    block_label(const size_t id)
    {
      return "B" + std::to_string(id);
    }

    inline std::string
    indent(const size_t count)
    {
      return std::string(count, ' ');
    }
  }

  /** @brief Emits portable C from one validated `Compiler_IR_Module`. */
  class Compiler_C_Backend
  {
    Compiler_C_Backend_Options options;

    struct Symbol_Names
    {
      std::string prefix;
      std::string upper_prefix;
      std::string tag_enum_name;
      std::string value_type_name;
      std::string function_type_name;
      std::string function_pointer_type_name;
      std::string module_struct_name;
      std::string module_init_symbol;
      std::string dump_globals_symbol;
      std::string print_value_symbol;
      std::string tag_name_symbol;
      std::string panic_symbol;
      std::string require_assigned_symbol;
      std::string require_int_symbol;
      std::string require_bool_symbol;
      std::string require_function_symbol;
      std::string equal_symbol;
      std::string call_symbol;
      std::string make_invalid_symbol;
      std::string make_unit_symbol;
      std::string make_bool_symbol;
      std::string make_int_symbol;
      std::string make_string_symbol;
      std::string make_char_symbol;
      std::string make_function_symbol;
      std::string top_level_symbol;
      std::string tag_invalid_name;
      std::string tag_unit_name;
      std::string tag_bool_name;
      std::string tag_int_name;
      std::string tag_string_name;
      std::string tag_char_name;
      std::string tag_function_name;
    };

    class Emitter
    {
      const Compiler_C_Backend_Options * options = nullptr;
      const Compiler_IR_Module * module = nullptr;
      const Compiler_Type_Context * types = nullptr;
      Compiler_C_Backend_Emission emission;
      Symbol_Names names;

      void
      fail(const std::string & message)
      {
        emission.valid = false;
        emission.errors.append(message);
      }

      void
      warn(const std::string & message)
      {
        emission.warnings.append(message);
      }

      [[nodiscard]] std::string
      type_comment(const Compiler_Type_Id type_id) const
      {
        if (types == nullptr or type_id == 0)
          return "";
        return types->to_string(type_id);
      }

      void
      initialize_symbols()
      {
        names.prefix = "alephc_" + Compiler_Backend_C_Detail::sanitize_identifier(
          options->module_name.empty() ? std::string("module") : options->module_name);
        names.upper_prefix = Compiler_Backend_C_Detail::upper_identifier(names.prefix);
        names.tag_enum_name = names.prefix + "_tag";
        names.value_type_name = names.prefix + "_value";
        names.function_type_name = names.prefix + "_function";
        names.function_pointer_type_name = names.prefix + "_function_ptr";
        names.module_struct_name = names.prefix + "_module";
        names.module_init_symbol = names.prefix + "_module_init";
        names.dump_globals_symbol = names.prefix + "_dump_globals";
        names.print_value_symbol = names.prefix + "_print_value";
        names.tag_name_symbol = names.prefix + "_tag_name";
        names.panic_symbol = names.prefix + "_panic";
        names.require_assigned_symbol = names.prefix + "_require_assigned";
        names.require_int_symbol = names.prefix + "_require_int";
        names.require_bool_symbol = names.prefix + "_require_bool";
        names.require_function_symbol = names.prefix + "_require_function";
        names.equal_symbol = names.prefix + "_equal";
        names.call_symbol = names.prefix + "_call";
        names.make_invalid_symbol = names.prefix + "_make_invalid";
        names.make_unit_symbol = names.prefix + "_make_unit";
        names.make_bool_symbol = names.prefix + "_make_bool";
        names.make_int_symbol = names.prefix + "_make_int";
        names.make_string_symbol = names.prefix + "_make_string";
        names.make_char_symbol = names.prefix + "_make_char";
        names.make_function_symbol = names.prefix + "_make_function";
        names.top_level_symbol = names.prefix + "_top_level";
        names.tag_invalid_name = names.upper_prefix + "_TAG_INVALID";
        names.tag_unit_name = names.upper_prefix + "_TAG_UNIT";
        names.tag_bool_name = names.upper_prefix + "_TAG_BOOL";
        names.tag_int_name = names.upper_prefix + "_TAG_INT";
        names.tag_string_name = names.upper_prefix + "_TAG_STRING";
        names.tag_char_name = names.upper_prefix + "_TAG_CHAR";
        names.tag_function_name = names.upper_prefix + "_TAG_FUNCTION";

        emission.c_prefix = names.prefix;
        emission.value_type_name = names.value_type_name;
        emission.module_struct_name = names.module_struct_name;
        emission.module_init_symbol = names.module_init_symbol;
        emission.dump_globals_symbol = names.dump_globals_symbol;
        emission.print_value_symbol = names.print_value_symbol;
        emission.top_level_symbol = module->top_level != nullptr ? names.top_level_symbol : "";

        for (size_t i = 0; i < module->functions.size(); ++i)
          {
            const auto & function = *module->functions.access(i);
            const std::string symbol = names.prefix
              + "_f" + std::to_string(i) + "_"
              + Compiler_Backend_C_Detail::sanitize_identifier(function.name);
            emission.function_symbols.append(symbol);
            emission.function_descriptor_symbols.append(symbol + "_descriptor");
          }
      }

      void
      validate_input()
      {
        const auto report = validate_ir_module(*module);
        if (not report.valid)
          {
            for (size_t i = 0; i < report.errors.size(); ++i)
              fail(report.errors.access(i));
          }
        for (size_t i = 0; i < report.warnings.size(); ++i)
          warn(report.warnings.access(i));

        for (size_t i = 0; i < module->functions.size(); ++i)
          if (module->functions.access(i)->id != i)
            fail("C backend requires dense zero-based function ids; function '"
                 + module->functions.access(i)->name + "' has id "
                 + std::to_string(module->functions.access(i)->id)
                 + " but its module index is " + std::to_string(i));
      }

      [[nodiscard]] std::string
      function_symbol(const size_t index) const
      {
        return emission.function_symbols.access(index);
      }

      [[nodiscard]] std::string
      function_descriptor_symbol(const size_t index) const
      {
        return emission.function_descriptor_symbols.access(index);
      }

      [[nodiscard]] bool
      emit_constant_expression(const Compiler_IR_Instruction & inst,
                               std::string & expression,
                               std::string & error) const
      {
        if (inst.text == "unit")
          {
            expression = names.make_unit_symbol + "()";
            return true;
          }

        if (inst.text == "true" or inst.text == "false")
          {
            expression = names.make_bool_symbol + "("
              + std::string(inst.text == "true" ? "true" : "false") + ")";
            return true;
          }

        long long integer_value = 0;
        if (Compiler_Backend_C_Detail::parse_integer_literal(inst.text, integer_value))
          {
            expression = names.make_int_symbol + "("
              + std::to_string(integer_value) + "LL)";
            return true;
          }

        if (inst.text.size() >= 2 and inst.text.front() == '"' and inst.text.back() == '"')
          {
            expression = names.make_string_symbol + "(" + inst.text + ")";
            return true;
          }

        if (inst.text.size() >= 3 and inst.text.front() == '\'' and inst.text.back() == '\'')
          {
            expression = names.make_char_symbol + "(" + inst.text + ")";
            return true;
          }

        error = "C backend does not know how to emit constant literal '" + inst.text + "'";
        return false;
      }

      void
      emit_runtime_prelude(std::ostream & out) const
      {
        out
          << "#include <stdbool.h>\n"
          << "#include <stddef.h>\n"
          << "#include <stdio.h>\n"
          << "#include <stdlib.h>\n"
          << "#include <string.h>\n\n"
          << "typedef struct " << names.value_type_name << " "
          << names.value_type_name << ";\n"
          << "typedef struct " << names.function_type_name << " "
          << names.function_type_name << ";\n"
          << "typedef struct " << names.module_struct_name << " "
          << names.module_struct_name << ";\n\n"
          << "typedef enum " << names.tag_enum_name << "\n"
          << "{\n"
          << "  " << names.tag_invalid_name << " = 0,\n"
          << "  " << names.tag_unit_name << ",\n"
          << "  " << names.tag_bool_name << ",\n"
          << "  " << names.tag_int_name << ",\n"
          << "  " << names.tag_string_name << ",\n"
          << "  " << names.tag_char_name << ",\n"
          << "  " << names.tag_function_name << "\n"
          << "} " << names.tag_enum_name << ";\n\n"
          << "struct " << names.value_type_name << "\n"
          << "{\n"
          << "  " << names.tag_enum_name << " tag;\n"
          << "  union\n"
          << "  {\n"
          << "    bool boolean;\n"
          << "    long long integer;\n"
          << "    const char * string;\n"
          << "    char character;\n"
          << "    const " << names.function_type_name << " * function;\n"
          << "  } as;\n"
          << "};\n\n"
          << "typedef " << names.value_type_name << " (*"
          << names.function_pointer_type_name << ")("
          << names.module_struct_name << " * module,\n"
          << Compiler_Backend_C_Detail::indent(52) << "const "
          << names.value_type_name << " * args,\n"
          << Compiler_Backend_C_Detail::indent(52) << "size_t argc);\n\n"
          << "struct " << names.function_type_name << "\n"
          << "{\n"
          << "  " << names.function_pointer_type_name << " pointer;\n"
          << "  size_t arity;\n"
          << "  const char * name;\n"
          << "};\n\n"
          << "static const char *\n"
          << names.tag_name_symbol << "(" << names.tag_enum_name << " tag)\n"
          << "{\n"
          << "  switch (tag)\n"
          << "    {\n"
          << "    case " << names.tag_invalid_name << ": return \"Invalid\";\n"
          << "    case " << names.tag_unit_name << ": return \"Unit\";\n"
          << "    case " << names.tag_bool_name << ": return \"Bool\";\n"
          << "    case " << names.tag_int_name << ": return \"Int\";\n"
          << "    case " << names.tag_string_name << ": return \"String\";\n"
          << "    case " << names.tag_char_name << ": return \"Char\";\n"
          << "    case " << names.tag_function_name << ": return \"Function\";\n"
          << "    }\n"
          << "  return \"Unknown\";\n"
          << "}\n\n"
          << "static void\n"
          << names.panic_symbol << "(const char * message)\n"
          << "{\n"
          << "  fprintf(stderr, \"[" << names.prefix << "] %s\\n\", message);\n"
          << "  exit(1);\n"
          << "}\n\n"
          << "static " << names.value_type_name << "\n"
          << names.make_invalid_symbol << "(void)\n"
          << "{\n"
          << "  " << names.value_type_name << " value;\n"
          << "  value.tag = " << names.tag_invalid_name << ";\n"
          << "  value.as.integer = 0;\n"
          << "  return value;\n"
          << "}\n\n"
          << "static " << names.value_type_name << "\n"
          << names.make_unit_symbol << "(void)\n"
          << "{\n"
          << "  " << names.value_type_name << " value;\n"
          << "  value.tag = " << names.tag_unit_name << ";\n"
          << "  value.as.integer = 0;\n"
          << "  return value;\n"
          << "}\n\n"
          << "static " << names.value_type_name << "\n"
          << names.make_bool_symbol << "(bool value_in)\n"
          << "{\n"
          << "  " << names.value_type_name << " value;\n"
          << "  value.tag = " << names.tag_bool_name << ";\n"
          << "  value.as.boolean = value_in;\n"
          << "  return value;\n"
          << "}\n\n"
          << "static " << names.value_type_name << "\n"
          << names.make_int_symbol << "(long long value_in)\n"
          << "{\n"
          << "  " << names.value_type_name << " value;\n"
          << "  value.tag = " << names.tag_int_name << ";\n"
          << "  value.as.integer = value_in;\n"
          << "  return value;\n"
          << "}\n\n"
          << "static " << names.value_type_name << "\n"
          << names.make_string_symbol << "(const char * value_in)\n"
          << "{\n"
          << "  " << names.value_type_name << " value;\n"
          << "  value.tag = " << names.tag_string_name << ";\n"
          << "  value.as.string = value_in;\n"
          << "  return value;\n"
          << "}\n\n"
          << "static " << names.value_type_name << "\n"
          << names.make_char_symbol << "(char value_in)\n"
          << "{\n"
          << "  " << names.value_type_name << " value;\n"
          << "  value.tag = " << names.tag_char_name << ";\n"
          << "  value.as.character = value_in;\n"
          << "  return value;\n"
          << "}\n\n"
          << "static " << names.value_type_name << "\n"
          << names.make_function_symbol << "(const "
          << names.function_type_name << " * function)\n"
          << "{\n"
          << "  " << names.value_type_name << " value;\n"
          << "  value.tag = " << names.tag_function_name << ";\n"
          << "  value.as.function = function;\n"
          << "  return value;\n"
          << "}\n\n"
          << "static " << names.value_type_name << "\n"
          << names.require_assigned_symbol << "(" << names.value_type_name
          << " value,\n"
          << Compiler_Backend_C_Detail::indent(37) << "const char * context)\n"
          << "{\n"
          << "  if (value.tag == " << names.tag_invalid_name << ")\n"
          << "    {\n"
          << "      fprintf(stderr, \"[" << names.prefix
          << "] %s uses an uninitialized value\\n\", context);\n"
          << "      exit(1);\n"
          << "    }\n"
          << "  return value;\n"
          << "}\n\n"
          << "static long long\n"
          << names.require_int_symbol << "(" << names.value_type_name << " value,\n"
          << Compiler_Backend_C_Detail::indent(32) << "const char * context)\n"
          << "{\n"
          << "  value = " << names.require_assigned_symbol << "(value, context);\n"
          << "  if (value.tag != " << names.tag_int_name << ")\n"
          << "    {\n"
          << "      fprintf(stderr, \"[" << names.prefix
          << "] %s expects Int but received %s\\n\",\n"
          << Compiler_Backend_C_Detail::indent(14)
          << "context,\n"
          << Compiler_Backend_C_Detail::indent(14)
          << names.tag_name_symbol << "(value.tag));\n"
          << "      exit(1);\n"
          << "    }\n"
          << "  return value.as.integer;\n"
          << "}\n\n"
          << "static bool\n"
          << names.require_bool_symbol << "(" << names.value_type_name << " value,\n"
          << Compiler_Backend_C_Detail::indent(33) << "const char * context)\n"
          << "{\n"
          << "  value = " << names.require_assigned_symbol << "(value, context);\n"
          << "  if (value.tag != " << names.tag_bool_name << ")\n"
          << "    {\n"
          << "      fprintf(stderr, \"[" << names.prefix
          << "] %s expects Bool but received %s\\n\",\n"
          << Compiler_Backend_C_Detail::indent(14)
          << "context,\n"
          << Compiler_Backend_C_Detail::indent(14)
          << names.tag_name_symbol << "(value.tag));\n"
          << "      exit(1);\n"
          << "    }\n"
          << "  return value.as.boolean;\n"
          << "}\n\n"
          << "static const " << names.function_type_name << " *\n"
          << names.require_function_symbol << "(" << names.value_type_name << " value,\n"
          << Compiler_Backend_C_Detail::indent(37) << "const char * context)\n"
          << "{\n"
          << "  value = " << names.require_assigned_symbol << "(value, context);\n"
          << "  if (value.tag != " << names.tag_function_name << ")\n"
          << "    {\n"
          << "      fprintf(stderr, \"[" << names.prefix
          << "] %s expects Function but received %s\\n\",\n"
          << Compiler_Backend_C_Detail::indent(14)
          << "context,\n"
          << Compiler_Backend_C_Detail::indent(14)
          << names.tag_name_symbol << "(value.tag));\n"
          << "      exit(1);\n"
          << "    }\n"
          << "  return value.as.function;\n"
          << "}\n\n"
          << "static bool\n"
          << names.equal_symbol << "(" << names.value_type_name << " lhs,\n"
          << Compiler_Backend_C_Detail::indent(22) << names.value_type_name << " rhs)\n"
          << "{\n"
          << "  lhs = " << names.require_assigned_symbol << "(lhs, \"equality lhs\");\n"
          << "  rhs = " << names.require_assigned_symbol << "(rhs, \"equality rhs\");\n"
          << "  if (lhs.tag != rhs.tag)\n"
          << "    return false;\n"
          << "  switch (lhs.tag)\n"
          << "    {\n"
          << "    case " << names.tag_invalid_name << ":\n"
          << "    case " << names.tag_unit_name << ":\n"
          << "      return true;\n"
          << "    case " << names.tag_bool_name << ":\n"
          << "      return lhs.as.boolean == rhs.as.boolean;\n"
          << "    case " << names.tag_int_name << ":\n"
          << "      return lhs.as.integer == rhs.as.integer;\n"
          << "    case " << names.tag_string_name << ":\n"
          << "      return strcmp(lhs.as.string, rhs.as.string) == 0;\n"
          << "    case " << names.tag_char_name << ":\n"
          << "      return lhs.as.character == rhs.as.character;\n"
          << "    case " << names.tag_function_name << ":\n"
          << "      return lhs.as.function == rhs.as.function;\n"
          << "    }\n"
          << "  return false;\n"
          << "}\n\n"
          << "static " << names.value_type_name << "\n"
          << names.call_symbol << "(" << names.module_struct_name << " * module,\n"
          << Compiler_Backend_C_Detail::indent(19) << names.value_type_name << " callee,\n"
          << Compiler_Backend_C_Detail::indent(19) << "const "
          << names.value_type_name << " * args,\n"
          << Compiler_Backend_C_Detail::indent(19) << "size_t argc)\n"
          << "{\n"
          << "  const " << names.function_type_name << " * function\n"
          << "    = " << names.require_function_symbol << "(callee, \"call callee\");\n"
          << "  if (function->arity != argc)\n"
          << "    {\n"
          << "      fprintf(stderr, \"[" << names.prefix
          << "] function %s expects %zu arguments but received %zu\\n\",\n"
          << Compiler_Backend_C_Detail::indent(14)
          << "function->name,\n"
          << Compiler_Backend_C_Detail::indent(14)
          << "function->arity,\n"
          << Compiler_Backend_C_Detail::indent(14)
          << "argc);\n"
          << "      exit(1);\n"
          << "    }\n"
          << "  return function->pointer(module, args, argc);\n"
          << "}\n\n"
          << "static void\n"
          << names.print_value_symbol << "(FILE * out,\n"
          << Compiler_Backend_C_Detail::indent(27)
          << names.value_type_name << " value)\n"
          << "{\n"
          << "  value = " << names.require_assigned_symbol << "(value, \"print value\");\n"
          << "  switch (value.tag)\n"
          << "    {\n"
          << "    case " << names.tag_invalid_name << ":\n"
          << "      fputs(\"Invalid\", out);\n"
          << "      return;\n"
          << "    case " << names.tag_unit_name << ":\n"
          << "      fputs(\"Unit\", out);\n"
          << "      return;\n"
          << "    case " << names.tag_bool_name << ":\n"
          << "      fputs(value.as.boolean ? \"Bool(true)\" : \"Bool(false)\", out);\n"
          << "      return;\n"
          << "    case " << names.tag_int_name << ":\n"
          << "      fprintf(out, \"Int(%lld)\", value.as.integer);\n"
          << "      return;\n"
          << "    case " << names.tag_string_name << ":\n"
          << "      fprintf(out, \"String(\\\"%s\\\")\", value.as.string);\n"
          << "      return;\n"
          << "    case " << names.tag_char_name << ":\n"
          << "      fprintf(out, \"Char('%c')\", value.as.character);\n"
          << "      return;\n"
          << "    case " << names.tag_function_name << ":\n"
          << "      fprintf(out, \"Function(%s)\", value.as.function->name);\n"
          << "      return;\n"
          << "    }\n"
          << "}\n\n";
      }

      void
      emit_module_layout(std::ostream & out) const
      {
        const size_t global_count = module->global_slots.size();
        out << "enum { " << names.upper_prefix << "_GLOBAL_COUNT = "
            << global_count << " };\n\n";

        out << "struct " << names.module_struct_name << "\n"
            << "{\n";
        if (global_count == 0)
          out << "  " << names.value_type_name << " unused_global;\n";
        else
          out << "  " << names.value_type_name << " globals["
              << names.upper_prefix << "_GLOBAL_COUNT];\n";
        out << "};\n\n";

        out << "static void\n"
            << names.module_init_symbol << "(" << names.module_struct_name
            << " * module)\n"
            << "{\n";
        if (global_count == 0)
          out << "  (void) module;\n";
        else
          {
            out << "  for (size_t i = 0; i < " << names.upper_prefix
                << "_GLOBAL_COUNT; ++i)\n"
                << "    module->globals[i] = " << names.make_invalid_symbol << "();\n";
          }
        out << "}\n\n";

        out << "static void\n"
            << names.dump_globals_symbol << "(FILE * out,\n"
            << Compiler_Backend_C_Detail::indent(27)
            << "const " << names.module_struct_name << " * module)\n"
            << "{\n"
            << "  fputs(\"Globals\\n\", out);\n";
        if (global_count == 0)
          out << "  fputs(\"  <none>\\n\", out);\n";
        else
          for (size_t i = 0; i < module->global_slots.size(); ++i)
            {
              const auto & slot = module->global_slots.access(i);
              out << "  fputs(\"  g" << slot.id << " "
                  << Compiler_Backend_C_Detail::safe_comment_text(slot.name)
                  << " = \", out);\n"
                  << "  " << names.print_value_symbol
                  << "(out, module->globals[" << i << "]);\n"
                  << "  fputc('\\n', out);\n";
            }
        out << "}\n\n";
      }

      void
      emit_prototypes(std::ostream & out) const
      {
        for (size_t i = 0; i < module->functions.size(); ++i)
          {
            out << "static " << names.value_type_name << "\n"
                << function_symbol(i) << "(" << names.module_struct_name
                << " * module,\n"
                << Compiler_Backend_C_Detail::indent(function_symbol(i).size() + 8)
                << "const " << names.value_type_name
                << " * args,\n"
                << Compiler_Backend_C_Detail::indent(function_symbol(i).size() + 8)
                << "size_t argc);\n"
                << "static const " << names.function_type_name << " "
                << function_descriptor_symbol(i) << ";\n\n";
          }

        if (module->top_level != nullptr)
          out << "static " << names.value_type_name << "\n"
              << names.top_level_symbol << "(" << names.module_struct_name
              << " * module,\n"
              << Compiler_Backend_C_Detail::indent(names.top_level_symbol.size() + 8)
              << "const " << names.value_type_name << " * args,\n"
              << Compiler_Backend_C_Detail::indent(names.top_level_symbol.size() + 8)
              << "size_t argc);\n\n";
      }

      void
      emit_function_signature(std::ostream & out,
                              const std::string & symbol) const
      {
        out << "static " << names.value_type_name << "\n"
            << symbol << "(" << names.module_struct_name << " * module,\n"
            << Compiler_Backend_C_Detail::indent(symbol.size() + 8)
            << "const " << names.value_type_name << " * args,\n"
            << Compiler_Backend_C_Detail::indent(symbol.size() + 8)
            << "size_t argc)\n";
      }

      void
      emit_slot_declarations(std::ostream & out,
                             const Compiler_IR_Function & function) const
      {
        for (size_t i = 0; i < function.local_slots.size(); ++i)
          {
            const auto & slot = function.local_slots.access(i);
            out << "  " << names.value_type_name << " "
                << Compiler_Backend_C_Detail::local_name(slot.id)
                << " = " << names.make_invalid_symbol << "();";
            if (options->emit_comments)
              {
                out << " /* " << compiler_ir_slot_kind_name(slot.kind) << ' '
                    << Compiler_Backend_C_Detail::safe_comment_text(slot.name);
                const auto type_text = type_comment(slot.type_id);
                if (not type_text.empty())
                  out << " : "
                      << Compiler_Backend_C_Detail::safe_comment_text(type_text);
                out << " */";
              }
            out << '\n';
          }
      }

      void
      emit_value_declarations(std::ostream & out,
                              const Compiler_IR_Function & function) const
      {
        for (size_t value_id = 1; value_id <= function.next_value_id; ++value_id)
          out << "  " << names.value_type_name << " "
              << Compiler_Backend_C_Detail::value_name(value_id)
              << " = " << names.make_invalid_symbol << "();\n";
      }

      void
      emit_parameter_bindings(std::ostream & out,
                              const Compiler_IR_Function & function) const
      {
        const size_t arity = Compiler_Backend_C_Detail::count_parameters(function);
        out << "  if (argc != " << arity << "u)\n"
            << "    {\n"
            << "      fprintf(stderr, \"[" << names.prefix
            << "] function "
            << Compiler_Backend_C_Detail::safe_comment_text(function.name)
            << " expects %zu arguments but received %zu\\n\",\n"
            << "              (size_t) " << arity << "u,\n"
            << "              argc);\n"
            << "      exit(1);\n"
            << "    }\n";

        size_t parameter_index = 0;
        for (size_t i = 0; i < function.local_slots.size(); ++i)
          {
            const auto & slot = function.local_slots.access(i);
            if (slot.kind != Compiler_IR_Slot_Kind::Parameter)
              continue;

            out << "  " << Compiler_Backend_C_Detail::local_name(slot.id)
                << " = args[" << parameter_index << "];\n";
            ++parameter_index;
          }
      }

      void
      emit_instruction_comment(std::ostream & out,
                               const Compiler_IR_Instruction & inst) const
      {
        if (not options->emit_comments)
          return;

        out << "    /* ";
        if (inst.result_id != 0)
          out << Compiler_Backend_C_Detail::value_name(inst.result_id) << " = ";
        out << compiler_ir_instruction_kind_name(inst.kind);
        if (inst.kind == Compiler_IR_Instruction_Kind::Unary
            or inst.kind == Compiler_IR_Instruction_Kind::Binary)
          out << '(' << Compiler_Backend_C_Detail::token_name(inst.op) << ')';
        if (inst.kind == Compiler_IR_Instruction_Kind::Constant)
          out << '(' << Compiler_Backend_C_Detail::safe_comment_text(inst.text) << ')';
        if (inst.kind == Compiler_IR_Instruction_Kind::Function_Ref)
          out << "(f" << inst.function_id << ')';
        out << " */\n";
      }

      void
      emit_instruction(std::ostream & out,
                       const Compiler_IR_Instruction & inst,
                       const Compiler_IR_Function & function)
      {
        (void) function;
        const auto value = [](const Compiler_IR_Value_Id id)
        {
          return Compiler_Backend_C_Detail::value_name(id);
        };
        const auto slot = [](const Compiler_IR_Local_Slot_Id id)
        {
          return Compiler_Backend_C_Detail::local_name(id);
        };

        emit_instruction_comment(out, inst);

        switch (inst.kind)
          {
          case Compiler_IR_Instruction_Kind::Constant:
            {
              std::string expression;
              std::string error;
              if (not emit_constant_expression(inst, expression, error))
                {
                  fail(error);
                  return;
                }

              out << "    " << value(inst.result_id) << " = "
                  << expression << ";\n";
              break;
            }

          case Compiler_IR_Instruction_Kind::Load:
            if (inst.global_slot_id != compiler_ir_invalid_id())
              out << "    " << value(inst.result_id) << " = "
                  << names.require_assigned_symbol << "(module->globals["
                  << inst.global_slot_id << "], "
                  << Compiler_Backend_C_Detail::c_string_literal(
                       "load " + Compiler_Backend_C_Detail::global_name(inst.global_slot_id))
                  << ");\n";
            else
              out << "    " << value(inst.result_id) << " = "
                  << names.require_assigned_symbol << "("
                  << slot(inst.local_slot_id) << ", "
                  << Compiler_Backend_C_Detail::c_string_literal(
                       "load " + Compiler_Backend_C_Detail::local_name(inst.local_slot_id))
                  << ");\n";
            break;

          case Compiler_IR_Instruction_Kind::Store:
            if (inst.operands.is_empty())
              {
                fail("C backend cannot emit Store without one operand");
                return;
              }

            if (inst.global_slot_id != compiler_ir_invalid_id())
              out << "    module->globals[" << inst.global_slot_id << "] = "
                  << names.require_assigned_symbol << "("
                  << value(inst.operands.access(0)) << ", "
                  << Compiler_Backend_C_Detail::c_string_literal(
                       "store operand " + value(inst.operands.access(0)))
                  << ");\n";
            else
              out << "    " << slot(inst.local_slot_id) << " = "
                  << names.require_assigned_symbol << "("
                  << value(inst.operands.access(0)) << ", "
                  << Compiler_Backend_C_Detail::c_string_literal(
                       "store operand " + value(inst.operands.access(0)))
                  << ");\n";
            break;

          case Compiler_IR_Instruction_Kind::Unary:
            if (inst.operands.is_empty())
              {
                fail("C backend cannot emit Unary without one operand");
                return;
              }

            switch (inst.op)
              {
              case Compiler_Token_Kind::Minus:
                out << "    " << value(inst.result_id) << " = "
                    << names.make_int_symbol << "(-"
                    << names.require_int_symbol << "("
                    << value(inst.operands.access(0)) << ", "
                    << Compiler_Backend_C_Detail::c_string_literal("unary minus")
                    << "));\n";
                break;

              case Compiler_Token_Kind::Bang:
                out << "    " << value(inst.result_id) << " = "
                    << names.make_bool_symbol << "(!"
                    << names.require_bool_symbol << "("
                    << value(inst.operands.access(0)) << ", "
                    << Compiler_Backend_C_Detail::c_string_literal("logical not")
                    << "));\n";
                break;

              case Compiler_Token_Kind::Tilde:
                out << "    " << value(inst.result_id) << " = "
                    << names.make_int_symbol << "(~"
                    << names.require_int_symbol << "("
                    << value(inst.operands.access(0)) << ", "
                    << Compiler_Backend_C_Detail::c_string_literal("bitwise not")
                    << "));\n";
                break;

              default:
                fail("C backend does not support unary operator '"
                     + Compiler_Backend_C_Detail::token_name(inst.op) + "'");
                break;
              }
            break;

          case Compiler_IR_Instruction_Kind::Binary:
            if (inst.operands.size() < 2)
              {
                fail("C backend cannot emit Binary without two operands");
                return;
              }

            switch (inst.op)
              {
              case Compiler_Token_Kind::Plus:
              case Compiler_Token_Kind::Minus:
              case Compiler_Token_Kind::Star:
              case Compiler_Token_Kind::Amp:
              case Compiler_Token_Kind::Pipe:
              case Compiler_Token_Kind::Caret:
                {
                  const char * op_text = inst.op == Compiler_Token_Kind::Plus ? "+"
                    : inst.op == Compiler_Token_Kind::Minus ? "-"
                    : inst.op == Compiler_Token_Kind::Star ? "*"
                    : inst.op == Compiler_Token_Kind::Amp ? "&"
                    : inst.op == Compiler_Token_Kind::Pipe ? "|"
                    : "^";
                  out << "    {\n"
                      << "      long long lhs_value = " << names.require_int_symbol
                      << "(" << value(inst.operands.access(0)) << ", "
                      << Compiler_Backend_C_Detail::c_string_literal(
                           "binary lhs " + std::string(op_text))
                      << ");\n"
                      << "      long long rhs_value = " << names.require_int_symbol
                      << "(" << value(inst.operands.access(1)) << ", "
                      << Compiler_Backend_C_Detail::c_string_literal(
                           "binary rhs " + std::string(op_text))
                      << ");\n"
                      << "      " << value(inst.result_id) << " = "
                      << names.make_int_symbol
                      << "(lhs_value " << op_text << " rhs_value);\n"
                      << "    }\n";
                  break;
                }

              case Compiler_Token_Kind::Slash:
              case Compiler_Token_Kind::Percent:
                {
                  const char * op_text = inst.op == Compiler_Token_Kind::Slash ? "/" : "%";
                  const char * message = inst.op == Compiler_Token_Kind::Slash
                    ? "division by zero"
                    : "modulo by zero";
                  out << "    {\n"
                      << "      long long lhs_value = " << names.require_int_symbol
                      << "(" << value(inst.operands.access(0)) << ", "
                      << Compiler_Backend_C_Detail::c_string_literal(
                           "binary lhs " + std::string(op_text))
                      << ");\n"
                      << "      long long rhs_value = " << names.require_int_symbol
                      << "(" << value(inst.operands.access(1)) << ", "
                      << Compiler_Backend_C_Detail::c_string_literal(
                           "binary rhs " + std::string(op_text))
                      << ");\n"
                      << "      if (rhs_value == 0LL)\n"
                      << "        " << names.panic_symbol << "("
                      << Compiler_Backend_C_Detail::c_string_literal(message)
                      << ");\n"
                      << "      " << value(inst.result_id) << " = "
                      << names.make_int_symbol
                      << "(lhs_value " << op_text << " rhs_value);\n"
                      << "    }\n";
                  break;
                }

              case Compiler_Token_Kind::Less:
              case Compiler_Token_Kind::LessEq:
              case Compiler_Token_Kind::Greater:
              case Compiler_Token_Kind::GreaterEq:
                {
                  const char * op_text = inst.op == Compiler_Token_Kind::Less ? "<"
                    : inst.op == Compiler_Token_Kind::LessEq ? "<="
                    : inst.op == Compiler_Token_Kind::Greater ? ">"
                    : ">=";
                  out << "    {\n"
                      << "      long long lhs_value = " << names.require_int_symbol
                      << "(" << value(inst.operands.access(0)) << ", "
                      << Compiler_Backend_C_Detail::c_string_literal(
                           "comparison lhs " + std::string(op_text))
                      << ");\n"
                      << "      long long rhs_value = " << names.require_int_symbol
                      << "(" << value(inst.operands.access(1)) << ", "
                      << Compiler_Backend_C_Detail::c_string_literal(
                           "comparison rhs " + std::string(op_text))
                      << ");\n"
                      << "      " << value(inst.result_id) << " = "
                      << names.make_bool_symbol
                      << "(lhs_value " << op_text << " rhs_value);\n"
                      << "    }\n";
                  break;
                }

              case Compiler_Token_Kind::AndAnd:
              case Compiler_Token_Kind::OrOr:
                {
                  const char * op_text = inst.op == Compiler_Token_Kind::AndAnd ? "&&" : "||";
                  out << "    {\n"
                      << "      bool lhs_value = " << names.require_bool_symbol
                      << "(" << value(inst.operands.access(0)) << ", "
                      << Compiler_Backend_C_Detail::c_string_literal(
                           "logical lhs " + std::string(op_text))
                      << ");\n"
                      << "      bool rhs_value = " << names.require_bool_symbol
                      << "(" << value(inst.operands.access(1)) << ", "
                      << Compiler_Backend_C_Detail::c_string_literal(
                           "logical rhs " + std::string(op_text))
                      << ");\n"
                      << "      " << value(inst.result_id) << " = "
                      << names.make_bool_symbol
                      << "(lhs_value " << op_text << " rhs_value);\n"
                      << "    }\n";
                  break;
                }

              case Compiler_Token_Kind::EqEq:
              case Compiler_Token_Kind::NotEq:
                out << "    " << value(inst.result_id) << " = "
                    << names.make_bool_symbol << "("
                    << (inst.op == Compiler_Token_Kind::NotEq ? "!" : "")
                    << names.equal_symbol << "("
                    << value(inst.operands.access(0)) << ", "
                    << value(inst.operands.access(1)) << "));\n";
                break;

              default:
                fail("C backend does not support binary operator '"
                     + Compiler_Backend_C_Detail::token_name(inst.op) + "'");
                break;
              }
            break;

          case Compiler_IR_Instruction_Kind::Call:
            if (inst.operands.is_empty())
              {
                fail("C backend cannot emit Call without a callee operand");
                return;
              }

            out << "    {\n";
            if (inst.operands.size() == 1)
              out << "      " << value(inst.result_id) << " = "
                  << names.call_symbol << "(module, "
                  << value(inst.operands.access(0)) << ", NULL, 0u);\n";
            else
              {
                out << "      const " << names.value_type_name << " call_args[] =\n"
                    << "        {\n";
                for (size_t i = 1; i < inst.operands.size(); ++i)
                  out << "          " << names.require_assigned_symbol << "("
                      << value(inst.operands.access(i)) << ", "
                      << Compiler_Backend_C_Detail::c_string_literal(
                           "call arg #" + std::to_string(i - 1))
                      << ")" << (i + 1 < inst.operands.size() ? "," : "") << '\n';
                out << "        };\n"
                    << "      " << value(inst.result_id) << " = "
                    << names.call_symbol << "(module, "
                    << value(inst.operands.access(0))
                    << ", call_args, " << (inst.operands.size() - 1) << "u);\n";
              }
            out << "    }\n";
            break;

          case Compiler_IR_Instruction_Kind::Function_Ref:
            out << "    " << value(inst.result_id) << " = "
                << names.make_function_symbol << "(&"
                << function_descriptor_symbol(inst.function_id) << ");\n";
            break;
          }
      }

      void
      emit_terminator(std::ostream & out,
                      const Compiler_IR_Block & block,
                      const Compiler_IR_Function & function)
      {
        if (options->emit_comments)
          out << "    /* Terminator: "
              << compiler_ir_terminator_kind_name(block.terminator.kind) << " */\n";

        switch (block.terminator.kind)
          {
          case Compiler_IR_Terminator_Kind::Jump:
            out << "    goto " << names.prefix << '_'
                << Compiler_Backend_C_Detail::block_label(
                     block.terminator.successors.access(0))
                << ";\n";
            break;

          case Compiler_IR_Terminator_Kind::Branch:
            out << "    if (" << names.require_bool_symbol << "("
                << Compiler_Backend_C_Detail::value_name(block.terminator.condition_value)
                << ", "
                << Compiler_Backend_C_Detail::c_string_literal("branch condition")
                << "))\n"
                << "      goto " << names.prefix << '_'
                << Compiler_Backend_C_Detail::block_label(
                     block.terminator.successors.access(0))
                << ";\n"
                << "    goto " << names.prefix << '_'
                << Compiler_Backend_C_Detail::block_label(
                     block.terminator.successors.access(1))
                << ";\n";
            break;

          case Compiler_IR_Terminator_Kind::Return:
            out << "    return " << names.require_assigned_symbol << "("
                << Compiler_Backend_C_Detail::value_name(block.terminator.return_value)
                << ", "
                << Compiler_Backend_C_Detail::c_string_literal("return value")
                << ");\n";
            break;

          case Compiler_IR_Terminator_Kind::Exit:
            out << "    return " << names.make_unit_symbol << "();\n";
            break;

          case Compiler_IR_Terminator_Kind::Unreachable:
            out << "    " << names.panic_symbol << "("
                << Compiler_Backend_C_Detail::c_string_literal(
                     "entered unreachable block in function " + function.name)
                << ");\n"
                << "    return " << names.make_invalid_symbol << "();\n";
            break;

          case Compiler_IR_Terminator_Kind::None:
            fail("C backend cannot emit unterminated block "
                 + Compiler_Backend_C_Detail::block_label(block.id));
            break;
          }
      }

      void
      emit_function_body(std::ostream & out,
                         const Compiler_IR_Function & function,
                         const std::string & symbol)
      {
        if (options->emit_comments)
          {
            out << "/* "
                << Compiler_Backend_C_Detail::safe_comment_text(function.name);
            const auto type_text = type_comment(function.type_id);
            if (not type_text.empty())
              out << " : "
                  << Compiler_Backend_C_Detail::safe_comment_text(type_text);
            out << " */\n";
          }

        emit_function_signature(out, symbol);
        out << "{\n"
            << "  (void) module;\n";
        emit_slot_declarations(out, function);
        emit_value_declarations(out, function);
        emit_parameter_bindings(out, function);
        out << "  goto " << names.prefix << '_'
            << Compiler_Backend_C_Detail::block_label(function.entry_block)
            << ";\n\n";

        for (size_t i = 0; i < function.blocks.size(); ++i)
          {
            const auto & block = function.blocks.access(i);
            out << names.prefix << '_'
                << Compiler_Backend_C_Detail::block_label(block.id) << ":\n";
            if (options->emit_comments)
              out << "    /* Block "
                  << Compiler_Backend_C_Detail::block_label(block.id)
                  << ": "
                  << Compiler_Backend_C_Detail::safe_comment_text(block.label)
                  << " */\n";
            for (size_t inst_index = 0; inst_index < block.instructions.size(); ++inst_index)
              emit_instruction(out, block.instructions.access(inst_index), function);
            emit_terminator(out, block, function);
            out << '\n';
          }

        out << "}\n\n";
      }

      void
      emit_function(const size_t index,
                    std::ostream & out)
      {
        emit_function_body(out,
                           *module->functions.access(index),
                           function_symbol(index));
      }

      void
      emit_top_level(std::ostream & out)
      {
        emit_function_body(out, *module->top_level, names.top_level_symbol);
      }

      void
      emit_function_descriptors(std::ostream & out) const
      {
        for (size_t i = 0; i < module->functions.size(); ++i)
          {
            const auto & function = *module->functions.access(i);
            out << "static const " << names.function_type_name << " "
                << function_descriptor_symbol(i) << " =\n"
                << "{\n"
                << "  " << function_symbol(i) << ",\n"
                << "  " << Compiler_Backend_C_Detail::count_parameters(function) << "u,\n"
                << "  "
                << Compiler_Backend_C_Detail::c_string_literal(function.name)
                << "\n"
                << "};\n\n";
          }
      }

      void
      emit_main(std::ostream & out)
      {
        if (not options->emit_main)
          return;

        if (module->top_level == nullptr)
          {
            warn("C backend was asked to emit main(), but the IR module has no top-level body");
            return;
          }

        out << "int\n"
            << "main(void)\n"
            << "{\n"
            << "  " << names.module_struct_name << " module;\n"
            << "  " << names.module_init_symbol << "(&module);\n"
            << "  " << names.value_type_name << " result = "
            << names.top_level_symbol << "(&module, NULL, 0u);\n"
            << "  fputs(\"Result = \", stdout);\n"
            << "  " << names.print_value_symbol << "(stdout, result);\n"
            << "  fputc('\\n', stdout);\n"
            << "  " << names.dump_globals_symbol << "(stdout, &module);\n"
            << "  return 0;\n"
            << "}\n";
      }

    public:
      /** @brief Builds an emitter over one module and one option set. */
      Emitter(const Compiler_C_Backend_Options & options_in,
              const Compiler_IR_Module & module_in,
              const Compiler_Type_Context * types_in)
        : options(&options_in),
          module(&module_in),
          types(types_in)
      {
        initialize_symbols();
      }

      /** @brief Emits the full C translation unit. */
      Compiler_C_Backend_Emission
      run()
      {
        validate_input();
        if (not emission.valid)
          return emission;

        std::ostringstream out;
        if (options->emit_comments)
          out << "/* Generated by Compiler_Backend_C.H from explicit IR. */\n\n";

        emit_runtime_prelude(out);
        emit_module_layout(out);
        emit_prototypes(out);
        for (size_t i = 0; i < module->functions.size(); ++i)
          emit_function(i, out);
        if (module->top_level != nullptr)
          emit_top_level(out);
        emit_function_descriptors(out);
        emit_main(out);

        if (emission.valid)
          emission.source = out.str();
        return emission;
      }
    };

  public:
    /** @brief Builds one reusable C backend with `options_in`. */
    explicit Compiler_C_Backend(Compiler_C_Backend_Options options_in = {})
      : options(std::move(options_in))
    {}

    /** @brief Emits one validated IR module as portable C.
     *
     *  @param module IR module to translate.
     *  @param types Optional type context used only for readable comments.
     *  @return Structured emission result containing the generated C source and
     *          the most relevant exported symbol names.
     */
    [[nodiscard]] Compiler_C_Backend_Emission
    emit_module(const Compiler_IR_Module * module,
                const Compiler_Type_Context * types = nullptr) const
    {
      Compiler_C_Backend_Emission emission;
      if (module == nullptr)
        {
          emission.valid = false;
          emission.errors.append("Compiler_C_Backend::emit_module(): null IR module");
          return emission;
        }

      return Emitter(options, *module, types).run();
    }
  };

  /** @brief Convenience wrapper that emits one IR module to portable C. */
  inline Compiler_C_Backend_Emission
  compiler_emit_c_module(const Compiler_IR_Module * module,
                         const Compiler_C_Backend_Options & options = {},
                         const Compiler_Type_Context * types = nullptr)
  {
    return Compiler_C_Backend(options).emit_module(module, types);
  }
}

#endif
