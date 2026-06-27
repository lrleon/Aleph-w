
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
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
