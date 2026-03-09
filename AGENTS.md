# Repository Guidelines

## Project Structure & Module Layout
- Top-level CMake project builds Aleph library plus `Examples/` and `Tests/`.
- Core headers in repo root (`tikzgeom.H`, `tikzgeom_algorithms.H`, `geom_algorithms.H`) drive both rendering helpers and algorithms.
- `Tests/` holds GoogleTest suites (e.g., `tikzgeom_algorithms_test.cc`), while `Examples/` hosts TikZ export demos; update lists in `Examples/CMakeLists.txt`.
- Assets: generated `.tex` lives in `Examples/` outputs; keep source edits in `Examples/*.cc`.

## Build, Test, and Development Commands
- `cmake -S . -B build-test-clean` configures an out-of-source build for tests/examples using current compiler flags.
- `cmake --build build-test-clean -jN` compiles Aleph, tests, and examples; reuse target names like `tikzgeom_algorithms_test` or `tikz_tree_structures_example`.
- `cd build-test-clean && ctest -R <regex>` runs selected GoogleTest suites.
- Example workflows: build + run `./build-test-clean/Tests/tikzgeom_algorithms_test` to verify TikZ helpers; run new examples and inspect generated `.tex` under `/tmp`.

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

### Aleph containers and algorithms

- Where Aleph can be used, Aleph should be used instead of the Standard Library (STL). For example, 'Aleph::Array' is preferred over 'std::vector', or DynSetTree over std::set.
### Style
- 2-space indentation, K&R braces
- `# include`, `# ifndef`, `# define` — space after `#`
- Header guards: `# ifndef TPL_<NAME>_H`
- Use `[[nodiscard]]`, `constexpr`, `noexcept` liberally
- Prefer `and`, `or`, `not` over `&&`, `||`, `!`
- Comments and documentation should be in English for broader accessibility, even if the conversation is in Spanish.


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

## Testing Guidelines
- GoogleTest is used across `Tests/`. Tests follow `TEST`/`TEST_F` naming (e.g., `TikzGeomAlgorithmsTest.*`).
- Coverage expectation: important features (visual helpers, snapshot exports) should have focused tests and command-line checks (e.g., `--gtest_filter`).
- Always run `cmake --build` + target + `ctest` for changed suites; new functionality should include direct binary tests when global `ctest` is flaky.

## Commit & Pull Request Guidelines
-  Never do a commit
- gh is at /snap/bin/gh

# Auxiliary scripts

- Utils scripts in `scripts/` and written in Ruby (not bash, Python, etc.) for better string handling and JSON support.
