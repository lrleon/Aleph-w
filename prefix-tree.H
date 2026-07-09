
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

/** @file prefix-tree.H
 *  @brief Trie (prefix tree) implementation.
 *
 *  Efficient string prefix search structure for autocomplete,
 *  dictionary lookup, and prefix matching. Prefer Prefix_Tree for ordinary
 *  use; Cnode remains available as the low-level node API.
 *
 *  ## Features
 *  - O(k) lookup for k-length strings
 *  - Prefix search support
 *  - Memory-efficient for shared prefixes
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

#ifndef PREFIX_TREE_H
#define PREFIX_TREE_H

#include <cassert>
#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include <tpl_tree_node.H>
#include <tpl_dynArray.H>
#include <tpl_dynList.H>
#include <ah-errors.H>

namespace Aleph {
/** @brief Low-level prefix tree node for storing character sequences.
 *
 * Cnode implements a prefix tree (also known as a trie or digital tree)
 * where each node represents a character. Words are stored as paths
 * from the root. A node-local flag marks complete word endings, so every
 * character value can be used as ordinary input data.
 *
 * This is the legacy/manual node API. A Cnode does not own its descendants in
 * its destructor; callers that build trees directly with Cnode must call
 * destroy() before the root node is destroyed. Prefer Prefix_Tree when a
 * normal owning container is desired.
 *
 * ## Features
 *
 * - **Insert**: O(m) where m is word length
 * - **Search**: O(m) where m is word length
 * - **Prefix search**: O(m) where m is prefix length
 * - **Space**: O(ALPHABET_SIZE * N * M) worst case
 *
 * ## Low-level Usage Example
 *
 * ```cpp
 * // Create a manual root node. Its symbol is not part of stored words.
 * Cnode root('\0');
 *
 * // Insert words
 * root.insert_word("hello");
 * root.insert_word("help");
 * root.insert_word("world");
 *
 * // Search for words
 * if (root.contains("hello"))
 *   std::cout << "Found 'hello'\n";
 *
 * // Get all words
 * auto all_words = root.words();
 * all_words.for_each([](const std::string& w) {
 *   std::cout << w << "\n";
 * });
 *
 * // Search by prefix
 * auto [node, remaining] = root.search_prefix("hel");
 * // node points to 'l' in "hel", remaining is ""
 *
 * // Manual cleanup is required when using Cnode directly.
 * root.destroy();
 * ```
 *
 * ## Implementation Details
 *
 * - Children are stored in sorted order by character for efficient lookup
 * - Word endings are marked by state stored in the terminal node
 * - The tree uses a leftmost-child, right-sibling representation
 * - to_str() reports structural nodes only; word-ending flags are not shown
 *   as sentinel children
 *
 * @see Prefix_Tree
 * @see Tree_Node
 * @ingroup Trees
 */
class Cnode : public Tree_Node<char>
{
  bool ends_word_ = false;

  /** @brief Shared implementation for mutable and const prefix searches.
   *
   * @tparam Node Either `Cnode` or `const Cnode`.
   * @param[in] root Node where the search starts.
   * @param[in] prefix Null-terminated prefix to search for.
   * @return Tuple of (node reached, remaining unmatched suffix).
   *         If the entire prefix was found, remaining is "".
   *
   * @pre `root` and `prefix` must not be nullptr.
   */
  template <typename Node>
  [[nodiscard]] static std::tuple<Node *, const char *> search_prefix_impl(Node *root,
                                                                           const char *prefix) noexcept
  {
    assert(root != nullptr);
    assert(prefix != nullptr);

    auto node = root;
    for (const char *ptr = prefix; *ptr != '\0'; ++ptr)
      {
        auto *child = node->search_child(*ptr);
        if (child == nullptr)
          return std::make_tuple(node, ptr);
        node = child;
      }

    return std::make_tuple(node, "");
  }

  /** @brief Shared implementation for mutable and const child lookup.
   *
   * @tparam Node Either `Cnode` or `const Cnode`.
   * @param[in] root Node whose children are searched.
   * @param[in] c Character to search for.
   * @return Pointer to the matching child, or nullptr if not found.
   *
   * @pre `root` must not be nullptr.
   */
  template <typename Node>
  [[nodiscard]] static Node *search_child_impl(Node *root, const char c) noexcept
  {
    assert(root != nullptr);

    for (auto *child = root->get_left_child(); child != nullptr; child = child->get_right_sibling())
      {
        const char key = child->get_key();
        if (key == c)
          return static_cast<Node *>(child);
        if (key > c)
          return nullptr;
      }

    return nullptr;
  }

  /** @brief Shared implementation for mutable and const sorted-child lookup.
   *
   * @tparam Node Either `Cnode` or `const Cnode`.
   * @param[in] root Node whose children are searched.
   * @param[in] c Character threshold.
   * @return Pointer to the first child with char > c, or nullptr.
   *
   * @pre `root` must not be nullptr.
   */
  template <typename Node>
  [[nodiscard]] static Node *greater_child_impl(Node *root, const char c) noexcept
  {
    assert(root != nullptr);

    for (auto *child = root->get_left_child(); child != nullptr; child = child->get_right_sibling())
      if (child->get_key() > c)
        return static_cast<Node *>(child);
    return nullptr;
  }

  /** @brief Own a detached subtree until it is committed elsewhere.
   *
   * The guarded node must be a standalone tree root, not a node already
   * linked into a live parent.
   */
  class Detached_Subtree_Guard
  {
    Cnode *root_ = nullptr;

  public:
    explicit Detached_Subtree_Guard(Cnode *root) noexcept : root_(root)
    {
      // empty
    }

    ~Detached_Subtree_Guard()
    {
      if (root_ != nullptr)
        {
          root_->destroy();
          delete root_;
        }
    }

    [[nodiscard]] Cnode *get() const noexcept
    {
      return root_;
    }

    [[nodiscard]] Cnode *release() noexcept
    {
      Cnode *ret = root_;
      root_ = nullptr;
      return ret;
    }
  };

  /** @brief Own standalone nodes until an insertion path is committed. */
  class Pending_Path_Guard
  {
    Cnode **nodes_ = nullptr;
    size_t count_ = 0;
    bool owns_nodes_ = true;

  public:
    explicit Pending_Path_Guard(const size_t count) : nodes_(new Cnode *[count]), count_(count)
    {
      for (size_t i = 0; i < count_; ++i)
        nodes_[i] = nullptr;
    }

    ~Pending_Path_Guard()
    {
      if (owns_nodes_)
        for (size_t i = 0; i < count_; ++i)
          delete nodes_[i];
      delete[] nodes_;
    }

    [[nodiscard]] Cnode *operator [] (const size_t i) const noexcept
    {
      return nodes_[i];
    }

    void set(const size_t i, Cnode *node) const noexcept
    {
      nodes_[i] = node;
    }

    void release_nodes() noexcept
    {
      owns_nodes_ = false;
    }
  };

  /** @brief Restore a clone target if appending cloned children fails. */
  class Clone_Target_Rollback
  {
    Cnode *target_ = nullptr;
    Tree_Node<char> *previous_rightmost_ = nullptr;
    bool previous_ends_word_ = false;
    bool active_ = true;

  public:
    explicit Clone_Target_Rollback(Cnode *target) noexcept : target_(target)
    {
      assert(target_ != nullptr);
      previous_rightmost_ = target_->get_right_child();
      previous_ends_word_ = target_->ends_word_;
    }

    void release() noexcept
    {
      active_ = false;
    }

    ~Clone_Target_Rollback()
    {
      if (not active_)
        return;

      while (target_->get_right_child() != previous_rightmost_)
        {
          auto *child = static_cast<Cnode *>(target_->get_right_child());
          const bool child_was_leftmost = child->is_leftmost();
          destroy_tree(child);
          if (child_was_leftmost)
            {
              if (previous_rightmost_ == nullptr)
                target_->set_is_leaf(true);
              break;
            }
        }

      target_->ends_word_ = previous_ends_word_;
    }
  };

public:
  /** @brief Construct a node with the given character.
   *
   * @param[in] c Character to store in this node.
   */
  explicit Cnode(const char c) noexcept
  {
    this->get_key() = c;
  }

  /** @brief Return the character stored in this node.
   *
   * @return The character value.
   */
  [[nodiscard]] char symbol() const noexcept
  {
    return get_key();
  }

  /** @brief Return a list of all child nodes.
   *
   * @return DynList containing pointers to all children.
   *
   * @note This creates a new list on each call. For iteration,
   *       consider using for_each_child() directly.
   */
  [[nodiscard]] DynList<Cnode *> children() const
  {
    DynList<Cnode *> r;
    for_each_child([&r](Tree_Node<char> *p)
      {
        r.append(static_cast<Cnode *>(p));
      });
    return r;
  }

  /** @brief Convert the subtree to a string representation.
   *
   * @return String representation of the tree structure.
   */
  [[nodiscard]] std::string to_str() const
  {
    std::string ret(1, symbol());
    for_each_child([&ret](const Tree_Node<char> *p)
      {
        const auto *node = static_cast<const Cnode *>(p);
        ret += "(" + node->to_str() + ")";
      });
    return ret;
  }

  /** @brief Check if this node marks the end of a word.
   *
   * A word ending is stored as node state instead of as a child sentinel.
   *
   * @return true if this node ends a word, false otherwise.
   */
  [[nodiscard]] bool is_end_word() const noexcept
  {
    return ends_word_;
  }

  /** @brief Mark this node as the end of a word.
   *
   * Sets the node-local word-ending flag.
   *
   * @pre The node must not already be marked as a word ending.
   */
  void mark_end_word()
  {
    assert(not is_end_word());
    ends_word_ = true;
  }

  /** @brief Search for a mutable child with the given character.
   *
   * @param[in] c Character to search for.
   * @return Pointer to the child node, or nullptr if not found.
   */
  [[nodiscard]] Cnode *search_child(const char c) noexcept
  {
    return search_child_impl(this, c);
  }

  /** @brief Search for a const child with the given character.
   *
   * @param[in] c Character to search for.
   * @return Const pointer to the child node, or nullptr if not found.
   */
  [[nodiscard]] const Cnode *search_child(const char c) const noexcept
  {
    return search_child_impl(this, c);
  }

  /** @brief Find the first mutable child with a character greater than c.
   *
   * Used for maintaining sorted order of children.
   *
   * @param[in] c Character threshold.
   * @return Pointer to the first child with char > c, or nullptr.
   */
  [[nodiscard]] Cnode *greater_child(const char c) noexcept
  {
    return greater_child_impl(this, c);
  }

  /** @brief Find the first const child with a character greater than c.
   *
   * Used for inspecting the sorted child order.
   *
   * @param[in] c Character threshold.
   * @return Const pointer to the first child with char > c, or nullptr.
   */
  [[nodiscard]] const Cnode *greater_child(const char c) const noexcept
  {
    return greater_child_impl(this, c);
  }

  /** @brief Search for a prefix in the mutable tree.
   *
   * Traverses the tree following the characters in prefix.
   *
   * @param[in] prefix Null-terminated string to search for.
   * @return Tuple of (node reached, remaining unmatched suffix).
   *         If the entire prefix was found, remaining is "".
   *
   * @pre `prefix` must not be nullptr.
   */
  [[nodiscard]] std::tuple<Cnode *, const char *> search_prefix(const char *prefix) noexcept
  {
    return search_prefix_impl(this, prefix);
  }

  /** @brief Search for a prefix in the const tree.
   *
   * Traverses the tree following the characters in prefix.
   *
   * @param[in] prefix Null-terminated string to search for.
   * @return Tuple of (node reached, remaining unmatched suffix).
   *         If the entire prefix was found, remaining is "".
   *
   * @pre `prefix` must not be nullptr.
   */
  [[nodiscard]] std::tuple<const Cnode *, const char *> search_prefix(const char *prefix) const noexcept
  {
    return search_prefix_impl(this, prefix);
  }

  /** @brief Search for a complete word in the tree.
   *
   * @param[in] word Null-terminated string to search for.
   * @return Pointer to the ending node if found, nullptr otherwise.
   *
   * @pre `word` must not be nullptr.
   */
  [[nodiscard]] const Cnode *search_word(const char *word) const noexcept
  {
    assert(word != nullptr);

    auto node = this;
    for (const char *ptr = word; *ptr != '\0'; ++ptr)
      {
        node = node->search_child(*ptr);
        if (node == nullptr)
          return nullptr;
      }

    return node->is_end_word() ? node : nullptr;
  }

  /** @brief Check if a word exists in the tree.
   *
   * @param[in] word Word to search for.
   * @return true if the word exists, false otherwise.
   */
  [[nodiscard]] bool contains(const std::string &word) const noexcept
  {
    return search_word(word.c_str()) != nullptr;
  }

  /** @brief Count total words stored in this subtree.
   *
   * @return Number of complete words.
   */
  [[nodiscard]] size_t count() const noexcept
  {
    size_t n = is_end_word() ? 1 : 0;
    for_each_child([&n](const Tree_Node<char> *p)
      {
        n += static_cast<const Cnode *>(p)->count();
      });
    return n;
  }

  /** @brief Get all words starting with a given prefix.
   *
   * @param[in] prefix Prefix to search for.
   * @param[in] max_word_length Maximum expected word length (default 2048).
   * @return Array of words that start with prefix.
   *
   * @note An empty prefix returns the same set of words as words().
   * @pre `max_word_length` must be at least `prefix.size()` and large
   *      enough for every emitted word when assertions are enabled.
   */
  [[nodiscard]] DynArray<std::string> words_with_prefix(const std::string &prefix,
                                                        const size_t max_word_length = 2048) const
  {
    auto [node, remaining] = search_prefix(prefix.c_str());

    // Prefix not fully matched
    if (*remaining != '\0')
      return {};

    assert(prefix.size() <= max_word_length);
    DynArray<std::string> ret_val;
    std::string word(prefix);
    word.reserve(max_word_length);

    node->words_impl(word, ret_val, max_word_length);
    return ret_val;
  }

  /** @brief Insert a child node in sorted order.
   *
   * Children are maintained in ascending order by character.
   *
   * @param[in] child Node to insert.
   * @return Pointer to the inserted child.
   *
   * @pre No child with the same character should exist.
   */
  Cnode *insert_child(Cnode *child)
  {
    assert(search_child(child->symbol()) == nullptr);

    if (Cnode *sibling = greater_child(child->symbol()); sibling == nullptr)
      this->insert_rightmost_child(child);
    else
      sibling->insert_left_sibling(child);

    return child;
  }

  /** @brief Insert a word into the tree.
   *
   * Creates all necessary nodes for the word and marks the ending.
   *
   * @param[in] word Word to insert.
   * @return true if the word was inserted, false if it already existed.
   *
   * @throw std::bad_alloc If memory allocation fails.
   *
   * @note Provides the strong guarantee for allocation failures: if inserting
   *       a new path throws, the trie is restored to its previous logical
   *       contents.
   */
  bool insert_word(const std::string &word)
  {
    auto [pp, rem] = search_prefix(word.c_str());

    if (*rem == '\0')
      {
        if (pp->is_end_word())
          return false;
        pp->mark_end_word();
        return true;
      }

    size_t n = 0;
    for (const char *ptr = rem; *ptr; ++ptr)
      ++n;

    Pending_Path_Guard path(n);
    for (size_t i = 0; i < n; ++i)
      path.set(i, new Cnode(rem[i]));

    path[n - 1]->mark_end_word();
    path.release_nodes();

    auto *parent = pp->insert_child(path[0]);
    for (size_t i = 1; i < n; ++i)
      {
        parent->insert_rightmost_child(path[i]);
        parent = path[i];
      }

    return true;
  }

  /** @brief Destroy all children of this node.
   *
   * Recursively deletes all descendant nodes.
   * The node itself is not deleted.
   */
  void destroy() noexcept
  {
    for (auto p = static_cast<Cnode *>(get_right_child()); p != nullptr;)
      {
        Cnode *to_delete = p;
        p = static_cast<Cnode *>(p->get_left_sibling());
        destroy_tree(to_delete);
      }
    this->set_is_leaf(true);
  }

private:
  void words_impl(std::string &word, DynArray<std::string> &l, const size_t max_word_length) const
  {
    if (is_end_word())
      l.append(word);

    for (const Tree_Node<char> *child = get_left_child(); child != nullptr;
         child = child->get_right_sibling())
      {
        assert(word.size() < max_word_length);
        const auto *node = static_cast<const Cnode *>(child);
        word.push_back(node->symbol());
        node->words_impl(word, l, max_word_length);
        word.pop_back();
      }
  }

public:
  /** @brief Get all words stored in this subtree.
   *
   * @param[in] max_word_length Maximum expected word length (default 2048).
   * @return Array containing all words.
   *
   * @note Reuses a single temporary string buffer during traversal.
   * @pre `max_word_length` must be large enough for every stored word when
   *      assertions are enabled.
   */
  [[nodiscard]] DynArray<std::string> words(size_t max_word_length = 2048) const
  {
    DynArray<std::string> ret_val;
    std::string word;
    word.reserve(max_word_length);

    words_impl(word, ret_val, max_word_length);

    return ret_val;
  }

  /** @brief Print all words to stdout.
   *
   * @param[in] max_word_length Maximum expected word length (default 2048).
   */
  void print_words(const size_t max_word_length = 2048) const
  {
    words(max_word_length)
      .for_each([](const std::string &w)
      {
        std::cout << w << '\n';
      });
  }

  /** @brief Clone helper - copies children from src to tgt.
   *
   * @param[in] src Source node to copy from.
   * @param[out] tgt Target node to copy to.
   *
   * @throw std::bad_alloc If memory allocation fails.
   *
   * @note Provides the strong guarantee for `tgt`: if cloning throws, any
   *       children appended by this call are removed and the previous
   *       word-ending flag is restored.
   */
  static void clone(const Tree_Node<char> *src, Tree_Node<char> *tgt)
  {
    const auto *src_node = static_cast<const Cnode *>(src);
    auto *tgt_node = static_cast<Cnode *>(tgt);
    Clone_Target_Rollback rollback(tgt_node);
    tgt_node->ends_word_ = src_node->ends_word_;

    for (const Tree_Node<char> *src_child = src_node->get_left_child(); src_child != nullptr;
         src_child = src_child->get_right_sibling())
      {
        const auto *child = static_cast<const Cnode *>(src_child);
        Detached_Subtree_Guard cloned_child(new Cnode(child->symbol()));
        Cnode *tgt_child = cloned_child.get();
        tgt_node->insert_rightmost_child(tgt_child);
        (void) cloned_child.release();
        clone(child, tgt_child);
      }

    rollback.release();
  }

  /** @brief Create a deep copy of this subtree.
   *
   * @return Pointer to the cloned tree root.
   *
   * @throw std::bad_alloc If memory allocation fails.
   *
   * @note Provides the strong guarantee: no partially cloned tree leaks if
   *       allocation fails.
   */
  [[nodiscard]] Cnode *clone() const
  {
    Detached_Subtree_Guard ret(new Cnode(symbol()));
    clone(this, ret.get());
    return ret.release();
  }
};

/** @brief Owning prefix tree wrapper.
 *
 * Prefix_Tree owns a root Cnode and destroys all descendant nodes in its
 * destructor. It provides the same common word-level operations as Cnode,
 * while preserving Cnode as the low-level node API for legacy code and
 * manual tree manipulation.
 *
 * Copies are deep copies. The root returned by root() is non-owning; callers
 * must not delete it or call destroy_tree() on it.
 *
 * @par Thread-safety
 * None. This is a sequential container; callers must externally synchronize
 * concurrent access to the same object. This includes concurrent `const`
 * calls to count() or size(), because those functions may refresh the mutable
 * cached word count after the low-level mutable root() API has been exposed.
 *
 * @ingroup Trees
 */
class Prefix_Tree
{
  Cnode *root_ = nullptr;
  mutable size_t word_count_ = 0;
  mutable bool count_dirty_ = false;
  mutable bool mutable_root_exposed_ = false;

  /** @brief Destroy an owned root and all its descendants. */
  static void destroy_root(Cnode *root) noexcept
  {
    if (root == nullptr)
      return;

    root->destroy();
    delete root;
  }

  /** @brief Swap internal ownership and cached count state. */
  void swap_state(Prefix_Tree &other) noexcept
  {
    std::swap(root_, other.root_);
    std::swap(word_count_, other.word_count_);
    std::swap(count_dirty_, other.count_dirty_);
    std::swap(mutable_root_exposed_, other.mutable_root_exposed_);
  }

public:
  /** @brief Construct an empty prefix tree.
   *
   * The root node uses the conventional `'\0'` symbol; the symbol is not
   * part of any stored word.
   *
   * @throw std::bad_alloc If allocating the root node fails.
   */
  Prefix_Tree() : root_(new Cnode('\0'))
  {
    // empty
  }

  /** @brief Construct a deep copy of another prefix tree.
   *
   * @param[in] other Tree to copy.
   *
   * @throw std::bad_alloc If allocating any copied node fails.
   */
  Prefix_Tree(const Prefix_Tree &other)
    : root_(other.root_->clone()), word_count_(other.count()),
      count_dirty_(false)
  {
    // empty
  }

  /** @brief Move-construct, taking ownership of another tree's contents.
   *
   * @param[in,out] other Tree to move from. Left as a valid empty tree.
   *
   * @throw std::bad_alloc If allocating the replacement empty root for
   *        `other` fails; in that case `other` is left unchanged.
   */
  Prefix_Tree(Prefix_Tree &&other) : Prefix_Tree()
  {
    swap_state(other);
  }

  /** @brief Destroy the owned root and all descendants.
   *
   * This destructor does not throw.
   */
  ~Prefix_Tree() noexcept
  {
    destroy_root(root_);
  }

  /** @brief Replace this tree with a deep copy of another tree.
   *
   * @param[in] other Tree to copy from.
   * @return Reference to this tree.
   *
   * @throw std::bad_alloc If allocating any copied node fails.
   *
   * @note Provides the strong guarantee: if copying fails, this tree keeps
   *       its previous contents.
   */
  Prefix_Tree &operator = (const Prefix_Tree &other)
  {
    if (this == &other)
      return *this;

    Cnode *new_root = other.root_->clone();
    const size_t new_count = other.count();
    destroy_root(root_);
    root_ = new_root;
    word_count_ = new_count;
    count_dirty_ = false;
    mutable_root_exposed_ = false;
    return *this;
  }

  /** @brief Move-assign, taking ownership of another tree's contents.
   *
   * @param[in,out] other Tree to move from. Left as a valid empty tree.
   * @return Reference to this tree.
   *
   * @throw std::bad_alloc If allocating the replacement empty root for
   *        `other` fails; in that case both trees are left unchanged.
   */
  Prefix_Tree &operator = (Prefix_Tree &&other)
  {
    if (this == &other)
      return *this;

    Prefix_Tree tmp(std::move(other));
    swap_state(tmp);
    return *this;
  }

  /** @brief Insert a word into the tree.
   *
   * @param[in] word Word to insert.
   * @return true if the word was inserted, false if it already existed.
   *
   * @throw std::bad_alloc If memory allocation fails.
   */
  bool insert_word(const std::string &word)
  {
    const bool inserted = root_->insert_word(word);
    if (inserted and not count_dirty_ and not mutable_root_exposed_)
      ++word_count_;
    return inserted;
  }

  /** @brief Check whether a word exists in the tree.
   *
   * @param[in] word Word to search for.
   * @return true if the word exists, false otherwise.
   */
  [[nodiscard]] bool contains(const std::string &word) const noexcept
  {
    return root_->contains(word);
  }

  /** @brief Get all words stored in the tree.
   *
   * @param[in] max_word_length Maximum expected word length (default 2048).
   * @return Array containing all words.
   *
   * @throw std::bad_alloc If memory allocation fails.
   *
   * @pre `max_word_length` must be large enough for every stored word when
   *      assertions are enabled.
   */
  [[nodiscard]] DynArray<std::string> words(const size_t max_word_length = 2048) const
  {
    return root_->words(max_word_length);
  }

  /** @brief Get all words with a given prefix.
   *
   * @param[in] prefix Prefix to search for.
   * @param[in] max_word_length Maximum expected word length (default 2048).
   * @return Array containing all matching words.
   *
   * @throw std::bad_alloc If memory allocation fails.
   *
   * @pre `max_word_length` must be at least `prefix.size()` and large
   *      enough for every emitted word when assertions are enabled.
   */
  [[nodiscard]] DynArray<std::string> words_with_prefix(const std::string &prefix,
                                                        const size_t max_word_length = 2048) const
  {
    return root_->words_with_prefix(prefix, max_word_length);
  }

  /** @brief Count the words stored in the tree.
   *
   * @return Number of complete words.
   *
   * @note This is O(1) while the tree is modified only through Prefix_Tree.
   *       If the mutable root() API was used, this recomputes in O(n), because
   *       external code may keep the root pointer and mutate the low-level
   *       Cnode API between calls.
   *
   * @warning This `const` method may update the internal mutable cache. It is
   *          not safe to call concurrently on the same tree without external
   *          synchronization.
   */
  [[nodiscard]] size_t count() const noexcept
  {
    if (root_ == nullptr)
      return 0;

    if (count_dirty_ or mutable_root_exposed_)
      {
        word_count_ = root_->count();
        count_dirty_ = false;
      }
    return word_count_;
  }

  /** @brief Return the number of words stored in the tree.
   *
   * @return Number of complete words.
   *
   * @note Alias for count(); it has the same cache-refresh and thread-safety
   *       caveats.
   */
  [[nodiscard]] size_t size() const noexcept
  {
    return count();
  }

  /** @brief Return the mutable root node.
   *
   * @return Non-owning pointer to the root node.
   *
   * @note The returned root remains owned by this Prefix_Tree. Do not delete
   *       it or call destroy_tree() on it.
   */
  [[nodiscard]] Cnode *root() noexcept
  {
    count_dirty_ = true;
    mutable_root_exposed_ = true;
    return root_;
  }

  /** @brief Return the const root node.
   *
   * @return Non-owning pointer to the root node.
   *
   * @note The returned root remains owned by this Prefix_Tree. Do not delete
   *       it or call destroy_tree() on it.
   */
  [[nodiscard]] const Cnode *root() const noexcept
  {
    return root_;
  }
};

/** @brief Owning prefix tree map from strings to values.
 *
 * Prefix_Tree_Map is the mapped-value counterpart of Prefix_Tree. It uses the
 * same one-character-per-edge trie shape, but terminal nodes store a mapped
 * value instead of only a word-ending flag. It is intentionally a separate
 * type so the legacy Prefix_Tree and Cnode APIs remain source-compatible.
 *
 * Insert, exact lookup, and prefix lookup are O(k) in the length of the key
 * plus the cost of scanning sibling lists at each level. size() and count()
 * are O(1). words() and words_with_prefix() return independent arrays of
 * copied keys.
 *
 * @par Recursion depth and long keys
 * Unlike `Aleph::RadixTree` (tpl_radix_tree.H), which compresses chains of
 * single-child nodes into one edge, `Prefix_Tree_Map` never compresses:
 * a single key of length k always creates (or reuses) k nodes on its own
 * path. Destruction, deep copy, and `words()`/`words_with_prefix()` all
 * recurse to that depth, so a single very long key (tens of thousands of
 * characters) can exhaust the stack. For workloads with long individual
 * keys or long shared prefixes, prefer `RadixTree`.
 *
 * @par Keys containing embedded NUL bytes
 * Internally, key traversal is done through `std::string::c_str()`, which
 * stops at the first `'\0'`. Two distinct `std::string` keys that only
 * differ after an embedded NUL byte (legal for `std::string`, unlike C
 * strings) are silently treated as the same key. This limitation is
 * shared with the legacy `Cnode`/`Prefix_Tree`.
 *
 * @tparam T Mapped value type. Move-only values are supported for insertion
 *         and lookup; copying the map requires T to be copy-constructible.
 *
 * @ingroup Trees
 */
template <typename T>
class Prefix_Tree_Map
{
public:
  /// Key type accepted by the map.
  using Key = std::string;

  /// Mapped value type stored in terminal nodes.
  using Value = T;

private:
  /** @brief Internal trie node storing one character and an optional value. */
  class Node : public Tree_Node<char>
  {
    std::optional<Value> value_;

    /** @brief Shared mutable and const child lookup. */
    template <typename Node_Type>
    [[nodiscard]] static Node_Type *search_child_impl(Node_Type *root,
                                                       const char c) noexcept
    {
      assert(root != nullptr);

      for (auto *child = root->get_left_child(); child != nullptr;
           child = child->get_right_sibling())
        {
          const char key = child->get_key();
          if (key == c)
            return static_cast<Node_Type *>(child);
          if (key > c)
            return nullptr;
        }

      return nullptr;
    }

    /** @brief Shared mutable and const sorted-child lookup. */
    template <typename Node_Type>
    [[nodiscard]] static Node_Type *greater_child_impl(Node_Type *root,
                                                        const char c) noexcept
    {
      assert(root != nullptr);

      for (auto *child = root->get_left_child(); child != nullptr;
           child = child->get_right_sibling())
        if (child->get_key() > c)
          return static_cast<Node_Type *>(child);
      return nullptr;
    }

    /** @brief Shared mutable and const prefix search. */
    template <typename Node_Type>
    [[nodiscard]] static std::tuple<Node_Type *, const char *>
    search_prefix_impl(Node_Type *root, const char *prefix) noexcept
    {
      assert(root != nullptr);
      assert(prefix != nullptr);

      auto *node = root;
      for (const char *ptr = prefix; *ptr != '\0'; ++ptr)
        {
          auto *child = node->search_child(*ptr);
          if (child == nullptr)
            return std::make_tuple(node, ptr);
          node = child;
        }

      return std::make_tuple(node, "");
    }

    /** @brief Own a detached map subtree until it is committed. */
    class Detached_Subtree_Guard
    {
      Node *root_ = nullptr;

    public:
      explicit Detached_Subtree_Guard(Node *root) noexcept : root_(root)
      {
        // empty
      }

      ~Detached_Subtree_Guard()
      {
        if (root_ != nullptr)
          {
            root_->destroy();
            delete root_;
          }
      }

      [[nodiscard]] Node *get() const noexcept
      {
        return root_;
      }

      [[nodiscard]] Node *release() noexcept
      {
        Node *ret = root_;
        root_ = nullptr;
        return ret;
      }
    };

    /** @brief Own standalone insertion nodes until a new path is committed. */
    class Pending_Path_Guard
    {
      Node **nodes_ = nullptr;
      size_t count_ = 0;
      bool owns_nodes_ = true;

    public:
      explicit Pending_Path_Guard(const size_t count)
        : nodes_(new Node *[count]), count_(count)
      {
        for (size_t i = 0; i < count_; ++i)
          nodes_[i] = nullptr;
      }

      ~Pending_Path_Guard()
      {
        if (owns_nodes_)
          for (size_t i = 0; i < count_; ++i)
            delete nodes_[i];
        delete[] nodes_;
      }

      [[nodiscard]] Node *operator [] (const size_t i) const noexcept
      {
        return nodes_[i];
      }

      void set(const size_t i, Node *node) const noexcept
      {
        nodes_[i] = node;
      }

      void release_nodes() noexcept
      {
        owns_nodes_ = false;
      }
    };

  public:
    /** @brief Construct a structural node with no mapped value.
     *
     * @param[in] c Character stored in this node.
     */
    explicit Node(const char c) noexcept
    {
      this->get_key() = c;
    }

    /** @brief Construct a terminal node with a mapped value.
     *
     * @tparam U Value forwarding type.
     * @param[in] c Character stored in this node.
     * @param[in] value Value to store.
     *
     * @throws Whatever Value construction throws.
     */
    template <typename U>
    Node(const char c, U &&value) : value_(std::forward<U>(value))
    {
      this->get_key() = c;
    }

    /** @brief Return the character stored in this node.
     *
     * @return The character value.
     */
    [[nodiscard]] char symbol() const noexcept
    {
      return get_key();
    }

    /** @brief Check whether this node terminates a key.
     *
     * @return true if the node has a mapped value.
     */
    [[nodiscard]] bool has_value() const noexcept
    {
      return value_.has_value();
    }

    /** @brief Return the stored mapped value.
     *
     * @return Const pointer to the value, or nullptr.
     */
    [[nodiscard]] const Value *value() const noexcept
    {
      return value_.has_value() ? &*value_ : nullptr;
    }

    /** @brief Return the stored mapped value.
     *
     * @return Mutable pointer to the value, or nullptr.
     */
    [[nodiscard]] Value *value() noexcept
    {
      return value_.has_value() ? &*value_ : nullptr;
    }

    /** @brief Remove this node's mapped value.
     *
     * @throws Nothing.
     */
    void reset_value() noexcept
    {
      value_.reset();
    }

    /** @brief Emplace this node's mapped value.
     *
     * @tparam U Value forwarding type.
     * @param[in] value Value to store.
     *
     * @throws Whatever Value construction throws.
     */
    template <typename U>
    void emplace_value(U &&value)
    {
      value_.emplace(std::forward<U>(value));
    }

    /** @brief Search for a mutable child with the given character.
     *
     * @param[in] c Character to search for.
     * @return Pointer to the child node, or nullptr.
     */
    [[nodiscard]] Node *search_child(const char c) noexcept
    {
      return search_child_impl(this, c);
    }

    /** @brief Search for a const child with the given character.
     *
     * @param[in] c Character to search for.
     * @return Const pointer to the child node, or nullptr.
     */
    [[nodiscard]] const Node *search_child(const char c) const noexcept
    {
      return search_child_impl(this, c);
    }

    /** @brief Find the first mutable child with a character greater than c.
     *
     * @param[in] c Character threshold.
     * @return Pointer to the first child with char > c, or nullptr.
     */
    [[nodiscard]] Node *greater_child(const char c) noexcept
    {
      return greater_child_impl(this, c);
    }

    /** @brief Find the first const child with a character greater than c.
     *
     * @param[in] c Character threshold.
     * @return Const pointer to the first child with char > c, or nullptr.
     */
    [[nodiscard]] const Node *greater_child(const char c) const noexcept
    {
      return greater_child_impl(this, c);
    }

    /** @brief Search for a prefix in the mutable tree.
     *
     * @param[in] prefix Null-terminated prefix to search for.
     * @return Tuple of (node reached, remaining unmatched suffix).
     *
     * @pre prefix must not be nullptr.
     */
    [[nodiscard]] std::tuple<Node *, const char *>
    search_prefix(const char *prefix) noexcept
    {
      return search_prefix_impl(this, prefix);
    }

    /** @brief Search for a prefix in the const tree.
     *
     * @param[in] prefix Null-terminated prefix to search for.
     * @return Tuple of (node reached, remaining unmatched suffix).
     *
     * @pre prefix must not be nullptr.
     */
    [[nodiscard]] std::tuple<const Node *, const char *>
    search_prefix(const char *prefix) const noexcept
    {
      return search_prefix_impl(this, prefix);
    }

    /** @brief Insert a child node in sorted order.
     *
     * @param[in] child Node to insert.
     * @return Pointer to the inserted child.
     *
     * @pre No child with the same character exists.
     */
    Node *insert_child(Node *child)
    {
      assert(search_child(child->symbol()) == nullptr);

      if (Node *sibling = greater_child(child->symbol()); sibling == nullptr)
        this->insert_rightmost_child(child);
      else
        sibling->insert_left_sibling(child);

      return child;
    }

    /** @brief Insert a key suffix below this node.
     *
     * @tparam U Value forwarding type.
     * @param[in] suffix Non-empty null-terminated suffix to create.
     * @param[in] value Value to place at the terminal node.
     *
     * @throws Whatever node or value allocation/construction throws.
     *
     * @note Provides the strong guarantee: no partial path is linked if
     *       allocation or value construction fails.
     */
    template <typename U>
    void insert_suffix(const char *suffix, U &&value)
    {
      assert(suffix != nullptr);
      assert(*suffix != '\0');

      size_t n = 0;
      for (const char *ptr = suffix; *ptr != '\0'; ++ptr)
        ++n;

      Pending_Path_Guard path(n);
      for (size_t i = 0; i + 1 < n; ++i)
        path.set(i, new Node(suffix[i]));
      path.set(n - 1, new Node(suffix[n - 1], std::forward<U>(value)));
      path.release_nodes();

      Node *parent = insert_child(path[0]);
      for (size_t i = 1; i < n; ++i)
        {
          parent->insert_rightmost_child(path[i]);
          parent = path[i];
        }
    }

    /** @brief Destroy all children of this node.
     *
     * Recursively deletes descendants. The node itself is not deleted.
     */
    void destroy() noexcept
    {
      for (auto *p = static_cast<Node *>(get_right_child()); p != nullptr;)
        {
          Node *to_delete = p;
          p = static_cast<Node *>(p->get_left_sibling());
          destroy_tree(to_delete);
        }
      this->set_is_leaf(true);
    }

    /** @brief Clone this node and all descendants.
     *
     * @return Newly allocated subtree root.
     *
     * @throws Whatever Value copying throws, or std::bad_alloc.
     */
    [[nodiscard]] Node *clone() const
      requires std::is_copy_constructible_v<Value>
    {
      Detached_Subtree_Guard ret(new Node(symbol()));
      clone_into(this, ret.get());
      return ret.release();
    }

    /** @brief Copy src contents into an already allocated target node.
     *
     * @param[in] src Source subtree.
     * @param[out] tgt Target node with the same symbol as src.
     *
     * @throws Whatever Value copying throws, or std::bad_alloc.
     */
    static void clone_into(const Node *src, Node *tgt)
      requires std::is_copy_constructible_v<Value>
    {
      assert(src != nullptr);
      assert(tgt != nullptr);

      if (src->value_.has_value())
        tgt->value_.emplace(*src->value_);

      for (const Tree_Node<char> *src_child = src->get_left_child();
           src_child != nullptr; src_child = src_child->get_right_sibling())
        {
          const auto *child = static_cast<const Node *>(src_child);
          Detached_Subtree_Guard cloned_child(new Node(child->symbol()));
          Node *tgt_child = cloned_child.get();
          tgt->insert_rightmost_child(tgt_child);
          (void) cloned_child.release();
          clone_into(child, tgt_child);
        }
    }

    /** @brief Append all terminal keys in this subtree.
     *
     * @param[in,out] word Current prefix buffer.
     * @param[out] out Destination array.
     * @param[in] max_word_length Maximum expected key length.
     *
     * @throws std::bad_alloc if appending grows the destination.
     */
    void words_impl(std::string &word, DynArray<std::string> &out,
                    const size_t max_word_length) const
    {
      if (has_value())
        out.append(word);

      for (const Tree_Node<char> *child = get_left_child(); child != nullptr;
           child = child->get_right_sibling())
        {
          assert(word.size() < max_word_length);
          const auto *node = static_cast<const Node *>(child);
          word.push_back(node->symbol());
          node->words_impl(word, out, max_word_length);
          word.pop_back();
        }
    }
  };

  Node *root_ = nullptr;
  size_t size_ = 0;

  /** @brief Destroy an owned root and all descendants. */
  static void destroy_root(Node *root) noexcept
  {
    if (root == nullptr)
      return;

    root->destroy();
    delete root;
  }

  /** @brief Ensure the root node exists after a move. */
  void ensure_root()
  {
    if (root_ == nullptr)
      root_ = new Node('\0');
  }

  /** @brief Find the terminal node for word. */
  [[nodiscard]] const Node *find_node(const std::string &word) const noexcept
  {
    if (root_ == nullptr)
      return nullptr;

    auto [node, remaining] = root_->search_prefix(word.c_str());
    return *remaining == '\0' ? node : nullptr;
  }

  /** @brief Find the terminal node for word. */
  [[nodiscard]] Node *find_node(const std::string &word) noexcept
  {
    return const_cast<Node *>(
      static_cast<const Prefix_Tree_Map *>(this)->find_node(word));
  }

  /** @brief Shared implementation for `insert()`'s copy/move overloads and
   *         for `insert_or_assign()`: when `assign_if_present` is `true`
   *         (only `insert_or_assign()` sets it), an already-present key's
   *         value is overwritten in place instead of leaving it untouched
   *         -- letting `insert_or_assign()` share this single tree
   *         descent instead of calling `find()` first and then
   *         re-descending from the root a second time via this function.
   */
  template <typename U>
  bool insert_impl(const std::string &word, U &&value,
                    const bool assign_if_present = false)
  {
    ensure_root();

    auto [node, remaining] = root_->search_prefix(word.c_str());
    if (*remaining == '\0')
      {
        if (node->has_value())
          {
            if (assign_if_present)
              *node->value() = std::forward<U>(value);
            return false;
          }
        node->emplace_value(std::forward<U>(value));
        ++size_;
        return true;
      }

    node->insert_suffix(remaining, std::forward<U>(value));
    ++size_;
    return true;
  }

public:
  /** @brief Construct an empty prefix map.
   *
   * @throws std::bad_alloc If allocating the root node fails.
   */
  Prefix_Tree_Map() : root_(new Node('\0'))
  {
    // empty
  }

  /** @brief Construct a deep copy of another prefix map.
   *
   * @param[in] other Map to copy.
   *
   * @throws Whatever T copying throws, or std::bad_alloc.
   */
  Prefix_Tree_Map(const Prefix_Tree_Map &other)
    requires std::is_copy_constructible_v<Value>
    : root_(other.root_ != nullptr ? other.root_->clone() : nullptr),
      size_(other.size_)
  {
    // empty
  }

  /** @brief Move-construct, taking ownership of another map's root.
   *
   * @param[in,out] other Source map, left empty and usable.
   */
  Prefix_Tree_Map(Prefix_Tree_Map &&other) noexcept
    : root_(other.root_), size_(other.size_)
  {
    other.root_ = nullptr;
    other.size_ = 0;
  }

  /** @brief Destroy the owned root and all descendants.
   *
   * @throws Nothing.
   */
  ~Prefix_Tree_Map() noexcept
  {
    destroy_root(root_);
  }

  /** @brief Replace this map with a deep copy of another map.
   *
   * @param[in] other Map to copy from.
   * @return Reference to this map.
   *
   * @throws Whatever T copying throws, or std::bad_alloc.
   *
   * @note Provides the strong guarantee: if copying fails, this map keeps its
   *       previous contents.
   */
  Prefix_Tree_Map &operator = (const Prefix_Tree_Map &other)
    requires std::is_copy_constructible_v<Value>
  {
    if (this == &other)
      return *this;

    Node *new_root = other.root_ != nullptr ? other.root_->clone() : nullptr;
    destroy_root(root_);
    root_ = new_root;
    size_ = other.size_;
    return *this;
  }

  /** @brief Move-assign, taking ownership of another map's root.
   *
   * @param[in,out] other Source map, left empty and usable.
   * @return Reference to this map.
   */
  Prefix_Tree_Map &operator = (Prefix_Tree_Map &&other) noexcept
  {
    if (this == &other)
      return *this;

    destroy_root(root_);
    root_ = other.root_;
    size_ = other.size_;
    other.root_ = nullptr;
    other.size_ = 0;
    return *this;
  }

  /** @brief Insert key with a copied value if absent.
   *
   * @param[in] key Key to insert. The empty string is valid.
   * @param[in] value Value to copy.
   * @return true if inserted, false if key already existed.
   *
   * @throws Whatever T copying throws, or std::bad_alloc.
   */
  bool insert(const Key &key, const Value &value)
  {
    return insert_impl(key, value);
  }

  /** @brief Insert key with a moved value if absent.
   *
   * @param[in] key Key to insert. The empty string is valid.
   * @param[in,out] value Value to move from if insertion succeeds.
   * @return true if inserted, false if key already existed.
   *
   * @throws Whatever T moving throws, or std::bad_alloc.
   *
   * @warning If key already exists, value is left untouched.
   */
  bool insert(const Key &key, Value &&value)
  {
    return insert_impl(key, std::move(value));
  }

  /** @brief Insert key or overwrite its mapped value.
   *
   * @param[in] key Key to insert or update.
   * @param[in] value New value, moved into place.
   *
   * @throws Whatever T construction or assignment throws, or std::bad_alloc.
   */
  void insert_or_assign(const Key &key, Value value)
  {
    insert_impl(key, std::move(value), /* assign_if_present = */ true);
  }

  /** @brief Remove key if present.
   *
   * @param[in] key Key to remove.
   * @return true if key was present, false otherwise.
   *
   * @throws Nothing.
   *
   * @note This removes the logical key by clearing its mapped value. Structural
   *       path nodes may be retained and reused by later insertions.
   */
  bool erase(const Key &key) noexcept
  {
    Node *node = find_node(key);
    if (node == nullptr or not node->has_value())
      return false;

    node->reset_value();
    --size_;
    return true;
  }

  /** @brief Check whether key is stored.
   *
   * @param[in] key Key to search for.
   * @return true if key exists.
   *
   * @throws Nothing.
   */
  [[nodiscard]] bool contains(const Key &key) const noexcept
  {
    return find(key) != nullptr;
  }

  /** @brief Look up key.
   *
   * @param[in] key Key to search for.
   * @return Pointer to the stored value, or nullptr. The pointer remains valid
   *         until the next non-const operation on this map.
   *
   * @throws Nothing.
   */
  [[nodiscard]] const Value *find(const Key &key) const noexcept
  {
    const Node *node = find_node(key);
    return node != nullptr ? node->value() : nullptr;
  }

  /** @brief Mutable lookup overload.
   *
   * @param[in] key Key to search for.
   * @return Mutable pointer to the stored value, or nullptr. The pointer
   *         remains valid until the next non-const operation on this map.
   *
   * @throws Nothing.
   */
  [[nodiscard]] Value *find(const Key &key) noexcept
  {
    Node *node = find_node(key);
    return node != nullptr ? node->value() : nullptr;
  }

  /** @brief Return the number of stored key-value pairs.
   *
   * @return Current cardinality.
   *
   * @throws Nothing.
   */
  [[nodiscard]] size_t size() const noexcept
  {
    return size_;
  }

  /** @brief Return the number of stored key-value pairs.
   *
   * @return Current cardinality.
   *
   * @throws Nothing.
   *
   * @note Alias for size().
   */
  [[nodiscard]] size_t count() const noexcept
  {
    return size();
  }

  /** @brief Check whether the map has no keys.
   *
   * @return true if size() == 0.
   *
   * @throws Nothing.
   */
  [[nodiscard]] bool is_empty() const noexcept
  {
    return size_ == 0;
  }

  /** @brief Remove every key-value pair from the map.
   *
   * @throws std::bad_alloc If recreating the empty root fails.
   */
  void clear()
  {
    Node *new_root = new Node('\0');
    destroy_root(root_);
    root_ = new_root;
    size_ = 0;
  }

  /** @brief Get all keys stored in the map.
   *
   * @param[in] max_word_length Maximum expected key length.
   * @return Array containing all keys.
   *
   * @throws std::bad_alloc If allocation fails.
   *
   * @pre max_word_length must be large enough for every stored key when
   *      assertions are enabled.
   */
  [[nodiscard]] DynArray<Key> words(const size_t max_word_length = 2048) const
  {
    DynArray<Key> ret;
    if (root_ == nullptr)
      return ret;

    std::string word;
    word.reserve(max_word_length);
    root_->words_impl(word, ret, max_word_length);
    return ret;
  }

  /** @brief Get all keys with a given prefix.
   *
   * @param[in] prefix Prefix to search for.
   * @param[in] max_word_length Maximum expected key length.
   * @return Array containing matching keys.
   *
   * @throws std::bad_alloc If allocation fails.
   *
   * @pre max_word_length must be at least prefix.size() and large enough for
   *      every emitted key when assertions are enabled.
   */
  [[nodiscard]] DynArray<Key> words_with_prefix(const Key &prefix,
                                                const size_t max_word_length = 2048) const
  {
    DynArray<Key> ret;
    if (root_ == nullptr)
      return ret;

    auto [node, remaining] = root_->search_prefix(prefix.c_str());
    if (*remaining != '\0')
      return ret;

    assert(prefix.size() <= max_word_length);
    std::string word(prefix);
    word.reserve(max_word_length);
    node->words_impl(word, ret, max_word_length);
    return ret;
  }
};
}  // end namespace Aleph

#endif  // PREFIX_TREE_H
