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
 * @file bytecode_interpreter_example.cc
 * @brief Minimal example that lowers IR to bytecode and executes it in the VM.
 */

#include <iostream>

#include <Bytecode_Interpreter.H>

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

  Compiler_IR_Module
  make_demo_module()
  {
    Compiler_IR_Module module;

    Compiler_IR_Slot global;
    global.id = 0;
    global.kind = Compiler_IR_Slot_Kind::Global;
    global.name = "answer";
    module.global_slots.append(global);

    auto * function = new Compiler_IR_Function();
    function->id = 0;
    function->name = "choose";
    function->entry_block = 0;
    function->exit_block = 1;
    function->next_value_id = 5;

    Compiler_IR_Slot flag;
    flag.id = 0;
    flag.kind = Compiler_IR_Slot_Kind::Parameter;
    flag.name = "flag";
    function->local_slots.append(flag);

    Compiler_IR_Slot value;
    value.id = 1;
    value.kind = Compiler_IR_Slot_Kind::Local;
    value.name = "value";
    function->local_slots.append(value);

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

    Compiler_IR_Block exit;
    exit.id = 1;
    exit.label = "exit";
    exit.terminator.kind = Compiler_IR_Terminator_Kind::Exit;
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

    function->blocks.append(entry);
    function->blocks.append(exit);
    function->blocks.append(then_block);
    function->blocks.append(join_block);
    function->blocks.append(else_block);
    module.functions.append(function);

    auto * top = new Compiler_IR_Function();
    top->id = compiler_ir_invalid_id();
    top->name = "<top-level>";
    top->entry_block = 0;
    top->exit_block = 1;
    top->next_value_id = 3;

    Compiler_IR_Block top_entry;
    top_entry.id = 0;
    top_entry.label = "entry";

    Compiler_IR_Instruction function_ref;
    function_ref.kind = Compiler_IR_Instruction_Kind::Function_Ref;
    function_ref.result_id = 1;
    function_ref.function_id = 0;
    top_entry.instructions.append(function_ref);

    Compiler_IR_Instruction constant;
    constant.kind = Compiler_IR_Instruction_Kind::Constant;
    constant.result_id = 2;
    constant.text = "true";
    constant.bool_value = true;
    top_entry.instructions.append(constant);

    Compiler_IR_Instruction call;
    call.kind = Compiler_IR_Instruction_Kind::Call;
    call.result_id = 3;
    call.operands.append(1);
    call.operands.append(2);
    top_entry.instructions.append(call);

    Compiler_IR_Instruction store_global;
    store_global.kind = Compiler_IR_Instruction_Kind::Store;
    store_global.global_slot_id = 0;
    store_global.operands.append(3);
    top_entry.instructions.append(store_global);

    top_entry.terminator.kind = Compiler_IR_Terminator_Kind::Jump;
    top_entry.terminator.successors.append(1);

    auto top_exit = make_exit_block();
    top_exit.predecessors.append(0);

    top->blocks.append(top_entry);
    top->blocks.append(top_exit);
    module.top_level = top;
    return module;
  }

  void
  print_result(const char * label,
               const Compiler_Bytecode_Execution_Result & result,
               const Compiler_Bytecode_Module * module)
  {
    std::cout << label << ": " << compiler_bytecode_completion_kind_name(result.kind);
    if (result.ok())
      std::cout << " -> " << compiler_bytecode_value_to_string(result.value, module);
    else
      std::cout << " [" << result.error.code << "] " << result.error.message;
    std::cout << " (steps=" << result.executed_instructions << ")\n";
  }
}

int main()
{
  Compiler_Bytecode_Context ctx(1 << 16);
  Compiler_Bytecode_Lowering lowering(ctx);

  auto module = make_demo_module();
  const auto * bytecode = lowering.lower_module(&module);
  const auto report = validate_bytecode_module(*bytecode);

  std::cout << compiler_dump_bytecode_module(bytecode) << '\n';
  if (not report.valid)
    {
      std::cout << "Validation errors:\n";
      for (size_t i = 0; i < report.errors.size(); ++i)
        std::cout << "- " << report.errors.access(i) << '\n';
      delete module.functions.access(0);
      delete module.top_level;
      return 1;
    }

  Compiler_Bytecode_VM vm(*bytecode);
  const auto top_level = vm.run_top_level();

  DynArray<Compiler_Bytecode_Value> arguments;
  arguments.append(Compiler_Bytecode_Value::make_bool(false));
  const auto direct_call = vm.call_function(0, arguments);

  print_result("TopLevel", top_level, bytecode);
  print_result("Call choose(false)", direct_call, bytecode);
  std::cout << '\n' << vm.dump_globals();

  delete module.functions.access(0);
  delete module.top_level;
  return top_level.ok() and direct_call.ok() ? 0 : 1;
}
