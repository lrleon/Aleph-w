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
 * @file compiler_builder_example.cc
 * @brief Minimal example that builds HIR and IR manually without using the MVP parser.
 */

# include <iostream>

# include <Compiler_HIR_Builder.H>
# include <Compiler_IR_Builder.H>

using namespace Aleph;

int main()
{
  Compiler_Type_Context types;

  Compiler_HIR_Context hir_ctx(1 << 15);
  Compiler_HIR_Builder hir(hir_ctx);
  auto * hir_module = hir.make_module();
  auto * hir_body = hir.make_block();
  auto * hir_function = hir.make_function(
    "add",
    types.make_function_type(
      {types.integer_type(), types.integer_type()}, types.integer_type()));
  hir.add_parameter(*hir_function, "x", types.integer_type());
  hir.add_parameter(*hir_function, "y", types.integer_type());
  hir.set_body(*hir_function, hir_body);
  hir.append_function(*hir_module, hir_function);
  hir.append_statement(
    *hir_body,
    hir.make_return_stmt(hir.make_binary(hir.make_variable("x", types.integer_type()),
                                         Compiler_Operator_Kind::Plus,
                                         hir.make_variable("y", types.integer_type()),
                                         types.integer_type())));

  auto * call = hir.make_call(
    hir.make_variable("add",
                      types.make_function_type(
                        {types.integer_type(), types.integer_type()},
                        types.integer_type())),
    types.integer_type());
  hir.append_argument(*call, hir.make_integer_constant(40, types.integer_type()));
  hir.append_argument(*call, hir.make_integer_constant(2, types.integer_type()));
  hir.append_top_level_statement(
    *hir_module, hir.make_let_stmt("value", types.integer_type(), {}, call));

  Compiler_IR_Context ir_ctx(1 << 15);
  Compiler_IR_Builder ir(ir_ctx);
  auto * ir_module = ir.make_module();
  const auto global_value = ir.add_global_slot(*ir_module, "value", types.integer_type());
  auto * ir_function = ir.make_function(
    "add",
    types.make_function_type(
      {types.integer_type(), types.integer_type()}, types.integer_type()));
  const auto ir_function_id = ir.append_function(*ir_module, ir_function);
  const auto x
    = ir.add_local_slot(*ir_function,
                        Compiler_IR_Slot_Kind::Parameter,
                        "x",
                        types.integer_type());
  const auto y
    = ir.add_local_slot(*ir_function,
                        Compiler_IR_Slot_Kind::Parameter,
                        "y",
                        types.integer_type());
  const auto entry = ir.create_block(*ir_function, "entry");
  const auto exit = ir.create_block(*ir_function, "exit");
  ir.set_entry_block(*ir_function, entry);
  ir.set_exit_block(*ir_function, exit);
  const auto vx = ir.emit_load_local(*ir_function, entry, x, types.integer_type());
  const auto vy = ir.emit_load_local(*ir_function, entry, y, types.integer_type());
  const auto sum
    = ir.emit_binary(*ir_function,
                     entry,
                     Compiler_Operator_Kind::Plus,
                     vx,
                     vy,
                     types.integer_type());
  ir.set_return(*ir_function, entry, sum);
  ir.set_exit(*ir_function, exit);

  auto * top = ir.make_function("<top-level>");
  ir.set_top_level(*ir_module, top);
  const auto top_entry = ir.create_block(*top, "entry");
  const auto top_exit = ir.create_block(*top, "exit");
  ir.set_entry_block(*top, top_entry);
  ir.set_exit_block(*top, top_exit);
  const auto add_ref = ir.emit_function_ref(
    *top,
    top_entry,
    ir_function_id,
    types.make_function_type(
      {types.integer_type(), types.integer_type()}, types.integer_type()));
  DynArray<Compiler_IR_Value_Id> arguments;
  arguments.append(ir.emit_constant(*top, top_entry, types.integer_type(), "40"));
  arguments.append(ir.emit_constant(*top, top_entry, types.integer_type(), "2"));
  const auto result
    = ir.emit_call(*top, top_entry, add_ref, arguments, types.integer_type());
  ir.emit_store_global(*top, top_entry, global_value, result, types.integer_type());
  ir.set_jump(*top, top_entry, top_exit);
  ir.set_exit(*top, top_exit);

  std::cout << compiler_dump_hir_module(hir_module, types);
  std::cout << '\n';
  std::cout << compiler_dump_ir_module(ir_module, &types);
  return 0;
}
