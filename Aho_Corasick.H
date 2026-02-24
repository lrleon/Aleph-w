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


/** @file Aho_Corasick.H
 *  @brief Multi-pattern string matching with the Aho-Corasick automaton.
 *
 *  Supports insertion of multiple byte-string patterns, automaton
 *  construction, and linear-time matching over the input text.
 *
 *  @example aho_corasick_example.cc
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath Leon
 */

# ifndef AHO_CORASICK_H
# define AHO_CORASICK_H

# include <string>
# include <string_view>
# include <utility>

# include <ah-errors.H>
# include <tpl_array.H>

namespace Aleph
{
  /** @brief Aho-Corasick multi-pattern automaton.
   *
   *  The automaton operates on bytes (`unsigned char` alphabet size 256).
   *  All matches (including overlaps) are reported.
   */
  class Aho_Corasick
  {
  public:
    /** @brief One match occurrence in the searched text.
     *
     *  `position` is the 0-based start index of the matched pattern.
     *  `pattern_id` is the insertion order id returned by `add_pattern()`.
     */
    struct Match
    {
      size_t position = 0;
      size_t pattern_id = 0;

      bool operator==(const Match & other) const noexcept
      {
        return position == other.position and pattern_id == other.pattern_id;
      }
    };

  private:
    struct Node
    {
      int next[256];
      int fail = 0;
      Array<size_t> output;

      Node()
      {
        for (size_t i = 0; i < 256; ++i)
          next[i] = -1;
      }
    };

    Array<Node> nodes_;
    Array<std::string> patterns_;
    bool built_ = false;

    size_t create_node()
    {
      nodes_.append(Node{});
      return nodes_.size() - 1;
    }

  public:
    /** @brief Build an empty automaton (root only). */
    Aho_Corasick()
    {
      nodes_.append(Node{});
    }

    /** @brief Clear all patterns and reset to an empty automaton. */
    void clear()
    {
      nodes_.empty();
      patterns_.empty();
      nodes_.append(Node{});
      built_ = false;
    }

    /** @brief Add one pattern.
     *
     *  @param[in] pattern Pattern to insert (must be non-empty).
     *  @return Pattern id to be used in match reporting.
     *
     *  @exception domain_error If `pattern` is empty.
     *
     *  @note Calling this invalidates the previously built automaton;
     *        call `build()` again before `search()`.
     */
    size_t add_pattern(std::string pattern)
    {
      ah_domain_error_if(pattern.empty())
        << "Aho_Corasick::add_pattern(): pattern cannot be empty";

      size_t state = 0;
      for (const unsigned char c: pattern)
        {
          int next_state = nodes_[state].next[c];
          if (next_state == -1)
            {
              const size_t new_state = create_node();
              nodes_[state].next[c] = static_cast<int>(new_state);
              next_state = static_cast<int>(new_state);
            }

          state = static_cast<size_t>(next_state);
        }

      const size_t id = patterns_.size();
      patterns_.append(std::move(pattern));
      nodes_[state].output.append(id);

      built_ = false;
      return id;
    }

    /** @brief Build failure links and transition completion.
     *
     *  Must be called after all `add_pattern()` calls and before `search()`.
     *
     *  @par Complexity
     *  O(sum(pattern lengths) * alphabet) with fixed alphabet 256.
     */
    void build()
    {
      Array<size_t> queue;
      size_t head = 0;

      for (size_t c = 0; c < 256; ++c)
        if (const int next_state = nodes_[0].next[c]; next_state == -1)
          nodes_[0].next[c] = 0;
        else
          {
            nodes_[static_cast<size_t>(next_state)].fail = 0;
            queue.append(static_cast<size_t>(next_state));
          }

      while (head < queue.size())
        {
          const size_t state = queue[head++];

          for (size_t c = 0; c < 256; ++c)
            {
              const int next_state = nodes_[state].next[c];
              if (next_state == -1)
                {
                  nodes_[state].next[c] =
                      nodes_[static_cast<size_t>(nodes_[state].fail)].next[c];
                  continue;
                }

              const auto child = static_cast<size_t>(next_state);
              const auto fail_to = static_cast<size_t>(
                nodes_[static_cast<size_t>(nodes_[state].fail)].next[c]);

              nodes_[child].fail = static_cast<int>(fail_to);

              const Array<size_t> & fail_output = nodes_[fail_to].output;
              for (size_t i = 0; i < fail_output.size(); ++i)
                nodes_[child].output.append(fail_output[i]);

              queue.append(child);
            }
        }

      built_ = true;
    }

    /** @brief Search all patterns in a text.
     *
     *  @param[in] text Text to scan.
     *  @return Matches sorted by end-scan order.
     *
     *  @exception runtime_error If `build()` has not been called.
     *
     *  @par Complexity
     *  O(text.size() + number_of_matches).
     */
    [[nodiscard]] Array<Match> search(const std::string_view text) const
    {
      ah_runtime_error_unless(built_)
        << "Aho_Corasick::search(): call build() before search()";

      Array<Match> matches;
      size_t state = 0;

      for (size_t i = 0; i < text.size(); ++i)
        {
          const unsigned char c = static_cast<unsigned char>(text[i]);
          state = static_cast<size_t>(nodes_[state].next[c]);

          const Array<size_t> & output = nodes_[state].output;
          for (size_t k = 0; k < output.size(); ++k)
            {
              const size_t id = output[k];
              matches.append(Match{i + 1 - patterns_[id].size(), id});
            }
        }

      return matches;
    }

    /** @brief Return true if at least one pattern appears in the text. */
    [[nodiscard]] bool contains_any(const std::string_view text) const
    {
      return not search(text).is_empty();
    }

    /** @brief Return the number of inserted patterns. */
    [[nodiscard]] size_t pattern_count() const noexcept
    {
      return patterns_.size();
    }

    /** @brief Return whether `build()` has been executed since last insertion. */
    [[nodiscard]] bool is_built() const noexcept
    {
      return built_;
    }

    /** @brief Return the pattern text by id. */
    [[nodiscard]] const std::string &pattern(const size_t id) const
    {
      ah_out_of_range_error_if(id >= patterns_.size())
        << "Aho_Corasick::pattern(): id out of range";
      return patterns_[id];
    }
  };
} // namespace Aleph

# endif // AHO_CORASICK_H
