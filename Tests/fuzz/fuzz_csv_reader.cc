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
 * @file fuzz_csv_reader.cc
 * @brief libFuzzer target for the CSV grid-snapshot reader
 *        (`read_csv_snapshot<int>`).
 *
 * Malformed input must be rejected via exceptions; only sanitizer-detected
 * memory errors are bugs.
 */

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>

#include <ca-io.H>

using namespace Aleph;
using namespace Aleph::CA;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  const std::string s(reinterpret_cast<const char *>(data), size);
  try
    {
      std::istringstream in(s);
      const Grid_Snapshot<int> snap = read_csv_snapshot<int>(in);
      (void) snap;
    }
  catch (...)
    {
    }
  return 0;
}