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
 * @file compiler_driver_test.cc
 * @brief End-to-end integration tests for Compiler_Driver.H.
 */

#include <gtest/gtest.h>

#include <Compiler_Driver.H>

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

TEST(CompilerDriver, ParsesAndMergesOrderedMultiFileModules)
{
  Compiler_Driver driver;
  ASSERT_TRUE(driver.execute_sources(make_program_inputs(),
                                     Compiler_Driver_Action::Parse_Only));

  ASSERT_EQ(driver.parsed_ast_modules().size(), 2u);
  ASSERT_NE(driver.merged_ast_module(), nullptr);
  ASSERT_NE(driver.find_artifact("ast[0]:main.aw"), nullptr);
  ASSERT_NE(driver.find_artifact("ast[1]:math.aw"), nullptr);

  const auto * main_ast = driver.find_artifact("ast[0]:main.aw");
  ASSERT_NE(main_ast, nullptr);
  EXPECT_NE(main_ast->text.find("Import(\"math.aw\")"), std::string::npos);

  const auto * import_order = driver.find_artifact("imports.order");
  ASSERT_NE(import_order, nullptr);
  EXPECT_EQ(import_order->text,
            "ImportOrder\n"
            "  [0] math.aw\n"
            "  [1] main.aw\n");

  const auto * merged = driver.find_artifact("ast");
  ASSERT_NE(merged, nullptr);
  EXPECT_NE(merged->text.find("Function(add)"), std::string::npos);
  EXPECT_NE(merged->text.find("Let(answer)"), std::string::npos);
  EXPECT_LT(merged->text.find("Function(add)"), merged->text.find("Let(answer)"));
  EXPECT_EQ(driver.last_action(), Compiler_Driver_Action::Parse_Only);

  Compiler_Driver_Action parsed = Compiler_Driver_Action::Run;
  ASSERT_TRUE(compiler_parse_driver_action("emit-bytecode", parsed));
  EXPECT_EQ(parsed, Compiler_Driver_Action::Emit_Bytecode);
  EXPECT_STREQ(compiler_driver_action_name(parsed), "emit-bytecode");
}

TEST(CompilerDriver, RunsMergedHIRModuleAndPublishesArtifacts)
{
  Compiler_Driver driver;
  ASSERT_TRUE(driver.execute_sources(make_program_inputs(),
                                     Compiler_Driver_Action::Run));

  ASSERT_TRUE(driver.run_output().executed);
  ASSERT_TRUE(driver.run_output().result.ok());
  EXPECT_EQ(driver.run_output().value_text, "Unit");
  EXPECT_EQ(driver.run_output().globals_text,
            "Globals\n"
            "  add = Function(add)\n"
            "  answer = Int(42)\n");

  const auto * hir = driver.find_artifact("hir");
  ASSERT_NE(hir, nullptr);
  EXPECT_NE(hir->text.find("HIRModule"), std::string::npos);
  EXPECT_NE(hir->text.find("Function(add): fn(Int, Int) -> Int"),
            std::string::npos);

  const auto * globals = driver.find_artifact("run.globals");
  ASSERT_NE(globals, nullptr);
  EXPECT_EQ(globals->text, driver.run_output().globals_text);
}

TEST(CompilerDriver, EmitsBytecodeAndPortableCArtifacts)
{
  Compiler_Driver bytecode_driver;
  ASSERT_TRUE(bytecode_driver.execute_sources(make_program_inputs(),
                                              Compiler_Driver_Action::Emit_Bytecode));
  ASSERT_NE(bytecode_driver.bytecode_module(), nullptr);

  const auto * bytecode = bytecode_driver.find_artifact("bytecode");
  ASSERT_NE(bytecode, nullptr);
  EXPECT_NE(bytecode->text.find("BytecodeModule"), std::string::npos);
  EXPECT_NE(bytecode->text.find("answer"), std::string::npos);

  Compiler_Driver_Options options;
  options.c_backend_options.module_name = "driver_demo";
  options.c_backend_options.emit_main = true;

  Compiler_Driver c_driver(options);
  ASSERT_TRUE(c_driver.execute_sources(make_program_inputs(),
                                       Compiler_Driver_Action::Emit_C));
  ASSERT_TRUE(c_driver.c_emission().valid);

  const auto * c_artifact = c_driver.find_artifact("emit-c");
  ASSERT_NE(c_artifact, nullptr);
  EXPECT_NE(c_artifact->text.find("main(void)"), std::string::npos);
  EXPECT_NE(c_artifact->text.find(c_driver.c_emission().module_init_symbol),
            std::string::npos);
  EXPECT_NE(c_artifact->text.find("driver_demo"), std::string::npos);
}

TEST(CompilerDriver, StopsBeforeLoweringWhenDiagnosticsExist)
{
  DynArray<Compiler_Driver_Source> inputs;
  inputs.append({"main.aw", "let answer = missing(1);\n"});

  Compiler_Driver driver;
  EXPECT_FALSE(driver.execute_sources(inputs, Compiler_Driver_Action::Run));
  EXPECT_TRUE(driver.diagnostic_engine().has_errors());
  EXPECT_NE(driver.diagnostics_text().find("SEM002"), std::string::npos);
  EXPECT_EQ(driver.hir_module(), nullptr);
  EXPECT_EQ(driver.find_artifact("hir"), nullptr);

  const auto * diagnostics = driver.find_artifact("diagnostics");
  ASSERT_NE(diagnostics, nullptr);
  EXPECT_NE(diagnostics->text.find("undeclared identifier"), std::string::npos);
}

TEST(CompilerDriver, ReportsMissingImportDependencies)
{
  DynArray<Compiler_Driver_Source> inputs;
  inputs.append({"main.aw",
                 "import \"missing.aw\";\n"
                 "let answer = 1;\n"});

  Compiler_Driver driver;
  EXPECT_FALSE(driver.execute_sources(inputs, Compiler_Driver_Action::Parse_Only));
  EXPECT_TRUE(driver.diagnostic_engine().has_errors());
  EXPECT_NE(driver.diagnostics_text().find("DRV001"), std::string::npos);
  EXPECT_NE(driver.diagnostics_text().find("cannot resolve import 'missing.aw'"),
            std::string::npos);
}

TEST(CompilerDriver, ReportsCyclicImports)
{
  DynArray<Compiler_Driver_Source> inputs;
  inputs.append({"a.aw", "import \"b.aw\";\nlet a = 1;\n"});
  inputs.append({"b.aw", "import \"a.aw\";\nlet b = 2;\n"});

  Compiler_Driver driver;
  EXPECT_FALSE(driver.execute_sources(inputs, Compiler_Driver_Action::Parse_Only));
  EXPECT_TRUE(driver.diagnostic_engine().has_errors());
  EXPECT_NE(driver.diagnostics_text().find("DRV002"), std::string::npos);
  EXPECT_NE(driver.diagnostics_text().find("cyclic import"), std::string::npos);
}
