
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
 * @file ah-arena.H
 * @brief Memory arena for fast bulk allocations.
 *
 * Provides an arena allocator that allocates memory using bump-pointer
 * allocation, enabling O(1) allocations with minimal fragmentation.
 * Memory is freed all at once when the arena is destroyed or reset.
 *
 * ## Overview
 *
 * Arena allocators (also known as region-based allocators) are useful when:
 * - Many small objects have the same lifetime
 * - Allocation speed is critical
 * - Memory fragmentation must be avoided
 * - Individual deallocation is not needed
 *
 * ## Usage Example
 *
 * ```cpp
 * // Create arena with 1MB of memory
 * AhArenaAllocator arena(1024 * 1024);
 *
 * // Allocate raw memory
 * void* buf = arena.alloc(100);
 *
 * // Allocate and construct objects
 * MyClass* obj = allocate<MyClass>(arena, arg1, arg2);
 *
 * // Reset arena to reuse memory (no destructors called!)
 * arena.reset();
 *
 * // Or use external buffer
 * char buffer[4096];
 * AhArenaAllocator stack_arena(buffer, sizeof(buffer));
 * ```
 *
 * ## Thread Safety
 *
 * This allocator is **not thread-safe**. For multi-threaded applications,
 * use one arena per thread or add external synchronization.
 *
 * @ingroup Memory
 * @author Leandro Rabindranath León
 */

# ifndef AH_ARENA_H
# define AH_ARENA_H

# include <cstdlib>
# include <cstddef>
# include <utility>
# include <new>
# include <aleph.H>
# include <ah-errors.H>

namespace Aleph
{

/**
 * @brief Arena allocator for fast bump-pointer allocation.
 *
 * AhArenaAllocator provides a simple and fast memory allocation strategy
 * where allocations are satisfied by incrementing a pointer ("bump" allocation).
 * Individual deallocations are only effective for LIFO (stack-like) patterns.
 *
 * ### Performance Characteristics
 *
 * | Operation | Complexity |
 * |-----------|------------|
 * | alloc()   | O(1)       |
 * | dealloc() | O(1)       |
 * | reset()   | O(1)       |
 *
 * ### Memory Layout
 *
 * ```
 * |<------- capacity() ------->|
 * |<-- allocated -->|<- avail ->|
 * [=================[           ]
 * ^                 ^           ^
 * base_addr    curr_addr    end_addr
 * ```
 *
 * @note Memory is automatically freed when the arena is destroyed.
 * @note Not thread-safe. Use separate arenas per thread.
 *
 * @ingroup Memory
 */
class AhArenaAllocator
{
  char* base_addr_ = nullptr;
  char* curr_addr_ = nullptr;
  char* end_addr_ = nullptr;
  bool owns_memory_ = false;

public:
  /// Tag type for dispatching to templated allocation methods.
  enum class TemplateType { TEMPLATE };

  /// Default arena size (1 MB).
  static constexpr size_t DEFAULT_SIZE = 1024 * 1024;

  /**
   * @brief Construct arena from existing memory buffer.
   *
   * The arena will use the provided buffer but will NOT free it on
   * destruction. The caller retains ownership of the buffer.
   *
   * @param[in] buffer Pointer to pre-allocated buffer.
   * @param[in] size Size of the buffer in bytes.
   *
   * @note The buffer must remain valid for the lifetime of the arena.
   * @note Passing a const buffer is allowed but the arena will write to it.
   */
  AhArenaAllocator(void* buffer, const size_t size) noexcept
    : base_addr_(static_cast<char*>(buffer)),
      curr_addr_(base_addr_),
      end_addr_(base_addr_ + size)
  {}

  /// @brief Overload for const buffer (uses const_cast internally).
  AhArenaAllocator(const char* buffer, const size_t size) noexcept
    : base_addr_(const_cast<char*>(buffer)),
      curr_addr_(base_addr_),
      end_addr_(base_addr_ + size)
  {}

  /**
   * @brief Construct arena with internally allocated memory.
   *
   * Allocates a contiguous block of memory using malloc(). The memory
   * is automatically freed when the arena is destroyed.
   *
   * @param[in] size Size of arena in bytes (default 1MB).
   * @throw std::runtime_error If memory allocation fails.
   */
  explicit AhArenaAllocator(const size_t size = DEFAULT_SIZE)
  {
    base_addr_ = static_cast<char*>(std::malloc(size));
    ah_runtime_error_if(base_addr_ == nullptr)
      << "AhArenaAllocator: cannot allocate " << size << " bytes";

    curr_addr_ = base_addr_;
    end_addr_ = base_addr_ + size;
    owns_memory_ = true;
  }

  /**
   * @brief Destructor. Frees internally allocated memory if any.
   *
   * @warning Does NOT call destructors on objects allocated in the arena.
   *          The caller must ensure all objects are properly destroyed
   *          before the arena is destroyed.
   */
  ~AhArenaAllocator()
  {
    if (owns_memory_ and base_addr_ != nullptr)
      std::free(base_addr_);
  }

  // === Move semantics ===

  /**
   * @brief Move constructor.
   *
   * Transfers ownership of the arena's memory. The moved-from arena
   * becomes empty and invalid for further allocations.
   */
  AhArenaAllocator(AhArenaAllocator&& other) noexcept
    : base_addr_(other.base_addr_),
      curr_addr_(other.curr_addr_),
      end_addr_(other.end_addr_),
      owns_memory_(other.owns_memory_)
  {
    other.base_addr_ = nullptr;
    other.curr_addr_ = nullptr;
    other.end_addr_ = nullptr;
    other.owns_memory_ = false;
  }

  /**
   * @brief Move assignment operator.
   *
   * Frees current memory (if owned) and takes ownership from other.
   */
  AhArenaAllocator& operator=(AhArenaAllocator&& other) noexcept
  {
    if (this != &other)
      {
        // Free current memory if we own it
        if (owns_memory_ && base_addr_ != nullptr)
          std::free(base_addr_);

        // Take ownership from other
        base_addr_ = other.base_addr_;
        curr_addr_ = other.curr_addr_;
        end_addr_ = other.end_addr_;
        owns_memory_ = other.owns_memory_;

        // Invalidate other
        other.base_addr_ = nullptr;
        other.curr_addr_ = nullptr;
        other.end_addr_ = nullptr;
        other.owns_memory_ = false;
      }
    return *this;
  }

  // Non-copyable (owns raw memory)
  AhArenaAllocator(const AhArenaAllocator&) = delete;
  AhArenaAllocator& operator=(const AhArenaAllocator&) = delete;

  // === Allocation interface ===

  /**
   * @brief Reset arena, making all memory available again.
   *
   * After reset, the arena can be reused for new allocations.
   * All previously returned pointers become invalid.
   *
   * @warning Does NOT call destructors on allocated objects.
   *          The caller must destroy objects manually before calling reset().
   */
  void reset() noexcept { curr_addr_ = base_addr_; }

  /**
   * @brief Allocate raw memory from the arena.
   *
   * Returns a pointer to at least `size` bytes of memory, or nullptr
   * if insufficient space remains.
   *
   * @param[in] size Number of bytes to allocate.
   * @return Pointer to allocated memory, or nullptr if insufficient space.
   *
   * @note Memory is NOT zero-initialized.
   * @note Alignment is not guaranteed beyond natural alignment of char.
   *       Use alloc_aligned() for specific alignment requirements.
   */
  [[nodiscard]] void* alloc(const size_t size) noexcept
  {
    if (size == 0)
      return nullptr;

    if (const auto avail = static_cast<size_t>(end_addr_ - curr_addr_); size > avail)
      return nullptr;

    char* result = curr_addr_;
    curr_addr_ += size;
    return result;
  }

  /**
   * @brief Allocate aligned memory from the arena.
   *
   * Returns a pointer to at least `size` bytes of memory aligned to
   * `alignment` bytes, or nullptr if insufficient space.
   *
   * @param[in] size Number of bytes to allocate.
   * @param[in] alignment Required alignment (must be power of 2).
   * @return Pointer to aligned memory, or nullptr if insufficient space.
   */
  [[nodiscard]] void* alloc_aligned(const size_t size, const size_t alignment) noexcept
  {
    if (size == 0 or alignment == 0)
      return nullptr;

    // Calculate aligned address
    const auto current = reinterpret_cast<uintptr_t>(curr_addr_);
    const uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);

    const auto padding = aligned - current;
    const auto avail = static_cast<size_t>(end_addr_ - curr_addr_);
    if (padding > avail)
      return nullptr;
    if (size > (avail - padding))
      return nullptr;

    curr_addr_ = reinterpret_cast<char*>(aligned) + size;
    return reinterpret_cast<void*>(aligned);
  }

  /**
   * @brief Deallocate memory (only effective for LIFO pattern).
   *
   * If the address matches the most recently allocated block (and the
   * size is correct), the memory is reclaimed. Otherwise, this is a no-op.
   *
   * @param[in] addr Address to deallocate.
   * @param[in] size Size of the allocation.
   *
   * @note This enables stack-like (LIFO) allocation patterns where
   *       the last allocation can be "undone".
   */
  void dealloc(const void* addr, const size_t size) noexcept
  {
    if (size == 0 or addr == nullptr)
      return;

    // Only deallocate if this was the last allocation (LIFO)
    if (static_cast<const char*>(addr) + size == curr_addr_)
      curr_addr_ -= size;
  }

  // === Typed allocation ===

  /**
   * @brief Allocate and construct an object of type T.
   *
   * Allocates sizeof(T) bytes and constructs T in-place using
   * placement new with the provided arguments.
   *
   * @tparam T Type to construct.
   * @tparam Args Constructor argument types.
   * @param[in] tag Disambiguation tag (use TemplateType::TEMPLATE) - unused parameter.
   * @param[in] args Constructor arguments (forwarded).
   * @return Pointer to constructed object, or nullptr if allocation fails.
   *
   * @note If allocation fails, no object is constructed and nullptr is returned.
  */
  template <typename T, typename... Args>
  [[nodiscard]] T* alloc([[maybe_unused]] TemplateType tag, Args&&... args)
  {
    void* ptr = alloc_aligned(sizeof(T), alignof(T));
    if (ptr == nullptr)
      return nullptr;
    return new (ptr) T(std::forward<Args>(args)...);
  }

  /**
   * @brief Destruct and deallocate an object.
   *
   * Calls the destructor of T and attempts to reclaim the memory
   * (only succeeds for LIFO deallocation pattern).
   *
   * @tparam T Type of object.
   * @param[in] tag Disambiguation tag (use TemplateType::TEMPLATE) - unused parameter.
   * @param[in] ptr Pointer to object.
   */
  template <typename T>
  void dealloc([[maybe_unused]] TemplateType tag, void* ptr) noexcept
  {
    if (ptr == nullptr)
      return;
    static_cast<T*>(ptr)->~T();
    dealloc(ptr, sizeof(T));
  }

  // === Query interface ===

  /// @brief Returns true if the arena is valid (has memory).
  [[nodiscard]] bool is_valid() const noexcept
  {
    return base_addr_ != nullptr;
  }

  /// @brief Returns true if the arena owns its memory.
  [[nodiscard]] bool owns_memory() const noexcept
  {
    return owns_memory_;
  }

  /// @brief Get total bytes currently allocated.
  [[nodiscard]] size_t allocated_size() const noexcept
  {
    return static_cast<size_t>(curr_addr_ - base_addr_);
  }

  /// @brief Get remaining bytes available.
  [[nodiscard]] size_t available_size() const noexcept
  {
    return static_cast<size_t>(end_addr_ - curr_addr_);
  }

  /// @brief Get total arena capacity.
  [[nodiscard]] size_t capacity() const noexcept
  {
    return static_cast<size_t>(end_addr_ - base_addr_);
  }

  /// @brief Returns true if the arena is empty (no allocations).
  [[nodiscard]] bool empty() const noexcept
  {
    return curr_addr_ == base_addr_;
  }

  /// @brief Returns true if the arena is full (no space left).
  [[nodiscard]] bool full() const noexcept
  {
    return curr_addr_ == end_addr_;
  }

  /// @brief Get base address of the arena.
  [[nodiscard]] const void* base_addr() const noexcept { return base_addr_; }

  /// @brief Get next available address (current allocation pointer).
  [[nodiscard]] const void* next_avail_addr() const noexcept { return curr_addr_; }

  /// @brief Get end address (one past last valid byte).
  [[nodiscard]] const void* end_addr() const noexcept { return end_addr_; }

  /**
   * @brief Check if a pointer is within this arena's range.
   *
   * @param[in] ptr Pointer to check.
   * @return true if ptr is within [base_addr, end_addr).
   */
  [[nodiscard]] bool contains(const void* ptr) const noexcept
  {
    const auto p = static_cast<const char*>(ptr);
    return p >= base_addr_ and p < end_addr_;
  }

  // === Backward compatibility aliases ===

  /// @deprecated Use alloc() instead.
  [[nodiscard]] void* allocate(size_t size) noexcept { return alloc(size); }

  /// @deprecated Use dealloc() instead.
  void deallocate(const void* addr, size_t size) noexcept { dealloc(addr, size); }
};

// === Free function interface ===

/**
 * @brief Allocate and construct an object in an arena.
 *
 * Convenience function that allocates and constructs an object of type T
 * in the given arena.
 *
 * @tparam T Type to construct.
 * @tparam Args Constructor argument types.
 * @param[in] arena Arena allocator to use.
 * @param[in] args Constructor arguments.
 * @return Pointer to constructed object, or nullptr if allocation fails.
 *
 * ## Example
 *
 * ```cpp
 * struct Point { int x, y; Point(int x, int y) : x(x), y(y) {} };
 *
 * AhArenaAllocator arena(1024);
 * Point* p = allocate<Point>(arena, 10, 20);
 * ```
 */
template <class T, typename... Args>
[[nodiscard]] T* allocate(AhArenaAllocator& arena, Args&&... args)
{
  return arena.alloc<T>(AhArenaAllocator::TemplateType::TEMPLATE,
                        std::forward<Args>(args)...);
}

/**
 * @brief Destruct and deallocate an object from an arena.
 *
 * Calls the destructor and attempts to reclaim memory (LIFO pattern only).
 *
 * @tparam T Type of object.
 * @param[in] arena Arena that owns the object.
 * @param[in] ptr Pointer to object.
 */
template <class T>
void deallocate(AhArenaAllocator& arena, T* ptr) noexcept
{
  arena.dealloc<T>(AhArenaAllocator::TemplateType::TEMPLATE, ptr);
}

/// @brief Alias for deallocate (backward compatibility).
template <class T>
void dealloc(AhArenaAllocator& arena, T* ptr) noexcept
{
  deallocate<T>(arena, ptr);
}

} // end namespace Aleph

# endif // AH_ARENA_H
