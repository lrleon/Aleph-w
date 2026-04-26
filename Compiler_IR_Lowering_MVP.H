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

/** @file Compiler_IR_Lowering_MVP.H
 *  @brief Lowering from the current MVP HIR into the reusable explicit IR model.
 *
 *  This header preserves the existing IR lowering strategy for the reference
 *  frontend, but isolates it from the reusable `Compiler_IR_Model.H` node
 *  definitions so other languages can target IR without depending on the MVP
 *  lowering pipeline.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_IR_LOWERING_MVP_H
#define COMPILER_IR_LOWERING_MVP_H

#include <Compiler_IR_Model.H>

namespace Aleph {
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

    static Compiler_IR_Block &block(const Compiler_IR_Function &function,
                                    const Compiler_IR_Block_Id id)
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
      dst = compiler_hir_merge_spans(dst, src);
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
            if (binding.id == compiler_ir_invalid_id())
              return emit_unit_constant(function, block_id, node->span);
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
            const bool is_compound_assignment = compiler_operator_is_compound_assignment(node->op);

            if (node->op == Compiler_Operator_Kind::Assign or is_compound_assignment)
              {
                ah_runtime_error_unless(node->left != nullptr
                                        and node->left->kind == Compiler_HIR_Expr_Kind::Variable)
                  << "Compiler_IR_Lowering: assignment expects variable lhs";

                const auto *lhs = static_cast<const Compiler_HIR_Variable_Expr *>(node->left);
                const auto binding = lookup(scopes, lhs->name);

                Compiler_IR_Value_Id stored_value = 0;
                if (node->op == Compiler_Operator_Kind::Assign)
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
                    binary.op = compiler_operator_compound_base(node->op);
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
            if (function.is_top_level() and scopes.size() == 2)
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
        ah_runtime_error_unless(function.source_function->body != nullptr)
          << "Compiler_IR_Lowering: function '" << function.source_function->name
          << "' has a null body";
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
}  // namespace Aleph

#endif
