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
 * @file fuzz_rle_parser.cc
 * @brief libFuzzer target for the RLE snapshot reader (`read_rle`).
 *
 * Malformed input is expected to be rejected via Aleph error macros
 * (exceptions); only memory-safety failures (OOB, UAF, integer overflow
 * leading to OOB) are real bugs and are caught by the address/undefined
 * sanitizers the fuzzer is built with.
 *
 * Build (see Tests/fuzz/CMakeLists.txt):
 *   clang++ -std=c++20 -fsanitize=fuzzer,address,undefined ...
 */

#include <cstddef>
#include <cstdint>
#include <string>

#include <ca-io.H>

using namespace Aleph;
using namespace Aleph::CA;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  const std::string s(reinterpret_cast<const char *>(data), size);
  try
    {
      const RLE_Pattern pattern = read_rle_string(s);
      (void) pattern;
    }
  catch (...)
    {
      // Rejecting malformed input is correct behavior, not a crash.
    }
  return 0;
}