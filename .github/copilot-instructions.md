# GitHub Copilot Review Instructions for Aleph-w

## Project Context
Aleph-w is a comprehensive C++20 library for graph algorithms and data structures. All code reviews must ensure correctness, performance, maintainability, and adherence to C++20 standards and best practices.

---

## 1. C++ Language Standards & Best Practices

### C++20 Standard (Primary)
- **MUST**: This project targets **C++20** as the primary standard
- **MUST**: Use `auto` for type deduction, range-based for loops, `nullptr`, `constexpr`, `override`, `final`
- **MUST**: Prefer `std::unique_ptr`/`std::shared_ptr` over raw pointers for ownership
- **MUST**: Use structured bindings for tuple/pair decomposition
- **MUST**: Use `std::optional` instead of sentinel values or nullable pointers where semantically clear
- **MUST**: Use `if constexpr` for compile-time branching in templates
- **MUST**: Use concepts to constrain template parameters (see section below)
- **SHOULD**: Use `std::span` for non-owning views of contiguous sequences
- **SHOULD**: Consider `std::variant` for type-safe unions
- **SHOULD**: Use fold expressions for variadic template expansion
- **SHOULD**: Use three-way comparison (spaceship operator `<=>`) for comparable types
- **SHOULD**: Use designated initializers for aggregate initialization clarity
- **NICE**: Use `std::format` for type-safe string formatting (prefer over printf/streams)
- **NICE**: Use `consteval` for functions that must be evaluated at compile-time
- **NICE**: Use `constinit` for static/thread_local variables requiring constant initialization

### C++20 Concepts
- **MUST**: Use concepts to constrain template parameters for better error messages and documentation
- **MUST**: Prefer standard library concepts: `std::integral`, `std::floating_point`, `std::ranges::range`, `std::invocable`, etc.
- **SHOULD**: Define custom concepts for domain-specific constraints (e.g., `Graph`, `Node`, `WeightFunction`)
- **SHOULD**: Use `requires` clauses for complex constraints
- **NICE**: Use abbreviated function templates with concept constraints: `void foo(std::integral auto x)`

**Example:**
```cpp
template <std::ranges::range R>
requires std::integral<std::ranges::range_value_t<R>>
auto sum(const R& range) { ... }
```

### C++20 Ranges
- **SHOULD**: Use ranges library for composable, lazy algorithm pipelines
- **SHOULD**: Prefer `std::ranges::` algorithms over classic `std::` algorithms for better composability
- **SHOULD**: Use views (`std::views::filter`, `std::views::transform`, `std::views::take`, etc.) for non-owning, lazy transformations
- **NICE**: Chain range adaptors with pipe syntax: `container | views::filter(...) | views::transform(...)`

**Example:**
```cpp
auto even_squares = numbers 
  | std::views::filter([](int n) { return n % 2 == 0; })
  | std::views::transform([](int n) { return n * n; });
```

### C++20 Coroutines (if applicable)
- **SHOULD**: Consider coroutines for asynchronous graph traversals or generators
- **SHOULD**: Use `co_await`, `co_yield`, `co_return` appropriately
- **NICE**: Implement custom generators for lazy graph traversals

### C++20 Modules (future consideration)
- **NICE**: Consider migrating to modules when toolchain support is stable
- **NICE**: Modules improve compile times and eliminate header-only issues

### RAII and Resource Management
- **MUST**: All resources (memory, file handles, locks) must follow RAII principles
- **MUST**: Constructors acquire resources, destructors release them
- **MUST**: Use standard smart pointers or custom RAII wrappers
- **MUST**: Never call `delete` manually; use smart pointers
- **MUST**: Ensure exception safety (basic, strong, or no-throw guarantee)
- **SHOULD**: Prefer stack allocation over heap when possible
- **SHOULD**: Document ownership semantics in comments

### Const-Correctness
- **MUST**: Mark all non-modifying member functions as `const`
- **MUST**: Use `const` references for input parameters that won't be modified
- **MUST**: Use `const` for variables that don't change after initialization
- **SHOULD**: Prefer `const` iterators when not modifying container elements
- **SHOULD**: Use `constexpr` for compile-time constants and functions
- **SHOULD**: Use `consteval` (C++20) for functions that *must* be compile-time evaluated
- **SHOULD**: Use `constinit` (C++20) for static/thread_local variables requiring compile-time initialization
- **NICE**: Consider `mutable` for internal caching in `const` methods (document why)

### Move Semantics and Copy Elision
- **MUST**: Implement move constructors and move assignment operators for heavy objects
- **MUST**: Mark move operations as `noexcept` when possible
- **SHOULD**: Use `std::move` when transferring ownership
- **SHOULD**: Return by value to enable copy elision (NRVO/RVO)
- **SHOULD**: Avoid unnecessary copies; use references or move semantics
- **NICE**: Consider rule of five (or zero) for resource-managing classes

---

## 2. Algorithms & Complexity Analysis

### Algorithmic Correctness
- **MUST**: Verify algorithm correctness for edge cases: empty inputs, single element, duplicates
- **MUST**: Check boundary conditions and off-by-one errors
- **MUST**: Ensure graph algorithms handle disconnected graphs, self-loops, and multi-edges
- **MUST**: Verify termination conditions in loops and recursive algorithms
- **SHOULD**: Consider numerical stability for floating-point arithmetic
- **SHOULD**: Validate input preconditions (e.g., non-negative weights for Dijkstra)

### Time and Space Complexity
- **MUST**: Document time complexity using Big-O notation in Doxygen comments
- **MUST**: Document space complexity for recursive algorithms (call stack depth)
- **MUST**: Flag algorithms with worse than expected complexity (e.g., O(n²) when O(n log n) is achievable)
- **SHOULD**: Prefer O(log n) lookups (maps/sets) over O(n) scans when appropriate
- **SHOULD**: Avoid unnecessary copies that increase complexity by a factor
- **SHOULD**: Consider amortized complexity for data structures with occasional expensive operations

### Common Algorithm Classes
- **Graph Traversal**: BFS, DFS, topological sort
  - Verify visited set handling
  - Check cycle detection logic
  - Ensure proper parent/predecessor tracking
- **Shortest Paths**: Dijkstra, Bellman-Ford, Floyd-Warshall, A*
  - Verify priority queue usage (min-heap)
  - Check for negative cycle handling
  - Ensure path reconstruction correctness
- **Minimum Spanning Trees**: Kruskal, Prim
  - Verify union-find implementation (path compression, union by rank)
  - Check edge weight comparison and sorting
- **Network Flow**: Ford-Fulkerson, Edmonds-Karp, Dinic
  - Verify residual graph updates
  - Check for augmenting path termination
- **Dynamic Programming**: Check memoization, subproblem definition, and recurrence relations
- **Greedy Algorithms**: Verify correctness of greedy choice property

### Data Structures
- **Trees**: BST, AVL, Red-Black, B-trees
  - Verify balance invariants
  - Check rotation logic
  - Ensure parent pointer updates (if used)
- **Heaps**: Binary heap, Fibonacci heap, binomial heap
  - Verify heap property maintenance
  - Check merge/decrease-key operations
- **Hash Tables**: Check collision handling (chaining, open addressing)
- **Union-Find**: Ensure path compression and union by rank/size
- **Graphs**: Adjacency list vs. matrix tradeoffs; directed vs. undirected handling

---

## 3. Performance & Optimization

### Performance Considerations
- **MUST**: Avoid premature optimization; prioritize correctness and readability first
- **MUST**: Use profiling data to identify bottlenecks before optimizing
- **SHOULD**: Reserve capacity for vectors/strings when final size is known
- **SHOULD**: Use `emplace_back` instead of `push_back` to avoid copies
- **SHOULD**: Prefer `++i` over `i++` for iterators (avoid temporary)
- **SHOULD**: Avoid repeated lookups; cache iterators/references
- **SHOULD**: Use `std::unordered_map`/`std::unordered_set` when ordering is not needed
- **NICE**: Consider cache locality for performance-critical inner loops
- **NICE**: Use `constexpr` to move computations to compile-time

### Memory Efficiency
- **MUST**: Avoid memory leaks; verify with sanitizers (ASan, LSan)
- **MUST**: Use appropriate container sizes (reserve, shrink_to_fit)
- **SHOULD**: Consider space-time tradeoffs (memoization vs. recomputation)
- **SHOULD**: Use bit-packing or compact representations for large datasets
- **NICE**: Profile memory usage for large-scale graph algorithms

---

## 4. Thread Safety & Concurrency

### Concurrency Primitives
- **MUST**: Use `std::mutex`, `std::lock_guard`, `std::unique_lock` for synchronization
- **MUST**: Avoid data races; verify with ThreadSanitizer (TSan)
- **MUST**: Document thread-safety guarantees in class/function comments
- **MUST**: Use `std::atomic` for lock-free shared state (document memory ordering)
- **SHOULD**: Prefer RAII lock guards over manual lock/unlock
- **SHOULD**: Minimize critical section duration
- **SHOULD**: Avoid deadlocks using lock ordering or `std::scoped_lock` (C++17)
- **NICE**: Consider reader-writer locks (`std::shared_mutex`) for read-heavy workloads

### Parallel Algorithms
- **MUST**: Verify absence of race conditions in parallel graph traversals
- **MUST**: Ensure proper synchronization for shared data structures
- **SHOULD**: Consider work-stealing or task-based parallelism for load balancing
- **SHOULD**: Document whether algorithms are thread-safe, reentrant, or require external synchronization

---

## 5. Templates & Generic Programming

### Template Design (C++20 Focus)
- **MUST**: Use **concepts** (C++20) to constrain template parameters; avoid SFINAE when possible
- **MUST**: Provide clear error messages for template instantiation failures (concepts help here)
- **SHOULD**: Use type traits (`std::is_same`, `std::type_identity`, etc.) for compile-time dispatch when concepts are insufficient
- **SHOULD**: Prefer templates over macros for type-safe generic code
- **SHOULD**: Document template parameter requirements in Doxygen comments (concepts serve as self-documentation)
- **SHOULD**: Use abbreviated function templates with concepts: `void foo(std::integral auto x)`
- **NICE**: Use variadic templates for flexible interfaces

### Template Metaprogramming (C++20 Simplifications)
- **SHOULD**: Keep metaprogramming readable; avoid overly complex TMP
- **MUST**: Use `constexpr`/`consteval` functions instead of TMP when possible (C++20 makes this much more powerful)
- **SHOULD**: Use concepts to replace complex SFINAE-based metaprogramming
- **NICE**: Consider template specialization for type-specific optimizations when concepts are insufficient

---

## 6. STL and Standard Library Usage

### Aleph Containers vs. Standard Library (CRITICAL RULE)
- **MUST**: **ALWAYS use Aleph containers when an equivalent exists. Standard library containers are ONLY permitted when no Aleph equivalent is available.**
  - This is a strict library policy, not a suggestion or preference
  - Aleph containers are optimized for graph algorithms and provide domain-specific functionality
  
#### Mandatory Aleph Container Usage (DO NOT use STL equivalents)
- **MUST**: Use these Aleph containers instead of their STL counterparts:
  - `DynList<T>` → **NEVER** `std::list<T>` or `std::vector<T>` (dynamic list with rich functional API)
  - `DynSetTree<T>` → **NEVER** `std::set<T>` (ordered set with graph-specific operations)
  - `DynMapTree<K, V>` → **NEVER** `std::map<K, V>` (ordered map)
  - `OLhashTable<Key, Data>` → **NEVER** `std::unordered_map<Key, Data>` (open addressing linear hash table, best hash table implementation)
  - `ArrayQueue<T>`, `DynListQueue<T>` → **NEVER** `std::queue<T>`
  - `ArrayStack<T>`, `DynListStack<T>` → **NEVER** `std::stack<T>`
  - Aleph heap implementations → **NEVER** `std::priority_queue<T>`
  - `SkipList<T>` → probabilistic data structure (no direct STL equivalent)
  
#### Hash Tables: Prefer OLhashTable
- **MUST**: Use `OLhashTable<Key, Data>` as the default hash table implementation
  - OLhashTable is the preferred Aleph hash table (open addressing, linear probing)
  - Other Aleph hash tables exist but OLhashTable is recommended for general use
  - **NEVER** use `std::unordered_map` or `std::unordered_set` when a hash table is needed

#### ONLY Acceptable Use of Standard Library Containers
- **MUST**: Use standard library containers **ONLY** when:
  1. **No equivalent Aleph container exists** for the required data structure
  2. **Interfacing with external libraries** that explicitly require STL types as part of their API contract (e.g., third-party library function signatures)
  3. **`std::vector` as return type for functional programming operations** where:
     - The number of elements is **known a priori** (can be calculated before insertion)
     - Memory can be **pre-reserved** using `reserve()` before populating
     - Compactness and memory layout efficiency are critical
     - Example: `map()`, `filter()`, `zip()`, or similar functional operations where output size is deterministic
     - **MUST**: Always call `reserve(expected_size)` before insertions when using this exception
  
- **IMPORTANT**: The following are **NOT** valid reasons to use STL containers:
  - ❌ "STL might be faster" (use Aleph containers regardless)
  - ❌ "STL is more familiar" (learn Aleph containers)
  - ❌ "STL has better documentation" (consult Aleph headers and examples)
  - ❌ "Easier to integrate with std algorithms" (use Aleph containers and convert if absolutely necessary)
  - ❌ "Memory efficiency concerns" (use Aleph containers unless this is the functional programming return case above)

- **SHOULD**: Familiarize with Aleph container APIs before suggesting STL alternatives
- **SHOULD**: Check header files (`htlist.H`, `tpl_dynSetTree.H`, `tpl_dynMapTree.H`, etc.) for available Aleph containers

### Standard Library Container Selection (when Aleph containers are not applicable)
- **MUST**: Choose appropriate containers based on access patterns and complexity requirements
  - `std::vector`: random access, cache-friendly, dynamic array
  - `std::deque`: double-ended queue, stable references
  - `std::list`: stable iterators, frequent insertions/deletions
  - `std::map`/`std::set`: ordered, O(log n) operations
  - `std::unordered_map`/`std::unordered_set`: unordered, O(1) average operations
- **SHOULD**: Prefer `std::array` over C-style arrays when size is known at compile-time
- **SHOULD**: Use `std::priority_queue` for heaps only if Aleph heaps are not suitable

### Algorithm Usage (C++20 Ranges Preferred)
- **MUST**: Use algorithms over hand-rolled loops for readability and correctness
- **SHOULD**: Prefer `std::ranges::` algorithms (`std::ranges::sort`, `std::ranges::find`, etc.) over classic `std::` algorithms
- **SHOULD**: Use range views for composable, lazy transformations
- **SHOULD**: Use range-based for loops for simple iterations
- **SHOULD**: Consider parallel algorithms with execution policies (`std::execution::par`, `std::execution::par_unseq`) for performance-critical code
- **NICE**: Chain range adaptors for expressive, functional-style code

---

## 7. Error Handling

### Aleph Exception Macros (CRITICAL RULE)
- **MUST**: **ALWAYS use Aleph exception macros from `ah-errors.H` instead of raw `throw` statements**
  - Aleph macros automatically include file location (`file:line`) in error messages
  - Provides consistent error reporting across the library
  - Supports stream-style error message composition
  
- **MUST**: Use these Aleph exception macros (defined in `ah-errors.H`):
  - `ah_domain_error_if(condition)` → for domain/logic errors (replaces `throw std::domain_error`)
  - `ah_runtime_error_if(condition)` → for runtime errors (replaces `throw std::runtime_error`)
  - `ah_range_error_if(condition)` → for out-of-range errors (replaces `throw std::range_error`)
  - `ah_invalid_argument_if(condition)` → for invalid arguments (replaces `throw std::invalid_argument`)
  - Unconditional versions: `ah_domain_error()`, `ah_runtime_error()`, `ah_range_error()`, `ah_invalid_argument()`
  - `_unless` variants: `ah_domain_error_unless(condition)`, etc. (inverted logic)
  - `ah_fatal_error()` → for unrecoverable fatal errors

- **Example (Aleph style - CORRECT)**:
  ```cpp
  ah_domain_error_if(x < 0) << "sqrt requires non-negative value, got " << x;
  ah_range_error_if(index >= size) << "Index " << index << " out of range [0, " << size << ")";
  ah_runtime_error_unless(file.is_open()) << "Failed to open file: " << filename;
  ```

- **Example (Raw throw - INCORRECT, DO NOT USE)**:
  ```cpp
  if (x < 0)  // ❌ DO NOT DO THIS
    throw std::domain_error("sqrt requires non-negative value");
  ```

### Exception Safety
- **MUST**: Document exception safety guarantees (no-throw, strong, basic)
- **MUST**: Use RAII to ensure cleanup on exception
- **MUST**: Catch exceptions by const reference: `catch (const std::exception& e)`
- **SHOULD**: Mark functions as `noexcept` when they guarantee no exceptions
- **NICE**: Avoid exceptions in destructors and move operations

### Assertions and Preconditions
- **MUST**: Use assertions (`assert`, custom `ah-errors.H` macros) for preconditions
- **MUST**: Validate external inputs before processing
- **SHOULD**: Document preconditions in Doxygen `@pre` tags
- **SHOULD**: Fail fast with clear error messages

---

## 8. Code Quality & Maintainability

### Readability
- **MUST**: Use descriptive variable and function names
- **MUST**: Keep functions short and focused (single responsibility)
- **MUST**: Avoid deep nesting (max 3-4 levels); extract functions or use early returns
- **SHOULD**: Prefer explicit over implicit (avoid type conversions that hide intent)
- **SHOULD**: Avoid magic numbers; use named constants or enums
- **SHOULD**: Use whitespace and formatting consistently

### Code Duplication
- **MUST**: Refactor duplicated code into reusable functions or templates
- **SHOULD**: Use inheritance or composition to share behavior
- **SHOULD**: Avoid copy-paste programming

### Cyclomatic Complexity
- **MUST**: Keep cyclomatic complexity low (< 10 per function ideally)
- **SHOULD**: Break complex functions into smaller helpers
- **SHOULD**: Use lookup tables or polymorphism instead of long switch/if chains

---

## 9. Documentation (Doxygen)

### Required Documentation
- **MUST**: All headers must have a Doxygen file-level comment with:
  - `@file` directive
  - Brief description (`@brief`)
  - MIT license header
  - Author and date if applicable
- **MUST**: All public classes must have `@brief` and detailed description
- **MUST**: **If a new public method or class is added, it MUST be fully documented with Doxygen.**
- **MUST**: All public member functions must have:
  - `@brief` description
  - `@param` for each parameter
  - `@return` if function returns a value
  - `@throws` if function can throw exceptions
  - Time/space complexity in `@note` or description
- **MUST**: Document preconditions (`@pre`) and postconditions (`@post`) when relevant
- **SHOULD**: Document thread-safety guarantees
- **SHOULD**: Provide usage examples in `@code` blocks for complex APIs

### Documentation Quality
- **MUST**: Doxygen must generate no warnings for changed files
- **MUST**: Use proper English grammar and punctuation
- **SHOULD**: Keep descriptions concise but complete
- **SHOULD**: Cross-reference related classes/functions with `@see`

---

## 10. Testing Requirements

### Test Coverage
- **MUST**: **If a new public method or class is added, a corresponding unit test MUST be created.**
  - The test must verify the method's contract (preconditions, postconditions, return values).
  - The test must be added to the appropriate test file in `Tests/` (e.g., `Tests/ClassName_test.cc`).
- **MUST**: If headers are modified, corresponding tests must be added or updated
- **MUST**: Tests must cover edge cases: empty inputs, single element, large inputs
- **MUST**: Tests must verify algorithmic correctness, not just absence of crashes
- **SHOULD**: Include performance regression tests for critical algorithms
- **SHOULD**: Use property-based testing or randomized tests for complex algorithms

### Test Quality
- **MUST**: Tests must be deterministic and repeatable
- **MUST**: Tests must have clear failure messages
- **SHOULD**: Use GTest framework conventions (ASSERT_*, EXPECT_*)
- **SHOULD**: Organize tests by functionality (setup, action, assertion)

---

## 11. License & Legal

### MIT License
- **MUST**: All `.H` header files must include the MIT license header at the top
- **MUST**: License header must include copyright notice
- **SHOULD**: Keep license text consistent with `LICENSE` file

---

## 12. Code Review Checklist

When reviewing a pull request, verify the following:

### Correctness
- [ ] Algorithm logic is correct for all cases (empty, single, typical, edge)
- [ ] No off-by-one errors or boundary issues
- [ ] Proper handling of nullptr, invalid inputs, and error conditions
- [ ] No undefined behavior (uninitialized variables, dangling pointers, data races)

### Performance
- [ ] Time complexity is as expected and documented
- [ ] No unnecessary copies or allocations in hot paths
- [ ] Appropriate data structures and algorithms selected

### C++ Best Practices
- [ ] RAII for all resource management
- [ ] Const-correctness throughout
- [ ] Move semantics used where beneficial
- [ ] Smart pointers for ownership, raw pointers only for observation
- [ ] Exception-safe code with proper RAII cleanup

### Concurrency (if applicable)
- [ ] No data races (verify with TSan)
- [ ] Proper synchronization primitives used
- [ ] Deadlock-free lock ordering
- [ ] Thread-safety documented

### Documentation
- [ ] All public APIs documented with Doxygen
- [ ] New methods/classes fully documented
- [ ] Complexity analysis included
- [ ] Preconditions and postconditions documented
- [ ] No Doxygen warnings for changed files
- [ ] MIT license header present in all `.H` files

### Testing
- [ ] Tests added/updated for modified headers
- [ ] New public methods have corresponding unit tests
- [ ] Edge cases covered in tests
- [ ] Tests pass locally and in CI

### Style & Maintainability
- [ ] Code follows project conventions
- [ ] Descriptive names, no magic numbers
- [ ] Low cyclomatic complexity
- [ ] No code duplication
- [ ] Clear separation of concerns

---

## 13. Common Issues to Flag

### Critical Issues (Block Merge)
- Memory leaks or undefined behavior
- Data races or deadlocks
- Incorrect algorithm logic or complexity
- Missing Doxygen documentation for new or modified code
- Missing MIT license header in `.H` files
- Test failures or missing tests for header changes/new features

### Important Issues (Should Fix)
- Non-const member functions that should be const
- Raw pointers where smart pointers should be used
- Missing move semantics for heavy objects
- Inefficient algorithms or data structures
- Missing `noexcept` on move operations
- Poor error handling or exception safety

### Style Issues (Nice to Fix)
- Long functions that should be split
- Magic numbers instead of named constants
- Inconsistent naming conventions
- Overly complex nested logic
- Code duplication

---

## 14. C++20 Feature Checklist

When reviewing code, actively look for opportunities to use C++20 features:

### Concepts
- [ ] Template parameters constrained with concepts instead of SFINAE
- [ ] Standard library concepts used (`std::integral`, `std::ranges::range`, etc.)
- [ ] Custom concepts defined for domain constraints

### Ranges
- [ ] `std::ranges::` algorithms used instead of classic algorithms where appropriate
- [ ] Range views used for lazy transformations
- [ ] Pipe syntax for composable range operations

### Modern Language Features
- [ ] Three-way comparison (`<=>`) for comparable types
- [ ] Designated initializers for clarity in aggregate initialization
- [ ] `std::span` for non-owning sequence views
- [ ] `std::format` for type-safe formatting
- [ ] `consteval`/`constinit` where appropriate

### Performance
- [ ] Parallel execution policies considered for large datasets
- [ ] Range views used to avoid intermediate containers

---

## 15. Review Tone & Format

- Be constructive and specific in feedback
- Reference line numbers and code snippets
- Suggest concrete improvements with examples
- Explain *why* a change improves the code (performance, safety, maintainability)
- Acknowledge good practices and clever solutions
- Use severity labels: **CRITICAL**, **IMPORTANT**, **NICE**, **QUESTION**

---

## Examples of Good Feedback

### Example 1: Algorithm Complexity
```
**IMPORTANT**: Line 42: This nested loop results in O(n²) complexity. 
Consider using std::unordered_set for O(1) lookups, reducing to O(n).

Suggested change:
std::unordered_set<int> seen;
for (const auto& item : items) {
  if (seen.count(item)) return true;
  seen.insert(item);
}
```

### Example 2: Const-Correctness
```
**IMPORTANT**: Line 87: Method `get_size()` doesn't modify state; mark as const.

Suggested change:
-size_t get_size() { return size; }
+size_t get_size() const { return size; }
```

### Example 3: Documentation
```
**CRITICAL**: Missing Doxygen documentation for public method `find_shortest_path`.

Add:
/** @brief Finds the shortest path between source and target using Dijkstra's algorithm.
 *  @param source Starting node
 *  @param target Destination node
 *  @return Vector of nodes forming the path, or empty if no path exists
 *  @throws std::invalid_argument if source or target not in graph
 *  @note Time complexity: O((V + E) log V), Space: O(V)
 */
```

---

## Priority Levels

- **MUST**: Mandatory requirement; blocking issue if violated
- **SHOULD**: Strongly recommended; may block merge for repeated violations
- **NICE**: Optional improvement; enhances code quality but not required

Focus on **MUST** items first, then **SHOULD**, then **NICE** to keep reviews efficient.
