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

/** @file Compiler_Module_Semantic_Environment.H
 *  @brief Reusable namespace-separated semantic environment for modules.
 *
 *  This header derives one semantic environment per module from
 *  `Compiler_Module_Name_Table.H`. Each environment exposes separate value and
 *  type namespaces so frontends can consume imported symbols without rebuilding
 *  that split from raw bindings on every run.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_MODULE_SEMANTIC_ENVIRONMENT_H
#define COMPILER_MODULE_SEMANTIC_ENVIRONMENT_H

#include <sstream>
#include <string>
#include <string_view>

#include <Compiler_Module_Name_Table.H>

namespace Aleph {
/** @brief One namespace-separated semantic environment for a module. */
struct Compiler_Module_Semantic_Module
{
  std::string module_name;
  size_t module_index = static_cast<size_t>(-1);
  size_t order_index = static_cast<size_t>(-1);
  DynArray<Compiler_Module_Bound_Name> values;
  DynArray<Compiler_Module_Bound_Name> types;
  DynArray<std::string> ambiguous_values;
  DynArray<std::string> ambiguous_types;
};

/** @brief Semantic environments for all modules in one compilation unit set. */
struct Compiler_Module_Semantic_Environment
{
  DynArray<Compiler_Module_Semantic_Module> modules;
};

namespace Compiler_Module_Semantic_Environment_Detail {
inline void append_unique_name(DynArray<std::string> &names, const std::string &name)
{
  for (size_t i = 0; i < names.size(); ++i)
    if (names.access(i) == name)
      return;
  names.append(name);
}
}  // namespace Compiler_Module_Semantic_Environment_Detail

/** @brief Builds one namespace-separated semantic environment from a name table.
 *
 *  @param table Per-module name table.
 *  @return Semantic environment with split value/type namespaces.
 */
inline Compiler_Module_Semantic_Environment compiler_build_module_semantic_environment(
  const Compiler_Module_Name_Table &table)
{
  Compiler_Module_Semantic_Environment env;

  for (size_t i = 0; i < table.modules.size(); ++i)
    {
      const auto &source = table.modules.access(i);
      Compiler_Module_Semantic_Module module;
      module.module_name = source.module_name;
      module.module_index = source.module_index;
      module.order_index = source.order_index;

      for (size_t j = 0; j < source.names.size(); ++j)
        {
          const auto &item = source.names.access(j);
          if (item.kind == Compiler_Module_Export_Kind::Type)
            module.types.append(item);
          else
            module.values.append(item);
        }

      for (size_t j = 0; j < source.ambiguous_names.size(); ++j)
        {
          const auto &item = source.ambiguous_names.access(j);
          if (item.namespace_kind == Compiler_Module_Export_Kind::Type)
            Compiler_Module_Semantic_Environment_Detail::append_unique_name(module.ambiguous_types,
                                                                            item.name);
          else
            Compiler_Module_Semantic_Environment_Detail::append_unique_name(module.ambiguous_values,
                                                                            item.name);
        }

      env.modules.append(std::move(module));
    }

  return env;
}

/** @brief Finds one semantic environment by module name. */
inline const Compiler_Module_Semantic_Module *compiler_find_module_semantic_environment(
  const Compiler_Module_Semantic_Environment &env, const std::string_view module_name) noexcept
{
  for (size_t i = 0; i < env.modules.size(); ++i)
    if (env.modules.access(i).module_name == module_name)
      return &env.modules.access(i);
  return nullptr;
}

/** @brief Looks up one visible value-namespace binding in a module environment. */
inline const Compiler_Module_Bound_Name *compiler_lookup_module_value(
  const Compiler_Module_Semantic_Environment &env,
  const std::string_view module_name,
  const std::string_view symbol_name) noexcept
{
  const auto *module = compiler_find_module_semantic_environment(env, module_name);
  if (module == nullptr)
    return nullptr;

  for (size_t i = 0; i < module->values.size(); ++i)
    if (module->values.access(i).name == symbol_name)
      return &module->values.access(i);
  return nullptr;
}

/** @brief Looks up one visible type-namespace binding in a module environment. */
inline const Compiler_Module_Bound_Name *compiler_lookup_module_type(
  const Compiler_Module_Semantic_Environment &env,
  const std::string_view module_name,
  const std::string_view symbol_name) noexcept
{
  const auto *module = compiler_find_module_semantic_environment(env, module_name);
  if (module == nullptr)
    return nullptr;

  for (size_t i = 0; i < module->types.size(); ++i)
    if (module->types.access(i).name == symbol_name)
      return &module->types.access(i);
  return nullptr;
}

/** @brief Renders one deterministic text artifact for semantic environments. */
inline std::string compiler_render_module_semantic_environment(
  const Compiler_Module_Semantic_Environment &env)
{
  std::ostringstream out;
  out << "ModuleSemanticEnvironment\n";
  for (size_t i = 0; i < env.modules.size(); ++i)
    {
      const auto &module = env.modules.access(i);
      out << "  Module(" << module.module_name << ") order=" << module.order_index << '\n';
      for (size_t j = 0; j < module.values.size(); ++j)
        {
          const auto &item = module.values.access(j);
          out << "    Value(" << compiler_module_export_kind_name(item.kind) << "(" << item.name
              << ")) -> " << item.provider_module_name << ' '
              << (item.imported ? "imported" : "local") << '\n';
        }
      for (size_t j = 0; j < module.types.size(); ++j)
        {
          const auto &item = module.types.access(j);
          out << "    Type(" << item.name << ") -> " << item.provider_module_name << ' '
              << (item.imported ? "imported" : "local") << '\n';
        }
      for (size_t j = 0; j < module.ambiguous_values.size(); ++j)
        out << "    AmbiguousValue(" << module.ambiguous_values.access(j) << ")\n";
      for (size_t j = 0; j < module.ambiguous_types.size(); ++j)
        out << "    AmbiguousType(" << module.ambiguous_types.access(j) << ")\n";
    }
  return out.str();
}
}  // namespace Aleph

#endif
