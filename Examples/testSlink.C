
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

# include <aleph.H>
# include <slink.H>

# define NumItems 10

struct Record1 : public Slink
{
  unsigned n;

  Record1* get_next() { return static_cast<Record1*>(Slink::get_next()); }
};

struct Record2 
{
  unsigned n;

  Slink link;

  SLINK_TO_TYPE(Record2, link);
};
# include <iostream>


void* foo();

int main()
{
  {
    Record1 head;
    unsigned i;
    Record1 *node; 
    Slink *aux;

    for (i = 0; i < NumItems; i++)
      {
	node = new Record1;

	node->n = i;
	head.insert_next(node);
      }

    for (node = head.get_next(); node not_eq &head; node = node->get_next())
      {
	printf("%u ", node->n);
      }

    while (not head.is_empty())
      {
	aux = head.remove_next();
	delete aux;
      }
  }

  {
    Slink head;
    unsigned i;
    Record2 *node; 
    Slink *link;

    for (i = 0; i < NumItems; i++)
      {
	node = new Record2;

	node->n = i;
	head.insert_next(&node->link);
      }

    for (link = head.get_next(); link not_eq &head; link = link->get_next())
      {
	node = Record2::slink_to_type(link);
	printf("%u ", node->n);
      }

    while (not head.is_empty())
      {
	node = Record2::slink_to_type(head.remove_next());
	delete node;
      }
  }

  printf("Ended\n");
}
