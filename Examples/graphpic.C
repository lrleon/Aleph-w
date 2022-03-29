

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
 
      cout << argp_program_version;
      TERMINATE(0);

      break;
      
    default: return ARGP_ERR_UNKNOWN;
    }

  return 0;
}


static struct argp arg_defs = { options, parser_opt, argDoc, doc, 0, 0, 0 }; 


int main(int argc, char *argv[]) 
{
  command_line = command_line_to_string(argc, argv);

  read_parameters();

  argp_parse(&arg_defs, argc, argv, ARGP_IN_ORDER, 0, NULL);

  if (input_file_name.size() == 0)
    AH_ERROR("Input file not given");

  ifstream input_stream(input_file_name.c_str());

  if (not input_stream)
    AH_ERROR("%s file does not exist", input_file_name.c_str());

  cout << hello;

  cout << "input from " << input_file_name << " file " << endl;

  if (output_file_name.size() == 0)
    AH_ERROR("Output file not given");

  ofstream output_stream(output_file_name.c_str(), ios::out);

  cout << "output sent to " << output_file_name << " file " << endl << endl;

  Graph * g = read_input_and_build_graph(input_stream);

  generate_graph(g, output_stream);
  
  save_parameters();
}
