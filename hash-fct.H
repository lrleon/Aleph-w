
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


/** @file hash-fct.H
    @brief Collection of general-purpose hash functions.

    This file provides a comprehensive collection of hash functions suitable
    for use in hash tables, bloom filters, checksums, and other applications
    requiring hash computation.

    ## Hash Function Categories

    The functions are organized into several categories:

    ### Simple Hash Functions (Educational/Legacy)
    - `add_hash()` - Simple additive hash (poor distribution)
    - `xor_hash()` - XOR folding hash (poor distribution)
    - `rot_hash()` - Rotating hash (acceptable minimum)

    ### Production-Quality Hash Functions
    - `djb_hash()` - Dan Bernstein's hash (good for small strings)
    - `sax_hash()` - Shift-Add-XOR hash (flexible, good distribution)
    - `fnv_hash()` - Fowler-Noll-Vo hash (simple, effective)
    - `oat_hash()` - One-at-a-Time hash by Bob Jenkins
    - `jsw_hash()` - Julienne Walker's hash (uses random table)
    - `elf_hash()` - ELF object file hash (well-tested)

    ### High-Quality Hash Functions
    - `jen_hash()` - Jenkins hash (excellent avalanche properties)
    - `SuperFastHash()` - Paul Hsieh's fast hash
    - `murmur3hash()` - MurmurHash3 (modern, high-quality)

    ## Usage Examples

    @code
    #include <hash-fct.H>
    using namespace Aleph;

    // Hash a string
    std::string key = "example";
    size_t h1 = dft_hash_fct(key);  // Uses SuperFastHash
    size_t h2 = fnv_hash(key);      // Uses FNV hash

    // Hash an integer
    int value = 42;
    size_t h3 = djb_hash(value);

    // Hash raw bytes
    char data[] = {1, 2, 3, 4};
    size_t h4 = jen_hash(data, sizeof(data));

    // Hash with seed (for double hashing)
    size_t h5 = murmur3hash(key, 12345);
    @endcode

    ## References

    Most simple hash functions are adapted from Julienne Walker's tutorial:
    http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx

    MurmurHash3 is ported from Austin Appleby's implementation:
    https://github.com/aappleby/smhasher

    SuperFastHash is by Paul Hsieh:
    http://www.azillionmonkeys.com/qed/hash.html

    @author Julienne Walker (original simple hashes)
    @author Bob Jenkins (Jenkins hash, One-at-a-Time)
    @author Austin Appleby (MurmurHash3)
    @author Paul Hsieh (SuperFastHash)
    @author Leandro Rabindranath Leon (adaptation and integration)

    @ingroup Hash
*/


/** @file hash-fct.H
    @brief Collection of general-purpose hash functions.

    This file provides a comprehensive collection of hash functions suitable
    for use in hash tables, bloom filters, checksums, and other applications
    requiring hash computation.

    ## Hash Function Categories

    The functions are organized into several categories:

    ### Simple Hash Functions (Educational/Legacy)
    - `add_hash()` - Simple additive hash (poor distribution)
    - `xor_hash()` - XOR folding hash (poor distribution)
    - `rot_hash()` - Rotating hash (acceptable minimum)

    ### Production-Quality Hash Functions
    - `djb_hash()` - Dan Bernstein's hash (good for small strings)
    - `sax_hash()` - Shift-Add-XOR hash (flexible, good distribution)
    - `fnv_hash()` - Fowler-Noll-Vo hash (simple, effective)
    - `oat_hash()` - One-at-a-Time hash by Bob Jenkins
    - `jsw_hash()` - Julienne Walker's hash (uses random table)
    - `elf_hash()` - ELF object file hash (well-tested)

    ### High-Quality Hash Functions
    - `jen_hash()` - Jenkins hash (excellent avalanche properties)
    - `SuperFastHash()` - Paul Hsieh's fast hash
    - `murmur3hash()` - MurmurHash3 (modern, high-quality)

    ## Usage Examples

    @code
    #include <hash-fct.H>
    using namespace Aleph;

    // Hash a string
    std::string key = "example";
    size_t h1 = dft_hash_fct(key);  // Uses SuperFastHash
    size_t h2 = fnv_hash(key);      // Uses FNV hash

    // Hash an integer
    int value = 42;
    size_t h3 = djb_hash(value);

    // Hash raw bytes
    char data[] = {1, 2, 3, 4};
    size_t h4 = jen_hash(data, sizeof(data));

    // Hash with seed (for double hashing)
    size_t h5 = murmur3hash(key, 12345);
    @endcode

    ## References

    Most simple hash functions are adapted from Julienne Walker's tutorial:
    http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx

    MurmurHash3 is ported from Austin Appleby's implementation:
    https://github.com/aappleby/smhasher

    SuperFastHash is by Paul Hsieh:
    http://www.azillionmonkeys.com/qed/hash.html

    @author Julienne Walker (original simple hashes)
    @author Bob Jenkins (Jenkins hash, One-at-a-Time)
    @author Austin Appleby (MurmurHash3)
    @author Paul Hsieh (SuperFastHash)
    @author Leandro Rabindranath Leon (adaptation and integration)

    @ingroup Hash
*/

# ifndef HASH_FCT_H
# define HASH_FCT_H

# include <cstdint>
# include <cstdlib>
# include <cstring>
# include <string>
# include <primes.H>

namespace Aleph
{

  /** @brief Hash functions (implementaciones concretas).
   *
   *  Este header aporta implementaciones de funciones hash (educativas y
   *  de uso general) para usarlas con las distintas tablas hash de Aleph-w.
   *
   *  @ingroup Hash
   */

  /** Default seed value for seeded hash functions.

      This value is used as the default initialization for hash functions
      that support seeding, such as Jenkins hash and MurmurHash3.

      @ingroup Hash
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
  inline size_t add_hash(const void * key, const size_t len) noexcept
  {
    const auto *p = static_cast<const unsigned char *>(key);
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
  inline size_t xor_hash(const void * key, const size_t len) noexcept
  {
    const auto *p = static_cast<const unsigned char *>(key);
    size_t h = 0;

    for (size_t i = 0; i < len; i++)
      h ^= p[i];

    return h;
  }


  /** Rotating hash

      "The rotating hash is identical to the XOR hash except instead of
      simply folding each byte of the input into the internal state, it
      also performs a fold of the internal state before combining it with
      the byte of the input. This extra mixing step is enough to give
      the rotating hash a much better distribution. Much of the time, the
      rotating hash is sufficient, and can be considered the minimal
      acceptable algorithm. Notice that with each improvement, the
      internal state is being mixed up more and more. This is a key
      element in a good hash function" 

      @author Julienne Walker
      @ingroup Hash
  */
  inline size_t rot_hash(const void * key, const size_t len) noexcept
  {
    const auto *p = static_cast<const unsigned char *>(key);
    size_t h = 0;
  
    for (size_t i = 0; i < len; i++)
      h = (h << 4) ^(h >> 28) ^ p[i];
  
    return h;
  }


  /** Modified Bernstein hash

      "Dan Bernstein created this algorithm and posted it in a
      newsgroup. It is known by many as the Chris Torek hash because Chris
      went a long way toward popularizing it. Since then, it has been used
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
  inline size_t djb_hash(const void * key, const size_t len) noexcept
  {
    const auto *p = static_cast<const unsigned char *>(key);
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
  inline size_t sax_hash(const void * key, const size_t len) noexcept
  {
    const auto *p = static_cast<const unsigned char *>(key);
    size_t h = 0;  
  
    for (size_t i = 0; i < len; i++)
      h ^= (h << 5) +(h >> 2) + p[i];
  
    return h;
  }

  /** Fowler-Noll-Vo (FNV-1) hash function.

      "The FNV hash, short for Fowler/Noll/Vo in honor of the creators, is
      a very powerful algorithm that, not surprisingly, follows the same
      lines as Bernstein's modified hash with carefully chosen
      constants. This algorithm has been used in many applications with
      wonderful results, and for its simplicity, the FNV hash should be
      one of the first hashes tried in an application. It is also
      recommended that the FNV website be visited for useful descriptions
      of how to modify the algorithm for various uses."

      The magic constants are:
      - 2166136261 (0x811c9dc5) - FNV offset basis for 32-bit
      - 16777619 (0x01000193) - FNV prime for 32-bit

      @param[in] key Pointer to the data to hash.
      @param[in] len Length of the data in bytes.
      @return The computed hash value.

      @note This implements FNV-1. The FNV-1a variant (which XORs before
            multiplying) is slightly better for short strings but this
            version is kept for compatibility.

      @see http://www.isthe.com/chongo/tech/comp/fnv/
      @author Julienne Walker
      @ingroup Hash
  */
  inline size_t fnv_hash(const void * key, const size_t len) noexcept
  {
    const auto *p = static_cast<const unsigned char *>(key);
    size_t h = 2166136261;

    for (size_t i = 0; i < len; i++)
      h = (h * 16777619) ^ p[i];

    return h;
  }


  /** One-at-a-Time hash by Bob Jenkins.

      This hash function processes one byte at a time with thorough mixing,
      providing excellent distribution properties. It was designed by Bob
      Jenkins and is known for its simplicity and effectiveness.

      The algorithm performs three operations per byte:
      1. Add the byte value
      2. Add shifted hash (mix in)
      3. XOR with right-shifted hash (fold)

      A final mixing step ensures good avalanche behavior.

      @param[in] key Pointer to the data to hash.
      @param[in] len Length of the data in bytes.
      @return The computed hash value.

      @see http://burtleburtle.net/bob/hash/doobs.html
      @author Bob Jenkins
      @ingroup Hash
  */
  inline size_t oat_hash(const void * key, const size_t len) noexcept
  {
    const auto *p = static_cast<const unsigned char *>(key);
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
  extern size_t jsw_hash(const void * key, size_t len);


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
  inline size_t elf_hash(const void * key, const size_t len) noexcept
  {
    const auto *p = static_cast<const unsigned char *>(key);
    size_t h = 0;
  
    for (size_t i = 0; i < len; i++) 
      {
        h = (h << 4) + p[i];
        size_t g = h & 0xf0000000L;
 
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
  inline size_t jen_hash(const void * key, const size_t length,
                         const unsigned initval = Default_Hash_Seed) noexcept
  {
    const auto * k = static_cast<const unsigned char *>(key);
    size_t b;
    size_t c = initval;
    size_t len = length;

    size_t a = b = 0x9e3779b9;

    while (len >= 12)
      {
        a += (k[0] +(static_cast<size_t>(k[1]) << 8) + (static_cast<size_t>(k[2]) << 16) +
              (static_cast<size_t>(k[3]) << 24));
        b += (k[4] +(static_cast<size_t>(k[5]) << 8) + (static_cast<size_t>(k[6]) << 16) +
              (static_cast<size_t>(k[7]) << 24));
        c += (k[8] +(static_cast<size_t>(k[9]) << 8) + (static_cast<size_t>(k[10]) << 16) +
              (static_cast<size_t>(k[11]) << 24));

        mix(a, b, c);

        k += 12;
        len -= 12;
      }

    c += length;

    switch(len)
      {
      case 11: c += (static_cast<size_t>(k[10]) << 24); [[fallthrough]];
      case 10: c += (static_cast<size_t>(k[9]) << 16);  [[fallthrough]];
      case 9 : c += (static_cast<size_t>(k[8]) << 8);   [[fallthrough]];
        /* First byte of c reserved for length */
      case 8 : b += (static_cast<size_t>(k[7]) << 24);  [[fallthrough]];
      case 7 : b += (static_cast<size_t>(k[6]) << 16);  [[fallthrough]];
      case 6 : b += (static_cast<size_t>(k[5]) << 8);   [[fallthrough]];
      case 5 : b += k[4];                  [[fallthrough]];
      case 4 : a += (static_cast<size_t>(k[3]) << 24);  [[fallthrough]];
      case 3 : a += (static_cast<size_t>(k[2]) << 16);  [[fallthrough]];
      case 2 : a += (static_cast<size_t>(k[1]) << 8);   [[fallthrough]];
      case 1 : a += k[0];                  break;
      default: break;
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


  inline size_t murmur3hash(const char * key, const unsigned long seed)
  {
    const size_t len = strlen(key);
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
  size_t murmur3hash(const std::string & key, const unsigned long seed)
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


  /* Paul Hsieh superfast hash function.

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
  inline size_t SuperFastHash(const void * key, size_t len) noexcept
  {
    const auto * data = static_cast<const unsigned char *>(key);
    size_t hash = len;

    if (len <= 0 || data == nullptr)
      return 0;

    const int rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; --len)
      {
        hash  += get16bits (data);
        const size_t tmp = (get16bits(data+2) << 11) ^ hash;
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
        hash ^= static_cast<signed char>(data[sizeof(uint16_t)]) << 18;
        hash += hash >> 11;
        break;
      case 2:
        hash += get16bits (data);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
      case 1:
        hash += static_cast<signed char>(*data);
        hash ^= hash << 10;
        hash += hash >> 1;
        break;
      default:
        break;
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
    return add_hash(&key, sizeof(key));
  }

  template <typename Key> inline
  size_t xor_hash(const Key & key) noexcept
  {
    return xor_hash(&key, sizeof(key));
  }

  template <typename Key>
  size_t rot_hash(const Key & key) noexcept
  {
    return rot_hash(&key, sizeof(key));
  }

  template <typename Key>
  size_t djb_hash(const Key & key) noexcept
  {
    return djb_hash(&key, sizeof(key));
  }

  template <typename Key>
  size_t sax_hash(const Key & key) noexcept
  {
    return sax_hash(&key, sizeof(key));
  }

  template <typename Key>
  size_t fnv_hash(const Key & key) noexcept
  {
    return fnv_hash(&key, sizeof(key));
  }

  template <typename Key>
  size_t oat_hash(const Key & key) noexcept
  {
    return oat_hash(&key, sizeof(key));
  }

  template <typename Key>
  size_t jsw_hash(const Key & key)
  {
    return jsw_hash(&key, sizeof(key));
  }

  template <typename Key>
  size_t elf_hash(const Key & key) noexcept
  {
    return elf_hash(&key, sizeof(key));
  }

  template <typename Key>
  size_t jen_hash(const Key &  key, const unsigned initval) noexcept
  {
    return jen_hash(&key, sizeof(key), initval);
  }

  template <typename Key>
  size_t SuperFastHash(const Key &  key) noexcept
  {
    return SuperFastHash(&key, sizeof(key));
  }

  inline size_t add_hash(const char * key) noexcept
  {
    auto p = reinterpret_cast<const unsigned char *>(key);
    size_t h = 0;
  
    while (*p)
      h += *p++;
  
    return h;
  }

  inline size_t xor_hash(const char * key) noexcept
  {
    auto p = reinterpret_cast<const unsigned char *>(key);
    size_t h = 0;
  
    while (*p)
      h ^= *p++;
  
    return h;
  }

  inline size_t rot_hash(const char * key) noexcept
  {
    auto p = reinterpret_cast<const unsigned char *>(key);
    size_t h = 0;

    while (*p)
      h = (h << 4) ^(h >> 28) ^ *p++;

    return h;
  }

  inline size_t djb_hash(const char * key) noexcept
  {
    auto p = reinterpret_cast<const unsigned char *>(key);
    size_t h = 0;

    while (*p)
      h = 33 * h ^ *p++;

    return h;
  }

  inline size_t sax_hash(const char * key) noexcept
  {
    auto p = reinterpret_cast<const unsigned char *>(key);
    size_t h = 0;

    while (*p)
      h ^= (h << 5) +(h >> 2) + *p++;

    return h;
  }

  inline size_t fnv_hash(const char * key) noexcept
  {
    auto p = reinterpret_cast<const unsigned char *>(key);
    size_t h = 2166136261;

    while (*p)
      h = (h * 16777619) ^ *p++;

    return h;
  }

  inline size_t oat_hash(const char * key) noexcept
  {
    auto p = reinterpret_cast<const unsigned char *>(key);
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
    auto p = reinterpret_cast<const unsigned char *>(key);
    size_t h = 0;

    while (*p)
      {
        h =(h << 4) + *p++;
        size_t g = h & 0xf0000000L;

        if(g != 0)
          h ^= g >> 24;

        h &= ~g;
      }

    return h;
  }

  inline size_t SuperFastHash(const char * key) noexcept
  {
    return SuperFastHash(key, strlen(key));
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

  inline size_t jsw_hash(const std::string & key)
  {
    return jsw_hash(key.c_str());
  }

  inline size_t elf_hash(const std::string & key) noexcept
  {
    return elf_hash(key.c_str());
  }

  inline size_t jen_hash(const std::string & key, unsigned initval) noexcept
  {
    return jen_hash(key.c_str(), key.size(), initval);
  }

  inline size_t SuperFastHash(const std::string & key) noexcept
  {
    return SuperFastHash(key.c_str(), key.size());
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
