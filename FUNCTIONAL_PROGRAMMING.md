\defgroup FunctionalProgramming Functional Programming
\brief Functional-style utilities (map/filter/fold), zip utilities, ranges adapters, and parallel variants.

@{

This module covers two related layers:

1. **Header-level utilities** (zip views/iterators, unified functional helpers for STL/Aleph containers, ranges adapters, and parallel variants).
2. **Container methods**: many Aleph-w containers (trees, lists, arrays, bit-vectors, etc.) expose functional methods directly (e.g. `maps()`, `filter()`, `foldl()`) via CRTP mixins.

## Headers in this module

- \ref ah-zip_8H
- \ref ah-zip-utils_8H
- \ref ah-uni-functional_8H
- \ref ah-stl-functional_8H
- \ref ah-ranges_8H
- \ref ah-parallel_8H

## Container-level functional methods (via `FunctionalMixin`)

Many Aleph-w containers provide functional operations as **member functions** because they inherit from the CRTP mixin `FunctionalMixin` (see \ref ah-dry-mixin_8H).

In practice, this means that if a container provides a suitable `traverse()` method (directly or via `TraverseMixin`), it automatically gets methods such as:

- `for_each(...)`
- `maps<NewT>(...)`
- `filter(...)`
- `foldl(init, ...)`
- `all(...)`, `exists(...)`
- `partition(...)`, `take(n)`, `drop(n)`, `rev()`, `length()`

### Example: `DynSetTree` supports `maps()` and `foldl()`

See \ref tpl__dynSetTree_8H.

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

### Example: `BitArray` is also a functional container

`BitArray` inherits from `FunctionalMixin`, so it also supports the same family of member functions.
See \ref bitArray_8H.

## Graph functional helpers

Graphs expose functional-style helpers, but they are generally **specialized to node/arc traversal** rather than being treated as a simple sequence container.

Common patterns include:

- Node/arc iteration helpers (e.g. `for_each_node`, `for_each_arc`).
- Predicates and counting helpers over nodes/arcs (e.g. `none_node`, `count_nodes`, etc.).
- Mapping utilities for node/arc information (e.g. `nodes_map`, `arcs_map`).

See \ref tpl__graph_8H and \ref graph-dry_8H.

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
