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
#ifndef AH_INIT_GUARD_H
#define AH_INIT_GUARD_H

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
  */
  template <typename CleanupFunc>
  class Init_Guard
  {
    CleanupFunc cleanup;
    bool released = false;

  public:
    /// Construct guard with cleanup function
    explicit Init_Guard(CleanupFunc f) : cleanup(std::move(f)) {}

    /// Destructor calls cleanup if not released
    ~Init_Guard()
    {
      if (not released)
        cleanup();
    }

    /// Release the guard - cleanup will not be called
    void release() noexcept { released = true; }

    // Non-copyable
    Init_Guard(const Init_Guard &) = delete;
    Init_Guard & operator=(const Init_Guard &) = delete;

    // Movable
    Init_Guard(Init_Guard && other) noexcept
      : cleanup(std::move(other.cleanup)), released(other.released)
    {
      other.released = true;
    }

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

} // end namespace Aleph

#endif // AH_INIT_GUARD_H
