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

/** @file log.H
 *  @brief Thread-safe logging utilities.
 *
 *  This file provides Log_Manager, a thread-safe logging class that
 *  writes timestamped messages to a file with mutex protection.
 *
 *  ## Features
 *
 *  - Thread-safe logging with mutex protection
 *  - Automatic timestamps in human-readable format
 *  - Process and thread ID tracking
 *  - Debug mode with file/line information
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  Log_Manager log(stderr);
 *
 *  // Use macro for automatic file/line in debug mode
 *  WRITE_IN_LOG(log, "Processing item %d", item_id);
 *
 *  // Output: "Mon Jan 01 2024 12:00:00 PM (12345:6789): Processing item 42"
 *  ```
 *
 *  @see useMutex.H Mutex utilities
 *
 *  @ingroup Utilidades
 *  @author Leandro Rabindranath León
 */

# ifndef LOG_H
# define LOG_H

# include <sys/utsname.h>
# include <errno.h>
# include <stdio.h>
# include <stdarg.h>
# include <string.h>
# include <useMutex.H>

/**
 * @brief Thread-safe log manager with timestamps.
 *
 * Provides synchronized logging to a file descriptor with automatic
 * timestamp generation and optional debug information.
 *
 * ## Thread Safety
 *
 * All logging operations are protected by a mutex, making it safe
 * for concurrent use from multiple threads.
 *
 * ## Log Format
 *
 * - Release: `TIMESTAMP (thread_id:pid): MESSAGE`
 * - Debug: `TIMESTAMP (file:line:thread_id:pid): MESSAGE`
 *
 * @ingroup Utilidades
 */
class Log_Manager
{
  static const short int Max_Log_Line_Length  = 512;  ///< Max message length
  static const short int Max_Time_Line_Length = 61;   ///< Max timestamp length

  FILE * file_descriptor;  ///< Output file descriptor

  pthread_mutex_t mutex;   ///< Thread synchronization mutex

  char print_buffer[Max_Log_Line_Length];   ///< Message buffer
  char time_buffer[Max_Time_Line_Length];   ///< Timestamp buffer

  size_t num_lines;      ///< Lines written (not used)
  size_t max_num_lines;  ///< Max lines (not used)

  /**
   * @brief Get current time as formatted string.
   *
   * Formats time as: "Weekday Month Day Year HH:MM:SS AM/PM"
   */
  void get_time()
  {
    time_t unix_time;
    time(&unix_time);
      
    struct tm broken_time;
    localtime_r(&unix_time, &broken_time); 

    strftime(time_buffer, Max_Time_Line_Length, "%a %b %d %G %r", 
	     &broken_time);
  }

public:

  /**
   * @brief Construct log manager for a file.
   *
   * @param _file_descriptor Output file (e.g., stdout, stderr, or fopen result)
   */
  Log_Manager(FILE * _file_descriptor)
    : file_descriptor(_file_descriptor)
  {
    init_mutex(mutex);
    struct utsname site_info;
    uname(&site_info);
  }

  /// Destructor releases mutex
  ~Log_Manager()
  {
    destroy_mutex(mutex);
  }

  /**
   * @brief Write log line without newline (for multi-part messages).
   *
   * @param format Printf-style format string
   * @param ... Format arguments
   *
   * @note In DEBUG mode, also accepts file and line parameters
   */
  void init_line(
# ifdef DEBUG
		  char * file, 
		  int line, 
# endif
		  const char *format, ...)
  {
    va_list ap;

    va_start(ap, format);
    {
      CRITICAL_SECTION(mutex);

      get_time();

# ifdef DEBUG
      fprintf(file_descriptor, "%s (%s:%d:%ld:%d) : ", 
	      time_buffer, file, line, pthread_self(), getpid());
# else
      fprintf(file_descriptor, "%s (%ld:%d): ", 
	      time_buffer, pthread_self(), getpid()); 
# endif
      vfprintf(file_descriptor, format, ap);
    }
    va_end(ap);
  }

  /**
   * @brief Write complete log line with newline.
   *
   * @param format Printf-style format string
   * @param ... Format arguments
   *
   * @note In DEBUG mode, also accepts file and line parameters
   */
  void write_line(
# ifdef DEBUG
		  char * file, 
		  int line, 
# endif
		  const char *format, ...)
  {
    va_list ap;

    va_start(ap, format);
    {
      CRITICAL_SECTION(mutex);

      get_time();

# ifdef DEBUG
      fprintf(file_descriptor, "%s (%s:%d:%ld:%d) : ", 
	      time_buffer, file, line, pthread_self(), getpid());
# else
      fprintf(file_descriptor, "%s (%ld:%d): ", 
	      time_buffer, pthread_self(), getpid());
# endif
      vfprintf(file_descriptor, format, ap);
      fprintf(file_descriptor, "\n");
    }
    va_end(ap);
  }


};

/**
 * @brief Macro to write to log with automatic file/line in debug mode.
 *
 * @param log Log_Manager instance
 * @param format Printf-style format and arguments
 */
# ifdef DEBUG
# define WRITE_IN_LOG(log, format...) log.write_line(__FILE__, __LINE__, format)
# else
# define WRITE_IN_LOG(log, format...) log.write_line(format)
# endif

# endif // LOG_H
