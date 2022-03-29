

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

      threaded_trees = true; 
      break;
    case 'D':
      dash_threaded_trees = true; 
      break;
    case 'e':
      with_external_nodes = true; 
      break;
    case 'A':
      with_arrow = true;
      break;
    case 'L':
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
      break;    default: return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


static struct argp argDefs = { options, parser_opt, argDoc, doc, 0, 0, 0 }; 


int main(int argc, char *argv[]) 
{
  command_line = command_line_to_string(argc, argv);

  read_parameters();

  argp_parse(&argDefs, argc, argv, ARGP_IN_ORDER, 0, NULL);

  if (input_file_name.size() == 0)
    AH_ERROR("Input file not given");

  cout << hello;

  file_to_dynarrays(input_file_name.c_str());

  EepicNode<long> * root = build_tree();

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
  
  cout << "input from " << input_file_name << " file " << endl
       << "output sent to " << output_file_name << " file " << endl << endl;

  ofstream output(output_file_name.c_str(), ios::out);
  generate_prologue(output);
  generate_tree(output, root);
  generate_split_lines(output, root);
  generate_epilogue(output);
  output.close();
  
  save_parameters();
}
