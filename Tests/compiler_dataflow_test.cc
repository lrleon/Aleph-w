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
 * @file compiler_dataflow_test.cc
 * @brief Tests for Compiler_Dataflow.H.
 */

#include <climits>

#include <gtest/gtest.h>

#include <Compiler_Dataflow.H>

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
  make_manual_uninitialized_ir()
  {
    Compiler_IR_Function function;
    function.name = "manual";
    function.entry_block = 0;
    function.exit_block = 1;
    function.next_value_id = 2;

    Compiler_IR_Slot slot;
    slot.id = 0;
    slot.kind = Compiler_IR_Slot_Kind::Local;
    slot.name = "x";
    function.local_slots.append(slot);

    Compiler_IR_Block entry;
    entry.id = 0;
    entry.label = "entry";

    Compiler_IR_Instruction load;
    load.kind = Compiler_IR_Instruction_Kind::Load;
    load.result_id = 1;
    load.local_slot_id = 0;
    entry.instructions.append(load);

    Compiler_IR_Instruction constant;
    constant.kind = Compiler_IR_Instruction_Kind::Constant;
    constant.result_id = 2;
    constant.text = "1";
    entry.instructions.append(constant);

    Compiler_IR_Instruction store;
    store.kind = Compiler_IR_Instruction_Kind::Store;
    store.local_slot_id = 0;
    store.operands.append(2);
    entry.instructions.append(store);

    entry.terminator.kind = Compiler_IR_Terminator_Kind::Return;
    entry.terminator.return_value = 1;
    entry.terminator.successors.append(1);

    auto exit = make_exit_block();
    exit.predecessors.append(0);

    function.blocks.append(entry);
    function.blocks.append(exit);
    return function;
  }

  Compiler_IR_Function
  make_branching_ir()
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

    Compiler_IR_Instruction c0;
    c0.kind = Compiler_IR_Instruction_Kind::Constant;
    c0.result_id = 1;
    c0.text = "0";
    entry.instructions.append(c0);

    Compiler_IR_Instruction store0;
    store0.kind = Compiler_IR_Instruction_Kind::Store;
    store0.local_slot_id = 1;
    store0.operands.append(1);
    entry.instructions.append(store0);

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

    Compiler_IR_Instruction c1;
    c1.kind = Compiler_IR_Instruction_Kind::Constant;
    c1.result_id = 3;
    c1.text = "1";
    then_block.instructions.append(c1);

    Compiler_IR_Instruction store1;
    store1.kind = Compiler_IR_Instruction_Kind::Store;
    store1.local_slot_id = 1;
    store1.operands.append(3);
    then_block.instructions.append(store1);

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

    Compiler_IR_Instruction c2;
    c2.kind = Compiler_IR_Instruction_Kind::Constant;
    c2.result_id = 4;
    c2.text = "2";
    else_block.instructions.append(c2);

    Compiler_IR_Instruction store2;
    store2.kind = Compiler_IR_Instruction_Kind::Store;
    store2.local_slot_id = 1;
    store2.operands.append(4);
    else_block.instructions.append(store2);

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
  make_constant_branch_ir()
  {
    Compiler_IR_Function function;
    function.name = "simplify";
    function.entry_block = 0;
    function.exit_block = 1;
    function.next_value_id = 5;

    Compiler_IR_Slot slot;
    slot.id = 0;
    slot.kind = Compiler_IR_Slot_Kind::Local;
    slot.name = "x";
    function.local_slots.append(slot);

    Compiler_IR_Block entry;
    entry.id = 0;
    entry.label = "entry";

    Compiler_IR_Instruction c1;
    c1.kind = Compiler_IR_Instruction_Kind::Constant;
    c1.result_id = 1;
    c1.text = "1";
    entry.instructions.append(c1);

    Compiler_IR_Instruction store1;
    store1.kind = Compiler_IR_Instruction_Kind::Store;
    store1.local_slot_id = 0;
    store1.operands.append(1);
    entry.instructions.append(store1);

    Compiler_IR_Instruction cond;
    cond.kind = Compiler_IR_Instruction_Kind::Constant;
    cond.result_id = 2;
    cond.text = "true";
    cond.bool_value = true;
    entry.instructions.append(cond);

    entry.terminator.kind = Compiler_IR_Terminator_Kind::Branch;
    entry.terminator.condition_value = 2;
    entry.terminator.successors.append(2);
    entry.terminator.successors.append(4);

    auto exit = make_exit_block();
    exit.predecessors.append(3);

    Compiler_IR_Block then_block;
    then_block.id = 2;
    then_block.label = "if.then.0";

    Compiler_IR_Instruction c2;
    c2.kind = Compiler_IR_Instruction_Kind::Constant;
    c2.result_id = 3;
    c2.text = "2";
    then_block.instructions.append(c2);

    Compiler_IR_Instruction store2;
    store2.kind = Compiler_IR_Instruction_Kind::Store;
    store2.local_slot_id = 0;
    store2.operands.append(3);
    then_block.instructions.append(store2);

    then_block.terminator.kind = Compiler_IR_Terminator_Kind::Jump;
    then_block.terminator.successors.append(3);
    then_block.predecessors.append(0);

    Compiler_IR_Block join_block;
    join_block.id = 3;
    join_block.label = "if.end.0";

    Compiler_IR_Instruction load_value;
    load_value.kind = Compiler_IR_Instruction_Kind::Load;
    load_value.result_id = 5;
    load_value.local_slot_id = 0;
    join_block.instructions.append(load_value);

    join_block.terminator.kind = Compiler_IR_Terminator_Kind::Return;
    join_block.terminator.return_value = 5;
    join_block.terminator.successors.append(1);
    join_block.predecessors.append(2);
    join_block.predecessors.append(4);

    Compiler_IR_Block else_block;
    else_block.id = 4;
    else_block.label = "if.else.0";

    Compiler_IR_Instruction c3;
    c3.kind = Compiler_IR_Instruction_Kind::Constant;
    c3.result_id = 4;
    c3.text = "3";
    else_block.instructions.append(c3);

    Compiler_IR_Instruction store3;
    store3.kind = Compiler_IR_Instruction_Kind::Store;
    store3.local_slot_id = 0;
    store3.operands.append(4);
    else_block.instructions.append(store3);

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
}

// ---------------------------------------------------------------------------
// Helpers that build minimal IR functions for integer-constant tests.
// ---------------------------------------------------------------------------
namespace
{
  // Build a branch-on-constant function where the branch condition is a
  // known boolean; after DCE the dead arm should be removed and live_out
  // for slot 0 should only reflect the taken successor.
  //
  // entry: store 0 -> x; cond = <flag>; branch cond -> then[2] / else[4]
  // then[2]: store 99 -> x; jump join[3]
  // else[4]: store -1 -> x; jump join[3]    <-- dead when flag==true
  // join[3]: load x; return x
  // exit[1]
  Compiler_IR_Function
  make_branch_fold_liveout_ir(bool flag_value)
  {
    Compiler_IR_Function function;
    function.name = "branch_fold";
    function.entry_block = 0;
    function.exit_block = 1;
    function.next_value_id = 7;

    Compiler_IR_Slot slot;
    slot.id = 0;
    slot.kind = Compiler_IR_Slot_Kind::Local;
    slot.name = "x";
    function.local_slots.append(slot);

    Compiler_IR_Block entry;
    entry.id = 0;
    entry.label = "entry";

    Compiler_IR_Instruction c0;
    c0.kind = Compiler_IR_Instruction_Kind::Constant;
    c0.result_id = 1;
    c0.text = "0";
    entry.instructions.append(c0);

    Compiler_IR_Instruction s0;
    s0.kind = Compiler_IR_Instruction_Kind::Store;
    s0.local_slot_id = 0;
    s0.operands.append(1);
    entry.instructions.append(s0);

    Compiler_IR_Instruction cond;
    cond.kind = Compiler_IR_Instruction_Kind::Constant;
    cond.result_id = 2;
    cond.text = flag_value ? "true" : "false";
    cond.bool_value = flag_value;
    entry.instructions.append(cond);

    entry.terminator.kind = Compiler_IR_Terminator_Kind::Branch;
    entry.terminator.condition_value = 2;
    entry.terminator.successors.append(2); // then
    entry.terminator.successors.append(4); // else

    Compiler_IR_Block exit_blk;
    exit_blk.id = 1;
    exit_blk.label = "exit";
    exit_blk.terminator.kind = Compiler_IR_Terminator_Kind::Exit;
    exit_blk.predecessors.append(3);

    Compiler_IR_Block then_block;
    then_block.id = 2;
    then_block.label = "if.then.0";
    then_block.predecessors.append(0);

    Compiler_IR_Instruction c99;
    c99.kind = Compiler_IR_Instruction_Kind::Constant;
    c99.result_id = 3;
    c99.text = "99";
    then_block.instructions.append(c99);

    Compiler_IR_Instruction s99;
    s99.kind = Compiler_IR_Instruction_Kind::Store;
    s99.local_slot_id = 0;
    s99.operands.append(3);
    then_block.instructions.append(s99);

    then_block.terminator.kind = Compiler_IR_Terminator_Kind::Jump;
    then_block.terminator.successors.append(3);

    Compiler_IR_Block join_block;
    join_block.id = 3;
    join_block.label = "if.end.0";
    join_block.predecessors.append(2);
    join_block.predecessors.append(4);

    Compiler_IR_Instruction load_x;
    load_x.kind = Compiler_IR_Instruction_Kind::Load;
    load_x.result_id = 6;
    load_x.local_slot_id = 0;
    join_block.instructions.append(load_x);

    join_block.terminator.kind = Compiler_IR_Terminator_Kind::Return;
    join_block.terminator.return_value = 6;
    join_block.terminator.successors.append(1);

    Compiler_IR_Block else_block;
    else_block.id = 4;
    else_block.label = "if.else.0";
    else_block.predecessors.append(0);

    Compiler_IR_Instruction cm1;
    cm1.kind = Compiler_IR_Instruction_Kind::Constant;
    cm1.result_id = 5;
    cm1.text = "-1";
    else_block.instructions.append(cm1);

    Compiler_IR_Instruction sm1;
    sm1.kind = Compiler_IR_Instruction_Kind::Store;
    sm1.local_slot_id = 0;
    sm1.operands.append(5);
    else_block.instructions.append(sm1);

    else_block.terminator.kind = Compiler_IR_Terminator_Kind::Jump;
    else_block.terminator.successors.append(3);

    function.blocks.append(entry);
    function.blocks.append(exit_blk);
    function.blocks.append(then_block);
    function.blocks.append(join_block);
    function.blocks.append(else_block);
    return function;
  }
}  // namespace

// ---------------------------------------------------------------------------
// parse_integer_constant boundary tests
// ---------------------------------------------------------------------------

// parse_integer_constant is a free function in Compiler_Dataflow_Detail.
// Expose it through a thin wrapper to avoid repeating the namespace.
namespace
{
  bool test_parse_int(const std::string &text, long long &value)
  {
    return Compiler_Dataflow_Detail::parse_integer_constant(text, value);
  }

  Compiler_Dataflow_Constant
  test_unary(Compiler_Operator_Kind op, long long v)
  {
    Compiler_Dataflow_Constant operand;
    operand.kind = Compiler_Dataflow_Constant_Kind::Integer;
    operand.integer_value = v;
    return Compiler_Dataflow_Detail::evaluate_unary(op, operand);
  }

  Compiler_Dataflow_Constant
  test_binary(Compiler_Operator_Kind op, long long lv, long long rv)
  {
    Compiler_Dataflow_Constant lhs, rhs;
    lhs.kind = rhs.kind = Compiler_Dataflow_Constant_Kind::Integer;
    lhs.integer_value = lv;
    rhs.integer_value = rv;
    return Compiler_Dataflow_Detail::evaluate_binary(op, lhs, rhs);
  }
}  // namespace

TEST(CompilerDataflowConstant, ParsesLLONG_MAX)
{
  long long value = 0;
  EXPECT_TRUE(test_parse_int(std::to_string(LLONG_MAX), value));
  EXPECT_EQ(value, LLONG_MAX);
}

TEST(CompilerDataflowConstant, ParsesLLONG_MINString)
{
  long long value = 0;
  EXPECT_TRUE(test_parse_int(std::to_string(LLONG_MIN), value));
  EXPECT_EQ(value, LLONG_MIN);
}

TEST(CompilerDataflowConstant, RejectsOverflowAboveLLONG_MAX)
{
  long long value = 0;
  EXPECT_FALSE(test_parse_int("9223372036854775808", value)); // LLONG_MAX + 1
}

TEST(CompilerDataflowConstant, RejectsEmptyString)
{
  long long value = 0;
  EXPECT_FALSE(test_parse_int("", value));
}

// ---------------------------------------------------------------------------
// evaluate_unary: -LLONG_MIN must NOT fold (overflow)
// ---------------------------------------------------------------------------

TEST(CompilerDataflowConstant, UnaryMinusLLONG_MINDoesNotFold)
{
  const auto result = test_unary(Compiler_Operator_Kind::Minus, LLONG_MIN);
  EXPECT_EQ(result.kind, Compiler_Dataflow_Constant_Kind::Unknown);
}

TEST(CompilerDataflowConstant, UnaryMinusNormalIntegerFolds)
{
  const auto result = test_unary(Compiler_Operator_Kind::Minus, 42);
  EXPECT_EQ(result.kind, Compiler_Dataflow_Constant_Kind::Integer);
  EXPECT_EQ(result.integer_value, -42);
}

// ---------------------------------------------------------------------------
// evaluate_binary: LLONG_MIN / -1 and LLONG_MIN % -1 must NOT fold
// ---------------------------------------------------------------------------

TEST(CompilerDataflowConstant, BinaryDivLLONG_MINByMinusOneDoesNotFold)
{
  const auto result = test_binary(Compiler_Operator_Kind::Slash, LLONG_MIN, -1);
  EXPECT_EQ(result.kind, Compiler_Dataflow_Constant_Kind::Unknown);
}

TEST(CompilerDataflowConstant, BinaryModLLONG_MINByMinusOneDoesNotFold)
{
  const auto result = test_binary(Compiler_Operator_Kind::Percent, LLONG_MIN, -1);
  EXPECT_EQ(result.kind, Compiler_Dataflow_Constant_Kind::Unknown);
}

TEST(CompilerDataflowConstant, BinaryAddOverflowDoesNotFold)
{
  const auto result = test_binary(Compiler_Operator_Kind::Plus, LLONG_MAX, 1);
  EXPECT_EQ(result.kind, Compiler_Dataflow_Constant_Kind::Unknown);
}

// ---------------------------------------------------------------------------
// Branch-fold live_out liveness preservation
// ---------------------------------------------------------------------------

TEST(CompilerDataflow, BranchFoldPreservesLiveOutForTakenArm)
{
  // With flag=true the "then" arm (stores 99) is taken; the "else" arm is dead.
  // After DCE: slot 0 must still be live going into the join block (so the
  // load of x at join is not erroneously eliminated as a dead store).
  const auto function = make_branch_fold_liveout_ir(true);
  const auto ir_report = validate_ir_function(function);
  ASSERT_TRUE(ir_report.valid);

  const auto optimized = eliminate_dead_code(function);
  const auto report = validate_dead_code_elimination(optimized);
  ASSERT_TRUE(report.valid);

  EXPECT_EQ(optimized.folded_branches, 1u);
  EXPECT_GT(optimized.removed_blocks, 0u);

  // The join block must survive and its load of x must be kept.
  bool found_join = false;
  for (size_t i = 0; i < optimized.function.blocks.size(); ++i)
    {
      const auto &blk = optimized.function.blocks.access(i);
      if (blk.label == "if.end.0")
        {
          found_join = true;
          bool has_load = false;
          for (size_t j = 0; j < blk.instructions.size(); ++j)
            if (blk.instructions.access(j).kind == Compiler_IR_Instruction_Kind::Load)
              has_load = true;
          EXPECT_TRUE(has_load) << "load of x must survive in join block";
        }
    }
  EXPECT_TRUE(found_join) << "join block must survive branch fold";

  // The else arm must be gone.
  for (size_t i = 0; i < optimized.function.blocks.size(); ++i)
    EXPECT_NE(optimized.function.blocks.access(i).label, "if.else.0")
      << "dead else block must be removed";
}

TEST(CompilerDataflow, BranchFoldFalseTakesElseArm)
{
  // With flag=false the "else" arm is taken; "then" arm is dead.
  const auto function = make_branch_fold_liveout_ir(false);
  const auto ir_report = validate_ir_function(function);
  ASSERT_TRUE(ir_report.valid);

  const auto optimized = eliminate_dead_code(function);
  ASSERT_TRUE(validate_dead_code_elimination(optimized).valid);

  EXPECT_EQ(optimized.folded_branches, 1u);

  for (size_t i = 0; i < optimized.function.blocks.size(); ++i)
    EXPECT_NE(optimized.function.blocks.access(i).label, "if.then.0")
      << "dead then block must be removed";
}

TEST(CompilerDataflow, ReportsUninitializedLocalLoads)
{
  const auto function = make_manual_uninitialized_ir();
  const auto ir_report = validate_ir_function(function);
  ASSERT_TRUE(ir_report.valid);

  const auto analysis = analyze_dataflow_function(function);
  const auto report = validate_dataflow_analysis(function, analysis);
  ASSERT_TRUE(report.valid);
  ASSERT_EQ(analysis.uninitialized_reads.size(), 1u);
  EXPECT_EQ(analysis.uninitialized_reads.access(0).block_id, 0u);
  EXPECT_EQ(analysis.uninitialized_reads.access(0).instruction_index, 0u);
  EXPECT_EQ(analysis.uninitialized_reads.access(0).slot_id, 0u);

  const auto dump = compiler_dump_dataflow_analysis(&function, analysis);
  EXPECT_NE(dump.find("UninitializedReads:"), std::string::npos);
  EXPECT_NE(dump.find("B0/I0 -> s0"), std::string::npos);
}

TEST(CompilerDataflow, ComputesReachabilityLivenessAndConstants)
{
  const auto function = make_branching_ir();
  const auto ir_report = validate_ir_function(function);
  ASSERT_TRUE(ir_report.valid);

  const auto analysis = analyze_dataflow_function(function);
  const auto report = validate_dataflow_analysis(function, analysis);
  ASSERT_TRUE(report.valid);

  ASSERT_EQ(function.blocks.size(), 5u);
  EXPECT_TRUE(analysis.reachable_blocks.access(0));
  EXPECT_TRUE(analysis.reachable_blocks.access(1));
  EXPECT_TRUE(analysis.reachable_blocks.access(2));
  EXPECT_TRUE(analysis.reachable_blocks.access(3));
  EXPECT_TRUE(analysis.reachable_blocks.access(4));
  EXPECT_EQ(analysis.foldable_branch_count, 0u);
  EXPECT_TRUE(analysis.uninitialized_reads.is_empty());

  ASSERT_EQ(function.local_slots.size(), 2u);
  EXPECT_TRUE(analysis.assigned_in_slots.access(0).test(0));
  EXPECT_FALSE(analysis.assigned_in_slots.access(0).test(1));
  EXPECT_EQ(analysis.constant_out_slots.access(0).access(1).kind,
            Compiler_Dataflow_Constant_Kind::Integer);
  EXPECT_EQ(analysis.constant_out_slots.access(0).access(1).integer_value, 0);
  EXPECT_TRUE(analysis.live_out_slots.access(2).test(1));
  EXPECT_TRUE(analysis.live_out_slots.access(4).test(1));
}

TEST(CompilerDataflow, EliminatesDeadStoresAndConstantBranches)
{
  const auto function = make_constant_branch_ir();
  const auto ir_report = validate_ir_function(function);
  ASSERT_TRUE(ir_report.valid);

  const auto optimized = eliminate_dead_code(function);
  const auto report = validate_dead_code_elimination(optimized);
  ASSERT_TRUE(report.valid);
  EXPECT_EQ(optimized.folded_branches, 1u);
  EXPECT_GT(optimized.removed_blocks, 0u);
  EXPECT_GT(optimized.removed_instructions, 0u);
  ASSERT_EQ(optimized.function.blocks.size(), 4u);
  EXPECT_EQ(optimized.function.blocks.access(0).terminator.kind,
            Compiler_IR_Terminator_Kind::Jump);
  EXPECT_EQ(optimized.function.blocks.access(0).terminator.successors.size(), 1u);
  EXPECT_EQ(optimized.function.blocks.access(2).label, "if.then.0");
  EXPECT_EQ(optimized.function.blocks.access(3).label, "if.end.0");
  for (size_t i = 0; i < optimized.function.blocks.size(); ++i)
    EXPECT_NE(optimized.function.blocks.access(i).label, "if.else.0");
}
