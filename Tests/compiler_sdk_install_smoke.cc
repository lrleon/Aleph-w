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
 * @file compiler_sdk_install_smoke.cc
 * @brief External-consumer smoke source for installed compiler SDK headers.
 */

#include <Compiler_Generic_Driver.H>
#include <Compiler_Line_Frontend.H>
#include <Compiler_Module_Name_Diagnostics.H>
#include <Compiler_Module_Semantic_Environment.H>

int
main()
{
  using namespace Aleph;

  DynArray<Compiler_Driver_Source> inputs;
  inputs.append({"main.line", "let answer = int 1\n"});

  Compiler_Line_Frontend frontend;
  Compiler_Generic_Driver driver(frontend);
  (void) driver.execute_sources(inputs, Compiler_Driver_Action::Parse_Only);

  Compiler_Module_Name_Resolution resolution;
  resolution.module_name = "main.line";
  resolution.symbol_name = "answer";
  resolution.filter = Compiler_Module_Name_Filter::Value;

  Compiler_Module_Semantic_Environment env;
  (void) compiler_find_module_semantic_environment(env, "main.line");
  (void) compiler_module_name_resolution_status_name(resolution.status);
  return 0;
}
