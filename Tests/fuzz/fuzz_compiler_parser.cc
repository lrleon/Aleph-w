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
 * @file fuzz_compiler_parser.cc
 * @brief Fuzz recursive-descent parsing and malformed-source recovery.
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>

#include <Compiler_Parser.H>

/**
 * @brief Parse arbitrary bounded source text and validate the module span.
 * @param data Fuzzer-generated options byte followed by source text.
 * @param size Number of available bytes.
 * @return Zero when parsing completes with a valid module range.
 * @note Limits source text to 512 bytes to bound recursion and allocation.
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  const uint8_t flags = size == 0 ? 0 : data[0];
  const size_t length = size == 0 ? 0 : std::min(size - 1, size_t{512});
  const char *begin = size == 0 ? "" : reinterpret_cast<const char *>(data + 1);
  const std::string source(begin, length);

  Aleph::Source_Manager sources;
  const auto file_id = sources.add_virtual_file("fuzz.aw", source);
  Aleph::Diagnostic_Engine diagnostics(sources);
  Aleph::Compiler_Ast_Context ast(1 << 15);
  Aleph::Compiler_Parser_Options options;
  options.allow_top_level_statements = (flags & 1) != 0;
  Aleph::Compiler_Parser parser(ast, sources, file_id, &diagnostics, options);
  const auto *module = parser.parse_module();

  if (module == nullptr or not module->span.is_valid()
      or module->span.file_id != file_id
      or module->span.end > source.size())
    std::abort();

  return 0;
}
