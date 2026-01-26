
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
 *  @brief Unique identifier generation.
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# ifndef UID_H
# define UID_H

# include <stdint.h>
# include <iostream>
# include <aleph.H>

using namespace Aleph;


class Uid
{
  friend struct Uid_Offsets;

private:

  mutable Aleph::IPv4_Address ipAddr;
  mutable uint32_t            port_number;
  mutable uint64_t            counter;
  mutable uint64_t            random_number;

public:

  static const int stringSize = 2*(sizeof(uint32_t) + sizeof(uint32_t) +
				   sizeof(uint64_t) + sizeof(uint64_t)) + 1;

private:

  char * stringficate(char * buffer, const size_t & src_size) const;

  void destringficate(char * str);

public:

  Uid() { /* empty */ }

  Uid(const IPv4_Address & _ipAddr,
      const unsigned int & _counter, 
      const unsigned int & _port_number);

  Uid(char* str);

  bool operator == (const Uid& uid) const;

  char* getStringUid(char * str, const size_t &) const; 

  const IPv4_Address & getIpAddr() const { return ipAddr; }

  const uint32_t & get_port_number() const { return port_number; }

  const uint64_t & get_counter() const { return counter; }

  const uint64_t & get_random_number() const { return random_number; }

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

