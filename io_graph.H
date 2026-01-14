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

/** @file io_graph.H
 *  @brief Graph serialization and deserialization utilities.
 *
 *  This file provides the IO_Graph class for reading and writing graphs
 *  to/from files in binary and text formats. The serialization format
 *  is customizable through functor template parameters.
 *
 *  ## Features
 *
 *  - **Binary mode**: Fast, compact representation for production use
 *  - **Text mode**: Human-readable format for debugging and interchange
 *  - **Customizable serializers**: Define custom functors for node/arc data
 *  - **Filtering**: Save only nodes/arcs that pass filter predicates
 *  - **Verbose mode**: Optional progress output during I/O operations
 *
 *  ## File Format
 *
 *  ### Binary Format
 *  ```
 *  [num_nodes: size_t]
 *  [node_0 data] [node_1 data] ... [node_n-1 data]
 *  [num_arcs: size_t]
 *  [src_idx: int] [tgt_idx: int] [arc_0 data]
 *  [src_idx: int] [tgt_idx: int] [arc_1 data]
 *  ...
 *  ```
 *
 *  ### Text Format
 *  ```
 *  num_nodes
 *  num_arcs
 *  node_0_data
 *  node_1_data
 *  ...
 *  src_idx tgt_idx arc_0_data
 *  src_idx tgt_idx arc_1_data
 *  ...
 *  ```
 *
 *  ## Usage Example
 *
 *  @code{.cpp}
 *  #include <io_graph.H>
 *  #include <tpl_graph.H>
 *
 *  using Graph = List_Graph<Graph_Node<std::string>, Graph_Arc<double>>;
 *
 *  // Save graph to binary file
 *  Graph g;
 *  // ... populate graph ...
 *  IO_Graph<Graph> io(g);
 *  std::ofstream out("graph.bin", std::ios::binary);
 *  io.save(out);
 *
 *  // Load graph from binary file
 *  Graph g2;
 *  IO_Graph<Graph> io2(g2);
 *  std::ifstream in("graph.bin", std::ios::binary);
 *  io2.load(in);
 *
 *  // Text mode
 *  std::ofstream tout("graph.txt");
 *  io.save_in_text_mode(tout);
 *
 *  std::ifstream tin("graph.txt");
 *  io2.load_in_text_mode(tin);
 *  @endcode
 *
 *  ## Custom Serializers
 *
 *  For complex node/arc types, define custom load/store functors:
 *
 *  @code{.cpp}
 *  struct MyStoreNode {
 *    void operator()(std::ofstream& out, Graph&, Graph::Node* p) {
 *      // Custom binary serialization
 *      const auto& info = p->get_info();
 *      size_t len = info.name.size();
 *      out.write(reinterpret_cast<const char*>(&len), sizeof(len));
 *      out.write(info.name.data(), len);
 *    }
 *    void operator()(std::ostream& out, Graph&, Graph::Node* p) {
 *      // Custom text serialization
 *      out << p->get_info().name << std::endl;
 *    }
 *  };
 *
 *  IO_Graph<Graph, MyLoadNode, MyStoreNode, MyLoadArc, MyStoreArc> io(g);
 *  @endcode
 *
 *  @see tpl_graph.H Graph class definitions
 *  @see load_digraph.H Alternative digraph loading utilities
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef IO_GRAPH_H
#define IO_GRAPH_H

#include <fstream>
#include <iostream>
#include <memory>
#include <tpl_graph.H>
#include <ah-errors.H>

namespace Aleph
{

/** @brief Default node storage functor for binary and text modes.
 *
 *  Stores node information by writing the raw bytes of get_info()
 *  to binary streams, or using operator<< for text streams.
 *
 *  @tparam GT Graph type
 *  @ingroup Graphs
 */
template <class GT>
struct Dft_Store_Node
{
  /** @brief Store node to binary stream.
   *  @param output Binary output stream
   *  @param g Reference to the graph (unused in default implementation)
   *  @param p Node pointer to store
   */
  void operator()(std::ofstream & output, GT & g, typename GT::Node * p)
  {
    output.write(reinterpret_cast<const char*>(&p->get_info()), 
                 sizeof(typename GT::Node_Type));
  }

  /** @brief Store node to text stream.
   *  @param output Text output stream
   *  @param g Reference to the graph (unused in default implementation)
   *  @param p Node pointer to store
   */
  void operator()(std::ostream & output, GT & g, typename GT::Node * p)
  {
    output << p->get_info() << std::endl;
  }
};

/** @brief Default arc storage functor for binary and text modes.
 *
 *  Stores arc information by writing the raw bytes of get_info()
 *  to binary streams, or using operator<< for text streams.
 *
 *  @tparam GT Graph type
 *  @ingroup Graphs
 */
template <class GT>
struct Dft_Store_Arc
{
  /** @brief Store arc to binary stream.
   *  @param output Binary output stream
   *  @param g Reference to the graph (unused in default implementation)
   *  @param a Arc pointer to store
   */
  void operator()(std::ofstream & output, GT & g, typename GT::Arc * a)
  {
    output.write(reinterpret_cast<const char*>(&a->get_info()), 
                 sizeof(typename GT::Arc_Type));
  }

  /** @brief Store arc to text stream.
   *  @param output Text output stream
   *  @param g Reference to the graph (unused in default implementation)
   *  @param a Arc pointer to store
   */
  void operator()(std::ostream & output, GT & g, typename GT::Arc * a)
  {
    output << a->get_info() << std::endl;
  }
};

/** @brief Default node loading functor for binary and text modes.
 *
 *  Loads node information by reading raw bytes from binary streams,
 *  or using operator>> for text streams.
 *
 *  @tparam GT Graph type
 *  @ingroup Graphs
 */
template <class GT>
struct Dft_Load_Node
{
  /** @brief Load node from binary stream.
   *  @param input Binary input stream
   *  @param g Reference to the graph (unused in default implementation)
   *  @param p Node pointer to load into
   */
  void operator()(std::ifstream & input, GT & g, typename GT::Node * p)
  {
    input.read(reinterpret_cast<char*>(&p->get_info()), 
               sizeof(typename GT::Node_Type));
  }

  /** @brief Load node from text stream.
   *  @param input Text input stream
   *  @param g Reference to the graph (unused in default implementation)
   *  @param p Node pointer to load into
   */
  void operator()(std::istream & input, GT & g, typename GT::Node * p)
  {
    input >> p->get_info();
  }
};

/** @brief Default arc loading functor for binary and text modes.
 *
 *  Loads arc information by reading raw bytes from binary streams,
 *  or using operator>> for text streams.
 *
 *  @tparam GT Graph type
 *  @ingroup Graphs
 */
template <class GT>
struct Dft_Load_Arc
{
  /** @brief Load arc from binary stream.
   *  @param input Binary input stream
   *  @param g Reference to the graph (unused in default implementation)
   *  @param a Arc pointer to load into
   */
  void operator()(std::ifstream & input, GT & g, typename GT::Arc * a)
  {
    input.read(reinterpret_cast<char*>(&a->get_info()), 
               sizeof(typename GT::Arc_Type));
  }

  /** @brief Load arc from text stream.
   *  @param input Text input stream
   *  @param g Reference to the graph (unused in default implementation)
   *  @param a Arc pointer to load into
   */
  void operator()(std::istream & input, GT & g, typename GT::Arc * a)
  {
    input >> a->get_info();
  }
};

/** @brief Graph serialization and deserialization class.
 *
 *  IO_Graph provides methods for saving and loading graphs to/from
 *  binary and text streams. The serialization format is customizable
 *  through template parameters for node/arc load and store functors.
 *
 *  @tparam GT Graph type (List_Graph, List_Digraph, etc.)
 *  @tparam Load_Node Functor for loading node data. Must provide:
 *          - `void operator()(std::ifstream&, GT&, typename GT::Node*)`
 *          - `void operator()(std::istream&, GT&, typename GT::Node*)`
 *  @tparam Store_Node Functor for storing node data. Must provide:
 *          - `void operator()(std::ofstream&, GT&, typename GT::Node*)`
 *          - `void operator()(std::ostream&, GT&, typename GT::Node*)`
 *  @tparam Load_Arc Functor for loading arc data. Must provide:
 *          - `void operator()(std::ifstream&, GT&, typename GT::Arc*)`
 *          - `void operator()(std::istream&, GT&, typename GT::Arc*)`
 *  @tparam Store_Arc Functor for storing arc data. Must provide:
 *          - `void operator()(std::ofstream&, GT&, typename GT::Arc*)`
 *          - `void operator()(std::ostream&, GT&, typename GT::Arc*)`
 *  @tparam NF Node filter functor (only nodes passing filter are saved)
 *  @tparam AF Arc filter functor (only arcs passing filter are saved)
 *
 *  @see Dft_Store_Node Default node storage implementation
 *  @see Dft_Store_Arc Default arc storage implementation
 *  @see Dft_Load_Node Default node loading implementation
 *  @see Dft_Load_Arc Default arc loading implementation
 *  @see Filter_Iterator For filter functor requirements
 *  @ingroup Graphs
 */
template <class GT, 
          class Load_Node  = Dft_Load_Node<GT>,
          class Store_Node = Dft_Store_Node<GT>,
          class Load_Arc   = Dft_Load_Arc<GT>,
          class Store_Arc  = Dft_Store_Arc<GT>,
          class NF         = Aleph::Dft_Show_Node<GT>,
          class AF         = Aleph::Dft_Show_Arc<GT>>
class IO_Graph
{
  GT & g;

  Load_Node load_node   = Load_Node();
  Store_Node store_node = Store_Node();
  Load_Arc load_arc     = Load_Arc();
  Store_Arc store_arc   = Store_Arc();

  NF node_filter = NF();
  AF arc_filter  = AF();

  bool verbose_mode = false;

public:

  /** @brief Enable or disable verbose mode.
   *  
   *  When enabled, progress information is printed to std::cout
   *  during save and load operations.
   *
   *  @param v True to enable verbose output, false to disable
   */
  void set_verbose(bool v) noexcept { verbose_mode = v; }

  /** @brief Check if verbose mode is enabled.
   *  @return True if verbose mode is enabled
   */
  [[nodiscard]] bool is_verbose() const noexcept { return verbose_mode; }

  /** @brief Set the node loading functor.
   *  @param ln Node loading functor to use
   */
  void set_load_node(const Load_Node & ln) { load_node = ln; }

  /** @brief Set the node storage functor.
   *  @param sn Node storage functor to use
   */
  void set_store_node(const Store_Node & sn) { store_node = sn; }

  /** @brief Set the arc loading functor.
   *  @param la Arc loading functor to use
   */
  void set_load_arc(const Load_Arc & la) { load_arc = la; }

  /** @brief Set the arc storage functor.
   *  @param sa Arc storage functor to use
   */
  void set_store_arc(const Store_Arc & sa) { store_arc = sa; }

  /** @brief Set the node filter for save operations.
   *  @param nf Node filter functor (nodes passing filter are saved)
   */
  void set_node_filter(const NF & nf) { node_filter = nf; }

  /** @brief Set the arc filter for save operations.
   *  @param af Arc filter functor (arcs passing filter are saved)
   */
  void set_arc_filter(const AF & af) { arc_filter = af; }

  /** @brief Construct from graph reference.
   *  @param __g Reference to the graph to save/load
   */
  explicit IO_Graph(GT & __g) noexcept : g(__g) {}

  /** @brief Construct from graph pointer.
   *  @param gptr Pointer to the graph to save/load
   */
  explicit IO_Graph(GT * gptr) noexcept : g(*gptr) {}

  /** @brief Save graph to binary stream.
   *
   *  Writes the graph in binary format:
   *  1. Number of nodes (size_t)
   *  2. Node data for each node (via Store_Node functor)
   *  3. Number of arcs (size_t)
   *  4. For each arc: source index, target index (int), arc data (via Store_Arc)
   *
   *  @param output Binary output stream (must be opened with std::ios::binary)
   *  @throw std::runtime_error If write operations fail
   *
   *  @note Only nodes/arcs passing the filters are saved.
   */
  void save(std::ofstream & output)
  {
    const size_t num_nodes = g.get_num_nodes();

    if (verbose_mode)
      std::cout << "Storing " << num_nodes << " nodes ... ";

    output.write(reinterpret_cast<const char*>(&num_nodes), sizeof(num_nodes));

    int i = 0;
    DynMapTreap<typename GT::Node*, int> nodes_table;

    for (Node_Iterator<GT, NF> it(g, node_filter); it.has_curr();
         it.next_ne(), ++i)
      {
        auto p = it.get_curr();

        if (verbose_mode)
          std::cout << i << " ";

        store_node(output, g, p);
        nodes_table.insert(p, i);
      }

    const size_t num_arcs = g.get_num_arcs();

    if (verbose_mode)
      std::cout << " done " << std::endl 
                << "Storing " << num_arcs << " arcs ... " << std::endl;

    output.write(reinterpret_cast<const char*>(&num_arcs), sizeof(num_arcs));

    for (Arc_Iterator<GT, AF> it(g, arc_filter); it.has_curr(); it.next_ne())
      {
        auto a = it.get_curr();

        auto src = g.get_src_node(a);
        auto tgt = g.get_tgt_node(a);

        const int src_idx = nodes_table.find(src);
        const int tgt_idx = nodes_table.find(tgt);

        output.write(reinterpret_cast<const char*>(&src_idx), sizeof(int));
        output.write(reinterpret_cast<const char*>(&tgt_idx), sizeof(int));

        if (verbose_mode)
          std::cout << " " << src_idx << "--" << tgt_idx << " ";

        store_arc(output, g, a);

        if (verbose_mode)
          std::cout << std::endl;
      }

    if (verbose_mode)
      std::cout << " done " << std::endl << std::endl;
  }

  /** @brief Load graph from binary stream.
   *
   *  Reads a graph from binary format. The graph is cleared and
   *  rebuilt with the loaded nodes and arcs.
   *
   *  @param input Binary input stream (must be opened with std::ios::binary)
   *  @throw std::runtime_error If read operations fail or stream is corrupted
   *
   *  @note The target graph should be empty before loading.
   */
  void load(std::ifstream & input)
  {
    size_t num_nodes;
    input.read(reinterpret_cast<char*>(&num_nodes), sizeof(num_nodes));
    ah_runtime_error_if(not input.good())
      << "Failed to read node count from binary stream";

    if (verbose_mode)
      std::cout << "Loading " << num_nodes << " nodes ...";

    DynArray<typename GT::Node*> nodes_table(num_nodes);
    if (num_nodes > 0)
      nodes_table.reserve(0, num_nodes - 1);

    for (size_t i = 0; i < num_nodes; ++i)
      {
        std::unique_ptr<typename GT::Node> p(new typename GT::Node);

        if (verbose_mode)
          std::cout << " " << i;

        load_node(input, g, p.get());
        ah_runtime_error_if(not input.good())
          << "Failed to load node " << i << " from binary stream";

        typename GT::Node * inserted = g.insert_node(p.release());
        nodes_table.access(i) = inserted;
      }

    size_t num_arcs;
    input.read(reinterpret_cast<char*>(&num_arcs), sizeof(num_arcs));
    ah_runtime_error_if(not input.good())
      << "Failed to read arc count from binary stream";

    if (verbose_mode)
      std::cout << " done " << std::endl
                << "Loading " << num_arcs << " arcs ... " << std::endl;

    for (size_t i = 0; i < num_arcs; ++i)
      {
        int src_idx;
        input.read(reinterpret_cast<char*>(&src_idx), sizeof(int));
        ah_runtime_error_if(not input.good())
          << "Failed to read source index for arc " << i;

        auto src = nodes_table.access(src_idx);

        int tgt_idx;
        input.read(reinterpret_cast<char*>(&tgt_idx), sizeof(int));
        ah_runtime_error_if(not input.good())
          << "Failed to read target index for arc " << i;

        auto tgt = nodes_table.access(tgt_idx);
        auto a = g.insert_arc(src, tgt);

        if (verbose_mode)
          std::cout << " " << src_idx << "--" << tgt_idx << " ";

        load_arc(input, g, a);
        ah_runtime_error_if(not input.good())
          << "Failed to load arc " << i << " data";

        if (verbose_mode)
          std::cout << std::endl;
      }

    if (verbose_mode)
      std::cout << " done " << std::endl << std::endl;
  }

  /** @brief Save graph to text stream.
   *
   *  Writes the graph in human-readable text format:
   *  ```
   *  num_nodes
   *  num_arcs
   *  node_0_data
   *  node_1_data
   *  ...
   *  src_idx tgt_idx arc_0_data
   *  src_idx tgt_idx arc_1_data
   *  ...
   *  ```
   *
   *  @param output Text output stream
   *  @throw std::runtime_error If write operations fail
   *
   *  @note Only nodes/arcs passing the filters are saved.
   */
  void save_in_text_mode(std::ostream & output)
  {
    const size_t num_nodes = g.get_num_nodes();
    const size_t num_arcs  = g.get_num_arcs();
    
    output << num_nodes << std::endl
           << num_arcs << std::endl;

    if (verbose_mode)
      std::cout << "Storing " << num_nodes << " nodes ... ";

    int i = 0;
    DynMapTreap<typename GT::Node*, int> nodes_table;

    for (Node_Iterator<GT, NF> it(g, node_filter); it.has_curr();
         it.next_ne(), ++i)
      {
        typename GT::Node * p = it.get_curr();

        if (verbose_mode)
          std::cout << i << " ";

        store_node(output, g, p);
        nodes_table.insert(p, i);
      }

    if (verbose_mode)
      std::cout << " done " << std::endl 
                << "Storing " << num_arcs << " arcs ... " << std::endl;

    for (Arc_Iterator<GT, AF> it(g, arc_filter); it.has_curr(); it.next_ne())
      {
        auto a = it.get_curr();

        auto src = g.get_src_node(a);
        auto tgt = g.get_tgt_node(a);

        const int src_idx = nodes_table.find(src);
        const int tgt_idx = nodes_table.find(tgt);

        output << src_idx << " " << tgt_idx << " ";

        if (verbose_mode)
          std::cout << " " << src_idx << "--" << tgt_idx << " ";

        store_arc(output, g, a);

        if (verbose_mode)
          std::cout << std::endl;
      }

    if (verbose_mode)
      std::cout << " done " << std::endl << std::endl;
  }

  /** @brief Load graph from text stream.
   *
   *  Reads a graph from text format. The graph is cleared and
   *  rebuilt with the loaded nodes and arcs.
   *
   *  @param input Text input stream
   *  @throw std::runtime_error If read operations fail or stream is corrupted
   *
   *  @note The target graph should be empty before loading.
   */
  void load_in_text_mode(std::istream & input)
  {
    size_t num_nodes;
    size_t num_arcs;

    input >> num_nodes >> num_arcs;
    ah_runtime_error_if(not input.good())
      << "Failed to read node/arc count from text stream";

    input.ignore();

    if (verbose_mode)
      std::cout << "Loading " << num_nodes << " nodes ...";

    DynArray<typename GT::Node*> nodes_table(num_nodes);
    if (num_nodes > 0)
      nodes_table.reserve(0, num_nodes - 1);

    for (size_t i = 0; i < num_nodes; ++i)
      {
        std::unique_ptr<typename GT::Node> p(new typename GT::Node);

        if (verbose_mode)
          std::cout << " " << i;

        load_node(input, g, p.get());
        ah_runtime_error_if(not input.good())
          << "Failed to load node " << i << " from text stream";

        typename GT::Node * inserted = g.insert_node(p.release());
        nodes_table.access(i) = inserted;
      }

    if (verbose_mode)
      std::cout << " done " << std::endl
                << "Loading " << num_arcs << " arcs ... " << std::endl;

    for (size_t i = 0; i < num_arcs; ++i)
      {
        int src_idx;
        int tgt_idx;

        input >> src_idx >> tgt_idx;
        ah_runtime_error_if(not input.good())
          << "Failed to read arc " << i << " indices from text stream";

        auto src = nodes_table.access(src_idx);
        auto tgt = nodes_table.access(tgt_idx);
        auto a = g.insert_arc(src, tgt);

        if (verbose_mode)
          std::cout << " " << src_idx << "--" << tgt_idx << " ";

        load_arc(input, g, a);
        ah_runtime_error_if(not input.good())
          << "Failed to load arc " << i << " data from text stream";

        if (verbose_mode)
          std::cout << std::endl;
      }

    if (verbose_mode)
      std::cout << " done " << std::endl << std::endl;
  }
};

} // namespace Aleph

// Global namespace compatibility
using Aleph::IO_Graph;
using Aleph::Dft_Store_Node;
using Aleph::Dft_Store_Arc;
using Aleph::Dft_Load_Node;
using Aleph::Dft_Load_Arc;

#endif // IO_GRAPH_H
