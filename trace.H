

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


# ifndef TRACE_H
# define TRACE_H

# include <stdio.h>


class Trace
{
  char * file;
  int line;

  char * name;

public:

  Trace(char * _file, int _line, char * _name) :
    file(_file), line(_line), name(_name)
  {
    printf("****ENTER %s in %s:%d\n", name, file, line);
  }

  ~Trace()
  {
    printf("****LEAVE %s\n", name);
  }
};

# define TRACE(name) Trace __trace__##__LINE__(__FILE__, __LINE__, name)

# endif // TRACE_H
