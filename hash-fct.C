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
# include <gsl/gsl_rng.h>
# include <stdexcept>
# include "hash-fct.H"

# include <ah-errors.H>

namespace Aleph
{ 

const unsigned Default_Hash_Seed = 52679987;  

static long tab[256];

static bool init = false; 

// ============================================================================
// Helpers
// ============================================================================

#ifdef __GNUC__
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

static FORCE_INLINE uint32_t rotl32 ( uint32_t x, int8_t r )
{
  return (x << r) | (x >> (32 - r));
}

static FORCE_INLINE uint64_t rotl64 ( uint64_t x, int8_t r )
{
  return (x << r) | (x >> (64 - r));
}

#define	ROTL32(x,y)	rotl32(x,y)
#define ROTL64(x,y)	rotl64(x,y)

#define BIG_CONSTANT(x) (x##LLU)

//-----------------------------------------------------------------------------
// Block read - if your platform needs to do endian-swapping or can only
// handle aligned reads, do the conversion here

#define getblock(p, i) (p[i])

//-----------------------------------------------------------------------------
// Finalization mix - force all bits of a hash block to avalanche

static FORCE_INLINE uint32_t fmix32 ( uint32_t h )
{
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;

  return h;
}

static FORCE_INLINE uint64_t fmix64 ( uint64_t k )
{
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xff51afd7ed558ccd);
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
  k ^= k >> 33;

  return k;
}

static FORCE_INLINE std::uint32_t read_le32(const std::uint8_t * p) noexcept
{
  return static_cast<std::uint32_t>(p[0])
    | (static_cast<std::uint32_t>(p[1]) << 8)
    | (static_cast<std::uint32_t>(p[2]) << 16)
    | (static_cast<std::uint32_t>(p[3]) << 24);
}

static FORCE_INLINE std::uint64_t read_le64(const std::uint8_t * p) noexcept
{
  return static_cast<std::uint64_t>(p[0])
    | (static_cast<std::uint64_t>(p[1]) << 8)
    | (static_cast<std::uint64_t>(p[2]) << 16)
    | (static_cast<std::uint64_t>(p[3]) << 24)
    | (static_cast<std::uint64_t>(p[4]) << 32)
    | (static_cast<std::uint64_t>(p[5]) << 40)
    | (static_cast<std::uint64_t>(p[6]) << 48)
    | (static_cast<std::uint64_t>(p[7]) << 56);
}

static FORCE_INLINE std::uint64_t mul_xor_fold64(std::uint64_t lhs,
                                                 std::uint64_t rhs) noexcept
{
#ifdef __SIZEOF_INT128__
  const __uint128_t prod = static_cast<__uint128_t>(lhs) * rhs;
  return static_cast<std::uint64_t>(prod)
    ^ static_cast<std::uint64_t>(prod >> 64);
#else
  const std::uint64_t lhs_lo = lhs & 0xffffffffULL;
  const std::uint64_t lhs_hi = lhs >> 32;
  const std::uint64_t rhs_lo = rhs & 0xffffffffULL;
  const std::uint64_t rhs_hi = rhs >> 32;
  const std::uint64_t ll = lhs_lo * rhs_lo;
  const std::uint64_t lh = lhs_lo * rhs_hi;
  const std::uint64_t hl = lhs_hi * rhs_lo;
  const std::uint64_t hh = lhs_hi * rhs_hi;
  std::uint64_t low = ll + (lh << 32) + (hl << 32);
  std::uint64_t high = hh + (lh >> 32) + (hl >> 32);
  if (low < ll)
    ++high;
  return low ^ high;
#endif
}

static FORCE_INLINE std::uint64_t xxh64_round(std::uint64_t acc,
                                              std::uint64_t input) noexcept
{
  constexpr std::uint64_t prime2 = 14029467366897019727ULL;
  constexpr std::uint64_t prime1 = 11400714785074694791ULL;
  acc += input * prime2;
  acc = ROTL64(acc, 31);
  acc *= prime1;
  return acc;
}

static FORCE_INLINE std::uint64_t xxh64_merge_round(std::uint64_t acc,
                                                    std::uint64_t value) noexcept
{
  constexpr std::uint64_t prime1 = 11400714785074694791ULL;
  constexpr std::uint64_t prime4 =  9650029242287828579ULL;
  value = xxh64_round(0, value);
  acc ^= value;
  acc = acc * prime1 + prime4;
  return acc;
}

static FORCE_INLINE std::uint64_t wyhash_mix(std::uint64_t lhs,
                                             std::uint64_t rhs) noexcept
{
  return mul_xor_fold64(lhs, rhs);
}

// ============================================================================
// Initialization and classic hashes
// ============================================================================

bool is_jsw_initialized() noexcept
{
  return init;
}
 
void init_jsw() noexcept
{
  init_jsw(static_cast<std::uint32_t>(time(nullptr)));
}

void init_jsw(std::uint32_t seed) noexcept
{
  gsl_rng * r = gsl_rng_alloc (gsl_rng_mt19937);
  gsl_rng_set(r, seed % gsl_rng_max(r));

  for (int i = 0; i < 256; ++i)
    tab[i] = gsl_rng_get(r);

  gsl_rng_free(r); 

  init = true; 
}

size_t jsw_hash(const void * key, size_t len) noexcept
{
  if (not init)
    init_jsw();

  const unsigned char *p = (const unsigned char*) key;
  size_t h = 16777551;

  for (size_t i = 0; i < len; i++)
    h = (h << 1 | h >> 31) ^ tab[p[i]];

  return h;
}

size_t jsw_hash(const char * key) noexcept
{
  if (not init)
    init_jsw();

  const unsigned char * p = (const unsigned char*) key;
  size_t h = 16777551;
  
  while (*p)
    h = (h << 1 | h >> 31) ^ tab[*p++];
  
  return h;
}

#define jen_mix(a,b,c) \
{ \
  a -= c;  a ^= ROTL32(c, 4);  c += b; \
  b -= a;  b ^= ROTL32(a, 6);  a += c; \
  c -= b;  c ^= ROTL32(b, 8);  b += a; \
  a -= c;  a ^= ROTL32(c,16);  c += b; \
  b -= a;  b ^= ROTL32(a,19);  a += c; \
  c -= b;  c ^= ROTL32(b, 4);  b += a; \
}

#define jen_final(a,b,c) \
{ \
  c ^= b; c -= ROTL32(b,14); \
  a ^= c; a -= ROTL32(c,11); \
  b ^= a; b -= ROTL32(a,25); \
  c ^= b; c -= ROTL32(b,16); \
  a ^= c; a -= ROTL32(c,4);  \
  b ^= a; b -= ROTL32(a,14); \
  c ^= b; c -= ROTL32(b,24); \
}

size_t jen_hash(const void *key, size_t length, unsigned initval) noexcept
{
  uint32_t a,b,c;                                          /* internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + initval;

  const uint8_t *p = static_cast<const uint8_t *>(key);

  /*------ all but the last block: safe reads and jen_mix() ------*/
  while (length > 12)
    {
      uint32_t k[3];
      memcpy(k, p, 12);
      a += k[0];
      b += k[1];
      c += k[2];
      jen_mix(a,b,c);
      length -= 12;
      p += 12;
    }

  /*----------------------------- handle the last (up to 11) bytes */
  switch(length)
    {
    case 12: c+=((uint32_t)p[11])<<24; [[fallthrough]];
    case 11: c+=((uint32_t)p[10])<<16; [[fallthrough]];
    case 10: c+=((uint32_t)p[9])<<8;   [[fallthrough]];
    case 9 : c+=p[8];                  [[fallthrough]];
    case 8 : b+=((uint32_t)p[7])<<24;  [[fallthrough]];
    case 7 : b+=((uint32_t)p[6])<<16;  [[fallthrough]];
    case 6 : b+=((uint32_t)p[5])<<8;   [[fallthrough]];
    case 5 : b+=p[4];                  [[fallthrough]];
    case 4 : a+=((uint32_t)p[3])<<24;  [[fallthrough]];
    case 3 : a+=((uint32_t)p[2])<<16;  [[fallthrough]];
    case 2 : a+=((uint32_t)p[1])<<8;   [[fallthrough]];
    case 1 : a+=p[0];
      break;
    case 0 : return static_cast<size_t>(c);
    }

  jen_final(a,b,c);
  return static_cast<size_t>(c);
}

// ============================================================================
// Modern hashes
// ============================================================================

void MurmurHash3_x86_32 ( const void * key, int len,
                          uint32_t seed, void * out )
{
  const uint8_t * data = (const uint8_t*)key;
  const int nblocks = len / 4;
  int i;

  uint32_t h1 = seed;

  uint32_t c1 = 0xcc9e2d51;
  uint32_t c2 = 0x1b873593;

  //----------
  // body

  const uint32_t * blocks = (const uint32_t *)(data + nblocks*4);

  for(i = -nblocks; i; i++)
  {
    uint32_t k1 = getblock(blocks,i);

    k1 *= c1;
    k1 = ROTL32(k1,15);
    k1 *= c2;
    
    h1 ^= k1;
    h1 = ROTL32(h1,13); 
    h1 = h1*5+0xe6546b64;
  }

  //----------
  // tail

  const uint8_t * tail = (const uint8_t*)(data + nblocks*4);

  uint32_t k1 = 0;

  switch(len & 3)
  {
  case 3: k1 ^= tail[2] << 16; [[fallthrough]];
  case 2: k1 ^= tail[1] << 8;  [[fallthrough]];
  case 1: k1 ^= tail[0];
          k1 *= c1; k1 = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
  };

  //----------
  // finalization

  h1 ^= len;

  h1 = fmix32(h1);

  *(uint32_t*)out = h1;
} 

void MurmurHash3_x86_128 ( const void * key, const int len,
                           uint32_t seed, void * out )
{
  const uint8_t * data = (const uint8_t*)key;
  const int nblocks = len / 16;
  int i;

  uint32_t h1 = seed;
  uint32_t h2 = seed;
  uint32_t h3 = seed;
  uint32_t h4 = seed;

  uint32_t c1 = 0x239b961b; 
  uint32_t c2 = 0xab0e9789;
  uint32_t c3 = 0x38b34ae5; 
  uint32_t c4 = 0xa1e38b93;

  //----------
  // body

  const uint32_t * blocks = (const uint32_t *)(data + nblocks*16);

  for(i = -nblocks; i; i++)
  {
    uint32_t k1 = getblock(blocks,i*4+0);
    uint32_t k2 = getblock(blocks,i*4+1);
    uint32_t k3 = getblock(blocks,i*4+2);
    uint32_t k4 = getblock(blocks,i*4+3);

    k1 *= c1; k1  = ROTL32(k1,15); k1 *= c2; h1 ^= k1;

    h1 = ROTL32(h1,19); h1 += h2; h1 = h1*5+0x561ccd1b;

    k2 *= c2; k2  = ROTL32(k2,16); k2 *= c3; h2 ^= k2;

    h2 = ROTL32(h2,17); h2 += h3; h2 = h2*5+0x0bcaa747;

    k3 *= c3; k3  = ROTL32(k3,17); k3 *= c4; h3 ^= k3;

    h3 = ROTL32(h3,15); h3 += h4; h3 = h3*5+0x96cd1c35;

    k4 *= c4; k4  = ROTL32(k4,18); k4 *= c1; h4 ^= k4;

    h4 = ROTL32(h4,13); h4 += h1; h4 = h4*5+0x32ac3b17;
  }

  //----------
  // tail

  const uint8_t * tail = (const uint8_t*)(data + nblocks*16);

  uint32_t k1 = 0;
  uint32_t k2 = 0;
  uint32_t k3 = 0;
  uint32_t k4 = 0;

  switch(len & 15)
  {
  case 15: k4 ^= tail[14] << 16; [[fallthrough]];
  case 14: k4 ^= tail[13] << 8;  [[fallthrough]];
  case 13: k4 ^= tail[12] << 0;
           k4 *= c4; k4  = ROTL32(k4,18); k4 *= c1; h4 ^= k4;
           [[fallthrough]];
  case 12: k3 ^= tail[11] << 24; [[fallthrough]];
  case 11: k3 ^= tail[10] << 16; [[fallthrough]];
  case 10: k3 ^= tail[ 9] << 8;  [[fallthrough]];
  case  9: k3 ^= tail[ 8] << 0;
           k3 *= c3; k3  = ROTL32(k3,17); k3 *= c4; h3 ^= k3;
           [[fallthrough]];
  case  8: k2 ^= tail[ 7] << 24; [[fallthrough]];
  case  7: k2 ^= tail[ 6] << 16; [[fallthrough]];
  case  6: k2 ^= tail[ 5] << 8;  [[fallthrough]];
  case  5: k2 ^= tail[ 4] << 0;
           k2 *= c2; k2  = ROTL32(k2,16); k2 *= c3; h2 ^= k2;
           [[fallthrough]];
  case  4: k1 ^= tail[ 3] << 24; [[fallthrough]];
  case  3: k1 ^= tail[ 2] << 16; [[fallthrough]];
  case  2: k1 ^= tail[ 1] << 8;  [[fallthrough]];
  case  1: k1 ^= tail[ 0] << 0;
           k1 *= c1; k1  = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
  };

  //----------
  // finalization

  h1 ^= len; h2 ^= len; h3 ^= len; h4 ^= len;

  h1 += h2; h1 += h3; h1 += h4;
  h2 += h1; h3 += h1; h4 += h1;

  h1 = fmix32(h1);
  h2 = fmix32(h2);
  h3 = fmix32(h3);
  h4 = fmix32(h4);

  h1 += h2; h1 += h3; h1 += h4;
  h2 += h1; h3 += h1; h4 += h1;

  ((uint32_t*)out)[0] = h1;
  ((uint32_t*)out)[1] = h2;
  ((uint32_t*)out)[2] = h3;
  ((uint32_t*)out)[3] = h4;
}

void MurmurHash3_x64_128 ( const void * key, const int len,
                           const uint32_t seed, void * out )
{
  const uint8_t * data = (const uint8_t*)key;
  const int nblocks = len / 16;
  int i;

  uint64_t h1 = seed;
  uint64_t h2 = seed;

  uint64_t c1 = BIG_CONSTANT(0x87c37b91114253d5);
  uint64_t c2 = BIG_CONSTANT(0x4cf5ad432745937f);

  //----------
  // body

  const uint64_t * blocks = (const uint64_t *)(data);

  for(i = 0; i < nblocks; i++)
  {
    uint64_t k1 = getblock(blocks,i*2+0);
    uint64_t k2 = getblock(blocks,i*2+1);

    k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;

    h1 = ROTL64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

    k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;

    h2 = ROTL64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
  }

  //----------
  // tail

  const uint8_t * tail = (const uint8_t*)(data + nblocks*16);

  uint64_t k1 = 0;
  uint64_t k2 = 0;

  switch(len & 15)
  {
  case 15: k2 ^= (uint64_t)(tail[14]) << 48; [[fallthrough]];
  case 14: k2 ^= (uint64_t)(tail[13]) << 40; [[fallthrough]];
  case 13: k2 ^= (uint64_t)(tail[12]) << 32; [[fallthrough]];
  case 12: k2 ^= (uint64_t)(tail[11]) << 24; [[fallthrough]];
  case 11: k2 ^= (uint64_t)(tail[10]) << 16; [[fallthrough]];
  case 10: k2 ^= (uint64_t)(tail[ 9]) << 8;  [[fallthrough]];
  case  9: k2 ^= (uint64_t)(tail[ 8]) << 0;
           k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;
           [[fallthrough]];
  case  8: k1 ^= (uint64_t)(tail[ 7]) << 56; [[fallthrough]];
  case  7: k1 ^= (uint64_t)(tail[ 6]) << 48; [[fallthrough]];
  case  6: k1 ^= (uint64_t)(tail[ 5]) << 40; [[fallthrough]];
  case  5: k1 ^= (uint64_t)(tail[ 4]) << 32; [[fallthrough]];
  case  4: k1 ^= (uint64_t)(tail[ 3]) << 24; [[fallthrough]];
  case  3: k1 ^= (uint64_t)(tail[ 2]) << 16; [[fallthrough]];
  case  2: k1 ^= (uint64_t)(tail[ 1]) << 8;  [[fallthrough]];
  case  1: k1 ^= (uint64_t)(tail[ 0]) << 0;
           k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;
  };

  //----------
  // finalization

  h1 ^= len; h2 ^= len;

  h1 += h2;
  h2 += h1;

  h1 = fmix64(h1);
  h2 = fmix64(h2);

  h1 += h2;
  h2 += h1;

  ((uint64_t*)out)[0] = h1;
  ((uint64_t*)out)[1] = h2;
}

size_t xxhash64_hash(const void * key, size_t len, std::uint64_t seed) noexcept
{
  constexpr std::uint64_t prime1 = 11400714785074694791ULL;
  constexpr std::uint64_t prime2 = 14029467366897019727ULL;
  constexpr std::uint64_t prime3 =  1609587929392839161ULL;
  constexpr std::uint64_t prime4 =  9650029242287828579ULL;
  constexpr std::uint64_t prime5 =  2870177450012600261ULL;

  const auto * p = static_cast<const std::uint8_t *>(key);
  const auto * const end = p + len;
  std::uint64_t hash = 0;

  if (len >= 32)
    {
      const auto * const limit = end - 32;
      std::uint64_t v1 = seed + prime1 + prime2;
      std::uint64_t v2 = seed + prime2;
      std::uint64_t v3 = seed + 0;
      std::uint64_t v4 = seed - prime1;

      do
        {
          v1 = xxh64_round(v1, read_le64(p)); p += 8;
          v2 = xxh64_round(v2, read_le64(p)); p += 8;
          v3 = xxh64_round(v3, read_le64(p)); p += 8;
          v4 = xxh64_round(v4, read_le64(p)); p += 8;
        }
      while (p <= limit);

      hash = ROTL64(v1, 1) + ROTL64(v2, 7) + ROTL64(v3, 12) + ROTL64(v4, 18);
      hash = xxh64_merge_round(hash, v1);
      hash = xxh64_merge_round(hash, v2);
      hash = xxh64_merge_round(hash, v3);
      hash = xxh64_merge_round(hash, v4);
    }
  else
    hash = seed + prime5;

  hash += len;

  while (p + 8 <= end)
    {
      const std::uint64_t k1 = xxh64_round(0, read_le64(p));
      hash ^= k1;
      hash = ROTL64(hash, 27) * prime1 + prime4;
      p += 8;
    }

  if (p + 4 <= end)
    {
      hash ^= static_cast<std::uint64_t>(read_le32(p)) * prime1;
      hash = ROTL64(hash, 23) * prime2 + prime3;
      p += 4;
    }

  while (p < end)
    {
      hash ^= static_cast<std::uint64_t>(*p++) * prime5;
      hash = ROTL64(hash, 11) * prime1;
    }

  hash ^= hash >> 33;
  hash *= prime2;
  hash ^= hash >> 29;
  hash *= prime3;
  hash ^= hash >> 32;

  return static_cast<size_t>(hash);
}

size_t wyhash_hash(const void * key, size_t len, std::uint64_t seed) noexcept
{
  static constexpr std::uint64_t secret[] =
    {
      0xa0761d6478bd642fULL,
      0xe7037ed1a0b428dbULL,
      0x8ebc6af09c88c6e3ULL,
      0x589965cc75374cc3ULL
    };

  const auto * p = static_cast<const std::uint8_t *>(key);
  std::uint64_t a = 0;
  std::uint64_t b = 0;
  std::uint64_t remaining = len;

  seed ^= secret[0];

  auto read_wyr3 = [] (const std::uint8_t * data, size_t n) noexcept
    {
      return (static_cast<std::uint64_t>(data[0]) << 16)
        | (static_cast<std::uint64_t>(data[n >> 1]) << 8)
        | static_cast<std::uint64_t>(data[n - 1]);
    };

  if (remaining <= 16)
    {
      if (remaining >= 4)
        {
          const size_t delta = (remaining >> 3) << 2;
          a = (static_cast<std::uint64_t>(read_le32(p)) << 32)
            | read_le32(p + delta);
          b = (static_cast<std::uint64_t>(read_le32(p + remaining - 4)) << 32)
            | read_le32(p + remaining - 4 - delta);
        }
      else if (remaining > 0)
        a = read_wyr3(p, remaining);
    }
  else
    {
      if (remaining > 48)
        {
          std::uint64_t see1 = seed;
          std::uint64_t see2 = seed;
          do
            {
              seed = wyhash_mix(read_le64(p) ^ secret[1], read_le64(p + 8) ^ seed);
              see1 = wyhash_mix(read_le64(p + 16) ^ secret[2],
                                read_le64(p + 24) ^ see1);
              see2 = wyhash_mix(read_le64(p + 32) ^ secret[3],
                                read_le64(p + 40) ^ see2);
              p += 48;
              remaining -= 48;
            }
          while (remaining > 48);
          seed ^= see1 ^ see2;
        }

      while (remaining > 16)
        {
          seed = wyhash_mix(read_le64(p) ^ secret[1], read_le64(p + 8) ^ seed);
          p += 16;
          remaining -= 16;
        }

      a = read_le64(p + remaining - 16);
      b = read_le64(p + remaining - 8);
    }

  return static_cast<size_t>(
      wyhash_mix(secret[1] ^ len, wyhash_mix(a ^ secret[1], b ^ seed)));
}

size_t siphash24_hash(const void * key, size_t len,
                      std::uint64_t key0, std::uint64_t key1) noexcept
{
  const auto * in = static_cast<const std::uint8_t *>(key);
  const auto * const end = in + (len - (len & 7));

  std::uint64_t v0 = 0x736f6d6570736575ULL ^ key0;
  std::uint64_t v1 = 0x646f72616e646f6dULL ^ key1;
  std::uint64_t v2 = 0x6c7967656e657261ULL ^ key0;
  std::uint64_t v3 = 0x7465646279746573ULL ^ key1;

  auto sipround = [&]() noexcept
    {
      v0 += v1;
      v1 = ROTL64(v1, 13);
      v1 ^= v0;
      v0 = ROTL64(v0, 32);
      v2 += v3;
      v3 = ROTL64(v3, 16);
      v3 ^= v2;
      v0 += v3;
      v3 = ROTL64(v3, 21);
      v3 ^= v0;
      v2 += v1;
      v1 = ROTL64(v1, 17);
      v1 ^= v2;
      v2 = ROTL64(v2, 32);
    };

  for (; in != end; in += 8)
    {
      const std::uint64_t m = read_le64(in);
      v3 ^= m;
      sipround();
      sipround();
      v0 ^= m;
    }

  std::uint64_t b = static_cast<std::uint64_t>(len) << 56;
  switch (len & 7)
    {
    case 7: b |= static_cast<std::uint64_t>(in[6]) << 48; [[fallthrough]];
    case 6: b |= static_cast<std::uint64_t>(in[5]) << 40; [[fallthrough]];
    case 5: b |= static_cast<std::uint64_t>(in[4]) << 32; [[fallthrough]];
    case 4: b |= static_cast<std::uint64_t>(in[3]) << 24; [[fallthrough]];
    case 3: b |= static_cast<std::uint64_t>(in[2]) << 16; [[fallthrough]];
    case 2: b |= static_cast<std::uint64_t>(in[1]) << 8;  [[fallthrough]];
    case 1: b |= static_cast<std::uint64_t>(in[0]);       [[fallthrough]];
    default: break;
    }

  v3 ^= b;
  sipround();
  sipround();
  v0 ^= b;
  v2 ^= 0xff;
  sipround();
  sipround();
  sipround();
  sipround();

  return static_cast<size_t>(v0 ^ v1 ^ v2 ^ v3);
}

} // end namespace Aleph
