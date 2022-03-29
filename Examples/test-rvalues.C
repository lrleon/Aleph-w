

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
] (int n) -> Tree
    {
      Tree t;
      for (int i = 0; i < n; ++i)
	t.insert(i, i+1);
      return t;
    };

  
  Tree tree;
  for (int i = 0; i < n; ++i)
    tree.insert(i, i);

  Tree t1 = tree;
  
  Tree t2 = (*create_tree)(n);

  t2 = (*create_tree)(2*n);

  print(t2) ;

  t1 = t2;
  
  print(t1);

  cout << endl;

  cout << "Probando diferentes combinaciones de insert\n"
       << endl
       << "L val L val\n";

  Tree tt;
  int i = n + 1, j = n + 2;
  tt.insert(i, j);
    
  cout << "\n\nL val R val\n";
  i++;
  tt.insert(i, j + 1);

  cout << "\n\nR val L val\n";
  tt.insert(i + 3, j);

  cout << "\n\nR val R val\n";
  tt.insert(i + 6, j + 7);  

  cout << endl << endl;
  (*print)(tt); cout << endl;
}

int main(int, char * argc[])
{
  if (argc[1])
    V = atoi(argc[1]);

  test_map_tree <DynMapBinTree<int,int>> (V);

  //return 0;

  cout << "Testing DynList" << endl;
  test_list <DynList<>> ();
  cout << endl; exit(0);
  
  cout << "Testing List_Graph" << endl;
  test <List_Graph<>> ();
  cout << endl;

  cout << "Testing List_Digraph" << endl;
  test <List_Digraph<>> ();
  cout << endl;

  cout << "Testing List_SGraph" << endl;
  test <List_SGraph<>> ();
  cout << endl;

  cout << "Testing List_SDigraph" << endl;
  test <List_SDigraph<>> ();
  cout << endl;

  cout << "Testing Array_Graph" << endl;
  test <Array_Graph<>> ();
  cout << endl;

  cout << "Testing Array_Digraph" << endl;
  test <Array_Digraph<>> ();
  cout << endl;

}
