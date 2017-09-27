# ifndef ARCHEAP_H
# define ARCHEAP_H

# include <tpl_binHeap.H>
# include <tpl_graph_utils.H>

    template <class GT, 
	      class Distance, 
	      class Access_Heap_Node>
class ArcHeap 
 : public BinHeap<typename GT::Arc*, Distance_Compare<GT, Distance>>
{
  Distance                       dist;
  Access_Heap_Node &             access_node;
  Distance_Compare<GT, Distance> dist_cmp;

public:

  Distance & get_distance() { return dist; }

  ArcHeap(Distance __dist = Distance(),
	  Access_Heap_Node && acc = Access_Heap_Node())
    : dist(__dist), access_node(acc), dist_cmp(dist)
  {
    // empty
  }

      typedef 
  BinHeap<typename GT::Arc*, Distance_Compare<GT, Distance> > Heap;

  typedef typename Heap::Node Node;

  void put_arc(typename GT::Arc * arc, typename GT::Node * tgt)
  {
    Node *& heap_node = access_node(tgt);

    if (heap_node == nullptr) // ¿existe arco insertado en el heap?
      {     // No ==> crear nuevo nodo para el heap y asignarle arco
	assert(heap_node == nullptr); // para ser consistente

	heap_node = new Node;
	heap_node->get_key() = arc;
	this->insert(heap_node);

	return;
      }

        // dos arcos con igual destino ==> tomar menor; descartar mayor
    typename GT::Arc *& arc_in_heap = heap_node->get_key();

        // ¿arc_in_heap tiene distancia menor que arc?
    if (dist_cmp(arc_in_heap, arc)) 
      return; // antiguo arco permanece en el heap; nuevo se ignora

        // arc_in_heap será el arco recién insertado arc
    arc_in_heap = arc; // cambia el arco
    this->update(heap_node); // actualiza el heap con el nuevo peso de arc
  }

  typename GT::Arc * get_min_arc()  
  {
    Node * heap_node = this->getMin();
    typename GT::Arc * arc = heap_node->get_key();

        // seleccionar nodo que retorne la clase de acceso
    typename GT::Node * p = (typename GT::Node*) arc->src_node;
    if (access_node(p) != heap_node)
      p = (typename GT::Node*) arc->tgt_node;

    assert(access_node(p) == heap_node); 

    access_node(p) = nullptr; 

    delete heap_node;

    return arc;
  }

  void empty()
  {
    this->remove_all_and_delete();
  }

  ~ArcHeap() 
  {
    empty();
  }  
};

# endif // ARCHEAP_H

