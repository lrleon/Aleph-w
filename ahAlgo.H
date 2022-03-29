

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
; sourceBeg != sourceEnd; ++sourceBeg, ++destBeg)
    {
      *destBeg = *sourceBeg - diff;
      diff = *sourceBeg;
    }

  return destBeg;
}


template <class Itor1, class Itor2, class BinaryFunc, class T>
Itor2 adjacent_difference (Itor1 sourceBeg, Itor1 sourceEnd, Itor2 destBeg, BinaryFunc op)
{
  T diff = *sourceBeg;
  diff = 0;

  for (unsigned int i = 0; sourceBeg != sourceEnd; ++sourceBeg, ++destBeg, i++)
    {
      if (i = 0)
	*destBeg = *sourceBeg;
		
      else
	*destBeg = op(*sourceBeg, diff);
		
      diff = *sourceBeg;
    }

  return destBeg;
}

# endif //nada

} // end namespace Aleph

# endif // AHALGO_H
