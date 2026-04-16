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
 * @file compiler_ir_example.cc
 * @brief Minimal example that lowers typed HIR into explicit-value IR.
 */

#include <iostream>

#include <Compiler_IR.H>
#include <Compiler_Parser.H>

using namespace Aleph;

int main()
{
  Source_Manager sm;
  const auto file_id = sm.add_virtual_file(
      "demo.aw",
      "let base = 10;\n"
      "fn add_base(x) {\n"
      "  return x + base;\n"
      "}\n"
      "let answer = add_base(32);\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ast_ctx(1 << 16);
  Compiler_Parser parser(ast_ctx, sm, file_id, &dx);
  const auto * module = parser.parse_module();

  Compiler_Typed_Semantic_Analyzer typed(&dx);
  typed.analyze_module(module);

  Compiler_HIR_Context hir_ctx(1 << 16);
  Compiler_HIR_Lowering hir_lowering(hir_ctx, typed);
  const auto * hir = hir_lowering.lower_module(module);

  Compiler_IR_Context ir_ctx(1 << 16);
  Compiler_IR_Lowering ir_lowering(ir_ctx, &typed.type_context());
  const auto * ir = ir_lowering.lower_module(hir);

  const auto report = validate_ir_module(*ir);
  std::cout << compiler_dump_ir_module(ir, &typed.type_context());
  if (not report.warnings.is_empty())
    {
      std::cout << "\nWarnings:\n";
      for (size_t i = 0; i < report.warnings.size(); ++i)
        std::cout << "- " << report.warnings.access(i) << '\n';
    }

  if (dx.size() > 0)
    {
      std::cout << "\nDiagnostics:\n";
      dx.render_plain(std::cout);
    }

  return report.valid && not dx.has_errors() ? 0 : 1;
}
