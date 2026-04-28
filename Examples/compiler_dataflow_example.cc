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
 * @file compiler_dataflow_example.cc
 * @brief Minimal example that runs reusable dataflow analyses and DCE over Compiler_IR.H.
 */

#include <iostream>

#include <Compiler_Dataflow.H>

using namespace Aleph;

namespace
{
  Compiler_IR_Function
  make_demo_function()
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

    Compiler_IR_Block exit;
    exit.id = 1;
    exit.label = "exit";
    exit.terminator.kind = Compiler_IR_Terminator_Kind::Exit;
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

int main()
{
  const auto function = make_demo_function();
  const auto analysis = analyze_dataflow_function(function);
  const auto dataflow_report = validate_dataflow_analysis(function, analysis);
  const auto optimized = eliminate_dead_code(function);
  const auto dce_report = validate_dead_code_elimination(optimized);

  std::cout << compiler_dump_dataflow_analysis(&function, analysis) << '\n';
  std::cout << "OptimizedSummary\n";
  std::cout << "  Blocks: " << optimized.function.blocks.size() << '\n';
  std::cout << "  RemovedBlocks: " << optimized.removed_blocks << '\n';
  std::cout << "  RemovedInstructions: " << optimized.removed_instructions << '\n';
  std::cout << "  FoldedBranches: " << optimized.folded_branches << '\n';
  for (size_t i = 0; i < optimized.function.blocks.size(); ++i)
    {
      const auto & block = optimized.function.blocks.access(i);
      std::cout << "  Block " << i << " [" << block.label << "] -> "
                << compiler_ir_terminator_kind_name(block.terminator.kind) << '\n';
    }

  if (not dataflow_report.warnings.is_empty() or not dce_report.warnings.is_empty())
    {
      std::cout << "\nWarnings:\n";
      for (size_t i = 0; i < dataflow_report.warnings.size(); ++i)
        std::cout << "- " << dataflow_report.warnings.access(i) << '\n';
      for (size_t i = 0; i < dce_report.warnings.size(); ++i)
        std::cout << "- " << dce_report.warnings.access(i) << '\n';
    }

  return dataflow_report.valid and dce_report.valid ? 0 : 1;
}
