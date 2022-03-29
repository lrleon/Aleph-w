

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

  void decode(BitArray & bit_stream, ostream & output)
  {
    const size_t & bit_stream_len = bit_stream.size();
    BinNode<string> * p = root;
    for (int i = 0; i < bit_stream_len; ++i)
      {
        if (bit_stream.read_bit(i) == 0)
          p = LLINK(p);
        else
          p = RLINK(p);


       if (p == nullptr)
         throw std::domain_error("Invalid bits sequence");

       if (is_leaf(p)) // ¿es hoja?
         {     // sí ==> escribir símbolo y reiniciar a raíz
           const string & symbol = p->get_key();
           if (symbol == end_symbol) // ¿se alcanza final?
             break;

           output << symbol;
           p = root; // reiniciar a raíz, pues se leerá un nuevo código
         }
      }
  }
};

} // end namespace Aleph

# endif // HUFFMAN_H
