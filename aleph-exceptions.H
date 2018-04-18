/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/
# ifndef ALEPH_EXCEPTIONS_H
# define ALEPH_EXCEPTIONS_H

# include <exception>
# include <sstream>

# include <ahUtils.H>

using namespace std;

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
  const string file_name = "No defined"; 

  const string type = "No defined"; /// Type name
  const string msg = "No defined"; /// Message describing the error

protected:

  string category_msg = "No defined";

private:

  static string make_what(const string & category_msg,
			  size_t line_number,
			  const string & file_name,
			  const string & type,
			  const string & msg)
  {
    ostringstream s;
    s << category_msg << "|" << file_name << "|" << line_number
      << "|" << type << "|" << msg;
    return s.str();
  }

public:

  AlephException() : runtime_error(msg) {}

  AlephException(const string & category_msg,
	       const size_t line_number,
	       const string & file_name,
	       const string & type,
	       const string & msg)
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
	 const string & file_name,					\
	 const string & type,						\
	 const string & msg)						\
      : AlephException(#category_msg, line_number, file_name, type, msg) {} \
  };

/** Macro for throwing an aleph exception
   
   @param[in] type aleph exception class name
   @param[in] msg message describing the error
   @ingroup Exceptions
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
