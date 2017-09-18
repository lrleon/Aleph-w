
# include <random_graph.H>
# include <tpl_agent_graph.H>
# include <tpl_indexNode.H>

size_t num_agents = 10;

size_t num_mutex = 14;

size_t num_threads = 7;

int node_num = 0;


struct Test_Node : public Agent_Node<int>
{
  unsigned long long * count; // un contador por agente

  unsigned long long num_rem; // cantidad de veces que se un agente quedó en nodo
  unsigned long long num_leaves; // cantidad de veces que se un agente quedó en arco

  Test_Node() : num_rem(0), num_leaves(0)
  {
    get_info() = node_num++;
    count =  new unsigned long long [num_agents];
    for (int i = 0; i < num_agents; ++i)
      count[i] = 0;
  }

  Test_Node(const int & i) 
    : Agent_Node<int>(i), count(NULL), num_rem(0), num_leaves(0)
  {
    // empty
  }

  Test_Node(Test_Node *) 
    : Agent_Node<int>(-1), count(NULL), num_rem(0), num_leaves(0) 
  {
    // Empty
  }

  ~Test_Node() 
  {
    delete [] count;
  }

  struct CMP;
};


struct Test_Arc : public Agent_Arc<Empty_Class>
{
  unsigned long long * count; // un contado por agente

  unsigned long long num_rem;
  unsigned long long num_leaves;
  

  Test_Arc() : num_rem(0), num_leaves(0)
  {
    count =  new unsigned long long [num_agents];
    for (int i = 0; i < num_agents; ++i)
      count[i] = 0;
  }

  Test_Arc(const Empty_Class&) : count(NULL), num_rem(0), num_leaves(0)
  {
    // empty
  }

  Test_Arc(Test_Arc *) : count(NULL), num_rem(0), num_leaves(0)
  {
    // empty
  }

  ~Test_Arc() 
  {
    delete [] count;
  }
};

int agt_count = 0;

struct Test_Agent : public Walking_Agent<int>
{
  unsigned long node_counter; // cantidad nodos visitados
  unsigned long arc_counter;  // catidad de arcos visitados

  unsigned long long num_rem; // cantidad de veces que agente quedó en nodo
  unsigned long long num_leaves; // cantidad de veces que agente quedó en arco

  static const unsigned long long max_it = 10000000;

  unsigned long long num_it;

  Test_Agent() 
    : Walking_Agent<int>(agt_count++), node_counter(0), arc_counter(0),
      num_rem(0), num_leaves(0), num_it(max_it)
  { /* empty */ }
};


typedef Agent_Graph<List_Graph, Test_Node, Test_Arc, Test_Agent> Grafo;


gsl_rng * gen = NULL;



long test_transit_node(void * __agent, void * __graph, void * __src, void *& __arc)
{
  Grafo::Agent * agent = (Grafo::Agent * ) __agent;
  
  if (agent->num_it-- == 0)
    return Grafo::Agent::Dead;

  //  cout << agent->num_it << " ";

  Grafo::Node * src    = (Grafo::Node*) __src;

  src->count[agent->get_info()]++;

  static const float p = 0.8; // prob de que agent se quede en nodo
  const double r = gsl_rng_uniform(gen);

      // sortea si pasa hacia arco con probabilidad p
  if (r < p) // ¿quedarse en nodo?
    { // sí ==> actualizar contadores de permanencias
      agent->num_rem++;
      src->num_rem++;

      __arc = NULL;

      return Grafo::Agent::Remain_In_Node;
    }
  else
    { // No ==> seleccionar arco al azar y entrar a él
      Grafo * g = (Grafo * ) __graph;
      const int i = gsl_rng_uniform_int(gen, g->get_num_arcs(src));

      Grafo::Node_Arc_Iterator it(src);
      for (int k = 0; k < i; it.next(), ++k) /* Nada más */ ;
	
      __arc = it.get_current();

      src->num_leaves++;
      agent->arc_counter++;

      return Grafo::Agent::Enter_In_Arc;
    }
}


long test_transit_arc(void * __agent, void * /* __graph */, void * __arc)
{
  Grafo::Arc * arc     = (Grafo::Arc *) __arc;
  Grafo::Agent * agent = (Grafo::Agent * ) __agent;

  arc->count[agent->get_info()]++;

  static const float p = 0.8; // prob de que agent tome nodo
  const double r = gsl_rng_uniform(gen);

      // sortea si pasa hacia arco con probabilidad p
  if (r < p) // ¿quedarse en arco?
    { // sí ==> actualizar contadores de permanencias
      agent->num_rem++;
      arc->num_rem++;

      return Grafo::Agent::Remain_In_Arc;
    }
  else
    { // No ==> ir hacia nodo destino
      agent->num_leaves++;
      arc->num_leaves++;

      return Grafo::Agent::Enter_In_Node;
    }
}

void print_graph(Grafo & g)
{
  for (Grafo::Node_Iterator it(g); it.has_current(); it.next())
    {
      Grafo::Node * p = it.get_current();
      cout << "Node: " << p->get_info() << " " << p->num_rem << " " << p->num_leaves
	   << ": ";
      for (int i = 0; i < num_agents; ++i)
	cout << " " << p->count[i];
      cout << endl;
    }

  for (Grafo::Arc_Iterator it(g); it.has_current(); it.next())
    {
      Grafo::Arc * a = it.get_current();
      Grafo::Node * src = g.get_src_node(a);
      Grafo::Node * tgt = g.get_tgt_node(a);
      cout << src->get_info() << "--" << tgt->get_info() << ": " << a->num_rem 
	   << " " << a->num_leaves;
      for (int i = 0; i < num_agents; ++i)
	cout << " " << a->count[i];
      cout << endl;
    }

  for  (Grafo::Agent_Iterator it(g); it.has_current(); it.next())
    {
      Test_Agent * agent = it.get_current();

      cout << agent->get_info() << ": " << agent->node_counter << " " 
	   << agent->arc_counter << " " << agent->num_rem << " " << agent->num_leaves
	   << endl;
    }
}


void * update_graph(void * ptr)
{
  cout << "callback" << endl; return NULL;

  Grafo * g = (Grafo*) ptr;
  print_graph(*g);
  return NULL;
}

Grafo * crear_grafo(const size_t & num_nodes, const double & p)
{
  gsl_rng * r = gsl_rng_alloc (gsl_rng_mt19937);
  gsl_rng_set(r, time(NULL) % gsl_rng_max(r));

  gen = gsl_rng_alloc (gsl_rng_mt19937);
  gsl_rng_set(gen, time(NULL) % gsl_rng_max(r));

  Grafo * ret = Random_Graph <Grafo> () (num_nodes, p);

  ret->set_time_callback(&update_graph, 30);

  IndexNode<Grafo> idx(*ret);

  ret->set_num_threads(num_threads);
  ret->set_num_mutexes(num_mutex);

  ret->distribute_mutexes_randomly();

  for (int i = 0; i < num_agents; ++i)
    {
      Test_Agent * agt = new Test_Agent;

      agt->set_process_node(&test_transit_node);
      agt->set_process_arc(&test_transit_arc);

      int node_num = gsl_rng_uniform_int(r, num_nodes);

      Grafo::Node * p = idx.search(node_num);

      ret->insert_agent_in_node(agt, p);
    }

  gsl_rng_free(r);

  return ret;
}



int main()
{
  Grafo * g = crear_grafo(500, 0.1);

  cout << "Arrancando grafo; main bloqueado" << endl;

  g->start_graph(true);

  g->suspend_graph();

  cout << "main ha reanudado" << endl;

  print_graph(*g);

  delete g;

  gsl_rng_free(gen);

  cout << "Terminado" << endl << endl;
}
