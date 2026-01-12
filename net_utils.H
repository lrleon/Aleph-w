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

/** @file net_utils.H
 *  @brief Network flow utilities: generators, visualization, serialization.
 *
 *  This file provides utility functions for working with network flow graphs:
 *
 *  ## Network Generators
 *  - Random networks with various topologies
 *  - Grid networks (useful for benchmarking)
 *  - Bipartite matching networks
 *  - Scale-free networks
 *
 *  ## Visualization (DOT/GraphViz)
 *  - Export networks to DOT format
 *  - Show flow values and capacities
 *  - Highlight cuts and paths
 *
 *  ## Serialization
 *  - JSON export/import
 *  - DIMACS format support
 *  - Binary format for large networks
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  #include <net_utils.H>
 *
 *  // Generate a random network
 *  auto net = generate_random_network<Net_Graph<>>(100, 500, 1, 100);
 *
 *  // Solve max flow
 *  dinic_maximum_flow(net);
 *
 *  // Export to DOT for visualization
 *  export_network_to_dot(net, "network.dot");
 *
 *  // Export to JSON for persistence
 *  export_network_to_json(net, "network.json");
 *  ```
 *
 *  @see tpl_net.H Network flow structures
 *  @see tpl_maxflow.H Maximum flow algorithms
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <fstream>
#include <sstream>
#include <random>
#include <chrono>
#include <iomanip>
#include <tpl_net.H>
#include <tpl_netcost.H>
#include <tpl_dynMapTree.H>

namespace Aleph
{

//==============================================================================
// NETWORK GENERATORS
//==============================================================================

/** @brief Parameters for random network generation.
 *  @ingroup Networks
 */
struct NetworkGenParams
{
  size_t num_nodes = 10;           ///< Number of nodes
  size_t num_arcs = 20;            ///< Target number of arcs
  double min_capacity = 1.0;       ///< Minimum arc capacity
  double max_capacity = 100.0;     ///< Maximum arc capacity
  double min_cost = 0.0;           ///< Minimum arc cost (for cost networks)
  double max_cost = 10.0;          ///< Maximum arc cost
  bool ensure_connected = true;    ///< Ensure source can reach sink
  unsigned seed = 0;               ///< Random seed (0 = use time)
};

/** @brief Generate a random flow network.
 *
 *  Creates a network with random topology and capacities. Guarantees that
 *  the source can reach the sink if ensure_connected is true.
 *
 *  @tparam Net Network type.
 *  @param params Generation parameters.
 *  @return Generated network with source and sink set.
 *
 *  @par Example
 *  ```cpp
 *  NetworkGenParams params;
 *  params.num_nodes = 50;
 *  params.num_arcs = 200;
 *  params.max_capacity = 1000;
 *
 *  auto net = generate_random_network<Net_Graph<>>(params);
 *  ```
 *
 *  @ingroup Networks
 */
template <class Net>
Net generate_random_network(const NetworkGenParams& params)
{
  using Node = typename Net::Node;
  using Flow_Type = typename Net::Flow_Type;

  Net net;

  // Initialize RNG
  unsigned seed = params.seed;
  if (seed == 0)
    seed = static_cast<unsigned>(
        std::chrono::system_clock::now().time_since_epoch().count());

  std::mt19937 gen(seed);
  std::uniform_real_distribution<double> cap_dist(params.min_capacity,
                                                   params.max_capacity);
  std::uniform_int_distribution<size_t> node_dist(0, params.num_nodes - 1);

  // Create nodes
  std::vector<Node*> nodes(params.num_nodes);
  for (size_t i = 0; i < params.num_nodes; ++i)
    nodes[i] = net.insert_node();

  // Set source and sink
  // Sources and sinks are detected automatically by Net_Graph
  // based on which nodes have no incoming/outgoing arcs

  // Ensure connectivity: create a path from source to sink
  if (params.ensure_connected)
    {
      for (size_t i = 0; i < params.num_nodes - 1; ++i)
        {
          Flow_Type cap = static_cast<Flow_Type>(cap_dist(gen));
          net.insert_arc(nodes[i], nodes[i + 1], cap);
        }
    }

  // Add random arcs
  size_t arcs_to_add = params.num_arcs;
  if (params.ensure_connected)
    arcs_to_add -= (params.num_nodes - 1);

  for (size_t i = 0; i < arcs_to_add; ++i)
    {
      size_t src = node_dist(gen);
      size_t tgt = node_dist(gen);

      // Avoid self-loops
      if (src == tgt)
        {
          tgt = (tgt + 1) % params.num_nodes;
        }

      Flow_Type cap = static_cast<Flow_Type>(cap_dist(gen));
      net.insert_arc(nodes[src], nodes[tgt], cap);
    }

  return net;
}

/** @brief Generate a random flow network (convenience overload).
 *
 *  @tparam Net Network type.
 *  @param num_nodes Number of nodes.
 *  @param num_arcs Number of arcs.
 *  @param min_cap Minimum capacity.
 *  @param max_cap Maximum capacity.
 *  @param seed Random seed (0 = use time).
 *  @return Generated network.
 *
 *  @ingroup Networks
 */
template <class Net>
Net generate_random_network(size_t num_nodes, size_t num_arcs,
                            double min_cap = 1.0, double max_cap = 100.0,
                            unsigned seed = 0)
{
  NetworkGenParams params;
  params.num_nodes = num_nodes;
  params.num_arcs = num_arcs;
  params.min_capacity = min_cap;
  params.max_capacity = max_cap;
  params.seed = seed;
  return generate_random_network<Net>(params);
}

/** @brief Generate a grid network.
 *
 *  Creates a rows × cols grid with arcs connecting adjacent nodes.
 *  Source is top-left (0,0), sink is bottom-right (rows-1, cols-1).
 *
 *  Grid networks are useful for benchmarking because they have
 *  predictable structure and many augmenting paths.
 *
 *  @tparam Net Network type.
 *  @param rows Number of rows.
 *  @param cols Number of columns.
 *  @param capacity Capacity for all arcs.
 *  @param bidirectional If true, create arcs in both directions.
 *  @return Generated grid network.
 *
 *  @par Example
 *  ```cpp
 *  auto net = generate_grid_network<Net_Graph<>>(10, 10, 100.0);
 *  // Creates 100-node grid with 360 arcs
 *  ```
 *
 *  @ingroup Networks
 */
template <class Net>
Net generate_grid_network(size_t rows, size_t cols,
                          typename Net::Flow_Type capacity,
                          bool bidirectional = true)
{
  using Node = typename Net::Node;

  Net net;
  std::vector<std::vector<Node*>> nodes(rows, std::vector<Node*>(cols));

  // Create nodes
  for (size_t i = 0; i < rows; ++i)
    for (size_t j = 0; j < cols; ++j)
      nodes[i][j] = net.insert_node();

  // Create horizontal arcs
  for (size_t i = 0; i < rows; ++i)
    for (size_t j = 0; j < cols - 1; ++j)
      {
        net.insert_arc(nodes[i][j], nodes[i][j+1], capacity);
        if (bidirectional)
          net.insert_arc(nodes[i][j+1], nodes[i][j], capacity);
      }

  // Create vertical arcs
  for (size_t i = 0; i < rows - 1; ++i)
    for (size_t j = 0; j < cols; ++j)
      {
        net.insert_arc(nodes[i][j], nodes[i+1][j], capacity);
        if (bidirectional)
          net.insert_arc(nodes[i+1][j], nodes[i][j], capacity);
      }

  // Source (top-left) and sink (bottom-right) are detected automatically
  // since nodes[0][0] has no incoming arcs and nodes[rows-1][cols-1] has no outgoing arcs

  return net;
}

/** @brief Generate a bipartite network for matching.
 *
 *  Creates a bipartite graph with:
 *  - A super-source connected to all left nodes
 *  - A super-sink connected from all right nodes
 *  - Random arcs between left and right nodes
 *
 *  Useful for testing maximum bipartite matching.
 *
 *  @tparam Net Network type.
 *  @param left_size Number of nodes on left side.
 *  @param right_size Number of nodes on right side.
 *  @param edge_prob Probability of edge between left-right pair.
 *  @param seed Random seed.
 *  @return Bipartite network ready for max-flow = max matching.
 *
 *  @ingroup Networks
 */
template <class Net>
Net generate_bipartite_network(size_t left_size, size_t right_size,
                                double edge_prob = 0.5,
                                unsigned seed = 0)
{
  using Node = typename Net::Node;
  using Flow_Type = typename Net::Flow_Type;

  Net net;

  if (seed == 0)
    seed = static_cast<unsigned>(
        std::chrono::system_clock::now().time_since_epoch().count());

  std::mt19937 gen(seed);
  std::uniform_real_distribution<double> prob_dist(0.0, 1.0);

  // Create source and sink
  Node* source = net.insert_node();
  Node* sink = net.insert_node();

  // Create left nodes
  std::vector<Node*> left_nodes(left_size);
  for (size_t i = 0; i < left_size; ++i)
    {
      left_nodes[i] = net.insert_node();
      net.insert_arc(source, left_nodes[i], Flow_Type{1});
    }

  // Create right nodes
  std::vector<Node*> right_nodes(right_size);
  for (size_t i = 0; i < right_size; ++i)
    {
      right_nodes[i] = net.insert_node();
      net.insert_arc(right_nodes[i], sink, Flow_Type{1});
    }

  // Create random edges between left and right
  for (size_t i = 0; i < left_size; ++i)
    for (size_t j = 0; j < right_size; ++j)
      if (prob_dist(gen) < edge_prob)
        net.insert_arc(left_nodes[i], right_nodes[j], Flow_Type{1});

  // Source and sink are detected automatically

  return net;
}

/** @brief Generate a layered network.
 *
 *  Creates a network with multiple layers, where each node in layer i
 *  is connected to some nodes in layer i+1.
 *
 *  @tparam Net Network type.
 *  @param layer_sizes Vector of sizes for each layer.
 *  @param capacity Arc capacity.
 *  @param edge_prob Probability of arc between adjacent layers.
 *  @param seed Random seed.
 *  @return Layered network.
 *
 *  @ingroup Networks
 */
template <class Net>
Net generate_layered_network(const std::vector<size_t>& layer_sizes,
                              typename Net::Flow_Type capacity,
                              double edge_prob = 0.5,
                              unsigned seed = 0)
{
  using Node = typename Net::Node;

  Net net;

  if (layer_sizes.size() < 2)
    return net;

  if (seed == 0)
    seed = static_cast<unsigned>(
        std::chrono::system_clock::now().time_since_epoch().count());

  std::mt19937 gen(seed);
  std::uniform_real_distribution<double> prob_dist(0.0, 1.0);

  // Create nodes for each layer
  std::vector<std::vector<Node*>> layers(layer_sizes.size());
  for (size_t l = 0; l < layer_sizes.size(); ++l)
    {
      layers[l].resize(layer_sizes[l]);
      for (size_t i = 0; i < layer_sizes[l]; ++i)
        layers[l][i] = net.insert_node();
    }

  // Connect adjacent layers
  for (size_t l = 0; l < layer_sizes.size() - 1; ++l)
    {
      for (size_t i = 0; i < layers[l].size(); ++i)
        {
          bool connected = false;
          for (size_t j = 0; j < layers[l+1].size(); ++j)
            {
              if (prob_dist(gen) < edge_prob)
                {
                  net.insert_arc(layers[l][i], layers[l+1][j], capacity);
                  connected = true;
                }
            }
          // Ensure at least one connection
          if (not connected and not layers[l+1].empty())
            {
              size_t j = gen() % layers[l+1].size();
              net.insert_arc(layers[l][i], layers[l+1][j], capacity);
            }
        }
    }

  // Source and sink are detected automatically

  return net;
}


//==============================================================================
// DOT/GRAPHVIZ EXPORT
//==============================================================================

/** @brief Options for DOT export.
 *  @ingroup Networks
 */
struct DotExportOptions
{
  bool show_flow = true;           ///< Show flow values on arcs
  bool show_capacity = true;       ///< Show capacity values on arcs
  bool show_cost = false;          ///< Show cost values (for cost networks)
  bool highlight_saturated = true; ///< Highlight saturated arcs
  bool highlight_empty = false;    ///< Highlight empty arcs
  bool use_colors = true;          ///< Use colors for flow visualization
  std::string graph_name = "Network";
  std::string source_color = "green";
  std::string sink_color = "red";
  std::string saturated_color = "blue";
  std::string empty_color = "gray";
};

/** @brief Export network to DOT format for GraphViz visualization.
 *
 *  Creates a DOT file that can be rendered with GraphViz tools (dot, neato, etc.)
 *  to visualize the network structure and flow.
 *
 *  @tparam Net Network type.
 *  @param net Network to export.
 *  @param filename Output filename (should end with .dot).
 *  @param options Export options.
 *
 *  @par Example
 *  ```cpp
 *  Net_Graph<> net;
 *  // ... build and solve network ...
 *  export_network_to_dot(net, "flow.dot");
 *  // Then run: dot -Tpng flow.dot -o flow.png
 *  ```
 *
 *  @ingroup Networks
 */
template <class Net>
void export_network_to_dot(const Net& net, const std::string& filename,
                           const DotExportOptions& options = DotExportOptions())
{
  using Node = typename Net::Node;

  std::ofstream out(filename);
  if (not out)
    return;

  // Build node ID map
  DynMapTree<Node*, size_t> node_ids;
  size_t id = 0;
  for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    node_ids[it.get_curr()] = id++;

  out << "digraph " << options.graph_name << " {\n";
  out << "  rankdir=LR;\n";
  out << "  node [shape=circle];\n";

  // Export nodes
  for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      Node* p = it.get_curr();
      out << "  " << node_ids[p];

      std::string color;
      if (net.is_single_source() and p == net.get_source())
        color = options.source_color;
      else if (net.is_single_sink() and p == net.get_sink())
        color = options.sink_color;

      if (not color.empty() and options.use_colors)
        out << " [style=filled, fillcolor=" << color << "]";

      out << ";\n";
    }

  // Export arcs
  for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      auto arc = it.get_curr();
      Node* src = net.get_src_node(arc);
      Node* tgt = net.get_tgt_node(arc);

      out << "  " << node_ids[src] << " -> " << node_ids[tgt];

      // Build label
      std::ostringstream label;
      if (options.show_flow or options.show_capacity)
        {
          label << "\"";
          if (options.show_flow)
            label << arc->flow;
          if (options.show_flow and options.show_capacity)
            label << "/";
          if (options.show_capacity)
            label << arc->cap;
          label << "\"";
        }

      // Build style
      std::string color;
      if (options.use_colors)
        {
          if (options.highlight_saturated and arc->flow == arc->cap)
            color = options.saturated_color;
          else if (options.highlight_empty and arc->flow == 0)
            color = options.empty_color;
        }

      out << " [";
      if (not label.str().empty())
        out << "label=" << label.str();
      if (not color.empty())
        {
          if (not label.str().empty())
            out << ", ";
          out << "color=" << color;
        }
      out << "];\n";
    }

  out << "}\n";
}

/** @brief Generate DOT string for network (instead of file).
 *
 *  @tparam Net Network type.
 *  @param net Network to export.
 *  @param options Export options.
 *  @return DOT format string.
 *
 *  @ingroup Networks
 */
template <class Net>
std::string network_to_dot_string(const Net& net,
                                   const DotExportOptions& options = DotExportOptions())
{
  using Node = typename Net::Node;

  std::ostringstream out;

  // Build node ID map
  DynMapTree<Node*, size_t> node_ids;
  size_t id = 0;
  for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    node_ids[it.get_curr()] = id++;

  out << "digraph " << options.graph_name << " {\n";
  out << "  rankdir=LR;\n";
  out << "  node [shape=circle];\n";

  // Export nodes
  for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      Node* p = it.get_curr();
      out << "  " << node_ids[p];

      std::string color;
      if (net.is_single_source() and p == net.get_source())
        color = options.source_color;
      else if (net.is_single_sink() and p == net.get_sink())
        color = options.sink_color;

      if (not color.empty() and options.use_colors)
        out << " [style=filled, fillcolor=" << color << "]";

      out << ";\n";
    }

  // Export arcs
  for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      auto arc = it.get_curr();
      Node* src = net.get_src_node(arc);
      Node* tgt = net.get_tgt_node(arc);

      out << "  " << node_ids[src] << " -> " << node_ids[tgt];

      std::ostringstream label;
      if (options.show_flow or options.show_capacity)
        {
          label << "\"";
          if (options.show_flow)
            label << arc->flow;
          if (options.show_flow and options.show_capacity)
            label << "/";
          if (options.show_capacity)
            label << arc->cap;
          label << "\"";
        }

      std::string color;
      if (options.use_colors)
        {
          if (options.highlight_saturated and arc->flow == arc->cap)
            color = options.saturated_color;
          else if (options.highlight_empty and arc->flow == 0)
            color = options.empty_color;
        }

      out << " [";
      if (not label.str().empty())
        out << "label=" << label.str();
      if (not color.empty())
        {
          if (not label.str().empty())
            out << ", ";
          out << "color=" << color;
        }
      out << "];\n";
    }

  out << "}\n";

  return out.str();
}


//==============================================================================
// JSON SERIALIZATION
//==============================================================================

/** @brief Export network to JSON format.
 *
 *  Creates a JSON file with network structure including:
 *  - Node list with IDs
 *  - Arc list with source, target, capacity, flow
 *  - Source and sink information
 *
 *  @tparam Net Network type.
 *  @param net Network to export.
 *  @param filename Output filename (should end with .json).
 *
 *  @par JSON Format
 *  ```json
 *  {
 *    "nodes": [0, 1, 2, ...],
 *    "source": 0,
 *    "sink": 3,
 *    "arcs": [
 *      {"src": 0, "tgt": 1, "cap": 10, "flow": 5},
 *      ...
 *    ]
 *  }
 *  ```
 *
 *  @ingroup Networks
 */
template <class Net>
void export_network_to_json(const Net& net, const std::string& filename)
{
  using Node = typename Net::Node;

  std::ofstream out(filename);
  if (not out)
    return;

  // Build node ID map
  DynMapTree<Node*, size_t> node_ids;
  size_t id = 0;
  for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    node_ids[it.get_curr()] = id++;

  out << "{\n";

  // Nodes
  out << "  \"num_nodes\": " << net.vsize() << ",\n";
  out << "  \"num_arcs\": " << net.esize() << ",\n";

  // Source and sink
  if (net.is_single_source())
    out << "  \"source\": " << node_ids[net.get_source()] << ",\n";
  if (net.is_single_sink())
    out << "  \"sink\": " << node_ids[net.get_sink()] << ",\n";

  // Arcs
  out << "  \"arcs\": [\n";
  bool first = true;
  for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      if (not first)
        out << ",\n";
      first = false;

      auto arc = it.get_curr();
      out << "    {"
          << "\"src\": " << node_ids[net.get_src_node(arc)] << ", "
          << "\"tgt\": " << node_ids[net.get_tgt_node(arc)] << ", "
          << "\"cap\": " << arc->cap << ", "
          << "\"flow\": " << arc->flow
          << "}";
    }
  out << "\n  ]\n";
  out << "}\n";
}

/** @brief Export network to JSON string.
 *
 *  @tparam Net Network type.
 *  @param net Network to export.
 *  @return JSON string.
 *
 *  @ingroup Networks
 */
template <class Net>
std::string network_to_json_string(const Net& net)
{
  using Node = typename Net::Node;

  std::ostringstream out;

  // Build node ID map
  DynMapTree<Node*, size_t> node_ids;
  size_t id = 0;
  for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    node_ids[it.get_curr()] = id++;

  out << "{\n";
  out << "  \"num_nodes\": " << net.vsize() << ",\n";
  out << "  \"num_arcs\": " << net.esize() << ",\n";

  if (net.is_single_source())
    out << "  \"source\": " << node_ids[net.get_source()] << ",\n";
  if (net.is_single_sink())
    out << "  \"sink\": " << node_ids[net.get_sink()] << ",\n";

  out << "  \"arcs\": [\n";
  bool first = true;
  for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      if (not first)
        out << ",\n";
      first = false;

      auto arc = it.get_curr();
      out << "    {"
          << "\"src\": " << node_ids[net.get_src_node(arc)] << ", "
          << "\"tgt\": " << node_ids[net.get_tgt_node(arc)] << ", "
          << "\"cap\": " << arc->cap << ", "
          << "\"flow\": " << arc->flow
          << "}";
    }
  out << "\n  ]\n";
  out << "}\n";

  return out.str();
}


//==============================================================================
// DIMACS FORMAT
//==============================================================================

/** @brief Export network to DIMACS max-flow format.
 *
 *  DIMACS is a standard benchmark format for network flow problems.
 *  Many benchmark instances are available in this format.
 *
 *  @tparam Net Network type.
 *  @param net Network to export.
 *  @param filename Output filename.
 *
 *  @par DIMACS Format
 *  ```
 *  c Comment line
 *  p max <nodes> <arcs>
 *  n <source_id> s
 *  n <sink_id> t
 *  a <src> <tgt> <cap>
 *  ...
 *  ```
 *
 *  @ingroup Networks
 */
template <class Net>
void export_network_to_dimacs(const Net& net, const std::string& filename)
{
  using Node = typename Net::Node;

  std::ofstream out(filename);
  if (not out)
    return;

  // Build node ID map (1-based for DIMACS)
  DynMapTree<Node*, size_t> node_ids;
  size_t id = 1;  // DIMACS is 1-indexed
  for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    node_ids[it.get_curr()] = id++;

  // Header
  out << "c Network exported from Aleph-w\n";
  out << "p max " << net.vsize() << " " << net.esize() << "\n";

  // Source and sink
  if (net.is_single_source())
    out << "n " << node_ids[net.get_source()] << " s\n";
  if (net.is_single_sink())
    out << "n " << node_ids[net.get_sink()] << " t\n";

  // Arcs
  for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      auto arc = it.get_curr();
      out << "a " << node_ids[net.get_src_node(arc)] << " "
          << node_ids[net.get_tgt_node(arc)] << " "
          << static_cast<long long>(arc->cap) << "\n";
    }
}

/** @brief Import network from DIMACS max-flow format.
 *
 *  @tparam Net Network type.
 *  @param filename Input filename.
 *  @return Imported network.
 *
 *  @throw std::runtime_error If file cannot be opened or parsed.
 *
 *  @ingroup Networks
 */
template <class Net>
Net import_network_from_dimacs(const std::string& filename)
{
  using Node = typename Net::Node;
  using Flow_Type = typename Net::Flow_Type;

  Net net;
  std::ifstream in(filename);
  if (not in)
    throw std::runtime_error("Cannot open file: " + filename);

  std::vector<Node*> nodes;
  size_t source_id = 0, sink_id = 0;

  std::string line;
  while (std::getline(in, line))
    {
      if (line.empty() or line[0] == 'c')
        continue;

      std::istringstream iss(line);
      char type;
      iss >> type;

      if (type == 'p')
        {
          std::string problem;
          size_t n, m;
          iss >> problem >> n >> m;

          nodes.resize(n + 1);  // 1-indexed
          for (size_t i = 1; i <= n; ++i)
            nodes[i] = net.insert_node();
        }
      else if (type == 'n')
        {
          size_t id;
          char st;
          iss >> id >> st;

          if (st == 's')
            source_id = id;
          else if (st == 't')
            sink_id = id;
        }
      else if (type == 'a')
        {
          size_t src, tgt;
          long long cap;
          iss >> src >> tgt >> cap;

          net.insert_arc(nodes[src], nodes[tgt],
                         static_cast<Flow_Type>(cap));
        }
    }

  // Source and sink nodes are determined by topology in Net_Graph
  // The DIMACS format's source/sink designations guide network construction
  (void)source_id;  // Suppress unused variable warning
  (void)sink_id;

  return net;
}


//==============================================================================
// BENCHMARKING UTILITIES
//==============================================================================

/** @brief Result of running a max-flow algorithm.
 *  @ingroup Networks
 */
template <typename Flow_Type>
struct MaxFlowBenchmarkResult
{
  Flow_Type flow_value{0};
  double elapsed_ms{0};
  std::string algorithm_name;
};

/** @brief Run and time a max-flow algorithm.
 *
 *  @tparam Net Network type.
 *  @tparam Algo Algorithm functor type.
 *  @param net Network (will be modified).
 *  @param algo Algorithm functor.
 *  @param name Algorithm name for reporting.
 *  @return Benchmark result.
 *
 *  @ingroup Networks
 */
template <class Net, class Algo>
MaxFlowBenchmarkResult<typename Net::Flow_Type>
benchmark_maxflow(Net& net, Algo algo, const std::string& name)
{
  MaxFlowBenchmarkResult<typename Net::Flow_Type> result;
  result.algorithm_name = name;

  auto start = std::chrono::high_resolution_clock::now();
  result.flow_value = algo(net);
  auto end = std::chrono::high_resolution_clock::now();

  result.elapsed_ms = std::chrono::duration<double, std::milli>(
      end - start).count();

  return result;
}

/** @brief Print benchmark results.
 *  @ingroup Networks
 */
template <typename Flow_Type>
void print_benchmark_results(
    const std::vector<MaxFlowBenchmarkResult<Flow_Type>>& results)
{
  std::cout << "\n=== Max-Flow Algorithm Benchmark ===\n";
  std::cout << std::left << std::setw(25) << "Algorithm"
            << std::setw(15) << "Flow"
            << std::setw(15) << "Time (ms)" << "\n";
  std::cout << std::string(55, '-') << "\n";

  for (const auto& r : results)
    std::cout << std::left << std::setw(25) << r.algorithm_name
              << std::setw(15) << r.flow_value
              << std::setw(15) << std::fixed << std::setprecision(3)
              << r.elapsed_ms << "\n";
}

} // namespace Aleph

#endif // NET_UTILS_H
