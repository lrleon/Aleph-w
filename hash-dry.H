
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
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
# ifndef HASH_DRY_H
# define HASH_DRY_H

# include <ahFunction.H>

# define OHASH_COMMON(class_name)                                     \
  public:                                                             \
                                                                      \
  Hash_Fct get_hash_fct() const noexcept { return hash_fct; }         \
                                                                      \
private:                                                              \
                                                                      \
 Key * test_resize(Bucket * curr_bucket, const Key & key)             \
 {                                                                    \
   if (not ((with_resize and this->current_alpha() >= upper_alpha)))	\
     return &curr_bucket->key;                                        \
                                                                      \
   Key key_copy = key;                                                \
   this->resize(Primes::next_prime(2*len));                           \
                                                                      \
   Key * key_ptr = search(key_copy);                                  \
   assert(key_ptr != nullptr);                                        \
   assert(*key_ptr == key_copy);                                      \
                                                                      \
   return key_ptr;                                                    \
 }                                                                    \
                                                                      \
 static void update_stat_len(DynArray<size_t> & lens, size_t i)       \
 {                                                                    \
   if (lens.exist(i))                                                 \
     lens(i) += 1;                                                    \
   else                                                               \
     lens.touch(i) = 1;                                               \
 }                                                                    \
                                                                      \
public:                                                               \
                                                                      \
 void set_hash_fct(Hash_Fct fct) noexcept                             \
 {                                                                    \
   hash_fct = fct;                                                    \
 }                                                                    \
                                                                      \
 void set_hash_fct(Hash_Fct_Ptr fct) noexcept                         \
 {                                                                    \
   hash_fct = Hash_Fct(fct);                                          \
 }

namespace Aleph
{

  extern const float hash_default_lower_alpha;

  extern const float hash_default_upper_alpha;

}


# endif // HASH_DRY_H
