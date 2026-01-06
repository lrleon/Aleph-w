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

/**
 * @file percolation_example.C
 * @brief Percolation Simulation using Union-Find
 * 
 * This example demonstrates the Union-Find (Disjoint Set Union) data
 * structure through a classic application: percolation simulation.
 * 
 * ## The Percolation Problem
 * 
 * Given an n×n grid of sites:
 * - Each site is either open (can flow through) or blocked
 * - System "percolates" if there's a path from top to bottom through open sites
 * 
 * This models physical phenomena like:
 * - Water flowing through porous rock
 * - Electricity conducting through composite materials
 * - Spread of forest fires
 * - Disease transmission in social networks
 * 
 * ## Union-Find Application
 * 
 * - Each open site is an element
 * - Adjacent open sites are in the same set (unioned)
 * - Virtual top/bottom nodes simplify percolation check
 * - Percolates if top and bottom are in same set
 * 
 * ## Percolation Threshold
 * 
 * Through simulation, we can estimate the critical probability p*
 * at which the system transitions from non-percolating to percolating.
 * For 2D square lattice: p* ≈ 0.593
 * 
 * @see tpl_union.H for Union-Find implementation
 * @author Leandro Rabindranath León
 * @ingroup Ejemplos
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <chrono>
#include <tpl_union.H>
#include <tclap/CmdLine.h>

using namespace std;
using namespace Aleph;

/**
 * @brief Percolation system using Union-Find
 * 
 * Models an n×n grid where each site can be open or blocked.
 * Uses virtual top and bottom nodes for efficient percolation testing.
 */
class PercolationSystem
{
private:
  size_t n;                    // Grid size
  vector<bool> open_sites;     // true if site is open
  Fixed_Relation uf;           // Union-Find structure
  size_t num_open;             // Count of open sites
  
  // Virtual nodes: top = n*n, bottom = n*n + 1
  size_t virtual_top() const { return n * n; }
  size_t virtual_bottom() const { return n * n + 1; }
  
  // Convert (row, col) to linear index (0-based)
  size_t index(size_t row, size_t col) const
  {
    return row * n + col;
  }
  
  // Check if (row, col) is valid
  bool valid(size_t row, size_t col) const
  {
    return row < n && col < n;
  }
  
public:
  /**
   * @brief Create an n×n percolation system with all sites blocked
   */
  PercolationSystem(size_t grid_size)
    : n(grid_size), 
      open_sites(n * n, false),
      uf(n * n + 2),  // n*n sites + 2 virtual nodes
      num_open(0)
  {
  }
  
  /**
   * @brief Open a site at (row, col) and connect to adjacent open sites
   */
  void open(size_t row, size_t col)
  {
    if (!valid(row, col) || is_open(row, col))
      return;
    
    size_t site = index(row, col);
    open_sites[site] = true;
    num_open++;
    
    // Connect to adjacent open sites
    const int dr[] = {-1, 1, 0, 0};
    const int dc[] = {0, 0, -1, 1};
    
    for (int i = 0; i < 4; i++)
    {
      size_t nr = row + dr[i];
      size_t nc = col + dc[i];
      if (valid(nr, nc) && is_open(nr, nc))
        uf.join(site, index(nr, nc));
    }
    
    // Connect top row to virtual top
    if (row == 0)
      uf.join(site, virtual_top());
    
    // Connect bottom row to virtual bottom
    if (row == n - 1)
      uf.join(site, virtual_bottom());
  }
  
  /**
   * @brief Check if site at (row, col) is open
   */
  bool is_open(size_t row, size_t col) const
  {
    return valid(row, col) && open_sites[index(row, col)];
  }
  
  /**
   * @brief Check if site at (row, col) is connected to top (is "full")
   */
  bool is_full(size_t row, size_t col)
  {
    if (!is_open(row, col))
      return false;
    return uf.are_connected(index(row, col), virtual_top());
  }
  
  /**
   * @brief Check if system percolates (top connected to bottom)
   */
  bool percolates()
  {
    return uf.are_connected(virtual_top(), virtual_bottom());
  }
  
  /**
   * @brief Get number of open sites
   */
  size_t number_of_open_sites() const { return num_open; }
  
  /**
   * @brief Get grid size
   */
  size_t size() const { return n; }
  
  /**
   * @brief Get fraction of open sites
   */
  double open_fraction() const 
  { 
    return static_cast<double>(num_open) / (n * n); 
  }
  
  /**
   * @brief Print the grid (for small grids)
   * 
   * Legend: '#' = blocked, '.' = open but not full, 'O' = full (connected to top)
   */
  void print() const
  {
    for (size_t row = 0; row < n; row++)
    {
      for (size_t col = 0; col < n; col++)
      {
        if (!is_open(row, col))
          cout << "# ";
        else if (const_cast<PercolationSystem*>(this)->is_full(row, col))
          cout << "O ";
        else
          cout << ". ";
      }
      cout << endl;
    }
  }
};

/**
 * @brief Run a single percolation experiment
 * 
 * Opens random sites until system percolates, returns threshold
 */
double run_experiment(size_t n, mt19937& rng)
{
  PercolationSystem perc(n);
  
  // Create list of all sites and shuffle
  vector<pair<size_t, size_t>> sites;
  for (size_t i = 0; i < n; i++)
    for (size_t j = 0; j < n; j++)
      sites.emplace_back(i, j);
  
  shuffle(sites.begin(), sites.end(), rng);
  
  // Open sites until percolation
  for (const auto& [row, col] : sites)
  {
    perc.open(row, col);
    if (perc.percolates())
      break;
  }
  
  return perc.open_fraction();
}

/**
 * @brief Monte Carlo estimation of percolation threshold
 */
void monte_carlo_simulation(size_t n, size_t trials, unsigned seed, bool verbose)
{
  cout << "\n=== Monte Carlo Percolation Threshold Estimation ===" << endl;
  cout << "Grid size: " << n << "×" << n << endl;
  cout << "Trials: " << trials << endl;
  cout << "Seed: " << seed << endl;
  
  mt19937 rng(seed);
  
  vector<double> thresholds(trials);
  double sum = 0, sum_sq = 0;
  
  auto start = chrono::high_resolution_clock::now();
  
  for (size_t t = 0; t < trials; t++)
  {
    double p = run_experiment(n, rng);
    thresholds[t] = p;
    sum += p;
    sum_sq += p * p;
    
    if (verbose && (t + 1) % (trials / 10) == 0)
      cout << "  Completed " << (t + 1) << "/" << trials << " trials..." << endl;
  }
  
  auto end = chrono::high_resolution_clock::now();
  double ms = chrono::duration<double, milli>(end - start).count();
  
  // Statistics
  double mean = sum / trials;
  double variance = (sum_sq - sum * sum / trials) / (trials - 1);
  double stddev = sqrt(variance);
  double confidence_95 = 1.96 * stddev / sqrt(trials);
  
  cout << "\n--- Results ---" << endl;
  cout << "Sample mean:              " << fixed << setprecision(6) << mean << endl;
  cout << "Sample standard deviation: " << stddev << endl;
  cout << "95% confidence interval:  [" 
       << (mean - confidence_95) << ", " 
       << (mean + confidence_95) << "]" << endl;
  cout << "\nTheoretical p* ≈ 0.592746" << endl;
  cout << "Difference from theory:   " << abs(mean - 0.592746) << endl;
  cout << "\nTime: " << setprecision(2) << ms << " ms" << endl;
}

/**
 * @brief Interactive demonstration with visualization
 */
void visual_demonstration(size_t n, unsigned seed)
{
  cout << "\n=== Visual Percolation Demo ===" << endl;
  cout << "Grid size: " << n << "×" << n << endl;
  
  PercolationSystem perc(n);
  mt19937 rng(seed);
  
  // Shuffle sites
  vector<pair<size_t, size_t>> sites;
  for (size_t i = 0; i < n; i++)
    for (size_t j = 0; j < n; j++)
      sites.emplace_back(i, j);
  shuffle(sites.begin(), sites.end(), rng);
  
  cout << "\nLegend: # = blocked, . = open (not full), O = full (connected to top)" << endl;
  
  // Show progression
  vector<double> checkpoints = {0.2, 0.4, 0.5, 0.55, 0.6, 0.65, 0.7};
  size_t site_idx = 0;
  size_t total_sites = n * n;
  
  for (double target : checkpoints)
  {
    size_t target_open = static_cast<size_t>(target * total_sites);
    
    while (perc.number_of_open_sites() < target_open && site_idx < sites.size())
    {
      auto [row, col] = sites[site_idx++];
      perc.open(row, col);
    }
    
    cout << "\n--- " << fixed << setprecision(0) << (target * 100) 
         << "% sites open (" << perc.number_of_open_sites() << "/" << total_sites 
         << ") ---" << endl;
    perc.print();
    
    if (perc.percolates())
    {
      cout << "\n*** SYSTEM PERCOLATES at p = " 
           << setprecision(3) << perc.open_fraction() << " ***" << endl;
      break;
    }
    else
    {
      cout << "(Does not percolate yet)" << endl;
    }
  }
  
  // If not percolated yet, continue until it does
  if (!perc.percolates())
  {
    while (!perc.percolates() && site_idx < sites.size())
    {
      auto [row, col] = sites[site_idx++];
      perc.open(row, col);
    }
    
    cout << "\n--- Percolation achieved ---" << endl;
    perc.print();
    cout << "\n*** SYSTEM PERCOLATES at p = " 
         << setprecision(3) << perc.open_fraction() << " ***" << endl;
  }
}

/**
 * @brief Explain Union-Find operations
 */
void explain_union_find()
{
  cout << "\n=== Union-Find Data Structure ===" << endl;
  
  cout << "\nThe Union-Find (Disjoint Set Union) structure supports:" << endl;
  cout << "  - make_set(x): Create a new set containing only x" << endl;
  cout << "  - find(x): Return the representative of x's set" << endl;
  cout << "  - union(x, y): Merge the sets containing x and y" << endl;
  
  cout << "\nOptimizations:" << endl;
  cout << "  - Path compression: During find(), make nodes point directly to root" << endl;
  cout << "  - Union by rank: Attach smaller tree under larger tree's root" << endl;
  
  cout << "\nComplexity (with both optimizations):" << endl;
  cout << "  - Nearly O(1) per operation (amortized)" << endl;
  cout << "  - Formally: O(α(n)) where α is inverse Ackermann function" << endl;
  cout << "  - For all practical n: α(n) ≤ 4" << endl;
  
  cout << "\nApplication in Percolation:" << endl;
  cout << "  - Each grid cell is an element" << endl;
  cout << "  - Opening a cell: union with adjacent open cells" << endl;
  cout << "  - Percolation test: are top and bottom connected?" << endl;
  cout << "  - Virtual nodes simplify: virtual_top connected to all top row sites" << endl;
}

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd("Percolation Example with Union-Find", ' ', "1.0");
    
    TCLAP::ValueArg<size_t> sizeArg("n", "size",
      "Grid size", false, 20, "size_t");
    TCLAP::ValueArg<size_t> trialsArg("t", "trials",
      "Number of Monte Carlo trials", false, 100, "size_t");
    TCLAP::ValueArg<unsigned> seedArg("s", "seed",
      "Random seed", false, 42, "unsigned");
    TCLAP::SwitchArg visualArg("i", "visual",
      "Show visual demonstration", false);
    TCLAP::SwitchArg monteArg("m", "monte-carlo",
      "Run Monte Carlo simulation", false);
    TCLAP::SwitchArg explainArg("e", "explain",
      "Explain Union-Find", false);
    TCLAP::SwitchArg allArg("a", "all",
      "Run all demos", false);
    TCLAP::SwitchArg verboseArg("v", "verbose",
      "Show detailed output", false);
    
    cmd.add(sizeArg);
    cmd.add(trialsArg);
    cmd.add(seedArg);
    cmd.add(visualArg);
    cmd.add(monteArg);
    cmd.add(explainArg);
    cmd.add(allArg);
    cmd.add(verboseArg);
    
    cmd.parse(argc, argv);
    
    size_t n = sizeArg.getValue();
    size_t trials = trialsArg.getValue();
    unsigned seed = seedArg.getValue();
    bool visual = visualArg.getValue();
    bool monte = monteArg.getValue();
    bool explain = explainArg.getValue();
    bool all = allArg.getValue();
    bool verbose = verboseArg.getValue();
    
    // Default: show visual and explain
    if (!visual && !monte && !explain)
      all = true;
    
    cout << "=== Percolation: A Union-Find Application ===" << endl;
    
    if (all || explain)
      explain_union_find();
    
    if (all || visual)
    {
      // Use smaller grid for visual demo
      size_t visual_size = min(n, (size_t)15);
      visual_demonstration(visual_size, seed);
    }
    
    if (all || monte)
      monte_carlo_simulation(n, trials, seed, verbose);
    
    cout << "\n=== Summary ===" << endl;
    cout << "Percolation threshold for 2D square lattice:" << endl;
    cout << "  Theoretical: p* ≈ 0.592746" << endl;
    cout << "  (Critical probability at which infinite cluster appears)" << endl;
    cout << "\nUnion-Find enables efficient connectivity queries:" << endl;
    cout << "  O(α(n)) per operation ≈ O(1) in practice" << endl;
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
    return 1;
  }
}

