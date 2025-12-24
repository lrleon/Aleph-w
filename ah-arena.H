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

# ifndef AH_ARENA_H
# define AH_ARENA_H

# include <cstdlib>
# include <utility>
# include <aleph.H>
# include <ah-errors.H>

namespace Aleph
{

/** @brief Arena allocator for fast bump-pointer allocation.
 *
 *  AhArenaAllocator provides a simple and fast memory allocation strategy
 *  where allocations are satisfied by incrementing a pointer (bump allocation).
 *  Deallocations are only effective if they free the most recently allocated block.
 *
 *  This allocator is ideal for:
 *  - Temporary allocations with known lifetime
 *  - Building data structures that are freed all at once
 *  - Avoiding fragmentation in tight loops
 *
 *  @note Memory is automatically freed when the arena is destroyed.
 *  @note Not thread-safe. Use separate arenas per thread.
 *
 *  @ingroup Memoria
 */
class AhArenaAllocator
{
  char * _base_addr = nullptr;
  char * _curr_addr = nullptr;
  char * _end_addr = nullptr;
  bool _was_allocated = false;

public:

  /// Tag type for dispatching to templated allocation methods.
  enum class TemplateType { TEMPLATE };

  /** @brief Construct arena from existing memory buffer.
   *  @param[in] base_addr Pointer to pre-allocated buffer.
   *  @param[in] sz Size of the buffer in bytes.
   *  @note The caller retains ownership of the buffer.
   */
  AhArenaAllocator(char * base_addr, const size_t sz) noexcept
    : _base_addr(base_addr),
      _curr_addr(base_addr),
      _end_addr(base_addr + sz)
  {
    // empty
  }

  /** @brief Construct arena with internally allocated memory.
   *  @param[in] sz Size of arena in bytes (default 1MB).
   *  @throw std::runtime_error If memory allocation fails.
   */
  explicit AhArenaAllocator(const size_t sz = 1024 * 1024)
  {
    _base_addr = static_cast<char*>(std::malloc(sz));
    ah_runtime_error_if(_base_addr == nullptr)
      << "cannot allocate " << sz << " bytes";

    _curr_addr = _base_addr;
    _end_addr = _base_addr + sz;
    _was_allocated = true;
  }

  /** @brief Reset arena, making all memory available again.
   *  @note Does not call destructors - caller must manage object lifetimes.
   */
  void reset() noexcept { _curr_addr = _base_addr; }

  /** @brief Allocate raw memory from the arena.
   *  @param[in] sz Number of bytes to allocate.
   *  @return Pointer to allocated memory, or nullptr if insufficient space.
   */
  [[nodiscard]] void * alloc(const size_t sz) noexcept
  {
    if (sz == 0)
      return nullptr;

    char * ret_addr = _curr_addr;
    if (ret_addr + sz > _end_addr)  // > not >= to allow using last byte
      return nullptr;
    
    _curr_addr += sz;
    return ret_addr;
  }

  /** @brief Deallocate memory (only effective for last allocation).
   *  @param[in] addr Address to deallocate.
   *  @param[in] sz Size of allocation.
   *  @note Only frees if addr was the most recent allocation.
   */
  void dealloc(const void * addr, size_t sz = 0) noexcept
  {
    if (sz == 0)
      return;

    if (_curr_addr - sz == addr)
      _curr_addr -= sz;
  }

  /// Destructor. Frees internally allocated memory if any.
  virtual ~AhArenaAllocator()
  {
    if (_was_allocated && _base_addr != nullptr)
      std::free(_base_addr);
  }

  /// @brief Get total bytes currently allocated.
  [[nodiscard]] size_t allocated_size() const noexcept
  {
    return static_cast<size_t>(_curr_addr - _base_addr);
  }

  /// @brief Get remaining bytes available.
  [[nodiscard]] size_t available_size() const noexcept
  {
    return static_cast<size_t>(_end_addr - _curr_addr);
  }

  /// @brief Get total arena capacity.
  [[nodiscard]] size_t capacity() const noexcept
  {
    return static_cast<size_t>(_end_addr - _base_addr);
  }

  /** @brief Allocate and construct an object of type T.
   *  @tparam T Type to construct.
   *  @tparam Args Constructor argument types.
   *  @param[in] args Constructor arguments (forwarded).
   *  @return Pointer to constructed object, or nullptr if allocation fails.
   */
  template <typename T, typename... Args>
  [[nodiscard]] T * alloc(TemplateType, Args&&... args)
  {
    T * ptr = static_cast<T*>(alloc(sizeof(T)));
    if (ptr == nullptr)
      return nullptr;
    new (ptr) T(std::forward<Args>(args)...);
    return ptr;
  }

  /** @brief Destruct and deallocate an object.
   *  @tparam T Type of object.
   *  @param[in] ptr Pointer to object.
   */
  template <typename T>
  void dealloc(TemplateType, void * ptr) noexcept
  {
    static_cast<T*>(ptr)->~T();
    this->dealloc(ptr, sizeof(T));
  }

  /// @brief Get base address of the arena.
  [[nodiscard]] const void * base_addr() const noexcept { return _base_addr; }

  /// @brief Get next available address.
  [[nodiscard]] const void * next_avail_addr() const noexcept { return _curr_addr; }

  /// @brief Get end address (one past last valid byte).
  [[nodiscard]] const void * end_addr() const noexcept { return _end_addr; }

  // Non-copyable (owns raw memory)
  AhArenaAllocator(const AhArenaAllocator&) = delete;
  AhArenaAllocator& operator=(const AhArenaAllocator&) = delete;
};

/** @brief Allocate and construct an object in an arena.
 *  @tparam T Type to construct.
 *  @tparam Args Constructor argument types.
 *  @param[in] arena Arena allocator to use.
 *  @param[in] args Constructor arguments.
 *  @return Pointer to constructed object, or nullptr if allocation fails.
 */
template <class T, typename... Args>
[[nodiscard]] T* allocate(AhArenaAllocator & arena, Args&&... args)
{
  return arena.alloc<T>(AhArenaAllocator::TemplateType::TEMPLATE,
                        std::forward<Args>(args)...);
}

/** @brief Destruct and deallocate an object from an arena.
 *  @tparam T Type of object.
 *  @param[in] arena Arena that owns the object.
 *  @param[in] ptr Pointer to object.
 */
template <class T>
void deallocate(AhArenaAllocator & arena, T * ptr) noexcept
{
  arena.dealloc<T>(AhArenaAllocator::TemplateType::TEMPLATE, ptr);
}

} // end namespace Aleph

# endif
