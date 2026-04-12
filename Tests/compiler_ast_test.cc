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
 * @file compiler_ast_test.cc
 * @brief Tests for Compiler_AST.H.
 */

# include <gtest/gtest.h>

# include <Compiler_AST.H>

using namespace Aleph;


namespace
{
  struct Tracked_Object
  {
    static int destroyed;
    std::string payload;

    explicit Tracked_Object(std::string value = "")
      : payload(std::move(value))
    {}

    ~Tracked_Object()
    {
      ++destroyed;
    }
  };

  int Tracked_Object::destroyed = 0;
}


TEST(CompilerAst, ContextResetRunsTrackedDestructors)
{
  Tracked_Object::destroyed = 0;

  Compiler_Ast_Context ctx(4096);
  auto * first = ctx.make<Tracked_Object>("a");
  auto * second = ctx.make<Tracked_Object>("b");

  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  EXPECT_EQ(Tracked_Object::destroyed, 0);

  ctx.reset();

  EXPECT_EQ(Tracked_Object::destroyed, 2);
  EXPECT_EQ(ctx.allocated_size(), 0u);
}


TEST(CompilerAst, DumpHelpersRenderDeterministicTreeShape)
{
  Compiler_Ast_Context ctx(16384);

  auto * lhs = ctx.make<Compiler_Identifier_Expr>("x", Source_Span{1, 0, 1});
  auto * rhs = ctx.make<Compiler_Literal_Expr>(Compiler_Expr_Kind::Integer_Literal,
                                               "1",
                                               Source_Span{1, 4, 5});
  auto * sum = ctx.make<Compiler_Binary_Expr>(lhs,
                                              Compiler_Token_Kind::Plus,
                                              Source_Span{1, 2, 3},
                                              rhs);

  auto * ret = ctx.make<Compiler_Return_Stmt>(Source_Span{1, 6, 12},
                                              sum,
                                              Source_Span{1, 13, 14});
  auto * body = ctx.make<Compiler_Block_Stmt>(Source_Span{1, 15, 16},
                                              Source_Span{1, 17, 18});
  body->statements.append(ret);

  auto * fn = ctx.make<Compiler_Function_Decl>(Source_Span{1, 19, 21},
                                               "inc",
                                               Source_Span{1, 22, 25},
                                               body);
  fn->parameters.append({"x", Source_Span{1, 26, 27}});

  auto * module = ctx.make<Compiler_Module>();
  module->functions.append(fn);

  const auto expr_dump = compiler_dump_expr(sum);
  const auto module_dump = compiler_dump_module(module);

  EXPECT_NE(expr_dump.find("Binary(Plus)"), std::string::npos);
  EXPECT_NE(expr_dump.find("Identifier(x)"), std::string::npos);
  EXPECT_NE(expr_dump.find("Integer(1)"), std::string::npos);

  EXPECT_NE(module_dump.find("Module"), std::string::npos);
  EXPECT_NE(module_dump.find("Function(inc)"), std::string::npos);
  EXPECT_NE(module_dump.find("Params: x"), std::string::npos);
  EXPECT_NE(module_dump.find("Return"), std::string::npos);
}
