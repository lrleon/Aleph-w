

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

inline
void execute_R_script(const string & scr, const string & file_name = "tmp.R")
{
  std::ofstream tmp(file_name);
  tmp << "#!/usr/bin/Rscript" << endl
      << endl
      << "X11()" << endl
      << scr << endl
      << "message(\"Press Return To Continue\")" << endl
      << "invisible(readLines(\"stdin\", n=1))" << endl;
  const string cmd = "Rscript " + file_name;
  system(cmd.c_str());
}

inline bool is_normal_number(double n)
{
  int status = fpclassify(n);
  return (status == FP_NORMAL or status == FP_ZERO) // and status != FP_NAN and
    // status != FP_INFINITE and status != FP_SUBNORMAL
    ;
}

}


# endif // AHUTILS_H

