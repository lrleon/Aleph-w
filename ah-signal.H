
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
 * @file ah-signal.H
 * @brief POSIX signal handling utilities with RAII semantics.
 *
 * This header provides C++ wrappers for POSIX signal handling:
 *
 * - **Signal**: RAII wrapper for signal handler registration. Automatically
 *   restores the previous handler when the object is destroyed.
 *
 * - **SignalSet**: C++ wrapper for `sigset_t` with a fluent interface.
 *
 * - **SignalBlocker**: RAII utility to temporarily block signals.
 *
 * ## Thread Safety
 *
 * Signal handlers are process-wide (not per-thread). While the classes in this
 * header are safe to use, be aware that:
 * - Signal handlers affect the entire process
 * - Signal masks can be set per-thread using `pthread_sigmask`
 * - `SignalBlocker` uses `sigprocmask` (process-wide) by default
 *
 * ## Example Usage
 *
 * ```cpp
 * #include <ah-signal.H>
 *
 * volatile sig_atomic_t got_signal = 0;
 *
 * void handler(int signo) {
 *     got_signal = signo;
 * }
 *
 * int main() {
 *     // Install handler - previous handler is saved
 *     Signal sig(SIGINT, handler);
 *
 *     // ... do work ...
 *
 *     // When 'sig' goes out of scope, previous handler is restored
 * }
 * ```
 *
 * @ingroup Utilities
 * @author Leandro Rabindranath León
 */

# ifndef AH_SIGNAL_H
# define AH_SIGNAL_H

# include <csignal>
# include <cstring>
# include <iostream>
# include <stdexcept>
# include <string>
# include <initializer_list>
# include <aleph.H>

/**
 * @brief Exception thrown when a signal operation fails.
 *
 * Contains the signal number and errno that caused the failure.
 */
class SignalError : public std::runtime_error
{
  int signal_num_;
  int errno_code_;

public:
  /**
   * @brief Constructs a SignalError.
   * @param msg Error message describing the failure.
   * @param signo The signal number involved (-1 if not applicable).
   * @param err The errno value at the time of failure.
   */
  SignalError(const std::string & msg, int signo = -1, int err = 0)
    : std::runtime_error(msg), signal_num_(signo), errno_code_(err) {}

  /// @brief Returns the signal number involved in the error.
  [[nodiscard]] int signal_number() const noexcept { return signal_num_; }

  /// @brief Returns the errno value at the time of failure.
  [[nodiscard]] int error_code() const noexcept { return errno_code_; }
};

/**
 * @brief C++ wrapper for sigset_t with a fluent interface.
 *
 * Provides a type-safe way to manipulate signal sets.
 *
 * ## Example
 *
 * ```cpp
 * SignalSet sigs;
 * sigs.add(SIGINT).add(SIGTERM).add(SIGHUP);
 *
 * // Or using initializer list:
 * SignalSet sigs2({SIGINT, SIGTERM, SIGHUP});
 * ```
 */
class SignalSet
{
  sigset_t set_;

public:
  /// @brief Creates an empty signal set.
  SignalSet() noexcept { sigemptyset(&set_); }

  /// @brief Creates a signal set from an initializer list.
  SignalSet(std::initializer_list<int> signals) noexcept
  {
    sigemptyset(&set_);
    for (int sig : signals)
      sigaddset(&set_, sig);
  }

  /// @brief Creates a full signal set (all signals).
  [[nodiscard]] static SignalSet full() noexcept
  {
    SignalSet s;
    sigfillset(&s.set_);
    return s;
  }

  /// @brief Creates an empty signal set.
  [[nodiscard]] static SignalSet empty() noexcept { return SignalSet(); }

  /// @brief Adds a signal to the set.
  /// @return Reference to this for chaining.
  SignalSet & add(int signo) noexcept
  {
    sigaddset(&set_, signo);
    return *this;
  }

  /// @brief Removes a signal from the set.
  /// @return Reference to this for chaining.
  SignalSet & remove(int signo) noexcept
  {
    sigdelset(&set_, signo);
    return *this;
  }

  /// @brief Clears the set (removes all signals).
  /// @return Reference to this for chaining.
  SignalSet & clear() noexcept
  {
    sigemptyset(&set_);
    return *this;
  }

  /// @brief Fills the set with all signals.
  /// @return Reference to this for chaining.
  SignalSet & fill() noexcept
  {
    sigfillset(&set_);
    return *this;
  }

  /// @brief Checks if a signal is in the set.
  [[nodiscard]] bool contains(int signo) const noexcept
  {
    return sigismember(&set_, signo) == 1;
  }

  /// @brief Returns a pointer to the underlying sigset_t.
  [[nodiscard]] sigset_t * get() noexcept { return &set_; }

  /// @brief Returns a const pointer to the underlying sigset_t.
  [[nodiscard]] const sigset_t * get() const noexcept { return &set_; }

  /// @brief Implicit conversion to sigset_t*.
  operator sigset_t *() noexcept { return &set_; }

  /// @brief Implicit conversion to const sigset_t*.
  operator const sigset_t *() const noexcept { return &set_; }
};

/**
 * @brief RAII wrapper for temporarily blocking signals.
 *
 * Blocks the specified signals on construction and restores the previous
 * signal mask on destruction.
 *
 * ## Example
 *
 * ```cpp
 * {
 *     // Block SIGINT and SIGTERM in this scope
 *     SignalBlocker blocker({SIGINT, SIGTERM});
 *
 *     // Critical section - these signals won't interrupt
 *     do_critical_work();
 *
 * } // Previous mask is restored here
 * ```
 *
 * @note Uses `sigprocmask()` which affects the calling thread in
 *       single-threaded programs or the process signal mask.
 */
class SignalBlocker
{
  sigset_t old_mask_;
  bool active_ = true;

public:
  /**
   * @brief Blocks the signals in the given set.
   * @param signals The set of signals to block.
   * @throws SignalError if sigprocmask fails.
   */
  explicit SignalBlocker(const SignalSet & signals)
  {
    if (sigprocmask(SIG_BLOCK, signals.get(), &old_mask_) < 0)
      throw SignalError("sigprocmask(SIG_BLOCK) failed", -1, errno);
  }

  /**
   * @brief Blocks the signals specified in the initializer list.
   * @param signals List of signal numbers to block.
   * @throws SignalError if sigprocmask fails.
   */
  SignalBlocker(std::initializer_list<int> signals)
    : SignalBlocker(SignalSet(signals)) {}

  /**
   * @brief Blocks a single signal.
   * @param signo The signal number to block.
   * @throws SignalError if sigprocmask fails.
   */
  explicit SignalBlocker(int signo)
    : SignalBlocker(SignalSet({signo})) {}

  /// @brief Restores the previous signal mask.
  ~SignalBlocker()
  {
    if (active_)
      sigprocmask(SIG_SETMASK, &old_mask_, nullptr);
  }

  /// @brief Releases ownership - the mask won't be restored on destruction.
  void release() noexcept { active_ = false; }

  /// @brief Returns the previous signal mask that will be restored.
  [[nodiscard]] const sigset_t & previous_mask() const noexcept
  {
    return old_mask_;
  }

  // Non-copyable
  SignalBlocker(const SignalBlocker &) = delete;
  SignalBlocker & operator=(const SignalBlocker &) = delete;

  // Movable
  SignalBlocker(SignalBlocker && other) noexcept
    : old_mask_(other.old_mask_), active_(other.active_)
  {
    other.active_ = false;
  }

  SignalBlocker & operator=(SignalBlocker && other) noexcept
  {
    if (this != &other)
      {
        if (active_)
          sigprocmask(SIG_SETMASK, &old_mask_, nullptr);
        old_mask_ = other.old_mask_;
        active_ = other.active_;
        other.active_ = false;
      }
    return *this;
  }
};

/**
 * @brief RAII wrapper for POSIX signal handler registration.
 *
 * Installs a signal handler on construction and automatically restores the
 * previous handler when the object is destroyed. This ensures proper cleanup
 * even when exceptions are thrown.
 *
 * ## Features
 *
 * - **RAII semantics**: Previous handler is automatically restored on destruction.
 * - **SA_RESTART support**: System calls can be automatically restarted after
 *   signal delivery (except for SIGALRM).
 * - **Move semantics**: Signal objects can be moved (but not copied).
 * - **Release mechanism**: Option to not restore the handler on destruction.
 *
 * ## Special Handling for SIGALRM
 *
 * For `SIGALRM`, the `SA_INTERRUPT` flag is used (if available) instead of
 * `SA_RESTART` to ensure that blocking calls (like `sleep()`, `read()`) are
 * interrupted by the alarm.
 *
 * ## Example
 *
 * ```cpp
 * void alarm_handler(int) {
 *     // Handle alarm
 * }
 *
 * void example() {
 *     Signal alarm_sig(SIGALRM, alarm_handler);
 *     alarm(5);  // Set alarm for 5 seconds
 *
 *     // ... do work ...
 *
 *     // When alarm_sig goes out of scope, previous SIGALRM handler is restored
 * }
 * ```
 *
 * ## Error Handling
 *
 * By default, the constructor calls `abort()` if `sigaction()` fails (for
 * backward compatibility). Use the throwing constructor overload or
 * `Signal::try_create()` for exception-based error handling.
 *
 * @see SignalBlocker for temporarily blocking signals
 * @see SignalSet for managing sets of signals
 */
class Signal
{
public:
  /// @brief Type alias for signal handler function.
  using Handler = void (*)(int);

private:
  struct sigaction old_action_;
  int signal_number_;
  bool restart_calls_;
  bool active_ = true;

  // Internal constructor that doesn't abort on error
  struct NoAbortTag {};

  Signal(NoAbortTag, int signo, Handler func, bool restart_calls, int & error_out)
    : signal_number_(signo), restart_calls_(restart_calls)
  {
    error_out = 0;
    struct sigaction act;

    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (signo == SIGALRM)
      {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#endif
      }
    else if (restart_calls_)
      {
#ifdef SA_RESTART
        act.sa_flags |= SA_RESTART;
#endif
      }

    if (sigaction(signo, &act, &old_action_) < 0)
      {
        error_out = errno;
        active_ = false;
      }
  }

public:
  /**
   * @brief Installs a signal handler.
   *
   * This constructor maintains backward compatibility by calling `abort()`
   * if `sigaction()` fails. For exception-based error handling, use
   * `Signal::try_create()` or the throwing overload.
   *
   * @param signo The signal number to handle (e.g., SIGINT, SIGTERM).
   * @param func The handler function to install. Can be a function pointer,
   *             SIG_IGN (to ignore), or SIG_DFL (for default action).
   * @param restart_calls If true (default), interrupted system calls are
   *                      automatically restarted (SA_RESTART flag).
   *                      Ignored for SIGALRM.
   *
   * @warning Calls `abort()` if sigaction fails. Use `try_create()` for
   *          exception-based error handling.
   */
  Signal(int signo, Handler func, bool restart_calls = true)
    : signal_number_(signo), restart_calls_(restart_calls)
  {
    struct sigaction act;

    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (signo == SIGALRM)
      {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#endif
      }
    else if (restart_calls_)
      {
#ifdef SA_RESTART
        act.sa_flags |= SA_RESTART;
#endif
      }

    if (sigaction(signo, &act, &old_action_) < 0)
      {
        std::cerr << "Cannot modify the action associated with the signal "
                  << signo << std::endl;
        abort();
      }
  }


  /**
   * @brief Creates a Signal object or throws on failure.
   *
   * Unlike the regular constructor, this factory method throws a SignalError
   * exception instead of calling abort() on failure.
   *
   * @param signo The signal number to handle.
   * @param func The handler function to install.
   * @param restart_calls Whether to restart interrupted system calls.
   * @return A Signal object managing the handler.
   * @throws SignalError if sigaction fails.
   *
   * ## Example
   *
   * ```cpp
   * try {
   *     auto sig = Signal::create(SIGINT, my_handler);
   *     // ... use sig ...
   * } catch (const SignalError & e) {
   *     std::cerr << "Failed to install handler: " << e.what() << std::endl;
   * }
   * ```
   */
  [[nodiscard]] static Signal create(int signo, Handler func,
                                     bool restart_calls = true)
  {
    int error = 0;
    Signal sig(NoAbortTag{}, signo, func, restart_calls, error);
    if (error != 0)
      throw SignalError("Cannot modify signal action for signal " +
                        std::to_string(signo) + ": " + std::strerror(error),
                        signo, error);
    return sig;
  }

  /**
   * @brief Attempts to create a Signal object.
   *
   * Non-throwing version that returns success status through an output parameter.
   *
   * @param signo The signal number to handle.
   * @param func The handler function to install.
   * @param restart_calls Whether to restart interrupted system calls.
   * @param[out] error_out Set to errno value on failure, 0 on success.
   * @return A Signal object (check error_out before using).
   *
   * ## Example
   *
   * ```cpp
   * int err;
   * Signal sig = Signal::try_create(SIGINT, my_handler, true, err);
   * if (err != 0) {
   *     // Handle error
   * }
   * ```
   */
  [[nodiscard]] static Signal try_create(int signo, Handler func,
                                         bool restart_calls, int & error_out)
  {
    return Signal(NoAbortTag{}, signo, func, restart_calls, error_out);
  }

  /**
   * @brief Restores the previous signal handler.
   *
   * Unless `release()` was called, the destructor restores the signal handler
   * that was in effect before this Signal object was constructed.
   *
   * @warning If restoration fails, the destructor prints an error message and
   *          calls `abort()`. This maintains backward compatibility but may
   *          not be ideal in all contexts. Consider calling `release()` if
   *          you don't need automatic restoration.
   */
  ~Signal()
  {
    if (active_)
      {
        if (sigaction(signal_number_, &old_action_, nullptr) < 0)
          {
            std::cerr << "Cannot restore signal handler for signal "
                      << signal_number_ << std::endl;
            abort();
          }
      }
  }

  /**
   * @brief Releases ownership of the signal handler.
   *
   * After calling this method, the destructor will not restore the previous
   * handler. This is useful when you want to permanently change a signal
   * handler.
   *
   * ## Example
   *
   * ```cpp
   * {
   *     Signal sig(SIGTERM, my_permanent_handler);
   *     sig.release();  // Don't restore on destruction
   * }
   * // my_permanent_handler is still installed
   * ```
   */
  void release() noexcept { active_ = false; }

  /// @brief Returns the signal number being handled.
  [[nodiscard]] int signal_number() const noexcept { return signal_number_; }

  /// @brief Returns whether SA_RESTART is enabled for this handler.
  [[nodiscard]] bool restarts_calls() const noexcept { return restart_calls_; }

  /// @brief Returns whether this object will restore the handler on destruction.
  [[nodiscard]] bool is_active() const noexcept { return active_; }

  /// @brief Returns the previous handler that will be restored.
  [[nodiscard]] Handler previous_handler() const noexcept
  {
    return old_action_.sa_handler;
  }

  /// @brief Returns the previous sigaction structure.
  [[nodiscard]] const struct sigaction & previous_action() const noexcept
  {
    return old_action_;
  }

  // Non-copyable (would cause double restoration)
  Signal(const Signal &) = delete;
  Signal & operator=(const Signal &) = delete;

  /**
   * @brief Move constructor.
   *
   * Transfers ownership of signal handler management. The moved-from object
   * will not restore the previous handler.
   */
  Signal(Signal && other) noexcept
    : old_action_(other.old_action_),
      signal_number_(other.signal_number_),
      restart_calls_(other.restart_calls_),
      active_(other.active_)
  {
    other.active_ = false;
  }

  /**
   * @brief Move assignment operator.
   *
   * If this object is active, restores its handler before taking ownership
   * from the other object.
   */
  Signal & operator=(Signal && other) noexcept
  {
    if (this != &other)
      {
        // Restore our handler first if active (ignore errors in move)
        if (active_)
          sigaction(signal_number_, &old_action_, nullptr);

        old_action_ = other.old_action_;
        signal_number_ = other.signal_number_;
        restart_calls_ = other.restart_calls_;
        active_ = other.active_;
        other.active_ = false;
      }
    return *this;
  }
};

/**
 * @brief Utility to wait for a signal.
 *
 * Blocks until one of the specified signals is delivered.
 *
 * @param signals The set of signals to wait for.
 * @return The signal number that was delivered.
 * @throws SignalError if sigwait fails.
 *
 * ## Example
 *
 * ```cpp
 * SignalSet wait_for({SIGINT, SIGTERM});
 * SignalBlocker blocker(wait_for);  // Block first, then wait
 *
 * int sig = wait_for_signal(wait_for);
 * std::cout << "Received signal " << sig << std::endl;
 * ```
 */
[[nodiscard]] inline int wait_for_signal(const SignalSet & signals)
{
  int sig = 0;
  int result = sigwait(signals.get(), &sig);
  if (result != 0)
    throw SignalError("sigwait failed", -1, result);
  return sig;
}

/**
 * @brief Sends a signal to the current process.
 *
 * Convenience wrapper for raise().
 *
 * @param signo The signal to send.
 * @return true if successful, false otherwise.
 */
inline bool send_signal_to_self(int signo) noexcept
{
  return raise(signo) == 0;
}

/**
 * @brief Returns the name of a signal.
 *
 * Returns the standard POSIX signal name (e.g., "SIGINT", "SIGTERM").
 *
 * @param signo The signal number.
 * @return A string with the signal name or "Signal N" if unknown.
 */
[[nodiscard]] inline std::string signal_name(int signo)
{
  switch (signo)
    {
    case SIGHUP:    return "SIGHUP";
    case SIGINT:    return "SIGINT";
    case SIGQUIT:   return "SIGQUIT";
    case SIGILL:    return "SIGILL";
    case SIGABRT:   return "SIGABRT";
    case SIGFPE:    return "SIGFPE";
    case SIGKILL:   return "SIGKILL";
    case SIGSEGV:   return "SIGSEGV";
    case SIGPIPE:   return "SIGPIPE";
    case SIGALRM:   return "SIGALRM";
    case SIGTERM:   return "SIGTERM";
    case SIGUSR1:   return "SIGUSR1";
    case SIGUSR2:   return "SIGUSR2";
    case SIGCHLD:   return "SIGCHLD";
    case SIGCONT:   return "SIGCONT";
    case SIGSTOP:   return "SIGSTOP";
    case SIGTSTP:   return "SIGTSTP";
    case SIGTTIN:   return "SIGTTIN";
    case SIGTTOU:   return "SIGTTOU";
#ifdef SIGBUS
    case SIGBUS:    return "SIGBUS";
#endif
#ifdef SIGTRAP
    case SIGTRAP:   return "SIGTRAP";
#endif
#ifdef SIGURG
    case SIGURG:    return "SIGURG";
#endif
#ifdef SIGXCPU
    case SIGXCPU:   return "SIGXCPU";
#endif
#ifdef SIGXFSZ
    case SIGXFSZ:   return "SIGXFSZ";
#endif
#ifdef SIGVTALRM
    case SIGVTALRM: return "SIGVTALRM";
#endif
#ifdef SIGPROF
    case SIGPROF:   return "SIGPROF";
#endif
#ifdef SIGWINCH
    case SIGWINCH:  return "SIGWINCH";
#endif
#ifdef SIGIO
    case SIGIO:     return "SIGIO";
#endif
#ifdef SIGSYS
    case SIGSYS:    return "SIGSYS";
#endif
    default:        return "Signal " + std::to_string(signo);
    }
}

/**
 * @brief Returns a human-readable description of a signal.
 *
 * Uses strsignal() if available, otherwise returns the signal name.
 *
 * @param signo The signal number.
 * @return A descriptive string for the signal.
 */
[[nodiscard]] inline std::string signal_description(int signo)
{
  const char * desc = strsignal(signo);
  if (desc)
    return desc;
  return signal_name(signo);
}

# endif // AH_SIGNAL_H
