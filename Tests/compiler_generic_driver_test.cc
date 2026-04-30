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
 * @file compiler_generic_driver_test.cc
 * @brief Integration tests for the reusable frontend/driver contracts.
 */

#include <gtest/gtest.h>

#include <Compiler_Generic_Driver.H>
#include <Compiler_MVP_Frontend.H>

using namespace Aleph;

namespace
{
  DynArray<Compiler_Driver_Source>
  make_program_inputs()
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
}

TEST(CompilerGenericDriver, RunsProgramThroughReusableFrontendContract)
{
  Compiler_MVP_Frontend frontend;
  Compiler_Generic_Driver driver(frontend);

  ASSERT_TRUE(driver.execute_sources(make_program_inputs(),
                                     Compiler_Driver_Action::Run));
  ASSERT_TRUE(driver.run_output().executed);
  ASSERT_TRUE(driver.run_output().result.ok());
  EXPECT_EQ(driver.run_output().globals_text,
            "Globals\n"
            "  add = Function(add)\n"
            "  answer = Int(42)\n");

  const auto * hir = driver.find_artifact("hir");
  ASSERT_NE(hir, nullptr);
  EXPECT_NE(hir->text.find("HIRModule"), std::string::npos);
  const auto * order = driver.find_artifact("imports.order");
  ASSERT_NE(order, nullptr);
  EXPECT_EQ(order->text,
            "ImportOrder\n"
            "  [0] math.aw\n"
            "  [1] main.aw\n");
  const auto * surface = driver.find_artifact("modules.surface");
  ASSERT_NE(surface, nullptr);
  EXPECT_NE(surface->text.find("Module(main.aw)"), std::string::npos);
  EXPECT_NE(surface->text.find("Module(math.aw)"), std::string::npos);
  EXPECT_NE(surface->text.find("Function(add)"), std::string::npos);
  EXPECT_NE(surface->text.find("Global(answer)"), std::string::npos);
  const auto * linkage = driver.find_artifact("modules.linkage");
  ASSERT_NE(linkage, nullptr);
  EXPECT_NE(linkage->text.find("ModuleLinkage"), std::string::npos);
  EXPECT_NE(linkage->text.find("Module(main.aw) order=1"), std::string::npos);
  EXPECT_NE(linkage->text.find("Visible(Function(add)) from math.aw imported"),
            std::string::npos);
  EXPECT_NE(linkage->text.find("Visible(Global(answer)) from main.aw local"),
            std::string::npos);
  const auto * binding = driver.find_artifact("modules.binding");
  ASSERT_NE(binding, nullptr);
  EXPECT_NE(binding->text.find("ModuleBindings"), std::string::npos);
  EXPECT_NE(binding->text.find("Bind(Function(add)) -> math.aw imported"),
            std::string::npos);
  EXPECT_NE(binding->text.find("Bind(Global(answer)) -> main.aw local"),
            std::string::npos);
  const auto * names = driver.find_artifact("modules.names");
  ASSERT_NE(names, nullptr);
  EXPECT_NE(names->text.find("ModuleNameTable"), std::string::npos);
  EXPECT_NE(names->text.find("Name(Function(add)) -> math.aw imported"),
            std::string::npos);
  const auto * semantic = driver.find_artifact("modules.semantic");
  ASSERT_NE(semantic, nullptr);
  EXPECT_NE(semantic->text.find("ModuleSemanticEnvironment"), std::string::npos);
  EXPECT_NE(semantic->text.find("Value(Function(add)) -> math.aw imported"),
            std::string::npos);
  ASSERT_EQ(frontend.module_descriptors().size(), 2u);
  ASSERT_EQ(frontend.module_merge_order().size(), 2u);
  ASSERT_EQ(frontend.module_metadata().size(), 2u);
  EXPECT_NE(frontend.find_artifact("ast"), nullptr);
  EXPECT_NE(frontend.find_artifact("typed-sema"), nullptr);
}

TEST(CompilerGenericDriver, EmitsBytecodeAndPortableCThroughReusableFrontendContract)
{
  Compiler_MVP_Frontend bytecode_frontend;
  Compiler_Generic_Driver bytecode_driver(bytecode_frontend);

  ASSERT_TRUE(bytecode_driver.execute_sources(make_program_inputs(),
                                              Compiler_Driver_Action::Emit_Bytecode));
  ASSERT_NE(bytecode_driver.bytecode_module(), nullptr);

  const auto * bytecode = bytecode_driver.find_artifact("bytecode");
  ASSERT_NE(bytecode, nullptr);
  EXPECT_NE(bytecode->text.find("BytecodeModule"), std::string::npos);

  Compiler_MVP_Frontend c_frontend;
  Compiler_Generic_Driver_Options options;
  options.c_backend_options.module_name = "generic_driver_demo";
  options.c_backend_options.emit_main = true;

  Compiler_Generic_Driver c_driver(c_frontend, options);
  ASSERT_TRUE(c_driver.execute_sources(make_program_inputs(),
                                       Compiler_Driver_Action::Emit_C));
  ASSERT_TRUE(c_driver.c_emission().valid);

  const auto * c_artifact = c_driver.find_artifact("emit-c");
  ASSERT_NE(c_artifact, nullptr);
  EXPECT_NE(c_artifact->text.find("main(void)"), std::string::npos);
  EXPECT_NE(c_artifact->text.find("generic_driver_demo"), std::string::npos);
}

TEST(CompilerGenericDriver, ReportsRuntimeFailuresThroughDriverMessages)
{
  DynArray<Compiler_Driver_Source> inputs;
  inputs.append({"main.aw",
                 "let crash = 1 / 0;\n"});

  Compiler_MVP_Frontend frontend;
  Compiler_Generic_Driver driver(frontend);

  EXPECT_FALSE(driver.execute_sources(inputs, Compiler_Driver_Action::Run));
  ASSERT_TRUE(driver.run_output().executed);
  EXPECT_FALSE(driver.run_output().result.ok());
  EXPECT_TRUE(driver.run_output().value_text.empty());
  EXPECT_EQ(driver.run_output().result.error.code, "RUN007");

  const auto * messages = driver.find_artifact("driver.messages");
  ASSERT_NE(messages, nullptr);
  EXPECT_NE(messages->text.find("run: HIR runtime execution failed: RUN007 division by zero"),
            std::string::npos);
}
