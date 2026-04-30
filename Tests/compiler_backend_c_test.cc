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
 * @file compiler_backend_c_test.cc
 * @brief Tests for Compiler_Backend_C.H.
 */

#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include <Compiler_Backend_C.H>

using namespace Aleph;

namespace
{
  struct Command_Result
  {
    int status = -1;
    std::string stdout_text;
    std::string stderr_text;
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
    add.op = Compiler_Operator_Kind::Plus;
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

  Compiler_IR_Module
  make_divide_by_zero_top_level_module()
  {
    Compiler_IR_Module module;

    auto * top = new Compiler_IR_Function();
    top->id = compiler_ir_invalid_id();
    top->name = "<top-level>";
    top->entry_block = 0;
    top->exit_block = 1;
    top->next_value_id = 3;

    Compiler_IR_Block entry;
    entry.id = 0;
    entry.label = "entry";

    Compiler_IR_Instruction lhs;
    lhs.kind = Compiler_IR_Instruction_Kind::Constant;
    lhs.result_id = 1;
    lhs.text = "5";
    entry.instructions.append(lhs);

    Compiler_IR_Instruction rhs;
    rhs.kind = Compiler_IR_Instruction_Kind::Constant;
    rhs.result_id = 2;
    rhs.text = "0";
    entry.instructions.append(rhs);

    Compiler_IR_Instruction div;
    div.kind = Compiler_IR_Instruction_Kind::Binary;
    div.result_id = 3;
    div.op = Compiler_Operator_Kind::Slash;
    div.operands.append(1);
    div.operands.append(2);
    entry.instructions.append(div);

    entry.terminator.kind = Compiler_IR_Terminator_Kind::Return;
    entry.terminator.return_value = 3;
    entry.terminator.successors.append(1);

    auto exit = make_exit_block();
    exit.predecessors.append(0);

    top->blocks.append(entry);
    top->blocks.append(exit);
    module.top_level = top;
    return module;
  }

  std::string
  read_text(const std::filesystem::path & path)
  {
    std::ifstream in(path);
    return std::string((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
  }

  void
  write_text(const std::filesystem::path & path,
             const std::string & text)
  {
    std::ofstream out(path);
    out << text;
  }

  std::string
  shell_quote(const std::string & text)
  {
    std::string quoted = "'";
    for (const char ch : text)
      {
        if (ch == '\'')
          quoted += "'\\''";
        else
          quoted.push_back(ch);
      }
    quoted.push_back('\'');
    return quoted;
  }

  bool
  host_has_cc()
  {
    return std::system("command -v cc >/dev/null 2>&1") == 0;
  }

  std::filesystem::path
  unique_temp_path(const std::string & stem,
                   const std::string & extension)
  {
    const auto nonce = std::to_string(
      std::chrono::steady_clock::now().time_since_epoch().count());
    return std::filesystem::temp_directory_path()
      / (stem + "_" + nonce + extension);
  }

  Command_Result
  run_command(const std::string & command)
  {
    Command_Result result;
    const auto stdout_path = unique_temp_path("aleph_backend_stdout", ".txt");
    const auto stderr_path = unique_temp_path("aleph_backend_stderr", ".txt");
    const auto full_command = command
      + " >" + shell_quote(stdout_path.string())
      + " 2>" + shell_quote(stderr_path.string());

    result.status = std::system(full_command.c_str());
    result.stdout_text = read_text(stdout_path);
    result.stderr_text = read_text(stderr_path);
    std::error_code ignore_error;
    std::filesystem::remove(stdout_path, ignore_error);
    std::filesystem::remove(stderr_path, ignore_error);
    return result;
  }

  Command_Result
  compile_and_run(const Compiler_C_Backend_Emission & emission,
                  const std::string & extra_c = "")
  {
    const auto source_path = unique_temp_path("aleph_backend_generated", ".c");
    const auto exe_path = unique_temp_path("aleph_backend_generated", ".out");

    write_text(source_path, emission.source + "\n" + extra_c);
    const auto compile = run_command(
      "cc -std=c11 -Wall -Wextra -pedantic "
      + shell_quote(source_path.string())
      + " -o "
      + shell_quote(exe_path.string()));
    if (compile.status != 0)
      return compile;

    const auto executed = run_command(shell_quote(exe_path.string()));
    std::error_code ignore_error;
    std::filesystem::remove(source_path, ignore_error);
    std::filesystem::remove(exe_path, ignore_error);
    return executed;
  }
}

TEST(CompilerBackendC, EmitsReadableGotoBasedC)
{
  Compiler_IR_Module module;
  auto source = make_branch_ir();
  module.functions.append(&source);

  Compiler_C_Backend_Options options;
  options.module_name = "branch-demo";

  const auto emitted = compiler_emit_c_module(&module, options);
  ASSERT_TRUE(emitted.valid);
  EXPECT_NE(emitted.source.find("goto " + emitted.c_prefix + "_B3"), std::string::npos);
  EXPECT_NE(emitted.source.find("if ("), std::string::npos);
  EXPECT_NE(emitted.source.find("static " + emitted.value_type_name), std::string::npos);
  EXPECT_NE(emitted.source.find(emitted.function_symbols.access(0)), std::string::npos);
  EXPECT_NE(emitted.source.find("choose"), std::string::npos);
}

TEST(CompilerBackendC, StandaloneGeneratedCCompilesAndRunsTopLevel)
{
  if (not host_has_cc())
    GTEST_SKIP() << "No se encontró `cc` en el host";

  auto module = make_module_with_top_level_call();

  Compiler_C_Backend_Options options;
  options.module_name = "top_level_demo";
  options.emit_main = true;

  const auto emitted = compiler_emit_c_module(&module, options);
  ASSERT_TRUE(emitted.valid);

  const auto executed = compile_and_run(emitted);
  EXPECT_EQ(executed.status, 0);
  EXPECT_NE(executed.stdout_text.find("Result = Unit"), std::string::npos);
  EXPECT_NE(executed.stdout_text.find("g0 answer = Int(42)"), std::string::npos);
  EXPECT_TRUE(executed.stderr_text.empty());

  delete module.functions.access(0);
  delete module.top_level;
}

TEST(CompilerBackendC, LibraryStyleEmissionCanCallFunctionsDirectly)
{
  if (not host_has_cc())
    GTEST_SKIP() << "No se encontró `cc` en el host";

  Compiler_IR_Module module;
  auto source = make_branch_ir();
  module.functions.append(&source);

  Compiler_C_Backend_Options options;
  options.module_name = "direct_call_demo";
  const auto emitted = compiler_emit_c_module(&module, options);
  ASSERT_TRUE(emitted.valid);

  const auto harness =
    "\nint main(void)\n"
    "{\n"
    "  " + emitted.module_struct_name + " module;\n"
    "  " + emitted.module_init_symbol + "(&module);\n"
    "  " + emitted.value_type_name + " args[1];\n"
    "  args[0] = " + emitted.c_prefix + "_make_bool(false);\n"
    "  " + emitted.value_type_name + " result = "
      + emitted.function_symbols.access(0) + "(&module, args, 1u);\n"
    "  " + emitted.print_value_symbol + "(stdout, result);\n"
    "  fputc('\\n', stdout);\n"
    "  return 0;\n"
    "}\n";

  const auto executed = compile_and_run(emitted, harness);
  EXPECT_EQ(executed.status, 0);
  EXPECT_EQ(executed.stdout_text, "Int(2)\n");
  EXPECT_TRUE(executed.stderr_text.empty());
}

TEST(CompilerBackendC, GeneratedCRuntimeChecksSurfaceErrors)
{
  if (not host_has_cc())
    GTEST_SKIP() << "No se encontró `cc` en el host";

  auto module = make_divide_by_zero_top_level_module();

  Compiler_C_Backend_Options options;
  options.module_name = "panic_demo";
  options.emit_main = true;

  const auto emitted = compiler_emit_c_module(&module, options);
  ASSERT_TRUE(emitted.valid);

  const auto executed = compile_and_run(emitted);
  EXPECT_NE(executed.status, 0);
  EXPECT_NE(executed.stderr_text.find("division by zero"), std::string::npos);

  delete module.top_level;
}
