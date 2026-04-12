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
 * @file compiler_types_example.cc
 * @brief Minimal example that builds types and solves simple equality constraints.
 */

# include <iostream>

# include <tpl_constraints.H>

using namespace Aleph;

int main()
{
  Compiler_Type_Context ctx;
  const auto alpha = ctx.make_type_variable("A");
  const auto beta = ctx.make_type_variable("B");

  const auto expected =
      ctx.make_function_type({ctx.integer_type(), ctx.bool_type()},
                             ctx.string_type());
  const auto inferred =
      ctx.make_function_type({alpha, ctx.bool_type()}, beta);

  Compiler_Type_Constraint_Set constraints;
  constraints.add(inferred, expected, {}, "function signature match");

  Compiler_Type_Unifier unifier(ctx);
  const auto result = unifier.solve(constraints);

  std::cout << ctx.dump_types();
  std::cout << '\n';
  if (result.ok())
    {
      std::cout << unifier.dump_substitution();
      std::cout << '\n';
      std::cout << "Normalized inferred type: "
                << ctx.to_string(unifier.apply(inferred))
                << '\n';
      return 0;
    }

  std::cout << "Unification failed: "
            << compiler_unify_result_kind_name(result.kind)
            << " - " << result.message
            << '\n';
  return 1;
}
