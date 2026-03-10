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
    | `ppartition` | Parallel stable partition | O(n/p + p) |
    | `pscan` | Parallel inclusive scan | O(n/p + p) |
    | `pexclusive_scan` | Parallel exclusive scan | O(n/p + p) |
    | `pmerge` | Parallel merge of sorted containers | O(n/p + p) |

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

    @ingroup Algorithms
    @ingroup FunctionalProgramming
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
    struct chunk_bounds
    {
      size_t offset;
      size_t end;
    };

    inline ThreadPool &selected_parallel_pool(const ParallelOptions & options)
    {
      return options.pool == nullptr ? default_pool() : *options.pool;
    }

    inline size_t chunk_count(const size_t n, const size_t chunk_size) noexcept
    {
      return n == 0 ? 0 : (n + chunk_size - 1) / chunk_size;
    }

    inline chunk_bounds bounds_for_chunk(const size_t idx, const size_t n,
                                         const size_t chunk_size) noexcept
    {
      const size_t offset = idx * chunk_size;
      return {offset, std::min(offset + chunk_size, n)};
    }

    /// Calculate optimal chunk size based on data size and thread count
    inline size_t chunk_size(const size_t n, const size_t num_threads, const size_t min_chunk = 64)
    {
      if (n == 0) return 1;
      // Use more chunks than threads for better load balancing
      const size_t chunks = num_threads * 4;
      const size_t size = (n + chunks - 1) / chunks;
      return std::max(size, min_chunk);
    }

    /// Check if container supports random access
    template <typename Container>
    constexpr bool has_random_access()
    {
      using It = decltype(std::begin(std::declval<Container &>()));
      return std::is_base_of_v<std::random_access_iterator_tag,
                               typename std::iterator_traits<It>::iterator_category>;
    }

    /// For containers with random access, just return a pointer to it
    /// For non-random access, copy to vector
    template <typename Container>
    auto ensure_random_access(const Container & c)
    {
      if constexpr (has_random_access<Container>())
        return &c; // Return pointer to original
      else
        return std::make_unique<std::vector<typename Container::value_type>>(std::begin(c), std::end(c));
    }

    /// Get reference from pointer or unique_ptr
    template <typename T>
    decltype(auto) deref(T && ptr)
    {
      if constexpr (std::is_pointer_v<std::decay_t<T>>)
        return *ptr;
      else
        return *ptr; // unique_ptr also supports *
    }

    inline size_t effective_parallel_chunk_size(const size_t n,
                                                const ThreadPool & pool,
                                                const ParallelOptions & options,
                                                const size_t min_chunk = 64)
    {
      size_t effective = options.chunk_size == 0 ? chunk_size(n, pool.num_threads(), min_chunk) : options.chunk_size;

      if (options.max_tasks > 0)
        {
          const size_t min_for_cap = (n + options.max_tasks - 1) / options.max_tasks;
          effective = std::max(effective, std::max(min_chunk, min_for_cap));
        }

      return std::max<size_t>(1, effective);
    }

    inline bool use_sequential_parallel_path(const size_t n,
                                             const ThreadPool & pool,
                                             const ParallelOptions & options) noexcept
    {
      if (n == 0)
        return true;
      if (options.cancel_token.stop_requested())
        return true;
      if (options.max_tasks == 1)
        return true;
      if (options.min_size > 0 and n < options.min_size)
        return true;
      return pool.num_threads() <= 1;
    }

    inline void throw_if_parallel_canceled(const CancellationToken & token)
    {
      token.throw_if_cancellation_requested();
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

      @ingroup Algorithms
  */
  template <typename ResultT = void, typename Container, typename Op>
  [[nodiscard]] auto pmaps(ThreadPool & pool, const Container & c, Op op,
                           size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    return pmaps<ResultT>(c, op, options);
  }

  template <typename ResultT = void, typename Container, typename Op>
  [[nodiscard]] auto pmaps(const Container & c, Op op,
                           const ParallelOptions & options = {})
  {
    ThreadPool & pool = parallel_detail::selected_parallel_pool(options);
    using InputT = std::decay_t<decltype(*std::begin(c))>;
    using ActualResultT = std::conditional_t<
      std::is_void_v<ResultT>,
      std::invoke_result_t<Op, const InputT &>,
      ResultT>;

    const size_t n = std::distance(std::begin(c), std::end(c));
    if (n == 0)
      return std::vector<ActualResultT>{};

    const size_t chunk_size =
        parallel_detail::effective_parallel_chunk_size(n, pool, options);

    // Ensure random access for parallel processing
    auto data_holder = parallel_detail::ensure_random_access(c);
    const auto & data = parallel_detail::deref(data_holder);

    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        std::vector<ActualResultT> result;
        result.reserve(n);
        for (auto it = std::begin(data); it != std::end(data); ++it)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            result.push_back(op(*it));
          }
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        return result;
      }

    std::vector<ActualResultT> result(n);
    std::vector<std::future<void>> futures;

    size_t offset = 0;

    while (offset < n)
      {
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&result, &data, op, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           auto in_it = std::begin(data);
                                           std::advance(in_it, offset);
                                           for (size_t i = offset; i < chunk_end; ++i, ++in_it)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               result[i] = op(*in_it);
                                             }
                                         }));

        offset = chunk_end;
      }

    for (auto & f: futures)
      f.get();

    parallel_detail::throw_if_parallel_canceled(options.cancel_token);

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

      @ingroup Algorithms
  */
  template <typename Container, typename Pred>
  [[nodiscard]] auto pfilter(ThreadPool & pool, const Container & c, Pred pred,
                             size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    return pfilter(c, pred, options);
  }

  template <typename Container, typename Pred>
  [[nodiscard]] auto pfilter(const Container & c, Pred pred,
                             const ParallelOptions & options = {})
  {
    ThreadPool & pool = parallel_detail::selected_parallel_pool(options);
    using T = std::decay_t<decltype(*std::begin(c))>;

    const size_t n = std::distance(std::begin(c), std::end(c));
    if (n == 0)
      return std::vector<T>{};

    const size_t chunk_size =
        parallel_detail::effective_parallel_chunk_size(n, pool, options);

    auto data_holder = parallel_detail::ensure_random_access(c);
    const auto & data = parallel_detail::deref(data_holder);

    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        std::vector<T> result;
        for (auto it = std::begin(data); it != std::end(data); ++it)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            if (pred(*it))
              result.push_back(*it);
          }
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        return result;
      }

    std::vector<std::future<std::vector<T>>> futures;
    const size_t num_chunks = parallel_detail::chunk_count(n, chunk_size);
    futures.reserve(num_chunks);

    for (size_t chunk_idx = 0; chunk_idx < num_chunks; ++chunk_idx)
      {
        const auto bounds = parallel_detail::bounds_for_chunk(chunk_idx, n, chunk_size);

        futures.push_back(pool.enqueue([&data, pred,
                                        offset = bounds.offset,
                                        chunk_end = bounds.end,
                                        token = options.cancel_token]()
          {
            std::vector<T> chunk_result;
            auto it = std::begin(data);
            std::advance(it, offset);
            for (size_t i = offset; i < chunk_end; ++i, ++it)
              {
                parallel_detail::throw_if_parallel_canceled(token);
                if (pred(*it))
                  chunk_result.push_back(*it);
              }
            return chunk_result;
          }));
      }

    std::vector<T> result;
    for (auto & f: futures)
      {
        auto chunk_result = f.get();
        result.insert(result.end(),
                      std::make_move_iterator(chunk_result.begin()),
                      std::make_move_iterator(chunk_result.end()));
      }

    parallel_detail::throw_if_parallel_canceled(options.cancel_token);

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

      @ingroup Algorithms
  */
  template <typename T, typename Container, typename BinaryOp>
  [[nodiscard]] T pfoldl(ThreadPool & pool, const Container & c, T init, BinaryOp op,
                         size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    return pfoldl(c, init, op, options);
  }

  template <typename T, typename Container, typename BinaryOp>
  [[nodiscard]] T pfoldl(const Container & c, T init, BinaryOp op,
                         const ParallelOptions & options = {})
  {
    ThreadPool & pool = parallel_detail::selected_parallel_pool(options);
    const size_t n = std::distance(std::begin(c), std::end(c));
    if (n == 0)
      return init;

    const size_t chunk_size =
        parallel_detail::effective_parallel_chunk_size(n, pool, options);

    auto data_holder = parallel_detail::ensure_random_access(c);
    const auto & data = parallel_detail::deref(data_holder);

    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        T result = init;
        for (auto it = std::begin(data); it != std::end(data); ++it)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            result = op(result, *it);
          }
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        return result;
      }

    std::vector<std::future<T>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&data, op, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           auto it = std::begin(data);
                                           std::advance(it, offset);
                                           parallel_detail::throw_if_parallel_canceled(token);
                                           T local = *it++;
                                           for (size_t i = offset + 1; i < chunk_end; ++i, ++it)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               local = op(local, *it);
                                             }
                                           return local;
                                         }));

        offset = chunk_end;
      }

    // Combine partial results
    T result = init;
    for (auto & f: futures)
      result = op(result, f.get());

    parallel_detail::throw_if_parallel_canceled(options.cancel_token);

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

      @ingroup Algorithms
  */
  template <typename Container, typename Op>
  void pfor_each(ThreadPool & pool, Container & c, Op op, size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    pfor_each(c, op, options);
  }

  template <typename Container, typename Op>
  void pfor_each(Container & c, Op op, const ParallelOptions & options = {})
  {
    ThreadPool & pool = parallel_detail::selected_parallel_pool(options);
    const size_t n = std::distance(std::begin(c), std::end(c));
    if (n == 0)
      return;

    const size_t chunk_size =
        parallel_detail::effective_parallel_chunk_size(n, pool, options);

    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        for (auto it = std::begin(c); it != std::end(c); ++it)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            op(*it);
          }
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        return;
      }

    std::vector<std::future<void>> futures;
    size_t offset = 0;

    while (offset < n)
      {
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&c, op, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           auto it = std::begin(c);
                                           std::advance(it, offset);
                                           for (size_t i = offset; i < chunk_end; ++i, ++it)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               op(*it);
                                             }
                                         }));

        offset = chunk_end;
      }

    for (auto & f: futures)
      f.get();

    parallel_detail::throw_if_parallel_canceled(options.cancel_token);
  }

  /** \overload
   *
   *  Parallel for_each for const containers.
   *
   *  @tparam Container Container type.
   *  @tparam Op Operation type.
   *  @param pool Thread pool to use.
   *  @param c Container (elements accessed by const reference).
   *  @param op Function to apply: `void op(const T&)`.
   *  @param chunk_size Elements per chunk (0 = auto).
   *  @see pfor_each(ThreadPool&, Container&, Op, size_t) For the non-const version.
   */
  template <typename Container, typename Op>
  void pfor_each(ThreadPool & pool, const Container & c, Op op, size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    pfor_each(c, op, options);
  }

  template <typename Container, typename Op>
  void pfor_each(const Container & c, Op op, const ParallelOptions & options = {})
  {
    ThreadPool & pool = parallel_detail::selected_parallel_pool(options);
    const size_t n = std::distance(std::begin(c), std::end(c));
    if (n == 0)
      return;

    const size_t chunk_size =
        parallel_detail::effective_parallel_chunk_size(n, pool, options);

    auto data_holder = parallel_detail::ensure_random_access(c);
    const auto & data = parallel_detail::deref(data_holder);

    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        for (auto it = std::begin(data); it != std::end(data); ++it)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            op(*it);
          }
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        return;
      }

    std::vector<std::future<void>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&data, op, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           auto it = std::begin(data);
                                           std::advance(it, offset);
                                           for (size_t i = offset; i < chunk_end; ++i, ++it)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               op(*it);
                                             }
                                         }));

        offset = chunk_end;
      }

    for (auto & f: futures)
      f.get();

    parallel_detail::throw_if_parallel_canceled(options.cancel_token);
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

      @ingroup Algorithms
  */
  template <typename Container, typename Pred>
  [[nodiscard]] bool pall(ThreadPool & pool, const Container & c, Pred pred,
                          size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    return pall(c, pred, options);
  }

  template <typename Container, typename Pred>
  [[nodiscard]] bool pall(const Container & c, Pred pred,
                          const ParallelOptions & options = {})
  {
    ThreadPool & pool = parallel_detail::selected_parallel_pool(options);
    const size_t n = std::distance(std::begin(c), std::end(c));
    if (n == 0)
      return true;

    const size_t chunk_size =
        parallel_detail::effective_parallel_chunk_size(n, pool, options);

    auto data_holder = parallel_detail::ensure_random_access(c);
    const auto & data = parallel_detail::deref(data_holder);

    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        for (auto it = std::begin(data); it != std::end(data); ++it)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            if (not pred(*it))
              return false;
          }
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        return true;
      }

    std::atomic<bool> found_false{false};
    std::vector<std::future<void>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&data, pred, &found_false, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           parallel_detail::throw_if_parallel_canceled(token);
                                           if (found_false.load(std::memory_order_relaxed))
                                             return; // Short-circuit

                                           auto it = std::begin(data);
                                           std::advance(it, offset);
                                           for (size_t i = offset; i < chunk_end; ++i, ++it)
                                             {
                                               if (not pred(*it))
                                                 {
                                                   found_false.store(true, std::memory_order_relaxed);
                                                   return;
                                                 }
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               if (found_false.load(std::memory_order_relaxed))
                                                 return;
                                             }
                                         }));

        offset = chunk_end;
      }

    for (auto & f: futures)
      f.get();

    parallel_detail::throw_if_parallel_canceled(options.cancel_token);
    return not found_false.load();
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

      @ingroup Algorithms
  */
  template <typename Container, typename Pred>
  [[nodiscard]] bool pexists(ThreadPool & pool, const Container & c, Pred pred,
                             size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    return pexists(c, pred, options);
  }

  template <typename Container, typename Pred>
  [[nodiscard]] bool pexists(const Container & c, Pred pred,
                             const ParallelOptions & options = {})
  {
    ThreadPool & pool = parallel_detail::selected_parallel_pool(options);
    const size_t n = std::distance(std::begin(c), std::end(c));
    if (n == 0)
      return false;

    const size_t chunk_size =
        parallel_detail::effective_parallel_chunk_size(n, pool, options);

    auto data_holder = parallel_detail::ensure_random_access(c);
    const auto & data = parallel_detail::deref(data_holder);

    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        for (auto it = std::begin(data); it != std::end(data); ++it)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            if (pred(*it))
              return true;
          }
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        return false;
      }

    std::atomic<bool> found{false};
    std::vector<std::future<void>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&data, pred, &found, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           parallel_detail::throw_if_parallel_canceled(token);
                                           if (found.load(std::memory_order_relaxed))
                                             return; // Short-circuit

                                           auto it = std::begin(data);
                                           std::advance(it, offset);
                                           for (size_t i = offset; i < chunk_end; ++i, ++it)
                                             {
                                               if (pred(*it))
                                                 {
                                                   found.store(true, std::memory_order_relaxed);
                                                   return;
                                                 }
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               if (found.load(std::memory_order_relaxed))
                                                 return;
                                             }
                                         }));

        offset = chunk_end;
      }

    for (auto & f: futures)
      f.get();

    parallel_detail::throw_if_parallel_canceled(options.cancel_token);
    return found.load();
  }

  /** @brief Parallel none predicate.

      Tests if no element satisfies a predicate.

      @param pool Thread pool to use.
      @param c Source container.
      @param pred Predicate: `bool pred(const T&)`.
      @param chunk_size Elements per chunk (0 = auto).

      @return true if no element satisfies pred, false otherwise.

      @ingroup Algorithms
  */
  template <typename Container, typename Pred>
  [[nodiscard]] bool pnone(ThreadPool & pool, const Container & c, Pred pred,
                           size_t chunk_size = 0)
  {
    return not pexists(pool, c, pred, chunk_size);
  }

  template <typename Container, typename Pred>
  [[nodiscard]] bool pnone(const Container & c, Pred pred,
                           const ParallelOptions & options = {})
  {
    return not pexists(c, pred, options);
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

      @ingroup Algorithms
  */
  template <typename Container, typename Pred>
  [[nodiscard]] size_t pcount_if(ThreadPool & pool, const Container & c, Pred pred,
                                 size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    return pcount_if(c, pred, options);
  }

  template <typename Container, typename Pred>
  [[nodiscard]] size_t pcount_if(const Container & c, Pred pred,
                                 const ParallelOptions & options = {})
  {
    ThreadPool & pool = parallel_detail::selected_parallel_pool(options);
    const size_t n = std::distance(std::begin(c), std::end(c));
    if (n == 0)
      return 0;

    const size_t chunk_size =
        parallel_detail::effective_parallel_chunk_size(n, pool, options);

    auto data_holder = parallel_detail::ensure_random_access(c);
    const auto & data = parallel_detail::deref(data_holder);

    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        size_t total = 0;
        for (auto it = std::begin(data); it != std::end(data); ++it)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            if (pred(*it))
              ++total;
          }
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        return total;
      }

    std::vector<std::future<size_t>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&data, pred, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           size_t count = 0;
                                           auto it = std::begin(data);
                                           std::advance(it, offset);
                                           for (size_t i = offset; i < chunk_end; ++i, ++it)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               if (pred(*it))
                                                 ++count;
                                             }
                                           return count;
                                         }));

        offset = chunk_end;
      }

    size_t total = 0;
    for (auto & f: futures)
      total += f.get();

    parallel_detail::throw_if_parallel_canceled(options.cancel_token);
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

      @ingroup Algorithms
  */
  template <typename Container, typename Pred>
  [[nodiscard]] std::optional<size_t> pfind(ThreadPool & pool, const Container & c,
                                            Pred pred, size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    return pfind(c, pred, options);
  }

  template <typename Container, typename Pred>
  [[nodiscard]] std::optional<size_t> pfind(const Container & c,
                                            Pred pred,
                                            const ParallelOptions & options = {})
  {
    ThreadPool & pool = parallel_detail::selected_parallel_pool(options);
    const size_t n = std::distance(std::begin(c), std::end(c));
    if (n == 0)
      return std::nullopt;

    const size_t chunk_size =
        parallel_detail::effective_parallel_chunk_size(n, pool, options);

    auto data_holder = parallel_detail::ensure_random_access(c);
    const auto & data = parallel_detail::deref(data_holder);

    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        size_t idx = 0;
        for (auto it = std::begin(data); it != std::end(data); ++it, ++idx)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            if (pred(*it))
              return idx;
          }
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        return std::nullopt;
      }

    // Track minimum found index
    std::atomic<size_t> min_index{n}; // n means not found
    std::vector<std::future<void>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&data, pred, &min_index, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           parallel_detail::throw_if_parallel_canceled(token);
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

                                               parallel_detail::throw_if_parallel_canceled(token);
                                               if (pred(*it))
                                                 {
                                                   // Atomically update minimum
                                                   size_t expected = min_index.load(std::memory_order_relaxed);
                                                   while (i < expected and
                                                          not min_index.compare_exchange_weak(expected, i,
                                                               std::memory_order_relaxed));
                                                   return;
                                                 }
                                             }
                                         }));

        offset = chunk_end;
      }

    for (auto & f: futures)
      f.get();

    parallel_detail::throw_if_parallel_canceled(options.cancel_token);
    if (size_t result = min_index.load(); result < n)
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

      @ingroup Algorithms
  */
  template <typename Container, typename Pred>
  [[nodiscard]] auto pfind_value(ThreadPool & pool, const Container & c,
                                 Pred pred, size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    return pfind_value(c, pred, options);
  }

  template <typename Container, typename Pred>
  [[nodiscard]] auto pfind_value(const Container & c,
                                 Pred pred,
                                 const ParallelOptions & options = {})
  {
    using T = std::decay_t<decltype(*std::begin(c))>;

    auto idx = pfind(c, pred, options);
    if (not idx)
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

      @ingroup Algorithms
  */
  template <typename Container,
            typename T = std::decay_t<decltype(*std::begin(std::declval<Container>()))>>
  [[nodiscard]] T psum(ThreadPool & pool, const Container & c, T init = T{},
                       size_t chunk_size = 0)
  {
    return pfoldl(pool, c, init, std::plus<T>{}, chunk_size);
  }

  template <typename Container,
            typename T = std::decay_t<decltype(*std::begin(std::declval<Container>()))>>
  [[nodiscard]] T psum(const Container & c, T init, const ParallelOptions & options)
  {
    return pfoldl(c, init, std::plus<T>{}, options);
  }

  /** @brief Parallel product of elements.

      @param pool Thread pool to use.
      @param c Source container.
      @param init Initial value (default 1).
      @param chunk_size Elements per chunk (0 = auto).

      @return Product of all elements times init.

      @ingroup Algorithms
  */
  template <typename Container,
            typename T = std::decay_t<decltype(*std::begin(std::declval<Container>()))>>
  [[nodiscard]] T pproduct(ThreadPool & pool, const Container & c, T init = T{1},
                           size_t chunk_size = 0)
  {
    return pfoldl(pool, c, init, std::multiplies<T>{}, chunk_size);
  }

  template <typename Container,
            typename T = std::decay_t<decltype(*std::begin(std::declval<Container>()))>>
  [[nodiscard]] T pproduct(const Container & c, T init, const ParallelOptions & options)
  {
    return pfoldl(c, init, std::multiplies<T>{}, options);
  }

  /** @brief Parallel minimum element.

      @param pool Thread pool to use.
      @param c Source container.
      @param chunk_size Elements per chunk (0 = auto).

      @return Optional with minimum element, or nullopt if empty.

      @ingroup Algorithms
  */
  template <typename Container>
  [[nodiscard]] auto pmin(ThreadPool & pool, const Container & c, size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    return pmin(c, options);
  }

  template <typename Container>
  [[nodiscard]] auto pmin(const Container & c, const ParallelOptions & options = {})
  {
    ThreadPool & pool = parallel_detail::selected_parallel_pool(options);
    using T = std::decay_t<decltype(*std::begin(c))>;

    const size_t n = std::distance(std::begin(c), std::end(c));
    if (n == 0)
      return std::optional<T>{std::nullopt};

    const size_t chunk_size =
        parallel_detail::effective_parallel_chunk_size(n, pool, options);

    auto data_holder = parallel_detail::ensure_random_access(c);
    const auto & data = parallel_detail::deref(data_holder);

    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        auto it = std::begin(data);
        T result = *it++;
        for (; it != std::end(data); ++it)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            if (*it < result)
              result = *it;
          }
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        return std::optional<T>{result};
      }

    std::vector<std::future<T>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&data, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           auto it = std::begin(data);
                                           std::advance(it, offset);
                                           parallel_detail::throw_if_parallel_canceled(token);
                                           T local_min = *it++;
                                           for (size_t i = offset + 1; i < chunk_end; ++i, ++it)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               if (*it < local_min)
                                                 local_min = *it;
                                             }
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

    parallel_detail::throw_if_parallel_canceled(options.cancel_token);
    return std::optional<T>{result};
  }

  /** @brief Parallel maximum element.

      @param pool Thread pool to use.
      @param c Source container.
      @param chunk_size Elements per chunk (0 = auto).

      @return Optional with maximum element, or nullopt if empty.

      @ingroup Algorithms
  */
  template <typename Container>
  [[nodiscard]] auto pmax(ThreadPool & pool, const Container & c, size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    return pmax(c, options);
  }

  template <typename Container>
  [[nodiscard]] auto pmax(const Container & c, const ParallelOptions & options = {})
  {
    ThreadPool & pool = parallel_detail::selected_parallel_pool(options);
    using T = std::decay_t<decltype(*std::begin(c))>;

    const size_t n = std::distance(std::begin(c), std::end(c));
    if (n == 0)
      return std::optional<T>{std::nullopt};

    const size_t chunk_size =
        parallel_detail::effective_parallel_chunk_size(n, pool, options);

    auto data_holder = parallel_detail::ensure_random_access(c);
    const auto & data = parallel_detail::deref(data_holder);

    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        auto it = std::begin(data);
        T result = *it++;
        for (; it != std::end(data); ++it)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            if (*it > result)
              result = *it;
          }
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        return std::optional<T>{result};
      }

    std::vector<std::future<T>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&data, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           auto it = std::begin(data);
                                           std::advance(it, offset);
                                           parallel_detail::throw_if_parallel_canceled(token);
                                           T local_max = *it++;
                                           for (size_t i = offset + 1; i < chunk_end; ++i, ++it)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               if (*it > local_max)
                                                 local_max = *it;
                                             }
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

    parallel_detail::throw_if_parallel_canceled(options.cancel_token);
    return std::optional<T>{result};
  }

  /** @brief Parallel min and max elements.

      @param pool Thread pool to use.
      @param c Source container.
      @param chunk_size Elements per chunk (0 = auto).

      @return Optional pair (min, max), or nullopt if empty.

      @ingroup Algorithms
  */
  template <typename Container>
  [[nodiscard]] auto pminmax(ThreadPool & pool, const Container & c, size_t chunk_size = 0)
  {
    using T = std::decay_t<decltype(*std::begin(c))>;

    const size_t n = std::distance(std::begin(c), std::end(c));
    if (n == 0)
      return std::optional<std::pair<T, T>>{std::nullopt};

    if (chunk_size == 0)
      chunk_size = parallel_detail::chunk_size(n, pool.num_threads());

    auto data_holder = parallel_detail::ensure_random_access(c);
    const auto & data = parallel_detail::deref(data_holder);

    std::vector<std::future<std::pair<T, T>>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&data, offset, chunk_end]()
                                         {
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

      @ingroup Algorithms
  */
  template <typename Container, typename Compare = std::less<>>
  void psort(ThreadPool & pool, Container & c, Compare cmp = Compare{},
             const size_t min_parallel_size = 1024)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.min_size = min_parallel_size;
    psort(c, std::move(cmp), options);
  }

  template <typename Container, typename Compare = std::less<>>
  void psort(Container & c, Compare cmp = Compare{},
             const ParallelOptions & options = {})
  {
    const size_t n = std::distance(std::begin(c), std::end(c));
    if (n <= 1)
      return;

    auto & pool = parallel_detail::selected_parallel_pool(options);
    const size_t min_parallel_size = options.min_size == 0 ? 1024 : options.min_size;

    // For small sizes, use regular sort
    if (parallel_detail::use_sequential_parallel_path(n, pool, options)
        or n <= min_parallel_size)
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        std::sort(std::begin(c), std::end(c), cmp);
        return;
      }

    // Split into chunks, sort each in parallel, then merge
    size_t num_chunks = std::min(pool.num_threads() * 2, n / min_parallel_size);
    if (options.max_tasks > 0)
      num_chunks = std::min(num_chunks, options.max_tasks);
    num_chunks = std::max<size_t>(1, num_chunks);
    const size_t chunk_size = (n + num_chunks - 1) / num_chunks;

    // Sort chunks in parallel
    std::vector<std::future<void>> futures;
    for (size_t i = 0; i < n; i += chunk_size)
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        size_t end = std::min(i + chunk_size, n);
        auto begin_it = std::begin(c);
        std::advance(begin_it, i);
        auto end_it = std::begin(c);
        std::advance(end_it, end);

        futures.push_back(pool.enqueue([begin_it, end_it, cmp, token = options.cancel_token]()
                                         {
                                           parallel_detail::throw_if_parallel_canceled(token);
                                           std::sort(begin_it, end_it, cmp);
                                         }));
      }

    for (auto & f: futures)
      f.get();

    // Merge sorted chunks
    using T = std::decay_t<decltype(*std::begin(c))>;
    std::vector<T> buffer(n);

    ParallelOptions merge_options = options;
    merge_options.pool = &pool;

    for (size_t width = chunk_size; width < n; width *= 2)
      {
        for (size_t i = 0; i < n; i += 2 * width)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
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

                Aleph::pmerge(begin_it, mid_it, mid_it, end_it,
                              buffer.begin() + i, cmp, merge_options);
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

      @ingroup Algorithms
  */
  template <typename Container1, typename Container2, typename Op>
  void pzip_for_each(ThreadPool & pool, const Container1 & c1, const Container2 & c2,
                     Op op, size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    pzip_for_each(c1, c2, op, options);
  }

  template <typename Container1, typename Container2, typename Op>
  void pzip_for_each(const Container1 & c1, const Container2 & c2, Op op,
                     const ParallelOptions & options = {})
  {
    const size_t n1 = std::distance(std::begin(c1), std::end(c1));
    const size_t n2 = std::distance(std::begin(c2), std::end(c2));
    const size_t n = std::min(n1, n2);

    if (n == 0)
      return;

    auto & pool = parallel_detail::selected_parallel_pool(options);
    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        auto it1 = std::begin(c1);
        auto it2 = std::begin(c2);
        for (size_t i = 0; i < n; ++i, ++it1, ++it2)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            op(*it1, *it2);
          }
        return;
      }

    size_t chunk_size = parallel_detail::effective_parallel_chunk_size(n, pool, options,
                                                                       pool.num_threads());

    auto h1 = parallel_detail::ensure_random_access(c1);
    auto h2 = parallel_detail::ensure_random_access(c2);
    const auto & d1 = parallel_detail::deref(h1);
    const auto & d2 = parallel_detail::deref(h2);

    std::vector<std::future<void>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&d1, &d2, op, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           auto it1 = std::begin(d1);
                                           auto it2 = std::begin(d2);
                                           std::advance(it1, offset);
                                           std::advance(it2, offset);
                                           for (size_t i = offset; i < chunk_end; ++i, ++it1, ++it2)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               op(*it1, *it2);
                                             }
                                         }));

        offset = chunk_end;
      }

    for (auto & f: futures)
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

      @ingroup Algorithms
  */
  template <typename Container1, typename Container2, typename Op>
  [[nodiscard]] auto pzip_maps(ThreadPool & pool, const Container1 & c1,
                               const Container2 & c2, Op op, size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    return pzip_maps(c1, c2, op, options);
  }

  template <typename Container1, typename Container2, typename Op>
  [[nodiscard]] auto pzip_maps(const Container1 & c1, const Container2 & c2, Op op,
                               const ParallelOptions & options = {})
  {
    using T1 = std::decay_t<decltype(*std::begin(c1))>;
    using T2 = std::decay_t<decltype(*std::begin(c2))>;
    using ResultT = std::invoke_result_t<Op, const T1 &, const T2 &>;

    const size_t n1 = std::distance(std::begin(c1), std::end(c1));
    const size_t n2 = std::distance(std::begin(c2), std::end(c2));
    const size_t n = std::min(n1, n2);

    if (n == 0)
      return std::vector<ResultT>{};

    auto & pool = parallel_detail::selected_parallel_pool(options);
    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        std::vector<ResultT> result;
        result.reserve(n);
        auto it1 = std::begin(c1);
        auto it2 = std::begin(c2);
        for (size_t i = 0; i < n; ++i, ++it1, ++it2)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            result.push_back(op(*it1, *it2));
          }
        return result;
      }

    size_t chunk_size = parallel_detail::effective_parallel_chunk_size(n, pool, options,
                                                                       pool.num_threads());

    auto h1 = parallel_detail::ensure_random_access(c1);
    auto h2 = parallel_detail::ensure_random_access(c2);
    const auto & d1 = parallel_detail::deref(h1);
    const auto & d2 = parallel_detail::deref(h2);

    std::vector<ResultT> result(n);
    std::vector<std::future<void>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&result, &d1, &d2, op, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           auto it1 = std::begin(d1);
                                           auto it2 = std::begin(d2);
                                           std::advance(it1, offset);
                                           std::advance(it2, offset);
                                           for (size_t i = offset; i < chunk_end; ++i, ++it1, ++it2)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               result[i] = op(*it1, *it2);
                                             }
                                         }));

        offset = chunk_end;
      }

    for (auto & f: futures)
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

      @ingroup Algorithms
  */
  template <typename Container1, typename Container2, typename T, typename Op>
  [[nodiscard]] T pzip_foldl(ThreadPool & pool, const Container1 & c1,
                             const Container2 & c2, T init, Op op,
                             size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    return pzip_foldl(c1, c2, init, op, options);
  }

  template <typename Container1, typename Container2, typename T, typename Op>
  [[nodiscard]] T pzip_foldl(const Container1 & c1, const Container2 & c2, T init, Op op,
                             const ParallelOptions & options = {})
  {
    const size_t n1 = std::distance(std::begin(c1), std::end(c1));
    const size_t n2 = std::distance(std::begin(c2), std::end(c2));
    const size_t n = std::min(n1, n2);

    if (n == 0)
      return init;

    auto & pool = parallel_detail::selected_parallel_pool(options);
    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        auto it1 = std::begin(c1);
        auto it2 = std::begin(c2);
        T result = init;
        for (size_t i = 0; i < n; ++i, ++it1, ++it2)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            result = op(result, *it1, *it2);
          }
        return result;
      }

    size_t chunk_size = parallel_detail::effective_parallel_chunk_size(n, pool, options,
                                                                       pool.num_threads());

    auto h1 = parallel_detail::ensure_random_access(c1);
    auto h2 = parallel_detail::ensure_random_access(c2);
    const auto & d1 = parallel_detail::deref(h1);
    const auto & d2 = parallel_detail::deref(h2);

    std::vector<std::future<T>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&d1, &d2, &init, op, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           auto it1 = std::begin(d1);
                                           auto it2 = std::begin(d2);
                                           std::advance(it1, offset);
                                           std::advance(it2, offset);

                                           parallel_detail::throw_if_parallel_canceled(token);
                                           T local = op(init, *it1++, *it2++);
                                           for (size_t i = offset + 1; i < chunk_end; ++i, ++it1, ++it2)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               local = op(local, *it1, *it2);
                                             }
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
        result = result + val - init; // Compensate for init being added in each chunk
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

      @ingroup Algorithms
  */
  template <typename Container, typename Pred>
  [[nodiscard]] auto ppartition(ThreadPool & pool, const Container & c, Pred pred,
                                size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    return ppartition(c, pred, options);
  }

  template <typename Container, typename Pred>
  [[nodiscard]] auto ppartition(const Container & c, Pred pred,
                                const ParallelOptions & options = {})
  {
    using T = std::decay_t<decltype(*std::begin(c))>;
    ThreadPool & pool = parallel_detail::selected_parallel_pool(options);

    const size_t n = std::distance(std::begin(c), std::end(c));
    if (n == 0)
      return std::make_pair(std::vector<T>{}, std::vector<T>{});

    const size_t chunk_size =
        parallel_detail::effective_parallel_chunk_size(n, pool, options);

    auto data_holder = parallel_detail::ensure_random_access(c);
    const auto & data = parallel_detail::deref(data_holder);

    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        std::vector<T> yes_result, no_result;
        for (auto it = std::begin(data); it != std::end(data); ++it)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            if (pred(*it))
              yes_result.push_back(*it);
            else
              no_result.push_back(*it);
          }
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        return std::make_pair(std::move(yes_result), std::move(no_result));
      }

    const size_t num_chunks = parallel_detail::chunk_count(n, chunk_size);
    std::vector<size_t> yes_counts(num_chunks, 0);
    std::vector<size_t> no_counts(num_chunks, 0);
    std::vector<std::future<void>> count_futures;
    count_futures.reserve(num_chunks);

    for (size_t chunk_idx = 0; chunk_idx < num_chunks; ++chunk_idx)
      {
        const auto bounds = parallel_detail::bounds_for_chunk(chunk_idx, n, chunk_size);

        count_futures.push_back(pool.enqueue([&data, &yes_counts, &no_counts, pred,
                                              chunk_idx,
                                              offset = bounds.offset,
                                              chunk_end = bounds.end,
                                              token = options.cancel_token]()
          {
            size_t yes = 0;
            size_t no = 0;
            auto it = std::begin(data);
            std::advance(it, offset);
            for (size_t i = offset; i < chunk_end; ++i, ++it)
              {
                parallel_detail::throw_if_parallel_canceled(token);
                if (pred(*it))
                  ++yes;
                else
                  ++no;
              }
            yes_counts[chunk_idx] = yes;
            no_counts[chunk_idx] = no;
          }));
      }

    for (auto & f: count_futures)
      f.get();

    parallel_detail::throw_if_parallel_canceled(options.cancel_token);

    std::vector<size_t> yes_offsets(num_chunks, 0);
    std::vector<size_t> no_offsets(num_chunks, 0);
    if (num_chunks > 0)
      {
        Aleph::pexclusive_scan(yes_counts.begin(), yes_counts.end(), yes_offsets.begin(),
                               size_t{0}, std::plus<size_t>{}, options);
        Aleph::pexclusive_scan(no_counts.begin(), no_counts.end(), no_offsets.begin(),
                               size_t{0}, std::plus<size_t>{}, options);
      }

    const size_t yes_total = num_chunks == 0 ? 0 : yes_offsets.back() + yes_counts.back();
    const size_t no_total = num_chunks == 0 ? 0 : no_offsets.back() + no_counts.back();

    std::vector<std::optional<T>> yes_slots(yes_total);
    std::vector<std::optional<T>> no_slots(no_total);
    std::vector<std::future<void>> fill_futures;
    fill_futures.reserve(num_chunks);

    for (size_t chunk_idx = 0; chunk_idx < num_chunks; ++chunk_idx)
      {
        const auto bounds = parallel_detail::bounds_for_chunk(chunk_idx, n, chunk_size);
        const size_t yes_offset = yes_offsets[chunk_idx];
        const size_t no_offset = no_offsets[chunk_idx];

        fill_futures.push_back(pool.enqueue([&data, &yes_slots, &no_slots, pred,
                                             offset = bounds.offset,
                                             chunk_end = bounds.end,
                                             yes_offset, no_offset,
                                             token = options.cancel_token]()
          {
            auto it = std::begin(data);
            std::advance(it, offset);
            size_t yes_pos = yes_offset;
            size_t no_pos = no_offset;
            for (size_t i = offset; i < chunk_end; ++i, ++it)
              {
                parallel_detail::throw_if_parallel_canceled(token);
                if (pred(*it))
                  yes_slots[yes_pos++].emplace(*it);
                else
                  no_slots[no_pos++].emplace(*it);
              }
          }));
      }

    for (auto & f: fill_futures)
      f.get();

    parallel_detail::throw_if_parallel_canceled(options.cancel_token);

    std::vector<T> yes_result, no_result;
    yes_result.reserve(yes_total);
    no_result.reserve(no_total);

    for (auto & slot: yes_slots)
      yes_result.push_back(std::move(*slot));
    for (auto & slot: no_slots)
      no_result.push_back(std::move(*slot));

    return std::make_pair(std::move(yes_result), std::move(no_result));
  }

  // =============================================================================
  // Parallel Scan and Merge Wrappers
  // =============================================================================

  /** @brief Parallel inclusive scan over a container.

      Returns a vector containing inclusive prefixes of the input container.
      The result has the same size as the input.
  */
  template <typename Container, typename BinaryOp>
  [[nodiscard]] auto pscan(ThreadPool & pool, const Container & c, BinaryOp op,
                           size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    return pscan(c, op, options);
  }

  /** @brief Parallel inclusive scan over a container with explicit options. */
  template <typename Container, typename BinaryOp>
  [[nodiscard]] auto pscan(const Container & c, BinaryOp op,
                           const ParallelOptions & options = {})
  {
    using T = std::decay_t<decltype(*std::begin(c))>;

    auto holder = parallel_detail::ensure_random_access(c);
    const auto & data = parallel_detail::deref(holder);
    const size_t n = static_cast<size_t>(std::distance(std::begin(data), std::end(data)));
    std::vector<std::optional<T>> slots(n);

    Aleph::pscan(std::begin(data), std::end(data), slots.begin(), op, options);

    std::vector<T> result;
    result.reserve(n);
    for (auto & slot: slots)
      result.push_back(std::move(*slot));
    return result;
  }

  /** @brief Parallel exclusive scan over a container.

      Returns a vector containing exclusive prefixes of the input container.
      The first result element is `init`.
  */
  template <typename Container, typename T, typename BinaryOp>
  [[nodiscard]] auto pexclusive_scan(ThreadPool & pool, const Container & c, T init,
                                     BinaryOp op, size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    return pexclusive_scan(c, init, op, options);
  }

  /** @brief Parallel exclusive scan over a container with explicit options. */
  template <typename Container, typename T, typename BinaryOp>
  [[nodiscard]] auto pexclusive_scan(const Container & c, T init, BinaryOp op,
                                     const ParallelOptions & options = {})
  {
    auto holder = parallel_detail::ensure_random_access(c);
    const auto & data = parallel_detail::deref(holder);
    const size_t n = static_cast<size_t>(std::distance(std::begin(data), std::end(data)));
    std::vector<std::optional<T>> slots(n);

    Aleph::pexclusive_scan(std::begin(data), std::end(data), slots.begin(),
                           init, op, options);

    std::vector<T> result;
    result.reserve(n);
    for (auto & slot: slots)
      result.push_back(std::move(*slot));
    return result;
  }

  /** @brief Parallel merge of two sorted containers.

      Both containers must already be sorted according to `comp`.
  */
  template <typename Container1, typename Container2, typename Compare = std::less<>>
  [[nodiscard]] auto pmerge(ThreadPool & pool, const Container1 & c1, const Container2 & c2,
                            Compare comp = Compare{}, size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    return pmerge(c1, c2, std::move(comp), options);
  }

  /** @brief Parallel merge of two sorted containers with explicit options. */
  template <typename Container1, typename Container2, typename Compare = std::less<>>
  [[nodiscard]] auto pmerge(const Container1 & c1, const Container2 & c2,
                            Compare comp = Compare{},
                            const ParallelOptions & options = {})
  {
    using T1 = std::decay_t<decltype(*std::begin(c1))>;
    using T2 = std::decay_t<decltype(*std::begin(c2))>;
    using ResultT = std::common_type_t<T1, T2>;

    auto h1 = parallel_detail::ensure_random_access(c1);
    auto h2 = parallel_detail::ensure_random_access(c2);
    const auto & d1 = parallel_detail::deref(h1);
    const auto & d2 = parallel_detail::deref(h2);

    const size_t total = static_cast<size_t>(std::distance(std::begin(d1), std::end(d1))
                                             + std::distance(std::begin(d2), std::end(d2)));
    std::vector<std::optional<ResultT>> slots(total);

    Aleph::pmerge(std::begin(d1), std::end(d1),
                  std::begin(d2), std::end(d2),
                  slots.begin(), comp, options);

    std::vector<ResultT> result;
    result.reserve(total);
    for (auto & slot: slots)
      result.push_back(std::move(*slot));
    return result;
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
      using value_type = std::decay_t<decltype(*std::begin(std::declval<Container &>()))>;
      using holder_type = std::conditional_t<
        parallel_detail::has_random_access<Container>(),
        const Container *,
        std::unique_ptr<std::vector<value_type>>>;

      holder_type data;
      size_t cached_size; ///< Cached size for O(1) access

      explicit ContainerHolder(const Container & c)
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
    size_t min_holder_size_impl(const std::tuple<Holders...> & holders,
                                std::index_sequence<Is...>)
    {
      return std::min({std::get<Is>(holders).size()...});
    }

    template <typename... Holders>
    size_t min_holder_size(const std::tuple<Holders...> & holders)
    {
      return min_holder_size_impl(holders, std::make_index_sequence<sizeof...(Holders)>{});
    }

    /// Create tuple of iterators at given offset
    template <typename... Holders, size_t... Is>
    auto make_iterators_at(size_t offset, const std::tuple<Holders...> & holders,
                           std::index_sequence<Is...>)
    {
      return std::make_tuple([&]()
                               {
                                 auto it = std::get<Is>(holders).begin();
                                 std::advance(it, offset);
                                 return it;
                               }()...);
    }

    /// Advance all iterators in tuple
    template <typename... Iters, size_t... Is>
    void advance_all_iters(std::tuple<Iters...> & iters, std::index_sequence<Is...>)
    {
      (++std::get<Is>(iters), ...);
    }

    /// Dereference all iterators and make tuple
    template <typename... Iters, size_t... Is>
    auto deref_all_iters(const std::tuple<Iters...> & iters, std::index_sequence<Is...>)
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

      @ingroup Algorithms
  */
  template <typename Op, typename... Containers>
  void pzip_for_each_n(ThreadPool & pool, Op op, const Containers &... cs)
  {
    ParallelOptions options;
    options.pool = &pool;
    pzip_for_each_n(op, options, cs...);
  }

  template <typename Op, typename... Containers>
  void pzip_for_each_n(Op op, const ParallelOptions & options, const Containers &... cs)
  {
    static_assert(sizeof...(Containers) >= 2,
                  "pzip_for_each requires at least 2 containers");

    // Convert all containers to random access FIRST
    // This is O(n) for non-RA containers, but unavoidable
    auto holders = std::make_tuple(parallel_zip_detail::ContainerHolder<Containers>(cs)...);

    // Now get min size - O(1) because all holders have cached sizes
    const size_t n = parallel_zip_detail::min_holder_size(holders);
    if (n == 0)
      return;

    auto & pool = parallel_detail::selected_parallel_pool(options);
    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        constexpr size_t N = sizeof...(Containers);
        auto iters = parallel_zip_detail::make_iterators_at(
                                                            0, holders, std::make_index_sequence<N>{});
        for (size_t i = 0; i < n; ++i)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            std::apply(op, parallel_zip_detail::deref_all_iters(
                                                                iters, std::make_index_sequence<N>{}));
            parallel_zip_detail::advance_all_iters(iters, std::make_index_sequence<N>{});
          }
        return;
      }

    const size_t chunk_size = parallel_detail::effective_parallel_chunk_size(
                                                                             n, pool, options, pool.num_threads());

    std::vector<std::future<void>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&holders, op, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           constexpr size_t N = sizeof...(Containers);
                                           auto iters = parallel_zip_detail::make_iterators_at(
                                              offset, holders, std::make_index_sequence<N>{});

                                           for (size_t i = offset; i < chunk_end; ++i)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               std::apply(op, parallel_zip_detail::deref_all_iters(
                                                             iters, std::make_index_sequence<N>{}));
                                               parallel_zip_detail::advance_all_iters(
                                                  iters, std::make_index_sequence<N>{});
                                             }
                                         }));

        offset = chunk_end;
      }

    for (auto & f: futures)
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

      @ingroup Algorithms
  */
  template <typename Op, typename... Containers>
  [[nodiscard]] auto pzip_maps_n(ThreadPool & pool, Op op, const Containers &... cs)
  {
    ParallelOptions options;
    options.pool = &pool;
    return pzip_maps_n(op, options, cs...);
  }

  template <typename Op, typename... Containers>
  [[nodiscard]] auto pzip_maps_n(Op op, const ParallelOptions & options,
                                 const Containers &... cs)
  {
    static_assert(sizeof...(Containers) >= 2,
                  "pzip_maps requires at least 2 containers");

    // Deduce result type from operation
    using ResultT = std::invoke_result_t<Op,
                                         std::decay_t<decltype(*std::begin(cs))>...>;

    // Convert all containers to random access FIRST
    auto holders = std::make_tuple(parallel_zip_detail::ContainerHolder<Containers>(cs)...);

    // Now get min size - O(1) because all holders have cached sizes
    const size_t n = parallel_zip_detail::min_holder_size(holders);
    if (n == 0)
      return std::vector<ResultT>{};

    auto & pool = parallel_detail::selected_parallel_pool(options);
    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        std::vector<ResultT> result(n);
        constexpr size_t N = sizeof...(Containers);
        auto iters = parallel_zip_detail::make_iterators_at(
                                                            0, holders, std::make_index_sequence<N>{});
        for (size_t i = 0; i < n; ++i)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            result[i] = std::apply(op, parallel_zip_detail::deref_all_iters(
                                                                            iters, std::make_index_sequence<N>{}));
            parallel_zip_detail::advance_all_iters(iters, std::make_index_sequence<N>{});
          }
        return result;
      }

    size_t chunk_size = parallel_detail::effective_parallel_chunk_size(n, pool, options,
                                                                       pool.num_threads());

    std::vector<ResultT> result(n);
    std::vector<std::future<void>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&result, &holders, op, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           constexpr size_t N = sizeof...(Containers);
                                           auto iters = parallel_zip_detail::make_iterators_at(
                                              offset, holders, std::make_index_sequence<N>{});

                                           for (size_t i = offset; i < chunk_end; ++i)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               result[i] = std::apply(op, parallel_zip_detail::deref_all_iters(
                                                                         iters, std::make_index_sequence<N>{}));
                                               parallel_zip_detail::advance_all_iters(
                                                  iters, std::make_index_sequence<N>{});
                                             }
                                         }));

        offset = chunk_end;
      }

    for (auto & f: futures)
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

      @ingroup Algorithms
  */
  template <typename T, typename Op, typename Combiner, typename... Containers>
  [[nodiscard]] T pzip_foldl_n(ThreadPool & pool, T init, Op op, Combiner combiner,
                               const Containers &... cs)
  {
    ParallelOptions options;
    options.pool = &pool;
    return pzip_foldl_n(init, op, combiner, options, cs...);
  }

  template <typename T, typename Op, typename Combiner, typename... Containers>
  [[nodiscard]] T pzip_foldl_n(T init, Op op, Combiner combiner,
                               const ParallelOptions & options, const Containers &... cs)
  {
    static_assert(sizeof...(Containers) >= 2,
                  "pzip_foldl requires at least 2 containers");

    // Convert all containers to random access FIRST
    auto holders = std::make_tuple(parallel_zip_detail::ContainerHolder<Containers>(cs)...);

    // Now get min size - O(1) because all holders have cached sizes
    const size_t n = parallel_zip_detail::min_holder_size(holders);
    if (n == 0)
      return init;

    auto & pool = parallel_detail::selected_parallel_pool(options);
    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        constexpr size_t N = sizeof...(Containers);
        auto iters = parallel_zip_detail::make_iterators_at(
                                                            0, holders, std::make_index_sequence<N>{});
        T result = init;
        for (size_t i = 0; i < n; ++i)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            auto tuple = parallel_zip_detail::deref_all_iters(iters, std::make_index_sequence<N>{});
            result = std::apply([&op, &result](auto &&... args)
                                  {
                                    return op(result, std::forward<decltype(args)>(args)...);
                                  }, tuple);
            parallel_zip_detail::advance_all_iters(iters, std::make_index_sequence<N>{});
          }
        return result;
      }

    size_t chunk_size = parallel_detail::effective_parallel_chunk_size(n, pool, options,
                                                                       pool.num_threads());

    std::vector<std::future<T>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&holders, init, op, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           constexpr size_t N = sizeof...(Containers);
                                           auto iters = parallel_zip_detail::make_iterators_at(
                                              offset, holders, std::make_index_sequence<N>{});

                                           // First element
                                           parallel_detail::throw_if_parallel_canceled(token);
                                           auto first_tuple = parallel_zip_detail::deref_all_iters(
                                              iters, std::make_index_sequence<N>{});
                                           T local = std::apply([&op, &init](auto &&... args)
                                                                  {
                                                                    return op(init, std::forward<decltype(args)>(args)
                                                                              ...);
                                                                  }, first_tuple);
                                           parallel_zip_detail::advance_all_iters(
                                              iters, std::make_index_sequence<N>{});

                                           // Remaining elements
                                           for (size_t i = offset + 1; i < chunk_end; ++i)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               auto tuple = parallel_zip_detail::deref_all_iters(
                                                  iters, std::make_index_sequence<N>{});
                                               local = std::apply([&op, &local](auto &&... args)
                                                                    {
                                                                      return op(local,
                                                                                  std::forward<decltype(args)>(args)
                                                                                  ...);
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

      @ingroup Algorithms
  */
  template <typename Pred, typename... Containers>
  [[nodiscard]] bool pzip_all_n(ThreadPool & pool, Pred pred, const Containers &... cs)
  {
    ParallelOptions options;
    options.pool = &pool;
    return pzip_all_n(pred, options, cs...);
  }

  template <typename Pred, typename... Containers>
  [[nodiscard]] bool pzip_all_n(Pred pred, const ParallelOptions & options,
                                const Containers &... cs)
  {
    static_assert(sizeof...(Containers) >= 2,
                  "pzip_all requires at least 2 containers");

    // Convert all containers to random access FIRST
    auto holders = std::make_tuple(
                                   parallel_zip_detail::ContainerHolder<Containers>(cs)...);

    // Now get min size - O(1) because all holders have cached sizes
    const size_t n = parallel_zip_detail::min_holder_size(holders);
    if (n == 0)
      return true; // Vacuous truth

    auto & pool = parallel_detail::selected_parallel_pool(options);
    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        constexpr size_t N = sizeof...(Containers);
        auto iters = parallel_zip_detail::make_iterators_at(
                                                            0, holders, std::make_index_sequence<N>{});
        for (size_t i = 0; i < n; ++i)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            auto tuple = parallel_zip_detail::deref_all_iters(iters, std::make_index_sequence<N>{});
            if (! std::apply(pred, tuple))
              return false;
            parallel_zip_detail::advance_all_iters(iters, std::make_index_sequence<N>{});
          }
        return true;
      }

    const size_t chunk_size = parallel_detail::effective_parallel_chunk_size(
                                                                             n, pool, options, pool.num_threads());

    std::atomic<bool> found_false{false};
    std::vector<std::future<void>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&holders, pred, &found_false, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           if (found_false.load(std::memory_order_relaxed))
                                             return;

                                           constexpr size_t N = sizeof...(Containers);
                                           auto iters = parallel_zip_detail::make_iterators_at(
                                              offset, holders, std::make_index_sequence<N>{});

                                           for (size_t i = offset; i < chunk_end; ++i)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               auto tuple = parallel_zip_detail::deref_all_iters(
                                                  iters, std::make_index_sequence<N>{});
                                               if (! std::apply(pred, tuple))
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

    for (auto & f: futures)
      f.get();

    return not found_false.load();
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

      @ingroup Algorithms
  */
  template <typename Pred, typename... Containers>
  [[nodiscard]] bool pzip_exists_n(ThreadPool & pool, Pred pred, const Containers &... cs)
  {
    ParallelOptions options;
    options.pool = &pool;
    return pzip_exists_n(pred, options, cs...);
  }

  template <typename Pred, typename... Containers>
  [[nodiscard]] bool pzip_exists_n(Pred pred, const ParallelOptions & options,
                                   const Containers &... cs)
  {
    static_assert(sizeof...(Containers) >= 2,
                  "pzip_exists requires at least 2 containers");

    // Convert all containers to random access FIRST
    auto holders = std::make_tuple(parallel_zip_detail::ContainerHolder<Containers>(cs)...);

    // Now get min size - O(1) because all holders have cached sizes
    const size_t n = parallel_zip_detail::min_holder_size(holders);
    if (n == 0)
      return false;

    auto & pool = parallel_detail::selected_parallel_pool(options);
    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        constexpr size_t N = sizeof...(Containers);
        auto iters = parallel_zip_detail::make_iterators_at(
                                                            0, holders, std::make_index_sequence<N>{});
        for (size_t i = 0; i < n; ++i)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            auto tuple = parallel_zip_detail::deref_all_iters(iters, std::make_index_sequence<N>{});
            if (std::apply(pred, tuple))
              return true;
            parallel_zip_detail::advance_all_iters(iters, std::make_index_sequence<N>{});
          }
        return false;
      }

    const size_t chunk_size = parallel_detail::effective_parallel_chunk_size(
                                                                             n, pool, options, pool.num_threads());

    std::atomic<bool> found{false};
    std::vector<std::future<void>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&holders, pred, &found, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           if (found.load(std::memory_order_relaxed))
                                             return;

                                           constexpr size_t N = sizeof...(Containers);
                                           auto iters = parallel_zip_detail::make_iterators_at(
                                              offset, holders, std::make_index_sequence<N>{});

                                           for (size_t i = offset; i < chunk_end; ++i)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
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

    for (auto & f: futures)
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

      @ingroup Algorithms
  */
  template <typename Pred, typename... Containers>
  [[nodiscard]] size_t pzip_count_if_n(ThreadPool & pool, Pred pred,
                                       const Containers &... cs)
  {
    ParallelOptions options;
    options.pool = &pool;
    return pzip_count_if_n(pred, options, cs...);
  }

  template <typename Pred, typename... Containers>
  [[nodiscard]] size_t pzip_count_if_n(Pred pred, const ParallelOptions & options,
                                       const Containers &... cs)
  {
    static_assert(sizeof...(Containers) >= 2,
                  "pzip_count_if requires at least 2 containers");

    // Convert all containers to random access FIRST
    auto holders = std::make_tuple(parallel_zip_detail::ContainerHolder<Containers>(cs)...);

    // Now get min size - O(1) because all holders have cached sizes
    const size_t n = parallel_zip_detail::min_holder_size(holders);
    if (n == 0)
      return 0;

    auto & pool = parallel_detail::selected_parallel_pool(options);
    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        constexpr size_t N = sizeof...(Containers);
        auto iters = parallel_zip_detail::make_iterators_at(
                                                            0, holders, std::make_index_sequence<N>{});
        size_t count = 0;
        for (size_t i = 0; i < n; ++i)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            auto tuple = parallel_zip_detail::deref_all_iters(iters, std::make_index_sequence<N>{});
            if (std::apply(pred, tuple))
              ++count;
            parallel_zip_detail::advance_all_iters(iters, std::make_index_sequence<N>{});
          }
        return count;
      }

    size_t chunk_size = parallel_detail::effective_parallel_chunk_size(n, pool, options,
                                                                       pool.num_threads());

    std::vector<std::future<size_t>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&holders, pred, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           constexpr size_t N = sizeof...(Containers);
                                           auto iters = parallel_zip_detail::make_iterators_at(
                                              offset, holders, std::make_index_sequence<N>{});

                                           size_t count = 0;
                                           for (size_t i = offset; i < chunk_end; ++i)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
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
    for (auto & f: futures)
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

      @ingroup Algorithms
  */
  template <typename Container, typename Op>
  void penumerate_for_each(ThreadPool & pool, Container & c, Op op,
                           size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    penumerate_for_each(c, op, options);
  }

  template <typename Container, typename Op>
  void penumerate_for_each(Container & c, Op op,
                           const ParallelOptions & options = {})
  {
    const size_t n = std::distance(std::begin(c), std::end(c));
    if (n == 0)
      return;

    auto & pool = parallel_detail::selected_parallel_pool(options);
    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        auto it = std::begin(c);
        for (size_t i = 0; i < n; ++i, ++it)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            op(i, *it);
          }
        return;
      }

    size_t chunk_size = parallel_detail::effective_parallel_chunk_size(n, pool, options,
                                                                       pool.num_threads());

    std::vector<std::future<void>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&c, op, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           auto it = std::begin(c);
                                           std::advance(it, offset);
                                           for (size_t i = offset; i < chunk_end; ++i, ++it)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               op(i, *it);
                                             }
                                         }));

        offset = chunk_end;
      }

    for (auto & f: futures)
      f.get();
  }

  /** \overload
   *
   *  Parallel enumerate_for_each for const containers.
   *
   *  @tparam Container Container type.
   *  @tparam Op Operation type: void(size_t index, const T&).
   *  @param pool Thread pool to use.
   *  @param c Container.
   *  @param op Function: void(size_t index, const T& element).
   *  @param chunk_size Elements per chunk (0 = auto).
   *  @see penumerate_for_each(ThreadPool&, Container&, Op, size_t) For the non-const version.
   */
  template <typename Container, typename Op>
  void penumerate_for_each(ThreadPool & pool, const Container & c, Op op,
                           size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    penumerate_for_each(c, op, options);
  }

  template <typename Container, typename Op>
  void penumerate_for_each(const Container & c, Op op,
                           const ParallelOptions & options = {})
  {
    const size_t n = std::distance(std::begin(c), std::end(c));
    if (n == 0)
      return;

    auto & pool = parallel_detail::selected_parallel_pool(options);
    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        auto it = std::begin(c);
        for (size_t i = 0; i < n; ++i, ++it)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            op(i, *it);
          }
        return;
      }

    size_t chunk_size = parallel_detail::effective_parallel_chunk_size(n, pool, options,
                                                                       pool.num_threads());

    auto data_holder = parallel_detail::ensure_random_access(c);
    const auto & data = parallel_detail::deref(data_holder);

    std::vector<std::future<void>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&data, op, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           auto it = std::begin(data);
                                           std::advance(it, offset);
                                           for (size_t i = offset; i < chunk_end; ++i, ++it)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               op(i, *it);
                                             }
                                         }));

        offset = chunk_end;
      }

    for (auto & f: futures)
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

      @ingroup Algorithms
  */
  template <typename Container, typename Op>
  [[nodiscard]] auto penumerate_maps(ThreadPool & pool, const Container & c, Op op,
                                     size_t chunk_size = 0)
  {
    ParallelOptions options;
    options.pool = &pool;
    options.chunk_size = chunk_size;
    return penumerate_maps(c, op, options);
  }

  template <typename Container, typename Op>
  [[nodiscard]] auto penumerate_maps(const Container & c, Op op,
                                     const ParallelOptions & options = {})
  {
    using T = std::decay_t<decltype(*std::begin(c))>;
    using ResultT = std::invoke_result_t<Op, size_t, const T &>;

    const size_t n = std::distance(std::begin(c), std::end(c));
    if (n == 0)
      return std::vector<ResultT>{};

    auto & pool = parallel_detail::selected_parallel_pool(options);
    if (parallel_detail::use_sequential_parallel_path(n, pool, options))
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        std::vector<ResultT> result;
        result.reserve(n);
        auto it = std::begin(c);
        for (size_t i = 0; i < n; ++i, ++it)
          {
            parallel_detail::throw_if_parallel_canceled(options.cancel_token);
            result.push_back(op(i, *it));
          }
        return result;
      }

    size_t chunk_size = parallel_detail::effective_parallel_chunk_size(n, pool, options,
                                                                       pool.num_threads());

    auto data_holder = parallel_detail::ensure_random_access(c);
    const auto & data = parallel_detail::deref(data_holder);

    std::vector<ResultT> result(n);
    std::vector<std::future<void>> futures;

    size_t offset = 0;
    while (offset < n)
      {
        parallel_detail::throw_if_parallel_canceled(options.cancel_token);
        size_t chunk_end = std::min(offset + chunk_size, n);

        futures.push_back(pool.enqueue([&result, &data, op, offset, chunk_end,
                                         token = options.cancel_token]()
                                         {
                                           auto it = std::begin(data);
                                           std::advance(it, offset);
                                           for (size_t i = offset; i < chunk_end; ++i, ++it)
                                             {
                                               parallel_detail::throw_if_parallel_canceled(token);
                                               result[i] = op(i, *it);
                                             }
                                         }));

        offset = chunk_end;
      }

    for (auto & f: futures)
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
  inline ThreadPool &parallel_default_pool()
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
