

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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
