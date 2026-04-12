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
 * @file compiler_constraints_test.cc
 * @brief Tests for tpl_constraints.H.
 */

# include <gtest/gtest.h>

# include <tpl_constraints.H>

using namespace Aleph;


TEST(CompilerConstraints, UnifiesTypeVariableWithBuiltinAndAppliesSubstitution)
{
  Compiler_Type_Context ctx;
  const auto alpha = ctx.make_type_variable("A");

  Compiler_Type_Unifier unifier(ctx);
  const auto result = unifier.unify(alpha, ctx.integer_type());

  ASSERT_TRUE(result.ok());
  EXPECT_EQ(unifier.apply(alpha), ctx.integer_type());
  EXPECT_NE(unifier.dump_substitution().find("A := Int"), std::string::npos);
}


TEST(CompilerConstraints, UnifiesStructuredFunctionTypesTransitively)
{
  Compiler_Type_Context ctx;
  const auto alpha = ctx.make_type_variable("A");
  const auto beta = ctx.make_type_variable("B");

  const auto lhs = ctx.make_function_type({alpha, ctx.bool_type()}, alpha);
  const auto rhs = ctx.make_function_type({ctx.integer_type(), beta}, ctx.integer_type());

  Compiler_Type_Unifier unifier(ctx);
  const auto result = unifier.unify(lhs, rhs);

  ASSERT_TRUE(result.ok());
  EXPECT_EQ(unifier.apply(alpha), ctx.integer_type());
  EXPECT_EQ(unifier.apply(beta), ctx.bool_type());
  EXPECT_EQ(ctx.to_string(unifier.apply(lhs)), "fn(Int, Bool) -> Int");
}


TEST(CompilerConstraints, OccursCheckRejectsRecursiveBinding)
{
  Compiler_Type_Context ctx;
  const auto alpha = ctx.make_type_variable("A");
  const auto recursive = ctx.make_function_type({ctx.integer_type()}, alpha);

  Compiler_Type_Unifier unifier(ctx);
  const auto result = unifier.unify(alpha, recursive);

  EXPECT_EQ(result.kind, Compiler_Unify_Result_Kind::Occurs_Check_Failed);
}


TEST(CompilerConstraints, RigidVariablesCannotBeBound)
{
  Compiler_Type_Context ctx;
  const auto rigid = ctx.make_type_variable("R", true);

  Compiler_Type_Unifier unifier(ctx);
  const auto result = unifier.unify(rigid, ctx.integer_type());

  EXPECT_EQ(result.kind, Compiler_Unify_Result_Kind::Rigid_Variable);
}


TEST(CompilerConstraints, SolvesConstraintSetsInInsertionOrder)
{
  Compiler_Type_Context ctx;
  const auto alpha = ctx.make_type_variable("A");
  const auto beta = ctx.make_type_variable("B");

  Compiler_Type_Constraint_Set constraints;
  constraints.add(alpha, ctx.integer_type(), {}, "parameter");
  constraints.add(beta, ctx.string_type(), {}, "result");
  constraints.add(ctx.make_tuple_type({alpha, beta}),
                  ctx.make_tuple_type({ctx.integer_type(), ctx.string_type()}),
                  {},
                  "tuple check");

  Compiler_Type_Unifier unifier(ctx);
  const auto result = unifier.solve(constraints);

  ASSERT_TRUE(result.ok());
  EXPECT_EQ(unifier.apply(alpha), ctx.integer_type());
  EXPECT_EQ(unifier.apply(beta), ctx.string_type());
}
