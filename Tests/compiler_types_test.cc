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
 * @file compiler_types_test.cc
 * @brief Tests for Compiler_Types.H.
 */

# include <gtest/gtest.h>

# include <Compiler_Types.H>

using namespace Aleph;


TEST(CompilerTypes, BuiltinsAreStableAndPrettyPrinted)
{
  Compiler_Type_Context ctx;

  EXPECT_EQ(ctx.builtin_count(), 6u);
  EXPECT_EQ(ctx.to_string(ctx.invalid_type()), "Invalid");
  EXPECT_EQ(ctx.to_string(ctx.unit_type()), "Unit");
  EXPECT_EQ(ctx.to_string(ctx.bool_type()), "Bool");
  EXPECT_EQ(ctx.to_string(ctx.integer_type()), "Int");
  EXPECT_EQ(ctx.to_string(ctx.string_type()), "String");
  EXPECT_EQ(ctx.to_string(ctx.character_type()), "Char");
}


TEST(CompilerTypes, BuildsTuplesFunctionsAndTypeVariables)
{
  Compiler_Type_Context ctx;

  const auto t0 = ctx.make_type_variable();
  const auto elem = ctx.make_type_variable("Elem");
  const auto tuple = ctx.make_tuple_type({ctx.integer_type(), ctx.bool_type(), elem});
  const auto fn = ctx.make_function_type({tuple, t0}, ctx.character_type());

  EXPECT_TRUE(ctx.is_type_variable(t0));
  EXPECT_EQ(ctx.to_string(t0), "T0");
  EXPECT_EQ(ctx.to_string(elem), "Elem");
  EXPECT_EQ(ctx.to_string(tuple), "(Int, Bool, Elem)");
  EXPECT_EQ(ctx.to_string(fn), "fn((Int, Bool, Elem), T0) -> Char");
}


TEST(CompilerTypes, ClearKeepsBuiltinsAndResetsFreshVariables)
{
  Compiler_Type_Context ctx;
  const auto before = ctx.make_type_variable();
  ASSERT_EQ(ctx.to_string(before), "T0");

  ctx.clear();

  EXPECT_EQ(ctx.builtin_count(), 6u);
  EXPECT_EQ(ctx.size(), 6u);
  const auto after = ctx.make_type_variable();
  EXPECT_EQ(ctx.to_string(after), "T0");
}


TEST(CompilerTypes, BuildsNominalStructsAndEnums)
{
  Compiler_Type_Context ctx;

  const auto point = ctx.make_struct_type("Point");
  DynArray<std::string> field_names;
  field_names.append("x");
  field_names.append("y");
  DynArray<Compiler_Type_Id> field_types;
  field_types.append(ctx.integer_type());
  field_types.append(ctx.integer_type());
  ctx.set_struct_fields(point, field_names, field_types);

  const auto color = ctx.make_enum_type("Color");
  DynArray<std::string> variants;
  variants.append("Red");
  variants.append("Green");
  variants.append("Blue");
  ctx.set_enum_variants(color, variants);

  EXPECT_TRUE(ctx.is_struct(point));
  EXPECT_TRUE(ctx.is_enum(color));
  EXPECT_EQ(ctx.to_string(point), "Point");
  EXPECT_EQ(ctx.to_string(color), "Color");
  EXPECT_EQ(ctx.type(point).member_names.size(), 2u);
  EXPECT_EQ(ctx.type(point).member_names.access(0), "x");
  EXPECT_EQ(ctx.type(point).components.access(1), ctx.integer_type());
  EXPECT_EQ(ctx.type(color).member_names.size(), 3u);
  EXPECT_EQ(ctx.type(color).member_names.access(2), "Blue");
}
