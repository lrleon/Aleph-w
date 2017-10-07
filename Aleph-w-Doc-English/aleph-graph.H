
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
  @file aleph-graph.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef ALEPH_GRAPH_H
# define ALEPH_GRAPH_H

# include <cassert>
# include <stdexcept>
# include <memory>

namespace Aleph {

static const long No_Visited = 0; /* Node or arc it has not been visited */

/** 
  @enum Magic numbers for the bits of control 
*/
enum Graph_Bits 
{
  Depth_First,   /** Bit of deep searching */ // (Change to Search)
  Breadth_First, /** Bit of breadth searching  */ // (Change to Search)
  Test_Cycle,    /** Bit of verification de cycle */
  Find_Path,     /** Bit of way searching */               
  Euler,         /** Bit of euleriano way */
  Maximum_Flow,  /** Bit of maximum flow */
  Spanning_Tree, /** Bit of comprehensive tree */
  Build_Subtree, /** Bit of subgraphs construction */
  Convert_Tree,  /** Bit of conversion Tree_Node */
  Cut,           /** Bit of cutting indication (node o arc) */
  Min,           /** Bit of minium arc indication */
  Num_Bits_Graph /** It provides maximum allowed bit */
};

  extern const unsigned int Processed;
  extern const unsigned int Processing;
  extern const unsigned int Unprocessed;

/** 
  @brief bitmask to mark parts of a graph.

  @details Each node and arc of a graph contains a bitmask
  intended to visit the property during the execution of algorithms
  specific.

  @note For some reason not yet clearly understood , copying
  bits with memcpy and other family system does not work
  properly on some systems. Possibly this only
  occur on little- endian systems. Therefore , do not use these
  mechanisms.

  @ingroup Grafos
*/
class Bit_Fields
{
public:

  unsigned int depth_first    : 1; /** Bit of deep searhing*/ 
  unsigned int breadth_first  : 1; /** Bit of breadth searching */
  unsigned int test_cycle     : 1; /** Bit of verification cycle*/
  unsigned int find_path      : 1; /** Bit of way searching */
  unsigned int euler          : 1; /** Bit of euleriano way*/
  unsigned int maximum_flow   : 1; /** Bit for maximum flow */
  unsigned int spanning_tree  : 1; /** Bit of comprehensive tree */
  unsigned int build_subtree  : 1; /** Bit of subgraph */
  unsigned int convert_tree   : 1; /** Conversion to Tree_Node */
  unsigned int cut            : 1; /** Node or arc of cut */
  unsigned int min            : 1; /** It belongs to a shortest path */
  unsigned int state          : 2; /** Visit status */

/** 
  @brief Default constructor that starts all the bits to zero 
*/
  Bit_Fields() 
    : depth_first(0), breadth_first(0), test_cycle(0), find_path(0), euler(0), 
      maximum_flow(0), spanning_tree(0), build_subtree(0), convert_tree(0), 
      cut(0), min(0) {}

/** 
  @brief Gets a bit value of control.

  @details Given control bits of a node or arc ,
  This method gets a specific value of a bit.

  @param [in] bit number bit to be read.
  
  @return A logical value corresponding to the read bit.
  
  @throw out_of_range if bit is higher than the number of bits
  defined. 
*/
  bool get_bit(const int & bit) const 
  {
    switch (bit)
      {
      case Aleph::Depth_First:   return depth_first;
      case Aleph::Breadth_First: return breadth_first;
      case Aleph::Test_Cycle:    return test_cycle;
      case Aleph::Find_Path:     return find_path;
      case Aleph::Euler:         return euler;
      case Aleph::Maximum_Flow:  return maximum_flow;
      case Aleph::Spanning_Tree: return spanning_tree;
      case Aleph::Build_Subtree: return build_subtree;
      case Aleph::Convert_Tree:  return convert_tree;
      case Aleph::Cut:           return cut;
      case Aleph::Min:           return min;
      default:
        throw std::out_of_range("bit number out of range");
      }
  }

/** 
  @brief Write a value in a control bit.

  @details Given control bits of a node or arc this method writes
  a value ( 0 or 1) in a specific bit.

  It does not verify that the value is 0 or 1.

  @param [in] bit Number bit to write.
  @param [in] value Value to write.
  
  @throw out_of_range If bit is higher than the number of bits defined.
*/
  void set_bit(const int & bit, const int & value) 
  {
    assert(value == 0 or value == 1);

    switch (bit)
      {
      case Aleph::Depth_First:   depth_first   = value; break;
      case Aleph::Breadth_First: breadth_first = value; break;
      case Aleph::Test_Cycle:    test_cycle    = value; break;
      case Aleph::Find_Path:     find_path     = value; break;
      case Aleph::Euler:         euler         = value; break;
      case Aleph::Maximum_Flow:  maximum_flow  = value; break;
      case Aleph::Spanning_Tree: spanning_tree = value; break;
      case Aleph::Build_Subtree: build_subtree = value; break;
      case Aleph::Convert_Tree:  convert_tree  = value; break;
      case Aleph::Cut:           cut           = value; break;
      case Aleph::Min:           min           = value; break;
      default:
	throw std::out_of_range("bit number out of range");
      }
  }

/** 
  @brief Reset bit to zero. 
*/
  void reset(const int & bit) { set_bit(bit, 0); }

/** 
  @brief Reset all the bits to zero. 
*/
  void reset()
  {
    depth_first   = 0; 
    breadth_first = 0; 
    test_cycle    = 0; 
    find_path     = 0; 
    euler         = 0; 
    maximum_flow  = 0; 
    spanning_tree = 0; 
    build_subtree = 0; 
    convert_tree  = 0;
    cut           = 0;
    min           = 0;
    state         = 0;
  }
};


struct Graph_Attr
{
  Bit_Fields control_bits; /** Bits of the arc control */
  long       counter;
  void *     cookie; 
  
  Graph_Attr() : counter(No_Visited), cookie(NULL) { }
};


# define GRAPH_NODE_COMMON(GT_Node_Name)				\
  Graph_Attr attrs;							\
  typedef Node_Info Item_Type;						\
  typedef GT_Node_Name Node;						\
  typedef Node_Info Node_Type;						\
  Node_Info  node_info;							\
  Node_Info & get_info() { return node_info; }				\
  const Node_Info & get_info() const { return node_info; }		\
  size_t       num_arcs;						\
  unsigned int state() const { return NODE_BITS(this).state; }		\
  void set_state(unsigned int s) { NODE_BITS(this).state = s; }


# define GRAPH_ARC_COMMON(GT_Arc_Name)					\
  typedef Arc_Info Item_Type;						\
  typedef GT_Arc_Name Arc;						\
  typedef Arc_Info Arc_Type;						\
  Arc_Info   arc_info;							\
  Graph_Attr attrs;							\
  void *     src_node;							\
  void *     tgt_node;							\
									\
  unsigned int state() const { return ARC_BITS(this).state; }		\
  void set_state(unsigned int s) { ARC_BITS(this).state = s; }		\
									\
  Arc_Info & get_info() { return arc_info; }				\
									\
  const Arc_Info & get_info() const { return arc_info; }		\
									\
  void * get_connected_node(void * node)				\
  {									\
    return src_node == node ? tgt_node : src_node;			\
  }									\
									\
  void * get_img_node(void * node)					\
  {									\
    return src_node == node ? src_node : tgt_node;			\
  }		

/** 
  @brief Obtains the control bits of a node.

  @ingroup Grafos
*/
# define NODE_BITS(p)            ((p)->attrs.control_bits)

/** 
  @brief Obtains the counter of a node.

  @ingroup Graph
*/
# define NODE_COUNTER(p)         ((p)->attrs.counter)

/** 
  @brief Obteins the colour of a node.

  @ingroup Graph
*/
# define NODE_COLOR(p)         ((p)->attrs.counter)

/** 
  @brief Determined if the bit is in the marked node.

  @param p Pointer to node
  @param bit Number of the bit to leer
  
  @return true if the bit is 1; false if is 0

  @ingroup Graphs
 */
# define IS_NODE_VISITED(p, bit) (NODE_BITS(p).get_bit(bit))

/** 
  @brief Returns the cookie to the node.

  @ingroup Grafos
*/
# define NODE_COOKIE(p)          ((p)->attrs.cookie) 

/** 
  @brief Obtains the counter of an arc.

  @ingroup Graphs
*/
# define ARC_COUNTER(p)          ((p)->attrs.counter)

/** 
  @brief Obtains the colour de un arco.

  @ingroup Grafos
*/
# define ARC_COLOR(p)          ((p)->attrs.counter)

/** 
  @brief Obtains the bits of the arc control.

  @ingroup Grafos
*/
# define ARC_BITS(p)             ((p)->attrs.control_bits)

/** 
  @brief Determined if the bit of an arc is market.

  @param p Pointer to arc
  @param bit Number of the bit to leer
  
  @return true if the bit is 1; false if is 0
  
  @ingroup Grafos
*/
# define IS_ARC_VISITED(p, bit)  (ARC_BITS(p).get_bit(bit))

/** 
  @brief Returns the cookie to the arc.

  @ingroup Graphs
*/
# define ARC_COOKIE(p)           ((p)->attrs.cookie) 


# define GRAPH_ATTR_COMMON(Graph_Name)					\
  public:								\
  typedef Graph_Name<__Graph_Node, __Graph_Arc> Graph_Class;		\
  typedef __Graph_Node             Node;				\
  typedef __Graph_Arc              Arc;					\
  typedef typename Node::Node_Type Node_Type;				\
  typedef typename Arc::Arc_Type   Arc_Type;				\
									\
protected:								\
									\
 void * cookie;								\
									\
 struct Reset_Node							\
 {									\
   void operator () (const Graph_Name&, Node * node) const		\
   {									\
     NODE_BITS(node).reset();						\
     NODE_COUNTER(node) = 0;						\
     NODE_COOKIE(node) = NULL;						\
   }									\
 };									\
									\
 struct Reset_Arc							\
 {									\
   void operator () (const Graph_Name&, Arc * arc) const		\
   {									\
     ARC_BITS(arc).reset();						\
     ARC_COUNTER(arc) = 0;						\
     ARC_COOKIE(arc) = NULL;						\
   }									\
 };									\
									\
 size_t num_nodes;							\
 size_t num_arcs;							\
 bool digraph;								\
									\
 void init()								\
 {									\
   num_nodes = num_arcs = 0;						\
   cookie = NULL;							\
   digraph = false;							\
 }									\
									\
protected:								\
									\
 void common_swap(Graph_Name & g)					\
 {									\
   if (digraph != g.digraph)						\
     throw std::domain_error("source and target incompatible");		\
									\
   std::swap(num_nodes, g.num_nodes);					\
   std::swap(num_arcs, g.num_arcs);					\
   std::swap(digraph, g.digraph);					\
   std::swap(cookie, g.cookie);						\
 }									\
									\
public:									\
									\
 void *& get_cookie() { return cookie; }				\
									\
 void * get_cookie() const { return cookie; }				\
									\
 bool is_digraph() const { return digraph; }				\
									\
 const size_t & get_num_nodes() const { return num_nodes; }		\
									\
 const size_t & vsize() const { return get_num_nodes(); }		\
 									\
 Node * get_src_node(Arc * arc) const { return (Node*) arc->src_node; }	\
									\
 Node * get_tgt_node(Arc * arc) const { return (Node*) arc->tgt_node; }	\
									\
 Node * get_connected_node(Arc * arc, Node * node) const		\
 {									\
   return (Node*) arc->get_connected_node(node);			\
 }									\
									\
 const size_t & get_num_arcs() const  { return num_arcs; }		\
									\
 const size_t & esize() const { return get_num_arcs(); }		\
									\
 const size_t & get_num_arcs(Node * node) const				\
 {									\
   return node->num_arcs;						\
 }									\
									\
 Bit_Fields & get_control_bits(Node * node) const			\
 {									\
   return NODE_BITS(node).reset();					\
 }									\
									\
 void reset_bit(Node * node, const int & bit) const			\
 {									\
   NODE_BITS(node).reset(bit);						\
 }									\
									\
 void reset_bits(Node * node) const { NODE_BITS(node).reset(); }	\
									\
 void set_bit(Node * node, const int bit, const int value)		\
 {									\
   NODE_BITS(node).set_bit(bit, value);					\
 }									\
 									\
 Bit_Fields & get_control_bits(Arc * arc) const { return ARC_BITS(arc); } \
									\
 void reset_bit(Arc * arc, const int bit) const				\
 {									\
   ARC_BITS(arc).reset(bit);						\
 }									\
 									\
 void reset_bits(Arc * arc) const { ARC_BITS(arc).reset(); }		\
 									\
 void set_bit(Arc * arc, const int bit, const int value) const 		\
 {									\
   ARC_BITS(arc).set_bit(bit, value);					\
 }									\
									\
 void *& get_cookie(Node * node) const { return NODE_COOKIE(node); }	\
									\
 void *& get_cookie(Arc * arc) const { return ARC_COOKIE(arc); }	\
									\
 long & get_counter(Node * node) const { return NODE_COUNTER(node); }	\
									\
 void reset_counter(Node * node) const { NODE_COUNTER(node) = No_Visited; } \
									\
 void reset_node(Node * node) const					\
 {									\
   node_bits(node).reset();						\
   NODE_COUNTER(node) = 0;						\
   NODE_COOKIE(node)  = NULL;						\
 }									\
									\
 long & get_counter(Arc * arc) const { return ARC_COUNTER(arc); }	\
									\
 void reset_counter(Arc * arc) const { ARC_COUNTER(arc) = No_Visited; }	\
									\
 void reset_arc(Arc * arc) const					\
 {									\
   ARC_BITS(arc).reset();						\
   ARC_COUNTER(arc) = 0;						\
   ARC_COOKIE(arc)  = NULL;						\
 }									\
 									\
 template <class N1, class N2> static					\
 void map_nodes(N1 * p, N2 * q)						\
 {									\
   assert(p != NULL and q != NULL);						\
   									\
   if (NODE_COOKIE(p) == NULL)						\
     {									\
       NODE_COOKIE(p) = q;						\
       NODE_COOKIE(q) = p;						\
       return;								\
     }									\
   									\
   NODE_COOKIE(q) = NODE_COOKIE(p);					\
   NODE_COOKIE(p) = q;							\
 }									\
									\
 template <class A1, class A2> static					\
 void map_arcs(A1 * p, A2 * q)						\
 {									\
   assert(p != NULL and q != NULL);						\
									\
   if (ARC_COOKIE(p) == NULL)						\
     {									\
       ARC_COOKIE(p) = q;						\
       ARC_COOKIE(q) = p;						\
									\
       return;								\
     }									\
									\
   ARC_COOKIE(q) = ARC_COOKIE(p);					\
   ARC_COOKIE(p) = q;							\
 }									


# define GRAPH_ITERATIVE_METHODS					\
  void reset_bit_nodes(const int & bit)	const				\
  {									\
    for (Node_Iterator itor(*this); itor.has_curr(); itor.next())	\
      reset_bit(itor.get_current_node(), bit);				\
  }									\
									\
  void reset_bit_arcs(const int & bit) const				\
  {									\
    for (Arc_Iterator itor(*this); itor.has_curr(); itor.next())	\
      reset_bit(itor.get_current_arc(), bit);				\
  }									\
  void reset_bit_nodes() const						\
  {									\
    for (Node_Iterator itor(*this); itor.has_curr(); itor.next())	\
      reset_bits(itor.get_current_node());				\
  }									\
									\
  void reset_bit_arcs()	const						\
  {									\
    for (Arc_Iterator itor(*this); itor.has_curr(); itor.next())	\
      reset_bits(itor.get_current_arc());				\
  }									\
									\
  Node * get_node(size_t k)						\
  {									\
    int m = num_nodes/2;						\
    Node_Iterator it(*this);						\
									\
    if (k < m)								\
      for (int i = 0; i < k; ++i)					\
	it.next();							\
    else								\
      {									\
	it.reset_last();						\
	for (int i = num_nodes - 1; i > k; --i)				\
	  it.prev();							\
      }									\
									\
    return it.get_curr();						\
  }									\
									\
  Node * get_arc(size_t k)						\
  {									\
    int m = num_arcs/2;							\
    Arc_Iterator it(*this);						\
									\
    if (k < m)								\
      for (int i = 0; i < k; ++i)					\
	it.next();							\
    else								\
      {									\
	it.reset_last();						\
	for (int i = num_arcs - 1; i > k; --i)				\
	  it.prev();							\
      }									\
									\
    return it.get_curr();						\
  }									\
									\
  void reset_counter_nodes() const					\
  {									\
    for (Node_Iterator itor(*this); itor.has_curr(); itor.next())	\
      reset_counter(itor.get_current_node());				\
  }									\
									\
  void reset_counter_arcs() const					\
  {									\
    for (Arc_Iterator itor(*this); itor.has_curr(); itor.next())	\
      reset_counter(itor.get_current_arc());				\
  }									\
									\
  void reset_cookie_nodes() const					\
  {									\
    for (Node_Iterator itor(*this); itor.has_curr(); itor.next())	\
      itor.get_current_node()->cookie = NULL;				\
  }									\
									\
  void reset_cookie_arcs() const					\
  {									\
    for (Arc_Iterator itor(*this); itor.has_curr(); itor.next())	\
      itor.get_current_arc()->cookie = NULL;				\
  }									\
  inline void reset_nodes() const;					\
  									\
  inline void reset_arcs() const;					\
									\
  typedef Node_Arc_Iterator Out_Iterator;				\
									\
  struct Dual_Arc_Iterator : public Arc_Iterator			\
  {									\
    bool first_tour = true;						\
    using Base = Arc_Iterator;						\
    using Base::Base;							\
									\
    void next()								\
    {									\
      this->Base::next();						\
      if (not this->has_curr() and first_tour)				\
	{								\
	  first_tour = false;						\
	  this->reset_first();						\
	}								\
    }									\
									\
    void prev()								\
    {									\
      this->Base::prev();						\
      if (not this->has_curr() and not first_tour)			\
	{								\
	  first_tour = true;						\
	  this->reset_last();						\
	}								\
    }									\
									\
    Node * get_src_node() const						\
    {									\
      if (first_tour)							\
	return (Node*) this->get_curr()->src_node;			\
      else								\
	return (Node*) this->get_curr()->tgt_node;			\
    }									\
									\
    Node * get_tgt_node() const						\
    {									\
      if (first_tour)							\
	return (Node*) this->get_curr()->tgt_node;			\
      else								\
	return (Node*) this->get_curr()->src_node;			\
    }									\
  };



# define GRAPH_SEARCH_METHODS						\
  template <class Equal>						\
  Node * search_node(const typename Node::Node_Type & node_info)	\
  {									\
    for (Node_Iterator itor(*this); itor.has_curr(); itor.next())	\
      {									\
	Node * curr_node = itor.get_current_node();			\
	if (Equal() (curr_node->get_info(), node_info))			\
	  return curr_node;						\
      }									\
									\
    return NULL;							\
  }									\
									\
  template <typename T, class Equal>					\
  Node * search_node(const T & data)					\
  {									\
    for (Node_Iterator it(*this); it.has_curr(); it.next())		\
      {									\
	Node * p = it.get_current_node();				\
	if (Equal () (p->get_info(), data))				\
	  return p;							\
      }									\
									\
    return NULL;							\
  }									\
									\
  Node * search_node(const typename Node::Node_Type & node_info)	\
  {									\
    return search_node<Aleph::equal_to<typename Node::Node_Type>>(node_info); \
  }									\
									\
  template <class Equal>						\
  Node * search_node(void * ptr)					\
  {									\
    for (Node_Iterator itor(*this); itor.has_curr(); itor.next())	\
      {									\
	Node * curr_node = itor.get_current_node();			\
	if (Equal () (curr_node, ptr))					\
	  return curr_node;						\
      }									\
									\
    return NULL;							\
  }									\
									\
  template <typename T, class Equal>					\
  Arc * search_Arc(const T & data)					\
  {									\
    for (Arc_Iterator it(*this); it.has_curr(); it.next())		\
      {									\
	Arc * a = it.get_current_arc();					\
	if (Equal () (a->get_info(), data))				\
	  return a;							\
      }									\
									\
    return NULL;							\
  }									\
									\
  template <class Equal>						\
  Arc * search_arc(const typename Arc::Arc_Type & arc_info)		\
  {									\
    for (Arc_Iterator it(*this); it.has_curr(); it.next())		\
      {									\
	Arc * curr_arc = it.get_current_arc();				\
	if (Equal () (curr_arc->get_info(), arc_info))			\
	  return curr_arc;						\
      }									\
									\
    return NULL;							\
  }									\
									\
  Arc * search_arc(const typename Arc::Arc_Type & arc_info)		\
  {									\
    return search_arc<Aleph::equal_to<Arc_Type>>(arc_info);		\
  }									\
									\
  template <class Equal>						\
  Arc * search_arc(void * ptr)						\
  {									\
    for (Arc_Iterator itor(*this); itor.has_curr(); itor.next())	\
      {									\
	Arc * curr_arc = itor.get_current_node();			\
	if (Equal () (curr_arc, ptr))					\
	  return curr_arc;						\
      }									\
									\
    return NULL;							\
  }


# define GRAPH_INSERTION_METHODS					\
  virtual Node * insert_node(const Node_Type & node_info)		\
  {									\
    return insert_node(new Node (node_info));				\
  }									\
									\
  virtual Node * insert_node(Node_Type && node_info = Node_Type())	\
  {									\
    return insert_node(new Node(std::move(node_info)));			\
  }									\
									\
  virtual Arc *								\
  insert_arc(Node * src, Node * tgt, const Arc_Type & arc_info)		\
  {									\
    std::unique_ptr<Arc> arc(new Arc(arc_info));			\
    insert_arc(src, tgt, arc.get());					\
    return arc.release();						\
  }									\
  									\
  virtual Arc *								\
  insert_arc(Node * src, Node * tgt, Arc_Type && arc_info = Arc_Type()) \
  {									\
    std::unique_ptr<Arc> arc(new Arc(std::move(arc_info)));		\
    insert_arc(src, tgt, arc.get());					\
    return arc.release();						\
  } 


# define GRAPH_METHODS_IMPLS(Name)				\
template <typename Node, typename Arc>			\
void Name<Node, Arc>::reset_nodes() const			\
{									\
  Operate_On_Nodes<Graph_Class, Reset_Node>()(*const_cast<Graph_Class*>(this));\
}									\
								\
template <typename Node, typename Arc>				\
void Name<Node, Arc>::reset_arcs() const			\
{								\
  Operate_On_Arcs<Graph_Class, Reset_Arc>() (*const_cast<Graph_Class*>(this));\
}

# define GRAPH_FUNCTIONAL_METHODS(Name)				\
  template <class Operation>					\
  bool traverse_nodes(Operation & operation) const		\
  {								\
    for (Node_Iterator it(*const_cast<Name<Node,Arc>*>(this));	\
	 it.has_curr(); it.next())				\
      if (not operation(it.get_curr()))				\
	return false;						\
    return true;						\
  }								\
								\
  template <class Operation>					\
  bool traverse_nodes(Operation & operation)			\
  {								\
    for (Node_Iterator it(*this); it.has_curr(); it.next())	\
      if (not operation(it.get_curr()))				\
	return false;						\
    return true;						\
  }								\
								\
  template <class Operation>					\
  bool traverse_nodes(Operation && operation = Operation()) const \
  {								 \
    return const_cast<Name<Node,Arc>*>(this)->			 \
      traverse_nodes<Operation>(operation);			 \
  }								 \
  								 \
  template <class Operation>					 \
  bool traverse_nodes(Operation && operation = Operation())	 \
  {								 \
    return traverse_nodes<Operation>(operation);		\
  }								\
								\
  template <class Operation>					\
  bool traverse_arcs(Operation & operation) const		\
  {								\
    for (Arc_Iterator it(*const_cast<Name<Node,Arc>*>(this));	\
	 it.has_curr(); it.next())				\
      if (not operation(it.get_curr()))				\
	return false;						\
    return true;						\
  }								\
								\
  template <class Operation>					\
  bool traverse_arcs(Operation & operation)			\
  {								\
    for (Arc_Iterator it(*this); it.has_curr(); it.next())	\
      if (not operation(it.get_curr()))				\
	return false;						\
    return true;						\
  }								\
								\
  template <class Operation>					\
  bool traverse_arcs(Operation && operation = Operation()) const \
  {								\
    return const_cast<Name<Node,Arc>*>(this)->			\
      traverse_arcs<Operation>(operation);			\
  }								\
								\
  template <class Operation>					\
  bool traverse_arcs(Operation && operation = Operation())	\
  {								\
    return traverse_arcs<Operation>(operation);			\
  }								\
								\
  template <class Operation>					\
  void for_each_node(Operation & operation) const		\
  {								\
    for (Node_Iterator it(*const_cast<Name<Node,Arc>*>(this));	\
         it.has_curr(); it.next())				\
      operation(it.get_curr());					\
  }								\
								\
  template <class Operation>					\
  void for_each_node(Operation & operation)			\
  {								\
    for (Node_Iterator it(*this); it.has_curr(); it.next())	\
      operation(it.get_curr());					\
  }								\
								\
  template <class Operation>					\
  void for_each_node(Operation && operation = Operation()) const \
  {								\
    const_cast<Name<Node,Arc>*>(this)->				\
      for_each_node<Operation>(operation);			\
  }								\
								\
  template <class Operation>					\
  void for_each_node(Operation && operation = Operation())	\
  {								\
    for_each_node<Operation>(operation);			\
  }								\
								\
  template <class Operation>					\
  bool all_node(Operation & operation) const			\
  {								\
    for (Node_Iterator it(*const_cast<Name<Node,Arc>*>(this));	\
	 it.has_curr(); it.next())				\
      if (not operation(it.get_curr()))				\
	return false;						\
    return true;						\
  }								\
								\
  template <class Operation>					\
  bool all_node(Operation & operation)				\
  {								\
    for (Node_Iterator it(*this); it.has_curr(); it.next())	\
      if (not operation(it.get_curr()))				\
	return false;						\
    return true;						\
  }								\
  								\
  template <class Operation>					\
  bool all_node(Operation && operation = Operation()) const	\
  {								\
    return const_cast<Name<Node,Arc>*>(this)->			\
      all_node<Operation>(operation);				\
  }								\
								\
  template <class Operation>					\
  bool all_node(Operation && operation = Operation())		\
  {								\
    return all_node<Operation>(operation);			\
  }								\
								\
  template <class Operation>					\
  bool forall_node(Operation & operation) const			\
  {								\
    for (Node_Iterator it(*const_cast<Name<Node,Arc>*>(this));	\
	 it.has_curr(); it.next())				\
      if (not operation(it.get_curr()))				\
	return false;						\
    return true;						\
  }								\
								\
  template <class Operation>					\
  bool forall_node(Operation & operation)			\
  {								\
    for (Node_Iterator it(*this); it.has_curr(); it.next())	\
      if (not operation(it.get_curr()))				\
	return false;						\
    return true;						\
  }								\
  								\
  template <class Operation>					\
  bool forall_node(Operation && operation = Operation()) const	\
  {								\
    return const_cast<Name<Node,Arc>*>(this)->			\
      all_node<Operation>(operation);				\
  }								\
								\
  template <class Operation>					\
  bool forall_node(Operation && operation = Operation())	\
  {								\
    return all_node<Operation>(operation);			\
  }								\
								\
  template <typename T = Node_Type,				\
	    template <typename> class Container = DynList>	\
  Container<T> map_nodes(std::function<T(Node *)> operation)	\
  {								\
    Container<T> ret_val;					\
    this->for_each_node([&ret_val, &operation] (Node * p)	\
			{					\
			  ret_val.append(operation(p));		\
			});					\
    return ret_val;						\
  }								\
								\
  template <typename T>							\
  T foldl_nodes(const T & init, std::function<T(const T&, Node*)> operation) \
  {									\
    T ret_val = init;							\
    this->for_each_node(/* Lambda */ [&ret_val, &operation] (Node * p)	\
			{						\
			  ret_val = operation(ret_val, p);		\
			});						\
    return ret_val;							\
  }									\
									\
  template <class Operation>						\
  void for_each_arc(Operation & operation) const			\
  {									\
    for (Arc_Iterator it(*const_cast<Name<Node,Arc>*>(this));		\
	 it.has_curr(); it.next())					\
      operation(it.get_curr());						\
  }									\
									\
  template <class Operation>						\
  void for_each_arc(Operation & operation)				\
  {									\
    for (Arc_Iterator it(*this); it.has_curr(); it.next())		\
      operation(it.get_curr());						\
  }									\
									\
  template <class Operation>						\
  void for_each_arc(Operation && operation = Operation()) const		\
  {									\
    const_cast<Name<Node,Arc>*>(this)->					\
      for_each_arc<Operation>(operation);				\
  }									\
									\
  template <class Operation>						\
  void for_each_arc(Operation && operation = Operation()) 		\
  {									\
    for_each_arc<Operation>(operation);					\
  }									\
									\
  template <class Operation>						\
  bool all_arc(Operation & operation) const				\
  {									\
    for (Arc_Iterator it(*const_cast<Name<Node,Arc>*>(this));		\
	 it.has_curr(); it.next())					\
      if (not operation(it.get_curr()))					\
	return false;							\
    return true;							\
  }									\
									\
  template <class Operation>						\
  bool all_arc(Operation & operation)					\
  {									\
    for (Arc_Iterator it(*this); it.has_curr(); it.next())		\
      if (not operation(it.get_curr()))					\
	return false;							\
    return true;							\
  }									\
									\
  template <class Operation>						\
  bool all_arc(Operation && operation = Operation()) const		\
  {									\
    return const_cast<Name<Node,Arc>*>(this)->				\
      all_arc<Operation>(operation);					\
  }									\
									\
  template <class Operation>						\
  bool all_arc(Operation && operation = Operation())			\
  {									\
    return all_arc<Operation>(operation);				\
  }									\
									\
  template <class Operation>						\
  bool forall_arc(Operation & operation) const				\
  {									\
    return const_cast<Name<Node,Arc>*>(this)->				\
      all_arc<Operation>(operation);					\
  }									\
									\
  template <class Operation>						\
  bool forall_arc(Operation & operation)				\
  {									\
    return all_arc<Operation>(operation);				\
  }									\
									\
  template <class Operation>						\
  bool forall_arc(Operation && operation = Operation()) const		\
  {									\
    return const_cast<Name<Node,Arc>*>(this)->				\
      all_arc<Operation>(operation);					\
  }									\
									\
  template <class Operation>						\
  bool forall_arc(Operation && operation = Operation())			\
  {									\
    return all_arc<Operation>(operation);				\
  }									\
									\
  template <typename T = Arc_Type,					\
	    template <typename> class Container = DynList>		\
  Container<T> map_arcs(std::function<T(Arc *)> operation)		\
  {									\
    Container<T> ret_val;						\
    this->for_each_arc([&ret_val, &operation] (Arc * p)			\
		       {						\
			 ret_val.append(operation(p));			\
		       });						\
    return ret_val;							\
  }									\
									\
  template <typename T>							\
  T foldl_arcs(const T & init, std::function<T(const T&, Arc*)> operation) \
  {									\
    T ret_val = init;							\
    this->for_each_arc([&ret_val, &operation] (Arc * p)			\
		       {						\
			 ret_val = operation(ret_val, p);		\
		       });						\
    return ret_val;							\
  }									\
									\
  template <class Operation>						\
  void for_each_arc(Node * p, Operation & operation) const		\
  {									\
    for (Node_Arc_Iterator it(p); it.has_curr(); it.next())		\
      operation(it.get_curr());						\
  }									\
									\
  template <class Operation>						\
  void for_each_arc(Node * p, Operation & operation)			\
  {									\
    for (Node_Arc_Iterator it(p); it.has_curr(); it.next())		\
      operation(it.get_curr());						\
  }									\
									\
  template <class Operation>						\
  void for_each_arc(Node * p, Operation && operation = Operation()) const \
  {									\
    for_each_arc<Operation>(p, operation);				\
  }									\
									\
  template <class Operation>						\
  void for_each_arc(Node * p, Operation && operation = Operation())	\
  {									\
    for_each_arc<Operation>(p, operation);				\
  }									\
									\
  template <class Operation>						\
  bool all_arc(Node * p, Operation & operation) const			\
  {									\
    for (Node_Arc_Iterator it(p); it.has_curr(); it.next())		\
      if (not operation(it.get_curr()))					\
	return false;							\
    return true;							\
  }									\
									\
  template <class Operation>						\
  bool all_arc(Node * p, Operation & operation)				\
  {									\
    for (Node_Arc_Iterator it(p); it.has_curr(); it.next())		\
      if (not operation(it.get_curr()))					\
	return false;							\
    return true;							\
  }									\
									\
  template <class Operation>						\
  bool all_arc(Node * p, Operation && operation = Operation()) const	\
  {									\
    return all_arc<Operation>(p, operation);				\
  }									\
									\
  template <class Operation>						\
  bool all_arc(Node * p, Operation && operation = Operation())		\
  {									\
    return all_arc<Operation>(p, operation);				\
  }									\
									\
  template <class Operation>						\
  bool forall_arc(Node * p, Operation & operation) const		\
  {									\
    return all_arc(p, operation);					\
  }									\
									\
  template <class Operation>						\
  bool forall_arc(Node * p, Operation & operation)			\
  {									\
    return all_arc(p, operation);					\
  }									\
									\
  template <class Operation>						\
  bool forall_arc(Node * p, Operation && operation = Operation()) const	\
  {									\
    return all_arc(p, operation);					\
  }									\
									\
  template <class Operation>						\
  bool forall_arc(Node * p, Operation && operation = Operation())	\
  {									\
    return all_arc<Operation>(p, operation);				\
  }									\
									\
  template <typename T = Arc_Type,					\
	    template <typename> class Container = Aleph::DynList>	\
  Container<T> map_arcs(Node * p, std::function<T(Arc *)> operation)	\
  {									\
    Container<T> ret_val;						\
    this->for_each_arc(p, [&ret_val, &operation] (Arc * a)		\
		       {						\
			 ret_val.append(operation(a));			\
		       });						\
    return ret_val;							\
  }									\
									\
  template <typename T>							\
  T foldl_arcs(Node * p, const T & init,				\
	       std::function<T(const T&, Arc*)> operation)		\
  {									\
    T ret_val = init;							\
    this->for_each_arc(p, [&ret_val, &operation] (Arc * a)		\
		       {						\
			 ret_val = operation(ret_val, a);		\
		       });						\
    return ret_val;							\
  }									\
									\
  template <template <typename> class Container = Aleph::DynList>	\
  Container<Node*> filter_nodes(std::function<bool(Node*)> operation)	\
  {									\
    Container<Node*> ret_val;						\
    this->for_each_node([&ret_val, &operation] (Node * p)		\
			{						\
			  if (operation(p))				\
			    ret_val.append(p);				\
			});						\
    return ret_val;							\
  }									\
									\
  template <template <typename> class Container = Aleph::DynList>	\
  Container<Arc*> filter_arcs(std::function<bool(Arc*)> operation)	\
  {									\
    Container<Arc*> ret_val;						\
    this->for_each_arc([&ret_val, &operation] (Arc * a)			\
		       {						\
			 if (operation(a))				\
			   ret_val.append(a);				\
		       });						\
    return ret_val;							\
  }									\
									\
  template <template <typename> class Container = Aleph::DynList>	\
  Container<Arc*> filter_arcs(Node * p, std::function<bool(Arc*)> operation) \
  {									\
    Container<Arc*> ret_val;						\
    this->for_each_node(p, [&ret_val, &operation] (Arc * a)		\
			{						\
			  if (operation(a))				\
			    ret_val.append(a);				\
			});						\
    return ret_val;							\
  }									\
  									\
  template <class Operation>						\
  bool exists_node(Operation & operation) const				\
  {									\
    return this->traverse_nodes([&operation] (Node * p)			\
			  {						\
			    return not operation(p);			\
			  });						\
  }									\
									\
  template <class Operation>						\
  bool exists_node(Operation & operation)				\
  {									\
    return this->traverse_nodes([&operation] (Node * p)			\
			  {						\
			    return not operation(p);			\
			  });						\
  }									\
									\
  template <class Operation>						\
  bool exists_node(Operation && operation = Operation()) const		\
  {									\
    return exists_node(operation);					\
  }									\
									\
  template <class Operation>						\
  bool exists_node(Operation && operation = Operation())		\
  {									\
    return exists_node(operation);					\
  }									\
									\
  template <class Operation>						\
  bool exists_arc(Operation & operation) const				\
  {									\
    return this->traverse_arcs([&operation] (Arc * a)			\
			  {						\
			    return not operation(a);			\
			  });						\
  }									\
									\
  template <class Operation>						\
  bool exists_arc(Operation & operation)				\
  {									\
    return this->traverse_arcs([&operation] (Arc * a)			\
			  {						\
			    return not operation(a);			\
			  });						\
  }									\
									\
  template <class Operation>						\
  bool exists_arc(Operation && operation = Operation()) const		\
  {									\
    return exists_arc(operation);					\
  }									\
									\
  template <class Operation>						\
  bool exists_arc(Operation && operation = Operation())			\
  {									\
    return exists_arc(operation);					\
  }									\
									\
  template <class Operation>						\
  bool exists_arc(Node * p, Operation & operation) const		\
  {									\
    return this->traverse_arcs(p, [&operation] (Arc * a)		\
			       {					\
				 return not operation(a);		\
			       });					\
  }									\
									\
  template <class Operation>						\
  bool exists_arc(Node * p, Operation & operation)			\
  {									\
    return this->traverse_arcs(p, [&operation] (Arc * a)		\
			       {					\
				 return not operation(a);		\
			       });					\
  }									\
									\
  template <class Operation>						\
  bool exists_arc(Node * p, Operation && operation = Operation()) const \
  {									\
    return exists_arc(p, operation);					\
  }									\
									\
  template <class Operation>						\
  bool exists_arc(Node * p, Operation && operation = Operation())	\
  {									\
    return exists_arc(p, operation);					\
  }									\
									\
  template <template <typename> class Container = Aleph::DynList>	\
  Container<Node*> nodes()						\
  {									\
    Container<Node*> ret_val;						\
    this->for_each_node([&ret_val] (Node * p)				\
			{						\
			  ret_val.append(p);				\
			});						\
    return ret_val;							\
  }									\
									\
  template <template <typename> class Container = Aleph::DynList>	\
  Container<Arc*> arcs()						\
  {									\
    Container<Arc*> ret_val;						\
    this->for_each_arc([&ret_val] (Arc * a)				\
		       {						\
			 ret_val.append(a);				\
		       });						\
    return ret_val;							\
  }									\
									\
  template <template <typename> class Container = Aleph::DynList>	\
  Container<Arc*> arcs(Node * p)					\
  {									\
    Container<Arc*> ret_val;						\
    this->for_each_arc(p, [&ret_val] (Arc * a)				\
		       {						\
			 ret_val.append(a);				\
		       });						\
    return ret_val;							\
  }									\
									\
  template <template <typename> class Container = Aleph::DynList>	\
  Container<Node*> nodes() const					\
  {									\
    Container<Node*> ret_val;						\
    this->for_each_node([&ret_val] (Node * p)				\
			{						\
			  ret_val.append(p);				\
			});						\
    return ret_val;							\
  }									\
									\
  template <template <typename> class Container = Aleph::DynList>	\
  Container<Arc*> arcs() const						\
  {									\
    Container<Arc*> ret_val;						\
    this->for_each_arc([&ret_val] (Arc * a)				\
		       {						\
			 ret_val.append(a);				\
		       });						\
    return ret_val;							\
  }									\
									\
  template <template <typename> class Container = Aleph::DynList>	\
  Container<Arc*> arcs(Node * p) const					\
  {									\
    Container<Arc*> ret_val;						\
    this->for_each_arc(p, [&ret_val] (Arc * a)				\
		       {						\
			 ret_val.append(a);				\
		       });						\
    return ret_val;							\
  }									\


/**
  @defgroup Graphs Graphs.

  @ { Graphs

  Aleph allows modeling graphs , represent many problems
  combinatorial and run the vast majority of known algorithms.


  All network object type handles two additional objects that are
  part of the definition of a graph :

  - Nodes which are modeled by the main class
   Graph_Node < Node_Info >.

  - Arcs: which are modeled by the main class
   Graph_Arc < Arc_info >.

  - Agents : These objects are used for certain types of graphs and
   entities modeled dynamically traverse the graph.

There are several types of graphs (or digraphs) modeled by different
  classes:

  - List_Graph <Node, Arc>: This is the simplest class and can handle the
  Most models with graphs and solve their problems.

  - List_Digraph <Node, Arc>: like List_Graph, except for graphs
  directed.

  - Net_graph <Node, Arc>: to model flow networks and other problems
   inherent (minimal cut, etc).

  - Concurrent_Graph <Node, Arc>: This class is similar to List_Graph, but with
  and designed completely reentrant routines
  multi-threading. Estro allows concurrent design algorithms
  would result in a better runtime.

  - Agent_Graph <Node, Arc, Agent>: This graph is concurrent and uses
  an additional class of agent. An agent is an entity that runs the
  graph according to a transition function and allows modeling problems
  by abstracting agent as an entity that runs the graph. By
  eg car traffic. Agents graphs are by
  yes concurrent; agents can stroll the same as
  the number of processors that have the hardware.

  - Ant_Graph: This graph extends the class in which Agent_Graph in
  Instead of ants agents are handled.

  - Sim_Agent_Graph <Node, Arc, Ant>: modeled a network of agents in the
  where time is a discrete event.
  These graphs are concurrent and are intended to represent
  simulation models.


  The classes that handle agents have the ability to change
  dynamically the graph, which is known on the grounds of
  modeling as "structural change".


  There are three phases in the management of a graph:

  - # Definition of the types of nodes and arcs. For graphs
  traditional, nodes and arcs are modeled. Suppose, for
  example, you have a transportation problem in which they wish
  modeling various means of transport. In this case, attributes
  nodes would be the names of cities, along with other
  relevant attributes. For arches, they contain
  Common parameters such as time, distance and cost. To
  modeling communications of various kinds, automobile, river,
  sea, rail and air, it can be achieved by deriving the class
  __Graph_Arc <Arc_Info>.

  - # Construction of the graph: This is the phase which defines the
    shape of the graph by the fundamental operations on nodes
    and arcs.

  - # Running solving a problem: once defined and
    He built the graph, then it proceeds to the resolution of
    specific problem.

  @}
*/


} /* End namespace Aleph */

# endif
