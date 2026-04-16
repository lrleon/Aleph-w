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
 * @file compiler_typed_sema_test.cc
 * @brief Tests for Compiler_Typed_Sema.H.
 */

# include <gtest/gtest.h>

# include <Compiler_Parser.H>
# include <Compiler_Typed_Sema.H>

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


TEST(CompilerTypedSema, InfersArithmeticFunctionAndCallTypes)
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

  Compiler_Typed_Semantic_Analyzer typed(&dx);
  typed.analyze_module(module);

  EXPECT_FALSE(dx.has_errors());

  const auto * fn = module->functions.access(0);
  EXPECT_EQ(typed.type_context().to_string(typed.function_type(fn)),
            "fn(Int, Int) -> Int");
  EXPECT_EQ(typed.type_context().to_string(typed.parameter_type(fn, 0)), "Int");
  EXPECT_EQ(typed.type_context().to_string(typed.parameter_type(fn, 1)), "Int");

  const auto * local =
      static_cast<const Compiler_Let_Stmt *>(fn->body->statements.access(0));
  EXPECT_EQ(typed.type_context().to_string(typed.let_type(local)), "Int");

  const auto * top =
      static_cast<const Compiler_Let_Stmt *>(module->statements.access(0));
  EXPECT_EQ(typed.type_context().to_string(typed.let_type(top)), "Int");

  const auto * call =
      static_cast<const Compiler_Call_Expr *>(top->initializer);
  EXPECT_EQ(typed.type_context().to_string(typed.inferred_type(call)), "Int");
}


TEST(CompilerTypedSema, InfersBoolConditionsAndUnitForFunctionsWithoutReturn)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file(
      "main.aw",
      "fn guard(flag) {\n"
      "  if (flag) { let x = 1; }\n"
      "}\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 16);
  Compiler_Parser parser(ctx, sm, id, &dx);
  const auto * module = parser.parse_module();
  ASSERT_FALSE(dx.has_errors());

  Compiler_Typed_Semantic_Analyzer typed(&dx);
  typed.analyze_module(module);

  EXPECT_FALSE(dx.has_errors());

  const auto * fn = module->functions.access(0);
  EXPECT_EQ(typed.type_context().to_string(typed.function_type(fn)),
            "fn(Bool) -> Unit");

  const auto * if_stmt =
      static_cast<const Compiler_If_Stmt *>(fn->body->statements.access(0));
  EXPECT_EQ(typed.type_context().to_string(typed.inferred_type(if_stmt->condition)),
            "Bool");
}


TEST(CompilerTypedSema, ResolvesExplicitAnnotationsIntoConcreteTypes)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file(
      "main.aw",
      "fn add(x: Int, y: Int) -> Int {\n"
      "  let z: Int = x + y;\n"
      "  return z;\n"
      "}\n"
      "let value: Int = add(1, 2);\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 16);
  Compiler_Parser parser(ctx, sm, id, &dx);
  const auto * module = parser.parse_module();
  ASSERT_FALSE(dx.has_errors());

  Compiler_Typed_Semantic_Analyzer typed(&dx);
  typed.analyze_module(module);

  EXPECT_FALSE(dx.has_errors());

  const auto * fn = module->functions.access(0);
  EXPECT_EQ(typed.type_context().to_string(typed.function_type(fn)),
            "fn(Int, Int) -> Int");
  EXPECT_EQ(typed.type_context().to_string(typed.parameter_type(fn, 0)), "Int");
  EXPECT_EQ(typed.type_context().to_string(typed.parameter_type(fn, 1)), "Int");

  const auto * local =
      static_cast<const Compiler_Let_Stmt *>(fn->body->statements.access(0));
  EXPECT_EQ(typed.type_context().to_string(typed.let_type(local)), "Int");

  const auto * top =
      static_cast<const Compiler_Let_Stmt *>(module->statements.access(0));
  EXPECT_EQ(typed.type_context().to_string(typed.let_type(top)), "Int");
}


TEST(CompilerTypedSema, ResolvesNominalTypesAndTransparentAliases)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file(
      "main.aw",
      "struct Point {\n"
      "  x: Int;\n"
      "  y: Int;\n"
      "}\n"
      "enum Color { Red, Green, Blue }\n"
      "type UserId = Int;\n"
      "fn paint(point: Point, color: Color, id: UserId) -> UserId {\n"
      "  return id;\n"
      "}\n"
      "let favorite: Color;\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 16);
  Compiler_Parser parser(ctx, sm, id, &dx);
  const auto * module = parser.parse_module();
  ASSERT_FALSE(dx.has_errors());

  Compiler_Typed_Semantic_Analyzer typed(&dx);
  typed.analyze_module(module);

  EXPECT_FALSE(dx.has_errors());

  const auto point = typed.declared_type("Point");
  const auto color = typed.declared_type("Color");
  const auto user_id = typed.declared_type("UserId");
  ASSERT_NE(point, 0u);
  ASSERT_NE(color, 0u);
  ASSERT_NE(user_id, 0u);
  EXPECT_TRUE(typed.type_context().is_struct(point));
  EXPECT_TRUE(typed.type_context().is_enum(color));
  EXPECT_EQ(user_id, typed.type_context().integer_type());
  EXPECT_EQ(typed.type_context().type(point).member_names.size(), 2u);
  EXPECT_EQ(typed.type_context().type(point).member_names.access(0), "x");
  EXPECT_EQ(typed.type_context().type(color).member_names.size(), 3u);
  EXPECT_EQ(typed.type_context().type(color).member_names.access(1), "Green");

  const auto * fn = module->functions.access(0);
  EXPECT_EQ(typed.type_context().to_string(typed.function_type(fn)),
            "fn(Point, Color, Int) -> Int");
  EXPECT_EQ(typed.type_context().to_string(typed.parameter_type(fn, 0)), "Point");
  EXPECT_EQ(typed.type_context().to_string(typed.parameter_type(fn, 1)), "Color");
  EXPECT_EQ(typed.type_context().to_string(typed.parameter_type(fn, 2)), "Int");

  const auto * favorite =
      static_cast<const Compiler_Let_Stmt *>(module->statements.access(0));
  EXPECT_EQ(typed.type_context().to_string(typed.let_type(favorite)), "Color");
}


TEST(CompilerTypedSema, InstantiatesExplicitlyGenericFunctionsAndGeneralizedLets)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file(
      "main.aw",
      "fn id(x: T) -> T {\n"
      "  return x;\n"
      "}\n"
      "let alias = id;\n"
      "let number = alias(1);\n"
      "let flag = alias(true);\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 16);
  Compiler_Parser parser(ctx, sm, id, &dx);
  const auto * module = parser.parse_module();
  ASSERT_FALSE(dx.has_errors());

  Compiler_Typed_Semantic_Analyzer typed(&dx);
  typed.analyze_module(module);

  EXPECT_FALSE(dx.has_errors());

  const auto * fn = module->functions.access(0);
  EXPECT_EQ(typed.type_context().to_string(typed.function_type(fn)),
            "fn(T!) -> T!");
  EXPECT_EQ(typed.type_context().to_string(typed.parameter_type(fn, 0)), "T!");

  const auto * alias =
      static_cast<const Compiler_Let_Stmt *>(module->statements.access(0));
  const auto alias_type = typed.type_context().to_string(typed.let_type(alias));
  EXPECT_NE(alias_type.find("fn("), std::string::npos);
  EXPECT_NE(alias_type.find(") -> "), std::string::npos);

  const auto * number =
      static_cast<const Compiler_Let_Stmt *>(module->statements.access(1));
  const auto * flag =
      static_cast<const Compiler_Let_Stmt *>(module->statements.access(2));
  EXPECT_EQ(typed.type_context().to_string(typed.let_type(number)), "Int");
  EXPECT_EQ(typed.type_context().to_string(typed.let_type(flag)), "Bool");
}


TEST(CompilerTypedSema, ReportsInvalidNominalTypeDeclarations)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file(
      "main.aw",
      "struct Bad {\n"
      "  next: Missing;\n"
      "  next: Int;\n"
      "}\n"
      "enum Color { Red, Red }\n"
      "type A = B;\n"
      "type B = A;\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 16);
  Compiler_Parser parser(ctx, sm, id, &dx);
  const auto * module = parser.parse_module();
  ASSERT_FALSE(dx.has_errors());

  Compiler_Typed_Semantic_Analyzer typed(&dx);
  typed.analyze_module(module);

  ASSERT_TRUE(dx.has_errors());
  const auto codes = collect_codes(dx);
  EXPECT_TRUE(contains_code(codes, "TYP009"));
  EXPECT_TRUE(contains_code(codes, "TYP010"));
  EXPECT_TRUE(contains_code(codes, "TYP011"));
}


TEST(CompilerTypedSema, ReportsTypeErrorsForConditionAndCall)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file(
      "main.aw",
      "let x = 1;\n"
      "if (x) { }\n"
      "x(2);\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 16);
  Compiler_Parser parser(ctx, sm, id, &dx);
  const auto * module = parser.parse_module();
  ASSERT_FALSE(dx.has_errors());

  Compiler_Typed_Semantic_Analyzer typed(&dx);
  typed.analyze_module(module);

  ASSERT_TRUE(dx.has_errors());
  const auto codes = collect_codes(dx);
  EXPECT_TRUE(contains_code(codes, "TYP002"));
  EXPECT_TRUE(contains_code(codes, "TYP005"));
}


TEST(CompilerTypedSema, ReportsAnnotatedBindingMismatch)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "let value: Bool = 1;\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 15);
  Compiler_Parser parser(ctx, sm, id, &dx);
  const auto * module = parser.parse_module();
  ASSERT_FALSE(dx.has_errors());

  Compiler_Typed_Semantic_Analyzer typed(&dx);
  typed.analyze_module(module);

  ASSERT_TRUE(dx.has_errors());
  const auto codes = collect_codes(dx);
  EXPECT_TRUE(contains_code(codes, "TYP007"));
}


TEST(CompilerTypedSema, ReportsReturnTypeMismatchAcrossBranches)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file(
      "main.aw",
      "fn bad(flag) {\n"
      "  if (flag) { return 1; }\n"
      "  return false;\n"
      "}\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 16);
  Compiler_Parser parser(ctx, sm, id, &dx);
  const auto * module = parser.parse_module();
  ASSERT_FALSE(dx.has_errors());

  Compiler_Typed_Semantic_Analyzer typed(&dx);
  typed.analyze_module(module);

  ASSERT_TRUE(dx.has_errors());
  const auto codes = collect_codes(dx);
  EXPECT_TRUE(contains_code(codes, "TYP006"));
}
