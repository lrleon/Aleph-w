
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


#ifndef AH_INIT_GUARD_H
#define AH_INIT_GUARD_H

/** @file ah_init_guard.H
    @brief RAII guard for exception-safe resource cleanup.

    Provides a scope guard that executes a cleanup function when destroyed,
    unless explicitly released. Commonly used in graph algorithms to ensure
    proper cleanup of node/arc cookies even if exceptions are thrown.

    @ingroup Algorithms
 *  @author Leandro Rabindranath León
*/

#include <utility>

namespace Aleph
{

/** @brief RAII guard for graph algorithm initialization.

    This template class provides exception-safe cleanup for graph algorithms
    that allocate resources during initialization (e.g., node/arc cookies).

    The guard calls the provided cleanup function when destroyed, unless
    `release()` has been called to indicate successful completion.

    @tparam CleanupFunc A callable type (lambda, function object) that
                        performs the cleanup when invoked with no arguments.

    Usage example:
    @code
    void my_algorithm(Graph& g) {
      init_resources(g);

      Init_Guard guard([&]() { cleanup_resources(g); });

      // ... algorithm that might throw ...

      guard.release(); // Success - don't cleanup
    }
    @endcode

    @ingroup Algorithms
*/
template <typename CleanupFunc>
class Init_Guard
{
  CleanupFunc cleanup;
  bool released = false;

public:

  /** @brief Construct guard with cleanup function.

      @param f Cleanup function to call on destruction (if not released).
  */
  explicit Init_Guard(CleanupFunc f) : cleanup(std::move(f)) {}

  /** @brief Destructor calls cleanup if not released.

      If `release()` was not called, the cleanup function is invoked.
  */
  ~Init_Guard()
  {
    if (not released)
      cleanup();
  }

  /** @brief Release the guard - cleanup will not be called.

      Call this method to indicate successful completion. After calling
      this, the cleanup function will not be invoked on destruction.
  */
  void release() noexcept { released = true; }

  /** @brief Check if the guard has been released.

      @return true if release() has been called, false otherwise.
  */
  [[nodiscard]] bool is_released() const noexcept { return released; }

  /// Non-copyable
  Init_Guard(const Init_Guard &) = delete;
  Init_Guard & operator=(const Init_Guard &) = delete;

  /** @brief Move constructor.

      Transfers ownership of the cleanup function to this guard.
      The moved-from guard is marked as released.

      @param other Guard to move from.
  */
  Init_Guard(Init_Guard && other) noexcept
    : cleanup(std::move(other.cleanup)), released(other.released)
  {
    other.released = true;
  }

  /** @brief Move assignment operator.

      If this guard holds an active cleanup, it is executed first.
      Then ownership is transferred from other.

      @param other Guard to move from.
      @return Reference to this.
  */
  Init_Guard & operator=(Init_Guard && other) noexcept
  {
    if (this != &other)
      {
        if (not released)
          cleanup();
        cleanup = std::move(other.cleanup);
        released = other.released;
        other.released = true;
      }
    return *this;
  }
};

/// Deduction guide for Init_Guard
template <typename CleanupFunc>
Init_Guard(CleanupFunc) -> Init_Guard<CleanupFunc>;

/** @brief Create an Init_Guard with a cleanup function.

    Convenience factory function that creates an Init_Guard.

    @tparam CleanupFunc Callable type.
    @param f Cleanup function.
    @return Init_Guard wrapping the cleanup function.

    @ingroup Algorithms
*/
template <typename CleanupFunc>
[[nodiscard]] inline auto make_init_guard(CleanupFunc f)
{
  return Init_Guard<CleanupFunc>(std::move(f));
}

} // end namespace Aleph

#endif // AH_INIT_GUARD_H
