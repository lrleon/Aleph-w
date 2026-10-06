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
 * @file fuzz_sort.cc
 * @brief Compare Aleph sorting and indirect sorting with standard references.
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <numeric>
#include <vector>

#include <ahSort.H>

/**
 * @brief Check sorted values and stable index order for a byte sequence.
 * @param data Fuzzer-generated values.
 * @param size Number of available bytes.
 * @return Zero when all Aleph results match the references.
 * @note Uses at most 256 values; duplicate-heavy values test stability.
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  const size_t count = std::min(size, size_t{256});
  Aleph::Array<int> array;
  Aleph::DynArray<int> dynarray;
  std::vector<int> values;
  values.reserve(count);

  for (size_t i = 0; i < count; ++i)
    {
      const int value = static_cast<int>(data[i] % 17) - 8;
      array.append(value);
      dynarray.append(value);
      values.push_back(value);
    }

  auto ascending = values;
  std::sort(ascending.begin(), ascending.end());
  const auto sorted_array = Aleph::sort(array);
  Aleph::in_place_sort(dynarray);
  if (sorted_array.size() != count or dynarray.size() != count)
    std::abort();

  for (size_t i = 0; i < count; ++i)
    if (sorted_array(i) != ascending[i] or dynarray(i) != ascending[i]
        or array(i) != values[i])
      std::abort();

  auto descending = values;
  std::sort(descending.begin(), descending.end(), std::greater<int>{});
  const auto descending_array = Aleph::sort(array, std::greater<int>{});
  for (size_t i = 0; i < count; ++i)
    if (descending_array(i) != descending[i])
      std::abort();

  std::vector<size_t> expected_indices(count);
  std::iota(expected_indices.begin(), expected_indices.end(), 0);
  std::stable_sort(expected_indices.begin(), expected_indices.end(),
                   [&values](size_t a, size_t b)
                   {
                     return values[a] < values[b];
                   });
  const auto indices = Aleph::stable_argsort(array);
  if (indices.size() != count)
    std::abort();
  for (size_t i = 0; i < count; ++i)
    if (indices(i) != expected_indices[i])
      std::abort();

  return 0;
}
