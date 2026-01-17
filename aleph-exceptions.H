
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

/** Aleph exception class

   @author Leandro Rabindranath Leon 
   @ingroup Exceptions
   @note This class is not intended for public use. Instead use the
   macro DEFINE_ALEPH_EXCEPTION 
 */
struct AlephException : public std::runtime_error
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

/** Define a new exception

    @param[in] name of the exception
    @param[in] category_msg

    @ingroup Exceptions
    @author Leandro Rabindranath Leon 
 */
# define DEFINE_ALEPH_EXCEPTION(name, category_msg)			\
  struct name : public AlephException					\
  {									\
    name(const size_t line_number,					\
	 const std::string & file_name,					\
	 const std::string & type,					\
	 const std::string & msg)					\
      : AlephException(#category_msg, line_number, file_name, type, msg) {} \
  };

/** Macro for throwing an aleph exception
   
   @param[in] type aleph exception class name
   @param[in] msg message describing the error
   @ingroup Exceptions
 *  @author Leandro Rabindranath León
 */
# define ALEPHTHROW(type, msg) throw type(__LINE__, __FILE__, #type, msg)

DEFINE_ALEPH_EXCEPTION(MinMaxReversed,
		     "minimum value is greater than maximum value");
DEFINE_ALEPH_EXCEPTION(InvalidCsvHeader, "Invalid csv header");
DEFINE_ALEPH_EXCEPTION(InvalidCsvRow, "Invalid csv row");
DEFINE_ALEPH_EXCEPTION(InvalidValue, "Invalid value");
DEFINE_ALEPH_EXCEPTION(OutOfMemory, "No memory");
DEFINE_ALEPH_EXCEPTION(OutOfRange, "Value is out of allowed range");
DEFINE_ALEPH_EXCEPTION(ValueNotFound, "Value not found");
DEFINE_ALEPH_EXCEPTION(CommandLineError, "error parsing command line");
DEFINE_ALEPH_EXCEPTION(InvariantError, "A bug has been detected. "
		     "Please send a bug report to leandro.r.leon@gmail.com");
DEFINE_ALEPH_EXCEPTION(SizeMismatch, "Sequences have different sizes");
DEFINE_ALEPH_EXCEPTION(InvalidConversion, "failure in numeric conversion");
DEFINE_ALEPH_EXCEPTION(NameNotFound, "name not found");
DEFINE_ALEPH_EXCEPTION(InvalidSize, "invalid size");
DEFINE_ALEPH_EXCEPTION(InvalidRange, "invalid range");
DEFINE_ALEPH_EXCEPTION(InvalidUnit, "invalid unit");
DEFINE_ALEPH_EXCEPTION(DuplicatedOperation, "duplicated operation");
DEFINE_ALEPH_EXCEPTION(DuplicatedName, "Duplicated name");
DEFINE_ALEPH_EXCEPTION(InvalidRead, "Error while reading input stream");
DEFINE_ALEPH_EXCEPTION(FileNotFound, "file not found");

# endif
