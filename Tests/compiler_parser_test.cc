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

  ASSERT_EQ(fn->body->statements.access(0)->kind, Compiler_Stmt_Kind::Let);
  const auto * let_stmt =
      static_cast<const Compiler_Let_Stmt *>(fn->body->statements.access(0));
  EXPECT_EQ(let_stmt->name, "z");
  ASSERT_NE(let_stmt->initializer, nullptr);
  ASSERT_EQ(let_stmt->initializer->kind, Compiler_Expr_Kind::Call);
  const auto * call_expr =
      static_cast<const Compiler_Call_Expr *>(let_stmt->initializer);
  ASSERT_EQ(call_expr->arguments.size(), 2u);

  ASSERT_EQ(fn->body->statements.access(1)->kind, Compiler_Stmt_Kind::Return);
  const auto * ret_stmt =
      static_cast<const Compiler_Return_Stmt *>(fn->body->statements.access(1));
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

  ASSERT_EQ(module->statements.access(0)->kind, Compiler_Stmt_Kind::Let);
  const auto * let_stmt =
      static_cast<const Compiler_Let_Stmt *>(module->statements.access(0));
  EXPECT_EQ(let_stmt->name, "x");

  ASSERT_EQ(module->statements.access(1)->kind, Compiler_Stmt_Kind::While);
  const auto * while_stmt =
      static_cast<const Compiler_While_Stmt *>(module->statements.access(1));
  ASSERT_NE(while_stmt->condition, nullptr);
  EXPECT_EQ(while_stmt->condition->kind, Compiler_Expr_Kind::Identifier);
  ASSERT_NE(while_stmt->body, nullptr);
  ASSERT_EQ(while_stmt->body->kind, Compiler_Stmt_Kind::Block);
}


TEST(CompilerParser, ParsesTypeAnnotationsInFunctionsAndLets)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file(
      "main.aw",
      "fn id(x: T) -> T {\n"
      "  let alias: fn(T) -> T = id;\n"
      "  return alias(x);\n"
      "}\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 15);
  Compiler_Parser parser(ctx, sm, id, &dx);

  auto * module = parser.parse_module();

  ASSERT_NE(module, nullptr);
  ASSERT_EQ(module->functions.size(), 1u);
  ASSERT_FALSE(dx.has_errors());

  const auto * fn = module->functions.access(0);
  ASSERT_EQ(fn->parameters.size(), 1u);
  ASSERT_NE(fn->parameters.access(0).annotation, nullptr);
  EXPECT_EQ(fn->parameters.access(0).annotation->kind, Compiler_Type_Expr_Kind::Named);
  ASSERT_NE(fn->return_annotation, nullptr);
  EXPECT_EQ(fn->return_annotation->kind, Compiler_Type_Expr_Kind::Named);

  ASSERT_NE(fn->body, nullptr);
  ASSERT_EQ(fn->body->statements.size(), 2u);
  const auto * let_stmt = static_cast<const Compiler_Let_Stmt *>(fn->body->statements.access(0));
  ASSERT_NE(let_stmt->annotation, nullptr);
  ASSERT_EQ(let_stmt->annotation->kind, Compiler_Type_Expr_Kind::Function);

  const auto * fn_type = static_cast<const Compiler_Function_Type_Expr *>(let_stmt->annotation);
  ASSERT_EQ(fn_type->parameters.size(), 1u);
  ASSERT_NE(fn_type->result, nullptr);
  EXPECT_EQ(fn_type->parameters.access(0)->kind, Compiler_Type_Expr_Kind::Named);
  EXPECT_EQ(fn_type->result->kind, Compiler_Type_Expr_Kind::Named);

  const auto dump = compiler_dump_function(fn);
  EXPECT_NE(dump.find("Params: x: T"), std::string::npos);
  EXPECT_NE(dump.find("Return: T"), std::string::npos);
  EXPECT_NE(dump.find("Let(alias): fn(T) -> T"), std::string::npos);
}


TEST(CompilerParser, ParsesNominalTypeDeclarations)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file(
      "types.aw",
      "struct Point {\n"
      "  x: Int;\n"
      "  y: Int;\n"
      "}\n"
      "enum Color { Red, Green, Blue, }\n"
      "type UserId = Int;\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 15);
  Compiler_Parser parser(ctx, sm, id, &dx);

  auto * module = parser.parse_module();

  ASSERT_NE(module, nullptr);
  ASSERT_EQ(module->type_declarations.size(), 3u);
  ASSERT_FALSE(dx.has_errors());

  const auto * point = static_cast<const Compiler_Struct_Decl *>(module->type_declarations.access(0));
  ASSERT_EQ(point->kind, Compiler_Type_Decl_Kind::Struct);
  ASSERT_EQ(point->fields.size(), 2u);
  EXPECT_EQ(point->fields.access(0).name, "x");
  EXPECT_EQ(point->fields.access(1).name, "y");

  const auto * color = static_cast<const Compiler_Enum_Decl *>(module->type_declarations.access(1));
  ASSERT_EQ(color->kind, Compiler_Type_Decl_Kind::Enum);
  ASSERT_EQ(color->variants.size(), 3u);
  EXPECT_EQ(color->variants.access(0).name, "Red");
  EXPECT_EQ(color->variants.access(2).name, "Blue");

  const auto * user_id
      = static_cast<const Compiler_Type_Alias_Decl *>(module->type_declarations.access(2));
  ASSERT_EQ(user_id->kind, Compiler_Type_Decl_Kind::Alias);
  ASSERT_NE(user_id->aliased_type, nullptr);
  EXPECT_EQ(user_id->aliased_type->kind, Compiler_Type_Expr_Kind::Named);

  const auto dump = compiler_dump_module(module);
  EXPECT_NE(dump.find("Struct(Point)"), std::string::npos);
  EXPECT_NE(dump.find("Enum(Color)"), std::string::npos);
  EXPECT_NE(dump.find("TypeAlias(UserId) = Int"), std::string::npos);
}


TEST(CompilerParser, ParsesTopLevelImportDeclarations)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file(
      "main.aw",
      "import \"math.aw\";\n"
      "import \"types.aw\";\n"
      "fn use() { return value; }\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 15);
  Compiler_Parser parser(ctx, sm, id, &dx);

  auto * module = parser.parse_module();

  ASSERT_NE(module, nullptr);
  ASSERT_FALSE(dx.has_errors());
  ASSERT_EQ(module->imports.size(), 2u);
  EXPECT_EQ(module->source_name, "main.aw");
  EXPECT_EQ(module->imports.access(0)->module_name, "math.aw");
  EXPECT_EQ(module->imports.access(1)->module_name, "types.aw");

  const auto dump = compiler_dump_module(module);
  EXPECT_NE(dump.find("Import(\"math.aw\")"), std::string::npos);
  EXPECT_NE(dump.find("Import(\"types.aw\")"), std::string::npos);
  EXPECT_NE(dump.find("Function(use)"), std::string::npos);
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

  ASSERT_EQ(module->statements.access(0)->kind, Compiler_Stmt_Kind::Let);
  const auto * let_stmt =
      static_cast<const Compiler_Let_Stmt *>(module->statements.access(0));
  ASSERT_NE(let_stmt->initializer, nullptr);
  EXPECT_EQ(let_stmt->initializer->kind, Compiler_Expr_Kind::Invalid);
}


TEST(CompilerParser, ParsesEmptyInput)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("empty.aw", "");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 15);
  Compiler_Parser parser(ctx, sm, id, &dx);

  auto * module = parser.parse_module();

  ASSERT_NE(module, nullptr);
  EXPECT_EQ(module->functions.size(), 0u);
  EXPECT_EQ(module->statements.size(), 0u);
  EXPECT_FALSE(dx.has_errors());
}
