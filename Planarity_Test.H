/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
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

/** @file Planarity_Test.H
 *  @brief Planarity testing on Aleph graphs.
 *
 *  This header provides a dedicated API to test whether a graph is planar,
 *  with optional combinatorial embedding and optional non-planarity witness.
 *
 *  ## Problem solved
 *
 *  Decide if a graph can be drawn on the plane with no edge crossings.
 *
 *  ## Notes on input model
 *
 *  The test is performed on the underlying undirected simple graph:
 *  - arc orientation is ignored for digraphs,
 *  - self-loops are ignored,
 *  - parallel arcs are collapsed.
 *
 *  This normalization preserves planarity.
 *
 *  ## Core algorithm
 *
 *  The core checker uses the Left-Right planarity test
 *  (de Fraysseix-Rosenstiehl / Brandes style), with:
 *  - DFS orientation,
 *  - lowpoint / lowpoint2 values,
 *  - conflict-pair stack constraints.
 *
 *  ## Optional advanced outputs
 *
 *  - Combinatorial embedding (rotation system + face walk summary):
 *    LR construction first, then bounded deterministic LR-local repair
 *    (reversals/swaps, multi-start coordinate descent),
 *    with optional bounded exact fallback.
 *  - Embedding-aware geometric drawing:
 *    2D node coordinates with harmonic relaxation and optional crossing validation.
 *  - Certificate export:
 *    machine-readable JSON, DOT, GraphML and GEXF serializers.
 *  - Certificate structural validation:
 *    checks witness internal consistency before external-tool ingestion.
 *  - Non-planarity witness:
 *    - Kuratowski subdivision classification (K5 or K3,3) on branch-core search,
 *    - otherwise a minimal non-planar obstruction (edge-minimal by deletion).
 *
 *  ## Complexity
 *
 *  - Normalization: O(E log E)
 *  - LR planarity test on simplified graph: O(V + E)
 *  - Optional embedding:
 *    - LR-first + bounded LR-local repair
 *    - optional bounded exponential exact fallback
 *  - Optional certificate extraction: O(M * (V + E)) LR invocations in practice
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef PLANARITY_TEST_H
#define PLANARITY_TEST_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <sstream>
#include <string>
#include <utility>

#include <ah-errors.H>
#include <tpl_array.H>
#include <tpl_dynMapTree.H>
#include <tpl_dynSetTree.H>
#include <tpl_graph.H>

namespace Aleph {
/** @brief Non-planarity witness classification.
 *
 *  @ingroup Graphs
 */
enum class Planarity_Certificate_Type
{
  None,
  K5_Subdivision,
  K33_Subdivision,
  Minimal_NonPlanar_Obstruction
};

/** @brief Return readable name for certificate type.
 *
 *  @ingroup Graphs
 */
inline const char *to_string(const Planarity_Certificate_Type type) noexcept
{
  switch (type)
    {
    case Planarity_Certificate_Type::None:
      return "None";
    case Planarity_Certificate_Type::K5_Subdivision:
      return "K5_Subdivision";
    case Planarity_Certificate_Type::K33_Subdivision:
      return "K33_Subdivision";
    case Planarity_Certificate_Type::Minimal_NonPlanar_Obstruction:
      return "Minimal_NonPlanar_Obstruction";
    }

  return "Unknown";
}

/** @brief Configuration options for planarity testing and auxiliary outputs.
 *
 *  Provides control over whether to compute advanced outputs (embedding,
 *  non-planarity witnesses) and sets search bounds for these potentially
 *  expensive operations.
 *
 *  @ingroup Graphs
 */
struct Planarity_Test_Options
{
  /** If true and the graph is found to be planar, computes a combinatorial
   *  embedding (rotation system and list of faces).
   *
   *  @note Computing the embedding can be significantly more expensive
   *        than just testing for planarity.
   */
  bool compute_embedding = false;

  /** If true, the algorithm tries to construct the embedding using a
   *  linear-time Left-Right (LR) heuristic first.
   */
  bool embedding_prefer_lr_linear = true;

  /** If the LR heuristic fails to find a valid embedding, allows
   *  falling back to an exact but potentially exponential-time search.
   *
   *  The search is still bounded by @ref embedding_max_combinations.
   */
  bool embedding_allow_bruteforce_fallback = true;

  /** If true, validates the generated rotation system using Euler's formula
   *  (@f$V - E + F = 1 + C@f$) to ensure it is a valid planar embedding.
   */
  bool embedding_validate_with_euler = true;

  /** If true and the graph is found to be non-planar, attempts to
   *  extract a Kuratowski subdivision (K5 or K3,3) or a minimal
   *  non-planar subgraph as a witness.
   */
  bool compute_nonplanar_certificate = false;

  /** Maximum number of candidate rotations to evaluate during embedding
   *  search or repair. Prevents excessive computation on complex graphs.
   */
  size_t embedding_max_combinations = 300000;

  /** Maximum number of edges in the simplified graph allowed for
   *  full non-planarity witness extraction.
   */
  size_t certificate_max_edges = 180;

  /** Maximum size of the core subgraph allowed for exhaustive
   *  Kuratowski pattern searching.
   */
  size_t certificate_max_branch_nodes_search = 24;

  /** Maximum number of global passes in the edge-reduction loop for
   *  minimal obstruction extraction.
   */
  size_t certificate_max_reduction_passes = std::numeric_limits<size_t>::max();
};

/** @brief Complete result of a planarity test execution.
 *
 *  This structure contains the primary planarity boolean and a wealth
 *  of diagnostic information about the input graph and the test process.
 *  Depending on @ref Planarity_Test_Options, it may also contain
 *  combinatorial embedding data or a non-planarity witness.
 *
 *  @tparam GT Graph type.
 *  @ingroup Graphs
 */
template <class GT>
struct Planarity_Test_Result
{
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;

  /** @brief Entry in a combinatorial rotation system. */
  struct Rotation_Entry
  {
    Node *node = nullptr;       /**< Primal graph node. */
    Array<Node *> cw_neighbors; /**< Neighbors in clockwise order. */
  };

  /** @brief Description of an edge participating in a witness. */
  struct Edge_Witness
  {
    Node *src = nullptr;                     /**< Source node in primal graph. */
    Node *tgt = nullptr;                     /**< Target node in primal graph. */
    Arc *representative_input_arc = nullptr; /**< One arc from primal matching this edge. */
    Array<Arc *> input_arcs;                 /**< All primal arcs matching this edge. */
  };

  /** @brief Description of a path participating in a Kuratowski witness. */
  struct Path_Witness
  {
    Array<Node *> nodes;       /**< Nodes along the path (including endpoints). */
    Array<Edge_Witness> edges; /**< Sequential edges along the path. */
  };

  bool is_planar = true;         /**< True iff the graph can be drawn without crossings. */
  bool input_is_digraph = false; /**< True if the input was a directed graph. */

  size_t num_nodes = 0;      /**< Original number of nodes in the graph. */
  size_t num_input_arcs = 0; /**< Number of arcs passing the @p SA filter. */

  size_t simplified_num_nodes = 0; /**< Nodes in simple undirected representation. */
  size_t simplified_num_edges = 0; /**< Edges in simple undirected representation. */

  size_t ignored_loops = 0;         /**< Count of self-loops filtered out. */
  size_t ignored_parallel_arcs = 0; /**< Count of parallel arcs collapsed. */

  /** True if rejected by Euler necessary condition (@f$E \le 3V - 6@f$). */
  bool failed_euler_bound = false;

  // --- Planar Embedding Output ---
  bool has_combinatorial_embedding = false; /**< True if embedding is valid. */
  bool embedding_is_lr_linear = false;      /**< True if LR linear heuristic worked. */
  bool embedding_search_truncated = false;  /**< True if search limit was reached. */
  size_t embedding_num_faces = 0;           /**< Total number of faces found. */
  Array<Rotation_Entry> embedding_rotation; /**< Full rotation system. */
  Array<Array<Node *>> embedding_faces;     /**< Lists of nodes forming each face. */

  // --- Non-Planarity Witness Output ---
  bool has_nonplanar_certificate = false;    /**< True if witness was extracted. */
  bool certificate_search_truncated = false; /**< True if search limit was reached. */
  Planarity_Certificate_Type certificate_type = Planarity_Certificate_Type::None;
  Array<Node *> certificate_branch_nodes;            /**< Branch nodes for Kuratowski witness. */
  Array<Path_Witness> certificate_paths;             /**< Paths connecting branch nodes. */
  Array<Edge_Witness> certificate_obstruction_edges; /**< Edges in minimal obstruction. */
};

/** @brief Dual-edge payload linked to a primal simplified edge.
 *
 *  @ingroup Graphs
 */
template <class GT>
struct Planar_Dual_Edge_Info
{
  using Node = typename GT::Node;

  size_t face_a = 0;
  size_t face_b = 0;
  Node *primal_src = nullptr;
  Node *primal_tgt = nullptr;
};

/** @brief Metadata extracted from a planar embedding for face/dual analysis.
 *
 *  Faces in this metadata are component-local (outer face is not merged
 *  across disconnected components). For connected graphs, local/global coincide.
 *
 *  @ingroup Graphs
 */
template <class GT>
struct Planar_Dual_Metadata
{
  using Node = typename GT::Node;

  struct Face_Dart
  {
    Node *src = nullptr;
    Node *tgt = nullptr;
  };

  struct Face_Boundary
  {
    Array<Face_Dart> darts;
  };

  bool has_embedding = false;
  bool faces_are_component_local = false;

  size_t num_components = 0;
  size_t num_faces_local = 0;
  size_t num_faces_global = 0;

  Array<Face_Boundary> faces;
  Array<Array<size_t>> face_adjacency;
  Array<Planar_Dual_Edge_Info<GT>> dual_edges;
};

/** @brief Options for embedding-aware 2D drawing extraction.
 *
 *  @ingroup Graphs
 */
struct Planar_Geometric_Drawing_Options
{
  /** Preferred outer-face index (component-local metadata indexing).
   *  Use `max(size_t)` for automatic selection.
   */
  size_t preferred_outer_face = std::numeric_limits<size_t>::max();

  /** Max outer-face candidates evaluated per component. */
  size_t max_outer_faces_to_try = 8;

  /** Max relaxation iterations per candidate. */
  size_t max_relaxation_iterations = 600;

  /** Relaxation stop threshold in max per-node movement. */
  double relaxation_tolerance = 1e-10;

  /** Base radius for boundary-node placement on outer-face polygon. */
  double outer_face_radius = 1.0;

  /** Horizontal gap between disconnected component drawings. */
  double component_spacing = 3.0;

  /** If true, evaluates straight-edge crossings and keeps best candidate. */
  bool validate_crossings = true;
};

/** @brief Embedding-aware geometric drawing result.
 *
 *  @ingroup Graphs
 */
template <class GT>
struct Planar_Geometric_Drawing
{
  using Node = typename GT::Node;

  struct Node_Position
  {
    Node *node = nullptr;
    double x = 0;
    double y = 0;
  };

  bool has_embedding = false;
  bool drawing_available = false;
  bool drawing_validated_no_crossings = false;
  bool drawing_search_truncated = false;

  size_t chosen_outer_face = std::numeric_limits<size_t>::max();
  size_t relaxation_iterations = 0;
  size_t crossing_count = 0;
  size_t num_components = 0;

  Array<Node_Position> node_positions;
};

/** @brief Export options for non-planar certificate serializers.
 *
 *  @ingroup Graphs
 */
struct NonPlanar_Certificate_Export_Options
{
  /** Pretty-print JSON with indentation/newlines. */
  bool pretty_json = true;

  /** Include path overlays in DOT output. */
  bool dot_highlight_paths = true;

  /** Include path overlays in GraphML output. */
  bool graphml_include_paths = true;

  /** Include path overlays in GEXF output. */
  bool gexf_include_paths = true;
};

/** @brief Structural validation report for non-planar certificates.
 *
 *  @ingroup Graphs
 */
template <class GT>
struct NonPlanar_Certificate_Validation
{
  bool has_certificate = false;
  bool is_valid = false;

  size_t num_nodes = 0;
  size_t num_branch_nodes = 0;
  size_t num_obstruction_edges = 0;
  size_t num_paths = 0;

  size_t null_branch_nodes = 0;
  size_t duplicate_branch_nodes = 0;
  size_t null_obstruction_edge_endpoints = 0;
  size_t null_path_nodes = 0;
  size_t null_path_edge_endpoints = 0;
  size_t path_node_edge_length_mismatch = 0;
  size_t path_edge_endpoint_mismatch = 0;
  size_t path_edge_not_in_obstruction = 0;
  size_t kuratowski_shape_mismatch = 0;
};

/** @brief Default node labeler for certificate export.
 *
 *  Uses pointer identity, so it is always available regardless of node info type.
 *
 *  @ingroup Graphs
 */
template <class GT>
struct Dft_Certificate_Node_Label
{
  std::string operator()(typename GT::Node *node) const
  {
    std::ostringstream out;
    out << node;
    return out.str();
  }
};

template <class GT>
using Default_Planar_Dual_Graph
  = List_Graph<Graph_Node<size_t>, Graph_Arc<Planar_Dual_Edge_Info<GT>>>;

namespace planarity_detail {

static constexpr size_t Null_Edge = std::numeric_limits<size_t>::max();

struct Edge_Key
{
  size_t u = 0;
  size_t v = 0;

  bool operator<(const Edge_Key &other) const noexcept
  {
    if (u < other.u)
      return true;
    if (other.u < u)
      return false;
    return v < other.v;
  }
};

struct Simple_Edge
{
  size_t u = 0;
  size_t v = 0;
};

struct Interval
{
  size_t low = Null_Edge;
  size_t high = Null_Edge;

  bool operator==(const Interval &other) const noexcept = default;
};

struct Conflict_Pair
{
  Interval left;
  Interval right;

  bool operator==(const Conflict_Pair &other) const noexcept = default;
};

inline bool interval_empty(const Interval &i) noexcept
{
  return i.low == Null_Edge;
}

inline bool pair_empty(const Conflict_Pair &p) noexcept
{
  return interval_empty(p.left) and interval_empty(p.right);
}

struct Dart_Key
{
  size_t u = 0;
  size_t v = 0;

  bool operator<(const Dart_Key &other) const noexcept
  {
    if (u < other.u)
      return true;
    if (other.u < u)
      return false;
    return v < other.v;
  }
};

struct Compressed_Path
{
  size_t u = Null_Edge;
  size_t v = Null_Edge;
  Array<size_t> nodes;
};

inline std::string json_escape_string(const std::string &s)
{
  std::ostringstream out;
  for (const char i : s)
    switch (const auto c = static_cast<unsigned char>(i))
      {
      case '\"':
        out << "\\\"";
        break;
      case '\\':
        out << "\\\\";
        break;
      case '\b':
        out << "\\b";
        break;
      case '\f':
        out << "\\f";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        if (c < 0x20)
          {
            out << "\\u00";
            const char *hex = "0123456789abcdef";
            out << hex[(c >> 4) & 0xF] << hex[c & 0xF];
          }
        else
          out << static_cast<char>(c);
        break;
      }

  return out.str();
}

inline std::string dot_escape_string(const std::string &s)
{
  std::ostringstream out;
  for (const char c : s)
    if (c == '\"' or c == '\\')
      out << '\\' << c;
    else if (c == '\n')
      out << "\\n";
    else
      out << c;
  return out.str();
}

inline std::string xml_escape_string(const std::string &s)
{
  std::ostringstream out;
  for (const char i : s)
    switch (const auto c = static_cast<unsigned char>(i))
      {
      case '&':
        out << "&amp;";
        break;
      case '<':
        out << "&lt;";
        break;
      case '>':
        out << "&gt;";
        break;
      case '\"':
        out << "&quot;";
        break;
      case '\'':
        out << "&apos;";
        break;
      default:
        if (c < 0x20 and c != '\n' and c != '\r' and c != '\t')
          {
            const char *hex = "0123456789ABCDEF";
            out << "&#x";
            out << hex[(c >> 4) & 0xF] << hex[c & 0xF] << ";";
          }
        else
          out << static_cast<char>(c);
        break;
      }
  return out.str();
}

template <class PtrT>
std::string pointer_to_string(const PtrT ptr)
{
  std::ostringstream out;
  out << ptr;
  return out.str();
}

template <class GT>
void collect_certificate_nodes(const Planarity_Test_Result<GT> &result,
                               Array<typename GT::Node *> &nodes,
                               DynMapTree<typename GT::Node *, size_t> &node_to_id)
{
  using Node = typename GT::Node;

  nodes.empty();
  node_to_id.empty();

  auto add_node = [&](Node *node)
  {
    if (node == nullptr or node_to_id.contains(node))
      return;
    const size_t id = nodes.size();
    node_to_id.insert(node, id);
    nodes.append(node);
  };

  for (typename Array<Node *>::Iterator it(result.certificate_branch_nodes); it.has_curr();
       it.next_ne())
    add_node(it.get_curr_ne());

  for (typename Array<typename Planarity_Test_Result<GT>::Edge_Witness>::Iterator it(
         result.certificate_obstruction_edges);
       it.has_curr();
       it.next_ne())
    {
      add_node(it.get_curr_ne().src);
      add_node(it.get_curr_ne().tgt);
    }

  for (typename Array<typename Planarity_Test_Result<GT>::Path_Witness>::Iterator pit(
         result.certificate_paths);
       pit.has_curr();
       pit.next_ne())
    {
      const auto &path = pit.get_curr_ne();
      for (typename Array<Node *>::Iterator nit(path.nodes); nit.has_curr(); nit.next_ne())
        add_node(nit.get_curr_ne());

      for (typename Array<typename Planarity_Test_Result<GT>::Edge_Witness>::Iterator eit(path.edges);
           eit.has_curr();
           eit.next_ne())
        {
          add_node(eit.get_curr_ne().src);
          add_node(eit.get_curr_ne().tgt);
        }
    }
}

inline double orient2d(const double ax,
                       const double ay,
                       const double bx,
                       const double by,
                       const double cx,
                       const double cy) noexcept
{
  return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
}

inline bool bounding_boxes_intersect(const double ax,
                                     const double ay,
                                     const double bx,
                                     const double by,
                                     const double cx,
                                     const double cy,
                                     const double dx,
                                     const double dy) noexcept
{
  const double min_ab_x = std::min(ax, bx);
  const double max_ab_x = std::max(ax, bx);
  const double min_ab_y = std::min(ay, by);
  const double max_ab_y = std::max(ay, by);

  const double min_cd_x = std::min(cx, dx);
  const double max_cd_x = std::max(cx, dx);
  const double min_cd_y = std::min(cy, dy);
  const double max_cd_y = std::max(cy, dy);

  return min_ab_x <= max_cd_x and min_cd_x <= max_ab_x and min_ab_y <= max_cd_y
     and min_cd_y <= max_ab_y;
}

inline bool segments_properly_intersect(const double ax,
                                        const double ay,
                                        const double bx,
                                        const double by,
                                        const double cx,
                                        const double cy,
                                        const double dx,
                                        const double dy) noexcept
{
  if (not bounding_boxes_intersect(ax, ay, bx, by, cx, cy, dx, dy))
    return false;

  constexpr double eps = 1e-12;
  const double o1 = orient2d(ax, ay, bx, by, cx, cy);
  const double o2 = orient2d(ax, ay, bx, by, dx, dy);
  const double o3 = orient2d(cx, cy, dx, dy, ax, ay);
  const double o4 = orient2d(cx, cy, dx, dy, bx, by);

  // Strict intersection only (shared endpoints handled outside).
  return (o1 * o2 < -eps) and (o3 * o4 < -eps);
}

template <class GT, class SA>
class LR_Planarity_Checker
{
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;

  const GT &g_;
  SA sa_;
  Planarity_Test_Options options_;

  Planarity_Test_Result<GT> result_;

  Array<Node *> nodes_;
  DynMapTree<Node *, size_t> node_to_idx_;

  Array<Simple_Edge> edges_;
  Array<Array<size_t>> incident_edges_;
  Array<Array<Arc *>> simplified_edge_input_arcs_;

  Array<long> height_;
  Array<size_t> parent_edge_;
  Array<Array<size_t>> child_edges_;

  Array<char> undirected_edge_seen_;
  Array<size_t> roots_;

  Array<size_t> oriented_src_;
  Array<size_t> oriented_tgt_;
  Array<long> side_;
  Array<long> lowpt_;
  Array<long> lowpt2_;
  Array<long> nesting_depth_;
  Array<size_t> undirected_to_oriented_;
  Array<size_t> lowpt_edge_;
  Array<size_t> ref_;
  Array<Conflict_Pair> stack_bottom_;

  Array<Conflict_Pair> stack_;

  bool planar_ = true;

  size_t add_oriented_edge(const size_t src, const size_t tgt)
  {
    const size_t id = oriented_src_.size();
    oriented_src_.append(src);
    oriented_tgt_.append(tgt);

    side_.append(1);
    lowpt_.append(0);
    lowpt2_.append(0);
    nesting_depth_.append(0);
    lowpt_edge_.append(Null_Edge);
    ref_.append(Null_Edge);
    stack_bottom_.append(Conflict_Pair());

    return id;
  }

  size_t other_endpoint(const size_t edge_id, const size_t x) const
  {
    const Simple_Edge &e = edges_[edge_id];
    return e.u == x ? e.v : e.u;
  }

  static void sort_size_t_array(Array<size_t> &a)
  {
    for (size_t i = 1; i < a.size(); ++i)
      {
        const size_t key = a[i];
        size_t j = i;
        while (j > 0 and key < a[j - 1])
          {
            a[j] = a[j - 1];
            --j;
          }
        a[j] = key;
      }
  }

  static size_t factorial_bounded(const size_t n, const size_t cap)
  {
    if (n <= 1)
      return 1;

    if (cap == 0)
      return 1;

    size_t ret = 1;
    for (size_t i = 2; i <= n; ++i)
      {
        if (ret > cap / i)
          return cap + 1;
        ret *= i;
      }

    return ret;
  }

  size_t count_components() const
  {
    if (result_.simplified_num_nodes == 0)
      return 0;

    Array<char> vis(result_.simplified_num_nodes, 0);
    size_t comps = 0;

    for (size_t s = 0; s < result_.simplified_num_nodes; ++s)
      {
        if (vis[s])
          continue;

        ++comps;
        vis[s] = 1;

        Array<size_t> stack;
        stack.append(s);

        while (not stack.is_empty())
          {
            const size_t v = stack.remove_last();

            for (typename Array<size_t>::Iterator it(incident_edges_[v]); it.has_curr(); it.next_ne())
              {
                const size_t eid = it.get_curr_ne();
                const size_t w = other_endpoint(eid, v);
                if (vis[w])
                  continue;

                vis[w] = 1;
                stack.append(w);
              }
          }
      }

    return comps;
  }

  static size_t find_in_array(const Array<size_t> &a, const size_t value)
  {
    for (size_t i = 0; i < a.size(); ++i)
      if (a[i] == value)
        return i;

    return Null_Edge;
  }

  static void generate_permutations(Array<size_t> &tail,
                                    const size_t pos,
                                    const size_t first,
                                    Array<Array<size_t>> &out)
  {
    if (pos >= tail.size())
      {
        Array<size_t> order;
        order.reserve(tail.size() + 1);
        order.append(first);
        for (unsigned long i : tail)
          order.append(i);
        out.append(std::move(order));
        return;
      }

    for (size_t i = pos; i < tail.size(); ++i)
      {
        std::swap(tail[pos], tail[i]);
        generate_permutations(tail, pos + 1, first, out);
        std::swap(tail[pos], tail[i]);
      }
  }

  bool compute_faces_from_rotation(const Array<Array<size_t>> &order,
                                   const size_t isolated_vertices,
                                   const size_t num_components,
                                   Array<Array<size_t>> &face_idx,
                                   size_t &global_faces,
                                   const bool enforce_euler = true) const
  {
    Array<size_t> dart_src;
    Array<size_t> alpha;
    dart_src.reserve(2 * result_.simplified_num_edges);
    alpha.reserve(2 * result_.simplified_num_edges);

    DynMapTree<Dart_Key, size_t> dart_id;

    for (typename Array<Simple_Edge>::Iterator it(edges_); it.has_curr(); it.next_ne())
      {
        const Simple_Edge &e = it.get_curr_ne();
        const size_t d1 = dart_src.size();

        dart_src.append(e.u);
        alpha.append(d1 + 1);
        dart_id.insert(Dart_Key{e.u, e.v}, d1);

        dart_src.append(e.v);
        alpha.append(d1);
        dart_id.insert(Dart_Key{e.v, e.u}, d1 + 1);
      }

    Array<size_t> sigma(dart_src.size(), Null_Edge);
    for (size_t v = 0; v < result_.simplified_num_nodes; ++v)
      {
        const auto &ord = order[v];
        if (ord.is_empty())
          continue;

        for (size_t i = 0; i < ord.size(); ++i)
          {
            const size_t to = ord[i];
            const size_t next = ord[(i + 1) % ord.size()];

            const size_t a = dart_id.find(Dart_Key{v, to});
            const size_t b = dart_id.find(Dart_Key{v, next});
            sigma[a] = b;
          }
      }

    for (const unsigned long d : sigma)
      if (d == Null_Edge)
        return false;

    face_idx.empty();

    Array<char> vis(dart_src.size(), 0);
    size_t local_faces = 0;
    for (size_t d = 0; d < dart_src.size(); ++d)
      {
        if (vis[d])
          continue;

        ++local_faces;
        Array<size_t> f;

        size_t x = d;
        while (not vis[x])
          {
            vis[x] = 1;
            f.append(dart_src[x]);
            x = sigma[alpha[x]];
          }

        face_idx.append(std::move(f));
      }

    global_faces = local_faces + isolated_vertices - (num_components == 0 ? 0 : (num_components - 1));

    if (not enforce_euler or not options_.embedding_validate_with_euler)
      return true;

    const long long lhs = static_cast<long long>(result_.simplified_num_nodes)
                        - static_cast<long long>(result_.simplified_num_edges)
                        + static_cast<long long>(global_faces);

    const long long rhs = static_cast<long long>(num_components) + 1;

    return lhs == rhs;
  }

  void fill_embedding_result(const Array<Array<size_t>> &order,
                             const Array<Array<size_t>> &face_idx,
                             const size_t global_faces,
                             const bool is_lr_linear)
  {
    result_.has_combinatorial_embedding = true;
    result_.embedding_is_lr_linear = is_lr_linear;
    result_.embedding_num_faces = global_faces;

    result_.embedding_rotation.empty();
    result_.embedding_rotation.reserve(result_.simplified_num_nodes);
    for (size_t v = 0; v < result_.simplified_num_nodes; ++v)
      {
        typename Planarity_Test_Result<GT>::Rotation_Entry re;
        re.node = nodes_[v];

        const auto &ord = order[v];
        re.cw_neighbors.reserve(ord.size());
        for (typename Array<size_t>::Iterator it(ord); it.has_curr(); it.next_ne())
          re.cw_neighbors.append(nodes_[it.get_curr_ne()]);

        result_.embedding_rotation.append(std::move(re));
      }

    result_.embedding_faces.empty();
    result_.embedding_faces.reserve(face_idx.size());
    for (typename Array<Array<size_t>>::Iterator fit(face_idx); fit.has_curr(); fit.next_ne())
      {
        const auto &f = fit.get_curr_ne();
        Array<Node *> mapped;
        mapped.reserve(f.size());
        for (typename Array<size_t>::Iterator it(f); it.has_curr(); it.next_ne())
          mapped.append(nodes_[it.get_curr_ne()]);
        result_.embedding_faces.append(std::move(mapped));
      }

    while (result_.embedding_faces.size() < result_.embedding_num_faces)
      result_.embedding_faces.append(Array<Node *>());
  }

  bool conflicting(const Interval &i, const size_t edge_id) const
  {
    if (interval_empty(i) or i.high == Null_Edge)
      return false;
    return lowpt_[i.high] > lowpt_[edge_id];
  }

  long lowest(const Conflict_Pair &p) const
  {
    const bool left_empty = interval_empty(p.left);
    const bool right_empty = interval_empty(p.right);

    if (left_empty and right_empty)
      return std::numeric_limits<long>::max();
    if (left_empty)
      return lowpt_[p.right.low];
    if (right_empty)
      return lowpt_[p.left.low];

    return std::min(lowpt_[p.left.low], lowpt_[p.right.low]);
  }

  void orient_dfs(const size_t v)
  {
    if (not planar_)
      return;

    for (typename Array<size_t>::Iterator it(incident_edges_[v]); it.has_curr(); it.next_ne())
      {
        const size_t uedge = it.get_curr_ne();
        if (undirected_edge_seen_[uedge])
          continue;

        undirected_edge_seen_[uedge] = 1;
        const size_t w = other_endpoint(uedge, v);

        const size_t e = add_oriented_edge(v, w);
        undirected_to_oriented_[uedge] = e;
        child_edges_[v].append(e);

        lowpt_[e] = height_[v];
        lowpt2_[e] = height_[v];

        if (height_[w] == -1)
          {
            parent_edge_[w] = e;
            height_[w] = height_[v] + 1;
            orient_dfs(w);
          }
        else
          lowpt_[e] = height_[w];

        nesting_depth_[e] = 2 * lowpt_[e] + (lowpt2_[e] < height_[v] ? 1 : 0);

        if (parent_edge_[v] == Null_Edge)
          continue;

        const size_t pe = parent_edge_[v];
        if (lowpt_[e] < lowpt_[pe])
          {
            lowpt2_[pe] = std::min(lowpt_[pe], lowpt2_[e]);
            lowpt_[pe] = lowpt_[e];
          }
        else if (lowpt_[e] > lowpt_[pe])
          lowpt2_[pe] = std::min(lowpt2_[pe], lowpt_[e]);
        else
          lowpt2_[pe] = std::min(lowpt2_[pe], lowpt2_[e]);
      }
  }

  void test_dfs(const size_t v)
  {
    if (not planar_)
      return;

    const size_t e = parent_edge_[v];

    auto &E = child_edges_[v];
    for (size_t i = 1; i < E.size(); ++i)
      {
        const size_t key = E[i];
        size_t j = i;
        while (j > 0 and nesting_depth_[key] < nesting_depth_[E[j - 1]])
          {
            E[j] = E[j - 1];
            --j;
          }
        E[j] = key;
      }

    for (typename Array<size_t>::Iterator it(E); it.has_curr(); it.next_ne())
      {
        const size_t ei = it.get_curr_ne();
        stack_bottom_[ei] = stack_.get_last();

        const size_t w = oriented_tgt_[ei];
        if (ei == parent_edge_[w])
          test_dfs(w);
        else
          {
            lowpt_edge_[ei] = ei;
            Conflict_Pair cp;
            cp.right.low = ei;
            cp.right.high = ei;
            stack_.append(cp);
          }

        if (not planar_)
          return;

        if (lowpt_[ei] < height_[v])
          {
            if (const size_t first = E[0]; ei == first)
              {
                if (e != Null_Edge)
                  lowpt_edge_[e] = lowpt_edge_[first];
              }
            else
              {
                Conflict_Pair p;
                do
                  {
                    Conflict_Pair q = stack_.get_last();
                    (void) stack_.remove_last();

                    if (not interval_empty(q.left))
                      std::swap(q.left, q.right);

                    if (not interval_empty(q.left))
                      {
                        planar_ = false;
                        return;
                      }

                    if (lowpt_[q.right.low] > lowpt_[e])
                      {
                        if (interval_empty(p.right))
                          p.right.high = q.right.high;
                        else
                          ref_[p.right.low] = q.right.high;

                        p.right.low = q.right.low;
                      }
                    else
                      {
                        ref_[q.right.low] = lowpt_edge_[e];
                      }
                } while (not(stack_.get_last() == stack_bottom_[ei]));

                while (conflicting(stack_.get_last().left, ei)
                       or conflicting(stack_.get_last().right, ei))
                  {
                    Conflict_Pair q = stack_.get_last();
                    (void) stack_.remove_last();

                    if (conflicting(q.right, ei))
                      std::swap(q.left, q.right);

                    if (conflicting(q.right, ei))
                      {
                        planar_ = false;
                        return;
                      }

                    if (p.right.low != Null_Edge)
                      ref_[p.right.low] = q.right.high;

                    if (not interval_empty(q.right))
                      p.right.low = q.right.low;

                    if (q.left.low != Null_Edge)
                      side_[q.left.low] = -1;

                    if (interval_empty(p.left))
                      p.left.high = q.left.high;
                    else
                      ref_[p.left.low] = q.left.high;

                    p.left.low = q.left.low;
                  }

                if (not pair_empty(p))
                  stack_.append(p);
              }
          }
      }

    if (e == Null_Edge)
      return;

    const size_t u = oriented_src_[e];
    while (stack_.size() > 1 and lowest(stack_.get_last()) == height_[u])
      {
        Conflict_Pair p = stack_.remove_last();
        if (not interval_empty(p.left))
          side_[p.left.low] = -1;
      }

    if (stack_.size() > 1)
      {
        Conflict_Pair p = stack_.remove_last();

        while (p.left.high != Null_Edge and oriented_tgt_[p.left.high] == u)
          p.left.high = ref_[p.left.high];

        if (p.left.high == Null_Edge and p.left.low != Null_Edge)
          {
            ref_[p.left.low] = p.right.low;
            side_[p.left.low] = -1;
            p.left.low = Null_Edge;
          }

        while (p.right.high != Null_Edge and oriented_tgt_[p.right.high] == u)
          p.right.high = ref_[p.right.high];

        if (p.right.high == Null_Edge and p.right.low != Null_Edge)
          {
            ref_[p.right.low] = p.left.low;
            p.right.low = Null_Edge;
          }

        stack_.append(p);
      }

    if (lowpt_[e] < height_[u])
      {
        const Conflict_Pair &top = stack_.get_last();
        const size_t h_left = top.left.high;
        const size_t h_right = top.right.high;

        if (h_left != Null_Edge and (h_right == Null_Edge or lowpt_[h_left] > lowpt_[h_right]))
          ref_[e] = h_left;
        else
          ref_[e] = h_right;
      }
  }

  bool fails_component_euler_bound() const
  {
    if (result_.simplified_num_edges == 0)
      return false;

    Array<long> comp_id(result_.simplified_num_nodes, -1);
    size_t next_comp = 0;

    for (size_t s = 0; s < result_.simplified_num_nodes; ++s)
      {
        if (comp_id[s] != -1)
          continue;

        Array<size_t> stack;
        stack.append(s);
        comp_id[s] = static_cast<long>(next_comp);

        size_t num_vertices = 0;
        size_t degree_sum = 0;

        while (not stack.is_empty())
          {
            const size_t v = stack.remove_last();
            ++num_vertices;
            degree_sum += incident_edges_[v].size();

            for (typename Array<size_t>::Iterator it(incident_edges_[v]); it.has_curr(); it.next_ne())
              {
                const size_t uedge = it.get_curr_ne();
                const size_t w = other_endpoint(uedge, v);
                if (comp_id[w] != -1)
                  continue;

                comp_id[w] = static_cast<long>(next_comp);
                stack.append(w);
              }
          }

        const size_t num_edges = degree_sum / 2;
        if (num_vertices >= 3 and num_edges > 3 * num_vertices - 6)
          return true;

        ++next_comp;
      }

    return false;
  }

  void build_underlying_simple_graph()
  {
    result_.num_nodes = g_.get_num_nodes();
    result_.input_is_digraph = g_.is_digraph();

    nodes_.reserve(result_.num_nodes);
    for (Node_Iterator<GT> it(g_); it.has_curr(); it.next_ne())
      {
        Node *node = it.get_curr_ne();
        node_to_idx_.insert(node, nodes_.size());
        nodes_.append(node);
      }

    incident_edges_.reserve(result_.num_nodes);
    child_edges_.reserve(result_.num_nodes);
    for (size_t i = 0; i < result_.num_nodes; ++i)
      {
        incident_edges_.append(Array<size_t>());
        child_edges_.append(Array<size_t>());
      }

    DynSetTree<Edge_Key> seen_edges;
    DynMapTree<Edge_Key, size_t> edge_to_idx;

    for (Arc_Iterator<GT, SA> it(g_, sa_); it.has_curr(); it.next_ne())
      {
        ++result_.num_input_arcs;
        Arc *arc = it.get_curr_ne();
        const size_t src = node_to_idx_.find(g_.get_src_node(arc));
        const size_t tgt = node_to_idx_.find(g_.get_tgt_node(arc));

        if (src == tgt)
          {
            ++result_.ignored_loops;
            continue;
          }

        const size_t u = std::min(src, tgt);
        const size_t v = std::max(src, tgt);
        const Edge_Key key{u, v};

        if (seen_edges.contains(key))
          {
            ++result_.ignored_parallel_arcs;
            const size_t eid = edge_to_idx.find(key);
            simplified_edge_input_arcs_[eid].append(arc);
            continue;
          }

        seen_edges.insert(key);
        const size_t eid = edges_.size();
        edge_to_idx.insert(key, eid);

        edges_.append(Simple_Edge{u, v});
        incident_edges_[u].append(eid);
        incident_edges_[v].append(eid);
        simplified_edge_input_arcs_.append(Array<Arc *>());
        simplified_edge_input_arcs_[eid].append(arc);
      }

    result_.simplified_num_nodes = result_.num_nodes;
    result_.simplified_num_edges = edges_.size();
  }

  bool run_lr_planarity_test()
  {
    if (result_.simplified_num_edges == 0)
      return true;

    height_ = Array<long>(result_.simplified_num_nodes, -1);
    parent_edge_ = Array<size_t>(result_.simplified_num_nodes, Null_Edge);
    undirected_edge_seen_ = Array<char>(result_.simplified_num_edges, 0);
    undirected_to_oriented_ = Array<size_t>(result_.simplified_num_edges, Null_Edge);

    for (size_t v = 0; v < result_.simplified_num_nodes; ++v)
      {
        if (height_[v] != -1)
          continue;

        roots_.append(v);
        height_[v] = 0;
        orient_dfs(v);
      }

    for (typename Array<size_t>::Iterator it(roots_); it.has_curr(); it.next_ne())
      {
        const size_t root = it.get_curr_ne();
        stack_.empty();
        stack_.append(Conflict_Pair());
        test_dfs(root);
        if (not planar_)
          return false;
      }

    return true;
  }

  bool simple_edges_are_planar(const Array<Simple_Edge> &simple_edges) const
  {
    using Tmp_Graph = List_Graph<Graph_Node<size_t>, Graph_Arc<int>>;

    Tmp_Graph tg;
    Array<typename Tmp_Graph::Node *> tmp_nodes(result_.simplified_num_nodes,
                                                static_cast<typename Tmp_Graph::Node *>(nullptr));

    for (size_t i = 0; i < result_.simplified_num_nodes; ++i)
      tmp_nodes[i] = tg.insert_node(i);

    for (typename Array<Simple_Edge>::Iterator it(simple_edges); it.has_curr(); it.next_ne())
      {
        const Simple_Edge &e = it.get_curr_ne();
        tg.insert_arc(tmp_nodes[e.u], tmp_nodes[e.v], 1);
      }

    Planarity_Test_Options fast_options;
    fast_options.compute_embedding = false;
    fast_options.compute_nonplanar_certificate = false;
    fast_options.embedding_max_combinations = 0;
    fast_options.certificate_max_edges = 0;
    fast_options.certificate_max_reduction_passes = 0;

    LR_Planarity_Checker<Tmp_Graph, Dft_Show_Arc<Tmp_Graph>> checker(tg,
                                                                     Dft_Show_Arc<Tmp_Graph>(),
                                                                     fast_options);

    return checker.run().is_planar;
  }

  static bool classify_k5(const Array<size_t> &branches, const Array<Compressed_Path> &paths)
  {
    if (branches.size() != 5 or paths.size() != 10)
      return false;

    Array<Array<char>> mat;
    mat.reserve(5);
    for (size_t i = 0; i < 5; ++i)
      mat.append(Array<char>(static_cast<size_t>(5), static_cast<char>(0)));

    Array<size_t> deg(static_cast<size_t>(5), static_cast<size_t>(0));

    for (typename Array<Compressed_Path>::Iterator it(paths); it.has_curr(); it.next_ne())
      {
        const Compressed_Path &p = it.get_curr_ne();
        const size_t iu = find_in_array(branches, p.u);
        const size_t iv = find_in_array(branches, p.v);
        if (iu == Null_Edge or iv == Null_Edge or iu == iv)
          return false;

        if (mat[iu][iv])
          return false;

        mat[iu][iv] = 1;
        mat[iv][iu] = 1;
        ++deg[iu];
        ++deg[iv];
      }

    for (size_t i = 0; i < 5; ++i)
      {
        if (deg[i] != 4)
          return false;

        for (size_t j = i + 1; j < 5; ++j)
          if (not mat[i][j])
            return false;
      }

    return true;
  }

  static bool classify_k33(const Array<size_t> &branches, const Array<Compressed_Path> &paths)
  {
    if (branches.size() != 6 or paths.size() != 9)
      return false;

    Array<Array<char>> mat;
    mat.reserve(6);
    for (size_t i = 0; i < 6; ++i)
      mat.append(Array<char>(static_cast<size_t>(6), static_cast<char>(0)));

    Array<Array<size_t>> adj;
    adj.reserve(6);
    for (size_t i = 0; i < 6; ++i)
      adj.append(Array<size_t>());

    Array<size_t> deg(static_cast<size_t>(6), static_cast<size_t>(0));

    for (typename Array<Compressed_Path>::Iterator it(paths); it.has_curr(); it.next_ne())
      {
        const Compressed_Path &p = it.get_curr_ne();
        const size_t iu = find_in_array(branches, p.u);
        const size_t iv = find_in_array(branches, p.v);
        if (iu == Null_Edge or iv == Null_Edge or iu == iv)
          return false;

        if (mat[iu][iv])
          return false;

        mat[iu][iv] = 1;
        mat[iv][iu] = 1;
        adj[iu].append(iv);
        adj[iv].append(iu);
        ++deg[iu];
        ++deg[iv];
      }

    for (size_t i = 0; i < 6; ++i)
      if (deg[i] != 3)
        return false;

    Array<long> color(static_cast<size_t>(6), static_cast<long>(-1));
    for (size_t s = 0; s < 6; ++s)
      {
        if (color[s] != -1)
          continue;

        color[s] = 0;
        Array<size_t> stack;
        stack.append(s);

        while (not stack.is_empty())
          {
            const size_t u = stack.remove_last();
            for (typename Array<size_t>::Iterator it(adj[u]); it.has_curr(); it.next_ne())
              {
                const size_t v = it.get_curr_ne();
                if (color[v] == -1)
                  {
                    color[v] = 1 - color[u];
                    stack.append(v);
                  }
                else if (color[v] == color[u])
                  return false;
              }
          }
      }

    size_t left_count = 0;
    size_t right_count = 0;
    for (size_t i = 0; i < 6; ++i)
      if (color[i] == 0)
        ++left_count;
      else
        ++right_count;

    if (left_count != 3 or right_count != 3)
      return false;

    for (size_t i = 0; i < 6; ++i)
      for (size_t j = i + 1; j < 6; ++j)
        {
          if (color[i] == color[j] and mat[i][j])
            return false;

          if (color[i] != color[j] and not mat[i][j])
            return false;
        }

    return true;
  }

  size_t find_simplified_edge_id(const size_t u, const size_t v) const
  {
    const size_t a = std::min(u, v);
    const size_t b = std::max(u, v);

    for (size_t i = 0; i < edges_.size(); ++i)
      if (edges_[i].u == a and edges_[i].v == b)
        return i;

    return Null_Edge;
  }

  typename Planarity_Test_Result<GT>::Edge_Witness make_edge_witness(const size_t u,
                                                                     const size_t v) const
  {
    typename Planarity_Test_Result<GT>::Edge_Witness w;
    if (u < nodes_.size())
      w.src = nodes_[u];
    if (v < nodes_.size())
      w.tgt = nodes_[v];

    const size_t eid = find_simplified_edge_id(u, v);
    if (eid == Null_Edge or eid >= simplified_edge_input_arcs_.size())
      return w;

    const auto &arcs = simplified_edge_input_arcs_[eid];
    if (not arcs.is_empty())
      w.representative_input_arc = arcs[0];

    w.input_arcs.reserve(arcs.size());
    for (typename Array<Arc *>::Iterator it(arcs); it.has_curr(); it.next_ne())
      w.input_arcs.append(it.get_curr_ne());

    return w;
  }

  void fill_certificate_paths(const Array<size_t> &branches, const Array<Compressed_Path> &paths)
  {
    result_.certificate_branch_nodes.empty();
    result_.certificate_paths.empty();

    result_.certificate_branch_nodes.reserve(branches.size());
    for (typename Array<size_t>::Iterator it(branches); it.has_curr(); it.next_ne())
      result_.certificate_branch_nodes.append(nodes_[it.get_curr_ne()]);

    result_.certificate_paths.reserve(paths.size());
    for (typename Array<Compressed_Path>::Iterator it(paths); it.has_curr(); it.next_ne())
      {
        const Compressed_Path &p = it.get_curr_ne();
        typename Planarity_Test_Result<GT>::Path_Witness witness;
        witness.nodes.reserve(p.nodes.size());
        for (typename Array<size_t>::Iterator pit(p.nodes); pit.has_curr(); pit.next_ne())
          witness.nodes.append(nodes_[pit.get_curr_ne()]);

        if (p.nodes.size() >= 2)
          {
            witness.edges.reserve(p.nodes.size() - 1);
            for (size_t i = 1; i < p.nodes.size(); ++i)
              witness.edges.append(make_edge_witness(p.nodes[i - 1], p.nodes[i]));
          }

        result_.certificate_paths.append(std::move(witness));
      }
  }

  void build_nonplanar_certificate()
  {
    if (result_.is_planar)
      return;

    if (edges_.is_empty())
      return;

    if (edges_.size() > options_.certificate_max_edges)
      {
        result_.certificate_search_truncated = true;
        return;
      }

    Array<Simple_Edge> witness = edges_;

    if (options_.certificate_max_reduction_passes == 0)
      result_.certificate_search_truncated = true;
    else
      {
        size_t pass_count = 0;

        while (true)
          {
            bool changed = false;
            size_t i = 0;

            while (i < witness.size())
              {
                Array<Simple_Edge> trial;
                trial.reserve(witness.size() - 1);
                for (size_t j = 0; j < witness.size(); ++j)
                  if (j != i)
                    trial.append(witness[j]);

                if (simple_edges_are_planar(trial))
                  ++i;
                else
                  {
                    witness = std::move(trial);
                    changed = true;
                  }
              }

            Array<size_t> degree(result_.simplified_num_nodes, static_cast<size_t>(0));
            for (typename Array<Simple_Edge>::Iterator it(witness); it.has_curr(); it.next_ne())
              {
                const auto &e = it.get_curr_ne();
                ++degree[e.u];
                ++degree[e.v];
              }

            size_t v = 0;
            while (v < result_.simplified_num_nodes)
              {
                if (degree[v] == 0)
                  {
                    ++v;
                    continue;
                  }

                Array<Simple_Edge> trial;
                trial.reserve(witness.size());
                for (typename Array<Simple_Edge>::Iterator it(witness); it.has_curr(); it.next_ne())
                  {
                    const auto &e = it.get_curr_ne();
                    if (e.u == v or e.v == v)
                      continue;
                    trial.append(e);
                  }

                if (trial.size() == witness.size())
                  {
                    ++v;
                    continue;
                  }

                if (simple_edges_are_planar(trial))
                  ++v;
                else
                  {
                    witness = std::move(trial);
                    changed = true;

                    degree = Array<size_t>(result_.simplified_num_nodes, static_cast<size_t>(0));
                    for (typename Array<Simple_Edge>::Iterator wit(witness); wit.has_curr();
                         wit.next_ne())
                      {
                        const auto &e = wit.get_curr_ne();
                        ++degree[e.u];
                        ++degree[e.v];
                      }
                    v = 0;
                  }
              }

            if (not changed)
              break;

            ++pass_count;
            if (pass_count >= options_.certificate_max_reduction_passes)
              {
                result_.certificate_search_truncated = true;
                break;
              }
          }
      }

    result_.has_nonplanar_certificate = true;
    result_.certificate_type = Planarity_Certificate_Type::Minimal_NonPlanar_Obstruction;

    result_.certificate_obstruction_edges.empty();
    result_.certificate_obstruction_edges.reserve(witness.size());
    for (typename Array<Simple_Edge>::Iterator it(witness); it.has_curr(); it.next_ne())
      {
        const Simple_Edge &e = it.get_curr_ne();
        result_.certificate_obstruction_edges.append(make_edge_witness(e.u, e.v));
      }

    Array<size_t> degree(result_.simplified_num_nodes, static_cast<size_t>(0));
    Array<Array<size_t>> inc;
    inc.reserve(result_.simplified_num_nodes);
    for (size_t i = 0; i < result_.simplified_num_nodes; ++i)
      inc.append(Array<size_t>());

    for (size_t i = 0; i < witness.size(); ++i)
      {
        const auto &e = witness[i];
        ++degree[e.u];
        ++degree[e.v];
        inc[e.u].append(i);
        inc[e.v].append(i);
      }

    Array<size_t> branches;
    for (size_t v = 0; v < result_.simplified_num_nodes; ++v)
      if (degree[v] > 0 and degree[v] != 2)
        branches.append(v);

    if (branches.size() < 5)
      return;

    if (branches.size() > options_.certificate_max_branch_nodes_search)
      {
        result_.certificate_search_truncated = true;
        return;
      }

    Array<char> is_branch(result_.simplified_num_nodes, static_cast<char>(0));
    for (typename Array<size_t>::Iterator it(branches); it.has_curr(); it.next_ne())
      is_branch[it.get_curr_ne()] = 1;

    Array<char> used(witness.size(), static_cast<char>(0));
    Array<Compressed_Path> paths;

    for (typename Array<size_t>::Iterator bit(branches); bit.has_curr(); bit.next_ne())
      {
        const size_t b = bit.get_curr_ne();

        for (typename Array<size_t>::Iterator eit(inc[b]); eit.has_curr(); eit.next_ne())
          {
            const size_t first_edge = eit.get_curr_ne();
            if (used[first_edge])
              continue;

            used[first_edge] = 1;

            Compressed_Path p;
            p.u = b;
            p.nodes.append(b);

            size_t curr = witness[first_edge].u == b ? witness[first_edge].v : witness[first_edge].u;
            size_t prev_edge = first_edge;
            size_t guard = 0;

            while (curr < result_.simplified_num_nodes and not is_branch[curr])
              {
                p.nodes.append(curr);

                if (inc[curr].size() != 2)
                  break;

                const size_t e0 = inc[curr][0];
                const size_t e1 = inc[curr][1];
                const size_t next_edge = e0 == prev_edge ? e1 : e0;

                if (used[next_edge])
                  break;

                used[next_edge] = 1;
                prev_edge = next_edge;

                const auto &ne = witness[next_edge];
                curr = ne.u == curr ? ne.v : ne.u;

                ++guard;
                if (guard > witness.size())
                  break;
              }

            p.v = curr;
            if (p.nodes.is_empty() or p.nodes.get_last() != curr)
              p.nodes.append(curr);

            if (p.u != p.v and p.v < result_.simplified_num_nodes and is_branch[p.v]
                and p.nodes.size() >= 2)
              paths.append(std::move(p));
          }
      }

    if (paths.is_empty())
      return;

    const size_t bcount = branches.size();
    Array<Array<long>> first_path;
    first_path.reserve(bcount);
    for (size_t i = 0; i < bcount; ++i)
      first_path.append(Array<long>(bcount, static_cast<long>(-1)));

    for (size_t i = 0; i < paths.size(); ++i)
      {
        const Compressed_Path &p = paths[i];
        const size_t iu = find_in_array(branches, p.u);
        const size_t iv = find_in_array(branches, p.v);
        if (iu == Null_Edge or iv == Null_Edge or iu == iv)
          continue;

        if (first_path[iu][iv] == -1)
          {
            first_path[iu][iv] = static_cast<long>(i);
            first_path[iv][iu] = static_cast<long>(i);
          }
      }

    auto has_pair = [&](const size_t a, const size_t b) -> bool
    {
      return first_path[a][b] != -1;
    };

    if (bcount >= 5)
      {
        for (size_t a = 0; a < bcount; ++a)
          for (size_t b = a + 1; b < bcount; ++b)
            for (size_t c = b + 1; c < bcount; ++c)
              for (size_t d = c + 1; d < bcount; ++d)
                for (size_t e = d + 1; e < bcount; ++e)
                  {
                    const size_t idx[5] = {a, b, c, d, e};
                    bool ok = true;
                    for (size_t i = 0; i < 5 and ok; ++i)
                      for (size_t j = i + 1; j < 5; ++j)
                        if (not has_pair(idx[i], idx[j]))
                          ok = false;

                    if (not ok)
                      continue;

                    Array<size_t> chosen_branches;
                    chosen_branches.reserve(5);
                    for (unsigned long i : idx)
                      chosen_branches.append(branches[i]);

                    Array<Compressed_Path> chosen_paths;
                    chosen_paths.reserve(10);
                    for (size_t i = 0; i < 5; ++i)
                      for (size_t j = i + 1; j < 5; ++j)
                        chosen_paths.append(paths[static_cast<size_t>(first_path[idx[i]][idx[j]])]);

                    if (not classify_k5(chosen_branches, chosen_paths))
                      continue;

                    result_.certificate_type = Planarity_Certificate_Type::K5_Subdivision;
                    fill_certificate_paths(chosen_branches, chosen_paths);
                    return;
                  }
      }

    if (bcount >= 6)
      {
        for (size_t a = 0; a < bcount; ++a)
          for (size_t b = a + 1; b < bcount; ++b)
            for (size_t c = b + 1; c < bcount; ++c)
              for (size_t d = c + 1; d < bcount; ++d)
                for (size_t e = d + 1; e < bcount; ++e)
                  for (size_t f = e + 1; f < bcount; ++f)
                    {
                      const size_t six[6] = {a, b, c, d, e, f};

                      const int partitions[10][3] = {{0, 1, 2},
                                                     {0, 1, 3},
                                                     {0, 1, 4},
                                                     {0, 1, 5},
                                                     {0, 2, 3},
                                                     {0, 2, 4},
                                                     {0, 2, 5},
                                                     {0, 3, 4},
                                                     {0, 3, 5},
                                                     {0, 4, 5}};

                      for (const auto &part : partitions)
                        {
                          Array<char> in_left(6, static_cast<char>(0));
                          in_left[static_cast<size_t>(part[0])] = 1;
                          in_left[static_cast<size_t>(part[1])] = 1;
                          in_left[static_cast<size_t>(part[2])] = 1;

                          size_t left[3];
                          size_t right[3];
                          size_t li = 0;
                          size_t ri = 0;
                          for (size_t i = 0; i < 6; ++i)
                            if (in_left[i])
                              left[li++] = six[i];
                            else
                              right[ri++] = six[i];

                          bool ok = true;
                          for (size_t i = 0; i < 3 and ok; ++i)
                            for (unsigned long & j : right)
                              if (not has_pair(left[i], j))
                                ok = false;

                          if (not ok)
                            continue;

                          Array<size_t> chosen_branches;
                          chosen_branches.reserve(6);
                          for (unsigned long i : left)
                            chosen_branches.append(branches[i]);
                          for (unsigned long i : right)
                            chosen_branches.append(branches[i]);

                          Array<Compressed_Path> chosen_paths;
                          chosen_paths.reserve(9);
                          for (unsigned long i : left)
                            for (unsigned long j : right)
                              chosen_paths.append(paths[static_cast<size_t>(first_path[i][j])]);

                          if (not classify_k33(chosen_branches, chosen_paths))
                            continue;

                          result_.certificate_type = Planarity_Certificate_Type::K33_Subdivision;
                          fill_certificate_paths(chosen_branches, chosen_paths);
                          return;
                        }
                    }
      }
  }

  bool build_combinatorial_embedding_linear_lr()
  {
    if (result_.simplified_num_nodes == 0)
      {
        result_.has_combinatorial_embedding = true;
        result_.embedding_is_lr_linear = true;
        result_.embedding_num_faces = 0;
        return true;
      }

    Array<Array<size_t>> order;
    order.reserve(result_.simplified_num_nodes);
    for (size_t i = 0; i < result_.simplified_num_nodes; ++i)
      order.append(Array<size_t>());

    if (result_.simplified_num_edges == 0)
      {
        Array<Array<size_t>> faces;
        size_t global_faces = 0;
        if (not compute_faces_from_rotation(
              order, result_.simplified_num_nodes, result_.simplified_num_nodes, faces, global_faces))
          return false;

        fill_embedding_result(order, faces, global_faces, true);
        return true;
      }

    if (undirected_to_oriented_.size() != result_.simplified_num_edges)
      return false;

    for (unsigned long ue : undirected_to_oriented_)
      if (ue == Null_Edge)
        return false;

    Array<long> final_side = side_;
    Array<char> vis(oriented_src_.size(), static_cast<char>(0));

    auto sign_of = [&](auto &&self, const size_t e) -> long
    {
      if (vis[e] == 2)
        return final_side[e];
      if (vis[e] == 1)
        return final_side[e];

      vis[e] = 1;
      if (ref_[e] != Null_Edge)
        final_side[e] *= self(self, ref_[e]);
      vis[e] = 2;
      return final_side[e];
    };

    Array<long> signed_depth(oriented_src_.size(), 0);
    for (size_t e = 0; e < oriented_src_.size(); ++e)
      {
        const long s = sign_of(sign_of, e);
        signed_depth[e] = s < 0 ? -nesting_depth_[e] : nesting_depth_[e];
      }

    size_t isolated_vertices = 0;
    for (size_t v = 0; v < result_.simplified_num_nodes; ++v)
      {
        auto &ord = order[v];
        ord.reserve(incident_edges_[v].size());

        if (incident_edges_[v].is_empty())
          ++isolated_vertices;

        for (typename Array<size_t>::Iterator it(incident_edges_[v]); it.has_curr(); it.next_ne())
          ord.append(it.get_curr_ne());

        auto less_local = [&](const size_t ue_a, const size_t ue_b) -> bool
        {
          const size_t oe_a = undirected_to_oriented_[ue_a];
          const size_t oe_b = undirected_to_oriented_[ue_b];

          const long a0 = (oriented_src_[oe_a] == v ? signed_depth[oe_a] : -signed_depth[oe_a]);
          const long b0 = (oriented_src_[oe_b] == v ? signed_depth[oe_b] : -signed_depth[oe_b]);
          if (a0 != b0)
            return a0 < b0;

          const size_t a1 = other_endpoint(ue_a, v);
          const size_t b1 = other_endpoint(ue_b, v);
          if (a1 != b1)
            return a1 < b1;

          return ue_a < ue_b;
        };

        for (size_t i = 1; i < ord.size(); ++i)
          {
            const size_t key_ue = ord[i];

            size_t j = i;
            while (j > 0 and less_local(key_ue, ord[j - 1]))
              {
                ord[j] = ord[j - 1];
                --j;
              }
            ord[j] = key_ue;
          }

        for (unsigned long &i : ord)
          i = other_endpoint(i, v);
      }

    const size_t num_components = count_components();
    {
      Array<Array<size_t>> faces;
      size_t global_faces = 0;
      if (compute_faces_from_rotation(order, isolated_vertices, num_components, faces, global_faces))
        {
          fill_embedding_result(order, faces, global_faces, true);
          return true;
        }
    }

    if (options_.embedding_max_combinations == 0)
      {
        result_.embedding_search_truncated = true;
        return false;
      }

    auto reverse_order = [](Array<size_t> &ord)
    {
      if (ord.size() <= 1)
        return;

      size_t i = 0;
      size_t j = ord.size() - 1;
      while (i < j)
        {
          std::swap(ord[i], ord[j]);
          ++i;
          --j;
        }
    };

    auto same_order = [](const Array<size_t> &a, const Array<size_t> &b) -> bool
    {
      if (a.size() != b.size())
        return false;

      for (size_t i = 0; i < a.size(); ++i)
        if (a[i] != b[i])
          return false;

      return true;
    };

    auto append_unique_order = [&](Array<Array<size_t>> &opts, const Array<size_t> &cand)
    {
      for (typename Array<Array<size_t>>::Iterator it(opts); it.has_curr(); it.next_ne())
        if (same_order(it.get_curr_ne(), cand))
          return;

      opts.append(cand);
    };

    auto add_adjacent_swaps = [&](Array<Array<size_t>> &opts, const Array<size_t> &src)
    {
      if (src.size() < 2)
        return;

      for (size_t i = 0; i + 1 < src.size(); ++i)
        {
          Array<size_t> cand = src;
          std::swap(cand[i], cand[i + 1]);
          append_unique_order(opts, cand);
        }
    };

    auto add_pair_swaps = [&](Array<Array<size_t>> &opts, const Array<size_t> &src)
    {
      if (src.size() < 2)
        return;

      for (size_t i = 0; i + 1 < src.size(); ++i)
        for (size_t j = i + 1; j < src.size(); ++j)
          {
            Array<size_t> cand = src;
            std::swap(cand[i], cand[j]);
            append_unique_order(opts, cand);
          }
    };

    Array<Array<Array<size_t>>> repair_options;
    repair_options.reserve(result_.simplified_num_nodes);

    for (size_t v = 0; v < result_.simplified_num_nodes; ++v)
      {
        Array<Array<size_t>> opts;
        opts.append(order[v]);

        if (order[v].size() >= 2)
          {
            Array<size_t> rev = order[v];
            reverse_order(rev);
            append_unique_order(opts, rev);

            add_adjacent_swaps(opts, order[v]);
            add_adjacent_swaps(opts, rev);

            // Dense planar subgraphs often need more than a flip.
            // For small local degree, include full pair swaps.
            if (order[v].size() <= 5)
              {
                add_pair_swaps(opts, order[v]);
                add_pair_swaps(opts, rev);
              }
          }

        repair_options.append(std::move(opts));
      }

    Array<size_t> vars;
    vars.reserve(result_.simplified_num_nodes);
    for (size_t v = 0; v < result_.simplified_num_nodes; ++v)
      if (repair_options[v].size() > 1)
        vars.append(v);

    if (vars.is_empty())
      return false;

    for (size_t i = 1; i < vars.size(); ++i)
      {
        const size_t key = vars[i];
        size_t j = i;
        while (j > 0 and repair_options[key].size() > repair_options[vars[j - 1]].size())
          {
            vars[j] = vars[j - 1];
            --j;
          }
        vars[j] = key;
      }

    struct Repair_Quality
    {
      bool available = false;
      bool valid_embedding = false;
      long long abs_euler_delta = std::numeric_limits<long long>::max();
      size_t global_faces = 0;
    };

    Array<size_t> selected(result_.simplified_num_nodes, static_cast<size_t>(0));
    size_t evaluated = 0;
    bool truncated = false;

    auto build_candidate_order = [&](const Array<size_t> &sel, Array<Array<size_t>> &candidate)
    {
      candidate.empty();
      candidate.reserve(result_.simplified_num_nodes);
      for (size_t v = 0; v < result_.simplified_num_nodes; ++v)
        candidate.append(repair_options[v][sel[v]]);
    };

    auto materialize_embedding = [&](const Array<size_t> &sel) -> bool
    {
      Array<Array<size_t>> candidate;
      build_candidate_order(sel, candidate);

      Array<Array<size_t>> faces;
      size_t global_faces = 0;
      if (not compute_faces_from_rotation(
            candidate, isolated_vertices, num_components, faces, global_faces))
        return false;

      fill_embedding_result(candidate, faces, global_faces, true);
      return true;
    };

    auto evaluate_quality = [&](const Array<size_t> &sel, Repair_Quality &out) -> bool
    {
      if (evaluated >= options_.embedding_max_combinations)
        {
          truncated = true;
          return false;
        }

      ++evaluated;

      Array<Array<size_t>> candidate;
      build_candidate_order(sel, candidate);

      Array<Array<size_t>> faces;
      size_t global_faces = 0;
      if (not compute_faces_from_rotation(
            candidate, isolated_vertices, num_components, faces, global_faces, false))
        {
          out.available = false;
          out.valid_embedding = false;
          out.abs_euler_delta = std::numeric_limits<long long>::max();
          out.global_faces = 0;
          return true;
        }

      const long long lhs = static_cast<long long>(result_.simplified_num_nodes)
                          - static_cast<long long>(result_.simplified_num_edges)
                          + static_cast<long long>(global_faces);
      const long long rhs = static_cast<long long>(num_components) + 1;

      long long delta = lhs - rhs;
      if (delta < 0)
        delta = -delta;

      out.available = true;
      out.valid_embedding = (not options_.embedding_validate_with_euler) or delta == 0;
      out.abs_euler_delta = delta;
      out.global_faces = global_faces;
      return true;
    };

    auto run_coordinate_descent = [&](const Array<size_t> &seed_selected) -> bool
    {
      selected = seed_selected;

      Repair_Quality current_quality;
      if (not evaluate_quality(selected, current_quality))
        return false;

      if (current_quality.valid_embedding and materialize_embedding(selected))
        return true;

      const size_t max_passes = vars.size() == 0 ? 0 : 2 * vars.size();
      bool budget_hit = false;

      for (size_t pass = 0; pass < max_passes; ++pass)
        {
          bool improved = false;

          for (typename Array<size_t>::Iterator vit(vars); vit.has_curr(); vit.next_ne())
            {
              const size_t v = vit.get_curr_ne();
              const size_t prev_opt = selected[v];
              size_t best_opt = prev_opt;
              Repair_Quality best_quality = current_quality;

              for (size_t i = 0; i < repair_options[v].size(); ++i)
                {
                  if (i == prev_opt)
                    continue;

                  selected[v] = i;
                  Repair_Quality cand_quality;
                  if (not evaluate_quality(selected, cand_quality))
                    {
                      budget_hit = true;
                      break;
                    }

                  if (not cand_quality.available)
                    continue;

                  if (cand_quality.valid_embedding and materialize_embedding(selected))
                    return true;

                  if (cand_quality.abs_euler_delta < best_quality.abs_euler_delta
                      or (cand_quality.abs_euler_delta == best_quality.abs_euler_delta
                          and cand_quality.global_faces > best_quality.global_faces))
                    {
                      best_opt = i;
                      best_quality = cand_quality;
                    }
                }

              if (budget_hit)
                break;

              selected[v] = best_opt;
              if (best_opt != prev_opt)
                {
                  current_quality = best_quality;
                  improved = true;
                }
            }

          if (budget_hit or not improved)
            break;
        }

      return false;
    };

    Array<size_t> base_seed(result_.simplified_num_nodes, static_cast<size_t>(0));
    if (run_coordinate_descent(base_seed))
      return true;

    if (truncated)
      {
        result_.embedding_search_truncated = true;
        return false;
      }

    Array<size_t> reverse_seed = base_seed;
    bool has_reverse_seed = false;
    for (size_t v = 0; v < result_.simplified_num_nodes; ++v)
      if (repair_options[v].size() > 1)
        {
          reverse_seed[v] = 1;
          has_reverse_seed = true;
        }

    if (has_reverse_seed and run_coordinate_descent(reverse_seed))
      return true;

    if (truncated)
      {
        result_.embedding_search_truncated = true;
        return false;
      }

    for (typename Array<size_t>::Iterator vit(vars); vit.has_curr(); vit.next_ne())
      {
        const size_t v = vit.get_curr_ne();
        for (size_t i = 1; i < repair_options[v].size(); ++i)
          {
            Array<size_t> seed = base_seed;
            seed[v] = i;
            if (run_coordinate_descent(seed))
              return true;

            if (truncated)
              {
                result_.embedding_search_truncated = true;
                return false;
              }
          }
      }

    const size_t max_pair_seed_vars = std::min(static_cast<size_t>(4), vars.size());
    for (size_t ia = 0; ia < max_pair_seed_vars; ++ia)
      for (size_t ib = ia + 1; ib < max_pair_seed_vars; ++ib)
        {
          const size_t va = vars[ia];
          const size_t vb = vars[ib];
          for (size_t oa = 1; oa < repair_options[va].size(); ++oa)
            for (size_t ob = 1; ob < repair_options[vb].size(); ++ob)
              {
                Array<size_t> seed = base_seed;
                seed[va] = oa;
                seed[vb] = ob;
                if (run_coordinate_descent(seed))
                  return true;

                if (truncated)
                  {
                    result_.embedding_search_truncated = true;
                    return false;
                  }
              }
        }

    if (truncated)
      result_.embedding_search_truncated = true;

    return false;
  }

  bool build_combinatorial_embedding_bruteforce()
  {
    if (result_.simplified_num_nodes == 0)
      {
        result_.has_combinatorial_embedding = true;
        result_.embedding_is_lr_linear = false;
        result_.embedding_num_faces = 0;
        return true;
      }

    Array<Array<size_t>> neighbors;
    neighbors.reserve(result_.simplified_num_nodes);

    size_t isolated_vertices = 0;
    for (size_t v = 0; v < result_.simplified_num_nodes; ++v)
      {
        Array<size_t> ng;
        ng.reserve(incident_edges_[v].size());
        for (typename Array<size_t>::Iterator it(incident_edges_[v]); it.has_curr(); it.next_ne())
          ng.append(other_endpoint(it.get_curr_ne(), v));

        sort_size_t_array(ng);
        if (ng.is_empty())
          ++isolated_vertices;

        neighbors.append(std::move(ng));
      }

    if (result_.simplified_num_edges == 0)
      {
        Array<Array<size_t>> order;
        order.reserve(result_.simplified_num_nodes);
        for (size_t v = 0; v < result_.simplified_num_nodes; ++v)
          order.append(Array<size_t>());

        const size_t num_components = result_.simplified_num_nodes;
        Array<Array<size_t>> faces;
        size_t global_faces = 0;

        if (not compute_faces_from_rotation(
              order, isolated_vertices, num_components, faces, global_faces))
          return false;

        fill_embedding_result(order, faces, global_faces, false);
        return true;
      }

    if (options_.embedding_max_combinations == 0)
      {
        result_.embedding_search_truncated = true;
        return false;
      }

    Array<Array<Array<size_t>>> order_options;
    order_options.reserve(result_.simplified_num_nodes);

    size_t combinations = 1;
    for (size_t v = 0; v < result_.simplified_num_nodes; ++v)
      {
        const auto &neigh = neighbors[v];
        const size_t d = neigh.size();

        Array<Array<size_t>> opts;
        if (d <= 2)
          opts.append(neigh);
        else
          {
            const size_t cnt = factorial_bounded(d - 1, options_.embedding_max_combinations);
            if (cnt > options_.embedding_max_combinations
                or combinations > options_.embedding_max_combinations / cnt)
              {
                result_.embedding_search_truncated = true;
                return false;
              }

            combinations *= cnt;

            const size_t first = neigh[0];
            Array<size_t> tail;
            tail.reserve(d - 1);
            for (size_t i = 1; i < d; ++i)
              tail.append(neigh[i]);

            generate_permutations(tail, 0, first, opts);
          }

        order_options.append(std::move(opts));
      }

    Array<size_t> vars;
    vars.reserve(result_.simplified_num_nodes);
    for (size_t v = 0; v < result_.simplified_num_nodes; ++v)
      if (order_options[v].size() > 1)
        vars.append(v);

    for (size_t i = 1; i < vars.size(); ++i)
      {
        const size_t key = vars[i];
        size_t j = i;
        while (j > 0 and order_options[key].size() > order_options[vars[j - 1]].size())
          {
            vars[j] = vars[j - 1];
            --j;
          }
        vars[j] = key;
      }

    const size_t num_components = count_components();
    Array<size_t> selected(result_.simplified_num_nodes, 0);

    auto evaluate = [&]() -> bool
    {
      Array<Array<size_t>> order;
      order.reserve(result_.simplified_num_nodes);
      for (size_t v = 0; v < result_.simplified_num_nodes; ++v)
        order.append(order_options[v][selected[v]]);

      Array<Array<size_t>> faces;
      size_t global_faces = 0;
      if (not compute_faces_from_rotation(order, isolated_vertices, num_components, faces, global_faces))
        return false;

      fill_embedding_result(order, faces, global_faces, false);
      return true;
    };

    auto search = [&](auto &&self, const size_t pos) -> bool
    {
      if (pos == vars.size())
        return evaluate();

      const size_t v = vars[pos];
      for (size_t i = 0; i < order_options[v].size(); ++i)
        {
          selected[v] = i;
          if (self(self, pos + 1))
            return true;
        }

      return false;
    };

    return search(search, 0);
  }

  bool build_combinatorial_embedding()
  {
    if (options_.embedding_prefer_lr_linear and build_combinatorial_embedding_linear_lr())
      return true;

    if (not options_.embedding_allow_bruteforce_fallback)
      {
        result_.embedding_search_truncated = true;
        return false;
      }

    return build_combinatorial_embedding_bruteforce();
  }

public:
  LR_Planarity_Checker(const GT &g, SA sa, Planarity_Test_Options options = Planarity_Test_Options())
    : g_(g), sa_(std::move(sa)), options_(std::move(options))
  {
    // empty
  }

  Planarity_Test_Result<GT> run()
  {
    build_underlying_simple_graph();

    if (fails_component_euler_bound())
      {
        result_.is_planar = false;
        result_.failed_euler_bound = true;

        if (options_.compute_nonplanar_certificate)
          build_nonplanar_certificate();

        return result_;
      }

    result_.is_planar = run_lr_planarity_test();

    if (result_.is_planar)
      {
        if (options_.compute_embedding)
          {
            result_.embedding_search_truncated = false;
            (void) build_combinatorial_embedding();
          }
      }
    else if (options_.compute_nonplanar_certificate)
      {
        result_.certificate_search_truncated = false;
        build_nonplanar_certificate();
      }

    return result_;
  }
};
}  // namespace planarity_detail

/** @brief Execute a comprehensive planarity test on an Aleph graph.
 *
 *  Performs the Left-Right planarity test on the underlying simple
 *  undirected representation of the graph. Depending on the @p options,
 *  it can also compute a planar embedding or a non-planarity witness.
 *
 *  @tparam GT Graph type.
 *  @tparam SA Arc filter type (default: show all arcs).
 *
 *  @param[in] g       The input graph (can be directed or undirected).
 *  @param[in] sa      Arc filter instance.
 *  @param[in] options Configuration for optional advanced outputs.
 *  @return A @ref Planarity_Test_Result containing the planarity status
 *          and requested diagnostic data.
 *
 *  @note The algorithm automatically simplifies the input:
 *        - Digraph arcs are treated as undirected edges.
 *        - Self-loops are ignored.
 *        - Parallel arcs are collapsed into a single edge.
 *
 *  \par Complexity:
 *  Time \f$O(V + E)\f$ for the core test.
 *  Advanced outputs may increase this (see \ref Planarity_Test_Options).
 *
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
Planarity_Test_Result<GT> planarity_test(const GT &g,
                                         SA sa = SA(),
                                         const Planarity_Test_Options &options
                                         = Planarity_Test_Options())
{
  return planarity_detail::LR_Planarity_Checker<GT, SA>(g, std::move(sa), options).run();
}

/** @brief Overload with default arc selector.
 *
 *  @ingroup Graphs
 */
template <class GT>
Planarity_Test_Result<GT> planarity_test(const GT &g, const Planarity_Test_Options &options)
{
  return planarity_test<GT, Dft_Show_Arc<GT>>(g, Dft_Show_Arc<GT>(), options);
}

/** @brief High-level boolean check for graph planarity.
 *
 *  A convenience API that returns only the planarity status.
 *
 *  @param[in] g       The input graph.
 *  @param[in] sa      Arc filter.
 *  @param[in] options Configuration (used if embedding is needed for check).
 *  @return `true` if the graph is planar, `false` otherwise.
 *
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
bool is_planar_graph(const GT &g,
                     SA sa = SA(),
                     const Planarity_Test_Options &options = Planarity_Test_Options())
{
  return planarity_test<GT, SA>(g, std::move(sa), options).is_planar;
}

/** @brief Overload with default arc selector.
 *
 *  @ingroup Graphs
 */
template <class GT>
bool is_planar_graph(const GT &g, const Planarity_Test_Options &options)
{
  return is_planar_graph<GT, Dft_Show_Arc<GT>>(g, Dft_Show_Arc<GT>(), options);
}

/** @brief Build face and dual-edge metadata from a planar embedding result.
 *
 *  Requires `result.is_planar` and `result.has_combinatorial_embedding`.
 *
 *  @ingroup Graphs
 */
template <class GT>
Planar_Dual_Metadata<GT> planar_dual_metadata(const Planarity_Test_Result<GT> &result)
{
  using Node = typename GT::Node;
  using Dart_Key = planarity_detail::Dart_Key;
  constexpr size_t Null_Edge = planarity_detail::Null_Edge;

  ah_runtime_error_unless(result.is_planar and result.has_combinatorial_embedding)
    << "planar_dual_metadata() requires a planar result with embedding";

  const size_t n = result.embedding_rotation.size();
  ah_runtime_error_unless(n == result.simplified_num_nodes) << "embedding_rotation size mismatch";

  Planar_Dual_Metadata<GT> md;
  md.has_embedding = true;

  if (n == 0)
    {
      md.num_components = 0;
      md.num_faces_local = 0;
      md.num_faces_global = 0;
      md.faces_are_component_local = false;
      return md;
    }

  Array<Node *> idx_to_node;
  idx_to_node.reserve(n);

  DynMapTree<Node *, size_t> node_to_idx;
  for (size_t i = 0; i < n; ++i)
    {
      Node *node = result.embedding_rotation[i].node;
      ah_runtime_error_unless(node != nullptr) << "embedding_rotation contains null node";
      ah_runtime_error_if(node_to_idx.contains(node))
        << "embedding_rotation contains duplicated node pointer";

      node_to_idx.insert(node, i);
      idx_to_node.append(node);
    }

  Array<Array<size_t>> order;
  order.reserve(n);
  for (size_t i = 0; i < n; ++i)
    order.append(Array<size_t>());

  for (size_t i = 0; i < n; ++i)
    {
      DynSetTree<size_t> seen;
      const auto &re = result.embedding_rotation[i];
      for (typename Array<Node *>::Iterator it(re.cw_neighbors); it.has_curr(); it.next_ne())
        {
          Node *neigh = it.get_curr_ne();
          ah_runtime_error_unless(neigh != nullptr) << "embedding_rotation contains null neighbor";
          ah_runtime_error_unless(node_to_idx.contains(neigh))
            << "embedding_rotation references unknown neighbor node";

          const size_t v = node_to_idx.find(neigh);
          ah_runtime_error_if(v == i) << "embedding_rotation has self-neighbor in simplified graph";
          ah_runtime_error_if(seen.contains(v))
            << "embedding_rotation has duplicated neighbor in same rotation";

          seen.insert(v);
          order[i].append(v);
        }
    }

  Array<long> comp_id(n, static_cast<long>(-1));
  size_t num_components = 0;
  size_t isolated_vertices = 0;

  for (size_t s = 0; s < n; ++s)
    {
      if (order[s].is_empty())
        ++isolated_vertices;

      if (comp_id[s] != -1)
        continue;

      comp_id[s] = static_cast<long>(num_components);
      Array<size_t> stack;
      stack.append(s);

      while (not stack.is_empty())
        {
          const size_t u = stack.remove_last();
          for (typename Array<size_t>::Iterator it(order[u]); it.has_curr(); it.next_ne())
            {
              const size_t v = it.get_curr_ne();
              if (comp_id[v] != -1)
                continue;
              comp_id[v] = static_cast<long>(num_components);
              stack.append(v);
            }
        }

      ++num_components;
    }

  md.num_components = num_components;

  Array<size_t> dart_src;
  Array<size_t> dart_tgt;
  dart_src.reserve(2 * result.simplified_num_edges);
  dart_tgt.reserve(2 * result.simplified_num_edges);

  DynMapTree<Dart_Key, size_t> dart_id;
  for (size_t u = 0; u < n; ++u)
    {
      for (typename Array<size_t>::Iterator it(order[u]); it.has_curr(); it.next_ne())
        {
          const size_t v = it.get_curr_ne();
          const Dart_Key key{u, v};
          ah_runtime_error_if(dart_id.contains(key))
            << "embedding rotation contains repeated directed edge";

          const size_t did = dart_src.size();
          dart_id.insert(key, did);
          dart_src.append(u);
          dart_tgt.append(v);
        }
    }

  ah_runtime_error_unless(dart_src.size() % 2 == 0) << "invalid embedding: odd number of darts";

  Array<size_t> alpha(dart_src.size(), Null_Edge);
  for (size_t d = 0; d < dart_src.size(); ++d)
    {
      const Dart_Key rev{dart_tgt[d], dart_src[d]};
      ah_runtime_error_unless(dart_id.contains(rev)) << "invalid embedding: missing reverse dart";
      alpha[d] = dart_id.find(rev);
    }

  Array<size_t> sigma(dart_src.size(), Null_Edge);
  for (size_t u = 0; u < n; ++u)
    {
      const auto &ord = order[u];
      if (ord.is_empty())
        continue;

      for (size_t i = 0; i < ord.size(); ++i)
        {
          const size_t to = ord[i];
          const size_t next = ord[(i + 1) % ord.size()];
          const size_t a = dart_id.find(Dart_Key{u, to});
          const size_t b = dart_id.find(Dart_Key{u, next});
          sigma[a] = b;
        }
    }

  for (unsigned long d : sigma)
    ah_runtime_error_unless(d != Null_Edge) << "invalid embedding: incomplete sigma permutation";

  Array<size_t> face_of_dart(dart_src.size(), Null_Edge);
  for (size_t d = 0; d < dart_src.size(); ++d)
    {
      if (face_of_dart[d] != Null_Edge)
        continue;

      const size_t fid = md.faces.size();
      typename Planar_Dual_Metadata<GT>::Face_Boundary face;

      size_t x = d;
      while (face_of_dart[x] == Null_Edge)
        {
          face_of_dart[x] = fid;

          typename Planar_Dual_Metadata<GT>::Face_Dart fd;
          fd.src = idx_to_node[dart_src[x]];
          fd.tgt = idx_to_node[dart_tgt[x]];
          face.darts.append(fd);

          x = sigma[alpha[x]];
        }

      md.faces.append(std::move(face));
    }

  for (size_t i = 0; i < isolated_vertices; ++i)
    md.faces.append(typename Planar_Dual_Metadata<GT>::Face_Boundary());

  md.num_faces_local = md.faces.size();

  const size_t m = dart_src.size() / 2;
  const size_t num_faces_global = m - n + num_components + 1;
  md.num_faces_global = num_faces_global;
  md.faces_are_component_local = md.num_faces_local != md.num_faces_global;

  md.face_adjacency.reserve(md.num_faces_local);
  for (size_t i = 0; i < md.num_faces_local; ++i)
    md.face_adjacency.append(Array<size_t>());

  md.dual_edges.reserve(m);
  for (size_t d = 0; d < dart_src.size(); ++d)
    {
      const size_t rev = alpha[d];
      if (d > rev)
        continue;

      const size_t f0 = face_of_dart[d];
      const size_t f1 = face_of_dart[rev];

      Planar_Dual_Edge_Info<GT> info;
      info.face_a = f0;
      info.face_b = f1;
      info.primal_src = idx_to_node[dart_src[d]];
      info.primal_tgt = idx_to_node[dart_tgt[d]];

      md.dual_edges.append(info);
      md.face_adjacency[f0].append(f1);
      md.face_adjacency[f1].append(f0);
    }

  for (size_t f = 0; f < md.face_adjacency.size(); ++f)
    {
      auto &adj = md.face_adjacency[f];
      for (size_t i = 1; i < adj.size(); ++i)
        {
          const size_t key = adj[i];
          size_t j = i;
          while (j > 0 and key < adj[j - 1])
            {
              adj[j] = adj[j - 1];
              --j;
            }
          adj[j] = key;
        }

      if (adj.is_empty())
        continue;

      Array<size_t> uniq;
      uniq.reserve(adj.size());
      uniq.append(adj[0]);
      for (size_t i = 1; i < adj.size(); ++i)
        if (adj[i] != uniq.get_last())
          uniq.append(adj[i]);

      adj = std::move(uniq);
    }

  return md;
}

/** @brief Build an Aleph dual graph from face/dual metadata.
 *
 *  @ingroup Graphs
 */
template <class GT, class DGT = Default_Planar_Dual_Graph<GT>>
DGT build_planar_dual_graph(const Planar_Dual_Metadata<GT> &md)
{
  ah_runtime_error_unless(md.has_embedding)
    << "build_planar_dual_graph() requires metadata with embedding";

  DGT dual;
  Array<typename DGT::Node *> face_nodes;
  face_nodes.reserve(md.num_faces_local);

  for (size_t f = 0; f < md.num_faces_local; ++f)
    face_nodes.append(dual.insert_node(f));

  for (typename Array<Planar_Dual_Edge_Info<GT>>::Iterator it(md.dual_edges); it.has_curr();
       it.next_ne())
    {
      const auto &e = it.get_curr_ne();
      dual.insert_arc(face_nodes[e.face_a], face_nodes[e.face_b], e);
    }

  return dual;
}

/** @brief Build an Aleph dual graph directly from a planarity result.
 *
 *  @ingroup Graphs
 */
template <class GT, class DGT = Default_Planar_Dual_Graph<GT>>
DGT build_planar_dual_graph(const Planarity_Test_Result<GT> &result)
{
  return build_planar_dual_graph<GT, DGT>(planar_dual_metadata(result));
}

/** @brief Build embedding-aware 2D node coordinates from a planar result.
 *
 *  The drawing uses combinatorial embedding information. For each component,
 *  it places a candidate outer-face boundary on a circle and applies
 *  harmonic relaxation to interior nodes. Multiple deterministic outer-face
 *  candidates are evaluated to reduce crossings.
 *
 *  Requires `result.is_planar` and `result.has_combinatorial_embedding`.
 *
 *  @ingroup Graphs
 */
template <class GT>
Planar_Geometric_Drawing<GT> planar_geometric_drawing(const Planarity_Test_Result<GT> &result,
                                                      const Planar_Geometric_Drawing_Options &options
                                                      = Planar_Geometric_Drawing_Options())
{
  using Node = typename GT::Node;
  using Face_Metadata = Planar_Dual_Metadata<GT>;

  constexpr size_t Null = std::numeric_limits<size_t>::max();
  constexpr double Pi = 3.1415926535897932384626433832795;

  ah_runtime_error_unless(result.is_planar and result.has_combinatorial_embedding)
    << "planar_geometric_drawing() requires a planar result with embedding";

  const size_t n = result.embedding_rotation.size();
  ah_runtime_error_unless(n == result.simplified_num_nodes) << "embedding_rotation size mismatch";

  Planar_Geometric_Drawing<GT> drawing;
  drawing.has_embedding = true;

  if (n == 0)
    {
      drawing.drawing_available = true;
      drawing.drawing_validated_no_crossings = true;
      return drawing;
    }

  if (options.max_outer_faces_to_try == 0)
    {
      drawing.drawing_search_truncated = true;
      return drawing;
    }

  DynMapTree<Node *, size_t> node_to_idx;
  Array<Node *> idx_to_node;
  idx_to_node.reserve(n);

  for (size_t i = 0; i < n; ++i)
    {
      Node *node = result.embedding_rotation[i].node;
      ah_runtime_error_unless(node != nullptr) << "embedding_rotation contains null node";
      ah_runtime_error_if(node_to_idx.contains(node))
        << "embedding_rotation contains duplicated node pointer";

      node_to_idx.insert(node, i);
      idx_to_node.append(node);
    }

  Array<Array<size_t>> order;
  order.reserve(n);
  for (size_t i = 0; i < n; ++i)
    order.append(Array<size_t>());

  for (size_t i = 0; i < n; ++i)
    {
      DynSetTree<size_t> seen;
      const auto &re = result.embedding_rotation[i];
      for (typename Array<Node *>::Iterator it(re.cw_neighbors); it.has_curr(); it.next_ne())
        {
          Node *neigh = it.get_curr_ne();
          ah_runtime_error_unless(neigh != nullptr) << "embedding_rotation contains null neighbor";
          ah_runtime_error_unless(node_to_idx.contains(neigh))
            << "embedding_rotation references unknown node";

          const size_t v = node_to_idx.find(neigh);
          ah_runtime_error_if(v == i) << "embedding_rotation has self-neighbor";
          ah_runtime_error_if(seen.contains(v)) << "embedding_rotation has duplicated neighbor";

          seen.insert(v);
          order[i].append(v);
        }
    }

  Array<long> comp_id(n, static_cast<long>(-1));
  size_t num_components = 0;
  for (size_t s = 0; s < n; ++s)
    {
      if (comp_id[s] != -1)
        continue;

      comp_id[s] = static_cast<long>(num_components);
      Array<size_t> stack;
      stack.append(s);

      while (not stack.is_empty())
        {
          const size_t u = stack.remove_last();
          for (typename Array<size_t>::Iterator it(order[u]); it.has_curr(); it.next_ne())
            {
              const size_t v = it.get_curr_ne();
              if (comp_id[v] != -1)
                continue;
              comp_id[v] = static_cast<long>(num_components);
              stack.append(v);
            }
        }

      ++num_components;
    }

  drawing.num_components = num_components;

  Array<Array<size_t>> comp_nodes;
  comp_nodes.reserve(num_components);
  for (size_t c = 0; c < num_components; ++c)
    comp_nodes.append(Array<size_t>());
  for (size_t i = 0; i < n; ++i)
    comp_nodes[static_cast<size_t>(comp_id[i])].append(i);

  struct Drawing_Edge
  {
    size_t u = 0;
    size_t v = 0;
    size_t comp = 0;
  };

  Array<Drawing_Edge> edges;
  edges.reserve(result.simplified_num_edges);
  for (size_t u = 0; u < n; ++u)
    for (typename Array<size_t>::Iterator it(order[u]); it.has_curr(); it.next_ne())
      {
        const size_t v = it.get_curr_ne();
        if (u < v)
          edges.append(Drawing_Edge{u, v, static_cast<size_t>(comp_id[u])});
      }

  Array<Array<size_t>> comp_edge_ids;
  comp_edge_ids.reserve(num_components);
  for (size_t c = 0; c < num_components; ++c)
    comp_edge_ids.append(Array<size_t>());
  for (size_t i = 0; i < edges.size(); ++i)
    comp_edge_ids[edges[i].comp].append(i);

  const Face_Metadata md = planar_dual_metadata(result);
  struct Face_Candidate
  {
    size_t face_id = Null;
    size_t comp = 0;
    Array<size_t> boundary_nodes;
    size_t score = 0;
  };

  Array<Face_Candidate> face_candidates;
  face_candidates.reserve(md.faces.size());

  for (size_t fid = 0; fid < md.faces.size(); ++fid)
    {
      const auto &face = md.faces[fid];
      if (face.darts.is_empty())
        continue;

      Array<size_t> raw;
      raw.reserve(face.darts.size());
      for (typename Array<typename Face_Metadata::Face_Dart>::Iterator it(face.darts); it.has_curr();
           it.next_ne())
        {
          const auto &d = it.get_curr_ne();
          ah_runtime_error_unless(node_to_idx.contains(d.src))
            << "face metadata references unknown node";
          raw.append(node_to_idx.find(d.src));
        }

      if (raw.is_empty())
        continue;

      Array<size_t> cleaned;
      cleaned.reserve(raw.size());
      for (unsigned long i : raw)
        if (cleaned.is_empty() or cleaned.get_last() != i)
          cleaned.append(i);

      if (cleaned.size() > 1 and cleaned[0] == cleaned.get_last())
        (void) cleaned.remove_last();

      DynSetTree<size_t> seen;
      Array<size_t> unique_cycle;
      unique_cycle.reserve(cleaned.size());
      for (typename Array<size_t>::Iterator it(cleaned); it.has_curr(); it.next_ne())
        {
          const size_t u = it.get_curr_ne();
          if (seen.contains(u))
            continue;
          seen.insert(u);
          unique_cycle.append(u);
        }

      if (unique_cycle.is_empty())
        continue;

      Face_Candidate cand;
      cand.face_id = fid;
      cand.comp = static_cast<size_t>(comp_id[unique_cycle[0]]);
      cand.boundary_nodes = std::move(unique_cycle);
      cand.score = cand.boundary_nodes.size();
      face_candidates.append(std::move(cand));
    }

  Array<Array<size_t>> comp_faces;
  comp_faces.reserve(num_components);
  for (size_t c = 0; c < num_components; ++c)
    comp_faces.append(Array<size_t>());

  for (size_t i = 0; i < face_candidates.size(); ++i)
    comp_faces[face_candidates[i].comp].append(i);

  for (size_t c = 0; c < num_components; ++c)
    {
      auto &faces = comp_faces[c];
      for (size_t i = 1; i < faces.size(); ++i)
        {
          const size_t key = faces[i];
          size_t j = i;
          while (j > 0 and face_candidates[key].score > face_candidates[faces[j - 1]].score)
            {
              faces[j] = faces[j - 1];
              --j;
            }
          faces[j] = key;
        }
    }

  auto count_crossings
    = [&](const size_t comp, const Array<double> &px, const Array<double> &py) -> size_t
  {
    size_t total = 0;
    const auto &ce = comp_edge_ids[comp];
    for (size_t i = 0; i < ce.size(); ++i)
      {
        const auto &e1 = edges[ce[i]];
        for (size_t j = i + 1; j < ce.size(); ++j)
          {
            const auto &e2 = edges[ce[j]];
            if (e1.u == e2.u or e1.u == e2.v or e1.v == e2.u or e1.v == e2.v)
              continue;

            if (planarity_detail::segments_properly_intersect(
                  px[e1.u], py[e1.u], px[e1.v], py[e1.v], px[e2.u], py[e2.u], px[e2.v], py[e2.v]))
              ++total;
          }
      }
    return total;
  };

  auto build_component_layout = [&](const size_t comp,
                                    const Array<size_t> &boundary,
                                    Array<double> &px,
                                    Array<double> &py,
                                    size_t &iterations) -> void
  {
    const auto &cnodes = comp_nodes[comp];
    Array<char> fixed(n, static_cast<char>(0));

    for (typename Array<size_t>::Iterator it(cnodes); it.has_curr(); it.next_ne())
      {
        const size_t u = it.get_curr_ne();
        px[u] = 0;
        py[u] = 0;
      }

    Array<size_t> use_boundary = boundary;
    if (use_boundary.size() < 3 and cnodes.size() >= 1)
      {
        use_boundary.empty();
        for (typename Array<size_t>::Iterator it(cnodes); it.has_curr(); it.next_ne())
          use_boundary.append(it.get_curr_ne());
      }

    const double scale = std::max(1.0, std::sqrt(static_cast<double>(cnodes.size())));
    const double radius = options.outer_face_radius * scale;

    if (use_boundary.is_empty())
      {
        iterations = 0;
        return;
      }

    for (size_t i = 0; i < use_boundary.size(); ++i)
      {
        const size_t u = use_boundary[i];
        const double ang
          = (2.0 * Pi * static_cast<double>(i)) / static_cast<double>(use_boundary.size());
        px[u] = radius * std::cos(ang);
        py[u] = radius * std::sin(ang);
        fixed[u] = 1;
      }

    for (typename Array<size_t>::Iterator it(cnodes); it.has_curr(); it.next_ne())
      {
        const size_t u = it.get_curr_ne();
        if (fixed[u])
          continue;

        double sx = 0;
        double sy = 0;
        size_t cnt = 0;
        for (typename Array<size_t>::Iterator nt(order[u]); nt.has_curr(); nt.next_ne())
          {
            const size_t v = nt.get_curr_ne();
            if (comp_id[v] != static_cast<long>(comp))
              continue;
            sx += px[v];
            sy += py[v];
            ++cnt;
          }

        if (cnt == 0)
          {
            px[u] = 0.01 * static_cast<double>(u + 1);
            py[u] = -0.01 * static_cast<double>(u + 1);
          }
        else
          {
            px[u] = sx / static_cast<double>(cnt);
            py[u] = sy / static_cast<double>(cnt);
          }
      }

    iterations = 0;
    for (size_t iter = 0; iter < options.max_relaxation_iterations; ++iter)
      {
        ++iterations;
        double max_delta = 0;

        for (typename Array<size_t>::Iterator it(cnodes); it.has_curr(); it.next_ne())
          {
            const size_t u = it.get_curr_ne();
            if (fixed[u])
              continue;

            double sx = 0;
            double sy = 0;
            size_t cnt = 0;
            for (typename Array<size_t>::Iterator nt(order[u]); nt.has_curr(); nt.next_ne())
              {
                const size_t v = nt.get_curr_ne();
                if (comp_id[v] != static_cast<long>(comp))
                  continue;
                sx += px[v];
                sy += py[v];
                ++cnt;
              }

            if (cnt == 0)
              continue;

            const double nx = sx / static_cast<double>(cnt);
            const double ny = sy / static_cast<double>(cnt);
            const double dx = nx - px[u];
            const double dy = ny - py[u];
            const double delta = std::sqrt(dx * dx + dy * dy);
            if (delta > max_delta)
              max_delta = delta;

            px[u] = nx;
            py[u] = ny;
          }

        if (max_delta <= options.relaxation_tolerance)
          break;
      }
  };

  Array<double> gx(n, 0.0);
  Array<double> gy(n, 0.0);
  double offset_x = 0.0;

  for (size_t c = 0; c < num_components; ++c)
    {
      Array<size_t> candidates = comp_faces[c];

      if (options.preferred_outer_face != Null and options.preferred_outer_face < md.faces.size())
        {
          for (size_t i = 0; i < candidates.size(); ++i)
            if (face_candidates[candidates[i]].face_id == options.preferred_outer_face)
              {
                const size_t fav = candidates[i];
                for (size_t j = i; j > 0; --j)
                  candidates[j] = candidates[j - 1];
                candidates[0] = fav;
                break;
              }
        }

      size_t num_candidate_evals = 0;
      bool has_best = false;
      size_t best_crossings = std::numeric_limits<size_t>::max();
      size_t best_iters = 0;
      size_t best_face_id = Null;
      Array<double> best_x(n, 0.0);
      Array<double> best_y(n, 0.0);

      auto evaluate_boundary = [&](const Array<size_t> &boundary, const size_t face_id) -> bool
      {
        if (num_candidate_evals >= options.max_outer_faces_to_try)
          {
            drawing.drawing_search_truncated = true;
            return false;
          }

        ++num_candidate_evals;

        Array<double> cx(n, 0.0);
        Array<double> cy(n, 0.0);
        size_t iters = 0;
        build_component_layout(c, boundary, cx, cy, iters);

        const size_t crossings = options.validate_crossings ? count_crossings(c, cx, cy) : 0;

        if (not has_best or crossings < best_crossings
            or (crossings == best_crossings and iters < best_iters))
          {
            has_best = true;
            best_crossings = crossings;
            best_iters = iters;
            best_face_id = face_id;
            best_x = std::move(cx);
            best_y = std::move(cy);
          }

        return crossings == 0;
      };

      for (typename Array<size_t>::Iterator fit(candidates); fit.has_curr(); fit.next_ne())
        {
          const size_t idx = fit.get_curr_ne();
          if (evaluate_boundary(face_candidates[idx].boundary_nodes, face_candidates[idx].face_id))
            break;
        }

      if (not has_best)
        {
          Array<size_t> fallback;
          for (typename Array<size_t>::Iterator it(comp_nodes[c]); it.has_curr(); it.next_ne())
            fallback.append(it.get_curr_ne());
          (void) evaluate_boundary(fallback, Null);
        }

      ah_runtime_error_unless(has_best) << "unable to build component drawing candidate";

      double min_x = std::numeric_limits<double>::max();
      double max_x = -std::numeric_limits<double>::max();
      for (typename Array<size_t>::Iterator it(comp_nodes[c]); it.has_curr(); it.next_ne())
        {
          const size_t u = it.get_curr_ne();
          min_x = std::min(min_x, best_x[u]);
          max_x = std::max(max_x, best_x[u]);
        }

      const double shift_x = offset_x - min_x;
      for (typename Array<size_t>::Iterator it(comp_nodes[c]); it.has_curr(); it.next_ne())
        {
          const size_t u = it.get_curr_ne();
          gx[u] = best_x[u] + shift_x;
          gy[u] = best_y[u];
        }

      offset_x += (max_x - min_x) + options.component_spacing;
      drawing.crossing_count += best_crossings;
      drawing.relaxation_iterations += best_iters;

      if (num_components == 1)
        drawing.chosen_outer_face = best_face_id;
    }

  drawing.node_positions.reserve(n);
  for (size_t i = 0; i < n; ++i)
    {
      typename Planar_Geometric_Drawing<GT>::Node_Position p;
      p.node = idx_to_node[i];
      p.x = gx[i];
      p.y = gy[i];
      drawing.node_positions.append(p);
    }

  drawing.drawing_available = true;
  drawing.drawing_validated_no_crossings
    = (not options.validate_crossings) or drawing.crossing_count == 0;
  return drawing;
}

/** @brief Export non-planar certificate as JSON.
 *
 *  Requires `result.has_nonplanar_certificate`.
 *
 *  @ingroup Graphs
 */
template <class GT, class Node_Label = Dft_Certificate_Node_Label<GT>>
std::string nonplanar_certificate_to_json(const Planarity_Test_Result<GT> &result,
                                          Node_Label node_label = Node_Label(),
                                          const NonPlanar_Certificate_Export_Options &options
                                          = NonPlanar_Certificate_Export_Options())
{
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;

  ah_runtime_error_unless(result.has_nonplanar_certificate)
    << "nonplanar_certificate_to_json() requires non-planar certificate";

  Array<Node *> nodes;
  DynMapTree<Node *, size_t> node_to_id;
  planarity_detail::collect_certificate_nodes(result, nodes, node_to_id);

  std::ostringstream out;
  auto newline = [&](const size_t indent)
  {
    if (not options.pretty_json)
      return;
    out << '\n';
    for (size_t i = 0; i < indent; ++i)
      out << "  ";
  };

  auto node_ref = [&](Node *node) -> std::string
  {
    if (node == nullptr or not node_to_id.contains(node))
      return "null";
    return "\"n" + std::to_string(node_to_id.find(node)) + "\"";
  };

  auto arc_ref = [&](Arc *arc) -> std::string
  {
    if (arc == nullptr)
      return "null";
    return "\"" + planarity_detail::json_escape_string(planarity_detail::pointer_to_string(arc))
         + "\"";
  };

  out << "{";
  newline(1);
  out << "\"is_planar\": false,";
  newline(1);
  out << "\"certificate_available\": true,";
  newline(1);
  out << "\"certificate_type\": \""
      << planarity_detail::json_escape_string(Aleph::to_string(result.certificate_type)) << "\",";
  newline(1);
  out << "\"search_truncated\": " << (result.certificate_search_truncated ? "true" : "false") << ",";

  newline(1);
  out << "\"nodes\": [";
  for (size_t i = 0; i < nodes.size(); ++i)
    {
      if (i > 0)
        out << ",";
      newline(2);
      out << "{";
      out << "\"id\": \"n" << i << "\", ";
      out << "\"label\": \"" << planarity_detail::json_escape_string(node_label(nodes[i])) << "\", ";
      out << "\"ptr\": \""
          << planarity_detail::json_escape_string(planarity_detail::pointer_to_string(nodes[i]))
          << "\"";
      out << "}";
    }
  if (not nodes.is_empty())
    newline(1);
  out << "],";

  newline(1);
  out << "\"branch_nodes\": [";
  for (size_t i = 0; i < result.certificate_branch_nodes.size(); ++i)
    {
      if (i > 0)
        out << ",";
      if (options.pretty_json)
        out << ' ';
      out << node_ref(result.certificate_branch_nodes[i]);
    }
  out << "],";

  newline(1);
  out << "\"obstruction_edges\": [";
  for (size_t i = 0; i < result.certificate_obstruction_edges.size(); ++i)
    {
      const auto &e = result.certificate_obstruction_edges[i];
      if (i > 0)
        out << ",";
      newline(2);
      out << "{";
      out << "\"src\": " << node_ref(e.src) << ", ";
      out << "\"tgt\": " << node_ref(e.tgt) << ", ";
      out << "\"input_arc_count\": " << e.input_arcs.size() << ", ";
      out << "\"representative_input_arc\": " << arc_ref(e.representative_input_arc);
      out << "}";
    }
  if (not result.certificate_obstruction_edges.is_empty())
    newline(1);
  out << "],";

  newline(1);
  out << "\"paths\": [";
  for (size_t i = 0; i < result.certificate_paths.size(); ++i)
    {
      const auto &path = result.certificate_paths[i];
      if (i > 0)
        out << ",";
      newline(2);
      out << "{";

      out << "\"nodes\": [";
      for (size_t k = 0; k < path.nodes.size(); ++k)
        {
          if (k > 0)
            out << ", ";
          out << node_ref(path.nodes[k]);
        }
      out << "], ";

      out << "\"edges\": [";
      for (size_t k = 0; k < path.edges.size(); ++k)
        {
          const auto &e = path.edges[k];
          if (k > 0)
            out << ", ";
          out << "{";
          out << "\"src\": " << node_ref(e.src) << ", ";
          out << "\"tgt\": " << node_ref(e.tgt) << ", ";
          out << "\"input_arc_count\": " << e.input_arcs.size() << ", ";
          out << "\"representative_input_arc\": " << arc_ref(e.representative_input_arc);
          out << "}";
        }
      out << "]";
      out << "}";
    }
  if (not result.certificate_paths.is_empty())
    newline(1);
  out << "]";

  newline(0);
  out << "}";
  return out.str();
}

/** @brief Export non-planar certificate as GraphViz DOT.
 *
 *  Requires `result.has_nonplanar_certificate`.
 *
 *  @ingroup Graphs
 */
template <class GT, class Node_Label = Dft_Certificate_Node_Label<GT>>
std::string nonplanar_certificate_to_dot(const Planarity_Test_Result<GT> &result,
                                         Node_Label node_label = Node_Label(),
                                         const NonPlanar_Certificate_Export_Options &options
                                         = NonPlanar_Certificate_Export_Options())
{
  using Node = typename GT::Node;

  ah_runtime_error_unless(result.has_nonplanar_certificate)
    << "nonplanar_certificate_to_dot() requires non-planar certificate";

  Array<Node *> nodes;
  DynMapTree<Node *, size_t> node_to_id;
  planarity_detail::collect_certificate_nodes(result, nodes, node_to_id);

  DynSetTree<size_t> branch_ids;
  for (typename Array<Node *>::Iterator it(result.certificate_branch_nodes); it.has_curr();
       it.next_ne())
    {
      Node *node = it.get_curr_ne();
      if (node != nullptr and node_to_id.contains(node))
        branch_ids.insert(node_to_id.find(node));
    }

  std::ostringstream out;
  out << "graph NonPlanarCertificate {\n";
  out << "  graph [labelloc=\"t\", label=\""
      << planarity_detail::dot_escape_string(Aleph::to_string(result.certificate_type)) << "\"];\n";
  out << "  node [shape=circle, fontname=\"Helvetica\"];\n";
  out << "  edge [fontname=\"Helvetica\"];\n";

  for (size_t i = 0; i < nodes.size(); ++i)
    {
      out << "  n" << i << " [label=\"n" << i << "\\n"
          << planarity_detail::dot_escape_string(node_label(nodes[i])) << "\"";
      if (branch_ids.contains(i))
        out << ", style=\"filled\", fillcolor=\"#fff3bf\"";
      out << "];\n";
    }

  for (size_t i = 0; i < result.certificate_obstruction_edges.size(); ++i)
    {
      const auto &e = result.certificate_obstruction_edges[i];
      if (e.src == nullptr or e.tgt == nullptr or not node_to_id.contains(e.src)
          or not node_to_id.contains(e.tgt))
        continue;
      const size_t u = node_to_id.find(e.src);
      const size_t v = node_to_id.find(e.tgt);
      out << "  n" << u << " -- n" << v << " [color=\"#d00000\", penwidth=2.2, label=\"obs x"
          << e.input_arcs.size() << "\"];\n";
    }

  if (options.dot_highlight_paths)
    {
      static const char *kPalette[]
        = {"#0077b6", "#2a9d8f", "#6a4c93", "#ef476f", "#ff9f1c", "#588157"};
      constexpr size_t palette_size = sizeof(kPalette) / sizeof(kPalette[0]);

      for (size_t pid = 0; pid < result.certificate_paths.size(); ++pid)
        {
          const auto &path = result.certificate_paths[pid];
          const char *color = kPalette[pid % palette_size];
          for (size_t k = 0; k < path.edges.size(); ++k)
            {
              const auto &e = path.edges[k];
              if (e.src == nullptr or e.tgt == nullptr or not node_to_id.contains(e.src)
                  or not node_to_id.contains(e.tgt))
                continue;
              const size_t u = node_to_id.find(e.src);
              const size_t v = node_to_id.find(e.tgt);
              out << "  n" << u << " -- n" << v << " [color=\"" << color
                  << "\", style=\"dashed\", penwidth=1.4, constraint=false, label=\"p" << pid
                  << "\"];\n";
            }
        }
    }

  out << "}\n";
  return out.str();
}

/** @brief Validate structural consistency of a non-planar certificate.
 *
 *  This function checks internal witness invariants (node/edge references,
 *  path chaining and obstruction/path coherence). It does not re-run planarity.
 *
 *  @ingroup Graphs
 */
template <class GT>
NonPlanar_Certificate_Validation<GT> validate_nonplanar_certificate(
  const Planarity_Test_Result<GT> &result)
{
  using Node = typename GT::Node;

  NonPlanar_Certificate_Validation<GT> report;
  report.has_certificate = result.has_nonplanar_certificate;
  report.num_branch_nodes = result.certificate_branch_nodes.size();
  report.num_obstruction_edges = result.certificate_obstruction_edges.size();
  report.num_paths = result.certificate_paths.size();

  if (not result.has_nonplanar_certificate)
    return report;

  Array<Node *> nodes;
  DynMapTree<Node *, size_t> node_to_id;
  planarity_detail::collect_certificate_nodes(result, nodes, node_to_id);
  report.num_nodes = nodes.size();

  DynSetTree<size_t> branch_ids;
  for (typename Array<Node *>::Iterator it(result.certificate_branch_nodes); it.has_curr();
       it.next_ne())
    {
      Node *node = it.get_curr_ne();
      if (node == nullptr or not node_to_id.contains(node))
        {
          ++report.null_branch_nodes;
          continue;
        }

      const size_t id = node_to_id.find(node);
      if (branch_ids.contains(id))
        ++report.duplicate_branch_nodes;
      else
        branch_ids.insert(id);
    }

  auto make_edge_key = [&](Node *src, Node *tgt, planarity_detail::Edge_Key &key) -> bool
  {
    if (src == nullptr or tgt == nullptr)
      return false;
    if (not node_to_id.contains(src) or not node_to_id.contains(tgt))
      return false;

    size_t u = node_to_id.find(src);
    size_t v = node_to_id.find(tgt);
    if (u > v)
      std::swap(u, v);

    key.u = u;
    key.v = v;
    return true;
  };

  DynSetTree<planarity_detail::Edge_Key> obstruction_keys;
  for (typename Array<typename Planarity_Test_Result<GT>::Edge_Witness>::Iterator it(
         result.certificate_obstruction_edges);
       it.has_curr();
       it.next_ne())
    {
      const auto &e = it.get_curr_ne();
      planarity_detail::Edge_Key key;
      if (not make_edge_key(e.src, e.tgt, key))
        {
          ++report.null_obstruction_edge_endpoints;
          continue;
        }
      obstruction_keys.insert(key);
    }

  for (typename Array<typename Planarity_Test_Result<GT>::Path_Witness>::Iterator pit(
         result.certificate_paths);
       pit.has_curr();
       pit.next_ne())
    {
      const auto &path = pit.get_curr_ne();

      for (typename Array<Node *>::Iterator nit(path.nodes); nit.has_curr(); nit.next_ne())
        if (nit.get_curr_ne() == nullptr)
          ++report.null_path_nodes;

      const size_t expected_nodes = path.edges.size() + (path.nodes.is_empty() ? 0 : 1);
      if (path.nodes.size() != expected_nodes)
        ++report.path_node_edge_length_mismatch;

      for (size_t i = 0; i < path.edges.size(); ++i)
        {
          const auto &e = path.edges[i];
          if (e.src == nullptr or e.tgt == nullptr)
            {
              ++report.null_path_edge_endpoints;
              continue;
            }

          if (i + 1 >= path.nodes.size() or path.nodes[i] != e.src or path.nodes[i + 1] != e.tgt)
            ++report.path_edge_endpoint_mismatch;

          planarity_detail::Edge_Key key;
          if (not make_edge_key(e.src, e.tgt, key))
            {
              ++report.null_path_edge_endpoints;
              continue;
            }

          if (not obstruction_keys.contains(key))
            ++report.path_edge_not_in_obstruction;
        }
    }

  if (result.certificate_type == Planarity_Certificate_Type::None)
    ++report.kuratowski_shape_mismatch;
  else if (result.certificate_type == Planarity_Certificate_Type::K5_Subdivision)
    {
      if (result.certificate_branch_nodes.size() != 5 or result.certificate_paths.is_empty())
        ++report.kuratowski_shape_mismatch;
    }
  else if (result.certificate_type == Planarity_Certificate_Type::K33_Subdivision)
    {
      if (result.certificate_branch_nodes.size() != 6 or result.certificate_paths.is_empty())
        ++report.kuratowski_shape_mismatch;
    }

  report.is_valid
    = report.has_certificate and report.null_branch_nodes == 0
  and report.duplicate_branch_nodes == 0 and report.null_obstruction_edge_endpoints == 0
  and report.null_path_nodes == 0 and report.null_path_edge_endpoints == 0
  and report.path_node_edge_length_mismatch == 0 and report.path_edge_endpoint_mismatch == 0
  and report.path_edge_not_in_obstruction == 0 and report.kuratowski_shape_mismatch == 0;

  return report;
}

/** @brief Convenience predicate over `validate_nonplanar_certificate()`.
 *
 *  @ingroup Graphs
 */
template <class GT>
bool nonplanar_certificate_is_valid(const Planarity_Test_Result<GT> &result)
{
  return validate_nonplanar_certificate(result).is_valid;
}

/** @brief Export non-planar certificate as GraphML.
 *
 *  Requires `result.has_nonplanar_certificate`.
 *
 *  @ingroup Graphs
 */
template <class GT, class Node_Label = Dft_Certificate_Node_Label<GT>>
std::string nonplanar_certificate_to_graphml(const Planarity_Test_Result<GT> &result,
                                             Node_Label node_label = Node_Label(),
                                             const NonPlanar_Certificate_Export_Options &options
                                             = NonPlanar_Certificate_Export_Options())
{
  using Node = typename GT::Node;

  ah_runtime_error_unless(result.has_nonplanar_certificate)
    << "nonplanar_certificate_to_graphml() requires non-planar certificate";

  Array<Node *> nodes;
  DynMapTree<Node *, size_t> node_to_id;
  planarity_detail::collect_certificate_nodes(result, nodes, node_to_id);

  DynSetTree<size_t> branch_ids;
  for (typename Array<Node *>::Iterator it(result.certificate_branch_nodes); it.has_curr();
       it.next_ne())
    {
      Node *node = it.get_curr_ne();
      if (node != nullptr and node_to_id.contains(node))
        branch_ids.insert(node_to_id.find(node));
    }

  auto node_ref = [&](Node *node) -> std::string
  {
    if (node == nullptr or not node_to_id.contains(node))
      return "";
    return "n" + std::to_string(node_to_id.find(node));
  };

  std::ostringstream out;
  out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  out << "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\">\n";
  out << "  <key id=\"k_node_label\" for=\"node\" attr.name=\"label\" attr.type=\"string\"/>\n";
  out << "  <key id=\"k_node_branch\" for=\"node\" attr.name=\"branch\" attr.type=\"boolean\"/>\n";
  out << "  <key id=\"k_node_ptr\" for=\"node\" attr.name=\"ptr\" attr.type=\"string\"/>\n";
  out << "  <key id=\"k_edge_kind\" for=\"edge\" attr.name=\"kind\" attr.type=\"string\"/>\n";
  out << "  <key id=\"k_edge_path_id\" for=\"edge\" attr.name=\"path_id\" attr.type=\"int\"/>\n";
  out << "  <key id=\"k_edge_input_mult\" for=\"edge\" attr.name=\"input_multiplicity\" "
         "attr.type=\"int\"/>\n";
  out << "  <graph id=\"NonPlanarCertificate\" edgedefault=\"undirected\">\n";

  for (size_t i = 0; i < nodes.size(); ++i)
    {
      out << "    <node id=\"n" << i << "\">\n";
      out << "      <data key=\"k_node_label\">"
          << planarity_detail::xml_escape_string(node_label(nodes[i])) << "</data>\n";
      out << "      <data key=\"k_node_branch\">" << (branch_ids.contains(i) ? "true" : "false")
          << "</data>\n";
      out << "      <data key=\"k_node_ptr\">"
          << planarity_detail::xml_escape_string(planarity_detail::pointer_to_string(nodes[i]))
          << "</data>\n";
      out << "    </node>\n";
    }

  size_t edge_id = 0;
  for (size_t i = 0; i < result.certificate_obstruction_edges.size(); ++i)
    {
      const auto &e = result.certificate_obstruction_edges[i];
      const std::string u = node_ref(e.src);
      const std::string v = node_ref(e.tgt);
      if (u.empty() or v.empty())
        continue;

      out << "    <edge id=\"e" << edge_id++ << "\" source=\"" << u << "\" target=\"" << v << "\">\n";
      out << "      <data key=\"k_edge_kind\">obstruction</data>\n";
      out << "      <data key=\"k_edge_path_id\">-1</data>\n";
      out << "      <data key=\"k_edge_input_mult\">" << e.input_arcs.size() << "</data>\n";
      out << "    </edge>\n";
    }

  if (options.graphml_include_paths)
    for (size_t pid = 0; pid < result.certificate_paths.size(); ++pid)
      {
        const auto &path = result.certificate_paths[pid];
        for (size_t k = 0; k < path.edges.size(); ++k)
          {
            const auto &e = path.edges[k];
            const std::string u = node_ref(e.src);
            const std::string v = node_ref(e.tgt);
            if (u.empty() or v.empty())
              continue;

            out << "    <edge id=\"e" << edge_id++ << "\" source=\"" << u << "\" target=\"" << v
                << "\">\n";
            out << "      <data key=\"k_edge_kind\">path</data>\n";
            out << "      <data key=\"k_edge_path_id\">" << pid << "</data>\n";
            out << "      <data key=\"k_edge_input_mult\">" << e.input_arcs.size() << "</data>\n";
            out << "    </edge>\n";
          }
      }

  out << "  </graph>\n";
  out << "</graphml>\n";
  return out.str();
}

/** @brief Export non-planar certificate as GEXF.
 *
 *  Requires `result.has_nonplanar_certificate`.
 *
 *  @ingroup Graphs
 */
template <class GT, class Node_Label = Dft_Certificate_Node_Label<GT>>
std::string nonplanar_certificate_to_gexf(const Planarity_Test_Result<GT> &result,
                                          Node_Label node_label = Node_Label(),
                                          const NonPlanar_Certificate_Export_Options &options
                                          = NonPlanar_Certificate_Export_Options())
{
  using Node = typename GT::Node;

  ah_runtime_error_unless(result.has_nonplanar_certificate)
    << "nonplanar_certificate_to_gexf() requires non-planar certificate";

  Array<Node *> nodes;
  DynMapTree<Node *, size_t> node_to_id;
  planarity_detail::collect_certificate_nodes(result, nodes, node_to_id);

  DynSetTree<size_t> branch_ids;
  for (typename Array<Node *>::Iterator it(result.certificate_branch_nodes); it.has_curr();
       it.next_ne())
    {
      Node *node = it.get_curr_ne();
      if (node != nullptr and node_to_id.contains(node))
        branch_ids.insert(node_to_id.find(node));
    }

  auto node_ref = [&](Node *node) -> std::string
  {
    if (node == nullptr or not node_to_id.contains(node))
      return "";
    return "n" + std::to_string(node_to_id.find(node));
  };

  std::ostringstream out;
  out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  out << "<gexf xmlns=\"http://www.gexf.net/1.2draft\" version=\"1.2\">\n";
  out << "  <meta lastmodifieddate=\"2026-02-22\">\n";
  out << "    <creator>Aleph Planarity_Test.H</creator>\n";
  out << "    <description>Non-planar certificate</description>\n";
  out << "  </meta>\n";
  out << "  <graph mode=\"static\" defaultedgetype=\"undirected\">\n";
  out << "    <attributes class=\"node\">\n";
  out << "      <attribute id=\"n_branch\" title=\"branch\" type=\"boolean\"/>\n";
  out << "      <attribute id=\"n_ptr\" title=\"ptr\" type=\"string\"/>\n";
  out << "    </attributes>\n";
  out << "    <attributes class=\"edge\">\n";
  out << "      <attribute id=\"e_kind\" title=\"kind\" type=\"string\"/>\n";
  out << "      <attribute id=\"e_path_id\" title=\"path_id\" type=\"integer\"/>\n";
  out << "      <attribute id=\"e_input_mult\" title=\"input_multiplicity\" type=\"integer\"/>\n";
  out << "    </attributes>\n";
  out << "    <nodes>\n";

  for (size_t i = 0; i < nodes.size(); ++i)
    {
      out << "      <node id=\"n" << i << "\" label=\""
          << planarity_detail::xml_escape_string(node_label(nodes[i])) << "\">\n";
      out << "        <attvalues>\n";
      out << "          <attvalue for=\"n_branch\" value=\""
          << (branch_ids.contains(i) ? "true" : "false") << "\"/>\n";
      out << "          <attvalue for=\"n_ptr\" value=\""
          << planarity_detail::xml_escape_string(planarity_detail::pointer_to_string(nodes[i]))
          << "\"/>\n";
      out << "        </attvalues>\n";
      out << "      </node>\n";
    }

  out << "    </nodes>\n";
  out << "    <edges>\n";

  size_t edge_id = 0;
  for (size_t i = 0; i < result.certificate_obstruction_edges.size(); ++i)
    {
      const auto &e = result.certificate_obstruction_edges[i];
      const std::string u = node_ref(e.src);
      const std::string v = node_ref(e.tgt);
      if (u.empty() or v.empty())
        continue;

      out << "      <edge id=\"e" << edge_id++ << "\" source=\"" << u << "\" target=\"" << v
          << "\">\n";
      out << "        <attvalues>\n";
      out << "          <attvalue for=\"e_kind\" value=\"obstruction\"/>\n";
      out << "          <attvalue for=\"e_path_id\" value=\"-1\"/>\n";
      out << "          <attvalue for=\"e_input_mult\" value=\"" << e.input_arcs.size() << "\"/>\n";
      out << "        </attvalues>\n";
      out << "      </edge>\n";
    }

  if (options.gexf_include_paths)
    for (size_t pid = 0; pid < result.certificate_paths.size(); ++pid)
      {
        const auto &path = result.certificate_paths[pid];
        for (size_t k = 0; k < path.edges.size(); ++k)
          {
            const auto &e = path.edges[k];
            const std::string u = node_ref(e.src);
            const std::string v = node_ref(e.tgt);
            if (u.empty() or v.empty())
              continue;

            out << "      <edge id=\"e" << edge_id++ << "\" source=\"" << u << "\" target=\"" << v
                << "\">\n";
            out << "        <attvalues>\n";
            out << "          <attvalue for=\"e_kind\" value=\"path\"/>\n";
            out << "          <attvalue for=\"e_path_id\" value=\"" << pid << "\"/>\n";
            out << "          <attvalue for=\"e_input_mult\" value=\"" << e.input_arcs.size()
                << "\"/>\n";
            out << "        </attvalues>\n";
            out << "      </edge>\n";
          }
      }

  out << "    </edges>\n";
  out << "  </graph>\n";
  out << "</gexf>\n";
  return out.str();
}

/** @brief Functor wrapper for `planarity_test()`.
 *
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
class Planarity_Test
{
  SA sa_;
  Planarity_Test_Options options_;

public:
  explicit Planarity_Test(SA sa = SA(), Planarity_Test_Options options = Planarity_Test_Options())
    : sa_(std::move(sa)), options_(std::move(options))
  {
    // empty
  }

  Planarity_Test_Result<GT> operator()(const GT &g) const
  {
    return planarity_test<GT, SA>(g, sa_, options_);
  }
};

/** @brief Functor wrapper for `is_planar_graph()`.
 *
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
class Is_Planar_Graph
{
  SA sa_;
  Planarity_Test_Options options_;

public:
  explicit Is_Planar_Graph(SA sa = SA(), Planarity_Test_Options options = Planarity_Test_Options())
    : sa_(std::move(sa)), options_(std::move(options))
  {
    // empty
  }

  bool operator()(const GT &g) const
  {
    return is_planar_graph<GT, SA>(g, sa_, options_);
  }
};
}  // namespace Aleph

#endif  // PLANARITY_TEST_H
