# Link-Cut Tree In Aleph-w

This document is architecture analysis only. It does not propose code yet.

## Goal

A Link-Cut Tree (LCT) is a dynamic forest structure for rooted or rerootable
trees. Its natural use cases in Aleph-w would be:

- `link(u, v)` to connect two different trees
- `cut(u, v)` or `cut(x)` to detach an existing tree edge
- `make_root(x)` / `evert(x)` to reroot a represented tree
- `find_root(x)` and `connected(u, v)`
- `lca(u, v)` on the represented forest
- path queries and updates such as min, max, sum, xor, edge replacement, etc.

This is a dynamic-tree structure, not a general dynamic-graph structure. If the
user wants to manage a graph with cycles, the LCT should manage only the current
spanning forest, while non-tree edges remain outside the core structure.

## Relevant Aleph-w Context

### `Tree_Node<T>`

`tpl_tree_node.H` implements a general tree using the classic first-child /
next-sibling representation. This is excellent for:

- static or materialized rooted trees
- recursive traversals
- visualization
- exported tree views

It is not designed as a self-adjusting path structure.

Important observations from the current implementation:

- A node stores `child` and `sibling` links, not binary splay links.
- Parent access is derived through sibling navigation instead of a direct
  O(1) parent field.
- The representation encodes the actual tree topology, not an auxiliary
  search tree topology.
- Flags such as `is_root`, `is_leaf`, `is_leftmost`, and `is_rightmost` are
  tied to general-tree semantics, not to splay auxiliary-tree semantics.

### Aleph graphs

`tpl_graph.H`, `graph-dry.H`, and `aleph-graph.H` show that graph nodes and
arcs are pointer-stable objects with:

- control bits
- counters
- `void *` cookies

This is useful for temporary mappings and algorithm state, and Aleph already
uses that heavily.

More importantly for LCT integration, the common graph layer already exposes a
uniform operational surface through `GraphCommon`, including:

- `Node` and `Arc` pointer types
- `get_src_node()`, `get_tgt_node()`, and `get_connected_node()`
- graph/node/arc cookies
- node/arc iterators and `Node_Arc_Iterator`
- `insert_arc()`, `remove_arc()`, `remove_node()`

That suggests the graph adapter should target the `GraphCommon`-style API, not
one concrete graph storage.

### `List_Graph`, `List_SGraph`, and `Array_Graph`

Aleph does not have only one graph representation.

Relevant families in the repository are:

- `List_Graph` in `tpl_graph.H`
- `List_SGraph` in `tpl_sgraph.H`
- `Array_Graph` in `tpl_agraph.H`
- their directed wrappers `List_Digraph`, `List_SDigraph`, and `Array_Digraph`

This matters because an LCT graph adapter should avoid assumptions tied to one
storage layout.

#### What they have in common

They all reuse the same conceptual graph API:

- stable node and arc objects addressed by pointer
- common node/arc metadata via `GTNodeCommon` / `GTArcCommon`
- adjacency traversal through node-local iterators
- graph-level insertion and deletion operations

That commonality is strong enough to support a generic adapter template.

#### What differs and matters

`List_Graph`:

- uses doubly-linked adjacency machinery
- is the most feature-rich and the de facto baseline graph representation

`List_SGraph`:

- uses singly-linked adjacency lists
- has lighter adjacency storage
- still provides compatible endpoint semantics through its iterators

`Array_Graph`:

- stores adjacency in dynamic arrays per node
- may reallocate and compact adjacency arrays
- removes incident arcs by swap-with-last style compaction inside node storage

For the LCT adapter, that means:

- node and arc pointers are the safe identity
- adjacency order must not matter
- references into adjacency containers must not be cached
- only `GT::Node*` and `GT::Arc*` should be stored durably

### Simplicity, multiarcs, and self-loops

`single_graph.H` is a reminder that Aleph graph families are not implicitly
"simple graph only" structures.

This is a subtle but important integration point:

- the graph may contain parallel arcs
- the graph may contain self-loops
- simplicity is something to test, not something guaranteed by the type

For an LCT-backed forest adapter, that implies:

- self-loops must be rejected as tree edges
- a `link(u, v)` operation must reject `u == v`
- parallel graph arcs may exist globally, but at most one of them can be chosen
  as the current tree edge between two represented components
- the adapter must distinguish "graph accepts this arc" from "forest accepts
  this arc as a tree edge"

This distinction becomes essential if the LCT is later used inside dynamic MST
or dynamic connectivity workflows.

### Cookie lifecycle

`cookie_guard.H` exists precisely because cookies are shared scratch space.
That is important for LCT integration: a persistent data structure should not
occupy graph cookies casually, because many other graph algorithms do.

### Existing splay tree

`tpl_splay_tree.H` provides a top-down splay tree specialized for binary search
tree operations driven by key comparisons.

That is related conceptually, but it is not the right abstraction to drop into
an LCT unchanged.

## Can It Be Related To `Tree_Node`?

Yes, but only at the boundary.

The right relationship is:

- `Tree_Node` as a static export/import format
- LCT as the dynamic internal engine

The wrong relationship is:

- using `Tree_Node` itself as the internal node representation of the LCT
- deriving the LCT node directly from `Tree_Node`

### Why `Tree_Node` should not be the internal representation

An LCT maintains two different notions of structure:

1. The represented forest, which is the user-visible dynamic tree.
2. The auxiliary splay trees, which are ephemeral and change after almost every
   access operation.

`Tree_Node` represents only the first notion. It has no direct vocabulary for
the second.

More concretely:

- LCT needs binary auxiliary-tree links `left`, `right`, and `parent`.
- LCT usually needs a lazy `reversed` flag.
- Path aggregates require `pull()`/`push()` style maintenance.
- Access operations repeatedly rewrite preferred-path decomposition.
- The auxiliary structure is not an n-ary child/sibling tree.

Trying to force that into `Tree_Node` would mix two incompatible invariants:

- general-tree sibling order and child list invariants
- auxiliary splay-tree rotation invariants

That would make the implementation fragile and hard to reason about.

### The useful relation to `Tree_Node`

`Tree_Node` is still valuable for:

- exporting the current represented forest for visualization
- debugging snapshots
- importing a static rooted tree into an LCT during initialization
- interoperating with existing tree-only utilities in Aleph

So the relationship should be adapter-based, not inheritance-based.

## Can It Reuse The Existing Splay Tree?

Not directly.

The current `tpl_splay_tree.H` implementation is a search tree over ordered
keys. A Link-Cut Tree uses splaying for a different purpose:

- splay by node handle, not by search key
- no BST ordering invariant
- implicit in-order structure represents preferred paths
- lazy path reversal must be propagated
- path aggregate maintenance is required

The existing top-down BST splay code is therefore useful as:

- conceptual precedent
- style reference
- possible rotation inspiration

But not as the actual engine.

## Recommended Internal Representation

The cleanest design is a dedicated node type, independent from `Tree_Node` and
from the current BST node templates.

At minimum, each internal LCT node should conceptually carry:

- `left`, `right`, `parent` for the auxiliary splay tree
- a `reversed` lazy flag
- the local value stored at the represented vertex or edge
- the aggregate value of the auxiliary subtree
- optional lazy-update tags if path updates are supported
- a back-pointer to the original Aleph graph object when used as an adapter

### Vertex-only versus edge-as-node

This is the first major design decision.

#### Vertex-only model

Pros:

- simpler mental model
- smaller node count
- easier first implementation

Cons:

- awkward if path queries are really about edge weights
- harder to represent graph arc payload naturally

#### Edge-as-node model

Pros:

- much better fit for Aleph graphs, because graph arcs already carry data
- path min/max/sum over tree edges becomes natural
- dynamic MST style algorithms become more feasible later

Cons:

- more nodes internally
- slightly more complex `link` and `cut`

For Aleph graph integration, edge-as-node is the stronger long-term choice.

If the implementation is phased, the pragmatic order is:

1. start with vertex-only LCT core
2. extend to edge-as-node when graph adaptation is introduced

## Recommended API Shape

The implementation should start as a standalone dynamic-forest structure, not
as a graph algorithm.

The most coherent progression is:

1. standalone LCT core
2. graph adapter on top
3. `Tree_Node` import/export on top

That keeps the core testable and avoids graph-cookie coupling in the first
version.

Conceptually, the public operations should be centered around handles or node
pointers owned by the LCT itself:

- create vertex handle
- `connected(u, v)`
- `make_root(x)`
- `find_root(x)`
- `lca(u, v)`
- `link(u, v[, edge_payload])`
- `cut(u, v)` or `cut(edge_handle)`
- `path_query(u, v)`
- later: `path_update(u, v, delta)`

The API should state explicitly whether queries are:

- vertex-based
- edge-based
- or configurable by policy

Subtree queries should not be promised initially. Classical LCT handles path
queries naturally; subtree queries need extra machinery and are easy to specify
incorrectly.

## How It Can Be Used With Aleph Graphs

### Case 1: A forest graph as the primary source of truth

This is the simplest integration.

The graph is guaranteed to remain a forest. The LCT is a dynamic index over the
same structure.

Recommended mapping:

- each `GT::Node*` maps to one LCT vertex node
- each forest `GT::Arc*` maps to one LCT edge node if edge-as-node is used

Good use cases:

- dynamic rooted-tree path queries
- rerooting
- LCA on a mutable forest
- maintaining path min/max/sum on tree edges

### Case 2: A general graph with a maintained spanning forest

This is the more advanced, but very useful, graph-oriented mode.

The graph may contain cycles, but the LCT manages only the current spanning
forest. Non-tree arcs stay outside the LCT.

This is the right foundation for algorithms such as:

- dynamic connectivity over a maintained forest
- replacement-edge logic
- dynamic MST style strategies
- max-edge-on-path swap tests

In this model, the graph adapter needs explicit classification:

- tree arc
- non-tree arc

Only tree arcs participate in the LCT.

### Case 3: Importing a static spanning tree from Aleph algorithms

Aleph already has utilities that materialize spanning trees:

- DFS spanning tree
- BFS spanning tree
- graph-to-tree conversion helpers

Those are useful as producers of an initial tree, after which the LCT can take
over dynamic operations.

This is a good bridge with existing functionality, but it is not a substitute
for the dynamic structure itself.

## How To Map Aleph Graph Objects To LCT Nodes

The best long-lived strategy is external mapping, not persistent cookie use.

Recommended ownership/mapping model:

- store back-pointers from LCT nodes to `GT::Node*` / `GT::Arc*`
- keep `DynMapTree<GT::Node*, LCTNode*>` for vertices
- keep `DynMapTree<GT::Arc*, LCTNode*>` for tree arcs if edge-as-node is used

Why not use cookies permanently?

- Aleph graph algorithms reuse cookies constantly
- persistent occupation would make composition with other algorithms brittle
- nested graph algorithms would collide
- graph cookies are intentionally scratch-oriented, not durable foreign storage

### Where cookies still make sense

Cookies remain useful for:

- one-shot construction from a graph
- one-shot export back to a graph
- temporary batch mapping during initialization

If cookies are used temporarily, they should be wrapped by:

- `Cookie_Guard<GT>`
- or `Cookie_Saver<GT>`

That matches Aleph conventions and avoids accidental interference.

## Additional Aleph Graph Subtleties

### The adapter should be generic over graph family

Because `List_Graph`, `List_SGraph`, and `Array_Graph` all expose compatible
node/arc semantics through `GraphCommon`, the graph-facing LCT layer should
ideally be templated on `GT`, with assumptions limited to:

- `typename GT::Node`
- `typename GT::Arc`
- endpoint access methods
- insertion/removal methods
- standard Aleph iterators

It should not depend on:

- `Dlink` presence in nodes
- adjacency container internals
- arc ordering
- one specific graph storage class

This is especially important because `Graph_Snode` and `Graph_Anode` do not
share the same physical node layout beyond the common graph interface.

### Do not cache adjacency iterators or adjacency slots

This is particularly important for `Array_Graph`.

Its node adjacency arrays may:

- grow by reallocation
- shrink by contraction
- reorder entries on removal

So any LCT adapter or dynamic-tree algorithm that needs to scan graph edges
must treat adjacency iteration as ephemeral and recalculate it when needed.

Durable state must live in:

- external maps
- graph node pointers
- graph arc pointers
- the LCT nodes themselves

### Graph deletion policy must be explicit

All graph families own and delete nodes/arcs on removal.

That means the adapter cannot quietly tolerate arbitrary external mutation.
There are only three coherent policies:

1. The graph is immutable while attached to the LCT adapter.
2. All structural mutations must go through the adapter.
3. The adapter is one-shot and rebuilt after graph mutations.

For Aleph, option 2 is the strongest long-term design, but option 1 is the
best first implementation because it keeps invariants simpler.

### Undirected endpoint semantics are already abstracted

Both `List_SGraph` and `Array_Graph` expose `Node_Arc_Iterator::get_tgt_node()`
in terms of the node from which iteration starts. That is exactly the kind of
abstraction a forest adapter wants.

So for undirected graph support, the adapter can remain high-level and iterate
incident arcs without caring about low-level adjacency representation.

### Directed wrappers exist, but they should still be deferred

The existence of:

- `List_Digraph`
- `List_SDigraph`
- `Array_Digraph`

means the future API can be made family-complete. But that is not a reason to
promise digraph support in the first phase.

The right initial position remains:

- support undirected graph families first
- reject `g.is_digraph()` in the first graph adapter version
- later decide whether digraph support means "underlying undirected forest plus
  arc metadata" or a narrower arborescence-oriented adapter

## Directed Graphs Versus Undirected Graphs

The natural first target is undirected forests over `List_Graph`.

For `List_Digraph`, the semantics are not as clean:

- an LCT fundamentally models an undirected underlying forest with a chosen root
- a digraph encodes arc direction explicitly
- an arborescence is a possible special case, but not the general case

So the clean recommendation is:

- support undirected graphs first
- optionally support digraphs later only as rooted arborescence views
- keep the underlying LCT semantics undirected, with direction treated as
  metadata or adapter policy

## Recommended Relationship With `Tree_Node`

The best design is to define a strict separation:

### LCT core

- dynamic
- binary auxiliary splay trees
- path-oriented
- not directly visual

### `Tree_Node`

- static snapshot
- n-ary rooted tree
- recursive traversal and visualization friendly

### Adapter layer

Possible later utilities:

- build LCT from `Tree_Node`
- export represented forest to `Tree_Node`
- export a preferred-path snapshot for debugging only

The last one is especially useful during testing, but it should be debug-only
because preferred paths are an internal decomposition, not the actual tree.

## Why `BinNode` Is Also Not A Great Direct Base

`tpl_binNode.H` shows a clean binary-node infrastructure, but it is still tuned
for classical binary-tree families where:

- nodes carry a search key
- algorithms reason in BST terms
- the node base is intentionally generic but not path-lazy aware

An LCT node needs:

- no BST ordering meaning
- explicit auxiliary parent
- lazy reversal
- aggregate maintenance
- often different payload semantics for represented vertices versus edges

So a dedicated node structure is still the better choice. Reusing only low-level
rotation ideas is fine; reusing the node abstraction directly is not compelling.

## Recommended Implementation Order

### Phase 1: Standalone dynamic forest core

Scope:

- vertices only
- `make_root`, `connected`, `find_root`, `link`, `cut`, `lca`
- no graph integration yet
- no `Tree_Node` integration yet

Why:

- easiest to test against naive parent-array models
- isolates the splay/access invariants

### Phase 2: Path aggregates

Scope:

- monoid-based aggregate over paths
- query only at first
- optional lazy path updates later

Why:

- this is where LCT becomes useful beyond connectivity

### Phase 3: Graph adapter

Scope:

- generic over `GT` through the common Aleph graph surface
- `GT::Node*` / `GT::Arc*` mapping
- forest-only mode first
- undirected graphs only in the first version
- edge-as-node support if path queries over arcs matter

Why:

- this is the natural Aleph graph integration point
- it lets one adapter work with `List_Graph`, `List_SGraph`, and `Array_Graph`

### Phase 4: `Tree_Node` import/export

Scope:

- initialize from rooted static tree
- export current represented forest for visualization

Why:

- useful for examples, debugging, and interoperability

### Phase 5: Advanced graph algorithms

Scope:

- spanning-forest maintenance in cyclic graphs
- replacement-edge logic
- dynamic MST-like operations

Why:

- these require more than just the raw LCT core

## Pitfalls To Expect

### 1. Auxiliary-root versus represented-root confusion

These are different concepts. Many bugs come from mixing them.

### 2. Lazy reversal propagation

Every rotation and every descent that assumes child orientation must respect
pending reverse flags.

### 3. Incorrect `cut` semantics

There are several common APIs:

- cut by endpoint pair
- cut parent of a node
- cut by explicit edge handle

These should not be mixed informally.

### 4. Cookie collisions in graph mode

This is a real Aleph-specific integration risk. The graph adapter should avoid
permanent cookie occupancy.

### 5. Graph removal and stale handles

If a graph node or arc is removed, the adapter must define whether:

- removal is forbidden while attached to the LCT
- or the adapter auto-detaches and invalidates the LCT handle

That policy must be explicit.

### 6. Overpromising subtree queries

LCT is naturally a path structure. Subtree queries in a dynamically rerooted
forest are a separate design problem.

## Best Initial Fit In Aleph-w

The most credible first integration is:

- a standalone `tpl_link_cut_tree.H`
- focused on dynamic forests
- later a graph adapter that targets the common `GT` graph interface and works
  with `List_Graph`, `List_SGraph`, and `Array_Graph`
- `Tree_Node` only as import/export or debug snapshot

This matches the current repository much better than trying to turn
`Tree_Node` into the internal engine.

## Refined First Implementation Plan

With the graph-family subtleties in mind, the most robust first plan is:

1. Implement a standalone LCT core with no dependency on Aleph graph classes.
2. Add a thin adapter for forest semantics over generic undirected `GT`.
3. In that adapter, keep durable mappings in `DynMapTree`, not in cookies.
4. Use cookies only for temporary build/import/export stages, protected by
   `Cookie_Guard` or `Cookie_Saver`.
5. Explicitly reject:
   - digraphs in phase 1 of the adapter
   - self-loops as tree edges
   - cycle-creating links
   - external graph mutations while attached, unless all mutations go through
     the adapter
6. Add `Tree_Node` import/export only after the graph adapter is stable.

## Proposed Minimal Public API

This section proposes names and boundaries, not final C++ declarations.

### Naming recommendation

To match the repository style, the most natural naming is:

- file: `tpl_link_cut_tree.H`
- generic core class: `Gen_Link_Cut_Tree`
- convenience alias or thin specialization: `Link_Cut_Tree`

If a graph-facing layer is added separately, a clear name would be:

- `Gen_Link_Cut_Forest_Adapter`

That keeps the structure honest: the core is a dynamic forest structure, while
the adapter is what binds it to Aleph graph types.

### Why the first API should be handle-based

Aleph often exposes node-centric low-level structures, but for an LCT the first
version should still prefer managed handles over externally constructed nodes.

Reason:

- the internal invariants are much tighter than in ordinary BST nodes
- accidental external mutation of auxiliary fields would be disastrous
- the graph adapter already provides a natural bridge for external object identity

So the most robust first public model is:

- the LCT owns its internal nodes
- the user receives stable handles
- those handles are the only public identity of represented vertices and edges

Later, if needed, a lower-level node-oriented variant can be considered.

### Core surface for `Gen_Link_Cut_Tree`

The first standalone core should expose only the operations needed to validate
the dynamic-forest logic.

#### Construction and handles

- create an empty forest
- create a new represented vertex and return a stable handle
- optionally create a vertex with an initial payload value
- query whether a handle is valid
- query the number of represented vertices

The first version should probably not support vertex deletion at all. Handle
invalidation rules become much simpler if nodes are append-only.

#### Vertex payload access

Only if payloads are part of phase 1:

- read the local value of a represented vertex
- overwrite the local value of a represented vertex

If payloads are deferred to the aggregate phase, these can wait.

#### Structural forest operations

The core minimal operations should be:

- `connected(u, v)`
- `make_root(x)`
- `find_root(x)`
- `link(u, v)`
- `cut(u, v)`
- `lca(u, v)`

Semantics should be explicit:

- `link(u, v)` requires `u` and `v` to belong to different trees
- `cut(u, v)` requires that the represented edge `(u, v)` actually exists
- `lca(u, v)` is valid only if `u` and `v` are connected

The first version should not expose several `cut` variants at once. Pick one
and keep it precise. For a vertex-only core, `cut(u, v)` is the clearest.

#### Path query surface

Once aggregates arrive, the next minimal operation should be:

- `path_query(u, v)`

This should return the aggregate over the represented path and nothing more.
The first aggregate API should stay query-only.

Deferred until later:

- `path_apply(u, v, update)`
- subtree queries
- subtree updates

### Aggregate policy recommendation

The public API should make aggregation policy explicit instead of baking one
operation into the structure.

The most Aleph-like direction is a policy-based design:

- one policy defines the stored local value
- one policy defines the aggregate identity and combine operation
- optionally one update policy is added later for lazy path updates

This keeps the core reusable for:

- connectivity-only mode
- path min/max
- path sum
- maximum-edge-on-path
- custom path semirings or monoids

The first implementation can still ship a trivial no-aggregate policy as the
default.

### Edge-oriented extension

When edge payloads become first-class, the public API should still remain small.

The cleanest conceptual surface is:

- create a represented edge between two vertex handles
- optionally attach edge payload to that represented edge
- remove a represented edge
- query path aggregate where the aggregate domain is edge-based

That does not require changing the external story much, even if the internal
representation switches to edge-as-node.

### Graph adapter surface

The graph adapter should be deliberately smaller than the raw graph API. It is
not a replacement graph container.

The minimal graph-facing surface should likely be:

- build adapter state from an existing forest graph `GT`
- query whether a graph node is registered in the adapter
- obtain the LCT handle associated to a graph node
- obtain the LCT handle associated to a tree arc, if edge-as-node is enabled
- `connected(gnode_u, gnode_v)`
- `make_root(gnode)`
- `find_root(gnode)`
- `lca(gnode_u, gnode_v)`
- `link_arc(ga)` for a graph arc that is to become a tree edge
- `cut_arc(ga)` for a graph arc that is currently a tree edge
- `path_query(gnode_u, gnode_v)`

The adapter should not try to be clever about arbitrary graph mutation in phase
1. It should assume that tree-structure mutations go through adapter methods.

### Import/export surface for `Tree_Node`

This should remain auxiliary.

The minimal useful functions are:

- build an LCT from a rooted `Tree_Node`
- export the current represented forest to one or more `Tree_Node` roots

It is better to export a materialized represented forest only, not the internal
preferred-path decomposition.

### API that should be explicitly deferred

To keep the first release coherent, the following should be marked out of scope:

- subtree queries on rerootable trees
- support for general digraphs
- automatic synchronization with arbitrary external graph mutation
- dynamic deletion of represented vertices
- exposing internal splay nodes
- combining multiple cut variants in the first public surface
- persistence or rollback
- concurrent mutation

## Recommended First Public Contract

If reduced to the absolute minimum that still feels useful, the first public
contract should be:

1. create vertices
2. `link`
3. `cut`
4. `connected`
5. `make_root`
6. `find_root`
7. `lca`
8. later `path_query`

Everything else should be treated as adapter or phase-2+ functionality.

## Aleph-Style Implementation Guidelines

Yes, the eventual implementation can and should follow Aleph style, especially
the style used in tree headers. But that should mean stylistic and architectural
alignment, not mechanically reusing the wrong node abstractions.

### What "following Aleph style" should mean here

The implementation should align with Aleph in the following ways:

- header-centric design in a flat root-level header such as
  `tpl_link_cut_tree.H`
- generic main template named `Gen_Link_Cut_Tree`
- convenient alias or thin specialization named `Link_Cut_Tree`
- 2-space indentation and K&R braces
- `# include`, `# ifndef`, and related preprocessor formatting as used by Aleph
- English comments and API documentation
- `[[nodiscard]]`, `constexpr`, and `noexcept` where semantically justified
- `and`, `or`, and `not` in place of `&&`, `||`, and `!`
- Aleph containers and helpers where they fit naturally
- Aleph error macros instead of raw `throw`

This is the correct interpretation of "Aleph style" for an LCT.

### What should be reused conceptually from Aleph binary-tree code

The binary-tree code in Aleph suggests a few good design habits worth copying:

- the generic `Gen_...` pattern
- explicit node-oriented terminology
- low-level, invariant-conscious implementation style
- clean separation between primitive operations and convenient wrappers
- optional specializations or aliases layered over a generic base

For the LCT, that suggests:

- one generic core class
- a small set of primitive internal operations
- optional higher-level wrappers for aggregates or graph adaptation

### What should not be copied literally from current BST implementations

The current BST and splay-tree implementations assume semantics that do not
hold for Link-Cut Trees.

So the implementation should not:

- force the internal node to derive from `BinNode`
- use key comparison as the organizing principle of splaying
- present the auxiliary trees as if they were search trees
- expose raw left/right manipulations as part of the public contract

That would produce code that looks superficially Aleph-like but is conceptually
wrong.

### Preferred internal layering

To stay close to Aleph style while keeping the implementation sound, the code
should likely be organized as:

1. internal node structure dedicated to LCT invariants
2. primitive internal operations:
   - `is_aux_root`
   - `push`
   - `pull`
   - rotations
   - `splay`
   - `access`
3. public dynamic-forest operations:
   - `make_root`
   - `find_root`
   - `connected`
   - `link`
   - `cut`
   - `lca`
4. optional aggregate layer
5. optional graph adapter

That is very much in the spirit of Aleph tree code: small trusted primitives,
then a compact public surface above them.

### Containers and support types

The implementation should prefer Aleph infrastructure where that does not fight
the algorithm.

Good candidates:

- `Array` for owned contiguous buffers if a handle table is needed
- `DynMapTree` for graph-node or graph-arc to LCT-handle mappings
- `DynList` for exported forests or batched results
- `Aleph::less` and related functors where comparison is needed

But the internal auxiliary-tree pointers should remain plain pointers. For an
LCT core, that is the natural representation.

### Error handling style

The public API should use the usual Aleph translated exception macros.

Typical contract checks should look conceptually like:

- invalid handle
- null graph node
- cycle-creating `link`
- `cut` on a non-existing tree edge
- graph adapter invoked on a digraph in a phase that only supports undirected graphs

In Aleph terms, these should become:

- `ah_invalid_argument_if(...)`
- `ah_domain_error_if(...)`
- `ah_runtime_error_if(...)`

depending on the contract being enforced.

### Public versus private mutability

Aleph binary-tree code is comfortable with low-level node manipulation, but the
LCT should still be more conservative publicly.

Recommended rule:

- keep internal node fields fully private to the implementation
- expose operations through class methods only
- do not let user code mutate auxiliary pointers directly

This is one area where following Aleph style should mean "same rigor", not
"same amount of exposed internals".

### Naming guidance for internal operations

The internal names should be simple, explicit, and close to the literature.

Good internal names:

- `rotate_left`
- `rotate_right`
- `splay`
- `access`
- `push`
- `pull`
- `make_root`
- `find_root`
- `is_aux_root`

Good public names:

- `connected`
- `link`
- `cut`
- `lca`
- `path_query`

Avoid overly clever or library-internal jargon for the public API. The Aleph
headers are usually most maintainable when the public names are direct.

### Specialization strategy

If the implementation grows, it should probably not fork into many unrelated
classes too early.

The clean Aleph-style path is:

- one generic `Gen_Link_Cut_Tree`
- maybe one default alias `Link_Cut_Tree`
- later, if useful, one graph adapter class rather than several graph-family-
  specific classes

That reduces surface area and fits the repository's template conventions.

### Testing style

Tests should follow the same discipline used in Aleph tree and graph modules:

- many focused tests over invariants
- small deterministic scenarios first
- then randomized cross-checks against a naive model
- explicit coverage of edge cases and contract violations

For an LCT, that means:

- tiny hand-built forests
- repeated operation sequences
- random `link` / `cut` / `make_root` / query scripts
- comparison against a slow reference forest representation

This is much closer to Aleph testing style than only benchmarking.

### Documentation style

The final header should document:

- what structure is represented
- amortized complexity versus worst-case intuition
- what "path query" means, vertex-based or edge-based
- whether digraphs are supported
- whether graph cookies are used permanently or only transiently
- invalid-operation contracts

That documentation style matches the newer, stronger Aleph headers and will be
important for something as easy to misuse as an LCT.

### Bottom line on style compatibility

So the answer is yes:

- the implementation can absolutely follow Aleph style
- it should especially follow the organizational style of Aleph tree headers
- but it should not imitate BST internals where the semantics differ

The right target is:

- Aleph externally
- dedicated LCT internally

## Internal Invariants To Preserve

This section is meant to guide the eventual implementation work. The most
common way to break an LCT is not by writing the wrong public API, but by
losing one of its internal invariants during `access`, `splay`, `link`, or
`cut`.

### Recommended first internal formulation

For the first implementation, the clearest formulation is:

- one dedicated internal node type
- explicit auxiliary-tree child pointers
- explicit auxiliary parent pointer
- explicit lazy reverse flag
- explicit aggregate fields if aggregates are enabled

And, importantly:

- prefer a formulation with a clearly distinguished "auxiliary parent" concept
  instead of relying on ambiguous external interpretation

Whether that is encoded as:

- separate `aux_parent` and `path_parent`

or as:

- one overloaded parent pointer plus strict helper predicates

is still an open implementation decision. But for correctness and ease of
debugging, the safer first target is the more explicit formulation.

If we resume implementation later and need a default choice, the recommended
choice is:

- vertex-only first
- no vertex deletion
- explicit auxiliary-parent semantics
- no lazy path updates yet

That is the least risky route.

### Global representation invariants

These should hold at all stable boundaries between public operations.

#### Represented-forest invariants

- each represented vertex corresponds to exactly one internal LCT node
- if edge-as-node is enabled, each represented tree edge corresponds to exactly
  one internal edge node
- the represented forest is acyclic
- each connected component has a well-defined current represented root after
  `make_root`

#### Auxiliary-tree invariants

- each internal node belongs to exactly one auxiliary splay tree at a time
- `left` and `right` pointers define a valid binary tree
- child-to-parent links are consistent
- the auxiliary root predicate is correct for every node
- any preferred-path interpretation used by `access` is consistent with the
  current auxiliary decomposition

#### Lazy-state invariants

- a pending reverse flag means the represented path orientation is logically
  inverted below that node
- no operation that relies on left/right meaning may proceed through a node
  without respecting pending lazy state
- `push` is the only operation that materializes pending lazy state downward

#### Aggregate invariants

If aggregates are enabled:

- each node stores a correct local payload
- each node stores a correct aggregate for its auxiliary subtree
- aggregate recomputation happens only through one trusted primitive
- after every structural mutation, affected nodes are restored through `pull`

### Primitive operation invariants

#### `push(x)`

Contract:

- consumes pending lazy state on `x`
- pushes any reversible orientation state into children
- leaves `x` itself logically clean

After `push(x)`:

- `x` has no unapplied local reverse flag
- children reflect the propagated logical reversal state
- represented semantics are preserved
- aggregate meaning is still correct modulo later `pull` on structurally changed
  ancestors

#### `pull(x)`

Contract:

- recomputes all derived metadata of `x` from:
  - its local payload
  - its left child
  - its right child

After `pull(x)`:

- size-like metadata is correct
- aggregate metadata is correct
- `x` is locally self-consistent

`pull` should be the only place where aggregate recomputation is performed.

#### Rotations

Whether implemented as `rotate_left` / `rotate_right` or a single generalized
rotation helper, every rotation must preserve:

- binary-tree connectivity
- child-parent consistency
- the in-order order of the auxiliary tree
- the represented preferred-path semantics

After a rotation:

- all affected nodes have correct parent links
- all affected nodes have correct child links
- `pull` has been applied in the right order to restore metadata

### Composite operation invariants

#### `splay(x)`

Contract:

- moves `x` to the root of its current auxiliary tree

After `splay(x)`:

- `x` is the auxiliary root of the same auxiliary tree as before
- the represented forest itself is unchanged
- all lazy state on the path handled during rotations has been respected
- local metadata of the rotated nodes is restored

Crucial subtlety:

- `splay` changes auxiliary structure only
- it does not by itself change which represented tree is encoded

#### `access(x)`

Recommended chosen contract for the future implementation:

- after `access(x)`, `x` is the auxiliary root of the preferred path from the
  represented root to `x`
- the preferred child direction convention is fixed consistently across the
  implementation
- under the standard convention, `x` ends with no preferred child on the side
  used to continue downward past `x`

After `access(x)`:

- the represented forest is unchanged
- the preferred-path decomposition has been updated so that the root-to-`x`
  path is exposed
- `x` is in the canonical exposed position expected by `make_root`,
  `find_root`, `lca`, and `path_query`

This operation is the structural heart of the LCT. If something later behaves
strangely, `access` is usually the first place to audit.

#### `make_root(x)`

Contract:

- reroots the represented tree at `x`

Typical logic conceptually:

- expose the current root-to-`x` path
- reverse that exposed path

After `make_root(x)`:

- `x` is the represented root of its component
- connectivity is unchanged
- no represented edge is created or destroyed
- future `lca` and `path_query` results interpret the component relative to
  that new root

#### `find_root(x)`

Contract:

- returns the represented root of the component containing `x`

After `find_root(x)`:

- the returned node is the represented root under the current rooting
- the represented forest is unchanged
- the auxiliary structure is left in a valid canonical state

#### `connected(u, v)`

Contract:

- returns true if and only if `u` and `v` belong to the same represented tree

After `connected(u, v)`:

- no represented structure changes
- auxiliary structure may have been rearranged, but remains valid

#### `link(u, v)`

For the first version, the intended contract should be:

- `u` and `v` are valid handles
- `u != v`
- `u` and `v` are currently in different represented trees

After `link(u, v)`:

- the two represented components become one
- exactly one represented tree edge has been added
- no cycle has been introduced
- connectivity reflects the merge immediately

If edge-as-node is enabled later:

- exactly one edge node must be created and inserted in the represented forest

#### `cut(u, v)`

For the first version, the intended contract should be:

- `u` and `v` are valid handles
- the represented tree edge between them exists

After `cut(u, v)`:

- exactly one represented tree edge has been removed
- the previous component splits into two components
- no other represented edge changes

This operation must be more precise than "disconnect these two connected
vertices somehow". It must remove the specific tree edge.

#### `lca(u, v)`

Contract:

- valid only if `u` and `v` are connected
- interpreted relative to the current represented root of their component

After `lca(u, v)`:

- the returned node is the lowest common ancestor in the currently rooted
  represented tree
- represented structure is unchanged

#### `path_query(u, v)`

When aggregates are added:

- valid only if `u` and `v` are connected
- semantics must state clearly whether the aggregate is vertex-based or
  edge-based

After `path_query(u, v)`:

- the returned aggregate matches the represented path between `u` and `v`
- represented structure is unchanged
- auxiliary structure remains valid

## Resume Context

This section is meant to make it easy to resume later without re-deriving the
whole design discussion.

### Decisions already taken

These are the conclusions currently established in this document:

1. `Tree_Node` should not be the internal representation of the LCT.
2. The LCT should be implemented as a dedicated core structure.
3. The core should follow Aleph external style, but not reuse BST internals
   blindly.
4. A future graph adapter should target the common Aleph graph API rather than
   only `List_Graph`.
5. The first graph-adapter target should be undirected graph families:
   `List_Graph`, `List_SGraph`, and `Array_Graph`.
6. Graph cookies should be used only transiently, not as permanent storage for
   the adapter.
7. The safest first scope is:
   - vertex-only core
   - no vertex deletion
   - no subtree queries
   - no digraph support
   - no lazy path updates initially

### Decisions still open

These still need to be frozen before coding:

1. internal parent formulation:
   - explicit auxiliary/path distinction
   - or overloaded single-parent style
2. whether the first aggregate-enabled version is:
   - connectivity only
   - or already path-query capable
3. exact graph-adapter mutation policy:
   - graph immutable while attached
   - or all mutations must pass through adapter methods
4. when to introduce edge-as-node:
   - only after vertex-only core is stable
   - or immediately in the graph-adapter phase

### Recommended next-session default choices

If we want to resume quickly and minimize design churn, the recommended choices
for the next session are:

1. choose explicit auxiliary-parent semantics
2. keep phase 1 vertex-only
3. keep phase 1 query surface limited to:
   - `connected`
   - `make_root`
   - `find_root`
   - `link`
   - `cut`
   - `lca`
4. defer `path_query` until the structural core is validated
5. defer graph adaptation until the standalone core has a solid naive-model test

### Recommended next concrete step

When work resumes, the next most rational step is not coding immediately. It is
to freeze one short implementation note that answers only these points:

1. exact node fields
2. exact meaning of parent pointers
3. exact `access` postcondition convention
4. whether `cut` is only `cut(u, v)` in phase 1
5. whether phase 1 ships without aggregates

Once those five points are frozen, implementation can proceed with much lower
risk of redesign.

### If we stop here

If we stop here and come back later, the working plan should be read as:

- keep the current document as the source of truth
- begin from the "Recommended next-session default choices" above
- only after that start the actual header skeleton and tests

## Suggested Test Strategy Once Implementation Starts

Even though this document does not include code, the eventual implementation
should be validated in layers.

### Core LCT tests

- isolated node
- simple chains
- stars
- repeated `make_root`
- `link` on disconnected trees only
- rejection of cycle-creating links
- `cut` on missing edges
- `lca` on same tree and different trees

### Aggregate tests

- path sum/min/max against a naive model
- edge-node and vertex-node semantics checked separately

### Graph adapter tests

- build from a forest `List_Graph`
- keep graph and LCT synchronized under link/cut
- verify node and arc back-mapping
- verify no cookie leakage after temporary import/export stages

### `Tree_Node` adapter tests

- import rooted tree into LCT
- export represented forest back to `Tree_Node`
- compare structure and payloads

## Bottom Line

A Link-Cut Tree can fit Aleph-w well, but not by reusing `Tree_Node` as its
internal node type.

The right architecture is:

1. dedicated LCT core
2. optional graph adapter using stable graph pointers plus Aleph mappings
3. optional `Tree_Node` import/export for visualization and interoperability

If the goal is strong Aleph integration, the most important design choices are:

- do not tie the core to graph cookies permanently
- do not force the core into `Tree_Node`
- treat general graphs as "graph plus maintained spanning forest", not as a
  direct LCT problem
- decide early whether edge payloads are first-class, because that strongly
  pushes the design toward an edge-as-node representation
