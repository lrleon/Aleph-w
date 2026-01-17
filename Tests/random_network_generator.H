
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


/**
 * @file random_network_generator.H
 * @brief Random network generators for testing flow algorithms
 * 
 * Provides utilities to generate various types of random networks for
 * comprehensive testing of flow algorithms.
 */

#ifndef RANDOM_NETWORK_GENERATOR_H
#define RANDOM_NETWORK_GENERATOR_H

#include <random>
#include <vector>
#include <queue>
#include <set>
#include <tpl_net.H>
#include <tpl_netcost.H>

namespace Aleph
{
namespace Testing
{

/**
 * @brief Random network generator configuration
 */
struct NetworkGeneratorConfig
{
  size_t num_nodes = 10;           ///< Number of nodes
  size_t num_arcs = 20;            ///< Number of arcs (ignored for some generators)
  double density = 0.3;            ///< Edge density [0, 1] (for Erdős–Rényi)
  double min_capacity = 1.0;       ///< Minimum arc capacity
  double max_capacity = 100.0;     ///< Maximum arc capacity
  double min_cost = 1.0;           ///< Minimum arc cost (for cost networks)
  double max_cost = 10.0;          ///< Maximum arc cost
  unsigned seed = 42;              ///< Random seed (0 = time-based)
  bool ensure_connected = true;    ///< Ensure source can reach sink
};

// Type trait to detect if a network has cost support
template<typename Net, typename = void>
struct has_cost_support : std::false_type {};

template<typename Net>
struct has_cost_support<Net, std::void_t<decltype(std::declval<typename Net::Arc>().cost)>> 
  : std::true_type {};

/**
 * @brief Base class for random network generators
 */
template <class Net>
class RandomNetworkGenerator
{
protected:
  NetworkGeneratorConfig config;
  std::mt19937 rng;
  
  using Node = typename Net::Node;
  using Arc = typename Net::Arc;
  
  // Random number generators
  std::uniform_real_distribution<double> capacity_dist;
  std::uniform_real_distribution<double> cost_dist;
  std::uniform_real_distribution<double> prob_dist{0.0, 1.0};
  
public:
  RandomNetworkGenerator(const NetworkGeneratorConfig& cfg)
    : config(cfg),
      rng(cfg.seed == 0 ? std::random_device()() : cfg.seed),
      capacity_dist(cfg.min_capacity, cfg.max_capacity),
      cost_dist(cfg.min_cost, cfg.max_cost)
  {}
  
  virtual ~RandomNetworkGenerator() = default;
  
  /// Reset the random number generator to the original seed.
  /// Call this before generate() to get reproducible networks.
  void reseed()
  {
    rng.seed(config.seed == 0 ? std::random_device()() : config.seed);
  }
  
  /// Reset with a specific seed (also updates config.seed)
  void reseed(unsigned new_seed)
  {
    config.seed = new_seed;
    rng.seed(new_seed);
  }
  
  // Generate a network (fills the provided network reference to avoid copy issues)
  virtual void generate(Net& out) = 0;
  
protected:
  // Helper: get random capacity
  double random_capacity()
  {
    return capacity_dist(rng);
  }
  
  // Helper: get random cost
  double random_cost()
  {
    return cost_dist(rng);
  }
  
  // Helper: random probability [0, 1]
  double random_prob()
  {
    return prob_dist(rng);
  }
  
  // Helper: Check if sink is reachable from source
  bool is_connected(const Net& net, Node* source, Node* sink)
  {
    std::set<Node*> visited;
    std::queue<Node*> q;
    
    visited.insert(source);
    q.push(source);
    
    while (!q.empty())
    {
      Node* u = q.front();
      q.pop();
      
      if (u == sink)
        return true;
      
      for (Out_Iterator<Net> it(u); it.has_curr(); it.next_ne())
      {
        Arc* arc = it.get_curr();
        Node* v = net.get_tgt_node(arc);
        
        if (visited.count(v) == 0)
        {
          visited.insert(v);
          q.push(v);
        }
      }
    }
    
    return false;
  }
  
  // Helper: Add path from source to sink if needed
  void ensure_path(Net& net, Node* source, Node* sink,
                   const std::vector<Node*>& nodes)
  {
    if (is_connected(net, source, sink))
      return;
    
    // Add a path through intermediate nodes
    Node* prev = source;
    for (size_t i = 2; i < nodes.size() - 1; ++i)
    {
      insert_arc(net, prev, nodes[i]);
      prev = nodes[i];
    }
    insert_arc(net, prev, sink);
  }
  
  // Helper: Insert arc with appropriate attributes
  void insert_arc(Net& net, Node* src, Node* tgt)
  {
    if constexpr (has_cost_support<Net>::value)
    {
      net.insert_arc(src, tgt, random_capacity(), random_cost());
    }
    else
    {
      net.insert_arc(src, tgt, random_capacity());
    }
  }
};


/**
 * @brief Erdős–Rényi random network generator
 * 
 * Generates G(n, p) random graphs where each possible arc exists with
 * probability p (density).
 */
template <class Net>
class ErdosRenyiGenerator : public RandomNetworkGenerator<Net>
{
  using Base = RandomNetworkGenerator<Net>;
  using typename Base::Node;
  using Base::config;
  using Base::random_prob;
  using Base::insert_arc;
  using Base::is_connected;
  using Base::ensure_path;
  
public:
  ErdosRenyiGenerator(const NetworkGeneratorConfig& cfg)
    : Base(cfg) {}
  
  void generate(Net& net) override
  {
    std::vector<Node*> nodes;
    
    // Create nodes
    for (size_t i = 0; i < config.num_nodes; ++i)
      nodes.push_back(net.insert_node());
    
    Node* source = nodes[0];
    Node* sink = nodes[config.num_nodes - 1];
    
    // Add arcs with probability = density
    // Skip direct source -> intermediate and intermediate -> source
    // Skip direct sink connections that would bypass the network
    for (size_t i = 0; i < config.num_nodes; ++i)
    {
      for (size_t j = 0; j < config.num_nodes; ++j)
      {
        if (i == j)
          continue;
        
        // Don't allow arcs back to source or from sink
        if (j == 0 || i == config.num_nodes - 1)
          continue;
        
        if (random_prob() < config.density)
          insert_arc(net, nodes[i], nodes[j]);
      }
    }
    
    // Ensure connectivity if requested
    if (config.ensure_connected)
      ensure_path(net, source, sink, nodes);
  }
};


/**
 * @brief Layered (staged) network generator
 * 
 * Generates networks with distinct layers where arcs only go from layer i
 * to layer i+1. Good for testing path-based algorithms.
 */
template <class Net>
class LayeredNetworkGenerator : public RandomNetworkGenerator<Net>
{
  using Base = RandomNetworkGenerator<Net>;
  using typename Base::Node;
  using Base::config;
  using Base::random_prob;
  using Base::insert_arc;
  
  size_t num_layers;
  size_t nodes_per_layer;
  
public:
  LayeredNetworkGenerator(const NetworkGeneratorConfig& cfg,
                         size_t layers = 4,
                         size_t per_layer = 5)
    : Base(cfg), num_layers(layers), nodes_per_layer(per_layer)
  {
    // Adjust num_nodes to match layers
    this->config.num_nodes = 2 + (layers - 2) * nodes_per_layer;
  }
  
  void generate(Net& net) override
  {
    std::vector<std::vector<Node*>> layers(num_layers);
    
    // Create source (layer 0)
    layers[0].push_back(net.insert_node());
    
    // Create intermediate layers
    for (size_t layer = 1; layer < num_layers - 1; ++layer)
    {
      for (size_t i = 0; i < nodes_per_layer; ++i)
        layers[layer].push_back(net.insert_node());
    }
    
    // Create sink (last layer)
    layers[num_layers - 1].push_back(net.insert_node());
    
    // Connect layers: each node in layer i connects to nodes in layer i+1
    for (size_t layer = 0; layer < num_layers - 1; ++layer)
    {
      for (Node* u : layers[layer])
      {
        for (Node* v : layers[layer + 1])
        {
          if (random_prob() < config.density)
            insert_arc(net, u, v);
        }
      }
    }
    
    // Ensure at least one path exists
    if (config.ensure_connected)
    {
      Node* prev = layers[0][0];
      for (size_t layer = 1; layer < num_layers; ++layer)
      {
        Node* next = layers[layer][0];
        // Check if connection exists
        bool has_connection = false;
        for (Out_Iterator<Net> it(prev); it.has_curr(); it.next_ne())
        {
          if (net.get_tgt_node(it.get_curr()) == next)
          {
            has_connection = true;
            break;
          }
        }
        if (!has_connection)
          insert_arc(net, prev, next);
        prev = next;
      }
    }
  }
};


/**
 * @brief Grid network generator
 * 
 * Generates a grid with flow from top-left to bottom-right.
 * Good for testing scalability.
 */
template <class Net>
class GridNetworkGenerator : public RandomNetworkGenerator<Net>
{
  using Base = RandomNetworkGenerator<Net>;
  using typename Base::Node;
  using Base::config;
  using Base::insert_arc;
  
  size_t rows;
  size_t cols;
  
public:
  GridNetworkGenerator(const NetworkGeneratorConfig& cfg,
                      size_t r = 5,
                      size_t c = 5)
    : Base(cfg), rows(r), cols(c)
  {
    this->config.num_nodes = r * c;
  }
  
  void generate(Net& net) override
  {
    std::vector<std::vector<Node*>> grid(rows, std::vector<Node*>(cols));
    
    // Create grid nodes
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j)
        grid[i][j] = net.insert_node();
    
    // Add horizontal arcs (left to right)
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols - 1; ++j)
        insert_arc(net, grid[i][j], grid[i][j+1]);
    
    // Add vertical arcs (top to bottom)
    for (size_t i = 0; i < rows - 1; ++i)
      for (size_t j = 0; j < cols; ++j)
        insert_arc(net, grid[i][j], grid[i+1][j]);
  }
};


/**
 * @brief Complete bipartite network generator
 * 
 * Generates K_{m,n} with source connected to left side and right side
 * connected to sink. Good for testing matching algorithms.
 */
template <class Net>
class BipartiteNetworkGenerator : public RandomNetworkGenerator<Net>
{
  using Base = RandomNetworkGenerator<Net>;
  using typename Base::Node;
  using Base::config;
  using Base::insert_arc;
  
  size_t left_size;
  size_t right_size;
  
public:
  BipartiteNetworkGenerator(const NetworkGeneratorConfig& cfg,
                           size_t left = 5,
                           size_t right = 5)
    : Base(cfg), left_size(left), right_size(right)
  {
    this->config.num_nodes = 2 + left + right;  // source + left + right + sink
  }
  
  void generate(Net& net) override
  {
    Node* source = net.insert_node();
    Node* sink = net.insert_node();
    
    std::vector<Node*> left_nodes;
    std::vector<Node*> right_nodes;
    
    // Create left side
    for (size_t i = 0; i < left_size; ++i)
      left_nodes.push_back(net.insert_node());
    
    // Create right side
    for (size_t i = 0; i < right_size; ++i)
      right_nodes.push_back(net.insert_node());
    
    // Connect source to left side
    for (Node* u : left_nodes)
      insert_arc(net, source, u);
    
    // Connect left to right (complete bipartite)
    for (Node* u : left_nodes)
      for (Node* v : right_nodes)
        insert_arc(net, u, v);
    
    // Connect right side to sink
    for (Node* v : right_nodes)
      insert_arc(net, v, sink);
  }
};


/**
 * @brief Factory function to create generators
 */
template <class Net>
std::unique_ptr<RandomNetworkGenerator<Net>>
create_generator(const std::string& type, const NetworkGeneratorConfig& config)
{
  if (type == "erdos-renyi")
    return std::make_unique<ErdosRenyiGenerator<Net>>(config);
  else if (type == "layered")
    return std::make_unique<LayeredNetworkGenerator<Net>>(config);
  else if (type == "grid")
    return std::make_unique<GridNetworkGenerator<Net>>(config);
  else if (type == "bipartite")
    return std::make_unique<BipartiteNetworkGenerator<Net>>(config);
  else
    throw std::invalid_argument("Unknown generator type: " + type);
}

} // namespace Testing
} // namespace Aleph

#endif // RANDOM_NETWORK_GENERATOR_H
