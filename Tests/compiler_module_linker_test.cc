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
 * @file compiler_module_linker_test.cc
 * @brief Tests for reusable module-surface linking.
 */

#include <gtest/gtest.h>

#include <Compiler_Module_Linker.H>

using namespace Aleph;

TEST(CompilerModuleLinker, LinksDirectImportSurfaces)
{
  DynArray<Compiler_Module_Descriptor> descriptors;
  Compiler_Module_Descriptor main_module;
  main_module.name = "main.aw";
  main_module.imports.append({"math.aw", {}});
  descriptors.append(std::move(main_module));
  descriptors.append({"math.aw", {}});

  DynArray<Compiler_Module_Metadata> metadata;
  Compiler_Module_Metadata main_metadata;
  main_metadata.name = "main.aw";
  main_metadata.exports.append({Compiler_Module_Export_Kind::Global, "answer", {}});
  metadata.append(std::move(main_metadata));

  Compiler_Module_Metadata math_metadata;
  math_metadata.name = "math.aw";
  math_metadata.exports.append({Compiler_Module_Export_Kind::Function, "add", {}});
  metadata.append(std::move(math_metadata));

  DynArray<size_t> order;
  order.append(1);
  order.append(0);

  const auto result = compiler_link_module_surfaces(descriptors, metadata, order);
  ASSERT_TRUE(result.valid);
  ASSERT_EQ(result.modules.size(), 2u);

  const auto & linked_main = result.modules.access(1);
  EXPECT_EQ(linked_main.module_name, "main.aw");
  ASSERT_EQ(linked_main.visible_exports.size(), 2u);
  EXPECT_EQ(linked_main.visible_exports.access(0).name, "answer");
  EXPECT_FALSE(linked_main.visible_exports.access(0).imported);
  EXPECT_EQ(linked_main.visible_exports.access(1).name, "add");
  EXPECT_TRUE(linked_main.visible_exports.access(1).imported);
}

TEST(CompilerModuleLinker, ReportsDuplicateVisibleValueExports)
{
  DynArray<Compiler_Module_Descriptor> descriptors;
  Compiler_Module_Descriptor main_module;
  main_module.name = "main.aw";
  main_module.imports.append({"a.aw", {}});
  main_module.imports.append({"b.aw", {}});
  descriptors.append(std::move(main_module));
  descriptors.append({"a.aw", {}});
  descriptors.append({"b.aw", {}});

  DynArray<Compiler_Module_Metadata> metadata;
  metadata.append({"main.aw", {}, {}});
  Compiler_Module_Metadata a;
  a.name = "a.aw";
  a.exports.append({Compiler_Module_Export_Kind::Function, "add", {}});
  metadata.append(std::move(a));
  Compiler_Module_Metadata b;
  b.name = "b.aw";
  b.exports.append({Compiler_Module_Export_Kind::Global, "add", {}});
  metadata.append(std::move(b));

  DynArray<size_t> order;
  order.append(1);
  order.append(2);
  order.append(0);

  const auto result = compiler_link_module_surfaces(descriptors, metadata, order);
  ASSERT_FALSE(result.valid);
  ASSERT_EQ(result.issues.size(), 1u);
  EXPECT_EQ(result.issues.access(0).kind,
            Compiler_Module_Linkage_Issue_Kind::Duplicate_Value_Export);
  EXPECT_EQ(result.issues.access(0).module_name, "main.aw");
  EXPECT_EQ(result.issues.access(0).export_name, "add");
}
