/*
                          Aleph_w

  Data structures & Algorithms
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
 * @file fuzz_compiler_lexer.cc
 * @brief Fuzz tokenization, lookahead, progress and source spans.
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>

#include <Compiler_Lexer.H>

/**
 * @brief Tokenize arbitrary source text and validate the lexer contract.
 * @param data Fuzzer-generated options byte followed by source text.
 * @param size Number of available bytes.
 * @return Zero when tokenization reaches EOF with valid tokens and spans.
 * @note Limits source text to 2048 bytes and token steps to source length + 1.
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  const uint8_t flags = size == 0 ? 0 : data[0];
  const size_t length = size == 0 ? 0 : std::min(size - 1, size_t{2048});
  const char *begin = size == 0 ? "" : reinterpret_cast<const char *>(data + 1);
  const std::string source(begin, length);

  Aleph::Source_Manager sources;
  const auto file_id = sources.add_virtual_file("fuzz.aw", source);
  Aleph::Diagnostic_Engine diagnostics(sources);
  Aleph::Compiler_Lexer_Options options;
  options.keep_comments = (flags & 1) != 0;
  options.allow_block_comments = (flags & 2) != 0;
  Aleph::Compiler_Lexer lexer(sources, file_id, &diagnostics, options);

  size_t previous_end = 0;
  for (size_t step = 0; step <= source.size(); ++step)
    {
      const auto before = lexer.current_offset();
      const auto looked_at = lexer.peek();
      const auto token = lexer.next();
      if (looked_at.kind != token.kind or looked_at.lexeme != token.lexeme
          or looked_at.span.file_id != token.span.file_id
          or looked_at.span.begin != token.span.begin
          or looked_at.span.end != token.span.end)
        std::abort();

      if (not token.span.is_valid() or token.span.file_id != file_id
          or token.span.begin < previous_end or token.span.end > source.size())
        std::abort();
      if (token.is_eof())
        {
          if (token.span.begin != source.size()
              or token.span.end != source.size())
            std::abort();
          return 0;
        }

      if (token.span.empty() or lexer.current_offset() <= before
          or token.lexeme != source.substr(token.span.begin, token.span.size()))
        std::abort();
      previous_end = token.span.end;
    }

  std::abort();
}
