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
 * @file compiler_line_frontend_example.cc
 * @brief Minimal example that runs a second frontend through the generic driver.
 */

# include <iostream>

# include <Compiler_Generic_Driver.H>
# include <Compiler_Line_Frontend.H>

using namespace Aleph;

int main()
{
  DynArray<Compiler_Driver_Source> inputs;
  inputs.append({"math.line", "func add x y = add x y\n"});
  inputs.append({"main.line", "import math.line\nlet answer = call add 40 2\n"});

  Compiler_Line_Frontend frontend;
  Compiler_Generic_Driver driver(frontend);
  const bool ok = driver.execute_sources(inputs, Compiler_Driver_Action::Run);

  if (const auto * order = driver.find_artifact("imports.order"); order != nullptr)
    std::cout << order->text << '\n';
  if (const auto * surface = driver.find_artifact("modules.surface"); surface != nullptr)
    std::cout << surface->text << '\n';
  if (const auto * linkage = driver.find_artifact("modules.linkage"); linkage != nullptr)
    std::cout << linkage->text << '\n';
  if (const auto * binding = driver.find_artifact("modules.binding"); binding != nullptr)
    std::cout << binding->text << '\n';
  if (const auto * names = driver.find_artifact("modules.names"); names != nullptr)
    std::cout << names->text << '\n';
  if (const auto * semantic = driver.find_artifact("modules.semantic"); semantic != nullptr)
    std::cout << semantic->text << '\n';
  if (const auto * parse = driver.find_artifact("line.parse"); parse != nullptr)
    std::cout << parse->text << '\n';
  if (const auto * hir = driver.find_artifact("hir"); hir != nullptr)
    std::cout << hir->text << '\n';
  if (const auto * ir = driver.find_artifact("ir"); ir != nullptr)
    std::cout << ir->text << '\n';
  if (const auto * run = driver.find_artifact("run.result"); run != nullptr)
    std::cout << run->text << '\n';
  if (const auto * globals = driver.find_artifact("run.globals"); globals != nullptr)
    std::cout << globals->text << '\n';

  return ok ? 0 : 1;
}
