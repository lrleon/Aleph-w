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

/** @file Compiler_IR_Builder.H
 *  @brief Reusable builder helpers for constructing explicit IR directly.
 *
 *  Frontends or tests that already operate on their own typed representation
 *  can use this builder to create `Compiler_IR_Model.H` modules without going
 *  through the MVP HIR lowering.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_IR_BUILDER_H
#define COMPILER_IR_BUILDER_H

#include <Compiler_IR_Model.H>

namespace Aleph {
/** @brief Small reusable helper that allocates and connects IR nodes.
 *
 *  The builder keeps block successors, predecessor lists, and aggregate spans
 *  in sync while reusing an external `Compiler_IR_Context` for ownership.
 */
class Compiler_IR_Builder
{
  Compiler_IR_Context *ir_ = nullptr;

  static void merge_span(Source_Span &dst, const Source_Span &src) noexcept
  {
    if (not src.is_valid())
      return;
    if (not dst.is_valid())
      {
        dst = src;
        return;
      }
    dst = compiler_hir_merge_spans(dst, src);
  }

  static Compiler_IR_Block &block(const Compiler_IR_Function &function, const Compiler_IR_Block_Id id)
  {
    ah_runtime_error_unless(id < function.blocks.size())
      << "Compiler_IR_Builder: invalid block id " << id;
    return function.blocks.access(id);
  }

  static void merge_function_span(Compiler_IR_Function &function, const Source_Span &span) noexcept
  {
    merge_span(function.span, span);
  }

  static void add_edge(Compiler_IR_Function &function,
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

  static Compiler_IR_Instruction &append_instruction(Compiler_IR_Function &function,
                                                     const Compiler_IR_Block_Id block_id,
                                                     Compiler_IR_Instruction instruction)
  {
    auto &blk = block(function, block_id);
    merge_span(blk.span, instruction.span);
    merge_function_span(function, instruction.span);
    blk.instructions.append(std::move(instruction));
    return blk.instructions.access(blk.instructions.size() - 1);
  }

  static void set_terminator_span(Compiler_IR_Function &function,
                                  Compiler_IR_Block &blk,
                                  const Source_Span &span) noexcept
  {
    blk.terminator.span = span;
    merge_span(blk.span, span);
    merge_function_span(function, span);
  }

public:
  /** @brief Builds IR nodes inside `context`. */
  explicit Compiler_IR_Builder(Compiler_IR_Context &context) noexcept : ir_(&context) {}

  /** @brief Returns the underlying ownership context. */
  [[nodiscard]] Compiler_IR_Context &context() const noexcept
  {
    return *ir_;
  }

  /** @brief Allocates one empty IR module. */
  Compiler_IR_Module *make_module() const
  {
    return ir_->make<Compiler_IR_Module>();
  }

  /** @brief Appends one global slot to `module`. */
  static Compiler_IR_Global_Slot_Id add_global_slot(Compiler_IR_Module &module,
                                                    std::string name,
                                                    const Compiler_Type_Id type_id,
                                                    const Source_Span &span = {})
  {
    Compiler_IR_Slot slot;
    slot.id = module.global_slots.size();
    slot.kind = Compiler_IR_Slot_Kind::Global;
    slot.name = std::move(name);
    slot.type_id = type_id;
    slot.span = span;
    module.global_slots.append(std::move(slot));
    return module.global_slots.size() - 1;
  }

  /** @brief Allocates one function without inserting it into a module. */
  Compiler_IR_Function *make_function(std::string name,
                                      const Compiler_Type_Id type_id = 0,
                                      const Source_Span &span = {},
                                      const Compiler_HIR_Function *source_function = nullptr) const
  {
    auto *function = ir_->make<Compiler_IR_Function>();
    function->name = std::move(name);
    function->type_id = type_id;
    function->span = span;
    function->source_function = source_function;
    function->next_value_id = 0;
    return function;
  }

  /** @brief Appends one non-top-level function to `module` and assigns its id. */
  static Compiler_IR_Function_Id append_function(Compiler_IR_Module &module,
                                                 Compiler_IR_Function *function)
  {
    ah_runtime_error_unless(function != nullptr)
      << "Compiler_IR_Builder::append_function(): null function";
    function->id = module.functions.size();
    module.functions.append(function);
    return function->id;
  }

  /** @brief Installs one top-level function body in `module`. */
  static Compiler_IR_Function *set_top_level(Compiler_IR_Module &module,
                                             Compiler_IR_Function *function)
  {
    ah_runtime_error_unless(function != nullptr)
      << "Compiler_IR_Builder::set_top_level(): null function";
    function->id = compiler_ir_invalid_id();
    module.top_level = function;
    return function;
  }

  /** @brief Appends one local or parameter slot to `function`. */
  static Compiler_IR_Local_Slot_Id add_local_slot(Compiler_IR_Function &function,
                                                  const Compiler_IR_Slot_Kind kind,
                                                  std::string name,
                                                  const Compiler_Type_Id type_id,
                                                  const Source_Span &span = {})
  {
    Compiler_IR_Slot slot;
    slot.id = function.local_slots.size();
    slot.kind = kind;
    slot.name = std::move(name);
    slot.type_id = type_id;
    slot.span = span;
    function.local_slots.append(std::move(slot));
    merge_function_span(function, span);
    return function.local_slots.size() - 1;
  }

  /** @brief Creates one empty block and returns its id. */
  static Compiler_IR_Block_Id create_block(Compiler_IR_Function &function, std::string label)
  {
    Compiler_IR_Block blk;
    blk.id = function.blocks.size();
    blk.label = std::move(label);
    function.blocks.append(std::move(blk));
    return function.blocks.size() - 1;
  }

  /** @brief Marks one block as the canonical entry block. */
  static void set_entry_block(Compiler_IR_Function &function, const Compiler_IR_Block_Id block_id)
  {
    (void) block(function, block_id);
    function.entry_block = block_id;
  }

  /** @brief Marks one block as the canonical exit block. */
  static void set_exit_block(Compiler_IR_Function &function, const Compiler_IR_Block_Id block_id)
  {
    (void) block(function, block_id);
    function.exit_block = block_id;
  }

  /** @brief Allocates the next explicit IR value id in `function`. */
  static Compiler_IR_Value_Id allocate_value(Compiler_IR_Function &function) noexcept
  {
    ++function.next_value_id;
    return function.next_value_id;
  }

  /** @brief Emits one constant instruction. */
  static Compiler_IR_Value_Id emit_constant(Compiler_IR_Function &function,
                                            const Compiler_IR_Block_Id block_id,
                                            const Compiler_Type_Id type_id,
                                            std::string text,
                                            const Source_Span &span = {},
                                            const bool bool_value = false)
  {
    Compiler_IR_Instruction inst;
    inst.kind = Compiler_IR_Instruction_Kind::Constant;
    inst.result_id = allocate_value(function);
    inst.type_id = type_id;
    inst.span = span;
    inst.text = std::move(text);
    inst.bool_value = bool_value;
    (void) append_instruction(function, block_id, std::move(inst));
    return function.next_value_id;
  }

  /** @brief Emits one load from a local or parameter slot. */
  static Compiler_IR_Value_Id emit_load_local(Compiler_IR_Function &function,
                                              const Compiler_IR_Block_Id block_id,
                                              const Compiler_IR_Local_Slot_Id slot_id,
                                              const Compiler_Type_Id type_id,
                                              const Source_Span &span = {})
  {
    ah_runtime_error_unless(slot_id < function.local_slots.size())
      << "Compiler_IR_Builder::emit_load_local(): invalid slot id " << slot_id;
    Compiler_IR_Instruction inst;
    inst.kind = Compiler_IR_Instruction_Kind::Load;
    inst.result_id = allocate_value(function);
    inst.type_id = type_id;
    inst.span = span;
    inst.local_slot_id = slot_id;
    (void) append_instruction(function, block_id, std::move(inst));
    return function.next_value_id;
  }

  /** @brief Emits one load from a global slot. */
  static Compiler_IR_Value_Id emit_load_global(Compiler_IR_Function &function,
                                               const Compiler_IR_Block_Id block_id,
                                               const Compiler_IR_Global_Slot_Id slot_id,
                                               const Compiler_Type_Id type_id,
                                               const Source_Span &span = {})
  {
    Compiler_IR_Instruction inst;
    inst.kind = Compiler_IR_Instruction_Kind::Load;
    inst.result_id = allocate_value(function);
    inst.type_id = type_id;
    inst.span = span;
    inst.global_slot_id = slot_id;
    (void) append_instruction(function, block_id, std::move(inst));
    return function.next_value_id;
  }

  /** @brief Emits one store into a local or parameter slot. */
  static void emit_store_local(Compiler_IR_Function &function,
                               const Compiler_IR_Block_Id block_id,
                               const Compiler_IR_Local_Slot_Id slot_id,
                               const Compiler_IR_Value_Id value_id,
                               const Compiler_Type_Id type_id,
                               const Source_Span &span = {})
  {
    ah_runtime_error_unless(slot_id < function.local_slots.size())
      << "Compiler_IR_Builder::emit_store_local(): invalid slot id " << slot_id;
    Compiler_IR_Instruction inst;
    inst.kind = Compiler_IR_Instruction_Kind::Store;
    inst.type_id = type_id;
    inst.span = span;
    inst.local_slot_id = slot_id;
    inst.operands.append(value_id);
    (void) append_instruction(function, block_id, std::move(inst));
  }

  /** @brief Emits one store into a global slot. */
  static void emit_store_global(Compiler_IR_Function &function,
                                const Compiler_IR_Block_Id block_id,
                                const Compiler_IR_Global_Slot_Id slot_id,
                                const Compiler_IR_Value_Id value_id,
                                const Compiler_Type_Id type_id,
                                const Source_Span &span = {})
  {
    Compiler_IR_Instruction inst;
    inst.kind = Compiler_IR_Instruction_Kind::Store;
    inst.type_id = type_id;
    inst.span = span;
    inst.global_slot_id = slot_id;
    inst.operands.append(value_id);
    (void) append_instruction(function, block_id, std::move(inst));
  }

  /** @brief Emits one unary instruction. */
  static Compiler_IR_Value_Id emit_unary(Compiler_IR_Function &function,
                                  const Compiler_IR_Block_Id block_id,
                                  const Compiler_Operator_Kind op,
                                  const Compiler_IR_Value_Id operand,
                                  const Compiler_Type_Id type_id,
                                  const Source_Span &span = {})
  {
    Compiler_IR_Instruction inst;
    inst.kind = Compiler_IR_Instruction_Kind::Unary;
    inst.result_id = allocate_value(function);
    inst.type_id = type_id;
    inst.span = span;
    inst.op = op;
    inst.operands.append(operand);
    (void) append_instruction(function, block_id, std::move(inst));
    return function.next_value_id;
  }

  /** @brief Emits one binary instruction. */
  static Compiler_IR_Value_Id emit_binary(Compiler_IR_Function &function,
                                   const Compiler_IR_Block_Id block_id,
                                   const Compiler_Operator_Kind op,
                                   const Compiler_IR_Value_Id lhs,
                                   const Compiler_IR_Value_Id rhs,
                                   const Compiler_Type_Id type_id,
                                   const Source_Span &span = {})
  {
    Compiler_IR_Instruction inst;
    inst.kind = Compiler_IR_Instruction_Kind::Binary;
    inst.result_id = allocate_value(function);
    inst.type_id = type_id;
    inst.span = span;
    inst.op = op;
    inst.operands.append(lhs);
    inst.operands.append(rhs);
    (void) append_instruction(function, block_id, std::move(inst));
    return function.next_value_id;
  }

  /** @brief Emits one function reference instruction. */
  static Compiler_IR_Value_Id emit_function_ref(Compiler_IR_Function &function,
                                         const Compiler_IR_Block_Id block_id,
                                         const Compiler_IR_Function_Id function_id,
                                         const Compiler_Type_Id type_id,
                                         const Source_Span &span = {})
  {
    Compiler_IR_Instruction inst;
    inst.kind = Compiler_IR_Instruction_Kind::Function_Ref;
    inst.result_id = allocate_value(function);
    inst.type_id = type_id;
    inst.span = span;
    inst.function_id = function_id;
    (void) append_instruction(function, block_id, std::move(inst));
    return function.next_value_id;
  }

  /** @brief Emits one call instruction. */
  static Compiler_IR_Value_Id emit_call(Compiler_IR_Function &function,
                                 const Compiler_IR_Block_Id block_id,
                                 const Compiler_IR_Value_Id callee,
                                 const DynArray<Compiler_IR_Value_Id> &arguments,
                                 const Compiler_Type_Id type_id,
                                 const Source_Span &span = {})
  {
    Compiler_IR_Instruction inst;
    inst.kind = Compiler_IR_Instruction_Kind::Call;
    inst.result_id = allocate_value(function);
    inst.type_id = type_id;
    inst.span = span;
    inst.operands.append(callee);
    for (size_t i = 0; i < arguments.size(); ++i)
      inst.operands.append(arguments.access(i));
    (void) append_instruction(function, block_id, std::move(inst));
    return function.next_value_id;
  }

  /** @brief Sets one jump terminator and predecessor/successor edge. */
  static void set_jump(Compiler_IR_Function &function,
                const Compiler_IR_Block_Id from,
                const Compiler_IR_Block_Id to,
                const Source_Span &span = {})
  {
    auto &blk = block(function, from);
    (void) block(function, to);
    ah_runtime_error_unless(not blk.is_terminated())
      << "Compiler_IR_Builder::set_jump(): block already terminated";
    blk.terminator.kind = Compiler_IR_Terminator_Kind::Jump;
    set_terminator_span(function, blk, span);
    add_edge(function, from, to);
  }

  /** @brief Sets one conditional branch terminator and both outgoing edges. */
  static void set_branch(Compiler_IR_Function &function,
                  const Compiler_IR_Block_Id from,
                  const Compiler_IR_Value_Id condition_value,
                  const Compiler_IR_Block_Id then_block,
                  const Compiler_IR_Block_Id else_block,
                  const Source_Span &span = {})
  {
    auto &blk = block(function, from);
    (void) block(function, then_block);
    (void) block(function, else_block);
    ah_runtime_error_unless(not blk.is_terminated())
      << "Compiler_IR_Builder::set_branch(): block already terminated";
    blk.terminator.kind = Compiler_IR_Terminator_Kind::Branch;
    blk.terminator.condition_value = condition_value;
    set_terminator_span(function, blk, span);
    add_edge(function, from, then_block);
    add_edge(function, from, else_block);
  }

  /** @brief Sets one return terminator and connects it to the exit block. */
  static void set_return(Compiler_IR_Function &function,
                  const Compiler_IR_Block_Id from,
                  const Compiler_IR_Value_Id return_value,
                  const Source_Span &span = {})
  {
    ah_runtime_error_unless(function.exit_block < function.blocks.size())
      << "Compiler_IR_Builder::set_return(): exit block not set";
    auto &blk = block(function, from);
    ah_runtime_error_unless(not blk.is_terminated())
      << "Compiler_IR_Builder::set_return(): block already terminated";
    blk.terminator.kind = Compiler_IR_Terminator_Kind::Return;
    blk.terminator.return_value = return_value;
    set_terminator_span(function, blk, span);
    add_edge(function, from, function.exit_block);
  }

  /** @brief Sets one block terminator to `Exit`. */
  static void set_exit(Compiler_IR_Function &function,
                const Compiler_IR_Block_Id block_id,
                const Source_Span &span = {})
  {
    auto &blk = block(function, block_id);
    ah_runtime_error_unless(not blk.is_terminated())
      << "Compiler_IR_Builder::set_exit(): block already terminated";
    function.exit_block = block_id;
    blk.terminator.kind = Compiler_IR_Terminator_Kind::Exit;
    blk.terminator.successors.clear();
    set_terminator_span(function, blk, span);
  }

  /** @brief Marks one block as unreachable. */
  void set_unreachable(Compiler_IR_Function &function,
                       const Compiler_IR_Block_Id block_id,
                       const Source_Span &span = {}) const
  {
    auto &blk = block(function, block_id);
    ah_runtime_error_unless(not blk.is_terminated())
      << "Compiler_IR_Builder::set_unreachable(): block already terminated";
    blk.terminator.kind = Compiler_IR_Terminator_Kind::Unreachable;
    set_terminator_span(function, blk, span);
  }
};
}  // namespace Aleph

#endif
