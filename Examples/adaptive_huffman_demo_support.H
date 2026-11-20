/** @file adaptive_huffman_demo_support.H
 *  @brief Shared, public-API-only traces for the adaptive Huffman examples.
 *
 *  Tree snapshots are reconstructed from codes obtained from independent
 *  copies of the real encoder. No implementation-detail model is accessed.
 */

#ifndef ALEPH_ADAPTIVE_HUFFMAN_DEMO_SUPPORT_H
#define ALEPH_ADAPTIVE_HUFFMAN_DEMO_SUPPORT_H

#include <Adaptive_Huffman.H>
#include <ah-errors.H>
#include <tpl_array.H>

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>

namespace adaptive_huffman_demo
{
  /** @brief Return an unambiguous, terminal-safe label for a byte.
   *  @param byte Byte to describe.
   *  @return Printable ASCII, a control name, or a hexadecimal byte value.
   */
  inline std::string byte_label(const unsigned char byte)
  {
    switch (byte)
      {
      case 0: return "NUL";
      case '\n': return "LF";
      case '\r': return "CR";
      case '\t': return "TAB";
      case ' ': return "SPACE";
      default:
        if (byte >= 33 and byte <= 126)
          return std::string(1, static_cast<char>(byte));
        constexpr char hex[] = "0123456789ABCDEF";
        std::string label = "0x";
        label += hex[byte >> 4];
        label += hex[byte & 15];
        return label;
      }
  }

  /** @brief Probe a byte's code without changing the source encoder.
   *  @param encoder State immediately before the proposed byte.
   *  @param byte Byte to probe.
   *  @return Actual transmitted bits, including a literal for an unseen byte.
   */
  inline std::string next_code(const Aleph::Adaptive_Huffman_Encoder &encoder,
                               const unsigned char byte)
  {
    auto copy = encoder;
    Aleph::BitArray bits;
    copy.encode(byte, bits);
    return bits.get_bit_str();
  }

  /// @brief One node of a reconstructed display tree.
  struct Display_Node
  {
    int children[2] = {-1, -1}; ///< Child indices; -1 denotes absence.
    int symbol = -2; ///< Byte value, -1 for NYT, -2 for an internal node.
    std::uint64_t weight = 0; ///< Sum of the descendant symbol frequencies.
    std::string code; ///< Root-to-node path.
    double x = 0; ///< Horizontal drawing coordinate.
    double y = 0; ///< Vertical drawing coordinate.
  };

  /// @brief A complete tree for one externally observable encoder state.
  struct Snapshot
  {
    Aleph::Array<Display_Node> nodes; ///< Root at index zero.
    size_t leaf_count = 0; ///< Includes the retained NYT leaf.
    size_t depth = 0; ///< Maximum leaf depth.
  };

  /** @brief Insert a code into the display trie and accumulate its frequency.
   *  @param tree Destination display tree.
   *  @param code Actual root-to-leaf code.
   *  @param symbol Byte value or -1 for NYT.
   *  @param weight Current symbol frequency.
   */
  inline void insert_code(Snapshot &tree, const std::string &code,
                          const int symbol, const std::uint64_t weight)
  {
    int node = 0;
    tree.nodes[node].weight += weight;
    for (const char digit : code)
      {
        const int bit = digit - '0';
        if (tree.nodes[node].children[bit] == -1)
          {
            const int child = static_cast<int>(tree.nodes.size());
            Display_Node added;
            added.code = tree.nodes[node].code + digit;
            tree.nodes.append(added);
            // append() may reallocate: reacquire the parent by index.
            tree.nodes[node].children[bit] = child;
          }
        node = tree.nodes[node].children[bit];
        tree.nodes[node].weight += weight;
      }
    tree.nodes[node].symbol = symbol;
  }

  /** @brief Assign inorder leaf positions and center parents over children.
   *  @param tree Tree to lay out.
   *  @param node Current node index.
   *  @param depth Current depth below the root.
   *  @param next_leaf Counter of already positioned leaves.
   */
  inline void position_nodes(Snapshot &tree, const int node, const size_t depth,
                              size_t &next_leaf)
  {
    auto &entry = tree.nodes[node];
    entry.y = 48 + 86 * depth;
    if (depth > tree.depth)
      tree.depth = depth;
    if (entry.symbol != -2)
      entry.x = 60 + 108 * next_leaf++;
    else
      {
        position_nodes(tree, entry.children[0], depth + 1, next_leaf);
        position_nodes(tree, entry.children[1], depth + 1, next_leaf);
        entry.x = (tree.nodes[entry.children[0]].x +
                   tree.nodes[entry.children[1]].x) / 2;
      }
  }

  /** @brief Reconstruct and validate the tree using public encoder operations.
   *  @param encoder Source encoder; it is never modified.
   *  @return Tree with exact codes, frequencies and drawing coordinates.
   *  @throws std::logic_error if the public codes do not form a full tree.
   */
  inline Snapshot snapshot(const Aleph::Adaptive_Huffman_Encoder &encoder)
  {
    Snapshot result;
    result.nodes.append(Display_Node{});
    int unseen = -1;
    for (int byte = 0; byte < 256; ++byte)
      {
        const auto value = static_cast<unsigned char>(byte);
        const auto frequency = encoder.frequency(value);
        if (frequency != 0)
          insert_code(result, next_code(encoder, value), byte, frequency);
        else if (unseen == -1)
          unseen = byte;
      }
    if (unseen != -1)
      {
        auto code = next_code(encoder, static_cast<unsigned char>(unseen));
        code.resize(code.size() - 8); // Remove the literal; retain the NYT path.
        insert_code(result, code, -1, 0);
      }
    else
      {
        // With all bytes present, NYT is the single unused branch of the trie.
        const size_t count = result.nodes.size();
        bool found = false;
        for (size_t i = 0; i < count and not found; ++i)
          if (result.nodes[i].symbol == -2)
            for (int bit = 0; bit < 2; ++bit)
              if (result.nodes[i].children[bit] == -1)
                {
                  const auto code = result.nodes[i].code + static_cast<char>('0' + bit);
                  insert_code(result, code, -1, 0);
                  found = true;
                  break;
                }
        ah_logic_error_unless(found) << "Display tree is missing its NYT branch";
      }

    for (const auto &node : result.nodes)
      if (node.symbol == -2)
        {
          ah_logic_error_if(node.children[0] == -1 or node.children[1] == -1)
            << "Display tree contains an incomplete internal node";
          ah_logic_error_if(node.weight != result.nodes[node.children[0]].weight +
                                           result.nodes[node.children[1]].weight)
            << "Display tree contains inconsistent frequencies";
        }
    ah_logic_error_if(result.nodes[0].weight != encoder.symbols_processed())
      << "Display tree does not represent the encoder's current state";
    position_nodes(result, 0, 0, result.leaf_count);
    return result;
  }

  /// @brief One verified encoding event, plus the tree after the event.
  struct Frame
  {
    unsigned char byte = 0; ///< Byte consumed by this event.
    bool first_appearance = false; ///< Whether the byte was previously unseen.
    std::string path; ///< Tree path before the update.
    std::string literal; ///< Eight raw bits on a first appearance.
    size_t total_bits = 0; ///< Cumulative encoded length.
    Snapshot tree; ///< Encoder state after the update.
  };

  /** @brief Record an actual online round trip, starting with the empty model.
   *  @param input Bytes to illustrate.
   *  @return Initial-state frame followed by one frame for every byte.
   *  @throws std::logic_error if the decoder disagrees at any symbol boundary.
   */
  inline Aleph::Array<Frame> trace(const std::string_view input)
  {
    Aleph::Array<Frame> frames;
    Aleph::Adaptive_Huffman_Encoder encoder;
    Aleph::Adaptive_Huffman_Decoder decoder;
    Frame initial;
    initial.tree = snapshot(encoder);
    frames.append(initial);
    size_t total = 0;
    for (const unsigned char byte : input)
      {
        Frame frame;
        frame.byte = byte;
        frame.first_appearance = encoder.frequency(byte) == 0;
        Aleph::BitArray bits;
        total += encoder.encode(byte, bits);
        frame.path = bits.get_bit_str();
        if (frame.first_appearance)
          {
            frame.literal = frame.path.substr(frame.path.size() - 8);
            frame.path.resize(frame.path.size() - 8);
          }
        unsigned char decoded = 0;
        for (size_t i = 0; i < bits.size(); ++i)
          {
            const bool ready = decoder.decode_bit(bits[i], decoded);
            ah_logic_error_if(ready != (i + 1 == bits.size()))
              << "A byte was emitted at the wrong bit boundary";
          }
        decoder.finish();
        ah_logic_error_if(decoded != byte) << "Online Huffman round trip failed";
        frame.total_bits = total;
        frame.tree = snapshot(encoder);
        frames.append(frame);
      }
    return frames;
  }

  /** @brief Print a byte-labelled, edge-labelled tree using portable ASCII.
   *  @param out Destination stream.
   *  @param tree Snapshot to render.
   *  @param node Current node index.
   *  @param indent Ancestor branch prefix.
   *  @param edge Empty at the root, otherwise the incoming edge decoration.
   */
  inline void print_tree(std::ostream &out, const Snapshot &tree, const int node = 0,
                          const std::string &indent = "  ",
                          const std::string &edge = "")
  {
    const auto &entry = tree.nodes[node];
    out << indent << edge;
    if (entry.symbol == -2)
      out << '[' << entry.weight << ']';
    else
      out << (entry.symbol == -1 ? "NYT" : byte_label(static_cast<unsigned char>(entry.symbol)))
          << " (" << entry.weight << ")";
    out << '\n';
    if (entry.symbol == -2)
      {
        const std::string next_indent = indent + (edge.empty() ? "" :
                                                   edge[0] == '+' ? "|    " : "     ");
        print_tree(out, tree, entry.children[0], next_indent, "+-0- ");
        print_tree(out, tree, entry.children[1], next_indent, "`-1- ");
      }
  }
}

#endif // ALEPH_ADAPTIVE_HUFFMAN_DEMO_SUPPORT_H
