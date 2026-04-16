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
 * @file interpreter_runtime_example.cc
 * @brief Minimal example that evaluates typed HIR with the reusable runtime.
 */

#include <iostream>

#include <Compiler_Parser.H>
#include <Interpreter_Runtime.H>

using namespace Aleph;

int main()
{
  Source_Manager sm;
  const auto file_id = sm.add_virtual_file(
      "demo.aw",
      "fn add(x, y) {\n"
      "  return x + y;\n"
      "}\n"
      "fn twice(x) {\n"
      "  return add(x, x);\n"
      "}\n"
      "let answer = twice(21);\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ast_ctx(1 << 16);
  Compiler_Parser parser(ast_ctx, sm, file_id, &dx);
  const auto * module = parser.parse_module();

  Compiler_Typed_Semantic_Analyzer typed(&dx);
  typed.analyze_module(module);

  Compiler_HIR_Context hir_ctx(1 << 16);
  Compiler_HIR_Lowering lowering(hir_ctx, typed);
  const auto * hir = lowering.lower_module(module);

  Interpreter_Runtime runtime(&dx);
  const auto evaluated = runtime.evaluate_module(hir);
  if (not evaluated.ok())
    {
      dx.render_plain(std::cout);
      return 1;
    }

  std::cout << interpreter_dump_globals(runtime);

  DynArray<Interpreter_Value> arguments;
  arguments.append(Interpreter_Value::make_integer(9));
  const auto called = runtime.call("twice", arguments);
  if (not called.ok())
    {
      dx.render_plain(std::cout);
      return 1;
    }

  std::cout << "twice(9) = " << interpreter_value_to_string(called.value) << '\n';
  return 0;
}
