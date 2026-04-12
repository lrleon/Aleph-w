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
 * @file compiler_lexer_example.cc
 * @brief Minimal example that tokenizes a small source file.
 */

# include <iostream>

# include <Compiler_Lexer.H>

using namespace Aleph;

int main()
{
  Source_Manager sm;
  const auto file_id = sm.add_virtual_file(
      "demo.aw",
      "fn greet(name) {\n"
      "  let message = \"hello\";\n"
      "  return message;\n"
      "}\n");

  Diagnostic_Engine dx(sm);
  Compiler_Lexer lexer(sm, file_id, &dx);

  while (true)
    {
      const auto tok = lexer.next();
      std::cout << tok.to_string() << '\n';
      if (tok.is_eof())
        break;
    }

  if (dx.size() > 0)
    {
      std::cout << "\nDiagnostics:\n";
      dx.render_plain(std::cout);
      return dx.has_errors() ? 1 : 0;
    }

  return 0;
}
