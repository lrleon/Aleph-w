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

# include <ctime>
# include <random>
# include <cstring>
# include <cctype>
# include <cstdint>
# include <uid.H>
# include <ah-errors.H>


typedef unsigned char Byte;

static char * hexadecimalize(const Byte & byte, char *& str)
{
  const Byte l = byte & 0x0F;
  const Byte h = (byte & 0xF0) >> 4;

  *str++ = Aleph::nibble_to_char(h);
  *str++ = Aleph::nibble_to_char(l);

  return str;
}


static char unhexadecimalize(char *& str)
{
  const Byte h = Aleph::char_to_nibble(*str++) << 4;
  const Byte l = Aleph::char_to_nibble(*str++);

  return h | l;
}


char * Uid::stringficate(char *buffer,
                         const size_t & src_size) const
{
  ah_range_error_if(src_size < 2*sizeof(Uid) + 1)
    << "Buffer size is not enough";

  auto this_str = (char *) this;
  char *ret_val = buffer;

  // copiar nibbles convertidas a ascii a buffer
  for (int i = 0; i < sizeof(Uid); ++i)
    hexadecimalize(*this_str++, buffer);

  assert(this_str - (char*) this == sizeof(Uid));

  *buffer = '\0';

  return ret_val;
}

void Uid::destringficate(char *str)
{
  auto this_str = reinterpret_cast<char *>(this);

  // Convert the string in ASCII to the rendering in Nibbles
  for (int i = 0; i < sizeof(Uid); ++i)
    *this_str++ = unhexadecimalize(str);
}


Uid::Uid(const Aleph::IPv4_Address & _ipAddr,
         const uint64_t & _counter,
         const uint32_t & _port_number) noexcept
  : ipAddr(_ipAddr), port_number(_port_number), counter(_counter)
{
  // Generate the random component directly from std::random_device,
  // which is typically backed by the operating system CSPRNG.
  // We intentionally avoid std::mt19937 and any time-based fallback to
  // keep the UID suitable for security-relevant use cases.
  std::random_device rd;

  // Combine two 32-bit random values to fill the 64-bit random_number
  // This ensures the full width is utilized even if random_device returns 32-bit values
  const auto high = static_cast<decltype(random_number)>(rd());
  const auto low = static_cast<decltype(random_number)>(rd());

  random_number = (high << 32) | low;
}


Uid::Uid(char *str)
{
  ah_invalid_argument_if(str == nullptr) << "Null string passed to Uid constructor";

  const size_t len = std::strlen(str);
  ah_invalid_argument_if(len < static_cast<size_t>(stringSize - 1))
    << "String too short for Uid (expected " << (stringSize - 1) << " chars)";

  // Validate hex characters
  for (size_t i = 0; i < static_cast<size_t>(stringSize - 1); ++i)
    ah_invalid_argument_if(not std::isxdigit(static_cast<unsigned char>(str[i])))
         << "Invalid hex character in Uid string at index " << i;

  destringficate(str);
}

bool Uid::operator ==(const Uid & uid) const noexcept
{
  return (ipAddr == uid.ipAddr and
          port_number == uid.port_number and
          counter == uid.counter and
          random_number == uid.random_number);
}

char * Uid::getStringUid(char *str, const size_t & size) const
{
  ah_range_error_if(size < stringSize)
    << "Buffer size too small for UID string representation";

  return stringficate(str, size);
}
