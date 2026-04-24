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
 * @file compiler_module_resolver_test.cc
 * @brief Tests for reusable module dependency resolution.
 */

#include <gtest/gtest.h>

#include <Compiler_Module_Resolver.H>

using namespace Aleph;

TEST(CompilerModuleResolver, ProducesDependencyFirstOrder)
{
  DynArray<Compiler_Module_Descriptor> modules;

  Compiler_Module_Descriptor main_module;
  main_module.name = "main.aw";
  main_module.imports.append({"math.aw", {}});
  modules.append(std::move(main_module));

  Compiler_Module_Descriptor math_module;
  math_module.name = "math.aw";
  modules.append(std::move(math_module));

  const auto result = compiler_resolve_module_order(modules);
  ASSERT_TRUE(result.valid);
  ASSERT_TRUE(result.issues.is_empty());
  ASSERT_EQ(result.order.size(), 2u);
  EXPECT_EQ(modules.access(result.order.access(0)).name, "math.aw");
  EXPECT_EQ(modules.access(result.order.access(1)).name, "main.aw");

  const auto artifact = compiler_render_module_order(result.order, modules);
  EXPECT_EQ(artifact,
            "ImportOrder\n"
            "  [0] math.aw\n"
            "  [1] main.aw\n");
}

TEST(CompilerModuleResolver, ReportsMissingDependenciesStructurally)
{
  DynArray<Compiler_Module_Descriptor> modules;

  Compiler_Module_Descriptor main_module;
  main_module.name = "main.aw";
  main_module.imports.append({"missing.aw", {}});
  modules.append(std::move(main_module));

  const auto result = compiler_resolve_module_order(modules);
  ASSERT_FALSE(result.valid);
  ASSERT_EQ(result.issues.size(), 1u);
  EXPECT_EQ(result.issues.access(0).kind,
            Compiler_Module_Resolution_Issue_Kind::Missing_Dependency);
  EXPECT_EQ(result.issues.access(0).module_name, "main.aw");
  EXPECT_EQ(result.issues.access(0).dependency_name, "missing.aw");
}

TEST(CompilerModuleResolver, ReportsCyclesStructurally)
{
  DynArray<Compiler_Module_Descriptor> modules;

  Compiler_Module_Descriptor a;
  a.name = "a.aw";
  a.imports.append({"b.aw", {}});
  modules.append(std::move(a));

  Compiler_Module_Descriptor b;
  b.name = "b.aw";
  b.imports.append({"a.aw", {}});
  modules.append(std::move(b));

  const auto result = compiler_resolve_module_order(modules);
  ASSERT_FALSE(result.valid);
  ASSERT_EQ(result.issues.size(), 1u);
  EXPECT_EQ(result.issues.access(0).kind,
            Compiler_Module_Resolution_Issue_Kind::Cyclic_Dependency);
  EXPECT_EQ(result.issues.access(0).module_name, "b.aw");
  EXPECT_EQ(result.issues.access(0).dependency_name, "a.aw");
}

TEST(CompilerModuleResolver, KeepsResolvingUnrelatedModulesAfterCycle)
{
  DynArray<Compiler_Module_Descriptor> modules;

  Compiler_Module_Descriptor a;
  a.name = "a.aw";
  a.imports.append({"b.aw", {}});
  modules.append(std::move(a));

  Compiler_Module_Descriptor b;
  b.name = "b.aw";
  b.imports.append({"a.aw", {}});
  modules.append(std::move(b));

  Compiler_Module_Descriptor util;
  util.name = "util.aw";
  modules.append(std::move(util));

  Compiler_Module_Descriptor main_module;
  main_module.name = "main.aw";
  main_module.imports.append({"util.aw", {}});
  modules.append(std::move(main_module));

  const auto result = compiler_resolve_module_order(modules);
  ASSERT_FALSE(result.valid);
  ASSERT_EQ(result.issues.size(), 1u);
  EXPECT_EQ(result.issues.access(0).kind,
            Compiler_Module_Resolution_Issue_Kind::Cyclic_Dependency);

  ASSERT_EQ(result.order.size(), 2u);
  EXPECT_EQ(modules.access(result.order.access(0)).name, "util.aw");
  EXPECT_EQ(modules.access(result.order.access(1)).name, "main.aw");
}

TEST(CompilerModuleResolver, ReportsSelfImportsAsCycles)
{
  DynArray<Compiler_Module_Descriptor> modules;

  Compiler_Module_Descriptor self_module;
  self_module.name = "self.aw";
  self_module.imports.append({"self.aw", {}});
  modules.append(std::move(self_module));

  const auto result = compiler_resolve_module_order(modules);
  ASSERT_FALSE(result.valid);
  ASSERT_EQ(result.issues.size(), 1u);
  EXPECT_EQ(result.issues.access(0).kind,
            Compiler_Module_Resolution_Issue_Kind::Cyclic_Dependency);
  EXPECT_EQ(result.issues.access(0).module_name, "self.aw");
  EXPECT_EQ(result.issues.access(0).dependency_name, "self.aw");

  ASSERT_EQ(result.order.size(), 1u);
  EXPECT_EQ(modules.access(result.order.access(0)).name, "self.aw");
}
