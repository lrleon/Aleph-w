
/*
                          Aleph_w

  Data structures & Algorithms
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

/** @file tpl_rope.H
 *  @brief Immutable, structurally-shared rope (`Aleph::Rope`) for large
 *         character sequences.
 *
 *  `Rope<Char, LeafSize>` is a persistent binary tree of character chunks:
 *  every operation that "modifies" a rope (`concat`, `substr`, `insert`,
 *  `erase`) returns a brand-new `Rope` and never mutates the receiver or
 *  any existing rope. Unmodified subtrees are shared (via
 *  `std::shared_ptr<const Node>`) between the old and new ropes instead of
 *  being copied, so a `concat` of two large ropes is cheap (it wraps both
 *  existing trees in one new node) and an old snapshot stays valid and
 *  unaffected by later operations on a derived rope.
 *
 *  @par Structure
 *  Leaves hold up to `LeafSize` characters each (`Aleph::SmallVector<Char,
 *  LeafSize>`); internal nodes hold no character data, just a left and a
 *  right child and the total length of their subtree. `at(pos)` descends
 *  in O(depth), comparing `pos` against each visited node's left-subtree
 *  length to choose a side.
 *
 *  @par Rebalancing
 *  Rebalancing is explicit and deliberately simple, not the classical
 *  Fibonacci-threshold criterion from Boehm, Atkinson &amp; Plass, "Ropes:
 *  An Alternative to Strings": after every `concat`, if the resulting
 *  node's depth exceeds `2 * bit_width(length) + 8` (a generous, easy-to-
 *  state upper bound -- true balanced depth is close to `log2(length)`),
 *  the whole subtree is rebuilt from its leaves into a balanced tree
 *  (`O(number of leaves)`, reusing every leaf node unchanged -- only
 *  internal nodes are rebuilt, so no character data is copied). Once a
 *  subtree is balanced, `at`/`substr` on it cost `O(log n)`; a `concat`
 *  that does not itself trigger a rebuild is `O(1)`.
 *
 *  @par Leaf absorption (small-piece concat fast path)
 *  Before wrapping two subtrees in a new node, `concat` first checks
 *  whether one side is a lone leaf and, if so, tries to merge it directly
 *  into the adjacent leaf on the other side (the rightmost leaf of the
 *  left operand for an append, or the leftmost leaf of the right operand
 *  for a prepend) when that leaf still has room below `LeafSize`. This
 *  touches only the `O(depth)` nodes on that one spine, shares everything
 *  else, and -- because a leaf is only ever replaced by another leaf of
 *  the same depth -- never changes the tree's depth and therefore never
 *  needs a rebalance. This exists specifically for the common pattern of
 *  building a rope by repeatedly `concat`-ing one small piece at a time
 *  (e.g. one character per loop iteration): without it, that pattern
 *  triggered a whole-subtree rebuild roughly every `O(log n)` concats, for
 *  a cost close to `O(n^2 / LeafSize)` over the whole loop; with it,
 *  appending is `O(depth)` per call as long as the trailing leaf has
 *  spare room, and a full-size internal node (with its own occasional
 *  rebalance) is only created once every `LeafSize` calls. Measured
 *  effect: 50000 single-character concats went from several seconds to
 *  single-digit milliseconds.
 *
 *  @par What this type does NOT provide (see docs/missing_data_structures_plan.md)
 *  No mutable iterators, no lazy/incremental rebalancing, and no Unicode
 *  awareness -- `Char` is treated as an opaque, fixed-width code unit; a
 *  `Rope<char>` over UTF-8 text indexes and splits by *byte*, not by
 *  Unicode code point or grapheme cluster.
 *
 *  @par Recursion / stack depth
 *  Every internal traversal (`slice`, `collect_leaves`,
 *  `build_balanced_from_leaves`, `flatten_into`, `string_append_into`,
 *  `try_absorb_right`/`try_absorb_left`, `verify_rec`) is recursive, with
 *  depth bounded by the tree's `depth` field -- which the rebalancing
 *  contract above keeps `O(log size())` once a subtree has been through at
 *  least one `concat`. The one path built without ever calling
 *  `maybe_rebalance` is the initial `Rope(View)` constructor
 *  (`build_from_view`), whose own midpoint-splitting recursion is
 *  separately `O(log(size()/LeafSize))` by construction. Neither path
 *  produces stack depth proportional to `size()` itself.
 *
 *  @par Thread safety
 *  `Rope` has no built-in synchronization, but its immutability makes most
 *  concurrent usage safe by construction: since no operation ever mutates
 *  an existing node, any number of threads may concurrently call any
 *  method on their own *distinct* `Rope` objects -- including ones that
 *  were copied from a common source and therefore share the same
 *  underlying tree -- with no locking, exactly like `std::shared_ptr`
 *  (whose reference-count updates are atomic). What is **not** safe,
 *  again exactly as for `std::shared_ptr`, is multiple threads reading
 *  and/or writing the *same* `Rope` variable (the same `root_` member)
 *  concurrently without external synchronization -- e.g. two threads both
 *  executing `shared_rope = shared_rope.concat(x);` on one shared
 *  variable is a data race on that variable's assignment, not on the tree
 *  it points to. The common "give each worker its own copy" pattern is
 *  therefore free; a single `Rope` variable shared and mutated across
 *  threads still needs a lock or an atomic wrapper around the variable
 *  itself.
 *
 *  @see prefix-tree.H, tpl_radix_tree.H, tpl_patricia_trie.H for other
 *       recently-added Aleph-w data structures.
 *  @ingroup Sequences
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_ROPE_H
#define TPL_ROPE_H

#include <algorithm>
#include <bit>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <ah-errors.H>
#include <tpl_array.H>
#include <tpl_small_vector.H>

namespace Aleph {

/// True exactly for the character types `std::char_traits` is specialized
/// for by the standard (`char`, `wchar_t`, `char8_t`, `char16_t`,
/// `char32_t`), i.e. the types `std::basic_string<T>` is meaningfully a
/// "string" of. Used to gate `Rope::to_string()`: a `typename
/// std::char_traits<T>` (or `std::basic_string<T>`) type-existence check is
/// NOT sufficient here, since both templates' primary forms are valid type
/// names for essentially any trivial `T`, not just actual character types.
template <class T>
concept RopeCharTraitsCompatible = std::is_same_v<T, char> or std::is_same_v<T, wchar_t> or
  std::is_same_v<T, char8_t> or std::is_same_v<T, char16_t> or std::is_same_v<T, char32_t>;

/** @brief Immutable, structurally-shared rope over a sequence of `Char`.
 *
 *  See the file-level documentation in `tpl_rope.H` for the full design
 *  rationale, structure, and rebalancing contract.
 *
 *  @tparam Char Character type stored (default `char`). Treated as an
 *          opaque fixed-width code unit; no Unicode awareness. Must be a
 *          trivial, standard-layout type: `View` is
 *          `std::basic_string_view<Char>`, which the standard library
 *          itself requires this of (see `<string_view>`'s own
 *          `static_assert`). One consequence: `Char`'s copy can never
 *          throw (a trivial copy constructor cannot run user code), so
 *          `Rope`'s only real exception source is allocation failure
 *          (`std::bad_alloc`), not anything `Char`-related.
 *  @tparam LeafSize Maximum number of characters per leaf node (default
 *          256). Larger values mean fewer, bigger leaves (cheaper to
 *          build, costlier to slice at a boundary inside one); smaller
 *          values mean more structural sharing granularity at the cost of
 *          more nodes.
 */
template <typename Char = char, size_t LeafSize = 256>
class Rope
{
  static_assert(LeafSize > 0, "Rope requires a positive LeafSize");

public:
  /// View type accepted by the constructor and compared against.
  using View = std::basic_string_view<Char>;

private:
  struct Node
  {
    size_t length = 0;
    size_t depth = 0;  // 0 for a leaf; 1 + max(child depths) for internal.
    // Heap-indirected rather than embedded: `SmallVector<Char, LeafSize>`
    // carries an inline buffer of `LeafSize * sizeof(Char)` bytes as part
    // of its own size (see tpl_small_vector.H), so embedding it directly
    // here would make *every* node -- including internal ones, which
    // never hold character data -- pay that cost (e.g. 280 bytes at the
    // default `LeafSize=256`, roughly 5x the size of everything else this
    // struct needs). A null pointer costs 8 bytes regardless of
    // `LeafSize`; only leaves pay for an actual buffer, and only once.
    std::unique_ptr<const SmallVector<Char, LeafSize>> leaf_data;
    std::shared_ptr<const Node> left;   // null iff this is a leaf.
    std::shared_ptr<const Node> right;  // null iff this is a leaf.

    [[nodiscard]] bool is_leaf() const noexcept
    {
      // Both children null iff leaf: every construction path sets
      // `left`/`right` together (never just one), so this is equivalent
      // in practice to checking only `left` -- but checking both means a
      // hypothetically corrupted "one child null" node is *not*
      // misclassified as a leaf, and instead falls through to
      // `verify_rec`'s existing `left == nullptr or right == nullptr`
      // rejection for internal nodes.
      return left == nullptr and right == nullptr;
    }
  };

  using NodePtr = std::shared_ptr<const Node>;

  NodePtr root_;  // null means the empty rope (length 0).

  [[nodiscard]] static NodePtr make_leaf(SmallVector<Char, LeafSize> data)
  {
    auto node = std::make_shared<Node>();
    node->length = data.size();
    node->depth = 0;
    node->leaf_data =
        std::make_unique<const SmallVector<Char, LeafSize>>(std::move(data));
    return node;
  }

  /// Raw construction of an internal node from two already-built, non-null
  /// subtrees: no rebalancing, just `length`/`depth` bookkeeping. Shared by
  /// every call site that assembles an internal node from scratch
  /// (`make_internal`, `build_balanced_from_leaves`, `build_from_view`) so
  /// the bookkeeping can't drift between them.
  [[nodiscard]] static NodePtr make_internal_node(NodePtr left, NodePtr right)
  {
    // Checked *before* adding, not after: structural sharing means a
    // logical length near SIZE_MAX is reachable with very little real
    // memory (e.g. ~64 self-concats of `r = r.concat(r)`, each O(1) since
    // both sides already share the same subtree). Left unchecked, a
    // wrapped `length` can desynchronize from the real `depth` and make
    // `is_balanced()` wrongly decide to rebalance -- and `collect_leaves`
    // does not deduplicate shared nodes, so rebalancing a "wide" shared
    // structure like that visits it once per (possibly exponential)
    // root-to-leaf path instead of once per distinct node.
    ah_overflow_error_if(left->length > std::numeric_limits<size_t>::max() - right->length)
      << "Rope: concatenated length would overflow size_t";
    auto node = std::make_shared<Node>();
    node->length = left->length + right->length;
    node->depth = 1 + std::max(left->depth, right->depth);
    node->left = std::move(left);
    node->right = std::move(right);
    return node;
  }

  /// Wrap two non-null subtrees in a new internal node, then rebalance if
  /// the result is deeper than `is_balanced()` allows. Never called with
  /// a null `left`/`right` -- callers (`concat_nodes`) handle the "one
  /// side is the empty rope" case by returning the other side directly.
  [[nodiscard]] static NodePtr make_internal(NodePtr left, NodePtr right)
  {
    return maybe_rebalance(make_internal_node(std::move(left), std::move(right)));
  }

  /// Try to merge `extra` into the *rightmost* leaf of `node`, sharing
  /// every other node unchanged. Returns an empty `NodePtr` if `node` is
  /// null or its rightmost leaf has no spare room for `extra` (the caller
  /// then falls back to a normal concat). This never changes any node's
  /// `depth`: by induction on the recursion, `node->right` is replaced by
  /// a `new_right` of the exact same depth (base case: leaf -> leaf, depth
  /// 0; inductive case: `new_right`'s own recursive call already
  /// preserved its depth) while `node->left` is shared verbatim -- so
  /// `1 + max(depth(left), depth(new_right))` equals `node->depth` exactly,
  /// regardless of which side held the max (see `concat_nodes` for why
  /// this means absorption never needs `maybe_rebalance`).
  [[nodiscard]] static NodePtr try_absorb_right(const NodePtr &node,
                                                const SmallVector<Char, LeafSize> &extra)
  {
    if (node == nullptr)
      return nullptr;
    if (node->is_leaf())
      {
        if (node->length + extra.size() > LeafSize)
          return nullptr;
        // No `reserve()`: the capacity check above already guarantees the
        // merged result fits within `SmallVector`'s inline capacity
        // (`LeafSize`), so this can never reallocate. `append_range`
        // (rather than the copy constructor plus a per-character loop)
        // lets a trivially copyable `Char` (the only kind `Rope` accepts,
        // see the class's own `@tparam` note) copy both pieces via
        // `memcpy` instead of one placement-new call per character.
        SmallVector<Char, LeafSize> merged;
        merged.append_range(node->leaf_data->data(), node->leaf_data->size());
        merged.append_range(extra.data(), extra.size());
        return make_leaf(std::move(merged));
      }
    NodePtr new_right = try_absorb_right(node->right, extra);
    if (new_right == nullptr)
      return nullptr;
    // See the matching check/comment in `make_internal_node`: `node` here
    // can be an arbitrarily large shared subtree, so this addition needs
    // the same overflow guard.
    ah_overflow_error_if(node->length > std::numeric_limits<size_t>::max() - extra.size())
      << "Rope: concatenated length would overflow size_t";
    auto result = std::make_shared<Node>();
    result->length = node->length + extra.size();
    result->depth = node->depth;
    result->left = node->left;
    result->right = std::move(new_right);
    return result;
  }

  /// Mirror of `try_absorb_right`: merge `extra` into the *leftmost* leaf
  /// of `node`, for the "prepend one small piece at a time" pattern. The
  /// same depth-preservation argument applies, symmetrically on `left`.
  [[nodiscard]] static NodePtr try_absorb_left(const NodePtr &node,
                                               const SmallVector<Char, LeafSize> &extra)
  {
    if (node == nullptr)
      return nullptr;
    if (node->is_leaf())
      {
        if (node->length + extra.size() > LeafSize)
          return nullptr;
        // No `reserve()`: see the matching note in `try_absorb_right`.
        SmallVector<Char, LeafSize> merged;
        merged.append_range(extra.data(), extra.size());
        merged.append_range(node->leaf_data->data(), node->leaf_data->size());
        return make_leaf(std::move(merged));
      }
    NodePtr new_left = try_absorb_left(node->left, extra);
    if (new_left == nullptr)
      return nullptr;
    // See the matching check/comment in `make_internal_node`.
    ah_overflow_error_if(node->length > std::numeric_limits<size_t>::max() - extra.size())
      << "Rope: concatenated length would overflow size_t";
    auto result = std::make_shared<Node>();
    result->length = node->length + extra.size();
    result->depth = node->depth;
    result->left = std::move(new_left);
    result->right = node->right;
    return result;
  }

  /// Concatenate two (possibly null/empty) subtrees.
  [[nodiscard]] static NodePtr concat_nodes(NodePtr left, NodePtr right)
  {
    if (left == nullptr)
      return right;
    if (right == nullptr)
      return left;

    // Fast path for the common "append/prepend one small piece at a
    // time" pattern (e.g. one character per loop iteration): if one side
    // is a single leaf, try to absorb it into the adjacent leaf on the
    // other side instead of growing the tree by one level. This touches
    // only the O(depth) nodes on that spine, shares everything else, and
    // -- because it never changes any depth -- never triggers a
    // rebalance. Falls through to the general case if there is no spare
    // room (or neither side is a lone leaf).
    if (right->is_leaf())
      if (NodePtr absorbed = try_absorb_right(left, *right->leaf_data))
        return absorbed;
    if (left->is_leaf())
      if (NodePtr absorbed = try_absorb_left(right, *left->leaf_data))
        return absorbed;

    return make_internal(std::move(left), std::move(right));
  }

  /// Generous upper bound on the depth a subtree of the given `length`
  /// should have if reasonably balanced -- see the file-level
  /// "Rebalancing" note for why this (rather than the classical
  /// Fibonacci-threshold test) was chosen for this first version.
  [[nodiscard]] static bool is_balanced(const Node *node) noexcept
  {
    // No separate `depth <= 2` early return: the formula below already
    // evaluates to `true` for any `depth` up to at least 8 (its minimum
    // possible value, at `length == 0`), so it subsumes that case.
    const unsigned bits = std::bit_width(node->length);
    return node->depth <= 2 * static_cast<size_t>(bits) + 8;
  }

  static void collect_leaves(const NodePtr &node, Array<NodePtr> &out)
  {
    if (node->is_leaf())
      {
        out.append(node);
        return;
      }
    collect_leaves(node->left, out);
    collect_leaves(node->right, out);
  }

  /// Read-only counterpart of `collect_leaves`: collects raw, non-owning
  /// `const Node *` instead of `NodePtr` (`shared_ptr<const Node>`).
  /// `operator==` is the only caller -- it only ever *reads* leaf
  /// character data during the comparison, never needs to extend any
  /// node's lifetime beyond it (the two ropes' own `root_`/`other.root_`
  /// already keep every visited node alive for the whole call), so
  /// collecting owning `shared_ptr`s there would pay for an atomic
  /// refcount increment and decrement per leaf for no benefit -- real
  /// traffic for a rope with many leaves. `maybe_rebalance` still uses
  /// the owning `collect_leaves` above, since it genuinely needs the
  /// `NodePtr`s themselves as the rebuilt tree's new owning children.
  static void collect_leaf_pointers(const Node *node, Array<const Node *> &out)
  {
    if (node->is_leaf())
      {
        out.append(node);
        return;
      }
    collect_leaf_pointers(node->left.get(), out);
    collect_leaf_pointers(node->right.get(), out);
  }

  [[nodiscard]] static NodePtr build_balanced_from_leaves(const Array<NodePtr> &leaves,
                                                          const size_t lo, const size_t hi)
  {
    if (hi - lo == 1)
      return leaves[lo];
    const size_t mid = lo + (hi - lo) / 2;
    NodePtr left = build_balanced_from_leaves(leaves, lo, mid);
    NodePtr right = build_balanced_from_leaves(leaves, mid, hi);
    return make_internal_node(std::move(left), std::move(right));
  }

  [[nodiscard]] static NodePtr maybe_rebalance(NodePtr node)
  {
    if (is_balanced(node.get()))
      return node;
    Array<NodePtr> leaves;
    // Estimated leaf count, computed in O(1): `length / LeafSize` is
    // exact when every leaf is full, which is the common case (the
    // leaf-absorption fast path keeps leaves packed). Reserving this
    // avoids Array's geometric-growth reallocations in that common case
    // without over-allocating -- an earlier version of this reservation
    // used `length` itself as the bound (technically valid: no leaf is
    // ever empty, so leaf count can't exceed it), but that is loose by
    // up to a factor of `LeafSize` whenever leaves are reasonably full,
    // and measurably regressed performance (over-reserving dominated the
    // reallocations it was meant to avoid). For a pathologically
    // fragmented tree (many near-empty leaves) this estimate undercounts
    // and `Array` still grows geometrically from here -- no worse than
    // not reserving at all.
    leaves.reserve(node->length / LeafSize + 1);
    collect_leaves(node, leaves);
    return build_balanced_from_leaves(leaves, 0, leaves.size());
  }

  /// Build a balanced tree of leaves directly from a flat view, splitting
  /// at the midpoint recursively. Used by the constructor: since it never
  /// produces an unbalanced intermediate node, no `maybe_rebalance()` call
  /// is needed here.
  [[nodiscard]] static NodePtr build_from_view(const View v)
  {
    if (v.size() <= LeafSize)
      {
        // No `reserve()` here: `v.size() <= LeafSize` is exactly
        // `SmallVector<Char, LeafSize>`'s inline capacity, so it can never
        // reallocate regardless. `View` (`basic_string_view<Char>`) is
        // contiguous, so `append_range` can copy it in one `memcpy`
        // instead of one placement-new call per character.
        SmallVector<Char, LeafSize> data;
        data.append_range(v.data(), v.size());
        return make_leaf(std::move(data));
      }
    const size_t mid = v.size() / 2;
    NodePtr left = build_from_view(v.substr(0, mid));
    NodePtr right = build_from_view(v.substr(mid));
    return make_internal_node(std::move(left), std::move(right));
  }

  [[nodiscard]] static const Char &char_at(const Node *node, size_t pos) noexcept
  {
    while (not node->is_leaf())
      if (const size_t left_len = node->left->length; pos < left_len)
        node = node->left.get();
      else
        {
          pos -= left_len;
          node = node->right.get();
        }
    // Unchecked access: `pos` is guaranteed in range by the descent above
    // (and, transitively, by `at()`'s own bounds check before calling
    // here). Using the checked `operator[]` would make this `noexcept`
    // function capable of calling `std::terminate()` on `std::out_of_range`
    // instead of safely returning -- and would repeat a bounds check that
    // can never fail.
    return (*node->leaf_data)(pos);
  }

  /// Extract `[pos, pos+len)` from `node` (whose own length is assumed to
  /// be `>= pos+len`) as a new, independently-rooted subtree. Whole
  /// children are returned by shared_ptr copy (O(1), no data touched);
  /// only children straddling the `[pos, pos+len)` boundary are recursed
  /// into.
  [[nodiscard]] static NodePtr slice(const NodePtr &node, size_t pos, size_t len)
  {
    if (len == 0)
      return nullptr;

    // Whole-node request: share `node` itself, regardless of whether it is
    // a leaf or an internal node. Without this check here, an internal
    // node's own "whole subtree" case fell through to the straddle branch
    // below, which slices both children (trivially, via the same
    // fast path) and then *rejoins* them with `concat_nodes` -- producing
    // a freshly-built node with identical content instead of sharing
    // `node`. That defeated the O(1) sharing `substr(0, size())` is
    // documented to provide for internal-rooted ropes.
    if (pos == 0 and len == node->length)
      return node;

    if (node->is_leaf())
      {
        // No `reserve()`: `len < node->length <= LeafSize` here (the
        // `len == node->length` case already returned above), so this
        // always stays within `SmallVector`'s inline capacity.
        // `append_range` copies the `[pos, pos+len)` subrange via one
        // `memcpy` instead of one placement-new call per character.
        SmallVector<Char, LeafSize> data;
        data.append_range(node->leaf_data->data() + pos, len);
        return make_leaf(std::move(data));
      }

    const size_t left_len = node->left->length;

    if (pos + len <= left_len)
      {
        if (pos == 0 and len == left_len)
          return node->left;
        return slice(node->left, pos, len);
      }

    if (pos >= left_len)
      {
        const size_t right_pos = pos - left_len;
        if (right_pos == 0 and len == node->right->length)
          return node->right;
        return slice(node->right, right_pos, len);
      }

    // Straddles both children: take the tail of `left` and the head of
    // `right`, then rejoin (may trigger a rebalance).
    const size_t left_part_len = left_len - pos;
    NodePtr left_part = slice(node->left, pos, left_part_len);
    NodePtr right_part = slice(node->right, 0, len - left_part_len);
    return concat_nodes(std::move(left_part), std::move(right_part));
  }

  static void flatten_into(const Node *node, Array<Char> &out)
  {
    if (node == nullptr)
      return;
    if (node->is_leaf())
      {
        for (const Char &c : *node->leaf_data)
          out.append(c);
        return;
      }
    flatten_into(node->left.get(), out);
    flatten_into(node->right.get(), out);
  }

  /// Like `flatten_into`, but appends directly into a `std::basic_string`
  /// (one bulk `append` per leaf) instead of an `Array<Char>`, so
  /// `to_string()` does not need a separate `Array<Char>` -> `string` copy
  /// pass.
  static void string_append_into(const Node *node, std::basic_string<Char> &out)
    requires RopeCharTraitsCompatible<Char>
  {
    if (node == nullptr)
      return;
    if (node->is_leaf())
      {
        out.append(node->leaf_data->data(), node->leaf_data->size());
        return;
      }
    string_append_into(node->left.get(), out);
    string_append_into(node->right.get(), out);
  }

  /// Recursive structural-invariant check backing the public `verify()`.
  /// Checks, at every node: a leaf has a non-null `leaf_data` whose
  /// `length` matches its stored data and is in `[1, LeafSize]` (leaves
  /// are never empty -- no code path ever builds one); an internal node
  /// holds no `leaf_data` (see the `Node::leaf_data` comment: only
  /// leaves ever pay for a buffer) and has two non-null children,
  /// `length` equal to the sum of its children's, and `depth` equal to
  /// `1 + max(child depths)`.
  [[nodiscard]] static bool verify_rec(const Node *node) noexcept
  {
    if (node == nullptr)
      return true;
    if (node->is_leaf())
      return node->leaf_data != nullptr and node->leaf_data->size() == node->length and
        node->length >= 1 and node->length <= LeafSize and node->depth == 0;
    if (node->leaf_data != nullptr)
      return false;
    if (node->left == nullptr or node->right == nullptr)
      return false;
    if (node->length != node->left->length + node->right->length)
      return false;
    if (node->depth != 1 + std::max(node->left->depth, node->right->depth))
      return false;
    return verify_rec(node->left.get()) and verify_rec(node->right.get());
  }

  explicit Rope(NodePtr root) noexcept : root_(std::move(root)) {}

public:
  /** @brief Construct the empty rope.
   *  @throws Nothing.
   */
  Rope() noexcept = default;

  /** @brief Construct a rope holding a copy of `v`'s characters.
   *  @param[in] v Characters to copy in.
   *  @throws std::bad_alloc.
   */
  explicit Rope(const View v) : root_(v.empty() ? nullptr : build_from_view(v)) {}

  /** @brief Copy constructor: O(1), shares the entire tree (immutable, so
   *         sharing is always safe).
   *  @param[in] other Rope to copy.
   *  @throws Nothing.
   */
  Rope(const Rope &other) = default;
  /** @brief Copy assignment operator: O(1), shares the entire tree.
   *  @param[in] other Rope to copy.
   *  @return `*this`, now sharing `other`'s tree.
   *  @throws Nothing.
   */
  Rope &operator = (const Rope &other) = default;
  /** @brief Move constructor.
   *  @param[in] other Rope to move from; left empty afterwards.
   *  @throws Nothing.
   */
  Rope(Rope &&other) noexcept = default;
  /** @brief Move assignment operator.
   *  @param[in] other Rope to move from; left empty afterwards.
   *  @return `*this`, now holding `other`'s former tree.
   *  @throws Nothing.
   */
  Rope &operator = (Rope &&other) noexcept = default;
  /// @brief Destructor: releases this rope's reference to its tree;
  /// nodes are only actually freed once no rope shares them anymore.
  ~Rope() = default;

  /** @brief Return the number of characters in this rope.
   *  @return Character count.
   *  @throws Nothing.
   */
  [[nodiscard]] size_t size() const noexcept
  {
    return root_ == nullptr ? 0 : root_->length;
  }

  /** @brief Check whether this rope holds no characters.
   *  @return `true` if `size() == 0`.
   *  @throws Nothing.
   */
  [[nodiscard]] bool is_empty() const noexcept
  {
    return root_ == nullptr;
  }

  /** @brief Return the character at `pos`.
   *  @param[in] pos Zero-based index.
   *  @return The character at `pos`.
   *  @pre `pos < size()`.
   *  @throws std::out_of_range if `pos >= size()`.
   *  @note O(depth), and depth is O(log size()) per the rebalancing
   *        contract.
   */
  [[nodiscard]] Char at(const size_t pos) const
  {
    ah_out_of_range_error_if(pos >= size()) << "Rope::at(): index out of range";
    return char_at(root_.get(), pos);
  }

  /** @brief Return a new rope that is `*this` followed by `other`.
   *  @param[in] other Rope to append.
   *  @return The concatenated rope. Neither `*this` nor `other` is
   *          modified.
   *  @throws std::bad_alloc.
   *  @throws std::overflow_error if `size() + other.size()` would exceed
   *          `std::numeric_limits<size_t>::max()`. Not just a theoretical
   *          concern: structural sharing makes this reachable with very
   *          little real memory, e.g. `r = r.concat(r)` doubles `size()`
   *          each call at O(1) real cost, so ~64 such calls reach the
   *          limit. Checked before the addition, never wraps silently.
   *  @note O(1) when neither side is a lone leaf. Otherwise O(depth +
   *        LeafSize) for the leaf-absorption fast path -- the `O(depth)`
   *        spine walk plus an O(LeafSize) copy of the leaf being grown, so
   *        a larger `LeafSize` makes each such `concat` proportionally
   *        more expensive, not cheaper -- plus the (amortized, occasional)
   *        cost of rebalancing; see the file-level "Rebalancing" and "Leaf
   *        absorption" notes.
   */
  [[nodiscard]] Rope concat(const Rope &other) const
  {
    return Rope(concat_nodes(root_, other.root_));
  }

  /** @brief Return a new rope holding `[pos, pos+len)` of `*this`.
   *  @param[in] pos Start index.
   *  @param[in] len Number of characters to extract.
   *  @return The extracted subrope.
   *  @pre `pos <= size() and len <= size() - pos`.
   *  @throws std::out_of_range if `pos + len > size()` (or `pos > size()`).
   *  @note O(log size()) typical case: whole shared subtrees are reused,
   *        only the boundary leaves are copied/sliced. A range that
   *        straddles a node's children is rejoined via `concat_nodes`,
   *        which -- like any other `concat` -- can trigger a rebalance if
   *        the rejoined result exceeds the depth threshold; when that
   *        happens the affected rejoin costs up to `O(number of leaves in
   *        the extracted range)` instead of `O(1)`, so the true worst
   *        case is closer to `O(len / LeafSize)` than `O(log size())`.
   *        Still far cheaper than copying the whole rope, but not a hard
   *        `O(log n)` guarantee for every possible range.
   */
  [[nodiscard]] Rope substr(const size_t pos, const size_t len) const
  {
    ah_out_of_range_error_if(pos > size() or len > size() - pos)
      << "Rope::substr(): range out of bounds";
    return Rope(slice(root_, pos, len));
  }

  /** @brief Return a new rope with `other` inserted at `pos`.
   *  @param[in] pos Insertion index.
   *  @param[in] other Rope to insert.
   *  @return `substr(0, pos) + other + substr(pos, size() - pos)`.
   *  @pre `pos <= size()`.
   *  @throws std::out_of_range if `pos > size()`.
   *  @note O(1) when `other` is empty: the current rope is returned by
   *        sharing its root. Otherwise this is two `substr` calls plus two
   *        `concat` calls: see each method's own `@note` for their
   *        individual costs -- typically `O(log size())` overall, subject
   *        to the same rebalance-triggered worst case `substr` documents.
   *        If this throws (`std::bad_alloc` from any component call), `*this`
   *        and `other` are left completely unchanged: every intermediate
   *        result here is a new, independent `Rope` value, so a failure
   *        partway through never touches the receiver or the argument.
   */
  [[nodiscard]] Rope insert(const size_t pos, const Rope &other) const
  {
    ah_out_of_range_error_if(pos > size()) << "Rope::insert(): index out of range";
    if (other.is_empty())
      return *this;
    return substr(0, pos).concat(other).concat(substr(pos, size() - pos));
  }

  /** @brief Return a new rope with `[pos, pos+len)` removed.
   *  @param[in] pos Start index.
   *  @param[in] len Number of characters to remove.
   *  @return `substr(0, pos) + substr(pos+len, size()-pos-len)`.
   *  @pre `pos <= size() and len <= size() - pos`.
   *  @throws std::out_of_range if `pos + len > size()`.
   *  @note O(1) when `len == 0`: the current rope is returned by sharing
   *        its root. Otherwise this is two `substr` calls plus one
   *        `concat`: see each method's own `@note` for their individual
   *        costs -- typically `O(log size())` overall, subject to the same
   *        rebalance-triggered worst case `substr` documents. Same strong
   *        exception-safety argument as `insert()`: a failure partway
   *        through leaves `*this` untouched.
   */
  [[nodiscard]] Rope erase(const size_t pos, const size_t len) const
  {
    ah_out_of_range_error_if(pos > size() or len > size() - pos)
      << "Rope::erase(): range out of bounds";
    if (len == 0)
      return *this;
    return substr(0, pos).concat(substr(pos + len, size() - pos - len));
  }

  /** @brief Return every character of this rope as an independent Array.
   *  @return `Array<Char>` holding a copy of all characters, in order.
   *  @throws std::bad_alloc.
   *  @note O(size()). Works for any `Char`, unlike `to_string()`.
   */
  [[nodiscard]] Array<Char> flatten() const
  {
    Array<Char> result;
    result.reserve(size());
    flatten_into(root_.get(), result);
    return result;
  }

  /** @brief Return every character of this rope as a `std::basic_string`.
   *  @return `std::basic_string<Char>` holding a copy of all characters.
   *  @throws std::bad_alloc.
   *  @note O(size()). Only available when `Char` has a
   *        `std::char_traits` specialization (`char`, `wchar_t`,
   *        `char8_t`, `char16_t`, `char32_t`); use `flatten()` for other
   *        `Char` types.
   */
  [[nodiscard]] std::basic_string<Char> to_string() const
    requires RopeCharTraitsCompatible<Char>
  {
    std::basic_string<Char> result;
    result.reserve(size());
    string_append_into(root_.get(), result);
    return result;
  }

  /** @brief Equality: `true` iff both ropes have the same length and the
   *         same characters in the same order (structural/identity
   *         sharing is irrelevant to this comparison).
   *  @param[in] other Rope to compare against.
   *  @return `true` if the two ropes hold identical character sequences.
   *  @throws Nothing beyond what `Char::operator==` throws (never, for
   *          built-in character types).
   *  @note O(size()): collects each side's leaves once (`O(number of
   *        leaves)` extra space) and then walks both leaf sequences in
   *        lock-step, comparing each pair of overlapping ranges directly
   *        -- never re-descending from the root the way per-index `at()`
   *        calls would. A leaf pair that is the very same shared node
   *        (structural sharing, aligned leaf boundaries) is skipped
   *        without touching its characters at all.
   */
  [[nodiscard]] bool operator == (const Rope &other) const
  {
    if (size() != other.size())
      return false;
    if (root_ == other.root_)
      return true;  // same shared tree (or both empty): trivially equal.

    Array<const Node *> mine, theirs;
    // Same reserve rationale as maybe_rebalance(): `length / LeafSize` is
    // exact when leaves are packed (the common case), avoiding Array's
    // geometric-growth reallocations without over-allocating.
    mine.reserve(size() / LeafSize + 1);
    theirs.reserve(size() / LeafSize + 1);
    collect_leaf_pointers(root_.get(), mine);
    collect_leaf_pointers(other.root_.get(), theirs);

    size_t i = 0, j = 0, i_off = 0, j_off = 0;
    while (i < mine.size())
      {
        if (i_off == 0 and j_off == 0 and mine[i] == theirs[j])
          {
            // Same shared leaf node on both sides: identical by
            // construction (Rope never mutates a leaf in place), skip
            // straight to the next pair without comparing characters.
            ++i;
            ++j;
            continue;
          }
        const Node &a = *mine[i];
        const Node &b = *theirs[j];
        const size_t n = std::min(a.length - i_off, b.length - j_off);
        // std::equal over a raw pointer range (rather than a hand-rolled
        // per-index loop) gives the compiler a standard, recognizable
        // pattern -- for `Char` types where equality is the built-in
        // one, this is commonly folded into a single memcmp-style bulk
        // comparison instead of one function-call-per-character.
        const Char *pa = a.leaf_data->data() + i_off;
        const Char *pb = b.leaf_data->data() + j_off;
        if (not std::equal(pa, pa + n, pb))
          return false;
        i_off += n;
        j_off += n;
        if (i_off == a.length)
          {
            ++i;
            i_off = 0;
          }
        if (j_off == b.length)
          {
            ++j;
            j_off = 0;
          }
      }
    return true;
  }

  /** @brief Check this rope's internal structural invariants.
   *
   *  Debug/test helper, not part of the regular contract -- mirrors the
   *  `verify()`/`check_invariants()` self-checks other Aleph-w tree types
   *  expose (`tpl_radix_tree.H`, `tpl_patricia_trie.H`, `tpl_rb_tree.H`,
   *  `tpl_avl.H`). Content-only tests (comparing `to_string()` against a
   *  reference) cannot detect a violation of these invariants, since none
   *  of them are individually observable through the public API; calling
   *  `verify()` after mutating operations in a test gives that coverage
   *  directly.
   *
   *  @return `true` if every leaf's length matches its stored data and is
   *          in `[1, LeafSize]`; every internal node has two non-null
   *          children; every internal node's `length` equals the sum of
   *          its children's; and every internal node's `depth` equals
   *          `1 + max(child depths)`.
   *  @throws Nothing.
   *  @note O(number of *distinct* nodes) for an ordinarily-built rope.
   *        Does not deduplicate shared subtrees while recursing, so for a
   *        rope built by extreme self-sharing (e.g. repeated `r =
   *        r.concat(r)`) it walks every root-to-leaf *path* rather than
   *        every distinct node -- up to `O(2^depth)` instead of `O(depth)`
   *        for that specific construction pattern. Not a concern for
   *        ordinary usage (sharing between otherwise-independent ropes,
   *        the common case) -- only for a rope that shares itself with
   *        itself many levels deep.
   */
  [[nodiscard]] bool verify() const noexcept
  {
    return verify_rec(root_.get());
  }
};

}  // namespace Aleph

#endif  // TPL_ROPE_H
