
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
