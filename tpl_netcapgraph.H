# ifndef TPL_NETCAPGRAPH_H
# define TPL_NETCAPGRAPH_H
# include <tpl_netgraph.H>

namespace Aleph {

    /** Nodo con capacidad.

        Un nodo con valor de capacidad especifica un máximo valor de
        flujo que el nodo puede tener como flujo de entrada y salida.

        @ingroup Grafos Redes
     */
    template <typename Node_Info, typename F_Type = double>
class Net_Cap_Node : public Net_Node<Node_Info, F_Type>
{
public:
         /// Tipo que representa el flujo 
  typedef F_Type Flow_Type;

  Flow_Type max_cap; 

  Net_Cap_Node() : max_cap(numeric_limits<Flow_Type>::max()) {}

      /// Constructor que recibe información de nodo a guardar. Los valores
      /// de atributos de capacidad y flujo son iniciados en cero.
  Net_Cap_Node(const Node_Info & node_info) 
    : Net_Node<Node_Info, F_Type>(node_info), 
      max_cap(numeric_limits<Flow_Type>::max())
  { /* empty */ }

       /// Constructor copia de nodo. Los valores
       /// de atributos de capacidad y flujo son iniciados en cero.
  Net_Cap_Node(Net_Cap_Node * node) 
    : Net_Node<Node_Info, F_Type>(*node), max_cap(node->max_cap) { /* empty */ }
}; 

  /** Red capacitada con límites de capacidad en nodos.

      Net_Cap_Graph define una red capacitada en la cual los nodos
      tienen límite de flujo entrante o saliente.

      La clase recibe dos parámetros tipo:
        -# NodeT: el tipo de nodo de la red, el cual debe ser
        descendiente de la clase Net_Node.
        -# ArcT: el tipo de arco de la red, el cual debe ser
        descendiente de la clase Net_Arc.

      Una red definida mediante Net_Cap_Graph no es usable por los
      algoritmos de maximización de flujo. En su lugar, puede obtenerse
      una red de tipo Net_Graph equivalente. La red equivalente es de
      tipo Net_Cap_Graph::Aux_Net, el cual fundamentalmente es Net_Graph
      sin capacidades en los nodos.

      La red equivalente puede calcularse mediante compute_aux_net().

      @see Net_Graph compute_aux_net()

      @ingroup Redes
   */
    template <class NodeT, class ArcT>
class Net_Cap_Graph : public Net_Graph<NodeT, ArcT>
{
public:
      /// La clase de red tradicional
  typedef Net_Graph<NodeT, ArcT> Net_Class;
      /// Tipo de arco.
  typedef ArcT Arc;
      /// Tipo de node.
  typedef NodeT Node;
      /// Tipo que representa la capacidad y el flujo
  typedef typename Arc::Flow_Type Flow_Type;
      /// Tipo de atributo que almacena un nodo
  typedef typename Node::Node_Type Node_Type;
      /// Tipo de atributo que almacena un arco
  typedef typename Arc::Arc_Type Arc_Type;

    /** Crea un nuevo nodo capacitado y lo inserta en la red.

        @param[in] node_info la información a guardar en el nodo según
        como ésta se haya definido al momento de especificación del nodo.
        
        @param[in] cap valor de flujo máximo que puede recibir o dejar el
        nodo. Por omisión este valor está fijado al valor máximo.
        @return un puntero al nodo creado e insertado en la red.
        @throw bad_alloc si no hay suficiente memoria.
     */
  Node * insert_node(const Node_Type & node_info, 
                     const Flow_Type & cap = numeric_limits<Flow_Type>::max())
  {
    Node * p = Net_Class::insert_node(node_info);
    p->max_cap = cap;
    return p;
  }
    /** El tipo de red tradicional equivalente a la red con capacidades en
        los nodos. 

        Net_Graph::Aux_Net representa el tipo de red capacitada
        equivalente a la red con capacidades en los nodos
        (Net_Cap_Graph). 

        Para obtener la red equivalente se invoca a compute_aux_net(). La red
        resultado está mapeada tanto en arcos como nodos. 

        Un nodo en Net_Cap_Graph es mapeado a un arco con valor de
        capacidad del nodo. Dado un nodo p en Net_Cap_Graph, el arco en
        Aux_Net se obtiene a través de NODE_COOKIE(p). 

        Un arco en Net_Cap_Graph es mapeado a un arco con el mismo valor
        de capacidad. Un arco en Aux_Net tiene un solo atributo lógico. Si
        el valor en true, entonces el arco mapea a un nodo, cuyo valor de
        capacidad es el mismo que la capacidad del nodo. De lo contrario,
        el arco mapea a otro arco.
    */
    typedef Net_Graph<Net_Node<Empty_Class, Flow_Type>, 
                      Net_Arc<bool, Flow_Type> > Aux_Net;
  private:
    Aux_Net * aux_net;
  public:
    Net_Cap_Graph() : aux_net(nullptr) { /* empty */ }
      /// Retorna un apuntador a la red equivalente a this. Previamente
      /// debe haberse creado mediante 
  Aux_Net * get_aux_net() { return aux_net; }
      /** Calcula la red tradicional equivalente a la con capacidades en
          los nodos.

          Dada una red con capacidades en los nodos (representada en this),
          compute_aux_net() calcula la red tradicional equivalente sobre
          la cual se pueden realizar cálculos de maximización de flujo,
          corte equivalente, etc.

          @return un puntero a la red equivalente.
          @exception bad_alloc si no hay suficiente memoria para crear la
          red equivalente.
          @exception domain_error si la red equivalente ya ha sido calculada.
       */
  Aux_Net * compute_aux_net()
  {
    if (aux_net != nullptr)
      throw std::domain_error("aux_net has already computed");
    aux_net = new Aux_Net;
    try
      {
        for (Node_Iterator<Net_Cap_Graph> it(*this); it.has_current(); it.next())
          {
            Node * p = it.get_current();
            typename Aux_Net::Node * src = aux_net->insert_node();
            typename Aux_Net::Node * tgt = aux_net->insert_node();
            typename Aux_Net::Arc  * arc = 
              aux_net->insert_arc(src, tgt, true, p->max_cap, 0);
            NODE_COOKIE(p)  = arc;
            ARC_COOKIE(arc) = p;
          }
        for (Arc_Iterator<Net_Cap_Graph> it(*this); it.has_current(); it.next())
          {
            Arc * arc = it.get_current();
            typename Aux_Net::Arc * src_arc = 
              (typename Aux_Net::Arc *) NODE_COOKIE(get_src_node(arc));
            typename Aux_Net::Arc * tgt_arc = 
              (typename Aux_Net::Arc *) NODE_COOKIE(get_tgt_node(arc));
            typename Aux_Net::Arc * a = 
              aux_net->insert_arc(aux_net->get_tgt_node(src_arc), 
                                  aux_net->get_src_node(tgt_arc), false, 
                                  arc->cap, arc->flow);
            ARC_COOKIE(arc) = a;
            ARC_COOKIE(a)   = arc;
          }
        return aux_net;
      }
    catch (...)
      {
        free_aux_net();
      }
  }
    /** Actualiza los valores de flujo de la red equivalente a this.

       update() recorre la red capacitad equivalente, previamente
       calculada mediante compute_aux_net(), y actualiza los valores de
       flujo sobre la red this.

       update() debe ser invocado antes de liberar la red equivalente, a
       efectos de reflejar sobre this los cálculos realizados sobre la red
       equivalente. Por ejemplo, si se desea maximizar el flujo de this,
       entonces se maximiza el flujo de la red equivalente y luego los
       valores se actualizan sobre this mediante update().

       @exception domain_error si la red equivalente no ha sido generada.
     */
  void update()
  {
    if (aux_net == nullptr)
      throw std::domain_error("Auxiliar net has not been generated");

    for (Arc_Iterator<Aux_Net, No_Res_Arc<Aux_Net> > it(*aux_net); 
         it.has_current(); it.next())
      {
        typename Aux_Net::Arc * arc = it.get_current();
        if (arc->get_info())
          {
            Node * p = (Node*) ARC_COOKIE(arc);
            p->in_flow = p->out_flow = arc->flow;
          }
        else
          {
            Arc * a = (Arc*) ARC_COOKIE(arc);
            a->flow = arc->flow;
          }
      }
  }
      /// Libera la memoria de la red equivalente. Dispara excepción
      /// domain_error si la red equivalente no ha sido creada.
  void free_aux_net()
  {
    if (aux_net == nullptr)
      throw std::domain_error("Auxiliar net has not been generated");

    clear_graph(*aux_net);
    delete aux_net;
    aux_net = nullptr;
  }

  ~Net_Cap_Graph()
  {
    if (aux_net != nullptr)
      free_aux_net();
  }
};

} // end namespace Aleph

# endif // endif
