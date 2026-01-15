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

/** @file aleph-graph.H
 *  @brief Simplified graph interface for common use cases.
 *
 *  Provides AlephGraph, a simplified graph class that wraps
 *  the more complex List_Graph with easier-to-use methods
 *  and automatic memory management.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef ALEPH_GRAPH_H
# define ALEPH_GRAPH_H

# include <cassert>
# include <cstring>
# include <stdexcept>
# include <memory>
# include <string>

namespace Aleph {


static const long No_Visited = 0; // nodo o arco no ha sido visitado

  /** Bit numbers of nodes or arcs.

      Nodes y arcs of graph have as control attributes (internal
      representation of state), a set of bits. These are their numbers
      named according to their use by the library.

      You can use then for purposes other than the suggested
      name. However, be careful.

      @see Bit_Fields

      @ingroup Graphs
   */
enum Graph_Bits
{
  Depth_First,
  Breadth_First,
  Test_Cycle,
  Find_Path,
  Euler,
  Maximum_Flow,
  Spanning_Tree,
  Build_Subtree,
  Convert_Tree,
  Cut,
  Min,

  Num_Bits_Graph
};


  /** The node or arc has already been processed
      @ingroup Graphs
  */
  extern const unsigned char Processed; 

  /** The node are being processed; probably it is inside a queue, stack
      or heap 

      @ingroup Graphs
  */
  extern const unsigned char Processing;

  /** The node have not bees processed. This must be the initial state
      before general processing 

      @ingroup Graphs
  */
  extern const unsigned char Unprocessed;


    /** Bit fields for nodes and arcs used for marking visit state
	during processing.

	Each node and arc of a graph contains a bitmask intended to mark
	visit state during the execution of an algorithm. The bits are
	named according to their use by the library.

	If your you intend to use some of these bits for your own
	processing, then make sure of two things:
	
	  1. Use at the most the eight first bits. Do no use other
	     because they could be removed in future releases.

	  2. Make sure that the bit that you use will no be used by an
	     algorithm that you invoke

        @note For some reason not yet clearly understood, the bit fields
        copy by memcpy and other relatives does not work properly on
        some systems. Possibly this would only happen on little-endian
        systems. Therefore, do not use these mechanisms.

        @ingroup Graphs
     */
class Bit_Fields
{
public:

  unsigned int depth_first    : 1; /// Depth first search
  unsigned int breadth_first  : 1; /// Breadth first search
  unsigned int test_cycle     : 1; /// Cycle existence test
  unsigned int find_path      : 1; /// Path searching (there are several types)
  unsigned int euler          : 1; /// Used during eulerian searching
  unsigned int maximum_flow   : 1; /// Used by the maximum flow algorithms
  unsigned int spanning_tree  : 1; /// Used by spannign tree algorithms
  unsigned int build_subtree  : 1; /// Used by subtree or subgraph building
  unsigned int convert_tree   : 1; /// Used for Tree_Node conversion
  unsigned int cut            : 1; /// Used for cut points computing
  unsigned int min            : 1; /// Used for min path or min spanning
				   /// tree
  /** Visit state.

      Note that there are two bits. So you can manage 4 different
      states.
  */
  unsigned int state          : 2; 

      /// All the bits are set to zero
  Bit_Fields() noexcept
  {
    memset(this, 0, sizeof *this);
  }

      /** Get a control bit.

	  It is recommended to use the bit name. By example

	      NODE_BITS(p).get_bit(Breadth_First)

          @param[in] bit number (or name) of the bit to be read
          @return the logical value of bit
          @throw out_of_range if bit is greater or equal than Num_Bits_Graph.
       */
  [[nodiscard]] bool get_bit(int bit) const noexcept
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
      default: assert(false);
      }
    return false;
  }

      /** Set a control bit.

          Set a specific control bit to zero or one.

          The real value of value is not verified. Anything different
          than zero will be considered as 1.

          @param[in] bit to set
          @param[in] value the value to set
          @throw out_of_range if bit is greater or equal than Num_Bits_Graph.
       */
  void set_bit(int bit, int value) noexcept
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
      default: assert(false);
      }
  }

  /// Return the state value
  [[nodiscard]] unsigned int get_state() const noexcept { return state; }

  /// Return a stringification version of state
  [[nodiscard]] std::string str_state() const
  {
    switch (state)
      {
      case 0: return {"Unprocessed"};
      case 1: return {"Processing"};
      case 2: return {"Processed"};
      default: break;
      }
    return {"Undefined"};
  }

  /// Set the state to the value `s`
  void set_state(unsigned char s) noexcept
  {
    assert(s < 4);
    state = s;
  }

      /// Reset `bit` to zero
  void reset(int bit) noexcept { set_bit(bit, 0); }

      /// Reset all bits and state to zero
  void reset() noexcept
  {
    memset(this, 0, sizeof *this);
  }
};


/** General attributes for nodes and arc of graphs.

    Each node and arc of a `Aleph-w` graph manages three fixed
    attributes:

      1. `control_bits`: a field of type Bit_Fields for storing little
         state.
      2. `counter`: a `long` allowing to represent a much more wider
         state (colors, visit order, etc)
      3. `cookie`: a `cookie` is an opaque `void` pointer. Thes pointer
         could be used in order to associate to the node or arc
         state wider or different beyond the possibilities allowed by
         the bit fields and the counter. The three main uses of this
         pointer are:

         - For mapping nodes and/or arcs to other isomorphic
            subgraphs. By example, you could compute a spanning tree of
            the graph. Now, in order to know which node or arc of
            spanning tree corresponds to its counterpart in the original
            graph, you could use this `cookie` and to perform an
            map. This has the great advantage that the map is
            deterministically \f$O(1)\f$, by contrast with the
            probabilistic \f$O(1)\f$ of a hash table or the \f$O(\lg
            n)\f$ of a binary search tree. In addition, this
            alternative, when required, is less space expensive.

         - If you need to associate to the node or arc a more
            sophisticated state, the you could define it with a class
	    (or tuple or whatever else you need), and then allocate it
	    and storing via this `cookie` pointer. Of course, be careful
	    and be sure of deallocating it when it is not longer
	    needed. Also, consider that you will need to cast the
	    `cookie` to the type of information that you have defined.

         - Reverse path tracing. This is a very useful and powerful
	    technique for storing partial and definitive path during
	    graph processing. The general rule is: when in a search you
	    visit a new node, the you saves the parent (the caller) in
	    the cookie. Thus, when you finish the search, the path
	    through the cookies is the revessed path from the the end
	    node to the start node.

    @see Bit_Fields NODE_BITS NODE_COUNTER NODE_COLOR IS_NODE_VISITED NODE_COOKIE ARC_COUNTER ARC_COLOR ARC_BITS IS_ARC_VISITED ARC_COOKIE

    @ingroup Graphs
 */
struct Graph_Attr
{
  Bit_Fields control_bits; 
  long       counter = 0;
  void *     cookie = nullptr; 
  
      /// Reset all attributes to their default value
  void reset()
  {
    control_bits.reset();
    counter = 0;
    cookie = nullptr;
  }  
};


    /** Get the control bits of a node.

        @ingroup Graphs
     */
# define NODE_BITS(p)            ((p)->attrs.control_bits)

    /** Get the counter of a node.

        @ingroup Graphs
    */
# define NODE_COUNTER(p)         ((p)->attrs.counter)

    /** Synonymous of NODE_COUNTER

        @ingroup Graphs
    */
# define NODE_COLOR(p)         ((p)->attrs.counter)

    /** Determine whether the control bit is set or not to one

        @param p pointer to the node
        @param bit number of bit to read
        @return true if the bit is 1; false if 0

        @ingroup Graphs
    */
# define IS_NODE_VISITED(p, bit) (NODE_BITS(p).get_bit(bit))

    /** Return the node `cookie`

        @ingroup Graphs
    */
# define NODE_COOKIE(p)          ((p)->attrs.cookie) 

    /** Return the `counter` of arc p

        @ingroup Graphs
    */
# define ARC_COUNTER(p)          ((p)->attrs.counter)

    /** Return the color of arc p 

        @ingroup Graphs
    */
# define ARC_COLOR(p)          ((p)->attrs.counter)

    /** Return the control bits of arc p

        @ingroup Graphs
    */
# define ARC_BITS(p)             ((p)->attrs.control_bits)

    /** Determine whether the `bit` field is or not set to one

        @param p pointer to arc
        @param bit number of bit to be read
        @return true if `bit` is 1; false otherwise (`bit` is set to 0)
        @ingroup Graphs
    */
# define IS_ARC_VISITED(p, bit)  (ARC_BITS(p).get_bit(bit))

    /** Return the arc `cookie`

        @ingroup Graphs
    */
# define ARC_COOKIE(p)           ((p)->attrs.cookie) 



} // end namespace Aleph





# endif
