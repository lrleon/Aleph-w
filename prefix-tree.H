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

/** @file prefix-tree.H
 *  @brief Trie (prefix tree) implementation.
 *
 *  Efficient string prefix search structure for autocomplete,
 *  dictionary lookup, and prefix matching.
 *
 *  ## Features
 *  - O(k) lookup for k-length strings
 *  - Prefix search support
 *  - Memory-efficient for shared prefixes
 *
 *  @ingroup Arboles
 *  @author Leandro Rabindranath León
 */

#ifndef PREFIX_TREE_H
#define PREFIX_TREE_H

# include <tpl_tree_node.H>
# include <tpl_dynArray.H>
# include <tpl_dynList.H>
# include <tpl_arrayStack.H>
# include <ah-errors.H>

namespace Aleph {

/** @brief Prefix tree (Trie) node for storing character sequences.
 *
 * Cnode implements a prefix tree (also known as a trie or digital tree)
 * where each node represents a character. Words are stored as paths
 * from the root, with a special '$' character marking word endings.
 *
 * ## Features
 *
 * - **Insert**: O(m) where m is word length
 * - **Search**: O(m) where m is word length
 * - **Prefix search**: O(m) where m is prefix length
 * - **Space**: O(ALPHABET_SIZE * N * M) worst case
 *
 * ## Usage Example
 *
 * ```cpp
 * // Create root node (typically with a sentinel character)
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
 * // Clean up
 * root.destroy();
 * ```
 *
 * ## Implementation Details
 *
 * - Children are stored in sorted order by character for efficient lookup
 * - Word endings are marked with a '$' sentinel child node
 * - The tree uses a leftmost-child, right-sibling representation
 *
 * @see Tree_Node
 * @ingroup Arboles
 */
class Cnode : public Tree_Node<char>
{
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
  [[nodiscard]] char symbol() const noexcept { return get_key(); }

  /** @brief Return a list of all child nodes.
   *
   * @return DynList containing pointers to all children.
   *
   * @note This creates a new list on each call. For iteration,
   *       consider using for_each_child() directly.
   */
  [[nodiscard]] DynList<Cnode*> children() const
  {
    DynList<Cnode*> r;
    for_each_child([&r](Tree_Node<char> * p) { 
      r.append(static_cast<Cnode*>(p)); 
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
    children().for_each([&ret](Cnode * p) { 
      ret += "(" + p->to_str() + ")"; 
    });
    return ret;
  }

  /** @brief Check if this node marks the end of a word.
   *
   * A word ending is marked by having a '$' child node.
   *
   * @return true if this node ends a word, false otherwise.
   */
  [[nodiscard]] bool is_end_word() const noexcept
  {
    Tree_Node<char> * child = get_left_child();
    if (child == nullptr)
      return false;
    return child->get_key() == '$';
  }

  /** @brief Mark this node as the end of a word.
   *
   * Inserts a '$' sentinel child node.
   *
   * @pre The node must not already be marked as a word ending.
   */
  void mark_end_word()
  {
    assert(not is_end_word());
    insert_leftmost_child(new Cnode('$'));
  }

  /** @brief Search for a child with the given character.
   *
   * @param[in] c Character to search for.
   * @return Pointer to the child node, or nullptr if not found.
   */
  [[nodiscard]] Cnode * search_child(const char c) const noexcept
  {
    for (Tree_Node<char> * child = get_left_child(); child != nullptr;
         child = child->get_right_sibling())
      if (child->get_key() == c)
        return static_cast<Cnode*>(child);

    return nullptr;
  }

  /** @brief Find the first child with a character greater than c.
   *
   * Used for maintaining sorted order of children.
   *
   * @param[in] c Character threshold.
   * @return Pointer to the first child with char > c, or nullptr.
   */
  [[nodiscard]] Cnode * greater_child(const char c) const noexcept
  {
    for (Tree_Node<char> * child = get_left_child(); child != nullptr;
         child = child->get_right_sibling())
      if (child->get_key() > c)
        return static_cast<Cnode*>(child);
    return nullptr;
  }

  /** @brief Search for a prefix in the tree.
   *
   * Traverses the tree following the characters in prefix.
   *
   * @param[in] prefix Null-terminated string to search for.
   * @return Tuple of (node reached, remaining unmatched suffix).
   *         If the entire prefix was found, remaining is "".
   */
  [[nodiscard]] std::tuple<const Cnode*, const char*> 
  search_prefix(const char * prefix) const noexcept
  {
    assert(prefix != nullptr);

    if (*prefix == '\0')
      return std::make_tuple(this, "");

    Cnode * p = search_child(*prefix);
    if (p == nullptr)
      return std::make_tuple(this, prefix);

    return p->search_prefix(prefix + 1);
  }

  /** @brief Search for a complete word in the tree.
   *
   * @param[in] word Null-terminated string to search for.
   * @return Pointer to the ending node if found, nullptr otherwise.
   */
  [[nodiscard]] const Cnode * search_word(const char * word) const noexcept
  {
    if (*word == '\0')
      return is_end_word() ? this : nullptr;

    Cnode * ptr = search_child(*word);
    if (ptr == nullptr)
      return nullptr;

    return ptr->search_word(word + 1);
  }

  /** @brief Check if a word exists in the tree.
   *
   * @param[in] word Word to search for.
   * @return true if the word exists, false otherwise.
   */
  [[nodiscard]] bool contains(const std::string & word) const noexcept
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
    for_each_child([&n](const Tree_Node<char> * p) {
      n += static_cast<const Cnode*>(p)->count();
    });
    return n;
  }

  /** @brief Get all words starting with a given prefix.
   *
   * @param[in] prefix Prefix to search for.
   * @param[in] max_word_length Maximum expected word length (default 2048).
   * @return Array of words that start with prefix.
   */
  [[nodiscard]] DynArray<std::string> words_with_prefix(
      const std::string & prefix,
      const size_t max_word_length = 2048) const
  {
    auto [node, remaining] = search_prefix(prefix.c_str());
    
    // Prefix not fully matched
    if (*remaining != '\0')
      return {};
    
    // Get all words from the prefix node and prepend the prefix
    FixedStack<char> stack(max_word_length);
    DynArray<std::string> ret_val;
    
    // Push prefix (except last char which will be pushed by words_impl)
    for (size_t i = 0; i + 1 < prefix.size(); ++i)
      stack.push(prefix[i]);
    
    node->words_impl(stack, ret_val);
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
  Cnode * insert_child(Cnode * child)
  {
    assert(search_child(child->symbol()) == nullptr);

    if (Cnode * sibling = greater_child(child->symbol()); sibling == nullptr)
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
   */
  bool insert_word(const std::string & word)
  {
    auto [pp, rem] = search_prefix(word.c_str());

    if (*rem == '\0')
      {
        if (pp->is_end_word())
          return false;
        const_cast<Cnode*>(pp)->mark_end_word();
        return true;
      }

    // Insert nodes directly without intermediate list
    auto * parent = const_cast<Cnode*>(pp);
    for (const char *ptr = rem; *ptr; ++ptr)
      parent = parent->insert_child(new Cnode(*ptr));

    parent->mark_end_word();
    return true;
  }

  /** @brief Destroy all children of this node.
   *
   * Recursively deletes all descendant nodes.
   * The node itself is not deleted.
   */
  void destroy() noexcept
  {
    // Must copy children list first - can't iterate while deleting
    children().for_each([](Cnode * p) { destroy_tree(p); });
  }

private:

  static std::string extract_word_from_stack(FixedStack<char> & stack)
  {
    std::string ret_val;
    ret_val.reserve(stack.size());

    const char * ptr = &stack.base();
    const char * last = &stack.top();
    while (ptr <= last)
      ret_val.append(1, *ptr++);

    return ret_val;
  }

  void words_impl(FixedStack<char> & stack, DynArray<std::string> & l) const
  {
    if (symbol() == '$')
      l.append(extract_word_from_stack(stack));

    stack.push(symbol());

    for (const Tree_Node<char> * child = get_left_child(); child != nullptr;
         child = child->get_right_sibling())
      static_cast<const Cnode*>(child)->words_impl(stack, l);

    stack.pop();
  }

public:

  /** @brief Get all words stored in this subtree.
   *
   * @param[in] max_word_length Maximum expected word length (default 2048).
   * @return Array containing all words.
   *
   * @note Uses a temporary stack for traversal.
   */
  [[nodiscard]] DynArray<std::string> words(size_t max_word_length = 2048) const
  {
    FixedStack<char> stack(max_word_length);
    DynArray<std::string> ret_val;

    for_each_child([&](Tree_Node<char> * p) { 
      static_cast<const Cnode*>(p)->words_impl(stack, ret_val); 
    });

    return ret_val;
  }

  /** @brief Print all words to stdout.
   *
   * @param[in] max_word_length Maximum expected word length (default 2048).
   */
  void print_words(const size_t max_word_length = 2048) const
  {
    words(max_word_length).for_each([](const std::string & w) { 
      std::cout << w << '\n'; 
    });
  }

  /** @brief Clone helper - copies children from src to tgt.
   *
   * @param[in] src Source node to copy from.
   * @param[out] tgt Target node to copy to.
   */
  static void clone(const Tree_Node<char> * src, Tree_Node<char> * tgt)
  {
    // First, create all children in target
    src->for_each_child([tgt](auto s) {
      tgt->insert_rightmost_child(new Cnode(s->get_key()));
    });
    
    // Then recursively clone each child subtree
    const Tree_Node<char> * src_child = src->get_left_child();
    Tree_Node<char> * tgt_child = tgt->get_left_child();
    while (src_child != nullptr)
      {
        clone(src_child, tgt_child);
        src_child = src_child->get_right_sibling();
        tgt_child = tgt_child->get_right_sibling();
      }
  }

  /** @brief Create a deep copy of this subtree.
   *
   * @return Pointer to the cloned tree root.
   *
   * @throw std::bad_alloc If memory allocation fails.
   */
  [[nodiscard]] Cnode * clone() const
  {
    auto * ret = new Cnode(symbol());
    clone(this, ret);
    return ret;
  }
};

} // end namespace Aleph

#endif // PREFIX_TREE_H
