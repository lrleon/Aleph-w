

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

    Iterator(const RingFileCache<T> & cache, const size_t offset = 0)
      : cache_ptr(&const_cast<RingFileCache<T>&>(cache)),
        pos(offset), curr_pos((cache_ptr->head + offset) % cache.dim)
    {
      if (cache_ptr->is_empty())
        return;

      if (not cache_ptr->is_valid_offset(pos))
        return;

      load_curr();
    }

    T get_curr_ne() const noexcept { return curr; }

    T get_curr() const
    {
      if (not has_curr())
        throw overflow_error("RingFileCache::Iterator::get_curr()");
      return get_curr_ne();
    }

    void next_ne() noexcept
    {
      increase_pos();
      if (has_curr())
        load_curr();
    }

    void next()
    {
      if (not has_curr())
        throw overflow_error("RingFileCache::Iterator::next()");
      next_ne();
    }
  };

  auto get_it() { return Iterator(*this); }
};

# endif // RINGFILECACHE_H
