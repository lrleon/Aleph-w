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
 * @file fuzz_dynarray.cc
 * @brief Compare DynArray operations with a vector reference model.
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include <tpl_dynArray.H>

/**
 * @brief Exercise bounded sequences of DynArray mutations.
 * @param data Fuzzer-generated operation bytes.
 * @param size Number of available bytes.
 * @return Zero when every state matches the reference model.
 * @note Processes at most 256 operations and holds at most 64 elements.
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  Aleph::DynArray<int> array;
  std::vector<int> reference;

  for (size_t i = 0; i < std::min(size, size_t{256}); ++i)
    {
      const uint8_t byte = data[i];
      const int value = static_cast<int>(byte) - 128;

      switch (byte % 5)
        {
        case 0:
          if (reference.size() < 64)
            {
              array.append(value);
              reference.push_back(value);
            }
          break;
        case 1:
          if (not reference.empty())
            {
              if (array.pop() != reference.back())
                std::abort();
              reference.pop_back();
            }
          break;
        case 2:
          array.reverse();
          std::reverse(reference.begin(), reference.end());
          break;
        case 3:
          if (not reference.empty())
            {
              const size_t index = byte % reference.size();
              array(index) = value;
              reference[index] = value;
            }
          break;
        case 4:
          if (not reference.empty())
            {
              const size_t new_size = byte % (reference.size() + 1);
              array.cut(new_size);
              reference.resize(new_size);
            }
          break;
        }

      if (array.size() != reference.size())
        std::abort();
      for (size_t j = 0; j < reference.size(); ++j)
        if (array(j) != reference[j])
          std::abort();
    }

  return 0;
}
