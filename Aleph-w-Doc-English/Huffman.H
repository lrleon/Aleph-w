
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file Huffman.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef HUFFMAN_H
# define HUFFMAN_H

# include <memory>
# include <tpl_binNodeUtils.H>
# include <tpl_treap.H>
# include <tpl_binHeap.H>
# include <tpl_dynMapTree.H>
# include <bitArray.H>

# include <autosprintf.h>


namespace Aleph {

struct Huffman_Node;

typedef DynMapTree<string, Huffman_Node *, Treap_Vtl> Symbol_Map;

typedef BinNode<std::pair<string, size_t>> Freq_Node;

struct  Huffman_Node : public BinHeap<size_t>::Node
{
  BinNode<string> * bin_node; 

  Freq_Node * freq_node;

public:

  Huffman_Node() : BinHeap<size_t>::Node(0), bin_node(NULL), freq_node(NULL) 
  {
    /* Empty */ 
  }

  Huffman_Node(BinNode<string> * node) 
    : BinHeap<size_t>::Node(0), bin_node(node), freq_node(NULL)
  {
    /* Empty */
  }

  ~Huffman_Node() { /* Should not be released memory bin_node */ }

};

typedef BinHeap<size_t> Huffman_Heap;

static inline const size_t & get_freq(Huffman_Node * huffman_node)
{
  return huffman_node->get_key();
}


static inline void increase_freq(Huffman_Node * huffman_node)
{
  huffman_node->get_key()++;
}


static inline void set_freq(Huffman_Node * huffman_node, const size_t & freq)
{
  huffman_node->get_key() = freq;
}

  typedef DynMapTree<string, BitArray, Treap_Vtl> Code_Map;
static inline bool is_leaf(BinNode<string> * p)
{
  return LLINK(p) == NULL and RLINK(p) == NULL;
}

/** 
  @brief encoder de Huffman.

  @see Huffman_Decoder_Engine
    
  @ingroup Trees
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

    assert(symbol_map.size() > 0);

    if (root == NULL)
      throw domain_error("Huffman encoding tree has not been generated");

    BitArray array(0);
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
    if (root != NULL)
      throw domain_error("Huffman encoding tree has already been generated");

    if (test_end(str))
      throw domain_error("End symbol has already been inserted");

    Huffman_Node * huffman_node      = NULL;
    Huffman_Node ** huffman_node_ptr = symbol_map.test(str);

    /** Is the symbol to be defined?
    If the answer is NO, then create an entry in
    symbol_map and insert it into heap */
    if (huffman_node_ptr == NULL) 
      { 
        unique_ptr<BinNode<string> > bin_node_auto(new BinNode<string>(str));
        huffman_node = static_cast<Huffman_Node*>
          (heap.insert(new Huffman_Node(bin_node_auto.get())));
        symbol_map.insert(str, huffman_node);
        bin_node_auto.release();
      }
    else
      huffman_node = *huffman_node_ptr; /* Ya definido, recuperarlo */

    increase_freq(huffman_node); 
    heap.update(huffman_node);
  }

  static void append_code(BitArray & bit_stream, const BitArray & symbol_code)
  {
    const size_t symbol_code_size = symbol_code.size();
    for (int i = 0; i < symbol_code_size; i++) 
      bit_stream.push(symbol_code[i]); 
  }

  public:

/** 
  @brief Encoder builder. 
*/
  Huffman_Encoder_Engine() 
    : root(NULL), freq_root(NULL), end_symbol("NO-END"), text_len(0)
  {
    /* Empty */
  }

  private:

  struct Get_Key
  {
    string operator () (BinNode<string> * p) const
    {
      return is_leaf(p) ? p->get_key() : "";
    }
  };

  struct Load_Key
  {
    void operator () (BinNode<string> * p, ifstream & input) const
    {
      if (is_leaf(p))
        input >> p->get_key();
    }
  };

  public:

/** 
  @brief Save huffman tree in a file.

  @details Save Huffman tree corresponding to the encoded text
  in a file.

  The method used is by-bit prefix code
  tree and then the sequence prefix tree leaves
  corresponding to the symbols of the text.

  @param [out] output Stream successfully opened where
  save the tree.

  @throw std :: domain_error If the tree has not yet been created.

  @see Huffman_Decoder_Engine::load_tree() save_tree() load_tree()
*/
  void save_tree(ofstream & output) 
  {
    if (root == NULL)
      throw std::domain_error("Huffman tree has not been generated");

    Aleph::save_tree<BinNode<string>, Get_Key> (root, output);
  }

/**   
  @brief generates statements and bit arrangements of keys for a
  Huffman tree.

  @details save_tree_in_array_of_chars (array_name, output) Generates
  statements for two arrangements with which you can define a
  binary tree. The statements are written to a file
  text, already opened, addressed by the output stream. the
  resulting statement has the general form:

   array_name_cdp const unsigned char [n] = {list} unsigned char;
  
   const char * array_name_k [] = {list} key in code;

  The first arrangement is bits and stores the tree topology
  under a code prefix bits (a word of Lukasiewicz). the
  second arrangement saves the contents of nodes and is
  ordained in the prefix sequence. The key values are
  get_key class given by () (root, str) which should return
  a constant char * representing the contents of the node.
  The class Get_Key()(), observing the contents of the node, you must
  be very carefully planned. Its overall structure is the
  following:

  string operator () (Node * p) const;

  Where p is the prefix of the current node in the route
  binary tree. The result of this call is placed as
  array_name constant value of settlement.

  @param [in] array_name Name content under the tree nodes
  @param [out] root Open stream where are written the
  arrangements statements and their values.
  @param [out] output Stream successfully opened where
  save the tree.
  
  @throw std :: domain_error If the tree has not yet been created.
  
  @see Huffman_Encoder_Engine :: load_tree () save_tree_in_array_of_chars ()
  void save_tree_in_array_of_chars(const string & array_name, ofstream & output)
*/
  {
    if (root == NULL)
      throw std::domain_error("Huffman tree has not been generated");

    Aleph::save_tree_in_array_of_chars<BinNode<string>, Get_Key> 
      (root, array_name, output);
  }

/** 
  @brief Returns the root of the tree Huffman decoder. 
*/
  BinNode<string> *& get_root()  
  {
    if (root == NULL)
      throw std::domain_error("Huffman tree has not been generated");

    return root; 
  }

/** 
  @brief It generates tree Huffman codes.

  @details generate_huffman_tree (with_freqs) executes the Huffman algorithm
  to build the tree of prefixes according to the text read. Yes
  with_freqs is true, then the generated tree nodes
  containing the frequencies of occurrence of symbols.

  @param [in] with_freqs If the value is true, then the nodes
  contain tree generated frequencies of appearance of the
  symbols. Otherwise, the tree contains only symbols.
  
  @return The root of the binary tree of prefixes. 
  This binary tree it is the decoder and class entry
  Huffman_Decoder_Engine.

  @throw bad_alloc If there is insufficient memory.

  @see build_prefix_encoding Huffman_Decoder_Engine
*/
  BinNode<string> * generate_huffman_tree(const bool & with_freqs = false) 
  {
    while (heap.size() > 1) /* Until only one node */
      {
        Huffman_Node * l_huffman_node = (Huffman_Node*) heap.getMin(); /* Izquierda */
        Huffman_Node * r_huffman_node = (Huffman_Node*) heap.getMin(); /* Derrecha */
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
            if (l_freq_node == NULL)
              {
                l_freq_node                    = new Freq_Node;
                l_freq_node->get_key().first   = 
                  l_huffman_node->bin_node->get_key();
                l_freq_node->get_key().second = l_huffman_node->get_key();
              }

            Freq_Node *& r_freq_node = r_huffman_node->freq_node;
            if (r_freq_node == NULL)
              {
                r_freq_node                    = new Freq_Node;
                r_freq_node->get_key().first   = 
                  r_huffman_node->bin_node->get_key();
                r_freq_node->get_key().second = r_huffman_node->get_key();
              }

            const string str = gnu::autosprintf ("%d", new_freq);
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
      } 

    /* Heap remaining node is the root of the tree of prefixes */
    Huffman_Node * huffman_root = (Huffman_Node *) heap.getMin();
    root = huffman_root->bin_node;

    if (with_freqs)
      freq_root = huffman_root->freq_node;

    delete huffman_root;
    build_encoding_map(); /* Building codes mapping */

    return root;
  }

/** 
  @brief Loading and builds a binary tree from a file.

  @details load_tree (input) reads the input file referenced, which
  Huffman tree contains a previously saved
  Huffman_Encoder_Engine :: save_tree () and restore it in memory.

  @param [in] Reference to the file input is located
  bina1rio saved the tree.

  @throw std :: bad_alloc If there is not enough to build
  Under intermediate bits or to build the binary tree.

  @see load_tree() Huffman_Encoder_Engine::save_tree()
*/
  void load_tree(ifstream & input)
  {
    if (root != BinNode<string>::NullPtr)
      destroyRec(root);
    
    root = Aleph::load_tree<BinNode<string>, Load_Key> (input);
  }

/** 
  @brief Returns the root of a tree of frequencies. 
*/
  Freq_Node *& get_freq_root() 
  { 
    if (freq_root == NULL)
      throw std::domain_error("Huffman tree has not been generated");

    return freq_root; 
  }

/** 
  @brief Defines the frequency of a symbol.

  @details set_freq (str, freq) instructs the encoder symbol to str
  It is associated with a frequency of occurrence freq.

  @param [in] str Symbol.
  @param [in] freq Frequency Symbol
  
  @throw bad_alloc If there is insufficient memory.
*/
  void set_freq(const string & str, const size_t & freq)
  {
    if (root != NULL)
      throw domain_error("Huffman encoding tree has already been generated");
    if (test_end(str))
      throw domain_error("End symbol has already been inserted");
       
    /* Search str symbol */
    Huffman_Node ** huffman_node_ptr = symbol_map.test(str);

    /** It will was already defined?
     If the answer is YES, then this is an error! */
    if (huffman_node_ptr != NULL) 
      throw std::domain_error
        (gnu::autosprintf("Frequency for symbol %s has already set",
			  str.c_str()));
   
    unique_ptr<BinNode<string> > bin_node_auto(new BinNode<string>(str));
    Huffman_Node * huffman_node = new Huffman_Node(bin_node_auto.get());
    Aleph::set_freq(huffman_node, freq); 
    heap.insert(huffman_node);
    symbol_map.insert(str, huffman_node);
    bin_node_auto.release();
  }

  private:

    static const size_t Max_Token_Size = 256;

  public:

/** 
  @brief Reads a character string , counts the frequency of their
  different symbols and generates the binary tree prefixes.

  @details read_input (input, with_freqs) Read the string input,
  counted frequencies of occurrence of each symbol and
  finally generating the binary tree of prefixes.

  @param [in] input String to encode.
  @param [in] with_freqs If the value is true, then also
  prefix tree generated with frequencies in each symbol.

  @throw bad_alloc If there is insufficient memory.
*/
  void read_input(char * input, const bool & with_freqs = false)
  {
    if (root != NULL)
      throw domain_error("Huffman encoding tree has already been generated");

    char * curr_stream = input;
    char curr_token[Max_Token_Size];
    curr_token[1] = '\0';

    while (*curr_stream != '\0')
      {
        curr_token[0] = *curr_stream++; 
        update_freq(curr_token);
        text_len++;
      }

    set_end_of_stream("");
    generate_huffman_tree(with_freqs);
  }

/** 
  @brief Reads a file, counts the frequency of their
  different symbols and generates the binary tree prefixes.

  @details read_input (input, with_freqs) reads the input file input,
  counted frequencies of occurrence of each symbol and
  finally generating the binary tree of prefixes.

  @param [in] input Contentive characters encoded file.
  @param [in] with_freqs If the value is true, then also
  prefix tree generated with frequencies in each symbol.

  @throw bad_alloc If there is insufficient memory.
*/
  void read_input(ifstream & input, const bool & with_freqs = false)
  {
    if (root != NULL)
      throw domain_error("Huffman encoding tree has already been generated");

    char curr_token[2];
    curr_token[0] = curr_token[1] = '\0';

    while (not input.eof())
      {
        input.read(curr_token, 1); 
        update_freq(curr_token);
        text_len++;
      }

    set_end_of_stream("");
    generate_huffman_tree(with_freqs);
  }

/** 
  @brief Defines the symbol to entry. 
*/
  void set_end_of_stream(const string & str)
  {
    if (test_end(str))
      throw domain_error("End symbol has already been inserted");

    if (root != NULL)
      throw domain_error("Huffman encoding tree has already been generated");

    unique_ptr<BinNode<string> > bin_node_auto ( new BinNode<string> (str) );

    Huffman_Node * huffman_node =  static_cast<Huffman_Node*>
      (heap.insert(new Huffman_Node(bin_node_auto.get())));
    symbol_map.insert(str, huffman_node); 
    bin_node_auto.release();
    end_symbol = str;
  }

/** 
  @brief Encodes the input text.

  @details encode (input, bit_stream) Lee INPUT, codifies and
  encoded text type arrangement bit_stream bits.

  @param [in] input Character string Text contentiva
  encode.
  @param [out] bit_stream Arrangement where writing bits
  encoded text.
  
  @return The total length encoded text.
  
  @throw domain_error If the tree has not been generated prefixes
  by generate_huffman_tree (method)
*/
  size_t encode(char * input, BitArray & bit_stream)
  {
    if (root == NULL)
      throw std::domain_error("Huffman tree has not been generated");

    char * curr_stream = input;
    char curr_token[Max_Token_Size];
    curr_token[1] = '\0';
    while (*curr_stream != '\0')
      {
        curr_token[0] = *curr_stream++; 
        append_code(bit_stream, code_map.find(curr_token));
      }
    append_code(bit_stream, code_map.find(""));

    return bit_stream.size();
  }

/** 
  @brief Encodes the input text.

  @details encode (input, bit_stream) Lee INPUT, codifies and
  encoded text type arrangement bit_stream bits.

  @param [in] input File with text characters
  encode.
  @param [out] bit_stream Arrangement where writing bits
  encoded text.
  
  @return The total length encoded text.
  
  @throw domain_error If the tree has not been generated prefixes
  by generate_huffman_tree (method)
*/
  size_t encode(ifstream & input, BitArray & bit_stream)
  {
    if (root == NULL)
      throw std::domain_error("Huffman tree has not been generated");
    char curr_token[2];
    curr_token[0] = curr_token[1] = '\0'; 
    while (not input.eof())
      {
        input.read(curr_token, 1); 
        append_code(bit_stream, code_map.find(curr_token));
      }
    append_code(bit_stream, code_map.find(""));
    return bit_stream.size();
  }
};

/** 
  @brief encode of Huffman.

  @see Huffman_Encoder_Engine
        
  @ingroup tress
*/
class Huffman_Decoder_Engine
{
  BinNode<string> * root; 
  string end_symbol;
  public:

/** 
  @brief Decoder manufacturer.

  @details Instance a decoder according to a previously Huffman tree
  built by Huffman_Encoder_Engine class.

  @param [in] p Root binary tree Huffman decoder.
  @param [in] end End symbol input.
*/
  Huffman_Decoder_Engine(BinNode<string> * p, const string & end)
    : root(p), end_symbol(end)
  {
    /* Empty */
  }

/** 
  @brief Returns the root of the tree Huffman decoder. 
*/
  BinNode<string> *& get_root()  
  {
    if (root == NULL)
      throw std::domain_error("Huffman tree has not been generated");

    return root; 
  }

/** 
  @brief Decodes a bit string.

  @details The decode method (bit_stream, bit_stream_len, output) decodes
  bit_stream bit string of bit_stream_len length,
  decoded according to the Huffman tree prefixes, and places the
  output decoded output.

  @param [in] Stream bit_stream coded bits.
  @param [out] output Stream where the text is placed
  decoded.
*/
  void decode(BitArray & bit_stream, ostream & output)
  {
    const size_t & bit_stream_len = bit_stream.size();
    BinNode<string> * p = root;
    for (int i = 0; i < bit_stream_len; ++i)
      {
        if (bit_stream.read_bit(i) == 0)
          p = LLINK(p);
        else
          p = RLINK(p);


       if (p == NULL)
         throw std::domain_error("Invalid bits sequence");

    /** Is leaf?
     YES? ==> write symbol and restart following */
       if (is_leaf(p)) 
         {     
           const string & symbol = p->get_key();
           if (symbol == end_symbol) 
             break;

           output << symbol;
           p = root; /* Restart following, for a new code is read */
         }
      }
  }
};

} /* End namespace Aleph */

# endif // HUFFMAN_H
