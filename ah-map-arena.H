
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

# ifndef MM_STRING_H
# define MM_STRING_H

# include <unistd.h>
# include <cstdlib>
# include <cassert>
# include <experimental/filesystem>
# include <fcntl.h>
# include <ah-errors.H>
# include <string>
# include <sys/mman.h>

# include <gtest/gtest.h>

namespace fs = std::experimental::filesystem;

namespace Aleph
{

class MapArena
{
public:

  static constexpr size_t initial_rgn_size = 4 * 1024; // 4 Kb

private:

  char * rgn_ptr = nullptr;
  size_t end_ = 0; // store offset respect to rgn_ptr
  size_t rgn_size = initial_rgn_size;
  int fd = -1;

  // remap enough memory for allocating a chunk of sz size
  bool remap(size_t sz)
  {
    assert(sz > avail());

    // determine new region size according to requested sz
    size_t new_size = 2 * rgn_size;
    while (new_size - end_ < sz)
      new_size *= 2;

    void * ptr = mremap(rgn_ptr, rgn_size, new_size, MREMAP_MAYMOVE);
    if (ptr == (void *) -1)
      return false;

    auto status = ftruncate(fd, new_size);
    ah_runtime_error_unless(status == 0)
        << "cannot truncate the file to " << new_size << " error = " << errno;

    rgn_ptr = (char *) ptr;
    rgn_size = new_size;

    return true;
  }

public:

  /// Initialize `this` which should have been instantiated with empty
  /// constructor
  void init(const std::string & file_path_name)
  {
    end_ = 0;
    if (fs::exists(fs::path(file_path_name)))
      {
        fd = open(file_path_name.c_str(), O_RDWR, (mode_t) 0600);
        ah_runtime_error_unless(fd >= 0) << "cannot open " << file_path_name;

        // read end_ value stored in the begin of file
        auto status = read(fd, &end_, sizeof(end_));
        ah_runtime_error_unless(status != -1)
          << "Cannot read file length of" << file_path_name;

        // Determine file size (maybe it is preferable to compute by
        // page size rounding according to the value of end_ instead
        // of this lseek, or worse, a system call)
        auto file_size = lseek(fd, 0, SEEK_END);
        ah_runtime_error_unless(file_size != (off_t)-1)
          << "Cannot determine file length of" << file_path_name;
        rgn_size = file_size;
      }
    else
      {
        fd = open(file_path_name.c_str(), O_RDWR | O_CREAT, (mode_t)0600);
        ah_runtime_error_unless(fd >= 0) << "cannot open " << file_path_name;
        auto status = ftruncate(fd, rgn_size);
        ah_runtime_error_unless(status == 0)
          << "cannot truncate the file to " << rgn_size << " error = " << errno;
      }

    void *ptr = mmap(NULL, rgn_size, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_POPULATE, fd, 0);
    ah_runtime_error_unless(ptr != (void *)-1)
        << "Cannot mmap. Error = " << errno;

    rgn_ptr = (char *) ptr;
  }

  void init_and_erase(const std::string & file_path_name)
  {
    fs::remove(file_path_name);
    init(file_path_name);
  }

  /** Create an arena store based on mmap memory with `file_name`.

      If the file already exist, then it is assumed that is a recovery
      process and the stored state is recovered. After recovery, the
      store can be used in a normal way.

      It could throw exception if it is not possible initialize
      (probably because mmap fails).
  */
  MapArena(const std::string & file_path_name)
  {
    init(file_path_name);
  }

  /// This constructor is intended to used when for any reason the
  /// file name is not known when the object needs to be declared
  MapArena() {}

  using iterator = char *;
  iterator begin() const noexcept { return rgn_ptr; }
  iterator end() const noexcept { return &rgn_ptr[end_]; }

  /// Return the base address of first reserved chunk. If there is
  /// nothing reserved then return the first available address
  char * base() const noexcept { return begin(); }

  /// Return the available memory in the already mapped memory
  /// region. That is how many bytes are available without need of to
  /// perform remap
  size_t avail() const noexcept
  {
    return rgn_size - end_;
  }

  ~MapArena()
  {
    if (rgn_ptr)
      munmap(rgn_ptr, end_);
    if (fd != -1)
      close(fd);
  }

  /** Reserve sz bytes for storing a msg. If successful, the sz bytes
      of memory access in write/read mode are guaranteed. Return
      `nullptr` if fails.

      This call could generate a mremap system call and the increase
      in file size.

      This call guarantees writing in the memory region between the
      returned value and sz bytes forward.

      Since this call could cause a remap, previous addresses returned
      by `reserve()` could became completely invalid. Extreme prudence!
  */
  char * reserve(size_t sz)
  {
    if (sz > avail() and not remap(sz))
      return nullptr;

    return &rgn_ptr[end_];
  }

  /* Notify to the storage handler that sz bytes of last call to
     reserve() operation must be allocated so the next reserve()'s
     operations will return larger addresses.

     Condition: sz <= sz' (sz less than the size value used in the
     last reserve(sz').

     Things could be very bad if you call `commit()` several times
     without previous `reserve()` or if `sz` is greater than what was
     used in `reserve()`.
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
