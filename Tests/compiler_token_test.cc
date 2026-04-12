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
 * @file compiler_token_test.cc
 * @brief Tests for Compiler_Token.H.
 */

# include <gtest/gtest.h>

# include <Compiler_Token.H>

using namespace Aleph;


TEST(CompilerToken, ClassifiesKeywordsAndPredicates)
{
  EXPECT_EQ(classify_compiler_keyword("fn"), Compiler_Token_Kind::Kw_Fn);
  EXPECT_EQ(classify_compiler_keyword("return"), Compiler_Token_Kind::Kw_Return);
  EXPECT_EQ(classify_compiler_keyword("identifier"),
            Compiler_Token_Kind::Identifier);

  EXPECT_TRUE(compiler_token_is_keyword(Compiler_Token_Kind::Kw_If));
  EXPECT_FALSE(compiler_token_is_keyword(Compiler_Token_Kind::Identifier));

  EXPECT_TRUE(compiler_token_is_literal(Compiler_Token_Kind::Integer_Literal));
  EXPECT_TRUE(compiler_token_is_literal(Compiler_Token_Kind::Kw_True));
  EXPECT_FALSE(compiler_token_is_literal(Compiler_Token_Kind::LBrace));

  EXPECT_TRUE(compiler_token_is_comment(Compiler_Token_Kind::Line_Comment));
  EXPECT_FALSE(compiler_token_is_comment(Compiler_Token_Kind::Slash));

  EXPECT_TRUE(compiler_token_is_operator(Compiler_Token_Kind::PlusEq));
  EXPECT_TRUE(compiler_token_is_operator(Compiler_Token_Kind::Arrow));
  EXPECT_FALSE(compiler_token_is_operator(Compiler_Token_Kind::Semicolon));
}


TEST(CompilerToken, PrecedenceAndAssociativityMatchParserMetadata)
{
  EXPECT_EQ(compiler_binary_precedence(Compiler_Token_Kind::Assign), 1);
  EXPECT_EQ(compiler_binary_precedence(Compiler_Token_Kind::OrOr), 2);
  EXPECT_EQ(compiler_binary_precedence(Compiler_Token_Kind::AndAnd), 3);
  EXPECT_EQ(compiler_binary_precedence(Compiler_Token_Kind::EqEq), 4);
  EXPECT_EQ(compiler_binary_precedence(Compiler_Token_Kind::LessEq), 5);
  EXPECT_EQ(compiler_binary_precedence(Compiler_Token_Kind::Plus), 6);
  EXPECT_EQ(compiler_binary_precedence(Compiler_Token_Kind::Star), 7);
  EXPECT_EQ(compiler_binary_precedence(Compiler_Token_Kind::Identifier), 0);

  EXPECT_EQ(compiler_binary_associativity(Compiler_Token_Kind::Assign),
            Compiler_Associativity::Right);
  EXPECT_EQ(compiler_binary_associativity(Compiler_Token_Kind::Plus),
            Compiler_Associativity::Left);
  EXPECT_EQ(compiler_binary_associativity(Compiler_Token_Kind::Identifier),
            Compiler_Associativity::None);
}


TEST(CompilerToken, ToStringIncludesKindLexemeAndSpan)
{
  Compiler_Token tok;
  tok.kind = Compiler_Token_Kind::Identifier;
  tok.lexeme = "value";
  tok.span = {1, 4, 9};

  EXPECT_EQ(tok.to_string(), "Identifier(\"value\")@#1:[4,9)");

  tok.kind = Compiler_Token_Kind::String_Literal;
  tok.lexeme = "\"hi\\n\"";
  tok.span = {1, 10, 16};

  EXPECT_EQ(tok.to_string(), "String_Literal(\"\\\"hi\\\\n\\\"\")@#1:[10,16)");
}
