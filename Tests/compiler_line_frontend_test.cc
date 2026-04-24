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
 * @file compiler_line_frontend_test.cc
 * @brief Integration tests for the alternate line-oriented frontend.
 */

#include <gtest/gtest.h>

#include <Compiler_Generic_Driver.H>
#include <Compiler_Line_Frontend.H>

using namespace Aleph;

namespace
{
  DynArray<Compiler_Driver_Source>
  make_line_inputs()
  {
    DynArray<Compiler_Driver_Source> inputs;
    inputs.append({"math.line",
                   "func add x y = add x y\n"});
    inputs.append({"main.line",
                   "import math.line\n"
                   "let answer = call add 40 2\n"});
    return inputs;
  }
}

TEST(CompilerLineFrontend, RunsProgramThroughGenericDriver)
{
  Compiler_Line_Frontend frontend;
  Compiler_Generic_Driver driver(frontend);

  ASSERT_TRUE(driver.execute_sources(make_line_inputs(), Compiler_Driver_Action::Run));
  ASSERT_TRUE(driver.run_output().executed);
  ASSERT_TRUE(driver.run_output().result.ok());
  EXPECT_EQ(driver.run_output().globals_text,
            "Globals\n"
            "  add = Function(add)\n"
            "  answer = Int(42)\n");

  const auto * parse_artifact = frontend.find_artifact("line.parse");
  ASSERT_NE(parse_artifact, nullptr);
  EXPECT_NE(parse_artifact->text.find("LineProgram"), std::string::npos);

  const auto * order_artifact = frontend.find_artifact("imports.order");
  ASSERT_NE(order_artifact, nullptr);
  EXPECT_EQ(order_artifact->text,
            "ImportOrder\n"
            "  [0] math.line\n"
            "  [1] main.line\n");
  const auto * surface = driver.find_artifact("modules.surface");
  ASSERT_NE(surface, nullptr);
  EXPECT_NE(surface->text.find("Module(math.line)"), std::string::npos);
  EXPECT_NE(surface->text.find("Module(main.line)"), std::string::npos);
  EXPECT_NE(surface->text.find("Function(add)"), std::string::npos);
  EXPECT_NE(surface->text.find("Global(answer)"), std::string::npos);
  const auto * linkage = driver.find_artifact("modules.linkage");
  ASSERT_NE(linkage, nullptr);
  EXPECT_NE(linkage->text.find("ModuleLinkage"), std::string::npos);
  EXPECT_NE(linkage->text.find("Module(main.line) order=1"), std::string::npos);
  EXPECT_NE(linkage->text.find("Visible(Function(add)) from math.line imported"),
            std::string::npos);
  EXPECT_NE(linkage->text.find("Visible(Global(answer)) from main.line local"),
            std::string::npos);
  const auto * binding = driver.find_artifact("modules.binding");
  ASSERT_NE(binding, nullptr);
  EXPECT_NE(binding->text.find("ModuleBindings"), std::string::npos);
  EXPECT_NE(binding->text.find("Bind(Function(add)) -> math.line imported"),
            std::string::npos);
  EXPECT_NE(binding->text.find("Bind(Global(answer)) -> main.line local"),
            std::string::npos);
  const auto * names = driver.find_artifact("modules.names");
  ASSERT_NE(names, nullptr);
  EXPECT_NE(names->text.find("ModuleNameTable"), std::string::npos);
  EXPECT_NE(names->text.find("Name(Function(add)) -> math.line imported"),
            std::string::npos);
  const auto * semantic = driver.find_artifact("modules.semantic");
  ASSERT_NE(semantic, nullptr);
  EXPECT_NE(semantic->text.find("ModuleSemanticEnvironment"), std::string::npos);
  EXPECT_NE(semantic->text.find("Value(Function(add)) -> math.line imported"),
            std::string::npos);
  ASSERT_EQ(frontend.module_metadata().size(), 2u);

  const auto * hir_artifact = driver.find_artifact("hir");
  ASSERT_NE(hir_artifact, nullptr);
  EXPECT_NE(hir_artifact->text.find("HIRModule"), std::string::npos);
}

TEST(CompilerLineFrontend, EmitsBytecodeAndPortableCThroughGenericDriver)
{
  Compiler_Line_Frontend bytecode_frontend;
  Compiler_Generic_Driver bytecode_driver(bytecode_frontend);

  ASSERT_TRUE(
    bytecode_driver.execute_sources(make_line_inputs(), Compiler_Driver_Action::Emit_Bytecode));
  ASSERT_NE(bytecode_driver.bytecode_module(), nullptr);

  const auto * bytecode = bytecode_driver.find_artifact("bytecode");
  ASSERT_NE(bytecode, nullptr);
  EXPECT_NE(bytecode->text.find("BytecodeModule"), std::string::npos);

  Compiler_Line_Frontend c_frontend;
  Compiler_Generic_Driver_Options options;
  options.c_backend_options.module_name = "line_driver_demo";
  options.c_backend_options.emit_main = true;

  Compiler_Generic_Driver c_driver(c_frontend, options);
  ASSERT_TRUE(c_driver.execute_sources(make_line_inputs(), Compiler_Driver_Action::Emit_C));
  ASSERT_TRUE(c_driver.c_emission().valid);

  const auto * c_artifact = c_driver.find_artifact("emit-c");
  ASSERT_NE(c_artifact, nullptr);
  EXPECT_NE(c_artifact->text.find("main(void)"), std::string::npos);
  EXPECT_NE(c_artifact->text.find("line_driver_demo"), std::string::npos);
}

TEST(CompilerLineFrontend, ReportsStructuredDiagnosticsForUnknownCallee)
{
  DynArray<Compiler_Driver_Source> inputs;
  inputs.append({"main.line", "let answer = call missing 1 2\n"});

  Compiler_Line_Frontend frontend;
  Compiler_Generic_Driver driver(frontend);

  EXPECT_FALSE(driver.execute_sources(inputs, Compiler_Driver_Action::Run));
  ASSERT_NE(frontend.diagnostic_engine(), nullptr);
  EXPECT_TRUE(frontend.diagnostic_engine()->has_errors());

  const auto * diagnostics = driver.find_artifact("diagnostics");
  ASSERT_NE(diagnostics, nullptr);
  EXPECT_NE(diagnostics->text.find("LIN015"), std::string::npos);
}

TEST(CompilerLineFrontend, RejectsCallsToNonImportedFunctions)
{
  DynArray<Compiler_Driver_Source> inputs;
  inputs.append({"helper.line", "func hidden x y = add x y\n"});
  inputs.append({"main.line", "let answer = call hidden 1 2\n"});

  Compiler_Line_Frontend frontend;
  Compiler_Generic_Driver driver(frontend);

  EXPECT_FALSE(driver.execute_sources(inputs, Compiler_Driver_Action::Run));
  ASSERT_NE(frontend.diagnostic_engine(), nullptr);
  EXPECT_TRUE(frontend.diagnostic_engine()->has_errors());

  const auto * diagnostics = driver.find_artifact("diagnostics");
  ASSERT_NE(diagnostics, nullptr);
  EXPECT_NE(diagnostics->text.find("LIN015"), std::string::npos);
}

TEST(CompilerLineFrontend, ReportsMissingImportsDuringParsing)
{
  DynArray<Compiler_Driver_Source> inputs;
  inputs.append({"main.line",
                 "import missing.line\n"
                 "let answer = int 1\n"});

  Compiler_Line_Frontend frontend;
  Compiler_Generic_Driver driver(frontend);

  EXPECT_FALSE(driver.execute_sources(inputs, Compiler_Driver_Action::Parse_Only));
  ASSERT_NE(frontend.diagnostic_engine(), nullptr);
  EXPECT_TRUE(frontend.diagnostic_engine()->has_errors());

  const auto * diagnostics = driver.find_artifact("diagnostics");
  ASSERT_NE(diagnostics, nullptr);
  EXPECT_NE(diagnostics->text.find("LIN017"), std::string::npos);
}

TEST(CompilerLineFrontend, RejectsInvalidOperandsDuringParsing)
{
  DynArray<Compiler_Driver_Source> inputs;
  inputs.append({"main.line",
                 "func add x y = add x +\n"
                 "let answer = call add 1 )\n"});

  Compiler_Line_Frontend frontend;
  Compiler_Generic_Driver driver(frontend);

  EXPECT_FALSE(driver.execute_sources(inputs, Compiler_Driver_Action::Parse_Only));
  ASSERT_NE(frontend.diagnostic_engine(), nullptr);
  EXPECT_TRUE(frontend.diagnostic_engine()->has_errors());

  const auto * diagnostics = driver.find_artifact("diagnostics");
  ASSERT_NE(diagnostics, nullptr);
  EXPECT_NE(diagnostics->text.find("LIN018"), std::string::npos);
}
