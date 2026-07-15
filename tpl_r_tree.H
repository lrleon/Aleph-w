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

/** @file tpl_r_tree.H
 *  @brief Dynamic R-tree spatial index over axis-aligned rectangles.
 *
 *  `RTree<Payload, MaxEntries, MinEntries>` is a dynamic spatial index that
 *  associates axis-aligned bounding boxes (`Aleph::Rectangle`) with user
 *  payloads and supports incremental `insert` / `erase` plus intersection and
 *  point-containment queries. It is Guttman's R-tree with the quadratic node
 *  split heuristic.
 *
 *  Unlike the static `AABBTree` (in `geom_algorithms.H`), which is built once
 *  from a fixed batch of boxes, `RTree` may be updated in place after
 *  construction. `AABBTree` is likely faster for immutable batches; `RTree`
 *  targets workloads that insert and delete over time.
 *
 *  @par Complexity
 *  With well-distributed data, `insert`, `erase` and point queries are expected
 *  O(log n). Overlapping-region queries cost O(log n + k) for k reported
 *  entries, degrading to O(n) when many node bounding boxes overlap the query.
 *  `height()` and `size()` are O(1).
 *
 *  @par Exception safety
 *  All operations use exact rational coordinates (`Aleph::Geom_Number`), so no
 *  floating-point rounding affects the structure. Allocation failure throws
 *  `std::bad_alloc`; size overflow throws `std::overflow_error`. Mutating
 *  operations are not transactional: they do not provide the strong guarantee
 *  after tree mutation has started, and exceptions from `Payload` operations
 *  propagate according to the payload type's own guarantees. Public
 *  preconditions are checked with the macros of `ah-errors.H`.
 *
 *  @par Thread safety
 *  `RTree` is a sequential container: it is not internally synchronized.
 *  Concurrent read-only queries on a tree that no one is mutating are safe;
 *  any concurrent mutation requires external synchronization.
 *
 *  @par Reference/iterator invalidation
 *  The tree exposes no live iterators. `search_intersects` and
 *  `search_contains` return independent `Array` copies. References passed to
 *  `for_each_intersecting` are valid only during the callback invocation. Any
 *  `insert`, `erase` or `clear` invalidates every previously obtained
 *  reference into the tree.
 *
 *  @ingroup Geometry
 */

#ifndef TPL_R_TREE_H
#define TPL_R_TREE_H

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#include <ah-errors.H>
#include <point.H>
#include <tpl_array.H>

namespace Aleph {

/** @brief Node-split / insertion strategy for `RTree`.
 *
 *  `Guttman` uses the original quadratic split. `RStar` uses the R*-tree
 *  heuristics (overlap-minimizing subtree choice, margin/overlap/area split and
 *  leaf-level forced reinsertion). See `RStarTree` in `tpl_r_star_tree.H`.
 */
enum class RTreeVariant
{
  Guttman,
  RStar
};

/** @brief Dynamic R-tree indexing axis-aligned rectangles by payload.
 *
 *  @tparam Payload Value associated with each stored bounding box. Must be
 *          default-initializable and movable. Copying APIs additionally require
 *          copy construction.
 *  @tparam MaxEntries Maximum number of entries per node. Must be at least 2.
 *  @tparam MinEntries Minimum number of entries per non-root node. Must be at
 *          least 1 and satisfy `2 * MinEntries <= MaxEntries + 1`. Defaults to
 *          `MaxEntries / 2`.
 *  @tparam Variant Node-split/insertion strategy (@ref RTreeVariant). Defaults
 *          to Guttman's quadratic split; `RStar` selects the R*-tree heuristics.
 */
template <typename Payload, size_t MaxEntries = 16, size_t MinEntries = MaxEntries / 2,
          RTreeVariant Variant = RTreeVariant::Guttman>
class RTree
{
  static_assert(MaxEntries >= 2, "RTree requires MaxEntries >= 2");
  static_assert(MinEntries >= 1, "RTree requires MinEntries >= 1");
  static_assert(2 * MinEntries <= MaxEntries + 1, "RTree requires 2 * MinEntries <= MaxEntries + 1");
  static_assert(std::default_initializable<Payload>,
                "RTree requires default-initializable Payload");
  static_assert(std::movable<Payload>, "RTree requires movable Payload");

public:
  /** @brief A stored (bounding box, payload) pair. */
  struct Entry
  {
    Rectangle bbox;  ///< Axis-aligned bounding box.
    Payload value;   ///< User payload associated with @ref bbox.
  };

private:
  struct Node;

  /// @brief Internal-node entry: a child subtree plus its tight bounding box.
  struct Child
  {
    Rectangle bbox;               ///< Tight MBR of @ref child.
    std::unique_ptr<Node> child;  ///< Owned child subtree.
  };

  /// @brief A node is a leaf holding @ref data or an internal node holding @ref children.
  struct Node
  {
    bool leaf = true;
    Array<Entry> data;      ///< Data entries (used iff @ref leaf).
    Array<Child> children;  ///< Child entries (used iff not @ref leaf).
  };

  std::unique_ptr<Node> root_;
  size_t size_ = 0;    ///< Number of stored data entries.
  size_t height_ = 0;  ///< Number of node levels (0 when empty).

  // R*-only transient state (empty/false while no insertion is in progress).
  bool rstar_reinsert_available_ = false;  ///< Leaf-level reinsert not yet used this insert.
  Array<Entry> rstar_reinsert_buffer_;     ///< Entries pending forced reinsertion.

  // ---- geometry helpers -------------------------------------------------

  [[nodiscard]] static Rectangle union_bbox(const Rectangle &a, const Rectangle &b)
  {
    const Geom_Number xmin = a.get_xmin() < b.get_xmin() ? a.get_xmin() : b.get_xmin();
    const Geom_Number ymin = a.get_ymin() < b.get_ymin() ? a.get_ymin() : b.get_ymin();
    const Geom_Number xmax = a.get_xmax() > b.get_xmax() ? a.get_xmax() : b.get_xmax();
    const Geom_Number ymax = a.get_ymax() > b.get_ymax() ? a.get_ymax() : b.get_ymax();
    return {xmin, ymin, xmax, ymax};
  }

  /// @brief Area added to @p base by growing it to also cover @p added.
  [[nodiscard]] static Geom_Number enlargement(const Rectangle &base, const Rectangle &added)
  {
    return union_bbox(base, added).area() - base.area();
  }

  /// @brief Area of the overlap between two rectangles (0 if disjoint).
  [[nodiscard]] static Geom_Number overlap_area(const Rectangle &a, const Rectangle &b)
  {
    const Geom_Number xlo = a.get_xmin() > b.get_xmin() ? a.get_xmin() : b.get_xmin();
    const Geom_Number xhi = a.get_xmax() < b.get_xmax() ? a.get_xmax() : b.get_xmax();
    if (xhi <= xlo)
      return Geom_Number(0);
    const Geom_Number ylo = a.get_ymin() > b.get_ymin() ? a.get_ymin() : b.get_ymin();
    const Geom_Number yhi = a.get_ymax() < b.get_ymax() ? a.get_ymax() : b.get_ymax();
    if (yhi <= ylo)
      return Geom_Number(0);
    return (xhi - xlo) * (yhi - ylo);
  }

  [[nodiscard]] static size_t entry_count(const Node &node)
  {
    return node.leaf ? node.data.size() : node.children.size();
  }

  /// @brief Tight bounding box covering every entry of @p node.
  [[nodiscard]] static Rectangle compute_mbr(const Node &node)
  {
    if (node.leaf)
      {
        Rectangle acc = node.data(0).bbox;
        for (size_t i = 1; i < node.data.size(); ++i)
          acc = union_bbox(acc, node.data(i).bbox);
        return acc;
      }
    Rectangle acc = node.children(0).bbox;
    for (size_t i = 1; i < node.children.size(); ++i)
      acc = union_bbox(acc, node.children(i).bbox);
    return acc;
  }

  /// @brief Choose the child of @p node into which @p bbox should be inserted.
  ///        R*-tree minimizes overlap enlargement when the children are leaves.
  [[nodiscard]] static size_t choose_subtree(const Node &node, const Rectangle &bbox)
  {
    if constexpr (Variant == RTreeVariant::RStar)
      if (node.children(0).child->leaf)
        return rstar_choose_overlap(node, bbox);

    size_t best = 0;
    Geom_Number best_enlarge = enlargement(node.children(0).bbox, bbox);
    Geom_Number best_area = node.children(0).bbox.area();
    for (size_t i = 1; i < node.children.size(); ++i)
      {
        const Geom_Number e = enlargement(node.children(i).bbox, bbox);
        const Geom_Number a = node.children(i).bbox.area();
        if (e < best_enlarge or (e == best_enlarge and a < best_area))
          {
            best = i;
            best_enlarge = e;
            best_area = a;
          }
      }
    return best;
  }

  /// @brief R*-tree ChooseSubtree for a node whose children are leaves: pick the
  ///        child whose growth adds the least overlap with its siblings (ties:
  ///        least area enlargement, then least area).
  [[nodiscard]] static size_t rstar_choose_overlap(const Node &node, const Rectangle &bbox)
  {
    const size_t k = node.children.size();
    size_t best = 0;
    bool first = true;
    Geom_Number best_ov = 0;
    Geom_Number best_enl = 0;
    Geom_Number best_area = 0;
    for (size_t i = 0; i < k; ++i)
      {
        const Rectangle grown = union_bbox(node.children(i).bbox, bbox);
        Geom_Number ov = 0;
        for (size_t j = 0; j < k; ++j)
          if (j != i)
            ov += overlap_area(grown, node.children(j).bbox) -
              overlap_area(node.children(i).bbox, node.children(j).bbox);
        const Geom_Number enl = grown.area() - node.children(i).bbox.area();
        const Geom_Number area = node.children(i).bbox.area();
        if (first or ov < best_ov or
            (ov == best_ov and (enl < best_enl or (enl == best_enl and area < best_area))))
          {
            best = i;
            best_ov = ov;
            best_enl = enl;
            best_area = area;
            first = false;
          }
      }
    return best;
  }

  // ---- quadratic split (Guttman) ----------------------------------------

  /// @brief Split an overflowed entry array in two using the quadratic
  ///        heuristic. @p entries keeps the first group; the second group is
  ///        returned. @tparam EntryT is either @ref Entry or @ref Child.
  template <typename EntryT>
  [[nodiscard]] static Array<EntryT> quadratic_split(Array<EntryT> &entries)
  {
    const size_t n = entries.size();
    Array<EntryT> group1;
    Array<EntryT> group2;
    group1.reserve(n);
    group2.reserve(n);

    Array<EntryT> items = std::move(entries);
    entries = Array<EntryT>();

    // PickSeeds: the pair wasting the most area if grouped together.
    size_t seed1 = 0;
    size_t seed2 = 1;
    Geom_Number worst =
      union_bbox(items(0).bbox, items(1).bbox).area() - items(0).bbox.area() - items(1).bbox.area();
    for (size_t i = 0; i < n; ++i)
      for (size_t j = i + 1; j < n; ++j)
        {
          const Geom_Number d = union_bbox(items(i).bbox, items(j).bbox).area() -
            items(i).bbox.area() - items(j).bbox.area();
          if (d > worst)
            {
              worst = d;
              seed1 = i;
              seed2 = j;
            }
        }

    std::array<bool, MaxEntries + 1> assigned{};
    Rectangle mbr1 = items(seed1).bbox;
    Rectangle mbr2 = items(seed2).bbox;
    assigned[seed1] = true;
    assigned[seed2] = true;
    group1.append(std::move(items(seed1)));
    group2.append(std::move(items(seed2)));
    size_t remaining = n - 2;

    auto dump_into = [&](Array<EntryT> &group)
      {
        for (size_t k = 0; k < n; ++k)
          if (not assigned[k])
            {
              assigned[k] = true;
              group.append(std::move(items(k)));
            }
        remaining = 0;
      };

    while (remaining > 0)
      {
        // Force the rest into a group that would otherwise miss MinEntries.
        if (group1.size() + remaining == MinEntries)
          {
            dump_into(group1);
            break;
          }
        if (group2.size() + remaining == MinEntries)
          {
            dump_into(group2);
            break;
          }

        // PickNext: entry with the strongest preference for one group.
        size_t pick = n;
        Geom_Number best_diff = 0;
        Geom_Number pick_d1 = 0;
        Geom_Number pick_d2 = 0;
        for (size_t k = 0; k < n; ++k)
          {
            if (assigned[k])
              continue;
            const Geom_Number d1 = enlargement(mbr1, items(k).bbox);
            const Geom_Number d2 = enlargement(mbr2, items(k).bbox);
            const Geom_Number diff = d1 > d2 ? d1 - d2 : d2 - d1;
            if (pick == n or diff > best_diff)
              {
                best_diff = diff;
                pick = k;
                pick_d1 = d1;
                pick_d2 = d2;
              }
          }

        // Assign to the group needing less enlargement (ties: smaller area,
        // then fewer entries).
        bool to_group1;
        if (pick_d1 < pick_d2)
          to_group1 = true;
        else if (pick_d2 < pick_d1)
          to_group1 = false;
        else if (mbr1.area() != mbr2.area())
          to_group1 = mbr1.area() < mbr2.area();
        else
          to_group1 = group1.size() <= group2.size();

        assigned[pick] = true;
        if (to_group1)
          {
            mbr1 = union_bbox(mbr1, items(pick).bbox);
            group1.append(std::move(items(pick)));
          }
        else
          {
            mbr2 = union_bbox(mbr2, items(pick).bbox);
            group2.append(std::move(items(pick)));
          }
        --remaining;
      }

    entries = std::move(group1);
    return group2;
  }

  /// @brief R*-tree split: choose the split axis minimizing the total margin of
  ///        the two groups, then the distribution minimizing their overlap area
  ///        (ties: minimum total area). @p entries keeps the first group; the
  ///        second group is returned.
  template <typename EntryT>
  [[nodiscard]] static Array<EntryT> rstar_split(Array<EntryT> &entries)
  {
    const size_t n = entries.size();
    Array<EntryT> group1;
    Array<EntryT> group2;
    group1.reserve(n);
    group2.reserve(n);

    Array<EntryT> items = std::move(entries);
    entries = Array<EntryT>();

    // prefix/suffix are indexed up to and including n (prefix[n], suffix
    // accessed down to suffix[0]), and n can reach MaxEntries + 1, so both
    // need MaxEntries + 2 slots -- one more than the MaxEntries + 1 used for
    // per-entry arrays like `order`.
    auto make_mbr_cache = [&items, n](const std::array<size_t, MaxEntries + 1> &order)
      {
        std::array<Rectangle, MaxEntries + 2> prefix{};
        std::array<Rectangle, MaxEntries + 2> suffix{};
        prefix[1] = items(order[0]).bbox;
        for (size_t k = 2; k <= n; ++k)
          prefix[k] = union_bbox(prefix[k - 1], items(order[k - 1]).bbox);
        suffix[n - 1] = items(order[n - 1]).bbox;
        for (size_t k = n - 1; k > 0; --k)
          suffix[k - 1] = union_bbox(items(order[k - 1]).bbox, suffix[k]);
        return std::pair{prefix, suffix};
      };

    auto make_order = [&items, n](const int axis, const int edge)
      {
        std::array<size_t, MaxEntries + 1> order{};
        for (size_t i = 0; i < n; ++i)
          order[i] = i;
        std::sort(order.begin(), order.begin() + n,
                  [&items, axis, edge](const size_t a, const size_t b)
          {
            const Rectangle &ra = items(a).bbox;
            const Rectangle &rb = items(b).bbox;
            const Geom_Number la = axis == 0 ? ra.get_xmin() : ra.get_ymin();
            const Geom_Number ua = axis == 0 ? ra.get_xmax() : ra.get_ymax();
            const Geom_Number lb = axis == 0 ? rb.get_xmin() : rb.get_ymin();
            const Geom_Number ub = axis == 0 ? rb.get_xmax() : rb.get_ymax();
            const Geom_Number pa = edge == 0 ? la : ua;
            const Geom_Number pb = edge == 0 ? lb : ub;
            if (pa != pb)
              return pa < pb;
            return (edge == 0 ? ua : la) < (edge == 0 ? ub : lb);
          });
        return order;
      };

    // ChooseSplitAxis: minimize the summed margin over both edge sortings and
    // all valid distributions.
    int chosen_axis = 0;
    Geom_Number best_margin_sum = 0;
    for (int axis = 0; axis < 2; ++axis)
      {
        Geom_Number margin_sum = 0;
        for (int edge = 0; edge < 2; ++edge)
          {
            const std::array<size_t, MaxEntries + 1> order = make_order(axis, edge);
            const auto [prefix, suffix] = make_mbr_cache(order);
            for (size_t sz = MinEntries; sz <= n - MinEntries; ++sz)
              margin_sum += prefix[sz].perimeter() + suffix[sz].perimeter();
          }
        if (axis == 0 or margin_sum < best_margin_sum)
          {
            best_margin_sum = margin_sum;
            chosen_axis = axis;
          }
      }

    // ChooseSplitIndex: on the chosen axis, minimize overlap area, then area.
    std::array<size_t, MaxEntries + 1> best_order{};
    size_t best_sz = MinEntries;
    bool first = true;
    Geom_Number best_overlap = 0;
    Geom_Number best_area = 0;
    for (int edge = 0; edge < 2; ++edge)
      {
        const std::array<size_t, MaxEntries + 1> order = make_order(chosen_axis, edge);
        const auto [prefix, suffix] = make_mbr_cache(order);
        for (size_t sz = MinEntries; sz <= n - MinEntries; ++sz)
          {
            const Rectangle &g1 = prefix[sz];
            const Rectangle &g2 = suffix[sz];
            const Geom_Number ov = overlap_area(g1, g2);
            const Geom_Number ar = g1.area() + g2.area();
            if (first or ov < best_overlap or (ov == best_overlap and ar < best_area))
              {
                first = false;
                best_overlap = ov;
                best_area = ar;
                best_order = order;
                best_sz = sz;
              }
          }
      }

    for (size_t k = 0; k < best_sz; ++k)
      group1.append(std::move(items(best_order[k])));
    for (size_t k = best_sz; k < n; ++k)
      group2.append(std::move(items(best_order[k])));
    entries = std::move(group1);
    return group2;
  }

  // ---- insertion --------------------------------------------------------

  /// @brief Split the entry array of @p node according to the active variant.
  template <typename EntryT>
  [[nodiscard]] static Array<EntryT> split_entries(Array<EntryT> &entries)
  {
    if constexpr (Variant == RTreeVariant::RStar)
      return rstar_split(entries);
    else
      return quadratic_split(entries);
  }

  /// @brief Move the R*-tree forced-reinsert candidates (the entries farthest
  ///        from the node centre) out of the overflowed leaf @p node and into
  ///        @ref rstar_reinsert_buffer_, leaving @p node validly sized.
  void reinsert_farthest(Node &node)
  {
    const size_t n = node.data.size();
    const Point centre = compute_mbr(node).center();

    std::array<size_t, MaxEntries + 1> order{};
    for (size_t i = 0; i < n; ++i)
      order[i] = i;
    std::sort(order.begin(), order.begin() + n, [&node, &centre](const size_t a, const size_t b)
      {
        return node.data(a).bbox.center().distance_squared_to(centre) >
          node.data(b).bbox.center().distance_squared_to(centre);
      });

    size_t p = (3 * MaxEntries) / 10;
    if (p < 1)
      p = 1;
    if (p > MaxEntries + 1 - MinEntries)
      p = MaxEntries + 1 - MinEntries;

    Array<Entry> kept;
    kept.reserve(n - p);
    rstar_reinsert_buffer_.reserve(rstar_reinsert_buffer_.size() + p);
    for (size_t k = p; k < n; ++k)
      kept.append(std::move(node.data(order[k])));
    for (size_t k = 0; k < p; ++k)
      rstar_reinsert_buffer_.append(std::move(node.data(order[k])));
    node.data = std::move(kept);
  }

  /// @brief Insert @p entry at the leaf level of @p node. Returns a new sibling
  ///        node when @p node overflowed and had to be split, else nullptr.
  [[nodiscard]] std::unique_ptr<Node> insert_descend(Node &node, Entry entry)
  {
    if (node.leaf)
      {
        node.data.reserve(MaxEntries + 1);
        node.data.append(std::move(entry));
        if (node.data.size() <= MaxEntries)
          return nullptr;

        if constexpr (Variant == RTreeVariant::RStar)
          if (rstar_reinsert_available_ and &node != root_.get())
            {
              rstar_reinsert_available_ = false;
              reinsert_farthest(node);
              return nullptr;
            }

        Array<Entry> group2 = split_entries(node.data);
        auto sibling = std::make_unique<Node>();
        sibling->leaf = true;
        sibling->data = std::move(group2);
        return sibling;
      }

    const size_t idx = choose_subtree(node, entry.bbox);
    std::unique_ptr<Node> split = insert_descend(*node.children(idx).child, std::move(entry));
    node.children(idx).bbox = compute_mbr(*node.children(idx).child);
    if (split == nullptr)
      return nullptr;

    Rectangle split_mbr = compute_mbr(*split);
    node.children.reserve(MaxEntries + 1);
    node.children.append(Child{std::move(split_mbr), std::move(split)});
    if (node.children.size() <= MaxEntries)
      return nullptr;

    Array<Child> group2 = split_entries(node.children);
    auto sibling = std::make_unique<Node>();
    sibling->leaf = false;
    sibling->children = std::move(group2);
    return sibling;
  }

  /// @brief Insert one data entry into the tree (no @ref size_ change, no
  ///        reinsert-buffer management).
  void insert_one(Entry entry)
  {
    if (root_ == nullptr)
      {
        auto root = std::make_unique<Node>();
        root->leaf = true;
        root->data.reserve(1);
        root->data.append(std::move(entry));
        root_ = std::move(root);
        height_ = 1;
        return;
      }

    std::unique_ptr<Node> split = insert_descend(*root_, std::move(entry));
    if (split == nullptr)
      return;

    auto new_root = std::make_unique<Node>();
    new_root->leaf = false;
    new_root->children.reserve(2);
    new_root->children.append(Child{compute_mbr(*root_), std::move(root_)});
    new_root->children.append(Child{compute_mbr(*split), std::move(split)});
    root_ = std::move(new_root);
    ++height_;
  }

  /// @brief Add a data entry without touching @ref size_ (shared by `insert`
  ///        and by erase-time reinsertion). For the R*-tree variant this also
  ///        drives one round of leaf-level forced reinsertion.
  void add_entry(Entry entry)
  {
    if constexpr (Variant == RTreeVariant::RStar)
      {
        rstar_reinsert_available_ = true;
        insert_one(std::move(entry));
        while (not rstar_reinsert_buffer_.is_empty())
          insert_one(rstar_reinsert_buffer_.remove_last());
      }
    else
      insert_one(std::move(entry));
  }

  // ---- deletion ---------------------------------------------------------

  /// @brief Move every data entry in @p node's subtree into @p out.
  static void collect_data_entries(Node &node, Array<Entry> &out)
  {
    if (node.leaf)
      {
        for (size_t i = 0; i < node.data.size(); ++i)
          out.append(std::move(node.data(i)));
        return;
      }
    for (size_t i = 0; i < node.children.size(); ++i)
      collect_data_entries(*node.children(i).child, out);
  }

  [[nodiscard]] static size_t data_entry_count(const Node &node)
  {
    if (node.leaf)
      return node.data.size();
    size_t total = 0;
    for (size_t i = 0; i < node.children.size(); ++i)
      {
        const size_t subtree_count = data_entry_count(*node.children(i).child);
        ah_overflow_error_if(total > std::numeric_limits<size_t>::max() - subtree_count)
          << "RTree: data entry count would overflow";
        total += subtree_count;
      }
    return total;
  }

  /// @brief Remove one entry equal to (@p bbox, @p value) from @p node's
  ///        subtree. On success sets @p removed and moves the data entries of
  ///        any underflowing descendant into @p orphans (for reinsertion).
  void erase_descend(Node &node, const Rectangle &bbox, const Payload &value, Array<Entry> &orphans,
                     bool &removed)
    requires std::equality_comparable<Payload>
  {
    if (node.leaf)
      {
        for (size_t i = 0; i < node.data.size(); ++i)
          if (node.data(i).bbox == bbox and node.data(i).value == value)
            {
              Array<Entry> kept;
              kept.reserve(node.data.size() - 1);
              for (size_t j = 0; j < node.data.size(); ++j)
                if (j != i)
                  kept.append(std::move(node.data(j)));
              node.data = std::move(kept);
              removed = true;
              return;
            }
        return;
      }

    for (size_t i = 0; i < node.children.size(); ++i)
      {
        Child &c = node.children(i);
        if (not box_contains_box(c.bbox, bbox))
          continue;

        erase_descend(*c.child, bbox, value, orphans, removed);
        if (not removed)
          continue;

        if (entry_count(*c.child) < MinEntries)
          {
            const size_t orphan_count = data_entry_count(*c.child);
            ah_overflow_error_if(orphans.size() >
                                 std::numeric_limits<size_t>::max() - orphan_count)
              << "RTree: orphan collection would overflow";
            orphans.reserve(orphans.size() + orphan_count);
            Array<Child> kept;
            kept.reserve(node.children.size() - 1);
            collect_data_entries(*c.child, orphans);
            for (size_t j = 0; j < node.children.size(); ++j)
              if (j != i)
                kept.append(std::move(node.children(j)));
            node.children = std::move(kept);
          }
        else
          c.bbox = compute_mbr(*c.child);
        return;
      }
  }

  /// @brief True iff @p outer fully covers @p inner.
  [[nodiscard]] static bool box_contains_box(const Rectangle &outer, const Rectangle &inner)
  {
    return outer.get_xmin() <= inner.get_xmin() and outer.get_xmax() >= inner.get_xmax() and
      outer.get_ymin() <= inner.get_ymin() and outer.get_ymax() >= inner.get_ymax();
  }

  // ---- queries ----------------------------------------------------------

  template <typename F>
  void for_each_intersecting_rec(const Node &node, const Rectangle &rect, F &f) const
  {
    if (node.leaf)
      {
        for (size_t i = 0; i < node.data.size(); ++i)
          if (node.data(i).bbox.intersects(rect))
            f(node.data(i).bbox, node.data(i).value);
        return;
      }
    for (size_t i = 0; i < node.children.size(); ++i)
      if (node.children(i).bbox.intersects(rect))
        for_each_intersecting_rec(*node.children(i).child, rect, f);
  }

  template <typename F>
  void for_each_containing_rec(const Node &node, const Point &p, F &f) const
  {
    if (node.leaf)
      {
        for (size_t i = 0; i < node.data.size(); ++i)
          if (node.data(i).bbox.contains(p))
            f(node.data(i).bbox, node.data(i).value);
        return;
      }
    for (size_t i = 0; i < node.children.size(); ++i)
      if (node.children(i).bbox.contains(p))
        for_each_containing_rec(*node.children(i).child, p, f);
  }

  // ---- copy / verify ----------------------------------------------------

  [[nodiscard]] static std::unique_ptr<Node> clone_node(const Node &node)
    requires (std::is_copy_constructible_v<Payload> and std::movable<Payload>)
  {
    auto copy = std::make_unique<Node>();
    copy->leaf = node.leaf;
    if (node.leaf)
      {
        copy->data.reserve(node.data.size());
        for (size_t i = 0; i < node.data.size(); ++i)
          copy->data.append(Entry{node.data(i).bbox, node.data(i).value});
      }
    else
      {
        copy->children.reserve(node.children.size());
        for (size_t i = 0; i < node.children.size(); ++i)
          copy->children.append(Child{node.children(i).bbox, clone_node(*node.children(i).child)});
      }
    return copy;
  }

  [[nodiscard]] bool verify_rec(const Node &node, const size_t depth, const bool is_root,
                                size_t &leaf_depth, size_t &count, Rectangle &out_mbr) const
  {
    const size_t n = entry_count(node);
    if (n > MaxEntries)
      return false;
    if (not is_root and n < MinEntries)
      return false;
    if (n == 0)
      return false;  // an empty node must never be retained

    if (node.leaf)
      {
        if (leaf_depth == std::numeric_limits<size_t>::max())
          leaf_depth = depth;
        else if (leaf_depth != depth)
          return false;
        out_mbr = compute_mbr(node);
        if (count > std::numeric_limits<size_t>::max() - n)
          return false;
        count += n;
        return true;
      }

    if (is_root and n < 2)
      return false;  // a non-leaf root must have at least two children

    Rectangle acc;
    for (size_t i = 0; i < node.children.size(); ++i)
      {
        Rectangle child_mbr;
        if (not verify_rec(*node.children(i).child, depth + 1, false, leaf_depth, count, child_mbr))
          return false;
        if (node.children(i).bbox != child_mbr)
          return false;  // stored MBR must be tight
        acc = i == 0 ? child_mbr : union_bbox(acc, child_mbr);
      }
    out_mbr = acc;
    return true;
  }

public:
  /** @brief Construct an empty R-tree.
   *  @throws Nothing.
   */
  RTree() noexcept = default;

  /** @brief Move-construct from @p other, leaving it empty and valid.
   *  @param other Tree to move from.
   *  @post `other.is_empty()` is `true` and `other` remains a valid,
   *        usable tree (not merely moved-from).
   *  @par Complexity O(1).
   *  @throws Nothing.
   */
  RTree(RTree &&other) noexcept
    : root_(std::move(other.root_)), size_(other.size_), height_(other.height_)
  {
    other.size_ = 0;
    other.height_ = 0;
  }

  /** @brief Move-assign from @p other, leaving it empty and valid.
   *  @param other Tree to move from.
   *  @return Reference to this tree.
   *  @post `other.is_empty()` is `true` and `other` remains a valid,
   *        usable tree (not merely moved-from).
   *  @par Complexity O(1).
   *  @throws Nothing.
   */
  RTree &operator = (RTree &&other) noexcept
  {
    if (this == &other)
      return *this;
    root_ = std::move(other.root_);
    size_ = other.size_;
    height_ = other.height_;
    other.size_ = 0;
    other.height_ = 0;
    return *this;
  }

  /** @brief Deep-copy @p other (requires copy-constructible, movable `Payload`).
   *  @param other Tree to clone.
   *  @throws std::bad_alloc or whatever copying `Payload` throws.
   */
  RTree(const RTree &other)
    requires (std::is_copy_constructible_v<Payload> and std::movable<Payload>)
    : size_(other.size_), height_(other.height_)
  {
    if (other.root_ != nullptr)
      root_ = clone_node(*other.root_);
  }

  /** @brief Deep-copy assign from @p other (requires copy-constructible, movable `Payload`).
   *  @param other Tree to clone.
   *  @return Reference to this tree.
   *  @throws std::bad_alloc or whatever copying `Payload` throws.
   */
  RTree &operator = (const RTree &other)
    requires (std::is_copy_constructible_v<Payload> and std::movable<Payload>)
  {
    if (this == &other)
      return *this;
    std::unique_ptr<Node> new_root;
    if (other.root_ != nullptr)
      new_root = clone_node(*other.root_);
    root_ = std::move(new_root);
    size_ = other.size_;
    height_ = other.height_;
    return *this;
  }

  /** @brief Return true when the tree has no entries.
   *  @return `true` iff `size() == 0`.
   *  @throws Nothing.
   */
  [[nodiscard]] bool is_empty() const noexcept
  {
    return size_ == 0;
  }

  /** @brief Return the number of stored entries.
   *  @return Entry count.
   *  @throws Nothing.
   */
  [[nodiscard]] size_t size() const noexcept
  {
    return size_;
  }

  /** @brief Return the number of node levels (0 when empty, 1 for a lone leaf).
   *  @return Tree height in levels.
   *  @throws Nothing.
   */
  [[nodiscard]] size_t height() const noexcept
  {
    return height_;
  }

  /** @brief Remove all entries.
   *  @throws Nothing.
   */
  void clear() noexcept
  {
    root_.reset();
    size_ = 0;
    height_ = 0;
  }

  /** @brief Insert a (bbox, value) entry, copying @p value.
   *  @param bbox Bounding box of the entry.
   *  @param value Payload to store.
   *  @throws std::bad_alloc if node allocation fails.
   *  @throws std::overflow_error if the stored-entry count would overflow.
   *  @throws Whatever copying `Payload` throws.
   */
  void insert(const Rectangle &bbox, const Payload &value)
  {
    ah_overflow_error_if(size_ == std::numeric_limits<size_t>::max())
      << "RTree: size would overflow";
    add_entry(Entry{bbox, value});
    ++size_;
  }

  /** @brief Insert a (bbox, value) entry, moving @p value.
   *  @param bbox Bounding box of the entry.
   *  @param value Payload to move into the tree.
   *  @throws std::bad_alloc if node allocation fails.
   *  @throws std::overflow_error if the stored-entry count would overflow.
   *  @throws Whatever moving `Payload` throws.
   */
  void insert(const Rectangle &bbox, Payload &&value)
  {
    ah_overflow_error_if(size_ == std::numeric_limits<size_t>::max())
      << "RTree: size would overflow";
    add_entry(Entry{bbox, std::move(value)});
    ++size_;
  }

  /** @brief Remove one entry equal to (@p bbox, @p value).
   *  @param bbox Bounding box of the entry to remove.
   *  @param value Payload to match (by `==`).
   *  @return `true` if an entry was removed, `false` if none matched.
   *  @throws std::bad_alloc if reinsertion of condensed entries allocates.
   *  @note Requires `Payload` to be equality-comparable. Uses a CondenseTree
   *        variant that reinserts the data entries of any underflowing node.
   */
  [[nodiscard]] bool erase(const Rectangle &bbox, const Payload &value)
    requires std::equality_comparable<Payload>
  {
    if (root_ == nullptr)
      return false;

    Array<Entry> orphans;
    bool removed = false;
    erase_descend(*root_, bbox, value, orphans, removed);
    if (not removed)
      return false;

    --size_;

    // Reinsert the entries orphaned by condensing (they remain in the tree, so
    // add_entry must not change size_).
    for (size_t i = 0; i < orphans.size(); ++i)
      add_entry(std::move(orphans(i)));

    // Shrink a non-leaf root that ended up with a single child.
    while (root_ != nullptr and not root_->leaf and root_->children.size() == 1)
      {
        root_ = std::move(root_->children(0).child);
        --height_;
      }
    // Drop an emptied leaf root.
    if (root_ != nullptr and root_->leaf and root_->data.is_empty())
      {
        root_.reset();
        height_ = 0;
      }
    return true;
  }

  /** @brief Invoke @p f for every entry whose bbox intersects @p rect.
   *  @tparam F Callable as `f(const Rectangle &bbox, const Payload &value)`.
   *  @param rect Query rectangle.
   *  @param f Visitor invoked for each intersecting entry.
   *  @throws Whatever @p f throws.
   *  @note Works for move-only `Payload`: it passes references, never copies.
   */
  template <typename F>
  void for_each_intersecting(const Rectangle &rect, F &&f) const
  {
    if (root_ != nullptr)
      for_each_intersecting_rec(*root_, rect, f);
  }

  /** @brief Return the payloads of every entry whose bbox intersects @p rect.
   *  @param rect Query rectangle.
   *  @return Array with a copy of each matching payload.
   *  @throws std::bad_alloc or whatever copying `Payload` throws.
  */
  [[nodiscard]] Array<Payload> search_intersects(const Rectangle &rect) const
    requires (std::is_copy_constructible_v<Payload> and std::movable<Payload>)
  {
    Array<Payload> out;
    for_each_intersecting(rect, [&out](const Rectangle &, const Payload &v)
      {
        out.append(Payload(v));
      });
    return out;
  }

  /** @brief Return the payloads of every entry whose bbox contains @p p.
   *  @param p Query point.
   *  @return Array with a copy of each matching payload.
   *  @throws std::bad_alloc or whatever copying `Payload` throws.
  */
  [[nodiscard]] Array<Payload> search_contains(const Point &p) const
    requires (std::is_copy_constructible_v<Payload> and std::movable<Payload>)
  {
    Array<Payload> out;
    auto collect = [&out](const Rectangle &, const Payload &v)
      {
        out.append(Payload(v));
      };
    if (root_ != nullptr)
      for_each_containing_rec(*root_, p, collect);
    return out;
  }

  /** @brief Verify the R-tree structural invariants.
   *  @return `true` iff parent boxes are tight unions of their children, node
   *          occupancy is within `[MinEntries, MaxEntries]` (root exempt from
   *          the lower bound), all leaves share one depth, and the counted
   *          entries match `size()`.
   *  @throws Nothing.
   *  @note Intended for tests and diagnostics; it is O(n).
   */
  [[nodiscard]] bool verify() const
  {
    if (root_ == nullptr)
      return size_ == 0 and height_ == 0;

    size_t leaf_depth = std::numeric_limits<size_t>::max();
    size_t count = 0;
    Rectangle mbr;
    if (not verify_rec(*root_, 1, true, leaf_depth, count, mbr))
      return false;
    return count == size_ and leaf_depth == height_;
  }
};

}  // namespace Aleph

#endif  // TPL_R_TREE_H
