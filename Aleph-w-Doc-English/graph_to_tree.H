
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
  @file graph_to_tree.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef GRAPH_TO_TREE_H
# define GRAPH_TO_TREE_H

# include <tpl_tree_node.H>
# include <tpl_graph.H>
# include <tpl_graph_utils.H>

namespace Aleph {

    template <class GT, typename Key, class Convert> static
Tree_Node<Key> * graph_to_tree_node(GT & g, typename GT::Node * groot);
    template <class GT, typename Key, class Convert> static void 
__graph_to_tree_node(GT & g, typename GT::Node * groot, 
                     Tree_Node<Key> * troot);

  template <class GT, typename Key, typename Convert, class SA> static inline
void __graph_to_tree_node(GT & g, typename GT::Node * groot,
                          Tree_Node<Key> * troot);

/** 
  @brief Converts a tree represented in a graph representing a
  Tree_Node tree.

  @details graph_to_tree_node () takes a tree represented by a
  type of graph represented with adjacency lists and a node
  groot root of the tree that is assumed and converts it to a tree
  Tree_Node represented type <Key>.

  The interest of this routine is to get a representation of
  tree in which other techniques can be applied, kinds and
  algorithms. A representative example is the drawing of trees
  inherent in the graphs; the occurrence, spanning tree.

  The procedure uses the bit to mark the nodes spanning_tree
  and arches already visited.

  As a means of representation, the type Tree_Node <Key> is less
  versatile than any kind based on a graph represented
  adjacency lists; eg List_graph. Note, for
  instance, that List_Graph <Node, Arc> provides a type for
  arches, while not true for Tree_Node <Key>. In
  Here, as representation, Tree_Node <Key> no
  any means for storing data associated with the arches.
  Consequently, it could only handle data
  contained in the nodes. Moreover, the type stored in
  GT :: Node can be of various kinds to saved Tree_Node <Key>.

  Often (that's drawn the typical case), the data type
  to save Tree_Node <Key> is of a different nature to save
  GT :: Node. Other times, you want to save each
  Tree_Node <Key> is a subset of what is saved in
  GT :: Node. Since it is impossible generizar all cases
  possible, writing each Tree_Node <Key> is delegated to a
  class whose operator () () is called by the routine
  conversion for each node in the graph. The class in question is the

  La clase Convert se invoca de la siguiente manera:  

  void Convert::operator () (gnode, tnode)

  Where:
    -# gnode es de tipo GT::Node
    -# tnode de tipo Tree_Node<Key>

  The routine test done on g by acyclicity
  is_graph_acyclique () function, which does not affect the complexity
  Algorithmic but it takes extra time.

  The method handles the following parameterized types:
    - # GT: graph type, which must be derived from List_Graph.
    - # Key: Data type in your tree house
   Tree_Node representation <Key>.
   - # Convert: Conversion Class GT :: Node Key, whose
   operator () (gnode, tnode) is invoked on each node of g when
   the conversion is done to one Tree_Node.
   - # SA: Filter arches.

  @param [in] g GT tree type (derived from List_Graph) that
  you want to convert.
  @param [in] g Groot node is considered tree root.
  
  @return The root of a tree type Tree_Node <Key>
  corresponding to the conversion
        
  @throw bad_alloc If no memory to divide the tree
  Tree_Node type <Key>
        
  @ingroup Graphs
      
  @see Tree_Node is_graph_acyclique ()
*/
    template <class GT, typename Key, 
              class Convert, class SA = Dft_Show_Arc<GT> > inline
Tree_Node<Key> * graph_to_tree_node(GT & g, typename GT::Node * groot)
{ 
  if (not is_graph_acyclique<GT>(g))
    throw std::domain_error("Graph is not a tree (not acyclique)");

  Tree_Node<Key> * troot = new Tree_Node<Key>; /* Apartar memoria raíz */

  Convert () (groot, troot); /* Convertir de groot y copiar a troot */

  __graph_to_tree_node <GT, Key, Convert, SA> (g, groot, troot);

  return troot;
}

/** 
  @brief Converts a tree represented in a graph representing a
  Tree_Node tree.

  @details graph_to_tree_node () takes a tree represented by a
  type of graph represented with adjacency lists and a node
  groot root of the tree that is assumed and converts it to a tree
  Tree_Node represented type <Key>.

  The interest of this routine is to get a representation of
  tree in which other techniques can be applied, kinds and
  algorithms. A representative example is the drawing of trees
  inherent in the graphs; in the occurrence, trees
  encompassing.

  The procedure uses the bit to mark the nodes spanning_tree
  and arches already visited.

  As a means of representation, the type Tree_Node <Key> is less
  versatile than any kind based on a graph represented
  adjacency lists; eg List_graph. Note, for
  instance, that List_Graph <Node, Arc> provides a type for
  arches, while not true for Tree_Node <Key>. In
  Here, as representation, Tree_Node <Key> no
  any means for storing data associated with the
  arches. Consequently, it could only handle data
  contained in the nodes. Moreover, the type stored in
  GT :: Node can be of various kinds to saved Tree_Node <Key>.

  Often (that's drawn the typical case), the data type
  to save Tree_Node <Key> is of a different nature to save
  GT :: Node. Other times, you want to save each
  Tree_Node <Key> is a subset of what is saved in
  GT :: Node. Since it is impossible generizar all cases
  possible, writing each Tree_Node <Key> is delegated to a
  class whose operator () () is called by the routine
  conversion for each node in the graph. The class in question is the
  Convert parameter type.

  The Convert class is invoked as follows:
  
        void Convert :: operator () (gnode, tnode)

       where:
        - # Gnode is of type GT :: Node
        - # TNODE type Tree_Node <Key>

  The routine test done on g by acyclicity
  is_graph_acyclique () function, which does not affect the complexity
  Algorithmic but it takes extra time.

  The method handles the following parameterized types:
   - # GT: graph type, which must be derived from List_Graph.
   - # Key: Data type in your tree house
  Tree_Node representation <Key>.
   - # Convert: Conversion Class GT :: Node Key, whose
   operator () (gnode, tnode) is invoked on each node of g when
   the conversion is done to one Tree_Node.
   - # SA: Filter arches.

  @ingroup Graphs

  @see Tree_Node is_graph_acyclique ()
*/
    template <class GT, typename Key, 
	      class Convert, 
	      class SA = Dft_Show_Arc<GT> > 
class Graph_To_Tree_Node
{
  SA &      sa;
  Convert * convert;

  void graph_to_tree(typename GT::Node * groot,
			  Tree_Node<Key> * troot)
  {           
    typedef typename GT::Node Node;
    typedef typename GT::Arc Arc;

    /** Browse groot arches and build recursively*/
    for (Node_Arc_Iterator<GT, SA> it(groot, sa); it.has_curr(); it.next())
      {
	Arc * arc = it.get_current_arc();
	if (IS_ARC_VISITED(arc, Convert_Tree)) 
	  continue;

	ARC_BITS(arc).set_bit(Convert_Tree, true); /* visited arc*/
	Node * gtgt = it.get_tgt_node();
	Tree_Node<Key> * ttgt = new Tree_Node<Key>; 

	(*convert) (gtgt, ttgt); /* Assign the key */

	troot->insert_rightmost_child(ttgt); /* Insert it as child */

	graph_to_tree(gtgt, ttgt);
      }
  }

  Tree_Node<Key> * graph_to_tree(GT & g,
				 typename GT::Node * groot,
				 Convert & conv)
  { 
    if (not is_graph_acyclique<GT>(g))
      throw std::domain_error("Graph is not a tree (not acyclique)");

    Tree_Node<Key> * troot = new Tree_Node<Key>; /* Following aside memory */

    convert = &conv;

    (*convert) (groot, troot); /* Groot convert and copy Troot */ 

    graph_to_tree(groot, troot);

    return troot;
  }

public:

  Graph_To_Tree_Node(SA && __sa = SA()) : sa(__sa) { /* Empty */ }

  Graph_To_Tree_Node(SA & __sa) : sa(__sa) { /* Empty */ }

/** 
  @brief Summons conversion to spanning tree graph.

  @param [in] g GT Tree type (derived from List_Graph) that
  you want to convert.
  @param [in] g Groot node is considered tree root.
  @param [in] conv Type conversion.
   
  @return The root Of a tree type Tree_Node <Key>
  corresponding to the conversion
  
  @throw bad_alloc If no memory to divide the tree
  Tree_Node type <Key>   
*/
      Tree_Node<Key> * 
  operator () (GT & g, typename GT::Node * groot, Convert && conv = Convert())
  { 
    return graph_to_tree(g, groot, conv);
  }

      Tree_Node<Key> * 
  operator () (GT & g, typename GT::Node * groot, Convert & conv)
  { 
    return graph_to_tree(g, groot, conv);
  }
};


  template <class GT, typename Key, typename Convert, class SA> static inline
void __graph_to_tree_node(GT & g, typename GT::Node * groot,
                          Tree_Node<Key> * troot)
{           
  typedef typename GT::Node Node;
  typedef typename GT::Arc Arc;

    /** Browse groot arches and build recursively */
  for (Node_Arc_Iterator<GT, SA> it(groot); it.has_current(); it.next())
    {
      Arc * arc = it.get_current_arc();
      if (IS_ARC_VISITED(arc, Convert_Tree)) 
        continue;

      ARC_BITS(arc).set_bit(Convert_Tree, true); /* visited arc */
      Node * gtgt = it.get_tgt_node();
      Tree_Node<Key> * ttgt = new Tree_Node<Key>; 

      Convert () (gtgt, ttgt); /* Assing the key */

      troot->insert_rightmost_child(ttgt); /* Insert it as child  */

      __graph_to_tree_node <GT, Key, Convert, SA> (g, gtgt, ttgt);
    }
}

} /* End namespace Aleph */

# endif // GRAPH_TO_TREE_H
