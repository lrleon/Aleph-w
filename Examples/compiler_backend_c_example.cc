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
 * @file compiler_backend_c_example.cc
 * @brief Minimal example that emits portable C from explicit IR.
 */

#include <iostream>
#include <memory>

#include <Compiler_Backend_C.H>

using namespace Aleph;

namespace
{
  struct Demo_IR_Module
  {
    Compiler_IR_Module module;
    std::unique_ptr<Compiler_IR_Function> function;
    std::unique_ptr<Compiler_IR_Function> top_level;
  };

  Compiler_IR_Block
  make_exit_block()
  {
    Compiler_IR_Block block;
    block.id = 1;
    block.label = "exit";
    block.terminator.kind = Compiler_IR_Terminator_Kind::Exit;
    return block;
  }

  Demo_IR_Module
  make_demo_module()
  {
    Demo_IR_Module demo;
    auto &module = demo.module;

    Compiler_IR_Slot global;
    global.id = 0;
    global.kind = Compiler_IR_Slot_Kind::Global;
    global.name = "answer";
    module.global_slots.append(global);

    demo.function = std::make_unique<Compiler_IR_Function>();
    auto * function = demo.function.get();
    function->id = 0;
    function->name = "add_one";
    function->entry_block = 0;
    function->exit_block = 1;
    function->next_value_id = 4;

    Compiler_IR_Slot param;
    param.id = 0;
    param.kind = Compiler_IR_Slot_Kind::Parameter;
    param.name = "x";
    function->local_slots.append(param);

    Compiler_IR_Slot local;
    local.id = 1;
    local.kind = Compiler_IR_Slot_Kind::Local;
    local.name = "tmp";
    function->local_slots.append(local);

    Compiler_IR_Block entry;
    entry.id = 0;
    entry.label = "entry";

    Compiler_IR_Instruction one;
    one.kind = Compiler_IR_Instruction_Kind::Constant;
    one.result_id = 1;
    one.text = "1";
    entry.instructions.append(one);

    Compiler_IR_Instruction store_tmp;
    store_tmp.kind = Compiler_IR_Instruction_Kind::Store;
    store_tmp.local_slot_id = 1;
    store_tmp.operands.append(1);
    entry.instructions.append(store_tmp);

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

    function->blocks.append(entry);
    function->blocks.append(exit);
    module.functions.append(function);

    demo.top_level = std::make_unique<Compiler_IR_Function>();
    auto * top = demo.top_level.get();
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
    constant.text = "41";
    top_entry.instructions.append(constant);

    Compiler_IR_Instruction call;
    call.kind = Compiler_IR_Instruction_Kind::Call;
    call.result_id = 3;
    call.operands.append(1);
    call.operands.append(2);
    top_entry.instructions.append(call);

    Compiler_IR_Instruction store_answer;
    store_answer.kind = Compiler_IR_Instruction_Kind::Store;
    store_answer.global_slot_id = 0;
    store_answer.operands.append(3);
    top_entry.instructions.append(store_answer);

    top_entry.terminator.kind = Compiler_IR_Terminator_Kind::Jump;
    top_entry.terminator.successors.append(1);

    auto top_exit = make_exit_block();
    top_exit.predecessors.append(0);

    top->blocks.append(top_entry);
    top->blocks.append(top_exit);
    module.top_level = top;
    return demo;
  }
}

int main()
{
  auto module = make_demo_module();

  Compiler_C_Backend_Options options;
  options.module_name = "example_demo";
  options.emit_main = true;

  const auto emitted = compiler_emit_c_module(&module.module, options);
  if (not emitted.valid)
    {
      std::cerr << "Emission errors:\n";
      for (size_t i = 0; i < emitted.errors.size(); ++i)
        std::cerr << "- " << emitted.errors.access(i) << '\n';
      return 1;
    }

  std::cout << emitted.source;
  if (not emitted.warnings.is_empty())
    {
      std::cout << "\n/* Warnings:\n";
      for (size_t i = 0; i < emitted.warnings.size(); ++i)
        std::cout << " - " << emitted.warnings.access(i) << '\n';
      std::cout << "*/\n";
    }

  return 0;
}
