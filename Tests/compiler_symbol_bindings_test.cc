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
 * @file compiler_symbol_bindings_test.cc
 * @brief Tests for reusable lexical symbol bindings.
 */

#include <gtest/gtest.h>

#include <Compiler_Symbol_Bindings.H>

using namespace Aleph;

TEST(CompilerSymbolBindings, DeclaresVisibleSymbolsAcrossNestedScopes)
{
  Compiler_Symbol_Bindings bindings;
  bindings.enter_scope();

  const auto global = bindings.declare(Compiler_Symbol_Kind::Global, "value", {});
  ASSERT_TRUE(global.inserted());
  ASSERT_EQ(bindings.symbol_count(), 1u);

  bindings.enter_scope();
  const auto local = bindings.declare(Compiler_Symbol_Kind::Local, "temp", {});
  ASSERT_TRUE(local.inserted());

  const auto * value = bindings.lookup("value");
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(bindings.symbol(*value).kind, Compiler_Symbol_Kind::Global);

  const auto * temp = bindings.lookup("temp");
  ASSERT_NE(temp, nullptr);
  EXPECT_EQ(bindings.symbol(*temp).kind, Compiler_Symbol_Kind::Local);
}

TEST(CompilerSymbolBindings, ReportsDuplicatesAndShadowingStructurally)
{
  Compiler_Symbol_Bindings bindings;
  bindings.enter_scope();

  const auto first = bindings.declare(Compiler_Symbol_Kind::Global, "answer", {});
  ASSERT_TRUE(first.inserted());

  const auto duplicate = bindings.declare(Compiler_Symbol_Kind::Global, "answer", {});
  EXPECT_TRUE(duplicate.duplicate_local());
  EXPECT_EQ(duplicate.symbol_id, first.symbol_id);

  bindings.enter_scope();
  const auto shadow = bindings.declare(Compiler_Symbol_Kind::Local, "answer", {});
  EXPECT_TRUE(shadow.inserted());
  EXPECT_TRUE(shadow.shadowing());
  EXPECT_EQ(shadow.related_symbol_id, first.symbol_id);
}

TEST(CompilerSymbolBindings, DumpsSymbolsDeterministically)
{
  Compiler_Symbol_Bindings bindings;
  bindings.enter_scope();
  (void) bindings.declare(Compiler_Symbol_Kind::Function, "add", Source_Span{});
  bindings.enter_scope();
  (void) bindings.declare(Compiler_Symbol_Kind::Parameter, "x", Source_Span{});

  const auto dump = bindings.dump_symbols();
  EXPECT_NE(dump.find("Symbols"), std::string::npos);
  EXPECT_NE(dump.find("Function add"), std::string::npos);
  EXPECT_NE(dump.find("Parameter x"), std::string::npos);
}
