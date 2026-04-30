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

/** @file Compiler_IR_Model.H
 *  @brief Reusable explicit-value IR model, validation, and deterministic dumps.
 *
 *  This header defines the stable middle-level IR node model that language
 *  frontends, optimizers, interpreters, and source-emitting backends can share
 *  without depending on the current reference frontend. It includes:
 *
 *  - explicit result values (`v0`, `v1`, ...)
 *  - explicit storage slots for parameters, locals, and globals
 *  - linear instructions inside basic blocks
 *  - explicit control-flow terminators
 *  - structural validation helpers
 *  - deterministic textual dump helpers
 *
 *  Lowering from the current MVP HIR lives separately in
 *  `Compiler_IR_Lowering_MVP.H`.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_IR_MODEL_H
#define COMPILER_IR_MODEL_H

#include <limits>
#include <sstream>
#include <string>
#include <utility>

#include <Compiler_HIR_Model.H>
#include <ah-arena.H>
#include <ah-errors.H>
#include <tpl_dynArray.H>

namespace Aleph {
using Compiler_IR_Value_Id = size_t;
using Compiler_IR_Local_Slot_Id = size_t;
using Compiler_IR_Global_Slot_Id = size_t;
using Compiler_IR_Function_Id = size_t;
using Compiler_IR_Block_Id = size_t;

/** @brief Returns the sentinel invalid IR id. */
inline constexpr size_t compiler_ir_invalid_id() noexcept
{
  return std::numeric_limits<size_t>::max();
}

/** @brief Storage-space classification for slots. */
enum class Compiler_IR_Slot_Kind
{
  Parameter,
  Local,
  Global
};

/** @brief Instruction kinds supported by the MVP IR. */
enum class Compiler_IR_Instruction_Kind
{
  Constant,
  Load,
  Store,
  Unary,
  Binary,
  Call,
  Function_Ref
};

/** @brief Terminator kinds supported by the MVP IR. */
enum class Compiler_IR_Terminator_Kind
{
  None,
  Jump,
  Branch,
  Return,
  Exit,
  Unreachable
};

/** @brief Stable debug name for one slot kind. */
inline const char *compiler_ir_slot_kind_name(const Compiler_IR_Slot_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_IR_Slot_Kind::Parameter:
      return "Parameter";
    case Compiler_IR_Slot_Kind::Local:
      return "Local";
    case Compiler_IR_Slot_Kind::Global:
      return "Global";
    }
  return "Unknown";
}

/** @brief Stable debug name for one instruction kind. */
inline const char *compiler_ir_instruction_kind_name(const Compiler_IR_Instruction_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_IR_Instruction_Kind::Constant:
      return "Constant";
    case Compiler_IR_Instruction_Kind::Load:
      return "Load";
    case Compiler_IR_Instruction_Kind::Store:
      return "Store";
    case Compiler_IR_Instruction_Kind::Unary:
      return "Unary";
    case Compiler_IR_Instruction_Kind::Binary:
      return "Binary";
    case Compiler_IR_Instruction_Kind::Call:
      return "Call";
    case Compiler_IR_Instruction_Kind::Function_Ref:
      return "FunctionRef";
    }
  return "Unknown";
}

/** @brief Stable debug name for one terminator kind. */
inline const char *compiler_ir_terminator_kind_name(const Compiler_IR_Terminator_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_IR_Terminator_Kind::None:
      return "None";
    case Compiler_IR_Terminator_Kind::Jump:
      return "Jump";
    case Compiler_IR_Terminator_Kind::Branch:
      return "Branch";
    case Compiler_IR_Terminator_Kind::Return:
      return "Return";
    case Compiler_IR_Terminator_Kind::Exit:
      return "Exit";
    case Compiler_IR_Terminator_Kind::Unreachable:
      return "Unreachable";
    }
  return "Unknown";
}

/** @brief One storage slot used by the IR. */
struct Compiler_IR_Slot
{
  size_t id = 0;  ///< Stable slot id within its storage space.
  Compiler_IR_Slot_Kind kind = Compiler_IR_Slot_Kind::Local;  ///< Slot storage category.
  std::string name;                                           ///< Debug name of the slot.
  Compiler_Type_Id type_id = 0;                               ///< Declared or inferred slot type.
  Source_Span span;                                           ///< Declaration span.
};

/** @brief One instruction producing an optional explicit result value. */
struct Compiler_IR_Instruction
{
  Compiler_IR_Instruction_Kind kind
    = Compiler_IR_Instruction_Kind::Constant;  ///< Instruction category.
  Compiler_IR_Value_Id result_id = 0;          ///< Produced value id, or `0` for void instructions.
  Compiler_Type_Id type_id = 0;                ///< Result type or stored value type.
  Source_Span span;                            ///< Source region associated with the instruction.
  Compiler_Operator_Kind op
    = Compiler_Operator_Kind::Invalid;  ///< Unary/binary operator kind, when relevant.
  Compiler_IR_Local_Slot_Id local_slot_id
    = compiler_ir_invalid_id();  ///< Referenced local/parameter slot, when relevant.
  Compiler_IR_Global_Slot_Id global_slot_id
    = compiler_ir_invalid_id();  ///< Referenced global slot, when relevant.
  Compiler_IR_Function_Id function_id
    = compiler_ir_invalid_id();             ///< Referenced function id, when relevant.
  DynArray<Compiler_IR_Value_Id> operands;  ///< Input values in deterministic order.
  std::string text;                         ///< Constant spelling or debug payload.
  bool bool_value = false;                  ///< Decoded boolean payload for bool constants.
};

/** @brief Explicit terminator for one IR basic block. */
struct Compiler_IR_Terminator
{
  Compiler_IR_Terminator_Kind kind = Compiler_IR_Terminator_Kind::None;  ///< Terminator category.
  Source_Span span;                           ///< Source region associated with the terminator.
  Compiler_IR_Value_Id condition_value = 0;   ///< Branch condition value, if any.
  Compiler_IR_Value_Id return_value = 0;      ///< Return value, if any.
  DynArray<Compiler_IR_Block_Id> successors;  ///< Successor blocks in deterministic order.
};

/** @brief One basic block of IR instructions. */
struct Compiler_IR_Block
{
  Compiler_IR_Block_Id id = compiler_ir_invalid_id();  ///< Stable block id.
  std::string label;                                   ///< Deterministic debug label.
  Source_Span span;  ///< Aggregate span covered by instructions and terminator.
  DynArray<Compiler_IR_Instruction> instructions;  ///< Linear instruction list.
  Compiler_IR_Terminator terminator;               ///< Explicit block terminator.
  DynArray<Compiler_IR_Block_Id> predecessors;     ///< Predecessor block ids.

  /** @brief Returns whether the block already has a terminator. */
  bool is_terminated() const noexcept
  {
    return terminator.kind != Compiler_IR_Terminator_Kind::None;
  }
};

/** @brief Lowered IR for one function or top-level body. */
struct Compiler_IR_Function
{
  Compiler_IR_Function_Id id = compiler_ir_invalid_id();  ///< Stable function id within the module.
  std::string name;                                       ///< Debug or source-level name.
  Source_Span span;                                       ///< Source span of the function/body.
  Compiler_Type_Id type_id = 0;                           ///< Full function type, when known.
  const Compiler_HIR_Function *source_function
    = nullptr;  ///< Originating HIR function, or `nullptr` for top-level.
  DynArray<Compiler_IR_Slot> local_slots;  ///< Parameter/local slots in stable order.
  DynArray<Compiler_IR_Block> blocks;      ///< Basic blocks in deterministic id order.
  Compiler_IR_Block_Id entry_block = compiler_ir_invalid_id();  ///< Canonical entry block.
  Compiler_IR_Block_Id exit_block = compiler_ir_invalid_id();   ///< Canonical exit block.
  Compiler_IR_Value_Id next_value_id = 0;  ///< Next value id to allocate while lowering.

  /** @brief Returns whether this IR function represents the top-level body. */
  bool is_top_level() const noexcept
  {
    return source_function == nullptr;
  }
};

/** @brief Lowered IR module with shared global slots and functions. */
struct Compiler_IR_Module
{
  DynArray<Compiler_IR_Slot> global_slots;     ///< Module-wide global slots.
  DynArray<Compiler_IR_Function *> functions;  ///< Lowered non-top-level functions.
  Compiler_IR_Function *top_level = nullptr;   ///< Optional top-level body lowered as a function.
};

/** @brief Arena-backed ownership context for IR nodes. */
class Compiler_IR_Context
{
  struct Owned_Object
  {
    void *ptr = nullptr;
    void (*destroy)(void *) noexcept = nullptr;
  };

  AhArenaAllocator arena;
  DynArray<Owned_Object> owned;

public:
  /** @brief Constructs an IR context with `arena_size` bytes. */
  explicit Compiler_IR_Context(const size_t arena_size = AhArenaAllocator::DEFAULT_SIZE)
    : arena(arena_size)
  {}

  Compiler_IR_Context(const Compiler_IR_Context &) = delete;
  Compiler_IR_Context &operator=(const Compiler_IR_Context &) = delete;

  ~Compiler_IR_Context() noexcept
  {
    reset();
  }

  /** @brief Allocates and constructs one IR object. */
  template <typename T, typename... Args>
  T *make(Args &&...args)
  {
    T *ptr = allocate<T>(arena, std::forward<Args>(args)...);
    ah_runtime_error_unless(ptr != nullptr) << "Compiler_IR_Context: arena allocation failed";

    owned.append({ptr,
                  [](void *raw) noexcept
    {
      static_cast<T *>(raw)->~T();
    }});
    return ptr;
  }

  /** @brief Destroys all tracked objects and rewinds the arena. */
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

/** @brief Structural validation report for one IR module or function. */
struct Compiler_IR_Validation_Report
{
  bool valid = true;               ///< Whether all hard validation checks passed.
  DynArray<std::string> errors;    ///< Hard validation failures.
  DynArray<std::string> warnings;  ///< Non-fatal findings such as unreachable blocks.
};

namespace Compiler_IR_Detail {
inline std::string block_name(const Compiler_IR_Block_Id id)
{
  return "B" + std::to_string(id);
}

inline bool contains_block_id(const DynArray<Compiler_IR_Block_Id> &ids,
                              const Compiler_IR_Block_Id id) noexcept
{
  for (size_t i = 0; i < ids.size(); ++i)
    if (ids.access(i) == id)
      return true;
  return false;
}

inline std::string value_name(const Compiler_IR_Value_Id id)
{
  return "v" + std::to_string(id);
}

inline std::string local_slot_name(const Compiler_IR_Local_Slot_Id id)
{
  return "s" + std::to_string(id);
}

inline std::string global_slot_name(const Compiler_IR_Global_Slot_Id id)
{
  return "g" + std::to_string(id);
}

inline std::string function_name(const Compiler_IR_Function_Id id)
{
  return "f" + std::to_string(id);
}

inline void append_indented_text(std::ostream &out, const std::string &text, const size_t indent)
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
}  // namespace Compiler_IR_Detail

/** @brief Validates one lowered IR function structurally. */
inline Compiler_IR_Validation_Report validate_ir_function(const Compiler_IR_Function &function,
                                                          const Compiler_IR_Module *module = nullptr)
{
  Compiler_IR_Validation_Report report;

  auto fail = [&report](const std::string &msg)
  {
    report.valid = false;
    report.errors.append(msg);
  };

  if (function.blocks.is_empty())
    {
      fail("IR function '" + function.name + "' has no blocks");
      return report;
    }

  if (function.entry_block >= function.blocks.size())
    fail("IR function '" + function.name + "' has invalid entry block");
  if (function.exit_block >= function.blocks.size())
    fail("IR function '" + function.name + "' has invalid exit block");
  if (not report.valid)
    return report;

  for (size_t i = 0; i < function.blocks.size(); ++i)
    {
      const auto &block = function.blocks.access(i);
      if (block.id != i)
        fail("IR function '" + function.name + "' has mismatched block id at "
             + Compiler_IR_Detail::block_name(i));
      if (block.terminator.kind == Compiler_IR_Terminator_Kind::None)
        fail("IR block " + Compiler_IR_Detail::block_name(i) + " in function '" + function.name
             + "' is unterminated");

      for (size_t j = 0; j < block.instructions.size(); ++j)
        {
          const auto &inst = block.instructions.access(j);
          if ((inst.kind == Compiler_IR_Instruction_Kind::Constant
               or inst.kind == Compiler_IR_Instruction_Kind::Load
               or inst.kind == Compiler_IR_Instruction_Kind::Unary
               or inst.kind == Compiler_IR_Instruction_Kind::Binary
               or inst.kind == Compiler_IR_Instruction_Kind::Call
               or inst.kind == Compiler_IR_Instruction_Kind::Function_Ref)
              and inst.result_id == 0)
            fail("IR instruction in " + Compiler_IR_Detail::block_name(i) + " of function '"
                 + function.name + "' must define a result");

          if (inst.local_slot_id != compiler_ir_invalid_id()
              and inst.local_slot_id >= function.local_slots.size())
            fail("IR instruction references invalid local slot in function '" + function.name + "'");
          if (inst.global_slot_id != compiler_ir_invalid_id()
              and (module == nullptr or inst.global_slot_id >= module->global_slots.size()))
            fail("IR instruction references invalid global slot in function '" + function.name
                 + "'");
          if (inst.function_id != compiler_ir_invalid_id()
              and (module == nullptr or inst.function_id >= module->functions.size()))
            fail("IR instruction references invalid function id in function '" + function.name
                 + "'");
        }

      switch (block.terminator.kind)
        {
        case Compiler_IR_Terminator_Kind::Jump:
          if (block.terminator.successors.size() != 1)
            fail("Jump terminator in " + Compiler_IR_Detail::block_name(i)
                 + " must have one successor");
          break;
        case Compiler_IR_Terminator_Kind::Branch:
          if (block.terminator.condition_value == 0)
            fail("Branch terminator in " + Compiler_IR_Detail::block_name(i)
                 + " must reference a condition value");
          if (block.terminator.successors.size() != 2)
            fail("Branch terminator in " + Compiler_IR_Detail::block_name(i)
                 + " must have two successors");
          break;
        case Compiler_IR_Terminator_Kind::Return:
          if (block.terminator.return_value == 0)
            fail("Return terminator in " + Compiler_IR_Detail::block_name(i)
                 + " must reference a return value");
          if (block.terminator.successors.size() != 1)
            fail("Return terminator in " + Compiler_IR_Detail::block_name(i)
                 + " must point to the exit block");
          else if (block.terminator.successors.access(0) != function.exit_block)
            fail("Return terminator in " + Compiler_IR_Detail::block_name(i)
                 + " must point to exit block "
                 + Compiler_IR_Detail::block_name(function.exit_block));
          break;
        case Compiler_IR_Terminator_Kind::Exit:
          if (block.id != function.exit_block)
            fail("Only the canonical exit block may use Exit in function '" + function.name + "'");
          if (not block.terminator.successors.is_empty())
            fail("Exit block " + Compiler_IR_Detail::block_name(i) + " must not have successors");
          break;
        case Compiler_IR_Terminator_Kind::Unreachable:
        case Compiler_IR_Terminator_Kind::None:
          break;
        }

      for (size_t j = 0; j < block.terminator.successors.size(); ++j)
        {
          const auto succ = block.terminator.successors.access(j);
          if (succ >= function.blocks.size())
            fail("IR block " + Compiler_IR_Detail::block_name(i) + " references invalid successor "
                 + Compiler_IR_Detail::block_name(succ));
        }
    }

  DynArray<bool> reachable;
  for (size_t i = 0; i < function.blocks.size(); ++i)
    reachable.append(false);

  DynArray<Compiler_IR_Block_Id> worklist;
  worklist.append(function.entry_block);
  while (not worklist.is_empty())
    {
      const auto current = worklist.pop();
      if (reachable.access(current))
        continue;
      reachable.access(current) = true;
      const auto &block = function.blocks.access(current);
      for (size_t i = 0; i < block.terminator.successors.size(); ++i)
        {
          const auto succ = block.terminator.successors.access(i);
          if (succ < function.blocks.size() and not reachable.access(succ))
            worklist.append(succ);
        }
    }

  for (size_t i = 0; i < function.blocks.size(); ++i)
    if (not reachable.access(i))
      report.warnings.append("IR block " + Compiler_IR_Detail::block_name(i) + " in function '"
                             + function.name + "' is unreachable from entry");

  return report;
}

/** @brief Validates all functions in one lowered IR module. */
inline Compiler_IR_Validation_Report validate_ir_module(const Compiler_IR_Module &module)
{
  Compiler_IR_Validation_Report report;

  auto merge = [&report](const Compiler_IR_Validation_Report &child)
  {
    if (not child.valid)
      report.valid = false;
    for (size_t i = 0; i < child.errors.size(); ++i)
      report.errors.append(child.errors.access(i));
    for (size_t i = 0; i < child.warnings.size(); ++i)
      report.warnings.append(child.warnings.access(i));
  };

  for (size_t i = 0; i < module.functions.size(); ++i)
    merge(validate_ir_function(*module.functions.access(i), &module));
  if (module.top_level != nullptr)
    merge(validate_ir_function(*module.top_level, &module));
  return report;
}

/** @brief Dumps one lowered IR function deterministically. */
inline std::string compiler_dump_ir_function(const Compiler_IR_Function *function,
                                             const Compiler_IR_Module *module = nullptr,
                                             const Compiler_Type_Context *types = nullptr)
{
  (void) module;
  std::ostringstream out;
  if (function == nullptr)
    {
      out << "<null-ir-function>\n";
      return out.str();
    }

  out << "IRFunction(" << function->name << ")";
  if (types != nullptr and function->type_id != 0)
    out << ": " << types->to_string(function->type_id);
  out << '\n';
  out << "  Entry: " << Compiler_IR_Detail::block_name(function->entry_block) << '\n';
  out << "  Exit: " << Compiler_IR_Detail::block_name(function->exit_block) << '\n';

  if (not function->local_slots.is_empty())
    {
      out << "  Slots:\n";
      for (size_t i = 0; i < function->local_slots.size(); ++i)
        {
          const auto &slot = function->local_slots.access(i);
          out << "    " << Compiler_IR_Detail::local_slot_name(slot.id) << " ["
              << compiler_ir_slot_kind_name(slot.kind) << "] " << slot.name;
          if (types != nullptr and slot.type_id != 0)
            out << ": " << types->to_string(slot.type_id);
          out << '\n';
        }
    }

  for (size_t i = 0; i < function->blocks.size(); ++i)
    {
      const auto &block = function->blocks.access(i);
      out << "  Block " << Compiler_IR_Detail::block_name(block.id) << " [" << block.label << "]\n";

      if (block.instructions.is_empty())
        out << "    Instructions: <none>\n";
      else
        {
          out << "    Instructions:\n";
          for (size_t j = 0; j < block.instructions.size(); ++j)
            {
              const auto &inst = block.instructions.access(j);
              out << "      ";
              if (inst.result_id != 0)
                out << Compiler_IR_Detail::value_name(inst.result_id) << " = ";
              out << compiler_ir_instruction_kind_name(inst.kind);

              if (inst.kind == Compiler_IR_Instruction_Kind::Load)
                {
                  out << ' ';
                  if (inst.global_slot_id != compiler_ir_invalid_id())
                    out << Compiler_IR_Detail::global_slot_name(inst.global_slot_id);
                  else
                    out << Compiler_IR_Detail::local_slot_name(inst.local_slot_id);
                }
              else if (inst.kind == Compiler_IR_Instruction_Kind::Store)
                {
                  out << ' ';
                  if (inst.global_slot_id != compiler_ir_invalid_id())
                    out << Compiler_IR_Detail::global_slot_name(inst.global_slot_id);
                  else
                    out << Compiler_IR_Detail::local_slot_name(inst.local_slot_id);
                }
              else if (inst.kind == Compiler_IR_Instruction_Kind::Function_Ref)
                out << ' ' << Compiler_IR_Detail::function_name(inst.function_id);
              else if (inst.kind == Compiler_IR_Instruction_Kind::Unary
                       or inst.kind == Compiler_IR_Instruction_Kind::Binary)
                out << '(' << compiler_operator_name(inst.op) << ')';
              else if (inst.kind == Compiler_IR_Instruction_Kind::Constant)
                out << '(' << inst.text << ')';

              if (not inst.operands.is_empty())
                {
                  out << " [";
                  for (size_t k = 0; k < inst.operands.size(); ++k)
                    {
                      if (k > 0)
                        out << ", ";
                      out << Compiler_IR_Detail::value_name(inst.operands.access(k));
                    }
                  out << ']';
                }

              if (types != nullptr and inst.type_id != 0)
                out << " : " << types->to_string(inst.type_id);
              out << '\n';
            }
        }

      out << "    Terminator: " << compiler_ir_terminator_kind_name(block.terminator.kind);
      if (block.terminator.kind == Compiler_IR_Terminator_Kind::Branch)
        out << " [" << Compiler_IR_Detail::value_name(block.terminator.condition_value) << "]";
      else if (block.terminator.kind == Compiler_IR_Terminator_Kind::Return)
        out << " [" << Compiler_IR_Detail::value_name(block.terminator.return_value) << "]";
      out << '\n';

      out << "    Successors:";
      if (block.terminator.successors.is_empty())
        out << " <none>\n";
      else
        {
          for (size_t j = 0; j < block.terminator.successors.size(); ++j)
            {
              out << (j == 0 ? " " : ", ");
              out << Compiler_IR_Detail::block_name(block.terminator.successors.access(j));
            }
          out << '\n';
        }
    }

  return out.str();
}

/** @brief Dumps all lowered IR in one module deterministically. */
inline std::string compiler_dump_ir_module(const Compiler_IR_Module *module,
                                           const Compiler_Type_Context *types = nullptr)
{
  std::ostringstream out;
  if (module == nullptr)
    {
      out << "<null-ir-module>\n";
      return out.str();
    }

  out << "IRModule\n";
  if (not module->global_slots.is_empty())
    {
      out << "  Globals:\n";
      for (size_t i = 0; i < module->global_slots.size(); ++i)
        {
          const auto &slot = module->global_slots.access(i);
          out << "    " << Compiler_IR_Detail::global_slot_name(slot.id) << " " << slot.name;
          if (types != nullptr and slot.type_id != 0)
            out << ": " << types->to_string(slot.type_id);
          out << '\n';
        }
    }

  for (size_t i = 0; i < module->functions.size(); ++i)
    Compiler_IR_Detail::append_indented_text(
      out, compiler_dump_ir_function(module->functions.access(i), module, types), 2);
  if (module->top_level != nullptr)
    Compiler_IR_Detail::append_indented_text(
      out, compiler_dump_ir_function(module->top_level, module, types), 2);
  return out.str();
}
}  // namespace Aleph

#endif
