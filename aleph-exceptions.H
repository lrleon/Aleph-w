
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


/** @file aleph-exceptions.H
 *  @brief Custom exception classes with source location tracking.
 *
 *  This file provides AlephException, a custom exception class that
 *  captures file name, line number, and function name where the
 *  exception was thrown.
 *
 *  ## Key Features
 *
 *  - Inherits from std::runtime_error
 *  - Captures source location (file, line, function)
 *  - Human-readable error messages
 *  - Macro for defining new exception types
 *
 *  ## AlephException Members
 *
 *  - `line_number`: Line where exception was thrown
 *  - `file_name`: Source file name
 *  - `function_name`: Function where thrown
 *
 *  ## Defining Custom Exceptions
 *
 *  Use the DEFINE_ALEPH_EXCEPTION macro:
 *
 *  ```cpp
 *  DEFINE_ALEPH_EXCEPTION(MyError);
 *
 *  // Usage:
 *  throw MyError(__FILE__, __LINE__, "my_function", "Something failed");
 *  ```
 *
 *  ## Recommended Alternative
 *
 *  For new code, prefer using `ah-errors.H` macros which provide
 *  automatic source location capture:
 *
 *  ```cpp
 *  ah_domain_error_if(x < 0, "x must be positive");
 *  ```
 *
 *  @deprecated Nothing of Aleph but `LFit` uses it, which throws
 *  SizeMismatch until 8.0.0: `aleph-exceptions.H` goes in 8.0.0, and
 *  `LFit` throws then `std::invalid_argument`, as the macros of
 *  `ah-errors.H` do. Its classes are `[[deprecated]]`, and
 *  DEFINE_ALEPH_EXCEPTION, which derives from AlephException, warns where
 *  it is used. To catch the errors of `LFit` both before and after 8.0.0,
 *  catch `std::exception`.
 *
 *  @see ah-errors.H Modern error handling macros
 *  @see ahUtils.H Utility functions used by exceptions
 *
 *  @ingroup Exceptions
 *  @author Leandro Rabindranath León
 */

# ifndef ALEPH_EXCEPTIONS_H
# define ALEPH_EXCEPTIONS_H

# include <exception>
# include <sstream>

# include <ahUtils.H>

// The classes are deprecated, and so are their uses here: they warn only
// where the programs use them. The public macros are defined outside of
// the regions without the warning, which, for GCC, cover what they spell
# if defined(_MSC_VER) && !defined(__clang__)
#   pragma warning(push)
#   pragma warning(disable : 4996)
# else
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated-declarations"
# endif

# define ALEPH_EXCEPTIONS_DEPRECATED \
  [[deprecated("aleph-exceptions.H goes in 8.0.0; use the macros of ah-errors.H instead")]]

/** Aleph exception class

   @author Leandro Rabindranath Leon 
   @ingroup Exceptions
   @note This class is not intended for public use. Instead use the
   macro DEFINE_ALEPH_EXCEPTION 
   @deprecated `aleph-exceptions.H` goes in 8.0.0: use the macros of
   `ah-errors.H`, which throw the exceptions of the standard library.
 */
struct ALEPH_EXCEPTIONS_DEPRECATED AlephException : public std::runtime_error
{
  /// Line number of source file where the exception was thrown
  const size_t line_number = 0;
  
  /// file name where the exception was thrown 
  const std::string file_name = "No defined"; 

  const std::string type = "No defined"; /// Type name
  const std::string msg = "No defined"; /// Message describing the error

protected:

  std::string category_msg = "No defined";

private:

  static std::string make_what(const std::string & category_msg,
			       size_t line_number,
			       const std::string & file_name,
			       const std::string & type,
			       const std::string & msg)
  {
    std::ostringstream s;
    s << category_msg << "|" << file_name << "|" << line_number
      << "|" << type << "|" << msg;
    return s.str();
  }

public:

  AlephException() : std::runtime_error(msg) {}

  AlephException(const std::string & category_msg,
	       const size_t line_number,
	       const std::string & file_name,
	       const std::string & type,
	       const std::string & msg)
    : runtime_error(make_what(category_msg, line_number, file_name, type, msg)),
      line_number(line_number), file_name(file_name), type(type) {}
};

# if defined(_MSC_VER) && !defined(__clang__)
#   pragma warning(pop)
# else
#   pragma GCC diagnostic pop
# endif

/** Define a new exception

    @param[in] name of the exception
    @param[in] category_msg

    @ingroup Exceptions
    @author Leandro Rabindranath Leon 
    @deprecated `aleph-exceptions.H` goes in 8.0.0: the class that it
    defines derives from the deprecated AlephException, and the macro
    warns where it is used.
 */
# define DEFINE_ALEPH_EXCEPTION(name, category_msg)			\
  struct name : public AlephException					\
  {									\
    name(const size_t line_number,					\
	 const std::string & file_name,					\
	 const std::string & type,					\
	 const std::string & msg)					\
      : AlephException(#category_msg, line_number, file_name, type, msg) {} \
									\
  private:								\
    /* GCC does not warn of a deprecated base class: of this, it does */ \
    using aleph_deprecated_base_ = AlephException;			\
  };

/** Macro for throwing an aleph exception
   
   @param[in] type aleph exception class name
   @param[in] msg message describing the error
   @ingroup Exceptions
 *  @author Leandro Rabindranath León
 *  @deprecated `aleph-exceptions.H` goes in 8.0.0: use the macros of
 *  `ah-errors.H`. The classes that it throws are deprecated.
 */
# define ALEPHTHROW(type, msg) throw type(__LINE__, __FILE__, #type, msg)

# if defined(_MSC_VER) && !defined(__clang__)
#   pragma warning(push)
#   pragma warning(disable : 4996)
# else
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated-declarations"
# endif

// The exceptions of Aleph, deprecated as AlephException
# define ALEPH_DEFINE_DEPRECATED_EXCEPTION(name, category_msg)		\
  struct ALEPH_EXCEPTIONS_DEPRECATED name : public AlephException	\
  {									\
    name(const size_t line_number,					\
	 const std::string & file_name,					\
	 const std::string & type,					\
	 const std::string & msg)					\
      : AlephException(#category_msg, line_number, file_name, type, msg) {} \
  };

ALEPH_DEFINE_DEPRECATED_EXCEPTION(MinMaxReversed,
                                  "minimum value is greater than maximum value");
ALEPH_DEFINE_DEPRECATED_EXCEPTION(InvalidCsvHeader, "Invalid csv header");
ALEPH_DEFINE_DEPRECATED_EXCEPTION(InvalidCsvRow, "Invalid csv row");
ALEPH_DEFINE_DEPRECATED_EXCEPTION(InvalidValue, "Invalid value");
ALEPH_DEFINE_DEPRECATED_EXCEPTION(OutOfMemory, "No memory");
ALEPH_DEFINE_DEPRECATED_EXCEPTION(OutOfRange, "Value is out of allowed range");
ALEPH_DEFINE_DEPRECATED_EXCEPTION(ValueNotFound, "Value not found");
ALEPH_DEFINE_DEPRECATED_EXCEPTION(CommandLineError, "error parsing command line");
ALEPH_DEFINE_DEPRECATED_EXCEPTION(InvariantError, "A bug has been detected. "
                                  "Please send a bug report to leandro.r.leon@gmail.com");
ALEPH_DEFINE_DEPRECATED_EXCEPTION(SizeMismatch, "Sequences have different sizes");
ALEPH_DEFINE_DEPRECATED_EXCEPTION(InvalidConversion, "failure in numeric conversion");
ALEPH_DEFINE_DEPRECATED_EXCEPTION(NameNotFound, "name not found");
ALEPH_DEFINE_DEPRECATED_EXCEPTION(InvalidSize, "invalid size");
ALEPH_DEFINE_DEPRECATED_EXCEPTION(InvalidRange, "invalid range");
ALEPH_DEFINE_DEPRECATED_EXCEPTION(InvalidUnit, "invalid unit");
ALEPH_DEFINE_DEPRECATED_EXCEPTION(DuplicatedOperation, "duplicated operation");
ALEPH_DEFINE_DEPRECATED_EXCEPTION(DuplicatedName, "Duplicated name");
ALEPH_DEFINE_DEPRECATED_EXCEPTION(InvalidRead, "Error while reading input stream");
ALEPH_DEFINE_DEPRECATED_EXCEPTION(FileNotFound, "file not found");

# undef ALEPH_DEFINE_DEPRECATED_EXCEPTION
# undef ALEPH_EXCEPTIONS_DEPRECATED

# if defined(_MSC_VER) && !defined(__clang__)
#   pragma warning(pop)
# else
#   pragma GCC diagnostic pop
# endif

# endif
