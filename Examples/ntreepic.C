

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

      draw_list_representation = false;
      break;
    case 'A':
      with_arrow = true;
      break;
    case 'K':
      with_arrow = true;
      if (arg == NULL)
	AH_ERROR("Waiting for arrow lenght in command line");
      arrow_lenght = atof(arg);
      break;
    case 'I':
      with_arrow = true;
      if (arg == NULL)
	AH_ERROR("Waiting for arrow width in command line");
      arrow_width = atof(arg);
      break;
    case 'F':
      flip_y = true;
      break;
    default: return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


static struct argp arg_defs = { options, parser_opt, arg_doc, doc, 0, 0, 0 }; 

int main(int argc, char *argv[]) 
{
  command_line = command_line_to_string(argc, argv);

  read_parameters();

  argp_parse(&arg_defs, argc, argv, ARGP_IN_ORDER, 0, NULL);

  if (input_file_name.size() == 0)
    AH_ERROR("Input file not given");

  ifstream input_stream(input_file_name.c_str(), ios::in);

  if (not input_stream)
    AH_ERROR("%s file does not exist", input_file_name.c_str());

  cout << hello;

  cout << "input from " << input_file_name << " file " << endl;

  if (output_file_name.size() == 0)
    {
      output_file_name = input_file_name;
      const int pos = input_file_name.rfind(".");
      if (pos != string::npos)
	output_file_name = 
	  string(input_file_name).replace(pos, input_file_name.size() - pos, 
					  string(""));
      output_file_name = output_file_name + (tiny_keys ? ".eepicaux" : ".eepic");
    }

  ofstream output_stream(output_file_name.c_str(), ios::out);

  cout << "output sent to " << output_file_name << " file " << endl << endl;

  EepicNode * root = read_input_and_build_tree(input_stream);

  if (generate_binary_tree)
    generate_bin_tree(output_stream, root);
  else
    {
      compute_coordinates_for_forest_and_set_picture_size(root);

      generate_forest(output_stream, root);
    }

  destroy_tree(root);

  save_parameters();
}
