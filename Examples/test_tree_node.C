
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
# include <string>
# include <stdexcept>
# include <tpl_tree_node.H>
# include <tpl_dynArray.H>
# include <ah-errors.H>

using namespace std;

void print_node(Tree_Node<int> * node, int level, int index)
{
  cout << node->get_data() << " level = " << level << " index = " 
       << index << endl;
}


bool is_string_an_int(const string& str)
{
  string::size_type len = str.size();

  for (string::size_type i = 0; i < len; i++)
    if (not isdigit(str[i]))
      return false;

  return true;
}


int * string_to_deway(const string& str, size_t & size) 
{
  DynArray<int> a;

  const string delimiters(".");
  string::size_type begin_index, end_index, number_str_len;

  begin_index = str.find_first_not_of(delimiters);
  while (begin_index not_eq string::npos)
    {
      end_index = str.find_first_of(delimiters, begin_index);

      number_str_len = end_index - begin_index;

      const string number_str = 
	str.substr(begin_index, number_str_len).c_str();

      if (not is_string_an_int(number_str))
	ah_invalid_argument_if(true) << "character is not a digit";
      
      a[a.size()] = atoi(number_str.c_str());

      begin_index = str.find_first_not_of(delimiters, end_index);
    }

  int * ret_val = new int [a.size() + 1];

  for (int i = 0; i < a.size(); i++)
    ret_val[i] = a[i];
  ret_val[a.size()] = -1;

  size = a.size();

  return ret_val;
}


int main()
{
  int number;
  cout << "Ingrese el valor de la raiz: ";
  cin >> number;

  Tree_Node<int> * root = new Tree_Node<int> (number);

  cout << "Ingreso de nodos del arbol (deway luego numero) (letra termina)" 
       << endl;

  while (true)
    {
      try
	{
	  cout << "    Numero de Deway del padre = ";
	  string deway_string;
	  size_t deway_size;
	  cin >> deway_string;
	  int * deway = string_to_deway(deway_string, deway_size);

	  Tree_Node<int> * node = deway_search(root, deway, deway_size);

	  if (node == NULL)
	    {
	      cout << "No existe el nodo " << deway_string << endl;
	      continue;
	    }

	  cout << "    Insercion de hijo derecho en " << deway_string 
	       << " - " << node->get_data() << endl
	       << "    Clave del nuevo nodo = ";

	  int key;
	  cin >> key;
	  Tree_Node<int> * p = new Tree_Node<int> (key);
	  node->insert_rightmost_child(p);

	  //	  assert(check_tree(root));

	  delete [] deway;
	  
	  cout << endl;
	}
      catch (invalid_argument & e)
	{
	  cout << e.what() << endl
	       << "Terminando " << endl;
	  break;
	}      
      catch (bad_alloc)
	{
	  cout << "No hay memoria";
	}
    }

  cout << "    preorder" << endl;
  tree_preorder_traversal(root, &print_node);

  cout << "    postorder" << endl;
  tree_postorder_traversal(root, &print_node);

  auto aux = clone_tree(root);

  assert(are_tree_equal(root, aux));

  destroy_tree(root);
  destroy_tree(aux);
}
