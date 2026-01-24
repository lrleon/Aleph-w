
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
 * @file ahMem.H
 * @brief Aleph-w memory allocation helpers and instrumentation hooks.
 *
 * This header centralizes Aleph's memory allocation conventions. 
 *
 * @section overview Overview
 * - Currently, it is a lightweight wrapper around `<cstddef>` and `<new>`, 
 *   enabling future expansion into custom allocators.
 * 
 * @section ahmem_usage Usage
 * @code
 * #include <ahMem.H>
 * 
 * void * ptr = Aleph::allocate(256);
 * Aleph::deallocate(ptr);
 * @endcode
 *
 * @ingroup Utilities
 * @author Leandro Rabindranath León
 */

#ifndef AHMEM_H
#define AHMEM_H

#include <cstddef>
#include <cstdlib>

#include <ah-errors.H>

namespace Aleph
{

  /** @brief Allocate @p size bytes using the C heap.
   *
   *  @param size Number of bytes to allocate. Must be greater than zero.
   *  @return Pointer to a block of @p size bytes.
   *  @throws std::invalid_argument when @p size == 0.
   *  @throws std::bad_alloc when the underlying allocation fails.
   */
  inline void * allocate(const std::size_t size)
  {
    ah_invalid_argument_if(size == 0)
      << "Aleph::allocate() expects size > 0";

    void * ptr = std::malloc(size);
    ah_bad_alloc_if(ptr == nullptr);

    return ptr;
  }

  /** @brief Allocate @p size zero-initialized bytes.
   *
   *  @param size Number of bytes to allocate and clear.
   *  @return Pointer to a zeroed block of @p size bytes.
   *  @throws std::invalid_argument when @p size == 0.
   *  @throws std::bad_alloc when the underlying allocation fails.
   */
  inline void * allocate_zeroed(const std::size_t size)
  {
    ah_invalid_argument_if(size == 0)
      << "Aleph::allocate_zeroed() expects size > 0";

    void * ptr = std::calloc(1u, size);
    ah_bad_alloc_if(ptr == nullptr);

    return ptr;
  }

  /** @brief Release memory previously obtained through Aleph allocation helpers.
   *
   *  @param ptr Pointer obtained from ::allocate or ::allocate_zeroed. Passing
   *             @c nullptr is a no-op (mirrors @c std::free semantics).
   */
  inline void deallocate(void * ptr) noexcept
  {
    std::free(ptr);
  }

} // namespace Aleph

#endif /* AHMEM_H */

