/* Aleph-w

     / \\  | | ___ _ __ | |__      __      __
    / _ \\ | |/ _ \\ '_ \\| '_ \\ ____\\ \\ /\\ / / Data structures \& Algorithms
   / ___ \\| |  __/ |_) | | | |_____\\ V  V /  version 1.9c
  /_/   \\_\\_|\\___| .__/|_| |_|      \\_/\\_/   https://github.com/lrleon/Aleph-w
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

# include <uid.H>
 # include <ah-errors.H>


typedef unsigned char Byte;

static char * hexadecimalize(const Byte & byte, char *& str)
{
  Byte l = byte & 0x0F;
  Byte h = (byte & 0xF0) >> 4;

  *str++ = nibble_to_char(h);
  *str++ = nibble_to_char(l);

  return str;
}


static char unhexadecimalize(char *& str)
{
  Byte h = char_to_nibble(*str++) << 4;
  Byte l = char_to_nibble(*str++);

  return h | l;
}


char * Uid::stringficate(char *         buffer,
			 const size_t & buf_size) const
{
  ah_range_error_if(buf_size < 2*sizeof(Uid) + 1)
    << "Buffer size is not enough";

  char * this_str = (char*) this;
  char * ret_val = buffer;

      // copiar nibbles convertidas a ascii a buffer
  for (int i = 0; i < sizeof(Uid); ++i)
    hexadecimalize(*this_str++, buffer);

  assert(this_str - (char*) this == sizeof(Uid));
  
  *buffer = '\0';

  return ret_val;
}

void Uid::destringficate(char * str)
{
  char * this_str = (char*) this;

      // convertir el string en ascii a la representación en nibbles
  for (int i = 0; i < sizeof(Uid); ++i)
    *this_str++ = unhexadecimalize(str);
}


Uid::Uid(const IPv4_Address & _ipAddr, 
	 const unsigned int & _counter, 
	 const unsigned int & _port_number)
  : ipAddr(_ipAddr), port_number(_port_number), counter(_counter)
{
  random_number = rand();
}


Uid::Uid(char* str)
{
  destringficate(str);
  
}

bool Uid::operator == (const Uid& uid) const
{
  return (ipAddr == uid.ipAddr and 
	  port_number == uid.port_number and
	  counter == uid.counter and
	  random_number == uid.random_number);
}

char * Uid::getStringUid(char * str, const size_t & str_size) const
{
  if (str_size < stringSize)
    return NULL;

  return stringficate(str, str_size);
}
