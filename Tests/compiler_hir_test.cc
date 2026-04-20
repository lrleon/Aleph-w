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
 * @file compiler_hir_test.cc
 * @brief Tests for Compiler_HIR.H.
 */

# include <gtest/gtest.h>

# include <Compiler_HIR_Lowering_MVP.H>
# include <Compiler_Parser.H>

using namespace Aleph;


TEST(CompilerHIR, LowersTypedFunctionAndTopLevelCallDeterministically)
{
  Source_Manager sm;
  const auto file_id = sm.add_virtual_file(
      "main.aw",
      "fn add(x, y) {\n"
      "  let z = x + y;\n"
      "  return z;\n"
      "}\n"
      "let value = add(1, 2);\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ast_ctx(1 << 16);
  Compiler_Parser parser(ast_ctx, sm, file_id, &dx);
  const auto * module = parser.parse_module();
  ASSERT_FALSE(dx.has_errors());

  Compiler_Typed_Semantic_Analyzer typed(&dx);
  typed.analyze_module(module);
  ASSERT_FALSE(dx.has_errors());

  Compiler_HIR_Context hir_ctx(1 << 16);
  Compiler_HIR_Lowering lowering(hir_ctx, typed);
  const auto * hir = lowering.lower_module(module);

  const auto dump = compiler_dump_hir_module(hir, typed.type_context());
  EXPECT_EQ(
      dump,
      "HIRModule\n"
      "  Function(add): fn(Int, Int) -> Int\n"
      "    Params: x: Int, y: Int\n"
      "    Body:\n"
      "      Block\n"
      "        Let(z): Int\n"
      "          Binary(Plus): Int\n"
      "            Variable(x): Int\n"
      "            Variable(y): Int\n"
      "        Return\n"
      "          Variable(z): Int\n"
      "  Let(value): Int\n"
      "    Call: Int\n"
      "      Callee:\n"
      "        Variable(add): fn(Int, Int) -> Int\n"
      "      Args:\n"
      "        Constant(Integer, 1): Int\n"
      "        Constant(Integer, 2): Int\n");
}


TEST(CompilerHIR, RemovesGroupingNodesDuringLowering)
{
  Source_Manager sm;
  const auto file_id = sm.add_virtual_file(
      "main.aw",
      "fn guard(flag) {\n"
      "  if ((flag)) { return 1; }\n"
      "  return 0;\n"
      "}\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ast_ctx(1 << 16);
  Compiler_Parser parser(ast_ctx, sm, file_id, &dx);
  const auto * module = parser.parse_module();
  ASSERT_FALSE(dx.has_errors());

  Compiler_Typed_Semantic_Analyzer typed(&dx);
  typed.analyze_module(module);
  ASSERT_FALSE(dx.has_errors());

  Compiler_HIR_Context hir_ctx(1 << 16);
  Compiler_HIR_Lowering lowering(hir_ctx, typed);
  const auto * hir = lowering.lower_module(module);
  const auto * function = hir->functions.access(0);
  const auto * if_stmt =
      static_cast<const Compiler_HIR_If_Stmt *>(function->body->statements.access(0));

  ASSERT_NE(if_stmt, nullptr);
  ASSERT_NE(if_stmt->condition, nullptr);
  EXPECT_EQ(if_stmt->condition->kind, Compiler_HIR_Expr_Kind::Variable);
  EXPECT_EQ(typed.type_context().to_string(if_stmt->condition->type_id), "Bool");
}


TEST(CompilerHIR, PreservesStructuredControlNodes)
{
  Source_Manager sm;
  const auto file_id = sm.add_virtual_file(
      "main.aw",
      "fn loop(flag) {\n"
      "  while (flag) {\n"
      "    flag = false;\n"
      "    continue;\n"
      "  }\n"
      "}\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ast_ctx(1 << 16);
  Compiler_Parser parser(ast_ctx, sm, file_id, &dx);
  const auto * module = parser.parse_module();
  ASSERT_FALSE(dx.has_errors());

  Compiler_Typed_Semantic_Analyzer typed(&dx);
  typed.analyze_module(module);
  ASSERT_FALSE(dx.has_errors());

  Compiler_HIR_Context hir_ctx(1 << 16);
  Compiler_HIR_Lowering lowering(hir_ctx, typed);
  const auto * hir = lowering.lower_module(module);
  const auto * function = hir->functions.access(0);
  ASSERT_EQ(typed.type_context().to_string(function->type_id), "fn(Bool) -> Unit");

  const auto * while_stmt =
      static_cast<const Compiler_HIR_While_Stmt *>(function->body->statements.access(0));
  ASSERT_NE(while_stmt, nullptr);
  ASSERT_NE(while_stmt->body, nullptr);
  EXPECT_EQ(while_stmt->kind, Compiler_HIR_Stmt_Kind::While);
  EXPECT_EQ(while_stmt->condition->kind, Compiler_HIR_Expr_Kind::Variable);

  const auto * body =
      static_cast<const Compiler_HIR_Block_Stmt *>(while_stmt->body);
  ASSERT_EQ(body->statements.size(), 2u);
  EXPECT_EQ(body->statements.access(0)->kind, Compiler_HIR_Stmt_Kind::Eval);
  EXPECT_EQ(body->statements.access(1)->kind, Compiler_HIR_Stmt_Kind::Continue);
}
