

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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
 }

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
    std::cout << "Uid:" << std::endl
	      << "    ipAddr        = " << ipAddr << std::endl
	      << "    port_number   = " << port_number << std::endl
	      << "    counter       = " << counter << std::endl
	      << "    random_number = " << random_number << std::endl
	      << std::endl;
  }
};

# endif  // UID_H

