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

/** @file Compiler_Dataflow.H
 *  @brief Reusable dataflow analyses and dead-code elimination over `Compiler_IR_Model.H`.
 *
 *  This header adds the first optimization-oriented layer on top of the
 *  reusable non-SSA IR. The current scope remains intentionally conservative:
 *
 *  - block reachability
 *  - local-slot liveness
 *  - definite assignment / initialization for parameters and locals
 *  - local constant propagation with a small constant lattice
 *  - dead-code elimination of unreachable blocks, dead pure values, dead local stores,
 *    and branches proven constant
 *  - invariant validators for analysis and DCE results
 *
 *  The analyses operate per `Compiler_IR_Function`, including the lowered
 *  top-level body when represented as `<top-level>`.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_DATAFLOW_H
#define COMPILER_DATAFLOW_H

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <sstream>
#include <string>
#include <utility>

#include <Compiler_IR_Model.H>
#include <ah-errors.H>
#include <tpl_dynArray.H>

namespace Aleph
{
  /** @brief Small reusable bit-set for slot-domain dataflow analyses. */
  struct Compiler_Dataflow_Bit_Set
  {
    DynArray<unsigned char> bits; ///< One byte per tracked element.

    /** @brief Resizes the bit-set to `count` bits and fills it with `value`. */
    void resize(const size_t count, const bool value = false)
    {
      bits.clear();
      for (size_t i = 0; i < count; ++i)
        bits.append(value ? 1u : 0u);
    }

    /** @brief Returns the number of tracked bits. */
    size_t size() const noexcept
    {
      return bits.size();
    }

    /** @brief Returns whether bit `index` is set. */
    bool test(const size_t index) const noexcept
    {
      return index < bits.size() and bits.access(index) != 0;
    }

    /** @brief Sets or clears bit `index`. Returns whether it changed. */
    bool set(const size_t index, const bool value = true)
    {
      ah_runtime_error_unless(index < bits.size())
        << "Compiler_Dataflow_Bit_Set::set(): index out of bounds";

      const auto encoded = static_cast<unsigned char>(value ? 1u : 0u);
      if (bits.access(index) == encoded)
        return false;
      bits.access(index) = encoded;
      return true;
    }
  };

  /** @brief Constant-lattice states used by local constant propagation. */
  enum class Compiler_Dataflow_Constant_Kind
  {
    Unknown,   ///< A value exists but is not known to be constant.
    Undefined, ///< A slot has not been definitely initialized yet.
    Integer,   ///< Integer constant.
    Boolean,   ///< Boolean constant.
    Unit       ///< Unit constant.
  };

  /** @brief Returns a stable debug name for one constant-lattice kind. */
  inline const char *
  compiler_dataflow_constant_kind_name(const Compiler_Dataflow_Constant_Kind kind) noexcept
  {
    switch (kind)
      {
      case Compiler_Dataflow_Constant_Kind::Unknown:
        return "Unknown";
      case Compiler_Dataflow_Constant_Kind::Undefined:
        return "Undefined";
      case Compiler_Dataflow_Constant_Kind::Integer:
        return "Integer";
      case Compiler_Dataflow_Constant_Kind::Boolean:
        return "Boolean";
      case Compiler_Dataflow_Constant_Kind::Unit:
        return "Unit";
      }

    return "Unknown";
  }

  /** @brief One propagated constant value in the local-slot lattice. */
  struct Compiler_Dataflow_Constant
  {
    Compiler_Dataflow_Constant_Kind kind = Compiler_Dataflow_Constant_Kind::Unknown; ///< Lattice kind.
    long long integer_value = 0; ///< Integer payload when `kind == Integer`.
    bool bool_value = false; ///< Boolean payload when `kind == Boolean`.
    std::string text; ///< Stable textual spelling for deterministic dumps.

    /** @brief Returns whether the lattice state represents a known constant. */
    bool is_known() const noexcept
    {
      return kind == Compiler_Dataflow_Constant_Kind::Integer
             or kind == Compiler_Dataflow_Constant_Kind::Boolean
             or kind == Compiler_Dataflow_Constant_Kind::Unit;
    }
  };

  /** @brief One definite-assignment finding for an uninitialized local read. */
  struct Compiler_Dataflow_Uninitialized_Read
  {
    Compiler_IR_Block_Id block_id = compiler_ir_invalid_id(); ///< Block containing the read.
    size_t instruction_index = 0; ///< Zero-based instruction index inside the block.
    Compiler_IR_Local_Slot_Id slot_id = compiler_ir_invalid_id(); ///< Read local slot.
    Source_Span span; ///< Source span associated with the load.
  };

  /** @brief Full dataflow result for one IR function or top-level body. */
  struct Compiler_Dataflow_Function_Analysis
  {
    size_t local_slot_count = 0; ///< Number of tracked local slots.
    DynArray<bool> reachable_blocks; ///< Reachability from entry for each block id.
    DynArray<Compiler_Dataflow_Bit_Set> live_in_slots; ///< Local slots live at block entry.
    DynArray<Compiler_Dataflow_Bit_Set> live_out_slots; ///< Local slots live at block exit.
    DynArray<Compiler_Dataflow_Bit_Set> assigned_in_slots; ///< Local slots definitely assigned at block entry.
    DynArray<Compiler_Dataflow_Bit_Set> assigned_out_slots; ///< Local slots definitely assigned at block exit.
    DynArray<DynArray<Compiler_Dataflow_Constant>> constant_in_slots; ///< Per-block constant state at entry.
    DynArray<DynArray<Compiler_Dataflow_Constant>> constant_out_slots; ///< Per-block constant state at exit.
    DynArray<Compiler_Dataflow_Uninitialized_Read> uninitialized_reads; ///< Local reads that are not definitely assigned.
    size_t foldable_branch_count = 0; ///< Reachable branches whose condition is statically known.
  };

  /** @brief Validation report for analysis and optimization passes. */
  struct Compiler_Dataflow_Validation_Report
  {
    bool valid = true; ///< Whether the checked invariants hold.
    DynArray<std::string> errors; ///< Hard invariant violations.
    DynArray<std::string> warnings; ///< Non-fatal observations.
  };

  /** @brief Result of applying dead-code elimination to one IR function. */
  struct Compiler_Dead_Code_Elimination_Result
  {
    Compiler_IR_Function function; ///< Optimized function copy.
    size_t removed_blocks = 0; ///< Number of removed blocks.
    size_t removed_instructions = 0; ///< Number of removed instructions.
    size_t folded_branches = 0; ///< Number of branches folded to jumps.
  };

  namespace Compiler_Dataflow_Detail
  {
    inline Compiler_Dataflow_Constant
    unknown_constant()
    {
      return {};
    }

    inline Compiler_Dataflow_Constant
    undefined_constant()
    {
      Compiler_Dataflow_Constant value;
      value.kind = Compiler_Dataflow_Constant_Kind::Undefined;
      value.text = "undef";
      return value;
    }

    inline Compiler_Dataflow_Constant
    integer_constant(const long long value)
    {
      Compiler_Dataflow_Constant constant;
      constant.kind = Compiler_Dataflow_Constant_Kind::Integer;
      constant.integer_value = value;
      constant.text = std::to_string(value);
      return constant;
    }

    inline Compiler_Dataflow_Constant
    boolean_constant(const bool value)
    {
      Compiler_Dataflow_Constant constant;
      constant.kind = Compiler_Dataflow_Constant_Kind::Boolean;
      constant.bool_value = value;
      constant.text = value ? "true" : "false";
      return constant;
    }

    inline Compiler_Dataflow_Constant
    unit_constant()
    {
      Compiler_Dataflow_Constant constant;
      constant.kind = Compiler_Dataflow_Constant_Kind::Unit;
      constant.text = "unit";
      return constant;
    }

    inline bool
    same_constant(const Compiler_Dataflow_Constant & lhs,
                  const Compiler_Dataflow_Constant & rhs) noexcept
    {
      if (lhs.kind != rhs.kind)
        return false;

      switch (lhs.kind)
        {
        case Compiler_Dataflow_Constant_Kind::Integer:
          return lhs.integer_value == rhs.integer_value;
        case Compiler_Dataflow_Constant_Kind::Boolean:
          return lhs.bool_value == rhs.bool_value;
        case Compiler_Dataflow_Constant_Kind::Unknown:
        case Compiler_Dataflow_Constant_Kind::Undefined:
        case Compiler_Dataflow_Constant_Kind::Unit:
          return true;
        }

      return false;
    }

    inline std::string
    constant_to_string(const Compiler_Dataflow_Constant & constant)
    {
      switch (constant.kind)
        {
        case Compiler_Dataflow_Constant_Kind::Unknown:
          return "?";
        case Compiler_Dataflow_Constant_Kind::Undefined:
          return "undef";
        case Compiler_Dataflow_Constant_Kind::Integer:
          return constant.text.empty() ? std::to_string(constant.integer_value)
                                       : constant.text;
        case Compiler_Dataflow_Constant_Kind::Boolean:
          return constant.bool_value ? "true" : "false";
        case Compiler_Dataflow_Constant_Kind::Unit:
          return "unit";
        }

      return "?";
    }

    inline bool
    parse_integer_constant(const std::string & text,
                           long long & value) noexcept
    {
      if (text.empty())
        return false;

      char * end = nullptr;
      errno = 0;
      const long long result = std::strtoll(text.c_str(), &end, 10);
      if (end == text.c_str() or *end != '\0')
        return false;
      if (errno == ERANGE)
        return false;
      value = result;
      return true;
    }

    inline Compiler_Dataflow_Constant
    constant_from_instruction(const Compiler_IR_Instruction & inst)
    {
      if (inst.kind != Compiler_IR_Instruction_Kind::Constant)
        return unknown_constant();

      if (inst.text == "unit")
        return unit_constant();
      if (inst.text == "true")
        return boolean_constant(true);
      if (inst.text == "false")
        return boolean_constant(false);

      long long value = 0;
      if (parse_integer_constant(inst.text, value))
        return integer_constant(value);

      return unknown_constant();
    }

    inline Compiler_Dataflow_Constant
    meet_constants(const Compiler_Dataflow_Constant & lhs,
                   const Compiler_Dataflow_Constant & rhs)
    {
      if (same_constant(lhs, rhs))
        return lhs;

      if (lhs.kind == Compiler_Dataflow_Constant_Kind::Unknown
          or rhs.kind == Compiler_Dataflow_Constant_Kind::Unknown)
        return unknown_constant();

      if (lhs.kind == Compiler_Dataflow_Constant_Kind::Undefined
          or rhs.kind == Compiler_Dataflow_Constant_Kind::Undefined)
        return unknown_constant();

      return unknown_constant();
    }

    inline Compiler_Dataflow_Bit_Set
    make_bit_set(const size_t count, const bool value = false)
    {
      Compiler_Dataflow_Bit_Set set;
      set.resize(count, value);
      return set;
    }

    inline DynArray<Compiler_Dataflow_Constant>
    make_constant_vector(const size_t count,
                         const Compiler_Dataflow_Constant & fill)
    {
      DynArray<Compiler_Dataflow_Constant> values;
      for (size_t i = 0; i < count; ++i)
        values.append(fill);
      return values;
    }

    inline bool
    bit_set_equals(const Compiler_Dataflow_Bit_Set & lhs,
                   const Compiler_Dataflow_Bit_Set & rhs) noexcept
    {
      if (lhs.size() != rhs.size())
        return false;

      for (size_t i = 0; i < lhs.size(); ++i)
        if (lhs.bits.access(i) != rhs.bits.access(i))
          return false;
      return true;
    }

    inline bool
    constant_vector_equals(const DynArray<Compiler_Dataflow_Constant> & lhs,
                           const DynArray<Compiler_Dataflow_Constant> & rhs) noexcept
    {
      if (lhs.size() != rhs.size())
        return false;

      for (size_t i = 0; i < lhs.size(); ++i)
        if (not same_constant(lhs.access(i), rhs.access(i)))
          return false;
      return true;
    }

    inline Compiler_Dataflow_Bit_Set
    bit_set_union(const Compiler_Dataflow_Bit_Set & lhs,
                  const Compiler_Dataflow_Bit_Set & rhs)
    {
      ah_runtime_error_unless(lhs.size() == rhs.size())
        << "Compiler_Dataflow: union requires sets with the same size";

      auto result = make_bit_set(lhs.size(), false);
      for (size_t i = 0; i < lhs.size(); ++i)
        result.bits.access(i) = static_cast<unsigned char>(lhs.test(i) or rhs.test(i));
      return result;
    }

    inline Compiler_Dataflow_Bit_Set
    bit_set_intersection(const Compiler_Dataflow_Bit_Set & lhs,
                         const Compiler_Dataflow_Bit_Set & rhs)
    {
      ah_runtime_error_unless(lhs.size() == rhs.size())
        << "Compiler_Dataflow: intersection requires sets with the same size";

      auto result = make_bit_set(lhs.size(), false);
      for (size_t i = 0; i < lhs.size(); ++i)
        result.bits.access(i) = static_cast<unsigned char>(lhs.test(i) and rhs.test(i));
      return result;
    }

    inline Compiler_Dataflow_Bit_Set
    bit_set_subtract(const Compiler_Dataflow_Bit_Set & lhs,
                     const Compiler_Dataflow_Bit_Set & rhs)
    {
      ah_runtime_error_unless(lhs.size() == rhs.size())
        << "Compiler_Dataflow: subtraction requires sets with the same size";

      auto result = make_bit_set(lhs.size(), false);
      for (size_t i = 0; i < lhs.size(); ++i)
        result.bits.access(i) = static_cast<unsigned char>(lhs.test(i) and not rhs.test(i));
      return result;
    }

    inline Compiler_Dataflow_Bit_Set
    initial_assigned_state(const Compiler_IR_Function & function)
    {
      auto assigned = make_bit_set(function.local_slots.size(), false);
      for (size_t i = 0; i < function.local_slots.size(); ++i)
        if (function.local_slots.access(i).kind == Compiler_IR_Slot_Kind::Parameter)
          assigned.set(i, true);
      return assigned;
    }

    inline DynArray<Compiler_Dataflow_Constant>
    initial_constant_state(const Compiler_IR_Function & function)
    {
      auto constants = make_constant_vector(function.local_slots.size(),
                                            undefined_constant());
      for (size_t i = 0; i < function.local_slots.size(); ++i)
        if (function.local_slots.access(i).kind == Compiler_IR_Slot_Kind::Parameter)
          constants.access(i) = unknown_constant();
      return constants;
    }

    inline Compiler_Dataflow_Constant
    value_constant(const DynArray<Compiler_Dataflow_Constant> & values,
                   const Compiler_IR_Value_Id id)
    {
      if (id == 0 or id >= values.size())
        return unknown_constant();
      return values.access(id);
    }

    inline void
    mark_value_used(DynArray<unsigned char> & used_values,
                    const Compiler_IR_Value_Id id)
    {
      if (id != 0 and id < used_values.size())
        used_values.access(id) = 1u;
    }

    inline bool
    is_value_used(const DynArray<unsigned char> & used_values,
                  const Compiler_IR_Value_Id id) noexcept
    {
      return id != 0 and id < used_values.size() and used_values.access(id) != 0;
    }

    inline bool
    is_pure_instruction(const Compiler_IR_Instruction & inst) noexcept
    {
      return inst.kind == Compiler_IR_Instruction_Kind::Constant
             or inst.kind == Compiler_IR_Instruction_Kind::Load
             or inst.kind == Compiler_IR_Instruction_Kind::Unary
             or inst.kind == Compiler_IR_Instruction_Kind::Binary
             or inst.kind == Compiler_IR_Instruction_Kind::Function_Ref;
    }

    inline Compiler_Dataflow_Constant
    evaluate_unary(const Compiler_Operator_Kind op,
                   const Compiler_Dataflow_Constant & operand)
    {
      switch (op)
        {
        case Compiler_Operator_Kind::Plus:
          if (operand.kind == Compiler_Dataflow_Constant_Kind::Integer)
            return integer_constant(operand.integer_value);
          break;

        case Compiler_Operator_Kind::Minus:
          if (operand.kind == Compiler_Dataflow_Constant_Kind::Integer)
            {
              if (operand.integer_value == LLONG_MIN)
                {
                  const auto u = static_cast<long long>(
                    0ULL - static_cast<unsigned long long>(operand.integer_value));
                  return integer_constant(u);
                }
              return integer_constant(-operand.integer_value);
            }
          break;

        case Compiler_Operator_Kind::Bang:
          if (operand.kind == Compiler_Dataflow_Constant_Kind::Boolean)
            return boolean_constant(not operand.bool_value);
          break;

        default:
          break;
        }

      return unknown_constant();
    }

    inline Compiler_Dataflow_Constant
    evaluate_binary(const Compiler_Operator_Kind op,
                    const Compiler_Dataflow_Constant & lhs,
                    const Compiler_Dataflow_Constant & rhs)
    {
      if (lhs.kind == Compiler_Dataflow_Constant_Kind::Integer
          and rhs.kind == Compiler_Dataflow_Constant_Kind::Integer)
        switch (op)
          {
          case Compiler_Operator_Kind::Plus:
            {
              long long result = 0;
              if (__builtin_add_overflow(lhs.integer_value, rhs.integer_value, &result))
                return unknown_constant();
              return integer_constant(result);
            }
          case Compiler_Operator_Kind::Minus:
            {
              long long result = 0;
              if (__builtin_sub_overflow(lhs.integer_value, rhs.integer_value, &result))
                return unknown_constant();
              return integer_constant(result);
            }
          case Compiler_Operator_Kind::Star:
            {
              long long result = 0;
              if (__builtin_mul_overflow(lhs.integer_value, rhs.integer_value, &result))
                return unknown_constant();
              return integer_constant(result);
            }
          case Compiler_Operator_Kind::Slash:
            if (rhs.integer_value == 0)
              return unknown_constant();
            if (lhs.integer_value == LLONG_MIN and rhs.integer_value == -1)
              return unknown_constant();
            return integer_constant(lhs.integer_value / rhs.integer_value);
          case Compiler_Operator_Kind::Percent:
            if (rhs.integer_value == 0)
              return unknown_constant();
            if (lhs.integer_value == LLONG_MIN and rhs.integer_value == -1)
              return unknown_constant();
            return integer_constant(lhs.integer_value % rhs.integer_value);
          case Compiler_Operator_Kind::EqEq:
            return boolean_constant(lhs.integer_value == rhs.integer_value);
          case Compiler_Operator_Kind::NotEq:
            return boolean_constant(lhs.integer_value != rhs.integer_value);
          case Compiler_Operator_Kind::Less:
            return boolean_constant(lhs.integer_value < rhs.integer_value);
          case Compiler_Operator_Kind::LessEq:
            return boolean_constant(lhs.integer_value <= rhs.integer_value);
          case Compiler_Operator_Kind::Greater:
            return boolean_constant(lhs.integer_value > rhs.integer_value);
          case Compiler_Operator_Kind::GreaterEq:
            return boolean_constant(lhs.integer_value >= rhs.integer_value);
          default:
            break;
          }

      if (lhs.kind == Compiler_Dataflow_Constant_Kind::Boolean
          and rhs.kind == Compiler_Dataflow_Constant_Kind::Boolean)
        switch (op)
          {
          case Compiler_Operator_Kind::AndAnd:
            return boolean_constant(lhs.bool_value and rhs.bool_value);
          case Compiler_Operator_Kind::OrOr:
            return boolean_constant(lhs.bool_value or rhs.bool_value);
          case Compiler_Operator_Kind::EqEq:
            return boolean_constant(lhs.bool_value == rhs.bool_value);
          case Compiler_Operator_Kind::NotEq:
            return boolean_constant(lhs.bool_value != rhs.bool_value);
          default:
            break;
          }

      if (lhs.kind == Compiler_Dataflow_Constant_Kind::Unit
          and rhs.kind == Compiler_Dataflow_Constant_Kind::Unit)
        switch (op)
          {
          case Compiler_Operator_Kind::EqEq:
            return boolean_constant(true);
          case Compiler_Operator_Kind::NotEq:
            return boolean_constant(false);
          default:
            break;
          }

      return unknown_constant();
    }

    struct Block_Use_Def
    {
      Compiler_Dataflow_Bit_Set use;
      Compiler_Dataflow_Bit_Set def;
    };

    inline Block_Use_Def
    compute_block_use_def(const Compiler_IR_Function & function,
                          const Compiler_IR_Block & block)
    {
      Block_Use_Def info;
      info.use = make_bit_set(function.local_slots.size(), false);
      info.def = make_bit_set(function.local_slots.size(), false);

      for (size_t i = 0; i < block.instructions.size(); ++i)
        {
          const auto & inst = block.instructions.access(i);
          if (inst.kind == Compiler_IR_Instruction_Kind::Load
              and inst.local_slot_id != compiler_ir_invalid_id()
              and inst.local_slot_id < function.local_slots.size()
              and not info.def.test(inst.local_slot_id))
            info.use.set(inst.local_slot_id, true);

          if (inst.kind == Compiler_IR_Instruction_Kind::Store
              and inst.local_slot_id != compiler_ir_invalid_id()
              and inst.local_slot_id < function.local_slots.size())
            info.def.set(inst.local_slot_id, true);
        }

      return info;
    }

    inline DynArray<bool>
    compute_reachability(const Compiler_IR_Function & function)
    {
      DynArray<bool> reachable;
      for (size_t i = 0; i < function.blocks.size(); ++i)
        reachable.append(false);

      if (function.blocks.is_empty() or function.entry_block >= function.blocks.size())
        return reachable;

      DynArray<Compiler_IR_Block_Id> worklist;
      worklist.append(function.entry_block);
      while (not worklist.is_empty())
        {
          const auto current = worklist.pop();
          if (current >= function.blocks.size() or reachable.access(current))
            continue;

          reachable.access(current) = true;
          const auto & block = function.blocks.access(current);
          for (size_t i = 0; i < block.terminator.successors.size(); ++i)
            {
              const auto succ = block.terminator.successors.access(i);
              if (succ < function.blocks.size() and not reachable.access(succ))
                worklist.append(succ);
            }
        }

      return reachable;
    }

    struct Block_Simulation
    {
      Compiler_Dataflow_Bit_Set assigned_out;
      DynArray<Compiler_Dataflow_Constant> constant_out;
      DynArray<Compiler_Dataflow_Constant> value_constants;
      Compiler_Dataflow_Constant branch_constant;
    };

    inline Block_Simulation
    simulate_block(const Compiler_IR_Function & function,
                   const Compiler_IR_Block & block,
                   const Compiler_Dataflow_Bit_Set & assigned_in,
                   const DynArray<Compiler_Dataflow_Constant> & constants_in,
                   DynArray<Compiler_Dataflow_Uninitialized_Read> * findings = nullptr)
    {
      Block_Simulation simulation;
      simulation.assigned_out = assigned_in;
      simulation.constant_out = constants_in;
      simulation.value_constants = make_constant_vector(function.next_value_id + 1,
                                                        unknown_constant());
      simulation.branch_constant = unknown_constant();

      for (size_t i = 0; i < block.instructions.size(); ++i)
        {
          const auto & inst = block.instructions.access(i);
          Compiler_Dataflow_Constant result = unknown_constant();

          switch (inst.kind)
            {
            case Compiler_IR_Instruction_Kind::Constant:
              result = constant_from_instruction(inst);
              break;

            case Compiler_IR_Instruction_Kind::Load:
              if (inst.local_slot_id != compiler_ir_invalid_id()
                  and inst.local_slot_id < simulation.constant_out.size())
                {
                  if (not simulation.assigned_out.test(inst.local_slot_id)
                      and findings != nullptr)
                    findings->append({block.id, i, inst.local_slot_id, inst.span});

                  result = simulation.assigned_out.test(inst.local_slot_id)
                    ? simulation.constant_out.access(inst.local_slot_id)
                    : unknown_constant();
                }
              else
                result = unknown_constant();
              break;

            case Compiler_IR_Instruction_Kind::Unary:
              result = evaluate_unary(inst.op,
                                      value_constant(simulation.value_constants,
                                                     inst.operands.is_empty()
                                                       ? 0
                                                       : inst.operands.access(0)));
              break;

            case Compiler_IR_Instruction_Kind::Binary:
              result = evaluate_binary(inst.op,
                                       value_constant(simulation.value_constants,
                                                      inst.operands.size() > 0
                                                        ? inst.operands.access(0)
                                                        : 0),
                                       value_constant(simulation.value_constants,
                                                      inst.operands.size() > 1
                                                        ? inst.operands.access(1)
                                                        : 0));
              break;

            case Compiler_IR_Instruction_Kind::Call:
            case Compiler_IR_Instruction_Kind::Function_Ref:
              result = unknown_constant();
              break;

            case Compiler_IR_Instruction_Kind::Store:
              if (inst.local_slot_id != compiler_ir_invalid_id()
                  and inst.local_slot_id < simulation.constant_out.size())
                {
                  simulation.assigned_out.set(inst.local_slot_id, true);
                  simulation.constant_out.access(inst.local_slot_id) =
                    value_constant(simulation.value_constants,
                                   inst.operands.is_empty()
                                     ? 0
                                     : inst.operands.access(0));
                }
              continue;
            }

          if (inst.result_id != 0 and inst.result_id < simulation.value_constants.size())
            simulation.value_constants.access(inst.result_id) = result;
        }

      if (block.terminator.kind == Compiler_IR_Terminator_Kind::Branch)
        simulation.branch_constant =
          value_constant(simulation.value_constants,
                         block.terminator.condition_value);

      return simulation;
    }

    inline std::string
    format_slot_set(const Compiler_IR_Function & function,
                    const Compiler_Dataflow_Bit_Set & set)
    {
      std::ostringstream out;
      bool first = true;
      for (size_t i = 0; i < set.size(); ++i)
        if (set.test(i))
          {
            if (not first)
              out << ", ";
            out << Compiler_IR_Detail::local_slot_name(function.local_slots.access(i).id);
            first = false;
          }

      return first ? "<none>" : out.str();
    }

    inline std::string
    format_slot_constants(const Compiler_IR_Function & function,
                          const DynArray<Compiler_Dataflow_Constant> & constants)
    {
      std::ostringstream out;
      for (size_t i = 0; i < constants.size(); ++i)
        {
          if (i > 0)
            out << ", ";
          out << Compiler_IR_Detail::local_slot_name(function.local_slots.access(i).id)
              << '=' << constant_to_string(constants.access(i));
        }

      return constants.is_empty() ? "<none>" : out.str();
    }

    inline void
    rebuild_predecessors(Compiler_IR_Function & function)
    {
      for (size_t i = 0; i < function.blocks.size(); ++i)
        function.blocks.access(i).predecessors.clear();

      for (size_t i = 0; i < function.blocks.size(); ++i)
        for (size_t j = 0; j < function.blocks.access(i).terminator.successors.size(); ++j)
          {
            const auto succ = function.blocks.access(i).terminator.successors.access(j);
            if (succ < function.blocks.size())
              function.blocks.access(succ).predecessors.append(i);
          }
    }

    inline size_t
    count_instructions(const Compiler_IR_Function & function)
    {
      size_t total = 0;
      for (size_t i = 0; i < function.blocks.size(); ++i)
        total += function.blocks.access(i).instructions.size();
      return total;
    }

    inline bool
    instruction_is_trivially_dead(const Compiler_IR_Instruction & inst,
                                  const Compiler_Dataflow_Bit_Set & live_after,
                                  const DynArray<unsigned char> & used_values,
                                  const Compiler_IR_Function & function) noexcept
    {
      if (inst.kind == Compiler_IR_Instruction_Kind::Store
          and inst.local_slot_id != compiler_ir_invalid_id()
          and inst.local_slot_id < function.local_slots.size())
        return not live_after.test(inst.local_slot_id);

      if (is_pure_instruction(inst) and inst.result_id != 0)
        return not is_value_used(used_values, inst.result_id);

      return false;
    }
  }

  /** @brief Computes reachability, liveness, definite assignment, and constant propagation for one IR function. */
  inline Compiler_Dataflow_Function_Analysis
  analyze_dataflow_function(const Compiler_IR_Function & function)
  {
    Compiler_Dataflow_Function_Analysis analysis;
    analysis.local_slot_count = function.local_slots.size();

    const auto reachable = Compiler_Dataflow_Detail::compute_reachability(function);
    analysis.reachable_blocks = reachable;

    DynArray<Compiler_Dataflow_Detail::Block_Use_Def> block_sets;
    for (size_t i = 0; i < function.blocks.size(); ++i)
      block_sets.append(Compiler_Dataflow_Detail::compute_block_use_def(function,
                                                                        function.blocks.access(i)));

    for (size_t i = 0; i < function.blocks.size(); ++i)
      {
        analysis.live_in_slots.append(Compiler_Dataflow_Detail::make_bit_set(function.local_slots.size(), false));
        analysis.live_out_slots.append(Compiler_Dataflow_Detail::make_bit_set(function.local_slots.size(), false));
        analysis.assigned_in_slots.append(Compiler_Dataflow_Detail::make_bit_set(function.local_slots.size(), false));
        analysis.assigned_out_slots.append(Compiler_Dataflow_Detail::make_bit_set(function.local_slots.size(), false));
        analysis.constant_in_slots.append(Compiler_Dataflow_Detail::make_constant_vector(function.local_slots.size(),
                                                                                         Compiler_Dataflow_Detail::unknown_constant()));
        analysis.constant_out_slots.append(Compiler_Dataflow_Detail::make_constant_vector(function.local_slots.size(),
                                                                                          Compiler_Dataflow_Detail::unknown_constant()));
      }

    bool changed = true;
    while (changed)
      {
        changed = false;
        for (size_t index = function.blocks.size(); index > 0; --index)
          {
            const auto block_id = index - 1;
            if (block_id >= reachable.size() or not reachable.access(block_id))
              continue;

            auto live_out = Compiler_Dataflow_Detail::make_bit_set(function.local_slots.size(), false);
            const auto & block = function.blocks.access(block_id);
            for (size_t i = 0; i < block.terminator.successors.size(); ++i)
              {
                const auto succ = block.terminator.successors.access(i);
                if (succ >= function.blocks.size() or not reachable.access(succ))
                  continue;
                live_out = Compiler_Dataflow_Detail::bit_set_union(live_out,
                                                                   analysis.live_in_slots.access(succ));
              }

            const auto live_in =
              Compiler_Dataflow_Detail::bit_set_union(block_sets.access(block_id).use,
                                                      Compiler_Dataflow_Detail::bit_set_subtract(
                                                        live_out,
                                                        block_sets.access(block_id).def));

            if (not Compiler_Dataflow_Detail::bit_set_equals(live_out,
                                                             analysis.live_out_slots.access(block_id)))
              {
                analysis.live_out_slots.access(block_id) = live_out;
                changed = true;
              }
            if (not Compiler_Dataflow_Detail::bit_set_equals(live_in,
                                                             analysis.live_in_slots.access(block_id)))
              {
                analysis.live_in_slots.access(block_id) = live_in;
                changed = true;
              }
          }
      }

    const auto initial_assigned = Compiler_Dataflow_Detail::initial_assigned_state(function);
    changed = true;
    while (changed)
      {
        changed = false;
        for (size_t block_id = 0; block_id < function.blocks.size(); ++block_id)
          {
            if (block_id >= reachable.size() or not reachable.access(block_id))
              continue;

            Compiler_Dataflow_Bit_Set assigned_in;
            if (block_id == function.entry_block)
              assigned_in = initial_assigned;
            else
              {
                bool have_predecessor = false;
                assigned_in = Compiler_Dataflow_Detail::make_bit_set(function.local_slots.size(), true);
                const auto & block = function.blocks.access(block_id);
                for (size_t i = 0; i < block.predecessors.size(); ++i)
                  {
                    const auto pred = block.predecessors.access(i);
                    if (pred >= function.blocks.size() or not reachable.access(pred))
                      continue;
                    if (not have_predecessor)
                      {
                        assigned_in = analysis.assigned_out_slots.access(pred);
                        have_predecessor = true;
                      }
                    else
                      assigned_in = Compiler_Dataflow_Detail::bit_set_intersection(
                        assigned_in,
                        analysis.assigned_out_slots.access(pred));
                  }

                if (not have_predecessor)
                  assigned_in = Compiler_Dataflow_Detail::make_bit_set(function.local_slots.size(), false);
              }

            auto simulation =
              Compiler_Dataflow_Detail::simulate_block(function,
                                                       function.blocks.access(block_id),
                                                       assigned_in,
                                                       Compiler_Dataflow_Detail::make_constant_vector(
                                                         function.local_slots.size(),
                                                         Compiler_Dataflow_Detail::unknown_constant()));

            if (not Compiler_Dataflow_Detail::bit_set_equals(assigned_in,
                                                             analysis.assigned_in_slots.access(block_id)))
              {
                analysis.assigned_in_slots.access(block_id) = assigned_in;
                changed = true;
              }
            if (not Compiler_Dataflow_Detail::bit_set_equals(simulation.assigned_out,
                                                             analysis.assigned_out_slots.access(block_id)))
              {
                analysis.assigned_out_slots.access(block_id) = simulation.assigned_out;
                changed = true;
              }
          }
      }

    const auto initial_constants = Compiler_Dataflow_Detail::initial_constant_state(function);
    changed = true;
    while (changed)
      {
        changed = false;
        for (size_t block_id = 0; block_id < function.blocks.size(); ++block_id)
          {
            if (block_id >= reachable.size() or not reachable.access(block_id))
              continue;

            DynArray<Compiler_Dataflow_Constant> constant_in;
            if (block_id == function.entry_block)
              constant_in = initial_constants;
            else
              {
                bool have_predecessor = false;
                constant_in = Compiler_Dataflow_Detail::make_constant_vector(
                  function.local_slots.size(),
                  Compiler_Dataflow_Detail::unknown_constant());

                const auto & block = function.blocks.access(block_id);
                for (size_t i = 0; i < block.predecessors.size(); ++i)
                  {
                    const auto pred = block.predecessors.access(i);
                    if (pred >= function.blocks.size() or not reachable.access(pred))
                      continue;

                    if (not have_predecessor)
                      {
                        constant_in = analysis.constant_out_slots.access(pred);
                        have_predecessor = true;
                        continue;
                      }

                    for (size_t slot = 0; slot < constant_in.size(); ++slot)
                      constant_in.access(slot) =
                        Compiler_Dataflow_Detail::meet_constants(constant_in.access(slot),
                                                                 analysis.constant_out_slots.access(pred).access(slot));
                  }

                if (not have_predecessor)
                  constant_in = Compiler_Dataflow_Detail::make_constant_vector(
                    function.local_slots.size(),
                    Compiler_Dataflow_Detail::unknown_constant());
              }

            const auto simulation =
              Compiler_Dataflow_Detail::simulate_block(function,
                                                       function.blocks.access(block_id),
                                                       analysis.assigned_in_slots.access(block_id),
                                                       constant_in);

            if (not Compiler_Dataflow_Detail::constant_vector_equals(constant_in,
                                                                     analysis.constant_in_slots.access(block_id)))
              {
                analysis.constant_in_slots.access(block_id) = constant_in;
                changed = true;
              }
            if (not Compiler_Dataflow_Detail::constant_vector_equals(simulation.constant_out,
                                                                     analysis.constant_out_slots.access(block_id)))
              {
                analysis.constant_out_slots.access(block_id) = simulation.constant_out;
                changed = true;
              }
          }
      }

    for (size_t block_id = 0; block_id < function.blocks.size(); ++block_id)
      {
        if (block_id >= reachable.size() or not reachable.access(block_id))
          continue;

        auto findings = DynArray<Compiler_Dataflow_Uninitialized_Read>{};
        const auto simulation =
          Compiler_Dataflow_Detail::simulate_block(function,
                                                   function.blocks.access(block_id),
                                                   analysis.assigned_in_slots.access(block_id),
                                                   analysis.constant_in_slots.access(block_id),
                                                   &findings);
        for (size_t i = 0; i < findings.size(); ++i)
          analysis.uninitialized_reads.append(findings.access(i));

        if (simulation.branch_constant.kind == Compiler_Dataflow_Constant_Kind::Boolean)
          ++analysis.foldable_branch_count;
      }

    return analysis;
  }

  /** @brief Validates that a dataflow result matches the analyzed IR function. */
  inline Compiler_Dataflow_Validation_Report
  validate_dataflow_analysis(const Compiler_IR_Function & function,
                             const Compiler_Dataflow_Function_Analysis & analysis)
  {
    Compiler_Dataflow_Validation_Report report;

    auto fail = [&report](const std::string & message)
    {
      report.valid = false;
      report.errors.append(message);
    };

    if (analysis.reachable_blocks.size() != function.blocks.size())
      fail("Dataflow analysis for '" + function.name
           + "' has mismatched reachability size");
    if (analysis.live_in_slots.size() != function.blocks.size()
        or analysis.live_out_slots.size() != function.blocks.size()
        or analysis.assigned_in_slots.size() != function.blocks.size()
        or analysis.assigned_out_slots.size() != function.blocks.size()
        or analysis.constant_in_slots.size() != function.blocks.size()
        or analysis.constant_out_slots.size() != function.blocks.size())
      fail("Dataflow analysis for '" + function.name
           + "' has mismatched per-block arrays");

    if (not report.valid)
      return report;

    for (size_t block_id = 0; block_id < function.blocks.size(); ++block_id)
      {
        const auto check_set = [&fail, &function, block_id](const char * label,
                                                            const Compiler_Dataflow_Bit_Set & set)
        {
          if (set.size() != function.local_slots.size())
            fail(std::string("Dataflow ") + label + " for "
                 + Compiler_IR_Detail::block_name(block_id)
                 + " in function '" + function.name + "' has wrong slot-domain size");
        };

        check_set("live-in", analysis.live_in_slots.access(block_id));
        check_set("live-out", analysis.live_out_slots.access(block_id));
        check_set("assigned-in", analysis.assigned_in_slots.access(block_id));
        check_set("assigned-out", analysis.assigned_out_slots.access(block_id));

        if (analysis.constant_in_slots.access(block_id).size() != function.local_slots.size())
          fail("Dataflow constant-in state for "
               + Compiler_IR_Detail::block_name(block_id)
               + " in function '" + function.name + "' has wrong slot-domain size");
        if (analysis.constant_out_slots.access(block_id).size() != function.local_slots.size())
          fail("Dataflow constant-out state for "
               + Compiler_IR_Detail::block_name(block_id)
               + " in function '" + function.name + "' has wrong slot-domain size");
      }

    if (function.entry_block < analysis.reachable_blocks.size()
        and not analysis.reachable_blocks.access(function.entry_block))
      fail("Dataflow analysis for '" + function.name
           + "' does not mark the entry block as reachable");

    for (size_t i = 0; i < analysis.uninitialized_reads.size(); ++i)
      {
        const auto & finding = analysis.uninitialized_reads.access(i);
        if (finding.block_id >= function.blocks.size())
          fail("Dataflow analysis for '" + function.name
               + "' contains an uninitialized-read finding with an invalid block id");
        else if (finding.instruction_index
                 >= function.blocks.access(finding.block_id).instructions.size())
          fail("Dataflow analysis for '" + function.name
               + "' contains an uninitialized-read finding with an invalid instruction index");

        if (finding.slot_id >= function.local_slots.size())
          fail("Dataflow analysis for '" + function.name
               + "' contains an uninitialized-read finding with an invalid slot id");
      }

    return report;
  }

  /** @brief Dumps one function-level dataflow result deterministically. */
  inline std::string
  compiler_dump_dataflow_analysis(const Compiler_IR_Function * function,
                                  const Compiler_Dataflow_Function_Analysis & analysis,
                                  const Compiler_Type_Context * types = nullptr)
  {
    std::ostringstream out;
    if (function == nullptr)
      {
        out << "<null-dataflow-function>\n";
        return out.str();
      }

    out << "Dataflow(" << function->name << ")\n";
    if (function->local_slots.is_empty())
      out << "  LocalSlots: <none>\n";
    else
      {
        out << "  LocalSlots:\n";
        for (size_t i = 0; i < function->local_slots.size(); ++i)
          {
            const auto & slot = function->local_slots.access(i);
            out << "    " << Compiler_IR_Detail::local_slot_name(slot.id)
                << " [" << compiler_ir_slot_kind_name(slot.kind) << "] "
                << slot.name;
            if (types != nullptr and slot.type_id != 0)
              out << ": " << types->to_string(slot.type_id);
            out << '\n';
          }
      }

    out << "  ReachableBlocks:";
    bool first = true;
    for (size_t i = 0; i < analysis.reachable_blocks.size(); ++i)
      if (analysis.reachable_blocks.access(i))
        {
          out << (first ? " " : ", ");
          out << Compiler_IR_Detail::block_name(i);
          first = false;
        }
    if (first)
      out << " <none>";
    out << '\n';

    out << "  FoldableBranches: " << analysis.foldable_branch_count << '\n';
    out << "  UninitializedReads:";
    if (analysis.uninitialized_reads.is_empty())
      out << " <none>\n";
    else
      {
        out << '\n';
        for (size_t i = 0; i < analysis.uninitialized_reads.size(); ++i)
          {
            const auto & finding = analysis.uninitialized_reads.access(i);
            out << "    " << Compiler_IR_Detail::block_name(finding.block_id)
                << "/I" << finding.instruction_index
                << " -> " << Compiler_IR_Detail::local_slot_name(finding.slot_id)
                << '\n';
          }
      }

    for (size_t block_id = 0; block_id < function->blocks.size(); ++block_id)
      {
        const auto & block = function->blocks.access(block_id);
        out << "  Block " << Compiler_IR_Detail::block_name(block_id)
            << " [" << block.label << "]\n";
        out << "    Reachable: "
            << ((block_id < analysis.reachable_blocks.size()
                 and analysis.reachable_blocks.access(block_id))
                  ? "yes"
                  : "no")
            << '\n';
        out << "    LiveIn: "
            << Compiler_Dataflow_Detail::format_slot_set(*function,
                                                         analysis.live_in_slots.access(block_id))
            << '\n';
        out << "    LiveOut: "
            << Compiler_Dataflow_Detail::format_slot_set(*function,
                                                         analysis.live_out_slots.access(block_id))
            << '\n';
        out << "    AssignedIn: "
            << Compiler_Dataflow_Detail::format_slot_set(*function,
                                                         analysis.assigned_in_slots.access(block_id))
            << '\n';
        out << "    AssignedOut: "
            << Compiler_Dataflow_Detail::format_slot_set(*function,
                                                         analysis.assigned_out_slots.access(block_id))
            << '\n';
        out << "    ConstantsIn: "
            << Compiler_Dataflow_Detail::format_slot_constants(*function,
                                                               analysis.constant_in_slots.access(block_id))
            << '\n';
        out << "    ConstantsOut: "
            << Compiler_Dataflow_Detail::format_slot_constants(*function,
                                                               analysis.constant_out_slots.access(block_id))
            << '\n';
      }

    return out.str();
  }

  /** @brief Eliminates unreachable blocks, dead pure instructions, dead local stores, and constant branches. */
  inline Compiler_Dead_Code_Elimination_Result
  eliminate_dead_code(const Compiler_IR_Function & function)
  {
    Compiler_Dead_Code_Elimination_Result result;
    result.function = function;

    const auto original_instruction_count =
      Compiler_Dataflow_Detail::count_instructions(function);
    const auto analysis = analyze_dataflow_function(function);

    Compiler_IR_Function provisional = function;
    for (size_t block_id = 0; block_id < provisional.blocks.size(); ++block_id)
      {
        provisional.blocks.access(block_id).predecessors.clear();

        if (block_id >= analysis.reachable_blocks.size()
            or not analysis.reachable_blocks.access(block_id))
          continue;

        const auto simulation =
          Compiler_Dataflow_Detail::simulate_block(function,
                                                   function.blocks.access(block_id),
                                                   analysis.assigned_in_slots.access(block_id),
                                                   analysis.constant_in_slots.access(block_id));

        auto & block = provisional.blocks.access(block_id);
        DynArray<unsigned char> used_values;
        for (size_t i = 0; i <= function.next_value_id; ++i)
          used_values.append(0u);

        auto live_slots = analysis.live_out_slots.access(block_id);
        auto terminator = block.terminator;
        if (terminator.kind == Compiler_IR_Terminator_Kind::Branch
            and simulation.branch_constant.kind == Compiler_Dataflow_Constant_Kind::Boolean
            and terminator.successors.size() == 2)
          {
            const auto chosen = simulation.branch_constant.bool_value
              ? terminator.successors.access(0)
              : terminator.successors.access(1);
            terminator.kind = Compiler_IR_Terminator_Kind::Jump;
            terminator.condition_value = 0;
            terminator.successors.clear();
            terminator.successors.append(chosen);
            ++result.folded_branches;
          }

        if (terminator.kind == Compiler_IR_Terminator_Kind::Branch)
          Compiler_Dataflow_Detail::mark_value_used(used_values,
                                                    terminator.condition_value);
        else if (terminator.kind == Compiler_IR_Terminator_Kind::Return)
          Compiler_Dataflow_Detail::mark_value_used(used_values,
                                                    terminator.return_value);

        DynArray<unsigned char> keep_flags;
        for (size_t i = 0; i < block.instructions.size(); ++i)
          keep_flags.append(1u);

        for (size_t index = block.instructions.size(); index > 0; --index)
          {
            const auto instruction_index = index - 1;
            const auto & inst = block.instructions.access(instruction_index);

            bool keep = true;
            if (inst.kind == Compiler_IR_Instruction_Kind::Store
                and inst.local_slot_id != compiler_ir_invalid_id()
                and inst.local_slot_id < function.local_slots.size()
                and not live_slots.test(inst.local_slot_id))
              keep = false;
            else if (Compiler_Dataflow_Detail::is_pure_instruction(inst)
                     and inst.result_id != 0
                     and not Compiler_Dataflow_Detail::is_value_used(used_values,
                                                                      inst.result_id))
              keep = false;

            keep_flags.access(instruction_index) = static_cast<unsigned char>(keep ? 1u : 0u);
            if (not keep)
              continue;

            for (size_t operand = 0; operand < inst.operands.size(); ++operand)
              Compiler_Dataflow_Detail::mark_value_used(used_values,
                                                        inst.operands.access(operand));

            if (inst.kind == Compiler_IR_Instruction_Kind::Store
                and inst.local_slot_id != compiler_ir_invalid_id()
                and inst.local_slot_id < function.local_slots.size())
              live_slots.set(inst.local_slot_id, false);
            else if (inst.kind == Compiler_IR_Instruction_Kind::Load
                     and inst.local_slot_id != compiler_ir_invalid_id()
                     and inst.local_slot_id < function.local_slots.size()
                     and inst.result_id != 0)
              live_slots.set(inst.local_slot_id, true);
          }

        DynArray<Compiler_IR_Instruction> instructions;
        for (size_t i = 0; i < block.instructions.size(); ++i)
          if (keep_flags.access(i) != 0)
            instructions.append(block.instructions.access(i));
        block.instructions = instructions;
        block.terminator = terminator;
      }

    const auto reachable_after_fold =
      Compiler_Dataflow_Detail::compute_reachability(provisional);
    DynArray<Compiler_IR_Block_Id> remap;
    for (size_t i = 0; i < provisional.blocks.size(); ++i)
      remap.append(compiler_ir_invalid_id());

    result.function = function;
    result.function.blocks.clear();
    for (size_t block_id = 0; block_id < provisional.blocks.size(); ++block_id)
      {
        if (block_id >= reachable_after_fold.size()
            or not reachable_after_fold.access(block_id))
          continue;

        remap.access(block_id) = result.function.blocks.size();
        auto block = provisional.blocks.access(block_id);
        block.id = result.function.blocks.size();
        block.predecessors.clear();
        result.function.blocks.append(std::move(block));
      }

    for (size_t block_id = 0; block_id < result.function.blocks.size(); ++block_id)
      {
        auto & block = result.function.blocks.access(block_id);
        DynArray<Compiler_IR_Block_Id> successors;
        for (size_t i = 0; i < block.terminator.successors.size(); ++i)
          {
            const auto old_succ = block.terminator.successors.access(i);
            if (old_succ < remap.size()
                and remap.access(old_succ) != compiler_ir_invalid_id())
              successors.append(remap.access(old_succ));
          }
        block.terminator.successors = successors;
      }

    result.function.entry_block =
      function.entry_block < remap.size() ? remap.access(function.entry_block)
                                          : compiler_ir_invalid_id();
    result.function.exit_block =
      function.exit_block < remap.size() ? remap.access(function.exit_block)
                                         : compiler_ir_invalid_id();

    Compiler_Dataflow_Detail::rebuild_predecessors(result.function);
    result.removed_blocks = function.blocks.size() - result.function.blocks.size();
    result.removed_instructions =
      original_instruction_count - Compiler_Dataflow_Detail::count_instructions(result.function);
    return result;
  }

  /** @brief Validates the result of dead-code elimination and checks that trivial dead code was removed. */
  inline Compiler_Dataflow_Validation_Report
  validate_dead_code_elimination(const Compiler_Dead_Code_Elimination_Result & result,
                                 const Compiler_IR_Module * module = nullptr)
  {
    Compiler_Dataflow_Validation_Report report;

    auto fail = [&report](const std::string & message)
    {
      report.valid = false;
      report.errors.append(message);
    };

    const auto ir_report = validate_ir_function(result.function, module);
    if (not ir_report.valid)
      {
        report.valid = false;
        for (size_t i = 0; i < ir_report.errors.size(); ++i)
          report.errors.append(ir_report.errors.access(i));
      }

    for (size_t i = 0; i < ir_report.warnings.size(); ++i)
      if (ir_report.warnings.access(i).find("unreachable") != std::string::npos)
        fail("DCE result for '" + result.function.name
             + "' still contains unreachable blocks");
      else
        report.warnings.append(ir_report.warnings.access(i));

    const auto analysis = analyze_dataflow_function(result.function);
    if (analysis.foldable_branch_count != 0)
      fail("DCE result for '" + result.function.name
           + "' still contains foldable branches");

    for (size_t block_id = 0; block_id < result.function.blocks.size(); ++block_id)
      {
        if (block_id >= analysis.reachable_blocks.size()
            or not analysis.reachable_blocks.access(block_id))
          continue;

        const auto & block = result.function.blocks.access(block_id);
        auto live_slots = analysis.live_out_slots.access(block_id);
        DynArray<unsigned char> used_values;
        for (size_t i = 0; i <= result.function.next_value_id; ++i)
          used_values.append(0u);

        if (block.terminator.kind == Compiler_IR_Terminator_Kind::Branch)
          Compiler_Dataflow_Detail::mark_value_used(used_values,
                                                    block.terminator.condition_value);
        else if (block.terminator.kind == Compiler_IR_Terminator_Kind::Return)
          Compiler_Dataflow_Detail::mark_value_used(used_values,
                                                    block.terminator.return_value);

        for (size_t index = block.instructions.size(); index > 0; --index)
          {
            const auto instruction_index = index - 1;
            const auto & inst = block.instructions.access(instruction_index);

            if (Compiler_Dataflow_Detail::instruction_is_trivially_dead(inst,
                                                                        live_slots,
                                                                        used_values,
                                                                        result.function))
              fail("DCE result for '" + result.function.name
                   + "' still contains removable code in "
                   + Compiler_IR_Detail::block_name(block_id)
                   + " at instruction #" + std::to_string(instruction_index));

            for (size_t operand = 0; operand < inst.operands.size(); ++operand)
              Compiler_Dataflow_Detail::mark_value_used(used_values,
                                                        inst.operands.access(operand));

            if (inst.kind == Compiler_IR_Instruction_Kind::Store
                and inst.local_slot_id != compiler_ir_invalid_id()
                and inst.local_slot_id < result.function.local_slots.size())
              live_slots.set(inst.local_slot_id, false);
            else if (inst.kind == Compiler_IR_Instruction_Kind::Load
                     and inst.local_slot_id != compiler_ir_invalid_id()
                     and inst.local_slot_id < result.function.local_slots.size()
                     and inst.result_id != 0)
              live_slots.set(inst.local_slot_id, true);
          }
      }

    return report;
  }
}

#endif
