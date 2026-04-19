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
 * @file compiler_cfg_test.cc
 * @brief Tests for Compiler_CFG.H.
 */

#include <gtest/gtest.h>

#include <Compiler_CFG.H>
#include <Compiler_Parser.H>

using namespace Aleph;

namespace
{
  const Compiler_CFG_Module *
  lower_to_cfg(const std::string & source,
               Source_Manager & sm,
               Diagnostic_Engine & dx,
               Compiler_Ast_Context & ast_ctx,
               Compiler_HIR_Context & hir_ctx,
               Compiler_CFG_Context & cfg_ctx,
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

    Compiler_CFG_Lowering cfg_lowering(cfg_ctx, &typed.type_context());
    return cfg_lowering.lower_module(hir);
  }
}

TEST(CompilerCFG, LowersLinearFunctionAndTopLevelDeterministically)
{
  Source_Manager sm;
  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ast_ctx(1 << 16);
  Compiler_HIR_Context hir_ctx(1 << 16);
  Compiler_CFG_Context cfg_ctx(1 << 16);
  Compiler_Typed_Semantic_Analyzer typed(&dx);

  const auto * cfg = lower_to_cfg(
      "fn add(x, y) {\n"
      "  let z = x + y;\n"
      "  return z;\n"
      "}\n"
      "let value = add(1, 2);\n",
      sm, dx, ast_ctx, hir_ctx, cfg_ctx, typed);

  const auto report = validate_cfg_module(*cfg);
  ASSERT_TRUE(report.valid);
  EXPECT_TRUE(report.warnings.is_empty());

  const auto dump = compiler_dump_cfg_module(cfg, &typed.type_context());
  EXPECT_EQ(
      dump,
      "CFGModule\n"
      "  CFGFunction(add): fn(Int, Int) -> Int\n"
      "    Entry: B0\n"
      "    Exit: B1\n"
      "    Block B0 [entry]\n"
      "      Statements:\n"
      "        Let(z): Int\n"
      "          Binary(Plus): Int\n"
      "            Variable(x): Int\n"
      "            Variable(y): Int\n"
      "      Terminator: Return\n"
      "        Value:\n"
      "          Variable(z): Int\n"
      "      Successors: B1\n"
      "      Predecessors: <none>\n"
      "    Block B1 [exit]\n"
      "      Statements: <none>\n"
      "      Terminator: Exit\n"
      "      Successors: <none>\n"
      "      Predecessors: B0\n"
      "  CFGFunction(<top-level>)\n"
      "    Entry: B0\n"
      "    Exit: B1\n"
      "    Block B0 [entry]\n"
      "      Statements:\n"
      "        Let(value): Int\n"
      "          Call: Int\n"
      "            Callee:\n"
      "              Variable(add): fn(Int, Int) -> Int\n"
      "            Args:\n"
      "              Constant(Integer, 1): Int\n"
      "              Constant(Integer, 2): Int\n"
      "      Terminator: Jump\n"
      "      Successors: B1\n"
      "      Predecessors: <none>\n"
      "    Block B1 [exit]\n"
      "      Statements: <none>\n"
      "      Terminator: Exit\n"
      "      Successors: <none>\n"
      "      Predecessors: B0\n");
}

TEST(CompilerCFG, LowersIfElseIntoBranchAndJoinBlocks)
{
  Source_Manager sm;
  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ast_ctx(1 << 16);
  Compiler_HIR_Context hir_ctx(1 << 16);
  Compiler_CFG_Context cfg_ctx(1 << 16);
  Compiler_Typed_Semantic_Analyzer typed(&dx);

  const auto * cfg = lower_to_cfg(
      "fn choose(flag) {\n"
      "  let value = 0;\n"
      "  if (flag) {\n"
      "    value = 1;\n"
      "  } else {\n"
      "    value = 2;\n"
      "  }\n"
      "  return value;\n"
      "}\n",
      sm, dx, ast_ctx, hir_ctx, cfg_ctx, typed);

  const auto & function = *cfg->functions.access(0);
  const auto report = validate_cfg_function(function);
  ASSERT_TRUE(report.valid);
  EXPECT_TRUE(report.warnings.is_empty());

  ASSERT_EQ(function.blocks.size(), 5u);
  EXPECT_EQ(function.blocks.access(0).terminator.kind,
            Compiler_CFG_Terminator_Kind::Branch);
  ASSERT_EQ(function.blocks.access(0).terminator.successors.size(), 2u);
  EXPECT_EQ(function.blocks.access(0).terminator.successors.access(0), 2u);
  EXPECT_EQ(function.blocks.access(0).terminator.successors.access(1), 4u);
  EXPECT_EQ(function.blocks.access(2).terminator.kind,
            Compiler_CFG_Terminator_Kind::Jump);
  EXPECT_EQ(function.blocks.access(4).terminator.kind,
            Compiler_CFG_Terminator_Kind::Jump);
  EXPECT_EQ(function.blocks.access(3).terminator.kind,
            Compiler_CFG_Terminator_Kind::Return);
  EXPECT_EQ(function.blocks.access(1).terminator.kind,
            Compiler_CFG_Terminator_Kind::Exit);
}

TEST(CompilerCFG, LowersWhileLoopWithContinue)
{
  Source_Manager sm;
  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ast_ctx(1 << 16);
  Compiler_HIR_Context hir_ctx(1 << 16);
  Compiler_CFG_Context cfg_ctx(1 << 16);
  Compiler_Typed_Semantic_Analyzer typed(&dx);

  const auto * cfg = lower_to_cfg(
      "fn loop(flag) {\n"
      "  while (flag) {\n"
      "    flag = false;\n"
      "    continue;\n"
      "  }\n"
      "}\n",
      sm, dx, ast_ctx, hir_ctx, cfg_ctx, typed);

  const auto & function = *cfg->functions.access(0);
  const auto report = validate_cfg_function(function);
  ASSERT_TRUE(report.valid);
  EXPECT_TRUE(report.warnings.is_empty());

  ASSERT_EQ(function.blocks.size(), 5u);
  EXPECT_EQ(function.blocks.access(0).terminator.kind,
            Compiler_CFG_Terminator_Kind::Jump);
  EXPECT_EQ(function.blocks.access(0).terminator.successors.access(0), 2u);
  EXPECT_EQ(function.blocks.access(2).terminator.kind,
            Compiler_CFG_Terminator_Kind::Branch);
  EXPECT_EQ(function.blocks.access(2).terminator.successors.access(0), 3u);
  EXPECT_EQ(function.blocks.access(2).terminator.successors.access(1), 4u);
  EXPECT_EQ(function.blocks.access(3).terminator.kind,
            Compiler_CFG_Terminator_Kind::Jump);
  EXPECT_EQ(function.blocks.access(3).terminator.successors.access(0), 2u);
  EXPECT_EQ(function.blocks.access(4).terminator.kind,
            Compiler_CFG_Terminator_Kind::Jump);
  EXPECT_EQ(function.blocks.access(4).terminator.successors.access(0), 1u);
}

TEST(CompilerCFG, ValidatorWarnsAboutUnreachableBlocks)
{
  Source_Manager sm;
  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ast_ctx(1 << 16);
  Compiler_HIR_Context hir_ctx(1 << 16);
  Compiler_CFG_Context cfg_ctx(1 << 16);
  Compiler_Typed_Semantic_Analyzer typed(&dx);

  const auto * cfg = lower_to_cfg(
      "fn dead() {\n"
      "  return 1;\n"
      "  let later = 2;\n"
      "}\n",
      sm, dx, ast_ctx, hir_ctx, cfg_ctx, typed);

  const auto report = validate_cfg_module(*cfg);
  ASSERT_TRUE(report.valid);
  ASSERT_EQ(report.warnings.size(), 1u);
  EXPECT_NE(report.warnings.access(0).find("unreachable"), std::string::npos);
}

TEST(CompilerCFG, ValidatorRejectsReturnToNonCanonicalExitBlock)
{
  Compiler_CFG_Function function;
  function.name = "bad_return";
  function.entry_block = 0;
  function.exit_block = 1;

  Compiler_CFG_Block entry;
  entry.id = 0;
  entry.label = "entry";
  entry.terminator.kind = Compiler_CFG_Terminator_Kind::Return;
  entry.terminator.successors.append(2);

  Compiler_CFG_Block exit;
  exit.id = 1;
  exit.label = "exit";
  exit.terminator.kind = Compiler_CFG_Terminator_Kind::Exit;

  Compiler_CFG_Block stray;
  stray.id = 2;
  stray.label = "stray";
  stray.terminator.kind = Compiler_CFG_Terminator_Kind::Jump;
  stray.terminator.successors.append(1);
  stray.predecessors.append(0);

  function.blocks.append(entry);
  function.blocks.append(exit);
  function.blocks.append(stray);

  const auto report = validate_cfg_function(function);
  EXPECT_FALSE(report.valid);
  bool found_canonical_exit_error = false;
  for (size_t i = 0; i < report.errors.size(); ++i)
    if (report.errors.access(i).find("must point to the canonical exit block")
        != std::string::npos)
      found_canonical_exit_error = true;
  EXPECT_TRUE(found_canonical_exit_error);
}
