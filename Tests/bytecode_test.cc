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

/**
 * @file bytecode_test.cc
 * @brief Tests for Bytecode.H.
 */

#include <gtest/gtest.h>

#include <Bytecode.H>

using namespace Aleph;

namespace
{
  Compiler_IR_Block
  make_exit_block()
  {
    Compiler_IR_Block block;
    block.id = 1;
    block.label = "exit";
    block.terminator.kind = Compiler_IR_Terminator_Kind::Exit;
    return block;
  }

  Compiler_IR_Function
  make_linear_ir()
  {
    Compiler_IR_Function function;
    function.id = 0;
    function.name = "add_one";
    function.entry_block = 0;
    function.exit_block = 1;
    function.next_value_id = 4;

    Compiler_IR_Slot param;
    param.id = 0;
    param.kind = Compiler_IR_Slot_Kind::Parameter;
    param.name = "x";
    function.local_slots.append(param);

    Compiler_IR_Slot local;
    local.id = 1;
    local.kind = Compiler_IR_Slot_Kind::Local;
    local.name = "tmp";
    function.local_slots.append(local);

    Compiler_IR_Block entry;
    entry.id = 0;
    entry.label = "entry";

    Compiler_IR_Instruction constant;
    constant.kind = Compiler_IR_Instruction_Kind::Constant;
    constant.result_id = 1;
    constant.text = "1";
    entry.instructions.append(constant);

    Compiler_IR_Instruction store;
    store.kind = Compiler_IR_Instruction_Kind::Store;
    store.local_slot_id = 1;
    store.operands.append(1);
    entry.instructions.append(store);

    Compiler_IR_Instruction load_x;
    load_x.kind = Compiler_IR_Instruction_Kind::Load;
    load_x.result_id = 2;
    load_x.local_slot_id = 0;
    entry.instructions.append(load_x);

    Compiler_IR_Instruction load_tmp;
    load_tmp.kind = Compiler_IR_Instruction_Kind::Load;
    load_tmp.result_id = 3;
    load_tmp.local_slot_id = 1;
    entry.instructions.append(load_tmp);

    Compiler_IR_Instruction add;
    add.kind = Compiler_IR_Instruction_Kind::Binary;
    add.result_id = 4;
    add.op = Compiler_Token_Kind::Plus;
    add.operands.append(2);
    add.operands.append(3);
    entry.instructions.append(add);

    entry.terminator.kind = Compiler_IR_Terminator_Kind::Return;
    entry.terminator.return_value = 4;
    entry.terminator.successors.append(1);

    auto exit = make_exit_block();
    exit.predecessors.append(0);

    function.blocks.append(entry);
    function.blocks.append(exit);
    return function;
  }

  Compiler_IR_Function
  make_branch_ir()
  {
    Compiler_IR_Function function;
    function.id = 0;
    function.name = "choose";
    function.entry_block = 0;
    function.exit_block = 1;
    function.next_value_id = 5;

    Compiler_IR_Slot flag;
    flag.id = 0;
    flag.kind = Compiler_IR_Slot_Kind::Parameter;
    flag.name = "flag";
    function.local_slots.append(flag);

    Compiler_IR_Slot value;
    value.id = 1;
    value.kind = Compiler_IR_Slot_Kind::Local;
    value.name = "value";
    function.local_slots.append(value);

    Compiler_IR_Block entry;
    entry.id = 0;
    entry.label = "entry";

    Compiler_IR_Instruction zero;
    zero.kind = Compiler_IR_Instruction_Kind::Constant;
    zero.result_id = 1;
    zero.text = "0";
    entry.instructions.append(zero);

    Compiler_IR_Instruction init_store;
    init_store.kind = Compiler_IR_Instruction_Kind::Store;
    init_store.local_slot_id = 1;
    init_store.operands.append(1);
    entry.instructions.append(init_store);

    Compiler_IR_Instruction load_flag;
    load_flag.kind = Compiler_IR_Instruction_Kind::Load;
    load_flag.result_id = 2;
    load_flag.local_slot_id = 0;
    entry.instructions.append(load_flag);

    entry.terminator.kind = Compiler_IR_Terminator_Kind::Branch;
    entry.terminator.condition_value = 2;
    entry.terminator.successors.append(2);
    entry.terminator.successors.append(4);

    auto exit = make_exit_block();
    exit.predecessors.append(3);

    Compiler_IR_Block then_block;
    then_block.id = 2;
    then_block.label = "if.then.0";

    Compiler_IR_Instruction one;
    one.kind = Compiler_IR_Instruction_Kind::Constant;
    one.result_id = 3;
    one.text = "1";
    then_block.instructions.append(one);

    Compiler_IR_Instruction then_store;
    then_store.kind = Compiler_IR_Instruction_Kind::Store;
    then_store.local_slot_id = 1;
    then_store.operands.append(3);
    then_block.instructions.append(then_store);

    then_block.terminator.kind = Compiler_IR_Terminator_Kind::Jump;
    then_block.terminator.successors.append(3);
    then_block.predecessors.append(0);

    Compiler_IR_Block join_block;
    join_block.id = 3;
    join_block.label = "if.end.0";

    Compiler_IR_Instruction load_value;
    load_value.kind = Compiler_IR_Instruction_Kind::Load;
    load_value.result_id = 5;
    load_value.local_slot_id = 1;
    join_block.instructions.append(load_value);

    join_block.terminator.kind = Compiler_IR_Terminator_Kind::Return;
    join_block.terminator.return_value = 5;
    join_block.terminator.successors.append(1);
    join_block.predecessors.append(2);
    join_block.predecessors.append(4);

    Compiler_IR_Block else_block;
    else_block.id = 4;
    else_block.label = "if.else.0";

    Compiler_IR_Instruction two;
    two.kind = Compiler_IR_Instruction_Kind::Constant;
    two.result_id = 4;
    two.text = "2";
    else_block.instructions.append(two);

    Compiler_IR_Instruction else_store;
    else_store.kind = Compiler_IR_Instruction_Kind::Store;
    else_store.local_slot_id = 1;
    else_store.operands.append(4);
    else_block.instructions.append(else_store);

    else_block.terminator.kind = Compiler_IR_Terminator_Kind::Jump;
    else_block.terminator.successors.append(3);
    else_block.predecessors.append(0);

    function.blocks.append(entry);
    function.blocks.append(exit);
    function.blocks.append(then_block);
    function.blocks.append(join_block);
    function.blocks.append(else_block);
    return function;
  }

  Compiler_IR_Module
  make_module_with_top_level_call()
  {
    Compiler_IR_Module module;

    Compiler_IR_Slot global;
    global.id = 0;
    global.kind = Compiler_IR_Slot_Kind::Global;
    global.name = "answer";
    module.global_slots.append(global);

    auto * function = new Compiler_IR_Function(make_linear_ir());
    module.functions.append(function);

    auto * top = new Compiler_IR_Function();
    top->id = compiler_ir_invalid_id();
    top->name = "<top-level>";
    top->entry_block = 0;
    top->exit_block = 1;
    top->next_value_id = 3;

    Compiler_IR_Block entry;
    entry.id = 0;
    entry.label = "entry";

    Compiler_IR_Instruction function_ref;
    function_ref.kind = Compiler_IR_Instruction_Kind::Function_Ref;
    function_ref.result_id = 1;
    function_ref.function_id = 0;
    entry.instructions.append(function_ref);

    Compiler_IR_Instruction constant;
    constant.kind = Compiler_IR_Instruction_Kind::Constant;
    constant.result_id = 2;
    constant.text = "41";
    entry.instructions.append(constant);

    Compiler_IR_Instruction call;
    call.kind = Compiler_IR_Instruction_Kind::Call;
    call.result_id = 3;
    call.operands.append(1);
    call.operands.append(2);
    entry.instructions.append(call);

    Compiler_IR_Instruction store_global;
    store_global.kind = Compiler_IR_Instruction_Kind::Store;
    store_global.global_slot_id = 0;
    store_global.operands.append(3);
    entry.instructions.append(store_global);

    entry.terminator.kind = Compiler_IR_Terminator_Kind::Jump;
    entry.terminator.successors.append(1);

    auto exit = make_exit_block();
    exit.predecessors.append(0);

    top->blocks.append(entry);
    top->blocks.append(exit);
    module.top_level = top;
    return module;
  }
}

TEST(CompilerBytecode, LowersLinearFunctionWithConstantsAndLocals)
{
  Compiler_Bytecode_Context ctx(1 << 16);
  Compiler_Bytecode_Lowering lowering(ctx);

  Compiler_IR_Module module;
  auto source = make_linear_ir();
  module.functions.append(&source);

  const auto * bytecode = lowering.lower_module(&module);
  ASSERT_EQ(bytecode->functions.size(), 1u);
  const auto & function = *bytecode->functions.access(0);

  const auto report = validate_bytecode_function(function, bytecode);
  ASSERT_TRUE(report.valid);
  EXPECT_EQ(function.register_count, 5u);
  ASSERT_EQ(function.constants.size(), 1u);
  EXPECT_EQ(function.constants.access(0).kind,
            Compiler_Bytecode_Constant_Kind::Integer);
  EXPECT_EQ(function.constants.access(0).integer_value, 1);

  ASSERT_GE(function.code.size(), 6u);
  EXPECT_EQ(function.code.access(0).opcode,
            Compiler_Bytecode_Opcode::Load_Constant);
  EXPECT_EQ(function.code.access(1).opcode,
            Compiler_Bytecode_Opcode::Store_Local);
  EXPECT_EQ(function.code.access(2).opcode,
            Compiler_Bytecode_Opcode::Load_Local);
  EXPECT_EQ(function.code.access(4).opcode,
            Compiler_Bytecode_Opcode::Binary);
  EXPECT_EQ(function.code.access(5).opcode,
            Compiler_Bytecode_Opcode::Return);
}

TEST(CompilerBytecode, PatchesBranchTargetsToConcreteProgramCounters)
{
  Compiler_Bytecode_Context ctx(1 << 16);
  Compiler_Bytecode_Lowering lowering(ctx);

  Compiler_IR_Module module;
  auto source = make_branch_ir();
  module.functions.append(&source);

  const auto * bytecode = lowering.lower_module(&module);
  const auto & function = *bytecode->functions.access(0);

  const auto report = validate_bytecode_function(function, bytecode);
  ASSERT_TRUE(report.valid);
  ASSERT_EQ(function.blocks.size(), 5u);

  const auto & entry_branch = function.code.access(3);
  EXPECT_EQ(entry_branch.opcode, Compiler_Bytecode_Opcode::Branch);
  EXPECT_EQ(entry_branch.target_pc, function.blocks.access(2).begin_pc);
  EXPECT_EQ(entry_branch.false_target_pc, function.blocks.access(4).begin_pc);

  const auto & then_jump = function.code.access(function.blocks.access(2).end_pc - 1);
  EXPECT_EQ(then_jump.opcode, Compiler_Bytecode_Opcode::Jump);
  EXPECT_EQ(then_jump.target_pc, function.blocks.access(3).begin_pc);
}

TEST(CompilerBytecode, LowersTopLevelCallAndGlobalStore)
{
  Compiler_Bytecode_Context ctx(1 << 16);
  Compiler_Bytecode_Lowering lowering(ctx);

  auto module = make_module_with_top_level_call();
  const auto * bytecode = lowering.lower_module(&module);

  const auto report = validate_bytecode_module(*bytecode);
  ASSERT_TRUE(report.valid);
  ASSERT_EQ(bytecode->functions.size(), 1u);
  ASSERT_NE(bytecode->top_level, nullptr);
  ASSERT_EQ(bytecode->global_slots.size(), 1u);

  const auto & top = *bytecode->top_level;
  ASSERT_GE(top.code.size(), 5u);
  EXPECT_EQ(top.code.access(0).opcode, Compiler_Bytecode_Opcode::Load_Function);
  EXPECT_EQ(top.code.access(1).opcode, Compiler_Bytecode_Opcode::Load_Constant);
  EXPECT_EQ(top.code.access(2).opcode, Compiler_Bytecode_Opcode::Call);
  EXPECT_EQ(top.code.access(3).opcode, Compiler_Bytecode_Opcode::Store_Global);
  EXPECT_EQ(top.code.access(4).opcode, Compiler_Bytecode_Opcode::Jump);

  const auto dump = compiler_dump_bytecode_module(bytecode);
  EXPECT_NE(dump.find("BytecodeModule"), std::string::npos);
  EXPECT_NE(dump.find("LoadFunction"), std::string::npos);
  EXPECT_NE(dump.find("StoreGlobal"), std::string::npos);

  delete module.functions.access(0);
  delete module.top_level;
}
