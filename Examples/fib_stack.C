

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

   if (stack.size() == 1) 
     return RESULT(caller_ar);

   caller_ar  = &stack.top(1);
   current_ar = &stack.top();

   switch (RETURN_POINT(current_ar))
     {
     case P1: goto p1;
     case P2: goto p2;
     default: AH_ERROR("Invalid return point case");
     } 

   return 0; // never reached!
}
