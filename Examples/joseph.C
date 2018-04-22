
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
# include "tpl_dynDlist.H"

void avanceItor(DynDlist<unsigned>::Iterator& itor, unsigned s)
{
  for (unsigned i = 0; i < s; i++)
    {
      if (not itor.has_current())
        itor.reset_first();

      itor.next();
    }

  if (not itor.has_current())
    itor.reset_first();
}
void orden_ejecucion(unsigned num_personas, unsigned paso)
{
  DynDlist<unsigned> list;

  for (unsigned i = 1; i <= num_personas; i++)
    list.append(i);
      
  DynDlist<unsigned>::Iterator itor(list);
  for (/* nothing */; num_personas > 1; num_personas--)
    {
      avanceItor(itor, paso);
      printf("%u ", itor.get_current());
      itor.del();
    }

  printf("\nEl sobreviviente es %u\n", list.get_first());    
}

int main(int, char** args)
{
  unsigned numPer = atoi(args[1]),
           paso   = atoi(args[2]);

  orden_ejecucion(numPer, paso);
}
