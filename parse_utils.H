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

# ifndef PARSE_UTILS_H
# define PARSE_UTILS_H

# include <string>
# include <fstream>
# include <iostream>
# include <aleph.H>

using namespace std;

# define PRINT_ERROR(str, args...)  ( \
  (cout << input_file_name << "(" << previous_line_number << "," \
        << previous_col_number << "): " << endl), \
  save_parameters(), \
  (cout << "Last token: " << token_instance << endl), \
  AH_ERROR(str, ##args))


extern const size_t Buffer_Size;

extern int line_number;
extern int pos_number;

extern int previous_line_number;
extern int previous_col_number;

extern string token_instance;

void init_token_scanning();
void close_token_scanning(char * buffer, char *& start_addr, char * end_addr);

int read_char_from_stream(ifstream& input_stream) throw(out_of_range);

void skip_white_spaces(ifstream& input_stream);

long load_number(ifstream& input_stream);

string load_string(ifstream & input_stream);

void print_parse_error_and_exit(const string & str);

void print_parse_warning(const string & str);

void put_char_in_buffer(char *& start_addr, char * end_addr, int c);

string command_line_to_string(int argc, char *argv[]);


# endif // PARSE_UTILS_H
