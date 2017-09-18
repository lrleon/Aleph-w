# ifndef TPL_GRAPH_INDEXES_H
# define TPL_GRAPH_INDEXES_H

# include <tpl_dynSetTree.H>
# include <tpl_graph.H>

using namespace Aleph;

namespace Aleph
{

template <class GT>
struct Dft_Node_Cmp
{
  bool operator () (typename GT::Node * p1, typename GT::Node * p2) const
  {
    return p1->get_info() < p2->get_info();
  }
};

template <class GT>
struct Dft_Arc_Cmp
{
  bool operator () (typename GT::Arc * a1, typename GT::Arc * a2) const
  {
    if (a1->src_node < a2->src_node)
      return true;

    return not (a2->src_node < a1->src_node) and a1->tgt_node < a2->tgt_node;
  }
};

  /** Construye un índice de nodos para su rápida búsqueda y recuperación.

      IndexNode indiza los nodos de un grafo según un clave dada
      definida por el usuario.

      La clase recibe los siguientes parámetros tipo:
      -# GT: el tipo de grafo basado en List_Graph.
      -# Compare: clase de comparación para la clave de indización. El
      contrato de esta clase es intrumentar el operador () así:  
      \code
          template <class GT>  
	struct Dft_Node_Cmp  
	{  
            bool  
          operator () (typename GT::Node * p1, typename GT::Node * p2) const 
          {  
	    // acceso a los nodos y comparación según el campo deseado 
          }  
      }; 
      \endcode
      IndexNode recibe esta clase por omisión programada para comparar
      el valor retornado por get_info() sobre cada nodo. Para ello, el
      operador < del tipo GT::Node_Type debe estar implementado.
      -# SN: clase filtro para el iterador de nodos en caso de que se construya
      -# Tree: el tipo de árbol binario de búsqueda usado internamente
      para indizar las claves. Por omisión se usan treaps.

      @see IndexArc Index_Graph

      @ingroup Grafos
      @author Leandro Rabindranath León (lrleon en ula punto ve)
      @author Alejandro Mujica (aledrums en gmail punto com)
  */
template <
          class GT,
          class Compare = Dft_Node_Cmp<GT>,
          template <typename /* Key */, class /* Compare */> class Tree = Treap,
          class SN = Dft_Show_Node<GT>
         >
class Nodes_Index : public DynSetTree <typename GT::Node *, Tree, Compare>
{
  typedef typename GT::Node GT_Node;
  typedef typename GT::Node_Type GT_Node_Info;
  typedef DynSetTree <GT_Node *, Tree, Compare> Tree_Type;

  GT & g;
  SN & sn;

  void init()
  {
    for (Node_Iterator<GT, SN> it(g, sn); it.has_current(); it.next())
      this->insert(it.get_current());
  }

public:

  Nodes_Index(GT & _g, Compare & cmp, SN & _sn)
    : Tree_Type(cmp), g(_g), sn(_sn)
  {
    init();
  }

  Nodes_Index(GT & _g, Compare && cmp = Compare(), SN && _sn = SN())
    : Tree_Type(cmp), g(_g), sn(_sn)
  {
    init();
  }

      /** Inserta el nodo p en el grafo y luego en el índice

	  @param[in] puntero al nodo que se quiere insertar.
	  @return puntero al nodo insertado.
      */

  GT_Node * insert_in_graph(GT_Node * p)
  {
    g.insert_node(p);

    if (insert(p) == nullptr)
      {
        g.remove_node(p);
        return nullptr;
      }

    return p;
  }

      /** Busca en el índice al nodo p y lo retorna, si no existe lo inserta
          tanto en el índice como en el grafo.

	  @param[in] puntero al nodo que se quiere buscar o insertar.
	  @return puntero al nodo encontrado o insertado.
      */
  GT_Node * search_or_insert_in_graph(GT_Node * p)
  {
    g.insert_node(p);
    GT_Node ** ptr_node = search_or_insert(p);
    GT_Node * q = *ptr_node;

    if (p != q)
      g.remove_node(p);

    return q;
  }

      /** Crea un nuevo nodo con contenido genérico info, lo inserta en
	  el grafo y luego en el índice

	  @param[in] info Contenido asociado al nuevo nodo.
	  @return puntero al nodo
	  @throw bad_alloc si no hay suficiente memoria
      */
  GT_Node * insert_in_graph(const GT_Node_Info & info)
  {
    GT_Node * p = g.insert_node(info);
    
    if (this->insert(p) == nullptr)
      {
        g.remove_node(p);
        return nullptr;
      }

    return p;
  }

      /** Busca en el índice un nodo con contenido genérico info, si no existe
          lo inserta en el grafo y luego en el índice

	  @param[in] info Contenido asociado al nodo que se quiere encontrar o
                     insertar.
	  @return puntero al nodo encontrado o insertado-
	  @throw bad_alloc si no hay suficiente memoria
      */
  GT_Node * search_or_insert_in_graph(const GT_Node_Info & info)
  {
    GT_Node * p = g.insert_node(info);
    GT_Node ** ptr_node = search_or_insert(p);
    GT_Node * q = *ptr_node;

    if (p != q)
      g.remove_node(p);

    return q;
  }
  
  /// \overload insert_in_graph
  GT_Node * insert_in_graph(const GT_Node_Info && info = GT_Node_Info())
  {
    return insert_in_graph(info);
  }

      /** Busca un nodo según el contenido de p.

	  Tome en cuenta que el criterio de búsqueda está determinado
	  por la clase parámetro de comparación Compare.

	  @param[in] p el nodo cuya información se desea buscar.
	  @return puntero al nodo si es encontrado o nulo de lo
	  contrario.
      */
  GT_Node * search(GT_Node * p)
  {
    GT_Node ** ret_val = Tree_Type::search(p);

    if (ret_val == nullptr)
      return nullptr;

    return *ret_val;
  }

  GT_Node * search(const GT_Node_Info & info)
  {
    GT_Node dummy_node(info);
    GT_Node * dummy_node_ptr = &dummy_node;

    return search(dummy_node_ptr);
  }

      /** Elimina el nodo p del grafo y del ìndice.

	  @param[in] p puntero al nodo a eliminar
	  @throw domain_error si el nodo no està insertado en el índice (lo
	  que posiblemente signifique que tampoco en el grafo).
   */
  void remove_from_graph(GT_Node * p) throw(std::exception, std::domain_error)
  {
    Tree_Type::find(p); // dispara excepción si p no está en el índice
    Tree_Type::remove(p);
    g.remove_node(p);
  }
};

  /** Construye un índice de arcos para su rápida búsqueda y
      recuperación dado sus nodos. 

      IndexArc indiza los arcos de un grafo según el par de nodos que el
      arco relaciona. La idea es implementar la detección de existencia
      de arco y su recuperación en tiempo logarítmico.

      La clase recibe los siguientes parámetros tipo:
      -# GT: el tipo de grafo basado en List_Graph.
      -# Tree: el tipo de árbol binario de búsqueda usado internamente
      para indizar las claves. Por omisión se usan treaps.

      @warning Para que la compilación correcta es imperativo que el
      exista el constructor GT::Arc(void * src, void * tgt,
      GT::Arc_Type). Del mismo modo, también debe existir el constructor
      por omisión GT::Arc_Type().

      @see IndexNode Index_Graph

      @ingroup Grafos
      @author Leandro Rabindranath León (lrleon en ula punto ve)
      @author Alejandro Mujica (aledrums en gmail punto com)
  */

template <
          class GT,
          class Compare = Dft_Arc_Cmp<GT>,
          template <typename /* Key */, class /* Compare */> class Tree = Treap,
          class SA = Dft_Show_Arc<GT>
         >
class Arcs_Index : public DynSetTree <typename GT::Arc *, Tree, Compare>
{
  typedef typename GT::Node GT_Node;
  typedef typename GT::Node_Type GT_Node_Info;
  typedef typename GT::Arc GT_Arc;
  typedef typename GT::Arc_Type GT_Arc_Info;
  typedef DynSetTree <typename GT::Arc *, Tree, Compare> Tree_Type;

  GT & g;
  SA & sa;

  void init()
  {
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_current(); it.next())
      this->insert(it.get_current());
  }

public:
  Arcs_Index(GT & _g, Compare & cmp, SA & _sa)
    : Tree_Type(cmp), g(_g), sa(_sa)
  {
    init();
  }

  Arcs_Index(GT & _g, Compare && cmp = Compare(), SA && _sa = SA())
    : Tree_Type(cmp), g(_g), sa(_sa)
  {
    init();
  }

      /** Crea un nuevo arco entre dos nodos, lo inserta en el grafo y
	  luego en el índice.

	  @param[in] src nodo origen.
	  @param[in] tgt nodo destino.
	  @param[in] info información a copiarse como contenido del
	  nodo. Por omisión se copia el contenido que arroje el
	  constructor GT::Arc_Type().
	  @return puntero al arco creado.
	  @throw bad_alloc si no hay suficiente memoria.
       */
  GT_Arc * insert_in_graph(GT_Node * src, GT_Node * tgt,
                           const GT_Arc_Info & info)
  {
    GT_Arc * a = g.insert_arc(src, tgt, info);
    
    if (this->insert(a) == nullptr)
      {
        g.remove_arc(a);
        return nullptr;
      }

    return a;
  }

        /// \overload  insert_in_graph
  GT_Arc * insert_in_graph(GT_Node * src, GT_Node * tgt,
                           const GT_Arc_Info && info = GT_Arc_Info())
  {
    return insert_in_graph(src, tgt, info);
  }

      /** Busca un arco que conecte a dos nodos.

	  @param[in] src nodo origen.
	  @param[in] tgt nodo destino.
          @param[in] info información asociada al arco.
	  @return puntero al arco si éste existe o nullptr de lo contrario.
      */
  GT_Arc * search(GT_Node * src, GT_Node * tgt, const GT_Arc_Info & info)
  {
    GT_Arc arc(info);
    arc.src_node = src; arc.tgt_node = tgt;
    GT_Arc * ptr_arc = &arc;
    
    GT_Arc ** ret_val = Tree_Type::search(ptr_arc);

    if (ret_val != nullptr)
      return *ret_val;
 
    if (g.is_digraph())
      return nullptr;

    std::swap(arc.src_node, arc.tgt_node);
    ret_val = Tree_Type::search(ptr_arc);
    if (ret_val == nullptr)
      return nullptr;

    assert(((src == (*ret_val)->src_node) and (tgt == (*ret_val)->tgt_node)) or 
      ((tgt == (*ret_val)->src_node) and (src == (*ret_val)->tgt_node)));

    return *ret_val;
  }

        /// \overload  search
  GT_Arc * search(GT_Node * src, GT_Node * tgt,
                  const GT_Arc_Info && info = GT_Arc_Info())
  {
    return search(src, tgt, info);
  }

      /** Elimina el arco a del grafo y del ìndice.

	  @param[in] a puntero al arco a eliminar
	  @throw domain_error si el arco no està insertado en el índice (lo
	  que posiblemente signifique que tampoco en el grafo).
   */
  void remove_from_graph(GT_Arc * a)  throw(std::exception, std::domain_error)
  {
    Tree_Type::find(a);
    Tree_Type::remove(a);
    g.remove_arc(a);
  }

};

} // End namespace Aleph

# endif // GRAPH_INDEXES_H

