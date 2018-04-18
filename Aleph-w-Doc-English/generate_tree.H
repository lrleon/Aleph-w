
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file generate_tree.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef GENERATE_TREE_H
# define GENERATE_TREE_H

# include <fstream>
# include <tpl_tree_node.H>
# include <tpl_binNodeUtils.H>

namespace Aleph
{

  template <class Node>
  struct Dft_Write
  {
    const string operator () (Node * p) 
    {
      return to_str(p->get_key());
    }
  };

/**
  @briefWrite must export Write const string :: operator () (Node *)
  Which is responsible for converting the key value contained in the node
  a printable string
*/

    template <typename Node, class Write> static
void __generate_tree(Node *          node, 
		     int             deway [], 
		     const size_t &  current_level, 
		     const size_t &  size,
		     std::ostream & output)
{ 
  if (current_level >= size)
    throw std::overflow_error("Allocated size for deway array has been "
			      "exceeded");

    /* Print number of parent Deway */
  output << "Node ";
  for (int i = 0; i < current_level; ++i)
    {
      output << deway[i];

      if (i < current_level - 1)
	output << ".";
    }
  output << " \"" << Write () (node) << "\" " << std::endl;

  Node * child = (Node*) node->get_left_child(); 
  for (int i = 0; child != NULL; i++, child = (Node*)child->get_right_sibling())
    {
      deway[current_level + 1] = i;
      __generate_tree<Node, Write>
	(child, deway, current_level + 1, size, output);
    }
}


const size_t Max_Tree_Node_Depth = 1024;

/** 
  @brief generates an entry for the program drawn ntrepic trees.

  @details generate_tree (root out, tree_number) Generates a specification
  drawn tree with root root for the program drawn
  ntreepic. The output is written to the file associated with out.

  To write the contents of the node, the routine uses the class
  parameterized Write, Write whose operator () (node) transforms
  key contained in the node to the string you want to be written
  as node content.

  @param [in] root Tree root drawing.
  @param [out] out Associated file where to write the specification drawn.
  @param [in] tree_number For internal use \b NO \b USE.

  @see generate_forest ()

  @ingroup Trees
*/
  template <typename Node, class Write = Dft_Write<Node>>
void generate_tree(Node *         root, 
		   std::ostream & out, 
		   const int &    tree_number = 0)
{                               
  out << "Root \"" << Write () (root) << "\" " << std::endl;

  int * deway = new int [Max_Tree_Node_Depth];

  const int level = 0; /* Este es el nivel de partida */

  deway[level] = tree_number; /* Nivel 0 en el número del árbol */

  Node * child = (Node*) root->get_left_child(); 
  for (int i = 0; child != NULL; i++, child = (Node*)child->get_right_sibling())
    {
      deway[1] = i;
      __generate_tree<Node, Write>(child, deway, level + 1, 
				   Max_Tree_Node_Depth, out);
    }

  delete [] deway;
}

/** 
  @brief generates an entry for the program drawn ntreepic trees.

  @details generate_forest (root out, tree_number) Generates a specification
  drawn down tree whose first tree is root for
  ntreepic program drawn. The output is written in the
  file associated with out.

  To write the contents of the node, the routine uses the class
  parameterized Write, Write whose operator () (node) transforms
  key contained in the node to the string you want to be written
  as node content.

  @param [in] root Tree root drawing.
  @param [out] out Associated file where to write the specification drawn.

  @see generate_tree ()

  @ingroup Trees
*/
    template <typename Node, class Write = Dft_Write<Node>>
void generate_forest(Node * root, std::ostream & out)
{                               
  Node * tree = root; 

  for (int i = 0; tree != NULL; i++, tree = tree->get_right_sibling())
    generate_tree<Node, Write>(tree, out, i);
}


/** 
  @brief generates an entry for the program drawn tree
  btrepic binaries.

  @details To write the contents of the node, the routine uses the class
  parameterized Write, Write whose operator () (node) transforms
  key contained in the node to the string you want to be written
  as node content.

  @param [in] root Tree root drawing.
  @param [out] out Associated file where to write the specification drawn.

  @see generate_tree ()

  @ingroup Trees
*/
    template <typename Node, class Write>
void generate_btree(Node * root, std::ostream & out)
{ 
  out << "start-prefix ";
  For_Each_Preorder<Node, Write> () (root, Write());
  out << endl
      << "start_infix ";
  For_Each_In_Order<Node, Write> () (root, Write());
  out << endl;
}

} /* End namespace Aleph */

# endif // GENERATE_TREE_H
