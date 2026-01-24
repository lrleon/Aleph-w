# Red-Black Trees in Aleph-w

This document describes the various red-black tree implementations available in Aleph-w.

## Overview

Red-black trees are self-balancing binary search trees that maintain balance through node coloring. The Aleph-w library provides five different implementations optimized for different use cases:

| Implementation | File | Algorithm | Rank Support | Best For |
|----------------|------|-----------|--------------|----------|
| `Gen_Rb_Tree` | `tpl_rb_tree.H` | Bottom-up | ❌ | General purpose |
| `GenTdRbTree` | `tpl_tdRbTree.H` | Top-down | ❌ | Insert-heavy workloads |
| `HtdRbTree` | `tpl_hRbTree.H` | Hybrid | ❌ | Mixed workloads |
| `Gen_Rb_Tree_Rk` | `tpl_rbRk.H` | Bottom-up | ✓ | Ranked access |
| `GenTdRbTreeRk` | `tpl_tdRbTreeRk.H` | Top-down | ✓ | Ranked + insert-heavy |
| `HtdRbTreeRk` | `tpl_hRbTreeRk.H` | Hybrid | ✓ | **Best: O(log n) all ops** |

## Complexity Comparison

| Operation | Bottom-up | Top-down | Hybrid | Bottom-up Rk | Top-down Rk | **Hybrid Rk** |
|-----------|-----------|----------|--------|--------------|-------------|---------------|
| `search()` | O(log n) | O(log n) | O(log n) | O(log n) | O(log n) | O(log n) |
| `insert()` | O(log n) | O(log n) | O(log n) | O(log n) | O(log n) | O(log n) |
| `remove()` | O(log n) | O(log n) | O(log n) | O(log n) | O(n)* | **O(n)** |
| `select(i)` | N/A | N/A | N/A | O(log n) | O(log n) | O(log n) |
| `position(k)` | N/A | N/A | N/A | O(log n) | O(log n) | O(log n) |
| `size()` | O(1) | O(1) | O(1) | O(1) | O(1) | O(1) |
| Stack space | O(log n) | O(1) | O(log n) | O(log n) | O(1)† | O(log n) |

*Top-down Rk and Hybrid Rk remove is O(n) due to complex node swaps requiring full count recalculation.
†Top-down uses O(log n) temporary stack during insert for count updates.

## Detailed Descriptions

### 1. Gen_Rb_Tree (Bottom-Up)

**File:** `tpl_rb_tree.H`

The classic bottom-up red-black tree implementation. During insertion/deletion, it descends to find the target position while pushing ancestors onto a stack, then ascends back up to fix violations.

```cpp
#include <tpl_rb_tree.H>

Rb_Tree<int> tree;
auto node = new Rb_Tree<int>::Node(42);
tree.insert(node);
```

**Pros:**
- Well-understood algorithm
- Minimal rotations (at most 2 for insert, 3 for delete)
- Good for balanced read/write workloads

**Cons:**
- Requires O(log n) stack space for ancestor tracking

### 2. GenTdRbTree (Top-Down)

**File:** `tpl_tdRbTree.H`

Top-down algorithm by Guibas and Sedgewick. Performs color flips and rotations during the descent, ensuring the tree is balanced before reaching the insertion/deletion point.

```cpp
#include <tpl_tdRbTree.H>

TdRbTree<int> tree;
auto node = new TdRbTree<int>::Node(42);
tree.insert(node);
```

**Pros:**
- No stack needed (O(1) auxiliary space)
- Single pass through tree (better cache locality)
- Good for insert-heavy workloads

**Cons:**
- More color flips than strictly necessary
- More complex deletion logic

### 3. HtdRbTree (Hybrid)

**File:** `tpl_hRbTree.H`

Combines top-down insertion with bottom-up deletion. Uses proactive color flipping during insert descent but maintains a stack for deletion.

```cpp
#include <tpl_hRbTree.H>

HtdRbTree<int> tree;
auto node = new HtdRbTree<int>::Node(42);
tree.insert(node);
```

**Pros:**
- Fast insertion (top-down, single pass)
- Robust deletion (bottom-up with stack)
- Good balance between insert and delete performance

**Cons:**
- Still needs O(log n) stack for deletion

### 4. Gen_Rb_Tree_Rk (Bottom-Up with Rank)

**File:** `tpl_rbRk.H`

Extended red-black tree with subtree size counts in each node. Enables O(log n) rank operations.

```cpp
#include <tpl_rbRk.H>

Rb_Tree_Rk<int> tree;
// Insert 100 elements
for (int i = 0; i < 100; ++i)
    tree.insert(new Rb_Tree_Rk<int>::Node(i));

// Get 50th smallest element (0-indexed)
auto node = tree.select(49);

// Get position of key 42
auto [pos, found] = tree.position(42);
```

**Rank Operations:**
- `select(i)`: Get the i-th smallest element
- `position(key)`: Get the rank of a key (-1 if not found)
- `find_position(key)`: Get insertion position (works even if key doesn't exist)
- `remove_pos(i)`: Remove element at position i
- `split_pos(pos, t1, t2)`: Split tree at position

**Pros:**
- O(log n) for all operations including rank queries
- Ideal for order statistics

**Cons:**
- Extra space per node for count
- Slightly higher overhead per rotation

### 5. GenTdRbTreeRk (Top-Down with Rank)

**File:** `tpl_tdRbTreeRk.H`

Top-down red-black tree with rank support. Combines single-pass insertion with subtree counts.

```cpp
#include <tpl_tdRbTreeRk.H>

TdRbTreeRk<int> tree;
for (int i = 0; i < 100; ++i)
    tree.insert(new TdRbTreeRk<int>::Node(i));

auto node = tree.select(49);
```

**Important:** Deletion is O(n) in this implementation because the complex node swaps during deletion invalidate incremental count updates. Use `Rb_Tree_Rk` if you need O(log n) deletion with rank support.

### 6. HtdRbTreeRk (Hybrid with Rank)

**File:** `tpl_hRbTreeRk.H`

Hybrid approach with subtree size counts. Combines top-down insertion with bottom-up deletion, both with rank updates.

```cpp
#include <tpl_hRbTreeRk.H>

HtdRbTreeRk<int> tree;
for (int i = 0; i < 100; ++i)
    tree.insert(new HtdRbTreeRk<int>::Node(i));

auto node = tree.select(49);  // Get 50th element
auto [pos, found] = tree.position(42);  // Get position of key 42
```

**Pros:**
- O(log n) insert with rank updates
- Uses familiar bottom-up deletion logic
- Good balance for mixed workloads

**Cons:**
- O(n) deletion (count recalculation after complex swaps)
- Extra space per node for count

## DynSetTree Aliases

For convenience, `tpl_dynSetTree.H` provides high-level container classes that manage node allocation automatically:

```cpp
#include <tpl_dynSetTree.H>

// Bottom-up implementations
DynSetRbTree<int> set1;           // Gen_Rb_Tree
DynSetRbRkTree<int> set2;         // Gen_Rb_Tree_Rk

// Top-down implementations
DynSetTdRbTree<int> set3;         // GenTdRbTree
DynSetTdRbRkTree<int> set4;       // GenTdRbTreeRk

// Hybrid implementations
DynSetHtdRbTree<int> set5;        // HtdRbTree
DynSetHtdRbRkTree<int> set6;      // HtdRbTreeRk (with rank)

// Splay trees (also available)
DynSetSplayTree<int> set7;        // Splay_Tree
DynSetSplayRkTree<int> set8;      // Splay_Tree_Rk (with rank)
```

## Choosing the Right Implementation

1. **General purpose:** Use `Rb_Tree` (bottom-up)
2. **Insert-heavy, few deletes:** Use `TdRbTree` (top-down)
3. **Mixed workload:** Use `HtdRbTree` (hybrid)
4. **Need rank + O(log n) delete:** Use `Rb_Tree_Rk` (bottom-up with rank)
5. **Rank + insert-heavy:** Use `TdRbTreeRk` or `HtdRbTreeRk`, but avoid if deletions are frequent (O(n))

## Node Types

Each tree template accepts a node type parameter:

| Node Type | Has Virtual Destructor |
|-----------|------------------------|
| `RbNode<Key>` | ❌ |
| `RbNodeVtl<Key>` | ✓ |
| `RbNodeRk<Key>` | ❌ (has count) |
| `RbNodeRkVtl<Key>` | ✓ (has count) |

Use the `Vtl` variants when you need polymorphic node destruction.

## Testing

All implementations are tested in:
- `Tests/rb-tree.cc` - Bottom-up RB, Hybrid RB tests
- `Tests/tdrbtree_test.cc` - Top-down RB tests
- `Tests/tdrbtreerk_test.cc` - Top-down RB with rank tests
- `Tests/htdrbtreerk_test.cc` - Hybrid RB with rank tests

Run tests:
```bash
cd Tests/build
make rb-tree tdrbtree_test tdrbtreerk_test htdrbtreerk_test splay-tree-rk
./rb-tree
./tdrbtree_test
./tdrbtreerk_test
./htdrbtreerk_test
./splay-tree-rk
```

## Other Trees with Rank Support

In addition to red-black trees, Aleph-w provides other balanced trees with rank:

| Tree | File | Description |
|------|------|-------------|
| `Splay_Tree_Rk` | `tpl_splay_treeRk.H` | Self-adjusting tree with amortized O(log n) |
| `Treap_Rk` | `tpl_treapRk.H` | Randomized BST with expected O(log n) |
| `Avl_Tree_Rk` | `tpl_avlRk.H` | Strictly balanced AVL with rank |

All provide `select()` and `position()` operations with O(log n) complexity.

