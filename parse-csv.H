
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
# ifndef PARSE_CSV_H
# define PARSE_CSV_H

# include <sstream>
# include <istream>
# include <string>
# include <tpl_array.H>

using namespace std;

inline Array<string> csv_read_row(istream &in, char delimiter = ',')
{
  stringstream ss;
  bool inquotes = false;
  Array<string> row; // relying on RVO
  while (in.good())
    {
      char c = in.get();
      if (not inquotes and c == '"') // begin quote char
        inquotes = true;
      else if (inquotes and c == '"') // quote char
        {
          if (in.peek() == '"') // 2 consecutive quotes resolve to 1
            ss << (char) in.get();
          else // endquotechar
            inquotes = false;
        }
      else if (not inquotes and c == delimiter) // end of field
        {
          row.append(ss.str());
          ss.str("");
        }
      else if (not inquotes and (c == '\r' or c == '\n'))
        {
          if (in.peek() == '\n')
            in.get();
          row.append(ss.str());
          return row;
        }
      else
        ss << c;
    }
  return row;
}

inline Array<string> csv_read_row(string &line, char delimiter)
{
  stringstream ss(line);
  return csv_read_row(ss, delimiter);
}

# endif // PARSE_CSV_H
