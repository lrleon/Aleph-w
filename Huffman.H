
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

/** @file Huffman.H
 *  @brief Huffman coding for data compression.
 *
 *  Implements Huffman coding algorithm for building optimal prefix-free
 *  codes based on symbol frequencies. Provides encoding and decoding
 *  using binary trees.
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */

#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <memory>
#include <istream>
#include <utility>
#include <tpl_binNodeUtils.H>
#include <tpl_treap.H>
#include <tpl_binHeap.H>
#include <tpl_dynMapTree.H>
#include <bitArray.H>
#include <ah-errors.H>

namespace Aleph {

struct Huffman_Node;

using Symbol_Map = DynMapTree<std::string, Huffman_Node *, Treap_Vtl>;

using Freq_Node = BinNode<std::pair<std::string, size_t>>;

struct Huffman_Node : public BinHeap<size_t>::Node
{
  BinNode<std::string> *bin_node;

  Freq_Node *freq_node;

  /// The node of the symbol that entered the encoder before this one, if
  /// this is the node of a symbol: the encoder walks them without its map.
  Huffman_Node *next_symbol = nullptr;

public:
  Huffman_Node() : BinHeap<size_t>::Node(0), bin_node(nullptr), freq_node(nullptr)
  {
    /* empty */
  }

  Huffman_Node(BinNode<std::string> *node)
    : BinHeap<size_t>::Node(0), bin_node(node), freq_node(nullptr)
  {
    /* empty */
  }

  ~Huffman_Node()
  { /* bin_node memory must not be released here */
  }
};

typedef BinHeap<size_t> Huffman_Heap;

static inline const size_t &get_freq(Huffman_Node *huffman_node) noexcept
{
  return huffman_node->get_key();
}

static inline void increase_freq(Huffman_Node *huffman_node) noexcept
{
  huffman_node->get_key()++;
}

static inline void set_freq(Huffman_Node *huffman_node, const size_t &freq) noexcept
{
  huffman_node->get_key() = freq;
}

typedef DynMapTree<std::string, BitArray, Treap_Vtl> Code_Map;
static inline bool is_leaf(BinNode<std::string> *p) noexcept
{
  return LLINK(p) == nullptr and RLINK(p) == nullptr;
}

/** Huffman encoder.

    @see Huffman_Decoder_Engine
    @ingroup Trees
 */
class Huffman_Encoder_Engine
{
  BinNode<std::string> *root_;
  Huffman_Heap heap_;
  Symbol_Map symbol_map_;
  Code_Map code_map_;

  // The nodes of the symbols, from the last one that entered, linked by
  // next_symbol. Undoing a generation and ending one walk them here: a
  // walk of the map may need memory, for a map that is very deep
  Huffman_Node *symbols_ = nullptr;

  Freq_Node *freq_root_;

  std::string end_symbol_;
  size_t text_len_;

  static void build_prefix_encoding(BinNode<std::string> *p, BitArray &array, Code_Map &codes)
  {
    if (is_leaf(p))
      {
        const std::string &str = p->get_key();
        codes.insert(str, BitArray(array));
        return;
      }
    array.push(0);
    build_prefix_encoding(LLINK(p), array, codes);
    array.pop();
    array.push(1);
    build_prefix_encoding(RLINK(p), array, codes);
    array.pop();
  }

  void build_encoding_map()
  {
    ah_domain_error_if(root_ == nullptr) << "Huffman encoding tree has not been generated";

    BitArray array(0);
    code_map_.empty();
    symbol_map_.empty();
    build_prefix_encoding(root_, array, code_map_);
  }

  // Frees the nodes of a tree that are not leaves: those that a generation
  // made. Its leaves are of the nodes of the symbols
  template <class Node>
  static void free_inner_nodes(Node *p) noexcept
  {
    if (p == nullptr or (LLINK(p) == nullptr and RLINK(p) == nullptr))
      return;
    free_inner_nodes(LLINK(p));
    free_inner_nodes(RLINK(p));
    delete p;
  }

  // Leaves the encoder as it was before a generation of the tree that did
  // not end: the heap with the nodes of the symbols alone, and nothing of
  // what the generation made. It allocates nothing
  void undo_generation() noexcept
  {
    while (not heap_.is_empty())
      if (auto *node = static_cast<Huffman_Node *>(heap_.getMin_ne());
          not is_leaf(node->bin_node))
        {
          free_inner_nodes(node->bin_node);
          free_inner_nodes(node->freq_node);
          delete node;
        }
    for (Huffman_Node *leaf = symbols_; leaf != nullptr; leaf = leaf->next_symbol)
      {
        delete leaf->freq_node;
        leaf->freq_node = nullptr;
        heap_.insert(leaf);  // noexcept: the keys are size_t
      }
  }

  // Gives a node its node of frequencies, if it has none: that of a
  // symbol, with its frequency
  static void make_freq_node(Huffman_Node *node)
  {
    if (node->freq_node != nullptr)
      return;
    std::unique_ptr<Freq_Node> freq_node(new Freq_Node);
    freq_node->get_key().first = node->bin_node->get_key();
    freq_node->get_key().second = node->get_key();
    node->freq_node = freq_node.release();
  }

  // A step of the algorithm of Huffman: the two nodes of least frequency
  // leave the heap, and a new one enters with their trees below it. Without
  // memory for it, the heap has the nodes that it had
  void merge_two_nodes(const bool with_freqs)
  {
    // The two nodes of the step are made before the heap changes
    std::unique_ptr<BinNode<std::string>> bin_node(new BinNode<std::string>);
    std::unique_ptr<Huffman_Node> huffman_node(new Huffman_Node(bin_node.get()));
    auto *l_huffman_node = static_cast<Huffman_Node *>(heap_.getMin());  // left
    auto *r_huffman_node = static_cast<Huffman_Node *>(heap_.getMin());  // right
    const size_t new_freq = get_freq(l_huffman_node) + get_freq(r_huffman_node);
    Aleph::set_freq(huffman_node.get(), new_freq);

    if (with_freqs)
      try
        {
          make_freq_node(l_huffman_node);
          make_freq_node(r_huffman_node);
          std::unique_ptr<Freq_Node> freq_node(new Freq_Node);
          freq_node->get_key().first = std::to_string(new_freq);
          freq_node->get_key().second = new_freq;
          LLINK(freq_node.get()) = l_huffman_node->freq_node;
          RLINK(freq_node.get()) = r_huffman_node->freq_node;
          huffman_node->freq_node = freq_node.release();
        }
      catch (...)
        {
          heap_.insert(l_huffman_node);  // noexcept: the keys are size_t
          heap_.insert(r_huffman_node);
          throw;
        }

    // Nothing throws from here on. The nodes of the symbols stay until the
    // tree is whole: the encoder goes back to them if the generation fails
    LLINK(bin_node.get()) = l_huffman_node->bin_node;
    RLINK(bin_node.get()) = r_huffman_node->bin_node;
    if (not is_leaf(l_huffman_node->bin_node))
      delete l_huffman_node;
    if (not is_leaf(r_huffman_node->bin_node))
      delete r_huffman_node;
    bin_node.release();
    heap_.insert(huffman_node.release());
  }

  bool test_end(const std::string &str) const
  {
    if (end_symbol_ == "NO-END")
      return false;
    return end_symbol_ == str;
  }

  // Makes the leaf of str and its node, of frequency freq, and enters the
  // node in the map of the symbols and then in the heap. Only the map can
  // throw: then neither node is anywhere, and both are freed here
  Huffman_Node * insert_symbol_node(const std::string &str, const size_t freq)
  {
    auto *bin_node = new BinNode<std::string>(str);
    Huffman_Node *huffman_node = nullptr;
    try
      {
        huffman_node = new Huffman_Node(bin_node);
        symbol_map_.insert(str, huffman_node);
      }
    catch (...)
      {
        delete huffman_node;
        delete bin_node;
        throw;
      }
    Aleph::set_freq(huffman_node, freq);
    heap_.insert(huffman_node);  // noexcept: the keys are size_t
    huffman_node->next_symbol = symbols_;
    symbols_ = huffman_node;
    return huffman_node;
  }

  void update_freq(const std::string &str)
  {
    ah_domain_error_if(root_ != nullptr) << "Huffman encoding tree has already been generated";

    ah_domain_error_if(test_end(str)) << "End symbol has already been inserted";

    Huffman_Node *huffman_node = nullptr;
    if (auto huffman_node_ptr = symbol_map_.search(str);
        huffman_node_ptr == nullptr)  // symbol defined previously?
      huffman_node = insert_symbol_node(str, 0);  // No ==> a new entry
    else
      huffman_node = huffman_node_ptr->second;  // already defined, retrieve it

    increase_freq(huffman_node);
    heap_.update(huffman_node);
  }

  static void append_code(BitArray &bit_stream, const BitArray &symbol_code)
  {
    const size_t symbol_code_size = symbol_code.size();
    for (size_t i = 0; i < symbol_code_size; ++i)
      bit_stream.push(symbol_code[i]);
  }

public:
  /// Encoder constructor.
  Huffman_Encoder_Engine() : root_(nullptr), freq_root_(nullptr), end_symbol_("NO-END"), text_len_(0)
  {
    // empty
  }

  /// Destructor - frees all allocated memory
  ~Huffman_Encoder_Engine()
  {
    clear_build_state();
  }

private:
  struct Get_Key
  {
    std::string operator () (BinNode<std::string> *p) const noexcept
    {
      return is_leaf(p) ? p->get_key() : "";
    }
  };

  struct Load_Key
  {
    void operator () (BinNode<std::string> *p, std::istream &input) const noexcept
    {
      if (is_leaf(p))
        input >> p->get_key();
    }
  };

public:
  /** Save a Huffman tree into a stream.

      The serialization format is:
      - a prefix bit sequence describing the tree topology
      - the end-of-stream symbol (length + bytes)
      - the leaf keys in preorder (length + bytes)

      @param[out] output An output stream where the tree will be written.
      @throw std::domain_error if the tree has not been generated.
      @see load_tree()
   */
  void save_tree(std::ostream &output) const
  {
    ah_domain_error_if(root_ == nullptr) << "Huffman tree has not been generated";

    BitArray prefix;
    tree_to_bits(root_, prefix);
    prefix.save(output);

    save_string_as_bytes(end_symbol_, output);
    output << '\n';

    save_leaf_keys_in_prefix(root_, output);
  }

  /** Generate C/C++ array declarations for a Huffman tree.

      save_tree_in_array_of_chars(array_name, output) writes two array
      declarations that can be used to reconstruct the binary tree:

      - `const unsigned char array_name_cdp[n] = { ... };`
      - `const char * array_name_k[] = { ... };`

      The first array stores the topology as a prefix bit code (a
      Lukasiewicz word). The second array stores node contents in
      preorder. The `Get_Key` functor is used to stringify the node
      contents; internal nodes typically return an empty std::string.

      @param[in] array_name Prefix name for the generated arrays.
      @param[out] output Output stream where declarations are written.
      @throw std::domain_error if the tree has not been generated.
      @see Aleph::load_tree_from_array()
   */
  void save_tree_in_array_of_chars(const std::string &array_name, std::ostream &output) const
  {
    ah_domain_error_if(root_ == nullptr) << "Huffman tree has not been generated";

    Aleph::save_tree_in_array_of_chars<BinNode<std::string>, Get_Key>(root_, array_name, output);
  }

  /// Returns the root of the Huffman decoding tree.
  BinNode<std::string> *&get_root()
  {
    ah_domain_error_if(root_ == nullptr) << "Huffman tree has not been generated";

    return root_;
  }

  /** Generate the Huffman prefix tree.

      generate_huffman_tree(with_freqs) runs Huffman's algorithm and
      builds the prefix tree from the collected symbol frequencies. If
      with_freqs is true, an additional tree containing frequencies is
      built as well.

      @param[in] with_freqs If true, also build the frequency tree.
      @return The root of the Huffman decoding (prefix) tree. The
      destructor of the encoder does not free it: free it with
      `destroyRec(encoder.get_root())`, which also clears the root that the
      encoder keeps. load_tree() frees the tree that the encoder still has.
      With `with_freqs`, the same holds for get_freq_root().
      @throw std::bad_alloc if there is not enough memory. The encoder is
      then as it was, with its symbols and their frequencies.
      @see build_prefix_encoding Huffman_Decoder_Engine
   */
  BinNode<std::string> *generate_huffman_tree(const bool &with_freqs = false)
  {
    ah_domain_error_if(root_ != nullptr) << "Huffman encoding tree has already been generated";
    ah_domain_error_if(heap_.is_empty()) << "No symbols have been inserted";

    freq_root_ = nullptr;

    Code_Map codes;
    Huffman_Node *huffman_root = nullptr;
    try
      {
        while (heap_.size() > 1)  // until only one node remains
          merge_two_nodes(with_freqs);

        // The remaining node has the prefix tree. It stays in the heap, which
        // is who frees it, while the map of the codes is built
        huffman_root = static_cast<Huffman_Node *>(heap_.top());
        if (with_freqs)
          make_freq_node(huffman_root);  // it has none if it is a single symbol
        BitArray array(0);
        build_prefix_encoding(huffman_root->bin_node, array, codes);
      }
    catch (...)
      {
        undo_generation();
        throw;
      }

    // Nothing throws from here on
    heap_.getMin_ne();
    root_ = huffman_root->bin_node;
    if (with_freqs)
      freq_root_ = huffman_root->freq_node;
    if (not is_leaf(root_))
      delete huffman_root;  // the node of a symbol goes with the others
    while (symbols_ != nullptr)
      delete std::exchange(symbols_, symbols_->next_symbol);
    symbol_map_.empty();
    code_map_.swap(codes);

    return root_;
  }

  /** Load and build a binary tree from a stream.

       load_tree(input) reads a Huffman tree previously saved with
       save_tree() and restores it into memory.

       @param[in] input Input stream containing the serialized tree.
       @throw std::bad_alloc if there is not enough memory.
       @throw std::domain_error if the stream is malformed.
       @see save_tree()
   */
  void load_tree(std::istream &input)
  {
    clear_build_state();

    destroyRec(root_);
    destroyRec(freq_root_);

    BitArray prefix;
    prefix.load(input);
    root_ = bits_to_tree<BinNode<std::string>>(prefix);

    end_symbol_ = load_string_from_bytes(input);
    load_leaf_keys_in_prefix(root_, input);

    build_encoding_map();
  }

  /// Returns the root of the frequency tree.
  Freq_Node *&get_freq_root()
  {
    ah_domain_error_if(freq_root_ == nullptr) << "Huffman tree has not been generated";

    return freq_root_;
  }

  /** Define the frequency of a symbol.

      set_freq(str, freq) tells the encoder that symbol `str` has
      frequency `freq`.

      @param[in] str Symbol.
      @param[in] freq Symbol frequency.
      @throw std::bad_alloc if there is not enough memory.
   */
  void set_freq(const std::string &str, const size_t &freq)
  {
    ah_domain_error_if(root_ != nullptr) << "Huffman encoding tree has already been generated";
    ah_domain_error_if(test_end(str)) << "End symbol has already been inserted";

    // Search symbol str
    auto huffman_node_ptr = symbol_map_.search(str);
    if (huffman_node_ptr != nullptr)  // already defined?
      {
        std::string msg = "Frequency for symbol " + str + " has already set";
        ah_domain_error() << msg;  // Yes ==> this is an error!
      }

    insert_symbol_node(str, freq);
  }

private:
  static const size_t Max_Token_Size = 256;

  static void save_string_as_bytes(const std::string &str, std::ostream &output)
  {
    output << str.size() << " ";
    for (unsigned char c : str)
      output << static_cast<unsigned int>(c) << " ";
  }

  static std::string load_string_from_bytes(std::istream &input)
  {
    size_t len = 0;
    input >> len;
    ah_domain_error_if(not input) << "Malformed Huffman tree stream";
    ah_domain_error_if(len > Max_Token_Size) << "Symbol too large in Huffman tree stream";

    std::string str;
    str.resize(len);
    for (size_t i = 0; i < len; ++i)
      {
        unsigned int byte = 0;
        input >> byte;
        ah_domain_error_if(not input) << "Malformed Huffman tree stream";
        ah_domain_error_if(byte > 255u) << "Invalid byte value in Huffman tree stream";
        str[i] = static_cast<char>(static_cast<unsigned char>(byte));
      }
    return str;
  }

  static void save_leaf_keys_in_prefix(BinNode<std::string> *p, std::ostream &output)
  {
    if (p == BinNode<std::string>::NullPtr)
      return;

    if (is_leaf(p))
      {
        save_string_as_bytes(p->get_key(), output);
        output << '\n';
        return;
      }

    save_leaf_keys_in_prefix(LLINK(p), output);
    save_leaf_keys_in_prefix(RLINK(p), output);
  }

  static void load_leaf_keys_in_prefix(BinNode<std::string> *p, std::istream &input)
  {
    if (p == BinNode<std::string>::NullPtr)
      return;

    if (is_leaf(p))
      {
        p->get_key() = load_string_from_bytes(input);
        return;
      }

    load_leaf_keys_in_prefix(LLINK(p), input);
    load_leaf_keys_in_prefix(RLINK(p), input);
  }

  void insert_end_symbol_node(const std::string &str)
  {
    insert_symbol_node(str, 0);
  }

  void clear_build_state() noexcept
  {
    while (not heap_.is_empty())
      {
        auto *node = static_cast<Huffman_Node *>(heap_.getMin_ne());
        delete node->bin_node;
        delete node;
      }
    symbols_ = nullptr;
    symbol_map_.empty();
    code_map_.empty();
    text_len_ = 0;
    end_symbol_ = "NO-END";
  }

public:
  /** Read a NUL-terminated character std::string, count frequencies and build the prefix tree.

      read_input(input, with_freqs) reads the NUL-terminated std::string
      `input`, counts the frequency of each symbol and builds the
      Huffman prefix tree.

      @param[in] input NUL-terminated std::string to encode.
      @param[in] with_freqs If true, also build the frequency tree.
      @throw std::bad_alloc if there is not enough memory.
   */
  void read_input(char *input, const bool &with_freqs = false)
  {
    ah_domain_error_if(root_ != nullptr) << "Huffman encoding tree has already been generated";

    char *curr_stream = input;
    char curr_token[Max_Token_Size];
    curr_token[1] = '\0';
    text_len_ = 0;

    while (*curr_stream != '\0')
      {
        curr_token[0] = *curr_stream++;
        update_freq(curr_token);
        text_len_++;
      }

    if (end_symbol_ == "NO-END")
      set_end_of_stream("");
    else if (symbol_map_.search(end_symbol_) == nullptr)
      insert_end_symbol_node(end_symbol_);
    generate_huffman_tree(with_freqs);
  }

  /** Read a stream, count frequencies and build the prefix tree.

      read_input(input, with_freqs) reads characters from `input`
      until EOF, counts symbol frequencies and builds the Huffman
      prefix tree.

      @param[in] input Stream containing the text to encode.
      @param[in] with_freqs If true, also build the frequency tree.
      @throw std::bad_alloc if there is not enough memory.
   */
  void read_input(std::istream &input, const bool &with_freqs = false)
  {
    ah_domain_error_if(root_ != nullptr) << "Huffman encoding tree has already been generated";

    char curr_token[2] = {'\0', '\0'};
    char ch = '\0';
    text_len_ = 0;
    while (input.get(ch))
      {
        curr_token[0] = ch;
        update_freq(curr_token);
        ++text_len_;
      }

    if (end_symbol_ == "NO-END")
      set_end_of_stream("");
    else if (symbol_map_.search(end_symbol_) == nullptr)
      insert_end_symbol_node(end_symbol_);
    generate_huffman_tree(with_freqs);
  }

  /// Defines the end-of-stream symbol.
  void set_end_of_stream(const std::string &str)
  {
    ah_domain_error_if(end_symbol_ != "NO-END") << "End symbol has already been inserted";

    ah_domain_error_if(root_ != nullptr) << "Huffman encoding tree has already been generated";

    ah_domain_error_if(symbol_map_.search(str) != nullptr)
      << "End symbol is already present as a normal symbol";

    std::string end = str;  // the copy can throw, before the symbol enters
    insert_end_symbol_node(str);
    end_symbol_.swap(end);
  }

  /// Return the configured end-of-stream symbol.
  const std::string &get_end_of_stream() const noexcept
  {
    return end_symbol_;
  }

  /** Encode the input text.

      encode(input, bit_stream) reads `input`, encodes it and appends
      the result to `bit_stream`.

      @param[in] input NUL-terminated std::string containing the text to encode.
      @param[out] bit_stream Bit array where the encoded stream is appended.
      @return Total bit length of `bit_stream` after encoding.
      @throw std::domain_error if the prefix tree has not been generated.
   */
  size_t encode(char *input, BitArray &bit_stream)
  {
    ah_domain_error_if(root_ == nullptr) << "Huffman tree has not been generated";

    ah_domain_error_if(end_symbol_ == "NO-END") << "End symbol has not been configured";

    char *curr_stream = input;
    char curr_token[Max_Token_Size];
    curr_token[1] = '\0';
    while (*curr_stream != '\0')
      {
        curr_token[0] = *curr_stream++;
        append_code(bit_stream, code_map_.find(curr_token));
      }
    append_code(bit_stream, code_map_.find(end_symbol_));

    return bit_stream.size();
  }

  /** Encode the input text from a stream.

      encode(input, bit_stream) reads from `input` until EOF, encodes
      the symbols and appends the result to `bit_stream`.

      @param[in] input Stream containing the text to encode.
      @param[out] bit_stream Bit array where the encoded stream is appended.
      @return Total bit length of `bit_stream` after encoding.
      @throw std::domain_error if the prefix tree has not been generated.
   */
  size_t encode(std::istream &input, BitArray &bit_stream)
  {
    ah_domain_error_if(root_ == nullptr) << "Huffman tree has not been generated";

    ah_domain_error_if(end_symbol_ == "NO-END") << "End symbol has not been configured";

    char curr_token[2] = {'\0', '\0'};
    char ch = '\0';
    while (input.get(ch))
      {
        curr_token[0] = ch;
        append_code(bit_stream, code_map_.find(curr_token));
      }

    append_code(bit_stream, code_map_.find(end_symbol_));
    return bit_stream.size();
  }
};

/** Huffman decoder.

@see Huffman_Encoder_Engine
@ingroup Trees
*/
class Huffman_Decoder_Engine
{
  BinNode<std::string> *root;
  std::string end_symbol;

public:
  /** Decoder constructor.

      Builds a decoder given a previously constructed Huffman tree.

      @param[in] p Root of the Huffman decoding tree.
      @param[in] end End-of-stream symbol.
   */
  Huffman_Decoder_Engine(BinNode<std::string> *p, const std::string &end) : root(p), end_symbol(end)
  {
    // empty
  }

  /// Returns the root of the Huffman decoding tree.
  BinNode<std::string> *&get_root()
  {
    ah_domain_error_if(root == nullptr) << "Huffman tree has not been generated";

    return root;
  }

  /** Decode a bit stream.

      decode(bit_stream, output) decodes `bit_stream` using the Huffman
      prefix tree and writes the decoded output into `output`.

      @param[in] bit_stream Encoded bit stream.
      @param[out] output Output stream where the decoded text is written.
  */
  void decode(const BitArray &bit_stream, std::ostream &output) const
  {
    const size_t &bit_stream_len = bit_stream.size();
    BinNode<std::string> *p = root;
    for (size_t i = 0; i < bit_stream_len; ++i)
      {
        if (bit_stream.read_bit(i) == 0)
          p = LLINK(p);
        else
          p = RLINK(p);

        ah_domain_error_if(p == nullptr) << "Invalid bits sequence";

        if (is_leaf(p))  // leaf?
          {              // yes ==> write symbol and reset to root
            const std::string &symbol = p->get_key();
            if (symbol == end_symbol)  // end reached?
              break;

            output << symbol;
            p = root;  // reset to root, a new code will be read
          }
      }
  }
};

}  // end namespace Aleph

#endif  // HUFFMAN_H
