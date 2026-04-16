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

/** @file Compiler_IR.H
 *  @brief Explicit value-based IR with lowering from structured HIR.
 *
 *  This header introduces the first explicit middle-level IR for the language
 *  implementation platform. Compared with `Compiler_HIR.H`, this IR exposes:
 *
 *  - explicit result values (`v0`, `v1`, ...)
 *  - explicit storage slots for parameters, locals, and globals
 *  - linear instructions inside basic blocks
 *  - terminators detached from expression trees
 *
 *  The current IR is intentionally non-SSA. It is meant to be the bridge from
 *  structured HIR/CFG into later dataflow and SSA passes without committing to
 *  a target backend yet.
 *
 *  The MVP includes:
 *
 *  - basic blocks and control-flow terminators
 *  - constants, loads, stores, unary/binary ops, calls, and function refs
 *  - module-level global slots
 *  - lowering from typed HIR while preserving lexical scoping
 *  - structural validation and deterministic textual dumps
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_IR_H
#define COMPILER_IR_H

#include <limits>
#include <sstream>
#include <string>
#include <utility>

#include <Compiler_HIR.H>
#include <Compiler_Token.H>
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
  Compiler_Token_Kind op
    = Compiler_Token_Kind::Invalid;  ///< Unary/binary operator kind, when relevant.
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
/** @brief Storage class used by the lowerer for one name binding. */
struct Binding
{
  enum class Kind
  {
    Local_Slot,
    Global_Slot,
    Function
  };

  Kind kind = Kind::Local_Slot;
  size_t id = compiler_ir_invalid_id();
  Compiler_Type_Id type_id = 0;
};

/** @brief One lexical scope frame used during IR lowering. */
struct Scope_Frame
{
  DynArray<std::pair<std::string, Binding>> bindings;
};

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

/** @brief Lowers typed HIR into explicit-value IR. */
class Compiler_IR_Lowering
{
  struct Module_Builder
  {
    Compiler_IR_Module *module = nullptr;
    const Compiler_HIR_Module *hir = nullptr;
    Compiler_IR_Context *ctx = nullptr;
    const Compiler_Type_Context *types = nullptr;
    size_t if_counter = 0;
    size_t while_counter = 0;
    size_t dead_counter = 0;
    DynArray<std::pair<std::string, Compiler_IR_Function_Id>> function_ids;

    Compiler_IR_Function &function(Compiler_IR_Function_Id id) const
    {
      return *module->functions.access(id);
    }

    Compiler_IR_Function_Id find_function_id(const std::string &name) const noexcept
    {
      for (size_t i = 0; i < function_ids.size(); ++i)
        if (function_ids.access(i).first == name)
          return function_ids.access(i).second;
      return compiler_ir_invalid_id();
    }

    Compiler_IR_Global_Slot_Id find_global_slot_id(const std::string &name) const noexcept
    {
      for (size_t i = 0; i < module->global_slots.size(); ++i)
        if (module->global_slots.access(i).name == name)
          return i;
      return compiler_ir_invalid_id();
    }

    Compiler_IR_Global_Slot_Id add_global_slot(const std::string &name,
                                               const Compiler_Type_Id type_id,
                                               const Source_Span &span) const
    {
      Compiler_IR_Slot slot;
      slot.id = module->global_slots.size();
      slot.kind = Compiler_IR_Slot_Kind::Global;
      slot.name = name;
      slot.type_id = type_id;
      slot.span = span;
      module->global_slots.append(std::move(slot));
      return module->global_slots.size() - 1;
    }

    Compiler_IR_Function_Id predeclare_function(const Compiler_HIR_Function *hir_function)
    {
      auto *ir_function = ctx->make<Compiler_IR_Function>();
      ir_function->id = module->functions.size();
      ir_function->name = hir_function->name;
      ir_function->span = hir_function->span;
      ir_function->type_id = hir_function->type_id;
      ir_function->source_function = hir_function;
      ir_function->next_value_id = 0;
      module->functions.append(ir_function);
      function_ids.append({hir_function->name, ir_function->id});
      return ir_function->id;
    }

    static Compiler_IR_Block &block(const Compiler_IR_Function &function, const Compiler_IR_Block_Id id)
    {
      return function.blocks.access(id);
    }

    static void merge_span(Source_Span &dst, const Source_Span &src)
    {
      if (not src.is_valid())
        return;
      if (not dst.is_valid())
        {
          dst = src;
          return;
        }
      dst = compiler_merge_spans(dst, src);
    }

    static Compiler_IR_Block_Id create_block(Compiler_IR_Function &function, std::string label)
    {
      Compiler_IR_Block block;
      block.id = function.blocks.size();
      block.label = std::move(label);
      function.blocks.append(std::move(block));
      return function.blocks.size() - 1;
    }

    static void add_edge(const Compiler_IR_Function &function,
                  const Compiler_IR_Block_Id from,
                  const Compiler_IR_Block_Id to)
    {
      auto &src = block(function, from);
      auto &dst = block(function, to);
      if (not Compiler_IR_Detail::contains_block_id(src.terminator.successors, to))
        src.terminator.successors.append(to);
      if (not Compiler_IR_Detail::contains_block_id(dst.predecessors, from))
        dst.predecessors.append(from);
    }

    static Compiler_IR_Value_Id next_value(Compiler_IR_Function &function) noexcept
    {
      ++function.next_value_id;
      return function.next_value_id;
    }

    static Compiler_IR_Local_Slot_Id add_local_slot(Compiler_IR_Function &function,
                                             const Compiler_IR_Slot_Kind kind,
                                             const std::string &name,
                                             const Compiler_Type_Id type_id,
                                             const Source_Span &span)
    {
      Compiler_IR_Slot slot;
      slot.id = function.local_slots.size();
      slot.kind = kind;
      slot.name = name;
      slot.type_id = type_id;
      slot.span = span;
      function.local_slots.append(std::move(slot));
      return function.local_slots.size() - 1;
    }

    static void emit_instruction(const Compiler_IR_Function &function,
                          const Compiler_IR_Block_Id block_id,
                          Compiler_IR_Instruction instruction)
    {
      auto &blk = block(function, block_id);
      merge_span(blk.span, instruction.span);
      blk.instructions.append(std::move(instruction));
    }

    static void set_jump(const Compiler_IR_Function &function,
                  const Compiler_IR_Block_Id from,
                  const Compiler_IR_Block_Id to,
                  const Source_Span &span = {})
    {
      auto &blk = block(function, from);
      ah_runtime_error_unless(not blk.is_terminated())
        << "Compiler_IR_Lowering: block already terminated";
      blk.terminator.kind = Compiler_IR_Terminator_Kind::Jump;
      blk.terminator.span = span;
      merge_span(blk.span, span);
      add_edge(function, from, to);
    }

    static void set_branch(const Compiler_IR_Function &function,
                    const Compiler_IR_Block_Id from,
                    const Compiler_IR_Value_Id condition,
                    const Source_Span &span,
                    const Compiler_IR_Block_Id then_block,
                    const Compiler_IR_Block_Id else_block)
    {
      auto &blk = block(function, from);
      ah_runtime_error_unless(not blk.is_terminated())
        << "Compiler_IR_Lowering: block already terminated";
      blk.terminator.kind = Compiler_IR_Terminator_Kind::Branch;
      blk.terminator.condition_value = condition;
      blk.terminator.span = span;
      merge_span(blk.span, span);
      add_edge(function, from, then_block);
      add_edge(function, from, else_block);
    }

    static void set_return(const Compiler_IR_Function &function,
                    const Compiler_IR_Block_Id from,
                    const Compiler_IR_Value_Id value,
                    const Source_Span &span = {})
    {
      auto &blk = block(function, from);
      ah_runtime_error_unless(not blk.is_terminated())
        << "Compiler_IR_Lowering: block already terminated";
      blk.terminator.kind = Compiler_IR_Terminator_Kind::Return;
      blk.terminator.return_value = value;
      blk.terminator.span = span;
      merge_span(blk.span, span);
      add_edge(function, from, function.exit_block);
    }

    static void set_exit(const Compiler_IR_Function &function, const Compiler_IR_Block_Id id)
    {
      auto &blk = block(function, id);
      blk.terminator.kind = Compiler_IR_Terminator_Kind::Exit;
    }

    static void set_unreachable(const Compiler_IR_Function &function,
                         const Compiler_IR_Block_Id id,
                         const Source_Span &span = {})
    {
      auto &blk = block(function, id);
      ah_runtime_error_unless(not blk.is_terminated())
        << "Compiler_IR_Lowering: block already terminated";
      blk.terminator.kind = Compiler_IR_Terminator_Kind::Unreachable;
      blk.terminator.span = span;
      merge_span(blk.span, span);
    }

    Compiler_IR_Block_Id ensure_block(Compiler_IR_Function &function, Compiler_IR_Block_Id current)
    {
      if (current != compiler_ir_invalid_id())
        return current;
      return create_block(function, "dead." + std::to_string(dead_counter++));
    }

    static void enter_scope(DynArray<Compiler_IR_Detail::Scope_Frame> &scopes)
    {
      scopes.append({});
    }

    static void leave_scope(DynArray<Compiler_IR_Detail::Scope_Frame> &scopes)
    {
      ah_runtime_error_unless(not scopes.is_empty())
        << "Compiler_IR_Lowering: attempted to leave empty scope stack";
      (void) scopes.pop();
    }

    static void bind(const DynArray<Compiler_IR_Detail::Scope_Frame> &scopes,
                     const std::string &name,
                     const Compiler_IR_Detail::Binding &binding)
    {
      ah_runtime_error_unless(not scopes.is_empty())
        << "Compiler_IR_Lowering: no active scope frame";
      scopes.access(scopes.size() - 1).bindings.append({name, binding});
    }

    static Compiler_IR_Detail::Binding lookup(const DynArray<Compiler_IR_Detail::Scope_Frame> &scopes,
                                              const std::string &name)
    {
      for (size_t i = scopes.size(); i > 0; --i)
        {
          const auto &[bindings] = scopes.access(i - 1);
          for (size_t j = bindings.size(); j > 0; --j)
            if (bindings.access(j - 1).first == name)
              return bindings.access(j - 1).second;
        }

      return {};
    }

    Compiler_IR_Value_Id emit_unit_constant(Compiler_IR_Function &function,
                                            const Compiler_IR_Block_Id block_id,
                                            const Source_Span &span) const
    {
      Compiler_IR_Instruction inst;
      inst.kind = Compiler_IR_Instruction_Kind::Constant;
      inst.result_id = next_value(function);
      inst.type_id = types != nullptr ? types->unit_type() : 0;
      inst.span = span;
      inst.text = "unit";
      emit_instruction(function, block_id, std::move(inst));
      return function.next_value_id;
    }

    Compiler_IR_Value_Id lower_expr(Compiler_IR_Function &function,
                                    const Compiler_HIR_Expr *expr,
                                    const Compiler_IR_Block_Id block_id,
                                    const DynArray<Compiler_IR_Detail::Scope_Frame> &scopes)
    {
      ah_runtime_error_unless(expr != nullptr) << "Compiler_IR_Lowering: null HIR expression";

      switch (expr->kind)
        {
        case Compiler_HIR_Expr_Kind::Invalid:
          return emit_unit_constant(function, block_id, expr->span);

        case Compiler_HIR_Expr_Kind::Constant:
          {
            const auto *node = static_cast<const Compiler_HIR_Constant_Expr *>(expr);
            Compiler_IR_Instruction inst;
            inst.kind = Compiler_IR_Instruction_Kind::Constant;
            inst.result_id = next_value(function);
            inst.type_id = node->type_id;
            inst.span = node->span;
            inst.text = node->text;
            inst.bool_value = node->bool_value;
            emit_instruction(function, block_id, std::move(inst));
            return function.next_value_id;
          }

        case Compiler_HIR_Expr_Kind::Variable:
          {
            const auto *node = static_cast<const Compiler_HIR_Variable_Expr *>(expr);
            const auto binding = lookup(scopes, node->name);
            Compiler_IR_Instruction inst;
            inst.type_id = node->type_id;
            inst.span = node->span;

            switch (binding.kind)
              {
              case Compiler_IR_Detail::Binding::Kind::Local_Slot:
                inst.kind = Compiler_IR_Instruction_Kind::Load;
                inst.result_id = next_value(function);
                inst.local_slot_id = binding.id;
                emit_instruction(function, block_id, std::move(inst));
                return function.next_value_id;

              case Compiler_IR_Detail::Binding::Kind::Global_Slot:
                inst.kind = Compiler_IR_Instruction_Kind::Load;
                inst.result_id = next_value(function);
                inst.global_slot_id = binding.id;
                emit_instruction(function, block_id, std::move(inst));
                return function.next_value_id;

              case Compiler_IR_Detail::Binding::Kind::Function:
                inst.kind = Compiler_IR_Instruction_Kind::Function_Ref;
                inst.result_id = next_value(function);
                inst.function_id = binding.id;
                emit_instruction(function, block_id, std::move(inst));
                return function.next_value_id;
              }

            return emit_unit_constant(function, block_id, node->span);
          }

        case Compiler_HIR_Expr_Kind::Unary:
          {
            const auto *node = static_cast<const Compiler_HIR_Unary_Expr *>(expr);
            const auto operand = lower_expr(function, node->operand, block_id, scopes);
            Compiler_IR_Instruction inst;
            inst.kind = Compiler_IR_Instruction_Kind::Unary;
            inst.result_id = next_value(function);
            inst.type_id = node->type_id;
            inst.span = node->span;
            inst.op = node->op;
            inst.operands.append(operand);
            emit_instruction(function, block_id, std::move(inst));
            return function.next_value_id;
          }

        case Compiler_HIR_Expr_Kind::Binary:
          {
            const auto *node = static_cast<const Compiler_HIR_Binary_Expr *>(expr);
            const bool is_compound_assignment
              = node->op == Compiler_Token_Kind::PlusEq or node->op == Compiler_Token_Kind::MinusEq
             or node->op == Compiler_Token_Kind::StarEq or node->op == Compiler_Token_Kind::SlashEq
             or node->op == Compiler_Token_Kind::PercentEq;

            if (node->op == Compiler_Token_Kind::Assign or is_compound_assignment)
              {
                ah_runtime_error_unless(node->left != nullptr
                                        and node->left->kind == Compiler_HIR_Expr_Kind::Variable)
                  << "Compiler_IR_Lowering: assignment expects variable lhs";

                const auto *lhs = static_cast<const Compiler_HIR_Variable_Expr *>(node->left);
                const auto binding = lookup(scopes, lhs->name);

                Compiler_IR_Value_Id stored_value = 0;
                if (node->op == Compiler_Token_Kind::Assign)
                  stored_value = lower_expr(function, node->right, block_id, scopes);
                else
                  {
                    Compiler_IR_Instruction load;
                    load.kind = Compiler_IR_Instruction_Kind::Load;
                    load.result_id = next_value(function);
                    load.type_id = lhs->type_id;
                    load.span = lhs->span;
                    if (binding.kind == Compiler_IR_Detail::Binding::Kind::Global_Slot)
                      load.global_slot_id = binding.id;
                    else
                      load.local_slot_id = binding.id;
                    emit_instruction(function, block_id, std::move(load));
                    const auto lhs_value = function.next_value_id;
                    const auto rhs_value = lower_expr(function, node->right, block_id, scopes);

                    Compiler_IR_Instruction binary;
                    binary.kind = Compiler_IR_Instruction_Kind::Binary;
                    binary.result_id = next_value(function);
                    binary.type_id = node->type_id;
                    binary.span = node->span;
                    binary.op = node->op == Compiler_Token_Kind::PlusEq ? Compiler_Token_Kind::Plus
                              : node->op == Compiler_Token_Kind::MinusEq ? Compiler_Token_Kind::Minus
                              : node->op == Compiler_Token_Kind::StarEq ? Compiler_Token_Kind::Star
                              : node->op == Compiler_Token_Kind::SlashEq
                                ? Compiler_Token_Kind::Slash
                                : Compiler_Token_Kind::Percent;
                    binary.operands.append(lhs_value);
                    binary.operands.append(rhs_value);
                    emit_instruction(function, block_id, std::move(binary));
                    stored_value = function.next_value_id;
                  }

                Compiler_IR_Instruction store;
                store.kind = Compiler_IR_Instruction_Kind::Store;
                store.type_id = node->type_id;
                store.span = node->span;
                store.operands.append(stored_value);
                if (binding.kind == Compiler_IR_Detail::Binding::Kind::Global_Slot)
                  store.global_slot_id = binding.id;
                else
                  store.local_slot_id = binding.id;
                emit_instruction(function, block_id, std::move(store));
                return stored_value;
              }

            const auto lhs = lower_expr(function, node->left, block_id, scopes);
            const auto rhs = lower_expr(function, node->right, block_id, scopes);

            Compiler_IR_Instruction inst;
            inst.kind = Compiler_IR_Instruction_Kind::Binary;
            inst.result_id = next_value(function);
            inst.type_id = node->type_id;
            inst.span = node->span;
            inst.op = node->op;
            inst.operands.append(lhs);
            inst.operands.append(rhs);
            emit_instruction(function, block_id, std::move(inst));
            return function.next_value_id;
          }

        case Compiler_HIR_Expr_Kind::Call:
          {
            const auto *node = static_cast<const Compiler_HIR_Call_Expr *>(expr);
            const auto callee = lower_expr(function, node->callee, block_id, scopes);
            DynArray<Compiler_IR_Value_Id> arguments;
            for (size_t i = 0; i < node->arguments.size(); ++i)
              arguments.append(lower_expr(function, node->arguments.access(i), block_id, scopes));

            Compiler_IR_Instruction inst;
            inst.kind = Compiler_IR_Instruction_Kind::Call;
            inst.result_id = next_value(function);
            inst.type_id = node->type_id;
            inst.span = node->span;
            inst.operands.append(callee);
            for (size_t i = 0; i < arguments.size(); ++i)
              inst.operands.append(arguments.access(i));
            emit_instruction(function, block_id, std::move(inst));
            return function.next_value_id;
          }
        }

      return emit_unit_constant(function, block_id, expr->span);
    }

    Compiler_IR_Block_Id lower_stmt(Compiler_IR_Function &function,
                                    const Compiler_HIR_Stmt *stmt,
                                    Compiler_IR_Block_Id current,
                                    DynArray<Compiler_IR_Detail::Scope_Frame> &scopes,
                                    DynArray<std::pair<Compiler_IR_Block_Id, Compiler_IR_Block_Id>> &loops)
    {
      if (stmt == nullptr)
        return current;

      switch (stmt->kind)
        {
        case Compiler_HIR_Stmt_Kind::Invalid:
          current = ensure_block(function, current);
          (void) emit_unit_constant(function, current, stmt->span);
          return current;

        case Compiler_HIR_Stmt_Kind::Eval:
          {
            current = ensure_block(function, current);
            const auto *node = static_cast<const Compiler_HIR_Eval_Stmt *>(stmt);
            (void) lower_expr(function, node->expr, current, scopes);
            return current;
          }

        case Compiler_HIR_Stmt_Kind::Let:
          {
            current = ensure_block(function, current);
            const auto *node = static_cast<const Compiler_HIR_Let_Stmt *>(stmt);

            Compiler_IR_Detail::Binding binding;
            if (const bool defines_global = function.is_top_level() and scopes.size() == 2)
              {
                binding.kind = Compiler_IR_Detail::Binding::Kind::Global_Slot;
                binding.id = find_global_slot_id(node->name);
                binding.type_id = node->binding_type;
              }
            else
              {
                binding.kind = Compiler_IR_Detail::Binding::Kind::Local_Slot;
                binding.id = add_local_slot(function,
                                            Compiler_IR_Slot_Kind::Local,
                                            node->name,
                                            node->binding_type,
                                            node->name_span);
                binding.type_id = node->binding_type;
              }

            const auto value = node->initializer != nullptr
                               ? lower_expr(function, node->initializer, current, scopes)
                               : emit_unit_constant(function, current, node->span);

            Compiler_IR_Instruction store;
            store.kind = Compiler_IR_Instruction_Kind::Store;
            store.type_id = node->binding_type;
            store.span = node->span;
            store.operands.append(value);
            if (binding.kind == Compiler_IR_Detail::Binding::Kind::Global_Slot)
              store.global_slot_id = binding.id;
            else
              store.local_slot_id = binding.id;
            emit_instruction(function, current, std::move(store));

            bind(scopes, node->name, binding);
            return current;
          }

        case Compiler_HIR_Stmt_Kind::Return:
          {
            current = ensure_block(function, current);
            const auto *node = static_cast<const Compiler_HIR_Return_Stmt *>(stmt);
            const auto value = node->value != nullptr
                               ? lower_expr(function, node->value, current, scopes)
                               : emit_unit_constant(function, current, node->span);
            set_return(function, current, value, node->span);
            return compiler_ir_invalid_id();
          }

        case Compiler_HIR_Stmt_Kind::Block:
          {
            const auto *node = static_cast<const Compiler_HIR_Block_Stmt *>(stmt);
            enter_scope(scopes);
            for (size_t i = 0; i < node->statements.size(); ++i)
              current = lower_stmt(function, node->statements.access(i), current, scopes, loops);
            leave_scope(scopes);
            return current;
          }

        case Compiler_HIR_Stmt_Kind::If:
          {
            current = ensure_block(function, current);
            const auto *node = static_cast<const Compiler_HIR_If_Stmt *>(stmt);

            const auto condition = lower_expr(function, node->condition, current, scopes);
            const auto id = if_counter++;
            const auto then_block = create_block(function, "if.then." + std::to_string(id));
            const auto join_block = create_block(function, "if.end." + std::to_string(id));
            Compiler_IR_Block_Id else_block = join_block;
            if (node->else_branch != nullptr)
              else_block = create_block(function, "if.else." + std::to_string(id));

            set_branch(function,
                       current,
                       condition,
                       node->condition != nullptr ? node->condition->span : node->span,
                       then_block,
                       else_block);

            auto then_end = lower_stmt(function, node->then_branch, then_block, scopes, loops);
            if (then_end != compiler_ir_invalid_id())
              set_jump(function, then_end, join_block);

            if (node->else_branch != nullptr)
              {
                auto else_end = lower_stmt(function, node->else_branch, else_block, scopes, loops);
                if (else_end != compiler_ir_invalid_id())
                  set_jump(function, else_end, join_block);
              }

            return join_block;
          }

        case Compiler_HIR_Stmt_Kind::While:
          {
            current = ensure_block(function, current);
            const auto *node = static_cast<const Compiler_HIR_While_Stmt *>(stmt);
            const auto id = while_counter++;
            const auto cond_block = create_block(function, "while.cond." + std::to_string(id));
            const auto body_block = create_block(function, "while.body." + std::to_string(id));
            const auto end_block = create_block(function, "while.end." + std::to_string(id));

            set_jump(function, current, cond_block, node->keyword_span);
            const auto condition = lower_expr(function, node->condition, cond_block, scopes);
            set_branch(function,
                       cond_block,
                       condition,
                       node->condition != nullptr ? node->condition->span : node->span,
                       body_block,
                       end_block);

            loops.append({end_block, cond_block});
            auto body_end = lower_stmt(function, node->body, body_block, scopes, loops);
            (void) loops.pop();

            if (body_end != compiler_ir_invalid_id())
              set_jump(function, body_end, cond_block);
            return end_block;
          }

        case Compiler_HIR_Stmt_Kind::Break:
          {
            current = ensure_block(function, current);
            if (loops.is_empty())
              {
                set_unreachable(function, current, stmt->span);
                return compiler_ir_invalid_id();
              }
            set_jump(function, current, loops.access(loops.size() - 1).first, stmt->span);
            return compiler_ir_invalid_id();
          }

        case Compiler_HIR_Stmt_Kind::Continue:
          {
            current = ensure_block(function, current);
            if (loops.is_empty())
              {
                set_unreachable(function, current, stmt->span);
                return compiler_ir_invalid_id();
              }
            set_jump(function, current, loops.access(loops.size() - 1).second, stmt->span);
            return compiler_ir_invalid_id();
          }
        }

      return current;
    }

    void preload_global_bindings(DynArray<Compiler_IR_Detail::Scope_Frame> &scopes) const
    {
      enter_scope(scopes);
      for (size_t i = 0; i < module->global_slots.size(); ++i)
        bind(scopes,
             module->global_slots.access(i).name,
             {Compiler_IR_Detail::Binding::Kind::Global_Slot,
              i,
              module->global_slots.access(i).type_id});

      for (size_t i = 0; i < function_ids.size(); ++i)
        bind(scopes,
             function_ids.access(i).first,
             {Compiler_IR_Detail::Binding::Kind::Function,
              function_ids.access(i).second,
              function(function_ids.access(i).second).type_id});
    }

    void lower_function_body(Compiler_IR_Function &function,
                             const DynArray<Compiler_HIR_Stmt *> &statements)
    {
      function.entry_block = create_block(function, "entry");
      function.exit_block = create_block(function, "exit");

      DynArray<Compiler_IR_Detail::Scope_Frame> scopes;
      preload_global_bindings(scopes);

      if (function.source_function != nullptr)
        {
          enter_scope(scopes);
          for (size_t i = 0; i < function.source_function->parameters.size(); ++i)
            {
              const auto &param = function.source_function->parameters.access(i);
              const auto slot_id = add_local_slot(
                function, Compiler_IR_Slot_Kind::Parameter, param.name, param.type_id, param.span);
              bind(scopes,
                   param.name,
                   {Compiler_IR_Detail::Binding::Kind::Local_Slot, slot_id, param.type_id});
            }
        }
      else
        enter_scope(scopes);

      DynArray<std::pair<Compiler_IR_Block_Id, Compiler_IR_Block_Id>> loops;
      Compiler_IR_Block_Id current = function.entry_block;
      for (size_t i = 0; i < statements.size(); ++i)
        current = lower_stmt(function, statements.access(i), current, scopes, loops);

      if (current != compiler_ir_invalid_id() and not block(function, current).is_terminated())
        set_jump(function, current, function.exit_block);

      set_exit(function, function.exit_block);
      leave_scope(scopes);
      leave_scope(scopes);
    }
  };

  Compiler_IR_Context *ir = nullptr;
  const Compiler_Type_Context *types = nullptr;

public:
  /** @brief Builds an IR lowerer over `ctx`. */
  explicit Compiler_IR_Lowering(Compiler_IR_Context &ctx,
                                const Compiler_Type_Context *type_ctx = nullptr) noexcept
    : ir(&ctx), types(type_ctx)
  {}

  /** @brief Returns the optional type context used by this lowerer. */
  [[nodiscard]] const Compiler_Type_Context *type_context() const noexcept
  {
    return types;
  }

  /** @brief Lowers one HIR module into explicit-value IR. */
  Compiler_IR_Module *lower_module(const Compiler_HIR_Module *module) const
  {
    ah_runtime_error_unless(module != nullptr)
      << "Compiler_IR_Lowering::lower_module(): null HIR module";

    Module_Builder builder;
    builder.module = ir->make<Compiler_IR_Module>();
    builder.hir = module;
    builder.ctx = ir;
    builder.types = types;

    for (size_t i = 0; i < module->functions.size(); ++i)
      (void) builder.predeclare_function(module->functions.access(i));

    for (size_t i = 0; i < module->statements.size(); ++i)
      if (module->statements.access(i)->kind == Compiler_HIR_Stmt_Kind::Let)
        {
          const auto *stmt = static_cast<const Compiler_HIR_Let_Stmt *>(module->statements.access(i));
          if (builder.find_global_slot_id(stmt->name) == compiler_ir_invalid_id())
            (void) builder.add_global_slot(stmt->name, stmt->binding_type, stmt->name_span);
        }

    for (size_t i = 0; i < builder.module->functions.size(); ++i)
      {
        auto &function = builder.function(i);
        builder.lower_function_body(function, function.source_function->body->statements);
      }

    if (not module->statements.is_empty())
      {
        auto *top = ir->make<Compiler_IR_Function>();
        top->id = compiler_ir_invalid_id();
        top->name = "<top-level>";
        top->span = module->span;
        top->next_value_id = 0;
        builder.module->top_level = top;
        builder.lower_function_body(*top, module->statements);
      }

    return builder.module;
  }
};

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
                out << '(' << compiler_token_kind_name(inst.op) << ')';
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
