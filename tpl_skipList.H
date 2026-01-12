/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

/** @file tpl_skipList.H
 *  @brief Skip list: probabilistic sorted linked structure.
 *
 *  Multi-level linked list with random tower heights providing
 *  expected O(log n) search, insert, and delete.
 *
 *  ## Features
 *  - Simple implementation (no rotations)
 *  - Good cache locality
 *  - Lock-free variants possible
 *
 *  ## Complexity: O(log n) expected for all operations
 *
 *  @see tpl_dynSkipList.H Dynamic variant
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_SKIPLIST_H
# define TPL_SKIPLIST_H

# include <climits>
# include <ctime>
# include <gsl/gsl_rng.h>
# include <aleph.H>
# include <ah-errors.H>

namespace Aleph
{
  /** @brief Skip List - a probabilistic ordered data structure.
   *
   *  A Skip List is an ordered list where each node contains an array of
   *  forward pointers. The structure was proposed by William Pugh (1990).
   *
   *  Each node has a randomly determined level that defines how many
   *  forward pointers it contains. Higher levels allow faster traversal,
   *  giving O(log n) expected time for search, insert, and delete.
   *
   *  Key properties:
   *  - Expected O(log n) search, insert, delete
   *  - O(n) space complexity
   *  - Simple implementation compared to balanced trees
   *  - Good cache locality for sequential access
   *
   *  The class internally uses the gsl random number generator of
   *  [GSL - GNU Scientific Library](https://www.gnu.org/software/gsl/).
   *  By default, the Mersenne twister is used and the seed is taken from
   *  system time.
   *
   *  @warning Nodes must be allocated with extra space for forward pointers:
   *  `sizeof(Node) + sizeof(Node*) * level`
   *
   *  @tparam Key Key type (must be comparable with <, ==)
   *  @tparam Type Data type stored alongside keys
   *
   *  @see Pugh, W. "Skip Lists: A Probabilistic Alternative to Balanced Trees"
   *       Communications of the ACM, 1990.
   *  @ingroup Sequences
   */
  template <class Key, class Type>
  class SkipList
  {
  public:
    /// Maximum level for any node (supports up to 2^32 elements efficiently)
    static const int maxLevel = 32;

    /// Default probability for level generation (0.5 = geometric distribution)
    static const double defaultProbability;

    class Node
    {
    protected:
      int level;

      static Node nodeSentinel;

      /*
         Public methods work in function of nullptr pointer in order to indicate
         unsuccessful conditions. We put NullPtr as private in order to avoid
         that to accidentally put NullPtr instead of nullptr
      */
      static Node *NullPtr;

    private:
      Key key;
      Type data = Type();

      void fillForwardNULL()
      {
        for (int i = 0; i < level; i++)
          forward[i] = const_cast<Node *>(&nodeSentinel);
      }

    public:
      friend class SkipList<Key, Type>;

      Node(const Key & _key, const Type & _data, int n)
        : level(n),
          key(_key),
          data(_data)
      {
        /* EMPTY */
      }

      Node(const Key & _key, int n)
        : level(n),
          key(_key)
      {
        /* EMPTY */
      }

      Node(const int n)
        : level(n)
      {
        /* EMPTY */
      }

      Node()
        : level(0),
          key(computeMaxKey())
      {
        /* EMPTY */
      }

      ~Node() = default;

      [[nodiscard]] Node * get_next() const noexcept
      {
        return (forward[0] == NullPtr) ? nullptr : forward[0];
      }

      Node *&getForward(int i)
      {
        assert(i < level);
        return forward[i];
      }

      Node ** getForward()
      {
        return forward;
      }

      [[nodiscard]] const Key &get_key() const noexcept { return key; }

      Key &get_key() noexcept { return key; }

      [[nodiscard]] const Type &get_data() const noexcept { return data; }

      Type &get_data() noexcept { return data; }

      [[nodiscard]] int getLevel() const noexcept { return level; }

      /// @brief Compute the maximum possible key value (used for sentinel).
      /// @note Must be specialized for each Key type.
      static Key computeMaxKey() noexcept;

    protected:
      Node *forward[0];
    };

  private:
    /*
      Headenode : it's used for header node,
      it's an static array with maxLevel elements
    */
    class HeaderNode : public SkipList<Key, Type>::Node
    {
    private:
      typename SkipList<Key, Type>::Node *forward[maxLevel];

      friend class SkipList<Key, Type>;

    public:
      HeaderNode()
      {
        this->level = maxLevel;

        for (int i = 0; i < maxLevel; i++)
          HeaderNode::forward[i] = Node::NullPtr;
      }

      Node *&getForward(int i)
      {
        assert(i < this->getLevel());
        assert(this->getLevel() <= maxLevel);

        return HeaderNode::forward[i];
      }
    };

    /*
      Node*& getForward(Node* p, int i)
      {
      if ( p == headerPtr )
      return  ((HeaderNode*) p)->HeaderNode::forward[ i ];
      else
      return p->Node::forward[ i ];
      }
    */

    HeaderNode header; // SkipList's header
    HeaderNode *headerPtr; // Pointer to header
    gsl_rng *r;  // GSL random number generator
    double probability; // probability for randomLevel
    int level; // Current Maximum level of list linked


    void init(unsigned long seed)
    {
      r = gsl_rng_alloc(gsl_rng_mt19937);
      ah_bad_alloc_if(r == nullptr);
      gsl_rng_set(r, seed % gsl_rng_max(r));
    }

  public:
    /// Set the random number generator seed
    void set_seed(unsigned long seed) noexcept { gsl_rng_set(r, seed); }

    /// Get a pointer to the GSL random number generator
    gsl_rng * gsl_rng_object() noexcept { return r; }

    /** @brief Construct a SkipList with given seed and probability.
     *  @param[in] seed Random seed for level generation.
     *  @param[in] p Probability for level increase (default 0.5).
     */
    SkipList(unsigned long seed, double p = defaultProbability)
      : headerPtr(&header),
        r(nullptr),
        probability(p),
        level(0)
    {
      assert(probability >= 0 and probability <= 1);
      init(seed);
    }

    /** @brief Construct a SkipList with time-based seed.
     *  @param[in] p Probability for level increase (default 0.5).
     */
    explicit SkipList(double p = defaultProbability)
      : SkipList(time(nullptr), p)
    {
      // Empty
    }

    /// Destructor - frees GSL random number generator
    ~SkipList()
    {
      if (r != nullptr)
        gsl_rng_free(r);
    }

    // Disable copy (owns GSL resource)
    SkipList(const SkipList &) = delete;
    SkipList & operator=(const SkipList &) = delete;

    // Move constructor
    SkipList(SkipList && other) noexcept
      : header(std::move(other.header)),
        headerPtr(&header),
        r(other.r),
        probability(other.probability),
        level(other.level)
    {
      other.r = nullptr;
      other.level = 0;
    }

    // Move assignment
    SkipList & operator=(SkipList && other) noexcept
    {
      if (this != &other)
        {
          if (r != nullptr)
            gsl_rng_free(r);
          header = std::move(other.header);
          headerPtr = &header;
          r = other.r;
          probability = other.probability;
          level = other.level;
          other.r = nullptr;
          other.level = 0;
        }
      return *this;
    }

    /** @brief Search for a key in the skip list.
     *  @param[in] searchKey Key to search for.
     *  @return Pointer to node containing the key, or nullptr if not found.
     */
    [[nodiscard]] Node * search(const Key & searchKey) noexcept
    {
      Node *x = headerPtr;

      for (int i = level - 1; i >= 0; i--)
        while (x->getForward(i)->get_key() < searchKey)
          x = x->getForward(i);

      x = x->getForward(0);

      // Return nullptr if not found (either sentinel or different key)
      if (x == Node::NullPtr || x->get_key() != searchKey)
        return nullptr;

      return x;
    }

    /** @brief Insert a node into the skip list.
     *  @param[in] p Node to insert (must be pre-allocated with correct level).
     *  @return The inserted node.
     *  @pre p->getLevel() > 0 and p->getLevel() <= maxLevel
     */
    Node * insert(Node *p) noexcept
    {
      int i;
      Node *update[maxLevel];
      Node *x = headerPtr;

      assert(p->getLevel() > 0 and p->getLevel() <= maxLevel);

      p->fillForwardNULL();

      for (i = level - 1; i >= 0; i--)
        {
          while (x->getForward(i)->get_key() < p->get_key())
            x = x->getForward(i);

          update[i] = x;
        }

      x = x->getForward(0); // go to next node

      if (p->getLevel() > level)
        {
          for (i = level; i < p->getLevel(); i++)
            update[i] = headerPtr;
          level = p->getLevel();
        }


      for (i = 0; i < p->getLevel(); i++)
        {
          p->getForward(i) = update[i]->getForward(i);
          update[i]->getForward(i) = p;
        }

      assert(checkSkipList() == true);
      return p;
    }

    /// @brief Get the first node in the list.
    /// @return First node, or NullPtr if empty.
    [[nodiscard]] Node * get_first() noexcept
    {
      return headerPtr->HeaderNode::forward[0];
    }

    /** @brief Remove a key from the skip list.
     *  @param[in] searchKey Key to remove.
     *  @return Pointer to removed node, or nullptr if not found.
     *  @note Caller is responsible for deallocating the returned node.
     */
    [[nodiscard]] Node * remove(const Key & searchKey) noexcept
    {
      Node *update[maxLevel];
      Node *x = headerPtr;
      int i;

      for (i = level - 1; i >= 0; i--)
        {
          while (x->getForward(i)->get_key() < searchKey)
            x = x->getForward(i);
          update[i] = x;
        }

      x = x->getForward(0);

      if (x->get_key() == searchKey)
        {
          for (i = 0; i < level; i++)
            {
              if (update[i]->getForward(i) != x)
                break;
              update[i]->getForward(i) = x->getForward(i);
            }

          while (level > 0 and headerPtr->getForward(level) == nullptr)
            level--;
        }
      else
        return nullptr;

      assert(checkSkipList() == true);

      return x;
    }

    /** @brief Generate a random level for a new node.
     *  Uses GSL Mersenne Twister for high-quality randomness.
     *  @return Random level in range [1, maxLevel].
     */
    [[nodiscard]] int generateRandomLevel() noexcept
    {
      int l = 1;
      assert(probability > 0 and probability < 1);
      while (gsl_rng_uniform(r) < probability and l < maxLevel)
        ++l;
      return l;
    }

    /** @brief Verify skip list invariants (for debugging).
     *  @return true if all keys are in ascending order.
     */
    [[nodiscard]] bool checkSkipList() const noexcept
    {
      Node *node = header.get_next();
      while (node != nullptr)
        {
          if (node->get_next() == nullptr)
            break;

          if (node->get_key() > node->get_next()->get_key())
            return false;
          node = node->get_next();
        }
      return true;
    }

    /// @brief Placeholder for node allocation (override in derived classes).
    [[nodiscard]] Node * new_node() noexcept { return nullptr; }

    /** @brief Iterator for traversing SkipList elements in ascending order.
     *
     *  Provides forward iteration over all elements in the skip list.
     *  Elements are visited in key order (smallest to largest).
     *
     *  Usage:
     *  @code
     *  SkipList<int, int> sl(...);
     *  for (SkipList<int,int>::Iterator it(sl); it.has_curr(); it.next())
     *    std::cout << it.get_curr()->get_key() << '\n';
     *  @endcode
     */
    class Iterator
    {
      const SkipList *list_ptr = nullptr;
      Node *curr = nullptr;

    public:
      using Set_Type = SkipList;

      /// @brief Default constructor (invalid iterator).
      Iterator() noexcept = default;

      /// @brief Construct iterator positioned at first element.
      /// @param[in] list SkipList to iterate over.
      Iterator(const SkipList & list) noexcept
        : list_ptr(&list), curr(list.headerPtr->HeaderNode::forward[0])
      {
        // Skip sentinel if we landed on it
        if (curr == Node::NullPtr)
          curr = nullptr;
      }

      /// @brief Check if iterator has a current element.
      /// @return true if positioned on a valid element.
      [[nodiscard]] bool has_curr() const noexcept { return curr != nullptr; }

      /// @brief Check if iterator is on the last element.
      [[nodiscard]] bool is_last() const noexcept
      {
        if (curr == nullptr)
          return false;
        Node *next = curr->get_next();
        return next == nullptr;
      }

      /// @brief Get current node (throws if no current).
      /// @return Pointer to current node.
      /// @throw std::overflow_error if no current element.
      [[nodiscard]] Node * get_curr() const
      {
        ah_overflow_error_if(curr == nullptr) << "SkipList::Iterator::get_curr(): no current";

        return curr;
      }

      /// @brief Get current node without exception check.
      /// @return Pointer to current node (may be nullptr).
      [[nodiscard]] Node * get_curr_ne() const noexcept { return curr; }

      /// @brief Get current key (throws if no current).
      [[nodiscard]] const Key &get_key() const { return get_curr()->get_key(); }

      /// @brief Get current data (throws if no current).
      [[nodiscard]] const Type &get_data() const { return get_curr()->get_data(); }

      /// @brief Advance to next element (throws if no current).
      void next()
      {
        ah_overflow_error_if(curr == nullptr) << "SkipList::Iterator::next(): no current";
        next_ne();
      }

      /// @brief Advance to next element without exception check.
      void next_ne() noexcept
      {
        if (curr != nullptr)
          {
            curr = curr->get_next();
            // get_next() returns nullptr for sentinel, so we're done
          }
      }

      /// @brief Reset iterator to first element.
      void reset_first() noexcept
      {
        if (list_ptr != nullptr)
          {
            curr = list_ptr->headerPtr->HeaderNode::forward[0];
            if (curr == Node::NullPtr)
              curr = nullptr;
          }
      }

      /// @brief Reset iterator to first element (alias).
      void reset() noexcept { reset_first(); }

      /// @brief Assignment operator.
      Iterator &operator=(const Iterator & it) noexcept
      {
        list_ptr = it.list_ptr;
        curr = it.curr;
        return *this;
      }

      /// @brief Equality comparison.
      [[nodiscard]] bool operator==(const Iterator & it) const noexcept
      {
        return curr == it.curr;
      }

      /// @brief Inequality comparison.
      [[nodiscard]] bool operator!=(const Iterator & it) const noexcept
      {
        return curr != it.curr;
      }

      /// @brief Prefix increment operator.
      Iterator &operator++() noexcept
      {
        next_ne();
        return *this;
      }

      /// @brief Postfix increment operator.
      Iterator operator++(int) noexcept
      {
        Iterator ret = *this;
        next_ne();
        return ret;
      }

      /// @brief Dereference operator (returns node pointer).
      [[nodiscard]] Node * operator*() const { return get_curr(); }

      /// @brief Arrow operator (returns node pointer).
      [[nodiscard]] Node * operator->() const { return get_curr(); }
    };

    /// @brief Get iterator to first element.
    [[nodiscard]] Iterator begin() const noexcept { return Iterator(*this); }

    /// @brief Get iterator past the last element.
    [[nodiscard]] Iterator end() const noexcept { return Iterator(); }
  };

  /*
    BEWARE: Ensure that this instantiation to be unique
  */
  template <class Key, class Type>
  typename SkipList<Key, Type>::Node SkipList<Key, Type>::Node::nodeSentinel;

  template <class Key, class Type>
  typename SkipList<Key, Type>::Node *SkipList<Key, Type>::Node::NullPtr =
      &SkipList<Key, Type>::Node::nodeSentinel;


  template <class Key, class Type>
  const double SkipList<Key, Type>::defaultProbability = 0.5;
} // end namespace Aleph

# endif // TPL_SKIPLIST_H
