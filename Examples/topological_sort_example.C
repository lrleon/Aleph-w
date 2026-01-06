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
 * @file topological_sort_example.C
 * @brief Topological Sort: Ordering tasks with dependencies
 * 
 * This example demonstrates topological sorting of directed acyclic
 * graphs (DAGs), a fundamental algorithm for ordering tasks with
 * dependencies.
 * 
 * ## What is Topological Sort?
 * 
 * A topological ordering of a DAG is a linear ordering of vertices
 * such that for every directed edge (u, v), vertex u comes before v.
 * 
 * ## Real-World Applications
 * 
 * - **Build systems**: Compile files in dependency order (Make, CMake)
 * - **Package managers**: Install packages respecting dependencies
 * - **Task scheduling**: Execute tasks in valid order
 * - **Course prerequisites**: Order courses by requirements
 * - **Spreadsheets**: Evaluate cells in dependency order
 * 
 * ## Algorithms
 * 
 * 1. **DFS-based (Topological_Sort)**: Post-order traversal
 * 2. **BFS-based / Kahn's (Q_Topological_Sort)**: Remove sources iteratively
 * 
 * Both run in O(V + E) time.
 * 
 * @see topological_sort.H
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <tpl_graph.H>
#include <topological_sort.H>
#include <tclap/CmdLine.h>

using namespace std;
using namespace Aleph;

// Task node
using TaskNode = Graph_Node<string>;
using DependencyArc = Graph_Arc<int>;  // Weight could be time/cost
using TaskGraph = List_Digraph<TaskNode, DependencyArc>;

/**
 * @brief Build a sample build system dependency graph
 * 
 * Represents a typical C++ project build:
 * 
 *   utils.h <-- config.h
 *      |          |
 *      v          v
 *   utils.o    parser.o <-- lexer.o
 *      |          |           |
 *      +----+-----+-----+-----+
 *           |           |
 *           v           v
 *        main.o     test.o
 *           |           |
 *           v           v
 *        program    test_suite
 */
TaskGraph build_project_graph()
{
  TaskGraph g;
  
  // Header files
  auto config_h = g.insert_node("config.h");
  auto utils_h = g.insert_node("utils.h");
  
  // Object files
  auto utils_o = g.insert_node("utils.o");
  auto parser_o = g.insert_node("parser.o");
  auto lexer_o = g.insert_node("lexer.o");
  auto main_o = g.insert_node("main.o");
  auto test_o = g.insert_node("test.o");
  
  // Executables
  auto program = g.insert_node("program");
  auto test_suite = g.insert_node("test_suite");
  
  // Dependencies (edges go from dependency to dependent)
  g.insert_arc(config_h, utils_h);
  g.insert_arc(config_h, parser_o);
  g.insert_arc(utils_h, utils_o);
  g.insert_arc(utils_h, lexer_o);
  g.insert_arc(utils_o, main_o);
  g.insert_arc(utils_o, test_o);
  g.insert_arc(parser_o, main_o);
  g.insert_arc(lexer_o, parser_o);
  g.insert_arc(lexer_o, test_o);
  g.insert_arc(main_o, program);
  g.insert_arc(test_o, test_suite);
  
  return g;
}

/**
 * @brief Build a course prerequisites graph
 * 
 * University course dependencies:
 * 
 *   Math101 --> Math201 --> Math301
 *      |          |
 *      v          v
 *   CS101 ----> CS201 ----> CS301
 *      |                      |
 *      v                      v
 *   CS102 ----------------> CS302
 */
TaskGraph build_courses_graph()
{
  TaskGraph g;
  
  // Math courses
  auto math101 = g.insert_node("Math101");
  auto math201 = g.insert_node("Math201");
  auto math301 = g.insert_node("Math301");
  
  // CS courses
  auto cs101 = g.insert_node("CS101");
  auto cs102 = g.insert_node("CS102");
  auto cs201 = g.insert_node("CS201");
  auto cs301 = g.insert_node("CS301");
  auto cs302 = g.insert_node("CS302");
  
  // Prerequisites
  g.insert_arc(math101, math201);
  g.insert_arc(math201, math301);
  g.insert_arc(math101, cs101);
  g.insert_arc(cs101, cs102);
  g.insert_arc(cs101, cs201);
  g.insert_arc(math201, cs201);
  g.insert_arc(cs201, cs301);
  g.insert_arc(cs301, cs302);
  g.insert_arc(cs102, cs302);
  
  return g;
}

/**
 * @brief Find a node by name
 */
TaskGraph::Node* find_task(TaskGraph& g, const string& name)
{
  for (auto it = g.get_node_it(); it.has_curr(); it.next())
    if (it.get_curr()->get_info() == name)
      return it.get_curr();
  return nullptr;
}

/**
 * @brief Print the graph structure
 */
void print_graph(TaskGraph& g, const string& title)
{
  cout << "\n=== " << title << " ===" << endl;
  cout << "Tasks: " << g.get_num_nodes() << endl;
  cout << "Dependencies: " << g.get_num_arcs() << endl;
  
  cout << "\nDependency structure:" << endl;
  for (auto nit = g.get_node_it(); nit.has_curr(); nit.next())
  {
    auto* node = nit.get_curr();
    cout << "  " << node->get_info() << " depends on: ";
    
    // Count incoming edges (dependencies)
    bool first = true;
    for (auto ait = g.get_arc_it(); ait.has_curr(); ait.next())
    {
      auto* arc = ait.get_curr();
      if (g.get_tgt_node(arc) == node)
      {
        if (not first) cout << ", ";
        cout << g.get_src_node(arc)->get_info();
        first = false;
      }
    }
    if (first) cout << "(none - root task)";
    cout << endl;
  }
}

/**
 * @brief Demonstrate DFS-based topological sort
 */
void demo_dfs_topological_sort(TaskGraph& g, bool verbose)
{
  cout << "\n--- DFS-based Topological Sort ---" << endl;
  cout << "Algorithm: Post-order DFS traversal" << endl;
  
  Topological_Sort<TaskGraph> sorter;
  DynDlist<TaskGraph::Node*> sorted;
  
  sorter(g, sorted);
  
  cout << "\nExecution order:" << endl;
  int step = 1;
  for (auto it = sorted.get_it(); it.has_curr(); it.next(), ++step)
  {
    auto* node = it.get_curr();
    cout << "  " << setw(2) << step << ". " << node->get_info() << endl;
  }
  
  if (verbose)
  {
    cout << "\nVerification: Each task appears after all its dependencies." << endl;
  }
}

/**
 * @brief Demonstrate BFS-based topological sort (Kahn's algorithm)
 */
void demo_bfs_topological_sort(TaskGraph& g, bool verbose)
{
  cout << "\n--- BFS-based Topological Sort (Kahn's Algorithm) ---" << endl;
  cout << "Algorithm: Iteratively remove source nodes (in-degree 0)" << endl;
  
  Q_Topological_Sort<TaskGraph> sorter;
  DynDlist<TaskGraph::Node*> sorted;
  
  sorter(g, sorted);
  
  cout << "\nExecution order:" << endl;
  int step = 1;
  for (auto it = sorted.get_it(); it.has_curr(); it.next(), ++step)
  {
    auto* node = it.get_curr();
    cout << "  " << setw(2) << step << ". " << node->get_info() << endl;
  }
  
  if (verbose)
  {
    // Show ranks (parallel execution levels)
    cout << "\nParallel execution ranks:" << endl;
    Q_Topological_Sort<TaskGraph> rank_sorter;
    auto ranks = rank_sorter.ranks(g);
    
    int rank_num = 0;
    for (auto rit = ranks.get_it(); rit.has_curr(); rit.next(), ++rank_num)
    {
      cout << "  Level " << rank_num << " (can run in parallel): ";
      auto& rank = rit.get_curr();
      bool first = true;
      for (auto nit = rank.get_it(); nit.has_curr(); nit.next())
      {
        if (not first) cout << ", ";
        cout << nit.get_curr()->get_info();
        first = false;
      }
      cout << endl;
    }
  }
}

/**
 * @brief Demonstrate practical application: build order
 */
void demo_build_order()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Example: Build System Dependencies" << endl;
  cout << string(60, '=') << endl;
  
  TaskGraph g = build_project_graph();
  print_graph(g, "Project Build Graph");
  
  cout << "\n--- Computing Build Order ---" << endl;
  
  Topological_Sort<TaskGraph> sorter;
  DynDlist<TaskGraph::Node*> build_order;
  
  sorter(g, build_order);
  
  cout << "\nBuild order (satisfies all dependencies):" << endl;
  cout << "  make ";
  bool first = true;
  for (auto it = build_order.get_it(); it.has_curr(); it.next())
  {
    if (not first) cout << " && make ";
    cout << it.get_curr()->get_info();
    first = false;
  }
  cout << endl;
}

/**
 * @brief Demonstrate practical application: course scheduling
 */
void demo_course_scheduling()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Example: University Course Prerequisites" << endl;
  cout << string(60, '=') << endl;
  
  TaskGraph g = build_courses_graph();
  print_graph(g, "Course Prerequisites Graph");
  
  cout << "\n--- Computing Course Order ---" << endl;
  
  Q_Topological_Sort<TaskGraph> sorter;
  auto semesters = sorter.ranks(g);
  
  cout << "\nSuggested course schedule:" << endl;
  int semester = 1;
  for (auto sit = semesters.get_it(); sit.has_curr(); sit.next(), ++semester)
  {
    cout << "  Semester " << semester << ": ";
    auto& courses = sit.get_curr();
    bool first = true;
    for (auto cit = courses.get_it(); cit.has_curr(); cit.next())
    {
      if (not first) cout << ", ";
      cout << cit.get_curr()->get_info();
      first = false;
    }
    cout << endl;
  }
  
  cout << "\nTotal semesters needed: " << (semester - 1) << endl;
}

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd("Topological Sort Example", ' ', "1.0");
    
    TCLAP::SwitchArg buildArg("b", "build",
      "Show build system example", false);
    TCLAP::SwitchArg courseArg("c", "courses",
      "Show course scheduling example", false);
    TCLAP::SwitchArg allArg("a", "all",
      "Run all demos", false);
    TCLAP::SwitchArg verboseArg("v", "verbose",
      "Show detailed output", false);
    
    cmd.add(buildArg);
    cmd.add(courseArg);
    cmd.add(allArg);
    cmd.add(verboseArg);
    
    cmd.parse(argc, argv);
    
    bool runBuild = buildArg.getValue();
    bool runCourse = courseArg.getValue();
    bool runAll = allArg.getValue();
    bool verbose = verboseArg.getValue();
    
    if (not runBuild and not runCourse)
      runAll = true;
    
    cout << "=== Topological Sort: Task Ordering with Dependencies ===" << endl;
    
    if (runAll or runBuild)
    {
      demo_build_order();
      
      // Also show both algorithms
      TaskGraph g = build_project_graph();
      demo_dfs_topological_sort(g, verbose);
      demo_bfs_topological_sort(g, verbose);
    }
    
    if (runAll or runCourse)
      demo_course_scheduling();
    
    cout << "\n=== Algorithm Summary ===" << endl;
    cout << "DFS-based:  O(V + E), post-order traversal" << endl;
    cout << "BFS-based:  O(V + E), Kahn's algorithm (removes sources)" << endl;
    cout << "Requirement: Input must be a DAG (no cycles)" << endl;
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
    return 1;
  }
}

