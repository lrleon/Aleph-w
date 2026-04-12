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
 * @file compiler_scope_test.cc
 * @brief Tests for tpl_scope.H.
 */

# include <gtest/gtest.h>

# include <tpl_scope.H>

# include <string>

using namespace Aleph;


TEST(CompilerScope, EnterLookupAndShadowAcrossNestedScopes)
{
  Scope<std::string, int> scope;

  EXPECT_TRUE(scope.empty());
  EXPECT_EQ(scope.enter_scope(), 1u);
  EXPECT_FALSE(scope.empty());

  EXPECT_TRUE(scope.insert("x", 1));
  EXPECT_FALSE(scope.insert("x", 2));
  ASSERT_NE(scope.lookup("x"), nullptr);
  EXPECT_EQ(*scope.lookup("x"), 1);
  ASSERT_NE(scope.lookup_local("x"), nullptr);
  EXPECT_EQ(*scope.lookup_local("x"), 1);

  EXPECT_EQ(scope.enter_scope(), 2u);
  ASSERT_NE(scope.lookup("x"), nullptr);
  EXPECT_EQ(*scope.lookup("x"), 1);
  EXPECT_EQ(scope.lookup_local("x"), nullptr);

  EXPECT_TRUE(scope.insert("x", 2));
  ASSERT_NE(scope.lookup("x"), nullptr);
  EXPECT_EQ(*scope.lookup("x"), 2);
  ASSERT_NE(scope.lookup_local("x"), nullptr);
  EXPECT_EQ(*scope.lookup_local("x"), 2);

  scope.leave_scope();
  ASSERT_NE(scope.lookup("x"), nullptr);
  EXPECT_EQ(*scope.lookup("x"), 1);

  scope.leave_scope();
  EXPECT_TRUE(scope.empty());
  EXPECT_EQ(scope.lookup("x"), nullptr);
}


TEST(CompilerScope, ClearDropsAllFramesAndBindings)
{
  Scope<std::string, int> scope;
  scope.enter_scope();
  scope.insert("a", 10);
  scope.enter_scope();
  scope.insert("b", 20);

  ASSERT_NE(scope.lookup("a"), nullptr);
  ASSERT_NE(scope.lookup("b"), nullptr);

  scope.clear();

  EXPECT_TRUE(scope.empty());
  EXPECT_EQ(scope.lookup("a"), nullptr);
  EXPECT_EQ(scope.lookup("b"), nullptr);
}


TEST(CompilerScope, ReenteringScopeAfterLeaveKeepsBindingsUsable)
{
  Scope<std::string, int> scope;
  scope.enter_scope();
  ASSERT_TRUE(scope.insert("root", 1));

  scope.enter_scope();
  ASSERT_TRUE(scope.insert("first", 2));
  scope.leave_scope();

  ASSERT_NE(scope.lookup("root"), nullptr);
  EXPECT_EQ(*scope.lookup("root"), 1);

  scope.enter_scope();
  EXPECT_TRUE(scope.insert("second", 3));
  ASSERT_NE(scope.lookup("second"), nullptr);
  EXPECT_EQ(*scope.lookup("second"), 3);
  ASSERT_NE(scope.lookup("root"), nullptr);
  EXPECT_EQ(*scope.lookup("root"), 1);
}
