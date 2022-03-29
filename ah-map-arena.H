

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

  void commit(size_t sz)
  {
    end_ += sz;
  }

  /// Guarantee persistence
  void sync()
  {
    msync(rgn_ptr, rgn_size, MS_ASYNC);
  }

  // return the size of all that has been committed
  size_t size() const noexcept
  {
    return end_;
  }

  // return the store capacity in bytes
  size_t capacity() const noexcept
  {
    return rgn_size;
  }

  void *mapped_addr() const noexcept { return rgn_ptr; }

private:

  FRIEND_TEST(NewSimpleStore, simple);

  friend std::ostream &operator << (std::ostream & o, const MapArena & s)
  {
    return o << "rgn_ptr          = " << s.rgn_ptr << std::endl
             << "end_             = " << s.end_ << std::endl
             << "rgn_size         = " << s.rgn_size << std::endl
             << "size             = " << s.size() << std::endl
             << "capacity         = " << s.capacity();
  }
};

}

#endif
