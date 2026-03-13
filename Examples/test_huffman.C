
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

# include <iostream>
# include <fstream>
# include <tpl_binNodeUtils.H>
# include <Huffman.H>
# include <huffman_btreepic.H>

using namespace std;

char  poema1 [] = 
"             Las cosas\n"
"\n"
"El bastón, las monedas, el llavero,\n"
"la dócil cerradura, las tardías\n"
"notas que no leerán los pocos días\n" 
"que me quedan, los naipes y el tablero,\n"
"\n"
"un libro y en sus páginas la ajada\n"
"violeta, monumento de una tarde\n"
"sin duda inolvidable y ya olvidada,\n"
"el rojo espejo occidental en que arde\n"
"\n"
"una ilusoria aurora. ¡Cuántas cosas,\n"
"láminas, umbrales, atlas, copas, clavos,\n"
"nos sirven como tácitos esclavos,\n"
"\n"
"ciegas y extrañamente sigilosas!\n"
"Durarán más allá de nuestro olvido;\n" 
"no sabrán nunca que nos hemos ido.\n"
"\n"
"            Jorge Luis Borges\n";


char  poema [] = 
"El enamorado\n"
"\n"
"Lunas, marfiles, instrumentos, rosas,\n"
"lámparas y la línea de Durero,\n"
"las nueve cifras y el cambiante cero,\n"
"debo fingir que existen esas cosas.\n"
"\n"
"Debo fingir que en el pasado fueron\n"
"Persépolis y Roma y que una arena\n"
"sutil midió la suerte de la almena\n"
"que los siglos de hierro deshicieron.\n" 
"\n"
"Debo fingir las armas y la pira\n"
"de la epopeya y los pesados mares\n"
"que roen de la tierra los pilares.\n"
"\n"
"Debo fingir que hay otros. Es mentira.\n"
"Sólo tú eres. Tú, mi desventura\n"
"y mi ventura, inagotable y pura.\n"
"\n"
"            Jorge Luis Borges\n";


const size_t read_and_encode(char *                   str, 
			     Huffman_Encoder_Engine & huffman_engine,
			     BitArray &               bit_stream)
{
  huffman_engine.read_input(str, true);

  const size_t bit_stream_len = huffman_engine.encode(str, bit_stream);

  return bit_stream_len;
}


void print_node(BinNode<string> * p, int, int)
{
  cout << p->get_key() << " ";
}

void print_code(BitArray & cod, const size_t & len)
{
  for (int i = 0; i < len; ++i)
    cout << cod[i] << " ";
  cout << endl << endl;
}

int main()
{
  ofstream output("borges.Tree", ios::out);
  output_ptr = &output;

  Huffman_Encoder_Engine encoder;

  size_t code_len = 2048;

  BitArray code(code_len);

  code_len = read_and_encode(poema, encoder, code);

  print_code(code, code_len);

  Huffman_Decoder_Engine decoder(encoder.get_root(), "");

  huffman_to_btreepic(encoder.get_freq_root(), true);

  decoder.decode(code, std::cout);

  std::cout << std::endl;

  destroyRec(encoder.get_root());
}
