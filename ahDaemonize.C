
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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
 * @file ahDaemonize.C
 * @brief Implementation of daemonization functionality
 * @ingroup Daemonization
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>     // for strerror
#include <ah-signal.H>  // for Signal
#include <ah-errors.H>  // for error handling macros
#include <ahDaemonize.H>

namespace {

constexpr int MAXIMUM_FILE_DESC = 256;

/**
 * @brief Redirects standard file descriptors to /dev/null
 * @throws std::runtime_error if redirection fails
 */
void redirect_standard_fds()
{
    const int null_fd = open("/dev/null", O_RDWR);
    ah_runtime_error_if(null_fd == -1) << "Cannot open /dev/null";

    // Redirect standard file descriptors to /dev/null
    ah_runtime_error_if(dup2(null_fd, STDIN_FILENO) != STDIN_FILENO)
        << "Cannot redirect stdin";

    ah_runtime_error_if(dup2(null_fd, STDOUT_FILENO) != STDOUT_FILENO)
        << "Cannot redirect stdout";

    ah_runtime_error_if(dup2(null_fd, STDERR_FILENO) != STDERR_FILENO)
        << "Cannot redirect stderr";

    // Close the original file descriptor if it's not a standard one
    if (null_fd > STDERR_FILENO)
        close(null_fd);
}

} // anonymous namespace

void daemonize(const char *program_name, int facility)
{
    // Input validation
    ah_invalid_argument_if(program_name == nullptr or *program_name == '\0')
        << "Program name cannot be null or empty";

    // First fork to detach from parent
    pid_t pid = fork();
    ah_runtime_error_if(pid < 0) << "First fork failed";
    
    if (pid > 0)
        _exit(0); // Parent exits

    // Create new session and become session leader
    ah_runtime_error_if(setsid() < 0) << "setsid failed";

    // Handle SIGHUP to prevent termination when the terminal is closed
    Signal signalHandler(SIGHUP, SIG_IGN);

    // Second fork to ensure we're not a session leader
    pid = fork();
    ah_runtime_error_if(pid < 0) << "Second fork failed";
    
    if (pid > 0)
        _exit(0); // First child exits

    // Change working directory to root to avoid keeping directories in use
    ah_runtime_error_if(chdir("/") < 0) << "Cannot change directory to root";

    // Clear file mode creation mask to allow creation of files with any permissions
    umask(0);

    // Close all open file descriptors
    for (int i = 0; i < MAXIMUM_FILE_DESC; i++)
        close(i);

    // Redirect standard file descriptors to /dev/null
    redirect_standard_fds();

    // Initialize syslog for logging
    openlog(program_name, LOG_PID | LOG_NDELAY, facility);

    // Set daemonized flag (if Aleph::daemonized is defined)
    #ifdef ALEPH_DAEMONIZED_FLAG
    Aleph::daemonized = true;
    #endif
}
