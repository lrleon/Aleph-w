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
 * @file compiler_driver_example.cc
 * @brief Minimal example that exercises import-aware multi-file driver execution.
 */

#include <iostream>
#include <sstream>
#include <string>

#include <Compiler_Driver.H>

using namespace Aleph;

namespace
{
  DynArray<Compiler_Driver_Source>
  build_inputs()
  {
    DynArray<Compiler_Driver_Source> inputs;
    inputs.append({"main.aw",
                   "import \"math.aw\";\n"
                   "let answer = add(40, 2);\n"});
    inputs.append({"math.aw",
                   "fn add(x, y) {\n"
                   "  return x + y;\n"
                   "}\n"});
    return inputs;
  }

  bool
  report_failure(const Compiler_Driver & driver,
                 const char * label)
  {
    std::cerr << label << " failed\n";
    std::cerr << driver.diagnostics_text();
    if (const auto * messages = driver.find_artifact("driver.messages");
        messages != nullptr)
      std::cerr << messages->text;
    return false;
  }

  void
  print_first_lines(const std::string & text,
                    const size_t max_lines)
  {
    std::istringstream input(text);
    std::string line;
    size_t printed = 0;
    while (printed < max_lines and std::getline(input, line))
      {
        std::cout << line << '\n';
        ++printed;
      }
  }
}

int main()
{
  const auto inputs = build_inputs();

  Compiler_Driver run_driver;
  if (not run_driver.execute_sources(inputs, Compiler_Driver_Action::Run))
    return report_failure(run_driver, "run") ? 0 : 1;

  std::cout << "=== imports.order ===\n";
  std::cout << run_driver.find_artifact("imports.order")->text;
  std::cout << "=== run.result ===\n";
  std::cout << run_driver.find_artifact("run.result")->text;
  std::cout << "=== run.globals ===\n";
  std::cout << run_driver.run_output().globals_text;

  Compiler_Driver bytecode_driver;
  if (not bytecode_driver.execute_sources(inputs,
                                          Compiler_Driver_Action::Emit_Bytecode))
    return report_failure(bytecode_driver, "emit-bytecode") ? 0 : 1;

  std::cout << "\n=== bytecode ===\n";
  std::cout << bytecode_driver.find_artifact("bytecode")->text;

  Compiler_Driver_Options c_options;
  c_options.c_backend_options.module_name = "driver_demo";
  c_options.c_backend_options.emit_main = true;

  Compiler_Driver c_driver(c_options);
  if (not c_driver.execute_sources(inputs, Compiler_Driver_Action::Emit_C))
    return report_failure(c_driver, "emit-c") ? 0 : 1;

  std::cout << "\n=== emit-c (first 24 lines) ===\n";
  print_first_lines(c_driver.c_emission().source, 24);

  return 0;
}
