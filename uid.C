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

# include <random>
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
                         const size_t & buf_size) const
{
  ah_range_error_if(buf_size < 2*sizeof(Uid) + 1)
    << "Buffer size is not enough";

  char *this_str = (char *) this;
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
  char *this_str = reinterpret_cast<char *>(this);

  // Convert the string in ASCII to the rendering in Nibbles
  for (int i = 0; i < sizeof(Uid); ++i)
    *this_str++ = unhexadecimalize(str);
}


Uid::Uid(const Aleph::IPv4_Address & _ipAddr,
         const unsigned int & _counter,
         const unsigned int & _port_number)
  : ipAddr(_ipAddr), port_number(_port_number), counter(_counter)
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<unsigned int> dis;
  random_number = dis(gen);
}


Uid::Uid(char *str)
{
  destringficate(str);
}

bool Uid::operator ==(const Uid & uid) const
{
  return (ipAddr == uid.ipAddr and
          port_number == uid.port_number and
          counter == uid.counter and
          random_number == uid.random_number);
}

char * Uid::getStringUid(char *str, const size_t & str_size) const
{
  if (str_size < stringSize)
    return nullptr;

  return stringficate(str, str_size);
}
