
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

# ifndef AHSINGLETON_H
# define AHSINGLETON_H

/** @file ahSingleton.H
 * @brief Minimal, header-only singleton helpers.
 *
 * This header provides:
 * - `Make_Singleton(T)`: a macro to inject a Meyers singleton accessor into a
 *   class.
 * - `Singleton`: a trivial singleton class, mostly as an example.
 *
 * Notes:
 * - This uses a function-local `static` instance (Meyers singleton).
 * - Since C++11, initialization of function-local statics is thread-safe.
 * - Destruction happens at program termination, in reverse construction order
 *   across translation units (usual static deinitialization rules).
 */

/** @brief Inject a Meyers singleton accessor into a class.
 *
 * Usage:
 *
 * ```cpp
 * class MyType
 * {
 *   Make_Singleton(MyType)
 * private:
 *   MyType() = default;
 * };
 * ```
 *
 * Requirements:
 * - The type `name` must be default-constructible, and the default
 *   constructor must be accessible from the injected `get_instance()`.
 *
 * Guarantees:
 * - Returns the same instance for all calls.
 * - Copy and move operations are disabled.
 */
# define Make_Singleton(name)                   \
  public:                                       \
                                                \
  static name & get_instance()                  \
  {                                             \
    static name instance;                       \
    return instance;                            \
  }                                             \
                                                \
  name(const name &) = delete;                  \
  name(name &&) = delete;                       \
  name & operator = (const name &) = delete;    \
  name & operator = (name &&) = delete;

                                                
class Singleton
{
public:

  static Singleton & get_instance()
  {
    static Singleton instance;
    return instance;
  }
  
private:
  
  Singleton() = default;

public:
  
  Singleton(Singleton const&) = delete;
  Singleton(Singleton &&) = delete;
  Singleton & operator = (Singleton const&) = delete;
  Singleton & operator = (Singleton &&) = delete;
};

# endif // AHSINGLETON_H
