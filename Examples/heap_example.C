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
 * @file heap_example.C
 * @brief Priority Queues: Binary Heap vs Fibonacci Heap
 * 
 * This example compares two heap implementations for priority queues:
 * - **Binary Heap**: Simple, cache-friendly, O(log n) operations
 * - **Fibonacci Heap**: O(1) amortized insert/decrease-key
 * 
 * ## When to Use Which?
 * 
 * ### Binary Heap
 * - General-purpose priority queue
 * - Simple heap sort
 * - When decrease-key is rare or not needed
 * 
 * ### Fibonacci Heap
 * - Dijkstra's algorithm (many decrease-key operations)
 * - Prim's MST algorithm
 * - When O(1) amortized decrease-key is critical
 * 
 * ## Complexity Comparison
 * 
 * | Operation     | Binary Heap | Fibonacci Heap |
 * |---------------|-------------|----------------|
 * | insert        | O(log n)    | O(1) amortized |
 * | find-min      | O(1)        | O(1)           |
 * | extract-min   | O(log n)    | O(log n) amor. |
 * | decrease-key  | O(log n)    | O(1) amortized |
 * | merge         | O(n)        | O(1)           |
 * 
 * @see tpl_binHeap.H
 * @author Leandro Rabindranath León
 * @ingroup Examples
 * @see tpl_dynBinHeap.H
 * @see tpl_fibonacci_heap.H
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <random>
#include <tpl_dynBinHeap.H>
#include <tpl_fibonacci_heap.H>
#include <tclap/CmdLine.h>

using namespace std;
using namespace Aleph;

/**
 * @brief Task with priority for job scheduling
 */
struct Task
{
  string name;
  int priority;      // Lower = higher priority
  int duration_ms;
  
  Task() : name(""), priority(0), duration_ms(0) {}
  Task(const string& n, int p, int d) : name(n), priority(p), duration_ms(d) {}
  
  bool operator<(const Task& other) const { return priority < other.priority; }
  bool operator>(const Task& other) const { return priority > other.priority; }
};

ostream& operator<<(ostream& os, const Task& t)
{
  return os << t.name << " (priority=" << t.priority << ", " << t.duration_ms << "ms)";
}

/**
 * @brief Demonstrate basic binary heap operations
 */
void demo_binary_heap()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Binary Heap: Basic Operations" << endl;
  cout << string(60, '=') << endl;
  
  cout << "\n--- Integer Min-Heap ---" << endl;
  
  DynBinHeap<int> heap;
  
  // Insert elements
  vector<int> values = {5, 3, 8, 1, 9, 2, 7, 4, 6};
  cout << "Inserting: ";
  for (int v : values)
  {
    cout << v << " ";
    heap.insert(v);
  }
  cout << endl;
  
  cout << "Heap size: " << heap.size() << endl;
  cout << "Minimum: " << heap.top() << endl;
  
  // Extract all in sorted order
  cout << "\nExtracting in order: ";
  while (not heap.is_empty())
  {
    cout << heap.get() << " ";  // get() removes and returns
  }
  cout << endl;
  
  cout << "\n--- Task Priority Queue ---" << endl;
  
  DynBinHeap<Task> task_queue;
  
  // Add tasks with different priorities
  task_queue.insert(Task("Background sync", 5, 1000));
  task_queue.insert(Task("User input", 1, 10));
  task_queue.insert(Task("Network request", 3, 200));
  task_queue.insert(Task("Logging", 4, 50));
  task_queue.insert(Task("Critical alert", 0, 5));
  task_queue.insert(Task("Data processing", 2, 500));
  
  cout << "Processing tasks by priority:" << endl;
  int order = 1;
  while (not task_queue.is_empty())
  {
    Task t = task_queue.get();
    cout << "  " << order++ << ". " << t << endl;
  }
}

/**
 * @brief Demonstrate Fibonacci heap operations
 */
void demo_fibonacci_heap()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Fibonacci Heap: Advanced Operations" << endl;
  cout << string(60, '=') << endl;
  
  Fibonacci_Heap<int> fib_heap;
  
  // Insert elements and keep handles for decrease-key
  cout << "\n--- Insert and Decrease-Key ---" << endl;
  
  vector<Fibonacci_Heap<int>::Node*> handles;
  
  cout << "Inserting: ";
  for (int v : {50, 30, 80, 10, 90, 20, 70})
  {
    cout << v << " ";
    handles.push_back(fib_heap.insert(v));
  }
  cout << endl;
  
  cout << "Current minimum: " << fib_heap.get_min() << endl;
  
  // Decrease key of 80 to 5
  cout << "\nDecreasing 80 to 5..." << endl;
  fib_heap.decrease_key(handles[2], 5);  // Handle for 80
  cout << "New minimum: " << fib_heap.get_min() << endl;
  
  // Decrease key of 90 to 3
  cout << "Decreasing 90 to 3..." << endl;
  fib_heap.decrease_key(handles[4], 3);  // Handle for 90
  cout << "New minimum: " << fib_heap.get_min() << endl;
  
  cout << "\nExtracting all: ";
  while (not fib_heap.is_empty())
    cout << fib_heap.extract_min() << " ";
  cout << endl;
  
  // Demonstrate merge operation
  cout << "\n--- Heap Merge (O(1) in Fibonacci Heap) ---" << endl;
  
  Fibonacci_Heap<int> heap1, heap2;
  
  cout << "Heap 1: ";
  for (int v : {10, 30, 50})
  {
    cout << v << " ";
    heap1.insert(v);
  }
  cout << endl;
  
  cout << "Heap 2: ";
  for (int v : {20, 40, 60})
  {
    cout << v << " ";
    heap2.insert(v);
  }
  cout << endl;
  
  heap1.merge(heap2);
  cout << "After merge (heap1 absorbs heap2):" << endl;
  cout << "  Heap 1 size: " << heap1.size() << endl;
  cout << "  Heap 1 min: " << heap1.get_min() << endl;
  cout << "  Heap 2 size: " << heap2.size() << " (empty)" << endl;
  
  cout << "\nExtracting merged heap: ";
  while (not heap1.is_empty())
    cout << heap1.extract_min() << " ";
  cout << endl;
}

/**
 * @brief Practical example: Event-driven simulation
 */
void demo_event_simulation()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Practical Example: Event-Driven Simulation" << endl;
  cout << string(60, '=') << endl;
  
  struct Event
  {
    double time;
    string description;
    
    Event() : time(0), description("") {}
    Event(double t, const string& d) : time(t), description(d) {}
    
    bool operator<(const Event& other) const { return time < other.time; }
  };
  
  DynBinHeap<Event> event_queue;
  
  // Schedule events in random order
  event_queue.insert(Event(1.5, "Customer arrives"));
  event_queue.insert(Event(0.5, "Open store"));
  event_queue.insert(Event(3.0, "Customer leaves"));
  event_queue.insert(Event(2.0, "Process payment"));
  event_queue.insert(Event(4.5, "Delivery arrives"));
  event_queue.insert(Event(2.5, "Answer phone"));
  event_queue.insert(Event(5.0, "Close store"));
  
  cout << "\nSimulation timeline:" << endl;
  cout << string(40, '-') << endl;
  
  while (not event_queue.is_empty())
  {
    Event e = event_queue.get();
    cout << fixed << setprecision(1);
    cout << "  t=" << setw(4) << e.time << ": " << e.description << endl;
  }
}

/**
 * @brief Performance comparison between heap types
 */
void demo_performance_comparison(int n)
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Performance Comparison (n = " << n << ")" << endl;
  cout << string(60, '=') << endl;
  
  random_device rd;
  mt19937 gen(rd());
  uniform_int_distribution<> dis(1, n * 10);
  
  vector<int> values(n);
  for (int i = 0; i < n; ++i)
    values[i] = dis(gen);
  
  // Binary Heap
  {
    DynBinHeap<int> heap;
    
    auto start = chrono::high_resolution_clock::now();
    
    for (int v : values)
      heap.insert(v);
    
    auto mid = chrono::high_resolution_clock::now();
    
    while (not heap.is_empty())
      (void)heap.get();
    
    auto end = chrono::high_resolution_clock::now();
    
    auto insert_time = chrono::duration_cast<chrono::microseconds>(mid - start).count();
    auto extract_time = chrono::duration_cast<chrono::microseconds>(end - mid).count();
    
    cout << "\nBinary Heap (DynBinHeap):" << endl;
    cout << "  Insert " << n << " elements: " << insert_time << " us" << endl;
    cout << "  Extract all elements: " << extract_time << " us" << endl;
    cout << "  Total: " << (insert_time + extract_time) << " us" << endl;
  }
  
  // Fibonacci Heap
  {
    Fibonacci_Heap<int> heap;
    
    auto start = chrono::high_resolution_clock::now();
    
    for (int v : values)
      heap.insert(v);
    
    auto mid = chrono::high_resolution_clock::now();
    
    while (not heap.is_empty())
      (void)heap.extract_min();
    
    auto end = chrono::high_resolution_clock::now();
    
    auto insert_time = chrono::duration_cast<chrono::microseconds>(mid - start).count();
    auto extract_time = chrono::duration_cast<chrono::microseconds>(end - mid).count();
    
    cout << "\nFibonacci Heap:" << endl;
    cout << "  Insert " << n << " elements: " << insert_time << " us" << endl;
    cout << "  Extract all elements: " << extract_time << " us" << endl;
    cout << "  Total: " << (insert_time + extract_time) << " us" << endl;
  }
  
  // Decrease-key comparison
  cout << "\n--- Decrease-Key Performance ---" << endl;
  
  int decreases = n / 2;
  vector<int> decrease_amounts(decreases);
  for (int i = 0; i < decreases; ++i)
    decrease_amounts[i] = dis(gen);
  
  // Fibonacci Heap with decrease-key
  {
    Fibonacci_Heap<int> heap;
    vector<Fibonacci_Heap<int>::Node*> handles(n);
    
    for (int i = 0; i < n; ++i)
      handles[i] = heap.insert(values[i] + n * 10);  // Start with large values
    
    auto start = chrono::high_resolution_clock::now();
    
    for (int i = 0; i < decreases; ++i)
    {
      // Decrease key to a smaller value
      heap.decrease_key(handles[i], decrease_amounts[i]);
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto time = chrono::duration_cast<chrono::microseconds>(end - start).count();
    
    cout << "\nFibonacci Heap decrease-key (" << decreases << " operations): " << time << " us" << endl;
    cout << "  Average per decrease-key: " << (time * 1000.0 / decreases) << " ns" << endl;
  }
  
  cout << "\n--- Analysis ---" << endl;
  cout << "Binary Heap: Better cache locality, simpler, good for general use" << endl;
  cout << "Fibonacci Heap: Better for algorithms with many decrease-key ops" << endl;
  cout << "  (e.g., Dijkstra, Prim where decrease-key dominates)" << endl;
}

/**
 * @brief Demonstrate max-heap usage
 */
void demo_max_heap()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Max-Heap: Largest Element First" << endl;
  cout << string(60, '=') << endl;
  
  // Use greater<int> for max-heap behavior
  DynBinHeap<int, Aleph::greater<int>> max_heap;
  
  cout << "\nInserting: ";
  for (int v : {5, 3, 8, 1, 9, 2, 7})
  {
    cout << v << " ";
    max_heap.insert(v);
  }
  cout << endl;
  
  cout << "Maximum: " << max_heap.top() << endl;
  
  cout << "Extracting in descending order: ";
  while (not max_heap.is_empty())
    cout << max_heap.get() << " ";
  cout << endl;
  
  // Fibonacci max-heap
  Fibonacci_Heap<int, Aleph::greater<int>> fib_max;
  
  cout << "\nFibonacci Max-Heap: ";
  for (int v : {15, 13, 18, 11, 19, 12, 17})
  {
    cout << v << " ";
    fib_max.insert(v);
  }
  cout << endl;
  
  cout << "Maximum: " << fib_max.get_min() << endl;  // "min" is max in max-heap
  cout << "Extracting: ";
  while (not fib_max.is_empty())
    cout << fib_max.extract_min() << " ";
  cout << endl;
}

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd("Heap Data Structures Example", ' ', "1.0");
    
    TCLAP::ValueArg<int> nArg("n", "count",
      "Number of elements for performance test", false, 10000, "int");
    TCLAP::SwitchArg basicArg("b", "basic",
      "Show basic binary heap operations", false);
    TCLAP::SwitchArg fibArg("f", "fibonacci",
      "Show Fibonacci heap operations", false);
    TCLAP::SwitchArg simArg("s", "simulation",
      "Show event simulation example", false);
    TCLAP::SwitchArg perfArg("p", "performance",
      "Run performance comparison", false);
    TCLAP::SwitchArg maxArg("m", "max",
      "Show max-heap examples", false);
    TCLAP::SwitchArg allArg("a", "all",
      "Run all demos", false);
    
    cmd.add(nArg);
    cmd.add(basicArg);
    cmd.add(fibArg);
    cmd.add(simArg);
    cmd.add(perfArg);
    cmd.add(maxArg);
    cmd.add(allArg);
    
    cmd.parse(argc, argv);
    
    int n = nArg.getValue();
    bool runBasic = basicArg.getValue();
    bool runFib = fibArg.getValue();
    bool runSim = simArg.getValue();
    bool runPerf = perfArg.getValue();
    bool runMax = maxArg.getValue();
    bool runAll = allArg.getValue();
    
    if (not runBasic and not runFib and not runSim and not runPerf and not runMax)
      runAll = true;
    
    cout << "=== Priority Queues: Binary Heap vs Fibonacci Heap ===" << endl;
    
    if (runAll or runBasic)
      demo_binary_heap();
    
    if (runAll or runFib)
      demo_fibonacci_heap();
    
    if (runAll or runSim)
      demo_event_simulation();
    
    if (runAll or runMax)
      demo_max_heap();
    
    if (runAll or runPerf)
      demo_performance_comparison(n);
    
    cout << "\n=== Summary ===" << endl;
    cout << "DynBinHeap: General-purpose, simple, cache-friendly" << endl;
    cout << "Fibonacci_Heap: O(1) amortized decrease-key, best for graph algorithms" << endl;
    cout << "Use DynBinHeap for most cases; Fibonacci for Dijkstra/Prim with many updates" << endl;
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
    return 1;
  }
}

