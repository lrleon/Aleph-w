\defgroup FunctionalProgramming Functional Programming
\brief Functional-style utilities (map/filter/fold), zip utilities, ranges adapters, and parallel variants.

@{

This module covers two related layers:

1. **Header-level utilities** (zip views/iterators, unified functional helpers for STL/Aleph containers, ranges adapters, and parallel variants).
2. **Container methods**: many Aleph-w containers (trees, lists, arrays, bit-vectors, etc.) expose functional methods directly (e.g. `maps()`, `filter()`, `foldl()`) via CRTP mixins.

## Headers in this module

- \ref ah-zip.H
- \ref ah-zip-utils.H
- \ref ah-uni-functional.H
- \ref ah-stl-functional.H
- \ref ah-ranges.H
- \ref ah-parallel.H

## Container-level functional methods (via `FunctionalMixin`)

Many Aleph-w containers provide functional operations as **member functions** because they inherit from the CRTP mixin `FunctionalMixin` (see \ref ah-dry-mixin.H).

In practice, this means that if a container provides a suitable `traverse()` method (directly or via `TraverseMixin`), it automatically gets methods such as:

- `for_each(...)`
- `maps<NewT>(...)`
- `filter(...)`
- `foldl(init, ...)`
- `all(...)`, `exists(...)`
- `partition(...)`, `take(n)`, `drop(n)`, `rev()`, `length()`

### Example: `DynSetTree` supports `maps()` and `foldl()`

See \ref tpl_dynSetTree.H.

@code
#include <tpl_dynSetTree.H>
#include <iostream>

using namespace Aleph;

int main()
{
  DynSetTree<int, Avl_Tree> s;
  for (int x : {1, 2, 3, 4, 5})
    s.insert(x);

  auto doubled = s.maps<int>([](int x) { return x * 2; });
  int sum = s.foldl(0, [](int acc, int x) { return acc + x; });

  (void) doubled;
  std::cout << "sum=" << sum << "\n";
}
@endcode

### Building a container from its items: `C::build()`

The containers that inherit the CRTP mixin `FunctionalMethods` (see \ref ah-dry.H): the lists, arrays, stacks, queues, heaps, sets and maps, provide a static `build()` that builds a container from its items:

@code
#include <htlist.H>
#include <tpl_arrayStack.H>
#include <tpl_dynMapTree.H>
#include <tpl_dynSetTree.H>
#include <memory>
#include <string>

using namespace Aleph;

int main()
{
  auto list  = DynList<int>::build(3, 1, 2);        // 3 1 2
  auto set   = DynSetTree<int>::build(3, 1, 2, 1);  // 1 2 3: one item per key
  auto stack = ArrayStack<int>::build(1, 2, 3);     // 3 on top
  auto map   = DynMapTree<int, std::string>::build(std::pair{1, "one"},
                                                   std::pair{2, "two"});
  auto ptrs  = DynList<std::unique_ptr<int>>::build(std::make_unique<int>(1));
}
@endcode

`C::build(a, b, c)` builds the same container as `C{a, b, c}`: each item goes, in argument order, through the `append()` of the container, so its rules apply. A stack ends with the last item on top, a queue with the first item at its front, and a set or a map keeps the first of two equal keys. Unlike the braces, the items are forwarded: an rvalue is moved, so move-only types can be stored, and an item may have any type implicitly convertible to the item type. Being a named function rather than a constructor, `Array<int>::build(10)` is always the one-element array `{10}`; a capacity is reserved with `Array<int>::create_reserved(10)`.

A class derived from a container (`DynSetAvlTree`, `DynMapRbTree`, `MapOLhash`, ...) builds itself rather than its base. A class of your own that derives from a container declares that `build()` with the macro `Derived_Build(Name)`. The free function `build_container<C>(items...)` remains for any type with `append()`.

### Example: `BitArray` is also a functional container

`BitArray` inherits from `FunctionalMixin`, so it also supports the same family of member functions.
See \ref bitArray.H.

## Graph functional helpers

Graphs expose functional-style helpers, but they are generally **specialized to node/arc traversal** rather than being treated as a simple sequence container.

Common patterns include:

- Node/arc iteration helpers (e.g. `for_each_node`, `for_each_arc`).
- Predicates and counting helpers over nodes/arcs (e.g. `none_node`, `count_nodes`, etc.).
- Mapping utilities for node/arc information (e.g. `nodes_map`, `arcs_map`).

See \ref tpl_graph.H and \ref graph-dry.H.

@code
#include <tpl_graph.H>

using namespace Aleph;

int main()
{
  using Node = Graph_Node<int>;
  using Arc  = Graph_Arc<double>;
  using G    = List_Graph<Node, Arc>;

  G g;
  auto * a = g.insert_node(1);
  auto * b = g.insert_node(2);
  g.insert_arc(a, b, 1.5);

  g.for_each_node([](auto * p) { (void) p; });
  const auto n_gt_1 = g.count_nodes([](auto * p) { return p->get_info() > 1; });

  (void) n_gt_1;
}
@endcode

@}
