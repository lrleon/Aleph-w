
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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

# ifndef HASH_FCT_H
# define HASH_FCT_H

# include <stdint.h>
# include <cstdlib>
# include <cstdint>
# include <cstring>
# include <string>
# include <primes.H>

namespace Aleph
{

  /* These group of hash functions are taken from Julienne Walker at

     http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx

     My only contribution was to copy and paste them and to adapt them to
     unsigned long instead of unsigned int. Apart that, all the credit
     for these functions must be awarded to Julienne Walker

     The quoted comments correspond to direct cites of Julienne comments
  */

  extern const unsigned Default_Hash_Seed;

  /** Additive hash

      "Probably the simplest algorithm for hashing a sequence of integral
      values (such as a string), is to add all of the characters together
      and then force the range into something suitable for lookup with the
      remainder of division. I will give an example of this algorithm only
      because books commonly suggest it in their rush to get past the
      topic of hash functions on their way to collision resolution
      methods. This algorithm is very bad"

      @author Julienne Walker
      @ingroup Hash
  */
  inline size_t add_hash(void * key, size_t len) noexcept
  {
    unsigned char *p = (unsigned char *) key;
    size_t h = 0;

    for (size_t i = 0; i < len; i++)
      h += p[i];

    return h;
  }

  /** XOR hash

      "The XOR hash is another algorithm commonly suggested by
      textbooks. Instead of adding together the bytes of an object as the
      additive hash does, the XOR hash repeatedly folds the bytes together
      to produce a seemingly random hash value"

      @author Julienne Walker
      @ingroup Hash
  */
  inline size_t xor_hash(void * key, size_t len) noexcept
  {
    unsigned char *p = (unsigned char *) key;
    size_t h = 0;

    for (size_t i = 0; i < len; i++)
      h ^= p[i];

    return h;
  }


  /** Rotating hash

      "The rotating hash is identical to the XOR hash except instead of
      simply folding each byte of the input into the internal state, it
      also performs a fold of the internal state before combining it with
      the each byte of the input. This extra mixing step is enough to give
      the rotating hash a much better distribution. Much of the time, the
      rotating hash is sufficient, and can be considered the minimal
      acceptable algorithm. Notice that with each improvement, the
      internal state is being mixed up more and more. This is a key
      element in a good hash function" 

      @author Julienne Walker
      @ingroup Hash
  */
  inline size_t rot_hash(void * key, size_t len) noexcept
  {
    unsigned char *p = (unsigned char *) key;
    size_t h = 0;
  
    for (size_t i = 0; i < len; i++)
      h = (h << 4) ^(h >> 28) ^ p[i];
  
    return h;
  }


  /** Modified Bernstein hash

      "Dan Bernstein created this algorithm and posted it in a
      newsgroup. It is known by many as the Chris Torek hash because Chris
      went a long way toward popularizing it. Since then it has been used
      successfully by many, but despite that the algorithm itself is not
      very sound when it comes to avalanche and permutation of the
      internal state. It has proven very good for small character keys,
      where it can outperform algorithms that result in a more random
      distribution" 

      "Bernstein's hash should be used with caution. It performs very well
      in practice, for no apparently known reasons (much like how the
      constant 33 does better than more logical constants for no apparent
      reason), but in theory it is not up to snuff. Always test this
      function with sample data for every application to ensure that it
      does not encounter a degenerate case and cause excessive
      collisions." 

      "A minor update to Bernstein's hash replaces addition with XOR for
      the combining step. This change does not appear to be well known or
      often used, the original algorithm is still recommended by nearly
      everyone, but the new algorithm typically results in a better
      distribution:" 

      @author Julienne Walker
      @ingroup Hash
  */
  inline size_t djb_hash(void * key, size_t len) noexcept
  {
    unsigned char *p = (unsigned char *) key;
    size_t h = 0;
  
    for (size_t i = 0; i < len; i++)
      h = 33 * h ^ p[i];
  
    return h;
  }

  /** Shift-Add-XOR hash

      "The shift-add-XOR hash was designed as a string hashing function,
      but because it is so effective, it works for any data as well with
      similar efficiency. The algorithm is surprisingly similar to the
      rotating hash except a different choice of constants for the
      rotation is used, and addition is a preferred operation for
      mixing. All in all, this is a surprisingly powerful and flexible
      hash. Like many effective hashes, it will fail tests for avalanche,
      but that does not seem to affect its performance in practice." 

      @author Julienne Walker
      @ingroup Hash
  */
  inline size_t sax_hash(void * key, size_t len) noexcept
  {
    unsigned char *p = (unsigned char *) key;
    size_t h = 0;  
  
    for (size_t i = 0; i < len; i++)
      h ^= (h << 5) +(h >> 2) + p[i];
  
    return h;
  }

  /** 

      @author Julienne Walker
      @ingroup Hash
  */
  inline size_t fnv_hash(void * key, size_t len) noexcept
  {
    unsigned char *p = (unsigned char *) key;
    size_t h = 2166136261;  
  
    for (size_t i = 0; i < len; i++)
      h = (h * 16777619) ^ p[i];
  
    return h;
  }


  /** One-at-a-Time hash

      "The FNV hash, short for Fowler/Noll/Vo in honor of the creators, is
      a very powerful algorithm that, not surprisingly, follows the same
      lines as Bernstein's modified hash with carefully chosen
      constants. This algorithm has been used in many applications with
      wonderful results, and for its simplicity, the FNV hash should be
      one of the first hashes tried in an application. It is also
      recommended that the FNV website be visited for useful descriptions
      of how to modify the algorithm for various uses." 

      @author Julienne Walker
      @ingroup Hash
  */
  inline size_t oat_hash(void * key, size_t len) noexcept
  {
    unsigned char *p = (unsigned char *) key;
    size_t h = 0;  
  
    for (size_t i = 0; i < len; i++) 
      {
        h += p[i];
        h += (h << 10);
        h ^= (h >> 6);
      }
 
    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);
 
    return h;
  }

  /** JSW hash (Julienne Walker) 

      "This is a hash of my own devising that combines a rotating hash with
      a table of randomly generated numbers. The algorithm walks through
      each byte of the input, and uses it as an index into a table of
      random integers generated by a good random number generator. The
      internal state is rotated to mix it up a bit, then XORed with the
      random number from the table. The result is a uniform distribution
      if the random numbers are uniform. The size of the table should
      match the values in a byte. For example, if a byte is eight bits
      then the table would hold 256 random numbers"

      @author Julienne Walker
      @ingroup Hash
  */
  extern size_t jsw_hash(void * key, size_t len) noexcept;


  /** ELF hash

      "The ELF hash function has been around for a while, and it is
      believed to be one of the better algorithms out there. In my
      experience, this is true, though ELF hash does not perform
      sufficiently better than most of the other algorithms presented in
      this tutorial to justify its slightly more complicated
      implementation. It should be on your list of first functions to test
      in a new lookup implementation"

      @author Julienne Walker
      @ingroup Hash
  */
  inline size_t elf_hash(void * key, size_t len) noexcept
  {
    unsigned char *p = (unsigned char *) key;
    size_t h = 0, g;
  
    for (size_t i = 0; i < len; i++) 
      {
        h = (h << 4) + p[i];
        g = h & 0xf0000000L;
 
        if(g != 0)
          h ^= g >> 24;
 
        h &= ~g;
      }
 
    return h;
  }


#define hashsize(n)(1U << (n))
#define hashmask(n)(hashsize(n) - 1)
  
  inline void  mix(size_t & a, size_t & b, size_t & c) noexcept
  { 
    a -= b; a -= c; a ^=(c >> 13); 
    b -= c; b -= a; b ^=(a << 8);			
    c -= a; c -= b; c ^=(b >> 13); 
    a -= b; a -= c; a ^=(c >> 12);
    b -= c; b -= a; b ^=(a << 16);
    c -= a; c -= b; c ^=(b >> 5);
    a -= b; a -= c; a ^=(c >> 3);
    b -= c; b -= a; b ^=(a << 10);
    c -= a; c -= b; c ^=(b >> 15);
  }


  /** Jenkins hash

      "The dreaded Jenkins hash has been thoroughly tested and passes all
      kinds of tests for avalanche and permutations. As such it is
      considered to be one of the best and most thoroughly analyzed
      algorithms on the market presently. Unfortunately, it is also
      ridiculously complicated compared to the other hashes examined in
      this tutorial"

      @author Julienne Walker
      @ingroup Hash
  */
  inline size_t jen_hash(void * key, size_t length,
                         unsigned initval = Default_Hash_Seed) noexcept
  {
    unsigned char * k = (unsigned char *) key;
    size_t a, b;
    size_t c = initval;
    size_t len = length;

    a = b = 0x9e3779b9;

    while (len >= 12)
      {
        a += (k[0] +((size_t)k[1] << 8) + ((size_t)k[2] << 16) +
              ((size_t)k[3] << 24));
        b += (k[4] +((size_t)k[5] << 8) + ((size_t)k[6] << 16) +
              ((size_t)k[7] << 24));
        c += (k[8] +((size_t)k[9] << 8) +((size_t)k[10] << 16) +
              ((size_t)k[11] << 24));

        mix(a, b, c);

        k += 12;
        len -= 12;
      }

    c += length;

    switch(len)
      {
      case 11: c += ((size_t)k[10] << 24);
      case 10: c += ((size_t)k[9] << 16);
      case 9 : c += ((size_t)k[8] << 8);
        /* First byte of c reserved for length */
      case 8 : b += ((size_t)k[7] << 24);
      case 7 : b += ((size_t)k[6] << 16);
      case 6 : b += ((size_t)k[5] << 8);
      case 5 : b += k[4];
      case 4 : a += ((size_t)k[3] << 24);
      case 3 : a += ((size_t)k[2] << 16);
      case 2 : a += ((size_t)k[1] << 8);
      case 1 : a += k[0];
      }

    mix(a, b, c);

    return c;
  }

  /* The following three functions were taken from

     https://github.com/PeterScott/murmur3

     They are a port of MurmurHash created by Austin Appleby and written
     by Peter Scott

     See at

     http://en.wikipedia.org/wiki/MurmurHash

     For more details
  */

  void MurmurHash3_x86_32 (const void *key, int len, uint32_t seed, void *out);

  void MurmurHash3_x86_128(const void *key, int len, uint32_t seed, void *out);

  void MurmurHash3_x64_128(const void *key, int len, uint32_t seed, void *out);

  struct Buf128Bits
  {
    std::uint32_t a[4];
  };

  template <typename Key> inline
  size_t murmur3hash(const Key & key, unsigned long seed)
  {
# ifdef __x86_64__
    Buf128Bits buf;
    MurmurHash3_x64_128(&key, sizeof(key), seed, &buf);
# else
    size_t buf;
    MurmurHash3_x86_32(&key, sizeof(key), seed, &buf);
# endif

    size_t ret;
    memcpy(&ret, &buf, sizeof(ret));

    return ret;
  }


  inline size_t murmur3hash(char * key, unsigned long seed)
  {
    size_t len = strlen(key);
# ifdef __x86_64__
    Buf128Bits buf;
    MurmurHash3_x64_128(key, len, seed, &buf);
# else
    size_t buf;
    MurmurHash3_x86_32(key, len, seed, &buf);
# endif

    size_t ret;
    memcpy(&ret, &buf, sizeof(ret));

    return ret;
  }

  template <> inline
  size_t murmur3hash(const std::string & key, unsigned long seed)
  {
# ifdef __x86_64__
    Buf128Bits buf;
    MurmurHash3_x64_128(key.c_str(), key.size(), seed, &buf);
# else
    size_t buf;
    MurmurHash3_x86_32(key.c_str(), key.size(), seed, &buf);
# endif

    size_t ret;
    memcpy(&ret, &buf, sizeof(ret));

    return ret;
  }


  /* Paul Hsieh super fast hash function.

     Taken from

     http://www.azillionmonkeys.com/qed/hash.html
  */

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__)    \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)	\
                      +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

  /** Paul Hsieh super fast hash function.

      @author Paul Hsieh
      @ingroup Hash
  */
  inline size_t SuperFastHash(void * key, size_t len) noexcept
  {
    const unsigned char * data = (unsigned char *) key;
    size_t hash = len, tmp;
    int rem;

    if (len <= 0 || data == nullptr)
      return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--)
      {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
      }

    /* Handle end cases */
    switch (rem)
      {
      case 3:
        hash += get16bits (data);
        hash ^= hash << 16;
        hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
        hash += hash >> 11;
        break;
      case 2:
        hash += get16bits (data);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
      case 1:
        hash += (signed char)*data;
        hash ^= hash << 10;
        hash += hash >> 1;
      }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
  }

  template <typename Key> inline
  size_t add_hash(const Key & key) noexcept
  {
    return add_hash((void *) &key, sizeof(key));
  }

  template <typename Key> inline
  size_t xor_hash(const Key & key) noexcept
  {
    return xor_hash((void *) &key, sizeof(key));
  }

  template <typename Key>
  inline size_t rot_hash(const Key & key) noexcept
  {
    return rot_hash((void *) &key, sizeof(key));
  }

  template <typename Key>
  inline size_t djb_hash(const Key & key) noexcept
  {
    return djb_hash((void *) &key, sizeof(key));
  }

  template <typename Key>
  inline size_t sax_hash(const Key & key) noexcept
  {
    return sax_hash((void *) &key, sizeof(key));
  }

  template <typename Key>
  inline size_t fnv_hash(const Key & key) noexcept
  {
    return fnv_hash((void *) &key, sizeof(key));
  }

  template <typename Key>
  inline size_t oat_hash(const Key & key) noexcept
  {
    return oat_hash((void *) &key, sizeof(key));
  }

  template <typename Key>
  inline size_t jsw_hash(const Key & key) noexcept
  {
    return jsw_hash((void *) &key, sizeof(key));
  }

  template <typename Key>
  inline size_t elf_hash(const Key & key) noexcept
  {
    return elf_hash((void *) &key, sizeof(key));
  }

  template <typename Key>
  inline size_t jen_hash(const Key &  key, unsigned initval) noexcept
  {
    return jen_hash((void *) &key, sizeof(key), initval);
  }

  template <typename Key>
  inline size_t SuperFastHash(const Key &  key) noexcept 
  {
    return SuperFastHash((void *) &key, sizeof(key));
  }

  inline size_t add_hash(char * key) noexcept
  {
    unsigned char * p = (unsigned char *) key;
    size_t h = 0;
  
    while (*p)
      h += *p++;
  
    return h;
  }

  inline size_t xor_hash(const char * key) noexcept
  {
    unsigned char * p = (unsigned char *) key;
    size_t h = 0;
  
    while (*p)
      h ^= *p++;
  
    return h;
  }

  inline size_t rot_hash(const char * key) noexcept
  {
    unsigned char *p = (unsigned char *) key;
    size_t h = 0;

    while (*p)
      h = (h << 4) ^(h >> 28) ^ *p++;

    return h;
  }

  inline size_t djb_hash(const char * key) noexcept
  {
    unsigned char *p = (unsigned char *) key;
    size_t h = 0;

    while (*p)
      h = 33 * h ^ *p++;

    return h;
  }

  inline size_t sax_hash(const char * key)
  {
    unsigned char *p = (unsigned char *) key;
    size_t h = 0;

    while (*p)
      h ^= (h << 5) +(h >> 2) + *p++;

    return h;
  }

  inline size_t fnv_hash(const char * key) noexcept
  {
    unsigned char *p = (unsigned char *) key;
    size_t h = 2166136261;

    while (*p)
      h = (h * 16777619) ^ *p++;

    return h;
  }

  inline size_t oat_hash(const char * key) noexcept
  {
    unsigned char *p = (unsigned char *) key;
    size_t h = 0;

    while (*p)
      {
        h += *p++;
        h += (h << 10);
        h ^= (h >> 6);
      }

    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);

    return h;
  }

  extern size_t jsw_hash(const char * key);

  inline size_t elf_hash(const char * key) noexcept
  {
    unsigned char *p = (unsigned char *) key;
    size_t h = 0, g;

    while (*p)
      {
        h =(h << 4) + *p++;
        g = h & 0xf0000000L;

        if(g != 0)
          h ^= g >> 24;

        h &= ~g;
      }

    return h;
  }

  inline size_t SuperFastHash(const char * key) noexcept
  {
    return SuperFastHash((void*) key, strlen(key));
  }

  template <>
  inline size_t add_hash(const std::string & key) noexcept
  {
    return add_hash(key.c_str());
  }

  inline size_t xor_hash(const std::string & key) noexcept
  {
    return xor_hash(key.c_str());
  }

  inline size_t rot_hash(const std::string & key) noexcept
  {
    return rot_hash(key.c_str());
  }

  inline size_t djb_hash(const std::string & key) noexcept
  {
    return djb_hash(key.c_str());
  }

  inline size_t sax_hash(const std::string & key) noexcept
  {
    return sax_hash(key.c_str());
  }

  inline size_t fnv_hash(const std::string & key) noexcept
  {
    return fnv_hash(key.c_str());
  }

  inline size_t oat_hash(const std::string & key) noexcept
  {
    return oat_hash(key.c_str());
  }

  inline size_t jsw_hash(const std::string & key) noexcept
  {
    return jsw_hash(key.c_str());
  }

  inline size_t elf_hash(const std::string & key) noexcept
  {
    return elf_hash(key.c_str());
  }

  inline size_t jen_hash(const std::string & key, unsigned initval) noexcept
  {
    return jen_hash((void*) key.c_str(), key.size(), initval);
  }

  inline size_t SuperFastHash(const std::string & key) noexcept
  {
    return SuperFastHash((void*) key.c_str(), key.size());
  }

  template <typename Key> inline
  size_t dft_hash_fct(const Key & key) noexcept
  {
    return SuperFastHash(key);
  }

  template <typename Key> inline
  size_t snd_hash_fct(const Key & key) noexcept
  {
    return murmur3hash(key, 52679987);
  }

  template <typename Key> inline
  size_t dft_hash_fct(const Key & key, unsigned long seed) noexcept
  {
    return murmur3hash(key, seed);
  }

  template<typename Key, typename Data, typename Fct>
  inline size_t map_hash_fct(Fct fct, const std::pair<Key, Data> & p) noexcept
  {
    return fct(p.first);
  }

  template <typename K1, typename K2> inline
  size_t pair_dft_hash_fct(const std::pair<K1, K2> & p) noexcept
  {
    return dft_hash_fct(p.first) + dft_hash_fct(p.second);
  }

  template <typename K1, typename K2> inline
  size_t pair_snd_hash_fct(const std::pair<K1, K2> & p) noexcept
  {
    return dft_hash_fct(p.first) + snd_hash_fct(p.second);
  }

} // end namespace Aleph


# endif // HASH_FCT_H
