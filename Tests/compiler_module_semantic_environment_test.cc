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
 * @file compiler_module_semantic_environment_test.cc
 * @brief Tests for reusable namespace-separated module environments.
 */

#include <gtest/gtest.h>

#include <Compiler_Module_Semantic_Environment.H>

using namespace Aleph;

namespace
{
  Compiler_Module_Semantic_Environment
  make_environment()
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
    main_metadata.exports.append({Compiler_Module_Export_Kind::Type, "Answer", {}});
    metadata.append(std::move(main_metadata));

    Compiler_Module_Metadata math_metadata;
    math_metadata.name = "math.aw";
    math_metadata.exports.append({Compiler_Module_Export_Kind::Function, "add", {}});
    metadata.append(std::move(math_metadata));

    DynArray<size_t> order;
    order.append(1);
    order.append(0);

    const auto linkage = compiler_link_module_surfaces(descriptors, metadata, order);
    const auto bindings = compiler_bind_module_surfaces(linkage);
    const auto table = compiler_build_module_name_table(bindings);
    return compiler_build_module_semantic_environment(table);
  }
}

TEST(CompilerModuleSemanticEnvironment, SplitsValueAndTypeNamespaces)
{
  const auto env = make_environment();
  ASSERT_EQ(env.modules.size(), 2u);

  const auto * main_module = compiler_find_module_semantic_environment(env, "main.aw");
  ASSERT_NE(main_module, nullptr);
  ASSERT_EQ(main_module->values.size(), 2u);
  ASSERT_EQ(main_module->types.size(), 1u);
  EXPECT_EQ(main_module->types.access(0).name, "Answer");
}

TEST(CompilerModuleSemanticEnvironment, SupportsNamespaceSpecificLookup)
{
  const auto env = make_environment();
  const auto * add = compiler_lookup_module_value(env, "main.aw", "add");
  ASSERT_NE(add, nullptr);
  EXPECT_EQ(add->provider_module_name, "math.aw");

  const auto * answer_type = compiler_lookup_module_type(env, "main.aw", "Answer");
  ASSERT_NE(answer_type, nullptr);
  EXPECT_EQ(answer_type->provider_module_name, "main.aw");
}
