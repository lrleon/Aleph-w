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
 * @file compiler_ir_test.cc
 * @brief Tests for Compiler_IR.H.
 */

#include <gtest/gtest.h>

#include <Compiler_HIR_Lowering_MVP.H>
#include <Compiler_IR_Lowering_MVP.H>
#include <Compiler_Parser.H>

using namespace Aleph;

namespace
{
  const Compiler_IR_Module *
  lower_to_ir(const std::string & source,
              Source_Manager & sm,
              Diagnostic_Engine & dx,
              Compiler_Ast_Context & ast_ctx,
              Compiler_HIR_Context & hir_ctx,
              Compiler_IR_Context & ir_ctx,
              Compiler_Typed_Semantic_Analyzer & typed)
  {
    const auto file_id = sm.add_virtual_file("main.aw", source);
    Compiler_Parser parser(ast_ctx, sm, file_id, &dx);
    const auto * module = parser.parse_module();
    EXPECT_FALSE(dx.has_errors());

    typed.analyze_module(module);
    EXPECT_FALSE(dx.has_errors());

    Compiler_HIR_Lowering hir_lowering(hir_ctx, typed);
    const auto * hir = hir_lowering.lower_module(module);

    Compiler_IR_Lowering ir_lowering(ir_ctx, &typed.type_context());
    return ir_lowering.lower_module(hir);
  }
}

TEST(CompilerIR, LowersLinearFunctionAndTopLevelDeterministically)
{
  Source_Manager sm;
  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ast_ctx(1 << 16);
  Compiler_HIR_Context hir_ctx(1 << 16);
  Compiler_IR_Context ir_ctx(1 << 16);
  Compiler_Typed_Semantic_Analyzer typed(&dx);

  const auto * ir = lower_to_ir(
      "fn add(x, y) {\n"
      "  let z = x + y;\n"
      "  return z;\n"
      "}\n"
      "let value = add(1, 2);\n",
      sm, dx, ast_ctx, hir_ctx, ir_ctx, typed);

  const auto report = validate_ir_module(*ir);
  ASSERT_TRUE(report.valid);
  EXPECT_TRUE(report.warnings.is_empty());

  const auto dump = compiler_dump_ir_module(ir, &typed.type_context());
  EXPECT_EQ(
      dump,
      "IRModule\n"
      "  Globals:\n"
      "    g0 value: Int\n"
      "  IRFunction(add): fn(Int, Int) -> Int\n"
      "    Entry: B0\n"
      "    Exit: B1\n"
      "    Slots:\n"
      "      s0 [Parameter] x: Int\n"
      "      s1 [Parameter] y: Int\n"
      "      s2 [Local] z: Int\n"
      "    Block B0 [entry]\n"
      "      Instructions:\n"
      "        v1 = Load s0 : Int\n"
      "        v2 = Load s1 : Int\n"
      "        v3 = Binary(Plus) [v1, v2] : Int\n"
      "        Store s2 [v3] : Int\n"
      "        v4 = Load s2 : Int\n"
      "      Terminator: Return [v4]\n"
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
      "        v2 = Constant(1) : Int\n"
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

TEST(CompilerIR, ResolvesGlobalSlotsInsideFunctions)
{
  Source_Manager sm;
  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ast_ctx(1 << 16);
  Compiler_HIR_Context hir_ctx(1 << 16);
  Compiler_IR_Context ir_ctx(1 << 16);
  Compiler_Typed_Semantic_Analyzer typed(&dx);

  const auto * ir = lower_to_ir(
      "let base = 10;\n"
      "fn add_base(x) {\n"
      "  return x + base;\n"
      "}\n",
      sm, dx, ast_ctx, hir_ctx, ir_ctx, typed);

  const auto & function = *ir->functions.access(0);
  ASSERT_EQ(ir->global_slots.size(), 1u);
  ASSERT_EQ(function.blocks.access(0).instructions.size(), 3u);
  EXPECT_EQ(function.blocks.access(0).instructions.access(1).kind,
            Compiler_IR_Instruction_Kind::Load);
  EXPECT_EQ(function.blocks.access(0).instructions.access(1).global_slot_id, 0u);
}

TEST(CompilerIR, LowersIfElseAndLoopsIntoExplicitBlocks)
{
  Source_Manager sm;
  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ast_ctx(1 << 16);
  Compiler_HIR_Context hir_ctx(1 << 16);
  Compiler_IR_Context ir_ctx(1 << 16);
  Compiler_Typed_Semantic_Analyzer typed(&dx);

  const auto * ir = lower_to_ir(
      "fn loop(flag) {\n"
      "  let value = 0;\n"
      "  while (flag) {\n"
      "    if (value == 0) {\n"
      "      value = 1;\n"
      "    } else {\n"
      "      value = 2;\n"
      "    }\n"
      "    continue;\n"
      "  }\n"
      "  return value;\n"
      "}\n",
      sm, dx, ast_ctx, hir_ctx, ir_ctx, typed);

  const auto & function = *ir->functions.access(0);
  const auto report = validate_ir_function(function, ir);
  ASSERT_TRUE(report.valid);
  EXPECT_TRUE(report.warnings.is_empty());
  EXPECT_GE(function.blocks.size(), 7u);
  EXPECT_EQ(function.blocks.access(2).terminator.kind,
            Compiler_IR_Terminator_Kind::Branch);
}

TEST(CompilerIR, ValidatorWarnsAboutUnreachableBlocks)
{
  Source_Manager sm;
  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ast_ctx(1 << 16);
  Compiler_HIR_Context hir_ctx(1 << 16);
  Compiler_IR_Context ir_ctx(1 << 16);
  Compiler_Typed_Semantic_Analyzer typed(&dx);

  const auto * ir = lower_to_ir(
      "fn dead() {\n"
      "  return 1;\n"
      "  let later = 2;\n"
      "}\n",
      sm, dx, ast_ctx, hir_ctx, ir_ctx, typed);

  const auto report = validate_ir_module(*ir);
  ASSERT_TRUE(report.valid);
  ASSERT_EQ(report.warnings.size(), 1u);
  EXPECT_NE(report.warnings.access(0).find("unreachable"), std::string::npos);
}
