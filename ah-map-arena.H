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

/** @file ah-map-arena.H
 *  @brief Memory-mapped file arena allocator.
 *
 *  Provides MapArena for allocating memory from memory-mapped files,
 *  enabling persistent allocations that survive process restarts.
 *  Uses mmap for efficient file-backed memory.
 *
 *  @ingroup Memory
 *  @author Leandro Rabindranath León
 */

#ifndef AH_MAP_ARENA_H
#define AH_MAP_ARENA_H

#include <unistd.h>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <filesystem>
#include <fcntl.h>
#include <sys/mman.h>
#include <string>

#include <ah-errors.H>

namespace fs = std::filesystem;

namespace Aleph
{

/** @brief Memory-mapped file arena allocator.
 *
 *  MapArena provides a persistent arena allocator backed by a memory-mapped
 *  file. It allows efficient allocation of memory chunks that are automatically
 *  persisted to disk, enabling crash recovery and data persistence across
 *  program restarts.
 *
 *  ## Features
 *
 *  - **Persistence**: Data is automatically persisted to the underlying file
 *  - **Crash recovery**: If the file exists on construction, previous state is recovered
 *  - **Dynamic growth**: Arena automatically grows via mremap when needed
 *  - **Zero-copy access**: Memory is directly mapped, no copying needed
 *  - **Shared memory**: Can be used for inter-process communication (MAP_SHARED)
 *
 *  ## Memory Model
 *
 *  The arena uses a simple bump allocator model:
 *  - `reserve(sz)` returns a pointer to sz bytes of memory
 *  - `commit(sz)` marks sz bytes as allocated, advancing the allocation pointer
 *  - Memory is never individually freed; the entire arena must be reset
 *
 *  ## Example Usage
 *
 *  @code
 *  // Create or recover an arena
 *  MapArena arena("/tmp/my_arena.dat");
 *
 *  // Allocate memory
 *  char* ptr = arena.reserve(100);
 *  if (ptr) {
 *    memcpy(ptr, "Hello, World!", 14);
 *    arena.commit(14);  // Only commit what was actually used
 *  }
 *
 *  // Ensure persistence
 *  arena.sync();
 *
 *  // Access allocated data
 *  for (auto it = arena.begin(); it != arena.end(); ++it)
 *    std::cout << *it;
 *  @endcode
 *
 *  ## Thread Safety
 *
 *  This class is NOT thread-safe. External synchronization is required
 *  for concurrent access from multiple threads.
 *
 *  ## Important Notes
 *
 *  - Pointers returned by `reserve()` may become invalid after subsequent
 *    `reserve()` calls that trigger a remap
 *  - The file format stores the allocation end offset at the beginning
 *  - Initial region size is 4KB, doubling as needed
 *
 *  @warning Calling `commit()` multiple times without intervening `reserve()`
 *           or committing more than was reserved leads to undefined behavior.
 *
 *  @see reserve() For allocating memory
 *  @see commit() For finalizing allocations
 *  @see sync() For ensuring persistence
 *
 *  @ingroup Memory
 */
class MapArena
{
public:

  /// @brief Initial region size in bytes (4 KB).
  static constexpr size_t initial_rgn_size = 4 * 1024;

private:

  char * rgn_ptr = nullptr;   ///< Pointer to the mapped memory region
  size_t end_ = 0;            ///< Allocation offset from rgn_ptr
  size_t rgn_size = initial_rgn_size;  ///< Current mapped region size
  int fd = -1;                ///< File descriptor for the backing file

  /** @brief Remap the memory region to accommodate more allocations.
   *
   *  Doubles the region size repeatedly until it can hold sz additional bytes.
   *
   *  @param[in] sz Required additional size in bytes.
   *  @return true if remap succeeded, false otherwise.
   */
  bool remap(size_t sz)
  {
    assert(sz > avail());

    // Determine new region size according to requested sz
    size_t new_size = 2 * rgn_size;
    while (new_size - end_ < sz)
      new_size *= 2;

    void * ptr = mremap(rgn_ptr, rgn_size, new_size, MREMAP_MAYMOVE);
    if (ptr == MAP_FAILED)
      return false;

    auto status = ftruncate(fd, static_cast<off_t>(new_size));
    ah_runtime_error_unless(status == 0)
        << "cannot truncate the file to " << new_size << " error = " << errno;

    rgn_ptr = static_cast<char*>(ptr);
    rgn_size = new_size;

    return true;
  }

public:

  /// @brief Iterator type for traversing allocated memory.
  using iterator = char *;

  /// @brief Const iterator type.
  using const_iterator = const char *;

  /// @brief Size type for memory sizes.
  using size_type = size_t;

  /** @brief Initialize the arena with a backing file.
   *
   *  Opens or creates the specified file and maps it to memory. If the file
   *  already exists, recovers the previous allocation state.
   *
   *  @param[in] file_path_name Path to the backing file.
   *  @param[in] addr Optional hint address for mmap (usually nullptr).
   *
   *  @throw std::runtime_error If file operations or mmap fails.
   */
  void init(const std::string & file_path_name, void * addr = nullptr)
  {
    end_ = 0;
    if (fs::exists(fs::path(file_path_name)))
      {
        fd = open(file_path_name.c_str(), O_RDWR, static_cast<mode_t>(0600));
        ah_runtime_error_unless(fd >= 0) << "cannot open " << file_path_name;

        // Read end_ value stored at the beginning of file
        auto status = ::read(fd, &end_, sizeof(end_));
        ah_runtime_error_unless(status != -1)
          << "Cannot read file length of " << file_path_name;

        // Determine file size
        auto file_size = lseek(fd, 0, SEEK_END);
        ah_runtime_error_unless(file_size != static_cast<off_t>(-1))
          << "Cannot determine file length of " << file_path_name;
        rgn_size = static_cast<size_t>(file_size);
      }
    else
      {
        fd = open(file_path_name.c_str(), O_RDWR | O_CREAT, static_cast<mode_t>(0600));
        ah_runtime_error_unless(fd >= 0) << "cannot open " << file_path_name;
        auto status = ftruncate(fd, static_cast<off_t>(rgn_size));
        ah_runtime_error_unless(status == 0)
          << "cannot truncate the file to " << rgn_size << " error = " << errno;
      }

    void *ptr = mmap(addr, rgn_size, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_POPULATE, fd, 0);
    ah_runtime_error_unless(ptr != MAP_FAILED)
        << "Cannot mmap. Error = " << errno;

    rgn_ptr = static_cast<char*>(ptr);
  }

  /** @brief Initialize and erase any existing data.
   *
   *  Removes the file if it exists, then initializes a fresh arena.
   *
   *  @param[in] file_path_name Path to the backing file.
   *
   *  @throw std::runtime_error If initialization fails.
   */
  void init_and_erase(const std::string & file_path_name)
  {
    fs::remove(file_path_name);
    init(file_path_name);
  }

  /** @brief Construct an arena with a backing file.
   *
   *  Creates or opens the specified file and maps it to memory. If the file
   *  already exists, it is assumed to be a recovery scenario and the stored
   *  state is recovered.
   *
   *  @param[in] file_path_name Path to the backing file.
   *
   *  @throw std::runtime_error If initialization fails.
   */
  explicit MapArena(const std::string & file_path_name)
  {
    init(file_path_name);
  }

  /** @brief Default constructor - creates an uninitialized arena.
   *
   *  The arena must be initialized with init() before use.
   */
  MapArena() noexcept = default;

  /// @brief Deleted copy constructor (arena cannot be copied).
  MapArena(const MapArena &) = delete;

  /// @brief Deleted copy assignment (arena cannot be copied).
  MapArena & operator=(const MapArena &) = delete;

  /** @brief Move constructor.
   *
   *  Transfers ownership of the mapped memory from another arena.
   *
   *  @param[in,out] other Arena to move from (left in uninitialized state).
   */
  MapArena(MapArena && other) noexcept
    : rgn_ptr(other.rgn_ptr), end_(other.end_),
      rgn_size(other.rgn_size), fd(other.fd)
  {
    other.rgn_ptr = nullptr;
    other.end_ = 0;
    other.rgn_size = initial_rgn_size;
    other.fd = -1;
  }

  /** @brief Move assignment operator.
   *
   *  @param[in,out] other Arena to move from.
   *  @return Reference to this arena.
   */
  MapArena & operator=(MapArena && other) noexcept
  {
    if (this != &other)
      {
        // Clean up current resources
        if (rgn_ptr)
          munmap(rgn_ptr, rgn_size);
        if (fd != -1)
          close(fd);

        // Transfer ownership
        rgn_ptr = other.rgn_ptr;
        end_ = other.end_;
        rgn_size = other.rgn_size;
        fd = other.fd;

        // Invalidate other
        other.rgn_ptr = nullptr;
        other.end_ = 0;
        other.rgn_size = initial_rgn_size;
        other.fd = -1;
      }
    return *this;
  }

  /** @brief Destructor - unmaps memory and closes file.
   */
  ~MapArena()
  {
    if (rgn_ptr)
      munmap(rgn_ptr, rgn_size);
    if (fd != -1)
      close(fd);
  }

  /** @brief Get iterator to the beginning of allocated memory.
   *
   *  @return Pointer to the first byte of the mapped region.
   */
  [[nodiscard]] iterator begin() noexcept { return rgn_ptr; }

  /** @brief Get const iterator to the beginning.
   *
   *  @return Const pointer to the first byte.
   */
  [[nodiscard]] const_iterator begin() const noexcept { return rgn_ptr; }

  /** @brief Get iterator past the last allocated byte.
   *
   *  @return Pointer past the last committed byte.
   */
  [[nodiscard]] iterator end() noexcept { return rgn_ptr + end_; }

  /** @brief Get const iterator past the last allocated byte.
   *
   *  @return Const pointer past the last committed byte.
   */
  [[nodiscard]] const_iterator end() const noexcept { return rgn_ptr + end_; }

  /** @brief Get the base address of the mapped region.
   *
   *  Equivalent to begin(). Returns the first address of reserved memory,
   *  or the first available address if nothing has been reserved.
   *
   *  @return Pointer to the base of the mapped region.
   */
  [[nodiscard]] char * base() const noexcept { return rgn_ptr; }

  /** @brief Get the available memory in the current mapping.
   *
   *  Returns how many bytes can be allocated without triggering a remap.
   *
   *  @return Available bytes in the current mapping.
   */
  [[nodiscard]] size_type avail() const noexcept
  {
    return rgn_size - end_;
  }

  /** @brief Reserve memory for allocation.
   *
   *  Returns a pointer to sz bytes of memory. The memory is not yet committed;
   *  you must call commit() to finalize the allocation.
   *
   *  @param[in] sz Number of bytes to reserve.
   *  @return Pointer to the reserved memory, or nullptr if allocation fails.
   *
   *  @warning This call may trigger mremap, which invalidates all previously
   *           returned pointers. Use with extreme caution!
   *
   *  @note The reserved memory is guaranteed to be readable/writable up to
   *        sz bytes from the returned pointer.
   */
  [[nodiscard]] char * reserve(size_type sz)
  {
    if (sz > avail() && !remap(sz))
      return nullptr;

    return rgn_ptr + end_;
  }

  /** @brief Commit a previous reservation.
   *
   *  Finalizes an allocation by advancing the end pointer. After commit(),
   *  the next reserve() will return addresses beyond the committed bytes.
   *
   *  @param[in] sz Number of bytes to commit (must be <= reserved amount).
   *
   *  @pre reserve() must have been called with size >= sz.
   *
   *  @warning Calling commit() without a matching reserve(), or with sz
   *           greater than what was reserved, leads to undefined behavior.
   */
  void commit(size_type sz) noexcept
  {
    end_ += sz;
  }

  /** @brief Ensure data is persisted to disk.
   *
   *  Calls msync() to flush changes to the backing file. Uses MS_ASYNC
   *  for better performance (non-blocking).
   */
  void sync() noexcept
  {
    msync(rgn_ptr, rgn_size, MS_ASYNC);
  }

  /** @brief Get the total committed (allocated) size.
   *
   *  @return Number of bytes that have been committed.
   */
  [[nodiscard]] size_type size() const noexcept
  {
    return end_;
  }

  /** @brief Get the current capacity (mapped region size).
   *
   *  @return Total size of the mapped region in bytes.
   */
  [[nodiscard]] size_type capacity() const noexcept
  {
    return rgn_size;
  }

  /** @brief Check if the arena is empty.
   *
   *  @return true if no memory has been committed, false otherwise.
   */
  [[nodiscard]] bool empty() const noexcept
  {
    return end_ == 0;
  }

  /** @brief Check if the arena has been initialized.
   *
   *  @return true if init() or constructor with filename was called.
   */
  [[nodiscard]] bool is_initialized() const noexcept
  {
    return rgn_ptr != nullptr;
  }

  /** @brief Get the mapped memory address.
   *
   *  @return Pointer to the mapped region, or nullptr if not initialized.
   */
  [[nodiscard]] void * mapped_addr() const noexcept
  {
    return rgn_ptr;
  }

  /** @brief Get the file descriptor.
   *
   *  @return File descriptor of the backing file, or -1 if not initialized.
   */
  [[nodiscard]] int file_descriptor() const noexcept
  {
    return fd;
  }

  /** @brief Output operator for debugging.
   *
   *  @param[in,out] o Output stream.
   *  @param[in] s Arena to output.
   *  @return Reference to the output stream.
   */
  friend std::ostream & operator<<(std::ostream & o, const MapArena & s)
  {
    return o << "MapArena:\n"
             << "  rgn_ptr   = " << static_cast<void*>(s.rgn_ptr) << '\n'
             << "  end_      = " << s.end_ << '\n'
             << "  rgn_size  = " << s.rgn_size << '\n'
             << "  size      = " << s.size() << '\n'
             << "  capacity  = " << s.capacity() << '\n'
             << "  avail     = " << s.avail();
  }
};

} // end namespace Aleph

#endif // AH_MAP_ARENA_H