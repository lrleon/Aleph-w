
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
