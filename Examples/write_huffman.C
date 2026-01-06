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

/**
 * @file write_huffman.C
 * @brief Huffman encoding/decoding demonstration
 * 
 * This example demonstrates Huffman compression by encoding a poem
 * and showing the compression ratio achieved. It also generates
 * LaTeX output files for visualization of the Huffman tree.
 * 
 * Usage: write_huffman [input_file]
 *        Without arguments: encodes a built-in poem
 *        With argument: encodes the specified file
 */

# include <cstring>
# include <iostream>
# include <fstream>
# include <tclap/CmdLine.h>
# include <Huffman.H>
# include <huffman_btreepic.H>

using namespace std;

// "Muerte de Antoñito el Camborio" by Federico García Lorca
char poem[] = 
"Muerte De Antoñito El Camborio\n"
"    Federico Garcia Lorca\n"
"\n"
"   Voces de muerte sonaron\n"
"cerca del Guadalquivir.\n"
"Voces antiguas que cercan\n"
"voz de clavel varonil.\n"
"Les clavo sobre las botas\n"
"mordiscos de jabali.\n"
"En la lucha daba saltos\n"
"jabonados de delfin.\n"
"Bano con sangre enemiga\n"
"su corbata carmesi,\n"
"pero eran cuatro punales\n"
"y tuvo que sucumbir.\n"  
"Cuando las estrellas clavan\n"
"rejones al agua gris,\n"
"cuando los erales suenan\n"
"veronicas de alheli,\n"
"voces de muerte sonaron\n"
"cerca del Guadalquivir.\n"
"\n"
"   Antonio Torres Heredia,\n"
"Camborio de dura crin,\n"
"moreno de verde luna,\n"
"voz de clavel varonil:\n"
"quien te ha quitado la vida\n"
"cerca del Guadalquivir?\n"
"Mis cuatro primos Heredias\n"
"hijos de Benameji.\n"
"Lo que en otros no envidiaban,\n"
"ya lo envidiaban en mi.\n"
"Zapatos color corinto,\n"
"medallones de marfil,\n"
"y este cutis amasado\n"
"con aceituna y jazmin.\n"
"Ay Antonito el Camborio,\n"
"digno de una Emperatriz!\n"
"Acuerdate de la Virgen\n"
"porque te vas a morir.\n"
"Ay Federico Garcia,\n"
"llama a la Guardia Civil!\n"
"Ya mi talle se ha quebrado\n"
"como cana de maiz.\n"
"\n"
"   Tres golpes de sangre tuvo\n"
"y se murio de perfil.\n"
"Viva moneda que nunca\n"
"se volvera a repetir.\n"
"Un angel marchoso pone\n"
"su cabeza en un cojin.\n"
"Otros de rubor cansado,\n"
"encendieron un candil.\n"
"Y cuando los cuatro primos\n"
"llegan a Benameji,\n"
"voces de muerte cesaron\n"
"cerca del Guadalquivir.\n";

/**
 * @brief Encode string and return encoded bit stream length
 */
size_t read_and_encode(char* str, 
                       Huffman_Encoder_Engine& huffman_engine,
                       BitArray& bit_stream)
{
  huffman_engine.read_input(str, true);
  return huffman_engine.encode(str, bit_stream);
}

/**
 * @brief Write poem in LaTeX verse format
 */
void write_verse(const char* text, const string& filename)
{
  ofstream verso(filename, ios::out);
  verso << "\\begin{verse}" << endl;
  
  const char* curr = text;
  while (*curr != '\0')
    {
      if (*curr == '\n')
        {
          if (curr[1] == '\n')
            {
              verso << " \\\\" << endl
                    << "\\ \\" << endl
                    << endl;
              curr++;
            }
          else if (curr[1] != '\0')
            {
              verso << " \\\\" << endl;
            }
        }
      else 
        {
          verso << *curr;
        }
      curr++;
    }
    
  verso << "\\end{verse}" << endl;
}

int main(int argc, char *argv[])
{
  try
    {
      TCLAP::CmdLine cmd("Huffman encoding demonstration", ' ', "1.0");

      TCLAP::UnlabeledValueArg<string> fileArg("input",
                                                "Input file to encode (optional, uses built-in poem if not provided)",
                                                false, "", "filename");
      cmd.add(fileArg);

      TCLAP::ValueArg<string> outputArg("o", "output",
                                         "Output tree file prefix",
                                         false, "huffman", "prefix");
      cmd.add(outputArg);

      cmd.parse(argc, argv);

      string inputFile = fileArg.getValue();
      string outputPrefix = outputArg.getValue();

      cout << "Huffman Encoding Example" << endl;
      cout << "========================" << endl << endl;

      ofstream huf_output(outputPrefix + ".Tree", ios::out);
      output_ptr = &huf_output;

      Huffman_Encoder_Engine encoder;
      BitArray code(0);
      size_t code_len = 0;

      if (inputFile.empty())
        {
          // Use built-in poem
          cout << "Encoding built-in poem (Lorca)..." << endl;
          
          code_len = read_and_encode(poem, encoder, code);
          
          // Generate Huffman tree visualization
          huffman_to_btreepic(encoder.get_freq_root());
          
          // Save tree in array format
          {
            ofstream output(outputPrefix + "_tree.h");
            encoder.save_tree_in_array_of_chars(outputPrefix.c_str(), output);
          }
          
          // Decode and verify
          cout << endl << "Decoded text:" << endl;
          cout << "-------------" << endl;
          Huffman_Decoder_Engine decoder(encoder.get_root(), "");
          decoder.decode(code, cout);
          cout << endl;
          
          // Write LaTeX files
          write_verse(poem, outputPrefix + "-verso.tex");
          
          {
            ofstream stat1(outputPrefix + "-stat1.tex", ios::out);
            stat1 << "$" << strlen(poem) * 8 << "$";
          }
          
          {
            ofstream stat2(outputPrefix + "-stat2.tex", ios::out);
            stat2 << "$" << code_len << "$";
          }
          
          // Print statistics
          size_t original_bits = strlen(poem) * 8;
          cout << endl << "Compression Statistics:" << endl;
          cout << "-----------------------" << endl;
          cout << "Original size: " << original_bits << " bits" << endl;
          cout << "Encoded size:  " << code_len << " bits" << endl;
          cout << "Compression ratio: " 
               << (100.0 * code_len / original_bits) << "%" << endl;
          cout << "Space saved: " 
               << (100.0 - 100.0 * code_len / original_bits) << "%" << endl;
          
          destroyRec(encoder.get_root());
          destroyRec(encoder.get_freq_root());
        }
      else
        {
          // Encode file
          cout << "Encoding file: " << inputFile << endl;
          
          ifstream input1(inputFile, ios::in);
          if (!input1)
            {
              cerr << "Error: cannot open file " << inputFile << endl;
              return 1;
            }
            
          encoder.read_input(input1, true);
          input1.close();
          
          huffman_to_btreepic(encoder.get_freq_root(), true);
          
          ifstream input2(inputFile, ios::in);
          BitArray bit_stream(1024 * 1024 * 500);  // 500MB max
          
          code_len = encoder.encode(input2, bit_stream);
          input2.close();
          
          // Get original file size
          ifstream input3(inputFile, ios::ate);
          size_t original_size = input3.tellg();
          input3.close();
          
          cout << endl << "Compression Statistics:" << endl;
          cout << "-----------------------" << endl;
          cout << "Original size: " << (original_size * 8) << " bits" << endl;
          cout << "Encoded size:  " << code_len << " bits" << endl;
          cout << "Compression ratio: " 
               << (100.0 * code_len / (original_size * 8)) << "%" << endl;
        }

      cout << endl << "Done. Output files:" << endl;
      cout << "  - " << outputPrefix << ".Tree" << endl;
      if (inputFile.empty())
        {
          cout << "  - " << outputPrefix << "_tree.h" << endl;
          cout << "  - " << outputPrefix << "-verso.tex" << endl;
          cout << "  - " << outputPrefix << "-stat1.tex" << endl;
          cout << "  - " << outputPrefix << "-stat2.tex" << endl;
        }
    }
  catch (TCLAP::ArgException &e)
    {
      cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
      return 1;
    }

  return 0;
}
