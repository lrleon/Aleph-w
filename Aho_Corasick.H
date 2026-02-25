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
# include <array>

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
      std::array<int, 256> next;
      int fail = 0;
      int out_link = -1;
      Array<size_t> output;

      Node()
      {
        next.fill(-1);
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

    /** @brief Add one pattern to the automaton.
     *
     *  @param[in] pattern Pattern to insert (must be non-empty).
     *  @return Pattern id to be used in match reporting.
     *
     *  @pre pattern is non-empty.
     *  @throws std::invalid_argument If `pattern` is empty.
     *
     *  @note Calling this invalidates the previously built automaton;
     *        call `build()` again before `search()`.
     */
    size_t add_pattern(std::string pattern)
    {
      ah_invalid_argument_if(pattern.empty())
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
     *  Finalizes the automaton structure. Must be called after all `add_pattern()`
     *  calls and before invoking `search()`.
     *
     *  @pre `add_pattern()` has been called for all desired patterns and no
     *       further modifications (no additional `add_pattern()` calls) have
     *       occurred since.
     *
     *  @par Complexity
     *  O(sum(pattern lengths) * alphabet) with fixed alphabet 256.
     */
    void build()
    {
      Array<size_t> queue;
      size_t head = 0;

      nodes_[0].fail = 0;
      nodes_[0].out_link = -1;

      for (size_t c = 0; c < 256; ++c)
        if (const int next_state = nodes_[0].next[c]; next_state != -1)
          {
            const auto child = static_cast<size_t>(next_state);
            nodes_[child].fail = 0;
            nodes_[child].out_link = -1;
            queue.append(child);
          }

      while (head < queue.size())
        {
          const size_t state = queue[head++];

          for (size_t c = 0; c < 256; ++c)
            {
              const int next_state = nodes_[state].next[c];
              if (next_state == -1)
                continue;

              const auto child = static_cast<size_t>(next_state);
              auto fail_to = static_cast<size_t>(nodes_[state].fail);

              while (fail_to != 0 and nodes_[fail_to].next[c] == -1)
                fail_to = static_cast<size_t>(nodes_[fail_to].fail);

              if (const int candidate = nodes_[fail_to].next[c]; candidate != -1)
                fail_to = static_cast<size_t>(candidate);

              nodes_[child].fail = static_cast<int>(fail_to);
              nodes_[child].out_link =
                  nodes_[fail_to].output.is_empty() ?
                  nodes_[fail_to].out_link : static_cast<int>(fail_to);

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
     *  @pre build() has been called since the last modification.
     *  @throws std::runtime_error If `build()` has not been called.
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

          while (state != 0 and nodes_[state].next[c] == -1)
            state = static_cast<size_t>(nodes_[state].fail);

          if (const int next_state = nodes_[state].next[c]; next_state != -1)
            state = static_cast<size_t>(next_state);
          else
            state = 0;

          size_t out_state = state;
          while (true)
            {
              const Array<size_t> & output = nodes_[out_state].output;
              for (size_t k = 0; k < output.size(); ++k)
                {
                  const size_t id = output[k];
                  matches.append(Match{i + 1 - patterns_[id].size(), id});
                }

              const int next_out = nodes_[out_state].out_link;
              if (next_out == -1)
                break;
              out_state = static_cast<size_t>(next_out);
            }
        }

      return matches;
    }

    /** @brief Return true if at least one pattern appears in the text.
     *
     *  @param[in] text Text to scan.
     *  @return true if any match is found, false otherwise.
     *
     *  @pre build() has been called since the last modification.
     *  @throws std::runtime_error If `build()` has not been called.
     */
    [[nodiscard]] bool contains_any(const std::string_view text) const
    {
      ah_runtime_error_unless(built_)
        << "Aho_Corasick::contains_any(): call build() before search()";

      size_t state = 0;
      for (size_t i = 0; i < text.size(); ++i)
        {
          const unsigned char c = static_cast<unsigned char>(text[i]);

          while (state != 0 and nodes_[state].next[c] == -1)
            state = static_cast<size_t>(nodes_[state].fail);

          if (const int next_state = nodes_[state].next[c]; next_state != -1)
            state = static_cast<size_t>(next_state);
          else
            state = 0;

          if (not nodes_[state].output.is_empty())
            return true;

          if (nodes_[state].out_link != -1)
            return true;
        }

      return false;
    }

    /** @brief Return the number of inserted patterns.
     *
     *  @return Number of patterns.
     */
    [[nodiscard]] size_t pattern_count() const noexcept
    {
      return patterns_.size();
    }

    /** @brief Return whether `build()` has been executed since last insertion.
     *
     *  @return true if the automaton is built, false otherwise.
     */
    [[nodiscard]] bool is_built() const noexcept
    {
      return built_;
    }

    /** @brief Return the pattern text by id.
     *
     *  @param[in] id Pattern id.
     *  @return Pattern string.
     *
     *  @pre id is a valid pattern id returned from add_pattern.
     *  @throws std::out_of_range If `id >= pattern_count()`.
     */
    [[nodiscard]] const std::string &pattern(const size_t id) const
    {
      ah_out_of_range_error_if(id >= patterns_.size())
        << "Aho_Corasick::pattern(): id out of range";
      return patterns_[id];
    }
  };
} // namespace Aleph

# endif // AHO_CORASICK_H
