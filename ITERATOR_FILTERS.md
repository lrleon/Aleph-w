\defgroup IteratorFilters Iterator Filters
\brief Filtering adaptors for Aleph iterators, including graph arc/node filters.

@{

This module documents iterator-level filtering in Aleph-w.

Aleph-w provides filtering in two complementary ways:

1. **Generic filtered iterators** using \ref filter__iterator_8H (\ref Aleph::Filter_Iterator)
2. **Graph-specific filtered iterators** (nodes/arcs) in \ref tpl__graph_8H, built on top of \ref Aleph::Filter_Iterator

## 1) Generic `Filter_Iterator`

\ref Aleph::Filter_Iterator wraps an Aleph iterator and hides elements that do not satisfy a predicate.

Key points:
- The predicate type is the template parameter `Show_Item`.
- The wrapper preserves the usual Aleph iterator interface (`has_curr()`, `get_curr()`, `next()` / `next_ne()`, etc.).
- You can attach context via `set_cookie()` / `get_cookie()` if needed.

@code
#include <filter_iterator.H>
#include <tpl_dynDlist.H>

using namespace Aleph;

struct ShowEven {
  bool operator()(int x) const { return x % 2 == 0; }
};

int main()
{
  DynDlist<int> xs;
  for (int i = 1; i <= 10; ++i)
    xs.append(i);

  Filter_Iterator<DynDlist<int>, DynDlist<int>::Iterator, ShowEven> it(xs);

  for (; it.has_curr(); it.next())
    {
      auto x = it.get_curr();
      (void) x;
    }
}
@endcode

## 2) Graph arc/node filters (practical example)

Graph traversal in Aleph-w often exposes a filter hook via a functor parameter (commonly called `Show_Arc` or `Show_Node`).
This is used throughout \ref tpl__graph_8H and shortest-path algorithms like \ref Dijkstra_8H.

### Example: exclude "highways" from shortest-path

If your arc payload indicates whether an arc represents a highway, you can provide a `Show_Arc` filter so that Dijkstra ignores those arcs.

@code
#include <tpl_graph.H>
#include <Dijkstra.H>

using namespace Aleph;

struct RoadInfo {
  double distance;
  bool is_highway;
};

using G = List_Graph<Graph_Node<int>, Graph_Arc<RoadInfo>>;

struct RoadDistance {
  using Distance_Type = double;
  double operator()(G::Arc * a) const noexcept { return a->get_info().distance; }
};

struct NoHighways {
  bool operator()(G::Arc * a) const noexcept { return !a->get_info().is_highway; }
};

int main()
{
  G g;
  auto * s = g.insert_node(0);
  auto * t = g.insert_node(1);

  g.insert_arc(s, t, RoadInfo{10.0, true});   // highway
  g.insert_arc(s, t, RoadInfo{12.0, false});  // regular road

  Dijkstra_Min_Paths<G, RoadDistance, Node_Arc_Iterator, NoHighways> dij(RoadDistance{}, NoHighways{});

  Path<G> path(g);
  auto d = dij(g, s, t, path);
  (void) d;
}
@endcode

Notes:
- The filter functor (`NoHighways`) is the `SA` template parameter in `Dijkstra_Min_Paths`.
- The distance functor (`RoadDistance`) defines how arc weights are read.

@}
