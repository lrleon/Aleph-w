
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file ahTypes.H
 *  @brief Type traits for primitive type detection and swap operations.
 *
 *  This file provides compile-time type traits for detecting primitive
 *  C++ types and optimized swap operations for those types.
 *
 *  ## Features
 *
 *  - `is_primitive_type<T>()`: Returns true for C++ primitives
 *  - `swap<T>()`: Optimized swap for primitive types
 *
 *  ## Supported Primitive Types
 *
 *  - char, unsigned char
 *  - short, unsigned short
 *  - int, unsigned int
 *  - long, unsigned long
 *  - long long, unsigned long long
 *  - float, double, long double
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  if (Aleph::is_primitive_type<int>())   // true
 *  if (Aleph::is_primitive_type<MyClass>()) // false
 *
 *  int a = 5, b = 10;
 *  Aleph::swap(a, b);  // Uses optimized primitive swap
 *  ```
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# ifndef AHTYPES_H
# define AHTYPES_H


namespace Aleph {

/**
 * @brief Check if type T is a primitive C++ type.
 *
 * Default template returns false for non-primitive types.
 * Specialized versions return true for primitives.
 *
 * @tparam T Type to check
 * @return true if T is a primitive type, false otherwise
 */
  template <typename T>
bool is_primitive_type() { return false; }

/**
 * @brief Macro to declare a type as primitive.
 *
 * Creates template specialization of is_primitive_type for type T.
 *
 * @param T Primitive type to declare
 */
# define DECLARE_PRIMITIVE(T)			\
  template <>					\
  bool is_primitive_type <T> ()			\
  {						\
    return true;				\
  }

  DECLARE_PRIMITIVE(char);
  DECLARE_PRIMITIVE(unsigned char);
  DECLARE_PRIMITIVE(short);
  DECLARE_PRIMITIVE(unsigned short);
  DECLARE_PRIMITIVE(int);
  DECLARE_PRIMITIVE(unsigned int);
  DECLARE_PRIMITIVE(long long);
  DECLARE_PRIMITIVE(unsigned long long);
  DECLARE_PRIMITIVE(float);
  DECLARE_PRIMITIVE(double);
  DECLARE_PRIMITIVE(long double);

/**
 * @brief Generic swap using object's swap method.
 *
 * Default template calls t1.swap(t2) for class types.
 *
 * @tparam T Type with swap() method
 * @param t1 First object
 * @param t2 Second object
 * @return Reference to t1
 */
  template <class T> T & swap(T & t1, T & t2)
  {
    return t1.swap(t2);
  }

/**
 * @brief Macro to declare optimized swap for primitive type.
 *
 * Creates template specialization using temporary variable.
 *
 * @param T Primitive type
 */
# define DECLARE_SWAP(T)			\
  template <> T & swap <T> (T & t1, T & t2)	\
  {						\
    T tmp = t1;					\
    t1 = t2;					\
    t2 = tmp;					\
    return t1;
  }

  DECLARE_SWAP(char);
  DECLARE_SWAP(unsigned char);
  DECLARE_SWAP(short);
  DECLARE_SWAP(unsigned shor);
  DECLARE_SWAP(int);
  DECLARE_SWAP(unsigned int);
  DECLARE_SWAP(long);
  DECLARE_SWAP(unsigned long);
  DECLARE_SWAP(long long);
  DECLARE_SWAP(unsigned long long);
  DECLARE_SWAP(float);
  DECLARE_SWAP(double);
  DECLARE_SWAP(long double);

} // end namespace Aleph

# endif // AHTYPES_H
