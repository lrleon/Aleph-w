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

/** @file SSA.H
 *  @brief Static single assignment form over `Compiler_IR.H`.
 *
 *  This header introduces the first reusable SSA layer over Aleph-w's explicit
 *  non-SSA IR. The current scope is intentionally conservative and targets the
 *  existing IR model:
 *
 *  - dominators and immediate dominators
 *  - dominance frontiers
 *  - phi placement for parameters and local slots
 *  - rename pass that rewrites local-slot traffic into SSA values
 *  - structural SSA verifier
 *  - deterministic textual dumps
 *
 *  The transformation converts local and parameter slots into SSA values while
 *  keeping global memory traffic explicit as loads and stores. This makes the
 *  result suitable for later bytecode or backend stages without prematurely
 *  committing to a target ISA.
 *
 *  @ingroup Utilities
 */

#ifndef SSA_H
#define SSA_H

#include <limits>
#include <sstream>
#include <string>
#include <utility>

#include <Compiler_Dataflow.H>
#include <ah-arena.H>
#include <ah-errors.H>
#include <tpl_dynArray.H>

namespace Aleph
{
  using Compiler_SSA_Value_Id = size_t;
  using Compiler_SSA_Block_Id = size_t;

  /** @brief Returns the sentinel invalid SSA id. */
  inline constexpr size_t
  compiler_ssa_invalid_id() noexcept
  {
    return std::numeric_limits<size_t>::max();
  }

  /** @brief Instruction kinds supported by the SSA form. */
  enum class Compiler_SSA_Instruction_Kind
  {
    Constant,
    Load_Global,
    Store_Global,
    Unary,
    Binary,
    Call,
    Function_Ref
  };

  /** @brief Returns a stable debug name for an SSA instruction kind. */
  inline const char *
  compiler_ssa_instruction_kind_name(const Compiler_SSA_Instruction_Kind kind) noexcept
  {
    switch (kind)
      {
      case Compiler_SSA_Instruction_Kind::Constant:
        return "Constant";
      case Compiler_SSA_Instruction_Kind::Load_Global:
        return "LoadGlobal";
      case Compiler_SSA_Instruction_Kind::Store_Global:
        return "StoreGlobal";
      case Compiler_SSA_Instruction_Kind::Unary:
        return "Unary";
      case Compiler_SSA_Instruction_Kind::Binary:
        return "Binary";
      case Compiler_SSA_Instruction_Kind::Call:
        return "Call";
      case Compiler_SSA_Instruction_Kind::Function_Ref:
        return "FunctionRef";
      }

    return "Unknown";
  }

  /** @brief SSA parameter definition that seeds renaming for one IR parameter slot. */
  struct Compiler_SSA_Parameter
  {
    Compiler_SSA_Value_Id value_id = 0; ///< SSA value defined by the parameter.
    Compiler_IR_Local_Slot_Id slot_id = compiler_ir_invalid_id(); ///< Original IR slot represented by this parameter.
    Compiler_Type_Id type_id = 0; ///< Parameter type.
    std::string name; ///< Debug/source name.
    Source_Span span; ///< Parameter declaration span.
  };

  /** @brief Phi node merging values for one original IR local slot. */
  struct Compiler_SSA_Phi
  {
    Compiler_SSA_Value_Id result_id = 0; ///< SSA value produced by the phi.
    Compiler_IR_Local_Slot_Id slot_id = compiler_ir_invalid_id(); ///< Original local/parameter slot represented by the phi.
    Compiler_Type_Id type_id = 0; ///< Merged type.
    Source_Span span; ///< Source location associated with the slot declaration.
    DynArray<Compiler_SSA_Value_Id> operands; ///< Incoming values aligned with predecessor order.
  };

  /** @brief One SSA instruction producing an optional explicit result value. */
  struct Compiler_SSA_Instruction
  {
    Compiler_SSA_Instruction_Kind kind = Compiler_SSA_Instruction_Kind::Constant; ///< Instruction category.
    Compiler_SSA_Value_Id result_id = 0; ///< Produced value id, or `0` for void instructions.
    Compiler_Type_Id type_id = 0; ///< Result or stored type.
    Source_Span span; ///< Source region associated with the instruction.
    Compiler_Token_Kind op = Compiler_Token_Kind::Invalid; ///< Unary/binary operator when relevant.
    Compiler_IR_Global_Slot_Id global_slot_id = compiler_ir_invalid_id(); ///< Referenced global slot, when relevant.
    Compiler_IR_Function_Id function_id = compiler_ir_invalid_id(); ///< Referenced callee function, when relevant.
    DynArray<Compiler_SSA_Value_Id> operands; ///< Input SSA values in deterministic order.
    std::string text; ///< Constant spelling or debug payload.
    bool bool_value = false; ///< Decoded boolean payload for bool constants.
  };

  /** @brief Explicit terminator for one SSA basic block. */
  struct Compiler_SSA_Terminator
  {
    Compiler_IR_Terminator_Kind kind = Compiler_IR_Terminator_Kind::None; ///< Terminator category.
    Source_Span span; ///< Source region associated with the terminator.
    Compiler_SSA_Value_Id condition_value = 0; ///< Branch condition, when relevant.
    Compiler_SSA_Value_Id return_value = 0; ///< Return value, when relevant.
    DynArray<Compiler_SSA_Block_Id> successors; ///< Successors in deterministic order.
  };

  /** @brief One SSA basic block containing phis, instructions, and a terminator. */
  struct Compiler_SSA_Block
  {
    Compiler_SSA_Block_Id id = compiler_ssa_invalid_id(); ///< Dense SSA block id.
    Compiler_IR_Block_Id source_block_id = compiler_ir_invalid_id(); ///< Original IR block id before SSA normalization.
    std::string label; ///< Deterministic debug label.
    Source_Span span; ///< Aggregate source span.
    DynArray<Compiler_SSA_Phi> phis; ///< Phi nodes evaluated before instructions.
    DynArray<Compiler_SSA_Instruction> instructions; ///< Linear SSA instruction list.
    Compiler_SSA_Terminator terminator; ///< Explicit block terminator.
    DynArray<Compiler_SSA_Block_Id> predecessors; ///< Predecessor ids in deterministic order.

    /** @brief Returns whether the block already has a terminator. */
    bool is_terminated() const noexcept
    {
      return terminator.kind != Compiler_IR_Terminator_Kind::None;
    }
  };

  /** @brief Dominance metadata computed for one SSA function. */
  struct Compiler_SSA_Dominance_Info
  {
    DynArray<bool> reachable_blocks; ///< Reachability from entry for each SSA block.
    DynArray<Compiler_Dataflow_Bit_Set> dominators; ///< Dominator sets by block.
    DynArray<Compiler_SSA_Block_Id> immediate_dominators; ///< Immediate dominator, or invalid for entry/unreachable.
    DynArray<DynArray<Compiler_SSA_Block_Id>> dominance_frontiers; ///< Dominance frontier for each block.
    DynArray<DynArray<Compiler_SSA_Block_Id>> dominator_tree_children; ///< Dominator-tree children by block.
  };

  /** @brief SSA form for one function or top-level body lowered from IR. */
  struct Compiler_SSA_Function
  {
    Compiler_IR_Function_Id id = compiler_ir_invalid_id(); ///< Stable function id within the source module.
    bool top_level = false; ///< Whether this function represents the lowered top-level body.
    std::string name; ///< Debug/source name.
    Source_Span span; ///< Source span of the function/body.
    Compiler_Type_Id type_id = 0; ///< Full function type, when known.
    const Compiler_IR_Function * source_function = nullptr; ///< Original non-SSA IR function.
    DynArray<Compiler_IR_Slot> local_slots; ///< Original local/parameter slots for debug and phi ownership.
    DynArray<Compiler_SSA_Parameter> parameters; ///< Entry parameter SSA definitions.
    DynArray<Compiler_SSA_Block> blocks; ///< Reachable SSA blocks in deterministic order.
    Compiler_SSA_Dominance_Info dominance; ///< Dominance metadata over `blocks`.
    Compiler_SSA_Block_Id entry_block = compiler_ssa_invalid_id(); ///< Canonical entry block.
    Compiler_SSA_Block_Id exit_block = compiler_ssa_invalid_id(); ///< Canonical exit block.
    Compiler_SSA_Value_Id next_value_id = 0; ///< Next SSA value id to allocate.

    /** @brief Returns whether this SSA function represents the top-level body. */
    bool is_top_level() const noexcept
    {
      return top_level;
    }
  };

  /** @brief SSA module sharing global slots with the source IR module. */
  struct Compiler_SSA_Module
  {
    DynArray<Compiler_IR_Slot> global_slots; ///< Shared global slot layout from the source IR module.
    DynArray<Compiler_SSA_Function *> functions; ///< Lowered non-top-level functions.
    Compiler_SSA_Function * top_level = nullptr; ///< Optional lowered top-level body.
  };

  /** @brief Arena-backed ownership context for SSA functions and modules. */
  class Compiler_SSA_Context
  {
    struct Owned_Object
    {
      void * ptr = nullptr;
      void (*destroy)(void *) noexcept = nullptr;
    };

    AhArenaAllocator arena;
    DynArray<Owned_Object> owned;

  public:
    /** @brief Constructs an SSA context. */
    explicit Compiler_SSA_Context(const size_t arena_size = AhArenaAllocator::DEFAULT_SIZE)
      : arena(arena_size)
    {}

    Compiler_SSA_Context(const Compiler_SSA_Context &) = delete;
    Compiler_SSA_Context & operator=(const Compiler_SSA_Context &) = delete;

    /** @brief Destroys all managed objects and rewinds the arena. */
    ~Compiler_SSA_Context() noexcept
    {
      reset();
    }

    /** @brief Allocates and constructs one SSA object in the arena. */
    template <typename T, typename... Args> T *
    make(Args &&... args)
    {
      T * ptr = allocate<T>(arena, std::forward<Args>(args)...);
      ah_runtime_error_unless(ptr != nullptr)
        << "Compiler_SSA_Context: arena allocation failed";

      owned.append({
          ptr,
          [](void * raw) noexcept
          {
            static_cast<T *>(raw)->~T();
          }
        });
      return ptr;
    }

    /** @brief Resets the arena and destroys all tracked objects. */
    void reset() noexcept
    {
      for (size_t i = owned.size(); i > 0; --i)
        {
          auto &[ptr, destroy] = owned.access(i - 1);
          if (destroy != nullptr and ptr != nullptr)
            destroy(ptr);
        }
      owned.clear();
      arena.reset();
    }
  };

  /** @brief Validation report for one SSA function or module. */
  struct Compiler_SSA_Validation_Report
  {
    bool valid = true; ///< Whether all hard validation checks passed.
    DynArray<std::string> errors; ///< Hard validation failures.
    DynArray<std::string> warnings; ///< Non-fatal observations.
  };

  namespace Compiler_SSA_Detail
  {
    struct Normalized_IR_Function
    {
      Compiler_IR_Function function;
      DynArray<Compiler_IR_Block_Id> source_block_ids;
    };

    struct Phi_Spec
    {
      Compiler_IR_Local_Slot_Id slot_id = compiler_ir_invalid_id();
      Compiler_Type_Id type_id = 0;
      Source_Span span;
    };

    struct Definition_Location
    {
      enum class Kind
      {
        Invalid,
        Parameter,
        Phi,
        Instruction
      };

      Kind kind = Kind::Invalid;
      Compiler_SSA_Block_Id block_id = compiler_ssa_invalid_id();
      size_t order = 0;
    };

    inline std::string
    block_name(const Compiler_SSA_Block_Id id)
    {
      return "B" + std::to_string(id);
    }

    inline std::string
    value_name(const Compiler_SSA_Value_Id id)
    {
      return "v" + std::to_string(id);
    }

    inline std::string
    local_slot_name(const Compiler_IR_Local_Slot_Id id)
    {
      return "s" + std::to_string(id);
    }

    inline std::string
    global_slot_name(const Compiler_IR_Global_Slot_Id id)
    {
      return "g" + std::to_string(id);
    }

    inline std::string
    function_name(const Compiler_IR_Function_Id id)
    {
      return "f" + std::to_string(id);
    }

    inline const char *
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

    inline void
    append_indented_text(std::ostream & out,
                         const std::string & text,
                         const size_t indent)
    {
      const std::string padding(indent, ' ');
      size_t begin = 0;
      while (begin < text.size())
        {
          const auto end = text.find('\n', begin);
          if (end == std::string::npos)
            {
              out << padding << text.substr(begin) << '\n';
              return;
            }

          if (end > begin)
            out << padding << text.substr(begin, end - begin) << '\n';
          else
            out << padding << '\n';
          begin = end + 1;
        }
    }

    inline bool
    contains_block_id(const DynArray<Compiler_SSA_Block_Id> & ids,
                      const Compiler_SSA_Block_Id id) noexcept
    {
      for (size_t i = 0; i < ids.size(); ++i)
        if (ids.access(i) == id)
          return true;
      return false;
    }

    inline DynArray<bool>
    compute_reachable_blocks(const Compiler_IR_Function & function)
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
          const auto block_id = worklist.pop();
          if (block_id >= function.blocks.size() or reachable.access(block_id))
            continue;

          reachable.access(block_id) = true;
          const auto & block = function.blocks.access(block_id);
          for (size_t i = 0; i < block.terminator.successors.size(); ++i)
            {
              const auto succ = block.terminator.successors.access(i);
              if (succ < function.blocks.size() and not reachable.access(succ))
                worklist.append(succ);
            }
        }

      return reachable;
    }

    inline Normalized_IR_Function
    normalize_ir_function(const Compiler_IR_Function & source)
    {
      Normalized_IR_Function normalized;
      normalized.function.id = source.id;
      normalized.function.name = source.name;
      normalized.function.span = source.span;
      normalized.function.type_id = source.type_id;
      normalized.function.source_function = source.source_function;
      normalized.function.local_slots = source.local_slots;
      normalized.function.next_value_id = source.next_value_id;

      const auto reachable = compute_reachable_blocks(source);
      DynArray<Compiler_IR_Block_Id> remap;
      for (size_t i = 0; i < source.blocks.size(); ++i)
        remap.append(compiler_ir_invalid_id());

      for (size_t i = 0; i < source.blocks.size(); ++i)
        if (reachable.access(i))
          {
            remap.access(i) = normalized.function.blocks.size();
            auto block = source.blocks.access(i);
            block.id = normalized.function.blocks.size();
            block.predecessors.clear();
            normalized.source_block_ids.append(i);
            normalized.function.blocks.append(std::move(block));
          }

      for (size_t i = 0; i < normalized.function.blocks.size(); ++i)
        {
          auto & block = normalized.function.blocks.access(i);
          DynArray<Compiler_IR_Block_Id> successors;
          const auto source_id = normalized.source_block_ids.access(i);
          const auto & source_block = source.blocks.access(source_id);
          for (size_t j = 0; j < source_block.terminator.successors.size(); ++j)
            {
              const auto succ = source_block.terminator.successors.access(j);
              if (succ < remap.size() and remap.access(succ) != compiler_ir_invalid_id())
                successors.append(remap.access(succ));
            }
          block.terminator.successors = successors;
        }

      normalized.function.entry_block =
        source.entry_block < remap.size() ? remap.access(source.entry_block)
                                          : compiler_ir_invalid_id();
      normalized.function.exit_block =
        source.exit_block < remap.size() ? remap.access(source.exit_block)
                                         : compiler_ir_invalid_id();

      for (size_t i = 0; i < normalized.function.blocks.size(); ++i)
        normalized.function.blocks.access(i).predecessors.clear();
      for (size_t i = 0; i < normalized.function.blocks.size(); ++i)
        for (size_t j = 0; j < normalized.function.blocks.access(i).terminator.successors.size(); ++j)
          {
            const auto succ = normalized.function.blocks.access(i).terminator.successors.access(j);
            if (succ < normalized.function.blocks.size()
                and not contains_block_id(normalized.function.blocks.access(succ).predecessors, i))
              normalized.function.blocks.access(succ).predecessors.append(i);
          }

      return normalized;
    }

    inline Compiler_SSA_Dominance_Info
    compute_dominance_info(const Compiler_IR_Function & function)
    {
      Compiler_SSA_Dominance_Info info;
      const auto block_count = function.blocks.size();

      for (size_t i = 0; i < block_count; ++i)
        {
          info.reachable_blocks.append(false);
          info.immediate_dominators.append(compiler_ssa_invalid_id());
          info.dominators.append({});
          info.dominance_frontiers.append({});
          info.dominator_tree_children.append({});
        }

      if (block_count == 0 or function.entry_block >= block_count)
        return info;

      info.reachable_blocks = compute_reachable_blocks(function);
      for (size_t i = 0; i < block_count; ++i)
        info.dominators.access(i).resize(block_count, false);

      for (size_t i = 0; i < block_count; ++i)
        {
          if (not info.reachable_blocks.access(i))
            continue;

          if (i == function.entry_block)
            info.dominators.access(i).set(i, true);
          else
            for (size_t j = 0; j < block_count; ++j)
              if (info.reachable_blocks.access(j))
                info.dominators.access(i).set(j, true);
        }

      bool changed = true;
      while (changed)
        {
          changed = false;
          for (size_t block_id = 0; block_id < block_count; ++block_id)
            {
              if (not info.reachable_blocks.access(block_id)
                  or block_id == function.entry_block)
                continue;

              Compiler_Dataflow_Bit_Set new_dom;
              bool have_pred = false;
              new_dom.resize(block_count, false);
              const auto & block = function.blocks.access(block_id);
              for (size_t i = 0; i < block.predecessors.size(); ++i)
                {
                  const auto pred = block.predecessors.access(i);
                  if (pred >= block_count or not info.reachable_blocks.access(pred))
                    continue;

                  if (not have_pred)
                    {
                      new_dom = info.dominators.access(pred);
                      have_pred = true;
                    }
                  else
                    new_dom = Compiler_Dataflow_Detail::bit_set_intersection(
                      new_dom,
                      info.dominators.access(pred));
                }

              if (not have_pred)
                new_dom.resize(block_count, false);
              new_dom.set(block_id, true);

              if (not Compiler_Dataflow_Detail::bit_set_equals(new_dom,
                                                               info.dominators.access(block_id)))
                {
                  info.dominators.access(block_id) = new_dom;
                  changed = true;
                }
            }
        }

      for (size_t block_id = 0; block_id < block_count; ++block_id)
        {
          if (not info.reachable_blocks.access(block_id)
              or block_id == function.entry_block)
            continue;

          Compiler_SSA_Block_Id idom = compiler_ssa_invalid_id();
          for (size_t candidate = 0; candidate < block_count; ++candidate)
            {
              if (candidate == block_id
                  or not info.reachable_blocks.access(candidate)
                  or not info.dominators.access(block_id).test(candidate))
                continue;

              bool dominated_by_other = false;
              for (size_t other = 0; other < block_count; ++other)
                {
                  if (other == block_id or other == candidate
                      or not info.reachable_blocks.access(other)
                      or not info.dominators.access(block_id).test(other))
                    continue;

                  if (info.dominators.access(other).test(candidate))
                    {
                      dominated_by_other = true;
                      break;
                    }
                }

              if (not dominated_by_other)
                {
                  idom = candidate;
                  break;
                }
            }

          info.immediate_dominators.access(block_id) = idom;
          if (idom != compiler_ssa_invalid_id())
            info.dominator_tree_children.access(idom).append(block_id);
        }

      for (size_t block_id = 0; block_id < block_count; ++block_id)
        {
          if (not info.reachable_blocks.access(block_id))
            continue;

          const auto & block = function.blocks.access(block_id);
          if (block.predecessors.size() < 2)
            continue;

          for (size_t i = 0; i < block.predecessors.size(); ++i)
            {
              auto runner = block.predecessors.access(i);
              while (runner != compiler_ssa_invalid_id()
                     and runner != info.immediate_dominators.access(block_id))
                {
                  if (not contains_block_id(info.dominance_frontiers.access(runner), block_id))
                    info.dominance_frontiers.access(runner).append(block_id);
                  runner = info.immediate_dominators.access(runner);
                }
            }
        }

      return info;
    }

    inline size_t
    next_value(Compiler_SSA_Function & function) noexcept
    {
      ++function.next_value_id;
      return function.next_value_id;
    }

    inline Compiler_SSA_Block &
    block(Compiler_SSA_Function & function,
          const Compiler_SSA_Block_Id id)
    {
      return function.blocks.access(id);
    }

    inline const Compiler_SSA_Block &
    block(const Compiler_SSA_Function & function,
          const Compiler_SSA_Block_Id id)
    {
      return function.blocks.access(id);
    }

    inline Compiler_SSA_Value_Id
    current_slot_value(const DynArray<DynArray<Compiler_SSA_Value_Id>> & stacks,
                       const Compiler_IR_Local_Slot_Id slot_id) noexcept
    {
      if (slot_id >= stacks.size() or stacks.access(slot_id).is_empty())
        return 0;
      return stacks.access(slot_id).access(stacks.access(slot_id).size() - 1);
    }

    inline size_t
    predecessor_index(const Compiler_SSA_Block & block,
                      const Compiler_SSA_Block_Id predecessor)
    {
      for (size_t i = 0; i < block.predecessors.size(); ++i)
        if (block.predecessors.access(i) == predecessor)
          return i;

      return compiler_ssa_invalid_id();
    }

    inline void
    rename_recursive(Compiler_SSA_Function & output,
                     const Compiler_IR_Function & normalized,
                     const Compiler_SSA_Dominance_Info & dominance,
                     const DynArray<DynArray<Phi_Spec>> & phi_specs,
                     Compiler_SSA_Block_Id block_id,
                     DynArray<DynArray<Compiler_SSA_Value_Id>> & slot_stacks,
                     DynArray<Compiler_SSA_Value_Id> & renamed_values)
    {
      DynArray<size_t> saved_stack_sizes;
      for (size_t i = 0; i < slot_stacks.size(); ++i)
        saved_stack_sizes.append(slot_stacks.access(i).size());

      auto & out_block = output.blocks.access(block_id);
      const auto & in_block = normalized.blocks.access(block_id);

      for (size_t i = 0; i < out_block.phis.size(); ++i)
        {
          auto & phi = out_block.phis.access(i);
          phi.result_id = next_value(output);
          if (phi.slot_id < slot_stacks.size())
            slot_stacks.access(phi.slot_id).append(phi.result_id);
        }

      for (size_t i = 0; i < in_block.instructions.size(); ++i)
        {
          const auto & inst = in_block.instructions.access(i);
          switch (inst.kind)
            {
            case Compiler_IR_Instruction_Kind::Constant:
              {
                Compiler_SSA_Instruction lowered;
                lowered.kind = Compiler_SSA_Instruction_Kind::Constant;
                lowered.result_id = next_value(output);
                lowered.type_id = inst.type_id;
                lowered.span = inst.span;
                lowered.text = inst.text;
                lowered.bool_value = inst.bool_value;
                renamed_values.access(inst.result_id) = lowered.result_id;
                out_block.instructions.append(std::move(lowered));
                break;
              }

            case Compiler_IR_Instruction_Kind::Load:
              if (inst.local_slot_id != compiler_ir_invalid_id())
                {
                  renamed_values.access(inst.result_id) =
                    current_slot_value(slot_stacks, inst.local_slot_id);
                }
              else
                {
                  Compiler_SSA_Instruction lowered;
                  lowered.kind = Compiler_SSA_Instruction_Kind::Load_Global;
                  lowered.result_id = next_value(output);
                  lowered.type_id = inst.type_id;
                  lowered.span = inst.span;
                  lowered.global_slot_id = inst.global_slot_id;
                  renamed_values.access(inst.result_id) = lowered.result_id;
                  out_block.instructions.append(std::move(lowered));
                }
              break;

            case Compiler_IR_Instruction_Kind::Store:
              if (inst.local_slot_id != compiler_ir_invalid_id())
                {
                  const auto incoming =
                    inst.operands.is_empty() ? 0 : renamed_values.access(inst.operands.access(0));
                  if (inst.local_slot_id < slot_stacks.size())
                    slot_stacks.access(inst.local_slot_id).append(incoming);
                }
              else
                {
                  Compiler_SSA_Instruction lowered;
                  lowered.kind = Compiler_SSA_Instruction_Kind::Store_Global;
                  lowered.type_id = inst.type_id;
                  lowered.span = inst.span;
                  lowered.global_slot_id = inst.global_slot_id;
                  if (not inst.operands.is_empty())
                    lowered.operands.append(renamed_values.access(inst.operands.access(0)));
                  out_block.instructions.append(std::move(lowered));
                }
              break;

            case Compiler_IR_Instruction_Kind::Unary:
            case Compiler_IR_Instruction_Kind::Binary:
            case Compiler_IR_Instruction_Kind::Call:
            case Compiler_IR_Instruction_Kind::Function_Ref:
              {
                Compiler_SSA_Instruction lowered;
                lowered.kind =
                  inst.kind == Compiler_IR_Instruction_Kind::Unary ? Compiler_SSA_Instruction_Kind::Unary :
                  inst.kind == Compiler_IR_Instruction_Kind::Binary ? Compiler_SSA_Instruction_Kind::Binary :
                  inst.kind == Compiler_IR_Instruction_Kind::Call ? Compiler_SSA_Instruction_Kind::Call :
                  Compiler_SSA_Instruction_Kind::Function_Ref;
                lowered.result_id = inst.result_id != 0 ? next_value(output) : 0;
                lowered.type_id = inst.type_id;
                lowered.span = inst.span;
                lowered.op = inst.op;
                lowered.function_id = inst.function_id;
                lowered.text = inst.text;
                lowered.bool_value = inst.bool_value;
                for (size_t operand = 0; operand < inst.operands.size(); ++operand)
                  lowered.operands.append(renamed_values.access(inst.operands.access(operand)));
                if (inst.result_id != 0)
                  renamed_values.access(inst.result_id) = lowered.result_id;
                out_block.instructions.append(std::move(lowered));
                break;
              }
            }
        }

      out_block.terminator.kind = in_block.terminator.kind;
      out_block.terminator.span = in_block.terminator.span;
      if (in_block.terminator.kind == Compiler_IR_Terminator_Kind::Branch)
        out_block.terminator.condition_value =
          renamed_values.access(in_block.terminator.condition_value);
      else if (in_block.terminator.kind == Compiler_IR_Terminator_Kind::Return)
        out_block.terminator.return_value =
          renamed_values.access(in_block.terminator.return_value);
      out_block.terminator.successors = in_block.terminator.successors;

      for (size_t i = 0; i < out_block.terminator.successors.size(); ++i)
        {
          const auto succ_id = out_block.terminator.successors.access(i);
          auto & succ = output.blocks.access(succ_id);
          const auto pred_index = predecessor_index(succ, block_id);
          if (pred_index == compiler_ssa_invalid_id())
            continue;

          for (size_t phi_index = 0; phi_index < succ.phis.size(); ++phi_index)
            {
              auto & phi = succ.phis.access(phi_index);
              phi.operands.access(pred_index) =
                current_slot_value(slot_stacks, phi.slot_id);
            }
        }

      for (size_t i = 0; i < dominance.dominator_tree_children.access(block_id).size(); ++i)
        rename_recursive(output,
                         normalized,
                         dominance,
                         phi_specs,
                         dominance.dominator_tree_children.access(block_id).access(i),
                         slot_stacks,
                         renamed_values);

      for (size_t slot = 0; slot < slot_stacks.size(); ++slot)
        while (slot_stacks.access(slot).size() > saved_stack_sizes.access(slot))
          (void) slot_stacks.access(slot).pop();
    }
  }

  /** @brief Lowers non-SSA IR to SSA form for all reachable blocks. */
  class Compiler_SSA_Lowering
  {
    Compiler_SSA_Context * ctx = nullptr;

    Compiler_SSA_Function *
    lower_function_internal(const Compiler_IR_Function & source,
                            const bool top_level) const
    {
      auto normalized = Compiler_SSA_Detail::normalize_ir_function(source);
      auto * function = ctx->make<Compiler_SSA_Function>();
      function->id = source.id;
      function->top_level = top_level;
      function->name = source.name;
      function->span = source.span;
      function->type_id = source.type_id;
      function->source_function = &source;
      function->local_slots = source.local_slots;
      function->entry_block = normalized.function.entry_block;
      function->exit_block = normalized.function.exit_block;
      function->dominance =
        Compiler_SSA_Detail::compute_dominance_info(normalized.function);

      for (size_t i = 0; i < normalized.function.blocks.size(); ++i)
        {
          Compiler_SSA_Block block;
          block.id = i;
          block.source_block_id = normalized.source_block_ids.access(i);
          block.label = normalized.function.blocks.access(i).label;
          block.span = normalized.function.blocks.access(i).span;
          block.predecessors = normalized.function.blocks.access(i).predecessors;
          function->blocks.append(std::move(block));
        }

      DynArray<DynArray<Compiler_SSA_Detail::Phi_Spec>> phi_specs;
      DynArray<Compiler_Dataflow_Bit_Set> has_phi;
      DynArray<Compiler_Dataflow_Bit_Set> is_def_block;
      for (size_t i = 0; i < normalized.function.blocks.size(); ++i)
        {
          phi_specs.append({});
          has_phi.append({});
          has_phi.access(i).resize(normalized.function.local_slots.size(), false);
          is_def_block.append({});
          is_def_block.access(i).resize(normalized.function.local_slots.size(), false);
        }

      DynArray<DynArray<Compiler_SSA_Block_Id>> definition_blocks;
      for (size_t i = 0; i < normalized.function.local_slots.size(); ++i)
        definition_blocks.append({});

      for (size_t slot_id = 0; slot_id < normalized.function.local_slots.size(); ++slot_id)
        if (normalized.function.local_slots.access(slot_id).kind == Compiler_IR_Slot_Kind::Parameter)
          {
            definition_blocks.access(slot_id).append(normalized.function.entry_block);
            is_def_block.access(normalized.function.entry_block).set(slot_id, true);
          }

      for (size_t block_id = 0; block_id < normalized.function.blocks.size(); ++block_id)
        {
          const auto & block = normalized.function.blocks.access(block_id);
          for (size_t inst_index = 0; inst_index < block.instructions.size(); ++inst_index)
            {
              const auto & inst = block.instructions.access(inst_index);
              if (inst.kind == Compiler_IR_Instruction_Kind::Store
                  and inst.local_slot_id != compiler_ir_invalid_id()
                  and inst.local_slot_id < normalized.function.local_slots.size()
                  and not is_def_block.access(block_id).test(inst.local_slot_id))
                {
                  definition_blocks.access(inst.local_slot_id).append(block_id);
                  is_def_block.access(block_id).set(inst.local_slot_id, true);
                }
            }
        }

      for (size_t slot_id = 0; slot_id < normalized.function.local_slots.size(); ++slot_id)
        {
          DynArray<Compiler_SSA_Block_Id> worklist = definition_blocks.access(slot_id);
          while (not worklist.is_empty())
            {
              const auto def_block = worklist.pop();
              if (def_block >= function->dominance.dominance_frontiers.size())
                continue;

              for (size_t i = 0;
                   i < function->dominance.dominance_frontiers.access(def_block).size();
                   ++i)
                {
                  const auto frontier =
                    function->dominance.dominance_frontiers.access(def_block).access(i);
                  if (has_phi.access(frontier).test(slot_id))
                    continue;

                  phi_specs.access(frontier).append(
                    {slot_id,
                     normalized.function.local_slots.access(slot_id).type_id,
                     normalized.function.local_slots.access(slot_id).span});
                  has_phi.access(frontier).set(slot_id, true);
                  if (not is_def_block.access(frontier).test(slot_id))
                    worklist.append(frontier);
                }
            }
        }

      for (size_t block_id = 0; block_id < function->blocks.size(); ++block_id)
        for (size_t i = 0; i < phi_specs.access(block_id).size(); ++i)
          {
            Compiler_SSA_Phi phi;
            phi.slot_id = phi_specs.access(block_id).access(i).slot_id;
            phi.type_id = phi_specs.access(block_id).access(i).type_id;
            phi.span = phi_specs.access(block_id).access(i).span;
            for (size_t operand = 0;
                 operand < function->blocks.access(block_id).predecessors.size();
                 ++operand)
              phi.operands.append(0);
            function->blocks.access(block_id).phis.append(std::move(phi));
          }

      DynArray<DynArray<Compiler_SSA_Value_Id>> slot_stacks;
      for (size_t i = 0; i < normalized.function.local_slots.size(); ++i)
        slot_stacks.append({});

      for (size_t slot_id = 0; slot_id < normalized.function.local_slots.size(); ++slot_id)
        if (normalized.function.local_slots.access(slot_id).kind == Compiler_IR_Slot_Kind::Parameter)
          {
            Compiler_SSA_Parameter parameter;
            parameter.value_id = Compiler_SSA_Detail::next_value(*function);
            parameter.slot_id = slot_id;
            parameter.type_id = normalized.function.local_slots.access(slot_id).type_id;
            parameter.name = normalized.function.local_slots.access(slot_id).name;
            parameter.span = normalized.function.local_slots.access(slot_id).span;
            function->parameters.append(parameter);
            slot_stacks.access(slot_id).append(parameter.value_id);
          }

      DynArray<Compiler_SSA_Value_Id> renamed_values;
      for (size_t i = 0; i <= normalized.function.next_value_id; ++i)
        renamed_values.append(0);

      if (function->entry_block != compiler_ssa_invalid_id())
        Compiler_SSA_Detail::rename_recursive(*function,
                                              normalized.function,
                                              function->dominance,
                                              phi_specs,
                                              function->entry_block,
                                              slot_stacks,
                                              renamed_values);

      return function;
    }

  public:
    /** @brief Builds an SSA lowerer over `ctx`. */
    explicit Compiler_SSA_Lowering(Compiler_SSA_Context & context) noexcept
      : ctx(&context)
    {}

    /** @brief Lowers one IR module to SSA form. */
    Compiler_SSA_Module *
    lower_module(const Compiler_IR_Module * module) const
    {
      ah_runtime_error_unless(module != nullptr)
        << "Compiler_SSA_Lowering::lower_module(): null IR module";

      auto * result = ctx->make<Compiler_SSA_Module>();
      result->global_slots = module->global_slots;
      for (size_t i = 0; i < module->functions.size(); ++i)
        result->functions.append(lower_function_internal(*module->functions.access(i), false));
      if (module->top_level != nullptr)
        result->top_level = lower_function_internal(*module->top_level, true);
      return result;
    }
  };

  /** @brief Validates one SSA function structurally and semantically. */
  inline Compiler_SSA_Validation_Report
  validate_ssa_function(const Compiler_SSA_Function & function,
                        const Compiler_SSA_Module * module = nullptr)
  {
    Compiler_SSA_Validation_Report report;

    auto fail = [&report](const std::string & message)
    {
      report.valid = false;
      report.errors.append(message);
    };

    if (function.blocks.is_empty())
      {
        fail("SSA function '" + function.name + "' has no blocks");
        return report;
      }

    if (function.entry_block >= function.blocks.size())
      fail("SSA function '" + function.name + "' has invalid entry block");
    if (function.exit_block >= function.blocks.size())
      fail("SSA function '" + function.name + "' has invalid exit block");

    if (function.dominance.reachable_blocks.size() != function.blocks.size()
        or function.dominance.dominators.size() != function.blocks.size()
        or function.dominance.immediate_dominators.size() != function.blocks.size()
        or function.dominance.dominance_frontiers.size() != function.blocks.size()
        or function.dominance.dominator_tree_children.size() != function.blocks.size())
      fail("SSA function '" + function.name
           + "' has mismatched dominance metadata sizes");

    if (not report.valid)
      return report;

    DynArray<Compiler_SSA_Detail::Definition_Location> definitions;
    for (size_t i = 0; i <= function.next_value_id; ++i)
      definitions.append({});

    for (size_t i = 0; i < function.parameters.size(); ++i)
      {
        const auto & parameter = function.parameters.access(i);
        if (parameter.value_id == 0 or parameter.value_id >= definitions.size())
          fail("SSA parameter in function '" + function.name + "' has invalid value id");
        else if (definitions.access(parameter.value_id).kind
                 != Compiler_SSA_Detail::Definition_Location::Kind::Invalid)
          fail("SSA function '" + function.name + "' defines value "
               + Compiler_SSA_Detail::value_name(parameter.value_id) + " multiple times");
        else
          definitions.access(parameter.value_id) =
            {Compiler_SSA_Detail::Definition_Location::Kind::Parameter,
             function.entry_block,
             i};
      }

    for (size_t block_id = 0; block_id < function.blocks.size(); ++block_id)
      {
        const auto & block = function.blocks.access(block_id);
        if (block.id != block_id)
          fail("SSA function '" + function.name
               + "' has mismatched block id at "
               + Compiler_SSA_Detail::block_name(block_id));
        if (block.terminator.kind == Compiler_IR_Terminator_Kind::None)
          fail("SSA block " + Compiler_SSA_Detail::block_name(block_id)
               + " in function '" + function.name + "' is unterminated");

        if (block_id == function.entry_block
            and function.dominance.immediate_dominators.access(block_id) != compiler_ssa_invalid_id())
          fail("SSA entry block " + Compiler_SSA_Detail::block_name(block_id)
               + " must not have an immediate dominator");

        for (size_t phi_index = 0; phi_index < block.phis.size(); ++phi_index)
          {
            const auto & phi = block.phis.access(phi_index);
            if (phi.result_id == 0 or phi.result_id >= definitions.size())
              fail("Phi in " + Compiler_SSA_Detail::block_name(block_id)
                   + " of function '" + function.name + "' has invalid result id");
            else if (definitions.access(phi.result_id).kind
                     != Compiler_SSA_Detail::Definition_Location::Kind::Invalid)
              fail("SSA function '" + function.name + "' defines value "
                   + Compiler_SSA_Detail::value_name(phi.result_id) + " multiple times");
            else
              definitions.access(phi.result_id) =
                {Compiler_SSA_Detail::Definition_Location::Kind::Phi,
                 block_id,
                 phi_index};

            if (phi.slot_id >= function.local_slots.size())
              fail("Phi in " + Compiler_SSA_Detail::block_name(block_id)
                   + " references an invalid local slot");
            if (phi.operands.size() != block.predecessors.size())
              fail("Phi in " + Compiler_SSA_Detail::block_name(block_id)
                   + " must have one operand per predecessor");
          }

        for (size_t inst_index = 0; inst_index < block.instructions.size(); ++inst_index)
          {
            const auto & inst = block.instructions.access(inst_index);
            if ((inst.kind == Compiler_SSA_Instruction_Kind::Constant
                 or inst.kind == Compiler_SSA_Instruction_Kind::Load_Global
                 or inst.kind == Compiler_SSA_Instruction_Kind::Unary
                 or inst.kind == Compiler_SSA_Instruction_Kind::Binary
                 or inst.kind == Compiler_SSA_Instruction_Kind::Call
                 or inst.kind == Compiler_SSA_Instruction_Kind::Function_Ref)
                and inst.result_id == 0)
              fail("SSA instruction in " + Compiler_SSA_Detail::block_name(block_id)
                   + " of function '" + function.name + "' must define a result");

            if (inst.result_id != 0)
              {
                if (inst.result_id >= definitions.size())
                  fail("SSA instruction in " + Compiler_SSA_Detail::block_name(block_id)
                       + " of function '" + function.name + "' has invalid result id");
                else if (definitions.access(inst.result_id).kind
                         != Compiler_SSA_Detail::Definition_Location::Kind::Invalid)
                  fail("SSA function '" + function.name + "' defines value "
                       + Compiler_SSA_Detail::value_name(inst.result_id) + " multiple times");
                else
                  definitions.access(inst.result_id) =
                    {Compiler_SSA_Detail::Definition_Location::Kind::Instruction,
                     block_id,
                     inst_index};
              }

            if ((inst.kind == Compiler_SSA_Instruction_Kind::Load_Global
                 or inst.kind == Compiler_SSA_Instruction_Kind::Store_Global)
                and (module == nullptr or inst.global_slot_id >= module->global_slots.size()))
              fail("SSA instruction references invalid global slot in function '"
                   + function.name + "'");
            if (inst.kind == Compiler_SSA_Instruction_Kind::Function_Ref
                and (module == nullptr or inst.function_id >= module->functions.size()))
              fail("SSA instruction references invalid function id in function '"
                   + function.name + "'");
          }
      }

    auto dominates = [&function](const Compiler_SSA_Block_Id a,
                                 const Compiler_SSA_Block_Id b) noexcept
    {
      return a < function.dominance.dominators.size()
             and b < function.dominance.dominators.size()
             and function.dominance.dominators.access(b).test(a);
    };

    auto ensure_use = [&](const Compiler_SSA_Value_Id value_id,
                          const Compiler_SSA_Block_Id user_block,
                          const size_t user_order,
                          const bool phi_edge_use,
                          const Compiler_SSA_Block_Id predecessor_block,
                          const std::string & context)
    {
      if (value_id == 0 or value_id >= definitions.size())
        {
          fail(context + " references invalid SSA value");
          return;
        }

      const auto def = definitions.access(value_id);
      if (def.kind == Compiler_SSA_Detail::Definition_Location::Kind::Invalid)
        {
          fail(context + " references undefined SSA value "
               + Compiler_SSA_Detail::value_name(value_id));
          return;
        }

      if (phi_edge_use)
        {
          if (def.kind == Compiler_SSA_Detail::Definition_Location::Kind::Parameter)
            return;
          if (def.block_id == predecessor_block)
            return;
          if (not dominates(def.block_id, predecessor_block))
            fail(context + " references non-dominating SSA value "
                 + Compiler_SSA_Detail::value_name(value_id));
          return;
        }

      if (def.kind == Compiler_SSA_Detail::Definition_Location::Kind::Parameter)
        return;

      if (def.block_id == user_block)
        {
          if (def.kind == Compiler_SSA_Detail::Definition_Location::Kind::Phi)
            return;
          if (def.order < user_order)
            return;
          fail(context + " references SSA value defined later in the same block");
          return;
        }

      if (not dominates(def.block_id, user_block))
        fail(context + " references non-dominating SSA value "
             + Compiler_SSA_Detail::value_name(value_id));
    };

    for (size_t block_id = 0; block_id < function.blocks.size(); ++block_id)
      {
        const auto & block = function.blocks.access(block_id);
        for (size_t phi_index = 0; phi_index < block.phis.size(); ++phi_index)
          for (size_t operand_index = 0; operand_index < block.phis.access(phi_index).operands.size(); ++operand_index)
            ensure_use(block.phis.access(phi_index).operands.access(operand_index),
                       block_id,
                       0,
                       true,
                       block.predecessors.access(operand_index),
                       "Phi in " + Compiler_SSA_Detail::block_name(block_id));

        for (size_t inst_index = 0; inst_index < block.instructions.size(); ++inst_index)
          for (size_t operand_index = 0; operand_index < block.instructions.access(inst_index).operands.size(); ++operand_index)
            ensure_use(block.instructions.access(inst_index).operands.access(operand_index),
                       block_id,
                       inst_index,
                       false,
                       compiler_ssa_invalid_id(),
                       "Instruction in " + Compiler_SSA_Detail::block_name(block_id));

        if (block.terminator.kind == Compiler_IR_Terminator_Kind::Branch)
          ensure_use(block.terminator.condition_value,
                     block_id,
                     block.instructions.size(),
                     false,
                     compiler_ssa_invalid_id(),
                     "Branch terminator in " + Compiler_SSA_Detail::block_name(block_id));
        else if (block.terminator.kind == Compiler_IR_Terminator_Kind::Return)
          ensure_use(block.terminator.return_value,
                     block_id,
                     block.instructions.size(),
                     false,
                     compiler_ssa_invalid_id(),
                     "Return terminator in " + Compiler_SSA_Detail::block_name(block_id));
      }

    return report;
  }

  /** @brief Validates all functions in one SSA module. */
  inline Compiler_SSA_Validation_Report
  validate_ssa_module(const Compiler_SSA_Module & module)
  {
    Compiler_SSA_Validation_Report report;

    auto merge = [&report](const Compiler_SSA_Validation_Report & child)
    {
      if (not child.valid)
        report.valid = false;
      for (size_t i = 0; i < child.errors.size(); ++i)
        report.errors.append(child.errors.access(i));
      for (size_t i = 0; i < child.warnings.size(); ++i)
        report.warnings.append(child.warnings.access(i));
    };

    for (size_t i = 0; i < module.functions.size(); ++i)
      merge(validate_ssa_function(*module.functions.access(i), &module));
    if (module.top_level != nullptr)
      merge(validate_ssa_function(*module.top_level, &module));
    return report;
  }

  /** @brief Dumps one SSA function deterministically. */
  inline std::string
  compiler_dump_ssa_function(const Compiler_SSA_Function * function,
                             const Compiler_SSA_Module * module = nullptr,
                             const Compiler_Type_Context * types = nullptr)
  {
    (void) module;
    std::ostringstream out;
    if (function == nullptr)
      {
        out << "<null-ssa-function>\n";
        return out.str();
      }

    out << "SSAFunction(" << function->name << ")";
    if (types != nullptr and function->type_id != 0)
      out << ": " << types->to_string(function->type_id);
    out << '\n';
    out << "  Entry: " << Compiler_SSA_Detail::block_name(function->entry_block) << '\n';
    out << "  Exit: " << Compiler_SSA_Detail::block_name(function->exit_block) << '\n';

    if (not function->parameters.is_empty())
      {
        out << "  Parameters:\n";
        for (size_t i = 0; i < function->parameters.size(); ++i)
          {
            const auto & parameter = function->parameters.access(i);
            out << "    " << Compiler_SSA_Detail::value_name(parameter.value_id)
                << " <- " << Compiler_SSA_Detail::local_slot_name(parameter.slot_id)
                << " (" << parameter.name << ")";
            if (types != nullptr and parameter.type_id != 0)
              out << " : " << types->to_string(parameter.type_id);
            out << '\n';
          }
      }

    for (size_t block_id = 0; block_id < function->blocks.size(); ++block_id)
      {
        const auto & block = function->blocks.access(block_id);
        out << "  Block " << Compiler_SSA_Detail::block_name(block_id)
            << " [src=" << Compiler_IR_Detail::block_name(block.source_block_id)
            << ", " << block.label << "]\n";

        if (block.phis.is_empty())
          out << "    Phis: <none>\n";
        else
          {
            out << "    Phis:\n";
            for (size_t phi_index = 0; phi_index < block.phis.size(); ++phi_index)
              {
                const auto & phi = block.phis.access(phi_index);
                out << "      " << Compiler_SSA_Detail::value_name(phi.result_id)
                    << " = Phi " << Compiler_SSA_Detail::local_slot_name(phi.slot_id)
                    << " [";
                for (size_t operand_index = 0; operand_index < phi.operands.size(); ++operand_index)
                  {
                    if (operand_index > 0)
                      out << ", ";
                    out << Compiler_SSA_Detail::block_name(block.predecessors.access(operand_index))
                        << ": "
                        << Compiler_SSA_Detail::value_name(phi.operands.access(operand_index));
                  }
                out << "]";
                if (types != nullptr and phi.type_id != 0)
                  out << " : " << types->to_string(phi.type_id);
                out << '\n';
              }
          }

        if (block.instructions.is_empty())
          out << "    Instructions: <none>\n";
        else
          {
            out << "    Instructions:\n";
            for (size_t inst_index = 0; inst_index < block.instructions.size(); ++inst_index)
              {
                const auto & inst = block.instructions.access(inst_index);
                out << "      ";
                if (inst.result_id != 0)
                  out << Compiler_SSA_Detail::value_name(inst.result_id) << " = ";
                out << compiler_ssa_instruction_kind_name(inst.kind);
                if (inst.kind == Compiler_SSA_Instruction_Kind::Load_Global
                    or inst.kind == Compiler_SSA_Instruction_Kind::Store_Global)
                  out << ' ' << Compiler_SSA_Detail::global_slot_name(inst.global_slot_id);
                else if (inst.kind == Compiler_SSA_Instruction_Kind::Function_Ref)
                  out << ' ' << Compiler_SSA_Detail::function_name(inst.function_id);
                else if (inst.kind == Compiler_SSA_Instruction_Kind::Unary
                         or inst.kind == Compiler_SSA_Instruction_Kind::Binary)
                  out << '(' << Compiler_SSA_Detail::token_name(inst.op) << ')';
                else if (inst.kind == Compiler_SSA_Instruction_Kind::Constant)
                  out << '(' << inst.text << ')';

                if (not inst.operands.is_empty())
                  {
                    out << " [";
                    for (size_t operand = 0; operand < inst.operands.size(); ++operand)
                      {
                        if (operand > 0)
                          out << ", ";
                        out << Compiler_SSA_Detail::value_name(inst.operands.access(operand));
                      }
                    out << ']';
                  }

                if (types != nullptr and inst.type_id != 0)
                  out << " : " << types->to_string(inst.type_id);
                out << '\n';
              }
          }

        out << "    Terminator: "
            << compiler_ir_terminator_kind_name(block.terminator.kind);
        if (block.terminator.kind == Compiler_IR_Terminator_Kind::Branch)
          out << " [" << Compiler_SSA_Detail::value_name(block.terminator.condition_value) << "]";
        else if (block.terminator.kind == Compiler_IR_Terminator_Kind::Return)
          out << " [" << Compiler_SSA_Detail::value_name(block.terminator.return_value) << "]";
        out << '\n';

        out << "    Successors:";
        if (block.terminator.successors.is_empty())
          out << " <none>\n";
        else
          {
            for (size_t i = 0; i < block.terminator.successors.size(); ++i)
              {
                out << (i == 0 ? " " : ", ");
                out << Compiler_SSA_Detail::block_name(block.terminator.successors.access(i));
              }
            out << '\n';
          }

        out << "    IDom: ";
        const auto idom = function->dominance.immediate_dominators.access(block_id);
        if (idom == compiler_ssa_invalid_id())
          out << "<none>\n";
        else
          out << Compiler_SSA_Detail::block_name(idom) << '\n';

        out << "    Frontier:";
        if (function->dominance.dominance_frontiers.access(block_id).is_empty())
          out << " <none>\n";
        else
          {
            for (size_t i = 0; i < function->dominance.dominance_frontiers.access(block_id).size(); ++i)
              {
                out << (i == 0 ? " " : ", ");
                out << Compiler_SSA_Detail::block_name(
                  function->dominance.dominance_frontiers.access(block_id).access(i));
              }
            out << '\n';
          }
      }

    return out.str();
  }

  /** @brief Dumps all SSA functions in one module deterministically. */
  inline std::string
  compiler_dump_ssa_module(const Compiler_SSA_Module * module,
                           const Compiler_Type_Context * types = nullptr)
  {
    std::ostringstream out;
    if (module == nullptr)
      {
        out << "<null-ssa-module>\n";
        return out.str();
      }

    out << "SSAModule\n";
    if (not module->global_slots.is_empty())
      {
        out << "  Globals:\n";
        for (size_t i = 0; i < module->global_slots.size(); ++i)
          {
            const auto & slot = module->global_slots.access(i);
            out << "    " << Compiler_SSA_Detail::global_slot_name(slot.id)
                << " " << slot.name;
            if (types != nullptr and slot.type_id != 0)
              out << ": " << types->to_string(slot.type_id);
            out << '\n';
          }
      }

    for (size_t i = 0; i < module->functions.size(); ++i)
      Compiler_SSA_Detail::append_indented_text(out,
                                                compiler_dump_ssa_function(
                                                  module->functions.access(i),
                                                  module,
                                                  types),
                                                2);
    if (module->top_level != nullptr)
      Compiler_SSA_Detail::append_indented_text(out,
                                                compiler_dump_ssa_function(
                                                  module->top_level,
                                                  module,
                                                  types),
                                                2);
    return out.str();
  }
}

#endif
