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

#ifndef AH_PARALLEL_H
#define AH_PARALLEL_H

/** @file ah-parallel.H
    @brief Parallel functional programming operations using ThreadPool.

    This file provides parallel versions of common functional operations
    like map, filter, fold, for_each, all, exists, etc. These operations
    leverage multi-core processors to accelerate data processing.

    All functions in this file require a ThreadPool instance and operate
    on STL-compatible containers (std::vector, std::deque, std::array)
    or Aleph-w containers with random access (Array, DynArray).

    ## Key Features

    - **Zero-copy where possible**: Operations on references avoid copying
    - **Automatic chunking**: Work is divided optimally across threads
    - **Short-circuit evaluation**: `pall`, `pexists`, `pfind` stop early
    - **Exception propagation**: Exceptions from any thread are captured

    ## Available Operations

    | Function | Description | Time Complexity |
    |----------|-------------|-----------------|
    | `pmaps`  | Parallel map | O(n/p) |
    | `pfilter` | Parallel filter | O(n/p) |
    | `pfoldl` | Parallel reduce | O(n/p + p) |
    | `pfor_each` | Parallel for_each | O(n/p) |
    | `pall` | Parallel all predicate | O(n/p) best, O(n) worst |
    | `pexists` | Parallel exists | O(1) best, O(n/p) worst |
    | `pnone` | Parallel none | O(n/p) |
    | `pcount_if` | Parallel count | O(n/p) |
    | `pfind` | Parallel find | O(1) best, O(n/p) worst |
    | `psum` | Parallel sum | O(n/p + p) |
    | `pproduct` | Parallel product | O(n/p + p) |
    | `pminmax` | Parallel min/max | O(n/p + p) |

    Where n = elements, p = threads.

    ## Usage Examples

    @code
    #include <ah-parallel.H>
    #include <vector>

    using namespace Aleph;

    ThreadPool pool(8);  // 8 worker threads

    std::vector<int> data(1000000);
    std::iota(data.begin(), data.end(), 0);

    // Parallel map: square each element
    auto squares = pmaps<long long>(pool, data, [](int x) { 
      return static_cast<long long>(x) * x; 
    });

    // Parallel filter: keep even numbers
    auto evens = pfilter(pool, data, [](int x) { return x % 2 == 0; });

    // Parallel sum
    long long total = psum(pool, data);

    // Parallel find
    auto it = pfind(pool, data, [](int x) { return x == 500000; });
    @endcode

    ## Thread Safety

    - All functions are thread-safe with respect to the ThreadPool
    - Input containers should not be modified during parallel operations
    - Output containers are constructed locally and returned by value

    @ingroup Algos
    @author Leandro Rabindranath León
*/

#include <vector>
#include <atomic>
#include <optional>
#include <algorithm>
#include <numeric>
#include <type_traits>
#include <iterator>
#include <functional>
#include <thread_pool.H>

namespace Aleph
{

// =============================================================================
// Implementation Details
// =============================================================================

namespace parallel_detail
{
  /// Calculate optimal chunk size based on data size and thread count
  inline size_t chunk_size(size_t n, size_t num_threads, size_t min_chunk = 64)
  {
    if (n == 0) return 1;
    // Use more chunks than threads for better load balancing
    size_t chunks = num_threads * 4;
    size_t size = (n + chunks - 1) / chunks;
    return std::max(size, min_chunk);
  }

  /// Check if container supports random access
  template <typename Container>
  constexpr bool has_random_access()
  {
    using It = decltype(std::begin(std::declval<Container&>()));
    return std::is_base_of_v<std::random_access_iterator_tag,
                             typename std::iterator_traits<It>::iterator_category>;
  }

  /// For containers with random access, just return a pointer to it
  /// For non-random access, copy to vector
  template <typename Container>
  auto ensure_random_access(const Container& c)
  {
    if constexpr (has_random_access<Container>())
      return &c;  // Return pointer to original
    else
      return std::make_unique<std::vector<typename Container::value_type>>(
        std::begin(c), std::end(c));
  }

  /// Get reference from pointer or unique_ptr
  template <typename T>
  decltype(auto) deref(T&& ptr)
  {
    if constexpr (std::is_pointer_v<std::decay_t<T>>)
      return *ptr;
    else
      return *ptr;  // unique_ptr also supports *
  }

} // namespace parallel_detail

// =============================================================================
// Parallel Map
// =============================================================================

/** @brief Parallel map operation.

    Applies a function to each element in parallel and returns a new
    container with the results.

    @tparam ResultT Result element type (deduced from Op if not specified).
    @tparam Container Input container type.
    @tparam Op Mapping operation type.

    @param pool Thread pool to use.
    @param c Source container.
    @param op Function to apply: `ResultT op(const T&)`.
    @param chunk_size Elements per chunk (0 = auto).

    @return std::vector<ResultT> with mapped elements.

    ## Example

    @code
    std::vector<int> nums = {1, 2, 3, 4, 5};
    auto squares = pmaps<int>(pool, nums, [](int x) { return x * x; });
    // squares = {1, 4, 9, 16, 25}

    // Type deduction:
    auto strings = pmaps(pool, nums, [](int x) { return std::to_string(x); });
    // strings = {"1", "2", "3", "4", "5"}
    @endcode

    @ingroup Algos
*/
template <typename ResultT = void, typename Container, typename Op>
[[nodiscard]] auto pmaps(ThreadPool& pool, const Container& c, Op op,
                         size_t chunk_size = 0)
{
  using InputT = std::decay_t<decltype(*std::begin(c))>;
  using ActualResultT = std::conditional_t<
    std::is_void_v<ResultT>,
    std::invoke_result_t<Op, const InputT&>,
    ResultT>;

  const size_t n = std::distance(std::begin(c), std::end(c));
  if (n == 0)
    return std::vector<ActualResultT>{};

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  // Ensure random access for parallel processing
  auto data_holder = parallel_detail::ensure_random_access(c);
  const auto& data = parallel_detail::deref(data_holder);

  std::vector<ActualResultT> result(n);
  std::vector<std::future<void>> futures;

  size_t offset = 0;

  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&result, &data, op, offset, chunk_end]() {
          auto in_it = std::begin(data);
          std::advance(in_it, offset);
          for (size_t i = offset; i < chunk_end; ++i, ++in_it)
            result[i] = op(*in_it);
        }));

      offset = chunk_end;
    }

  for (auto& f : futures)
    f.get();

  return result;
}

// =============================================================================
// Parallel Filter
// =============================================================================

/** @brief Parallel filter operation.

    Filters elements that satisfy a predicate, processing in parallel.
    The relative order of elements is preserved.

    @tparam Container Input container type.
    @tparam Pred Predicate type.

    @param pool Thread pool to use.
    @param c Source container.
    @param pred Predicate: `bool pred(const T&)`.
    @param chunk_size Elements per chunk (0 = auto).

    @return std::vector with elements satisfying the predicate.

    ## Example

    @code
    std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto evens = pfilter(pool, nums, [](int x) { return x % 2 == 0; });
    // evens = {2, 4, 6, 8, 10}
    @endcode

    @note Order is preserved by processing chunks sequentially at the merge stage.

    @ingroup Algos
*/
template <typename Container, typename Pred>
[[nodiscard]] auto pfilter(ThreadPool& pool, const Container& c, Pred pred,
                           size_t chunk_size = 0)
{
  using T = std::decay_t<decltype(*std::begin(c))>;
  
  const size_t n = std::distance(std::begin(c), std::end(c));
  if (n == 0)
    return std::vector<T>{};

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  auto data_holder = parallel_detail::ensure_random_access(c);
  const auto& data = parallel_detail::deref(data_holder);

  // Each chunk produces its own filtered result
  std::vector<std::future<std::vector<T>>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&data, pred, offset, chunk_end]() {
          std::vector<T> chunk_result;
          auto it = std::begin(data);
          std::advance(it, offset);
          for (size_t i = offset; i < chunk_end; ++i, ++it)
            if (pred(*it))
              chunk_result.push_back(*it);
          return chunk_result;
        }));

      offset = chunk_end;
    }

  // Merge results in order
  std::vector<T> result;
  for (auto& f : futures)
    {
      auto chunk_result = f.get();
      result.insert(result.end(), 
                    std::make_move_iterator(chunk_result.begin()),
                    std::make_move_iterator(chunk_result.end()));
    }

  return result;
}

// =============================================================================
// Parallel Fold/Reduce
// =============================================================================

/** @brief Parallel left fold (reduce).

    Computes a reduction over the container using multiple threads.
    The binary operation must be associative for correct results.

    @tparam T Accumulator type.
    @tparam Container Container type.
    @tparam BinaryOp Binary operation type.

    @param pool Thread pool to use.
    @param c Source container.
    @param init Initial accumulator value.
    @param op Binary operation: `T op(T acc, const ElemT&)`.
    @param chunk_size Elements per chunk (0 = auto).

    @return Reduced value.

    @warning The operation must be associative: op(op(a,b),c) == op(a,op(b,c)).
             Non-associative operations will produce incorrect results.

    ## Example

    @code
    std::vector<int> nums = {1, 2, 3, 4, 5};
    
    // Sum
    int sum = pfoldl(pool, nums, 0, std::plus<int>());
    // sum = 15

    // String concatenation (associative)
    std::vector<std::string> words = {"Hello", " ", "World"};
    auto msg = pfoldl(pool, words, std::string{}, std::plus<std::string>());
    // msg = "Hello World"
    @endcode

    @ingroup Algos
*/
template <typename T, typename Container, typename BinaryOp>
[[nodiscard]] T pfoldl(ThreadPool& pool, const Container& c, T init, BinaryOp op,
                       size_t chunk_size = 0)
{
  const size_t n = std::distance(std::begin(c), std::end(c));
  if (n == 0)
    return init;

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  auto data_holder = parallel_detail::ensure_random_access(c);
  const auto& data = parallel_detail::deref(data_holder);

  std::vector<std::future<T>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&data, &init, op, offset, chunk_end]() {
          auto it = std::begin(data);
          std::advance(it, offset);
          T local = *it++;
          for (size_t i = offset + 1; i < chunk_end; ++i, ++it)
            local = op(local, *it);
          return local;
        }));

      offset = chunk_end;
    }

  // Combine partial results
  T result = init;
  for (auto& f : futures)
    result = op(result, f.get());

  return result;
}

// =============================================================================
// Parallel For Each
// =============================================================================

/** @brief Parallel for_each operation.

    Applies a function to each element in parallel. Use this for side
    effects; for transformations, prefer `pmaps`.

    @tparam Container Container type.
    @tparam Op Operation type.

    @param pool Thread pool to use.
    @param c Container (elements accessed by non-const reference).
    @param op Function to apply: `void op(T&)` or `void op(const T&)`.
    @param chunk_size Elements per chunk (0 = auto).

    ## Example

    @code
    std::vector<int> nums(1000000);
    
    // Initialize in parallel
    pfor_each(pool, nums, [](int& x) { x = 42; });
    
    // With index (using capturing)
    std::atomic<int> count{0};
    pfor_each(pool, nums, [&count](const int& x) { 
      if (x > 0) ++count; 
    });
    @endcode

    @ingroup Algos
*/
template <typename Container, typename Op>
void pfor_each(ThreadPool& pool, Container& c, Op op, size_t chunk_size = 0)
{
  const size_t n = std::distance(std::begin(c), std::end(c));
  if (n == 0)
    return;

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  std::vector<std::future<void>> futures;
  size_t offset = 0;

  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&c, op, offset, chunk_end]() {
          auto it = std::begin(c);
          std::advance(it, offset);
          for (size_t i = offset; i < chunk_end; ++i, ++it)
            op(*it);
        }));

      offset = chunk_end;
    }

  for (auto& f : futures)
    f.get();
}

/** @overload pfor_each for const containers. */
template <typename Container, typename Op>
void pfor_each(ThreadPool& pool, const Container& c, Op op, size_t chunk_size = 0)
{
  const size_t n = std::distance(std::begin(c), std::end(c));
  if (n == 0)
    return;

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  auto data_holder = parallel_detail::ensure_random_access(c);
  const auto& data = parallel_detail::deref(data_holder);

  std::vector<std::future<void>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&data, op, offset, chunk_end]() {
          auto it = std::begin(data);
          std::advance(it, offset);
          for (size_t i = offset; i < chunk_end; ++i, ++it)
            op(*it);
        }));

      offset = chunk_end;
    }

  for (auto& f : futures)
    f.get();
}

// =============================================================================
// Parallel Predicates (all, exists, none)
// =============================================================================

/** @brief Parallel all predicate (short-circuit).

    Tests if all elements satisfy a predicate. Stops early if any
    chunk finds a non-satisfying element.

    @param pool Thread pool to use.
    @param c Source container.
    @param pred Predicate: `bool pred(const T&)`.
    @param chunk_size Elements per chunk (0 = auto).

    @return true if all elements satisfy pred, false otherwise.

    ## Example

    @code
    std::vector<int> nums = {2, 4, 6, 8, 10};
    bool all_even = pall(pool, nums, [](int x) { return x % 2 == 0; });
    // all_even = true
    @endcode

    @ingroup Algos
*/
template <typename Container, typename Pred>
[[nodiscard]] bool pall(ThreadPool& pool, const Container& c, Pred pred,
                        size_t chunk_size = 0)
{
  const size_t n = std::distance(std::begin(c), std::end(c));
  if (n == 0)
    return true;

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  auto data_holder = parallel_detail::ensure_random_access(c);
  const auto& data = parallel_detail::deref(data_holder);

  std::atomic<bool> found_false{false};
  std::vector<std::future<void>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&data, pred, &found_false, offset, chunk_end]() {
          if (found_false.load(std::memory_order_relaxed))
            return;  // Short-circuit
          
          auto it = std::begin(data);
          std::advance(it, offset);
          for (size_t i = offset; i < chunk_end; ++i, ++it)
            {
              if (!pred(*it))
                {
                  found_false.store(true, std::memory_order_relaxed);
                  return;
                }
              if (found_false.load(std::memory_order_relaxed))
                return;
            }
        }));

      offset = chunk_end;
    }

  for (auto& f : futures)
    f.get();

  return !found_false.load();
}

/** @brief Parallel exists predicate (short-circuit).

    Tests if any element satisfies a predicate. Stops early when found.

    @param pool Thread pool to use.
    @param c Source container.
    @param pred Predicate: `bool pred(const T&)`.
    @param chunk_size Elements per chunk (0 = auto).

    @return true if any element satisfies pred, false otherwise.

    ## Example

    @code
    std::vector<int> nums = {1, 2, 3, 4, 5};
    bool has_three = pexists(pool, nums, [](int x) { return x == 3; });
    // has_three = true
    @endcode

    @ingroup Algos
*/
template <typename Container, typename Pred>
[[nodiscard]] bool pexists(ThreadPool& pool, const Container& c, Pred pred,
                           size_t chunk_size = 0)
{
  const size_t n = std::distance(std::begin(c), std::end(c));
  if (n == 0)
    return false;

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  auto data_holder = parallel_detail::ensure_random_access(c);
  const auto& data = parallel_detail::deref(data_holder);

  std::atomic<bool> found{false};
  std::vector<std::future<void>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&data, pred, &found, offset, chunk_end]() {
          if (found.load(std::memory_order_relaxed))
            return;  // Short-circuit
          
          auto it = std::begin(data);
          std::advance(it, offset);
          for (size_t i = offset; i < chunk_end; ++i, ++it)
            {
              if (pred(*it))
                {
                  found.store(true, std::memory_order_relaxed);
                  return;
                }
              if (found.load(std::memory_order_relaxed))
                return;
            }
        }));

      offset = chunk_end;
    }

  for (auto& f : futures)
    f.get();

  return found.load();
}

/** @brief Parallel none predicate.

    Tests if no element satisfies a predicate.

    @param pool Thread pool to use.
    @param c Source container.
    @param pred Predicate: `bool pred(const T&)`.
    @param chunk_size Elements per chunk (0 = auto).

    @return true if no element satisfies pred, false otherwise.

    @ingroup Algos
*/
template <typename Container, typename Pred>
[[nodiscard]] bool pnone(ThreadPool& pool, const Container& c, Pred pred,
                         size_t chunk_size = 0)
{
  return !pexists(pool, c, pred, chunk_size);
}

// =============================================================================
// Parallel Count
// =============================================================================

/** @brief Parallel count_if operation.

    Counts elements satisfying a predicate using multiple threads.

    @param pool Thread pool to use.
    @param c Source container.
    @param pred Predicate: `bool pred(const T&)`.
    @param chunk_size Elements per chunk (0 = auto).

    @return Number of elements satisfying pred.

    ## Example

    @code
    std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    size_t evens = pcount_if(pool, nums, [](int x) { return x % 2 == 0; });
    // evens = 5
    @endcode

    @ingroup Algos
*/
template <typename Container, typename Pred>
[[nodiscard]] size_t pcount_if(ThreadPool& pool, const Container& c, Pred pred,
                               size_t chunk_size = 0)
{
  const size_t n = std::distance(std::begin(c), std::end(c));
  if (n == 0)
    return 0;

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  auto data_holder = parallel_detail::ensure_random_access(c);
  const auto& data = parallel_detail::deref(data_holder);

  std::vector<std::future<size_t>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&data, pred, offset, chunk_end]() {
          size_t count = 0;
          auto it = std::begin(data);
          std::advance(it, offset);
          for (size_t i = offset; i < chunk_end; ++i, ++it)
            if (pred(*it))
              ++count;
          return count;
        }));

      offset = chunk_end;
    }

  size_t total = 0;
  for (auto& f : futures)
    total += f.get();

  return total;
}

// =============================================================================
// Parallel Find
// =============================================================================

/** @brief Parallel find operation (returns index).

    Searches for an element satisfying a predicate in parallel.
    Returns the index of the first match (by position in container).

    @param pool Thread pool to use.
    @param c Source container.
    @param pred Predicate: `bool pred(const T&)`.
    @param chunk_size Elements per chunk (0 = auto).

    @return Index of first element satisfying pred, or std::nullopt if not found.

    @note "First" refers to the element with the smallest index, not
          necessarily the first one found by any thread.

    ## Example

    @code
    std::vector<int> nums = {1, 2, 3, 4, 5};
    auto idx = pfind(pool, nums, [](int x) { return x > 3; });
    // idx = 3 (value 4)
    @endcode

    @ingroup Algos
*/
template <typename Container, typename Pred>
[[nodiscard]] std::optional<size_t> pfind(ThreadPool& pool, const Container& c, 
                                          Pred pred, size_t chunk_size = 0)
{
  const size_t n = std::distance(std::begin(c), std::end(c));
  if (n == 0)
    return std::nullopt;

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  auto data_holder = parallel_detail::ensure_random_access(c);
  const auto& data = parallel_detail::deref(data_holder);

  // Track minimum found index
  std::atomic<size_t> min_index{n};  // n means not found
  std::vector<std::future<void>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&data, pred, &min_index, offset, chunk_end]() {
          // Skip if we already found something earlier
          if (min_index.load(std::memory_order_relaxed) <= offset)
            return;
          
          auto it = std::begin(data);
          std::advance(it, offset);
          for (size_t i = offset; i < chunk_end; ++i, ++it)
            {
              // Stop if earlier match found
              if (min_index.load(std::memory_order_relaxed) <= i)
                return;
              
              if (pred(*it))
                {
                  // Atomically update minimum
                  size_t expected = min_index.load(std::memory_order_relaxed);
                  while (i < expected and
                         not min_index.compare_exchange_weak(expected, i,
                           std::memory_order_relaxed))
                    ;
                  return;
                }
            }
        }));

      offset = chunk_end;
    }

  for (auto& f : futures)
    f.get();

  size_t result = min_index.load();
  if (result < n)
    return result;
  return std::nullopt;
}

/** @brief Parallel find with value return.

    Like pfind, but returns an optional containing the found value.

    @param pool Thread pool to use.
    @param c Source container.
    @param pred Predicate: `bool pred(const T&)`.
    @param chunk_size Elements per chunk (0 = auto).

    @return Optional with first matching element, or nullopt if not found.

    ## Example

    @code
    std::vector<std::string> words = {"apple", "banana", "cherry"};
    auto found = pfind_value(pool, words, [](const auto& s) { 
      return s.length() > 5; 
    });
    // found = "banana"
    @endcode

    @ingroup Algos
*/
template <typename Container, typename Pred>
[[nodiscard]] auto pfind_value(ThreadPool& pool, const Container& c, 
                               Pred pred, size_t chunk_size = 0)
{
  using T = std::decay_t<decltype(*std::begin(c))>;
  
  auto idx = pfind(pool, c, pred, chunk_size);
  if (!idx)
    return std::optional<T>{std::nullopt};
  
  auto it = std::begin(c);
  std::advance(it, *idx);
  return std::optional<T>{*it};
}

// =============================================================================
// Parallel Numeric Operations
// =============================================================================

/** @brief Parallel sum of elements.

    @param pool Thread pool to use.
    @param c Source container.
    @param init Initial value (default 0).
    @param chunk_size Elements per chunk (0 = auto).

    @return Sum of all elements plus init.

    ## Example

    @code
    std::vector<int> nums = {1, 2, 3, 4, 5};
    auto total = psum(pool, nums);
    // total = 15
    @endcode

    @ingroup Algos
*/
template <typename Container, 
          typename T = std::decay_t<decltype(*std::begin(std::declval<Container>()))>>
[[nodiscard]] T psum(ThreadPool& pool, const Container& c, T init = T{},
                     size_t chunk_size = 0)
{
  return pfoldl(pool, c, init, std::plus<T>{}, chunk_size);
}

/** @brief Parallel product of elements.

    @param pool Thread pool to use.
    @param c Source container.
    @param init Initial value (default 1).
    @param chunk_size Elements per chunk (0 = auto).

    @return Product of all elements times init.

    @ingroup Algos
*/
template <typename Container,
          typename T = std::decay_t<decltype(*std::begin(std::declval<Container>()))>>
[[nodiscard]] T pproduct(ThreadPool& pool, const Container& c, T init = T{1},
                         size_t chunk_size = 0)
{
  return pfoldl(pool, c, init, std::multiplies<T>{}, chunk_size);
}

/** @brief Parallel minimum element.

    @param pool Thread pool to use.
    @param c Source container.
    @param chunk_size Elements per chunk (0 = auto).

    @return Optional with minimum element, or nullopt if empty.

    @ingroup Algos
*/
template <typename Container>
[[nodiscard]] auto pmin(ThreadPool& pool, const Container& c, size_t chunk_size = 0)
{
  using T = std::decay_t<decltype(*std::begin(c))>;
  
  const size_t n = std::distance(std::begin(c), std::end(c));
  if (n == 0)
    return std::optional<T>{std::nullopt};

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  auto data_holder = parallel_detail::ensure_random_access(c);
  const auto& data = parallel_detail::deref(data_holder);

  std::vector<std::future<T>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&data, offset, chunk_end]() {
          auto it = std::begin(data);
          std::advance(it, offset);
          T local_min = *it++;
          for (size_t i = offset + 1; i < chunk_end; ++i, ++it)
            if (*it < local_min)
              local_min = *it;
          return local_min;
        }));

      offset = chunk_end;
    }

  T result = futures[0].get();
  for (size_t i = 1; i < futures.size(); ++i)
    {
      T val = futures[i].get();
      if (val < result)
        result = val;
    }

  return std::optional<T>{result};
}

/** @brief Parallel maximum element.

    @param pool Thread pool to use.
    @param c Source container.
    @param chunk_size Elements per chunk (0 = auto).

    @return Optional with maximum element, or nullopt if empty.

    @ingroup Algos
*/
template <typename Container>
[[nodiscard]] auto pmax(ThreadPool& pool, const Container& c, size_t chunk_size = 0)
{
  using T = std::decay_t<decltype(*std::begin(c))>;
  
  const size_t n = std::distance(std::begin(c), std::end(c));
  if (n == 0)
    return std::optional<T>{std::nullopt};

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  auto data_holder = parallel_detail::ensure_random_access(c);
  const auto& data = parallel_detail::deref(data_holder);

  std::vector<std::future<T>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&data, offset, chunk_end]() {
          auto it = std::begin(data);
          std::advance(it, offset);
          T local_max = *it++;
          for (size_t i = offset + 1; i < chunk_end; ++i, ++it)
            if (*it > local_max)
              local_max = *it;
          return local_max;
        }));

      offset = chunk_end;
    }

  T result = futures[0].get();
  for (size_t i = 1; i < futures.size(); ++i)
    {
      T val = futures[i].get();
      if (val > result)
        result = val;
    }

  return std::optional<T>{result};
}

/** @brief Parallel min and max elements.

    @param pool Thread pool to use.
    @param c Source container.
    @param chunk_size Elements per chunk (0 = auto).

    @return Optional pair (min, max), or nullopt if empty.

    @ingroup Algos
*/
template <typename Container>
[[nodiscard]] auto pminmax(ThreadPool& pool, const Container& c, size_t chunk_size = 0)
{
  using T = std::decay_t<decltype(*std::begin(c))>;
  
  const size_t n = std::distance(std::begin(c), std::end(c));
  if (n == 0)
    return std::optional<std::pair<T, T>>{std::nullopt};

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  auto data_holder = parallel_detail::ensure_random_access(c);
  const auto& data = parallel_detail::deref(data_holder);

  std::vector<std::future<std::pair<T, T>>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&data, offset, chunk_end]() {
          auto it = std::begin(data);
          std::advance(it, offset);
          T local_min = *it;
          T local_max = *it++;
          for (size_t i = offset + 1; i < chunk_end; ++i, ++it)
            {
              if (*it < local_min) local_min = *it;
              if (*it > local_max) local_max = *it;
            }
          return std::make_pair(local_min, local_max);
        }));

      offset = chunk_end;
    }

  auto result = futures[0].get();
  for (size_t i = 1; i < futures.size(); ++i)
    {
      auto [mi, ma] = futures[i].get();
      if (mi < result.first) result.first = mi;
      if (ma > result.second) result.second = ma;
    }

  return std::optional<std::pair<T, T>>{result};
}

// =============================================================================
// Parallel Sort
// =============================================================================

/** @brief Parallel sort (in-place).

    Sorts a container using multiple threads for the comparison work.
    Uses a parallel merge sort approach.

    @tparam Container Container with random access iterators.
    @tparam Compare Comparison function type.

    @param pool Thread pool to use.
    @param c Container to sort.
    @param cmp Comparison function (default: std::less<>).
    @param min_parallel_size Minimum size for parallel recursion.

    ## Example

    @code
    std::vector<int> nums = {5, 2, 8, 1, 9, 3};
    psort(pool, nums);
    // nums = {1, 2, 3, 5, 8, 9}

    // Custom comparison
    psort(pool, nums, std::greater<int>());
    // nums = {9, 8, 5, 3, 2, 1}
    @endcode

    @ingroup Algos
*/
template <typename Container, typename Compare = std::less<>>
void psort(ThreadPool& pool, Container& c, Compare cmp = Compare{},
           size_t min_parallel_size = 1024)
{
  const size_t n = std::distance(std::begin(c), std::end(c));
  if (n <= 1)
    return;

  // For small sizes, use regular sort
  if (n <= min_parallel_size or pool.num_threads() <= 1)
    {
      std::sort(std::begin(c), std::end(c), cmp);
      return;
    }

  // Split into chunks, sort each in parallel, then merge
  const size_t num_chunks = std::min(pool.num_threads() * 2, n / min_parallel_size);
  const size_t chunk_size = (n + num_chunks - 1) / num_chunks;

  // Sort chunks in parallel
  std::vector<std::future<void>> futures;
  for (size_t i = 0; i < n; i += chunk_size)
    {
      size_t end = std::min(i + chunk_size, n);
      auto begin_it = std::begin(c);
      std::advance(begin_it, i);
      auto end_it = std::begin(c);
      std::advance(end_it, end);
      
      futures.push_back(pool.enqueue([begin_it, end_it, cmp]() {
        std::sort(begin_it, end_it, cmp);
      }));
    }

  for (auto& f : futures)
    f.get();

  // Merge sorted chunks
  using T = std::decay_t<decltype(*std::begin(c))>;
  std::vector<T> buffer(n);

  for (size_t width = chunk_size; width < n; width *= 2)
    {
      std::vector<std::future<void>> merge_futures;
      
      for (size_t i = 0; i < n; i += 2 * width)
        {
          size_t mid = std::min(i + width, n);
          size_t end = std::min(i + 2 * width, n);
          
          if (mid < end)
            {
              auto begin_it = std::begin(c);
              std::advance(begin_it, i);
              auto mid_it = std::begin(c);
              std::advance(mid_it, mid);
              auto end_it = std::begin(c);
              std::advance(end_it, end);
              
              merge_futures.push_back(pool.enqueue(
                [begin_it, mid_it, end_it, &buffer, i, cmp]() {
                  std::merge(begin_it, mid_it, mid_it, end_it, 
                             buffer.begin() + i, cmp);
                }));
            }
          else
            {
              // Copy remaining elements
              auto begin_it = std::begin(c);
              std::advance(begin_it, i);
              auto end_it = std::begin(c);
              std::advance(end_it, mid);
              std::copy(begin_it, end_it, buffer.begin() + i);
            }
        }
      
      for (auto& f : merge_futures)
        f.get();
      
      // Swap buffer back to container
      auto it = std::begin(c);
      for (size_t i = 0; i < n; ++i, ++it)
        *it = std::move(buffer[i]);
    }
}

// =============================================================================
// Parallel Zip Operations
// =============================================================================

/** @brief Parallel zip + for_each.

    Applies a function to corresponding elements of two containers in parallel.

    @param pool Thread pool to use.
    @param c1 First container.
    @param c2 Second container.
    @param op Function: `void op(const T1&, const T2&)`.
    @param chunk_size Elements per chunk (0 = auto).

    @note Stops at the shorter container.

    ## Example

    @code
    std::vector<int> a = {1, 2, 3};
    std::vector<int> b = {4, 5, 6};
    std::atomic<int> sum{0};
    
    pzip_for_each(pool, a, b, [&sum](int x, int y) {
      sum += x * y;
    });
    // sum = 1*4 + 2*5 + 3*6 = 32
    @endcode

    @ingroup Algos
*/
template <typename Container1, typename Container2, typename Op>
void pzip_for_each(ThreadPool& pool, const Container1& c1, const Container2& c2,
                   Op op, size_t chunk_size = 0)
{
  const size_t n1 = std::distance(std::begin(c1), std::end(c1));
  const size_t n2 = std::distance(std::begin(c2), std::end(c2));
  const size_t n = std::min(n1, n2);
  
  if (n == 0)
    return;

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  auto h1 = parallel_detail::ensure_random_access(c1);
  auto h2 = parallel_detail::ensure_random_access(c2);
  const auto& d1 = parallel_detail::deref(h1);
  const auto& d2 = parallel_detail::deref(h2);

  std::vector<std::future<void>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&d1, &d2, op, offset, chunk_end]() {
          auto it1 = std::begin(d1);
          auto it2 = std::begin(d2);
          std::advance(it1, offset);
          std::advance(it2, offset);
          for (size_t i = offset; i < chunk_end; ++i, ++it1, ++it2)
            op(*it1, *it2);
        }));

      offset = chunk_end;
    }

  for (auto& f : futures)
    f.get();
}

/** @brief Parallel zip + map.

    Maps corresponding elements of two containers using a binary function.

    @param pool Thread pool to use.
    @param c1 First container.
    @param c2 Second container.
    @param op Function: `R op(const T1&, const T2&)`.
    @param chunk_size Elements per chunk (0 = auto).

    @return Vector of results.

    ## Example

    @code
    std::vector<int> a = {1, 2, 3};
    std::vector<int> b = {4, 5, 6};
    
    auto products = pzip_maps(pool, a, b, [](int x, int y) { return x * y; });
    // products = {4, 10, 18}
    @endcode

    @ingroup Algos
*/
template <typename Container1, typename Container2, typename Op>
[[nodiscard]] auto pzip_maps(ThreadPool& pool, const Container1& c1, 
                             const Container2& c2, Op op, size_t chunk_size = 0)
{
  using T1 = std::decay_t<decltype(*std::begin(c1))>;
  using T2 = std::decay_t<decltype(*std::begin(c2))>;
  using ResultT = std::invoke_result_t<Op, const T1&, const T2&>;

  const size_t n1 = std::distance(std::begin(c1), std::end(c1));
  const size_t n2 = std::distance(std::begin(c2), std::end(c2));
  const size_t n = std::min(n1, n2);
  
  if (n == 0)
    return std::vector<ResultT>{};

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  auto h1 = parallel_detail::ensure_random_access(c1);
  auto h2 = parallel_detail::ensure_random_access(c2);
  const auto& d1 = parallel_detail::deref(h1);
  const auto& d2 = parallel_detail::deref(h2);

  std::vector<ResultT> result(n);
  std::vector<std::future<void>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&result, &d1, &d2, op, offset, chunk_end]() {
          auto it1 = std::begin(d1);
          auto it2 = std::begin(d2);
          std::advance(it1, offset);
          std::advance(it2, offset);
          for (size_t i = offset; i < chunk_end; ++i, ++it1, ++it2)
            result[i] = op(*it1, *it2);
        }));

      offset = chunk_end;
    }

  for (auto& f : futures)
    f.get();

  return result;
}

/** @brief Parallel zip + fold.

    Reduces corresponding elements of two containers using a ternary function.

    @param pool Thread pool to use.
    @param c1 First container.
    @param c2 Second container.
    @param init Initial value.
    @param op Ternary function: `R op(R acc, const T1&, const T2&)`.
    @param chunk_size Elements per chunk (0 = auto).

    @return Reduced value.

    @warning The operation must be associative for correct parallel results.

    ## Example

    @code
    std::vector<int> a = {1, 2, 3};
    std::vector<int> b = {4, 5, 6};
    
    int dot_product = pzip_foldl(pool, a, b, 0,
      [](int acc, int x, int y) { return acc + x * y; });
    // dot_product = 32
    @endcode

    @ingroup Algos
*/
template <typename Container1, typename Container2, typename T, typename Op>
[[nodiscard]] T pzip_foldl(ThreadPool& pool, const Container1& c1, 
                           const Container2& c2, T init, Op op,
                           size_t chunk_size = 0)
{
  const size_t n1 = std::distance(std::begin(c1), std::end(c1));
  const size_t n2 = std::distance(std::begin(c2), std::end(c2));
  const size_t n = std::min(n1, n2);
  
  if (n == 0)
    return init;

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  auto h1 = parallel_detail::ensure_random_access(c1);
  auto h2 = parallel_detail::ensure_random_access(c2);
  const auto& d1 = parallel_detail::deref(h1);
  const auto& d2 = parallel_detail::deref(h2);

  std::vector<std::future<T>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&d1, &d2, &init, op, offset, chunk_end]() {
          auto it1 = std::begin(d1);
          auto it2 = std::begin(d2);
          std::advance(it1, offset);
          std::advance(it2, offset);
          
          T local = op(init, *it1++, *it2++);
          for (size_t i = offset + 1; i < chunk_end; ++i, ++it1, ++it2)
            local = op(local, *it1, *it2);
          return local;
        }));

      offset = chunk_end;
    }

  // Binary reduce the partial results
  // We need a binary op for this - derive it from the ternary op
  T result = futures[0].get();
  for (size_t i = 1; i < futures.size(); ++i)
    {
      T val = futures[i].get();
      // Combine using addition - user should use pfoldl + pzip_maps for complex cases
      result = result + val - init;  // Compensate for init being added in each chunk
    }

  return result;
}

// =============================================================================
// Parallel Partition
// =============================================================================

/** @brief Parallel partition (stable).

    Partitions elements based on a predicate, preserving relative order.

    @param pool Thread pool to use.
    @param c Source container.
    @param pred Predicate: `bool pred(const T&)`.
    @param chunk_size Elements per chunk (0 = auto).

    @return Pair of vectors: (elements satisfying pred, elements not satisfying).

    ## Example

    @code
    std::vector<int> nums = {1, 2, 3, 4, 5, 6};
    auto [evens, odds] = ppartition(pool, nums, [](int x) { return x % 2 == 0; });
    // evens = {2, 4, 6}, odds = {1, 3, 5}
    @endcode

    @ingroup Algos
*/
template <typename Container, typename Pred>
[[nodiscard]] auto ppartition(ThreadPool& pool, const Container& c, Pred pred,
                              size_t chunk_size = 0)
{
  using T = std::decay_t<decltype(*std::begin(c))>;
  
  const size_t n = std::distance(std::begin(c), std::end(c));
  if (n == 0)
    return std::make_pair(std::vector<T>{}, std::vector<T>{});

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  auto data_holder = parallel_detail::ensure_random_access(c);
  const auto& data = parallel_detail::deref(data_holder);

  using ChunkResult = std::pair<std::vector<T>, std::vector<T>>;
  std::vector<std::future<ChunkResult>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&data, pred, offset, chunk_end]() {
          std::vector<T> yes, no;
          auto it = std::begin(data);
          std::advance(it, offset);
          for (size_t i = offset; i < chunk_end; ++i, ++it)
            {
              if (pred(*it))
                yes.push_back(*it);
              else
                no.push_back(*it);
            }
          return std::make_pair(std::move(yes), std::move(no));
        }));

      offset = chunk_end;
    }

  // Merge results in order
  std::vector<T> yes_result, no_result;
  for (auto& f : futures)
    {
      auto [yes, no] = f.get();
      yes_result.insert(yes_result.end(), 
                        std::make_move_iterator(yes.begin()),
                        std::make_move_iterator(yes.end()));
      no_result.insert(no_result.end(),
                       std::make_move_iterator(no.begin()),
                       std::make_move_iterator(no.end()));
    }

  return std::make_pair(std::move(yes_result), std::move(no_result));
}

// =============================================================================
// Variadic Parallel Zip Operations (N containers)
// =============================================================================

namespace parallel_zip_detail
{
  /// Holder for converted containers (either pointer or unique_ptr to vector).
  /// 
  /// For containers with random access, stores a pointer to the original.
  /// For containers without random access (like DynList), copies to a vector.
  /// 
  /// IMPORTANT: After construction, size() is always O(1) because:
  /// - Random access containers: std::distance is O(1)
  /// - Non-random access: already copied to vector, vector.size() is O(1)
  template <typename Container>
  struct ContainerHolder
  {
    using value_type = std::decay_t<decltype(*std::begin(std::declval<Container&>()))>;
    using holder_type = std::conditional_t<
      parallel_detail::has_random_access<Container>(),
      const Container*,
      std::unique_ptr<std::vector<value_type>>>;
    
    holder_type data;
    size_t cached_size;  ///< Cached size for O(1) access
    
    explicit ContainerHolder(const Container& c)
    {
      if constexpr (parallel_detail::has_random_access<Container>())
        {
          data = &c;
          // For random access, std::distance is O(1)
          cached_size = static_cast<size_t>(std::distance(std::begin(c), std::end(c)));
        }
      else
        {
          // Copy to vector (O(n) - unavoidable), then get size from vector (O(1))
          data = std::make_unique<std::vector<value_type>>(std::begin(c), std::end(c));
          cached_size = data->size();
        }
    }
    
    decltype(auto) get() const
    {
      if constexpr (parallel_detail::has_random_access<Container>())
        return *data;
      else
        return *data;
    }
    
    /// Size is always O(1) - either from random access or from cached vector size
    [[nodiscard]] size_t size() const noexcept { return cached_size; }
    
    auto begin() const { return std::begin(get()); }
    auto end() const { return std::end(get()); }
  };

  /// Get minimum size from tuple of holders - always O(1) per holder
  template <typename... Holders, size_t... Is>
  size_t min_holder_size_impl(const std::tuple<Holders...>& holders,
                               std::index_sequence<Is...>)
  {
    return std::min({std::get<Is>(holders).size()...});
  }

  template <typename... Holders>
  size_t min_holder_size(const std::tuple<Holders...>& holders)
  {
    return min_holder_size_impl(holders, std::make_index_sequence<sizeof...(Holders)>{});
  }

  /// Create tuple of iterators at given offset
  template <typename... Holders, size_t... Is>
  auto make_iterators_at(size_t offset, const std::tuple<Holders...>& holders,
                         std::index_sequence<Is...>)
  {
    return std::make_tuple([&]() {
      auto it = std::get<Is>(holders).begin();
      std::advance(it, offset);
      return it;
    }()...);
  }

  /// Advance all iterators in tuple
  template <typename... Iters, size_t... Is>
  void advance_all_iters(std::tuple<Iters...>& iters, std::index_sequence<Is...>)
  {
    (++std::get<Is>(iters), ...);
  }

  /// Dereference all iterators and make tuple
  template <typename... Iters, size_t... Is>
  auto deref_all_iters(const std::tuple<Iters...>& iters, std::index_sequence<Is...>)
  {
    return std::make_tuple(*std::get<Is>(iters)...);
  }

} // namespace parallel_zip_detail

/** @brief Parallel for_each over N zipped containers (variadic).

    Applies a function to corresponding elements from multiple containers
    in parallel. Stops at the shortest container.

    @tparam Op      Operation type: void(const T1&, const T2&, ..., const TN&)
    @tparam Containers Container types (variadic).

    @param pool Thread pool to use.
    @param op   Function to apply to each tuple of elements.
    @param cs   Containers to zip (2 or more).

    ## Example

    @code
    std::vector<int> a = {1, 2, 3};
    std::vector<int> b = {4, 5, 6};
    std::vector<int> c = {7, 8, 9};
    std::atomic<int> sum{0};

    pzip_for_each_n(pool, [&sum](int x, int y, int z) {
      sum += x + y + z;
    }, a, b, c);
    // sum = (1+4+7) + (2+5+8) + (3+6+9) = 45
    @endcode

    @note The operation receives individual elements, not a tuple.
          Use std::apply if you need tuple access.
    @note Use `pzip_for_each` for 2 containers, `pzip_for_each_n` for 3+.

    @ingroup Algos
*/
template <typename Op, typename... Containers>
void pzip_for_each_n(ThreadPool& pool, Op op, const Containers&... cs)
{
  static_assert(sizeof...(Containers) >= 2, 
                "pzip_for_each requires at least 2 containers");

  // Convert all containers to random access FIRST
  // This is O(n) for non-RA containers, but unavoidable
  auto holders = std::make_tuple(
    parallel_zip_detail::ContainerHolder<Containers>(cs)...);

  // Now get min size - O(1) because all holders have cached sizes
  const size_t n = parallel_zip_detail::min_holder_size(holders);
  if (n == 0)
    return;

  size_t chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  std::vector<std::future<void>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&holders, op, offset, chunk_end]() {
          constexpr size_t N = sizeof...(Containers);
          auto iters = parallel_zip_detail::make_iterators_at(
            offset, holders, std::make_index_sequence<N>{});
          
          for (size_t i = offset; i < chunk_end; ++i)
            {
              std::apply(op, parallel_zip_detail::deref_all_iters(
                iters, std::make_index_sequence<N>{}));
              parallel_zip_detail::advance_all_iters(
                iters, std::make_index_sequence<N>{});
            }
        }));

      offset = chunk_end;
    }

  for (auto& f : futures)
    f.get();
}

/** @brief Parallel map over N zipped containers (variadic).

    Maps corresponding elements from multiple containers using a function,
    processing in parallel.

    @tparam Op      Operation type: R(const T1&, const T2&, ..., const TN&)
    @tparam Containers Container types (variadic).

    @param pool Thread pool to use.
    @param op   Function to apply to each tuple of elements.
    @param cs   Containers to zip (2 or more).

    @return std::vector<R> with mapped results.

    ## Example

    @code
    std::vector<int> a = {1, 2, 3};
    std::vector<int> b = {4, 5, 6};
    std::vector<int> c = {7, 8, 9};

    auto sums = pzip_maps_n(pool, [](int x, int y, int z) {
      return x + y + z;
    }, a, b, c);
    // sums = {12, 15, 18}
    @endcode

    @note Use `pzip_maps` for 2 containers, `pzip_maps_n` for 3+.

    @ingroup Algos
*/
template <typename Op, typename... Containers>
[[nodiscard]] auto pzip_maps_n(ThreadPool& pool, Op op, const Containers&... cs)
{
  static_assert(sizeof...(Containers) >= 2, 
                "pzip_maps requires at least 2 containers");

  // Deduce result type from operation
  using ResultT = std::invoke_result_t<Op, 
    std::decay_t<decltype(*std::begin(cs))>...>;

  // Convert all containers to random access FIRST
  auto holders = std::make_tuple(
    parallel_zip_detail::ContainerHolder<Containers>(cs)...);

  // Now get min size - O(1) because all holders have cached sizes
  const size_t n = parallel_zip_detail::min_holder_size(holders);
  if (n == 0)
    return std::vector<ResultT>{};

  size_t chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  std::vector<ResultT> result(n);
  std::vector<std::future<void>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&result, &holders, op, offset, chunk_end]() {
          constexpr size_t N = sizeof...(Containers);
          auto iters = parallel_zip_detail::make_iterators_at(
            offset, holders, std::make_index_sequence<N>{});
          
          for (size_t i = offset; i < chunk_end; ++i)
            {
              result[i] = std::apply(op, parallel_zip_detail::deref_all_iters(
                iters, std::make_index_sequence<N>{}));
              parallel_zip_detail::advance_all_iters(
                iters, std::make_index_sequence<N>{});
            }
        }));

      offset = chunk_end;
    }

  for (auto& f : futures)
    f.get();

  return result;
}

/** @brief Parallel fold/reduce over N zipped containers (variadic).

    Reduces corresponding elements from multiple containers using a function,
    processing in parallel. Requires a binary combiner for merging partial results.

    @tparam T       Accumulator type.
    @tparam Op      Operation type: T(T acc, const T1&, ..., const TN&)
    @tparam Combiner Binary combiner: T(T, T)
    @tparam Containers Container types (variadic).

    @param pool Thread pool to use.
    @param init Initial accumulator value.
    @param op   Function: T op(T acc, const T1&, ..., const TN&).
    @param combiner Binary function to combine partial results: T(T, T).
    @param cs   Containers to zip (2 or more).

    @return Reduced value.

    @warning Both op and combiner must be associative for correct parallel results.

    ## Example

    @code
    std::vector<int> a = {1, 2, 3};
    std::vector<int> b = {4, 5, 6};

    // Dot product: accumulate with x*y, combine partial sums with +
    int dot = pzip_foldl_n(pool, 0, 
      [](int acc, int x, int y) { return acc + x * y; },
      std::plus<int>(),
      a, b);
    // dot = 1*4 + 2*5 + 3*6 = 32
    @endcode

    @note Use `pzip_foldl` for 2 containers, `pzip_foldl_n` for 3+.

    @ingroup Algos
*/
template <typename T, typename Op, typename Combiner, typename... Containers>
[[nodiscard]] T pzip_foldl_n(ThreadPool& pool, T init, Op op, Combiner combiner,
                             const Containers&... cs)
{
  static_assert(sizeof...(Containers) >= 2, 
                "pzip_foldl requires at least 2 containers");

  // Convert all containers to random access FIRST
  auto holders = std::make_tuple(
    parallel_zip_detail::ContainerHolder<Containers>(cs)...);

  // Now get min size - O(1) because all holders have cached sizes
  const size_t n = parallel_zip_detail::min_holder_size(holders);
  if (n == 0)
    return init;

  size_t chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  std::vector<std::future<T>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&holders, init, op, offset, chunk_end]() {
          constexpr size_t N = sizeof...(Containers);
          auto iters = parallel_zip_detail::make_iterators_at(
            offset, holders, std::make_index_sequence<N>{});
          
          // First element
          auto first_tuple = parallel_zip_detail::deref_all_iters(
            iters, std::make_index_sequence<N>{});
          T local = std::apply([&op, &init](auto&&... args) {
            return op(init, std::forward<decltype(args)>(args)...);
          }, first_tuple);
          parallel_zip_detail::advance_all_iters(
            iters, std::make_index_sequence<N>{});
          
          // Remaining elements
          for (size_t i = offset + 1; i < chunk_end; ++i)
            {
              auto tuple = parallel_zip_detail::deref_all_iters(
                iters, std::make_index_sequence<N>{});
              local = std::apply([&op, &local](auto&&... args) {
                return op(local, std::forward<decltype(args)>(args)...);
              }, tuple);
              parallel_zip_detail::advance_all_iters(
                iters, std::make_index_sequence<N>{});
            }
          
          return local;
        }));

      offset = chunk_end;
    }

  // Combine partial results using the combiner
  T result = futures[0].get();
  for (size_t i = 1; i < futures.size(); ++i)
    result = combiner(result, futures[i].get());

  return result;
}

/** @brief Parallel all predicate over N zipped containers (variadic).

    Tests if a predicate holds for all corresponding tuples of elements.
    Short-circuits when a false result is found.

    @tparam Pred    Predicate type: bool(const T1&, ..., const TN&)
    @tparam Containers Container types (variadic).

    @param pool Thread pool to use.
    @param pred Predicate to test.
    @param cs   Containers to zip (2 or more).

    @return true if pred returns true for all tuples.

    ## Example

    @code
    std::vector<int> a = {1, 2, 3};
    std::vector<int> b = {2, 3, 4};

    bool all_less = pzip_all_n(pool, [](int x, int y) {
      return x < y;
    }, a, b);
    // all_less = true
    @endcode

    @ingroup Algos
*/
template <typename Pred, typename... Containers>
[[nodiscard]] bool pzip_all_n(ThreadPool& pool, Pred pred, const Containers&... cs)
{
  static_assert(sizeof...(Containers) >= 2, 
                "pzip_all requires at least 2 containers");

  // Convert all containers to random access FIRST
  auto holders = std::make_tuple(
    parallel_zip_detail::ContainerHolder<Containers>(cs)...);

  // Now get min size - O(1) because all holders have cached sizes
  const size_t n = parallel_zip_detail::min_holder_size(holders);
  if (n == 0)
    return true;  // Vacuous truth

  size_t chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  std::atomic<bool> found_false{false};
  std::vector<std::future<void>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&holders, pred, &found_false, offset, chunk_end]() {
          if (found_false.load(std::memory_order_relaxed))
            return;
          
          constexpr size_t N = sizeof...(Containers);
          auto iters = parallel_zip_detail::make_iterators_at(
            offset, holders, std::make_index_sequence<N>{});
          
          for (size_t i = offset; i < chunk_end; ++i)
            {
              auto tuple = parallel_zip_detail::deref_all_iters(
                iters, std::make_index_sequence<N>{});
              if (!std::apply(pred, tuple))
                {
                  found_false.store(true, std::memory_order_relaxed);
                  return;
                }
              if (found_false.load(std::memory_order_relaxed))
                return;
              parallel_zip_detail::advance_all_iters(
                iters, std::make_index_sequence<N>{});
            }
        }));

      offset = chunk_end;
    }

  for (auto& f : futures)
    f.get();

  return !found_false.load();
}

/** @brief Parallel exists predicate over N zipped containers (variadic).

    Tests if a predicate holds for any corresponding tuple of elements.
    Short-circuits when a true result is found.

    @tparam Pred    Predicate type: bool(const T1&, ..., const TN&)
    @tparam Containers Container types (variadic).

    @param pool Thread pool to use.
    @param pred Predicate to test.
    @param cs   Containers to zip (2 or more).

    @return true if pred returns true for any tuple.

    ## Example

    @code
    std::vector<int> a = {1, 2, 3};
    std::vector<int> b = {2, 2, 4};

    bool any_equal = pzip_exists_n(pool, [](int x, int y) {
      return x == y;
    }, a, b);
    // any_equal = true (2 == 2)
    @endcode

    @ingroup Algos
*/
template <typename Pred, typename... Containers>
[[nodiscard]] bool pzip_exists_n(ThreadPool& pool, Pred pred, const Containers&... cs)
{
  static_assert(sizeof...(Containers) >= 2, 
                "pzip_exists requires at least 2 containers");

  // Convert all containers to random access FIRST
  auto holders = std::make_tuple(
    parallel_zip_detail::ContainerHolder<Containers>(cs)...);

  // Now get min size - O(1) because all holders have cached sizes
  const size_t n = parallel_zip_detail::min_holder_size(holders);
  if (n == 0)
    return false;

  size_t chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  std::atomic<bool> found{false};
  std::vector<std::future<void>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&holders, pred, &found, offset, chunk_end]() {
          if (found.load(std::memory_order_relaxed))
            return;
          
          constexpr size_t N = sizeof...(Containers);
          auto iters = parallel_zip_detail::make_iterators_at(
            offset, holders, std::make_index_sequence<N>{});
          
          for (size_t i = offset; i < chunk_end; ++i)
            {
              auto tuple = parallel_zip_detail::deref_all_iters(
                iters, std::make_index_sequence<N>{});
              if (std::apply(pred, tuple))
                {
                  found.store(true, std::memory_order_relaxed);
                  return;
                }
              if (found.load(std::memory_order_relaxed))
                return;
              parallel_zip_detail::advance_all_iters(
                iters, std::make_index_sequence<N>{});
            }
        }));

      offset = chunk_end;
    }

  for (auto& f : futures)
    f.get();

  return found.load();
}

/** @brief Parallel count over N zipped containers (variadic).

    Counts tuples that satisfy a predicate.

    @tparam Pred    Predicate type: bool(const T1&, ..., const TN&)
    @tparam Containers Container types (variadic).

    @param pool Thread pool to use.
    @param pred Predicate to test.
    @param cs   Containers to zip (2 or more).

    @return Number of tuples satisfying pred.

    @ingroup Algos
*/
template <typename Pred, typename... Containers>
[[nodiscard]] size_t pzip_count_if_n(ThreadPool& pool, Pred pred, 
                                     const Containers&... cs)
{
  static_assert(sizeof...(Containers) >= 2, 
                "pzip_count_if requires at least 2 containers");

  // Convert all containers to random access FIRST
  auto holders = std::make_tuple(
    parallel_zip_detail::ContainerHolder<Containers>(cs)...);

  // Now get min size - O(1) because all holders have cached sizes
  const size_t n = parallel_zip_detail::min_holder_size(holders);
  if (n == 0)
    return 0;

  size_t chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  std::vector<std::future<size_t>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&holders, pred, offset, chunk_end]() {
          constexpr size_t N = sizeof...(Containers);
          auto iters = parallel_zip_detail::make_iterators_at(
            offset, holders, std::make_index_sequence<N>{});
          
          size_t count = 0;
          for (size_t i = offset; i < chunk_end; ++i)
            {
              auto tuple = parallel_zip_detail::deref_all_iters(
                iters, std::make_index_sequence<N>{});
              if (std::apply(pred, tuple))
                ++count;
              parallel_zip_detail::advance_all_iters(
                iters, std::make_index_sequence<N>{});
            }
          return count;
        }));

      offset = chunk_end;
    }

  size_t total = 0;
  for (auto& f : futures)
    total += f.get();

  return total;
}

// =============================================================================
// Parallel Enumerate
// =============================================================================

/** @brief Parallel for_each with index (enumerate).

    Applies a function to each element along with its index, in parallel.

    @tparam Container Container type.
    @tparam Op       Operation type: void(size_t index, const T&) or void(size_t, T&).

    @param pool Thread pool to use.
    @param c    Container.
    @param op   Function: void(size_t index, const T& element).
    @param chunk_size Elements per chunk (0 = auto).

    ## Example

    @code
    std::vector<int> data(1000);
    
    penumerate_for_each(pool, data, [](size_t i, int& x) {
      x = static_cast<int>(i * 2);
    });
    // data[i] = i * 2
    @endcode

    @ingroup Algos
*/
template <typename Container, typename Op>
void penumerate_for_each(ThreadPool& pool, Container& c, Op op,
                         size_t chunk_size = 0)
{
  const size_t n = std::distance(std::begin(c), std::end(c));
  if (n == 0)
    return;

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  std::vector<std::future<void>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&c, op, offset, chunk_end]() {
          auto it = std::begin(c);
          std::advance(it, offset);
          for (size_t i = offset; i < chunk_end; ++i, ++it)
            op(i, *it);
        }));

      offset = chunk_end;
    }

  for (auto& f : futures)
    f.get();
}

/** @overload penumerate_for_each for const containers. */
template <typename Container, typename Op>
void penumerate_for_each(ThreadPool& pool, const Container& c, Op op,
                         size_t chunk_size = 0)
{
  const size_t n = std::distance(std::begin(c), std::end(c));
  if (n == 0)
    return;

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  auto data_holder = parallel_detail::ensure_random_access(c);
  const auto& data = parallel_detail::deref(data_holder);

  std::vector<std::future<void>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&data, op, offset, chunk_end]() {
          auto it = std::begin(data);
          std::advance(it, offset);
          for (size_t i = offset; i < chunk_end; ++i, ++it)
            op(i, *it);
        }));

      offset = chunk_end;
    }

  for (auto& f : futures)
    f.get();
}

/** @brief Parallel enumerate with map.

    Maps each element along with its index to a result.

    @tparam Container Container type.
    @tparam Op       Operation type: R(size_t index, const T&).

    @param pool Thread pool to use.
    @param c    Container.
    @param op   Function: R(size_t index, const T& element).
    @param chunk_size Elements per chunk (0 = auto).

    @return std::vector<R> with results.

    ## Example

    @code
    std::vector<std::string> words = {"hello", "world"};
    
    auto indexed = penumerate_maps(pool, words, 
      [](size_t i, const std::string& s) {
        return std::to_string(i) + ": " + s;
      });
    // indexed = {"0: hello", "1: world"}
    @endcode

    @ingroup Algos
*/
template <typename Container, typename Op>
[[nodiscard]] auto penumerate_maps(ThreadPool& pool, const Container& c, Op op,
                                   size_t chunk_size = 0)
{
  using T = std::decay_t<decltype(*std::begin(c))>;
  using ResultT = std::invoke_result_t<Op, size_t, const T&>;

  const size_t n = std::distance(std::begin(c), std::end(c));
  if (n == 0)
    return std::vector<ResultT>{};

  if (chunk_size == 0)
    chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

  auto data_holder = parallel_detail::ensure_random_access(c);
  const auto& data = parallel_detail::deref(data_holder);

  std::vector<ResultT> result(n);
  std::vector<std::future<void>> futures;
  
  size_t offset = 0;
  while (offset < n)
    {
      size_t chunk_end = std::min(offset + chunk_size, n);
      
      futures.push_back(pool.enqueue(
        [&result, &data, op, offset, chunk_end]() {
          auto it = std::begin(data);
          std::advance(it, offset);
          for (size_t i = offset; i < chunk_end; ++i, ++it)
            result[i] = op(i, *it);
        }));

      offset = chunk_end;
    }

  for (auto& f : futures)
    f.get();

  return result;
}

// =============================================================================
// Convenience: Default Pool Variants
// =============================================================================

/** @brief Global default pool for parallel operations.

    Uses the default ThreadPool for operations when no pool is specified.
    
    @return Reference to the default pool.
*/
inline ThreadPool& parallel_default_pool()
{
  return default_pool();
}

// Convenience macros for using default pool (optional)
#ifdef AH_PARALLEL_USE_DEFAULT_POOL

#define PMAP(c, op) pmaps(parallel_default_pool(), c, op)
#define PFILTER(c, pred) pfilter(parallel_default_pool(), c, pred)
#define PFOLD(c, init, op) pfoldl(parallel_default_pool(), c, init, op)
#define PFOR_EACH(c, op) pfor_each(parallel_default_pool(), c, op)
#define PALL(c, pred) pall(parallel_default_pool(), c, pred)
#define PEXISTS(c, pred) pexists(parallel_default_pool(), c, pred)
#define PSUM(c) psum(parallel_default_pool(), c)

#endif // AH_PARALLEL_USE_DEFAULT_POOL

} // namespace Aleph

#endif // AH_PARALLEL_H
