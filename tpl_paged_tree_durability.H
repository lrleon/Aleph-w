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

/** @file tpl_paged_tree_durability.H
 *  @brief Helpers for durable paged-tree files.
 *
 *  This header centralizes process locking, parent-directory syncing, and
 *  other low-level durability helpers shared by the paged persistent tree
 *  implementations.
 *
 *  @ingroup Trees
 */

# ifndef TPL_PAGED_TREE_DURABILITY_H
# define TPL_PAGED_TREE_DURABILITY_H

# include <algorithm>
# include <array>
# include <cerrno>
# include <cstdint>
# include <cstdio>
# include <cstdlib>
# include <filesystem>
# include <fstream>
# include <mutex>
# include <string>
# include <type_traits>
# include <vector>

# include <ah-errors.H>

# if defined(__unix__) || defined(__APPLE__)
#   include <fcntl.h>
#   include <signal.h>
#   include <sys/stat.h>
#   include <sys/types.h>
#   include <unistd.h>
# endif

namespace Aleph
{
  /** @brief Open mode for paged persistent tree files.
   *
   *  `Read_Write` opens the backing file for mutations and durable commits.
   *  `Read_Only` opens an existing clean file for read-only access under a
   *  shared advisory lock.
   */
  enum class Paged_File_Open_Mode : std::uint8_t
  {
    Read_Write,
    Read_Only
  };

  namespace detail
  {
    enum class Pid_File_Lock_Mode : std::uint8_t
    {
      Shared,
      Exclusive
    };

    inline void sync_parent_directory(const std::string & path,
                                      const char * kind)
    {
# if defined(__unix__) || defined(__APPLE__)
      namespace fs = std::filesystem;
      const fs::path parent = fs::path(path).has_parent_path()
                                ? fs::path(path).parent_path()
                                : fs::current_path();

      const int fd = ::open(parent.c_str(), O_RDONLY | O_DIRECTORY);
      ah_runtime_error_unless(fd >= 0)
        << kind << ": cannot open parent directory " << parent.string()
        << " for sync";

      const int status = ::fsync(fd);
      const int close_status = ::close(fd);
      ah_runtime_error_unless(status == 0)
        << kind << ": cannot fsync parent directory " << parent.string();
      ah_runtime_error_unless(close_status == 0)
        << kind << ": cannot close parent directory " << parent.string();
# else
      (void) path;
      (void) kind;
# endif
    }

    inline void sync_file_by_path(const std::string & path, const char * kind)
    {
# if defined(__unix__) || defined(__APPLE__)
      const int fd = ::open(path.c_str(), O_RDONLY);
      ah_runtime_error_unless(fd >= 0)
        << kind << ": cannot open " << path << " for fsync";

      const int status = ::fsync(fd);
      const int close_status = ::close(fd);
      ah_runtime_error_unless(status == 0)
        << kind << ": cannot fsync " << path;
      ah_runtime_error_unless(close_status == 0)
        << kind << ": cannot close " << path;
# else
      (void) path;
      (void) kind;
# endif
    }

    inline void sync_file_descriptor(const int fd,
                                     const std::string & path,
                                     const char * kind)
    {
# if defined(__unix__) || defined(__APPLE__)
      const int status = ::fsync(fd);
      ah_runtime_error_unless(status == 0)
        << kind << ": cannot fsync " << path;
# else
      (void) fd;
      (void) path;
      (void) kind;
# endif
    }

    inline void remove_file_if_exists(const std::string & path,
                                      const char * kind)
    {
      namespace fs = std::filesystem;
      std::error_code ec;
      const bool removed = fs::remove(path, ec);
      ah_runtime_error_unless(not ec)
        << kind << ": cannot remove " << path << " (" << ec.message() << ")";
      if (removed)
        sync_parent_directory(path, kind);
    }

    inline void rename_file_atomic(const std::string & src,
                                   const std::string & dst,
                                   const char * kind)
    {
      namespace fs = std::filesystem;
      std::error_code ec;
      fs::rename(src, dst, ec);
      ah_runtime_error_unless(not ec)
        << kind << ": cannot rename " << src << " to " << dst
        << " (" << ec.message() << ")";
      sync_parent_directory(dst, kind);
    }

    inline void copy_file_contents(const std::string & src,
                                   const std::string & dst,
                                   const char * kind)
    {
      std::ifstream in(src, std::ios::binary);
      ah_runtime_error_unless(in.good())
        << kind << ": cannot open " << src << " for reading";

      std::ofstream out(dst, std::ios::binary | std::ios::trunc);
      ah_runtime_error_unless(out.good())
        << kind << ": cannot open " << dst << " for writing";

      std::array<char, 8192> buffer = {};
      while (in.good())
        {
          in.read(buffer.data(), buffer.size());
          const auto count = in.gcount();
          if (count > 0)
            out.write(buffer.data(), count);
        }

      ah_runtime_error_unless(in.eof() and not in.bad())
        << kind << ": cannot read " << src;

      out.flush();
      ah_runtime_error_unless(out.good())
        << kind << ": cannot flush " << dst;

      out.close();
      ah_runtime_error_unless(out.good())
        << kind << ": cannot close " << dst;

      sync_file_by_path(dst, kind);
      sync_parent_directory(dst, kind);
    }

    [[nodiscard]] inline std::uint32_t crc32_begin() noexcept
    {
      return 0xFFFFFFFFu;
    }

    [[nodiscard]] inline std::uint32_t crc32_add_bytes(std::uint32_t crc,
                                                       const void * data,
                                                       const size_t size) noexcept
    {
      const auto * ptr = static_cast<const unsigned char *>(data);
      for (size_t i = 0; i < size; ++i)
        {
          crc ^= ptr[i];
          for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xEDB88320u & static_cast<std::uint32_t>(-static_cast<int>(crc & 1u)));
        }
      return crc;
    }

    template <typename T>
    [[nodiscard]] inline std::uint32_t crc32_add(std::uint32_t crc,
                                                 const T & value) noexcept
    {
      static_assert(std::is_trivially_copyable_v<T>,
                    "crc32_add requires a trivially copyable value");
      return crc32_add_bytes(crc, &value, sizeof(T));
    }

    [[nodiscard]] inline std::uint32_t crc32_finish(const std::uint32_t crc) noexcept
    {
      return ~crc;
    }

    [[nodiscard]] inline long long current_process_id() noexcept
    {
# if defined(__unix__) || defined(__APPLE__)
      return static_cast<long long>(::getpid());
# else
      return 0;
# endif
    }

    [[nodiscard]] inline bool process_is_alive(const long long pid) noexcept
    {
# if defined(__unix__) || defined(__APPLE__)
      if (pid <= 0)
        return false;

      if (::kill(static_cast<pid_t>(pid), 0) == 0)
        return true;

      return errno == EPERM;
# else
      (void) pid;
      return false;
# endif
    }

    class Pid_File_Lock
    {
      struct Local_Lock_State
      {
        std::string path;
        size_t shared_count = 0;
        bool exclusive = false;
      };

      std::string path_;
      bool acquired_ = false;
      Pid_File_Lock_Mode mode_ = Pid_File_Lock_Mode::Exclusive;
# if defined(__unix__) || defined(__APPLE__)
      int fd_ = -1;
# endif

      [[nodiscard]] static std::mutex & registry_mutex()
      {
        static std::mutex mutex;
        return mutex;
      }

      [[nodiscard]] static std::vector<Local_Lock_State> & registry()
      {
        static std::vector<Local_Lock_State> states;
        return states;
      }

      static void register_path(const std::string & path,
                                const Pid_File_Lock_Mode mode,
                                const char * kind)
      {
        std::lock_guard<std::mutex> guard(registry_mutex());
        auto & states = registry();
        const auto it = std::find_if(states.begin(), states.end(),
                                     [&path] (const Local_Lock_State & state)
                                     {
                                       return state.path == path;
                                     });

        if (it == states.end())
          {
            Local_Lock_State state;
            state.path = path;
            state.shared_count = mode == Pid_File_Lock_Mode::Shared ? 1u : 0u;
            state.exclusive = mode == Pid_File_Lock_Mode::Exclusive;
            states.push_back(std::move(state));
            return;
          }

        if (mode == Pid_File_Lock_Mode::Shared)
          {
            ah_runtime_error_unless(not it->exclusive)
              << kind << ": this process already holds an exclusive lock "
              << path;
            ++it->shared_count;
            return;
          }

        ah_runtime_error()
          << kind << ": this process already holds the lock " << path;
      }

      static void unregister_path(const std::string & path,
                                  const Pid_File_Lock_Mode mode) noexcept
      {
        std::lock_guard<std::mutex> guard(registry_mutex());
        auto & states = registry();
        const auto it = std::find_if(states.begin(), states.end(),
                                     [&path] (const Local_Lock_State & state)
                                     {
                                       return state.path == path;
                                     });
        if (it == states.end())
          return;

        if (mode == Pid_File_Lock_Mode::Shared)
          {
            if (it->shared_count > 0)
              --it->shared_count;
          }
        else
          it->exclusive = false;

        if (it->shared_count == 0 and not it->exclusive)
          states.erase(it);
      }

      [[nodiscard]] static bool stale_lock(const std::string & path)
      {
        std::ifstream in(path);
        if (not in.good())
          return true;

        long long pid = 0;
        in >> pid;
        if (not in.good())
          return true;

        return not process_is_alive(pid);
      }

# if defined(__unix__) || defined(__APPLE__)
      static void acquire_fd_lock(const int fd,
                                  const Pid_File_Lock_Mode mode,
                                  const std::string & path,
                                  const char * kind)
      {
        struct flock lock = {};
        lock.l_type = mode == Pid_File_Lock_Mode::Shared ? F_RDLCK : F_WRLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = 0;
        lock.l_len = 0;

        if (::fcntl(fd, F_SETLK, &lock) == 0)
          return;

        ah_runtime_error_unless(errno == EACCES or errno == EAGAIN)
          << kind << ": cannot acquire advisory lock " << path;
        ah_runtime_error()
          << kind << ": another process already holds the lock " << path;
      }

      static void write_pid_marker(const int fd,
                                   const std::string & path,
                                   const bool created,
                                   const char * kind)
      {
        const std::string text = std::to_string(current_process_id()) + "\n";
        ah_runtime_error_unless(::ftruncate(fd, 0) == 0)
          << kind << ": cannot truncate pid lock " << path;
        ah_runtime_error_unless(::lseek(fd, 0, SEEK_SET) >= 0)
          << kind << ": cannot seek pid lock " << path;

        const ssize_t written = ::write(fd, text.data(), text.size());
        ah_runtime_error_unless(written == static_cast<ssize_t>(text.size()))
          << kind << ": cannot write pid lock " << path;

        sync_file_descriptor(fd, path, kind);
        if (created)
          sync_parent_directory(path, kind);
      }
# endif

    public:
      Pid_File_Lock() = default;

      Pid_File_Lock(const Pid_File_Lock &) = delete;
      Pid_File_Lock & operator = (const Pid_File_Lock &) = delete;
      Pid_File_Lock(Pid_File_Lock &&) = delete;
      Pid_File_Lock & operator = (Pid_File_Lock &&) = delete;

      ~Pid_File_Lock() noexcept
      {
        release();
      }

      void acquire(const std::string & path,
                   const char * kind,
                   const Pid_File_Lock_Mode mode = Pid_File_Lock_Mode::Exclusive)
      {
        release();
        path_ = path;
        mode_ = mode;
        register_path(path_, mode_, kind);

        namespace fs = std::filesystem;
        if (fs::path(path_).has_parent_path())
          {
            std::error_code ec;
            (void) fs::create_directories(fs::path(path_).parent_path(), ec);
          }

# if defined(__unix__) || defined(__APPLE__)
        std::error_code exists_ec;
        const bool existed = fs::exists(path_, exists_ec);
        ah_runtime_error_unless(not exists_ec)
          << kind << ": cannot query pid lock " << path_ << " ("
          << exists_ec.message() << ")";

        fd_ = ::open(path_.c_str(), O_RDWR | O_CREAT, 0644);
        if (fd_ < 0)
          {
            const std::string lock_path = path_;
            unregister_path(path_, mode_);
            path_.clear();
            ah_runtime_error()
              << kind << ": cannot open pid lock " << lock_path;
          }

        try
          {
            acquire_fd_lock(fd_, mode_, path_, kind);
            write_pid_marker(fd_, path_, not existed, kind);
            acquired_ = true;
            return;
          }
        catch (...)
          {
            const int close_status = ::close(fd_);
            (void) close_status;
            fd_ = -1;
            unregister_path(path_, mode_);
            path_.clear();
            throw;
          }
# else
        while (true)
          {
            std::ofstream out(path_, std::ios::out | std::ios::trunc);
            if (out.good())
              {
                out << current_process_id() << "\n";
                out.close();
                acquired_ = true;
                return;
              }

            ah_runtime_error_unless(stale_lock(path_))
              << kind << ": another process already holds the lock " << path_;

            remove_file_if_exists(path_, kind);
          }
# endif
      }

      void release() noexcept
      {
        if (not acquired_ and path_.empty())
          return;

# if defined(__unix__) || defined(__APPLE__)
        if (fd_ >= 0)
          {
            const int close_status = ::close(fd_);
            (void) close_status;
            fd_ = -1;
          }
# else
        std::error_code ec;
        std::filesystem::remove(path_, ec);
        if (not ec)
          {
            try
              {
                sync_parent_directory(path_, "Pid_File_Lock");
              }
            catch (...)
              {
              }
          }
# endif
        unregister_path(path_, mode_);
        acquired_ = false;
        mode_ = Pid_File_Lock_Mode::Exclusive;
        path_.clear();
      }
    };
  } // namespace detail
} // namespace Aleph

# endif // TPL_PAGED_TREE_DURABILITY_H
