# include <Dijkstra.H>
# include <random_graph.H>
# include <point.H>

using namespace Aleph;

unsigned int seed;


struct Nodo
{
  int x, y;
  int num;
};

typedef List_Graph<Graph_Node<Nodo>, Graph_Arc<int> > Grafo;

struct Inicia_Punto
{
  gsl_rng * r;
  unsigned long W;
  unsigned long H;

  int count;

  Grafo::Node * Max;
  Grafo::Node * Min;
  const Point zero;
  const Point last;

  Inicia_Punto(unsigned long w, unsigned long h)
    : W(w), H(h), count(0), Max(NULL), Min(NULL), zero(0, 0), last(H,W)
  {
    r = gsl_rng_alloc (gsl_rng_mt19937);
    gsl_rng_set(r, seed % gsl_rng_max(r));
  }

  Inicia_Punto(const Inicia_Punto & i) 
    : r(i.r), W(i.W), H(i.H), count(i.count)
  {
    const_cast<Inicia_Punto&>(i).r = NULL;
  }

  ~Inicia_Punto()
  {
    if (r != NULL)
       gsl_rng_free(r);
    r = NULL;
  }

  void operator () (Grafo & g, Grafo::Node * p)
  {
    int x = gsl_rng_uniform_int(r, W);
    int y = gsl_rng_uniform_int(r, H);
    
    Nodo & info = p->get_info();

    info.x = x;
    info.y = y;
    info.num = count++;

    if (Min == NULL)
      Min = p;

    if (Max == NULL)
      Max = p;

    Point point(x,y);

    Point pmin(Min->get_info().x, Min->get_info().y);
    if (point.distance_with(zero) < pmin.distance_with(zero))
      Min = p;

    Point pmax(Max->get_info().x, Max->get_info().y);
    if (point.distance_with(last) < pmax.distance_with(last))
      Max = p;
  }
};


struct Inicia_Arco
{
  gsl_rng * r;
  unsigned long offset;

  Inicia_Arco(int off) : offset(off)
  {
    r = gsl_rng_alloc (gsl_rng_mt19937);
    gsl_rng_set(r, seed % gsl_rng_max(r));
  }

  Inicia_Arco(const Inicia_Arco & a) 
    : r(a.r), offset(a.offset)
  {
    const_cast<Inicia_Arco&>(a).r = NULL;
  }

  ~Inicia_Arco()
  {
    if (r != NULL)
      gsl_rng_free(r);
    r = NULL;
  }

  void operator () (Grafo & g, Grafo::Arc * a) const
  {
    Grafo::Node * src = g.get_src_node(a);
    Grafo::Node * tgt = g.get_tgt_node(a);

      // distancia euclidiana entre los nodos
    Point psrc(src->get_info().x, src->get_info().y);
    Point ptgt(tgt->get_info().x, tgt->get_info().y);
    Geom_Number __d = psrc.distance_with(ptgt);
    float d = geom_number_to_double(__d);

    unsigned long rand_offset = gsl_rng_uniform_int(r, offset);

    a->get_info() = d + rand_offset;
  }
};

struct Dist_Arco
{
  typedef float Distance_Type;

  static const Distance_Type Zero_Distance;

  static const Distance_Type Max_Distance;

  Distance_Type operator () (Grafo::Arc * a)
  {
    return a->get_info();
  }
};

struct Aplus
{
  Grafo & g;
  float x, y;
  float p2;

  float len(const float tx, const float ty)
  {
    float diff = tx - ty;
    return sqrt(p2 + diff*diff);
  }

      Dist_Arco::Distance_Type 
  operator () (Grafo::Arc * arc,
	       const Dist_Arco::Distance_Type & op1,
	       const Dist_Arco::Distance_Type & op2)
  {
    Grafo::Node * tgt = g.get_tgt_node(arc);
    if (IS_NODE_VISITED(tgt, Aleph::Dijkstra))
      tgt = g.get_src_node(arc);

    Nodo & info = tgt->get_info();

    float H = len(info.x, info.y);

    return op1 + op2 + 2*H;
  }

      Dist_Arco::Distance_Type 
  operator () (const Dist_Arco::Distance_Type & op1,
	       const Dist_Arco::Distance_Type & op2)
  {
    return op1 + op2;
  }

  Aplus(Grafo & __g, Grafo::Node * end) 
    : g(__g)
  {
    Nodo & info = end->get_info();

    x = info.x;
    y = info.y;
    
    float diff = x - y;
    p2 = diff*diff;
  }
};


const float Dist_Arco::Zero_Distance = 0;
const float Dist_Arco::Max_Distance  = INT_MAX;

Grafo * generar_grafo(size_t num_nodes, size_t num_arcs,
		      long double w, long double h, long double offset,
		      Grafo::Node *& smallest, Grafo::Node *& biggest)
{
  Inicia_Punto plano(w, h);
  Inicia_Arco iarc(offset);
  Grafo * g = Random_Graph <Grafo, Inicia_Punto, Inicia_Arco> 
    (seed, plano, iarc) (num_nodes, num_arcs);

  smallest = plano.Min;
  biggest  = plano.Max;
  
  return g;
}

void imprimir_camino(Path<Grafo> & p)
{
  float suma = 0;
  cout << "El camino es: ";
  for (Path<Grafo>::Iterator it(p); it.has_curr(); it.next())
    {
      Nodo & info = it.get_current_node()->get_info();
      cout << "(" << info.x << "," << info.y << ")";

      if (it.has_current_arc())
	{
	  int & w = it.get_current_arc()->get_info();
	  suma += w;
	  cout << " " << w << " ";
	}
    }
  int total = suma;
  cout << endl
       << "Son " << p.size() << " nodos" << endl
       << "dist = " << total << endl
       << endl;
}


int main(int argn, char ** argc)
{
  size_t num_nodes = argc[1] ? atoi(argc[1]) : 1000;
  size_t num_arcs = 4*num_nodes;
  if (argn > 2)
    num_arcs = atoi(argc[2]);

  seed = time(0);
  if (argn > 3)
    seed = atoi(argc[3]);

  cout << argc[0] << " " << num_nodes << " " << num_arcs << " " << seed << endl;

  cout << "Calculando grafo aleatorio de " << num_nodes << " nodos y "
       << num_arcs << " arcos" << endl;
  Grafo::Node * s, *t;
  Grafo * g = 
    generar_grafo(num_nodes, num_arcs, 1000000, 1000000, 100, s, t);
  cout << "Listo!" << endl;  

  Path<Grafo> p(*g);
  
  {
    cout << "Buscando camino desde (" << s->get_info().x << "," 
	 << s->get_info().y << ") hacia ("
	 << t->get_info().x << "," << s->get_info().y << ")" << endl;

    Aplus aplus(*g, t);

    (Dijkstra_Min_Paths<Grafo, Dist_Arco, Aplus> (aplus)) (*g, s, t, p);
    
    imprimir_camino(p);
  }

  {
    Grafo tree;
    Dijkstra_Min_Paths<Grafo, Dist_Arco> dij;
    dij.compute_partial_min_paths_tree(*g, s, t, tree);
    dij.get_min_path(tree, t, p);
    
    imprimir_camino(p);

    cout << "Árbol sin Astar tiene " << tree.get_num_nodes() << " nodos y "
	 << tree.get_num_arcs() << " arcos" << endl;
  }  

  {
    Aplus aplus(*g, t);
    Grafo tree;
    Dijkstra_Min_Paths<Grafo, Dist_Arco, Aplus> dij(aplus);
    dij.compute_partial_min_paths_tree(*g, s, t, tree);
    dij.get_min_path(tree, t, p);
    
    imprimir_camino(p);

    cout << "Árbol con Astar tiene " << tree.get_num_nodes() << " nodos y "
	 << tree.get_num_arcs() << " arcos" << endl;
  }  

  delete g;
}
