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
 * @file compiler_sema_test.cc
 * @brief Tests for Compiler_Sema.H.
 */

# include <gtest/gtest.h>

# include <Compiler_Parser.H>
# include <Compiler_Sema.H>

# include <string>
# include <vector>

using namespace Aleph;


namespace
{
  std::vector<std::string>
  collect_codes(const Diagnostic_Engine & dx)
  {
    std::vector<std::string> codes;
    for (size_t i = 0; i < dx.size(); ++i)
      codes.push_back(dx.get(i).code);
    return codes;
  }

  bool
  contains_code(const std::vector<std::string> & codes,
                const std::string & code)
  {
    for (const auto & item : codes)
      if (item == code)
        return true;
    return false;
  }
}


TEST(CompilerSema, ResolvesFunctionsParametersLocalsAndGlobals)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file(
      "main.aw",
      "fn add(x, y) {\n"
      "  let z = x + y;\n"
      "  return z;\n"
      "}\n"
      "let value = add(1, 2);\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 16);
  Compiler_Parser parser(ctx, sm, id, &dx);
  const auto * module = parser.parse_module();
  ASSERT_FALSE(dx.has_errors());

  Compiler_Semantic_Analyzer sema(&dx);
  sema.analyze_module(module);

  EXPECT_FALSE(dx.has_errors());
  EXPECT_EQ(sema.symbol_count(), 5u);
  EXPECT_EQ(sema.resolution_count(), 4u);

  const auto * fn = module->functions.access(0);
  const auto * let_stmt =
      static_cast<const Compiler_Let_Stmt *>(fn->body->statements.access(0));
  const auto * sum =
      static_cast<const Compiler_Binary_Expr *>(let_stmt->initializer);
  const auto * x_ref =
      static_cast<const Compiler_Identifier_Expr *>(sum->left);
  const auto * y_ref =
      static_cast<const Compiler_Identifier_Expr *>(sum->right);
  const auto * ret_stmt =
      static_cast<const Compiler_Return_Stmt *>(fn->body->statements.access(1));
  const auto * z_ref =
      static_cast<const Compiler_Identifier_Expr *>(ret_stmt->value);
  const auto * top_let =
      static_cast<const Compiler_Let_Stmt *>(module->statements.access(0));
  const auto * call =
      static_cast<const Compiler_Call_Expr *>(top_let->initializer);
  const auto * add_ref =
      static_cast<const Compiler_Identifier_Expr *>(call->callee);

  ASSERT_NE(sema.resolved_symbol(x_ref), nullptr);
  EXPECT_EQ(sema.resolved_symbol(x_ref)->kind, Compiler_Symbol_Kind::Parameter);
  EXPECT_EQ(sema.resolved_symbol(x_ref)->name, "x");

  ASSERT_NE(sema.resolved_symbol(y_ref), nullptr);
  EXPECT_EQ(sema.resolved_symbol(y_ref)->kind, Compiler_Symbol_Kind::Parameter);
  EXPECT_EQ(sema.resolved_symbol(y_ref)->name, "y");

  ASSERT_NE(sema.resolved_symbol(z_ref), nullptr);
  EXPECT_EQ(sema.resolved_symbol(z_ref)->kind, Compiler_Symbol_Kind::Local);
  EXPECT_EQ(sema.resolved_symbol(z_ref)->name, "z");

  ASSERT_NE(sema.resolved_symbol(add_ref), nullptr);
  EXPECT_EQ(sema.resolved_symbol(add_ref)->kind, Compiler_Symbol_Kind::Function);
  EXPECT_EQ(sema.resolved_symbol(add_ref)->name, "add");
}


TEST(CompilerSema, ReportsDuplicateDeclarationsAndShadowing)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file(
      "main.aw",
      "fn dup(x, x) { let y = 1; let y = 2; }\n"
      "fn dup() {}\n"
      "fn shadow(a) { { let a = 1; } }\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 16);
  Compiler_Parser parser(ctx, sm, id, &dx);
  const auto * module = parser.parse_module();
  ASSERT_FALSE(dx.has_errors());

  Compiler_Semantic_Options opts;
  opts.allow_shadowing = false;
  Compiler_Semantic_Analyzer sema(&dx, opts);
  sema.analyze_module(module);

  ASSERT_TRUE(dx.has_errors());
  const auto codes = collect_codes(dx);
  EXPECT_TRUE(contains_code(codes, "SEM001"));
  EXPECT_TRUE(contains_code(codes, "SEM007"));
}


TEST(CompilerSema, ReportsUnresolvedNamesAndControlFlowMisuse)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file(
      "main.aw",
      "return missing;\n"
      "break;\n"
      "continue;\n"
      "let x = 1;\n"
      "x();\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 16);
  Compiler_Parser parser(ctx, sm, id, &dx);
  const auto * module = parser.parse_module();
  ASSERT_FALSE(dx.has_errors());

  Compiler_Semantic_Analyzer sema(&dx);
  sema.analyze_module(module);

  ASSERT_TRUE(dx.has_errors());
  const auto codes = collect_codes(dx);
  EXPECT_TRUE(contains_code(codes, "SEM002"));
  EXPECT_TRUE(contains_code(codes, "SEM003"));
  EXPECT_TRUE(contains_code(codes, "SEM004"));
  EXPECT_TRUE(contains_code(codes, "SEM005"));
  EXPECT_TRUE(contains_code(codes, "SEM006"));
}
