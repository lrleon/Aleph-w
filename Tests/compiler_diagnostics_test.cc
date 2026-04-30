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
 * @file compiler_diagnostics_test.cc
 * @brief Tests for ah-diagnostics.H.
 */

# include <gtest/gtest.h>

# include <ah-diagnostics.H>

# include <sstream>
# include <string>

using namespace Aleph;


TEST(Diagnostics, ErrorBuilderStoresPrimaryLabelAndCode)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "let x = ;\n");
  Diagnostic_Engine dx(sm);

  const auto where = sm.span(id, 8, 9);
  dx.error(where, "expected expression")
      .code("E001")
      .note("parser recovered at ';'")
      .help("insert an expression after '='")
      .emit();

  ASSERT_EQ(dx.size(), 1u);
  EXPECT_EQ(dx.error_count(), 1u);
  EXPECT_EQ(dx.warning_count(), 0u);

  const auto & diag = dx.get(0);
  EXPECT_EQ(diag.code, "E001");
  EXPECT_EQ(diag.message, "expected expression");
  ASSERT_EQ(diag.labels.size(), 1u);
  EXPECT_EQ(diag.labels.access(0).kind, Diagnostic_Label_Kind::Primary);
  EXPECT_EQ(diag.notes.size(), 1u);
  EXPECT_EQ(diag.help.size(), 1u);
}


TEST(Diagnostics, SecondaryLabelsAndCountersWork)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "let x = y;\nlet y = 3;\n");
  Diagnostic_Engine dx(sm);

  dx.warning(sm.span(id, 8, 9), "use before declaration")
      .code("W001")
      .secondary(sm.span(id, 15, 16), "symbol declared here")
      .emit();

  ASSERT_EQ(dx.size(), 1u);
  EXPECT_EQ(dx.error_count(), 0u);
  EXPECT_EQ(dx.warning_count(), 1u);

  const auto & diag = dx.get(0);
  ASSERT_EQ(diag.labels.size(), 2u);
  EXPECT_EQ(diag.labels.access(1).kind, Diagnostic_Label_Kind::Secondary);
}


TEST(Diagnostics, RenderPlainIncludesFileLocationAndCaret)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "let x = ;\n");
  Diagnostic_Engine dx(sm);

  dx.error(sm.span(id, 8, 9), "expected expression")
      .code("E001")
      .note("parser recovered at ';'")
      .help("insert an expression after '='")
      .emit();

  std::ostringstream out;
  dx.render_plain(out);
  const std::string text = out.str();

  EXPECT_NE(text.find("error[E001]: expected expression"), std::string::npos);
  EXPECT_NE(text.find("main.aw:1:9"), std::string::npos);
  EXPECT_NE(text.find("1 | let x = ;"), std::string::npos);
  EXPECT_NE(text.find("^"), std::string::npos);
  EXPECT_NE(text.find("note: parser recovered at ';'"), std::string::npos);
  EXPECT_NE(text.find("help: insert an expression after '='"), std::string::npos);
}


TEST(Diagnostics, RenderingIsDeterministic)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "foo(\n");
  Diagnostic_Engine dx(sm);

  dx.warning(sm.span(id, 0, 3), "call target is unresolved")
      .code("W010")
      .emit();
  dx.error(sm.span(id, 3, 4), "expected ')'")
      .code("E010")
      .emit();

  std::ostringstream a;
  std::ostringstream b;
  dx.render_plain(a);
  dx.render_plain(b);

  EXPECT_EQ(a.str(), b.str());
}


TEST(Diagnostics, ClearResetsState)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "x");
  Diagnostic_Engine dx(sm);

  dx.error(sm.span(id, 0, 1), "bad token").emit();
  dx.warning(sm.span(id, 0, 1), "suspicious token").emit();

  ASSERT_EQ(dx.size(), 2u);
  ASSERT_EQ(dx.error_count(), 1u);
  ASSERT_EQ(dx.warning_count(), 1u);

  dx.clear();

  EXPECT_EQ(dx.size(), 0u);
  EXPECT_EQ(dx.error_count(), 0u);
  EXPECT_EQ(dx.warning_count(), 0u);
  EXPECT_FALSE(dx.has_errors());
}
