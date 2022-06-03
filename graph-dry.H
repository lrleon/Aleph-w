
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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

# ifndef GRAPH_DRY_H
# define GRAPH_DRY_H


/** Common node iterator for graph having its node derived from Dlink
    class.

    This class serves as implementation support for iterador on nodes
    that are derived for Dlink class. This is currently the case for
    List_Graph and Array_Graph.

    \ingroup Grafos
*/
template <class GT>
struct GTNodeIterator : public Dlink::Iterator
{
  using Node = typename GT::Node;

  /// The type of item that returns the iterator
  using Item_Type = Node*;

  /// The type of container on which iterate
  using Set_Type = GT;

  GTNodeIterator() noexcept { /* empty */ }

      /// Build a iterator for all the nodes of g
  GTNodeIterator(Dlink & head) noexcept : Dlink::Iterator(head) { /* empty */ }

  /// Return the current node without exception
  Node * get_curr_ne() const noexcept
  {
    return (Node*) Dlink::Iterator::get_curr_ne();
  }

  /// Return the current node
  Node * get_curr() const { return (Node*) Dlink::Iterator::get_curr(); }

  /// Return the current node
  Node * get_current_node() const { return get_curr(); }

  Node * get_current_node_ne() const { return get_curr_ne(); }
};


/**  Common arc iterator for graph having its arcs derived from Dlink
     class.

     This class serves as implementation support for iterador on arcs
     that are derived for Dlink class. This is currently the case for
     List_Graph and Array_Graph.

     \ingroup Grafos
 */
template <class GT>
struct GTArcIterator : public Dlink::Iterator
{
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;

  /// The type of item that returns the iterator
  using Item_Type = Arc*;

  /// The type of container on which iterate
  using Set_Type = GT;

  GTArcIterator() noexcept { /* empty */ }

  /// Build a iterator for all the arcs of g
  GTArcIterator(Dlink & head) noexcept
    : Dlink::Iterator(head) 
  { /* empty */ }

  /// Return current arc without exception
  Arc * get_curr_ne() const noexcept
  {
    return (Arc*) Dlink::Iterator::get_curr_ne();
  }

  /// Return current arc
  Arc * get_curr() const { return (Arc*) Dlink::Iterator::get_curr(); }

  /// Return the current arc
  Arc * get_current_arc() const { return get_curr(); }

  /// Return the current arc without exception
  Arc * get_current_arc_ne() const noexcept { return get_curr_ne(); }

  /// Return the sourcenode of current arc (if it is a directed graph) 
  Node * get_src_node_ne() const noexcept 
  {
    return (Node*) get_curr_ne()->src_node; 
  }

  /// Return the target node of current arc (if it is a directed graph) 
  Node * get_tgt_node_ne() const noexcept
  {
    return (Node*) get_curr_ne()->tgt_node; 
  }

  /// Return the sourcenode of current arc (if it is a directed graph) 
  Node * get_src_node() const 
  {
    return (Node*) get_curr()->src_node; 
  }

  /// Return the target node of current arc (if it is a directed graph) 
  Node * get_tgt_node() const
  {
    return (Node*) get_curr()->tgt_node; 
  }
};


/** Used internally for some graphs for compare their nodes.

    \ingroup Grafos
*/
template <class GT, class Cmp>
struct Cmp_Dlink_Node
{
  using Node = typename GT::Node;

  Cmp & cmp;

  Cmp_Dlink_Node(Cmp && __cmp = Cmp()) noexcept : cmp(__cmp) { /* empty */ }

  Cmp_Dlink_Node(Cmp & __cmp) noexcept : cmp(__cmp) { /* empty */ }

  bool operator () (Dlink * d1, Dlink * d2) const noexcept
  {
    Node * p1 = static_cast<Node*>(d1); 
    Node * p2 = static_cast<Node*>(d2); 
    return cmp(p1, p2);
  }
};

/** Used internally for some graphs for compare their arcs.

    \ingroup Grafos
*/
template <class GT, class Cmp>
struct Cmp_Dlink_Arc
{
  using Arc = typename GT::Arc;

  Cmp & cmp;

  Cmp_Dlink_Arc(Cmp && __cmp = Cmp()) noexcept : cmp(__cmp) { /* empty */ }

  Cmp_Dlink_Arc(Cmp & __cmp) noexcept : cmp(__cmp) { /* empty */ }

  bool operator () (Dlink * d1, Dlink * d2) const noexcept
  {
    Arc * arc1 = static_cast<Arc*>(d1); 
    Arc * arc2 = static_cast<Arc*>(d2); 
    return cmp(arc1, arc2);
  }
};

/** Common attributes and methods for nodes (vertexes) belonging to
    graphs.

    This class defines common attributes and methods for graphs.  All
    graph nodes inherit the data and function members of this class.
    Although this class exports data members, it is not recommended to
    directly access them. Instead use the macros `NODE_BITS`,
    `NODE_COUNTER` and `NODE_COOKIE`, which define access to the three
    attributes.

    @see NODE_BITS NODE_COUNTER NODE_COOKIE IS_NODE_VISITED NODE_COLOR

    @warning This class is not intended to be used explicitly by the
    user. Its use is internal to the graphs implementations. 

    @ingroup Grafos
 */
template <typename NodeInfo>
class GTNodeCommon
{
public:
  
  /** Attributes of node

      @see Graph_Attr
  */
  Graph_Attr attrs;
  
  NodeInfo node_info; /// data associated to the node. Access it with get_info()


  /** Number of arcs.

      If the graph is directed, then this field does not have much
      sense, since it counts the total of arcs (incoming and outcoming).

      @warning Don't modifiy this field (NEVER!). It is public in order
      to facilitate the implementation of some graph operations.
   */
  size_t num_arcs = 0; 

  using Item_Type = NodeInfo; /// Common alias for set types

  using Node = GTNodeCommon;  /// The node

  using Node_Type = NodeInfo; /// another alias for set type

  GTNodeCommon() noexcept {}

  GTNodeCommon(const NodeInfo & info) : node_info(info) {}

  GTNodeCommon(NodeInfo && info) : node_info(move(info)) {}

  /// Return a modifiable reference to the data contained in the node
  NodeInfo & get_info() noexcept { return node_info; }

  /// Return a constant reference to the data contained in the node
  const NodeInfo & get_info() const noexcept { return node_info; }

  /// Return the state's value
  unsigned int state() const noexcept { return NODE_BITS(this).state; }	

  /// Set the state to value `s`
  void set_state(unsigned int s) noexcept { NODE_BITS(this).state = s; }
};


/** Common methods for the arc of a graph.

    This class defines common attributes and methods for arcs graphs.
    All graph arcs inherit the data and function members of this class.
    Although this class exports data members, it is not recommended to
    directly access them. Instead use the macros `ARC_BITS`,
    `ARC_COUNTER` and `ARC_COOKIE`, which define access to the three
    attributes.

    @see ARC_BITS ARC_COUNTER ARC_COOKIE IS_ARC_VISITED ARC_COLOR

    @warning This class is not intended to be used explicitly by the
    user. Its use is internal to the graphs implementations. 

    @ingroup Grafos
 */
template <typename ArcInfo>
class GTArcCommon
{
public:

  using Item_Type = ArcInfo;
  using Arc_Type = ArcInfo;

  void *     src_node = nullptr; /// Please don't use
  void *     tgt_node = nullptr; /// Please don't use

  /** Arc control attributes.

      @see Graph_Attr
  */
  Graph_Attr attrs; 

  ArcInfo    arc_info; /// data contained in arc

  GTArcCommon(const ArcInfo & info) : arc_info(info) {}

  GTArcCommon(ArcInfo && info) : arc_info(move(info)) {}

  GTArcCommon(void * src, void * tgt, const ArcInfo & data)
    : src_node(src), tgt_node(tgt), arc_info(data) {}

  GTArcCommon(void * src, void * tgt, ArcInfo && data = ArcInfo())
    : src_node(src), tgt_node(tgt), arc_info(move(data)) {}

  /// Return the state of arc
  unsigned int state() const noexcept { return ARC_BITS(this).state; }

  /// Set the state of arc to value `s`
  void set_state(unsigned int s) noexcept { ARC_BITS(this).state = s; }

  /// Return a modifiable reference to the arc data 
  ArcInfo & get_info() noexcept { return arc_info; }

  /// Return a constant reference to the arc data 
  const ArcInfo & get_info() const noexcept { return arc_info; }

  void * get_connected_node(void * node) noexcept
  {
    return src_node == node ? tgt_node : src_node;
  }

  void * get_img_node(void * node) noexcept
  {
    return src_node == node ? src_node : tgt_node;
  }
};


/** Common methods to the `Aleph-w` (\f$\aleph_\omega\f$) graps classes.

    @ingroup Grafos
*/
template <class GT, class Node, class Arc>
class GraphCommon
{
  GT * me() { return static_cast<GT*>(this); }

  const GT * const_me() const { return static_cast<const GT*>(this); }

protected:

  void * cookie = nullptr;
  size_t num_nodes = 0;
  size_t num_arcs = 0;
  bool digraph = false;

public:

  using Node_Type = typename Node::Node_Type;
  using Arc_Type = typename Arc::Arc_Type;
  
protected:

  void init() noexcept
  {
    num_nodes = num_arcs = 0;
    cookie = nullptr;
    digraph = false;
  }

  void common_swap(GT & g) noexcept
  { 
    std::swap(num_nodes, g.num_nodes);
    std::swap(num_arcs, g.num_arcs);
    std::swap(digraph, g.digraph);
    std::swap(cookie, g.cookie);	
  }

public:

  /// Return a modifiable reference to graph's cookie
  void *& get_cookie() noexcept { return cookie; }

  /// Return a constant reference to graph's cookie
  void * get_cookie() const noexcept { return cookie; }

  /// Return true if the graph `this` is directed
  bool is_digraph() const noexcept { return digraph; }

  /** Temporal indication for preventing to other algorithms that an
      graph must be treated as a directed graph.

      Sometimes (in `Aleph-w` (\f$\aleph_\omega\f$) many times) is
      desirable to deal with a non-directed graph as if this was a
      directed one. Such is the case, for example, of functor
      `Random_Digraph`, which is used for generating random digraphs. Of
      course, since `Random_Digraph` is designed for generating
      digraphs, it manages its internal state thinking that a digraph is
      generated. 

      Now, in a few circunstances, very often for practical purpuses
      (performace, easiness of coding, etc), a non-directed graph is
      wanted to be dealt as a directed one. In this case some algorithms
      need to know that the graph is in reality directed. In order to
      solve this ambiguity, the internal state that indicates that the
      graph is directed could be changed. In this way, foin order to
      instruct to `Random_Digraph` that deals a non-directed as a
      directed one, the flag "directed" is changed to `true`. 

      Of course, this technique is very very risky. Take into account
      that topological changing operations variate according to the
      directness of graph. Thus, to change the directed flag and then to
      insert and/or to remove nodes and/or arcs will mess everything and
      the chances of disater will be very high. For this reason, the use
      of this method is not recommended and its presence must be
      considered temporal while we find a more proper approach,
      concretely while `Random_Digraph` remains without to be
      refactored, which is, in fact, the unique part of `Aleph-w`
      (\f$\aleph_\omega\f$) that temporally uses this method.

      \warning Our advice is: don't use!
   */
  void set_digraph(bool val) { digraph = val; } // TODO: delete after

  /// Return the total of nodes of graph
  size_t get_num_nodes() const noexcept { return num_nodes; }

  /// \verload get_num_nodes()
  size_t vsize() const noexcept { return get_num_nodes(); }

  /** Return any node in the graph.

      This method serves as an entry point to the graph. Some algorithms
      only need an initial node which could be anyone. In these cases,
      this method is indicated.

      \return A pointer to a node in the graph
  */
  Node * get_node() const { return const_me()->get_first_node(); }
  
  /** Return any arc in the graph.

      This method serves as an depart arc in the graph. Some algorithms
      only need an initial arc which could be anyone. In these cases,
      this method is indicated.

      \return A pointer to a arc in the graph
  */
  Node * get_arc() const { return const_me()->get_first_arc(); }

  /** Return any arc adjacent to a node.

      This method serves as an arc of depart given a node. Some
      algorithms only need any arc adjacent to a node in order to start
      their computations. In these cases, this method is indicated.

      \return A pointer to an arc adjacent to `p`
  */
  Node * get_arc(Node * p) { return const_me()->get_first_arc(p); }

  /// Return the source node of `arc` (only for directed graphs)
  Node * get_src_node(Arc * arc) const noexcept
  {
    return (Node*) arc->src_node;
  }

  /// Return the target node of `arc` (only for directed graphs)
  Node * get_tgt_node(Arc * arc) const noexcept
  {
    return (Node*) arc->tgt_node;
  } 

  /** Return the adjacent node to `node` through `arc`.

      This method only has sense for non-directed graphs. In fact, for
      non directed graphs, this is the proper way for accessing to a
      target node from a specific arc.

      This is the expected way for accesing the adjacent nodes to a
      specific node. From a node `p`, we can see its arcs through a
      iterator of a funcrional `for_each()`.

      As example, consider the following code snippet which examinates
      the adjacent nodes from a node `p`:

          for (auto it = g.get_arc_it(p); it.has_curr(); it.next())
	    {
	      auto arc = it.get_curr(); // obtain the current arc
	      auto tgt = g.get_connected_node(arc, p); // the adjacent node
	      // ...
	    }

      @param[in] arc the arc
      @param[in] node the node from you must be seeing the arc
      @return an adjacent node tgt with form `p ---arc--- tgt`

      @warning This method has not any sense, and very probably gives
      incorrect results, on directed graphs.  The subtle detail is that
      sometimes, unfortunately but incorrectly, can work. So be very
      careful and be sure of understand its use in the context of non
      directed graphs
   */
  Node * get_connected_node(Arc * arc, Node * node) const noexcept
  {
    return (Node*) arc->get_connected_node(node);		
  }
  
  /// \overload degree()
  size_t get_num_arcs() const noexcept { return num_arcs; }

  /// Return the total of arcs of a node. Only has sense for non
  /// directed graphs
  size_t get_num_arcs(Node * node) const noexcept
  {
    return node->num_arcs;
  }

  /// Return the total of arcs (or degree) of a node. Only has sense for
  /// non directed graphs
  size_t degree(Node * p) const noexcept { return get_num_arcs(p); }

  /// Return the total of arcs of graph
  size_t esize() const noexcept { return get_num_arcs(); }

  /// Return a reference to control fields of `node`
  Bit_Fields & get_control_bits(Node * node) const noexcept
  {
    return NODE_BITS(node).reset();
  }

  /// Reset the `bit` of `node` (to zero)
  void reset_bit(Node * node, int bit) const noexcept
  {
    NODE_BITS(node).reset(bit);
  }
  
  /// Reset all the control bits of `node`
  void reset_bits(Node * node) const noexcept { NODE_BITS(node).reset(); }

  /// Get the control `bit` of `node`
  int get_bit(Node * node, int bit) const noexcept
  {
    return NODE_BITS(node).get_bit(bit);
  }

  /// Set the control `bit` of `node` to `value`
  void set_bit(Node * node, int bit, int value) const noexcept
  {
    NODE_BITS(node).set_bit(bit, value);
  }

  /// Return a reference to the control bits of `arc`
  Bit_Fields & get_control_bits(Arc * arc) const noexcept
  {
    return ARC_BITS(arc);
  }

  /// Reset the `bit` of `arc` to zero
  void reset_bit(Arc * arc, int bit) const noexcept
  {
    ARC_BITS(arc).reset(bit);
  }

    /// Reset all the control bits of `arc`
  void reset_bits(Arc * arc) const noexcept { ARC_BITS(arc).reset(); } 

    /// Get the control `bit` of `arc`
  int get_bit(Arc * arc, int bit) const noexcept
  {
    return ARC_BITS(arc).get_bit(bit);
  }

  /// Set the control `bit` of `arc` to `value`
  void set_bit(Arc * arc, int bit, int value) const noexcept
  {
    ARC_BITS(arc).set_bit(bit, value);
  }

  /// Get a modifiable reference to the cookie pointer of `node`
  void *& get_cookie(Node * node) const noexcept
  {
    return NODE_COOKIE(node);
  }

  /// Get a modifiable reference to the cookie pointer of `arc`
  void *& get_cookie(Arc * arc) const noexcept
  {
    return ARC_COOKIE(arc);
  }

  /// Get a modifiable reference to the counter of `node`
  long & get_counter(Node * node) const noexcept
  {
    return NODE_COUNTER(node);
  }

  /// Reset the `node` counter to zero
  void reset_counter(Node * node) const noexcept 
  {
    NODE_COUNTER(node) = 0;
  }

  /// Reset all the node counters of graph to zero
  void reset_node_counters() const noexcept 
  {
    for_each_node([this] (auto p) { this->reset_counter(p); });
  }

  /// Reset all the control attributes of node `p`. That is: all control
  /// bits, the state and the counter are set to zero. The cookie is set
  /// to `nullptr` value
  void reset_node(Node * p) const noexcept
  {
    p->attrs.reset();
  }

  /// Get a modifiable reference to the counter of `arc`
  long & get_counter(Arc * arc) const noexcept
  {
    return ARC_COUNTER(arc);
  }

  /// Reset the `acr` counter to zero
  void reset_counter(Arc * arc) const noexcept
  {
    ARC_COUNTER(arc) = No_Visited;
  }

  /// Reset all the arc counters of graph to zero
  void reset_arc_counters() const noexcept 
  {
    for_each_arc([this] (auto a) { this->reset_counter(a); });
  }

  /// Reset all the control attributes of `arc`. That is: all control
  /// bits, the state and the counter are set to zero. The cookie is set
  /// to `nullptr` value
  void reset_arc(Arc * arc) const noexcept
  {
    arc->attrs.reset();
  }
 
  /// Reset all the nodes of graph (the control bits, the state, the
  /// counter and the cookie)
  void reset_nodes() const
  {
    for_each_node([] (auto p) { p->attrs.reset(); });
  }

  /// Reset all the arcs of graph (the control bits, the state, the
  /// counter and the cookie)
  void reset_arcs() const
  {			
    for_each_arc([] (auto a) { a->attrs.reset(); });
  }

  /** Map the nodes through their cookies.

      `map_nodes(p, q)` is intended to biyectively to map the node `p`
      towards the node `q` through their cookies. Basically, after
      calling the cookie of `p` points to `q` and the cookie of `q`
      points to `p`. 

      If the cookie value of `p` is not `nullptr`, then
      `map_nodes(p, q)` assumes that there already is a mapping and then
      perform the composition. This could be very useful for having
      several mapping. 

      Supposse that we have three nodes `p`, `q` and `r` belonging to
      three different, but from certain way isomorphic, graphs `g1` and
      `g2` of type `GT`, and `g3` of type `GTT`. Note that the types of
      graph could variate. Thus, this call:
      
          g1.map_nodes(p, q);

      Instruments a biyective mapping \f$p \longleftrightarrow q\f$.
      Note that since `p` and `q` are of the same type (although
      belong to different graphs), it is not necessary to specify as
      template parameters the graph types. Another way for invoking
      exactly the same function is:

          GT::map_nodes(p, q);

      This is because `map_nodes()` is a static member.

      Now, suppose that you wish to map the node `r` to `p` and `q`- Ypu
      could do as follows:

          g1.map_nodes<GT, GTT>(p, r);

      Now the mapping is \f$p \longleftrightarrow q \longleftrightarrow
      r\f$. Note that in this case, since the graph types are not the
      same, you must specify them as template parameters. Of course, in
      this case, for retrieving `r` you must do two cookie's
      inspections, one first on `p` for retrieving `q`, and a second on
      `q` for retrieving `r`.

      In our experiences, this is the fastest way for implementing
      mappings. Apart from the fact that the retrieval is \f$O(1)\f$,
      this is also deterministic. Compare this fact with other
      approaches requiring bookkeeping throug hash tables or trees. In
      addiction, if many (or all) nodes are required to map, then this
      approach is also the less space consuming. More still, correctly
      used, this approach could be simpler than hash tables or trees,
      since you do not require additional declarations, inicializations,
      etc. 

      Of course, this approach has the big con that it is not very typed
      neither validated. If you do a mistake all probably will
      crash. However, the big gain in speed rewards and deserves the
      presence of this feature.

      @param[in] p source node of mapping
      @param[in] q target node of mapping

      @see mapped_node()
   */
  template <class N1, class N2 = N1> static
  void map_nodes(N1 * p, N2 * q) noexcept
  {
    assert(p != nullptr and q != nullptr);
    if (NODE_COOKIE(p) == nullptr)
      {
       NODE_COOKIE(p) = q;
       NODE_COOKIE(q) = p;
       return;
     }
    NODE_COOKIE(q) = NODE_COOKIE(p);
    NODE_COOKIE(p) = q;
  }

  /** Map the arcs through their cookies.

      `map_arcs(p, q)` is intended to biyectively to map the arc `p`
      towards the arc `q` through their cookies. Basically, after
      calling the cookie of `p` points to `q` and the cookie of `q`
      points to `p`. 

      See map_nodes() for a better explanation.

      @param[in] p source arc of mapping
      @param[in] q target arc of mapping

      @see mapped_arc()
 */
  template <class A1, class A2 = A1> static
  void map_arcs(A1 * p, A2 * q) noexcept
  {
    assert(p != nullptr and q != nullptr);
    if (ARC_COOKIE(p) == nullptr)
      {
	ARC_COOKIE(p) = q;
	ARC_COOKIE(q) = p;
	return;
      }
    ARC_COOKIE(q) = ARC_COOKIE(p);
    ARC_COOKIE(p) = q;
  }
  
  /// Reset `bit` to zero for all the nodes of graph
  void reset_bit_nodes(int bit) const noexcept
  {
    for_each_node([bit, this] (auto p) { this->reset_bit(p, bit); });
  }

  /// Reset `bit` to zero for all the arcs of graph
  void reset_bit_arcs(int bit) const noexcept
  {
    for_each_arc([bit, this] (auto a) { this->reset_bit(a, bit); });
  }

  /// Reset all the bits for all the nodes of graph
  void reset_bit_nodes() const noexcept
  {
    for_each_node([this] (auto p) { this->reset_bits(p); });
  }

  /// Reset all the bits for all the arcs of graph
  void reset_bit_arcs() const noexcept
  {
    for_each_arc([this] (auto a) { this->reset_bits(a); });
  }
  
  /// Reset all the counters to zero for all the nodes of graph
  void reset_counter_nodes() const noexcept
  {
    for_each_node([this] (auto p) { this->reset_counter(p); });
  }

  /// Reset all the counters to zero for all the arcs of graph
  void reset_counter_arcs() const noexcept
  {
    for_each_arc([this] (auto a) { this->reset_counter(a); });
  }

  /** Reset all the cookies to `nullptr for all the nodes of graph.

      @warning Be sure to verify that the cookie is not being used,
      otherwise most likely will lose memory or worse, your algorithm will
      not work properly 
  */
  void reset_cookie_nodes() const noexcept
  {
    for_each_node([] (auto p) { NODE_COOKIE(p) = nullptr; });
  }

  /** Reset all the cookies to `nullptr for all the arcs of graph.

      @warning Be sure to verify that the cookie is not being used,
      otherwise most likely will lose memory or worse, your algorithm will
      not work properly 
  */
  void reset_cookie_arcs() const noexcept
  {
    for_each_arc([] (auto a) { ARC_COOKIE(a) = nullptr; });
  }

  /** Allocate a new node, set by copy its data content and insert it
      into the graph.

      This method perform several actions. First, it allocates memory
      for a graph node. Then the data `node_info` is copied to the data
      associated to the node. This copy is done via the copy constructor
      and assign operator. So, these functionalities must be present for
      the class `Node_Type`. Finally, the node is topologically
      inserted into the graph.

      @remark If this method does not throw exception, then this always
      returns a valid pointer,

      @param[in] node_info infopr to copy to the new node. The copy
      constructor and the assign operator must be defined for the class
      Node_Type. 
      @return a pointer to the new inserted node.
      @throw bad_allod if there is no enough memory
   */
  Node * insert_node(const Node_Type & node_info)
  {
    return me()->insert_node(new Node (node_info));
  }

  /** Allocate a new node, set by moving its data content and insert it
      into the graph.

      This method perform several actions. First, it allocates memory
      for a graph node. Then the data `node_info` is moved to the data
      associated to the node. This movement is done via the move
      constructor and move assign operator. So, these functionalities
      must be present for the class `Node_Type`. Finally, the node is
      topologically inserted into the graph.

      @remark If this method does not throw exception, then this always
      returns a valid pointer,

      @param[in] node_info info to move to the new node. The move
      constructor and the move assign operator must be defined for the class
      Node_Type. 
      @return a pointer to the new inserted node.
      @throw bad_allod if there is no enough memory.
  */
  Node * insert_node(Node_Type && node_info = Node_Type())
  {
    return me()->insert_node(new Node(std::forward<Node_Type>(node_info)));
  }

  /** Insert a new node in the graph by constructing it in-place with
      the given args. 

      This method is an effective way for inserting a new node in the
      graph and bypassing innecesary copies. `emplace_node()` allocates
      a new node, takes the parameters received and calls to the
      constructor of data associated to the node. Then the data is
      forwarded to the node data constructor avoiding, if possible,
      innecessary copy. Once the node is completely built, this is
      inserted, at this moment without any logic possibility of failure,
      into the graph.

      @remark If this method does not throw exception, then this always
      returns a valid pointer,

      @param[in] args variadic argument list for the construction of
      data associated to the node.
      @return a pointer to the new and inserted node.
      @throw bad_allod if there is no enough memory.
   */
  template <typename ...Args>
  Node * emplace_node(Args && ... args)
  {
    return me()->insert_node(Node_Type(args...));
  }

  /** Create and insert a new arc linking two nodes and copying data.

      `insert_arc()` allocates a new arc linking the nodes pointed by
      `src` and `tgt`. If `this` is a directed graph, then `src` is
      considered the source node and `tgt` the target one. When the arc
      has been effectively allocated, the data contained in `arc_info`
      is copied to the node. Finally, the arc is inserted into the
      graph.
      
      The pointers `src`and `tgt` must be valid; that is, of course,
      they must correspond to real nodes already inserted in the
      graph. At this regard, no validation is done.

      @remark If this method does not throw exception, then this always
      returns a valid pointer,

      @param[in] src pointer to the source node.
      @param[in] tgt pointer to the target node.
      @param[in] arc_info the data to be copied to the node.
      @return a pointer to the arc already inserted into the graph
      @throw bad_alloc if there is no enough memory.      
   */
  Arc * insert_arc(Node * src, Node * tgt, const Arc_Type & arc_info)
  {
    std::unique_ptr<Arc> arc(new Arc(arc_info));
    me()->insert_arc(src, tgt, arc.get());
    return arc.release();
  }

  /** Create and insert a new arc linking two nodes and moving the
      received data.

      Thi method first` allocates a new arc linking the nodes pointed by
      `src` and `tgt`. If `this` is a directed graph, then `src` is
      considered the source node and `tgt` the target one. Once the arc
      has been effectively allocated, if possible, the data contained in
      `arc_info` is moved to the node avoiding copy. Finally, the arc is
      inserted into the graph.
      
      The pointers `src`and `tgt` must be valid; that is, of course,
      they must correspond to real nodes already inserted in the
      graph. At this regard, no validation is done.

      @remark If this method does not throw exception, then this always
      returns a valid pointer,

      @param[in] src pointer to the source node.
      @param[in] tgt pointer to the target node.
      @param[in] arc_info the data to be moved to the node.
      @return a pointer to the arc already inserted into the graph
      @throw bad_alloc if there is no enough memory.      
   */
  Arc * insert_arc(Node * src, Node * tgt, Arc_Type && arc_info = Arc_Type())
  {
    std::unique_ptr<Arc> arc(new Arc(std::forward<Arc_Type>(arc_info)));
    me()->insert_arc(src, tgt, arc.get());
    return arc.release();
  }

  /** Insert a new arc in the graph by constructing its associated data
      in-place with the given args.

      This method allows to insert a new arc in the graph bypassing
      innecesary copies. `emplace_arc()` allocates a new arc, takes the
      parameters received and calls to the constructor of data
      associated to the arc. Then the data is forwarded to the arc data
      constructor avoiding, if possible, innecessary copies. Once the
      arc is completely built, this is inserted, at this moment without
      any logic possibility of failure, into the graph.

      @remark If this method does not throw exception, then this always
      returns a valid pointer,

      @param[in] src pointer to the source node.
      @param[in] tgt pointer to the target node.
      @param[in] args variadic argument list for the construction of
      data associated to the arc.
      @return a pointer to the new and inserted arc.
      @throw bad_allod if there is no enough memory.
  */
  template <typename ...Args>
  Arc * emplace_arc(Node * src, Node * tgt, Args && ... args)
  {
    return me()->insert_arc(src, tgt, Arc_Type(args...));
  }
  
  /** Conditioned traversal of all the nodes of a graph.

      `traverse_nodes(operation)` condionally traverses all the nodes of
      a graph and on each node `p` executes `operation(p)`. The
      `operation` must have the following functor structure:

          bool operation(Node * p)

      This signature can be accomplished through a function pointer, a
      functor or a lambda (via the overloaded rvalue version).  Note
      that there is a return value. If `operation(p)` returns `true`,
      then the next node is traversed. Otherwise the traversal stops and
      the remainder nodes are not visited.

      For example, supposse that we wish print out the nodes content for
      all the nodes. Then we could do it as follows:

          g.traverse_nodes([] (auto p) 
	    { 
	      cout << p->get_info() << endl; 
	      return true; // for instructing to visit the next node
	    });

      `traverse_nodes()` is the heart of functional primitives on nodes
      of a graph. All other functional primitives depend on it. In
      general, ait is as fast as to directly iterate on the nodes.

      The operation is \f$O(V)\f$ for the worst and average case.

      @remark According to the compiler, invalid or uncompliant
      signatures for `operation`, by example, to not return anything,
      not only they probably will not compile, but the diagnostic errors
      could be hard to undertand. Perhaps, on some compilers, bad
      signatures could compile. So, be sure to match the correct
      operation signature.

      @param[in] operation operation to be performed on each node. The
      operation must imperatively return a `bool` indicating whether or
      not the next node must be visited.
      @return a `bool` with the last `operation`result. If `true` then
      all the mode were visited and on each one `operation`
      executed. Otherwise, the traversal was stopped.
      @throw any exception that could throw `operation`

      @see traverse_arcs() all_nodes()
   */
  template <class Operation>
  bool traverse_nodes(Operation & op) const
  {
    for (typename GT::Node_Iterator it(*const_me()); it.has_curr(); it.next_ne())
      if (not op(it.get_curr_ne()))
	return false;
    return true;
  }

  /// \overload traverse_nodes()
  template <class Operation>
  bool traverse_nodes(Operation && op = Operation()) const
  {
    return traverse_nodes(op);
  }

  /** Conditioned traversal of all the arcs of a graph.

      `traverse_arc(operation)` condionally traverses all the arcs  of
      a graph and on each arc `a` executes `operation(a)`. The
      `operation` must have the following functor structure:

          bool operation(Arc * a)

      This signature can be accomplished through a function pointer, a
      functor or a lambda (via the overloaded rvalue version).  Note
      that there is a return value. If `operation(p)` returns `true`,
      then the next node is traversed. Otherwise the traversal stops and
      the remainder nodes are not visited.

      if the graph type was `GT` then the arc type would be
      `GT::Arc`. In this primitive as well for the majority of
      functional primitives, you could declare the parameter types as
      auto and to leave to compiler to infer them. This could be more
      practical and easier for writing but it also could hide the true
      type and in some cases to do incomprehensible the the type.

      The operation is \f$O(E)\f$ for the worst and average case.

      For example, supposse that we wish print out the arcs content for
      all the arcs. Then we could do it as follows:

          g.traverse_arcs([] (auto a) 
	    { 
	      cout << a->get_info() << endl; 
	      return true; // for instructing to visit the next node
	    });

      @remark According to the compiler, invalid or uncompliant
      signatures for `operation`, by example, to not return anything,
      not only they probably will not compile, but the diagnostic errors
      could be hard to undertand. Perhaps, on some compilers, bad
      signatures could compile. So, be sure to match the correct
      operation signature.

      @param[in] operation operation to be performed on each arc. The
      operation must imperatively return a `bool` indicating whether or
      not the next arc must be visited.
      @return a `bool` with the last `operation`result. If `true` then
      all the arcs were visited and on each one `operation`
      executed. Otherwise, the traversal was stopped.
      @throw any exception that could throw `operation`

      @see traverse_arcs() all_arcs()
   */
  template <class Operation>
  bool traverse_arcs(Operation & op) const
  {
    for (typename GT::Arc_Iterator it(*const_me()); it.has_curr(); it.next_ne())
      if (not op(it.get_curr_ne()))
	return false;
    return true;
  }

  /// \overload traverse_arcs()
  template <class Operation>
  bool traverse_arcs(Operation && op = Operation()) const
  {
    return traverse_arcs(op);
  } 

  /** Conditioned traversal of all the adjacent arcs of a node.

      `traverse_arc(p, operation)` condionally traverses all the arcs of
      the node `p` and on each arc `a` executes `operation(a)`. The
      `operation` must have the following functor structure:

          bool operation(Arc * a)

      This signature can be accomplished through a function pointer, a
      functor or a lambda (via the overloaded rvalue version).  Note
      that there is a `bool` return value. If `operation(p)` returns
      `true`, then the next node is traversed. Otherwise the traversal
      stops and the remainder nodes are not visited.

      if the graph type was `GT` then the arc type would be
      `GT::Arc`. In this primitive as well for the majority of
      functional primitives, you could declare the parameter types as
      auto and to leave to compiler to infer them. This could be more
      practical and easier for writing but it also could hide the true
      type and in some cases to do incomprehensible the the type.

      The operation is \f$O(V)\f$ for the worst case, but it could be
      much less for sparsed graphs.

      For example, supposse that we wish print out the arcs content for
      the nodes adjacent to the node `p`. Then we could do it as follows:

          g.traverse_arcs(p, [] (auto a) 
	  { 
	    cout << a->get_info() << endl; 
	    return true; // for instructing to visit the next node
          });

      @remark According to the compiler, invalid or uncompliant
      signatures for `operation`, by example, to not return anything,
      not only they probably will not compile, but the diagnostic errors
      could be hard to undertand. Perhaps, on some compilers, bad
      signatures could compile. So, be sure to match the correct
      operation signature.

      @param[in] p node from you want to access its adjacent arcs.
      @param[in] operation operation to be performed on each arc. The
      operation must imperatively return a `bool` indicating whether or
      not the next arc must be visited.
      @return a `bool` with the last `operation`result. If `true` then
      all the arcs were visited and on each one `operation`
      executed. Otherwise, the traversal was stopped.
      @throw any exception that could throw `operation`

      @see traverse_arcs() all_arcs()
  */
  template <class Operation>
  bool traverse_arcs(Node * p, Operation & op) const
  {
    for (typename GT::Node_Arc_Iterator it(p); it.has_curr(); it.next_ne())
      if (not op(it.get_curr_ne()))
	return false;
    return true; 
  }

  /// \overload traverse_arcs(Node * p, Operation & operation)
  template <class Operation>
  bool traverse_arcs(Node * p, Operation && op = Operation()) const
  {
    return traverse_arcs(p, op);
  }

  /** Unconditionally traverse all the nodes of graph and on each one
      perform an operation.
      
      `for_each_node()` unconditionally traverse all the nodes of a
      graph and on each one performs `operation`, whicsh must have the
      following signature:
      
          void operation(Node * p)

      The operation could be a function pointer matching that signature,
      a functor or a lambda. Note that the `operation` does not require
      return anything. 

      Your algorithm should not assume any particular order of visit.

      If your goal is to initialize control state of the nodes, then
      don't use this method. Instead, use `reset_nodes()` or their
      derivatives according your situation.

      The complexity for this primitive always is \f$O(V)\f$.

      @param[in] operation to be performed on each node.
      @throw anything that can throw `operation`.
   */
  template <class Operation>
  void for_each_node(Operation & operation) const
  {
    for (typename GT::Node_Iterator it(*const_me()); it.has_curr(); it.next_ne())
      operation(it.get_curr_ne());
  }

  /// \overload for_each_node()
  template <class Operation>
  void for_each_node(Operation && operation = Operation()) const
  {
    for_each_node(operation);
  }

  /** Unconditionally traverse all the arcs of graph and on each one
      perform an operation.
      
      `for_each_arc()` unconditionally traverse all the arcs of a
      graph and on each one performs `operation`, which must have the
      following signature:
      
          void operation(Arc * p)

      The operation could be a function pointer matching that signature,
      a functor or a lambda. Note that the `operation` does not require
      return anything. 

      Your algorithm should not assume any particular order of visit.

      If your goal is to initialize control state of the arcs, then
      don't use this method. Instead, use `reset_arcs()` or their
      derivatives according your situation.

      The complexity for this primitive always is \f$O(E)\f$.

      @param[in] operation to be performed on each node.
      @throw anything that can throw `operation`.
  */
  template <class Operation> 
  void for_each_arc(Operation & op) const
  {
    for (typename GT::Arc_Iterator it(*const_me()); it.has_curr(); it.next_ne())
      op(it.get_curr_ne());
  }

  /// \overload for_each_arc(Operation & operation)
  template <class Operation>
  void for_each_arc(Operation && operation = Operation()) const
  {
    for_each_arc(operation);
  }

  /** Unconditionally traverse all the arcs adjacnt to a node and on
      each one perform an operation.
      
      `for_each_ars(p)` unconditionally traverse all the nodes of node
      `p` on each one performs `operation`, which must have the
      following signature:
      
          void operation(Arc * p)

      The operation could be a function pointer matching that signature,
      a functor or a lambda. Note that the `operation` does not require
      return anything. 

      This probably is the most common primitive for algorithms on graphs.

      The complexity for this primitive always is \f$O(V)\f$ worst case,
      but on sparsed graphs a fraction of \f$V\f$ or better \f$O(1)\f$
      for special buy common graphs such as "small word" networks.

      Your algorithm should not assume any particular order of visit.

      @remark This primitive only has sense for non directed graph. Note
      that each seen arc does not distinguish to `p`. If you want to
      know the node connected to `p` then invoke to
      `g.get_connected_node(a,p)`.

      @remark For directed graphs use `for_each_in_arc()` or
      `for_each_out_arc()`- 

      @param[i] p pointer to the node from you want to see its arcs.
      @param[in] operation to be performed on each node.
      @throw anything that can throw `operation`.
   */
    template <class Operation>
  void for_each_arc(Node * p, Operation & op) const
  {
    for (typename GT::Node_Arc_Iterator it(p); it.has_curr(); it.next_ne())
      op(it.get_curr_ne());
  }

  /// \overlaod for_each_arc(Node * p, Operation & operation)
  template <class Operation>
  void for_each_arc(Node * p, Operation && op = Operation()) const
  {
    for_each_arc(p, op);
  }

  /** Check if all the nodes of graph satisfy an boolean condition.

      `all_nodes()` traverse each node of graph and on each one
      `operation` is tested. If `operation` returns `true`, then the
      next node is inspected. Otherwise, the traversal stops and `false`
      is returned as result.

      The `operation` must imperatively have the following structure:

          bool operation(Node * p)

      The idea is to perform a test and according to its result to
      return `true` is the test was passed, or false otherwise. For
      example, in order to check if all the nodes content is even you
      could do:

          g.all_nodes([] (auto p) { return p->get_info() % 2 == 0; });

      \remark Note that `all_nodes()` is semantically equivalent to
      `traverse_nodes()`. 

      The complexity for this primitive always is \f$O(V)\f$ worst case.
      
      @param[in] operation condition to test on each node.
      @return `true` if all the nodes satisfy the condition; `false`
      otherwise. 

      @see exists_node()
   */
  template <class Operation>
  bool all_nodes(Operation & op) const
  {
    return traverse_nodes(op);
  }

  /// \overload all_nodes()
  template <class Operation>
  bool all_nodes(Operation && op = Operation()) const
  {
    return all_nodes(op);
  }

    /** Check if all the arcs of graph satisfy a boolean condition.

      `all_arcs()` traverse each arc of graph and on each one the
      `operation` is tested. If `operation` returns `true`, then the
      next arc is inspected. Otherwise, the traversal stops and `false` is
      returned as result.

      The `operation` must imperatively have the following structure:

          bool operation(Arc * p)

      The idea is to perform a test and according to its result to
      return `true` is the test was passed, or false otherwise. For
      example, in order to check if all the arc content is odd you
      could do:

          g.all_arcs([] (auto a) { return a->get_info() % 2; });

      \remark Note that `all_arcs()` is semantically equivalent to
      `traverse_arcs()`. 

      The complexity for this primitive always is \f$O(E)\f$ worst case.
      
      @param[in] operation condition to test on each arc.
      @return `true` if all the arcs satisfy the condition; `false`
      otherwise. 

      @see exists_arc()
   */
  template <class Operation>
  bool all_arcs(Operation & op) const
  {
    return traverse_arcs(op);
  }

  /// \overload all_arcs(Operation & operation)
  template <class Operation>
  bool all_arcs(Operation && op = Operation()) const
  {
    return all_arcs(op);
  }

  /** Check if all the arcs adjacent to a node satisfy an boolean
      condition. 

      `all_arcs(p)` traverse each arc adjacent to node `p` and on each
      one the `operation` is tested. If `operation` returns `true`, then
      the next arc is inspected. Otherwise, the traversal stops and
      `false` is returned as result.

      The `operation` must imperatively have the following structure:

          bool operation(Arc * p)

      The idea is to perform a test and according to its result to
      return `true` is the test was passed, or false otherwise. For
      example, in order to check if all the arc content is odd you
      could do:

      g.all_arcs(p, [] (auto a) { return a->get_info() % 2; });

      \remark Note that `all_arcs(p)` is semantically equivalent to
      traverse_arcs(). 

      The complexity for this primitive always is \f$O(V)\f$ worst case
      for dense graphs.
      
      @param[in] p node pointer
      @param[in] operation condition to test on each arc.
      @return `true` if all the arcs satisfy the condition; `false`
      otherwise. 

      \see bool exists_arc(Operation & operation)
  */
  template <class Operation>
  bool all_arcs(Node * p, Operation & op) const
  { 
    return traverse_arcs(p, op);
  }

  /// \overload all_arcs(Node * p, Operation & operation)
  template <class Operation>
  bool all_arcs(Node * p, Operation && op = Operation()) const 
  {
    return all_arcs(p, op);
  }
  
  /** Map the nodes of a graph to a specific range.

      `nodes_map(operation)` produces a mapping list of the graph's
      nodes. The transformation is done by `operation` whose signature
      must imperatively be:

          T operation(Node * p)

      `operation` instruments a map from the node to the range type
      `T`. By default, `T` is `Node_Type` (the type associated to the
      node's data). In this case, you do not require to specify the
      range type as template parameter. Otherwise, in order to the
      compiler can know the range type, you must specify it as template
      parameter.

      Suppose by example that the nodes have integers and that you want
      to produce a mapping int --> double corresponding to the division
      of the data node between a specific divisor. Then you can do this
      as follows:

          auto l = g.map_nodes<double>([divisor] (auto p) 
	  {
	    return 1.0*p->get_info()/divisor;
          });

     \remark The name of this method (`nodes_map()` instead of
     `map_nodes()`) is due to the ambiguity that would cause with the
     `map_nodes()` intended for mapping the nodes through their cookies.

     @note In metaprogramming you will probably need add the `template`
     keyword before the name `nodes_map<your-target-type>`. 

         g.template nodes_map<ulong>([] (auto p) { return p->get_info(); });

     @param[in] operation transformation from `Node*` to `T` to be
     performed on each node.
     @return a `DynList<T>` containing the mapping.
     @throw bad_alloc if there is no enough memory for fuilding the
     dynamic list or anything else that can throw `operation`
   */
  template <typename T = Node_Type>
  auto nodes_map(std::function<T(Node *)> op) const
  {
    DynList<T> ret_val;
    for_each_node([&ret_val, &op] (Node * p) { ret_val.append(op(p)); });
    return ret_val;
  }

  /** Map the arcs of a graph to a specific range.

      `arcs_map(operation)` produces a mapping list of the graph's
      arcs. The transformation is done by `operation` whose signature
      must imperatively be:

          T operation(Arc * a)

      `operation` instruments a map from the arc to the range type
      `T`. By default, `T` is `Arc_Type` (the type associated to the
      arc's data). In this case, you do not require to specify the
      range type as template parameter. Otherwise, in order to the
      compiler can know the range type, you must specify it as template
      parameter.

      Suppose by example that the arcs have integers and that you want
      to produce a mapping int --> string corresponding to string
      reprsentation of the data arc. Then you can do this as follows:

          auto l = g.map_arcs<string>([] (auto a) 
	  {
	    return to_string(a->get_info());
          });

     \remark The name of this method (`arcs_map()` instead of
     `map_arcs()`) is due to the ambiguity that would cause with the
     `map_arcs()` intended for mapping the arcs through their cookies.

     @note In metaprogramming you will probably need add the `template`
     keyword before the name `arcs_map<your-target-type>`.

         g.template arcs_map<ulong>([] (auto p) { return p->get_info(); });

     @param[in] operation transformation from `ARc*` to `T` to be
     performed on each arc.
     @return a `DynList<T>` containing the mapping.
     @throw bad_alloc if there is no enough memory for fuilding the
     dynamic list or anything else that can throw `operation`
   */
  template <typename T = Arc_Type>
  auto arcs_map(std::function<T(Arc *)> operation) const
  {
    DynList<T> ret_val;
    for_each_arc([&ret_val, &operation] (Arc * p)
		 {
		   ret_val.append(operation(p));
		 });
    return ret_val;
  }

  /** Map the adjacent arcs of a node to a specific range.

      `arcs_map(p, operation)` produces a mapping list of the arcs
      adjacent to node `p`. The transformation is done by `operation`
      whose signature must imperatively be:

          T operation(Arc * a)

      `operation` instruments a map from the arc to the range type
      `T`. By default, `T` is `Arc_Type` (the type associated to the
      arc's data). In this case, you do not require to specify the range
      type as template parameter. Otherwise, in order to compiler can
      know the range type, you must specify it as template parameter.

      Suppose by example that the arcs have doubles and that you want to
      produce a mapping double --> \f$\sqrt double \f$. Then you can do
      this as follows:

          auto l = g.map_arcs([] (auto a) { return sqrt(a->get_info()); });

     Since the return type is the same than the arc content, it would
     not be necessary to pass a template parameter indicating the range
     type. If this was not be the situation, then you would have to
     specify the range type as template parameter.

     \remark The name of this method (`arcs_map()` instead of
     `map_arcs()`) is due to the ambiguity that would cause with the
     `map_arcs()` intended for mapping the arcs through their cookies.

     @note In metaprogramming you will probably need add the `template`
     keyword before the name `arcs_map<your-target-type>`. 

         g.template arcs_map<ulong>([] (auto p) { return p->get_info(); });

     @param[in] operation transformation from `Arc*` to `T` to be
     performed on each arc.
     @return a `DynList<T>` containing the mapping.
     @throw bad_alloc if there is no enough memory for fuilding the
     dynamic list or anything else that can throw `operation`
   */
  template <typename T = Arc_Type>
  auto arcs_map(Node * p, std::function<T(Arc *)> operation) const
  {
    DynList<T> ret_val;
    for_each_arc(p, [&ret_val, &operation] (Arc * a)
		 {
		   ret_val.append(operation(a));
		 });
    return ret_val;
  }

  /** Folding of nodes on a graph.

      `foldl_nodes(init, operation)` traverse all node of graph and
      maintain an accumulator value `acc` whose initial value is
      `init`. On each node `p`, it performs `operation(acu, p)` which
      must match with the following signature:
      
          T operation(const T &, Node * p)

      So, on each call it is peformed:

          acc = operation(acc, p);

      Therefore `acc` serve as an accumulator.

      Since `foldl_nodes()` is defined via `std::function` wrapper, it
      is very important to match the expected signature of
      `operation`. A proper and sure way of quickly define `operation`
      is by using `auto` parameters.
      
      For example, suppose that the nodes contain integers and that you
      want to compute the maximum value. Then you could do it as
      follows:

          g.foldl_nodes(numeric_limits<int>::min(), [] (auto acc, auto node)
	  {
	    return max(acc, node->get_info();
	  });

      @param[in] init initial value of accumulator
      @param[in] operation to be performed, which must match the
      signature previously explained.
      @return the final value of accumulator after full traversal.
   */
  template <typename T = Node_Type>
  T foldl_nodes(const T & init,
		std::function<T(const T&, Node*)> op) const
  {
    T ret = init;
    for_each_node([&ret, &op] (Node * p) { ret = op(ret, p); });			  return ret;
  }

  /** Folding of arcs on a graph.

      `foldl_arcs(init, operation)` traverse all arc of graph and
      maintain an accumulator value `acc` whose initial value is
      `init`. On each acr `a`, it performs `operation(acu, a)` which
      must match with the following signature:
      
          T operation(const T &, Arc * a)

      So, on each call it is peformed:

          acc = operation(acc, a);

      Therefore `acc` serve as an accumulator.

      Since `foldl_arcs()` is defined via `std::function` wrapper, it is very
      important to match the expected signature of `operation`. A proper
      and sure way of quickly define `operation` is by using `auto`
      parameters. 
      
      For example, suppose that the arcs contain doubles and that you
      want to compute the total sum, then you could do it as follows:

          g.foldl_arcs(p, [] (auto acc, auto node)
	  {
	    return acc + arc->get_info();
	  });

      @param[in] init initial value of accumulator
      @param[in] operation to be performed, which must match the
      signature previously explained.
      @return the final value of accumulator after full traversal.
   */
  template <typename T = Arc_Type>
  T foldl_arcs(const T & init,
	       std::function<T(const T&, Arc*)> op) const
  {
    T ret = init;
    for_each_arc([&ret, &op] (Arc * p) { ret = op(ret, p); });
    return ret;
  }

  /** Folding of arcs of a node.

      `foldl_arcs(p, init, operation)` traverse all the arcs adjacent to
      node `p` and maintain an accumulator value `acc` whose initial
      value is `init`. On each arc `a`, it performs `operation(acu, a)`
      which must match with the following signature:
      
          T operation(const T &, Arc * a)

      So, on each call it is peformed:

          acc = operation(acc, a);

      Therefore `acc` serves as an accumulator.

      Since `foldl_arcs()` is defined via `std::function` wrapper, it is very
      important to match the expected signature of `operation`. A proper
      and sure way of quickly define `operation` is by using `auto`
      parameters. 
      
      For example, suppose that the arcs contain doubles and that you
      want to compute the total sum, then you could do it as follows:

          g.foldl_arcs(p, [] (auto acc, auto node)
	  {
	    return acc + arc->get_info();
	  });

      @param[in] p node from where to access its adjacent arcs
      @param[in] init initial value of accumulator
      @param[in] operation to be performed, which must match the
      signature previously explained.
      @return the final value of accumulator after full traversal.
   */
  template <typename T = Arc_Type>
  T foldl_arcs(Node * p, const T & init,
	       std::function<T(const T&, Arc*)> op) const
  {
    T ret = init;
    for_each_arc(p, [&ret, &op] (Arc * a) { ret = op(ret, a); });
    return ret;
  }
  
  /** Filter the nodes satisfying a condition.

      `filter_nodes(op)` traverses each node of graph and recolects
      those satisfying the condition expressed by `op`. `op` must match
      the following signature:

          bool op(Node * p)

      If `op(p)` returns `true`, then `p` is filtered (recollected)
      towards a final dynamic list containing the filtered nodes.

      For example, if the nodes contain integer, then the following code
      snippet recollects those nodes whose value is greater than `x`:

          g.filter_nodes([x] (auto p) { return p->get_info() > x; });

      @param[in] op operation implementing the filtering condition.
      @return a DynList<Node*> object containing the filtered nodes.
      @throw bad_alloc if there is no enough memory for building the
      full returning list.
   */
  template <class Op> auto filter_nodes(Op & op) const
  {
    DynList<Node*> ret;
    for_each_node([&ret, &op] (Node * p)
		  {
		    if (op(p))
		      ret.append(p); 
		  });
    return ret;
  }

  /// \overload filter_nodes(Op & op)
  template <class Op> auto filter_nodes(Op && op) const
  {
    return filter_nodes(op);
  }

  /** Filter the arcs of graph satisfying a condition.

      `filter_arcs(op)` traverses each arc of graph and recolects
      those satisfying the condition expressed by `op`. `op` must match
      the following signature:

          bool op(Arc * a)

      If `op(p)` returns `true`, then `a` is filtered (recollected)
      towards a final dynamic list containing the filtered arcs.

      For example, if the arcs contain integers, then the following code
      snippet recollects those nodes whose value is greater than `x`:

          g.filter_arcs([x] (auto a) { return a->get_info() > x; });

      @param[in] op operation implementing the filtering condition.
      @return a DynList<Arc*> object containing the filtered arcs.
      @throw bad_alloc if there is no enough memory for building the
      full returning list.
   */
  template <class Op> auto filter_arcs(Op & op) const
  {
    DynList<Arc*> ret;
    for_each_arc([&ret, &op] (Arc * a)
		 {
		   if (op(a))
		     ret.append(a);
		 });
    return ret;
  }

  /// \overload filter_arcs(Op & op)
  template <class Op> auto filter_arcs(Op && op) const
  {
    return filter_arcs(op);
  }

  /** Filter the arcs adjacent to a node satisfying a condition.

      `filter_arcs(p, op)` traverses each arc adjacent to node `p` and
      recolects those satisfying the condition expressed by `op`. `op`
      must match the following signature:

          bool op(Arc * a)

      If `op(p)` returns `true`, then `a` is filtered (recollected)
      towards a final dynamic list containing the filtered arcs.

      For example, in order to get the arcs whose target node have degree
      greater than `n` you could do it as follows:

          g.filter_arcs(p, [&g, p, n] (auto a) 
	  {
	    return g.degree(g.get_connected_node(a, p)) > n;
	  });

      Note the closure (or captured) data: the graph `g`, by reference
      in order to avoid copying it, the pointer `p` to te source node
      and the value of `n`.

      @param[in] p pointer to node wanted to explore and to filter its
      adjacent arcs.
      @param[in] op operation implementing the filtering condition.
      @return a DynList<Arc*> object containing the filtered arcs.
      @throw bad_alloc if there is no enough memory for building the
      full returning list.
  */
  template <class Op> auto filter_arcs(Node * p, Op & op) const
  {
    DynList<Arc*> ret;
    for_each_arc(p, [&ret, &op] (Arc * a)
    {
      if (op(a))
	ret.append(a);
    });
    return ret;
  } 

  /// \overload filter_arcs()
  template <class Op> auto filter_arcs(Node * p, Op && op) const
  {
    return filter_arcs(p, op);
  }

  /** Determine if exists at least a node satisfying a condition.

      `exists_node(op)` returns `true` if at least a node of graph
      satisfies the condition expressed by `operation`.

      The operation has \f$O(V)\f$ complexity for the worst case.
      
      \remark Note that `exists_node()` is the oppossite or complement
      of `all_nodes()`. In fact, `exists_node(op)` could be written, in
      terms of `all_nodes(op)` as follows:
      
          not g.all_nodes([&op] (auto p) { return not op(p); });
	  
      \param[in] operation for testing existing condition
      \return `true` if at least a node satisfaying the condition is
      found; `false` otherwise.
      @see all_nodes()
   */
  template <class Operation>
  bool exists_node(Operation & op) const
  {
    return not traverse_nodes([&op] (Node * p) { return not op(p); });
  }

  /// \overload exists_node()
  template <class Operation>
  bool exists_node(Operation && op = Operation()) const
  {
    return exists_node(op);
  }

  /** Determine if exists at least a arc satisfying a condition.

      `exists_arc(op)` returns `true` if at least a node of graph
      satisfies the condition expressed by `operation`.

      The operation has \f$O(E)\f$ complexity for the worst case.
      
      \remark Note that `exists_arc()` is the oppossite or complement
      of `all_arcs()`. In fact, `exists_arc(op)` could be written, in
      terms of `all_arc(op)` as follows:
      
          not g.all_arc([&op] (auto p) { return not op(p); });
	  
      \param[in] operation for testing existing condition
      \return `true` if at least a node satisfaying the condition is
      found; `false` otherwise.
      @see all_arcs()
   */
  template <class Operation>
  bool exists_arc(Operation & op) const
  {
    return not traverse_arcs([&op] (Arc * a) { return not op(a); });
  }

  /// \overload exists_arc(Operation & operation)
  template <class Operation>
  bool exists_arc(Operation && op = Operation()) const
  {
    return exists_arc(op);
  }

  /** Determine if exists at least a arc adjacent to a node satisfying a
      condition.

      `exists_arc(p, op)` returns `true` if at least a node adjacent to `p`
      satisfies the condition expressed by `operation`.

      The operation has \f$O(V)\f$ complexity for the worst case;
      concretely on dense graphs, but it trends to be much lesser for
      sparsed graphs.
      
      \remark Note that `exists_arc()` is the oppossite or complement
      of `all_arcs()`. In fact, `exists_arc(p, op)` could be written, in
      terms of `all_arc(p, op)` as follows:
      
          not g.all_arc(p, [&op] (auto p) { return not op(p); });
	  
      \param[in] p node from which you want to access its arcs
      \param[in] operation for testing existing condition
      \return `true` if at least a node satisfaying the condition is
      found; `false` otherwise.
      @see all_arcs(Node * p)
   */
  template <class Operation>
  bool exists_arc(Node * p, Operation & op) const
  {
    return not traverse_arcs(p, [&op] (Arc * a) { return not op(a); });
  }

  /// \overload exists_arc(Node * p, Operation & operation)
  template <class Operation>
  bool exists_arc(Node * p, Operation && op = Operation()) const
  {
    return exists_arc(p, op);
  }

  /** Linear search of a node.

      Search linearly a node satisfying the condition `op`.

      This method is some similar to `exists_node()`; the difference is
      that it returns a pointer and its value indicates the success of
      search.

      The complexity is \f$O(V)\f$ for the worst case.

      \param[in] op the criteria.
      @return a valid pointer to found node if a node satisfies the
      criteria or `nullptr` otherwise

      @see exists_node()
   */
  template <class Op>
  Node * search_node(Op & op) const
  {
    for (typename GT::Node_Iterator it(*const_me()); it.has_curr(); it.next_ne())
      {
	auto p = it.get_curr_ne();
	if (op(p))
	  return p;
      }
    return nullptr;
  }

  /// \overload search_node()
  template <class Op>
  Node * search_node(Op && op) const
  {
    return search_node(op);
  }

  /** Find a node mathing a content.

      `find_node(info)`, which is a wrapper to `search_node()`, search a
      nodo whose `get_info()` result matches with the `info`
      parameter. The comparison is done via de operator `==` on the
      class `Node_Type`, which is usually the case.

      \param[in] info to be compared with nodes content
      \return a valid pointer to a node if there is one mathing `info`
      or `nullptr` otherwise.
   */
  Node * find_node(const Node_Type & info) const noexcept
  {
    return search_node([&info] (auto p) { return p->get_info() == info; });
  }

  /** Linear search of an arc.

      Search linearly an arc satisfying the condition `op`.

      This method is some similar to `exists_arc()`; the difference is
      that it returns a pointer and its value indicates the success of
      search.

      The complexity is \f$O(E)\f$ for the worst case.

      \param[in] op the criteria.
      @return a valid pointer to found arc if an arc satisfies the
      criteria or `nullptr` otherwise

      @see exists_arc()
   */
  template <class Op>
  Arc * search_arc(Op & op) const
  {
    for (typename GT::Arc_Iterator it(*const_me()); it.has_curr(); it.next_ne())
      {
	auto a = it.get_curr_ne();
	if (op(a))
	  return a;
      }
    return nullptr;
  }

  /// \overload search_arc(Op & op) 
  template <class Op>
  Arc * search_arc(Op && op) const
  {
    return search_arc(op);
  }

  /** Find an arc mathing a content.

      `find_arc(info)`, which is a wrapper to `search_arc()`, searches a
      arc whose `get_info()` result matches with the `info`
      parameter. The comparison is done via de operator `==` on the
      class `Arc_Type`, which is usually the case.

      \param[in] info to be compared with arcs content
      \return a valid pointer to a arc if there is one mathing `info`
      or `nullptr` otherwise.
  */
  Arc * find_arc(const Arc_Type & info) const noexcept
  {
    return search_arc([&info] (auto a) { return a->get_info() == info; });
  }
  
  /** Linear search of an arc.

      Search linearly an arc satisfying the condition `op`.

      This method is some similar to `exists_arc()`; the difference is
      that it returns a pointer and its value indicates the success of
      search.

      The complexity is \f$O(V)\f$ for the worst case.

      \param[in] p pointer to the node from which the adjacent arcs want
      to be searched. 
      \param[in] op the criteria.
      @return a valid pointer to found node if a node satisfies the
      criteria or `nullptr` otherwise

      @see exists_arc(Node * p, Operation & op)
   */
  template <class Operation>
  Arc * search_arc(Node * p, Operation & op) const
  { 
    for (typename GT::Node_Arc_Iterator it(p); it.has_curr(); it.next_ne())
      {
	Arc * arc = it.get_curr_ne();
	if (op(arc))
	  return arc;
      }
    return nullptr;
  }

  /// \overload search_arc(Node * p, Operation & op)
  template <class Operation>
  Arc * search_arc(Node * p, Operation && op = Operation()) const
  {
    return search_arc(p, op);
  }

  /** Search an arc linking two nodes.

      `search_arc(src, tgt)` searches an arc linking `src` with
      `tgt`. The method inspect the ars adjacent to `src` verifying
      whether a target is or not `tgt`.

      This method is conceived for both directed and non directed
      graphs. But in the not directed case, the notion of source and
      target node has not much sense. The result of `search_node(src,
      tgt)` is the same than `search_node(tgt, src)`.

      \note If you require search arcs in directed graphs, the use
      `search_directed()`

      The complexity is $\f$O(V)\f$ worst case.

      \param[in] src source node
      \paran[in] tgt target node
      \return a valid pointer to an arc if this was found or `nullptr`
      otherwise. 

      \see search_directed_arc()
   */
  Arc * search_arc(Node * src, Node * tgt) const noexcept
  {
    for (typename GT::Node_Arc_Iterator it(src); it.has_curr(); it.next_ne())
      if (it.get_tgt_node_ne() == tgt)
	return it.get_curr_ne();
    return nullptr;
  }

  /** Return a container with all the nodes of the graph.
      
      `nodes()` recollects all the nodes of the graph and builds a
      container with all them inserted. The container type is a template
      parameter, which by default is `DynList`.

      \return a container, according the template parameter, with all
      the nodes into it.
      \throw bad_alloc if there is no enough memory
   */
  template <template <typename> class Container = Aleph::DynList>
  Container<Node*> nodes() const
  {
    Container<Node*> ret;
    for_each_node([&ret] (Node * p) { ret.append(p); });
    return ret;
  }

  /** Return a container with all the arcs of the graph.
      
      `arcs()` recollects all the arcs of the graph and builds a
      container with all them inserted. The container type is a template
      parameter, which by default is `DynList`.

      \return a container, according the template parameter, with all
      the arcs into it.
      \throw bad_alloc if there is no enough memory
   */
  template <template <typename> class Container = Aleph::DynList>
  Container<Arc*> arcs() const
  {
    Container<Arc*> ret;
    for_each_arc([&ret] (Arc * a) { ret.append(a); });
    return ret;
  }

  /** Return a container with all the arcs adjacent to a node.
      
      `arcs()` recollects all the arcs adjacent to node `p` and builds a
      container with all them inserted. The container type is a template
      parameter, which by default is `DynList`.

      \return a container, according the template parameter, with all
      the arcs into it.
      \throw bad_alloc if there is no enough memory
   */
  template <template <typename> class Container = Aleph::DynList>
  Container<Arc*> arcs(Node * p) const
  {
    Container<Arc*> ret;
    this->for_each_arc(p, [&ret] (Arc * a) { ret.append(a); });
    return ret;
  }

  /** Obtains an iterator to the nodes of graph.

      This method is useful for shorting the iterator initialization. A
      traditional way for writing an iterator is as follows:

          for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
	    operation on current node

      With get_node_it() The previous snippet could be more shortly written
      thus:

          for (auto it = g.get_node_it(); it.has_curr(); it.next())
	    operation on current node

      @return an instantiated iterator on the nodes positioned at the
      first node.
  */
  auto get_node_it() const noexcept 
  {
    return typename GT::Node_Iterator(*const_me()); 
  }

  /** Obtains an iterator to the arc of graph.

      This method is useful for shorting the iterator initialization. A
      traditional way for writing an iterator is as follows:

          for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next())
	    operation on current arc

      With get_arc_it() The previous snippet could be more shortly written
      thus:

          for (auto it = g.get_arc_it(); it.has_curr(); it.next())
	    operation on current arc

      @return an instantiated iterator on the arcs positioned at the
      first arc.
  */
  auto get_arc_it() const noexcept
  {
    return typename GT::Arc_Iterator(*const_me()); 
  }

  /** Obtains an iterator to the adjacent arcs of a node.

      This method is useful for shorting the iterator initialization. A
      traditional way for writing an iterator on the arcs of a node is
      as follows: 

          for (typename GT::Node_Arc_Iterator it(g); it.has_curr(); it.next())
	    operation on current arc

      With get_arc_it(p) The previous snippet could be more shortly written
      thus:

          for (auto it = g.get_arc_it(p); it.has_curr(); it.next())
	    operation on current arc

      @return an instantiated iterator on the arcs positioned at the
      first arc.
  */
  auto get_arc_it(Node * p) const noexcept
  {
    return typename GT::Node_Arc_Iterator(p); 
  }

  /** Filter for input arcs of a node.

      This class is intended to be used in a arc iterator for only
      shwoing the input arcs.

      When an arc is created, the source and target nodes are
      specified. For example:

          auto a = g.insert_arc(s, t)

      creates an arc linking the nodes `s` and `t`. If `g` is a directed
      graph, then the notion of source and target has much sense. This
      sense in esential. In this case \f$s \longrightarrow t \ neq t
      \longrightarrow s\f$. The classes `List_Digraph`, `List_SDigraph`
      and `Array_Digraph` were conceived for explicit modeling of
      directed graphs.

      However, sometimes is very useful to consider the sense
      \f$s \longrightarrow t\f$ i graph classes that do not explicitely
      model the sense. The `In_Filt` filter used in combination with a
      filter iterator on arcs, is used for filtering arcs whose target is
      the node on the which one is iterating. 

      This class in intensively used in `Aleph-w` (\f$\aleph_\omega\f$)
      network flows, which, in order to directly have the residual net,
      use non-directed graphs instead of directed ones.

      Normally you must not worry by the existence of this class. It is
      transparently used by the class `In_Iterator`

      @ingroup Grafos
   */
  struct In_Filt
  {
    Node * tgt = nullptr; /// target node of iteration

    /// Buld a filter on input arcs to arc `__tgt`
    In_Filt(Node * __tgt = nullptr) noexcept : tgt(__tgt) { /* empty */ }

    /// Return true if the arc `a` is incoming arc to `tgt`; `false`
    /// otherwise
    bool operator () (Arc * a) const noexcept
    {
      assert(tgt);
      return a->tgt_node == tgt;
    }

    ///  Return the source node of arc `a`
    Node * get_node(Arc * a) const noexcept
    {
      assert(tgt);
      return (typename GT::Node *) a->src_node;
    }
  };

  /** Filter for output arcs of a node.

      This class is intended to be used in a arc iterator for only
      shwoing the output arcs.

      When an arc is created, the source and target nodes are
      specified. For example:

          auto a = g.insert_arc(s, t)

      creates an arc linking the nodes `s` and `t`. If `g` is a directed
      graph, then the notion of source and target has an esential
      sense. In this case \f$s \longrightarrow t \ neq t \longrightarrow
      s\f$. The classes `List_Digraph`, `List_SDigraph` and
      `Array_Digraph` were conceived for explicit modeling of directed
      graphs.

      However, sometimes is very useful to consider the sense
      \f$s \longrightarrow t\f$ i graph classes that do not explicitely
      model the sense. The `Out_Filt` filter used in combination with a
      filter iterator on arcs, is used for filtering arcs whose source
      is connected to the node on the which one is iterating. 

      This class in intensively used in `Aleph-w` (\f$\aleph_\omega\f$)
      network flows, which, in order to directly have the residual net,
      use non-directed graphs instead of directed ones.

      Normally you must not worry by the existence of this class. It is
      transparently used by the class `Out_Iterator`

      @ingroup Grafos
   */
  struct Out_Filt
  {
    Node * src = nullptr; /// source node of iteration

    /// Build a output filter of arc outcoming from `__src`
    Out_Filt(Node * __src) noexcept : src(__src) { /* empty */ }

    /// Return `true` if `a` is a outcoming arc from `src`; `false`
    /// otherwise
    bool operator () (Arc * a) const noexcept
    {
      assert(src);
      return a->src_node == src;
    }

    /// Return the source node of arc `a` (whose target is `tgt`)
    Node * get_node(Arc * a) const noexcept
    {
      assert(src);
      return (Node *) a->tgt_node;
    }
  };

  /** Special iterator for distinguishing input arcs of output ones.

      The intended use of this iterator is to traverse the ars linked to
      a node `p` inserted in a non-directed graph and to filter those
      arcs that are incoming or outcoming respect to `p`.

      The in/out feature is given by the filter. 

      For example, for iterating on the input arcs of node `p`, we can do:

          for (GT::Digraph_Iterator<In_Filt> it(p); it.has_curr(); it.next())
	    auto a = it.get_curr(); // this arc is incoming q --> p

      Although this iterator is exclusivelly conceived for dealing with
      incoming or outcoming arcs. it could be used as filtered
      iterator. That is, you could to configure the filter for filtering
      arcs according your own criteria. In this case, we recommend to
      use the aliased version Filter_Iterator.

      Apart from the fact that some arcs are filtered, the interface is
      identical to the `Aleph-w` (\f$\aleph_\omega\f$) iterators.

      @see Out_Iterator In_Iterator Filter_Iterator
      @ingroup Grafos
   */
  template <class Filter>
  class Digraph_Iterator
  {
    using Itor = Filter_Iterator<Node*, typename GT::Node_Arc_Iterator, Filter>;

    Filter filt;
    Itor it;

  public:

    using Item_Type = typename Itor::Item_Type; /// the type of items (Arc*)

    using Iterator_Type = Itor;

    /// Instantiate an filtered iterator for arcs on the node `p`
    Digraph_Iterator(Node * p) : filt(p), it(p, filt) 
    {
      // empty
    }

    void next_ne() noexcept { it.next_ne(); }

    /// Advance to next arc. Throw `overflow_error` if there is no
    /// current arc
    void next() { it.next(); }

    /// back to previous item. Throw `underflow_error` if there is no
    /// current arc
    void prev() { it.prev(); }

    void prev_ne() { it.prev_ne(); }

    /// Return true is the iterator has a current arc
    bool has_curr() const noexcept { return it.has_curr(); }

    /// Return the current arc. Throws `overflow_error` if there is no
    /// current arc
    typename GT::Arc * get_curr() const { return it.get_curr(); }

    typename GT::Arc * get_curr_ne() const noexcept { return it.get_curr_ne(); }

    /// \overload get_curr()
    auto get_current_arc() const { return get_curr(); }

    auto get_current_arc_ne() const noexcept { return get_curr_ne(); }
	
    /// Return the node connected to `p` (passed during construction)
    /// and linked through `a`
    typename GT::Node * get_node(typename GT::Arc * a) const noexcept
    {
      return filt.get_node(a);
    }

    /// Return the node connected to `p` (passed during construction)
    /// and linked through the current arc
    typename GT::Node * get_node_ne() const noexcept
    { 
      return this->get_node(this->get_curr_ne()); 
    }

    /// \overload get_node()
    auto get_tgt_node_ne() const noexcept { return get_node_ne(); }

    typename GT::Node * get_node() const 
    { 
      return this->get_node(this->get_curr()); 
    }

    /// \overload get_node()
    auto get_tgt_node() const { return get_node(); }

    /// Reset the iterator to first arc
    void reset_first() noexcept { it.reset_first(); }

        /// Reset the iterator to last arc
    void reset_last() noexcept { it.reset_last(); }
  };

  /** Alias for `Digraph_Iterator` */
  template <class Filt> using Filter_Iterator = Digraph_Iterator<Filt>;

  /** Iterator on incoming arcs of node */
  using In_Iterator = Digraph_Iterator<In_Filt>;

  /** Iterator on incoming arcs of node */
  using Out_Iterator = Digraph_Iterator<Out_Filt>;

  /** Return an input iterator on the incoming arcs to `p`

      This method is useful for shorting an input iterator
      initialization. A traditional way for writing an input iterator on
      the arcs of a node is as follows:

          for (typename GT::In_Iterator it(g); it.has_curr(); it.next())
	    operation on current arc

      With get_in_it(p) The previous snippet could be more shortly written
      thus:

          for (auto it = g.get_in_it(p); it.has_curr(); it.next())
	    operation on current arc

      @return an instantiated input iterator on the arcs positioned at the
      first arc.
  */
  In_Iterator get_in_it(Node * p) const noexcept { return In_Iterator(p); }

  /** Return an output iterator on the incoming nodes to `p`

      This method is useful for shorting an output iterator
      initialization. A traditional way for writing an output iterator on
      the arcs of a node is as follows:

          for (typename GT::Out_Iterator it(g); it.has_curr(); it.next())
	    operation on current arc

      With get_out_it(p) The previous snippet could be more shortly written
      thus:

          for (auto it = g.get_out_it(p); it.has_curr(); it.next())
	    operation on current arc

      @return an instantiated output iterator on the arcs positioned at the
      first arc.
  */
  Out_Iterator get_out_it(Node * p) const noexcept { return Out_Iterator(p); }

  /** Search a directed arc linking two nodes.

      `search_directed_arc(src, tgt)` searches an arc of form `src --> tgt`,
      on bot directed and non directed graphs.

      @param[in] src pinter to source node
      @param[in] tgt pinter to target node
      \return a valid pointer to an arc if this is found or `nullptr`
      otherwise. 
   */
  Arc * search_directed_arc(Node * src, Node * tgt) const noexcept
  {
    for (typename GT::Out_Iterator it(src); it.has_curr(); it.next_ne())
      if (it.get_tgt_node() == tgt)
	return it.get_curr_ne();
    return nullptr;
  }

  /** Return a list with the incoming nodes to `p`

      The method filters all the incoming arcs `s --> p` and return their
      nodes.

      \param[in] p pointer to node
      \return a list containing the incoming nodes. It could be empty if
      p is a source node (not have input arcs).
      \throw bad_alloc if there is no enough memory
  */
  DynList<Node*> in_nodes(Node * p) const
  {
    DynList<Node*> ret;
    for (In_Iterator it(p); it.has_curr(); it.next_ne())
      ret.append(it.get_node());
    return ret;
  }

  /** Return a list with the outcoming nodes to `p`

      The method filters all the outcoming arcs `p --> t` and return their
      nodes.

      \param[in] p pointer to node
      \return a list containing the outcoming nodes. It could be empty if
      p is a sink node (not have ouytut arcs).
      \throw bad_alloc if there is no enough memory
  */
  DynList<Node*> out_nodes(Node * p) const
  {
    DynList<Node*> ret;
    for (Out_Iterator it(p); it.has_curr(); it.next_ne())
      ret.append(it.get_node_ne());
    return ret;
  }

  /** Return a list with the outcoming arcs to p`

      The method returns all the outcoming arcs `p --> t`.
      
      \param[in] p pointer to node
      \return a list containing the outcoming arcs. It could be empty if
      p is a sink node (not have ouytut arcs).
      \throw bad_alloc if there is no enough memory
  */
  DynList<Arc*> out_arcs(Node * p) const
  {
    DynList<Arc*> ret;
    for (Out_Iterator it(p); it.has_curr(); it.next_ne())
      ret.append(it.get_curr_ne());
    return ret;
  }

  /** Return a list with the incoming arcs to p`

      The method returns all the incoming arcs `s --> p`.

      \return a list containing the incoming arcs. It could be empty if
      p is a sink node (not have ouytut arcs).
      \throw bad_alloc if there is no enough memory
  */
  DynList<Arc*> in_arcs(Node * p) const
  {
    DynList<Arc*> ret;
    for (In_Iterator it(p); it.has_curr(); it.next_ne())
      ret.append(it.get_curr_ne());
    return ret;
  }

  /// Pair of arc and node (topologically related)
  using ArcPair = tuple<Arc*, Node*>;

  /** Return a list of pair incoming arcs and nodes.

      Sometimes is useful to directly have the incoming arcs and their
      nodes. This method perform that task in only a pass.

      The pair are implemented with tuples. First item has the arc and
      the second the node.

      \param[in] p pointer to node
      \return a list of incoming pairs arc, node 
      \throw bad_alloc if there is no enough memory
   */
  auto in_pairs(Node * p) const
  {
    DynList<ArcPair> ret;
    for (In_Iterator it(p); it.has_curr(); it.next_ne())
      {
	auto a = it.get_curr_ne();
	ret.append(make_tuple(a, (Node*) a->get_connected_node(p)));
      }
    return ret;
  }

  /** Return a list of pair outcoming arcs and nodes.

      Sometimes is useful to directly have the outcoming arcs and their
      nodes. This method perform that task in only a pass.

      The pair are implemented with tuples. First item has the arc and
      the second the node.

      \param[in] p pointer to node
      \return a list of outcoming pairs arc, node 
      \throw bad_alloc if there is no enough memory
   */
  auto out_pairs(Node * p) const
  {
    DynList<ArcPair> ret;
    for (Out_Iterator it(p); it.has_curr(); it.next_ne())
      {
	auto a = it.get_curr_ne();
	ret.append(make_tuple(a, (Node*) a->get_connected_node(p)));
      }
    return ret;
  }

  /** Compute the input degree of a node.

      The method computes, not retrieves, the number of incoming arcs to
      node `p`. Be careful with the fact that this is a computation and
      to perform very often could degrade the performance.

      Note that this method does not only serve too for explicit
      digraphs, but that would be necessary, since the in degree is
      neves stored.

      \param[in] p poinger to the node
      @return number of incoming arcs to `p`
  */
  size_t in_degree(Node * p) const noexcept
  {
    size_t count = 0;
    for (In_Iterator it(p); it.has_curr(); it.next_ne())
      ++count;
    return count;
  }
  
  /** Compute the output degree of a node.

      The method computes, not retrieves, the number of outcoming arcs to
      node `p`. Be careful with the fact that this is a computation and
      to perform very often could degrade the performance.

      Note that if you are using a digraph, then it is not use to
      have this method, since the digraph stores the outcoming
      degree. In this case, you could use `get_num_arcs(p)` in order to
      obtain the same result wihout any computation

      \param[in] p poinger to the node
      @return number of incoming arcs to `p`
      \see get_num_arcs(Node*)
  */
  size_t out_degree(Node * p) const noexcept
  {
    size_t count = 0;
    for (Out_Iterator it(p); it.has_curr(); it.next_ne())
      ++count;
    return count;
  }

  /** Traverse of arcs of a node according to specific arcs iterator.

      `traverse_arcs<Itor, Operation>(p, operation)` is very similar a
      `traverse_arcs()`, except that this version receives the arc
      iterator as template parameter. Its use is intended to serve as
      implementation base for functional primitives on incoming and
      outcoming arcs.

      \param[in] p node to traverse its arcs
      \param[in] operation to perform on each arc given by the
      iterator. The convention is the the same than for `traverse`
      methods. If `operation`return `true`, then the next arc is
      revised; otherwise the traversal stops.
      \return `true` if all the arcs were traversed; `false` otherwise.
      \throw whatever could throw `operation`

      \see traverse(Node * p, Operation & operation)
   */
  template <class Itor, class Operation> 
  bool traverse_arcs(Node * p, Operation & op) const
  {
    for (Itor it(p); it.has_curr(); it.next_ne())
      if (not op(it.get_curr_ne()))
	return false;
    return true;
  }

  /// Perform `operation` on each arc of node `p`
  template <class Itor, class Operation>
  void for_each_arc(Node * p, Operation & op) const
  {
    for (Itor it(p); it.has_curr(); it.next_ne())
      operation(it.get_curr_ne());
  }

  /// Traverse the incoming arcs of node `p` executing the conditioned
  /// `operation` 
  template <class Op>
  bool traverse_in_arcs(Node * p, Op & op) const
  {
    return traverse_arcs<In_Iterator, Op>(p, op);
  }

  /// \overload traverse_in_arcs(Node * p, Op & op)
  template <class Op>
  bool traverse_in_arcs(Node * p, Op && op = Op()) const
  {
    return traverse_in_arcs(p, op);
  }

  /// Perform `operation` on each incoming arc of node `p`
  template <class Op>
  void for_each_in_arc(Node * p, Op & op) const
  {
    for_each_arc(p, op); 
  }

  // \overload for_each_in_arc(Node * p, Op & op)
  template <class Op>
  void for_each_in_arc(Node * p, Op && op = Op()) const
  {
    for_each_in_arc(p, op);
  }

  /// Return `true` if `op` is true for all the incoming arcs to node `p`
  template <class Op>
  bool all_in_arcs(Node * p, Op & op) const
  {
    return traverse_in_arcs(p, [&op] (auto a) { return op(a); });
  }

  /// \overload all_in_arcs(Node * p, Op & op)
  template <class Op>
  bool all_in_arcs(Node * p, Op && op = Op()) const
  {
    return all_in_arc(p, op);
  }

  /// Return true if it exists a incoming arc to `p` returning `true`
  /// for `op` 
  template <class Op>
  bool exists_in_arc(Node * p, Op & op) const
  {
    return not traverse_in_arcs(p, [&op] (auto a) { return not op(a); });
  }

  /// \overload exists_in_arc(Node * p, Op & op)
  template <class Op>
  bool exists_in_arc(Node * p, Op && op = Op()) const
  {
    return exists_in_arc(p, op);
  }

  /** Search an incoming arc to a node satisfaying a condition.

      `search_in_arc(p, op)` traveses the incoming arcs of `p` and
      search one for which `op` return `true`.

      \param[in] p pointer to the node.
      \param[in] op operation to be executed on each arc in order to
      match certain search criteria. `op` must return `true` when the
      arc satisfies the search criteria.
      \return a valid pointer to arc satisfaying the search criteria it
      this exists; `nullptr` otherwise 
      \throw anything that `op` could throw
   */
  template <class Op>
  auto search_in_arc(Node * p, Op & op) const
  {
    Arc * ret = nullptr;
    traverse_in_arcs(p, [&op, &ret] (auto a)
		     {
		       if (op(a))
			 {
			   ret = a;
			   return false;
			 }
		       return true;
		     });
    return ret;
  }

  /// \overload search_in_arc(Node * p, Op & op)
  template <class Op>
  auto search_in_arc(Node * p, Op && op = Op()) const
  {
    return search_in_arc(p, op);
  }

  /** Return a list of incoming arcs of a node mapped to items of type
      given by transformation `op`.

      This method is analogous to `arcs_map()` but only operates on the
      incoming arcs.

      \see arcs_map(Node * p, std::function<T(Arc *)> operation)
  */
  template <typename T>
  auto in_arcs_map(Node * p, std::function<T(Arc*)> op) const
  {
    DynList<T> ret;
    for_each_in_arc(p, [&ret, &op] (auto a) { ret.append(op(a)); });
    return ret;
  }

  /** Fold the incoming arcs of a node.

       This method is analogous to `foldl_arcs()` but only operates on the
      incoming arcs.

      \see foldl_arcs(Node * p, const T & init, std::function<T(Arc *)> operation)
   */
  template <typename T = Arc_Type>
  T foldl_in_arcs(Node * p, const T & init, 
		  std::function<T(const T&, Arc*)> op) const
  {
    T ret = init;
    for_each_in_arc(p, [&ret, &op] (auto a) { ret = op(ret, a); });
    return ret;
  }

   /** Filter the incoming arcs of a node.

       This method is analogous to `filter_arcs()` but only operates on the
       incoming arcs.

      \see filter_arcs(Node * p, Operation & operation)
   */
  template <class Op>
  DynList<Arc*> filter_in_arcs(Node * p, Op & op) const 
  {
    DynList<Arc*> ret;
    for_each_in_arc(p, [&ret, &op] (auto a) 
		    {
		      if (op(a))
			ret.append(a);
		    });
    return ret;
  }

  /// \overload filter_in_arcs(Node * p, Op & op)
  template <class Op>
  auto filter_in_arcs(Node * p, Op && op = Op()) const
  {
    return filter_in_arcs(p, op);
  }

  /// Traverse the outcoming arcs of node `p` executing the conditioned
  /// `operation` 
  template <class Op>
  bool traverse_out_arcs(Node * p, Op & op) const
  {
    return traverse_arcs(p, op);
  }

  /// \overload traverse_out_arcs(Node * p, Op & op)
  template <class Op>
  bool traverse_out_arcs(Node * p, Op && op = Op()) const
  {
    return traverse_out_arcs(p, op);
  }

  /// Perform `operation` on each incoming arc of node `p`
  template <class Op>
  void for_each_out_arc(Node * p, Op & op) const
  {
    for_each_arc(p, op);
  }

  /// \overload for_each_out_arc(Node * p, Op & op)
  template <class Op>
  void for_each_out_arc(Node * p, Op && op = Op()) const
  {
    for_each_out_arc(p, op);
  }

  /// Return `true` if `op` is true for all the outcoming arcs to node `p`
  template <class Op>
  bool all_out_arcs(Node * p, Op & op) const
  {
    return traverse_out_arcs(p, [&op] (auto a) { return op(a); });
  }

  /// \overload all_out_arcs(Node * p, Op & op)
  template <class Op>
  bool all_out_arcs(Node * p, Op && op = Op()) const
  {
    return all_out_arc(p, op);
  }

  /// Return true if it exists a outcoming arc to `p` returning `true`
  /// for `op` 
  template <class Op>
  bool exists_out_arc(Node * p, Op & op) const
  {
    return not traverse_out_arcs(p, [&op] (auto a) { return not op(a); });
  }

  /// \overload exists_out_arc(Node * p, Op & op)
  template <class Op>
  bool exists_out_arc(Node * p, Op && op = Op()) const
  {
    return exists_out_arc(p, op);
  }

  /** Search an outcoming arc to a node satisfaying a condition.

      `search_out_arc(p, op)` traveses the outcoming arcs of `p` and
      search one for which `op` return `true`.

      \param[in] p pointer to the node.
      \param[in] op operation to be executed on each arc in order to
      match certain search criteria. `op` must return `true` when the
      arc satisfies the search criteria.
      \return a valid pointer to arc satisfaying the search criteria it
      this exists; `nullptr` otherwise 
      \throw anything that `op` could throw
  */  template <class Op>
  auto search_out_arc(Node * p, Op & op) const
  {
    typename GT::Arc * ret = nullptr;
    traverse_out_arcs(p, [&op, &ret] (auto a)
		      {
			if (op(a))
			  {
			    ret = a;
			    return false;
			  }
			return true;
		      });
    return ret;
  }

  /// \overload search_out_arc(Node * p, Op & op)
  template <class Op>
  auto search_out_arc(Node * p, Op && op = Op()) const
  {
    return search_out_arc(p, op);
  }

  /** Return a list of outcoming arcs of a node mapped to items of type
      given by transformation `op`.

      This method is analogous to `arcs_map()` but only operates on the
      outcoming arcs.

      \see arcs_map(Node * p, std::function<T(Arc *)> operation)
  */
  template <typename T = Arc_Type>
  auto out_arcs_map(Node * p, std::function<T(Arc*)> op) const
  {
    DynList<T> ret;
    for_each_out_arc(p, [&ret, &op] (auto a) { ret.append(op(a)); });
    return ret;
  }

  /// \overload out_arcs_map(Node * p, std::function<T(Arc*)> op)
  template <typename T = Arc_Type>
  T foldl_out_arcs(Node * p, const T & init,
		   std::function<T(const T&, Arc*)> op) const
  {
    T ret = init;
    for_each_out_arc(p, [&ret, &op] (auto a) { ret = op(ret, a); });
    return ret;
  }

  /** Filter the outcoming arcs of a node.

      This method is analogous to `filter_arcs()` but only operates on the
      outcoming arcs.

      \see filter_arcs(Node * p, Operation & operation)
  */
  template <class Op>
  DynList<Arc*> filter_out_arcs(Node * p, Op & op) const
  {
    DynList<Arc*> ret;
    for_each_out_arc(p, [&ret, &op] (auto a) 
		     {
		       if (op(a))
			 ret.append(a);
		     });
    return ret;
  }

  /// \overload filter_out_arcs(Node * p, Op & op)
  template <class Op>
  auto filter_out_arcs(Node * p, Op && op = Op()) const
  {
    return filter_out_arcs(p, op);
  }
};



# endif // GRAPH_DRY_H
