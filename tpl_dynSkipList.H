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

# ifndef TPL_DYNSKIPLIST_H
# define TPL_DYNSKIPLIST_H

# include <climits>
# include <ctime>
# include <utility>
# include <initializer_list>
# include <gsl/gsl_rng.h>
# include <ah-errors.H>
# include <ah-args-ctor.H>
# include <ahDry.H>
# include <ahFunctional.H>
# include <ahIterator.H>
# include <ah-iterator.H>

namespace Aleph {

# include <ah-dry.H>

/** @brief Dynamic ordered set implemented with a Skip List.
 *
 *  DynSkipList<Key, Compare> is a dynamic set that automatically manages
 *  memory for its elements. It uses a Skip List as the underlying data
 *  structure, providing expected O(log n) time for search, insert, and
 *  delete operations.
 *
 *  The class internally uses the gsl random number generator of
 *  [GSL - GNU Scientific Library](https://www.gnu.org/software/gsl/).
 *  By default, the Mersenne twister is used and the seed is taken from
 *  system time.
 *
 *  @tparam Key Type of elements stored in the set.
 *  @tparam Compare Comparison functor (default: std::less<Key>).
 *
 *  @see DynSetTree
 *  @ingroup Secuencias
 */
template <typename Key, class Compare = std::less<Key>>
class DynSkipList
      : public GenericTraverse<DynSkipList<Key, Compare>>,
        public SpecialCtors<DynSkipList<Key, Compare>, Key>,
        public LocateFunctions<DynSkipList<Key, Compare>, Key>,
        public FunctionalMethods<DynSkipList<Key, Compare>, Key>,
        public GenericItems<DynSkipList<Key, Compare>, Key>,
        public StlAlephIterator<DynSkipList<Key, Compare>>
{
public:

  /// The type of container
  using Set_Type = DynSkipList;

  /// The type of element that stores the container
  using Item_Type = Key;

  /// The type of element that stores the container
  using Key_Type = Key;
  
  static constexpr int maxLevel = 32;
  static constexpr double defaultProbability = 0.5;

private:

  struct Node
  {
    Key key;
    int level;
    Node ** forward;

    Node(const Key & k, int lvl) : key(k), level(lvl)
    {
      forward = new Node*[lvl];
      for (int i = 0; i < lvl; ++i)
        forward[i] = nullptr;
    }

    Node(Key && k, int lvl) : key(std::move(k)), level(lvl)
    {
      forward = new Node*[lvl];
      for (int i = 0; i < lvl; ++i)
        forward[i] = nullptr;
    }

    // Dummy header node constructor
    explicit Node(int lvl) : level(lvl)
    {
      forward = new Node*[lvl];
      for (int i = 0; i < lvl; ++i)
        forward[i] = nullptr;
    }

    ~Node()
    {
      delete[] forward;
    }
  };

  Node * header;
  gsl_rng * r;  // GSL random number generator
  size_t num_nodes = 0;
  int current_level = 1;
  double probability;
  Compare cmp;

  void init(unsigned long seed)
  {
    r = gsl_rng_alloc(gsl_rng_mt19937);
    ah_bad_alloc_if(r == nullptr);
    gsl_rng_set(r, seed % gsl_rng_max(r));
  }

  int random_level() const noexcept
  {
    int lvl = 1;
    while (gsl_rng_uniform(r) < probability && lvl < maxLevel)
      ++lvl;
    return lvl;
  }

public:

  /// Set the random number generator seed
  void set_seed(unsigned long seed) noexcept { gsl_rng_set(r, seed); }

  /// Get a pointer to the GSL random number generator
  gsl_rng * gsl_rng_object() noexcept { return r; }

  /** @brief Construct a DynSkipList with given seed and probability.
   *  @param[in] seed Random seed for level generation.
   *  @param[in] p Probability for level increase (default 0.5).
   */
  DynSkipList(unsigned long seed, double p = defaultProbability)
    : r(nullptr), probability(p)
  {
    header = new Node(maxLevel);
    init(seed);
  }

  /** @brief Construct a DynSkipList with time-based seed.
   *  @param[in] p Probability for level increase (default 0.5).
   */
  explicit DynSkipList(double p = defaultProbability)
    : DynSkipList(time(nullptr), p)
  {
    // Empty
  }


  Special_Ctors(DynSkipList, Key);

  DynSkipList(const DynSkipList & other)
    : r(nullptr), probability(other.probability)
  {
    header = new Node(maxLevel);
    init(time(nullptr));  // New GSL RNG for the copy
    for (auto it = other.begin(); it.has_curr(); it.next())
      insert(it.get_curr());
  }

  DynSkipList(DynSkipList && other) noexcept
    : header(other.header),
      r(other.r),
      num_nodes(other.num_nodes),
      current_level(other.current_level),
      probability(other.probability)
  {
    other.header = new Node(maxLevel);
    other.r = nullptr;
    other.num_nodes = 0;
    other.current_level = 1;
  }

  ~DynSkipList()
  {
    empty();
    delete header;
    if (r != nullptr)
      gsl_rng_free(r);
  }

  DynSkipList & operator=(const DynSkipList & other)
  {
    if (this == &other)
      return *this;

    empty();
    probability = other.probability;
    // Keep our own GSL RNG
    
    for (auto it = other.begin(); it.has_curr(); it.next())
      insert(it.get_curr());

    return *this;
  }

  DynSkipList & operator=(DynSkipList && other) noexcept
  {
    if (this == &other)
      return *this;

    empty();
    delete header;
    if (r != nullptr)
      gsl_rng_free(r);
    
    header = other.header;
    r = other.r;
    num_nodes = other.num_nodes;
    current_level = other.current_level;
    probability = other.probability;
    
    other.header = new Node(maxLevel);
    other.r = nullptr;
    other.num_nodes = 0;
    other.current_level = 1;
    
    return *this;
  }

  void swap(DynSkipList & other) noexcept
  {
    std::swap(header, other.header);
    std::swap(r, other.r);
    std::swap(num_nodes, other.num_nodes);
    std::swap(current_level, other.current_level);
    std::swap(probability, other.probability);
  }

  void empty() noexcept
  {
    Node * curr = header->forward[0];
    while (curr != nullptr)
      {
        Node * next = curr->forward[0];
        delete curr;
        curr = next;
      }
    for (int i = 0; i < maxLevel; ++i)
      header->forward[i] = nullptr;
    num_nodes = 0;
    current_level = 1;
  }

  [[nodiscard]] size_t size() const noexcept { return num_nodes; }
  [[nodiscard]] bool is_empty() const noexcept { return num_nodes == 0; }

  Key * insert(const Key & key)
  {
    Node * update[maxLevel];
    Node * x = header;

    // Find position
    for (int i = current_level - 1; i >= 0; --i)
      {
        while (x->forward[i] != nullptr && cmp(x->forward[i]->key, key))
          x = x->forward[i];
        update[i] = x;
      }

    x = x->forward[0];

    // Key already exists?
    if (x != nullptr && !cmp(key, x->key) && !cmp(x->key, key))
      return nullptr;

    int lvl = random_level();

    if (lvl > current_level)
      {
        for (int i = current_level; i < lvl; ++i)
          update[i] = header;
        current_level = lvl;
      }

    Node * new_node = new Node(key, lvl);

    for (int i = 0; i < lvl; ++i)
      {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
      }

    ++num_nodes;
    return &new_node->key;
  }

  Key * insert(Key && key)
  {
    Node * update[maxLevel];
    Node * x = header;

    for (int i = current_level - 1; i >= 0; --i)
      {
        while (x->forward[i] != nullptr && cmp(x->forward[i]->key, key))
          x = x->forward[i];
        update[i] = x;
      }

    x = x->forward[0];

    if (x != nullptr && !cmp(key, x->key) && !cmp(x->key, key))
      return nullptr;

    int lvl = random_level();

    if (lvl > current_level)
      {
        for (int i = current_level; i < lvl; ++i)
          update[i] = header;
        current_level = lvl;
      }

    Node * new_node = new Node(std::move(key), lvl);

    for (int i = 0; i < lvl; ++i)
      {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
      }

    ++num_nodes;
    return &new_node->key;
  }

  /// Append method for Special_Ctors compatibility (aliases to insert)
  Key & append(const Key & key) { return *insert(key); }
  Key & append(Key && key) { return *insert(std::move(key)); }

  Key * search_or_insert(const Key & key)
  {
    Key * existing = search(key);
    if (existing != nullptr)
      return existing;
    return insert(key);
  }

  [[nodiscard]] Key * search(const Key & key) const noexcept
  {
    Node * x = header;
    
    for (int i = current_level - 1; i >= 0; --i)
      while (x->forward[i] != nullptr && cmp(x->forward[i]->key, key))
        x = x->forward[i];

    x = x->forward[0];
    
    if (x != nullptr && !cmp(key, x->key) && !cmp(x->key, key))
      return const_cast<Key*>(&x->key);

    return nullptr;
  }

  [[nodiscard]] bool has(const Key & key) const noexcept
  {
    return search(key) != nullptr;
  }

  [[nodiscard]] bool contains(const Key & key) const noexcept { return has(key); }
  [[nodiscard]] bool exist(const Key & key) const noexcept { return has(key); }

  [[nodiscard]] Key & find(const Key & key)
  {
    Key * ptr = search(key);
    ah_domain_error_if(ptr == nullptr)
      << "DynSkipList::find: key not found";
    return *ptr;
  }

  [[nodiscard]] const Key & find(const Key & key) const
  {
    Key * ptr = search(key);
    ah_domain_error_if(ptr == nullptr)
      << "DynSkipList::find: key not found";
    return *ptr;
  }

  size_t remove(const Key & key)
  {
    Node * update[maxLevel];
    Node * x = header;

    for (int i = current_level - 1; i >= 0; --i)
      {
        while (x->forward[i] != nullptr && cmp(x->forward[i]->key, key))
          x = x->forward[i];
        update[i] = x;
      }

    x = x->forward[0];

    if (x == nullptr || cmp(key, x->key) || cmp(x->key, key))
      return num_nodes;

    for (int i = 0; i < current_level; ++i)
      {
        if (update[i]->forward[i] != x)
          break;
        update[i]->forward[i] = x->forward[i];
      }

    delete x;

    while (current_level > 1 && header->forward[current_level - 1] == nullptr)
      --current_level;

    return --num_nodes;
  }

  Key del(const Key & key)
  {
    Node * update[maxLevel];
    Node * x = header;

    for (int i = current_level - 1; i >= 0; --i)
      {
        while (x->forward[i] != nullptr && cmp(x->forward[i]->key, key))
          x = x->forward[i];
        update[i] = x;
      }

    x = x->forward[0];

    ah_domain_error_if(x == nullptr || cmp(key, x->key) || cmp(x->key, key))
      << "DynSkipList::del: key not found";

    Key ret_val = std::move(x->key);

    for (int i = 0; i < current_level; ++i)
      {
        if (update[i]->forward[i] != x)
          break;
        update[i]->forward[i] = x->forward[i];
      }

    delete x;

    while (current_level > 1 && header->forward[current_level - 1] == nullptr)
      --current_level;

    --num_nodes;
    return ret_val;
  }

  [[nodiscard]] const Key & min() const
  {
    ah_domain_error_if(is_empty()) << "DynSkipList::min: set is empty";
    return header->forward[0]->key;
  }

  [[nodiscard]] const Key & get_first() const { return min(); }

  [[nodiscard]] const Key & max() const
  {
    ah_domain_error_if(is_empty()) << "DynSkipList::max: set is empty";
    
    Node * x = header->forward[0];
    while (x->forward[0] != nullptr)
      x = x->forward[0];
    return x->key;
  }

  [[nodiscard]] const Key & get_last() const { return max(); }

  // =========================================================================
  // Iterator
  // =========================================================================

  class Iterator
  {
    const DynSkipList * list_ptr = nullptr;
    Node * curr = nullptr;

  public:
    using Set_Type = DynSkipList;

    Iterator() noexcept = default;

    explicit Iterator(const DynSkipList & list) noexcept
      : list_ptr(&list), curr(list.header->forward[0])
    {}

    [[nodiscard]] bool has_curr() const noexcept { return curr != nullptr; }

    [[nodiscard]] bool is_last() const noexcept
    {
      return curr != nullptr && curr->forward[0] == nullptr;
    }

    [[nodiscard]] const Key & get_curr() const
    {
      ah_overflow_error_if(curr == nullptr) << "DynSkipList::Iterator overflow";
      return curr->key;
    }

    [[nodiscard]] const Key & get_curr_ne() const noexcept
    {
      return curr->key;
    }

    Key & get_curr()
    {
      ah_overflow_error_if(curr == nullptr) << "DynSkipList::Iterator overflow";
      return curr->key;
    }

    [[nodiscard]] const Key & get_key() const { return get_curr(); }

    void next()
    {
      ah_overflow_error_if(curr == nullptr) << "DynSkipList::Iterator::next overflow";
      next_ne();
    }

    void next_ne() noexcept
    {
      if (curr != nullptr)
        curr = curr->forward[0];
    }

    void reset_first() noexcept
    {
      if (list_ptr != nullptr)
        curr = list_ptr->header->forward[0];
    }

    void reset() noexcept { reset_first(); }

    Iterator & operator=(const Iterator &) noexcept = default;

    [[nodiscard]] bool operator==(const Iterator & it) const noexcept
    {
      return curr == it.curr;
    }

    [[nodiscard]] bool operator!=(const Iterator & it) const noexcept
    {
      return curr != it.curr;
    }

    Iterator & operator++() noexcept
    {
      next_ne();
      return *this;
    }

    Iterator operator++(int) noexcept
    {
      Iterator ret = *this;
      next_ne();
      return ret;
    }

    [[nodiscard]] const Key & operator*() const { return get_curr(); }
    [[nodiscard]] const Key * operator->() const { return &get_curr(); }
  };

  [[nodiscard]] Iterator begin() const noexcept { return Iterator(*this); }
  [[nodiscard]] Iterator end() const noexcept { return Iterator(); }

  // =========================================================================
  // Functional operations
  // =========================================================================

  template <class Operation>
  bool traverse(Operation & op) const
  {
    for (auto it = begin(); it.has_curr(); it.next())
      if (!op(it.get_curr()))
        return false;
    return true;
  }

  template <class Operation>
  bool traverse(Operation && op = Operation()) const
  {
    return traverse<Operation>(op);
  }

  template <class Operation>
  void for_each(Operation & op) const
  {
    for (auto it = begin(); it.has_curr(); it.next())
      op(it.get_curr());
  }

  template <class Operation>
  void for_each(Operation && op = Operation()) const
  {
    for_each<Operation>(op);
  }

  template <class Pred>
  bool all(Pred & pred) const
  {
    for (auto it = begin(); it.has_curr(); it.next())
      if (!pred(it.get_curr()))
        return false;
    return true;
  }

  template <class Pred>
  bool all(Pred && pred = Pred()) const
  {
    return all<Pred>(pred);
  }

  template <class Pred>
  bool exists(Pred & pred) const
  {
    for (auto it = begin(); it.has_curr(); it.next())
      if (pred(it.get_curr()))
        return true;
    return false;
  }

  template <class Pred>
  bool exists(Pred && pred = Pred()) const
  {
    return exists<Pred>(pred);
  }
};

} // end namespace Aleph

# endif // TPL_DYNSKIPLIST_H
