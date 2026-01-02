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

/** @file parse_utils.H
 *  @brief General parsing utilities.
 *  @ingroup Utilidades
 *  @author Leandro Rabindranath León
 */

# ifndef PARSE_UTILS_H
# define PARSE_UTILS_H

# include <string>
# include <fstream>
# include <iostream>
# include <aleph.H>

# define PRINT_ERROR(str, args...)  (                                   \
                                     (std::cout << input_file_name << "(" << previous_line_number << "," \
                                      << previous_col_number << "): " << std::endl), \
                                     save_parameters(),                 \
                                     (std::cout << "Last token: " << token_instance << std::endl), \
                                     AH_ERROR(str, ##args))


extern const size_t Buffer_Size;

extern int line_number;
extern int pos_number;

extern int previous_line_number;
extern int previous_col_number;

extern std::string token_instance;

void init_token_scanning();
void close_token_scanning(char * buffer, char *& start_addr, char * end_addr);

int read_char_from_stream(std::ifstream& input_stream);

void skip_white_spaces(std::ifstream& input_stream);

long load_number(std::ifstream& input_stream);

std::string load_string(std::ifstream & input_stream);

void print_parse_error_and_exit(const std::string & str);

void print_parse_warning(const std::string & str);

void put_char_in_buffer(char *& start_addr, char * end_addr, int c);

std::string command_line_to_string(int argc, char *argv[]);


# endif // PARSE_UTILS_H
