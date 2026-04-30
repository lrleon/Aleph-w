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
 * @file compiler_module_metadata_test.cc
 * @brief Tests for reusable module-surface metadata.
 */

#include <gtest/gtest.h>

#include <Compiler_Module_Metadata.H>

using namespace Aleph;

TEST(CompilerModuleMetadata, RendersDeterministicModuleSurface)
{
  DynArray<Compiler_Module_Metadata> modules;

  Compiler_Module_Metadata module;
  module.name = "math.aw";
  module.exports.append({Compiler_Module_Export_Kind::Function, "add", {}});
  module.exports.append({Compiler_Module_Export_Kind::Global, "answer", {}});
  modules.append(std::move(module));

  const auto text = compiler_render_module_metadata(modules);
  EXPECT_NE(text.find("ModuleMetadata"), std::string::npos);
  EXPECT_NE(text.find("Module(math.aw)"), std::string::npos);
  EXPECT_NE(text.find("Function(add)"), std::string::npos);
  EXPECT_NE(text.find("Global(answer)"), std::string::npos);
}
