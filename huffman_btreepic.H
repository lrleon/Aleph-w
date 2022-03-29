
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

void huffman_to_btreepic(Freq_Node * p, const bool with_level_adjust = false)
{
  inOrderRec(p, save_infix_pos);

  *output_ptr << "start-prefix ";

  preOrderRec(p, write_prefix);

  *output_ptr << endl
	      << "start-key ";

  inOrderRec(p, write_freq);  

  *output_ptr << endl;

  inOrderRec(p, write_leaves);  

  *output_ptr << endl;

  if (not with_level_adjust)
    return;

  const size_t tree_size = infix_table.size();

  levelOrder(p, save_level_pos_1);

  levelOrder(p, save_level_pos_2);

  // Recorrer por infijo y desplazar etiquetas del mismo nivel que
  // queden muy pegadas  
  inOrderRec(p, adjust_nodes);
}


