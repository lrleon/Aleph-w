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
 * @file compiler_parser_example.cc
 * @brief Minimal example that parses and dumps a small module with imports.
 */

# include <iostream>

# include <Compiler_Parser.H>

using namespace Aleph;

int main()
{
  Source_Manager sm;
  const auto file_id = sm.add_virtual_file(
      "demo.aw",
      "import \"math.aw\";\n"
      "fn inc(x) {\n"
      "  let y = x + 1;\n"
      "  return y;\n"
      "}\n"
      "let value = inc(41);\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 16);
  Compiler_Parser parser(ctx, sm, file_id, &dx);

  const auto * module = parser.parse_module();

  std::cout << compiler_dump_module(module);
  if (dx.size() > 0)
    {
      std::cout << "\nDiagnostics:\n";
      dx.render_plain(std::cout);
      return dx.has_errors() ? 1 : 0;
    }

  return 0;
}
