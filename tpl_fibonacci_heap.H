
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


/** @file tpl_fibonacci_heap.H
 *  @brief Fibonacci Heap implementation.
 *
 *  This file provides `Fibonacci_Heap`, a priority queue data structure
 *  with excellent amortized time complexity for decrease-key operations,
 *  making it ideal for algorithms like Dijkstra and Prim.
 *
 *  @ingroup Heaps
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_FIBONACCI_HEAP_H
# define TPL_FIBONACCI_HEAP_H

# include <ah-concepts.H>
# include <ahUtils.H>
# include <ah-errors.H>
# include <ahFunctional.H>
# include <vector>
# include <algorithm>
# include <memory>
# include <type_traits>
# include <utility>

namespace Aleph
{
  /**
   * @class Fibonacci_Heap
   * @brief Implementation of a Fibonacci Heap priority queue.
   *
   * A Fibonacci Heap is a collection of heap-ordered trees that supports
   * a rich set of operations with excellent amortized time complexity.
   * It is particularly well-suited for graph algorithms like Dijkstra's
   * shortest path and Prim's minimum spanning tree where decrease-key
   * operations are frequent.
   *
   * ## Time Complexities (Amortized)
   *
   * | Operation      | Time Complexity |
   * |----------------|-----------------|
   * | insert         | O(1)            |
   * | get_min        | O(1)            |
   * | extract_min    | O(log n)        |
   * | decrease_key   | O(1)            |
   * | delete_node    | O(log n)        |
   * | merge          | O(1)            |
   * | is_empty       | O(1)            |
   * | size           | O(1)            |
   *
   * ## Usage Example
   *
   * @code
   * Fibonacci_Heap<int> heap;
   * auto n1 = heap.insert(10);
   * auto n2 = heap.insert(5);
   * auto n3 = heap.insert(15);
   *
   * std::cout << heap.get_min(); // Outputs: 5
   *
   * heap.decrease_key(n3, 3);    // Decrease 15 to 3
   * std::cout << heap.get_min(); // Outputs: 3
   *
   * int min = heap.extract_min(); // Returns 3, removes it
   * @endcode
   *
   * ## Implementation Notes
   *
   * - The heap maintains a circular doubly-linked list of root trees.
   * - Each node can have multiple children organized as a circular list.
   * - The "mark" field tracks whether a node has lost a child since
   *   becoming a non-root node (used for cascading cuts).
   * - Copy operations are disabled; use move semantics or merge instead.
   *
   * @tparam T The data type of elements stored in the heap.
   * @tparam Compare The comparison functor (defaults to Aleph::less<T>
   *         for a min-heap). Use Aleph::greater<T> for a max-heap.
   *
   * @ingroup Heaps
   * @see BinHeap DynBinHeap
   */
  template <typename T, class Compare = Aleph::less<T>>
    requires StrictWeakOrder<Compare, T>
  class Fibonacci_Heap
  {
  public:
    /**
     * @struct Node
     * @brief Represents a node in the Fibonacci Heap.
     *
     * This structure is publicly accessible to allow external algorithms
     * (such as Dijkstra or Prim) to store handles (pointers) to nodes
     * and perform efficient decrease_key operations.
     *
     * @warning Users should not modify node fields directly. Use the
     *          heap's methods (decrease_key, delete_node) instead.
     */
    struct Node
    {
      T data;               ///< The data stored in this node
      Node *parent = nullptr; ///< Parent node (nullptr if root)
      Node *child = nullptr;  ///< Pointer to one child (head of child list)
      Node *left = this;      ///< Left sibling in circular list
      Node *right = this;     ///< Right sibling in circular list
      size_t degree = 0;      ///< Number of children
      bool mark = false;      ///< Has this node lost a child since becoming non-root?

      /** Construct a node copying \p d into the internal storage. */
      explicit Node(const T & d) : data(d) {}

      /** Construct a node moving \p d into the internal storage. */
      explicit Node(T && d) noexcept(std::is_nothrow_move_constructible_v<T>)
        : data(std::move(d)) {}

      /**
       * @brief Perfect-forwarding constructor for in-place construction.
       *
       * This overload allows callers of Fibonacci_Heap::emplace() to forward
       * arbitrary constructor arguments for \c T without creating temporaries.
       *
       * @tparam Arg First argument forwarded to the \c T constructor.
       * @tparam Args Remaining constructor arguments.
       * @param arg First argument forwarded to the stored value.
       * @param args Remaining arguments forwarded to the stored value.
       */
      template <typename Arg, typename... Args>
        requires ((sizeof...(Args) >= 1) or
                  (not std::is_same_v<std::decay_t<Arg>, T> and
                   not std::is_same_v<std::decay_t<Arg>, Node>))
      explicit Node(Arg&& arg, Args&&... args)
        : data(std::forward<Arg>(arg), std::forward<Args>(args)...) {}
    };

  private:
    /**
     * @brief Maximum degree tracked during consolidation.
     *
     * The degree of any Fibonacci heap root is bounded by log_phi(n). The
     * value 128 comfortably covers heaps larger than 2^64 elements.
     */
    static constexpr size_t MAX_DEGREE = 128;

    Node *min_node = nullptr;  ///< Pointer to the current minimum root.
    size_t num_nodes = 0;      ///< Number of elements stored in the heap.
    Compare cmp;               ///< Comparison functor used to order nodes.

    /**
     * @brief Reusable scratch array for Fibonacci_Heap::consolidate().
     *
     * Keeping the buffer as a member avoids heap allocations on every
     * extract_min() call, which keeps amortized costs constant.
     */
    std::vector<Node *> consolidate_array_;

    /**
     * @brief Links two trees of the same degree.
     *
     * Makes y a child of x. Assumes both are roots and x has the smaller key.
     *
     * @param y The node to become a child
     * @param x The node to become the parent
     */
    static void link(Node *y, Node *x)
    {
      // Remove y from root list
      y->left->right = y->right;
      y->right->left = y->left;

      // Make y a child of x
      y->parent = x;
      if (x->child == nullptr)
        {
          x->child = y;
          y->right = y;
          y->left = y;
        }
      else
        {
          y->left = x->child;
          y->right = x->child->right;
          x->child->right->left = y;
          x->child->right = y;
        }

      ++x->degree;
      y->mark = false;
    }

    /**
     * @brief Consolidates the root list after extract_min.
     *
     * Ensures no two roots have the same degree by linking trees
     * of equal degree until all roots have distinct degrees.
     *
     * It allocates nothing: extract_min() and delete_node() reserve
     * consolidate_array_ before they change the heap. If the comparison
     * throws, every tree is still in the root list and min_node is one of
     * its roots, perhaps not the minimum.
     */
    void consolidate()
    {
      if (min_node == nullptr)
        return;

      // Use member vector to avoid repeated allocations
      if (consolidate_array_.size() < MAX_DEGREE)
        consolidate_array_.resize(MAX_DEGREE, nullptr);
      else
        std::fill(consolidate_array_.begin(), consolidate_array_.end(), nullptr);

      // The roots are counted and then visited from min_node on, by their
      // right links. Only visited roots leave the list when they are linked,
      // so each root but the last still links to the next one when visited
      size_t num_roots = 0;
      Node *curr = min_node;
      do
        {
          ++num_roots;
          curr = curr->right;
        }
      while (curr != min_node);

      for (; num_roots > 0; --num_roots)
        {
          Node *x = curr;
          curr = curr->right;
          size_t d = x->degree;

          while (d < MAX_DEGREE && consolidate_array_[d] != nullptr)
            {
              Node *y = consolidate_array_[d];
              // Ensure x has the smaller key (maintains heap property)
              if (cmp(y->data, x->data))
                std::swap(x, y);

              link(y, x);
              min_node = x; // a root, if a later comparison throws
              consolidate_array_[d] = nullptr;
              ++d;
            }

          if (d < MAX_DEGREE)
            consolidate_array_[d] = x;
        }

      // Find the new minimum, by degree; each root moves right after the
      // minimum found so far. The roots move within the list, so it keeps
      // all of them if the comparison throws
      min_node = nullptr;
      for (size_t i = 0; i < MAX_DEGREE; ++i)
        {
          Node *x = consolidate_array_[i];
          if (x == nullptr)
            continue;

          x->parent = nullptr;
          if (min_node == nullptr)
            {
              min_node = x;
              continue;
            }

          // Move x right after min_node
          x->left->right = x->right;
          x->right->left = x->left;
          x->left = min_node;
          x->right = min_node->right;
          min_node->right->left = x;
          min_node->right = x;

          if (cmp(x->data, min_node->data))
            min_node = x;
        }
    }

    /**
     * @brief Cuts a node from its parent and adds it to the root list.
     *
     * @param x The node to cut
     * @param y The parent of x
     */
    void cut(Node *x, Node *y)
    {
      // Remove x from the child list of y
      if (x->right == x)
        y->child = nullptr;
      else
        {
          x->left->right = x->right;
          x->right->left = x->left;
          if (y->child == x)
            y->child = x->right;
        }
      --y->degree;

      // Add x to the root list
      x->left = min_node;
      x->right = min_node->right;
      min_node->right->left = x;
      min_node->right = x;
      x->parent = nullptr;
      x->mark = false;
    }

    /**
     * @brief Performs cascading cut operation.
     *
     * If y has lost a child before (mark == true), cut it from its
     * parent and recursively check the parent. This maintains the
     * amortized O(1) decrease_key complexity.
     *
     * @param y The node to potentially cut
     */
    void cascading_cut(Node *y)
    {
      while (y != nullptr)
        {
          Node *z = y->parent;
          if (z == nullptr)
            break;

          if (not y->mark)
            {
              y->mark = true;
              break;
            }
          cut(y, z);
          y = z;  // Continue with parent (iterative instead of recursive)
        }
    }

    /**
     * @brief Takes a root out of the root list, after putting its children
     *        in it as roots.
     *
     * min_node is left on any other root, or null if there is none. The
     * node keeps its data and is left without children.
     *
     * @param z The root to take out
     */
    void take_root_out(Node *z) noexcept
    {
      if (z->child != nullptr)
        {
          // First, set all children's parent to nullptr
          Node *child = z->child;
          do
            {
              child->parent = nullptr;
              child = child->right;
            }
          while (child != z->child);

          // Splice the child list into the root list
          Node *child_left = z->child->left;
          Node *z_right = z->right;

          z->right = z->child;
          z->child->left = z;
          child_left->right = z_right;
          z_right->left = child_left;

          z->child = nullptr;
          z->degree = 0;
        }

      if (z == z->right)
        min_node = nullptr; // z was the only root
      else
        {
          z->left->right = z->right;
          z->right->left = z->left;
          min_node = z->right;
        }
    }

    /**
     * @brief Undoes take_root_out() after a comparison threw.
     *
     * Puts the node back in the root list, in which consolidate() leaves
     * every tree when the comparison throws, and makes the minimum the node
     * that was it before. A consolidation may have left that node under one
     * of an equivalent key: then it is cut from it as decrease_key() cuts a
     * node, so the one that loses it is marked, or cut too if it was. So the
     * heap has the elements and the minimum that it had, though its trees
     * may be others.
     *
     * @param z The node that take_root_out() took out
     * @param min The node that was the minimum, which may be z
     */
    void put_root_back(Node *z, Node *min) noexcept
    {
      if (min_node == nullptr)
        z->left = z->right = z;
      else
        {
          z->left = min_node;
          z->right = min_node->right;
          min_node->right->left = z;
          min_node->right = z;
        }
      min_node = z; // a root for cut() to link after

      if (Node *over = min->parent; over != nullptr)
        {
          cut(min, over);
          cascading_cut(over); // it has lost a child
        }
      min_node = min;
    }

    /**
     * @brief Deletes all the nodes of the trees in a circular list.
     *
     * Used by clear() for O(n) cleanup instead of O(n log n). Before
     * deleting a node, it puts the children of the node in the list, right
     * after it, so it needs no recursion: a tree can be as high as the heap
     * is large.
     *
     * @param node A node of the list, or nullptr
     */
    void delete_all_nodes(Node *node)
    {
      if (node == nullptr)
        return;

      // Break the circular list to avoid use-after-free when checking
      // loop termination condition against a deleted node
      node->left->right = nullptr;

      Node *curr = node;
      while (curr != nullptr)
        {
          if (curr->child != nullptr) // its children go next in the list
            {
              Node *last_child = curr->child->left;
              last_child->right = curr->right;
              curr->right = curr->child;
            }
          Node *next = curr->right;
          delete curr;
          curr = next;
        }
    }

    /**
     * @brief Adds a node to the root list.
     *
     * The keys are compared before the node enters the list, so that if
     * the comparison throws the heap is as it was.
     *
     * @param node The node to add (must not be in any list)
     */
    void add_to_root_list(Node *node)
    {
      node->parent = nullptr;
      if (min_node == nullptr)
        {
          min_node = node;
          node->left = node;
          node->right = node;
        }
      else
        {
          const bool is_min = cmp(node->data, min_node->data);

          node->left = min_node;
          node->right = min_node->right;
          min_node->right->left = node;
          min_node->right = node;

          if (is_min)
            min_node = node;
        }
    }

    // The heap takes node, made with new by the caller. If the comparison
    // throws, add_to_root_list() has not linked node yet, and the guard
    // frees it. The guard is not a unique_ptr: its release() would say
    // that the caller takes the node, and the node is the heap's
    Node * insert_new_node(Node *node)
    {
      struct Guard
      {
        Node *node;
        ~Guard() { delete node; }
      } guard{node};

      add_to_root_list(node);
      guard.node = nullptr;  // the heap took node
      ++num_nodes;
      return node;
    }

  public:
    /// Type alias for the element type
    using value_type = T;

    /// Type alias for the comparison functor
    using key_compare = Compare;

    /// Type alias for node handles
    using handle_type = Node *;

    /**
     * @brief Default constructor.
     *
     * Creates an empty heap using a copy of \p compare to order keys.
     *
     * @param compare Comparison functor (defaults to \c Compare()).
     */
    explicit Fibonacci_Heap(Compare compare = Compare())
      noexcept(std::is_nothrow_copy_constructible_v<Compare>)
      : cmp(compare)
    {
      // empty: extract_min() and delete_node() reserve the room of
      // consolidate() before they change the heap
    }

    /**
 * @brief Constructor with in-place comparator construction.
 *
 * Allows constructing the comparator from arbitrary arguments.
 * Use std::in_place as first argument to disambiguate.
 *
 * @tparam Args Types of comparator constructor arguments
 * @param args Arguments forwarded to Compare's constructor
 */
    template <typename... Args>
    explicit Fibonacci_Heap(std::in_place_t, Args&&... args)
      noexcept(std::is_nothrow_constructible_v<Compare, Args&&...>)
      : cmp(std::forward<Args>(args)...)
    {
      // empty, as the other constructor
    }

    /**
     * @brief Destructor.
     *
     * Deallocates all nodes. Runs in O(n) time.
     */
    ~Fibonacci_Heap()
    {
      clear();
    }

    /// Copy constructor is disabled (use merge or manual copy)
    Fibonacci_Heap(const Fibonacci_Heap &) = delete;

    /// Copy assignment is disabled (use merge or manual copy)
    Fibonacci_Heap &operator=(const Fibonacci_Heap &) = delete;

    /**
     * @brief Move constructor.
     *
     * Transfers ownership of all nodes from other to this heap.
     * The source heap becomes empty.
     *
     * @param other The heap to move from
     */
    Fibonacci_Heap(Fibonacci_Heap && other)
      noexcept(std::is_nothrow_move_constructible_v<Compare>)
      : min_node(other.min_node),
        num_nodes(other.num_nodes),
        cmp(std::move(other.cmp)),
        consolidate_array_(std::move(other.consolidate_array_))
    {
      other.min_node = nullptr;
      other.num_nodes = 0;
    }

    /**
     * @brief Move assignment operator.
     *
     * Clears this heap and transfers ownership from other.
     *
     * @param other The heap to move from
     * @return Reference to this heap
     * @throws anything that the destructor of T, in clear(), or the move
     *         assignment of the comparison throws. If the comparison
     *         throws, this heap is empty and other keeps its elements.
     */
    Fibonacci_Heap &operator=(Fibonacci_Heap && other)
      noexcept(std::is_nothrow_move_assignable_v<Compare> and
               std::is_nothrow_destructible_v<T>)
    {
      if (this != &other)
        {
          clear();
          // The comparison before the nodes: if its move throws, other
          // still owns them, and this heap none
          cmp = std::move(other.cmp);
          consolidate_array_ = std::move(other.consolidate_array_);
          min_node = other.min_node;
          num_nodes = other.num_nodes;
          other.min_node = nullptr;
          other.num_nodes = 0;
        }
      return *this;
    }

    /**
     * @brief Swaps contents with another heap.
     *
     * Exchanges all nodes between this heap and other in O(1) time.
     *
     * @param other The heap to swap with
     * @throws anything that the swap of the comparisons throws: the swap
     *         that is_nothrow_swappable looks at, the one of Compare if it
     *         has one, else std::swap. Then the nodes stay where they were.
     */
    void swap(Fibonacci_Heap & other)
      noexcept(std::is_nothrow_swappable_v<Compare>)
    {
      using std::swap;
      swap(cmp, other.cmp); // before the nodes, which then cannot fail
      std::swap(min_node, other.min_node);
      std::swap(num_nodes, other.num_nodes);
      std::swap(consolidate_array_, other.consolidate_array_);
    }

    /**
     * @brief Inserts a new element (copy).
     *
     * Creates a new node with a copy of val and adds it to the heap.
     *
     * @param val The value to insert
     * @return A handle (pointer) to the created node for later operations
     * @throws anything that the comparison throws. Then the heap is as it
     *         was.
     *
     * @note Amortized O(1) time complexity
     * @note The return value can be safely ignored if you don't need
     *       the handle for decrease_key or delete_node operations.
     */
    [[nodiscard]] Node * insert(const T & val)
    {
      return insert_new_node(new Node(val));
    }

    /**
     * @brief Inserts a new element (move).
     *
     * Creates a new node by moving val into it.
     *
     * @param val The value to insert (will be moved)
     * @return A handle to the created node
     * @throws anything that the comparison throws. Then the heap is as it
     *         was.
     *
     * @note Amortized O(1) time complexity
     */
    [[nodiscard]]Node * insert(T && val)
    {
      return insert_new_node(new Node(std::move(val)));
    }

    /**
     * @brief Constructs and inserts an element in-place.
     *
     * Constructs the element directly in the node using the provided
     * arguments, avoiding unnecessary copies or moves.
     *
     * @tparam Args Types of constructor arguments
     * @param args Arguments to forward to T's constructor
     * @return A handle to the created node
     * @throws anything that the comparison throws. Then the heap is as it
     *         was.
     *
     * @note Amortized O(1) time complexity
     */
    template <typename... Args>
    [[nodiscard]] Node * emplace(Args&&... args)
    {
      return insert_new_node(new Node(std::forward<Args>(args)...));
    }

    /**
     * @brief Returns the minimum element without removing it.
     *
     * @return Const reference to the minimum element
     * @throws std::underflow_error if the heap is empty
     *
     * @note O(1) time complexity
     */
    [[nodiscard]] const T & get_min() const
    {
      ah_underflow_error_if(is_empty()) << "Fibonacci_Heap::get_min: heap is empty";
      return min_node->data;
    }

    /**
     * @brief Returns a pointer to the minimum node.
     *
     * Useful when you need the handle rather than just the value.
     *
     * @return Pointer to the minimum node, or nullptr if empty
     *
     * @note O(1) time complexity
     */
    [[nodiscard]] Node * get_min_node() const noexcept
    {
      return min_node;
    }

    /**
     * @brief Extracts and returns the minimum element.
     *
     * Removes the minimum element from the heap and returns it.
     * The heap is restructured (consolidated) after removal.
     *
     * @return The minimum element (moved out)
     * @throws std::underflow_error if the heap is empty
     * @throws anything that the comparison throws. Then the heap has the
     *         elements that it had, the minimum among them, and every
     *         handle is valid; its trees may be linked otherwise.
     *
     * @note Amortized O(log n) time complexity
     */
    T extract_min()
    {
      ah_underflow_error_if(is_empty()) << "Fibonacci_Heap::extract_min: heap is empty";

      // A heap that was moved from has no room for consolidate(); it is
      // made before the heap changes, in case the allocation fails
      consolidate_array_.reserve(MAX_DEGREE);

      Node *z = min_node;
      take_root_out(z);
      if (min_node != nullptr)
        try
          {
            consolidate();
          }
        catch (...)
          {
            put_root_back(z, z);
            throw;
          }

      // z is out of the heap: it is freed and counted out from here on
      std::unique_ptr<Node> node(z);
      --num_nodes;

      return std::move(z->data);
    }

    /**
     * @brief Decreases the key of a node.
     *
     * Decreases the key of node x to the new value k. If this violates
     * the heap property with x's parent, x is cut and added to the root
     * list, potentially triggering cascading cuts.
     *
     * @param x Handle to the node (returned by insert/emplace)
     * @param k The new key value (must be <= current key)
     * @throws std::invalid_argument if x is nullptr
     * @throws std::domain_error if k > current key
     * @throws anything that the comparison throws. Then the heap is as it
     *         was, and x has its old key.
     * @throws anything that the assignment of the key throws. Then the heap
     *         is as it was but for the key of x, which is as the assignment
     *         left it: if that is not its old key, x may be out of order.
     *
     * @note Amortized O(1) time complexity
     */
    void decrease_key(Node *x, const T & k)
    {
      ah_invalid_argument_if(x == nullptr)
        << "Fibonacci_Heap::decrease_key: null node pointer";
      ah_domain_error_if(cmp(x->data, k))
        << "Fibonacci_Heap::decrease_key: new key is greater than current key";

      // The comparisons take the new key, and come before the heap changes
      Node *y = x->parent;
      const bool is_cut = y != nullptr and cmp(k, y->data);
      const bool is_min = cmp(k, min_node->data);

      x->data = k;
      if (is_cut)
        {
          cut(x, y);
          cascading_cut(y);
        }

      if (is_min)
        min_node = x;
    }

    /**
     * @brief Decreases the key of a node (move version).
     *
     * @param x Handle to the node
     * @param k The new key value (moved into node)
     * @throws std::invalid_argument if x is nullptr
     * @throws std::domain_error if k > current key
     * @throws anything that the comparison or the assignment of the key
     *         throw, as the copy version; if the comparison throws, k is
     *         not moved from.
     */
    void decrease_key(Node *x, T && k)
    {
      ah_invalid_argument_if(x == nullptr)
        << "Fibonacci_Heap::decrease_key: null node pointer";
      ah_domain_error_if(cmp(x->data, k))
        << "Fibonacci_Heap::decrease_key: new key is greater than current key";

      // The comparisons take the new key, and come before the heap changes
      Node *y = x->parent;
      const bool is_cut = y != nullptr and cmp(k, y->data);
      const bool is_min = cmp(k, min_node->data);

      x->data = std::move(k);
      if (is_cut)
        {
          cut(x, y);
          cascading_cut(y);
        }

      if (is_min)
        min_node = x;
    }

    /**
     * @brief Updates the key of a node (increase or decrease).
     *
     * Unlike decrease_key, this method handles both increases and
     * decreases. For increases, the node leaves its tree and goes back to
     * the heap as a root with the new key, as if it were deleted and
     * inserted again, but it is the same node: no node is allocated.
     *
     * @param x Handle to the node
     * @param k The new key value
     * @return Handle to the node, which is x
     * @throws std::invalid_argument if x is nullptr
     * @throws anything that the comparison throws. Then x has its old key,
     *         and the heap has the elements that it had, the minimum among
     *         them, and every handle is valid; if the key increases, its
     *         trees may be linked otherwise.
     * @throws anything that the copy of the key throws. If the key
     *         increases, the copy is made before the heap changes, which is
     *         then as it was, and it is moved into the node at the end: the
     *         above holds if that move assignment does not throw. If the
     *         key decreases, see decrease_key().
     * @throws std::bad_alloc if the key increases and there is no memory
     *         for the array of the consolidation, which is reserved once,
     *         before the heap changes.
     *
     * @note O(1) amortized if decreasing, O(log n) amortized if increasing
     */
    [[nodiscard]] Node * update_key(Node *x, const T & k)
    {
      ah_invalid_argument_if(x == nullptr)
        << "Fibonacci_Heap::update_key: null node pointer";

      if (cmp(k, x->data))
        {
          // New key is smaller, use decrease_key
          decrease_key(x, k);
          return x;
        }
      if (not cmp(x->data, k))
        return x; // Keys are equal, do nothing

      // New key is larger: x leaves the heap as in delete_node() and, once
      // the rest is in order, goes back as a root with the new key, which
      // is copied first, while a copy that throws changes nothing
      T key(k);
      consolidate_array_.reserve(MAX_DEGREE);

      Node *old_min = min_node;
      if (x->parent != nullptr)
        {
          Node *parent = x->parent;
          cut(x, parent);
          cascading_cut(parent);
        }
      take_root_out(x);

      bool is_min = true; // if x is the only node
      try
        {
          if (min_node != nullptr)
            {
              consolidate();
              is_min = cmp(k, min_node->data);
            }
          x->data = std::move(key);
        }
      catch (...)
        {
          put_root_back(x, old_min);
          throw;
        }

      Node *min = is_min ? x : min_node;
      put_root_back(x, min);
      return x;
    }

    /**
     * @brief Deletes a specific node from the heap.
     *
     * Removes the node x from the heap regardless of its position.
     *
     * @param x Handle to the node to delete
     * @throws std::invalid_argument if x is nullptr
     * @throws anything that the comparison throws. Then x is still in the
     *         heap, which has the elements that it had, the minimum among
     *         them, and every handle is valid; its trees may be linked
     *         otherwise.
     *
     * @note Amortized O(log n) time complexity
     *
     * @warning After this call, if it does not throw, x is invalid and must
     *          not be used.
     */
    void delete_node(Node *x)
    {
      ah_invalid_argument_if(x == nullptr)
        << "Fibonacci_Heap::delete_node: null node pointer";

      // As in extract_min(), before the heap changes
      consolidate_array_.reserve(MAX_DEGREE);

      Node *old_min = min_node;

      // If x is not a root, cut it and perform cascading cuts
      if (x->parent != nullptr)
        {
          Node *parent = x->parent;  // Save parent BEFORE cut
          cut(x, parent);
          cascading_cut(parent);     // Use saved parent
        }

      // x leaves the root list as the minimum does in extract_min(), and
      // the rest is put in order, which finds the true minimum
      take_root_out(x);
      if (min_node != nullptr)
        try
          {
            consolidate();
          }
        catch (...)
          {
            put_root_back(x, old_min);
            throw;
          }

      --num_nodes;
      delete x;
    }

    /**
     * @brief Merges another heap into this one.
     *
     * All nodes from other are transferred to this heap.
     * The other heap becomes empty after this operation.
     *
     * @param other The heap to merge (will be emptied)
     * @throws anything that the comparison throws. Then both heaps are as
     *         they were.
     *
     * @note O(1) time complexity
     *
     * @warning Both heaps must use compatible comparison functors.
     */
    void merge(Fibonacci_Heap & other)
    {
      if (&other == this || other.is_empty())
        return;

      if (is_empty())
        {
          min_node = other.min_node;
          num_nodes = other.num_nodes;
        }
      else
        {
          // Compared before the lists are joined, so that if the comparison
          // throws each heap keeps its own nodes
          const bool other_is_min = cmp(other.min_node->data, min_node->data);

          // Concatenate root lists
          Node *this_right = min_node->right;
          Node *other_left = other.min_node->left;

          min_node->right = other.min_node;
          other.min_node->left = min_node;
          other_left->right = this_right;
          this_right->left = other_left;

          // Update minimum if necessary
          if (other_is_min)
            min_node = other.min_node;

          num_nodes += other.num_nodes;
        }

      // Clear the other heap (nodes now belong to us)
      other.min_node = nullptr;
      other.num_nodes = 0;
    }

    /**
     * @brief Merges another heap into this one (rvalue version).
     *
     * @param other Temporary heap whose contents will be transferred.
     *
     * @note This overload simply forwards to the lvalue reference version,
     *       enabling idioms such as \c merge(Fibonacci_Heap{...}).
     */
    void merge(Fibonacci_Heap && other)
    {
      merge(other);
    }

    /**
     * @brief Checks if the heap is empty.
     *
     * @return true if the heap has no elements, false otherwise
     *
     * @note O(1) time complexity
     */
    [[nodiscard]] bool is_empty() const noexcept
    {
      return min_node == nullptr;
    }

    /**
     * @brief Returns the number of elements in the heap.
     *
     * @return Number of elements
     *
     * @note O(1) time complexity
     */
    [[nodiscard]] size_t size() const noexcept
    {
      return num_nodes;
    }

    /**
     * @brief Removes all elements from the heap.
     *
     * Deallocates all nodes and resets the heap to empty state.
     *
     * @note O(n) time complexity
     */
    void clear() noexcept(std::is_nothrow_destructible_v<T>)
    {
      if (min_node != nullptr)
        {
          delete_all_nodes(min_node);
          min_node = nullptr;
          num_nodes = 0;
        }
    }

    [[nodiscard]] bool empty() const noexcept
    {
      return is_empty();
    }

    void pop()
    {
      extract_min();
    }

    [[nodiscard]] const T & top() const
    {
      return get_min();
    }

    /**
     * @brief Returns the comparison functor.
     *
     * @return Copy of the comparison functor
     */
    [[nodiscard]] Compare key_comp() const
    {
      return cmp;
    }

    /**
     * @brief Returns the comparison functor that orders the heap.
     *
     * Changing it while the heap has elements may leave the heap out of
     * order.
     *
     * @return Reference to the comparison functor
     */
    [[nodiscard]] Compare & get_compare() noexcept
    {
      return cmp;
    }
  };

  /**
   * @brief Swaps two Fibonacci heaps.
   *
   * @relates Fibonacci_Heap
   */
  template <typename T, class Compare>
  void swap(Fibonacci_Heap<T, Compare> & a, Fibonacci_Heap<T, Compare> & b)
    noexcept(noexcept(a.swap(b)))
  {
    a.swap(b);
  }

} // namespace Aleph

# endif // TPL_FIBONACCI_HEAP_H