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


/** @file uid.H
 *  @brief Unique identifier generation for distributed systems.
 *
 *  This file provides the Uid class for generating globally unique identifiers
 *  that combine IP address, port number, counter, and random number to ensure
 *  uniqueness across distributed systems.
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# ifndef UID_H
# define UID_H

# include <stdint.h>
# include <iostream>
# include <aleph.H>

/** @brief Unique identifier for distributed systems.
 *
 *  The Uid class generates and manages globally unique identifiers by combining:
 *  - IPv4 address (32 bits)
 *  - Port number (32 bits)
 *  - Counter (64 bits)
 *  - Random number (64 bits)
 *
 *  This combination ensures uniqueness even across multiple machines and processes
 *  running simultaneously in a distributed system.
 *
 *  ## String Representation
 *
 *  UIDs can be converted to/from hexadecimal string format for storage and
 *  transmission. The string size is fixed at `stringSize` bytes.
 *
 *  ## Use Cases
 *
 *  - Distributed database keys
 *  - Message identifiers in distributed systems
 *  - Session identifiers
 *  - Transaction identifiers
 *
 *  @par Thread Safety
 *  Not thread-safe. Use external synchronization if sharing Uid objects
 *  across threads.
 *
 *  @par Example
 *  @code
 *  IPv4_Address my_ip("192.168.1.100");
 *  Uid id(my_ip, 12345, 8080);
 *
 *  char buffer[Uid::stringSize];
 *  id.getStringUid(buffer, sizeof(buffer));
 *  std::cout << "UID: " << buffer << '\n';
 *
 *  // Reconstruct from string
 *  Uid id2(buffer);
 *  assert(id == id2);
 *  @endcode
 *
 *  @ingroup Utilities
 */
class Uid
{
  friend struct Uid_Offsets;

  mutable Aleph::IPv4_Address ipAddr = 0; ///< IPv4 address component
  mutable uint32_t port_number = 0; ///< Port number component
  mutable uint64_t counter = 0; ///< Counter component
  mutable uint64_t random_number = 0; ///< Random number component

public:
  /** @brief Required buffer size for string representation.
   *
   *  The string representation uses hexadecimal encoding, requiring
   *  2 characters per byte plus null terminator.
   */
  static constexpr int stringSize = 2 * (sizeof(uint32_t) + sizeof(uint32_t) +
                                         sizeof(uint64_t) + sizeof(uint64_t)) + 1;

private:
  /** @brief Convert UID components to hexadecimal string.
   *  @param buffer output buffer for string
   *  @param src_size size of output buffer
   *  @return pointer to buffer
   */
  char * stringficate(char *buffer, const size_t & src_size) const;

  /** @brief Parse hexadecimal string and populate UID components.
   *  @param str input string in hexadecimal format
   */
  void destringficate(char *str);

public:
  /** @brief Default constructor.
   *
   *  Creates an uninitialized UID. Components must be set manually
   *  or via assignment.
   *
   *  @throws none
   *  @note Complexity: O(1)
   *  @note Exception safety: noexcept
   */
  Uid() = default;

  /** @brief Construct UID from components.
   *
   *  Creates a unique identifier from the provided components.
   *
   *  @param _ipAddr IPv4 address of the originating machine
   *  @param _counter sequence counter (typically incremented per UID)
   *  @param _port_number port number of the generating process
   *  @throws none
   *  @note Complexity: O(1)
   *  @note Exception safety: noexcept
   */
  Uid(const Aleph::IPv4_Address & _ipAddr,
      const uint64_t & _counter,
      const uint32_t & _port_number);

  /** @brief Backward-compatible constructor with legacy parameter types.
   *
   *  This overload preserves source compatibility with older code that used
   *  unsigned int for the counter and port number. It delegates to the
   *  constructor taking uint64_t and uint32_t.
   *
   *  @deprecated Use Uid(const Aleph::IPv4_Address&, const uint64_t&,
   *                      const uint32_t&) instead.
   */
  [[deprecated("Use Uid(const Aleph::IPv4_Address&, const uint64_t&, const uint32_t&) instead")]]
  Uid(const Aleph::IPv4_Address & _ipAddr,
      const unsigned int & _counter,
      const unsigned int & _port_number)
    : Uid(_ipAddr,
          static_cast<uint64_t>(_counter),
          static_cast<uint32_t>(_port_number))
  {}
  /** @brief Construct UID from string representation.
   *
   *  Reconstructs a UID from its hexadecimal string format.
   *
   *  @param str hexadecimal string (must be at least `stringSize` bytes)
   *  @throws std::invalid_argument if string format is invalid
   *  @note Complexity: O(1) (fixed size parsing)
   *  @note Exception safety: strong
   */
  Uid(char *str);

  /** @brief Compare two UIDs for equality.
   *
   *  Two UIDs are equal if all their components match.
   *
   *  @param uid the UID to compare against
   *  @return true if UIDs are identical, false otherwise
   *  @throws none
   *  @note Complexity: O(1)
   *  @note Exception safety: noexcept
   */
  bool operator ==(const Uid & uid) const;

  /** @brief Convert UID to hexadecimal string representation.
   *
   *  Generates a string representation suitable for storage or transmission.
   *
   *  @param str output buffer (must be at least `stringSize` bytes)
   *  @param size size of output buffer
   *  @return pointer to the output buffer
   *  @throws std::range_error if buffer size is insufficient
   *  @note Complexity: O(1) (fixed size generation)
   *  @note Exception safety: strong
   */
  char * getStringUid(char *str, const size_t & size) const;

  /** @brief Get IP address component.
   *  @return the IPv4 address
   *  @throws none
   *  @note Complexity: O(1)
   *  @note Exception safety: noexcept
   */
  const Aleph::IPv4_Address &getIpAddr() const { return ipAddr; }

  /** @brief Get port number component.
   *  @return the port number
   *  @throws none
   *  @note Complexity: O(1)
   *  @note Exception safety: noexcept
   */
  const uint32_t &get_port_number() const { return port_number; }

  /** @brief Get counter component.
   *  @return the counter value
   *  @throws none
   *  @note Complexity: O(1)
   *  @note Exception safety: noexcept
   */
  const uint64_t &get_counter() const { return counter; }

  /** @brief Get random number component.
   *  @return the random number
   *  @throws none
   *  @note Complexity: O(1)
   *  @note Exception safety: noexcept
   */
  const uint64_t &get_random_number() const { return random_number; }

  /** @brief Print UID components to standard output.
   *
   *  Prints all four components in human-readable format for debugging.
   */
  void print()
  {
    std::cout << "Uid:" << '\n'
        << "    ipAddr        = " << ipAddr << '\n'
        << "    port_number   = " << port_number << '\n'
        << "    counter       = " << counter << '\n'
        << "    random_number = " << random_number << '\n'
        << '\n';
  }
};

# endif  // UID_H
