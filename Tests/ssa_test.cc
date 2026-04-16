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
 * @file ssa_test.cc
 * @brief Tests for SSA.H.
 */

#include <gtest/gtest.h>

#include <SSA.H>

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
  make_diamond_ir()
  {
    Compiler_IR_Function function;
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

  Compiler_IR_Function
  make_loop_ir()
  {
    Compiler_IR_Function function;
    function.name = "loop";
    function.entry_block = 0;
    function.exit_block = 1;
    function.next_value_id = 6;

    Compiler_IR_Slot flag;
    flag.id = 0;
    flag.kind = Compiler_IR_Slot_Kind::Parameter;
    flag.name = "flag";
    function.local_slots.append(flag);

    Compiler_IR_Slot i_slot;
    i_slot.id = 1;
    i_slot.kind = Compiler_IR_Slot_Kind::Local;
    i_slot.name = "i";
    function.local_slots.append(i_slot);

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

    entry.terminator.kind = Compiler_IR_Terminator_Kind::Jump;
    entry.terminator.successors.append(2);

    auto exit = make_exit_block();
    exit.predecessors.append(4);

    Compiler_IR_Block header;
    header.id = 2;
    header.label = "while.cond.0";

    Compiler_IR_Instruction load_flag;
    load_flag.kind = Compiler_IR_Instruction_Kind::Load;
    load_flag.result_id = 2;
    load_flag.local_slot_id = 0;
    header.instructions.append(load_flag);

    header.terminator.kind = Compiler_IR_Terminator_Kind::Branch;
    header.terminator.condition_value = 2;
    header.terminator.successors.append(3);
    header.terminator.successors.append(4);
    header.predecessors.append(0);
    header.predecessors.append(3);

    Compiler_IR_Block body;
    body.id = 3;
    body.label = "while.body.0";

    Compiler_IR_Instruction load_i;
    load_i.kind = Compiler_IR_Instruction_Kind::Load;
    load_i.result_id = 3;
    load_i.local_slot_id = 1;
    body.instructions.append(load_i);

    Compiler_IR_Instruction one;
    one.kind = Compiler_IR_Instruction_Kind::Constant;
    one.result_id = 4;
    one.text = "1";
    body.instructions.append(one);

    Compiler_IR_Instruction add;
    add.kind = Compiler_IR_Instruction_Kind::Binary;
    add.result_id = 5;
    add.op = Compiler_Token_Kind::Plus;
    add.operands.append(3);
    add.operands.append(4);
    body.instructions.append(add);

    Compiler_IR_Instruction store_i;
    store_i.kind = Compiler_IR_Instruction_Kind::Store;
    store_i.local_slot_id = 1;
    store_i.operands.append(5);
    body.instructions.append(store_i);

    body.terminator.kind = Compiler_IR_Terminator_Kind::Jump;
    body.terminator.successors.append(2);
    body.predecessors.append(2);

    Compiler_IR_Block end_block;
    end_block.id = 4;
    end_block.label = "while.end.0";

    Compiler_IR_Instruction load_final;
    load_final.kind = Compiler_IR_Instruction_Kind::Load;
    load_final.result_id = 6;
    load_final.local_slot_id = 1;
    end_block.instructions.append(load_final);

    end_block.terminator.kind = Compiler_IR_Terminator_Kind::Return;
    end_block.terminator.return_value = 6;
    end_block.terminator.successors.append(1);
    end_block.predecessors.append(2);

    function.blocks.append(entry);
    function.blocks.append(exit);
    function.blocks.append(header);
    function.blocks.append(body);
    function.blocks.append(end_block);
    return function;
  }
}

TEST(CompilerSSA, ComputesDominatorsAndFrontiersForDiamond)
{
  Compiler_SSA_Context ctx(1 << 16);
  Compiler_SSA_Lowering lowering(ctx);

  Compiler_IR_Module module;
  auto source = make_diamond_ir();
  module.functions.append(&source);

  const auto * ssa = lowering.lower_module(&module);
  ASSERT_EQ(ssa->functions.size(), 1u);
  const auto & function = *ssa->functions.access(0);

  const auto report = validate_ssa_function(function, ssa);
  ASSERT_TRUE(report.valid);
  ASSERT_EQ(function.blocks.size(), 5u);
  EXPECT_EQ(function.dominance.immediate_dominators.access(0), compiler_ssa_invalid_id());
  EXPECT_EQ(function.dominance.immediate_dominators.access(2), 0u);
  EXPECT_EQ(function.dominance.immediate_dominators.access(4), 0u);
  EXPECT_EQ(function.dominance.immediate_dominators.access(3), 0u);
  EXPECT_EQ(function.dominance.immediate_dominators.access(1), 3u);
  ASSERT_EQ(function.dominance.dominance_frontiers.access(2).size(), 1u);
  EXPECT_EQ(function.dominance.dominance_frontiers.access(2).access(0), 3u);
  ASSERT_EQ(function.dominance.dominance_frontiers.access(4).size(), 1u);
  EXPECT_EQ(function.dominance.dominance_frontiers.access(4).access(0), 3u);
}

TEST(CompilerSSA, InsertsPhiAndRemovesLocalLoadsStoresInDiamond)
{
  Compiler_SSA_Context ctx(1 << 16);
  Compiler_SSA_Lowering lowering(ctx);

  Compiler_IR_Module module;
  auto source = make_diamond_ir();
  module.functions.append(&source);

  const auto * ssa = lowering.lower_module(&module);
  const auto & function = *ssa->functions.access(0);

  const auto report = validate_ssa_function(function, ssa);
  ASSERT_TRUE(report.valid);

  ASSERT_EQ(function.parameters.size(), 1u);
  EXPECT_EQ(function.parameters.access(0).slot_id, 0u);

  const auto & entry = function.blocks.access(0);
  ASSERT_EQ(entry.instructions.size(), 1u);
  EXPECT_EQ(entry.instructions.access(0).kind,
            Compiler_SSA_Instruction_Kind::Constant);
  EXPECT_EQ(entry.terminator.kind, Compiler_IR_Terminator_Kind::Branch);
  EXPECT_EQ(entry.terminator.condition_value, function.parameters.access(0).value_id);

  const auto & join = function.blocks.access(3);
  ASSERT_EQ(join.phis.size(), 1u);
  EXPECT_EQ(join.phis.access(0).slot_id, 1u);
  ASSERT_EQ(join.phis.access(0).operands.size(), 2u);
  EXPECT_EQ(join.instructions.size(), 0u);
  EXPECT_EQ(join.terminator.kind, Compiler_IR_Terminator_Kind::Return);
  EXPECT_EQ(join.terminator.return_value, join.phis.access(0).result_id);

  const auto dump = compiler_dump_ssa_function(&function, ssa);
  EXPECT_NE(dump.find("Phi s1"), std::string::npos);
}

TEST(CompilerSSA, RenamesLoopBackedgeThroughHeaderPhi)
{
  Compiler_SSA_Context ctx(1 << 16);
  Compiler_SSA_Lowering lowering(ctx);

  Compiler_IR_Module module;
  auto source = make_loop_ir();
  module.functions.append(&source);

  const auto * ssa = lowering.lower_module(&module);
  const auto & function = *ssa->functions.access(0);

  const auto report = validate_ssa_function(function, ssa);
  ASSERT_TRUE(report.valid);
  ASSERT_EQ(function.blocks.size(), 5u);

  const auto & header = function.blocks.access(2);
  ASSERT_EQ(header.phis.size(), 1u);
  EXPECT_EQ(header.phis.access(0).slot_id, 1u);
  ASSERT_EQ(header.phis.access(0).operands.size(), 2u);
  EXPECT_NE(header.phis.access(0).operands.access(0), 0u);
  EXPECT_NE(header.phis.access(0).operands.access(1), 0u);
  EXPECT_EQ(header.terminator.kind, Compiler_IR_Terminator_Kind::Branch);
  EXPECT_EQ(header.terminator.condition_value, function.parameters.access(0).value_id);

  const auto & body = function.blocks.access(3);
  ASSERT_EQ(body.instructions.size(), 2u);
  EXPECT_EQ(body.instructions.access(0).kind,
            Compiler_SSA_Instruction_Kind::Constant);
  EXPECT_EQ(body.instructions.access(1).kind,
            Compiler_SSA_Instruction_Kind::Binary);

  const auto & end = function.blocks.access(4);
  EXPECT_EQ(end.instructions.size(), 0u);
  EXPECT_EQ(end.terminator.kind, Compiler_IR_Terminator_Kind::Return);
  EXPECT_EQ(end.terminator.return_value, header.phis.access(0).result_id);
}
