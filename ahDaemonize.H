
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


/**
 * @file ahDaemonize.H
 * @brief Functionality for daemonizing a process
 * @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

/**
 * @defgroup Daemonization Daemonization Utilities
 * @brief Functions for creating Unix daemons
 * @ingroup Utilities
 *  @author Leandro Rabindranath León
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
