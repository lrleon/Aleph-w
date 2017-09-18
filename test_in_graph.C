# include <tpl_indexGraph.H>


    template <class Tn>
struct My_Node : public Graph_Node<Tn>
{
  My_Node(const Tn& tn) : Graph_Node<Tn>(tn) { }

  My_Node() { }
};


    template <class Ta>
struct My_Arc : public Graph_Arc<Ta>
{
  My_Arc(const Ta& ta) : Graph_Arc<Ta>(ta) { }

  My_Arc(void * src, void * tgt, const Ta& ta) : Graph_Arc<Ta>(src, tgt, ta) { }

  My_Arc() { }
};


struct Ciudad
{
  string nombre;

  Ciudad() {}

  Ciudad(const string & n) : nombre(n) {}

  bool operator < (const Ciudad & c) const
  {
    return nombre < c.nombre;
  }
};

struct Via
{
  string nombre;

  Via() {}

  Via(const string & n) : nombre(n) {}

};

typedef My_Node<Ciudad> Nodo_Ciudad;

typedef My_Arc<Via> Arco_Via;


int main()
{

  List_Graph<Nodo_Ciudad, Arco_Via> g; 
  
  Index_Graph<List_Graph<Nodo_Ciudad, Arco_Via> > idx(g);

  Nodo_Ciudad * n1 = idx.insert_node(Ciudad("Merida"));
  Nodo_Ciudad * n2 = idx.insert_node(Ciudad("Valera"));
  Nodo_Ciudad * n3 = idx.insert_node(Ciudad("Caracas"));
  Nodo_Ciudad * n4 = idx.insert_node(Ciudad("Trujillo"));
  Nodo_Ciudad * n5 = idx.insert_node(Ciudad("Barinas"));

  assert(idx.search_node(Ciudad("Merida")) == n1);

  Via v("Prueba");

  cout << "Hay " << idx.get_num_arcs() << endl;

  DynArray<List_Graph<Nodo_Ciudad, Arco_Via>::Arc*> arcos;
  arcos.append(idx.insert_arc(n1, n2, v));
  arcos.append(idx.insert_arc(n1, n3, v));
  arcos.append(idx.insert_arc(n1, n4, v));
  arcos.append(idx.insert_arc(n1, n5, v));
  arcos.append(idx.insert_arc(n2, n3, v));
  arcos.append(idx.insert_arc(n2, n4, v));
  arcos.append(idx.insert_arc(n2, n5, v));
  arcos.append(idx.insert_arc(n3, n4, v));
  arcos.append(idx.insert_arc(n3, n5, v));
  arcos.append(idx.insert_arc(n4, n5, v));

  assert(idx.search_arc(n5, n4) != NULL);

  for (int i = 0; i < arcos.size(); i++)
    idx.remove_arc(arcos.access(i));

  idx.remove_node(n1);

}
