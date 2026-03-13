
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
# include <tpl_dnode.H>

using namespace std;

using Node = Dnode<long>;

static long counter = 0;

Node * create_list_with_header(size_t n)
{
  Node * head = new Node;
  for (auto i = 0; i < n; ++i)
    head->append(new Node(counter++));

  return head;
}

Node * create_list_without_header(size_t n)
{
  Node * p = new Node(1000 + counter++);
  for (auto i = 1; i < n; ++i)
    p->append(new Node(1000 + counter++));
  return p;
}

Node * access(Node * list, size_t n)
{
  Node::Iterator it(list); 
  for (auto i = 0; i < n; ++i)
    it.next();
  return it.get_curr();
}

ostream & operator << (ostream & out, Node * p)
{
  for (Node::Iterator it(p); it.has_curr(); it.next())
    out << it.get_curr()->get_data() << " ";
  return out;
}


int main(int argc, char *argv[])
{
  if (argc != 4)
    {
      cout << "usage: " << argv[0] << " n m i" << endl
	   << endl
	   << "Where n: number of items of big list" << endl
	   << "      m: number of items of sublist to be inserted" << endl
	   << "      i: position where the sublist will be inserted" << endl;
      exit(0);
    }

  size_t n = atoi(argv[1]);
  size_t m = atoi(argv[2]);
  size_t i = atoi(argv[3]);

  Node * list = create_list_with_header(n);

  access(list, i)->splice(create_list_without_header(m));

  cout << list << endl;

  list->remove_all_and_delete();
  delete list;
}
