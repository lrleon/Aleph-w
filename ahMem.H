/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
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

