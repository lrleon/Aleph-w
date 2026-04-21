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
 * @file compiler_builder_test.cc
 * @brief Tests for the reusable HIR/IR builders.
 */

#include <gtest/gtest.h>

#include <Compiler_HIR_Builder.H>
#include <Compiler_IR_Builder.H>

using namespace Aleph;

TEST(CompilerBuilder, BuildsTypedHIRWithoutParserOrLowering)
{
  Compiler_Type_Context types;
  Compiler_HIR_Context hir_ctx(1 << 15);
  Compiler_HIR_Builder hir(hir_ctx);

  auto * module = hir.make_module();
  auto * body = hir.make_block();
  auto * function = hir.make_function(
    "add",
    types.make_function_type(
      {types.integer_type(), types.integer_type()}, types.integer_type()),
    {});
  hir.add_parameter(*function, "x", types.integer_type());
  hir.add_parameter(*function, "y", types.integer_type());
  hir.set_body(*function, body);
  hir.append_function(*module, function);

  auto * sum = hir.make_binary(hir.make_variable("x", types.integer_type()),
                               Compiler_Operator_Kind::Plus,
                               hir.make_variable("y", types.integer_type()),
                               types.integer_type());
  hir.append_statement(*body, hir.make_return_stmt(sum));

  auto * call = hir.make_call(
    hir.make_variable("add",
                      types.make_function_type(
                        {types.integer_type(), types.integer_type()},
                        types.integer_type())),
    types.integer_type());
  hir.append_argument(*call, hir.make_integer_constant(40, types.integer_type()));
  hir.append_argument(*call, hir.make_integer_constant(2, types.integer_type()));
  hir.append_top_level_statement(
    *module, hir.make_let_stmt("value", types.integer_type(), {}, call));

  EXPECT_EQ(
    compiler_dump_hir_module(module, types),
    "HIRModule\n"
    "  Function(add): fn(Int, Int) -> Int\n"
    "    Params: x: Int, y: Int\n"
    "    Body:\n"
    "      Block\n"
    "        Return\n"
    "          Binary(Plus): Int\n"
    "            Variable(x): Int\n"
    "            Variable(y): Int\n"
    "  Let(value): Int\n"
    "    Call: Int\n"
    "      Callee:\n"
    "        Variable(add): fn(Int, Int) -> Int\n"
    "      Args:\n"
    "        Constant(Integer, 40): Int\n"
    "        Constant(Integer, 2): Int\n");
}

TEST(CompilerBuilder, BuildsValidatedIRWithoutHIRLowering)
{
  Compiler_Type_Context types;
  Compiler_IR_Context ir_ctx(1 << 15);
  Compiler_IR_Builder ir(ir_ctx);

  auto * module = ir.make_module();
  const auto global_value = ir.add_global_slot(*module, "value", types.integer_type());

  auto * function = ir.make_function(
    "add",
    types.make_function_type(
      {types.integer_type(), types.integer_type()}, types.integer_type()));
  const auto function_id = ir.append_function(*module, function);
  const auto x
    = ir.add_local_slot(*function,
                        Compiler_IR_Slot_Kind::Parameter,
                        "x",
                        types.integer_type());
  const auto y
    = ir.add_local_slot(*function,
                        Compiler_IR_Slot_Kind::Parameter,
                        "y",
                        types.integer_type());
  const auto entry = ir.create_block(*function, "entry");
  const auto exit = ir.create_block(*function, "exit");
  ir.set_entry_block(*function, entry);
  ir.set_exit_block(*function, exit);

  const auto vx = ir.emit_load_local(*function, entry, x, types.integer_type());
  const auto vy = ir.emit_load_local(*function, entry, y, types.integer_type());
  const auto sum = ir.emit_binary(
    *function, entry, Compiler_Operator_Kind::Plus, vx, vy, types.integer_type());
  ir.set_return(*function, entry, sum);
  ir.set_exit(*function, exit);

  auto * top = ir.make_function("<top-level>");
  ir.set_top_level(*module, top);
  const auto top_entry = ir.create_block(*top, "entry");
  const auto top_exit = ir.create_block(*top, "exit");
  ir.set_entry_block(*top, top_entry);
  ir.set_exit_block(*top, top_exit);

  const auto add_ref = ir.emit_function_ref(
    *top,
    top_entry,
    function_id,
    types.make_function_type(
      {types.integer_type(), types.integer_type()}, types.integer_type()));
  DynArray<Compiler_IR_Value_Id> arguments;
  arguments.append(ir.emit_constant(*top, top_entry, types.integer_type(), "40"));
  arguments.append(ir.emit_constant(*top, top_entry, types.integer_type(), "2"));
  const auto call
    = ir.emit_call(*top, top_entry, add_ref, arguments, types.integer_type());
  ir.emit_store_global(*top, top_entry, global_value, call, types.integer_type());
  ir.set_jump(*top, top_entry, top_exit);
  ir.set_exit(*top, top_exit);

  const auto report = validate_ir_module(*module);
  ASSERT_TRUE(report.valid);
  EXPECT_TRUE(report.warnings.is_empty());
  EXPECT_EQ(
    compiler_dump_ir_module(module, &types),
    "IRModule\n"
    "  Globals:\n"
    "    g0 value: Int\n"
    "  IRFunction(add): fn(Int, Int) -> Int\n"
    "    Entry: B0\n"
    "    Exit: B1\n"
    "    Slots:\n"
    "      s0 [Parameter] x: Int\n"
    "      s1 [Parameter] y: Int\n"
    "    Block B0 [entry]\n"
    "      Instructions:\n"
    "        v1 = Load s0 : Int\n"
    "        v2 = Load s1 : Int\n"
    "        v3 = Binary(Plus) [v1, v2] : Int\n"
    "      Terminator: Return [v3]\n"
    "      Successors: B1\n"
    "    Block B1 [exit]\n"
    "      Instructions: <none>\n"
    "      Terminator: Exit\n"
    "      Successors: <none>\n"
    "  IRFunction(<top-level>)\n"
    "    Entry: B0\n"
    "    Exit: B1\n"
    "    Block B0 [entry]\n"
    "      Instructions:\n"
    "        v1 = FunctionRef f0 : fn(Int, Int) -> Int\n"
    "        v2 = Constant(40) : Int\n"
    "        v3 = Constant(2) : Int\n"
    "        v4 = Call [v1, v2, v3] : Int\n"
    "        Store g0 [v4] : Int\n"
    "      Terminator: Jump\n"
    "      Successors: B1\n"
    "    Block B1 [exit]\n"
    "      Instructions: <none>\n"
    "      Terminator: Exit\n"
    "      Successors: <none>\n");
}
