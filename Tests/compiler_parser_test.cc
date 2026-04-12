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
 * @file compiler_parser_test.cc
 * @brief Tests for Compiler_Parser.H.
 */

# include <gtest/gtest.h>

# include <Compiler_Parser.H>

using namespace Aleph;


TEST(CompilerParser, ParsesFunctionBodiesCallsAndBinaryExpressions)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file(
      "main.aw",
      "fn add(x, y) {\n"
      "  let z = add(x, y);\n"
      "  return z + 1;\n"
      "}\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 16);
  Compiler_Parser parser(ctx, sm, id, &dx);

  auto * module = parser.parse_module();

  ASSERT_NE(module, nullptr);
  ASSERT_EQ(module->functions.size(), 1u);
  EXPECT_EQ(module->statements.size(), 0u);
  EXPECT_FALSE(dx.has_errors());

  const auto * fn = module->functions.access(0);
  ASSERT_NE(fn, nullptr);
  EXPECT_EQ(fn->name, "add");
  ASSERT_EQ(fn->parameters.size(), 2u);
  EXPECT_EQ(fn->parameters.access(0).name, "x");
  EXPECT_EQ(fn->parameters.access(1).name, "y");

  ASSERT_NE(fn->body, nullptr);
  ASSERT_EQ(fn->body->statements.size(), 2u);

  const auto * let_stmt =
      static_cast<const Compiler_Let_Stmt *>(fn->body->statements.access(0));
  ASSERT_EQ(let_stmt->kind, Compiler_Stmt_Kind::Let);
  EXPECT_EQ(let_stmt->name, "z");
  ASSERT_NE(let_stmt->initializer, nullptr);
  ASSERT_EQ(let_stmt->initializer->kind, Compiler_Expr_Kind::Call);
  const auto * call_expr =
      static_cast<const Compiler_Call_Expr *>(let_stmt->initializer);
  ASSERT_EQ(call_expr->arguments.size(), 2u);

  const auto * ret_stmt =
      static_cast<const Compiler_Return_Stmt *>(fn->body->statements.access(1));
  ASSERT_EQ(ret_stmt->kind, Compiler_Stmt_Kind::Return);
  ASSERT_NE(ret_stmt->value, nullptr);
  ASSERT_EQ(ret_stmt->value->kind, Compiler_Expr_Kind::Binary);
  const auto * ret_expr =
      static_cast<const Compiler_Binary_Expr *>(ret_stmt->value);
  EXPECT_EQ(ret_expr->op, Compiler_Token_Kind::Plus);

  const auto dump = compiler_dump_module(module);
  EXPECT_NE(dump.find("Function(add)"), std::string::npos);
  EXPECT_NE(dump.find("Call"), std::string::npos);
  EXPECT_NE(dump.find("Binary(Plus)"), std::string::npos);
}


TEST(CompilerParser, ParsesTopLevelStatementsAndControlFlow)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file(
      "main.aw",
      "let x = 3;\n"
      "while x {\n"
      "  x = x - 1;\n"
      "}\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 16);
  Compiler_Parser parser(ctx, sm, id, &dx);

  auto * module = parser.parse_module();

  ASSERT_NE(module, nullptr);
  ASSERT_EQ(module->functions.size(), 0u);
  ASSERT_EQ(module->statements.size(), 2u);
  EXPECT_FALSE(dx.has_errors());

  const auto * let_stmt =
      static_cast<const Compiler_Let_Stmt *>(module->statements.access(0));
  ASSERT_EQ(let_stmt->kind, Compiler_Stmt_Kind::Let);
  EXPECT_EQ(let_stmt->name, "x");

  const auto * while_stmt =
      static_cast<const Compiler_While_Stmt *>(module->statements.access(1));
  ASSERT_EQ(while_stmt->kind, Compiler_Stmt_Kind::While);
  ASSERT_NE(while_stmt->condition, nullptr);
  EXPECT_EQ(while_stmt->condition->kind, Compiler_Expr_Kind::Identifier);
  ASSERT_NE(while_stmt->body, nullptr);
  ASSERT_EQ(while_stmt->body->kind, Compiler_Stmt_Kind::Block);
}


TEST(CompilerParser, RespectsTopLevelRestrictionOption)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "let x = 1;\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 15);
  Compiler_Parser_Options opts;
  opts.allow_top_level_statements = false;
  Compiler_Parser parser(ctx, sm, id, &dx, opts);

  auto * module = parser.parse_module();

  ASSERT_NE(module, nullptr);
  ASSERT_EQ(module->statements.size(), 1u);
  EXPECT_TRUE(dx.has_errors());
  ASSERT_GE(dx.size(), 1u);
  EXPECT_EQ(dx.get(0).code, "PAR004");
}


TEST(CompilerParser, ProducesDiagnosticsAndInvalidNodesForMalformedInput)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "let x = ;\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 15);
  Compiler_Parser parser(ctx, sm, id, &dx);

  auto * module = parser.parse_module();

  ASSERT_NE(module, nullptr);
  ASSERT_EQ(module->statements.size(), 1u);
  ASSERT_TRUE(dx.has_errors());
  ASSERT_GE(dx.size(), 1u);

  const auto * let_stmt =
      static_cast<const Compiler_Let_Stmt *>(module->statements.access(0));
  ASSERT_NE(let_stmt->initializer, nullptr);
  EXPECT_EQ(let_stmt->initializer->kind, Compiler_Expr_Kind::Invalid);
}
