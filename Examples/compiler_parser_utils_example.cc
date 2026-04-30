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
 * @file compiler_parser_utils_example.cc
 * @brief Minimal example showing reusable parser-stream helpers over the lexer.
 */

#include <iostream>

#include <Compiler_Parser_Utils.H>

using namespace Aleph;

int
main()
{
  Source_Manager sm;
  const auto file_id = sm.add_virtual_file("demo.aw", "fn add(left, right) {}");
  Diagnostic_Engine dx(sm);
  Compiler_Lexer lexer(sm, file_id, &dx);
  Compiler_Parser_Stream stream(lexer, &dx);

  (void) stream.expect(Compiler_Token_Kind::Kw_Fn, "expected 'fn'", "EX001");
  const auto name = stream.expect_identifier("after 'fn'", "EX002");
  (void) stream.expect(Compiler_Token_Kind::LParen, "expected '('", "EX003");

  DynArray<std::string> parameters;
  (void) stream.parse_separated_list(Compiler_Token_Kind::Comma,
                                     Compiler_Token_Kind::RParen,
                                     [&]()
                                     {
                                       parameters.append(
                                         stream.expect_identifier("in parameter list", "EX004").lexeme);
                                       return true;
                                     });
  (void) stream.expect(Compiler_Token_Kind::RParen, "expected ')'", "EX005");

  std::cout << "Function: " << name.lexeme << '\n';
  std::cout << "Params:";
  for (size_t i = 0; i < parameters.size(); ++i)
    std::cout << ' ' << parameters.access(i);
  std::cout << '\n';

  if (dx.size() > 0)
    dx.render_plain(std::cout);
}
