# ifndef HUFFMAN_H
# define HUFFMAN_H


# include <memory>
# include <tpl_binNodeUtils.H>
# include <tpl_treap.H>
# include <tpl_binHeap.H>
# include <tpl_dynMapTree.H>
# include <bitArray.H>


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

  ~Huffman_Node() { /* No debe liberarse memoria de bin_node */ }

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

    /** Codificador de Huffman.

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
    assert(symbol_map.size() > 0);

    if (root == nullptr)
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
    if (root != nullptr)
      throw domain_error("Huffman encoding tree has already been generated");

    if (test_end(str))
      throw domain_error("End symbol has already been inserted");

    Huffman_Node * huffman_node      = nullptr;
    auto huffman_node_ptr = symbol_map.search(str);
    if (huffman_node_ptr == nullptr) // ¿símbolo definido previamente?
      { // No ==> crear una entrada en symbol_map e insertarlo en heap
        unique_ptr<BinNode<string> > bin_node_auto(new BinNode<string>(str));
        huffman_node = static_cast<Huffman_Node*>
          (heap.insert(new Huffman_Node(bin_node_auto.get())));
        symbol_map.insert(str, huffman_node);
        bin_node_auto.release();
      }
    else
      huffman_node = huffman_node_ptr->second; // ya definido, recuperarlo

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

    /// Constructor de codificador.
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

      /** Guarda un árbol de Huffman en un archivo.

          Guarda el árbol de Huffman correspondiente al texto codificado
          en un archivo.  

          El método utilizado es a través del código prefijo en bits del
          árbol y luego la secuencia prefija de las hojas del árbol
          correspondientes a los símbolos del texto.

          @param[out] output un stream correctamente abierto donde se
          guarda el árbol.
          @throw std::domain_error si el árbol aún no ha sido generado. 
          @see Huffman_Decoder_Engine::load_tree() save_tree() load_tree()
       */
  void save_tree(ostream & output) 
  {
    if (root == nullptr)
      throw std::domain_error("Huffman tree has not been generated");

    Aleph::save_tree<BinNode<string>> (root, output);
  }

      /** Genera declaraciones de arreglos de bits y de claves para un
          árbol de Huffman.

          save_tree_in_array_of_chars(array_name, output) genera
          declaraciones para dos arreglos con los que se puede definir un
          árbol binario. Las declaraciones son escritas en un archivo
          texto, ya abierto, direccionado por el stream output. La
          declaración resultante tiene la forma genérica siguiente: 

          const unsigned char array_name_cdp[n] = { lista de unsigned char };
          
          const char * array_name_k[] = { lista de claves en prefijo };

          El primer arreglo es de bits y guarda la topología del árbol
          bajo un código prefijo de bits (una palabra de Lukasiewicz). El
          segundo arreglo, guarda los contenidos de los nodos y está
          ordenado en la secuencia prefija. Los valores de las claves son
          dados por la clase Get_Key()(root, str) el cual debe retornar
          una constante char* que represente el contenido del nodo.

          La clase Get_Key()(), observadora del contenido del nodo, debe
          ser muy cuidadosamente programada. Su estructura general es la
          siguiente:

          string operator() (Node * p) const;

          Donde p es el nodo actual dentro del recorrido prefijo del
          árbol binario. El resultado de esta llamada es colocado como
          valor constante del arreglo array_name.

          @param[in] array_name El nombre del arreglo de contenidos de los
          nodos del árbol
          @param[out] output stream abierto donde se escriben las
          declaraciones de los arreglos, así como sus valores.

          @param[out] output un stream correctamente abierto donde se
          guarda el árbol.
          @throw std::domain_error si el árbol aún no ha sido generado. 
          @see Huffman_Encoder_Engine::load_tree() save_tree_in_array_of_chars()
       */
  void save_tree_in_array_of_chars(const string & array_name, ostream & output)
  {
    if (root == nullptr)
      throw std::domain_error("Huffman tree has not been generated");

    Aleph::save_tree_in_array_of_chars<BinNode<string>, Get_Key> 
      (root, array_name, output);
  }

    /// Retorna la raíz del árbol decodificador de Huffman.
  BinNode<string> *& get_root()  
  {
    if (root == nullptr)
      throw std::domain_error("Huffman tree has not been generated");

    return root; 
  }

    /** Genera el árbol de prefijos de Huffman.

        generate_huffman_tree(with_freqs) ejecuta el algoritmo de Huffman
        para generar el árbol de prefijos en función del texto leído. Si
        with_freqs es true, entonces los nodos del árbol generado
        contienen las frecuencias de aparición de los símbolos.

        @param[in] with_freqs si el valor es true, entonces los nodos del
        árbol generado contendrán las frecuencias de aparición de los
        símbolos. De lo contrario, el árbol sólo contiene los símbolos.
        @return la raíz del árbol binario de prefijos. Este árbol binario
        es el decodificador y es entrada de la clase
        Huffman_Decoder_Engine.
        @throw bad_alloc si no hay suficiente memoria.
        @see build_prefix_encoding Huffman_Decoder_Engine
     */
  BinNode<string> * generate_huffman_tree(const bool & with_freqs = false) 
  {
    while (heap.size() > 1) // hasta que quede solo un nodo
      {
        Huffman_Node * l_huffman_node = (Huffman_Node*) heap.getMin(); // izq
        Huffman_Node * r_huffman_node = (Huffman_Node*) heap.getMin(); // der
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
      } // nodo que queda en heap es la raíz del árbol de prefijos  

    Huffman_Node * huffman_root = (Huffman_Node *) heap.getMin();
    root = huffman_root->bin_node;

    if (with_freqs)
      freq_root = huffman_root->freq_node;

    delete huffman_root;
    build_encoding_map(); // construir mapeo de códigos

    return root;
  }

     /** Carga y construye un árbol binario desde un archivo.

          load_tree(input) lee el archivo referenciado por input, el cual
          contiene un árbol de Huffman previamente guardado con 
          Huffman_Encoder_Engine::save_tree() y lo restaura en memoria.

          @param[in] input referencia al archivo donde se encuentra
          guardado el árbol bina1rio. 
          @throw std::bad_alloc sin no hay suficiente para construir
          arreglo de bits intermedio o para construir el árbol binario.
          @see load_tree() Huffman_Encoder_Engine::save_tree()

      */
  void load_tree(ifstream & input)
  {
    if (root != BinNode<string>::NullPtr)
      destroyRec(root);
    
    root = Aleph::load_tree<BinNode<string>>(input);
  }

    /// Retorna la raíz de un árbol de frecuencias.
  Freq_Node *& get_freq_root() 
  { 
    if (freq_root == nullptr)
      throw std::domain_error("Huffman tree has not been generated");

    return freq_root; 
  }

    /** Define la frecuencia de aparición de un símbolo.

        set_freq(str, freq) instruye al codificador que al símbolo str
        se le asocia una frecuencia de aparición freq.

        @param[in] str símbolo.
        @param[in] freq frecuencia del símbolo
        @throw bad_alloc si no hay suficiente memoria.
     */
  void set_freq(const string & str, const size_t & freq)
  {
    if (root != nullptr)
      throw domain_error("Huffman encoding tree has already been generated");
    if (test_end(str))
      throw domain_error("End symbol has already been inserted");
       
        // Buscar símbolo str
    auto huffman_node_ptr = symbol_map.search(str);
    if (huffman_node_ptr != nullptr) // ¿ya fue definido?
      {
	std::string msg = "Frequency for symbol " + str + " has already set";
	throw std::domain_error(msg); // Sí ==> esto es un error!
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

  public:

      /** Lee una cadena caracteres, contabiliza las frecuencias de sus
          distintos símbolos y genera el árbol binario de prefijos.

          read_input(input, with_freqs) lee la cadena de caracteres input,
          contabiliza las frecuencias de aparición de cada símbolo y
          finalmente genera el árbol binario de prefijos.

          @param[in] input cadena de caracteres a codificar.
          @param[in] with_freqs si el valor es true, entonces también se
          genera el árbol de prefijos con las frecuencias en cada símbolo.
          @throw bad_alloc si no hay suficiente memoria.
       */
  void read_input(char * input, const bool & with_freqs = false)
  {
    if (root != nullptr)
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

      /** Lee un archivo, contabiliza las frecuencias de sus
          distintos símbolos y genera el árbol binario de prefijos.

          read_input(input, with_freqs) lee el archivo de entrada input,
          contabiliza las frecuencias de aparición de cada símbolo y
          finalmente genera el árbol binario de prefijos.

          @param[in] input archivo contentivo de los caracteres a codificar.
          @param[in] with_freqs si el valor es true, entonces también se
          genera el árbol de prefijos con las frecuencias en cada símbolo.
          @throw bad_alloc si no hay suficiente memoria.
       */
  void read_input(ifstream & input, const bool & with_freqs = false)
  {
    if (root != nullptr)
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

    /// Define el símbolo fin de entrada.
  void set_end_of_stream(const string & str)
  {
    if (test_end(str))
      throw domain_error("End symbol has already been inserted");

    if (root != nullptr)
      throw domain_error("Huffman encoding tree has already been generated");

    unique_ptr<BinNode<string> > bin_node_auto ( new BinNode<string> (str) );

    Huffman_Node * huffman_node =  static_cast<Huffman_Node*>
      (heap.insert(new Huffman_Node(bin_node_auto.get())));
    symbol_map.insert(str, huffman_node); 
    bin_node_auto.release();
    end_symbol = str;
  }

      /** Codifica el texto input.

          encode(input,bit_stream) lee la entrada input, la codifica y
          escribe el texto codificado en el arreglo de bits bit_stream.

          @param[in] input cadena de caracteres contentiva del texto a
          codificar.
          @param[out] bit_stream el arreglo de bits donde escribir el
          texto codificado.  
          @return la longitud de total del texto codificado.
          @throw domain_error si el árbol de prefijos no ha sido generado
          mediante el método generate_huffman_tree()
       */
  size_t encode(char * input, BitArray & bit_stream)
  {
    if (root == nullptr)
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

      /** Codifica el texto input.

          encode(input,bit_stream) lee la entrada input, la codifica y
          escribe el texto codificado en el arreglo de bits bit_stream.

          @param[in] input archivo con los caracteres del texto a
          codificar.
          @param[out] bit_stream el arreglo de bits donde escribir el
          texto codificado.  
          @return la longitud de total del texto codificado.
          @throw domain_error si el árbol de prefijos no ha sido generado
          mediante el método generate_huffman_tree()
       */
  size_t encode(ifstream & input, BitArray & bit_stream)
  {
    if (root == nullptr)
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

    /** Decodificador de Huffman.

        @see Huffman_Encoder_Engine
        @ingroup Arboles
     */
class Huffman_Decoder_Engine
{
  BinNode<string> * root; 
  string end_symbol;
  public:

    /** Constructor del decodificador.

        Instancia un decodificador según un árbol de Huffman previamente
        construido mediante la clase Huffman_Encoder_Engine.

        @param[in] p raíz del árbol binario de Huffman decodificador.
        @param[in] end símbolo de fin de fin de entrada.
     */
  Huffman_Decoder_Engine(BinNode<string> * p, const string & end)
    : root(p), end_symbol(end)
  {
    // empty
  }

    /// Retorna la raíz del árbol decodificador de Huffman.
  BinNode<string> *& get_root()  
  {
    if (root == nullptr)
      throw std::domain_error("Huffman tree has not been generated");

    return root; 
  }

    /** Decodifica una cadena de bits.

        El método decode(bit_stream, bit_stream_len, output) decodifica la
        cadena de bits bit_stream de longitud bit_stream_len, la
        decodifica según el árbol de prefijos de Huffman, y coloca la
        salida decodificada en output.

        @param[in] bit_stream la cadena de bits codificada.
        @param[out] output stream por donde se colocará el texto
        decodificado. 
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


       if (p == nullptr)
         throw std::domain_error("Invalid bits sequence");

       if (is_leaf(p)) // ¿es hoja?
         {     // sí ==> escribir símbolo y reiniciar a raíz
           const string & symbol = p->get_key();
           if (symbol == end_symbol) // ¿se alcanza final?
             break;

           output << symbol;
           p = root; // reiniciar a raíz, pues se leerá un nuevo código
         }
      }
  }
};

} // end namespace Aleph

# endif // HUFFMAN_H
