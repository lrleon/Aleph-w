

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
