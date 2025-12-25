
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

/**
 * @file ahDaemonize.H
 * @brief Functionality for daemonizing a process
 * @ingroup Utils
 */

/**
 * @defgroup Daemonization Daemonization Utilities
 * @brief Functions for creating Unix daemons
 * @ingroup Utils
 */

#ifndef AHDAEMONIZE_H
#define AHDAEMONIZE_H

#include <syslog.h>
#include <stdexcept>

/**
 * @brief Converts the calling process into a Unix daemon
 * @param program_name Name of the program (for syslog identification)
 * @param facility The syslog facility to use (default: LOG_DAEMON)
 * @throws std::runtime_error if daemonization fails
 * @throws std::invalid_argument if program_name is null or empty
 * 
 * @note Based on daemon_init from "The Unix Network Programming" by Richard Stevens
 * @note This function will:
 *   - Fork and terminate the parent process
 *   - Create a new session
 *   - Change the working directory to root
 *   - Clear the file mode creation mask
 *   - Close all open file descriptors
 *   - Redirect stdin, stdout, and stderr to /dev/null
 *   - Initialize syslog logging
 * 
 * @warning This function should be called early in program execution
 * @warning After calling this function, the process will be running in the background
 * 
 * @code
 * #include <ahDaemonize.H>
 * 
 * int main(int argc, char *argv[]) {
 *     try {
 *         daemonize("my_daemon", LOG_DAEMON);
 *         // Daemon code here
 *     } catch (const std::exception& e) {
 *         syslog(LOG_ERR, "Failed to daemonize: %s", e.what());
 *         return 1;
 *     }
 *     return 0;
 * }
 * @endcode
 */
void daemonize(const char *program_name, int facility = LOG_DAEMON);

#endif // AHDAEMONIZE_H
