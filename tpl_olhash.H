

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

    void remove(const Key & key)
    {
      Key * key_ptr = search(key);
      if (key_ptr == nullptr)
        throw std::domain_error("Key not in hash table");

      this->remove_ptr(key_ptr);
    }

    OHASH_COMMON(OLhashTable);

    using Stats = typename OhashCommon<OLhashTable<Key, Cmp>, Key>::Stats;

    Stats stats() const
    {
      DynArray<size_t> lens;
      size_t num_busy = 0, num_deleted = 0, num_empty = 0;
      size_t max_len = std::numeric_limits<size_t>::min();
      for (int i = 0; i < len; ++i)
        switch (table[i].status)
          {
          case BUSY:
            {
              ++num_busy;
              const Key & key = table[i].key;
              long i = hash_fct(key) % len;
              size_t count = 1;

              while (true)
                {
                  if (table[i].status == BUSY and cmp(table[i].key, key))
                    break;
                  ++count;
                  if (++i == len)
                    i = 0;
                }

              max_len = std::max(max_len, count);				
              update_stat_len(lens, count);				
              break;							
            }								
          case EMPTY:							
            ++num_empty;							
            update_stat_len(lens, 0);					
            break;								
          case DELETED:							
            ++num_deleted;							
            break;								
          }								
 									
      float avg = 0, sum = 0;						
      for (int i = 0; i < lens.size(); ++i)				
        {									
          avg += lens(i)*i;						
          sum += lens(i);							
        }									
									
      avg /= sum;								
      float var = 0;							
      for (int i = 0; i < lens.size(); ++i)				
        {									
          float s = i - avg;						
          var += lens(i)*s*s;						
        }									
      var /= sum;								
   									
      Stats stats;								
      stats.num_busy = num_busy;						
      stats.num_deleted = num_deleted;					
      stats.num_empty = num_empty;						
      std::swap(lens, stats.lens);						
      stats.avg = avg;							
      stats.var = var;							
      stats.max_len = max_len;						
									
      return stats;						
    }
  };



  template <typename Key, class Cmp = Aleph::equal_to<Key>>
  using SetOLhash = OLhashTable<Key, Cmp>;

								
}
# endif // TPL_OLHASH_H

