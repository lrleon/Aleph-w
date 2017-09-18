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
    std::cout << "Uid:" << std::endl
	      << "    ipAddr        = " << ipAddr << std::endl
	      << "    port_number   = " << port_number << std::endl
	      << "    counter       = " << counter << std::endl
	      << "    random_number = " << random_number << std::endl
	      << std::endl;
  }
};

# endif  // UID_H

