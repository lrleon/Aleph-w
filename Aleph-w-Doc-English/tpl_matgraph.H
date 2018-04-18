
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
  @file tpl_matgraph.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_MAT_GRAPH_H
# define TPL_MAT_GRAPH_H

# include <tpl_graph.H>
# include <tpl_sort_utils.H>

using namespace Aleph;

namespace Aleph 
{

      template <typename GT> inline static 
typename GT::Node * get_node(DynArray<typename GT::Node *> & nodes, 
			     const long &                    i)
{
  if (i >= nodes.size())
    throw std::out_of_range("Index of node out of range");

  return nodes.access(i);
} 

      template <typename GT> inline static long 
node_index(DynArray<typename GT::Node *> & nodes, 
	   const long & n, typename GT::Node * p) 
{
  return Aleph::binary_search<typename GT::Node*>(nodes, p, 0, n - 1);
}

      inline static long 
index_array(const long & i, const long & j, const long & n) 
{
  if (i >= n or j >= n)
    throw std::out_of_range("Matrix index out of range");

  return i + j*n;
}

      template <typename Entry>
inline static Entry * read_matrix(const DynArray<Entry> & mat,
				  const long & i, const long & j, 
				  const long & n)
{
  const long index = index_array(i, j, n);
  if (not mat.exist(index)) 
    return NULL;

  return &const_cast<Entry&>(mat.access(index));
}

      template <typename Entry>
inline static void write_matrix(DynArray<Entry> & mat,
				const long &      i, 
				const long &      j, 
				const long &      n, 
				const Entry &     entry)
{
  mat[index_array(i, j, n)] = entry;
}

/** 
  @brief Adjacency matrix of a graph mapped to a graph
  represented by a variant of GT.
            
  @details The class Map_Matrix_Graph modeled an adjacency matrix
  mapping to the arcs of a graph represented with lists
  adjacency. She is the simplest way to
  build an adjacency matrix in Aleph. It is a
  matrix whose entries are pointers to arcs of a graph
  represented by a class based on GT. A
  NULL entry indicates no arc.

  To construct an object of type
  Map_Matrix_Graph is required to have an instance of graph
  derivative-based GT object.

  Access to the matrix is ​​performed by the operator (i, j).
  If mat is an object of Map_Matrix_Graph type, then the
  mat (i, j) refers to the arc between the node i and j. A value of NULL
  indicates no arc; otherwise mat (i, j) is a
  GT :: Arc pointer type arc in the representation
  adjacency lists.

  The inputs to the matrix may also refer by
  node pointers. In this regard mat (src, tgt) refers to
  arc between targeted by src and tgt nodes.

  Arguably, the memory usage of an object
  Map_Matrix_Graph try to be as efficient as possible. For the
  Generally, the memory consumption is proportional to the number of
  arches. Internally, this efficiency is implanted through
  Using a dynamic array dynArray.

  It is not possible to modify an input of a matrix
  Map_Matrix_Graph. There is no control on the matrix and
  List_graph relation to the associated object. In other
  Thus, topological changes (inserting and deleting nodes and
  arches) in the GT object will not be reflected in the
  matrix. However, the ability to access nodes and arcs in
  representation with lists allows without any problems change
  any of its attributes.

  Map_Matrix_Graph is not intended to multigraphs or
  multidigrafos.

  @param GT TPO graph derived from a class based on
  GT.

  @see Matrix_Graph Ady_Mat Bit_Mat_Graph DynArray
  
  @ingroup Grafos
*/
        template <class GT, class SA = Dft_Show_Arc<GT> >
class Map_Matrix_Graph
{

public:

/** @brief The Graph_List type associated with the matrix. */
  typedef GT Graph_Type;

/** @brief The type attribute node guarding the nodes in the GT object */
  typedef typename GT::Node_Type Node_Type;

/** @brief The type attribute arc guarding the nodes in the GT object */
  typedef typename GT::Arc_Type Arc_Type;

/** @brief The type of node that manages the object GT */
  typedef typename GT::Node Node;

/** @brief The type of arc that manages the object GT */
  typedef typename GT::Arc Arc;

private:

  DynArray<Node*> nodes;
  GT * lgraph;
  mutable size_t num_nodes;
  SA & sa;

  struct Mat_Entry
  {
    Arc * arc;
    Mat_Entry(Arc * __arc = NULL) : arc(__arc) { /* Empty */ }
  };

  DynArray<Mat_Entry> mat;

public:

/** @brief Copy the graph g matrix represented with adjacency lists;
the above values in the array are deleted. */
  inline void copy_list_graph(GT & g);

/** @brief Builder from a graph represented with adjacency lists. */
  Map_Matrix_Graph(GT & g, SA && __sa = SA())
    : lgraph(&g), num_nodes(g.get_num_nodes()), sa(__sa)
  {
    copy_list_graph(g); 
  }

  Map_Matrix_Graph(GT & g, SA & __sa)
    : lgraph(&g), num_nodes(g.get_num_nodes()), sa(__sa)
  {
    copy_list_graph(g); 
  }

/** @brief Copy constructor. */
  Map_Matrix_Graph(Map_Matrix_Graph & mat)
  {
    copy_list_graph(*(mat.lgraph));
  }

/** @brief Assigning adjacency matrix */
  inline Map_Matrix_Graph & operator = (Map_Matrix_Graph & mat);

/** 
  @brief Assignin graph represented with adjacency lists.

  @details Clean matrix (all memory is released) and build a
  new adjacency matrix based on the graph represented
  g adjacency lists.

  @param[in] g Graph represented ready to be assigned.

  @throw bad_alloc If there is insufficient memory by the matrix.
*/
  inline Map_Matrix_Graph & operator = (GT & g);

/** 
  @brief returns the pointer to the node in the representation with lists
  corresponding to the index i in the matrix.

  @details The operator (i) with i integer returns a pointer to the node of the
  graph representation lists associated with the matrix
  index i.

  @param[in] i Array index.

  @return UA pointer to the node in the graph representation lists
  associated with the "i" index matrix. 

  @throw out_of_range If "i" is greater or equal to the number of nodes.
*/
  inline Node * operator () (const long & i);

/** 
  @brief Returns the index into the "node" node in the representation
  with lists.

  @details operator (node) with a pointer node to node, returns the index
  within the adjacency matrix.

  No checks on the correctness of the pointer are made.

  @param [in] Pointer to node node representation with lists
  adjacency.

  @return UA pointer to the node in the graph representation lists
  associated with the "i" index matrix. 

  @throw out_of_range If "i" is greater or equal to the number of nodes.
*/
  
  inline long operator () (Node * node) const;

/** 

  @brief returns a pointer to an arc source and destination node src_node
  tgt_node.

  @details The operator (src_node, tgt_node) reads input matrix
  src_node corresponding to the source node and destination node tgt_node
  and returns a pointer to the arc. If there is no arc, then
  NULL is returned.

  No verification is done on the correctness of
  pointers.

  This mode of access is logarithmic. For that reason, it is preferable
  first investigate the indices of the nodes and then use the
  same operator (i, j) with the indexes of the nodes.

  @param[in] src_node Pointer to the source node.
  @param[in] tgt_node Pointer to the destination node.

  @return A pointer to the arch if it exists; NULL otherwise.
*/
  inline Arc * operator () (Node * src_node, Node * tgt_node) const;
  
/** 
  @brief Returns a pointer to the arc contained in the entry (i, j)
  the adjacency matrix.

  @details operator (i, j) reads input from the corresponding matrix
  and returns a pointer to the arc. If there is no arc, then
  NULL is returned.

  This mode of access is constant.

  @param[in] i Index of source node.
  @param[in] j Index of destination node.

  @throw out_of_range If "i" or "j" is greater or equal to the amount of nodes
  graph.

  @return A pointer to the arch if it exists; NULL otherwise.
*/
  inline Arc * operator () (const long & i, const long & j) const;  

/** @brief Returns a reference to the graph represented with linked lists. */
  GT & get_list_graph() { return *lgraph; }

/** @brief Returns the number of nodes having the graph (equivalent to
the dimension of the matrix. */
  const size_t & get_num_nodes() const { return num_nodes; }
};


    template <class GT, class SA> typename GT::Node * 
Map_Matrix_Graph<GT, SA>::operator () (const long & i) 
{
  return get_node <GT> (nodes, i); 
}

      template <class GT, class SA> long 
Map_Matrix_Graph<GT, SA>::operator () (typename GT::Node * node) const
{
  return node_index <GT> (nodes, num_nodes, node);
}

      template <class GT, class SA> typename GT::Arc * 
Map_Matrix_Graph<GT, SA>::operator () (const long & i, const long & j) const
{ 
  Mat_Entry * mat_entry = read_matrix<Mat_Entry>(mat, i, j, num_nodes); 
  if (mat_entry == NULL) 
    return NULL;

  return mat_entry->arc;
} 

      template <class GT, class SA> typename GT::Arc * 
Map_Matrix_Graph<GT, SA>::operator () (Node * src_node, Node * tgt_node) const
{ 
  Mat_Entry * mat_entry = 
    read_matrix<Mat_Entry>(mat, node_index<GT>(nodes, num_nodes, src_node), 
			   node_index<GT>(nodes, num_nodes, tgt_node), 
			   num_nodes); 
  if (mat_entry == NULL)
    return NULL;

  return mat_entry->arc;
} 

    template <class GT, class SA> Map_Matrix_Graph<GT, SA> & 
Map_Matrix_Graph<GT, SA>::operator = (Map_Matrix_Graph & mat)
{
  if (this == &mat)
    return *this;

  copy_list_graph(*(mat.lgraph));
  return *this;
}

      template <class GT, class SA>
Map_Matrix_Graph<GT, SA> & Map_Matrix_Graph<GT, SA>::operator = (GT & g)
{
  copy_list_graph(g);
}

      template <class GT, class SA>
void Map_Matrix_Graph<GT, SA>::copy_list_graph(GT & g)
{
    /* Copy attributes */
  lgraph    = &g;
  num_nodes = g.get_num_nodes();
  nodes.cut(); /* Clean hot fixes from previous content */
  mat.cut();
  int i = 0; 
  for (typename GT::Node_Iterator it(g); it.has_curr(); it.next(), ++i)
    nodes[i] = it.get_current_node();

  quicksort(nodes); 

  /** Sort "nodes" to support binary search */
  for (typename GT::Node_Iterator nit(g); nit.has_current(); nit.next())
    {
      Node * src = nit.get_current_node();
      const long src_idx = node_index<GT>(nodes, num_nodes, src);

    /** For each arc of "src" write in the matrix "mat" */
      for (Node_Arc_Iterator<GT, SA> ait(src, sa); ait.has_curr(); ait.next())
        {
          Arc * arc = ait.get_current_arc();
          Node * tgt = g.get_connected_node(arc, src);
          const long tgt_idx = node_index<GT>(nodes, num_nodes, tgt);
          write_matrix<Mat_Entry>(mat, src_idx, tgt_idx, num_nodes, arc);
        }
    }
}
 
/** 
  @brief adjacency matrix of a graph taken from a graph
  represented with adjacency lists.

  @details The Matrix_Graph classd modeled adjacency matrix
  stores the attributes in the nodes and arcs of a
  graph previously was represented with adjacency lists
  (Derivation of the GT class).

  A Matrix_Graph object is a copy of a graph represented with
  adjacency lists or a Map_Matrix_Graph object (which is
  associated with adjacency lists representation).
  Unlike Map_Matrix_Graph, Matrix_Graph is modifiable in
  that it lets you add new arcs in the graph by
  modification of a null array entry.

  When an object is constructed Matrix_Graph, specify
  a constant value that represents the null input; that is, to
  the absence of arc.

  It is expected that memory consumption is proportional arches
  its quantity.

  note Since the matrix holds the attributes associated with
  nodes and arcs, Matrix_Graph not used to store attributes
  Brothers derivatives Graph_Arc Graph_Node or classes.

  @see Map_Matrix_Graph Ady_Mat

  @ingroup Grafos
*/
      template <typename GT, class SA = Dft_Show_Arc<GT> >
class Matrix_Graph
{
public:

/** @brief The type of graph represented with adjacency lists. */
  typedef GT Graph_Type;

/** @brief The attribute type that keep the nodes of the graph
represented with adjacency lists */
  typedef typename GT::Node_Type Node_Type;

/** @brief The attribute type that keep the arches of the graph
represented with adjacency lists */
  typedef typename GT::Arc_Type Arc_Type;

/** @brief The Graph_Node type that used in the GT. */
  typedef typename GT::Node Node;

/** @brief The Graph_Arc type that used in the GT. */
  typedef typename GT::Arc Arc;

private:

  DynArray<Node_Type> nodes;
  DynArray<Arc_Type>  arcs;
  mutable size_t      n;
  mutable Arc_Type    Null_Value;
  SA &                sa;
  
  void copy(Matrix_Graph & mat)
  {
    if (this == &mat)
      return;

    n          = mat.n; /* Copy attributes */
    Null_Value = mat.Null_value;
    nodes.cut(); /* Clean memory of dynamic arrays */
    arcs.cut();
    arcs.set_default_initial_value(Null_Value);

    /* Travel for mat[i,j] and copy to nodes[], arcs[] */
    for (int i = 0; i < n; ++i) 
      {
	nodes[i] = mat.nodes[i];
	for (int j = 0; j < n; ++j)
	  {
	    const long mat_index = index_array(i, j, n);
	    if (not arcs.exist(mat_index))
	      continue;
	    arcs.touch(mat_index) = mat.arcs.access(mat_index);
	  }
      }
  }

  void copy(GT & g)
  {
    n = g.get_num_nodes();
    DynArray<typename GT::Node *> ptr; /* Temporal array*/

    int i = 0;
    for (typename GT::Node_Iterator it(g); it.has_current(); it.next(), ++i)
      ptr[i] = it.get_current_node();

    quicksort(ptr); /* Sort by if it relates to Map_Matrix_Graph */

    arcs.set_default_initial_value(Null_Value); 

    /* Place contents of ptr[] of nodes[] */
    for (i = 0; i < n; ++i) 
      {
	typename GT::Node * src = ptr[i];
	nodes[i] = src->get_info();
	for (Node_Arc_Iterator<GT, SA> it(src); it.has_current(); it.next())
	  {
	    typename GT::Arc * arc = it.get_current_arc();
	    typename GT::Node * tgt = it.get_tgt_node();
	    const long j = node_index<GT>(ptr, n, tgt);
	    const long mat_index = index_array(i, j, n);
	    arcs[mat_index] = arc->get_info();
	  }
      }
  }

public:

/** @brief Returns the number of nodes in the graph
(the dimension of the matrix) */
  const size_t & get_num_nodes() const { return n; }

/** @brief The constant value representing the null entry in the matrix. */
  const Arc_Type & null_value() const { return Null_Value; }

/** 
  @brief Builder from a g graph represented with adjacency lists.

  @details This constructor receives a graph based on a value g GT
  Arc type GT :: :: corresponding to the value that will be Arc_Type
  considered null entry. P for each node of the graph, type
  GT :: Node, its internal attribute copy obtained by
  p-> get_info (). Similarly, it is for the arches.

  @param [in] g Graph represented with adjacency lists from
  which the array is copied.
  @param [in] null value that it will be considered null input
  the matrix and the absence indicating arc.
  @param [in] __sa Filter of arches.

  @throw bad_alloc If there is insufficient memory para construir la
  matriz. 
*/
  Matrix_Graph(GT & g, const Arc_Type & null, SA && __sa = SA()) 
    : Null_Value(null), sa(__sa)
  {
    copy(g);
  }
  
  Matrix_Graph(GT & g, const Arc_Type & null, SA & __sa) 
    : Null_Value(null), sa(__sa)
  {
    copy(g);
  }
  
/** 
  @brief Builder copy.

  @details Construye una matriz a partir de otra matriz.

  @param[in] mat Matrix to be copied.
  
  @throw bad_alloc If there is insufficient memory para construir la
  matriz. 
*/
  Matrix_Graph(Matrix_Graph & mat) 
  {
    copy(mat);
  }
  
/** 
  @brief Assign an array another array.

  @details Deletes all from "this" statewide and then copy the graph represented
  with the adjacency matrix mat. 

  @param[in] mat Matrix to be copied.

  @throw bad_alloc If there is insufficient memory para construir la
  matriz. 

  @note GT graph types used to build and this
  mat must match exactly; otherwise it will happen one
  compilation error.
*/
  Matrix_Graph & operator = (Matrix_Graph & mat) 
  {
    copy(mat);
    return *this;
  }

/** 
  @brief Assign an array a graph represented with adjacency lists.

  details Deletes this statewide and then copy the graph represented
  g with adjacency lists.

  @param[in] g Grafo represented with adjacency lists to be
  copied.

  @throw bad_alloc If there is insufficient memory para construir la
  matriz. 

  @note The GT type must exactly match the type
  GT associated with the matrix; otherwise it will happen one
  compilation error.
*/
  Matrix_Graph & operator = (GT & g) 
  {
    copy(g);
    return *this;
  }

/** 
  @brief Returns a reference to content attribute arch at the entrance
  (i, j) of the adjacency matrix.

  @details operator (i, j) reads input from the corresponding matrix
  returns the attribute of the arc. If there is no arc, then
  returns the value Matrix_Graph :: null_value ().

  This mode of access is constant.

  @param[in] i Index of source node.
  @param[in] j Index of destination node.

  @throw out_of_range If "i" or "j" is greater or equal to the amount of nodes
  graph.

  @return A reference to the attribute of the arc; remember that
  Matrix_Graph::null_value() indicates no arc.
*/
  const Arc_Type & operator () (const long & i, const long & j) const
  {
    const long mat_index = index_array(i, j, n);

    /** Is there entry?
    If the answer is NO, then there arc */
    if (not arcs.exist(mat_index)) 
      return Null_Value;

    return arcs.access(mat_index);      
  }

/** 
  @brief Returns the attribute associated with the node whose index is i.

  @details The operator (i) with i integer returns the attribute value associated
  the node with index i.

  @param[in] i Array index.

  @return Una referencia al atributo del nodo. 

  @throw out_of_range If "i" is greater or equal to the number of nodes.
*/
  Node_Type & operator () (const long & i) const
  {
    if (i >= n)
      throw std::out_of_range("node index out of range");

    return const_cast<Node_Type &>(nodes.access(i));
  }

/** 
  @brief returns a reference to the attribute contained in the arc
  entry (i, j) of the adjacency matrix.

  @details operator (i, j) reads input matrix
  and returns the corresponding attribute of the arc. If there
  bow, then the value Matrix_Graph :: null_value () returns.

  This mode of access is constant.

  @param[in] i Index of source node.
  @param[in] j Index of destination node.

  @throw out_of_range If "i" or "j" is greater or equal to the amount of nodes
  graph.

  @return A reference to the attribute of the arc; remember that
  Matrix_Graph::null_value() indicates no arc.
*/
  Arc_Type & operator () (const long & i, const long & j) 
  {
    const long mat_index = index_array(i, j, n);

    return arcs.touch(mat_index);
  }
};

/** 
  @brief auxiliary adjacency matrix.

  @details Many applications of graph-based arrays
  They use temporary adjacency information, type information
  different attributes stored in gafo or part of the
  attributes. In these three situations the use of Type
  Matriz_Graph or severely restricted or impossible.

  To overcome the above situation, the type defines a Ady_Mat
  matrix associated to a graph represented with lists
  adjacency but whose entries are of a type defined by the
  user.

  As with other types of adjacency matrix,
  Ady_Mat handles the null element Ady_Mat :: null_value (), but
  this case does not necessarily represent presence or absence of
  arc. Still, entries whose value is Ady_Mat :: null_value ()
  tend not occupy memory.

  Ady_Mat<GT,__Entry_Type> manages two type parameters:
  -# GT: type graph represented with adjacency lists.
  -# __Entry_Type: Type of data to be stored in the array

  @see Matrix_Graph Map_Matrix_Graph

  @ingroup Grafos
*/
  template <class GT, typename __Entry_Type, class SA = Dft_Show_Arc<GT> >
class Ady_Mat
{
public:

/** @brief The type of graph represented with adjacency lists. */
  typedef GT Graph_Type;

/** @brief The data type that houses the matrix. */
  typedef __Entry_Type Entry_Type;

/** @brief The type of data stored in the graph nodes represented
adjacency lists. */ 
  typedef typename GT::Node_Type Node_Type;

/** @brief The type of data stored in the graph arcs represented
adjacency lists. */
  typedef typename GT::Arc_Type Arc_Type;

/** @brief The node type used in the adjacency list representation. */
  typedef typename GT::Node Node;

/** @brief The type of bow used in the adjacency list representation. */
  typedef typename GT::Arc Arc;

private:

  GT *                 lgraph;
  DynArray<Node*>      nodes;
  DynArray<Entry_Type> mat;
  mutable size_t       num_nodes;
  mutable Entry_Type   Null_Value;

  void test_same_graph(Ady_Mat & mat)
  {
    if (lgraph == mat.lgraph) 
      return;

    throw 
      std::domain_error("mat does not refers the same GT than this");
  }

  void copy_nodes(GT & g)
  {
    lgraph     = &g;
    num_nodes  = g.get_num_nodes();
    nodes.cut(); /* Clean memory nodes */

    /* Place node pointers in the array nodes[] */
    int i = 0;
    for (typename GT::Node_Iterator it(g); it.has_current(); it.next(), ++i)
      nodes[i] = it.get_current_node();

    quicksort(nodes); /* Orders and then do binary search */
  }

  Ady_Mat & copy(Ady_Mat & mat)
  {
    if (this == &mat) 
      return *this;

    test_same_graph(mat);
    Null_Value = mat.Null_Value;
    copy(mat.lgraph);
    
    return *this;
  }

public:

/** @brief Returns the pointer to the node in the representation with lists
of adjacency of the index "i" in the auxiliary matrix. */
  Node * operator () (const long & i) 
  {
    return Aleph::get_node<GT>(nodes, i);
  }

/** @brief Returns the index within the auxiliary matrix of node within in
the representation by adjacency lists. */
  long operator () (Node * node) const
  {
    return node_index<GT>(nodes, num_nodes, node);
  }

/** 
  @brief Returns a reference to the entry (i,j) of the matrix of 
  auxiliary adjacency.

  @details The operator (i, j) reads the entry of the auxiliary matrix
  corresponding and returns his entry.

  This mode of access is constant.

  @param[in] i Index of source node.
  @param[in] j Index of destination node.

  @throw out_of_range If "i" or "j" is greater or equal to the amount of nodes
  graph.

  @return A reference to the entry (i,j).
*/
  Entry_Type & operator () (const long & i, const long & j)
  {
    return mat.touch(index_array(i, j, num_nodes));
  }

/** 
  @brief Returns a constant reference to the entry (i,j) of the matrix of 
  auxiliary adjacency.

  @details The operator (i, j) reads the entry of the auxiliary matrix
  corresponding and returns his entry.

  This mode of access is constant.

  @param[in] i Index of source node.
  @param[in] j Index of destination node.

  @throw out_of_range If "i" or "j" is greater or equal to the amount of nodes
  graph.

  @return A constant reference to the entry (i,j).
*/
  const Entry_Type & operator () (const long & i, const long & j) const
  {
    const long index = index_array(i, j, num_nodes);

    if (mat.exist(index)) 
      return mat.access(index);
        
    return Null_Value;
  }

/** 
  @brief Returns a reference to an entry in the matrix
  Auxiliary adjacency given pointers to their origin nodes and
  destination within the adjacency list representation.

  @details The operator (src, tgt) reads input from the auxiliary matrix
  corresponding to the pointers to the nodes src and tgt in
  adjacency lists representation and returns its input.

  This mode of access is logarithmic. Therefore, it is advisable
  use rates nodes.

  @param [in] src Pointer to the source node in the representation
  adjacency lists.
  @param [in] Pointer to the destination node tgt in the representation
  adjacency lists.

  @return A constant reference to the corresponding input.
*/
  Entry_Type & operator () (Node * src, Node * tgt) 
  {
    return (*this)(node_index <GT> (nodes, num_nodes, src),
		   node_index <GT> (nodes, num_nodes, tgt));
  }

/** 
  @brief Returns a constant reference to an entry in the matrix
  Auxiliary adjacency given pointers to their origin nodes and
  destination within the adjacency list representation.

  @details The operator (src, tgt) reads input from the auxiliary matrix
  corresponding to the pointers to the nodes src and tgt in
  adjacency lists representation and returns its input.

  This mode of access is logarithmic. Therefore, it is advisable
  use rates nodes.

  @param [in] src Pointer to the source node in the representation
  adjacency lists.
  @param [in] Pointer to the destination node tgt in the representation
  adjacency lists.

  @return A constant reference to the corresponding input.
*/
  const Entry_Type & operator () (Node * src, Node * tgt) const
  {
    return (*this)(node_index <GT> (nodes, num_nodes, src),
		   node_index <GT> (nodes, num_nodes, tgt));
  }

/** @brief Retorna una referencia al grafo representado con lista de adyacencia. */
  GT & get_list_graph() { return *lgraph; }

/** @brief Returns the value considered void. */
  const Entry_Type & null_value() const { return Null_Value; }

/** @brief Returns the number of nodes in the graph
(ie, the dimension of the matrix). */
  const size_t & get_num_nodes() const { return num_nodes; }

/** 
  @brief Auxiliary builder matrix from a graph shown with adjacency lists.

  @details This constructor takes graph represented ag
  with adjacency lists and build an auxiliary matrix whose
  Tickets are not initialized.

  If you want to save memory explicitly by an element
  designated as a null value, then this value must be indicated
  with early Ady_Mat :: set_null_value ().

  @param [in] g Graph represented with adjacency lists.

  @throw bad_alloc If there is insufficient memory.

  @see Ady_Mat::set_null_value() Ady_Mat::null_value()
*/
  Ady_Mat(GT & g) : lgraph(&g), num_nodes(lgraph->get_num_nodes()) 
  {
    copy_nodes(g); 
  }

/** 
  @brief Auxiliary builder matrix from a graph shown with adjacency lists 
  and definition of null.

  @details This constructor takes graph represented ag lists
  adjacency, null and build an auxiliary matrix whose
  Tickets are not initialized.

  Access the entry (i, j) return the value Ady_Mat :: null_value ().

  @param [in] g Graph represented with adjacency lists.
  @param [in] null null value of the designated item.

  @throw bad_alloc If there is insufficient memory.

  @see Ady_Mat::set_null_value() Ady_Mat::null_value()
*/
  Ady_Mat(GT & g, const Entry_Type & null) 
    : lgraph(&g), num_nodes(lgraph->get_num_nodes()), Null_Value(null) 
  {
    copy_nodes(*lgraph); 
  }
  
/** @brief Declares null for auxiliary adjacency matrix. */
  void set_null_value(const Entry_Type & null) { Null_Value = null; }
  
/** @brief Copy constructor. */
  Ady_Mat(Ady_Mat & mat) 
  {
    copy(mat);
  }

/** 
  @brief Performs an operation on the auxiliary matrix according to each arc
  the graph represented with adjacency lists.

  @details operate_all_arcs_list_graph () traverses the graph arcs
  represented with adjacency lists and invokes the operation
  Operation () (mat, arc, i, j, entry, ptr) where:
  - # Mat: Reference to the matrix on which the operation is being performed.
  - # Arc: arc pointer inside the GT associated with the matrix.
  - # I: Index of the source node in the array.
  - # J: index of the destination in the parent node.
  - # Entry: reference to the content of the entry (i, j) in the matrix.

  Using this method is mainly for writing
  initial values within the matrix depending on the contents of
  arcs in the graph represented with adjacency lists.

  @note Note that the operation runs only for entries
  refer to an arc. The rest of the entries contains the value
  Ady_Mat :: null_value () and probably will not take up memory.
*/
  template <class Operation> void operate_all_arcs_list_graph();

/** 
  @brief Performs an operation on the auxiliary matrix according to each arc
  the graph represented with adjacency lists and transmitted
  operation an opaque pointer where to send and receive
  information.

  @details operate_all_arcs_list_graph () traverses the graph arcs
  represented with adjacency lists and invokes the operation
  Operation () (mat, arc, i, j, entry, ptr) where:
  - # Mat: Reference to the matrix on which the operation is being performed.
  - # Arc: arc pointer inside the GT associated with the matrix.
  - # I: Index of the source node in the array.
  - # J: index of the destination in the parent node.
  - # Entry: reference to the content of the entry (i, j) in the matrix.
  - # Ptr: Pointer opaque.

  Using this method is mainly for writing
  initial values within the matrix depending on the contents of
  arcs in the graph represented with adjacency lists.

  @note Note that the operation runs only for entries
  refer to an arc. The rest of the entries contains the value
  Ady_Mat :: null_value () and probably will not take up memory.

  @param[in] ptr Opaque pointer information you want
  transmit operation
*/
  template <class Operation> void operate_all_arcs_list_graph(void * ptr);
  
/** 
  @brief Performs an operation on all inputs auxiliary matrix.

  @details operate_all_arcs_matrix() recorre todas las entradas de matriz
  auxiliar e invoca una operación Operation()(mat,src,tgt,s,t,entry)
  cuyos parámetros se definen así:
  -# mat: Reference to the matrix
  -# src: Pointer to the source node.
  -# tgt: Pointer to the destination node.
  -# s: Index of source node.
  -# t: Index of destination node.
  -# entry: Reference to the matrix entry.

  If this method is invoked, then the matrix occupies the maximum
  memory, as each entry must be written so that it
  ensure a valid reference to mat (s,t).

  @note Note that there is no shortcut to arc in the representation with
  adjacency lists.
*/
  template <class Operation> void operate_all_arcs_matrix();

/** 
  @brief Performs an operation on all inputs auxiliary matrix
  with an opaque pointer where to send or receive information.

  @details operate_all_arcs_matrix() recorre todas las entradas de matriz
  auxiliar e invoca una operación Operation()(mat,src,tgt,s,t,entry)
  cuyos parámetros se definen así:
  -# mat: Reference to the matrix
  -# src: Pointer to the source node.
  -# tgt: Pointer to the destination node.
  -# s: Index of source node.
  -# t: Index of destination node.
  -# entry: Reference to the matrix entry.
  -# ptr: Opaque pointer passed to the operation.

  If this method is invoked, then the matrix occupies the maximum
  memory, as each entry must be written so that it
  ensure a valid reference to mat (s, t).

  Note that there is no shortcut to arc in the representation with
  adjacency lists.

  @param[in] ptr Opaque pointer information you want
  transmit operation
*/
      template <class Operation> 
  void operate_all_arcs_matrix(void * ptr); 
}; 


      template <class GT, typename __Entry_Type, class SA>
      template <class Operation>
void Ady_Mat<GT, __Entry_Type, SA>::operate_all_arcs_list_graph()
{      
    /** Browse all nodes in the graph */
  for (typename GT::Node_Iterator nit(*lgraph); nit.has_current(); nit.next())
    {
      Node * src = nit.get_current_node();
      const long src_idx = node_index<Graph_Type>(nodes, num_nodes, src);

    /** Browse all arcs source node */
      for (Node_Arc_Iterator<GT, SA> at(src); at.has_current(); at.next())
	{
	  Arc * arc = at.get_current_arc();
	  const long tgt_idx = 
	    node_index<Graph_Type>(nodes, num_nodes, arc->get_tgt_node());
	  Entry_Type & entry = 
	    mat.touch(index_array(src_idx, tgt_idx, num_nodes));

	  Operation () (*this, arc, src_idx, tgt_idx, entry);
	}
    }
}

      template <class GT, typename __Entry_Type, class SA>
      template <class Operation>
void Ady_Mat<GT, __Entry_Type, SA>::operate_all_arcs_matrix()
{
  const long & n = num_nodes;
  for (int s = 0; s < n; ++s)
    {
      Node * src_node = get_node<GT>(nodes, s);
      for (int t = 0; t < n; ++t)
	{
	  Node * tgt_node = get_node<GT>(nodes, t);
	  Entry_Type & entry = mat.touch(index_array(s, t, num_nodes));

	  Operation () (*this, src_node, tgt_node, s, t, entry);
	}
    }
}

      template <class GT, typename __Entry_Type, class SA>
      template <class Operation>
void Ady_Mat<GT, __Entry_Type, SA>::operate_all_arcs_list_graph(void * ptr)
{      
    /** Browse all nodes in the graph */
  for (typename GT::Node_Iterator nit(*lgraph); nit.has_current(); nit.next())
    {
      Node * src = nit.get_current_node();
      const long src_idx = node_index<Graph_Type>(nodes, num_nodes, src);

    /** Browse all arcs source node */
      for (Node_Arc_Iterator<GT, SA> at(src); at.has_current(); at.next())
	{
	  Arc * arc = at.get_current_arc();
	  Node * tgt = lgraph->get_tgt_node(arc);
	  const long tgt_idx = node_index<Graph_Type>(nodes, num_nodes, tgt);
	  Entry_Type & entry = /* Ensure access to sea (src_idx, tgt_idx) */
	    mat.touch(index_array(src_idx, tgt_idx, num_nodes));

	  Operation () (*this, arc, src_idx, tgt_idx, entry, ptr);
	}
    }
}

      template <class GT, typename __Entry_Type, class SA>
      template <class Operation>
void Ady_Mat<GT, __Entry_Type, SA>::operate_all_arcs_matrix(void * ptr)
{
  const long & n = num_nodes;

  for (int s = 0; s < n; ++s)
    {
      Node * src_node = get_node<GT>(nodes, s);

      for (int t = 0; t < n; ++t)
	{
	  Node * tgt_node = get_node<GT>(nodes, t);
	  Entry_Type & entry = mat.touch(index_array(s, t, num_nodes));
	  Operation () (*this, src_node, tgt_node, s, t, entry, ptr);
	}
    }
}

/** 
  @brief bit adjacency matrix of a graph.

  @details Bit_Mat_Graph The type defines the simplest matrix
  adjacency on a graph. As the types of matrices
  Aleph adjacency, Bit_Mat_Graph can associate a
  graph represented with adjacency lists based on
  GT, but unlike the above matrices, this
  association is not mandatory.

  This is an array of bits implemented by type
  BitArray. An entry (i, j) with value 1 indicates presence of
  arc, while the value 0 indicates absence.

  This kind of array takes all possible memory. Namely,
  the dimension of the matrix divided by 8.

  @see BitArray warshall_compute_transitive_clausure()

  @ingroup Grafos
*/
      template <class GT, class SA = Dft_Show_Arc<GT> >
class Bit_Mat_Graph
{

public:

/** @brief Graph type in GT */
  typedef GT Graph_Type;

/** @brief Node type in GT */
  typedef typename GT::Node Node;

/** @brief Arc type in GT */
  typedef typename GT::Arc Arc;

private:

  BitArray bit_array;
  GT *                         lgraph;
  DynArray<typename GT::Node*> nodes;
  mutable size_t               n;
  
  void copy_list_graph(GT & g)
  {
    n = g.get_num_nodes();
    nodes.cut(); /* Free memory */

    
    int i = 0;

    /** Copy all g nodes in the array nodes[] */
    for (typename GT::Node_Iterator it(g); it.has_current(); it.next())
      nodes[i++] = static_cast<typename GT::Node*>(it.get_current_node());

    quicksort(nodes); /* Sort and then do binary search */

    /** Browse all nodes of g to assign arcs in bit_array */
    for (typename GT::Node_Iterator it(g); it.has_current(); it.next())
      {
	typename GT::Node * src = it.get_current_node();
	const size_t src_idx = node_index<Graph_Type>(nodes, n, src);
          
    /** Browse arches src array to assign input */
	for (Node_Arc_Iterator<GT, SA> jt(src); jt.has_current(); jt.next())
	  { 
	    typename GT::Node * tgt = jt.get_tgt_node();
	    const size_t tgt_idx = node_index<Graph_Type>(nodes, n, tgt);
	    bit_array[index_array(src_idx, tgt_idx, n)] = 1;
	  }
      }
  }

  struct Proxy
  {
    BitArray & bit_array;

    const size_t bit_index;

    Proxy(Bit_Mat_Graph & __bitmat, const long & i, const long & j)
      : bit_array(__bitmat.bit_array), bit_index(index_array(i, j, __bitmat.n))
    {
      /* Empty */
    }

    Proxy& operator = (const Proxy & proxy)
    {
      bit_array[bit_index] = proxy.bit_array[proxy.bit_index];
    }
      
    Proxy& operator = (const int & i)
    {
      bit_array[bit_index] = i;

      return *this;
    }

    operator int () const
    {
      return bit_array[bit_index];
    }
  };

public: 

/** @brief Retorna el número de nodos del grafo (dimensión de la matriz). */
  const size_t & get_num_nodes() const { return n; }

/** @brief Empty constructor. */
  Bit_Mat_Graph() : lgraph(NULL) {}

/** @brief Builder from a g graph represented with adjacency lists. */
  Bit_Mat_Graph(GT & g) 
    : bit_array(g.get_num_nodes()*g.get_num_nodes()), lgraph(&g)
  { 
    copy_list_graph(g); 
  }
  
/** @brief Copy constructor. */
  Bit_Mat_Graph(const Bit_Mat_Graph & bitmat)
    : bit_array(bitmat.bit_array), lgraph(bitmat.lgraph), 
      nodes(bitmat.nodes), n(bitmat.n) 
  {
    /* Empty */
  }

/** @brief Constructor specifying a dimension. */
  Bit_Mat_Graph(const size_t & dim) 
    : bit_array(dim*dim), lgraph(NULL), nodes(dim), n(dim) 
  {
    /* Empty */
  }

/** @brief Bit array associated to the graph g represented with adjacency lists. */
  void set_list_graph(GT & g)
  {
    const size_t & n = g.get_num_nodes();
    bit_array.set_size(n*n);
    lgraph = &g;
    copy_list_graph(g);
  }

/** @brief Returns a pointer to the graph represented with lists
adjacency; if the value is NULL, then no associated graph. */
  GT * get_list_graph() { return lgraph; }

/** @brief Matrix assignation */
  Bit_Mat_Graph & operator = (const Bit_Mat_Graph & bitmat)
  {
    if (this == &bitmat) 
      return *this;

    lgraph    = bitmat.lgraph;
    bit_array = bitmat.bit_array;

    return *this;
  }

/** @brief Allocation graph represented with adjacency lists. */
  Bit_Mat_Graph & operator = (GT & g)
  {
    if (&g == lgraph) 
      return *this;

    copy_list_graph(g);

    return *this;
  }
  
/** @brief Returns the pointer to the node in the representation lists
corresponding adjacency i index */
  Node * operator () (const long & i) 
  {
    if (lgraph == NULL)
      throw std::domain_error("There is no a GT object");

    return Aleph::get_node<GT>(nodes, i);
  }

/** @brief Returns the index into the node array in the representation
adjacency lists. */
  long operator () (Node * node) const
  {
    if (lgraph == NULL)
      throw std::domain_error("There is no a GT object");

    return node_index<GT>(nodes, n, node);
  }

  Proxy operator () (const long & i, const long & j)
  {
    return Proxy(*this, i, j);
  }

  Proxy operator () (const long & i, const long & j) const
  {
    return Proxy(const_cast<Bit_Mat_Graph&>(*this), i, j);
  }

  Proxy operator () (Node * src_node, Node * tgt_node) 
  {
    if (lgraph == NULL)
      throw std::domain_error("There is no a GT object");

    return Proxy(*this, node_index<GT>(nodes, n, src_node), 
		 node_index<GT>(nodes, n, tgt_node)); 
  }

  Proxy operator () (Node * src_node, Node * tgt_node) const
  {
    if (lgraph == NULL)
      throw std::domain_error("There is no a GT object");

    return Proxy(*this, node_index<GT>(nodes, n, src_node), 
		 node_index<GT>(nodes, n, tgt_node)); 
  }
};

} /* End namespace Aleph */

# endif // TPL_MAT_GRAPH_H
