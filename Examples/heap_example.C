
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/**
 * @file heap_example.C
 * @brief Priority Queues: Binary Heap vs Fibonacci Heap
 * 
 * This example compares two fundamental heap implementations for priority
 * queues, demonstrating when to use each and their performance characteristics.
 * Priority queues are essential for many algorithms, and choosing the right
 * implementation can significantly impact performance.
 *
 * ## What is a Priority Queue?
 *
 * A priority queue is a data structure that supports:
 * - **insert(x)**: Add element with priority
 * - **extract_min()**: Remove and return element with highest priority
 * - **decrease_key(x, k)**: Lower priority of element x to k
 * - **find_min()**: View highest priority element without removal
 *
 * Elements are ordered by priority (typically: lower value = higher priority).
 *
 * ## Two Heap Implementations Compared
 *
 * ### Binary Heap
 *
 * A complete binary tree stored in an array satisfying the heap property
 * (parent ≤ children for min-heap).
 *
 * **Characteristics**:
 * - **Simple**: Easy to understand and implement
 * - **Cache-friendly**: Array-based, good memory locality
 * - **Predictable**: O(log n) worst-case for all operations
 * - **Space efficient**: No pointer overhead
 *
 * **Operations**:
 * - **insert**: O(log n) - bubble up
 * - **extract_min**: O(log n) - bubble down
 * - **decrease_key**: O(log n) - bubble up
 * - **find_min**: O(1) - root element
 *
 * ### Fibonacci Heap
 *
 * A more sophisticated heap structure using a collection of trees with
 * lazy consolidation, achieving better amortized complexity.
 *
 * **Characteristics**:
 * - **Complex**: More sophisticated implementation
 * - **Amortized performance**: Better average-case complexity
 * - **Lazy consolidation**: Defers work until necessary
 * - **Theoretical optimal**: Best known complexity for some operations
 *
 * **Operations**:
 * - **insert**: O(1) amortized - just add to root list
 * - **extract_min**: O(log n) amortized - consolidate trees
 * - **decrease_key**: O(1) amortized - cut and promote
 * - **find_min**: O(1) - pointer to minimum
 * - **merge**: O(1) - concatenate root lists
 *
 * ## Complexity Comparison
 *
 * | Operation | Binary Heap | Fibonacci Heap | Winner |
 * |-----------|-------------|----------------|--------|
 * | insert | O(log n) | O(1) amortized | Fibonacci |
 * | find-min | O(1) | O(1) | Tie |
 * | extract-min | O(log n) | O(log n) amortized | Tie |
 * | decrease-key | O(log n) | O(1) amortized | Fibonacci |
 * | merge | O(n) | O(1) | Fibonacci |
 *
 * **Note**: Amortized complexity means average over many operations.
 * Individual operations may be slower, but overall performance is better.
 *
 * ## When to Use Which?
 *
 * ### Use Binary Heap When:
 *
 * ✅ **General-purpose priority queue**
 *   - Most common use case
 *   - Simple implementation preferred
 *   - Predictable performance needed
 *
 * ✅ **Heap sort**
 *   - In-place sorting algorithm
 *   - O(n log n) time, O(1) extra space
 *
 * ✅ **Decrease-key is rare**
 *   - Most operations are insert/extract
 *   - Few or no decrease-key operations
 *
 * ✅ **Memory matters**
 *   - Array-based, no pointer overhead
 *   - Better cache performance
 *
 * ### Use Fibonacci Heap When:
 *
 * ✅ **Many decrease-key operations**
 *   - Algorithms like Dijkstra's (many decrease-key calls)
 *   - Prim's MST algorithm
 *   - When decrease-key dominates runtime
 *
 * ✅ **Theoretical optimality needed**
 *   - Academic/research applications
 *   - When best complexity matters
 *
 * ✅ **Merge operations needed**
 *   - Frequently merging heaps
 *   - O(1) merge is critical
 *
 * ## Real-World Performance
 *
 * Despite better theoretical complexity, Fibonacci heaps often have:
 * - **Higher constant factors**: More complex operations
 * - **Worse cache performance**: Pointer-based structure
 * - **More memory overhead**: Multiple pointers per node
 *
 * **Result**: Binary heaps are often faster in practice for typical workloads!
 *
 * **Rule of thumb**: Use binary heap unless you have a specific need for
 * Fibonacci heap's advantages (many decrease-key operations).
 *
 * ## Applications
 *
 * ### Binary Heap Applications
 * - **Task scheduling**: Operating system schedulers
 * - **Event simulation**: Discrete event simulation
 * - **Graph algorithms**: A* search, some shortest path variants
 * - **Heap sort**: Efficient in-place sorting
 *
 * ### Fibonacci Heap Applications
 * - **Dijkstra's algorithm**: Many decrease-key operations
 * - **Prim's MST**: Minimum spanning tree with decrease-key
 * - **Network flow**: Some flow algorithms benefit
 * - **Theoretical algorithms**: When optimal complexity matters
 *
 * ## Example: Dijkstra's Algorithm
 *
 * Dijkstra's algorithm for shortest paths:
 * - **Operations**: Many decrease-key calls (update distances)
 * - **Binary heap**: O(E log V) total
 * - **Fibonacci heap**: O(E + V log V) total (better!)
 *
 * For dense graphs (E ≈ V²), Fibonacci heap provides significant speedup.
 *
 * ## Usage
 *
 * ```bash
 * # Run all demos (default if no flags are given)
 * ./heap_example
 *
 * # Explicitly run all demos
 * ./heap_example --all
 *
 * # Run specific demos
 * ./heap_example --basic
 * ./heap_example --fibonacci
 * ./heap_example --simulation
 * ./heap_example --performance
 * ./heap_example --max
 *
 * # Show help
 * ./heap_example --help
 *
 * # Performance comparison size
 * ./heap_example --performance --count 50000
 * ```
 *
 * @see tpl_binHeap.H Binary heap implementation
 * @see tpl_dynBinHeap.H Dynamic binary heap
 * @see tpl_fibonacci_heap.H Fibonacci heap implementation
 * @see writeHeap.C Heap visualization
 * @author Leandro Rabindranath León
 * @ingroup Examples
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
    (void) heap1.insert(v);
  }
  cout << endl;
  
  cout << "Heap 2: ";
  for (int v : {20, 40, 60})
  {
    cout << v << " ";
    (void) heap2.insert(v);
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
      (void) heap.insert(v);
    
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
    (void) fib_max.insert(v);
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

