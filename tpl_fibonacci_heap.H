
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

# include <ahUtils.H>
# include <ah-errors.H>
# include <ahFunctional.H>
# include <vector>
# include <algorithm>
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
      template <typename Arg, typename... Args,
                std::enable_if_t<(sizeof...(Args) >= 1) ||
                                 (!std::is_same_v<std::decay_t<Arg>, T> &&
                                  !std::is_same_v<std::decay_t<Arg>, Node>), int> = 0>
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

      // Collect all root nodes to iterate safely while modifying links
      std::vector<Node *> root_nodes;
      Node *curr = min_node;
      do
        {
          root_nodes.push_back(curr);
          curr = curr->right;
        }
      while (curr != min_node);

      for (Node *w : root_nodes)
        {
          Node *x = w;
          size_t d = x->degree;

          while (d < MAX_DEGREE && consolidate_array_[d] != nullptr)
            {
              Node *y = consolidate_array_[d];
              // Ensure x has the smaller key (maintains heap property)
              if (cmp(y->data, x->data))
                std::swap(x, y);

              link(y, x);
              consolidate_array_[d] = nullptr;
              ++d;
            }

          if (d < MAX_DEGREE)
            consolidate_array_[d] = x;
        }

      // Reconstruct root list and find new minimum
      min_node = nullptr;
      for (size_t i = 0; i < MAX_DEGREE; ++i)
        {
          if (consolidate_array_[i] != nullptr)
            {
              consolidate_array_[i]->parent = nullptr;
              if (min_node == nullptr)
                {
                  min_node = consolidate_array_[i];
                  min_node->left = min_node;
                  min_node->right = min_node;
                }
              else
                {
                  // Insert into root list
                  consolidate_array_[i]->left = min_node;
                  consolidate_array_[i]->right = min_node->right;
                  min_node->right->left = consolidate_array_[i];
                  min_node->right = consolidate_array_[i];

                  if (cmp(consolidate_array_[i]->data, min_node->data))
                    min_node = consolidate_array_[i];
                }
            }
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
     * @brief Recursively deletes all nodes in a tree.
     *
     * Used by clear() for O(n) cleanup instead of O(n log n).
     *
     * @param node Root of the subtree to delete
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
          Node *next = curr->right;
          // Recursively delete children
          if (curr->child != nullptr)
            delete_all_nodes(curr->child);
          delete curr;
          curr = next;
        }
    }

    /**
     * @brief Adds a node to the root list.
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
          node->left = min_node;
          node->right = min_node->right;
          min_node->right->left = node;
          min_node->right = node;

          if (cmp(node->data, min_node->data))
            min_node = node;
        }
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
    explicit Fibonacci_Heap(Compare compare = Compare()) noexcept
      : cmp(compare)
    {
      consolidate_array_.reserve(MAX_DEGREE);
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
    explicit Fibonacci_Heap(std::in_place_t, Args&&... args) noexcept
      : cmp(std::forward<Args>(args)...)
    {
      consolidate_array_.reserve(MAX_DEGREE);
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
    Fibonacci_Heap(Fibonacci_Heap && other) noexcept
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
     */
    Fibonacci_Heap &operator=(Fibonacci_Heap && other) noexcept
    {
      if (this != &other)
        {
          clear();
          min_node = other.min_node;
          num_nodes = other.num_nodes;
          cmp = std::move(other.cmp);
          consolidate_array_ = std::move(other.consolidate_array_);
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
     */
    void swap(Fibonacci_Heap & other) noexcept
    {
      std::swap(min_node, other.min_node);
      std::swap(num_nodes, other.num_nodes);
      std::swap(cmp, other.cmp);
      std::swap(consolidate_array_, other.consolidate_array_);
    }

    /**
     * @brief Inserts a new element (copy).
     *
     * Creates a new node with a copy of val and adds it to the heap.
     *
     * @param val The value to insert
     * @return A handle (pointer) to the created node for later operations
     *
     * @note Amortized O(1) time complexity
     * @note The return value can be safely ignored if you don't need
     *       the handle for decrease_key or delete_node operations.
     */
    [[nodiscard]] Node * insert(const T & val)
    {
      Node *node = new Node(val);
      add_to_root_list(node);
      ++num_nodes;
      return node;
    }

    /**
     * @brief Inserts a new element (move).
     *
     * Creates a new node by moving val into it.
     *
     * @param val The value to insert (will be moved)
     * @return A handle to the created node
     *
     * @note Amortized O(1) time complexity
     */
    [[nodiscard]]Node * insert(T && val)
    {
      Node *node = new Node(std::move(val));
      add_to_root_list(node);
      ++num_nodes;
      return node;
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
     *
     * @note Amortized O(1) time complexity
     */
    template <typename... Args>
    [[nodiscard]] Node * emplace(Args&&... args)
    {
      Node *node = new Node(std::forward<Args>(args)...);
      add_to_root_list(node);
      ++num_nodes;
      return node;
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
     *
     * @note Amortized O(log n) time complexity
     */
    T extract_min()
    {
      ah_underflow_error_if(is_empty()) << "Fibonacci_Heap::extract_min: heap is empty";

      Node *z = min_node;

      // Add all children of z to the root list
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
        }

      // Remove z from root list
      if (z == z->right)
        {
          // z was the only root
          min_node = nullptr;
        }
      else
        {
          z->left->right = z->right;
          z->right->left = z->left;
          min_node = z->right;
          consolidate();
        }

      --num_nodes;
      T data = std::move(z->data);
      delete z;
      return data;
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
     *
     * @note Amortized O(1) time complexity
     */
    void decrease_key(Node *x, const T & k)
    {
      ah_invalid_argument_if(x == nullptr)
        << "Fibonacci_Heap::decrease_key: null node pointer";
      ah_domain_error_if(cmp(x->data, k))
        << "Fibonacci_Heap::decrease_key: new key is greater than current key";

      x->data = k;
      Node *y = x->parent;

      if (y != nullptr && cmp(x->data, y->data))
        {
          cut(x, y);
          cascading_cut(y);
        }

      if (cmp(x->data, min_node->data))
        min_node = x;
    }

    /**
     * @brief Decreases the key of a node (move version).
     *
     * @param x Handle to the node
     * @param k The new key value (moved into node)
     */
    void decrease_key(Node *x, T && k)
    {
      ah_invalid_argument_if(x == nullptr)
        << "Fibonacci_Heap::decrease_key: null node pointer";
      ah_domain_error_if(cmp(x->data, k))
        << "Fibonacci_Heap::decrease_key: new key is greater than current key";

      x->data = std::move(k);
      Node *y = x->parent;

      if (y != nullptr && cmp(x->data, y->data))
        {
          cut(x, y);
          cascading_cut(y);
        }

      if (cmp(x->data, min_node->data))
        min_node = x;
    }

    /**
     * @brief Updates the key of a node (increase or decrease).
     *
     * Unlike decrease_key, this method handles both increases and
     * decreases. For increases, it deletes and reinserts the node.
     *
     * @param x Handle to the node
     * @param k The new key value
     * @return Handle to the node (may be different if key increased)
     * @throws std::invalid_argument if x is nullptr
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
      if (cmp(x->data, k))
        {
          // New key is larger, delete and reinsert
          delete_node(x);
          return insert(k);
        }
      // Keys are equal, do nothing
      return x;
    }

    /**
     * @brief Deletes a specific node from the heap.
     *
     * Removes the node x from the heap regardless of its position.
     *
     * @param x Handle to the node to delete
     * @throws std::invalid_argument if x is nullptr
     *
     * @note Amortized O(log n) time complexity
     *
     * @warning After this call, x is invalid and must not be used.
     */
    void delete_node(Node *x)
    {
      ah_invalid_argument_if(x == nullptr)
        << "Fibonacci_Heap::delete_node: null node pointer";

      // Step 1: If x is not a root, cut it and perform cascading cuts
      if (x->parent != nullptr)
        {
          Node *parent = x->parent;  // Save parent BEFORE cut
          cut(x, parent);
          cascading_cut(parent);     // Use saved parent
        }

      // Step 2: Make x the minimum (by temporarily pointing min_node to it)
      min_node = x;

      // Step 3: Extract x (which is now the "minimum")
      // We need to do extract_min logic but without returning data
      if (x->child != nullptr)
        {
          Node *child = x->child;
          do
            {
              child->parent = nullptr;
              child = child->right;
            }
          while (child != x->child);

          // Splice children into root list
          Node *child_left = x->child->left;
          Node *x_right = x->right;

          if (x == x->right)
            {
              // x was alone, children become the root list
              min_node = x->child;
            }
          else
            {
              x->right = x->child;
              x->child->left = x;
              child_left->right = x_right;
              x_right->left = child_left;
            }
        }

      // Remove x from root list
      if (x == x->right)
        {
          // x was alone in root list
          if (min_node == x)
            min_node = nullptr;  // x had no children either
          else
            consolidate();  // x had children, need to find true minimum
        }
      else
        {
          x->left->right = x->right;
          x->right->left = x->left;
          if (min_node == x)
            min_node = x->right;
          consolidate();
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
          // Concatenate root lists
          Node *this_right = min_node->right;
          Node *other_left = other.min_node->left;

          min_node->right = other.min_node;
          other.min_node->left = min_node;
          other_left->right = this_right;
          this_right->left = other_left;

          // Update minimum if necessary
          if (cmp(other.min_node->data, min_node->data))
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
  };

  /**
   * @brief Swaps two Fibonacci heaps.
   *
   * @relates Fibonacci_Heap
   */
  template <typename T, class Compare>
  void swap(Fibonacci_Heap<T, Compare> & a, Fibonacci_Heap<T, Compare> & b) noexcept
  {
    a.swap(b);
  }

} // namespace Aleph

# endif // TPL_FIBONACCI_HEAP_H