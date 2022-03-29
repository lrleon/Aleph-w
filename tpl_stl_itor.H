

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

  iterator (Tree_Type & tree) : itor (tree)
  {
    init_flags ();
  }

  /// Proporciona una referencia al elemento actual.
  Pair & operator * ()
  {
    return KEY (itor.get_current () );
  }

  /// "Dereferencia" un puntero al elemento actual.
  Pair * operator -> ()
  {
    return &KEY (itor.get_current () );
  }

  /// Avanza el iterador una posición hacia adelante, luego carga
  /// el elemento actual.
  Pair & operator ++ ()
  {
    forward ();

    return KEY (itor.get_current () );
  }

  Pair & operator ++ (int)
  {
    Pair & retPair = KEY (itor.get_current () );
    forward ();

    return retPair;
  }

  Pair & operator -- ()
  {
    backward ();

    return KEY (itor.get_current () );
  }

  Pair & operator -- (int)
  {
    Pair & retPair = KEY (itor.get_current () );
    backward ();

    return retPair;
  }
    
  Pair & operator += (const size_type & n)
  {
    itor.reset_to_pos (itor.get_current_position () + n);

    return KEY (itor.get_current () );
  } 

  Pair & operator -= (const size_type & n)
  {
    itor.reset_to_pos (itor.get_current_position () - n);

    return KEY (itor.get_current () );
  } 

  bool operator == (const iterator & _itor) const
  {
    return itor == _itor.itor;
  }

  bool operator != (const iterator & _itor) const
  {
    return not (itor == _itor.itor);
  }

  bool verify (const map & _map) const
  {
    return itor.verify ( (Tree_Type*) &_map.tree); 
  }

  bool verify (const iterator & it) const
  {
    return itor.verify (it.itor);
  }
};



} // end namespace Aleph

# endif // TPL_STD_ITRO_H
