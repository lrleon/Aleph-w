
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

