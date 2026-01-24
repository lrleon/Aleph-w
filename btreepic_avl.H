
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file btreepic_avl.H
 *  @brief AVL tree visualization utilities.
 *
 *  Provides preorder traversal and utilities for generating
 *  visual representations of AVL trees.
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# include <tpl_avl.H>


    template <class Node>
struct Preorder_Nodes
{
  ofstream & output;

  Preorder_Nodes(ofstream & out) 
     : output(out)
  {
    output << "start-prefix ";
  }

  ~Preorder_Nodes()
  {
    output << std::endl
	   << std::endl;
  }

  void operator () (Node * p)
  {
    output << (long) p << " ";
  }
};

    template <class Node>
struct Inorder_Nodes
{
  ofstream & output;

  Inorder_Nodes(ofstream & out) : output(out) 
  {
    output << "start-infix ";
  }

  ~Inorder_Nodes()
  {
    output << std::endl
	   << std::endl;
  }

  void operator () (Node * p)
  {
    output << (long) p << " ";
  }
};

    template <class Node, class Key>
struct Inorder_Keys
{
  ofstream & output;

  Inorder_Keys(ofstream & out) : output(out) 
  {
    output << "start-key ";
  }

  ~Inorder_Keys()
  {
    output << std::endl
	   << std::endl;
  }

  void operator () (Node * p)
  {
    output << Key () (KEY(p)) << " ";
  }
};

    template <class Node>
struct Inorder_Diff
{
  ofstream & output;

  Inorder_Diff(ofstream & out) : output(out) 
  {
    output << "start-aux ";
  }

  ~Inorder_Diff()
  {
    output << std::endl
	   << std::endl;
  }

  void operator () (Node * p)
  {
    output << (int) DIFF(p) << " ";
  }
};



    template <class Node, class Key_To_Text>
void avl_to_btreepic(Node * root, ofstream & output)
{
  {
    Preorder_Nodes<Node> preorder (output);
    For_Each_Preorder<Node> () (root, preorder);
  }

  {
    Inorder_Nodes<Node> inorder (output);

    For_Each_In_Order<Node> () (root, inorder);
  }

  {
    Inorder_Diff<Node> diffs (output);
    For_Each_In_Order<Node> () (root, diffs);
  }

  {
    Inorder_Keys<Node, Key_To_Text> inorder (output);
    For_Each_In_Order<Node> () (root, inorder);
  }
}

