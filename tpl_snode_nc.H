

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
 }

      // Instancia iterador en lista cuyo nodo cabecera es head_ptr y
      // coloca el elemento actual en el apuntado por curr_ptr 
      Iterator(Snode_Nc<T> * head_ptr, Snode_Nc<T> * curr_ptr) 
        : Slink_Nc::Iterator(head_ptr, curr_ptr)
      {
        // Empty
      }

      Snode_Nc * get_curr() /// retorna puntero al nodo actual 
      {
        return (Snode_Nc *) this->Slink_Nc::Iterator::get_current(); 
      }
    };
  };
}

# endif

