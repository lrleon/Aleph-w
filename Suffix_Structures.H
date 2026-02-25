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


/** @file Suffix_Structures.H
 *  @brief Suffix structures: suffix array/LCP, suffix tree, suffix automaton.
 *
 *  Includes:
 *  - Suffix array via doubling O(n log n)
 *  - LCP array via Kasai O(n)
 *  - Didactic naive compressed suffix tree O(n^2)
 *  - Suffix automaton O(n)
 *
 *  @example suffix_array_lcp_example.cc
 *  @example suffix_tree_example.cc
 *  @example suffix_automaton_example.cc
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath Leon
 */

# ifndef SUFFIX_STRUCTURES_H
# define SUFFIX_STRUCTURES_H

# include <algorithm>
# include <limits>
# include <string>
# include <string_view>
# include <utility>

# include <ah-errors.H>
# include <tpl_array.H>
# include <tpl_sort_utils.H>

namespace Aleph
{
  namespace suffix_structures_detail
  {
    /** @brief Generate an array of all positions from 0 to n.
     *  @param[in] n The maximum position.
     *  @return Array of positions.
     */
    inline Array<size_t> all_positions(const size_t n)
    {
      Array<size_t> out;
      out.reserve(n + 1);
      for (size_t i = 0; i <= n; ++i)
        out.append(i);
      return out;
    }


    /** @brief Sort matches in place using introsort.
     *  @param[in,out] a Array of matches to sort.
     */
    inline void sort_matches(Array<size_t> & a)
    {
      introsort(a);
    }
  } // namespace suffix_structures_detail


  /** @brief Build suffix array with doubling algorithm.
   *
   *  Uses two-pass counting sort per doubling phase, achieving
   *  O(n) per phase and O(n log n) total.
   *
   *  @param[in] text Input text.
   *  @return Array `sa` where `sa[i]` is the start index of the i-th suffix
   *          in lexicographic order.
   *
   *  @par Complexity
   *  O(n log n) time, O(n) space.
   */
  inline Array<size_t> suffix_array(const std::string_view text)
  {
    const size_t n = text.size();
    if (n == 0)
      return {};

    Array<size_t> sa = Array<size_t>::create(n);
    Array<int> rank = Array<int>::create(n);
    Array<int> tmp_rank = Array<int>::create(n);
    Array<size_t> tmp_sa = Array<size_t>::create(n);

    for (size_t i = 0; i < n; ++i)
      {
        sa[i] = i;
        rank[i] = static_cast<unsigned char>(text[i]);
      }

    int max_rank = 255; // initial ranks are char codes in [0, 255]

    for (size_t k = 1; ; k *= 2)
      {
        auto key2 = [&](const size_t idx) -> int
          {
            return (idx + k < n) ? rank[idx + k] : -1;
          };

        auto key1 = [&](const size_t idx) -> int
          {
            return rank[idx];
          };

        counting_sort_indices(sa, tmp_sa, n, -1, max_rank, key2);
        counting_sort_indices(sa, tmp_sa, n, 0, max_rank, key1);

        auto less_idx = [&](const size_t a, const size_t b)
          {
            if (rank[a] != rank[b])
              return rank[a] < rank[b];
            const int ra = (a + k < n) ? rank[a + k] : -1;
            const int rb = (b + k < n) ? rank[b + k] : -1;
            return ra < rb;
          };

        tmp_rank[sa[0]] = 0;
        for (size_t i = 1; i < n; ++i)
          tmp_rank[sa[i]] =
              tmp_rank[sa[i - 1]] + (less_idx(sa[i - 1], sa[i]) ? 1 : 0);

        for (size_t i = 0; i < n; ++i)
          rank[i] = tmp_rank[i];

        max_rank = rank[sa[n - 1]];

        if (max_rank == static_cast<int>(n - 1))
          break;

        if (k >= n)
          break;
      }

    return sa;
  }


  /** @brief Compute LCP array from text and suffix array using Kasai.
   *
   *  Returns an array `lcp` of size `n` where:
   *  - `lcp[i] = LCP(sa[i], sa[i+1])` for `i < n-1`
   *  - `lcp[n-1] = 0`
   *
   *  @param[in] text Input text.
   *  @param[in] sa Suffix array for `text`.
   *  @return LCP array.
   *  @throws domain_error If `sa.size() != text.size()`.
   *  @throws out_of_range If some `sa[i] >= text.size()`.
   *
   *  @par Complexity
   *  O(n) time, O(n) space.
   */
  inline Array<size_t> lcp_array_kasai(const std::string_view text,
                                       const Array<size_t> & sa)
  {
    const size_t n = text.size();

    ah_domain_error_if(sa.size() != n)
      << "lcp_array_kasai(): suffix array size must match text size";

    Array<size_t> lcp;
    if (n == 0)
      return lcp;

    lcp = Array<size_t>::create(n);
    for (size_t i = 0; i < n; ++i)
      lcp[i] = 0;

    Array<size_t> rank = Array<size_t>::create(n);
    for (size_t i = 0; i < n; ++i)
      {
        ah_out_of_range_error_if(sa[i] >= n)
          << "lcp_array_kasai(): suffix array contains invalid index";
        rank[sa[i]] = i;
      }

    size_t k = 0;
    for (size_t i = 0; i < n; ++i)
      {
        const size_t r = rank[i];
        if (r + 1 >= n)
          {
            k = 0;
            continue;
          }

        const size_t j = sa[r + 1];
        while (i + k < n and j + k < n and text[i + k] == text[j + k])
          ++k;

        lcp[r] = k;

        if (k > 0)
          --k;
      }

    return lcp;
  }


  /** @brief Naive compressed suffix tree (didactic implementation).
   *
   *  This is intentionally simple and educational (O(n^2) construction), not
   *  Ukkonen's linear-time algorithm.
   *
   *  @note Construction is O(n^2) in time and space.  This class is meant
   *        for teaching and small inputs; for production use prefer the
   *        suffix array + LCP approach or an external linear-time library.
   *  @note A unique terminal byte is appended internally.  If every byte
   *        value (1--255) appears in the input, construction will throw
   *        `domain_error`.
   */
  class Naive_Suffix_Tree
  {
  public:
    static constexpr size_t npos = std::numeric_limits<size_t>::max();

    /** @brief One tree node.
     *
     *  Edge label from parent to this node is `text[start:end)`.
     */
    struct Node
    {
      size_t start = 0; /**< Start index in text */
      size_t end = 0; /**< End index in text */
      size_t parent = npos; /**< Parent node index */
      Array<size_t> children; /**< Indices of children nodes */
      size_t suffix_index = npos; /**< Index of suffix starting at leaf */
    };

  private:
    std::string text_;
    size_t original_size_ = 0;
    Array<Node> nodes_;

    static char choose_terminal(const std::string_view s)
    {
      bool used[256] = {false};
      for (const unsigned char c: s)
        used[c] = true;

      for (size_t i = 0; i < 256; ++i)
        if (not used[i])
          return static_cast<char>(i);

      ah_domain_error()
          << "Naive_Suffix_Tree::choose_terminal(): all 256 byte values "
          << "are present in the input; no terminal marker available";

      return '\0'; // unreachable
    }

    size_t create_node(const size_t start,
                       const size_t end,
                       const size_t parent,
                       const size_t suffix_index)
    {
      Node n;
      n.start = start;
      n.end = end;
      n.parent = parent;
      n.suffix_index = suffix_index;
      nodes_.append(n);
      return nodes_.size() - 1;
    }

    size_t find_child_by_first_char(const size_t node,
                                    const char c) const
    {
      const Array<size_t> & children = nodes_[node].children;
      for (size_t i = 0; i < children.size(); ++i)
        if (const size_t child = children[i]; text_[nodes_[child].start] == c)
          return child;

      return npos;
    }

    void replace_child(const size_t parent,
                       const size_t old_child,
                       const size_t new_child)
    {
      for (Array<size_t> & children = nodes_[parent].children; unsigned long & i : children)
        if (i == old_child)
          {
            i = new_child;
            return;
          }
    }

    void insert_suffix(const size_t suffix_start)
    {
      size_t current = 0;
      size_t pos = suffix_start;

      while (pos < text_.size())
        {
          const size_t child = find_child_by_first_char(current, text_[pos]);
          if (child == npos)
            {
              const size_t leaf = create_node(pos, text_.size(), current, suffix_start);
              nodes_[current].children.append(leaf);
              return;
            }

          const size_t edge_start = nodes_[child].start;
          const size_t edge_end = nodes_[child].end;

          size_t k = 0;
          while (pos + k < text_.size()
                 and edge_start + k < edge_end
                 and text_[pos + k] == text_[edge_start + k])
            ++k;

          if (edge_start + k == edge_end)
            {
              current = child;
              pos += k;
              continue;
            }

          const size_t split = create_node(edge_start,
                                           edge_start + k,
                                           current,
                                           npos);

          replace_child(current, child, split);

          nodes_[child].start = edge_start + k;
          nodes_[child].parent = split;
          nodes_[split].children.append(child);

          const size_t leaf = create_node(pos + k,
                                          text_.size(),
                                          split,
                                          suffix_start);
          nodes_[split].children.append(leaf);

          return;
        }
    }

    void collect_leaf_suffixes(const size_t node,
                               Array<size_t> & out) const
    {
      Array<size_t> stack;
      stack.append(node);

      while (not stack.is_empty())
        {
          const size_t u = stack.remove_last();
          if (nodes_[u].suffix_index != npos)
            {
              if (nodes_[u].suffix_index < original_size_)
                out.append(nodes_[u].suffix_index);
              continue;
            }

          for (const Array<size_t> & children = nodes_[u].children; size_t i: children)
            stack.append(i);
        }
    }

  public:
    /** @brief Construct a naive suffix tree.
     *  @param[in] text The text to build the tree from. If empty, the tree
     *                  will only contain the terminal character.
     *  @return None.
     *  @throws None.
     *  @par Complexity
     *  O(n^2) where n is text.size().
     */
    explicit Naive_Suffix_Tree(const std::string_view text = "")
    {
      build(text);
    }

    /** @brief Build or rebuild the suffix tree for `text`.
     *  @param[in] text The text to build the tree from.
     *  @return None.
     *  @throws None.
     *  @par Complexity
     *  O(n^2) time and O(n^2) space in the worst case, where n is text.size().
     */
    void build(const std::string_view text)
    {
      original_size_ = text.size();
      text_.assign(text.begin(), text.end());
      text_.push_back(choose_terminal(text));

      nodes_.empty();
      nodes_.append(Node{}); // root

      for (size_t i = 0; i < text_.size(); ++i)
        insert_suffix(i);
    }

    /** @brief Return true if the pattern appears in the text.
     *  @param[in] pattern The substring pattern to search for.
     *  @return True if `pattern` is found in the text, false otherwise.
     *          If `pattern` is empty, returns true.
     *  @throws None.
     *  @par Complexity
     *  O(m) where m is pattern.size().
     */
    [[nodiscard]] bool contains(const std::string_view pattern) const
    {
      if (pattern.empty())
        return true;

      size_t current = 0;
      size_t pos = 0;

      while (pos < pattern.size())
        {
          const size_t child = find_child_by_first_char(current, pattern[pos]);
          if (child == npos)
            return false;

          const size_t edge_start = nodes_[child].start;
          const size_t edge_end = nodes_[child].end;

          size_t k = 0;
          while (pos + k < pattern.size()
                 and edge_start + k < edge_end
                 and text_[edge_start + k] != text_.back() // Never match sentinel
                 and pattern[pos + k] == text_[edge_start + k])
            ++k;

          if (pos + k == pattern.size())
            return true;

          if (edge_start + k == edge_end)
            {
              current = child;
              pos += k;
              continue;
            }

          return false;
        }

      return true;
    }

    /** @brief Return all occurrences of a pattern.
     *  @param[in] pattern The substring pattern to search for.
     *  @return An array containing the starting positions of all occurrences
     *          of `pattern` in the text, sorted in ascending order.
     *          Returns an empty array if no occurrences are found.
     *  @note If `pattern` is empty, all positions `[0, text.size()]` are
     *        returned.
     *  @throws None.
     *  @par Complexity
     *  O(m + k log k) where m is pattern.size() and k is the number of
     *  occurrences.
     */
    [[nodiscard]] Array<size_t> find_all(const std::string_view pattern) const
    {
      if (pattern.empty())
        return suffix_structures_detail::all_positions(original_size_);

      size_t current = 0;
      size_t pos = 0;

      while (pos < pattern.size())
        {
          const size_t child = find_child_by_first_char(current, pattern[pos]);
          if (child == npos)
            return {};

          const size_t edge_start = nodes_[child].start;
          const size_t edge_end = nodes_[child].end;

          size_t k = 0;
          while (pos + k < pattern.size()
                 and edge_start + k < edge_end
                 and text_[edge_start + k] != text_.back() // Never match sentinel
                 and pattern[pos + k] == text_[edge_start + k])
            ++k;

          if (pos + k == pattern.size())
            {
              Array<size_t> matches;
              collect_leaf_suffixes(child, matches);
              suffix_structures_detail::sort_matches(matches);
              return matches;
            }

          if (edge_start + k == edge_end)
            {
              current = child;
              pos += k;
              continue;
            }

          return {};
        }

      return {};
    }

    /** @brief Return the number of nodes in the tree.
     *  @return Total number of nodes (internal and leaves).
     *  @throws None.
     *  @par Complexity
     *  O(1).
     */
    [[nodiscard]] size_t node_count() const noexcept
    {
      return nodes_.size();
    }

    /** @brief Return the text size (without terminal marker).
     *  @return The size of the original text used to build the tree.
     *          This corresponds to the `original_size_` member.
     *  @throws None.
     *  @par Complexity
     *  O(1).
     */
    [[nodiscard]] size_t text_size() const noexcept
    {
      return original_size_;
    }

    /** @brief Return the internal node array for inspection/debugging.
     *  @return A constant reference to the underlying Array of nodes.
     *  @throws None.
     *  @par Complexity
     *  O(1).
     */
    [[nodiscard]] const Array<Node> &nodes() const noexcept
    {
      return nodes_;
    }
  };


  /** @brief Suffix automaton (SAM) over byte alphabet.
   *
   *  Supports substring checks, distinct substring counting, and longest
   *  common substring against another string.
   */
  class Suffix_Automaton
  {
  public:
    /** @brief One automaton state. */
    struct State
    {
      int next[256]; /**< Transitions for each byte */
      int link = -1; /**< Suffix link */
      size_t len = 0; /**< Max length of substring in this state */
      size_t first_pos = 0; /**< First occurrence end position */
      bool terminal = false; /**< true if state is terminal */

      State()
      {
        for (int & i: next)
          i = -1;
      }
    };

  private:
    Array<State> states_;
    int last_ = 0;

    /** @brief Traverse suffix links from last and mark all states as terminal.
     *  @throws None.
     */
    void mark_terminals()
    {
      int p = last_;
      while (p != -1)
        {
          states_[static_cast<size_t>(p)].terminal = true;
          p = states_[static_cast<size_t>(p)].link;
        }
    }

  public:
    /** @brief Construct an empty suffix automaton (SAM) with only the root state.
     *  @return None.
     *  @throws None.
     *  @par Complexity
     *  O(1).
     */
    Suffix_Automaton()
    {
      clear();
    }

    /** @brief Reset the automaton to its initial empty state (root only).
     *  @return None.
     *  @throws None.
     *  @par Complexity
     *  O(1).
     */
    void clear()
    {
      states_.empty();
      states_.append(State{});
      states_[0].link = -1;
      states_[0].len = 0;
      states_[0].first_pos = 0;
      states_[0].terminal = false;
      last_ = 0;
    }

    /** @brief Extend the SAM with a single character.
     *  @param[in] ch The character to append to the current text representation.
     *  @return None.
     *  @throws None.
     *  @par Complexity
     *  Amortized O(1).
     */
    void extend(const char ch)
    {
      const auto c = static_cast<unsigned char>(ch);

      const int cur = static_cast<int>(states_.size());
      states_.append(State{});
      states_[static_cast<size_t>(cur)].len = states_[static_cast<size_t>(last_)].len + 1;
      states_[static_cast<size_t>(cur)].first_pos =
          states_[static_cast<size_t>(cur)].len - 1;

      int p = last_;
      while (p != -1 and states_[static_cast<size_t>(p)].next[c] == -1)
        {
          states_[static_cast<size_t>(p)].next[c] = cur;
          p = states_[static_cast<size_t>(p)].link;
        }

      if (p == -1)
        states_[static_cast<size_t>(cur)].link = 0;
      else
        {
          const int q = states_[static_cast<size_t>(p)].next[c];
          if (states_[static_cast<size_t>(p)].len + 1 == states_[static_cast<size_t>(q)].len)
            states_[static_cast<size_t>(cur)].link = q;
          else
            {
              const int clone = static_cast<int>(states_.size());
              // Copy q's state before append: append may reallocate states_,
              // invalidating any reference into the old buffer.
              const State clone_state = states_[static_cast<size_t>(q)];
              states_.append(clone_state);
              states_[static_cast<size_t>(clone)].len =
                  states_[static_cast<size_t>(p)].len + 1;
              states_[static_cast<size_t>(clone)].terminal = false;
              // Enforce invariant: only root (state 0) may have link == -1
              if (states_[static_cast<size_t>(clone)].link == -1)
                states_[static_cast<size_t>(clone)].link = 0;

              while (p != -1 and states_[static_cast<size_t>(p)].next[c] == q)
                {
                  states_[static_cast<size_t>(p)].next[c] = clone;
                  p = states_[static_cast<size_t>(p)].link;
                }

              states_[static_cast<size_t>(q)].link = clone;
              states_[static_cast<size_t>(cur)].link = clone;
            }
        }

      last_ = cur;
    }

    /** @brief Build the SAM from an entire string.
     *  @param[in] text The input string to process.
     *  @return None.
     *  @throws None.
     *  @par Complexity
     *  O(n) where n is text.size().
     */
    void build(const std::string_view text)
    {
      clear();
      for (const char c: text)
        extend(c);

      mark_terminals();
    }

    /** @brief Return true if `pattern` is a substring of the built text.
     *  @param[in] pattern The substring pattern to search for.
     *  @return True if the SAM contains `pattern`, false otherwise.
     *  @throws None.
     *  @par Complexity
     *  O(m) where m is pattern.size().
     */
    [[nodiscard]] bool contains(const std::string_view pattern) const
    {
      int state = 0;
      for (const unsigned char c: pattern)
        {
          state = states_[static_cast<size_t>(state)].next[c];
          if (state == -1)
            return false;
        }

      return true;
    }

    /** @brief Count the number of distinct substrings in the built text.
     *  @return The total count of unique substrings.
     *  @throws None.
     *  @par Complexity
     *  O(V) where V is the number of states.
     */
    [[nodiscard]] size_t distinct_substring_count() const
    {
      size_t total = 0;
      for (size_t i = 1; i < states_.size(); ++i)
        {
          const State & s = states_[i];
          const State & parent = states_[static_cast<size_t>(s.link)];
          total += s.len - parent.len;
        }

      return total;
    }

    /** @brief Find the longest common substring between the built SAM text and `other`.
     *  @param[in] other String to compare against the built SAM text.
     *  @return The longest common substring. Returns an empty string if no
     *          common substring exists.
     *  @throws None.
     *  @par Complexity
     *  O(m) where m is other.size().
     */
    [[nodiscard]] std::string longest_common_substring(const std::string_view other) const
    {
      int state = 0;
      size_t len = 0;

      size_t best_len = 0;
      size_t best_pos = 0;

      for (size_t i = 0; i < other.size(); ++i)
        {
          const auto c = static_cast<unsigned char>(other[i]);

          while (state != 0 and states_[static_cast<size_t>(state)].next[c] == -1)
            {
              state = states_[static_cast<size_t>(state)].link;
              len = states_[static_cast<size_t>(state)].len;
            }

          if (states_[static_cast<size_t>(state)].next[c] != -1)
            {
              state = states_[static_cast<size_t>(state)].next[c];
              ++len;
            }
          else
            {
              state = 0;
              len = 0;
            }

          if (len > best_len)
            {
              best_len = len;
              best_pos = i;
            }
        }

      if (best_len == 0)
        return "";

      return std::string(other.substr(best_pos + 1 - best_len, best_len));
    }

    /** @brief Return the number of states in the SAM.
     *  @return The size of the internal states array.
     *  @throws None.
     *  @par Complexity
     *  O(1).
     */
    [[nodiscard]] size_t state_count() const noexcept
    {
      return states_.size();
    }

    /** @brief Access the internal states array for testing/inspection.
     *  @return A constant reference to the underlying Array of states.
     *  @throws None.
     *  @par Complexity
     *  O(1).
     */
    [[nodiscard]] const Array<State> &states() const noexcept
    {
      return states_;
    }
  };


  /** @brief Convenience function: LCS via suffix automaton.
   *
   *  @param[in] a First string.
   *  @param[in] b Second string.
   *  @return Longest common substring between `a` and `b`.
   */
  inline std::string longest_common_substring_sam(const std::string_view a,
                                                  const std::string_view b)
  {
    Suffix_Automaton sam;
    sam.build(a);
    return sam.longest_common_substring(b);
  }
} // namespace Aleph

# endif // SUFFIX_STRUCTURES_H
