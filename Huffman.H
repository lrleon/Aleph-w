
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
# ifndef HUFFMAN_H
# define HUFFMAN_H


# include <memory>
# include <istream>
# include <tpl_binNodeUtils.H>
# include <tpl_treap.H>
# include <tpl_binHeap.H>
# include <tpl_dynMapTree.H>
# include <bitArray.H>
# include <ah-errors.H>


namespace Aleph {

struct Huffman_Node;

using Symbol_Map = DynMapTree<string, Huffman_Node *, Treap_Vtl>;

using Freq_Node = BinNode<std::pair<string, size_t>>;

struct  Huffman_Node : public BinHeap<size_t>::Node
{
  BinNode<string> * bin_node; 

  Freq_Node * freq_node;

public:

  Huffman_Node() 
    : BinHeap<size_t>::Node(0), bin_node(nullptr), freq_node(nullptr) 
  {
    /* empty */ 
  }

  Huffman_Node(BinNode<string> * node) 
    : BinHeap<size_t>::Node(0), bin_node(node), freq_node(nullptr)
  {
    /* empty */
  }

  ~Huffman_Node() { /* bin_node memory must not be released here */ }

};

typedef BinHeap<size_t> Huffman_Heap;

static inline const size_t & get_freq(Huffman_Node * huffman_node) noexcept
{
  return huffman_node->get_key();
}


static inline void increase_freq(Huffman_Node * huffman_node) noexcept
{
  huffman_node->get_key()++;
}


static inline void set_freq(Huffman_Node * huffman_node, const size_t & freq)
  noexcept
{
  huffman_node->get_key() = freq;
}

  typedef DynMapTree<string, BitArray, Treap_Vtl> Code_Map;
static inline bool is_leaf(BinNode<string> * p) noexcept
{
  return LLINK(p) == nullptr and RLINK(p) == nullptr;
}

	    /** Huffman encoder.

	        @see Huffman_Decoder_Engine
	        @ingroup Arboles
	     */
class Huffman_Encoder_Engine
{
  BinNode<string> * root; 
  Huffman_Heap heap;
  Symbol_Map   symbol_map; 
  Code_Map code_map;

  Freq_Node * freq_root;

  string end_symbol;
  size_t text_len;

  void build_prefix_encoding(BinNode<string> * p, BitArray & array)
  {
    if (is_leaf(p))
      {
        const string & str = p->get_key();
        code_map.insert(str, BitArray(array));
        return;
      }
    array.push(0); build_prefix_encoding(LLINK(p), array); array.pop();
    array.push(1); build_prefix_encoding(RLINK(p), array); array.pop();
  }

	  void build_encoding_map()
	  {
	    ah_domain_error_if(root == nullptr) << "Huffman encoding tree has not been generated";

	    BitArray array(0);
	    code_map.empty();
	    symbol_map.empty();
	    build_prefix_encoding(root, array);
	  }

  bool test_end(const string & str) const
  {
    if (end_symbol == "NO-END") 
      return false;
    return end_symbol == str;
  }

  void update_freq(const string & str)
  {
    ah_domain_error_if(root != nullptr) << "Huffman encoding tree has already been generated";

    ah_domain_error_if(test_end(str)) << "End symbol has already been inserted";

    Huffman_Node * huffman_node      = nullptr;
    auto huffman_node_ptr = symbol_map.search(str);
	    if (huffman_node_ptr == nullptr) // symbol defined previously?
	      { // No ==> create a new entry in symbol_map and insert it into heap
        unique_ptr<BinNode<string> > bin_node_auto(new BinNode<string>(str));
        huffman_node = static_cast<Huffman_Node*>
          (heap.insert(new Huffman_Node(bin_node_auto.get())));
        symbol_map.insert(str, huffman_node);
        bin_node_auto.release();
      }
	    else
	      huffman_node = huffman_node_ptr->second; // already defined, retrieve it

    increase_freq(huffman_node); 
    heap.update(huffman_node);
  }

	  static void append_code(BitArray & bit_stream, const BitArray & symbol_code)
	  {
	    const size_t symbol_code_size = symbol_code.size();
	    for (size_t i = 0; i < symbol_code_size; ++i) 
	      bit_stream.push(symbol_code[i]); 
	  }

  public:

	    /// Encoder constructor.
  Huffman_Encoder_Engine() 
    : root(nullptr), freq_root(nullptr), end_symbol("NO-END"), text_len(0)
  {
    // empty
  }

  private:

  struct Get_Key
  {
    string operator () (BinNode<string> * p) const noexcept
    {
      return is_leaf(p) ? p->get_key() : "";
    }
  };

  struct Load_Key
  {
    void operator () (BinNode<string> * p, istream & input) const noexcept
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
  void save_tree(ostream & output) 
  {
    ah_domain_error_if(root == nullptr)
      << "Huffman tree has not been generated";

    BitArray prefix;
    tree_to_bits(root, prefix);
    prefix.save(output);

    save_string_as_bytes(end_symbol, output);
    output << endl;

    save_leaf_keys_in_prefix(root, output);
  }

	      /** Generate C/C++ array declarations for a Huffman tree.

	          save_tree_in_array_of_chars(array_name, output) writes two array
	          declarations that can be used to reconstruct the binary tree:

	          - `const unsigned char array_name_cdp[n] = { ... };`
	          - `const char * array_name_k[] = { ... };`

	          The first array stores the topology as a prefix bit code (a
	          Lukasiewicz word). The second array stores node contents in
	          preorder. The `Get_Key` functor is used to stringify the node
	          contents; internal nodes typically return an empty string.

	          @param[in] array_name Prefix name for the generated arrays.
	          @param[out] output Output stream where declarations are written.
	          @throw std::domain_error if the tree has not been generated.
	          @see Aleph::load_tree_from_array()
	       */
  void save_tree_in_array_of_chars(const string & array_name, ostream & output)
  {
    ah_domain_error_if(root == nullptr)
      << "Huffman tree has not been generated";

    Aleph::save_tree_in_array_of_chars<BinNode<string>, Get_Key> 
      (root, array_name, output);
  }

	    /// Returns the root of the Huffman decoding tree.
  BinNode<string> *& get_root()  
  {
    ah_domain_error_if(root == nullptr)
      << "Huffman tree has not been generated";

    return root; 
  }

	    /** Generate the Huffman prefix tree.

	        generate_huffman_tree(with_freqs) runs Huffman's algorithm and
	        builds the prefix tree from the collected symbol frequencies. If
	        with_freqs is true, an additional tree containing frequencies is
	        built as well.

	        @param[in] with_freqs If true, also build the frequency tree.
	        @return The root of the Huffman decoding (prefix) tree.
	        @throw std::bad_alloc if there is not enough memory.
	        @see build_prefix_encoding Huffman_Decoder_Engine
	     */
	  BinNode<string> * generate_huffman_tree(const bool & with_freqs = false) 
	  {
	    ah_domain_error_if(root != nullptr) << "Huffman encoding tree has already been generated";
	    ah_domain_error_if(heap.is_empty()) << "No symbols have been inserted";

	    freq_root = nullptr;

	    while (heap.size() > 1) // until only one node remains
	      {
	        Huffman_Node * l_huffman_node = (Huffman_Node*) heap.getMin(); // left
	        Huffman_Node * r_huffman_node = (Huffman_Node*) heap.getMin(); // right
        BinNode <string> * bin_node = new BinNode <string>;
        Huffman_Node * huffman_node = new Huffman_Node (bin_node);
        LLINK(bin_node) = l_huffman_node->bin_node;
        RLINK(bin_node) = r_huffman_node->bin_node;
        const size_t new_freq = get_freq(l_huffman_node) + 
	  get_freq(r_huffman_node);
        Aleph::set_freq(huffman_node, new_freq);

        if (with_freqs)
          {
            Freq_Node *& l_freq_node = l_huffman_node->freq_node;
            if (l_freq_node == nullptr)
              {
                l_freq_node                    = new Freq_Node;
                l_freq_node->get_key().first   = 
                  l_huffman_node->bin_node->get_key();
                l_freq_node->get_key().second = l_huffman_node->get_key();
              }

            Freq_Node *& r_freq_node = r_huffman_node->freq_node;
            if (r_freq_node == nullptr)
              {
                r_freq_node                    = new Freq_Node;
                r_freq_node->get_key().first   = 
                  r_huffman_node->bin_node->get_key();
                r_freq_node->get_key().second = r_huffman_node->get_key();
              }

            const string str = to_string(new_freq);
            Freq_Node *& freq_node      = huffman_node->freq_node;
            freq_node                   = new Freq_Node;
            freq_node->get_key().first  = str;
            freq_node->get_key().second = huffman_node->get_key();
            LLINK(freq_node)            = l_freq_node;
            RLINK(freq_node)            = r_freq_node;
          }

        delete l_huffman_node;
        delete r_huffman_node;
        heap.insert(huffman_node);
	      } // the remaining node in heap is the prefix tree root

    Huffman_Node * huffman_root = (Huffman_Node *) heap.getMin();
    root = huffman_root->bin_node;

    if (with_freqs)
      freq_root = huffman_root->freq_node;

    delete huffman_root;
	    build_encoding_map(); // build code map

    return root;
  }

	     /** Load and build a binary tree from a stream.

	          load_tree(input) reads a Huffman tree previously saved with
	          save_tree() and restores it into memory.

	          @param[in] input Input stream containing the serialized tree.
	          @throw std::bad_alloc if there is not enough memory.
	          @throw std::domain_error if the stream is malformed.
	          @see save_tree()
	      */
  void load_tree(istream & input)
  {
    clear_build_state();

    destroyRec(root);
    destroyRec(freq_root);

    BitArray prefix;
    prefix.load(input);
    root = bits_to_tree<BinNode<string>>(prefix);

    end_symbol = load_string_from_bytes(input);
    load_leaf_keys_in_prefix(root, input);

    build_encoding_map();
  }

	    /// Returns the root of the frequency tree.
  Freq_Node *& get_freq_root() 
  { 
    ah_domain_error_if(freq_root == nullptr)
      << "Huffman tree has not been generated";

    return freq_root; 
  }

	    /** Define the frequency of a symbol.

	        set_freq(str, freq) tells the encoder that symbol `str` has
	        frequency `freq`.

	        @param[in] str Symbol.
	        @param[in] freq Symbol frequency.
	        @throw std::bad_alloc if there is not enough memory.
	     */
  void set_freq(const string & str, const size_t & freq)
  {
    ah_domain_error_if(root != nullptr) << "Huffman encoding tree has already been generated";
    ah_domain_error_if(test_end(str)) << "End symbol has already been inserted";
	       
	        // Search symbol str
	    auto huffman_node_ptr = symbol_map.search(str);
	    if (huffman_node_ptr != nullptr) // already defined?
	      {
		std::string msg = "Frequency for symbol " + str + " has already set";
		ah_domain_error() << msg; // Yes ==> this is an error!
	      }
   
    unique_ptr<BinNode<string> > bin_node_auto(new BinNode<string>(str));
    Huffman_Node * huffman_node = new Huffman_Node(bin_node_auto.get());
    Aleph::set_freq(huffman_node, freq); 
    heap.insert(huffman_node);
    symbol_map.insert(str, huffman_node);
    bin_node_auto.release();
  }

  private:

    static const size_t Max_Token_Size = 256;

    static void save_string_as_bytes(const string & str, ostream & output)
    {
      output << str.size() << " ";
      for (unsigned char c : str)
        output << static_cast<unsigned int>(c) << " ";
    }

    static string load_string_from_bytes(istream & input)
    {
      size_t len = 0;
      input >> len;
      ah_domain_error_if(not input) << "Malformed Huffman tree stream";
      ah_domain_error_if(len > Max_Token_Size) << "Symbol too large in Huffman tree stream";

      string str;
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

    static void save_leaf_keys_in_prefix(BinNode<string> * p, ostream & output)
    {
      if (p == BinNode<string>::NullPtr)
        return;

      if (is_leaf(p))
        {
          save_string_as_bytes(p->get_key(), output);
          output << endl;
          return;
        }

      save_leaf_keys_in_prefix(LLINK(p), output);
      save_leaf_keys_in_prefix(RLINK(p), output);
    }

    static void load_leaf_keys_in_prefix(BinNode<string> * p, istream & input)
    {
      if (p == BinNode<string>::NullPtr)
        return;

      if (is_leaf(p))
        {
          p->get_key() = load_string_from_bytes(input);
          return;
        }

      load_leaf_keys_in_prefix(LLINK(p), input);
      load_leaf_keys_in_prefix(RLINK(p), input);
    }

    void insert_end_symbol_node(const string & str)
    {
      unique_ptr<BinNode<string> > bin_node_auto(new BinNode<string>(str));

      Huffman_Node * huffman_node = static_cast<Huffman_Node*>
        (heap.insert(new Huffman_Node(bin_node_auto.get())));
      symbol_map.insert(str, huffman_node);
      bin_node_auto.release();
    }

    void clear_build_state() noexcept
    {
      while (not heap.is_empty())
        {
          auto * node = static_cast<Huffman_Node*>(heap.getMin_ne());
          delete node->bin_node;
          delete node;
        }
      symbol_map.empty();
      code_map.empty();
      text_len = 0;
      end_symbol = "NO-END";
    }

  public:

	      /** Read a NUL-terminated character string, count frequencies and build the prefix tree.

	          read_input(input, with_freqs) reads the NUL-terminated string
	          `input`, counts the frequency of each symbol and builds the
	          Huffman prefix tree.

	          @param[in] input NUL-terminated string to encode.
	          @param[in] with_freqs If true, also build the frequency tree.
	          @throw std::bad_alloc if there is not enough memory.
	       */
  void read_input(char * input, const bool & with_freqs = false)
  {
    ah_domain_error_if(root != nullptr) << "Huffman encoding tree has already been generated";

    char * curr_stream = input;
    char curr_token[Max_Token_Size];
    curr_token[1] = '\0';
    text_len = 0;

    while (*curr_stream != '\0')
      {
        curr_token[0] = *curr_stream++; 
        update_freq(curr_token);
        text_len++;
      }

    if (end_symbol == "NO-END")
      set_end_of_stream("");
    else if (symbol_map.search(end_symbol) == nullptr)
      insert_end_symbol_node(end_symbol);
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
  void read_input(istream & input, const bool & with_freqs = false)
  {
    ah_domain_error_if(root != nullptr) << "Huffman encoding tree has already been generated";

    char curr_token[2] = {'\0', '\0'};
    char ch = '\0';
    text_len = 0;
    while (input.get(ch))
      {
        curr_token[0] = ch;
        update_freq(curr_token);
        ++text_len;
      }

    if (end_symbol == "NO-END")
      set_end_of_stream("");
    else if (symbol_map.search(end_symbol) == nullptr)
      insert_end_symbol_node(end_symbol);
    generate_huffman_tree(with_freqs);
  }

	    /// Defines the end-of-stream symbol.
  void set_end_of_stream(const string & str)
  {
    ah_domain_error_if(end_symbol != "NO-END") << "End symbol has already been inserted";

    ah_domain_error_if(root != nullptr) << "Huffman encoding tree has already been generated";

    ah_domain_error_if(symbol_map.search(str) != nullptr)
      << "End symbol is already present as a normal symbol";

    insert_end_symbol_node(str);
    end_symbol = str;
  }

    /// Return the configured end-of-stream symbol.
  const string & get_end_of_stream() const noexcept
  {
    return end_symbol;
  }

	      /** Encode the input text.

	          encode(input, bit_stream) reads `input`, encodes it and appends
	          the result to `bit_stream`.

	          @param[in] input NUL-terminated string containing the text to encode.
	          @param[out] bit_stream Bit array where the encoded stream is appended.
	          @return Total bit length of `bit_stream` after encoding.
	          @throw std::domain_error if the prefix tree has not been generated.
	       */
  size_t encode(char * input, BitArray & bit_stream)
  {
    ah_domain_error_if(root == nullptr)
      << "Huffman tree has not been generated";

    ah_domain_error_if(end_symbol == "NO-END")
      << "End symbol has not been configured";

    char * curr_stream = input;
    char curr_token[Max_Token_Size];
    curr_token[1] = '\0';
    while (*curr_stream != '\0')
      {
        curr_token[0] = *curr_stream++; 
        append_code(bit_stream, code_map.find(curr_token));
      }
    append_code(bit_stream, code_map.find(end_symbol));

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
  size_t encode(istream & input, BitArray & bit_stream)
  {
    ah_domain_error_if(root == nullptr)
      << "Huffman tree has not been generated";

    ah_domain_error_if(end_symbol == "NO-END")
      << "End symbol has not been configured";

    char curr_token[2] = {'\0', '\0'};
    char ch = '\0';
    while (input.get(ch))
      {
        curr_token[0] = ch;
        append_code(bit_stream, code_map.find(curr_token));
      }

    append_code(bit_stream, code_map.find(end_symbol));
    return bit_stream.size();
  }
};

	    /** Huffman decoder.

        @see Huffman_Encoder_Engine
        @ingroup Arboles
     */
class Huffman_Decoder_Engine
{
  BinNode<string> * root; 
  string end_symbol;
  public:

	    /** Decoder constructor.

	        Builds a decoder given a previously constructed Huffman tree.

	        @param[in] p Root of the Huffman decoding tree.
	        @param[in] end End-of-stream symbol.
	     */
  Huffman_Decoder_Engine(BinNode<string> * p, const string & end)
    : root(p), end_symbol(end)
  {
    // empty
  }

	    /// Returns the root of the Huffman decoding tree.
  BinNode<string> *& get_root()  
  {
    ah_domain_error_if(root == nullptr)
      << "Huffman tree has not been generated";

    return root; 
  }

	    /** Decode a bit stream.

	        decode(bit_stream, output) decodes `bit_stream` using the Huffman
	        prefix tree and writes the decoded output into `output`.

	        @param[in] bit_stream Encoded bit stream.
	        @param[out] output Output stream where the decoded text is written.
	    */
	  void decode(BitArray & bit_stream, ostream & output)
	  {
	    const size_t & bit_stream_len = bit_stream.size();
	    BinNode<string> * p = root;
	    for (size_t i = 0; i < bit_stream_len; ++i)
	      {
	        if (bit_stream.read_bit(i) == 0)
	          p = LLINK(p);
	        else
	          p = RLINK(p);


       ah_domain_error_if(p == nullptr)
         << "Invalid bits sequence";

	       if (is_leaf(p)) // leaf?
	         {     // yes ==> write symbol and reset to root
	           const string & symbol = p->get_key();
	           if (symbol == end_symbol) // end reached?
	             break;

	           output << symbol;
	           p = root; // reset to root, a new code will be read
	         }
      }
  }
};

} // end namespace Aleph

# endif // HUFFMAN_H
