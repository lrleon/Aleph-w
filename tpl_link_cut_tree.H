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

/** @file tpl_link_cut_tree.H
 *  @brief Link-Cut Tree: a dynamic forest with path queries.
 *
 *  A Link-Cut Tree (Sleator & Tarjan, 1983) is a powerful data structure 
 *  that maintains a forest of rooted trees. It supports dynamic modifications 
 *  (adding and removing edges) and queries on paths (such as the sum, 
 *  minimum, maximum, or generic aggregation of node values along a path), 
 *  all in amortized \f$O(\log n)\f$ time per operation.
 *
 *  ### Theoretical Background
 *  The trees in the forest (called *represented trees*) are decomposed 
 *  into vertex-disjoint *preferred paths*. Each preferred path is stored 
 *  as a Splay Tree (called an *auxiliary tree*), keyed by depth in the 
 *  represented tree. This means an in-order traversal of the Splay Tree 
 *  yields the path from the highest node (closest to the root) to the lowest.
 *  The root of each auxiliary tree contains a pointer to the parent of the 
 *  highest node in the path it represents (called a *path-parent pointer*).
 *
 *  ### Main Operations
 *  - `link(u, v)` — Merges two trees by making `u` a child of `v`.
 *  - `cut(u, v)` — Splits a tree by removing the edge between `u` and `v`.
 *  - `make_root(x)` — Reroots the represented tree containing `x` so that `x` becomes the root.
 *  - `find_root(x)` — Finds the root of the tree containing `x`.
 *  - `connected(u, v)` — Checks if `u` and `v` are in the same tree.
 *  - `lca(u, v)` — Finds the lowest common ancestor of `u` and `v`.
 *  - `path_query(u, v)` — Computes the aggregate value over the path between `u` and `v`.
 *  - `path_apply(u, v, tag)` — Applies a lazy update to all nodes on the path between `u` and `v`.
 *
 *  ### Genericity and Concepts
 *  The structure is heavily customizable using C++20 concepts:
 *  - `T`: The value type stored at each node.
 *  - `Monoid`: A type satisfying `LCTMonoid`, dictating how node values are aggregated (e.g., Sum, Min, Max).
 *  - `LazyTag`: A type satisfying `LCTLazyTag`, dictating how range updates (like adding a delta to a path) are applied lazily.
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_LINK_CUT_TREE_H
# define TPL_LINK_CUT_TREE_H

# include <concepts>
# include <limits>
# include <tpl_array.H>
# include <tpl_arrayStack.H>
# include <tpl_tree_node.H>
# include <ah-errors.H>

namespace Aleph
{
  // ---------------------------------------------------------------
  //  Concepts
  // ---------------------------------------------------------------

  /** @brief Concept for a monoidal combiner over path values.
   *
   *  A Monoid requires an identity element and an associative binary
   *  operation `combine(a, b)`. It dictates how values on a path
   *  are aggregated together (e.g., finding the sum or minimum).
   */
  template <typename M, typename T>
  concept LCTMonoid =
      requires(const M & m, const T & a, const T & b)
        {
          { M::identity() } -> std::convertible_to<T>;
          { M::combine(a, b) } -> std::convertible_to<T>;
        };

  /** @brief Concept for a lazy-tag policy used in deferred path updates.
   *
   *  A Lazy Tag dictates how deferred (lazy) operations apply to nodes
   *  and how multiple deferred operations compose together. This is
   *  essential for efficiently updating an entire path in O(log n) time.
   */
  template <typename L, typename T>
  concept LCTLazyTag =
      requires(const typename L::tag_type & t1,
               const typename L::tag_type & t2,
               const T & val, size_t cnt)
        {
          typename L::tag_type;
          { L::tag_identity() } -> std::convertible_to<typename L::tag_type>;
          { L::apply(val, t1, cnt) } -> std::convertible_to<T>;
          { L::compose(t1, t2) } -> std::convertible_to<typename L::tag_type>;
        };

  // ---------------------------------------------------------------
  //  Built-in monoids
  // ---------------------------------------------------------------

  /** @brief Default (no-op) monoid for connectivity-only usage. */
  template <typename T>
  struct DefaultMonoid
  {
    static constexpr T identity() noexcept { return T{}; }
    static constexpr T combine(const T &, const T & b) noexcept { return b; }
  };

  /** @brief Sum monoid: identity = 0, combine = a + b. */
  template <typename T>
  struct SumMonoid
  {
    using supports_counted_lazy = std::true_type;

    static constexpr T identity() noexcept { return T{0}; }

    static constexpr T combine(const T & a, const T & b) noexcept
    {
      return a + b;
    }
  };

  /** @brief Min monoid: identity = numeric max, combine = min(a, b). */
  template <typename T>
  struct MinMonoid
  {
    static constexpr T identity() noexcept
    {
      return std::numeric_limits<T>::max();
    }

    static constexpr T combine(const T & a, const T & b) noexcept
    {
      return a < b ? a : b;
    }
  };

  /** @brief Max monoid: identity = numeric lowest, combine = max(a, b). */
  template <typename T>
  struct MaxMonoid
  {
    static constexpr T identity() noexcept
    {
      return std::numeric_limits<T>::lowest();
    }

    static constexpr T combine(const T & a, const T & b) noexcept
    {
      return a > b ? a : b;
    }
  };

  /** @brief XOR monoid: identity = 0, combine = a ^ b. */
  template <typename T>
  struct XorMonoid
  {
    static constexpr T identity() noexcept { return T{0}; }

    static constexpr T combine(const T & a, const T & b) noexcept
    {
      return a ^ b;
    }
  };

  /** @brief GCD monoid: identity = 0 (since gcd(0, x) = x), combine = gcd. */
  template <typename T>
  struct GcdMonoid
  {
    static constexpr T identity() noexcept { return T{0}; }

    static constexpr T combine(const T & a, const T & b) noexcept
    {
      T x = a < T{0} ? -a : a;
      T y = b < T{0} ? -b : b;
      while (y != T{0})
        {
          T t = y;
          y = x % y;
          x = t;
        }
      return x;
    }
  };

  /** @brief Product monoid: identity = 1, combine = a * b. */
  template <typename T>
  struct ProductMonoid
  {
    static constexpr T identity() noexcept { return T{1}; }

    static constexpr T combine(const T & a, const T & b) noexcept
    {
      return a * b;
    }
  };

  // ---------------------------------------------------------------
  //  Lazy-tag policies
  // ---------------------------------------------------------------

  /** @brief No-op lazy tag (default — no deferred path updates). */
  template <typename T>
  struct NoLazyTag
  {
    using tag_type = bool;
    static constexpr bool tag_identity() noexcept { return false; }
    static constexpr T apply(const T & v, bool, size_t) noexcept { return v; }
    static constexpr bool compose(bool, bool) noexcept { return false; }
  };

  /** @brief Additive lazy tag: adds a delta to every node on a path.
   *
   *  When combined with `SumMonoid`, the aggregate of a subtree of
   *  `cnt` nodes increases by `tag * cnt`.  For other monoids the
   *  count parameter may be ignored.
   */
  template <typename T>
  struct AddLazyTag
  {
    using tag_type = T;
    static constexpr T tag_identity() noexcept { return T{0}; }

    static constexpr T apply(const T & v, const T & tag, size_t cnt) noexcept
    {
      return v + tag * static_cast<T>(cnt);
    }

    static constexpr T compose(const T & a, const T & b) noexcept
    {
      return a + b;
    }
  };

  /** @brief Assignment lazy tag: sets every node on a path to a fixed value.
   *
   *  Designed for use with `SumMonoid`.  When assigning value `v` to a
   *  subtree of `cnt` nodes, the aggregate becomes `v * cnt`.
   *
   *  @note For `MinMonoid` / `MaxMonoid` the aggregate after assignment
   *        is simply the assigned value (all nodes are equal).  Write a
   *        custom tag in that case.
   */
  template <typename T>
  struct AssignLazyTag
  {
    /** @brief Payload for the assignment lazy tag. */
    struct tag_type
    {
      T val = {};
      bool active = false;

      constexpr bool operator==(const tag_type & o) const noexcept
      {
        return active == o.active and (not active or val == o.val);
      }
    };

    static constexpr tag_type tag_identity() noexcept { return {}; }

    static constexpr T apply(const T & v, const tag_type & tag,
                             size_t cnt) noexcept
    {
      return tag.active ? tag.val * static_cast<T>(cnt) : v;
    }

    static constexpr tag_type compose(const tag_type & existing,
                                      const tag_type & newer) noexcept
    {
      return newer.active ? newer : existing;
    }
  };

  /** @brief Metafunction to check if a monoid supports counted lazy tags. */
  template <class Monoid, class = void>
  struct monoid_supports_counted_lazy : std::false_type {};

  /** @brief Specialization for monoids with supports_counted_lazy member. */
  template <class Monoid>
  struct monoid_supports_counted_lazy<Monoid, std::void_t<typename Monoid::supports_counted_lazy>>
      : std::bool_constant<Monoid::supports_counted_lazy::value> {};

  /** @brief Metafunction to check if a tag is a counted lazy tag. */
  template <class LazyTag, typename T>
  struct is_counted_lazy_tag : std::false_type {};

  /** @brief Specialization for AddLazyTag. */
  template <typename T>
  struct is_counted_lazy_tag<AddLazyTag<T>, T> : std::true_type {};

  /** @brief Specialization for AssignLazyTag. */
  template <typename T>
  struct is_counted_lazy_tag<AssignLazyTag<T>, T> : std::true_type {};

  // ---------------------------------------------------------------
  //  Gen_Link_Cut_Tree
  // ---------------------------------------------------------------

  /** @brief Dynamic forest with path queries via Link-Cut Trees.
   *
   *  @tparam T        Value type stored at each vertex.
   *  @tparam Monoid   Associative combiner for path aggregation. Must satisfy `LCTMonoid`.
   *  @tparam LazyTag  Lazy-tag policy for deferred path updates. Must satisfy `LCTLazyTag`.
   *
   *  @details
   *  The `Gen_Link_Cut_Tree` class allows you to manage a dynamic forest of nodes.
   *  It is particularly useful for problems where you need to answer connectivity
   *  queries or path aggregation queries (e.g., "what is the bottleneck edge on the
   *  path from A to B?") in a network that is constantly changing (edges being added
   *  and removed).
   *
   *  ### Lifecycle and Memory Management
   *  Vertices are created via `make_vertex()`, returning an opaque `Node*` handle.
   *  The tree assumes ownership of all created nodes and will destroy them upon
   *  destruction. Handles remain stable as long as the tree is alive.
   *
   *  ### Typical Use Cases
   *  1. **Dynamic Connectivity**: Use `Link_Cut_Tree` (which defaults to `int` and no-ops)
   *     simply to call `link()`, `cut()`, and `connected()`.
   *  2. **Dynamic Minimum Spanning Forest**: Maintain edges with weights using a `MaxMonoid`
   *     or `MinMonoid`. When adding an edge that forms a cycle, find the max/min weight
   *     edge on the cycle using `path_query()` and `cut()` it.
   *  3. **Path Updates / Range Queries on Trees**: Use a `SumMonoid` and `AddLazyTag`
   *     to dynamically add values to paths and query path sums.
   *
   *  @par Complexity
   *  Every public operation runs in amortized \f$O(\log n)\f$ time, where \f$n\f$ is
   *  the number of nodes currently in the forest.
   *
   *  @warning The behavior is undefined if handles from different `Gen_Link_Cut_Tree`
   *  instances are mixed.
   */
  template <typename T = int,
            class Monoid = DefaultMonoid<T>,
            class LazyTag = NoLazyTag<T>>
    requires LCTMonoid<Monoid, T> and LCTLazyTag<LazyTag, T>
  class Gen_Link_Cut_Tree
  {
    static_assert(not is_counted_lazy_tag<LazyTag, T>::value
                  or monoid_supports_counted_lazy<Monoid>::value,
                  "AddLazyTag::apply and AssignLazyTag::apply require a monoid "
                  "with supports_counted_lazy=true");

  public:
    /** @brief Internal node of the Link-Cut Tree (opaque handle).
     *  @details Users interact with vertices using pointers to this structure.
     */
    struct Node
    {
      friend class Gen_Link_Cut_Tree;

      /** @brief Return the stored value. */
      [[nodiscard]] const T &get_val() const noexcept { return val; }

    private:
      Node *left = nullptr;
      Node *right = nullptr;
      Node *parent = nullptr;
      bool rev = false;
      size_t sz = 1;

      T val;
      T agg;

      using tag_type = typename LazyTag::tag_type;
      tag_type lazy = LazyTag::tag_identity();

      Node() : val(T{}), agg(T{}) {}
      explicit Node(const T & v) : val(v), agg(v) {}
      explicit Node(T && v) : val(std::move(v)), agg(val) {}
    };

  private:
    Array<Node *> nodes;
    size_t n_components = 0;

    // ---- auxiliary predicates ----

    static bool is_root(const Node *x) noexcept
    {
      return x->parent == nullptr or
             (x->parent->left != x and x->parent->right != x);
    }

    static bool is_left(const Node *x) noexcept
    {
      return x->parent and x->parent->left == x;
    }

    // ---- lazy propagation ----

    static void push(Node *x) noexcept
    {
      if (not x)
        return;

      if (x->rev)
        {
          std::swap(x->left, x->right);
          if (x->left) x->left->rev ^= true;
          if (x->right) x->right->rev ^= true;
          x->rev = false;
        }

      if constexpr (not std::is_same_v<LazyTag, NoLazyTag<T>>)
        {
          if (not (x->lazy == LazyTag::tag_identity()))
            {
              auto propagate = [&](Node *c)
                {
                  if (not c)
                    return;
                  c->val = LazyTag::apply(c->val, x->lazy, 1);
                  c->agg = LazyTag::apply(c->agg, x->lazy, c->sz);
                  c->lazy = LazyTag::compose(c->lazy, x->lazy);
                };
              propagate(x->left);
              propagate(x->right);
              x->lazy = LazyTag::tag_identity();
            }
        }
    }

    // ---- aggregate recomputation ----

    static void pull(Node *x) noexcept
    {
      if (not x)
        return;

      x->sz = 1;
      x->agg = x->val;

      if (x->left)
        {
          x->sz += x->left->sz;
          x->agg = Monoid::combine(x->left->agg, x->agg);
        }
      if (x->right)
        {
          x->sz += x->right->sz;
          x->agg = Monoid::combine(x->agg, x->right->agg);
        }
    }

    // ---- rotations ----

    static void rotate(Node *x) noexcept
    {
      Node *p = x->parent;
      Node *g = p->parent;

      if (not is_root(p))
        {
          if (g->left == p)
            g->left = x;
          else
            g->right = x;
        }

      if (p->left == x)
        {
          p->left = x->right;
          if (x->right)
            x->right->parent = p;
          x->right = p;
        }
      else
        {
          p->right = x->left;
          if (x->left)
            x->left->parent = p;
          x->left = p;
        }

      x->parent = g;
      p->parent = x;

      pull(p);
      pull(x);
    }

    // ---- splay ----

    static void splay(Node *x) noexcept
    {
      // collect ancestors within this auxiliary tree
      // and push lazy flags top-down
      {
        Node *u = x;
        ArrayStack<Node *> stk;
        while (not is_root(u))
          {
            stk.push(u->parent);
            u = u->parent;
          }
        push(u); // push the aux-tree root
        while (not stk.is_empty())
          push(stk.pop());
        push(x);
      }

      while (not is_root(x))
        {
          Node *p = x->parent;
          if (not is_root(p))
            // zig-zig or zig-zag
            if (Node *g = p->parent; (g->left == p) == (p->left == x))
              rotate(p); // zig-zig: rotate parent first
            else
              rotate(x); // zig-zag: rotate x first
          rotate(x);
        }
    }

    // ---- access ----

    // Exposes the root-to-x preferred path.  Returns the last
    // node splayed during traversal (the LCA when preceded by
    // a prior access).
    static Node * access(Node *x) noexcept
    {
      Node *last = nullptr;
      for (Node *u = x; u != nullptr; u = u->parent)
        {
          splay(u);
          u->right = last;
          pull(u);
          last = u;
        }
      splay(x);
      return last;
    }

    // ---- iterative in-order traversal of a splay subtree ----

    template <typename Op>
    static void inorder_traverse(Node *root, Op && op)
    {
      ArrayStack<Node *> stk;
      Node *cur = root;
      while (cur or not stk.is_empty())
        {
          while (cur)
            {
              push(cur);
              stk.push(cur);
              cur = cur->left;
            }
          cur = stk.pop();
          op(cur);
          cur = cur->right;
        }
    }

  public:
    /** @brief Default constructor for an empty Link-Cut Tree. */
    Gen_Link_Cut_Tree() = default;

    /** @brief Destructor. Frees all nodes managed by the LCT. */
    ~Gen_Link_Cut_Tree()
    {
      for (size_t i = 0; i < nodes.size(); ++i)
        delete nodes(i);
    }

    /** @brief Deleted copy constructor. */
    Gen_Link_Cut_Tree(const Gen_Link_Cut_Tree &) = delete;

    /** @brief Deleted copy assignment. */
    Gen_Link_Cut_Tree &operator=(const Gen_Link_Cut_Tree &) = delete;

    /** @brief Deleted move constructor. */
    Gen_Link_Cut_Tree(Gen_Link_Cut_Tree &&) = delete;

    /** @brief Deleted move assignment. */
    Gen_Link_Cut_Tree &operator=(Gen_Link_Cut_Tree &&) = delete;

    /** @brief Create an isolated vertex with payload `val`.
     *
     *  @details The vertex is created as a tree of a single node. The Link-Cut
     *  Tree retains ownership of the allocated memory. The returned handle
     *  must be used for all subsequent operations involving this vertex.
     *
     *  @param val Initial vertex value (default-constructed if omitted).
     *  @return    Stable handle to the new vertex.
     */
    Node * make_vertex(const T & val = T{})
    {
      auto *nd = new Node(val);
      nodes.append(nd);
      ++n_components;
      return nd;
    }

    /** @brief Create an isolated vertex (move semantics).
     *
     *  @param val Initial vertex value.
     *  @return    Stable handle to the new vertex.
     */
    Node * make_vertex(T && val)
    {
      auto *nd = new Node(std::move(val));
      nodes.append(nd);
      ++n_components;
      return nd;
    }

    /** @brief Destroy an isolated vertex.
     *
     *  @param x Handle to the isolated vertex to destroy.
     *  @throws std::invalid_argument if `x` is null.
     *  @throws std::domain_error if `x` is not isolated or does not belong to this tree.
     */
    void destroy_vertex(Node *x)
    {
      ah_invalid_argument_if(x == nullptr) << "null handle";
      access(x);
      ah_domain_error_if(x->left != nullptr or x->right != nullptr or x->parent != nullptr)
        << "vertex must be isolated";

      size_t idx = 0;
      while (idx < nodes.size() and nodes(idx) != x)
        ++idx;
      ah_domain_error_if(idx == nodes.size())
        << "vertex does not belong to this link-cut tree";

      if (idx + 1 < nodes.size())
        nodes(idx) = nodes(nodes.size() - 1);
      nodes.remove_last();
      delete x;
      --n_components;
    }

    /** @brief Total number of vertices currently in the forest. */
    [[nodiscard]] size_t size() const noexcept { return nodes.size(); }

    /** @brief Make `x` the root of its represented tree.
     *
     *  @details This operation changes the represented tree so that `x` becomes
     *  the new root. It exposes the path to `x` (bringing `x` to the root of its
     *  auxiliary tree) and flips its `rev` lazy flag. This reverses the depth
     *  order of the path, making `x` the highest node (the new root).
     *  Does not create or destroy any edge. After this call `find_root(x) == x`.
     *
     *  @param x Handle to the vertex to become the new root.
     *  @throws std::invalid_argument if `x` is null.
     */
    void make_root(Node *x)
    {
      ah_invalid_argument_if(x == nullptr) << "null handle";
      access(x);
      x->rev ^= true;
      push(x);
    }

    /** @brief Find the root of the represented tree containing `x`.
     *
     *  @details This exposes the path to `x` and then traverses down the left
     *  children of the auxiliary tree to find the node with the minimum
     *  depth in the represented tree, which corresponds to the root.
     *  The structure is splayed at the end to maintain amortized time bounds.
     *
     *  @param x Handle to a vertex in the tree.
     *  @return Handle to the root vertex of the tree containing `x`.
     *  @throws std::invalid_argument if `x` is null.
     */
    [[nodiscard]] Node * find_root(Node *x)
    {
      ah_invalid_argument_if(x == nullptr) << "null handle";
      access(x);
      while (true)
        {
          push(x);
          if (x->left == nullptr)
            break;
          x = x->left;
        }
      splay(x);
      return x;
    }

    /** @brief Test whether `u` and `v` are in the same represented tree.
     *
     *  @details Internally, it checks if `find_root(u)` is the same as
     *  `find_root(v)`.
     *
     *  @param u Handle to the first vertex.
     *  @param v Handle to the second vertex.
     *  @return `true` if they are connected (in the same tree), `false` otherwise.
     *  @throws std::invalid_argument if `u` or `v` is null.
     */
    [[nodiscard]] bool connected(Node *u, Node *v)
    {
      ah_invalid_argument_if(u == nullptr or v == nullptr) << "null handle";
      if (u == v)
        return true;
      find_root(u);
      find_root(v);
      // after find_root(v), if they were connected, u would be
      // an ancestor of v in the aux tree → u->parent != nullptr
      // But the standard check is simpler:
      return find_root(u) == find_root(v);
    }

    /** @brief Add an edge between `u` and `v`, merging their trees.
     *
     *  @details This operation links the tree containing `u` to the tree containing `v`.
     *  It does this by first making `u` the root of its tree (`make_root(u)`).
     *  Then, it simply sets `v` as the parent of `u` in the represented forest.
     *
     *  The operation requires that `u` and `v` are currently in different trees.
     *  If they are already connected, a cycle would be formed, which is not
     *  allowed in a valid forest representation.
     *
     *  @param u Handle to the vertex that will become a child.
     *  @param v Handle to the vertex that will become the parent.
     *  @throws std::domain_error if `u` and `v` are already in the same tree.
     *  @throws std::invalid_argument if `u == v` or either handle is null.
     */
    void link(Node *u, Node *v)
    {
      ah_invalid_argument_if(u == nullptr or v == nullptr) << "null handle";
      ah_invalid_argument_if(u == v) << "self-loop";
      make_root(u);
      ah_domain_error_if(find_root(v) == u) << "already connected";
      u->parent = v;
      --n_components;
    }

    /** @brief Remove the represented edge between `u` and `v`.
     *
     *  @details Splits the tree containing `u` and `v` into two trees by
     *  deleting the edge between them. It achieves this by making `u` the
     *  root of the tree, and then accessing `v`. If `v` is adjacent to `u`
     *  in the represented tree, `u` will be the left child of `v` in the
     *  auxiliary tree. Breaking this child connection separates them.
     *
     *  @param u Handle to the first vertex of the edge.
     *  @param v Handle to the second vertex of the edge.
     *  @throws std::domain_error if the edge (u, v) does not exist.
     *  @throws std::invalid_argument if either handle is null.
     */
    void cut(Node *u, Node *v)
    {
      ah_invalid_argument_if(u == nullptr or v == nullptr) << "null handle";
      make_root(u);
      access(v);
      ah_domain_error_if(v->left != u or u->parent != v or u->right != nullptr)
        << "edge does not exist";
      v->left = nullptr;
      u->parent = nullptr;
      pull(v);
      ++n_components;
    }

    /** @brief Lowest common ancestor of `u` and `v`.
     *
     *  @details The LCA is relative to the current rooting of the tree.
     *  If the structure has been rerooted via `make_root()`, the LCA will reflect
     *  that new hierarchy. Internally, it exposes the path from the root to `u`,
     *  and then begins exposing the path from the root to `v`. The point where
     *  the `v` path intersects the `u` path is the LCA.
     *
     *  @param u Handle to the first vertex.
     *  @param v Handle to the second vertex.
     *  @return Handle to the LCA of `u` and `v`.
     *  @throws std::domain_error if `u` and `v` are not connected.
     *  @throws std::invalid_argument if either handle is null.
     */
    [[nodiscard]] Node * lca(Node *u, Node *v)
    {
      ah_invalid_argument_if(u == nullptr or v == nullptr) << "null handle";
      if (u == v)
        return u;
      access(u);
      Node *ans = access(v);
      // verify they were connected: after access(v),
      // u should be reachable via parent chain from v
      // (access(u) already exposed the root-to-u path)
      ah_domain_error_if(find_root(u) != find_root(v)) << "not connected";
      return ans;
    }

    /** @brief Aggregate (under Monoid) over all vertex values on the
     *         path from `u` to `v`.
     *
     *  @details This method first makes `u` the root of the tree, and then
     *  exposes the path up to `v` by calling `access(v)`. The path from `u`
     *  to `v` is perfectly captured in the auxiliary tree rooted at `v`,
     *  so the aggregate value is trivially available in `v->agg`.
     *
     *  @param u Start vertex of the path.
     *  @param v End vertex of the path.
     *  @return The monoidal aggregation of values along the path.
     *  @throws std::domain_error if `u` and `v` are not connected.
     *  @throws std::invalid_argument if either handle is null.
     */
    [[nodiscard]] T path_query(Node *u, Node *v)
    {
      ah_invalid_argument_if(u == nullptr or v == nullptr) << "null handle";
      make_root(u);
      ah_domain_error_if(find_root(v) != u) << "vertices not connected";
      access(v);
      return v->agg;
    }

    /** @brief Number of vertices on the path from `u` to `v`.
     *
     *  @param u Start vertex of the path.
     *  @param v End vertex of the path.
     *  @return The number of vertices on the path between `u` and `v` (inclusive).
     *  @throws std::domain_error if `u` and `v` are not connected.
     *  @throws std::invalid_argument if either handle is null.
     */
    [[nodiscard]] size_t path_size(Node *u, Node *v)
    {
      ah_invalid_argument_if(u == nullptr or v == nullptr) << "null handle";
      make_root(u);
      ah_domain_error_if(find_root(v) != u) << "vertices not connected";
      access(v);
      return v->sz;
    }

    /** @brief Read the payload value of vertex `x`.
     *
     *  @param x Handle to the vertex.
     *  @return Constant reference to the stored value.
     *  @throws std::invalid_argument if `x` is null.
     */
    [[nodiscard]] const T &get_val(Node *x)
    {
      ah_invalid_argument_if(x == nullptr) << "null handle";
      access(x);
      return x->val;
    }

    /** @brief Update the payload value of vertex `x` and recompute aggregates.
     *
     *  @param x Handle to the vertex to update.
     *  @param val The new value to set.
     *  @throws std::invalid_argument if `x` is null.
     */
    void set_val(Node *x, const T & val)
    {
      ah_invalid_argument_if(x == nullptr) << "null handle";
      access(x);
      x->val = val;
      pull(x);
    }

    /** @brief Apply a lazy tag to every vertex on the path from `u` to `v`.
     *
     *  @details Exposes the path between `u` and `v`, then applies the given
     *  tag to the root of the auxiliary tree representing this path. The change
     *  will be pushed down efficiently (lazily) during subsequent operations.
     *
     *  Available only when `LazyTag` is not `NoLazyTag`.
     *
     *  @param u Start vertex of the path.
     *  @param v End vertex of the path.
     *  @param tag The lazy tag to apply.
     *  @throws std::domain_error if `u` and `v` are not connected.
     *  @throws std::invalid_argument if either handle is null.
     */
    void path_apply(Node *u, Node *v, const typename LazyTag::tag_type & tag)
      requires (not std::is_same_v<LazyTag, NoLazyTag<T>>)
    {
      ah_invalid_argument_if(u == nullptr or v == nullptr) << "null handle";
      make_root(u);
      ah_domain_error_if(find_root(v) != u) << "vertices not connected";
      access(v);
      v->val = LazyTag::apply(v->val, tag, 1);
      v->agg = LazyTag::apply(v->agg, tag, v->sz);
      v->lazy = LazyTag::compose(v->lazy, tag);
    }

    // ---- component & tree queries ----

    /** @brief Number of connected components (trees) in the forest.
     *  @note O(1).
     */
    [[nodiscard]] size_t num_components() const noexcept
    {
      return n_components;
    }

    /** @brief Number of vertices in the tree containing `x`.
     *
     *  @note O(n log n) — scans all vertices.  Use sparingly.
     */
    [[nodiscard]] size_t tree_size(Node *x)
    {
      ah_invalid_argument_if(x == nullptr) << "null handle";
      size_t count = 0;
      for (size_t i = 0; i < nodes.size(); ++i)
        if (connected(nodes(i), x))
          ++count;
      return count;
    }

    /** @brief Depth of `x` relative to the current root of its tree.
     *
     *  The depth is the number of edges on the path from the root
     *  to `x`.  O(log n) amortised.
     */
    [[nodiscard]] size_t depth(Node *x)
    {
      ah_invalid_argument_if(x == nullptr) << "null handle";
      access(x);
      return x->left ? x->left->sz : 0;
    }

    /** @brief Parent of `x` in the represented tree under the current rooting.
     *
     *  Returns `nullptr` if `x` is the root of its tree.
     *  O(log n) amortised.
     */
    [[nodiscard]] Node * parent(Node *x)
    {
      ah_invalid_argument_if(x == nullptr) << "null handle";
      access(x);
      if (not x->left)
        return nullptr;
      Node *p = x->left;
      push(p);
      while (p->right)
        {
          p = p->right;
          push(p);
        }
      splay(p);
      return p;
    }

    // ---- iteration ----

    /** @brief Call `op(Node *)` for every vertex in the forest. */
    template <typename Op>
    void for_each_node(Op && op) const
    {
      for (size_t i = 0; i < nodes.size(); ++i)
        op(nodes(i));
    }

    /** @brief Call `op(Node *)` for every vertex on the path from `u` to `v`,
     *         in order from `u` to `v`.
     *
     *  O(path_length log n) amortised.
     */
    template <typename Op>
    void for_each_on_path(Node *u, Node *v, Op && op)
    {
      ah_invalid_argument_if(u == nullptr or v == nullptr) << "null handle";
      make_root(u);
      ah_domain_error_if(find_root(v) != u) << "vertices not connected";

      Array<Node *> rev_path;
      for (Node *cur = v; cur != nullptr; cur = parent(cur))
        rev_path.append(cur);

      for (size_t i = rev_path.size(); i > 0; --i)
        op(rev_path(i - 1));
    }

    // ---- batch construction ----

    /** @brief Create a path of vertices from an initializer list of values.
     *
     *  Returns an `Array` of handles in order.  Each consecutive pair
     *  is linked.
     *
     *  @par Example
     *  @code
     *  auto path = lct.make_path({10, 20, 30});
     *  // path(0) — path(1) — path(2) with values 10, 20, 30
     *  @endcode
     */
    Array<Node *> make_path(std::initializer_list<T> vals)
    {
      Array<Node *> result;
      Node *prev = nullptr;
      for (const auto & v: vals)
        {
          auto *nd = make_vertex(v);
          if (prev)
            link(prev, nd);
          prev = nd;
          result.append(nd);
        }
      return result;
    }

    /** @brief Create isolated vertices from an initializer list of values.
     *
     *  @return `Array` of handles (no edges created).
     */
    Array<Node *> make_vertices(std::initializer_list<T> vals)
    {
      Array<Node *> result;
      for (const auto & v: vals)
        result.append(make_vertex(v));
      return result;
    }

    /** @brief Link every pair in an edge container.
     *
     *  @tparam Container  Iterable of `std::pair<Node *, Node *>`.
     */
    template <typename Container>
    void link_edges(const Container & edges)
    {
      for (const auto & [u, v]: edges)
        link(u, v);
    }

    // ---- snapshot export ----

    /** @brief Export the represented tree rooted at `root` as a `Tree_Node<T>`.
     *
     *  Materializes the current represented tree containing `root` into
     *  an explicit n-ary tree of `Tree_Node<T>` nodes.  A `make_root(root)`
     *  is performed internally to establish the rooting.
     *
     *  The caller owns the returned tree and must free it with
     *  `destroy_tree(result)`.
     *
     *  @param root Vertex to use as root of the exported tree.
     *  @return Newly allocated `Tree_Node<T> *` mirroring the represented tree.
     *  @throws std::invalid_argument if `root` is null.
     *
     *  @par Complexity
     *  O(n^2) where n is the number of vertices in the component
     *  (dominated by the parent-lookup sweep).
     */
    [[nodiscard]] Tree_Node<T> * export_to_tree_node(Node * root)
    {
      ah_invalid_argument_if(root == nullptr) << "null handle";
      make_root(root);

      // Collect all nodes in root's component and create Tree_Node copies
      Array<Node *> lct_nodes;
      Array<Tree_Node<T> *> tree_nodes;

      for_each_node([&](Node * nd)
      {
        if (connected(nd, root))
          {
            lct_nodes.append(nd);
            tree_nodes.append(new Tree_Node<T>(nd->val));
          }
      });

      auto find_idx = [&](Node * nd) -> size_t
      {
        for (size_t i = 0; i < lct_nodes.size(); ++i)
          if (lct_nodes(i) == nd)
            return i;
        return lct_nodes.size();
      };

      size_t root_idx = find_idx(root);
      for (size_t i = 0; i < lct_nodes.size(); ++i)
        {
          if (i == root_idx)
            continue;
          Node * par = parent(lct_nodes(i));
          if (par != nullptr)
            {
              size_t pi = find_idx(par);
              tree_nodes(pi)->insert_rightmost_child(tree_nodes(i));
            }
        }

      return tree_nodes(root_idx);
    }
  };

  /** @brief Convenience alias for a connectivity-only Link-Cut Tree.
   *  @details Uses the `DefaultMonoid` and `NoLazyTag`, meaning it only
   *  stores an `int` per vertex and is typically used just for structural
   *  operations (`link`, `cut`, `connected`, `lca`).
   */
  using Link_Cut_Tree = Gen_Link_Cut_Tree<int>;
} // namespace Aleph

# endif /* TPL_LINK_CUT_TREE_H */
