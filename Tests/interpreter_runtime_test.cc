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
 * @file interpreter_runtime_test.cc
 * @brief Tests for Interpreter_Runtime.H.
 */

#include <gtest/gtest.h>

#include <Compiler_Parser.H>
#include <Interpreter_Runtime.H>

using namespace Aleph;

namespace
{
  const Compiler_HIR_Module *
  lower_to_hir(const std::string & source,
               Source_Manager & sm,
               Diagnostic_Engine & dx,
               Compiler_Ast_Context & ast_ctx,
               Compiler_HIR_Context & hir_ctx,
               Compiler_Typed_Semantic_Analyzer & typed)
  {
    const auto file_id = sm.add_virtual_file("main.aw", source);
    Compiler_Parser parser(ast_ctx, sm, file_id, &dx);
    const auto * module = parser.parse_module();
    EXPECT_FALSE(dx.has_errors());

    typed.analyze_module(module);
    EXPECT_FALSE(dx.has_errors());

    Compiler_HIR_Lowering lowering(hir_ctx, typed);
    return lowering.lower_module(module);
  }

  bool
  host_pair(void *,
            const DynArray<Interpreter_Value> & arguments,
            Interpreter_Value & result,
            Interpreter_Runtime_Error & error)
  {
    if (arguments.size() != 2)
      {
        error.code = "HOST001";
        error.message = "pair expects exactly 2 arguments";
        return false;
      }

    DynArray<Interpreter_Value> tuple;
    tuple.append(arguments.access(0));
    tuple.append(arguments.access(1));
    result = Interpreter_Value::make_tuple(std::move(tuple));
    return true;
  }
}

TEST(InterpreterRuntime, EvaluatesTopLevelBindingsAndCallsFunctions)
{
  Source_Manager sm;
  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ast_ctx(1 << 16);
  Compiler_HIR_Context hir_ctx(1 << 16);
  Compiler_Typed_Semantic_Analyzer typed(&dx);

  const auto * hir = lower_to_hir(
      "fn add(x, y) {\n"
      "  return x + y;\n"
      "}\n"
      "let value = add(20, 22);\n",
      sm, dx, ast_ctx, hir_ctx, typed);

  Interpreter_Runtime runtime(&dx);
  const auto evaluated = runtime.evaluate_module(hir);
  ASSERT_TRUE(evaluated.ok());
  EXPECT_FALSE(dx.has_errors());

  const auto dump = interpreter_dump_globals(runtime);
  EXPECT_EQ(
      dump,
      "Globals\n"
      "  add = Function(add)\n"
      "  value = Int(42)\n");

  DynArray<Interpreter_Value> arguments;
  arguments.append(Interpreter_Value::make_integer(7));
  arguments.append(Interpreter_Value::make_integer(5));
  const auto called = runtime.call("add", arguments);
  ASSERT_TRUE(called.ok());
  EXPECT_EQ(called.value.kind, Interpreter_Value_Kind::Integer);
  EXPECT_EQ(called.value.integer_value, 12);
}

TEST(InterpreterRuntime, SupportsLoopsAssignmentsAndLocalMutation)
{
  Source_Manager sm;
  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ast_ctx(1 << 16);
  Compiler_HIR_Context hir_ctx(1 << 16);
  Compiler_Typed_Semantic_Analyzer typed(&dx);

  const auto * hir = lower_to_hir(
      "fn countdown(n) {\n"
      "  while (n > 0) {\n"
      "    n = n - 1;\n"
      "  }\n"
      "  return n;\n"
      "}\n"
      "let result = countdown(3);\n",
      sm, dx, ast_ctx, hir_ctx, typed);

  Interpreter_Runtime runtime(&dx);
  const auto evaluated = runtime.evaluate_module(hir);
  ASSERT_TRUE(evaluated.ok());
  EXPECT_FALSE(dx.has_errors());

  const auto * result = runtime.global_environment().lookup("result");
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->kind, Interpreter_Value_Kind::Integer);
  EXPECT_EQ(result->integer_value, 0);
}

TEST(InterpreterRuntime, BindsHostFunctionsInPrelude)
{
  Interpreter_Runtime runtime;
  ASSERT_TRUE(runtime.bind_host_function("pair", host_pair));

  DynArray<Interpreter_Value> arguments;
  arguments.append(Interpreter_Value::make_integer(1));
  arguments.append(Interpreter_Value::make_integer(2));

  const auto called = runtime.call("pair", arguments);
  ASSERT_TRUE(called.ok());
  EXPECT_EQ(called.value.kind, Interpreter_Value_Kind::Tuple);
  ASSERT_EQ(called.value.tuple_elements().size(), 2u);
  EXPECT_EQ(called.value.tuple_elements().access(0).integer_value, 1);
  EXPECT_EQ(called.value.tuple_elements().access(1).integer_value, 2);
  EXPECT_EQ(interpreter_value_to_string(called.value), "Tuple(Int(1), Int(2))");
}

TEST(InterpreterRuntime, TupleValuesKeepDeepCopySemantics)
{
  DynArray<Interpreter_Value> elements;
  elements.append(Interpreter_Value::make_integer(1));
  elements.append(Interpreter_Value::make_integer(2));

  const auto original = Interpreter_Value::make_tuple(std::move(elements));
  auto copied = original;
  copied.tuple_storage->elements.access(0) = Interpreter_Value::make_integer(99);

  EXPECT_EQ(original.tuple_elements().access(0).integer_value, 1);
  EXPECT_EQ(copied.tuple_elements().access(0).integer_value, 99);

  auto moved = std::move(copied);
  EXPECT_EQ(moved.tuple_elements().access(0).integer_value, 99);
  EXPECT_EQ(copied.kind, Interpreter_Value_Kind::Invalid);
}

TEST(InterpreterRuntime, ReportsStructuredRuntimeErrors)
{
  Source_Manager sm;
  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ast_ctx(1 << 16);
  Compiler_HIR_Context hir_ctx(1 << 16);
  Compiler_Typed_Semantic_Analyzer typed(&dx);

  const auto * hir = lower_to_hir(
      "fn explode(x) {\n"
      "  return x / 0;\n"
      "}\n"
      "let doomed = explode(5);\n",
      sm, dx, ast_ctx, hir_ctx, typed);

  Interpreter_Runtime runtime(&dx);
  const auto evaluated = runtime.evaluate_module(hir);
  ASSERT_FALSE(evaluated.ok());
  EXPECT_EQ(evaluated.flow, Interpreter_Control_Flow_Kind::Runtime_Error);
  EXPECT_EQ(evaluated.error.code, "RUN007");
  EXPECT_EQ(evaluated.error.message, "division by zero");
  EXPECT_TRUE(dx.has_errors());
}
