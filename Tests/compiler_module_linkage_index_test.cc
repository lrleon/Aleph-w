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
 * @file compiler_module_linkage_index_test.cc
 * @brief Regression tests for descriptor-index propagation through module linkage.
 */

#include <gtest/gtest.h>

#include <Compiler_Module_Binder.H>

using namespace Aleph;

TEST(CompilerModuleBinder, PreservesDescriptorIndexesWhenMetadataOrderDiffers)
{
  DynArray<Compiler_Module_Descriptor> descriptors;
  Compiler_Module_Descriptor main_module;
  main_module.name = "main.aw";
  main_module.imports.append({"dep.aw", {}});
  descriptors.append(std::move(main_module));
  descriptors.append({"dep.aw", {}});

  DynArray<Compiler_Module_Metadata> metadata;
  Compiler_Module_Metadata dep_metadata;
  dep_metadata.name = "dep.aw";
  dep_metadata.exports.append({Compiler_Module_Export_Kind::Function, "add", {}});
  metadata.append(std::move(dep_metadata));

  Compiler_Module_Metadata main_metadata;
  main_metadata.name = "main.aw";
  main_metadata.exports.append({Compiler_Module_Export_Kind::Global, "answer", {}});
  metadata.append(std::move(main_metadata));

  DynArray<size_t> order;
  order.append(1);
  order.append(0);

  const auto linkage = compiler_link_module_surfaces(descriptors, metadata, order);
  ASSERT_TRUE(linkage.valid);
  ASSERT_EQ(linkage.modules.size(), 2u);

  const auto & linked_main = linkage.modules.access(1);
  ASSERT_EQ(linked_main.visible_exports.size(), 2u);
  EXPECT_EQ(linked_main.visible_exports.access(0).source_module_index, 0u);
  EXPECT_EQ(linked_main.visible_exports.access(0).source_module_name, "main.aw");
  EXPECT_EQ(linked_main.visible_exports.access(1).source_module_index, 1u);
  EXPECT_EQ(linked_main.visible_exports.access(1).source_module_name, "dep.aw");

  const auto bindings = compiler_bind_module_surfaces(linkage);
  ASSERT_TRUE(bindings.valid);
  ASSERT_EQ(bindings.modules.size(), 2u);

  const auto & bound_main = bindings.modules.access(1);
  ASSERT_EQ(bound_main.names.size(), 2u);
  EXPECT_EQ(bound_main.names.access(0).provider_module_index, 0u);
  EXPECT_EQ(bound_main.names.access(1).provider_module_index, 1u);
}
