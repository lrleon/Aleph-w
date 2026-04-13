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
 * @file compiler_lexer_test.cc
 * @brief Tests for Compiler_Lexer.H.
 */

# include <gtest/gtest.h>

# include <Compiler_Lexer.H>

# include <vector>

using namespace Aleph;


namespace
{
  std::vector<Compiler_Token_Kind>
  collect_kinds(Compiler_Lexer & lexer)
  {
    std::vector<Compiler_Token_Kind> kinds;
    while (true)
      {
        const auto tok = lexer.next();
        kinds.push_back(tok.kind);
        if (tok.is_eof() or tok.is_invalid())
          break;
      }
    return kinds;
  }
}


TEST(CompilerLexer, LexesKeywordsIdentifiersNumbersAndSpans)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw",
                                      "fn add(x, y) { return x + 42; }\n");
  Diagnostic_Engine dx(sm);
  Compiler_Lexer lexer(sm, id, &dx);

  const auto t0 = lexer.next();
  const auto t1 = lexer.next();
  const auto t2 = lexer.next();
  const auto t3 = lexer.next();
  const auto t4 = lexer.next();
  const auto t5 = lexer.next();
  const auto t6 = lexer.next();
  const auto t7 = lexer.next();
  const auto t8 = lexer.next();
  const auto t9 = lexer.next();
  const auto t10 = lexer.next();
  const auto t11 = lexer.next();
  const auto t12 = lexer.next();
  const auto t13 = lexer.next();
  const auto t14 = lexer.next();

  EXPECT_EQ(t0.kind, Compiler_Token_Kind::Kw_Fn);
  EXPECT_EQ(t1.kind, Compiler_Token_Kind::Identifier);
  EXPECT_EQ(t1.lexeme, "add");
  EXPECT_EQ(t1.span.begin, 3u);
  EXPECT_EQ(t1.span.end, 6u);
  EXPECT_EQ(t2.kind, Compiler_Token_Kind::LParen);
  EXPECT_EQ(t3.kind, Compiler_Token_Kind::Identifier);
  EXPECT_EQ(t3.lexeme, "x");
  EXPECT_EQ(t4.kind, Compiler_Token_Kind::Comma);
  EXPECT_EQ(t5.kind, Compiler_Token_Kind::Identifier);
  EXPECT_EQ(t5.lexeme, "y");
  EXPECT_EQ(t6.kind, Compiler_Token_Kind::RParen);
  EXPECT_EQ(t7.kind, Compiler_Token_Kind::LBrace);
  EXPECT_EQ(t8.kind, Compiler_Token_Kind::Kw_Return);
  EXPECT_EQ(t9.kind, Compiler_Token_Kind::Identifier);
  EXPECT_EQ(t10.kind, Compiler_Token_Kind::Plus);
  EXPECT_EQ(t11.kind, Compiler_Token_Kind::Integer_Literal);
  EXPECT_EQ(t11.lexeme, "42");
  EXPECT_EQ(t11.span.begin, 26u);
  EXPECT_EQ(t11.span.end, 28u);
  EXPECT_EQ(t12.kind, Compiler_Token_Kind::Semicolon);
  EXPECT_EQ(t13.kind, Compiler_Token_Kind::RBrace);
  EXPECT_TRUE(t14.is_eof());

  EXPECT_FALSE(dx.has_errors());
}


TEST(CompilerLexer, PeekDoesNotConsumeTheNextToken)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "let value");
  Compiler_Lexer lexer(sm, id);

  const auto look1 = lexer.peek();
  const auto look2 = lexer.peek();
  const auto next = lexer.next();
  const auto ident = lexer.next();

  EXPECT_EQ(look1.kind, Compiler_Token_Kind::Kw_Let);
  EXPECT_EQ(look1.lexeme, "let");
  EXPECT_EQ(look2.kind, Compiler_Token_Kind::Kw_Let);
  EXPECT_EQ(next.kind, Compiler_Token_Kind::Kw_Let);
  EXPECT_EQ(ident.kind, Compiler_Token_Kind::Identifier);
  EXPECT_EQ(ident.lexeme, "value");
}


TEST(CompilerLexer, SkipsCommentsByDefault)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw",
                                      "let x = 1; // note\n/* block */ return x;");
  Diagnostic_Engine dx(sm);
  Compiler_Lexer lexer(sm, id, &dx);

  const auto kinds = collect_kinds(lexer);
  const std::vector<Compiler_Token_Kind> expected = {
    Compiler_Token_Kind::Kw_Let,
    Compiler_Token_Kind::Identifier,
    Compiler_Token_Kind::Assign,
    Compiler_Token_Kind::Integer_Literal,
    Compiler_Token_Kind::Semicolon,
    Compiler_Token_Kind::Kw_Return,
    Compiler_Token_Kind::Identifier,
    Compiler_Token_Kind::Semicolon,
    Compiler_Token_Kind::End_Of_File
  };

  EXPECT_EQ(kinds, expected);
  EXPECT_FALSE(dx.has_errors());
}


TEST(CompilerLexer, KeepsCommentTokensWhenRequested)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "// note\n/* block */");
  Compiler_Lexer_Options opts;
  opts.keep_comments = true;
  Compiler_Lexer lexer(sm, id, nullptr, opts);

  const auto t0 = lexer.next();
  const auto t1 = lexer.next();
  const auto t2 = lexer.next();

  EXPECT_EQ(t0.kind, Compiler_Token_Kind::Line_Comment);
  EXPECT_EQ(t0.lexeme, "// note");
  EXPECT_EQ(t1.kind, Compiler_Token_Kind::Block_Comment);
  EXPECT_EQ(t1.lexeme, "/* block */");
  EXPECT_TRUE(t2.is_eof());
}


TEST(CompilerLexer, CanDisableBlockCommentLexing)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "/*x*/");
  Compiler_Lexer_Options opts;
  opts.allow_block_comments = false;
  Compiler_Lexer lexer(sm, id, nullptr, opts);

  const auto kinds = collect_kinds(lexer);
  const std::vector<Compiler_Token_Kind> expected = {
    Compiler_Token_Kind::Slash,
    Compiler_Token_Kind::Star,
    Compiler_Token_Kind::Identifier,
    Compiler_Token_Kind::Star,
    Compiler_Token_Kind::Slash,
    Compiler_Token_Kind::End_Of_File
  };

  EXPECT_EQ(kinds, expected);
}


TEST(CompilerLexer, LexesStringsCharsAndCompoundOperators)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "'a' \"hi\\n\" += == && ->");
  Diagnostic_Engine dx(sm);
  Compiler_Lexer lexer(sm, id, &dx);

  const auto t0 = lexer.next();
  const auto t1 = lexer.next();
  const auto t2 = lexer.next();
  const auto t3 = lexer.next();
  const auto t4 = lexer.next();
  const auto t5 = lexer.next();
  const auto t6 = lexer.next();

  EXPECT_EQ(t0.kind, Compiler_Token_Kind::Char_Literal);
  EXPECT_EQ(t0.lexeme, "'a'");
  EXPECT_EQ(t1.kind, Compiler_Token_Kind::String_Literal);
  EXPECT_EQ(t1.lexeme, "\"hi\\n\"");
  EXPECT_EQ(t2.kind, Compiler_Token_Kind::PlusEq);
  EXPECT_EQ(t3.kind, Compiler_Token_Kind::EqEq);
  EXPECT_EQ(t4.kind, Compiler_Token_Kind::AndAnd);
  EXPECT_EQ(t5.kind, Compiler_Token_Kind::Arrow);
  EXPECT_TRUE(t6.is_eof());

  EXPECT_FALSE(dx.has_errors());
}


TEST(CompilerLexer, UnexpectedCharacterProducesInvalidTokenAndDiagnostic)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "@");
  Diagnostic_Engine dx(sm);
  Compiler_Lexer lexer(sm, id, &dx);

  const auto tok = lexer.next();

  EXPECT_TRUE(tok.is_invalid());
  EXPECT_EQ(tok.lexeme, "@");
  ASSERT_EQ(dx.size(), 1u);
  EXPECT_EQ(dx.get(0).code, "LEX001");
  EXPECT_EQ(dx.get(0).message, "unexpected character '@'");
}


TEST(CompilerLexer, UnterminatedStringProducesInvalidTokenAndDiagnostic)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "\"abc");
  Diagnostic_Engine dx(sm);
  Compiler_Lexer lexer(sm, id, &dx);

  const auto tok = lexer.next();

  EXPECT_TRUE(tok.is_invalid());
  EXPECT_EQ(tok.lexeme, "\"abc");
  ASSERT_EQ(dx.size(), 1u);
  EXPECT_EQ(dx.get(0).code, "LEX002");
  EXPECT_EQ(dx.get(0).message, "unterminated string literal");
}


TEST(CompilerLexer, UnterminatedBlockCommentIsNotSilentlyDropped)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "/* block");
  Diagnostic_Engine dx(sm);
  Compiler_Lexer lexer(sm, id, &dx);

  const auto tok = lexer.next();

  EXPECT_TRUE(tok.is_invalid());
  EXPECT_EQ(tok.lexeme, "/* block");
  ASSERT_EQ(dx.size(), 1u);
  EXPECT_EQ(dx.get(0).code, "LEX004");
  EXPECT_EQ(dx.get(0).message, "unterminated block comment");
}


TEST(CompilerLexer, MalformedCharacterLiteralProducesInvalidToken)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "'ab'");
  Diagnostic_Engine dx(sm);
  Compiler_Lexer lexer(sm, id, &dx);

  const auto tok = lexer.next();

  EXPECT_TRUE(tok.is_invalid());
  ASSERT_EQ(dx.size(), 1u);
  EXPECT_EQ(dx.get(0).code, "LEX003");
  EXPECT_EQ(dx.get(0).message, "malformed character literal");
}


TEST(CompilerLexer, EmptyInputReturnsEOFImmediately)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("empty.aw", "");
  Diagnostic_Engine dx(sm);
  Compiler_Lexer lexer(sm, id, &dx);

  // Empty input should report EOF immediately
  EXPECT_TRUE(lexer.eof());
  const auto tok = lexer.next();
  EXPECT_TRUE(tok.is_eof());
}


TEST(CompilerLexer, PublicAPIContract)
{
  Source_Manager sm;
  // "fn x" is 4 bytes (fn=0..1, space=2, x=3)
  const auto id = sm.add_virtual_file("api.aw", "fn x");
  Diagnostic_Engine dx(sm);
  Compiler_Lexer lexer(sm, id, &dx);

  // source_file_id() must match the file we registered
  EXPECT_EQ(lexer.source_file_id(), id);

  // At start, cursor is at 0
  EXPECT_EQ(lexer.current_offset(), 0u);
  EXPECT_FALSE(lexer.eof());

  // Consume "fn" token; offset should advance
  const auto tok_fn = lexer.next();
  EXPECT_EQ(tok_fn.kind, Compiler_Token_Kind::Kw_Fn);
  EXPECT_GT(lexer.current_offset(), 0u);

  // Consume "x" identifier
  const auto tok_x = lexer.next();
  EXPECT_EQ(tok_x.kind, Compiler_Token_Kind::Identifier);

  // Next token is EOF
  EXPECT_TRUE(lexer.eof());
  const auto tok_eof = lexer.next();
  EXPECT_TRUE(tok_eof.is_eof());

  // reset(0) restores cursor and lets us re-lex from the start
  lexer.reset(0);
  EXPECT_EQ(lexer.current_offset(), 0u);
  EXPECT_FALSE(lexer.eof());
  const auto tok_fn2 = lexer.next();
  EXPECT_EQ(tok_fn2.kind, Compiler_Token_Kind::Kw_Fn);

  // reset(offset) past end of file must throw
  const Source_Offset past_end = sm.file_text(id).size() + 1;
  EXPECT_THROW(lexer.reset(past_end), std::out_of_range);
}
