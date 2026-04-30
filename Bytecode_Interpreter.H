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

/** @file Bytecode_Interpreter.H
 *  @brief Simple portable VM for executing the register bytecode in `Bytecode.H`.
 *
 *  This header closes the first end-to-end execution loop over the compiler
 *  platform:
 *
 *  `HIR -> IR -> Bytecode -> Bytecode_Interpreter`
 *
 *  The VM is intentionally conservative and explicit:
 *
 *  - registers are addressed by the ids already emitted by `Bytecode.H`
 *  - locals and globals remain explicit slot arrays instead of hidden stack
 *    memory
 *  - runtime failures are reported as structured values instead of exceptions
 *  - host callbacks from `Interpreter_Runtime.H` can be injected into globals
 *  - bytecode function references are kept distinct from HIR function values
 *
 *  The execution model is deliberately small. It is meant to be easy to read,
 *  validate, and extend before any future optimized backend or threaded VM.
 *
 *  @ingroup Utilities
 */

#ifndef BYTECODE_INTERPRETER_H
#define BYTECODE_INTERPRETER_H

#include <sstream>
#include <string>
#include <utility>

#include <Bytecode.H>
#include <Interpreter_Runtime.H>

namespace Aleph
{
  /** @brief Runtime categories that can live inside bytecode registers/slots.
   *
   *  Bytecode execution needs one extra case that `Interpreter_Value` does not
   *  model: references to bytecode functions addressed by `function_id`.
   *  Those references are not interchangeable with HIR callables, so the VM
   *  wraps both concepts in a distinct tagged value.
   */
  enum class Compiler_Bytecode_Value_Kind
  {
    Invalid,      ///< Uninitialized register/slot or explicit invalid placeholder.
    Value,        ///< Regular runtime payload stored as `Interpreter_Value`.
    Function_Ref  ///< Reference to one bytecode function in the current module.
  };

  /** @brief Final status produced by one VM execution request. */
  enum class Compiler_Bytecode_Completion_Kind
  {
    Returned,      ///< Execution finished with an explicit `Return`.
    Halted,        ///< Execution reached `Halt` and therefore produced `Unit`.
    Runtime_Error  ///< Execution failed with a structured runtime error.
  };

  /** @brief Stable debug name for one bytecode runtime value kind. */
  inline const char *
  compiler_bytecode_value_kind_name(const Compiler_Bytecode_Value_Kind kind) noexcept
  {
    switch (kind)
      {
      case Compiler_Bytecode_Value_Kind::Invalid:
        return "Invalid";
      case Compiler_Bytecode_Value_Kind::Value:
        return "Value";
      case Compiler_Bytecode_Value_Kind::Function_Ref:
        return "FunctionRef";
      }

    return "Unknown";
  }

  /** @brief Stable debug name for one execution completion kind. */
  inline const char *
  compiler_bytecode_completion_kind_name(const Compiler_Bytecode_Completion_Kind kind) noexcept
  {
    switch (kind)
      {
      case Compiler_Bytecode_Completion_Kind::Returned:
        return "Returned";
      case Compiler_Bytecode_Completion_Kind::Halted:
        return "Halted";
      case Compiler_Bytecode_Completion_Kind::Runtime_Error:
        return "RuntimeError";
      }

    return "Unknown";
  }

  /** @brief One value stored in bytecode registers, locals, or globals.
   *
   *  The VM reuses `Interpreter_Value` for ordinary runtime data such as
   *  integers, booleans, strings, tuples, or host functions. Bytecode function
   *  references remain separate so the VM never confuses bytecode callables
   *  with HIR function values.
   */
  struct Compiler_Bytecode_Value
  {
    Compiler_Bytecode_Value_Kind kind = Compiler_Bytecode_Value_Kind::Invalid; ///< Active payload category.
    Interpreter_Value runtime_value = Interpreter_Value::make_invalid(); ///< Wrapped scalar/tuple/host-function value.
    Compiler_Bytecode_Function_Id function_id = compiler_bytecode_invalid_id(); ///< Referenced bytecode function when `kind == Function_Ref`.

    /** @brief Builds an invalid placeholder value. */
    static Compiler_Bytecode_Value
    make_invalid()
    {
      return {};
    }

    /** @brief Wraps one regular runtime value. */
    static Compiler_Bytecode_Value
    make_value(const Interpreter_Value & value)
    {
      Compiler_Bytecode_Value result;
      result.kind = Compiler_Bytecode_Value_Kind::Value;
      result.runtime_value = value;
      return result;
    }

    /** @brief Wraps one regular runtime value by move. */
    static Compiler_Bytecode_Value
    make_value(Interpreter_Value && value)
    {
      Compiler_Bytecode_Value result;
      result.kind = Compiler_Bytecode_Value_Kind::Value;
      result.runtime_value = std::move(value);
      return result;
    }

    /** @brief Builds the Unit singleton. */
    static Compiler_Bytecode_Value
    make_unit()
    {
      return make_value(Interpreter_Value::make_unit());
    }

    /** @brief Builds a boolean runtime value. */
    static Compiler_Bytecode_Value
    make_bool(const bool value)
    {
      return make_value(Interpreter_Value::make_bool(value));
    }

    /** @brief Builds an integer runtime value. */
    static Compiler_Bytecode_Value
    make_integer(const long long value)
    {
      return make_value(Interpreter_Value::make_integer(value));
    }

    /** @brief Builds a string runtime value. */
    static Compiler_Bytecode_Value
    make_string(std::string text)
    {
      return make_value(Interpreter_Value::make_string(std::move(text)));
    }

    /** @brief Builds a character runtime value. */
    static Compiler_Bytecode_Value
    make_character(const char ch)
    {
      return make_value(Interpreter_Value::make_character(ch));
    }

    /** @brief Builds a callable value backed by host code.
     *
     *  This is useful for embeddings that want to expose helper functions to
     *  bytecode by preloading globals before execution.
     */
    static Compiler_Bytecode_Value
    make_host_function(std::string name,
                       const Interpreter_Host_Function callback,
                       void * user_data = nullptr)
    {
      return make_value(Interpreter_Value::make_host_function(std::move(name),
                                                              callback,
                                                              user_data));
    }

    /** @brief Builds a reference to one bytecode function in the current module. */
    static Compiler_Bytecode_Value
    make_function_ref(const Compiler_Bytecode_Function_Id id)
    {
      Compiler_Bytecode_Value result;
      result.kind = Compiler_Bytecode_Value_Kind::Function_Ref;
      result.function_id = id;
      return result;
    }

    /** @brief Returns whether this value is invalid. */
    [[nodiscard]] bool
    is_invalid() const noexcept
    {
      return kind == Compiler_Bytecode_Value_Kind::Invalid;
    }

    /** @brief Returns whether this value wraps an `Interpreter_Value`. */
    [[nodiscard]] bool
    is_runtime_value() const noexcept
    {
      return kind == Compiler_Bytecode_Value_Kind::Value;
    }

    /** @brief Returns whether this value is a bytecode function reference. */
    [[nodiscard]] bool
    is_function_ref() const noexcept
    {
      return kind == Compiler_Bytecode_Value_Kind::Function_Ref;
    }
  };

  /** @brief Structured runtime error produced by the bytecode VM. */
  struct Compiler_Bytecode_Runtime_Error
  {
    std::string code; ///< Stable runtime code such as `BCV013`.
    std::string message; ///< Human-readable runtime message.
    std::string function_name; ///< Function active at the failure point, when known.
    Compiler_Bytecode_PC pc = compiler_bytecode_invalid_id(); ///< Program counter active at the failure point, when known.
    Source_Span span; ///< Source span associated with the current instruction, when known.

    /** @brief Returns whether this error object is populated. */
    [[nodiscard]] bool
    has_error() const noexcept
    {
      return not code.empty() or not message.empty();
    }
  };

  /** @brief Result of one bytecode execution request. */
  struct Compiler_Bytecode_Execution_Result
  {
    Compiler_Bytecode_Completion_Kind kind = Compiler_Bytecode_Completion_Kind::Halted; ///< Final completion category.
    Compiler_Bytecode_Value value = Compiler_Bytecode_Value::make_unit(); ///< Produced return value or final `Unit`.
    Compiler_Bytecode_Runtime_Error error; ///< Structured failure when `kind == Runtime_Error`.
    size_t executed_instructions = 0; ///< Number of bytecode instructions observed by the VM.

    /** @brief Returns whether execution completed without runtime errors. */
    [[nodiscard]] bool
    ok() const noexcept
    {
      return kind != Compiler_Bytecode_Completion_Kind::Runtime_Error;
    }

    /** @brief Builds a successful return result. */
    static Compiler_Bytecode_Execution_Result
    returned(const Compiler_Bytecode_Value & value,
             const size_t executed_instructions)
    {
      Compiler_Bytecode_Execution_Result result;
      result.kind = Compiler_Bytecode_Completion_Kind::Returned;
      result.value = value;
      result.executed_instructions = executed_instructions;
      return result;
    }

    /** @brief Builds a successful halt result. */
    static Compiler_Bytecode_Execution_Result
    halted(const Compiler_Bytecode_Value & value,
           const size_t executed_instructions)
    {
      Compiler_Bytecode_Execution_Result result;
      result.kind = Compiler_Bytecode_Completion_Kind::Halted;
      result.value = value;
      result.executed_instructions = executed_instructions;
      return result;
    }

    /** @brief Builds a failed runtime result. */
    static Compiler_Bytecode_Execution_Result
    runtime_error(const Compiler_Bytecode_Runtime_Error & error,
                  const size_t executed_instructions)
    {
      Compiler_Bytecode_Execution_Result result;
      result.kind = Compiler_Bytecode_Completion_Kind::Runtime_Error;
      result.error = error;
      result.executed_instructions = executed_instructions;
      result.value = Compiler_Bytecode_Value::make_invalid();
      return result;
    }
  };

  /** @brief Returns whether two bytecode values compare equal.
   *
   *  Equality is structural for wrapped `Interpreter_Value` payloads and exact
   *  by function id for bytecode function references.
   */
  inline bool
  compiler_bytecode_values_equal(const Compiler_Bytecode_Value & lhs,
                                 const Compiler_Bytecode_Value & rhs)
  {
    if (lhs.kind != rhs.kind)
      return false;

    switch (lhs.kind)
      {
      case Compiler_Bytecode_Value_Kind::Invalid:
        return true;
      case Compiler_Bytecode_Value_Kind::Value:
        return interpreter_values_equal(lhs.runtime_value, rhs.runtime_value);
      case Compiler_Bytecode_Value_Kind::Function_Ref:
        return lhs.function_id == rhs.function_id;
      }

    return false;
  }

  /** @brief Renders one bytecode value deterministically.
   *
   *  @param value Value to render.
   *  @param module Optional module used to enrich bytecode function references
   *         with function names.
   *  @return Stable human-readable rendering.
   */
  inline std::string
  compiler_bytecode_value_to_string(const Compiler_Bytecode_Value & value,
                                    const Compiler_Bytecode_Module * module = nullptr)
  {
    switch (value.kind)
      {
      case Compiler_Bytecode_Value_Kind::Invalid:
        return "Invalid";

      case Compiler_Bytecode_Value_Kind::Value:
        return interpreter_value_to_string(value.runtime_value);

      case Compiler_Bytecode_Value_Kind::Function_Ref:
        {
          std::ostringstream out;
          out << "BytecodeFunction(f" << value.function_id;
          if (module != nullptr and value.function_id < module->functions.size())
            out << ", " << module->functions.access(value.function_id)->name;
          out << ')';
          return out.str();
        }
      }

    return "Unknown";
  }

  /** @brief Dumps the current global state associated with one bytecode module.
   *
   *  @param module Bytecode module that owns the slot layout.
   *  @param globals Runtime values stored in each global slot.
   *  @return Deterministic plain-text dump.
   */
  inline std::string
  compiler_dump_bytecode_globals(const Compiler_Bytecode_Module * module,
                                 const DynArray<Compiler_Bytecode_Value> & globals)
  {
    std::ostringstream out;
    if (module == nullptr)
      {
        out << "<null-bytecode-globals>\n";
        return out.str();
      }

    out << "BytecodeGlobals\n";
    if (module->global_slots.is_empty())
      {
        out << "  <none>\n";
        return out.str();
      }

    for (size_t i = 0; i < module->global_slots.size(); ++i)
      {
        const auto & slot = module->global_slots.access(i);
        out << "  g" << slot.id << ' ' << slot.name << " = ";
        if (slot.id < globals.size())
          out << compiler_bytecode_value_to_string(globals.access(slot.id), module);
        else
          out << "<missing>";
        out << '\n';
      }
    return out.str();
  }

  namespace Compiler_Bytecode_Interpreter_Detail
  {
    inline void
    reset_values(DynArray<Compiler_Bytecode_Value> & values,
                 const size_t count,
                 const Compiler_Bytecode_Value & fill = Compiler_Bytecode_Value::make_invalid())
    {
      values.clear();
      for (size_t i = 0; i < count; ++i)
        values.append(fill);
    }

    inline size_t
    count_parameters(const Compiler_Bytecode_Function & function) noexcept
    {
      size_t count = 0;
      for (size_t i = 0; i < function.local_slots.size(); ++i)
        if (function.local_slots.access(i).kind == Compiler_IR_Slot_Kind::Parameter)
          ++count;
      return count;
    }

    inline bool
    constant_to_value(const Compiler_Bytecode_Constant & constant,
                      Compiler_Bytecode_Value & value,
                      std::string & error)
    {
      switch (constant.kind)
        {
        case Compiler_Bytecode_Constant_Kind::Unit:
          value = Compiler_Bytecode_Value::make_unit();
          return true;

        case Compiler_Bytecode_Constant_Kind::Boolean:
          value = Compiler_Bytecode_Value::make_bool(constant.bool_value);
          return true;

        case Compiler_Bytecode_Constant_Kind::Integer:
          value = Compiler_Bytecode_Value::make_integer(constant.integer_value);
          return true;

        case Compiler_Bytecode_Constant_Kind::String:
          {
            std::string decoded = constant.text;
            if (decoded.size() >= 2 and decoded.front() == '"' and decoded.back() == '"')
              decoded = decoded.substr(1, decoded.size() - 2);
            value = Compiler_Bytecode_Value::make_string(std::move(decoded));
            return true;
          }

        case Compiler_Bytecode_Constant_Kind::Character:
          value = Compiler_Bytecode_Value::make_character(constant.char_value);
          return true;

        case Compiler_Bytecode_Constant_Kind::Raw_Text:
          error = "unsupported raw constant payload '" + constant.text + "'";
          return false;
        }

      error = "unknown constant kind";
      return false;
    }
  }

  /** @brief Small portable VM for executing one `Compiler_Bytecode_Module`.
   *
   *  The VM owns mutable runtime state only:
   *
   *  - global slot values
   *  - the active call stack
   *  - transient register/local arrays per frame
   *
   *  The bytecode module itself remains immutable and can be shared.
   */
  class Compiler_Bytecode_VM
  {
    struct Frame
    {
      const Compiler_Bytecode_Function * function = nullptr;
      Compiler_Bytecode_PC pc = compiler_bytecode_invalid_id();
      DynArray<Compiler_Bytecode_Value> registers;
      DynArray<Compiler_Bytecode_Value> locals;
      Compiler_Bytecode_PC caller_resume_pc = compiler_bytecode_invalid_id();
      Compiler_Bytecode_Register_Id caller_destination = 0;
      bool returns_to_caller = false;
    };

    const Compiler_Bytecode_Module * bytecode_module = nullptr;
    DynArray<Compiler_Bytecode_Value> globals;
    DynArray<Frame> call_stack;

    [[nodiscard]] const Frame &
    current_frame() const
    {
      return call_stack.get_last();
    }

    Frame &
    current_frame()
    {
      return call_stack.get_last();
    }

    [[nodiscard]] Compiler_Bytecode_Execution_Result
    make_error(const std::string & code,
               const std::string & message,
               const size_t executed_instructions,
               const Frame * frame = nullptr,
               const Compiler_Bytecode_Instruction * inst = nullptr) const
    {
      Compiler_Bytecode_Runtime_Error error;
      error.code = code;
      error.message = message;
      if (frame != nullptr and frame->function != nullptr)
        {
          error.function_name = frame->function->name;
          error.pc = frame->pc;
        }
      if (inst != nullptr)
        error.span = inst->span;
      return Compiler_Bytecode_Execution_Result::runtime_error(error,
                                                               executed_instructions);
    }

    [[nodiscard]] Compiler_Bytecode_Execution_Result
    make_validation_error(const Compiler_Bytecode_Validation_Report & report,
                          const size_t executed_instructions = 0) const
    {
      const std::string message = report.errors.is_empty()
        ? "invalid bytecode module"
        : report.errors.access(0);
      return make_error("BCV001", message, executed_instructions);
    }

    [[nodiscard]] bool
    module_is_valid(Compiler_Bytecode_Execution_Result & error) const
    {
      const auto report = validate_bytecode_module(*bytecode_module);
      if (report.valid)
        return true;

      error = make_validation_error(report);
      return false;
    }

    [[nodiscard]] bool
    push_frame(const Compiler_Bytecode_Function & function,
               const DynArray<Compiler_Bytecode_Value> & arguments,
               const bool returns_to_caller,
               const Compiler_Bytecode_PC caller_resume_pc,
               const Compiler_Bytecode_Register_Id caller_destination,
               const size_t executed_instructions,
               Compiler_Bytecode_Execution_Result & error)
    {
      const auto expected_arity
        = Compiler_Bytecode_Interpreter_Detail::count_parameters(function);
      if (arguments.size() != expected_arity)
        {
          error = make_error("BCV004",
                             "function '" + function.name + "' expects "
                               + std::to_string(expected_arity) + " arguments but received "
                               + std::to_string(arguments.size()),
                             executed_instructions);
          return false;
        }

      Frame frame;
      frame.function = &function;
      frame.pc = function.entry_pc;
      frame.caller_resume_pc = caller_resume_pc;
      frame.caller_destination = caller_destination;
      frame.returns_to_caller = returns_to_caller;

      Compiler_Bytecode_Interpreter_Detail::reset_values(frame.registers,
                                                         function.register_count);
      Compiler_Bytecode_Interpreter_Detail::reset_values(frame.locals,
                                                         function.local_slots.size());

      size_t arg_index = 0;
      for (size_t i = 0; i < function.local_slots.size(); ++i)
        {
          const auto & slot = function.local_slots.access(i);
          if (slot.kind == Compiler_IR_Slot_Kind::Parameter)
            {
              if (slot.id >= frame.locals.size())
                {
                  error = make_error("BCV006",
                                     "parameter slot id " + std::to_string(slot.id)
                                       + " is out of bounds in function '" + function.name + "'",
                                     executed_instructions);
                  return false;
                }
              frame.locals.access(slot.id) = arguments.access(arg_index++);
            }
        }

      call_stack.append(std::move(frame));
      return true;
    }

    [[nodiscard]] bool
    read_register(const Frame & frame,
                  const Compiler_Bytecode_Register_Id id,
                  const Compiler_Bytecode_Instruction & inst,
                  const size_t executed_instructions,
                  Compiler_Bytecode_Value & value,
                  Compiler_Bytecode_Execution_Result & error) const
    {
      if (id >= frame.registers.size())
        {
          error = make_error("BCV006",
                             "invalid register r" + std::to_string(id),
                             executed_instructions,
                             &frame,
                             &inst);
          return false;
        }

      value = frame.registers.access(id);
      if (value.is_invalid())
        {
          error = make_error("BCV007",
                             "read from uninitialized register r" + std::to_string(id),
                             executed_instructions,
                             &frame,
                             &inst);
          return false;
        }

      return true;
    }

    [[nodiscard]] bool
    write_register(Frame & frame,
                   const Compiler_Bytecode_Register_Id id,
                   const Compiler_Bytecode_Value & value,
                   const Compiler_Bytecode_Instruction & inst,
                   const size_t executed_instructions,
                   Compiler_Bytecode_Execution_Result & error) const
    {
      if (id >= frame.registers.size())
        {
          error = make_error("BCV006",
                             "invalid register r" + std::to_string(id),
                             executed_instructions,
                             &frame,
                             &inst);
          return false;
        }

      frame.registers.access(id) = value;
      return true;
    }

    [[nodiscard]] bool
    read_local(const Frame & frame,
               const Compiler_IR_Local_Slot_Id id,
               const Compiler_Bytecode_Instruction & inst,
               const size_t executed_instructions,
               Compiler_Bytecode_Value & value,
               Compiler_Bytecode_Execution_Result & error) const
    {
      if (id >= frame.locals.size())
        {
          error = make_error("BCV006",
                             "invalid local slot s" + std::to_string(id),
                             executed_instructions,
                             &frame,
                             &inst);
          return false;
        }

      value = frame.locals.access(id);
      if (value.is_invalid())
        {
          error = make_error("BCV008",
                             "read from uninitialized local slot s" + std::to_string(id),
                             executed_instructions,
                             &frame,
                             &inst);
          return false;
        }

      return true;
    }

    [[nodiscard]] bool
    write_local(Frame & frame,
                const Compiler_IR_Local_Slot_Id id,
                const Compiler_Bytecode_Value & value,
                const Compiler_Bytecode_Instruction & inst,
                const size_t executed_instructions,
                Compiler_Bytecode_Execution_Result & error) const
    {
      if (id >= frame.locals.size())
        {
          error = make_error("BCV006",
                             "invalid local slot s" + std::to_string(id),
                             executed_instructions,
                             &frame,
                             &inst);
          return false;
        }

      frame.locals.access(id) = value;
      return true;
    }

    [[nodiscard]] bool
    read_global(const Frame & frame,
                const Compiler_IR_Global_Slot_Id id,
                const Compiler_Bytecode_Instruction & inst,
                const size_t executed_instructions,
                Compiler_Bytecode_Value & value,
                Compiler_Bytecode_Execution_Result & error) const
    {
      if (id >= globals.size())
        {
          error = make_error("BCV006",
                             "invalid global slot g" + std::to_string(id),
                             executed_instructions,
                             &frame,
                             &inst);
          return false;
        }

      value = globals.access(id);
      if (value.is_invalid())
        {
          error = make_error("BCV009",
                             "read from uninitialized global slot g" + std::to_string(id),
                             executed_instructions,
                             &frame,
                             &inst);
          return false;
        }

      return true;
    }

    [[nodiscard]] bool
    write_global(Frame & frame,
                 const Compiler_IR_Global_Slot_Id id,
                 const Compiler_Bytecode_Value & value,
                 const Compiler_Bytecode_Instruction & inst,
                 const size_t executed_instructions,
                 Compiler_Bytecode_Execution_Result & error)
    {
      if (id >= globals.size())
        {
          error = make_error("BCV006",
                             "invalid global slot g" + std::to_string(id),
                             executed_instructions,
                             &frame,
                             &inst);
          return false;
        }

      globals.access(id) = value;
      return true;
    }

    [[nodiscard]] bool
    require_integer(const Compiler_Bytecode_Value & value,
                    const Frame & frame,
                    const Compiler_Bytecode_Instruction & inst,
                    const size_t executed_instructions,
                    const std::string & context,
                    long long & result,
                    Compiler_Bytecode_Execution_Result & error) const
    {
      if (not value.is_runtime_value()
          or value.runtime_value.kind != Interpreter_Value_Kind::Integer)
        {
          error = make_error("BCV011",
                             context + " expects Integer but received "
                               + compiler_bytecode_value_to_string(value, bytecode_module),
                             executed_instructions,
                             &frame,
                             &inst);
          return false;
        }

      result = value.runtime_value.integer_value;
      return true;
    }

    [[nodiscard]] bool
    require_bool(const Compiler_Bytecode_Value & value,
                 const Frame & frame,
                 const Compiler_Bytecode_Instruction & inst,
                 const size_t executed_instructions,
                 const std::string & context,
                 bool & result,
                 Compiler_Bytecode_Execution_Result & error) const
    {
      if (not value.is_runtime_value()
          or value.runtime_value.kind != Interpreter_Value_Kind::Bool)
        {
          error = make_error("BCV012",
                             context + " expects Bool but received "
                               + compiler_bytecode_value_to_string(value, bytecode_module),
                             executed_instructions,
                             &frame,
                             &inst);
          return false;
        }

      result = value.runtime_value.bool_value;
      return true;
    }

    [[nodiscard]] bool
    read_callee_and_arguments(const Frame & frame,
                             const Compiler_Bytecode_Instruction & inst,
                             const size_t executed_instructions,
                             Compiler_Bytecode_Value & callee,
                             DynArray<Compiler_Bytecode_Value> & arguments,
                             Compiler_Bytecode_Execution_Result & error) const
    {
      if (inst.operands.is_empty())
        {
          error = make_error("BCV017",
                             "call instruction has no callee operand",
                             executed_instructions,
                             &frame,
                             &inst);
          return false;
        }

      if (not read_register(frame,
                            inst.operands.access(0),
                            inst,
                            executed_instructions,
                            callee,
                            error))
        return false;

      arguments.clear();
      for (size_t i = 1; i < inst.operands.size(); ++i)
        {
          Compiler_Bytecode_Value argument;
          if (not read_register(frame,
                                inst.operands.access(i),
                                inst,
                                executed_instructions,
                                argument,
                                error))
            return false;
          arguments.append(argument);
        }

      return true;
    }

    [[nodiscard]] bool
    complete_current_frame(const Compiler_Bytecode_Value & value,
                           const Compiler_Bytecode_Completion_Kind kind,
                           const size_t executed_instructions,
                           Compiler_Bytecode_Execution_Result & completed,
                           Compiler_Bytecode_Execution_Result & error)
    {
      const auto finished = call_stack.pop();
      if (not finished.returns_to_caller)
        {
          completed = kind == Compiler_Bytecode_Completion_Kind::Returned
            ? Compiler_Bytecode_Execution_Result::returned(value, executed_instructions)
            : Compiler_Bytecode_Execution_Result::halted(value, executed_instructions);
          return true;
        }

      auto & caller = current_frame();
      if (not write_register(caller,
                             finished.caller_destination,
                             value,
                             caller.function->code.access(caller.pc),
                             executed_instructions,
                             error))
        return true;

      caller.pc = finished.caller_resume_pc;
      return false;
    }

    [[nodiscard]] bool
    execute_host_call(Frame & frame,
                      const Compiler_Bytecode_Instruction & inst,
                      const Compiler_Bytecode_Value & callee,
                      const DynArray<Compiler_Bytecode_Value> & arguments,
                      const size_t executed_instructions,
                      Compiler_Bytecode_Execution_Result & error)
    {
      DynArray<Interpreter_Value> runtime_arguments;
      for (size_t i = 0; i < arguments.size(); ++i)
        {
          if (not arguments.access(i).is_runtime_value())
            {
              error = make_error("BCV019",
                                 "host function arguments cannot contain bytecode function references",
                                 executed_instructions,
                                 &frame,
                                 &inst);
              return false;
            }
          runtime_arguments.append(arguments.access(i).runtime_value);
        }

      Interpreter_Value runtime_result = Interpreter_Value::make_invalid();
      Interpreter_Runtime_Error host_error;
      const bool ok = callee.runtime_value.host_function(callee.runtime_value.host_user_data,
                                                         runtime_arguments,
                                                         runtime_result,
                                                         host_error);
      if (not ok)
        {
          error = make_error(host_error.code.empty() ? "BCV021" : host_error.code,
                             host_error.message.empty()
                               ? "host function call failed"
                               : host_error.message,
                             executed_instructions,
                             &frame,
                             &inst);
          if (host_error.has_error() and not host_error.code.empty())
            error.error.span = host_error.span;
          return false;
        }

      if (not write_register(frame,
                             inst.destination,
                             Compiler_Bytecode_Value::make_value(std::move(runtime_result)),
                             inst,
                             executed_instructions,
                             error))
        return false;

      ++frame.pc;
      return true;
    }

    [[nodiscard]] Compiler_Bytecode_Execution_Result
    execute_loop(const size_t max_steps)
    {
      size_t executed_instructions = 0;

      while (not call_stack.is_empty())
        {
          if (max_steps != 0 and executed_instructions >= max_steps)
            return make_error("BCV020",
                              "bytecode step limit exceeded",
                              executed_instructions,
                              &current_frame());

          auto & frame = current_frame();
          if (frame.pc >= frame.function->code.size())
            return make_error("BCV005",
                              "program counter " + std::to_string(frame.pc)
                                + " is out of bounds in function '" + frame.function->name + "'",
                              executed_instructions,
                              &frame);

          const auto & inst = frame.function->code.access(frame.pc);
          ++executed_instructions;

          switch (inst.opcode)
            {
            case Compiler_Bytecode_Opcode::Load_Constant:
              {
                if (inst.constant_id >= frame.function->constants.size())
                  return make_error("BCV006",
                                    "invalid constant id k"
                                      + std::to_string(inst.constant_id),
                                    executed_instructions,
                                    &frame,
                                    &inst);

                Compiler_Bytecode_Value value;
                std::string conversion_error;
                if (not Compiler_Bytecode_Interpreter_Detail::constant_to_value(
                      frame.function->constants.access(inst.constant_id),
                      value,
                      conversion_error))
                  return make_error("BCV010",
                                    conversion_error,
                                    executed_instructions,
                                    &frame,
                                    &inst);

                Compiler_Bytecode_Execution_Result error;
                if (not write_register(frame,
                                       inst.destination,
                                       value,
                                       inst,
                                       executed_instructions,
                                       error))
                  return error;

                ++frame.pc;
                break;
              }

            case Compiler_Bytecode_Opcode::Load_Local:
              {
                Compiler_Bytecode_Value value;
                Compiler_Bytecode_Execution_Result error;
                if (not read_local(frame,
                                   inst.local_slot_id,
                                   inst,
                                   executed_instructions,
                                   value,
                                   error))
                  return error;
                if (not write_register(frame,
                                       inst.destination,
                                       value,
                                       inst,
                                       executed_instructions,
                                       error))
                  return error;

                ++frame.pc;
                break;
              }

            case Compiler_Bytecode_Opcode::Store_Local:
              {
                if (inst.operands.is_empty())
                  return make_error("BCV006",
                                    "StoreLocal requires one source operand",
                                    executed_instructions,
                                    &frame,
                                    &inst);

                Compiler_Bytecode_Value value;
                Compiler_Bytecode_Execution_Result error;
                if (not read_register(frame,
                                      inst.operands.access(0),
                                      inst,
                                      executed_instructions,
                                      value,
                                      error))
                  return error;
                if (not write_local(frame,
                                    inst.local_slot_id,
                                    value,
                                    inst,
                                    executed_instructions,
                                    error))
                  return error;

                ++frame.pc;
                break;
              }

            case Compiler_Bytecode_Opcode::Load_Global:
              {
                Compiler_Bytecode_Value value;
                Compiler_Bytecode_Execution_Result error;
                if (not read_global(frame,
                                    inst.global_slot_id,
                                    inst,
                                    executed_instructions,
                                    value,
                                    error))
                  return error;
                if (not write_register(frame,
                                       inst.destination,
                                       value,
                                       inst,
                                       executed_instructions,
                                       error))
                  return error;

                ++frame.pc;
                break;
              }

            case Compiler_Bytecode_Opcode::Store_Global:
              {
                if (inst.operands.is_empty())
                  return make_error("BCV006",
                                    "StoreGlobal requires one source operand",
                                    executed_instructions,
                                    &frame,
                                    &inst);

                Compiler_Bytecode_Value value;
                Compiler_Bytecode_Execution_Result error;
                if (not read_register(frame,
                                      inst.operands.access(0),
                                      inst,
                                      executed_instructions,
                                      value,
                                      error))
                  return error;
                if (not write_global(frame,
                                     inst.global_slot_id,
                                     value,
                                     inst,
                                     executed_instructions,
                                     error))
                  return error;

                ++frame.pc;
                break;
              }

            case Compiler_Bytecode_Opcode::Load_Function:
              {
                Compiler_Bytecode_Execution_Result error;
                if (not write_register(frame,
                                       inst.destination,
                                       Compiler_Bytecode_Value::make_function_ref(inst.function_id),
                                       inst,
                                       executed_instructions,
                                       error))
                  return error;

                ++frame.pc;
                break;
              }

            case Compiler_Bytecode_Opcode::Unary:
              {
                if (inst.operands.is_empty())
                  return make_error("BCV006",
                                    "Unary requires one source operand",
                                    executed_instructions,
                                    &frame,
                                    &inst);

                Compiler_Bytecode_Value operand;
                Compiler_Bytecode_Execution_Result error;
                if (not read_register(frame,
                                      inst.operands.access(0),
                                      inst,
                                      executed_instructions,
                                      operand,
                                      error))
                  return error;

                Compiler_Bytecode_Value result;
                switch (inst.op)
                  {
                  case Compiler_Operator_Kind::Minus:
                    {
                      long long integer_value = 0;
                      if (not require_integer(operand,
                                              frame,
                                              inst,
                                              executed_instructions,
                                              "unary '-'",
                                              integer_value,
                                              error))
                        return error;
                      result = Compiler_Bytecode_Value::make_integer(-integer_value);
                      break;
                    }

                  case Compiler_Operator_Kind::Bang:
                    {
                      bool bool_value = false;
                      if (not require_bool(operand,
                                           frame,
                                           inst,
                                           executed_instructions,
                                           "unary '!'",
                                           bool_value,
                                           error))
                        return error;
                      result = Compiler_Bytecode_Value::make_bool(not bool_value);
                      break;
                    }

                  case Compiler_Operator_Kind::Tilde:
                    {
                      long long integer_value = 0;
                      if (not require_integer(operand,
                                              frame,
                                              inst,
                                              executed_instructions,
                                              "unary '~'",
                                              integer_value,
                                              error))
                        return error;
                      result = Compiler_Bytecode_Value::make_integer(~integer_value);
                      break;
                    }

                  default:
                    return make_error("BCV015",
                                      "unsupported unary operator '"
                                        + std::string(Compiler_Bytecode_Detail::token_name(inst.op))
                                        + "'",
                                      executed_instructions,
                                      &frame,
                                      &inst);
                  }

                if (not write_register(frame,
                                       inst.destination,
                                       result,
                                       inst,
                                       executed_instructions,
                                       error))
                  return error;

                ++frame.pc;
                break;
              }

            case Compiler_Bytecode_Opcode::Binary:
              {
                if (inst.operands.size() < 2)
                  return make_error("BCV006",
                                    "Binary requires two source operands",
                                    executed_instructions,
                                    &frame,
                                    &inst);

                Compiler_Bytecode_Value lhs_value;
                Compiler_Bytecode_Value rhs_value;
                Compiler_Bytecode_Execution_Result error;
                if (not read_register(frame,
                                      inst.operands.access(0),
                                      inst,
                                      executed_instructions,
                                      lhs_value,
                                      error))
                  return error;
                if (not read_register(frame,
                                      inst.operands.access(1),
                                      inst,
                                      executed_instructions,
                                      rhs_value,
                                      error))
                  return error;

                Compiler_Bytecode_Value result;
                switch (inst.op)
                  {
                  case Compiler_Operator_Kind::Plus:
                  case Compiler_Operator_Kind::Minus:
                  case Compiler_Operator_Kind::Star:
                  case Compiler_Operator_Kind::Slash:
                  case Compiler_Operator_Kind::Percent:
                  case Compiler_Operator_Kind::Amp:
                  case Compiler_Operator_Kind::Pipe:
                  case Compiler_Operator_Kind::Caret:
                    {
                      long long lhs = 0;
                      long long rhs = 0;
                      if (not require_integer(lhs_value,
                                              frame,
                                              inst,
                                              executed_instructions,
                                              "binary operator",
                                              lhs,
                                              error))
                        return error;
                      if (not require_integer(rhs_value,
                                              frame,
                                              inst,
                                              executed_instructions,
                                              "binary operator",
                                              rhs,
                                              error))
                        return error;

                      switch (inst.op)
                        {
                        case Compiler_Operator_Kind::Plus:
                          result = Compiler_Bytecode_Value::make_integer(lhs + rhs);
                          break;
                        case Compiler_Operator_Kind::Minus:
                          result = Compiler_Bytecode_Value::make_integer(lhs - rhs);
                          break;
                        case Compiler_Operator_Kind::Star:
                          result = Compiler_Bytecode_Value::make_integer(lhs * rhs);
                          break;
                        case Compiler_Operator_Kind::Slash:
                          if (rhs == 0)
                            return make_error("BCV013",
                                              "division by zero",
                                              executed_instructions,
                                              &frame,
                                              &inst);
                          result = Compiler_Bytecode_Value::make_integer(lhs / rhs);
                          break;
                        case Compiler_Operator_Kind::Percent:
                          if (rhs == 0)
                            return make_error("BCV014",
                                              "modulo by zero",
                                              executed_instructions,
                                              &frame,
                                              &inst);
                          result = Compiler_Bytecode_Value::make_integer(lhs % rhs);
                          break;
                        case Compiler_Operator_Kind::Amp:
                          result = Compiler_Bytecode_Value::make_integer(lhs & rhs);
                          break;
                        case Compiler_Operator_Kind::Pipe:
                          result = Compiler_Bytecode_Value::make_integer(lhs | rhs);
                          break;
                        case Compiler_Operator_Kind::Caret:
                          result = Compiler_Bytecode_Value::make_integer(lhs ^ rhs);
                          break;
                        default:
                          break;
                        }
                      break;
                    }

                  case Compiler_Operator_Kind::Less:
                  case Compiler_Operator_Kind::LessEq:
                  case Compiler_Operator_Kind::Greater:
                  case Compiler_Operator_Kind::GreaterEq:
                    {
                      long long lhs = 0;
                      long long rhs = 0;
                      if (not require_integer(lhs_value,
                                              frame,
                                              inst,
                                              executed_instructions,
                                              "comparison",
                                              lhs,
                                              error))
                        return error;
                      if (not require_integer(rhs_value,
                                              frame,
                                              inst,
                                              executed_instructions,
                                              "comparison",
                                              rhs,
                                              error))
                        return error;

                      bool comparison = false;
                      switch (inst.op)
                        {
                        case Compiler_Operator_Kind::Less:
                          comparison = lhs < rhs;
                          break;
                        case Compiler_Operator_Kind::LessEq:
                          comparison = lhs <= rhs;
                          break;
                        case Compiler_Operator_Kind::Greater:
                          comparison = lhs > rhs;
                          break;
                        case Compiler_Operator_Kind::GreaterEq:
                          comparison = lhs >= rhs;
                          break;
                        default:
                          break;
                        }
                      result = Compiler_Bytecode_Value::make_bool(comparison);
                      break;
                    }

                  case Compiler_Operator_Kind::AndAnd:
                  case Compiler_Operator_Kind::OrOr:
                    {
                      bool lhs = false;
                      bool rhs = false;
                      if (not require_bool(lhs_value,
                                           frame,
                                           inst,
                                           executed_instructions,
                                           "logical operator",
                                           lhs,
                                           error))
                        return error;
                      if (not require_bool(rhs_value,
                                           frame,
                                           inst,
                                           executed_instructions,
                                           "logical operator",
                                           rhs,
                                           error))
                        return error;
                      result = Compiler_Bytecode_Value::make_bool(inst.op
                                                                    == Compiler_Operator_Kind::AndAnd
                                                                  ? lhs and rhs
                                                                  : lhs or rhs);
                      break;
                    }

                  case Compiler_Operator_Kind::EqEq:
                    result = Compiler_Bytecode_Value::make_bool(
                      compiler_bytecode_values_equal(lhs_value, rhs_value));
                    break;

                  case Compiler_Operator_Kind::NotEq:
                    result = Compiler_Bytecode_Value::make_bool(
                      not compiler_bytecode_values_equal(lhs_value, rhs_value));
                    break;

                  default:
                    return make_error("BCV016",
                                      "unsupported binary operator '"
                                        + std::string(Compiler_Bytecode_Detail::token_name(inst.op))
                                        + "'",
                                      executed_instructions,
                                      &frame,
                                      &inst);
                  }

                if (not write_register(frame,
                                       inst.destination,
                                       result,
                                       inst,
                                       executed_instructions,
                                       error))
                  return error;

                ++frame.pc;
                break;
              }

            case Compiler_Bytecode_Opcode::Call:
              {
                Compiler_Bytecode_Value callee;
                DynArray<Compiler_Bytecode_Value> arguments;
                Compiler_Bytecode_Execution_Result error;
                if (not read_callee_and_arguments(frame,
                                                 inst,
                                                 executed_instructions,
                                                 callee,
                                                 arguments,
                                                 error))
                  return error;

                if (callee.is_function_ref())
                  {
                    if (callee.function_id >= bytecode_module->functions.size())
                      return make_error("BCV003",
                                        "invalid function id f"
                                          + std::to_string(callee.function_id),
                                        executed_instructions,
                                        &frame,
                                        &inst);

                    if (not push_frame(*bytecode_module->functions.access(callee.function_id),
                                       arguments,
                                       true,
                                       frame.pc + 1,
                                       inst.destination,
                                       executed_instructions,
                                       error))
                      return error;
                    break;
                  }

                if (not callee.is_runtime_value())
                  return make_error("BCV017",
                                    "callee is not callable: "
                                      + compiler_bytecode_value_to_string(callee, bytecode_module),
                                    executed_instructions,
                                    &frame,
                                    &inst);

                if (callee.runtime_value.kind == Interpreter_Value_Kind::Host_Function)
                  {
                    if (not execute_host_call(frame,
                                              inst,
                                              callee,
                                              arguments,
                                              executed_instructions,
                                              error))
                      return error;
                    break;
                  }

                if (callee.runtime_value.kind == Interpreter_Value_Kind::Function)
                  return make_error("BCV018",
                                    "HIR function values are not callable from bytecode VM",
                                    executed_instructions,
                                    &frame,
                                    &inst);

                return make_error("BCV017",
                                  "callee is not callable: "
                                    + compiler_bytecode_value_to_string(callee, bytecode_module),
                                  executed_instructions,
                                  &frame,
                                  &inst);
              }

            case Compiler_Bytecode_Opcode::Jump:
              frame.pc = inst.target_pc;
              break;

            case Compiler_Bytecode_Opcode::Branch:
              {
                if (inst.operands.is_empty())
                  return make_error("BCV006",
                                    "Branch requires one condition operand",
                                    executed_instructions,
                                    &frame,
                                    &inst);

                Compiler_Bytecode_Value condition;
                Compiler_Bytecode_Execution_Result error;
                if (not read_register(frame,
                                      inst.operands.access(0),
                                      inst,
                                      executed_instructions,
                                      condition,
                                      error))
                  return error;

                bool branch = false;
                if (not require_bool(condition,
                                     frame,
                                     inst,
                                     executed_instructions,
                                     "branch condition",
                                     branch,
                                     error))
                  return error;

                frame.pc = branch ? inst.target_pc : inst.false_target_pc;
                break;
              }

            case Compiler_Bytecode_Opcode::Return:
              {
                Compiler_Bytecode_Value value = Compiler_Bytecode_Value::make_unit();
                Compiler_Bytecode_Execution_Result error;
                if (not inst.operands.is_empty()
                    and not read_register(frame,
                                          inst.operands.access(0),
                                          inst,
                                          executed_instructions,
                                          value,
                                          error))
                  return error;

                Compiler_Bytecode_Execution_Result completed;
                if (complete_current_frame(value,
                                           Compiler_Bytecode_Completion_Kind::Returned,
                                           executed_instructions,
                                           completed,
                                           error))
                  return error.ok() ? completed : error;
                break;
              }

            case Compiler_Bytecode_Opcode::Halt:
              {
                Compiler_Bytecode_Execution_Result completed;
                Compiler_Bytecode_Execution_Result error;
                if (complete_current_frame(Compiler_Bytecode_Value::make_unit(),
                                           Compiler_Bytecode_Completion_Kind::Halted,
                                           executed_instructions,
                                           completed,
                                           error))
                  return error.ok() ? completed : error;
                break;
              }

            case Compiler_Bytecode_Opcode::Trap:
              return make_error("BCV022",
                                "executed trap/unreachable instruction",
                                executed_instructions,
                                &frame,
                                &inst);
            }
        }

      return Compiler_Bytecode_Execution_Result::halted(Compiler_Bytecode_Value::make_unit(), 0);
    }

    [[nodiscard]] Compiler_Bytecode_Execution_Result
    execute_entry(const Compiler_Bytecode_Function & function,
                  const DynArray<Compiler_Bytecode_Value> & arguments,
                  const size_t max_steps)
    {
      call_stack.clear();

      Compiler_Bytecode_Execution_Result error;
      if (not module_is_valid(error))
        return error;

      if (not push_frame(function,
                         arguments,
                         false,
                         compiler_bytecode_invalid_id(),
                         0,
                         0,
                         error))
        return error;

      return execute_loop(max_steps);
    }

  public:
    /** @brief Builds a VM over one immutable bytecode module.
     *
     *  Global slots start as `Invalid`. Call `set_global()` before execution if
     *  you want to preload constants or host functions.
     */
    explicit Compiler_Bytecode_VM(const Compiler_Bytecode_Module & module)
      : bytecode_module(&module)
    {
      reset();
    }

    /** @brief Resets globals to `Invalid` and clears any pending call stack. */
    void
    reset()
    {
      call_stack.clear();
      Compiler_Bytecode_Interpreter_Detail::reset_values(globals,
                                                         bytecode_module != nullptr
                                                           ? bytecode_module->global_slots.size()
                                                           : 0);
    }

    /** @brief Returns the immutable bytecode module executed by this VM. */
    [[nodiscard]] const Compiler_Bytecode_Module &
    module() const noexcept
    {
      return *bytecode_module;
    }

    /** @brief Returns the current global slot values. */
    [[nodiscard]] const DynArray<Compiler_Bytecode_Value> &
    global_values() const noexcept
    {
      return globals;
    }

    /** @brief Returns one global slot value if the slot id is valid. */
    [[nodiscard]] const Compiler_Bytecode_Value *
    global_slot(const Compiler_IR_Global_Slot_Id slot_id) const noexcept
    {
      if (slot_id >= globals.size())
        return nullptr;
      return &globals.access(slot_id);
    }

    /** @brief Returns the current dynamic call-stack depth. */
    [[nodiscard]] size_t
    call_stack_depth() const noexcept
    {
      return call_stack.size();
    }

    /** @brief Writes one global slot before or after execution.
     *
     *  @param slot_id Global slot id to update.
     *  @param value New runtime value.
     *  @return `true` if the slot exists, `false` otherwise.
     */
    bool
    set_global(const Compiler_IR_Global_Slot_Id slot_id,
               const Compiler_Bytecode_Value & value)
    {
      if (slot_id >= globals.size())
        return false;
      globals.access(slot_id) = value;
      return true;
    }

    /** @brief Executes the module top-level body.
     *
     *  @param max_steps Optional execution limit. Use `0` for no limit.
     *  @return Structured completion result.
     */
    [[nodiscard]] Compiler_Bytecode_Execution_Result
    run_top_level(const size_t max_steps = 0)
    {
      if (bytecode_module == nullptr)
        return make_error("BCV001", "null bytecode module", 0);

      if (bytecode_module->top_level == nullptr)
        return make_error("BCV002", "bytecode module has no top-level body", 0);

      DynArray<Compiler_Bytecode_Value> arguments;
      return execute_entry(*bytecode_module->top_level, arguments, max_steps);
    }

    /** @brief Executes one bytecode function directly.
     *
     *  @param function_id Callee id inside `module().functions`.
     *  @param arguments Positional bytecode values already prepared by the caller.
     *  @param max_steps Optional execution limit. Use `0` for no limit.
     *  @return Structured completion result.
     */
    [[nodiscard]] Compiler_Bytecode_Execution_Result
    call_function(const Compiler_Bytecode_Function_Id function_id,
                  const DynArray<Compiler_Bytecode_Value> & arguments,
                  const size_t max_steps = 0)
    {
      if (bytecode_module == nullptr)
        return make_error("BCV001", "null bytecode module", 0);

      if (function_id >= bytecode_module->functions.size())
        return make_error("BCV003",
                          "invalid function id f" + std::to_string(function_id),
                          0);

      return execute_entry(*bytecode_module->functions.access(function_id),
                           arguments,
                           max_steps);
    }

    /** @brief Returns a deterministic dump of the current global state. */
    [[nodiscard]] std::string
    dump_globals() const
    {
      return compiler_dump_bytecode_globals(bytecode_module, globals);
    }
  };
}

#endif
