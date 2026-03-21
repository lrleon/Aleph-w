# C++20 Ranges with Aleph-w Containers

## Introduction

Starting from C++20, the standard library includes **Ranges**, a modern and composable way to work with data sequences. Aleph-w containers are fully compatible with this functionality, allowing the use of views, adapters, and range algorithms.

## Requirements

- Compiler with C++20 support (GCC 10+, Clang 13+, MSVC 19.29+)
- Compilation flag: `-std=c++20`

```cpp
#include <ranges>
#include <ah-ranges.H>  // Aleph adapters
```

## Compatible Containers

All major Aleph-w containers satisfy `std::ranges::range`:

| Category | Containers |
|-----------|--------------|
| **Lists** | `DynList<T>`, `DynDlist<T>` |
| **Arrays** | `DynArray<T>` |
| **Stacks** | `DynListStack<T>`, `ArrayStack<T>` |
| **Queues** | `DynListQueue<T>`, `ArrayQueue<T>` |
| **Trees** | `DynSetRbTree<T>`, `DynSetAvlTree<T>`, `DynSetSplayTree<T>`, `DynSetTreap<T>` |
| **Hash** | `DynSetLhash<T>`, `ODhashTable<T>`, `OLhashTable<T>` |
| **Heaps** | `DynBinHeap<T>` |
| **Other** | `Random_Set<T>` |

## Basic Usage: Range-Based For

All containers support range-based for syntax:

```cpp
#include <htlist.H>
#include <tpl_dynSetTree.H>

using namespace Aleph;

DynList<int> list;
list.append(1);
list.append(2);
list.append(3);

// Range-based for loop
for (auto x : list) 
    std::cout << x << " ";  // Output: 1 2 3

// With trees (sorted order)
DynSetRbTree<int> set;
set.insert(5);
set.insert(2);
set.insert(8);

for (auto x : set)
    std::cout << x << " ";  // Output: 2 5 8 (sorted)
```

## Standard Views (std::views)

Aleph containers can be used with all C++20 views:

### filter - Filter elements

```cpp
DynArray<int> arr = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// Only even numbers
for (auto x : arr | std::views::filter([](int x) { return x % 2 == 0; }))
    std::cout << x << " ";  // Output: 2 4 6 8 10
```

### transform - Transform elements

```cpp
DynList<int> nums;
for (int i = 1; i <= 5; i++) nums.append(i);

// Squares
for (auto x : nums | std::views::transform([](int x) { return x * x; }))
    std::cout << x << " ";  // Output: 1 4 9 16 25
```

### take / drop - Limit elements

```cpp
DynArray<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// First 3
for (auto x : data | std::views::take(3))
    std::cout << x << " ";  // Output: 1 2 3

// Skip first 5
for (auto x : data | std::views::drop(5))
    std::cout << x << " ";  // Output: 6 7 8 9 10
```

### View Composition (Pipelines)

Views can be chained with the `|` operator:

```cpp
auto result = data 
    | std::views::filter([](int x) { return x % 2 == 0; })  // Only even
    | std::views::transform([](int x) { return x * 2; })    // Double
    | std::views::take(3);                                   // Take 3

for (auto x : result)
    std::cout << x << " ";  // Output: 4 8 12
```

## Range Generation

### iota - Sequence of numbers

```cpp
// Numbers from 1 to 10
for (auto x : std::views::iota(1, 11))
    std::cout << x << " ";  // Output: 1 2 3 4 5 6 7 8 9 10

// Infinite (use with take)
for (auto x : std::views::iota(1) | std::views::take(5))
    std::cout << x << " ";  // Output: 1 2 3 4 5
```

## Conversion to Aleph Containers

The `ah-ranges.H` library provides pipe adapters to convert ranges to Aleph containers:

### Available Pipe Adapters

| Adapter | Target Container |
|---------|-------------------|
| `to_dynlist_v` | `DynList<T>` |
| `to_dynarray_v` | `DynArray<T>` |
| `to_dyndlist_v` | `DynDlist<T>` |
| `to_dynliststack_v` | `DynListStack<T>` |
| `to_arraystack_v` | `ArrayStack<T>` |
| `to_dynlistqueue_v` | `DynListQueue<T>` |
| `to_arrayqueue_v` | `ArrayQueue<T>` |
| `to_randomset_v` | `Random_Set<T>` |

### Conversion Examples

```cpp
#include <ah-ranges.H>
using namespace Aleph;

// Range to DynList
auto list = std::views::iota(1, 11) | to_dynlist_v;
// list contains: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10

// Complete pipeline to DynArray
auto squares = std::views::iota(1, 20)
    | std::views::filter([](int x) { return x % 2 == 0; })
    | std::views::transform([](int x) { return x * x; })
    | to_dynarray_v;
// squares contains: 4, 16, 36, 64, 100, 144, 196, 256, 324
```

### Generic Conversion: to<Container>()

For containers with more complex templates, use `to<Container>()`:

```cpp
// To a sorted tree
auto set = std::views::iota(1, 100) 
    | std::views::filter([](int x) { return x % 7 == 0; })
    | to<DynSetRbTree<int>>();

// To a queue
auto queue = std::views::iota(1, 6) | to<DynListQueue<int>>();
```

## STL Algorithms with Iterators

Aleph iterators are compatible with traditional STL algorithms:

```cpp
DynList<int> nums;
for (int i : {3, 1, 4, 1, 5, 9, 2, 6})
    nums.append(i);

// min/max with explicit iterators
auto min_it = std::min_element(nums.begin(), nums.end());
auto max_it = std::max_element(nums.begin(), nums.end());
std::cout << "Min: " << *min_it << ", Max: " << *max_it << std::endl;

// find
auto it = std::find(nums.begin(), nums.end(), 5);
if (it != nums.end())
    std::cout << "Found: " << *it << std::endl;

// count_if
auto count = std::count_if(nums.begin(), nums.end(), 
                           [](int x) { return x > 3; });
std::cout << "Count > 3: " << count << std::endl;
```

The same queries also work with `std::ranges::`:

```cpp
#include <ranges>

auto min_r = std::ranges::min_element(nums);
auto max_r = std::ranges::max_element(nums);
auto found_r = std::ranges::find(nums, 5);
auto count_r = std::ranges::count_if(nums, [](int x) { return x > 3; });
```

## Functions from ah-ranges.H

### Lazy Generation

```cpp
#include <ah-ranges.H>
using namespace Aleph;

// Infinite range of numbers
auto naturals = lazy_iota(1);  // 1, 2, 3, ...

// First 10
auto first10 = naturals | std::views::take(10) | to_dynlist_v;

// Infinite range; filter or transform to get specific patterns
auto even_nums = lazy_iota(0)
               | std::views::filter([](int x) { return x % 2 == 0; });  // 0, 2, 4, 6, ...
```

## Complete Example

```cpp
#include <ranges>
#include <iostream>
#include <htlist.H>
#include <tpl_dynArray.H>
#include <ah-ranges.H>

using namespace Aleph;

int main() {
    // Create test data
    DynArray<int> data;
    for (int i = 1; i <= 20; i++)
        data.append(i);
    
    // Pipeline: filter, transform, limit
    auto processed = data
        | std::views::filter([](int x) { return x % 3 == 0; })  // Multiples of 3
        | std::views::transform([](int x) { return x * x; })    // Squares
        | std::views::take(4);                                   // Only 4
    
    std::cout << "Result: ";
    for (auto x : processed)
        std::cout << x << " ";  // Output: 9 36 81 144
    std::cout << std::endl;
    
    // Convert result to DynList
    auto result_list = processed | to_dynlist_v;
    std::cout << "Size: " << result_list.size() << std::endl;
    
    // Generate from scratch
    auto fibonacci_like = std::views::iota(1, 10)
        | std::views::transform([](int n) {
            int a = 0, b = 1;
            for (int i = 0; i < n; i++) {
                int temp = a + b;
                a = b;
                b = temp;
            }
            return a;
        })
        | to_dynarray_v;
    
    std::cout << "Fibonacci-like: ";
    for (auto x : fibonacci_like)
        std::cout << x << " ";  // Output: 1 1 2 3 5 8 13 21 34
    std::cout << std::endl;
    
    return 0;
}
```

## Known Limitations

1. **Optional wrappers**: Aleph-w containers covered by this module's tests work with `std::ranges::` algorithms like `std::ranges::min_element` and `std::ranges::count_if`. If you prefer a uniform API along with the rest of `ah-ranges.H` utilities, you can also use the `detail::ranges_min()` and `detail::ranges_count_if()` wrappers.

2. **Non-sequential containers**: Hash tables and heaps iterate in non-deterministic order.

3. **Lazy evaluation**: Views are lazy; don't materialize large ranges unnecessarily.

## Compatibility with Existing Aleph Functions

Functions from `ahFunctional.H` continue to work and use ranges internally when possible:

```cpp
DynList<int> nums = {1, 2, 3, 4, 5};

// Aleph functional API (optimized with ranges internally)
auto sum = nums.foldl(0, [](int acc, int x) { return acc + x; });
auto doubled = nums.maps<int>([](int x) { return x * 2; });
auto evens = nums.filter([](int x) { return x % 2 == 0; });
```

## Internal Functions (namespace detail)

For internal optimization, `ah-ranges.H` provides wrappers over `std::ranges`:

```cpp
#include <ah-ranges.H>
using namespace Aleph;

std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// Predicates
bool all_pos = detail::ranges_all_of(vec, [](int x) { return x > 0; });
bool has_even = detail::ranges_any_of(vec, [](int x) { return x % 2 == 0; });
bool no_neg = detail::ranges_none_of(vec, [](int x) { return x < 0; });

// Search
auto it = detail::ranges_find_if(vec, [](int x) { return x > 5; });
auto count = detail::ranges_count_if(vec, [](int x) { return x % 2 == 0; });

// Transformation (lazy)
auto doubled = detail::ranges_transform(vec, [](int x) { return x * 2; });
auto evens = detail::ranges_filter(vec, [](int x) { return x % 2 == 0; });

// Slicing (lazy)
auto first3 = detail::ranges_take(vec, 3);
auto skip2 = detail::ranges_drop(vec, 2);
auto rev = detail::ranges_reverse(vec);

// Flattening
std::vector<std::vector<int>> nested = {{1, 2}, {3}, {4, 5, 6}};
auto flat = detail::ranges_flatten(nested);

// Reduction
auto sum = detail::ranges_fold_left(vec, 0, std::plus<>{});
auto product = detail::ranges_fold_left(vec, 1, std::multiplies<>{});

// Convenience
auto total = detail::ranges_sum(vec);      // 55
auto prod = detail::ranges_product(vec);   // 3628800

// Min/Max
auto min_it = detail::ranges_min(vec);
auto max_it = detail::ranges_max(vec);

// Sort (in-place)
detail::ranges_sort(vec);                   // Ascending
detail::ranges_sort(vec, std::greater<>{}); // Descending
```

## collect<Container>() Function

To materialize ranges in any Aleph container:

```cpp
// Works with append(), insert(), or push()
auto list = collect<DynList<int>>(std::views::iota(1, 10));
auto set = collect<DynSetRbTree<int>>(std::views::iota(1, 10));
auto stack = collect<DynListStack<int>>(std::views::iota(1, 10));
```

## Tests

Comprehensive tests are in `Tests/ah_ranges_test.cc`:

```bash
cd Tests/build
cmake --build . --target ah_ranges_test
./ah_ranges_test
```

### Test Coverage

| Category | Tests |
|-----------|-------|
| Feature Detection | 2 |
| Pipe Adaptors | 14 |
| Generic to<>() | 3 |
| detail::ranges_* | 24 |
| Lazy Ranges | 5 |
| Concepts | 2 |
| Edge Cases | 9 |
| String Types | 2 |
| Stress Tests | 5 |
| Complex Types | 2 |
| Aleph Container Iteration | 3 |
| Chained Operations | 3 |
| Collect Function | 4 |
| Aleph + std::ranges | 2 |
| StdRanges sanity | 1 |

## Compilation

```bash
g++ -std=c++20 -I/path/to/Aleph-w my_program.cc -o my_program
```

---

**Version**: Aleph-w with C++20 Ranges  
**Date**: 2026  
**Tests**: see `Tests/ah_ranges_test.cc` and CI for current total
