/*
  Aleph-w: Data structures & Algorithms
  Copyright (c) 2026 Leandro Rabindranath Leon

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

/** @file Adaptive_Huffman.H
 *  @brief One-pass adaptive Huffman coding of bytes using FGK updates.
 *
 *  Encoder and decoder start with a single NYT (not yet transmitted) leaf.
 *  An unseen byte is represented by the current NYT path followed by its
 *  eight bits, most significant bit first. A known byte uses its current
 *  leaf path. Left edges are 0 and right edges are 1. Both sides update
 *  the tree after each complete byte using the Faller-Gallager-Knuth rule.
 *  The NYT leaf is retained even after all 256 bytes have appeared.
 *
 *  There is no tree header, end marker, padding, or checksum. The caller must
 *  preserve the exact bit count (for example with BitArray::save()/load())
 *  or frame the transmission separately. finish() detects a partial final
 *  codeword; it cannot detect missing complete codewords or arbitrary bit
 *  corruption. This format is independent of the static codec in Huffman.H.
 *
 *  @see Adaptive_Huffman_Encoder
 *  @see Adaptive_Huffman_Decoder
 *  @see https://www.ittc.ku.edu/~jsv/Papers/Vit87.jacmACMversion.pdf
 *       J. S. Vitter, Design and Analysis of Dynamic Huffman Codes (1987),
 *       Section 2 (Algorithm FGK).
 *  @ingroup Algorithms
 */

#ifndef ALEPH_ADAPTIVE_HUFFMAN_H
#define ALEPH_ADAPTIVE_HUFFMAN_H

#include <climits>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include <ah-errors.H>
#include <bitArray.H>

namespace Aleph {
/// @internal Implementation details; not a stable public interface.
namespace adaptive_huffman_detail {
static_assert(CHAR_BIT == 8, "Adaptive Huffman requires eight-bit bytes");

/// @internal Indexed FGK tree shared by the encoder and decoder.
struct Model
{
  static constexpr int alphabet_size = 256;                ///< Number of possible bytes.
  static constexpr int max_nodes = 2 * alphabet_size + 1;  ///< Including NYT.
  static constexpr int root = 0;                           ///< Stable root node index.

  /// @internal A node's identity is stable; its order changes on swaps.
  struct Node
  {
    std::uint64_t weight = 0;  ///< Subtree frequency.
    int parent = -1;           ///< Parent index, or -1 at the root.
    int left = -1;             ///< Left child index, or -1 at a leaf.
    int right = -1;            ///< Right child index, or -1 at a leaf.
    int order = 0;             ///< Position in the sibling-property numbering.
    unsigned char symbol = 0;  ///< Byte value, meaningful only at data leaves.

    /** @brief Test whether this node is a leaf.
     *  @return True if the node has no children.
     */
    [[nodiscard]] bool is_leaf() const noexcept
    {
      return left == -1;
    }
  };

  // The alphabet bounds every table at compile time. Native fixed arrays
  // keep the model allocation-free, including updates, reset and copying.
  Node nodes[max_nodes] = {};      ///< Stable node storage.
  int by_order[max_nodes] = {};    ///< Order-to-node index.
  int leaves[alphabet_size] = {};  ///< Byte-to-leaf index, or -1 if unseen.
  int nyt = root;                  ///< Current NYT node index.
  int used = 1;                    ///< Number of active nodes.
  size_t distinct = 0;             ///< Number of bytes seen so far.

  /// @brief Construct the initial tree containing only NYT.
  Model() noexcept
  {
    reset();
  }

  /// @brief Restore the initial tree in O(alphabet_size) time.
  void reset() noexcept
  {
    for (int i = 0; i < max_nodes; ++i)
      {
        nodes[i] = Node{};
        by_order[i] = -1;
      }
    for (int &leaf : leaves)
      leaf = -1;
    nodes[root].order = max_nodes - 1;
    by_order[max_nodes - 1] = root;
    nyt = root;
    used = 1;
    distinct = 0;
  }

  /** @brief Reject an update that would overflow the frequency counters.
   *  @throws std::overflow_error if UINT64_MAX bytes were already processed.
   */
  void check_capacity() const
  {
    ah_overflow_error_if(nodes[root].weight == std::numeric_limits<std::uint64_t>::max())
      << "Adaptive Huffman frequency limit reached; reset both codecs";
  }

  /** @brief Find the highest-order node of a node's weight block.
   *  @param node Index of the current node.
   *  @return Index of the block leader.
   *  @note O(log alphabet_size). Only the suffix starting at node is
   *        searched: an incremented NYT sibling can temporarily precede
   *        its not-yet-incremented parent, but this suffix stays sorted.
   */
  [[nodiscard]] int block_leader(const int node) const noexcept
  {
    int low = nodes[node].order;
    int high = max_nodes;
    const auto weight = nodes[node].weight;
    while (low + 1 < high)
      if (const int mid = low + (high - low) / 2; nodes[by_order[mid]].weight <= weight)
        low = mid;
      else
        high = mid;
    return by_order[low];
  }

  /** @brief Exchange two non-ancestor subtrees and their order numbers.
   *  @param a First node index (not the root).
   *  @param b Second node index (not the root or an ancestor of a).
   */
  void swap_nodes(const int a, const int b) noexcept
  {
    const int pa = nodes[a].parent;
    const int pb = nodes[b].parent;
    const bool a_left = nodes[pa].left == a;
    const bool b_left = nodes[pb].left == b;
    (a_left ? nodes[pa].left : nodes[pa].right) = b;
    (b_left ? nodes[pb].left : nodes[pb].right) = a;
    nodes[a].parent = pb;
    nodes[b].parent = pa;

    const int order_a = nodes[a].order;
    const int order_b = nodes[b].order;
    nodes[a].order = order_b;
    nodes[b].order = order_a;
    by_order[order_a] = b;
    by_order[order_b] = a;
  }

  /** @brief Insert an unseen byte or increment an existing byte's path.
   *  @param symbol Byte just encoded or decoded.
   *  @pre check_capacity() succeeded before processing this byte.
   *  @note O((h + 1) log alphabet_size), where h is the update path length.
   */
  void update(const unsigned char symbol) noexcept
  {
    int node = leaves[symbol];
    if (node == -1)
      {
        const int old_nyt = nyt;
        const int order = nodes[old_nyt].order;
        nyt = used++;
        node = used++;
        nodes[nyt].parent = old_nyt;
        nodes[nyt].order = order - 2;
        nodes[node].parent = old_nyt;
        nodes[node].order = order - 1;
        nodes[node].weight = 1;
        nodes[node].symbol = symbol;
        by_order[order - 2] = nyt;
        by_order[order - 1] = node;
        nodes[old_nyt].left = nyt;
        nodes[old_nyt].right = node;
        nodes[old_nyt].weight = 1;
        leaves[symbol] = node;
        ++distinct;
        node = nodes[old_nyt].parent;
      }

    while (node != -1)
      {
        const int leader = block_leader(node);
        // The only possible ancestor of equal weight is the NYT parent.
        if (leader != node and leader != nodes[node].parent)
          swap_nodes(node, leader);
        ++nodes[node].weight;
        node = nodes[node].parent;
      }
  }
};
}  // namespace adaptive_huffman_detail

/** @brief Stateful, one-pass FGK Huffman encoder for binary byte strings.
 *
 *  encode() appends to a caller-owned BitArray and retains its tree between
 *  calls. Feeding chunks produces exactly the same bits as feeding their
 *  concatenation. No input storage or frequency prepass is needed.
 *
 *  Each instance owns a fixed, allocation-free model of at most 513 nodes.
 *  Encoding costs O((h + 1) log 256) per byte, apart from output allocation,
 *  where h is the tree height (at most 256). Copies own independent models.
 *  Different instances may be used concurrently; a shared instance requires
 *  external synchronization. Frequencies are not rescaled: after UINT64_MAX
 *  bytes, reset both peers at the same externally framed boundary.
 *
 *  @code
 *  Aleph::Adaptive_Huffman_Encoder encoder;
 *  Aleph::BitArray bits;
 *  encoder.encode("abracadabra", bits);
 *  @endcode
 *  @see Adaptive_Huffman_Decoder
 *  @ingroup Algorithms
 */
class Adaptive_Huffman_Encoder
{
  adaptive_huffman_detail::Model model;  ///< Owned adaptive tree.

public:
  /// @brief Construct an encoder with an empty model.
  Adaptive_Huffman_Encoder() noexcept = default;

  /// @brief Reset the model in O(256) time; caller-owned output is untouched.
  void reset() noexcept
  {
    model.reset();
  }

  /** @brief Return the number of bytes encoded since construction or reset.
   *  @return Total frequency at the root, in O(1).
   */
  [[nodiscard]] std::uint64_t symbols_processed() const noexcept
  {
    return model.nodes[model.root].weight;
  }

  /** @brief Return the number of different byte values seen.
   *  @return A value in [0, 256], in O(1).
   */
  [[nodiscard]] size_t distinct_symbols() const noexcept
  {
    return model.distinct;
  }

  /** @brief Return the current frequency of a byte.
   *  @param symbol Byte to query.
   *  @return Occurrence count, or zero if unseen, in O(1).
   */
  [[nodiscard]] std::uint64_t frequency(const unsigned char symbol) const noexcept
  {
    const int leaf = model.leaves[symbol];
    return leaf == -1 ? 0 : model.nodes[leaf].weight;
  }

  /** @brief Append the current code for one byte, then update the tree.
   *  @param symbol Byte to encode, including NUL and values above 127.
   *  @param[out] output Bit array to append to (its existing prefix is kept).
   *  @return Number of appended bits, between 1 and 263: a known byte takes
   *          at most 256, and an unseen one at most 255 and its eight bits.
   *  @throws std::overflow_error if the frequency or output size would overflow.
   *          The output size can only overflow where size_t has 32 bits: with
   *          64 bits a BitArray cannot hold that many bits. It stays seven
   *          bits below the maximum of size_t, so that BitArray::set_size()
   *          can restore any size that the output had.
   *  @throws std::bad_alloc if BitArray cannot allocate output storage.
   *  @note On an allocation failure the model is unchanged, but output may
   *        contain a partial codeword. Restore its previous size before retrying.
   *  @note O((h + 1) log 256), apart from output allocation, where h is the
   *        tree height: the code is read in O(h) and the update takes the rest.
   */
  size_t encode(const unsigned char symbol, BitArray &output)
  {
    model.check_capacity();
    const bool unseen = model.leaves[symbol] == -1;
    int node = unseen ? model.nyt : model.leaves[symbol];
    unsigned char path[adaptive_huffman_detail::Model::alphabet_size];
    size_t length = 0;
    while (model.nodes[node].parent != -1)
      {
        const int parent = model.nodes[node].parent;
        path[length++] = model.nodes[parent].right == node ? 1 : 0;
        node = parent;
      }
    const size_t count = length + (unseen ? 8 : 0);
    // BitArray::set_size() computes its bytes as (size + 7) / 8.
    ah_overflow_error_if(output.size() > std::numeric_limits<size_t>::max() - 7 - count)
      << "Adaptive Huffman output bit count overflow";
    while (length != 0)
      output.push(path[--length]);
    if (unseen)
      for (int shift = 7; shift >= 0; --shift)
        output.push((symbol >> shift) & 1u);
    model.update(symbol);
    return count;
  }

  /** @brief Encode a chunk, retaining the model for subsequent chunks.
   *  @param input Bytes to encode; embedded NULs are preserved.
   *  @param[out] output Bit array to append to.
   *  @return Number of bits appended by this call (zero for empty input).
   *  @throws std::overflow_error if a frequency or bit count would overflow.
   *  @throws std::bad_alloc if output storage cannot be allocated.
   *  @note Successfully encoded bytes remain committed on an exception;
   *        the failing byte has the exception guarantee of encode(unsigned char,
   *        BitArray &).
   *  @note O(n (h + 1) log 256) for n input bytes, apart from output
   *        allocation, where h is the tree height (at most 256).
   */
  size_t encode(const std::string_view input, BitArray &output)
  {
    const size_t initial_size = output.size();
    for (const unsigned char symbol : input)
      encode(symbol, output);
    return output.size() - initial_size;
  }
};

/** @brief Incremental FGK decoder accepting arbitrarily fragmented bits.
 *
 *  A byte is emitted as soon as its last bit arrives. Partial tree paths
 *  and partial NYT literals survive across calls, so chunks may end at any
 *  bit position. Call finish() at the externally known end of the message
 *  to check that no partial codeword remains.
 *
 *  Memory, update complexity, copying, counter limits and thread-safety are
 *  the same as for Adaptive_Huffman_Encoder. Each bit costs O(1), plus an
 *  O((h + 1) log 256) tree update when a byte is completed. A copy includes
 *  any partially decoded codeword.
 *
 *  @code
 *  Aleph::Adaptive_Huffman_Decoder decoder;
 *  std::string text;
 *  decoder.decode(bits, text);
 *  decoder.finish();
 *  @endcode
 *  @see Adaptive_Huffman_Encoder
 *  @ingroup Algorithms
 */
class Adaptive_Huffman_Decoder
{
  adaptive_huffman_detail::Model model;  ///< Owned adaptive tree.
  int current = model.root;              ///< Node reached by the partial codeword.
  unsigned int literal = 0;              ///< Partial, most-significant-bit-first byte.
  unsigned int literal_bits = 0;         ///< Number of literal bits collected.
  bool failed = false;                   ///< Malformed input or a lost output byte requires reset.

public:
  /// @brief Construct a decoder with an empty model and no pending bits.
  Adaptive_Huffman_Decoder() noexcept = default;

  /// @brief Clear the tree, pending bits and error state in O(256) time.
  void reset() noexcept
  {
    model.reset();
    current = model.root;
    literal = literal_bits = 0;
    failed = false;
  }

  /** @brief Return the number of complete bytes decoded.
   *  @return Total frequency at the root, in O(1).
   */
  [[nodiscard]] std::uint64_t symbols_processed() const noexcept
  {
    return model.nodes[model.root].weight;
  }

  /** @brief Return the number of different decoded byte values.
   *  @return A value in [0, 256], in O(1).
   */
  [[nodiscard]] size_t distinct_symbols() const noexcept
  {
    return model.distinct;
  }

  /** @brief Return the current frequency of a decoded byte.
   *  @param symbol Byte to query.
   *  @return Occurrence count, or zero if unseen, in O(1).
   */
  [[nodiscard]] std::uint64_t frequency(const unsigned char symbol) const noexcept
  {
    const int leaf = model.leaves[symbol];
    return leaf == -1 ? 0 : model.nodes[leaf].weight;
  }

  /** @brief Test whether a tree path or NYT literal is incomplete.
   *  @return True if bits for the next byte have been consumed, in O(1).
   */
  [[nodiscard]] bool has_pending_symbol() const noexcept
  {
    return current != model.root or literal_bits != 0;
  }

  /** @brief Consume one bit and emit a byte if its codeword is complete.
   *  @param bit Next encoded bit, either 0 or 1.
   *  @param[out] symbol Decoded byte; left unchanged when returning false.
   *  @return True exactly when one byte has been decoded.
   *  @throws std::out_of_range if bit is not binary; no state is changed.
   *  @throws std::invalid_argument if NYT introduces a byte already seen,
   *          or is used after all 256 bytes have appeared. Call reset()
   *          on both peers before starting a new message after this error.
   *  @throws std::domain_error if called, without reset(), after malformed
   *          input or after decode() could not append a decoded byte.
   *  @throws std::overflow_error if the frequency limit has been reached.
   *  @note O(1) for a bit that does not complete a byte, and O((h + 1) log 256)
   *        for one that does, which updates the tree, where h is its height.
   */
  bool decode_bit(const unsigned int bit, unsigned char &symbol)
  {
    ah_out_of_range_error_if(bit > 1) << "Adaptive Huffman expects a binary bit";
    ah_domain_error_if(failed)
      << "Reset the adaptive Huffman decoder after malformed input or a lost byte";
    model.check_capacity();

    unsigned char value;
    if (not model.nodes[current].is_leaf())
      {
        current = bit == 0 ? model.nodes[current].left : model.nodes[current].right;
        if (current == model.nyt)
          {
            if (model.distinct == model.alphabet_size)
              {
                failed = true;
                ah_invalid_argument() << "Adaptive Huffman NYT used with a full alphabet";
              }
            return false;
          }
        if (not model.nodes[current].is_leaf())
          return false;
        value = model.nodes[current].symbol;
      }
    else
      {
        literal = (literal << 1) | bit;
        if (++literal_bits < 8)
          return false;
        value = static_cast<unsigned char>(literal);
        if (model.leaves[value] != -1)
          {
            failed = true;
            ah_invalid_argument() << "Adaptive Huffman NYT introduced a known byte";
          }
      }

    model.update(value);
    current = model.root;
    literal = literal_bits = 0;
    symbol = value;
    return true;
  }

  /** @brief Decode a bit chunk and append all complete bytes to a string.
   *  @param input Next encoded bits; no byte alignment is required.
   *  @param[out] output String to append to, preserving embedded NULs.
   *  @return Number of bytes appended; incomplete codewords remain buffered.
   *  @throws std::invalid_argument, std::domain_error or std::overflow_error
   *          under the same conditions as decode_bit(), for a chunk that is
   *          not empty: an empty one consumes nothing and returns zero.
   *  @throws std::bad_alloc or std::length_error if output cannot grow. The
   *          decoder has then counted a byte that is not in output, so it is
   *          left failed: every later call throws std::domain_error until
   *          reset(). Use decode_bit() to control the allocation of the
   *          output, or to go on after it fails.
   *  @note On an exception the bytes decoded before it, in the same chunk,
   *        stay in output, and the return value is lost: compare output.size(),
   *        or symbols_processed(), with their values before the call. Input
   *        and output are caller-owned.
   *  @note O(b + m (h + 1) log 256) for b input bits that complete m bytes,
   *        apart from output allocation: each bit is a step down the tree, and
   *        each completed byte a tree update, where h is the tree height.
   */
  size_t decode(const BitArray &input, std::string &output)
  {
    const size_t initial_size = output.size();
    for (size_t i = 0; i < input.size(); ++i)
      {
        unsigned char symbol;
        if (decode_bit(static_cast<unsigned int>(input[i]), symbol))
          try
            {
              output.push_back(static_cast<char>(symbol));
            }
          catch (...)
            {  // The model counted the byte: going on would lose it unseen.
              failed = true;
              throw;
            }
      }
    return output.size() - initial_size;
  }

  /** @brief Verify that an externally framed message ended at a byte boundary.
   *  @throws std::invalid_argument if a codeword or NYT literal is incomplete.
   *  @throws std::domain_error after malformed input, or after decode() could
   *          not append a decoded byte, until reset().
   *  @note O(1). Does not reset or seal the decoder; after a truncation error
   *        more bits may be supplied. Empty messages are valid. Byte boundary
   *        here means a decoded symbol boundary, not a multiple of eight bits.
   */
  void finish() const
  {
    ah_domain_error_if(failed)
      << "Reset the adaptive Huffman decoder after malformed input or a lost byte";
    ah_invalid_argument_if(has_pending_symbol()) << "Truncated adaptive Huffman codeword";
  }
};
}  // namespace Aleph

#endif  // ALEPH_ADAPTIVE_HUFFMAN_H
