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
 * @file compiler_module_name_diagnostics_test.cc
 * @brief Tests for reusable inter-module name-resolution diagnostics.
 */

#include <gtest/gtest.h>

#include <Compiler_Module_Name_Diagnostics.H>

using namespace Aleph;

TEST(CompilerModuleNameDiagnostics, EmitsMissingNameDiagnostic)
{
  Source_Manager sm;
  const auto file = sm.add_virtual_file("main.aw", "let answer = add(1, 2);\n");
  Diagnostic_Engine diagnostics(sm);

  Compiler_Module_Name_Resolution resolution;
  resolution.status = Compiler_Module_Name_Resolution_Status::Missing_Name;
  resolution.module_name = "main.aw";
  resolution.symbol_name = "add";
  resolution.filter = Compiler_Module_Name_Filter::Function;

  const auto index = compiler_emit_module_name_resolution_diagnostic(
    diagnostics, sm.span(file, 13, 16), resolution, "SDK001", "test callee");
  EXPECT_EQ(index, 0u);
  ASSERT_EQ(diagnostics.size(), 1u);

  std::ostringstream out;
  diagnostics.render_plain(out);
  EXPECT_NE(out.str().find("error[SDK001]: unknown test callee 'add'"), std::string::npos);
  EXPECT_NE(out.str().find("does not expose that name"), std::string::npos);
}

TEST(CompilerModuleNameDiagnostics, EmitsWrongKindDiagnostic)
{
  Source_Manager sm;
  const auto file = sm.add_virtual_file("main.aw", "let answer = add(1, 2);\n");
  Diagnostic_Engine diagnostics(sm);

  Compiler_Module_Name_Resolution resolution;
  resolution.status = Compiler_Module_Name_Resolution_Status::Wrong_Kind;
  resolution.module_name = "main.aw";
  resolution.symbol_name = "add";
  resolution.filter = Compiler_Module_Name_Filter::Function;
  resolution.binding.kind = Compiler_Module_Export_Kind::Global;

  (void) compiler_emit_module_name_resolution_diagnostic(
    diagnostics, sm.span(file, 13, 16), resolution, "SDK002", "test callee");
  ASSERT_EQ(diagnostics.size(), 1u);

  std::ostringstream out;
  diagnostics.render_plain(out);
  EXPECT_NE(out.str().find("error[SDK002]: invalid test callee 'add'"), std::string::npos);
  EXPECT_NE(out.str().find("resolves that name as Global"), std::string::npos);
}
