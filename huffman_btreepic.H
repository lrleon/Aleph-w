
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/**
 * @file huffman_btreepic.H
 * @brief Huffman tree visualization for btreepic LaTeX package.
 *
 * This header provides utilities for generating btreepic specifications
 * from Huffman trees. The btreepic package is a LaTeX tool for drawing
 * binary trees with customizable node labels, positions, and annotations.
 *
 * ## Features
 *
 * - Generates btreepic format output for Huffman frequency trees
 * - Supports automatic node positioning with level-based adjustments
 * - Handles special LaTeX characters in symbol labels
 * - Produces publication-quality tree diagrams
 *
 * ## Output Format
 *
 * The generated output consists of:
 * - `start-prefix`: Node positions in prefix order
 * - `start-key`: Frequency values for each node
 * - `TAG` commands: Leaf node labels with symbol names
 * - `xoffset` commands: Horizontal adjustments for overlapping labels
 *
 * ## Usage Example
 *
 * @code{.cpp}
 * #include <Huffman.H>
 * #include <huffman_btreepic.H>
 * #include <fstream>
 *
 * // Build Huffman tree
 * Huffman_Encoder_Engine encoder;
 * encoder.read_input("sample text", true);
 *
 * // Method 1: Using output stream parameter (preferred)
 * std::ofstream out("tree.btreepic");
 * huffman_to_btreepic(encoder.get_freq_root(), out);
 *
 * // Method 2: Using global output_ptr (legacy, deprecated)
 * output_ptr = &out;
 * huffman_to_btreepic(encoder.get_freq_root());
 *
 * // With level adjustment for dense trees
 * huffman_to_btreepic(encoder.get_freq_root(), out, true);
 * @endcode
 *
 * ## Thread Safety
 *
 * @warning This module uses global state for tree traversal and is
 *          NOT thread-safe. Each call to `huffman_to_btreepic()` must
 *          complete before another call begins.
 *
 * @see Huffman.H for the Huffman encoding implementation
 * @see btreepic LaTeX package documentation
 *
 * @ingroup Utilities
 * @author Leandro Rabindranath Leon
 */

#ifndef HUFFMAN_BTREEPIC_H
#define HUFFMAN_BTREEPIC_H

#include <cstdlib>
#include <ostream>
#include <algorithm>
#include <tpl_dynMapTree.H>
#include <Huffman.H>

/**
 * @brief Horizontal offset direction for node label adjustment.
 *
 * Used to track whether a node's label has been shifted left or right
 * to avoid overlapping with adjacent nodes at the same level.
 */
enum Offset
{
  NO_OFFSET,  ///< No offset applied
  LEFT,       ///< Shifted to the left
  RIGHT       ///< Shifted to the right
};

/**
 * @brief Descriptor for infix (in-order) traversal position.
 *
 * Stores the position and level of a node during in-order traversal,
 * along with any horizontal offset that has been applied to avoid
 * label collisions.
 */
struct Infix_Desc
{
  int pos = 0;       ///< In-order position (0-based)
  int level = 0;     ///< Tree level (0 = root)
  Offset offset;     ///< Applied offset direction

  /// Default constructor
  Infix_Desc() : offset(NO_OFFSET) {}

  /**
   * @brief Construct with position and level.
   * @param i In-order position
   * @param l Tree level
   */
  Infix_Desc(int i, int l) : pos(i), level(l), offset(NO_OFFSET) {}
};

/**
 * @brief Descriptor for level-order traversal information.
 *
 * Stores whether a node is a left or right child and its successor
 * in level-order traversal. This information is used to detect
 * potentially overlapping labels that need adjustment.
 */
struct Level_Desc
{
  bool is_left = false;             ///< True if this node is a left child
  Freq_Node * level_succ = nullptr; ///< Next node in level-order traversal

  /// Default constructor
  Level_Desc() = default;

  /**
   * @brief Construct with child direction and optional successor.
   * @param il True if left child
   * @param succ Successor in level-order (optional)
   */
  Level_Desc(bool il, Freq_Node * succ = nullptr)
    : is_left(il), level_succ(succ) {}
};

/// Type alias for the level descriptor map
using Level_Table = DynMapTree<Freq_Node *, Level_Desc, Treap>;

// ============================================================================
// Global state (module-level, not thread-safe)
// ============================================================================

/// Map from node pointer to infix position descriptor
inline DynMapTree<Freq_Node *, Infix_Desc, Treap> infix_table;

/// Map from node pointer to level traversal descriptor
inline Level_Table level_table;

/// Output stream pointer for btreepic commands
inline std::ostream * output_ptr = nullptr;

/// Predecessor node in level-order traversal
inline Freq_Node * pred = nullptr;

/// Minimum gap (in nodes) before applying offset adjustment
inline int minimal_gap = 4;

/// Maximum valid index for offset array
inline constexpr size_t MAX_OFFSET_INDEX = 7;

/// Offset values indexed by symbol string length (bounded by MAX_OFFSET_INDEX)
inline const long double offset[] = { 10, 15, 25, 40, 55, 65, 85, 90 };

/**
 * @brief Get safe offset value with bounds checking.
 * @param index Symbol length
 * @return Offset value, clamped to valid range
 */
[[nodiscard]] inline long double get_offset(size_t index) noexcept
{
  return offset[std::min(index, MAX_OFFSET_INDEX)];
}

/**
 * @brief Reset global state for a new tree processing.
 *
 * Clears all traversal tables and resets the predecessor pointer.
 * Called automatically at the start of huffman_to_btreepic().
 */
inline void reset_huffman_btreepic_state() noexcept
{
  infix_table.empty();
  level_table.empty();
  pred = nullptr;
}

// ============================================================================
// Traversal callback functions
// ============================================================================

/**
 * @brief Save infix position for a node.
 *
 * Callback for in-order traversal that records each node's position
 * and level in the infix_table.
 *
 * @param p Node pointer
 * @param level Tree level
 * @param pos In-order position
 */
inline void save_infix_pos(Freq_Node * p, int level, int pos)
{
  infix_table.insert(p, Infix_Desc(pos, level));
}

/**
 * @brief First pass level-order callback: record child direction.
 *
 * Callback for level-order traversal that records whether each node
 * is a left or right child.
 *
 * @param p Node pointer
 * @param pos Position in level (unused)
 * @param is_left True if node is a left child
 */
inline void save_level_pos_1(Freq_Node * p, int pos, bool is_left)
{
  (void)pos;
  level_table.insert(p, Level_Desc(is_left));
}

/**
 * @brief Second pass level-order callback: link successors.
 *
 * Callback for level-order traversal that links each node to its
 * successor in level-order.
 *
 * @param p Node pointer
 * @param pos Position in level-order traversal
 * @param is_left Unused
 */
inline void save_level_pos_2(Freq_Node * p, int pos, bool is_left)
{
  (void)is_left;

  if (pos == 0)
    {
      assert(pred == nullptr);
      pred = p;
      return;
    }

  assert(pred != nullptr and pred != p);

  Level_Desc & level_desc_pred = level_table.find(pred);
  level_desc_pred.level_succ = p;
  pred = p;
}

/**
 * @brief Write node position in prefix order.
 *
 * Callback for prefix-order traversal that outputs the infix position
 * of each node, building the btreepic structure specification.
 *
 * @param p Node pointer
 * @param level Unused
 * @param prefix_pos Unused
 */
inline void write_prefix(Freq_Node * p, int level, int prefix_pos)
{
  (void)level;
  (void)prefix_pos;

  const Infix_Desc & infix_desc = infix_table[p];
  *output_ptr << static_cast<int>(infix_desc.pos) << " ";
}

/**
 * @brief Write node frequency value.
 *
 * Callback for in-order traversal that outputs the frequency value
 * of each node as a quoted string for btreepic labels.
 *
 * @param p Node pointer
 * @param level Unused
 * @param pos Unused
 */
inline void write_freq(Freq_Node * p, int level, int pos)
{
  (void)level;
  (void)pos;

  const size_t & freq = p->get_key().second;
  *output_ptr << "\"" << freq << "\" ";
}

/**
 * @brief Write leaf node labels with LaTeX escaping.
 *
 * Callback for in-order traversal that outputs TAG commands for leaf
 * nodes (symbols). Handles special LaTeX characters and whitespace
 * with appropriate escaping.
 *
 * @param p Node pointer
 * @param level Unused
 * @param pos In-order position
 */
inline void write_leaves(Freq_Node * p, int level, int pos)
{
  (void)level;

  // Only process leaf nodes
  if (not (LLINK(p) == nullptr and RLINK(p) == nullptr))
    return;

  const std::string & key = p->get_key().first;

  *output_ptr << "TAG " << pos << " \"";

  // Escape special LaTeX characters
  if (key == "\n")
    *output_ptr << "$\\backslash$n";
  else if (key == "")
    *output_ptr << "$\\neg$";
  else if (key == "$")
    *output_ptr << "\\$";
  else if (key == "&")
    *output_ptr << "\\&";
  else if (key == "#")
    *output_ptr << "\\#";
  else if (key == "%")
    *output_ptr << "\\%";
  else if (key == "{")
    *output_ptr << "$\\{$";
  else if (key == "}")
    *output_ptr << "$\\}$";
  else if (key == "^")
    *output_ptr << "\\^";
  else if (key == "_")
    *output_ptr << "\\_";
  else if (key == "\\")
    *output_ptr << "$\\backslash$";
  else if (key == " ")
    *output_ptr << "$\\square$";
  else if (key == "\"")
    *output_ptr << "$\\prime\\prime$";
  else
    *output_ptr << key;

  *output_ptr << "\" S 0 -20 " << std::endl;
}

/**
 * @brief Compute number of decimal digits in an integer.
 * @param n Integer value
 * @return Number of digits
 */
[[nodiscard]] inline size_t num_digits(const int & n)
{
  return std::to_string(n).size();
}

/**
 * @brief Adjust node positions to prevent label overlap.
 *
 * Callback for in-order traversal that checks if adjacent nodes at
 * the same level are too close together. If they have opposite
 * orientations, xoffset commands are generated to shift labels apart.
 *
 * @param p Node pointer
 * @param p_level Tree level of node
 * @param p_infix_pos In-order position of node
 */
inline void adjust_nodes(Freq_Node * p, int p_level, int p_infix_pos)
{
  // Last node has no successor to check
  if (static_cast<size_t>(p_infix_pos) == level_table.size() - 1)
    return;

  Level_Desc & p_level_desc = level_table[p];
  Freq_Node *& p_succ = p_level_desc.level_succ;

  // No successor in level-order?
  if (p_succ == nullptr)
    return;

  Infix_Desc & p_succ_infix_desc = infix_table[p_succ];

  // Not at the same level?
  if (p_level != p_succ_infix_desc.level)
    return;

  const int & p_succ_infix_pos = p_succ_infix_desc.pos;

  // Separation in nodes between the two
  const int diff_pos = p_succ_infix_pos - p_infix_pos;

  // Sufficiently separated?
  if (diff_pos > minimal_gap)
    return;

  Level_Desc & p_suc_level_desc = level_table[p_succ];

  // Same orientation (both left or both right children)?
  if (p_suc_level_desc.is_left == p_level_desc.is_left)
    return;

  Infix_Desc & p_infix_desc = infix_table[p];

  const std::string & kp = p->get_key().first;
  const size_t kp_sz = kp.size();

  // Apply left offset to current node if not already offset
  if (p_infix_desc.offset == NO_OFFSET)
    {
      *output_ptr << "xoffset " << p_infix_pos << " "
                  << -get_offset(kp_sz) << std::endl;
      p_infix_desc.offset = LEFT;
    }

  assert(p_succ_infix_desc.offset == NO_OFFSET);

  const std::string & k_succ = p_succ->get_key().first;
  const size_t k_succ_sz = k_succ.size();

  // Apply right offset to successor
  *output_ptr << "xoffset " << p_succ_infix_pos << " "
              << static_cast<int>(get_offset(k_succ_sz) / 2) << std::endl;

  p_succ_infix_desc.offset = RIGHT;
}

// ============================================================================
// Main API
// ============================================================================

/**
 * @brief Generate btreepic specification for a Huffman tree.
 *
 * Produces a complete btreepic specification that can be compiled with
 * LaTeX to render the Huffman frequency tree. The output includes:
 *
 * 1. Tree structure (`start-prefix` line)
 * 2. Node labels with frequencies (`start-key` line)
 * 3. Leaf symbol annotations (`TAG` commands)
 * 4. Optional position adjustments (`xoffset` commands)
 *
 * @param p Root of the Huffman frequency tree
 * @param with_level_adjust If true, apply automatic position adjustments
 *                          to prevent overlapping labels at dense levels
 *
 * @pre output_ptr must be set to a valid output stream before calling
 * @pre p must be a valid Freq_Node pointer (may be nullptr for empty tree)
 *
 * @note This function uses global state and is NOT thread-safe.
 *
 * @see Huffman_Encoder_Engine::get_freq_root() for obtaining the root
 */
inline void huffman_to_btreepic(Freq_Node * p, const bool with_level_adjust = false)
{
  if (p == nullptr or output_ptr == nullptr)
    return;

  // Reset state for new tree
  reset_huffman_btreepic_state();

  // First pass: record infix positions
  inOrderRec(p, save_infix_pos);

  // Write tree structure in prefix order
  *output_ptr << "start-prefix ";
  preOrderRec(p, write_prefix);
  *output_ptr << std::endl << "start-key ";

  // Write frequency labels
  inOrderRec(p, write_freq);
  *output_ptr << std::endl;

  // Write leaf node tags
  inOrderRec(p, write_leaves);
  *output_ptr << std::endl;

  if (not with_level_adjust)
    return;

  // Level adjustment: two passes for linking successors
  levelOrder(p, save_level_pos_1);
  levelOrder(p, save_level_pos_2);

  // Final pass: adjust overlapping labels
  inOrderRec(p, adjust_nodes);
}

/**
 * @brief Generate btreepic specification to an output stream.
 *
 * Preferred overload that takes an explicit output stream parameter
 * instead of using the global output_ptr.
 *
 * @param p Root of the Huffman frequency tree
 * @param output Output stream for btreepic commands
 * @param with_level_adjust If true, apply automatic position adjustments
 *
 * @see huffman_to_btreepic(Freq_Node*, bool) for details
 */
inline void huffman_to_btreepic(Freq_Node * p,
                                std::ostream & output,
                                bool with_level_adjust = false)
{
  output_ptr = &output;
  huffman_to_btreepic(p, with_level_adjust);
}

#endif // HUFFMAN_BTREEPIC_H
