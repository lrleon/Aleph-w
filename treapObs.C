
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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
# include <fstream>
# include <time.h>
# include <aleph.H>
# include <tpl_binNodeUtils.H>


using namespace std;
using namespace Aleph;


unsigned long randomLong()
{
  return (long) (100.0*rand()/RAND_MAX);
}


fstream * file_ptr;


# include <tpl_treap.H>


void print_key(TreapTreeVtl<int>::Node *node)
{
  *file_ptr << node->get_key() << " ";
}

void print_prio(TreapTreeVtl<int>::Node *node)
{
  *file_ptr << node->getPriority() << " ";
}


fstream file[12];


int main(int argn, char *argc[])
{
  int n = 15;
  unsigned int t = time(0);
  int value;
  int val_del = 0;

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    t = atoi(argc[2]);

  if (argn > 3)
    val_del = atoi(argc[3]);

  srand(t);

  cout << "treapObs " << n << " " << t << endl;

  TreapTreeVtl<int> tree( randomLong );
  TreapTreeVtl<int>::Node *node;
  int i;

  cout << "Inserting " << n << " random values in treee ...\n";

  for (i = 0; i < n; i++)
    {
      value = 1+(int) (n*5.0*rand()/(RAND_MAX+1.0));
      node = tree.search(value);
      if (node == nullptr)
	{
          node = new TreapTreeVtl<int>::Node (value);
          tree.insert(node);
        }
    }

  file[0].open("treap-00.treap", ios::out);
  file[1].open("treap-01.treap", ios::out);
  file[2].open("treap-02.treap", ios::out);
  file[3].open("treap-03.treap", ios::out);
  file[4].open("treap-04.treap", ios::out);
  file[5].open("treap-05.treap", ios::out);
  file[6].open("treap-06.treap", ios::out);
  file[7].open("treap-07.treap", ios::out);
  file[8].open("treap-08.treap", ios::out);
  file[9].open("treap-09.treap", ios::out);
  file[10].open("treap-10.treap", ios::out);
  file[11].open("treap-11.treap", ios::out);

  file_ptr = &file[0];
  *file_ptr << "-b ";
  preOrderRec(tree.getRoot(), print_key);
  *file_ptr << "-D ";
  preOrderRec(tree.getRoot(), print_prio);
  file_ptr->close();

  cout << endl << "verifying TreapTreeVtl after insertions ... " 
       << endl;
  assert(is_treap(tree.getRoot()));
  cout << " done" << endl;

  tree.remove(val_del);

  cout << "The path length is " << compute_path_length(tree.getRoot())
       << endl;

  destroyRec(tree.getRoot()); 

  cout << "treapObs " << n << " " << t << endl;
}
