
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
# include <memory>
# include <string>
# include <stdexcept>
# include <ah-errors.H>
# include <tpl_tree_node.H>
# include <tpl_dynArray.H>

# include <cassert>
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
    if (not std::isdigit(static_cast<unsigned char>(str[i])))
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

      ah_invalid_argument_if(not is_string_an_int(number_str))
        << "character is not a digit";
      
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
  cout << "Enter the root value: ";
  if (not (cin >> number))
    {
      cout << "Error: Invalid input for root value." << endl;
      return 1;
    }

  Tree_Node<int> * root = new Tree_Node<int> (number);

  cout << "Entering tree nodes (deway then number) (any letter to finish)" 
       << endl;

  while (true)
    {
      try
	{
	  cout << "    Parent Deway number = ";
	  string deway_string;
	  size_t deway_size;
	  if (not (cin >> deway_string))
            break;

	  unique_ptr<int[]> deway(string_to_deway(deway_string, deway_size));

	  Tree_Node<int> * node = deway_search(root, deway.get(), deway_size);

	  if (node == NULL)
	    {
	      cout << "Node " << deway_string << " does not exist" << endl;
	      continue;
	    }

	  cout << "    Inserting rightmost child in " << deway_string 
	       << " - " << node->get_data() << endl
	       << "    New node key = ";

	  int key;
	  if (not (cin >> key))
            break;

	  Tree_Node<int> * p = new Tree_Node<int> (key);
	  node->insert_rightmost_child(p);

	  //	  assert(check_tree(root));

	  cout << endl;
	}
      catch (invalid_argument & e)
	{
	  cout << e.what() << endl
	       << "Finishing " << endl;
	  break;
	}      
      catch (bad_alloc)
	{
	  cout << "Out of memory";
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
