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
 * @author Leandro Rabindranath León
 * @ingroup Examples
*/

/**
 * @file write_huffman.C
 * @brief Huffman encoding/decoding demonstration with tree visualization
 * 
 * This example demonstrates Huffman compression, one of the most important
 * lossless data compression algorithms. It encodes text using variable-length
 * codes based on character frequencies, achieving optimal compression for
 * the given frequency distribution. Huffman coding is the foundation of many
 * modern compression algorithms.
 *
 * ## What is Huffman Coding?
 *
 * ### Problem
 *
 * **Goal**: Compress text by assigning shorter codes to frequent characters
 *
 * **Challenge**: 
 * - Fixed-length codes (ASCII) waste space
 * - Variable-length codes must be unambiguous
 * - Need prefix-free property (no code is prefix of another)
 *
 * ### Solution: Huffman Coding
 *
 * Huffman coding is a **greedy algorithm** that assigns shorter codes to more
 * frequent characters and longer codes to less frequent ones. It builds a
 * binary tree where:
 * - **Leaves**: Represent characters
 * - **Paths**: Path from root to leaf gives the character's code
 * - **Edges**: Left edges = 0, right edges = 1
 * - **Property**: Prefix-free (no code is prefix of another)
 *
 * ### Example
 *
 * ```
 * Frequencies: a=5, b=2, c=1, d=1
 *
 * Tree:
 *         9
 *        / \
 *       5   4
 *      a   / \
 *         2   2
 *        b   / \
 *           1   1
 *          c   d
 *
 * Codes:
 *   a: 0
 *   b: 10
 *   c: 110
 *   d: 111
 * ```
 *
 * ## Algorithm Steps
 *
 * ### Step 1: Frequency Analysis
 *
 * Count character frequencies in input:
 * ```
 * for each character in input:
 *   frequency[character]++
 * ```
 *
 * **Time**: O(n) where n = input length
 *
 * ### Step 2: Build Priority Queue
 *
 * Create min-heap of character nodes:
 * ```
 * for each character:
 *   create node(character, frequency)
 *   insert into min-heap
 * ```
 *
 * **Time**: O(k log k) where k = number of unique characters
 *
 * ### Step 3: Build Tree (Greedy Merging)
 *
 * Repeatedly merge two lowest-frequency nodes:
 * ```
 * while heap.size() > 1:
 *   left = extract_min()   // Lowest frequency
 *   right = extract_min()   // Second lowest
 *   merged = new_node(left, right)
 *   merged.frequency = left.frequency + right.frequency
 *   insert(merged)
 * ```
 *
 * **Time**: O(k log k) - k extractions from heap
 * **Result**: Single tree with all characters as leaves
 *
 * ### Step 4: Generate Codes
 *
 * Traverse tree to assign binary codes:
 * ```
 * generate_codes(node, code=""):
 *   if node is leaf:
 *     codes[node.char] = code
 *   else:
 *     generate_codes(node.left, code + "0")
 *     generate_codes(node.right, code + "1")
 * ```
 *
 * **Time**: O(k) - visit each leaf once
 *
 * ### Step 5: Encode
 *
 * Replace characters with their codes:
 * ```
 * for each character in input:
 *   output += codes[character]
 * ```
 *
 * **Time**: O(n) - process each character
 *
 * ### Total Complexity
 *
 * - **Time**: O(n + k log k) where k = unique characters
 * - **Space**: O(k) for tree and codes
 *
 * ## Compression Ratio
 *
 * ### Metrics
 *
 * The example shows:
 * - **Original size**: n × bits_per_char (e.g., n × 8 for ASCII)
 * - **Encoded size**: Σ (frequency × code_length)
 * - **Compression ratio**: encoded_size / original_size
 * - **Average bits per character**: encoded_size / n
 *
 * ### Optimality
 *
 * **Huffman coding is optimal** for the given frequency distribution:
 * - No other prefix-free code can achieve better compression
 * - Achieves entropy limit (Shannon's theorem)
 * - Best possible for fixed frequencies
 *
 * ### Example
 *
 * ```
 * Text: "aabacada"
 * Frequencies: a=5, b=1, c=1, d=1
 *
 * Original: 8 × 8 = 64 bits
 * Encoded: 5×1 + 1×2 + 1×3 + 1×3 = 13 bits
 * Compression: 13/64 = 20.3%
 * ```
 *
 * ## Output Files
 *
 * - **Huffman tree visualization**: LaTeX file for tree diagram
 *   - Shows tree structure
 *   - Displays character frequencies
 *   - Shows code assignments
 *
 * - **Encoding statistics**: Compression metrics
 *   - Original and encoded sizes
 *   - Compression ratio
 *   - Code table
 *
 * - **Encoded data**: Binary representation
 *   - Compressed output
 *   - Can be decoded using tree
 *
 * ## Applications
 *
 * ### File Compression
 * - **ZIP**: Uses Huffman coding (DEFLATE algorithm)
 * - **GZIP**: GNU zip compression
 * - **PNG**: Image compression format
 *
 * ### Image Compression
 * - **JPEG**: Uses Huffman for entropy coding
 * - **Lossless compression**: Maintains image quality
 *
 * ### Network Protocols
 * - **HTTP/2**: Header compression (HPACK)
 * - **Data transmission**: Reduce bandwidth usage
 *
 * ### Database Systems
 * - **Column compression**: Compress database columns
 * - **Storage optimization**: Reduce storage requirements
 *
 * ### Multimedia
 * - **Audio compression**: MP3 uses Huffman
 * - **Video compression**: Part of video codecs
 *
 * ## Example Input
 *
 * By default, encodes a poem by Federico García Lorca:
 * "Muerte de Antoñito el Camborio"
 *
 * This demonstrates:
 * - Spanish text with special characters
 * - Character frequency distribution
 * - Compression on real text
 *
 * ## Usage
 *
 * ```bash
 * # Encode built-in poem
 * ./write_huffman
 *
 * # Encode custom file
 * ./write_huffman input.txt
 *
 * # Set output prefix (files like <prefix>.Tree, <prefix>_tree.h, etc.)
 * ./write_huffman input.txt --output myhuffman
 * ```
 *
 * ## Decoding
 *
 * ### Process
 *
 * Decoding uses the same tree:
 * ```
 * current = root
 * for each bit in encoded:
 *   if bit == 0:
 *     current = current.left
 *   else:
 *     current = current.right
 *   if current is leaf:
 *     output += current.char
 *     current = root
 * ```
 *
 * **Time**: O(n) where n = encoded length
 *
 * ## Advantages
 *
 * ✅ **Optimal**: Best compression for given frequencies
 * ✅ **Fast**: O(n + k log k) encoding time
 * ✅ **Simple**: Easy to understand and implement
 * ✅ **Adaptive**: Can adapt to frequency changes
 *
 * ## Limitations
 *
 * ❌ **Two passes**: Need to analyze frequencies first
 * ❌ **Tree overhead**: Must store tree for decoding
 * ❌ **Fixed frequencies**: Doesn't adapt during encoding
 *
 * @see Huffman.H Huffman encoding implementation
 * @see huffman_btreepic.H Tree visualization utilities
 * @see heap_example.C Priority queues (used in Huffman)
 * @author Leandro Rabindranath León
 * @ingroup Examples
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
