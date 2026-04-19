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

/** @file Interpreter_Runtime.H
 *  @brief Reusable runtime and structured evaluator for HIR-based interpreters.
 *
 *  This header adds the first execution-oriented layer on top of
 *  `Compiler_HIR.H`. The goal is not to hard-code one language runtime, but to
 *  provide a compact, reusable substrate that can execute small typed HIR
 *  programs and support interpreter-like tooling.
 *
 *  The current runtime includes:
 *
 *  - runtime values for Unit, Bool, Int, String, Char, tuples, HIR functions,
 *    and host functions
 *  - nested environments with lexical lookup and assignment
 *  - a call stack for HIR function invocations
 *  - structured runtime errors with stable codes and source spans
 *  - evaluation of top-level statements and HIR functions
 *
 *  The MVP intentionally avoids heap management, user-defined closure syntax,
 *  and exceptions with stack unwinding. Those extensions are possible later
 *  without coupling the runtime to one concrete language front-end.
 *
 *  @ingroup Utilities
 */

#ifndef INTERPRETER_RUNTIME_H
#define INTERPRETER_RUNTIME_H

#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <Compiler_HIR.H>
#include <ah-diagnostics.H>
#include <ah-errors.H>
#include <tpl_dynArray.H>

namespace Aleph {
class Interpreter_Environment;
struct Interpreter_Value;
struct Interpreter_Tuple_Storage;
struct Interpreter_Runtime_Error;

/** @brief Runtime categories supported by the interpreter MVP. */
enum class Interpreter_Value_Kind
{
  Invalid,
  Unit,
  Bool,
  Integer,
  String,
  Character,
  Tuple,
  Function,
  Host_Function
};

/** @brief Structured control-flow outcomes during evaluation. */
enum class Interpreter_Control_Flow_Kind
{
  None,
  Return,
  Break,
  Continue,
  Runtime_Error
};

/** @brief Stable debug name for one runtime value kind. */
inline const char *interpreter_value_kind_name(const Interpreter_Value_Kind kind) noexcept
{
  switch (kind)
    {
    case Interpreter_Value_Kind::Invalid:
      return "Invalid";
    case Interpreter_Value_Kind::Unit:
      return "Unit";
    case Interpreter_Value_Kind::Bool:
      return "Bool";
    case Interpreter_Value_Kind::Integer:
      return "Int";
    case Interpreter_Value_Kind::String:
      return "String";
    case Interpreter_Value_Kind::Character:
      return "Char";
    case Interpreter_Value_Kind::Tuple:
      return "Tuple";
    case Interpreter_Value_Kind::Function:
      return "Function";
    case Interpreter_Value_Kind::Host_Function:
      return "HostFunction";
    }

  return "Unknown";
}

/** @brief Stable debug name for one control-flow outcome. */
inline const char *interpreter_control_flow_name(const Interpreter_Control_Flow_Kind kind) noexcept
{
  switch (kind)
    {
    case Interpreter_Control_Flow_Kind::None:
      return "None";
    case Interpreter_Control_Flow_Kind::Return:
      return "Return";
    case Interpreter_Control_Flow_Kind::Break:
      return "Break";
    case Interpreter_Control_Flow_Kind::Continue:
      return "Continue";
    case Interpreter_Control_Flow_Kind::Runtime_Error:
      return "RuntimeError";
    }

  return "Unknown";
}

/** @brief Structured runtime error.
 *
 *  Runtime failures are reported as values instead of exceptions so callers
 *  can render, transform, or ignore them as needed. When available, `span`
 *  points to the HIR node that triggered the failure.
 */
struct Interpreter_Runtime_Error
{
  std::string code;     ///< Stable runtime code such as `RUN004`.
  std::string message;  ///< Human-readable runtime message.
  Source_Span span;     ///< Source region associated with the failure.

  /** @brief Returns whether this error object is populated. */
  [[nodiscard]] bool has_error() const noexcept
  {
    return not code.empty() or not message.empty();
  }
};

/** @brief Host callback signature used by runtime built-ins or embeddings.
 *
 *  @param user_data Opaque embedding-owned pointer.
 *  @param arguments Positional arguments already evaluated by the runtime.
 *  @param result Output slot for the produced runtime value.
 *  @param error Output slot for structured runtime failures.
 *  @return `true` on success, `false` if `error` was populated.
 */
using Interpreter_Host_Function = bool (*)(void *user_data,
                                           const DynArray<Interpreter_Value> &arguments,
                                           Interpreter_Value &result,
                                           Interpreter_Runtime_Error &error);

/** @brief One runtime value.
 *
 *  The representation is intentionally compact and explicit. Values can be
 *  copied freely, which keeps the MVP simple for interpreter experiments and
 *  tests.
 */
struct Interpreter_Value
{
  Interpreter_Value_Kind kind = Interpreter_Value_Kind::Invalid;  ///< Active value kind.
  bool bool_value = false;                                        ///< Payload for Bool values.
  long long integer_value = 0;                                    ///< Payload for Int values.
  char character_value = '\0';                                    ///< Payload for Char values.
  std::string string_value;                                       ///< Payload for String values.
  std::unique_ptr<Interpreter_Tuple_Storage> tuple_storage;       ///< Payload for Tuple values.
  const Compiler_HIR_Function *function = nullptr;  ///< Payload for HIR function values.
  Interpreter_Environment *closure_environment
    = nullptr;  ///< Captured lexical parent for function values.
  Interpreter_Host_Function host_function = nullptr;  ///< Payload for host function values.
  void *host_user_data = nullptr;  ///< Embedding-specific payload for host functions.
  std::string callable_name;       ///< Stable debug name for callable values.

  /** @brief Builds a fresh value with no tuple storage. */
  Interpreter_Value() = default;

  /** @brief Deep-copies one runtime value. */
  Interpreter_Value(const Interpreter_Value &other);

  /** @brief Moves one runtime value. */
  Interpreter_Value(Interpreter_Value &&other) noexcept;

  /** @brief Releases tuple storage if present. */
  ~Interpreter_Value() noexcept;

  /** @brief Assigns one runtime value by deep copy. */
  Interpreter_Value &operator=(const Interpreter_Value &other);

  /** @brief Assigns one runtime value by move. */
  Interpreter_Value &operator=(Interpreter_Value &&other) noexcept;

  /** @brief Builds an invalid value placeholder. */
  static Interpreter_Value make_invalid()
  {
    return {};
  }

  /** @brief Builds the Unit value. */
  static Interpreter_Value make_unit()
  {
    Interpreter_Value value;
    value.kind = Interpreter_Value_Kind::Unit;
    return value;
  }

  /** @brief Builds a boolean runtime value. */
  static Interpreter_Value make_bool(const bool b)
  {
    Interpreter_Value value;
    value.kind = Interpreter_Value_Kind::Bool;
    value.bool_value = b;
    return value;
  }

  /** @brief Builds an integer runtime value. */
  static Interpreter_Value make_integer(const long long i)
  {
    Interpreter_Value value;
    value.kind = Interpreter_Value_Kind::Integer;
    value.integer_value = i;
    return value;
  }

  /** @brief Builds a string runtime value. */
  static Interpreter_Value make_string(std::string text)
  {
    Interpreter_Value value;
    value.kind = Interpreter_Value_Kind::String;
    value.string_value = std::move(text);
    return value;
  }

  /** @brief Builds a character runtime value. */
  static Interpreter_Value make_character(const char ch)
  {
    Interpreter_Value value;
    value.kind = Interpreter_Value_Kind::Character;
    value.character_value = ch;
    return value;
  }

  /** @brief Builds a tuple runtime value by copying `elements`. */
  static Interpreter_Value make_tuple(const DynArray<Interpreter_Value> &elements);

  /** @brief Builds a tuple runtime value by moving `elements`. */
  static Interpreter_Value make_tuple(DynArray<Interpreter_Value> &&elements);

  /** @brief Builds a callable value backed by HIR function code. */
  static Interpreter_Value make_function(const Compiler_HIR_Function *fn,
                                         Interpreter_Environment *env = nullptr)
  {
    Interpreter_Value value;
    value.kind = Interpreter_Value_Kind::Function;
    value.function = fn;
    value.closure_environment = env;
    value.callable_name = fn != nullptr ? fn->name : "<null-function>";
    return value;
  }

  /** @brief Builds a callable value backed by host code. */
  static Interpreter_Value make_host_function(std::string name,
                                              const Interpreter_Host_Function callback,
                                              void *user_data = nullptr)
  {
    Interpreter_Value value;
    value.kind = Interpreter_Value_Kind::Host_Function;
    value.host_function = callback;
    value.host_user_data = user_data;
    value.callable_name = std::move(name);
    return value;
  }

  /** @brief Returns whether this value is the Unit singleton. */
  [[nodiscard]] bool is_unit() const noexcept
  {
    return kind == Interpreter_Value_Kind::Unit;
  }

  /** @brief Returns whether this value is callable. */
  [[nodiscard]] bool is_callable() const noexcept
  {
    return kind == Interpreter_Value_Kind::Function or kind == Interpreter_Value_Kind::Host_Function;
  }

  /** @brief Returns the tuple payload.
   *
   *  @return Read-only tuple elements.
   *  @note Precondition: `kind == Tuple`.
   */
  [[nodiscard]] const DynArray<Interpreter_Value> &tuple_elements() const noexcept;
};

/** @brief Heap-owned tuple payload for recursive runtime values. */
struct Interpreter_Tuple_Storage
{
  DynArray<Interpreter_Value> elements;  ///< Tuple members in lexical order.

  Interpreter_Tuple_Storage() = default;

  explicit Interpreter_Tuple_Storage(const DynArray<Interpreter_Value> &values) : elements(values)
  {}

  explicit Interpreter_Tuple_Storage(DynArray<Interpreter_Value> &&values)
    : elements(std::move(values))
  {}
};

inline Interpreter_Value::Interpreter_Value(const Interpreter_Value &other)
  : kind(other.kind), bool_value(other.bool_value), integer_value(other.integer_value),
    character_value(other.character_value), string_value(other.string_value),
    function(other.function), closure_environment(other.closure_environment),
    host_function(other.host_function), host_user_data(other.host_user_data),
    callable_name(other.callable_name)
{
  if (other.tuple_storage != nullptr)
    tuple_storage = std::make_unique<Interpreter_Tuple_Storage>(*other.tuple_storage);
}

inline Interpreter_Value::Interpreter_Value(Interpreter_Value &&other) noexcept
  : kind(other.kind), bool_value(other.bool_value), integer_value(other.integer_value),
    character_value(other.character_value), string_value(std::move(other.string_value)),
    tuple_storage(std::move(other.tuple_storage)), function(other.function),
    closure_environment(other.closure_environment), host_function(other.host_function),
    host_user_data(other.host_user_data), callable_name(std::move(other.callable_name))
{
  other.kind = Interpreter_Value_Kind::Invalid;
}

inline Interpreter_Value::~Interpreter_Value() noexcept = default;

inline Interpreter_Value &Interpreter_Value::operator=(const Interpreter_Value &other)
{
  if (this == &other)
    return *this;

  tuple_storage.reset();

  kind = other.kind;
  bool_value = other.bool_value;
  integer_value = other.integer_value;
  character_value = other.character_value;
  string_value = other.string_value;
  function = other.function;
  closure_environment = other.closure_environment;
  host_function = other.host_function;
  host_user_data = other.host_user_data;
  callable_name = other.callable_name;

  if (other.tuple_storage != nullptr)
    tuple_storage = std::make_unique<Interpreter_Tuple_Storage>(*other.tuple_storage);
  return *this;
}

inline Interpreter_Value &Interpreter_Value::operator=(Interpreter_Value &&other) noexcept
{
  if (this == &other)
    return *this;

  kind = other.kind;
  bool_value = other.bool_value;
  integer_value = other.integer_value;
  character_value = other.character_value;
  string_value = std::move(other.string_value);
  tuple_storage = std::move(other.tuple_storage);
  function = other.function;
  closure_environment = other.closure_environment;
  host_function = other.host_function;
  host_user_data = other.host_user_data;
  callable_name = std::move(other.callable_name);

  other.kind = Interpreter_Value_Kind::Invalid;
  return *this;
}

inline Interpreter_Value Interpreter_Value::make_tuple(const DynArray<Interpreter_Value> &elements)
{
  Interpreter_Value value;
  value.kind = Interpreter_Value_Kind::Tuple;
  value.tuple_storage = std::make_unique<Interpreter_Tuple_Storage>(elements);
  return value;
}

inline Interpreter_Value Interpreter_Value::make_tuple(DynArray<Interpreter_Value> &&elements)
{
  Interpreter_Value value;
  value.kind = Interpreter_Value_Kind::Tuple;
  value.tuple_storage = std::make_unique<Interpreter_Tuple_Storage>(std::move(elements));
  return value;
}

inline const DynArray<Interpreter_Value> &Interpreter_Value::tuple_elements() const noexcept
{
  return tuple_storage->elements;
}

/** @brief Returns whether two runtime values compare equal.
 *
 *  Tuples are compared element-wise. Function values compare by the
 *  underlying function pointer and captured environment pointer.
 *
 *  @param lhs Left runtime value.
 *  @param rhs Right runtime value.
 *  @return `true` when the runtime values compare equal.
 */
inline bool interpreter_values_equal(const Interpreter_Value &lhs, const Interpreter_Value &rhs)
{
  if (lhs.kind != rhs.kind)
    return false;

  switch (lhs.kind)
    {
    case Interpreter_Value_Kind::Invalid:
      return false;

    case Interpreter_Value_Kind::Unit:
      return true;

    case Interpreter_Value_Kind::Bool:
      return lhs.bool_value == rhs.bool_value;

    case Interpreter_Value_Kind::Integer:
      return lhs.integer_value == rhs.integer_value;

    case Interpreter_Value_Kind::String:
      return lhs.string_value == rhs.string_value;

    case Interpreter_Value_Kind::Character:
      return lhs.character_value == rhs.character_value;

    case Interpreter_Value_Kind::Tuple:
      if (lhs.tuple_elements().size() != rhs.tuple_elements().size())
        return false;
      for (size_t i = 0; i < lhs.tuple_elements().size(); ++i)
        if (not interpreter_values_equal(lhs.tuple_elements().access(i),
                                         rhs.tuple_elements().access(i)))
          return false;
      return true;

    case Interpreter_Value_Kind::Function:
      return lhs.function == rhs.function and lhs.closure_environment == rhs.closure_environment;

    case Interpreter_Value_Kind::Host_Function:
      return lhs.host_function == rhs.host_function and lhs.host_user_data == rhs.host_user_data;
    }

  return false;
}

/** @brief Formats one runtime value deterministically for dumps and tests.
 *
 *  @param value Runtime value to render.
 *  @return Deterministic textual representation.
 */
inline std::string interpreter_value_to_string(const Interpreter_Value &value)
{
  auto escape_text = [](std::string_view text)
  {
    std::string escaped;
    escaped.reserve(text.size());
    for (const char ch : text)
      switch (ch)
        {
        case '\\':
          escaped += "\\\\";
          break;
        case '"':
          escaped += "\\\"";
          break;
        case '\'':
          escaped += "\\'";
          break;
        case '\n':
          escaped += "\\n";
          break;
        case '\r':
          escaped += "\\r";
          break;
        case '\t':
          escaped += "\\t";
          break;
        default:
          escaped.push_back(ch);
        }
    return escaped;
  };

  std::ostringstream out;
  switch (value.kind)
    {
    case Interpreter_Value_Kind::Invalid:
      out << "Invalid";
      break;

    case Interpreter_Value_Kind::Unit:
      out << "Unit";
      break;

    case Interpreter_Value_Kind::Bool:
      out << "Bool(" << (value.bool_value ? "true" : "false") << ")";
      break;

    case Interpreter_Value_Kind::Integer:
      out << "Int(" << value.integer_value << ")";
      break;

    case Interpreter_Value_Kind::String:
      out << "String(\"" << escape_text(value.string_value) << "\")";
      break;

    case Interpreter_Value_Kind::Character:
      out << "Char('" << escape_text(std::string(1, value.character_value)) << "')";
      break;

    case Interpreter_Value_Kind::Tuple:
      out << "Tuple(";
      for (size_t i = 0; i < value.tuple_elements().size(); ++i)
        {
          if (i > 0)
            out << ", ";
          out << interpreter_value_to_string(value.tuple_elements().access(i));
        }
      out << ')';
      break;

    case Interpreter_Value_Kind::Function:
      out << "Function(" << value.callable_name << ")";
      break;

    case Interpreter_Value_Kind::Host_Function:
      out << "HostFunction(" << value.callable_name << ")";
      break;
    }

  return out.str();
}

/** @brief One lexical binding inside an interpreter environment. */
struct Interpreter_Binding
{
  std::string name;         ///< Binding name.
  Interpreter_Value value;  ///< Stored runtime value.
};

/** @brief Nested runtime environment with lexical lookup semantics. */
class Interpreter_Environment
{
  Interpreter_Environment *parent_env = nullptr;
  DynArray<Interpreter_Binding> bindings_;

  Interpreter_Value *find_local(const std::string &name) noexcept
  {
    for (size_t i = bindings_.size(); i > 0; --i)
      if (bindings_.access(i - 1).name == name)
        return &bindings_.access(i - 1).value;
    return nullptr;
  }

  const Interpreter_Value *find_local(const std::string &name) const noexcept
  {
    for (size_t i = bindings_.size(); i > 0; --i)
      if (bindings_.access(i - 1).name == name)
        return &bindings_.access(i - 1).value;
    return nullptr;
  }

public:
  /** @brief Builds an environment with optional lexical parent. */
  explicit Interpreter_Environment(Interpreter_Environment *parent = nullptr) noexcept
    : parent_env(parent)
  {}

  /** @brief Changes the lexical parent for this environment. */
  void set_parent(Interpreter_Environment *parent) noexcept
  {
    parent_env = parent;
  }

  /** @brief Returns the lexical parent, if any. */
  Interpreter_Environment *parent() const noexcept
  {
    return parent_env;
  }

  /** @brief Drops all bindings from this environment only. */
  void clear() noexcept
  {
    bindings_.clear();
  }

  /** @brief Returns whether this environment is empty. */
  bool is_empty() const noexcept
  {
    return bindings_.is_empty();
  }

  /** @brief Returns the number of bindings in this environment. */
  size_t size() const noexcept
  {
    return bindings_.size();
  }

  /** @brief Returns read-only access to the local bindings.
   *
   *  The bindings are stored in insertion order, which keeps dumps
   *  deterministic.
   */
  const DynArray<Interpreter_Binding> &bindings() const noexcept
  {
    return bindings_;
  }

  /** @brief Defines one local binding.
   *
   *  @param name New binding name.
   *  @param value Runtime value to store.
   *  @return `false` if `name` already exists in this environment.
   */
  bool define(const std::string &name, const Interpreter_Value &value)
  {
    if (find_local(name) != nullptr)
      return false;
    bindings_.append({name, value});
    return true;
  }

  /** @brief Finds a visible binding recursively.
   *
   *  @param name Binding name to resolve.
   *  @return Mutable pointer to the bound value, or `nullptr`.
   */
  Interpreter_Value *lookup(const std::string &name) noexcept
  {
    if (auto *value = find_local(name); value != nullptr)
      return value;
    return parent_env != nullptr ? parent_env->lookup(name) : nullptr;
  }

  /** @brief Finds a visible binding recursively.
   *
   *  @param name Binding name to resolve.
   *  @return Read-only pointer to the bound value, or `nullptr`.
   */
  const Interpreter_Value *lookup(const std::string &name) const noexcept
  {
    if (const auto *value = find_local(name); value != nullptr)
      return value;
    return parent_env != nullptr ? parent_env->lookup(name) : nullptr;
  }

  /** @brief Assigns an existing visible binding.
   *
   *  @param name Binding name to update.
   *  @param value New runtime value.
   *  @return `true` if a visible binding was updated.
   */
  bool assign(const std::string &name, const Interpreter_Value &value)
  {
    if (auto *slot = find_local(name); slot != nullptr)
      {
        *slot = value;
        return true;
      }
    return parent_env != nullptr ? parent_env->assign(name, value) : false;
  }
};

/** @brief Runtime options for the HIR evaluator. */
struct Interpreter_Runtime_Options
{
  size_t max_call_depth = 256;  ///< Maximum nested HIR calls before `RUN010`.
};

/** @brief Result of evaluating one expression, statement, module, or call. */
struct Interpreter_Execution_Result
{
  Interpreter_Control_Flow_Kind flow
    = Interpreter_Control_Flow_Kind::None;  ///< Control-flow outcome.
  Interpreter_Value value
    = Interpreter_Value::make_unit();  ///< Produced runtime value when relevant.
  Interpreter_Runtime_Error error;     ///< Structured runtime failure when `flow == Runtime_Error`.

  /** @brief Returns whether this result represents a runtime failure. */
  [[nodiscard]] bool ok() const noexcept
  {
    return flow != Interpreter_Control_Flow_Kind::Runtime_Error;
  }

  /** @brief Returns whether this result carries non-local control flow. */
  [[nodiscard]] bool has_control_flow() const noexcept
  {
    return flow == Interpreter_Control_Flow_Kind::Return
        or flow == Interpreter_Control_Flow_Kind::Break
        or flow == Interpreter_Control_Flow_Kind::Continue;
  }
};

/** @brief Structured runtime for executing typed HIR.
 *
 *  Host functions live in a persistent prelude environment, while user
 *  functions and top-level `let` bindings are installed in the globals
 *  environment for the last evaluated module.
 */
class Interpreter_Runtime
{
  struct Interpreter_Call_Frame
  {
    const Compiler_HIR_Function *function = nullptr;
    Interpreter_Environment locals;
  };

  Interpreter_Runtime_Options options;
  Diagnostic_Engine *diagnostics = nullptr;
  Interpreter_Environment prelude;
  Interpreter_Environment globals;
  DynArray<Interpreter_Call_Frame> call_stack;

  static Interpreter_Execution_Result ok(const Interpreter_Value &value
                                         = Interpreter_Value::make_unit())
  {
    Interpreter_Execution_Result result;
    result.flow = Interpreter_Control_Flow_Kind::None;
    result.value = value;
    return result;
  }

  static Interpreter_Execution_Result control(const Interpreter_Control_Flow_Kind flow,
                                              const Interpreter_Value &value
                                              = Interpreter_Value::make_unit())
  {
    Interpreter_Execution_Result result;
    result.flow = flow;
    result.value = value;
    return result;
  }

  Interpreter_Execution_Result runtime_error(const Source_Span &span,
                                             const std::string &code,
                                             const std::string &message,
                                             const std::string &note = "",
                                             const std::string &help = "") const
  {
    if (diagnostics != nullptr)
      {
        auto builder = diagnostics->error(span, message).code(code);
        if (not note.empty())
          builder.note(note);
        if (not help.empty())
          builder.help(help);
        builder.emit();
      }

    Interpreter_Execution_Result result;
    result.flow = Interpreter_Control_Flow_Kind::Runtime_Error;
    result.value = Interpreter_Value::make_invalid();
    result.error = {code, message, span};
    return result;
  }

  static bool strip_quotes(std::string_view text, const char quote, std::string_view &body) noexcept
  {
    if (text.size() < 2)
      return false;
    if (text.front() != quote or text.back() != quote)
      return false;
    body = text.substr(1, text.size() - 2);
    return true;
  }

  static bool decode_escaped_text(const std::string_view body, std::string &out, std::string &error)
  {
    out.clear();
    out.reserve(body.size());

    for (size_t i = 0; i < body.size(); ++i)
      {
        const char ch = body[i];
        if (ch != '\\')
          {
            out.push_back(ch);
            continue;
          }

        if (i + 1 >= body.size())
          {
            error = "unterminated escape sequence";
            return false;
          }

        switch (const char esc = body[++i])
          {
          case '\\':
            out.push_back('\\');
            break;
          case '"':
            out.push_back('"');
            break;
          case '\'':
            out.push_back('\'');
            break;
          case 'n':
            out.push_back('\n');
            break;
          case 'r':
            out.push_back('\r');
            break;
          case 't':
            out.push_back('\t');
            break;
          case '0':
            out.push_back('\0');
            break;
          default:
            error = std::string("unsupported escape sequence \\") + esc;
            return false;
          }
      }

    return true;
  }

  Interpreter_Execution_Result parse_integer_literal(const Compiler_HIR_Constant_Expr *node) const
  {
    std::string digits;
    digits.reserve(node->text.size());
    for (const char ch : node->text)
      if (ch != '_')
        digits.push_back(ch);

    if (digits.empty())
      return runtime_error(node->span, "RUN012", "malformed integer literal");

    char *end = nullptr;
    errno = 0;
    const long long value = std::strtoll(digits.c_str(), &end, 10);
    if (errno == ERANGE or end == nullptr or *end != '\0')
      return runtime_error(node->span,
                           "RUN012",
                           "integer literal is out of range for the interpreter");

    return ok(Interpreter_Value::make_integer(value));
  }

  Interpreter_Execution_Result parse_string_literal(const Compiler_HIR_Constant_Expr *node) const
  {
    std::string_view body;
    if (not strip_quotes(node->text, '"', body))
      return runtime_error(node->span, "RUN012", "malformed string literal");

    std::string decoded;
    std::string error_message;
    if (not decode_escaped_text(body, decoded, error_message))
      return runtime_error(node->span, "RUN012", error_message);
    return ok(Interpreter_Value::make_string(std::move(decoded)));
  }

  Interpreter_Execution_Result parse_char_literal(const Compiler_HIR_Constant_Expr *node) const
  {
    std::string_view body;
    if (not strip_quotes(node->text, '\'', body))
      return runtime_error(node->span, "RUN012", "malformed character literal");

    std::string decoded;
    std::string error_message;
    if (not decode_escaped_text(body, decoded, error_message))
      return runtime_error(node->span, "RUN012", error_message);
    if (decoded.size() != 1)
      return runtime_error(node->span,
                           "RUN012",
                           "character literals must decode to exactly one character");

    return ok(Interpreter_Value::make_character(decoded[0]));
  }

  Interpreter_Execution_Result require_integer(const Interpreter_Value &value,
                                               const Source_Span &span,
                                               const std::string &context) const
  {
    if (value.kind == Interpreter_Value_Kind::Integer)
      return ok(value);

    return runtime_error(span,
                         "RUN008",
                         context + " requires Int but received "
                           + std::string(interpreter_value_kind_name(value.kind)));
  }

  Interpreter_Execution_Result require_bool(const Interpreter_Value &value,
                                            const Source_Span &span,
                                            const std::string &context) const
  {
    if (value.kind == Interpreter_Value_Kind::Bool)
      return ok(value);

    return runtime_error(span,
                         "RUN003",
                         context + " requires Bool but received "
                           + std::string(interpreter_value_kind_name(value.kind)));
  }

  Interpreter_Execution_Result evaluate_expr(const Compiler_HIR_Expr *expr,
                                             Interpreter_Environment &env)
  {
    if (expr == nullptr)
      return runtime_error({}, "RUN001", "null HIR expression");

    switch (expr->kind)
      {
      case Compiler_HIR_Expr_Kind::Invalid:
        return runtime_error(expr->span, "RUN001", "invalid HIR expression");

      case Compiler_HIR_Expr_Kind::Constant:
        {
          const auto *node = static_cast<const Compiler_HIR_Constant_Expr *>(expr);
          switch (node->constant_kind)
            {
            case Compiler_HIR_Constant_Kind::Unit:
              return ok(Interpreter_Value::make_unit());
            case Compiler_HIR_Constant_Kind::Bool:
              return ok(Interpreter_Value::make_bool(node->bool_value));
            case Compiler_HIR_Constant_Kind::Integer:
              return parse_integer_literal(node);
            case Compiler_HIR_Constant_Kind::String:
              return parse_string_literal(node);
            case Compiler_HIR_Constant_Kind::Character:
              return parse_char_literal(node);
            }
          return runtime_error(node->span, "RUN001", "unknown HIR constant kind");
        }

      case Compiler_HIR_Expr_Kind::Variable:
        {
          const auto *node = static_cast<const Compiler_HIR_Variable_Expr *>(expr);
          if (auto *value = env.lookup(node->name); value != nullptr)
            return ok(*value);
          return runtime_error(node->span,
                               "RUN002",
                               "unknown runtime identifier '" + node->name + "'");
        }

      case Compiler_HIR_Expr_Kind::Unary:
        {
          const auto *node = static_cast<const Compiler_HIR_Unary_Expr *>(expr);
          auto operand = evaluate_expr(node->operand, env);
          if (not operand.ok())
            return operand;

          switch (node->op)
            {
            case Compiler_Token_Kind::Plus:
              return require_integer(operand.value, node->span, "unary '+'");

            case Compiler_Token_Kind::Minus:
              {
                auto checked = require_integer(operand.value, node->span, "unary '-'");
                if (not checked.ok())
                  return checked;
                return ok(Interpreter_Value::make_integer(-checked.value.integer_value));
              }

            case Compiler_Token_Kind::Bang:
              {
                auto checked = require_bool(operand.value, node->span, "unary '!'");
                if (not checked.ok())
                  return checked;
                return ok(Interpreter_Value::make_bool(not checked.value.bool_value));
              }

            default:
              return runtime_error(node->span,
                                   "RUN008",
                                   "unsupported unary operator '"
                                     + std::string(compiler_token_kind_name(node->op)) + "'");
            }
        }

      case Compiler_HIR_Expr_Kind::Binary:
        {
          const auto *node = static_cast<const Compiler_HIR_Binary_Expr *>(expr);

          if (node->op == Compiler_Token_Kind::AndAnd or node->op == Compiler_Token_Kind::OrOr)
            {
              auto left = evaluate_expr(node->left, env);
              if (not left.ok())
                return left;
              auto left_bool = require_bool(left.value, node->span, "logical operator");
              if (not left_bool.ok())
                return left_bool;

              if (node->op == Compiler_Token_Kind::AndAnd and not left_bool.value.bool_value)
                return ok(Interpreter_Value::make_bool(false));
              if (node->op == Compiler_Token_Kind::OrOr and left_bool.value.bool_value)
                return ok(Interpreter_Value::make_bool(true));

              auto right = evaluate_expr(node->right, env);
              if (not right.ok())
                return right;
              auto right_bool = require_bool(right.value, node->span, "logical operator");
              if (not right_bool.ok())
                return right_bool;
              return ok(Interpreter_Value::make_bool(right_bool.value.bool_value));
            }

          const bool is_compound_assignment
            = node->op == Compiler_Token_Kind::PlusEq or node->op == Compiler_Token_Kind::MinusEq
           or node->op == Compiler_Token_Kind::StarEq or node->op == Compiler_Token_Kind::SlashEq
           or node->op == Compiler_Token_Kind::PercentEq;

          if (node->op == Compiler_Token_Kind::Assign or is_compound_assignment)
            {
              if (node->left == nullptr or node->left->kind != Compiler_HIR_Expr_Kind::Variable)
                return runtime_error(node->span,
                                     "RUN006",
                                     "assignment requires a variable on the left-hand side");
              const auto *lhs = static_cast<const Compiler_HIR_Variable_Expr *>(node->left);

              if (node->op == Compiler_Token_Kind::Assign)
                {
                  auto rhs = evaluate_expr(node->right, env);
                  if (not rhs.ok())
                    return rhs;

                  if (not env.assign(lhs->name, rhs.value))
                    return runtime_error(lhs->span,
                                         "RUN002",
                                         "unknown runtime identifier '" + lhs->name + "'");
                  return ok(rhs.value);
                }

              auto current_ptr = env.lookup(lhs->name);
              if (current_ptr == nullptr)
                return runtime_error(lhs->span,
                                     "RUN002",
                                     "unknown runtime identifier '" + lhs->name + "'");

              auto current = require_integer(*current_ptr, lhs->span, "compound assignment");
              if (not current.ok())
                return current;

              auto rhs = evaluate_expr(node->right, env);
              if (not rhs.ok())
                return rhs;
              auto rhs_int = require_integer(rhs.value, node->right->span, "compound assignment");
              if (not rhs_int.ok())
                return rhs_int;

              long long updated = current.value.integer_value;
              switch (node->op)
                {
                case Compiler_Token_Kind::PlusEq:
                  updated += rhs_int.value.integer_value;
                  break;
                case Compiler_Token_Kind::MinusEq:
                  updated -= rhs_int.value.integer_value;
                  break;
                case Compiler_Token_Kind::StarEq:
                  updated *= rhs_int.value.integer_value;
                  break;
                case Compiler_Token_Kind::SlashEq:
                  if (rhs_int.value.integer_value == 0)
                    return runtime_error(node->span, "RUN007", "division by zero");
                  updated /= rhs_int.value.integer_value;
                  break;
                case Compiler_Token_Kind::PercentEq:
                  if (rhs_int.value.integer_value == 0)
                    return runtime_error(node->span, "RUN007", "modulo by zero");
                  updated %= rhs_int.value.integer_value;
                  break;
                default:
                  return runtime_error(node->span,
                                       "RUN008",
                                       "unsupported compound assignment operator");
                }

              const auto result = Interpreter_Value::make_integer(updated);
              (void) env.assign(lhs->name, result);
              return ok(result);
            }

          auto left = evaluate_expr(node->left, env);
          if (not left.ok())
            return left;
          auto right = evaluate_expr(node->right, env);
          if (not right.ok())
            return right;

          switch (node->op)
            {
            case Compiler_Token_Kind::Plus:
            case Compiler_Token_Kind::Minus:
            case Compiler_Token_Kind::Star:
            case Compiler_Token_Kind::Slash:
            case Compiler_Token_Kind::Percent:
            case Compiler_Token_Kind::Amp:
            case Compiler_Token_Kind::Pipe:
            case Compiler_Token_Kind::Caret:
              {
                auto lhs = require_integer(left.value, node->left->span, "binary operator");
                if (not lhs.ok())
                  return lhs;
                auto rhs = require_integer(right.value, node->right->span, "binary operator");
                if (not rhs.ok())
                  return rhs;

                switch (node->op)
                  {
                  case Compiler_Token_Kind::Plus:
                    return ok(Interpreter_Value::make_integer(lhs.value.integer_value
                                                              + rhs.value.integer_value));
                  case Compiler_Token_Kind::Minus:
                    return ok(Interpreter_Value::make_integer(lhs.value.integer_value
                                                              - rhs.value.integer_value));
                  case Compiler_Token_Kind::Star:
                    return ok(Interpreter_Value::make_integer(lhs.value.integer_value
                                                              * rhs.value.integer_value));
                  case Compiler_Token_Kind::Slash:
                    if (rhs.value.integer_value == 0)
                      return runtime_error(node->span, "RUN007", "division by zero");
                    return ok(Interpreter_Value::make_integer(lhs.value.integer_value
                                                              / rhs.value.integer_value));
                  case Compiler_Token_Kind::Percent:
                    if (rhs.value.integer_value == 0)
                      return runtime_error(node->span, "RUN007", "modulo by zero");
                    return ok(Interpreter_Value::make_integer(lhs.value.integer_value
                                                              % rhs.value.integer_value));
                  case Compiler_Token_Kind::Amp:
                    return ok(Interpreter_Value::make_integer(lhs.value.integer_value
                                                              & rhs.value.integer_value));
                  case Compiler_Token_Kind::Pipe:
                    return ok(Interpreter_Value::make_integer(lhs.value.integer_value
                                                              | rhs.value.integer_value));
                  case Compiler_Token_Kind::Caret:
                    return ok(Interpreter_Value::make_integer(lhs.value.integer_value
                                                              ^ rhs.value.integer_value));
                  default:
                    return runtime_error(node->span,
                                         "RUN008",
                                         "unsupported arithmetic operator '"
                                           + std::string(compiler_token_kind_name(node->op)) + "'");
                  }
              }

            case Compiler_Token_Kind::Less:
            case Compiler_Token_Kind::LessEq:
            case Compiler_Token_Kind::Greater:
            case Compiler_Token_Kind::GreaterEq:
              {
                auto lhs = require_integer(left.value, node->left->span, "comparison");
                if (not lhs.ok())
                  return lhs;
                auto rhs = require_integer(right.value, node->right->span, "comparison");
                if (not rhs.ok())
                  return rhs;

                bool result = false;
                switch (node->op)
                  {
                  case Compiler_Token_Kind::Less:
                    result = lhs.value.integer_value < rhs.value.integer_value;
                    break;
                  case Compiler_Token_Kind::LessEq:
                    result = lhs.value.integer_value <= rhs.value.integer_value;
                    break;
                  case Compiler_Token_Kind::Greater:
                    result = lhs.value.integer_value > rhs.value.integer_value;
                    break;
                  case Compiler_Token_Kind::GreaterEq:
                    result = lhs.value.integer_value >= rhs.value.integer_value;
                    break;
                  default:
                    break;
                  }

                return ok(Interpreter_Value::make_bool(result));
              }

            case Compiler_Token_Kind::EqEq:
              return ok(
                Interpreter_Value::make_bool(interpreter_values_equal(left.value, right.value)));

            case Compiler_Token_Kind::NotEq:
              return ok(
                Interpreter_Value::make_bool(not interpreter_values_equal(left.value, right.value)));

            default:
              return runtime_error(node->span,
                                   "RUN008",
                                   "unsupported binary operator '"
                                     + std::string(compiler_token_kind_name(node->op)) + "'");
            }
        }

      case Compiler_HIR_Expr_Kind::Call:
        {
          const auto *node = static_cast<const Compiler_HIR_Call_Expr *>(expr);
          auto callee = evaluate_expr(node->callee, env);
          if (not callee.ok())
            return callee;

          DynArray<Interpreter_Value> arguments;
          for (size_t i = 0; i < node->arguments.size(); ++i)
            {
              auto arg = evaluate_expr(node->arguments.access(i), env);
              if (not arg.ok())
                return arg;
              arguments.append(arg.value);
            }

          return call_value(callee.value, arguments, node->span);
        }
      }

    return runtime_error(expr->span, "RUN001", "unknown HIR expression kind");
  }

  Interpreter_Execution_Result evaluate_stmt(const Compiler_HIR_Stmt *stmt,
                                             Interpreter_Environment &env)
  {
    if (stmt == nullptr)
      return runtime_error({}, "RUN001", "null HIR statement");

    switch (stmt->kind)
      {
      case Compiler_HIR_Stmt_Kind::Invalid:
        return runtime_error(stmt->span, "RUN001", "invalid HIR statement");

      case Compiler_HIR_Stmt_Kind::Eval:
        {
          const auto *node = static_cast<const Compiler_HIR_Eval_Stmt *>(stmt);
          auto value = evaluate_expr(node->expr, env);
          if (not value.ok())
            return value;
          return ok(value.value);
        }

      case Compiler_HIR_Stmt_Kind::Let:
        {
          const auto *node = static_cast<const Compiler_HIR_Let_Stmt *>(stmt);
          Interpreter_Value value = Interpreter_Value::make_unit();
          if (node->initializer != nullptr)
            {
              auto init = evaluate_expr(node->initializer, env);
              if (not init.ok())
                return init;
              value = init.value;
            }

          if (not env.define(node->name, value))
            return runtime_error(node->span,
                                 "RUN011",
                                 "duplicate runtime binding '" + node->name + "'");
          return ok(value);
        }

      case Compiler_HIR_Stmt_Kind::Return:
        {
          const auto *node = static_cast<const Compiler_HIR_Return_Stmt *>(stmt);
          if (node->value == nullptr)
            return control(Interpreter_Control_Flow_Kind::Return, Interpreter_Value::make_unit());
          auto value = evaluate_expr(node->value, env);
          if (not value.ok())
            return value;
          return control(Interpreter_Control_Flow_Kind::Return, value.value);
        }

      case Compiler_HIR_Stmt_Kind::Block:
        {
          const auto *node = static_cast<const Compiler_HIR_Block_Stmt *>(stmt);
          Interpreter_Environment block_env(&env);
          for (size_t i = 0; i < node->statements.size(); ++i)
            {
              auto result = evaluate_stmt(node->statements.access(i), block_env);
              if (result.flow != Interpreter_Control_Flow_Kind::None)
                return result;
            }
          return ok();
        }

      case Compiler_HIR_Stmt_Kind::If:
        {
          const auto *node = static_cast<const Compiler_HIR_If_Stmt *>(stmt);
          auto cond = evaluate_expr(node->condition, env);
          if (not cond.ok())
            return cond;
          auto checked = require_bool(cond.value, node->condition->span, "if condition");
          if (not checked.ok())
            return checked;

          if (checked.value.bool_value)
            return node->then_branch != nullptr ? evaluate_stmt(node->then_branch, env) : ok();

          return node->else_branch != nullptr ? evaluate_stmt(node->else_branch, env) : ok();
        }

      case Compiler_HIR_Stmt_Kind::While:
        {
          const auto *node = static_cast<const Compiler_HIR_While_Stmt *>(stmt);
          while (true)
            {
              auto cond = evaluate_expr(node->condition, env);
              if (not cond.ok())
                return cond;
              auto checked = require_bool(cond.value, node->condition->span, "while condition");
              if (not checked.ok())
                return checked;
              if (not checked.value.bool_value)
                return ok();

              auto body = node->body != nullptr ? evaluate_stmt(node->body, env) : ok();
              if (body.flow == Interpreter_Control_Flow_Kind::Runtime_Error
                  or body.flow == Interpreter_Control_Flow_Kind::Return)
                return body;
              if (body.flow == Interpreter_Control_Flow_Kind::Break)
                return ok();
              if (body.flow == Interpreter_Control_Flow_Kind::Continue
                  or body.flow == Interpreter_Control_Flow_Kind::None)
                continue;
            }
        }

      case Compiler_HIR_Stmt_Kind::Break:
        return control(Interpreter_Control_Flow_Kind::Break);

      case Compiler_HIR_Stmt_Kind::Continue:
        return control(Interpreter_Control_Flow_Kind::Continue);
      }

    return runtime_error(stmt->span, "RUN001", "unknown HIR statement kind");
  }

  Interpreter_Execution_Result call_hir_function(const Interpreter_Value &callee,
                                                 const DynArray<Interpreter_Value> &arguments,
                                                 const Source_Span &span)
  {
    const auto *function = callee.function;
    if (function == nullptr)
      return runtime_error(span, "RUN004", "null HIR function");

    if (function->parameters.size() != arguments.size())
      return runtime_error(span,
                           "RUN005",
                           "function '" + function->name + "' expects "
                             + std::to_string(function->parameters.size())
                             + " arguments but received " + std::to_string(arguments.size()));

    if (call_stack.size() >= options.max_call_depth)
      return runtime_error(span,
                           "RUN010",
                           "maximum call depth exceeded while calling '" + function->name + "'");

    Interpreter_Call_Frame frame;
    frame.function = function;
    frame.locals.set_parent(callee.closure_environment != nullptr ? callee.closure_environment
                                                                  : &globals);

    for (size_t i = 0; i < function->parameters.size(); ++i)
      (void) frame.locals.define(function->parameters.access(i).name, arguments.access(i));

    call_stack.append(std::move(frame));
    auto result = evaluate_stmt(function->body, call_stack.access(call_stack.size() - 1).locals);
    (void) call_stack.pop();

    if (result.flow == Interpreter_Control_Flow_Kind::Return)
      return ok(result.value);
    if (result.flow == Interpreter_Control_Flow_Kind::Break
        or result.flow == Interpreter_Control_Flow_Kind::Continue)
      return runtime_error(function->span,
                           "RUN009",
                           "control-flow escaped function '" + function->name + "'");
    return result;
  }

  Interpreter_Execution_Result call_host_function(const Interpreter_Value &callee,
                                                  const DynArray<Interpreter_Value> &arguments,
                                                  const Source_Span &span) const
  {
    if (callee.host_function == nullptr)
      return runtime_error(span, "RUN004", "null host function");

    Interpreter_Value result = Interpreter_Value::make_invalid();
    Interpreter_Runtime_Error error;
    if (callee.host_function(callee.host_user_data, arguments, result, error))
      return ok(result);

    auto code = error.code.empty() ? std::string("RUN004") : error.code;
    auto message = error.message.empty() ? "host function '" + callee.callable_name + "' failed"
                                         : error.message;
    return runtime_error(error.span.is_valid() ? error.span : span, code, message);
  }

  Interpreter_Execution_Result call_value(const Interpreter_Value &callee,
                                          const DynArray<Interpreter_Value> &arguments,
                                          const Source_Span &span)
  {
    switch (callee.kind)
      {
      case Interpreter_Value_Kind::Function:
        return call_hir_function(callee, arguments, span);

      case Interpreter_Value_Kind::Host_Function:
        return call_host_function(callee, arguments, span);

      default:
        return runtime_error(span,
                             "RUN004",
                             "attempted to call a non-callable value of kind "
                               + std::string(interpreter_value_kind_name(callee.kind)));
      }
  }

  bool install_module_function(const Compiler_HIR_Function *function)
  {
    if (function == nullptr)
      return false;
    return globals.define(function->name, Interpreter_Value::make_function(function, &globals));
  }

public:
  /** @brief Builds a fresh interpreter runtime.
   *
   *  @param dx Optional diagnostic sink for mirrored runtime errors.
   *  @param opts Runtime options such as maximum call depth.
   */
  explicit Interpreter_Runtime(Diagnostic_Engine *dx = nullptr,
                               const Interpreter_Runtime_Options &opts = {}) noexcept
    : options(opts), diagnostics(dx), prelude(nullptr), globals(&prelude)
  {}

  /** @brief Sets the optional diagnostic sink for runtime failures. */
  void set_diagnostics(Diagnostic_Engine *dx) noexcept
  {
    diagnostics = dx;
  }

  /** @brief Returns the persistent prelude environment. */
  const Interpreter_Environment &prelude_environment() const noexcept
  {
    return prelude;
  }

  /** @brief Returns the globals environment for the last evaluated module. */
  const Interpreter_Environment &global_environment() const noexcept
  {
    return globals;
  }

  /** @brief Returns the current call depth. */
  size_t call_depth() const noexcept
  {
    return call_stack.size();
  }

  /** @brief Clears globals and the call stack while preserving host bindings. */
  void reset() noexcept
  {
    globals.clear();
    globals.set_parent(&prelude);
    call_stack.clear();
  }

  /** @brief Defines one host function in the persistent prelude.
   *
   *  @param name Host-visible callable name.
   *  @param callback Host callback.
   *  @param user_data Opaque embedding-owned pointer forwarded to `callback`.
   *  @return `false` if `name` already exists in the prelude.
   */
  bool bind_host_function(const std::string &name,
                          const Interpreter_Host_Function callback,
                          void *user_data = nullptr)
  {
    ah_runtime_error_unless(callback != nullptr)
      << "Interpreter_Runtime::bind_host_function(): callback is null";
    return prelude.define(name, Interpreter_Value::make_host_function(name, callback, user_data));
  }

  /** @brief Evaluates one HIR module and leaves its globals installed.
   *
   *  Function declarations are installed first to support recursion and
   *  mutual recursion. Top-level statements are then evaluated in order.
   *
   *  @param module HIR module to evaluate.
   *  @return Structured runtime result for the module evaluation.
   */
  Interpreter_Execution_Result evaluate_module(const Compiler_HIR_Module *module)
  {
    if (module == nullptr)
      return runtime_error({}, "RUN001", "null HIR module");

    reset();

    for (size_t i = 0; i < module->functions.size(); ++i)
      if (not install_module_function(module->functions.access(i)))
        return runtime_error(module->functions.access(i)->span,
                             "RUN011",
                             "duplicate global function '" + module->functions.access(i)->name
                               + "'");

    for (size_t i = 0; i < module->statements.size(); ++i)
      {
        auto result = evaluate_stmt(module->statements.access(i), globals);
        if (result.flow == Interpreter_Control_Flow_Kind::Runtime_Error)
          return result;
        if (result.flow == Interpreter_Control_Flow_Kind::Return
            or result.flow == Interpreter_Control_Flow_Kind::Break
            or result.flow == Interpreter_Control_Flow_Kind::Continue)
          return runtime_error(module->statements.access(i)->span,
                               "RUN009",
                               "top-level control-flow statement is not allowed");
      }

    return ok();
  }

  /** @brief Calls one visible function by name.
   *
   *  The name is resolved first in globals, then in the persistent prelude.
   *
   *  @param name Function or host function name.
   *  @param arguments Positional arguments.
   *  @return Structured call result.
   */
  Interpreter_Execution_Result call(const std::string &name,
                                    const DynArray<Interpreter_Value> &arguments)
  {
    if (auto *value = globals.lookup(name); value != nullptr)
      return call_value(*value, arguments, {});
    if (auto *value = prelude.lookup(name); value != nullptr)
      return call_value(*value, arguments, {});
    return runtime_error({}, "RUN002", "unknown runtime identifier '" + name + "'");
  }
};

/** @brief Dumps one environment deterministically.
 *
 *  @param env Environment to render.
 *  @return Stable textual dump of local bindings.
 */
inline std::string interpreter_dump_environment(const Interpreter_Environment &env)
{
  std::ostringstream out;
  out << "Environment\n";
  for (size_t i = 0; i < env.bindings().size(); ++i)
    {
      const auto &binding = env.bindings().access(i);
      out << "  " << binding.name << " = " << interpreter_value_to_string(binding.value) << '\n';
    }
  return out.str();
}

/** @brief Dumps the globals currently installed in one runtime. */
inline std::string interpreter_dump_globals(const Interpreter_Runtime &runtime)
{
  std::ostringstream out;
  out << "Globals\n";
  const auto &env = runtime.global_environment();
  for (size_t i = 0; i < env.bindings().size(); ++i)
    {
      const auto &binding = env.bindings().access(i);
      out << "  " << binding.name << " = " << interpreter_value_to_string(binding.value) << '\n';
    }
  return out.str();
}
}  // namespace Aleph

#endif
