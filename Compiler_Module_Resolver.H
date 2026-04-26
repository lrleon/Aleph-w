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

/** @file Compiler_Module_Resolver.H
 *  @brief Reusable source-name module dependency resolution helpers.
 *
 *  This header provides a deliberately small dependency resolver for frontends
 *  whose imports are identified by exact source names. It computes one stable
 *  merge order, reports missing dependencies and cycles structurally, and can
 *  render the resulting order as a deterministic text artifact.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_MODULE_RESOLVER_H
#define COMPILER_MODULE_RESOLVER_H

#include <sstream>
#include <string>

#include <ah-source.H>
#include <tpl_dynArray.H>

namespace Aleph {
/** @brief One source-name dependency edge originating from one module. */
struct Compiler_Module_Dependency
{
  std::string module_name;  ///< Exact imported source name.
  Source_Span span;         ///< Source location where the import was declared.
};

/** @brief One module descriptor consumed by the reusable resolver. */
struct Compiler_Module_Descriptor
{
  std::string name;                              ///< Stable module/source name.
  DynArray<Compiler_Module_Dependency> imports;  ///< Ordered dependency list.
};

/** @brief Structural issue kinds produced by module dependency resolution. */
enum class Compiler_Module_Resolution_Issue_Kind
{
  Missing_Dependency,
  Cyclic_Dependency
};

/** @brief One structured issue emitted by the reusable module resolver. */
struct Compiler_Module_Resolution_Issue
{
  Compiler_Module_Resolution_Issue_Kind kind
    = Compiler_Module_Resolution_Issue_Kind::Missing_Dependency;
  size_t module_index = 0;                            ///< Module whose import triggered the issue.
  size_t dependency_index = static_cast<size_t>(-1);  ///< Known dependency index when available.
  std::string module_name;                            ///< Stable importer name.
  std::string dependency_name;                        ///< Missing or cyclic dependency name.
  Source_Span span;  ///< Source location of the problematic import.
};

/** @brief Resolution result for one set of source-name module descriptors. */
struct Compiler_Module_Resolution_Result
{
  bool valid = true;       ///< `true` when no missing dependencies or cycles were found.
  DynArray<size_t> order;  ///< Stable dependency-first module order.
  DynArray<Compiler_Module_Resolution_Issue> issues;  ///< Structured issues, if any.
};

/** @brief Returns the stable debug name of one module-resolution issue kind. */
inline const char *compiler_module_resolution_issue_name(
  const Compiler_Module_Resolution_Issue_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Module_Resolution_Issue_Kind::Missing_Dependency:
      return "Missing_Dependency";
    case Compiler_Module_Resolution_Issue_Kind::Cyclic_Dependency:
      return "Cyclic_Dependency";
    }

  return "Unknown";
}

/** @brief Renders one deterministic text artifact for a module order. */
inline std::string compiler_render_module_order(const DynArray<size_t> &order,
                                                const DynArray<Compiler_Module_Descriptor> &modules)
{
  std::ostringstream out;
  out << "ImportOrder\n";
  for (size_t i = 0; i < order.size(); ++i)
    {
      const auto index = order.access(i);
      const auto name = index < modules.size() ? modules.access(index).name : "<invalid-module>";
      out << "  [" << i << "] " << name << '\n';
    }
  return out.str();
}

namespace Compiler_Module_Resolver_Detail {
inline size_t find_module_index(const DynArray<Compiler_Module_Descriptor> &modules,
                                const std::string &name) noexcept
{
  for (size_t i = 0; i < modules.size(); ++i)
    if (modules.access(i).name == name)
      return i;
  return static_cast<size_t>(-1);
}

inline bool resolve_visit(const DynArray<Compiler_Module_Descriptor> &modules,
                          const size_t index,
                          DynArray<unsigned char> &states,
                          Compiler_Module_Resolution_Result &result)
{
  const auto state = states.access(index);
  if (state == 2)
    return true;
  if (state == 1 or state == 3)
    return false;

  states.access(index) = 1;
  const auto &module = modules.access(index);
  for (size_t i = 0; i < module.imports.size(); ++i)
    {
      const auto &dep = module.imports.access(i);
      const auto dep_index = find_module_index(modules, dep.module_name);
      if (dep_index == static_cast<size_t>(-1))
        {
          result.valid = false;
          result.issues.append({Compiler_Module_Resolution_Issue_Kind::Missing_Dependency,
                                index,
                                static_cast<size_t>(-1),
                                module.name,
                                dep.module_name,
                                dep.span});
          continue;
        }

      if (dep_index == index)
        {
          result.valid = false;
          result.issues.append({Compiler_Module_Resolution_Issue_Kind::Cyclic_Dependency,
                                index,
                                dep_index,
                                module.name,
                                dep.module_name,
                                dep.span});
          continue;
        }

      if (states.access(dep_index) == 1)
        {
          result.valid = false;
          result.issues.append({Compiler_Module_Resolution_Issue_Kind::Cyclic_Dependency,
                                index,
                                dep_index,
                                module.name,
                                dep.module_name,
                                dep.span});
          states.access(index) = 3;
          return false;
        }

      if (states.access(dep_index) == 0 and not resolve_visit(modules, dep_index, states, result))
        {
          states.access(index) = 3;
          return false;
        }
    }

  states.access(index) = 2;
  result.order.append(index);
  return true;
}
}  // namespace Compiler_Module_Resolver_Detail

/** @brief Resolves one dependency-first order for exact-name module imports.
 *
 *  Missing dependencies are reported and skipped so the caller can still get
 *  a best-effort order. Cycles stop DFS along the offending path because no
 *  valid dependency-first ordering exists for that strongly connected region.
 */
inline Compiler_Module_Resolution_Result compiler_resolve_module_order(
  const DynArray<Compiler_Module_Descriptor> &modules)
{
  Compiler_Module_Resolution_Result result;
  DynArray<unsigned char> states;
  for (size_t i = 0; i < modules.size(); ++i)
    states.append(0);

  for (size_t i = 0; i < modules.size(); ++i)
    if (states.access(i) == 0)
      Compiler_Module_Resolver_Detail::resolve_visit(modules, i, states, result);

  return result;
}
}  // namespace Aleph

#endif
