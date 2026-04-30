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
 * @file compiler_module_name_table_example.cc
 * @brief Minimal example showing cached per-module name tables.
 */

#include <iostream>

#include <Compiler_Module_Name_Resolver.H>

using namespace Aleph;

int
main()
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

  const auto linkage = compiler_link_module_surfaces(descriptors, metadata, order);
  const auto bindings = compiler_bind_module_surfaces(linkage);
  const auto table = compiler_build_module_name_table(bindings);
  std::cout << compiler_render_module_name_table(table);
}
