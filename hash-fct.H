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

# ifndef HASH_FCT_H
# define HASH_FCT_H

# include <cstddef>
# include <cstring>
# include <string>
# include <type_traits>
# include <cstdint>

/** @defgroup Hash Hash functions
    @brief Collection of high-performance hash functions for various data types.
    
    Aleph-w provides a wide range of hash functions, from classical ones
    (like FNV, DJB, ELF) to modern ones (like Murmur3, xxHash64, wyhash, SipHash).
    
    The default policy is defined by:
    - `dft_hash_fct()` - Default hash function (uses SuperFastHash)
    - `snd_hash_fct()` - Second hash function for double hashing (uses SuperFastHash with fixed seed)
    
    Key highlights:
    - Classical: `add_hash()`, `xor_hash()`, `rot_hash()`, `djb_hash()`, `sax_hash()`, `fnv_hash()`, `oat_hash()`, `elf_hash()`.
    - Randomized: `jsw_hash()` (Julienne Walker).
    - Modern non-cryptographic: `SuperFastHash()`, `murmur3hash()`, `xxhash64_hash()`, `wyhash_hash()`.
    - Cryptographically strong (keyed): `siphash24_hash()`.
*/

namespace Aleph
{
  extern const unsigned Default_Hash_Seed;

  /** Initializes the randomized lookup table used by jsw_hash().

      This overload preserves the historical behavior and derives the seed
      from the current time.

      @ingroup Hash
  */
  void init_jsw() noexcept;

  /** Initializes the randomized lookup table used by jsw_hash().

      This overload is deterministic and is intended for reproducible tests
      and benchmarks.

      @param seed Seed used to populate the internal lookup table.
      @ingroup Hash
  */
  void init_jsw(std::uint32_t seed) noexcept;

  /** Checks if the jsw_hash() lookup table has been initialized.

      @return true if initialized, false otherwise.
      @ingroup Hash
  */
  bool is_jsw_initialized() noexcept;

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

      "The rotating hash is identical to the XOR hash except that it
      incorporates a shift and the addition of the next byte into the loop
      instead of just XORing it. Many people mistakenly call this hash the
      DJB hash"

      @author Julienne Walker
      @ingroup Hash
  */
  inline size_t rot_hash(const void * key, const size_t len) noexcept
  {
    const auto *p = static_cast<const unsigned char *>(key);
    size_t h = 0;

    for (size_t i = 0; i < len; i++)
      h = (h << 4) ^ (h >> 28) ^ p[i];

    return h;
  }

  /** Bernstein's hash (DJB hash)

      "The DJB hash is a simple and effective hash function that works well
      on many types of data. It was first published by Daniel J. Bernstein
      on comp.lang.c"

      @author Daniel J. Bernstein
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

  /** Shift-Add-XOR hash (SAX hash)

      "The SAX hash is a simple hash function that works well on many types
      of data. It is similar to the rotating hash except that it uses
      addition instead of XOR to mix the bits"

      @author Julienne Walker
      @ingroup Hash
  */
  inline size_t sax_hash(const void * key, const size_t len) noexcept
  {
    const auto *p = static_cast<const unsigned char *>(key);
    size_t h = 0;

    for (size_t i = 0; i < len; i++)
      h ^= (h << 5) + (h >> 2) + p[i];

    return h;
  }

  /** FNV-1a hash

      "The FNV-1a hash is a high-performance hash function that is designed
      to be fast while maintaining a low collision rate. It is widely used
      in many applications"

      @author Fowler, Noll, and Vo
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

  /** One-at-a-Time hash (OAT hash)

      "The One-at-a-Time hash is a simple and effective hash function
      designed by Bob Jenkins. It is used in many applications, including
      the Perl interpreter"

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

      @note If init_jsw() has not been called, it will be lazily
            initialized with a non-deterministic seed (time-based).

      @param key Pointer to the data to hash
      @param len Length of the data in bytes
      @return Hash value

      @author Julienne Walker
      @ingroup Hash
  */
  extern size_t jsw_hash(const void * key, size_t len) noexcept;

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

        if (g != 0)
          h ^= g >> 24;

        h &= ~g;
      }

    return h;
  }

  /** Jenkins hash (lookup3)

      "Bob Jenkins' lookup3.c, is the result of many years of research and
      testing. It is considered one of the best general-purpose hash
      functions available"

      @author Bob Jenkins
      @ingroup Hash
  */
  extern size_t jen_hash(const void *key, size_t len, unsigned initval) noexcept;

  /* The following is just a exact copy of functions implemented by Peter
     Scott. They are at:

     https://github.com/PeterScott/murmur3

     They are a port of MurmurHash  by Austin Appleby. See

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

  template <typename Key>
    requires std::is_trivially_copyable_v<Key>
  inline size_t murmur3hash(const Key & key, std::uint32_t seed)
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


  inline size_t murmur3hash(const char * key, const std::uint32_t seed)
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

  inline size_t murmur3hash(const std::string & key, const std::uint32_t seed)
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
inline std::uint16_t get16bits(const void * p) noexcept
{
  std::uint16_t val;
  memcpy(&val, p, sizeof(val));
  return val;
}

  /** Paul Hsieh super fast hash function.

      @param key Pointer to the data to hash.
      @param len Length of the data in bytes.
      @param seed Optional seed to randomize the hash.
      @author Paul Hsieh
      @ingroup Hash
  */
  inline size_t SuperFastHash(const void * key, size_t len, std::uint32_t seed = 0) noexcept
  {
    const auto * data = static_cast<const unsigned char *>(key);
    std::uint32_t hash = static_cast<std::uint32_t>(len) ^ seed;

    if (len <= 0 || data == nullptr)
      return static_cast<size_t>(hash);

    const int rem = len & 3;
    const size_t nblocks = len >> 2;

    /* Main loop */
    for (size_t i = 0; i < nblocks; ++i)
      {
        hash  += get16bits (data);
        const std::uint32_t tmp = (static_cast<std::uint32_t>(get16bits(data+2)) << 11) ^ hash;
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

    return static_cast<size_t>(hash);
  }

  /** xxHash64 from the xxHash family.

      This function implements the 64-bit xxHash variant and is intended as
      a modern high-throughput non-cryptographic hash for in-memory tables,
      sketches and filters.

      @param key Pointer to the input bytes.
      @param len Length of the input in bytes.
      @param seed Seed used to randomize the hash stream.
      @return Hash value truncated to size_t.
      @ingroup Hash
  */
  size_t xxhash64_hash(const void * key, size_t len,
                       std::uint64_t seed = Default_Hash_Seed) noexcept;

  /** wyhash non-cryptographic hash.

      This function implements a wyhash-style 64-bit hash suitable for
      hash tables and other latency-sensitive data structures.

      @param key Pointer to the input bytes.
      @param len Length of the input in bytes.
      @param seed Seed used to randomize the hash stream.
      @return Hash value truncated to size_t.
      @ingroup Hash
  */
  size_t wyhash_hash(const void * key, size_t len,
                     std::uint64_t seed = Default_Hash_Seed) noexcept;

  /** SipHash-2-4 keyed hash.

      This function implements SipHash-2-4, a keyed PRF-style hash suitable
      for adversarial or untrusted keys.

      @param key Pointer to the input bytes.
      @param len Length of the input in bytes.
      @param key0 First 64-bit secret key word.
      @param key1 Second 64-bit secret key word.
      @return Hash value truncated to size_t.
      @ingroup Hash
  */
  size_t siphash24_hash(const void * key, size_t len,
                        std::uint64_t key0 = 0x0706050403020100ULL,
                        std::uint64_t key1 = 0x0f0e0d0c0b0a0908ULL) noexcept;

  template <typename Key>
    requires std::is_trivially_copyable_v<Key>
  inline size_t add_hash(const Key & key) noexcept
  {
    return add_hash(&key, sizeof(key));
  }

  template <typename Key>
    requires std::is_trivially_copyable_v<Key>
  inline size_t xor_hash(const Key & key) noexcept
  {
    return xor_hash(&key, sizeof(key));
  }

  template <typename Key>
    requires std::is_trivially_copyable_v<Key>
  inline size_t rot_hash(const Key & key) noexcept
  {
    return rot_hash(&key, sizeof(key));
  }

  template <typename Key>
    requires std::is_trivially_copyable_v<Key>
  inline size_t djb_hash(const Key & key) noexcept
  {
    return djb_hash(&key, sizeof(key));
  }

  template <typename Key>
    requires std::is_trivially_copyable_v<Key>
  inline size_t sax_hash(const Key & key) noexcept
  {
    return sax_hash(&key, sizeof(key));
  }

  template <typename Key>
    requires std::is_trivially_copyable_v<Key>
  inline size_t fnv_hash(const Key & key) noexcept
  {
    return fnv_hash(&key, sizeof(key));
  }

  template <typename Key>
    requires std::is_trivially_copyable_v<Key>
  inline size_t oat_hash(const Key & key) noexcept
  {
    return oat_hash(&key, sizeof(key));
  }

  /// JSW hash for arbitrary key types
  /// @ingroup Hash
  template <typename Key>
    requires std::is_trivially_copyable_v<Key>
  inline size_t jsw_hash(const Key & key) noexcept
  {
    return jsw_hash(&key, sizeof(key));
  }

  template <typename Key>
    requires std::is_trivially_copyable_v<Key>
  inline size_t elf_hash(const Key & key) noexcept
  {
    return elf_hash(&key, sizeof(key));
  }

  template <typename Key>
    requires std::is_trivially_copyable_v<Key>
  inline size_t jen_hash(const Key &  key, const unsigned initval) noexcept
  {
    return jen_hash(&key, sizeof(key), initval);
  }

  template <typename Key>
    requires (std::is_trivially_copyable_v<Key> and 
              (not std::is_pointer_v<Key>) and (not std::is_array_v<Key>))
  inline size_t SuperFastHash(const Key & key, std::uint32_t seed = 0) noexcept
  {
    return SuperFastHash(&key, sizeof(key), seed);
  }

  template <typename Key>
    requires std::is_trivially_copyable_v<Key>
  size_t xxhash64_hash(const Key & key,
                       std::uint64_t seed = Default_Hash_Seed) noexcept
  {
    return xxhash64_hash(&key, sizeof(key), seed);
  }

  template <typename Key>
    requires std::is_trivially_copyable_v<Key>
  size_t wyhash_hash(const Key & key,
                     std::uint64_t seed = Default_Hash_Seed) noexcept
  {
    return wyhash_hash(&key, sizeof(key), seed);
  }

  template <typename Key>
    requires std::is_trivially_copyable_v<Key>
  size_t siphash24_hash(const Key & key,
                        std::uint64_t key0 = 0x0706050403020100ULL,
                        std::uint64_t key1 = 0x0f0e0d0c0b0a0908ULL) noexcept
  {
    return siphash24_hash(&key, sizeof(key), key0, key1);
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

  extern size_t jsw_hash(const char * key) noexcept;

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

  inline size_t SuperFastHash(const char * key, std::uint32_t seed = 0) noexcept
  {
    return SuperFastHash(key, strlen(key), seed);
  }

  inline size_t xxhash64_hash(const char * key,
                              std::uint64_t seed = Default_Hash_Seed) noexcept
  {
    return xxhash64_hash(key, strlen(key), seed);
  }

  inline size_t wyhash_hash(const char * key,
                            std::uint64_t seed = Default_Hash_Seed) noexcept
  {
    return wyhash_hash(key, strlen(key), seed);
  }

  inline size_t siphash24_hash(const char * key,
                               std::uint64_t key0 = 0x0706050403020100ULL,
                               std::uint64_t key1 = 0x0f0e0d0c0b0a0908ULL) noexcept
  {
    return siphash24_hash(key, strlen(key), key0, key1);
  }

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

  /// JSW hash for std::string
  /// @ingroup Hash
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
    return jen_hash(key.c_str(), key.size(), initval);
  }

  inline size_t SuperFastHash(const std::string & key, std::uint32_t seed = 0) noexcept
  {
    return SuperFastHash(key.c_str(), key.size(), seed);
  }

  inline size_t xxhash64_hash(const std::string & key,
                              std::uint64_t seed = Default_Hash_Seed) noexcept
  {
    return xxhash64_hash(key.data(), key.size(), seed);
  }

  inline size_t wyhash_hash(const std::string & key,
                            std::uint64_t seed = Default_Hash_Seed) noexcept
  {
    return wyhash_hash(key.data(), key.size(), seed);
  }

  inline size_t siphash24_hash(const std::string & key,
                               std::uint64_t key0 = 0x0706050403020100ULL,
                               std::uint64_t key1 = 0x0f0e0d0c0b0a0908ULL) noexcept
  {
    return siphash24_hash(key.data(), key.size(), key0, key1);
  }

  template <typename Key>
    requires (std::is_trivially_copyable_v<Key> and
              (not std::is_pointer_v<Key>) and (not std::is_array_v<Key>))
  inline size_t dft_hash_fct(const Key & key) noexcept
  {
    return SuperFastHash(key);
  }

  template <typename Key>
    requires std::is_pointer_v<Key>
  inline size_t dft_hash_fct(const Key & key) noexcept
  {
    return SuperFastHash(&key, sizeof(key));
  }

  inline size_t dft_hash_fct(const std::string & key) noexcept
  {
    return SuperFastHash(key);
  }

  template <typename Key>
    requires (std::is_trivially_copyable_v<Key> and
              (not std::is_pointer_v<Key>) and (not std::is_array_v<Key>))
  inline size_t dft_hash_fct(const Key & key, std::uint32_t seed) noexcept
  {
    return SuperFastHash(key, seed);
  }

  template <typename Key>
    requires std::is_pointer_v<Key>
  inline size_t dft_hash_fct(const Key & key, std::uint32_t seed) noexcept
  {
    return SuperFastHash(&key, sizeof(key), seed);
  }

  inline size_t dft_hash_fct(const std::string & key, std::uint32_t seed) noexcept
  {
    return SuperFastHash(key, seed);
  }

  template <typename Key>
  inline size_t dft_hash_ptr_fct(const Key & key) noexcept
  {
    return dft_hash_fct(key);
  }

  template <typename Key>
  inline size_t dft_seed_hash_ptr_fct(const Key & key,
                                      unsigned long seed) noexcept
  {
    return dft_hash_fct(key, static_cast<std::uint32_t>(seed));
  }

  template <typename Key>
    requires (std::is_trivially_copyable_v<Key> and
              (not std::is_pointer_v<Key>) and (not std::is_array_v<Key>))
  inline size_t snd_hash_fct(const Key & key) noexcept
  {
    return SuperFastHash(key, 52679987);
  }

  template <typename Key>
    requires std::is_pointer_v<Key>
  inline size_t snd_hash_fct(const Key & key) noexcept
  {
    return SuperFastHash(&key, sizeof(key), 52679987);
  }

  inline size_t snd_hash_fct(const std::string & key) noexcept
  {
    return SuperFastHash(key, 52679987);
  }

  template <typename Key>
  inline size_t snd_hash_ptr_fct(const Key & key) noexcept
  {
    return snd_hash_fct(key);
  }

  template<typename Key, typename Data, typename Fct>
  inline size_t map_hash_fct(Fct fct, const std::pair<Key, Data> & p) noexcept
  {
    return fct(p.first);
  }

  template <typename K1, typename K2>
    requires requires(const K1 & k1, const K2 & k2)
    {
      dft_hash_fct(k1);
      dft_hash_fct(k2);
    }
  inline size_t pair_dft_hash_fct(const std::pair<K1, K2> & p) noexcept
  {
    return dft_hash_fct(p.first) + dft_hash_fct(p.second);
  }

  template <typename K1, typename K2>
    requires requires(const K1 & k1, const K2 & k2)
    {
      dft_hash_fct(k1);
      snd_hash_fct(k2);
    }
  inline size_t pair_snd_hash_fct(const std::pair<K1, K2> & p) noexcept
  {
    return dft_hash_fct(p.first) + snd_hash_fct(p.second);
  }

  template <typename K1, typename K2>
  inline size_t pair_dft_hash_ptr_fct(const std::pair<K1, K2> & p) noexcept
  {
    return pair_dft_hash_fct(p);
  }

  template <typename K1, typename K2>
  inline size_t pair_snd_hash_ptr_fct(const std::pair<K1, K2> & p) noexcept
  {
    return pair_snd_hash_fct(p);
  }

} // end namespace Aleph


# endif // HASH_FCT_H
