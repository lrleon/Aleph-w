# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Test Commands

```bash
# Configure and build (Ninja recommended)
cmake -S . -B build -G Ninja
cmake --build build

# Build a single test target
cmake --build build --target geom_algorithms_test

# Run all tests
ctest --test-dir build --output-on-failure

# Run tests matching a pattern (CTest regex)
ctest --test-dir build -R dynlist --output-on-failure

# Run specific GTest cases within a binary
./build/Tests/geom_algorithms_test --gtest_filter="*ShortestPath*"

# Build with presets
cmake --preset gcc-debug && cmake --build --preset gcc-debug

# Build with sanitizers
cmake -S . -B build-san -G Ninja -DUSE_SANITIZERS=ON
cmake --build build-san
```

Tests live in `Tests/*.cc`. Each `.cc` produces `build/Tests/<name>`. New test files are auto-registered by CMake.

Required system libraries: `libgmp-dev libmpfr-dev libgsl-dev libx11-dev`. GoogleTest is auto-fetched if not installed.

## Architecture

**Aleph-w** is a C++20 header-dominant library (MIT license, 2002-2026 Leandro Rabindranath Leon). All code is in `namespace Aleph`. Headers are flat in the root directory (no `include/` subdirectory).

### Header naming
- `tpl_<name>.H` — template data structure/algorithm implementations
- `ah-<name>.H` / `ah<Name>.H` — utilities, mixins, support headers
- `<Algorithm>.H` — named algorithm headers (Dijkstra.H, Kruskal.H, etc.)

### Template pattern: `Gen_<Name>` + typedef
```cpp
template <typename Key, class Compare = Aleph::less<Key>>
class Gen_AVL_Tree { ... };
using AVL_Tree = Gen_AVL_Tree<int>;  // convenient default
```

### Iterator convention (not STL begin/end)
```cpp
for (DynList<int>::Iterator it(list); it.has_curr(); it.next())
  process(it.get_curr());
// Or functional: list.for_each([](int x) { ... });
```
STL range-for is available on containers that include the `StlAlephIterator` mixin.

### CRTP mixins (ah-dry.H)
Functionality is injected via CRTP: `FunctionalMethods` (for_each/map/filter/fold), `LocateFunctions`, `GenericKeys`, `EqualToMethod`, `StlAlephIterator`.

### Key macros
- `Special_Ctors(SetType, ElemType)` (ahDry.H) — generates constructors from lists/iterators/initializer_list
- `Args_Ctor(Name, Type)` (ah-args-ctor.H) — variadic element constructors

## Coding Conventions

### Style
- 2-space indentation, K&R braces
- `# include`, `# ifndef`, `# define` — space after `#`
- Header guards: `# ifndef TPL_<NAME>_H`
- Use `[[nodiscard]]`, `constexpr`, `noexcept` liberally
- Prefer `and`, `or`, `not` over `&&`, `||`, `!`

### Error handling — ALWAYS use Aleph macros (ah-errors.H), NEVER raw `throw`
```cpp
ah_out_of_range_error_if(i >= size) << "Index " << i << " out of range";
ah_domain_error_if(x < 0) << "Negative value";
ah_runtime_error_unless(ok) << "Failed";
```

### Container usage — ALWAYS prefer Aleph containers over STL
| Instead of | Use |
|---|---|
| `std::vector<T>` | `Array<T>` (tpl_array.H) or `DynList<T>` (htlist.H) |
| `std::list<T>` | `DynList<T>` |
| `std::set<T>` | `DynSetTree<T>` (tpl_dynSetTree.H) |
| `std::map<K,V>` | `DynMapTree<K,V>` (tpl_dynMapTree.H) |
| `std::unordered_map` | `OLhashTable<Key,Data>` (tpl_olhash.H) |
| `std::queue<T>` | `ArrayQueue<T>` or `DynListQueue<T>` |
| `std::stack<T>` | `ArrayStack<T>` or `DynListStack<T>` |
| `std::priority_queue` | `BinHeap<T>` or `ArrayHeap<T>` |

STL containers are only acceptable when no Aleph equivalent exists, or interfacing with external libraries that require STL types.

### Functors
Use `Aleph::less<T>`, `Aleph::plus<T>`, etc. from `ahFunction.H` instead of `std::less<T>`.

## Critical Technical Notes

**`-fno-strict-aliasing` is REQUIRED in Release builds.** `htlist.H` uses `reinterpret_cast` that violates strict aliasing. GCC generates incorrect code for `DynList`, `DynListStack`, `DynListQueue` without this flag. Do NOT remove it until `htlist.H` is refactored with CRTP.

## Testing
- Framework: Google Test
- Test files: `Tests/<name>.cc`
- Fixture pattern: inherit `::testing::Test`
- Use `EXPECT_*` (non-fatal) / `ASSERT_*` (fatal)
- All test files use `using namespace Aleph;`

## Documentation
Public APIs require Doxygen: `@brief`, `@param`, `@return`, `@throws`, complexity in `@note`. All `.H` files must have the MIT license header and `@file` directive.

# git usage

- Never does a commit directly
- gh is at /snap/bin/gh

# Auxiliary scripts

- Utils scripts in `scripts/` and written in Ruby (not bash, Python, etc.) for better string handling and JSON support.
