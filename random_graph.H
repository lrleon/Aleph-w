
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
# ifndef RANDOM_GRAPH_H
# define RANDOM_GRAPH_H

# include <gsl/gsl_rng.h> 
# include <memory>
# include <tpl_indexArc.H>
# include <tpl_graph_utils.H>
# include <tpl_components.H>
# include <single_graph.H>
# include <Tarjan.H>

namespace Aleph 
{

    template <class GT>
struct Dft_Init_Rand_Node
{
  void operator () (GT &, typename GT::Node *) const noexcept
  {
    // empty
  }
};


    template <class GT>
struct Dft_Init_Rand_Arc
{
  void operator () (GT &, typename GT::Arc *) const noexcept
  {
    // empty
  }
};

  // TODO: eliminar clases Init_Node e Init_Arc y en su lugar usar lambdas

  template <class GT, class Init_Node, class Init_Arc>
class Random_Graph_Base
{
protected:

  typedef typename GT::Node GT_Node;
  typedef typename GT::Arc GT_Arc;

  gsl_rng * r;

  Init_Node & init_node;
  Init_Arc &  init_arc;

  unique_ptr<DynArray<GT_Node*>> nodes; // puntero para ahorrar
				        // directorio en caso de que no 
                                        // se utilice

  unique_ptr<IndexArc<GT>> idx_arc; // puntero porque el constructor
				    // requiere el grafo.
  mutable size_t num_nodes;
  mutable size_t num_arcs;
  mutable unsigned long rand_max; 

  GT g;

  bool save_parity; // indica si se debe o no guardar relaciones de
		    // paridad entre los nodos. Sólo para construir
		    // eulerianos y hamiltonianos

       virtual void 
  update_parity_after_arc_insertion(GT_Node * src, GT_Node * tgt) = 0;

  GT_Arc * insert_arc(GT_Node * src, GT_Node * tgt)
  {
    auto a = idx_arc->insert(g.insert_arc(src, tgt));
    init_arc (g, a);
    update_parity_after_arc_insertion(src, tgt);
    return a;
  }

      // selecciona un nodo azar distinto a excluded
  GT_Node * select_random_node(GT_Node * excluded = nullptr) noexcept
  {
    assert(nodes.get() != nullptr);

    unsigned long idx;
    GT_Node * ret_val = nullptr;
    while (true)
      {
	idx = gsl_rng_uniform_int(r, num_nodes);
	ret_val = nodes->access(idx);
	if (excluded == nullptr or ret_val != excluded)
	  break;
      }

    return ret_val;
  }

      // selecciona un nodo azar entre los contenidos en la lista list
  GT_Node * select_random_node(DynList<GT_Node*> & list) noexcept
  {
    const unsigned long k = gsl_rng_uniform_int(r, list.size());
    typename DynList<GT_Node*>::Iterator it(list);
    for (int i = 0; i < k; ++i, it.next_ne()) ;

    return it.get_curr_ne();
  }

  virtual void create_nodes_and_initialize_arc_index() = 0;

  virtual void connect() = 0;

  void initialize_and_create_nodes(const size_t & __num_nodes, 
				   const size_t & __num_arcs)
  {
    num_nodes = __num_nodes;

    const size_t num_nodes_2 = num_nodes*num_nodes;
    if (g.is_digraph())
      num_arcs = std::min(__num_arcs, num_nodes_2 - num_nodes);
    else
      num_arcs = std::min(__num_arcs, (num_nodes_2 - num_nodes)/2);
    
    create_nodes_and_initialize_arc_index();
  }

  Random_Graph_Base(unsigned long seed,
		    const Init_Node & __init_node,
		    const Init_Arc  & __init_arc) 
    : r(gsl_rng_alloc (gsl_rng_mt19937)), 
      init_node(const_cast<Init_Node&>(__init_node)), 
      init_arc(const_cast<Init_Arc&>(__init_arc)),
      num_nodes(0), num_arcs(0),
      rand_max(gsl_rng_max(r)), save_parity(false) 
  {
    if (r == nullptr)
      throw std::bad_alloc();

    gsl_rng_set(r, seed % rand_max);
  }

  ~Random_Graph_Base()
  {
    if (r != nullptr)
      gsl_rng_free(r);
  }

       // Crea un grafo aleatorio esparcido.
  GT create(const size_t & __num_nodes, const size_t & __num_arcs, 
	    bool connected)
  {
    initialize_and_create_nodes(__num_nodes, __num_arcs);

      // inserta al azar los arcos seleccionando al azar pares de nodos
    for (int i = 0; i < num_arcs; ++i)
      {
	auto src = select_random_node();
	auto tgt = select_random_node(src);
	if (idx_arc->search(src, tgt) == nullptr) // se repite el arco?
	  insert_arc(src, tgt);
      }

    if (connected)
      connect();
    
    return move(g);
  }

  virtual GT create_p(const size_t & __num_nodes, const double & p, 
		      bool connected) = 0;

  virtual void make_eulerian() = 0;

  virtual void make_hamiltonian () = 0;

public:

      /** Crea un grafo aleatorio que es euleriano.

	  Esta versión construye un grafo aleatorio esparcido que se
	  garantiza es euleriano; es decir, que contiene ciclos
	  eulerianos.

	  El proceso primero genera un grafo esparcido
	  aleatorio, luego examina el grafo resultante y crea nuevos
	  arcos de modo que el resultado contenga un ciclo euleriano.

	  @param[in] __num_nodes cantidad de nodos que debe tener el grafo.
	  @param[in] __num_arcs cantidad de arcos  que debe tener el grafo.
	  @return puntero al grafo aleatorio creado.
	  @throw bad_alloc si no hay suficiente memoria.

       */
  GT eulerian(const size_t & __num_nodes, const size_t & __num_arcs) 
  {
    save_parity = true;
    g = this->create(__num_nodes, __num_arcs, true);
    make_eulerian();

    return std::move(g);
  }

      /** Crea un grafo aleatorio euleriano.

	  Esta versión construye un grafo aleatorio euleriano con
	  probabilidad p de existencia de arco entre cualquier par de
	  nodos. 

	  Úsese esta rutina para generar grafos densos. En este caso,
	  empléese un valor de p mayor o igual a 0.5.

	  @warning El procedimiento puede ser extremadamente lento y goloso
	  en memoria conforme aumento el valor de __num_nodes.

	  @param[in] __num_nodes cantidad de nodos que debe tener el grafo.
	  @param[in] p probabilidad de existencia de arco entre cualquier
	  par de nodos.
	  @return puntero al grafo aleatorio creado.
	  @throw bad_alloc si no hay suficiente memoria.
	  @throw domain_error si el valor de p no está entre 0 y 1.
      */
  GT eulerian (const size_t & __num_nodes, const double & p) 
  {
    save_parity = true;
    g = this->create_p(__num_nodes, p, true);
    make_eulerian();

    return std::move(g);
  }

  /** Crea un grafo aleatorio hamiltoniano.

      Esta versión construye un grafo aleatorio hamiltoniano.

      El proceso primero genera un grafo aleatorio, luego examina el
      grafo resultante y crea nuevos 
      arcos de modo que el resultado contenga un ciclo hamiltoniano.

      @warning El procedimiento puede ser extremadamente lento y goloso
      en memoria conforme aumente el valor de __num_nodes.

      @warning El grafo se asegura a ser hamiltoniano por los teoremas de
      Ore y Dirac. En este sentido, nótese que que la condición de
      hamiltoniano es de suficiencia, lo cual significa que no se trata
      de un grafo mínimo. De hecho, el grafo generado tiende a ser denso.

      @param[in] __num_nodes cantidad de nodos que debe tener el grafo.
      @param[in] p probabilidad de existencia de arco entre cualquier
      par de nodos. Por omisión este parámetro tiene valor 0.5, valor
      que intuitiva y empíricamente tiende a generar menor arcos y
      consumir menos tiempo.
      @return puntero al grafo aleatorio creado.
      @throw bad_alloc si no hay suficiente memoria.
      @throw domain_error si el valor de p no está entre 0 y 1.

      @ingroup Grafos
  */
  GT sufficient_hamiltonian (const size_t & __num_nodes, 
			     const double & p = 0.5) 
  {
    g = this->create_p(__num_nodes, p, true);
    make_hamiltonian();

    return std::move(g);
  }
};

 
  /** Generador de grafos aleatorios.

      Random_Graph es una clase generadora de grafos aleatorios de
      num_nodes y según el tipo de operación que se invoque de num_arcs.

      La clase exporta dos métodos de generación, a través del operador
      paréntesis, destinados a la generación de grafos esparcidos y
      densos respectivamente.

      Para que la clase compile deben existir los constructores
      GT::Node_Type y GT::Arc_Type.

      Los parámetros de la clase son:
      - GT: el tipo de grafo a construir.
      - Init_Node: clase de inicialización de los datos del nodo.
      - Init_Arc: clase de inicialización de los datos del arco.
      .

      La clase Init_Node tiene la siguiente estructura:
      \code
      struct Init_Node
      {
        void operator () (GT & g, GT::Node * p) { ... }
      \endcode

      La clase Init_Arc tiene la siguiente estructura:
      \code
      struct Init_Arc
      {
        void operator () (GT & g, GT::Arc * a) { ... }
      \endcode

      Las rutinas de generación contienen un parámetro lógico que
      determina si el grafo debe ser o no conexo. Por omisión el
      parámetro es true. Tómese eso en consideración si no es necesario
      que el grafo sea conexo, pues para hacer el grafo conexo se
      requiere determinar los componentes inconexos, lo cual requiere
      más tiempo y consumo de memoria.

      @author Leandro Rabindranath León
      @ingroup Grafos
   */
  template <class GT, 
	    class Init_Node = Dft_Init_Rand_Node<GT>,
	    class Init_Arc = Dft_Init_Rand_Arc<GT> >
class Random_Graph : public Random_Graph_Base<GT, Init_Node, Init_Arc>
{
  typedef typename GT::Node GT_Node;
  typedef typename GT::Arc GT_Arc;

  DynSetRandTree<GT_Node*> odd_nodes;  // nodos con grado impar
  DynSetRandTree<GT_Node*> even_nodes; // nodos con grado par
  
  virtual void update_parity_after_arc_insertion(GT_Node * src, GT_Node * tgt)
  {
    if (not this->save_parity)
      return;

    if (is_even(this->g.get_num_arcs(src)))
      { // era impar antes de la inserción
	this->odd_nodes.remove(src);
	this->even_nodes.insert(src);
      }
    else
      {
	this->even_nodes.remove(src);
	this->odd_nodes.insert(src);
      }

    if (is_even(this->g.get_num_arcs(tgt)))
      { // era impar antes de la inserción
	this->odd_nodes.remove(tgt);
	this->even_nodes.insert(tgt);
      }
    else
      {
	this->even_nodes.remove(tgt);
	this->odd_nodes.insert(tgt);
      }
  }

  virtual void create_nodes_and_initialize_arc_index()
  {
    this->nodes = unique_ptr<DynArray<GT_Node*>>
      (new DynArray<GT_Node*>(this->num_nodes));

    this->nodes->reserve(this->num_nodes);

    for (int i = 0; i < this->num_nodes; ++i)
      {
	auto p = this->g.insert_node(new GT_Node);
	this->nodes->access(i) = p;
	this->init_node(this->g, p);
	if (this->save_parity)
	  {
	    this->even_nodes.insert(p);
	    NODE_COUNTER(p) = 0;
	  }
      }

    this->idx_arc = unique_ptr<IndexArc<GT>> (new IndexArc<GT>(this->g) ); 
  }

  virtual void connect()
  {
    DynList<DynList<GT_Node*>> subgraphs; // lista sufgrafos

    Inconnected_Components <GT> () (this->g, subgraphs);

    const size_t & num_subs = subgraphs.size();

    if (num_subs == 1)
      return;

    DynArray<GT_Node*> block_nodes;

    for (typename DynList<DynList<GT_Node*>>::Iterator it(subgraphs); 
	 it.has_curr(); it.next_ne())
      block_nodes.append(this->select_random_node(it.get_curr_ne()));
    
    for (int i = 1; i < num_subs; ++i)
      {
	auto src = block_nodes.access(i - 1);
	auto tgt = block_nodes.access(i);
	this->insert_arc(src, tgt);
      }
  }

  // crea un grafo aleatorio en el cual la probabilida por arco entre un
  // par de nodos es p
  virtual GT create_p (const size_t & __num_nodes, const double & p, 
		       bool connected)
  {
    if (p > 1.0 or p <= 0.0)
      throw std::domain_error("Invalid value for p");

    this->initialize_and_create_nodes(__num_nodes, __num_nodes);

    for (int i = 0; i < this->num_nodes - 1; ++i)
      {
	auto src = this->nodes->access(i);
	for (int j = i + 1; j < this->num_nodes; ++j)
	  if (gsl_rng_uniform(this->r) <= p) // sorteo
	    {
	      auto tgt = this->nodes->access(j);
	      assert(src != tgt);
	      this->insert_arc(src, tgt);
	    }	
      }

    if (connected)
      connect();

    return move(this->g);
  }

public:

  /** Constructor

      @param[in] seed semilla del generador de números aleatorios
      @param[in] __init_node clase de inicialización de nodo 
      @param[in] __init_arc clase de inicialización de arco
   */  
  Random_Graph(unsigned long seed,
	       const Init_Node & __init_node,
	       const Init_Arc  & __init_arc)	       
    : Random_Graph_Base<GT, Init_Node, Init_Arc>(seed, __init_node, __init_arc)
  {
    if (this->g.is_digraph())
      throw std::domain_error("Building of random digraph through a graph");
  }

  Random_Graph(unsigned long seed = time(nullptr),
	       const Init_Node && __init_node = Init_Node(),
	       const Init_Arc  && __init_arc  = Init_Arc())	       
    : Random_Graph_Base<GT, Init_Node, Init_Arc>(seed, __init_node, __init_arc)
  {
    if (this->g.is_digraph())
      throw std::domain_error("Building of random digraph through a graph");
  }


  /** Crea un grafo aleatorio esparcido.

      Esta versión construye un grafo aleatorio de aproximadamente
      __num_arcs. El grafo  no contiene arcos paralelos. 

      El procedimiento de aleatorización es seleccionar al azar dos
      nodos e insertar un arco entre ellos en caso de que éste no
      exista. La rutina es lineal, proporcional a __num_arcs.

      @param[in] __num_nodes cantidad de nodos que debe tener el grafo.
      @param[in] __num_arcs cantidad de arcos  que debe tener el grafo.
      @param[in] connected si el grafo a generar debe ser conexo o
      no. Por omisión lo es.
      @return puntero al grafo aleatorio creado.
      @throw bad_alloc si no hay suficiente memoria.
   */
  GT operator () (const size_t & __num_nodes, const size_t & __num_arcs, 
		  bool connected = true) 
  {
    return this->create(__num_nodes, __num_arcs, connected);
  }
  
public:

  /** Crea un grafo aleatorio.

      Esta versión construye un grafo aleatorio con probabilidad p de
      existencia de arco entre cualquier par de nodos.

      Úsese esta rutina para generar grafos densos. En este caso,
      empléese un valor de p mayor o igual a 0.5.

      El procedimiento es cuadrático. Se generan todas las combinaciones
      de nodos y por cada par de nodos (i,j) se
      sortea con probabilidad p si debe haber un arco entre i y j.

      @warning El procedimiento puede ser extremadamente lento y goloso
      en memoria conforme aumento el valor de __num_nodes.

      @param[in] __num_nodes cantidad de nodos que debe tener el grafo.
      @param[in] p probabilidad de existencia de arco entre cualquier
      par de nodos.
      @param[in] connected si el grafo a generar debe ser conexo o
      no. Por omisión lo es.
      @return puntero al grafo aleatorio creado.
      @throw bad_alloc si no hay suficiente memoria.
      @throw domain_error si el valor de p no está entre 0 y 1.
  */
  GT operator () (const size_t & __num_nodes, const double & p, 
		  bool connected = true) 
  {
    return create_p(__num_nodes, p, connected);
  }

private:

  virtual void make_eulerian()
  {
    while (this->odd_nodes.size() > 1)
      {
	GT_Node * src = nullptr;
	GT_Node * tgt = nullptr;

	while (true)
	  {
	    src = this->odd_nodes.select
	      (gsl_rng_uniform_int(this->r, this->odd_nodes.size()));
	    do
	      tgt = this->odd_nodes.select
		(gsl_rng_uniform_int(this->r, this->odd_nodes.size()));
	    while (tgt == src);

	    if (this->idx_arc->search(src, tgt) == nullptr) 
	      break;
	    else if (this->odd_nodes.size() == 2)
	      { // seleccionar nodo al azar que no tenga arco hacia src o tgt
		GT_Node * p = nullptr;
		do
		  p = this->even_nodes.select
		    (gsl_rng_uniform_int(this->r, this->even_nodes.size()));
		while (this->idx_arc->search(src, p) != nullptr or
		       this->idx_arc->search(tgt, p) != nullptr);
		this->insert_arc(src, p);
		this->insert_arc(p, tgt);

		return;
	      }
	  }
	   
	this->insert_arc(src, tgt);
      }
    
    assert(this->odd_nodes.size() == 0);
  }

  void balance_graph_nodes_degree(GT_Node * src, GT_Node * tgt)
  {
    if (this->idx_arc->search(src, tgt) == nullptr)
      this->insert_arc(src, tgt);

    const size_t & n = this->g.get_num_nodes();

    while (this->g.get_num_arcs(src) + this->g.get_num_arcs(tgt) < n)
      {
	auto p = this->nodes->access(gsl_rng_uniform_int(this->r, n));
	if (p == src or p == tgt)
	  continue;
	
	if (this->idx_arc->search(src, p) == nullptr)
	  this->insert_arc(src, p);

	if (this->g.get_num_arcs(src) + this->g.get_num_arcs(tgt) == n)
	  break;

	if (this->idx_arc->search(tgt, p) == nullptr)
	  this->insert_arc(tgt, p);
      }
  }

  virtual void make_hamiltonian ()
  {
    const size_t & n = this->g.get_num_nodes();
    for (int i = 0; i < n - 1; ++i)
      {
	auto src = this->nodes->access(i);
	for (int j = i + 1; j < n; ++j)
	  balance_graph_nodes_degree(src, this->nodes->access(j));
      }
  }

public:

      /** Crea un grafo aleatorio que es euleriano.

	  Esta versión construye un grafo aleatorio esparcido que se
	  garantiza es euleriano; es decir, que contiene ciclos
	  eulerianos.

	  El proceso primero genera un grafo esparcido
	  aleatorio, luego examina el grafo resultante y crea nuevos
	  arcos de modo que el resultado contenga un ciclo euleriano.

	  @param[in] __num_nodes cantidad de nodos que debe tener el grafo.
	  @param[in] __num_arcs cantidad de arcos  que debe tener el grafo.
	  @return puntero al grafo aleatorio creado.
	  @throw bad_alloc si no hay suficiente memoria.

       */
  GT eulerian (const size_t & __num_nodes, const size_t & __num_arcs) 
  {
    this->save_parity = true;
    this->g = this->create(__num_nodes, __num_arcs, true);
    make_eulerian();

    return std::move(this->g);
  }

      /** Crea un grafo aleatorio euleriano.

	  Esta versión construye un grafo aleatorio euleriano con
	  probabilidad p de existencia de arco entre cualquier par de
	  nodos. 

	  Úsese esta rutina para generar grafos densos. En este caso,
	  empléese un valor de p mayor o igual a 0.5.

	  @warning El procedimiento puede ser extremadamente lento y goloso
	  en memoria conforme aumento el valor de __num_nodes.

	  @param[in] __num_nodes cantidad de nodos que debe tener el grafo.
	  @param[in] p probabilidad de existencia de arco entre cualquier
	  par de nodos.
	  @return puntero al grafo aleatorio creado.
	  @throw bad_alloc si no hay suficiente memoria.
	  @throw domain_error si el valor de p no está entre 0 y 1.
      */
  GT eulerian (const size_t & __num_nodes, const double & p) 
  {
    this->save_parity = true;
    this->g = this->create_p(__num_nodes, p, true);
    make_eulerian();
 
    return std::move(this->g);
  }

  /** Crea un grafo aleatorio hamiltoniano.

      Esta versión construye un grafo aleatorio hamiltoniano.

      El proceso primero genera un grafo aleatorio, luego examina el
      grafo resultante y crea nuevos 
      arcos de modo que el resultado contenga un ciclo hamiltoniano.

      @warning El procedimiento puede ser extremadamente lento y goloso
      en memoria conforme aumente el valor de __num_nodes.

      @warning El grafo se asegura a ser hamiltoniano por los teoremas de
      Ore y Dirac. En este sentido, nótese que que la condición de
      hamiltoniano es de suficiencia, lo cual significa que no se trata
      de un grafo mínimo. De hecho, el grafo generado tiende a ser denso.

      @param[in] __num_nodes cantidad de nodos que debe tener el grafo.
      @param[in] p probabilidad de existencia de arco entre cualquier
      par de nodos. Por omisión este parámetro tiene valor 0.5, valor
      que intuitiva y empíricamente tiende a generar menor arcos y
      consumir menos tiempo.
      @return puntero al grafo aleatorio creado.
      @throw bad_alloc si no hay suficiente memoria.
      @throw domain_error si el valor de p no está entre 0 y 1.

      @ingroup Grafos
  */
  GT sufficient_hamiltonian (const size_t & __num_nodes, 
			     const double & p = 0.5) 
  {
    this->g = this->create_p(__num_nodes, p, true);
    make_hamiltonian();

    return std::move(this->g);
  }
};


  template <class GT, 
	    class Init_Node = Dft_Init_Rand_Node<GT>,
	    class Init_Arc = Dft_Init_Rand_Arc<GT>>
class Random_Digraph : public Random_Graph_Base<GT, Init_Node, Init_Arc>
{
  typedef typename GT::Node GT_Node;
  typedef typename GT::Arc GT_Arc;

  DynSetRandTree<GT_Node*> greater; // nodos con grado out mayor que in
  DynSetRandTree<GT_Node*> smaller; // nodos con grado out menor que in
  DynSetRandTree<GT_Node*> equal;   // nodos con grado out igual a in

  bool verify_tables()
  {
    const size_t & n = this->nodes->size();

    if (n != this->g.get_num_nodes())
      cout << "Warning num of nodes of graph does not match with array "
	   << this->g.get_num_nodes() << "!=" << n << endl;
    
    size_t total = greater.size() + smaller.size() + equal.size();
    if (total != this->g.get_num_nodes())
      cout << "Inconsistency with nodes parity" << endl
	   << "greater = " << greater.size() << endl
	   << "smaller = " << smaller.size() << endl
	   << "equal   = " << equal.size() << endl
	   << "total   = " << total << endl
	   << "|V|     = " << this->g.get_num_nodes();

    for (int i = 0; i < n; ++i)
      {
	auto p = this->nodes->access(i);
	
	const long & in_sz    = NODE_COUNTER(p);
	const size_t & out_sz = this->g.get_num_arcs(p);
	
	if (in_sz == out_sz)
	  {
	    if (smaller.search(p) != nullptr)
	      cout << "Inconsistency " << in_sz << "/" << out_sz << " found "
		   << " in smaller table" << endl;
	    
	    if (greater.search(p) != nullptr)
	      cout << "Inconsistency " << in_sz << "/" << out_sz << " found "
		   << " in greater table" << endl;
	    
	    if (equal.search(p) == nullptr)
	      {
		cout << "node of same in/out degree is not in equal table" 
		     << endl;

		return false;
	      }
	  }
	else if (in_sz > out_sz)
	  {
	    if (greater.search(p) != nullptr)
	      cout << "Inconsistency " << in_sz << "/" << out_sz << " found "
		   << " in greater table" << endl;
	    
	    if (equal.search(p) != nullptr)
	      cout << "Inconsistency " << in_sz << "/" << out_sz << " found "
		   << endl;

	    if (smaller.search(p) == nullptr)
	      {
		cout << "node with " << in_sz << "/" << out_sz << " not found "
		     << "smaller table" << endl;

		return false;
	      }	    	    
	  }
	else
	  {
	   if (smaller.search(p) != nullptr)
	      cout << "Inconsistency " << in_sz << "/" << out_sz << " found "
		   << " in smaller table" << endl;
	    
	    if (equal.search(p) != nullptr)
	      cout << "Inconsistency " << in_sz << "/" << out_sz << " found "
		   << endl;

	    if (greater.search(p) == nullptr)
	      {
		cout << "node with " << in_sz << "/" << out_sz << " not found "
		     << "greater table" << endl;

		return false;
	      }	    	     
	  }
      }

    return true;
  }  
 
  // Esta llamada se efectúa justo después de insertar un nuevo
  // arco src-->tgt. Esto implica que out(src) está actualizado, pero
  // in(tgt) no 
  virtual void update_parity_after_arc_insertion(GT_Node * src, GT_Node * tgt)
  {
    if (not this->save_parity)
      return;

    const size_t & src_out_degree = this->g.get_num_arcs(src);
    const long & src_in_degree    = NODE_COUNTER(src);

    if (src_out_degree == src_in_degree)
      { // src está en greater ==> sacarlo y meterlo en equal
	assert(this->smaller.search(src) != nullptr);
	this->smaller.remove(src);
	this->equal.insert(src);
      }
    else if (src_out_degree > src_in_degree)
      if (src_out_degree == src_in_degree + 1)
	{
	  assert(this->equal.search(src) != nullptr);
	  this->equal.remove(src);
	  this->greater.insert(src);
	}
      else
	assert(this->greater.search(src) != nullptr);
    else // src_out_degree < src_in_degree
      assert(this->smaller.search(src) != nullptr);
    
    const size_t & tgt_out_degree = this->g.get_num_arcs(tgt);
    const long tgt_in_degree      = ++NODE_COUNTER(tgt);

    if (tgt_out_degree == tgt_in_degree)
      {
	assert(this->greater.search(tgt));
	this->greater.remove(tgt);
	this->equal.insert(tgt);
      }
    else if (tgt_out_degree > tgt_in_degree)
      assert(this->greater.search(tgt));
    else // (tgt_out_degree < tgt_in_degree)
      {
	if (tgt_in_degree - 1 == tgt_out_degree)
	  { // tgt está en equal ==> sacarlo
	    assert(this->equal.search(tgt) != nullptr);
	    this->smaller.insert(tgt);	  
	    this->equal.remove(tgt);
	  }
	else
	  assert(this->smaller.search(tgt) != nullptr);
      } 
  }

  virtual void create_nodes_and_initialize_arc_index()
  {
    this->nodes = unique_ptr<DynArray<GT_Node*>>
      (new DynArray<GT_Node*>(this->num_nodes));

    this->nodes->reserve(this->num_nodes);

    for (int i = 0; i < this->num_nodes; ++i)
      {
	typename GT::Node * p = this->g.insert_node(new GT_Node);
	this->nodes->access(i) = p;
	this->init_node(this->g, p);

	if (this->save_parity)
	  {
	    NODE_COUNTER(p) = 0;
	    this->equal.insert(p);
	  }
      }

    this->idx_arc = unique_ptr<IndexArc<GT>> (new IndexArc<GT>(this->g) ); 
  }

  virtual void connect()
  {
    DynList<DynList<typename GT::Node*>> blk_list; // subgrafos inconexos

    {     // guarda los grados de entrada puesto que el algoritmo de Tarjan
          // los va a modificar 
      DynArray<int> in_degree; 
      in_degree.reserve(this->g.get_num_nodes());

      typename GT::Node_Iterator it(this->g);
      for (int i = 0; it.has_curr(); it.next_ne(), ++i)
	in_degree.access(i) = NODE_COUNTER(it.get_curr_ne());

      Tarjan_Connected_Components <GT> () (this->g, blk_list);

      it.reset_first(); // restaurar los grados de entrada
      for (int i = 0; it.has_curr(); it.next_ne(), ++i)
	NODE_COUNTER(it.get_curr_ne()) = in_degree.access(i);
    }

    const size_t & num_blocks = blk_list.size();

    if (num_blocks == 1) 
      return;

        // cada nodo de esta lista es un nodo del bloque i seleccionado
        // aleatoriamente  
    DynArray<typename GT::Node*> b1; b1.reserve(num_blocks);
    DynArray<typename GT::Node*> b2; b2.reserve(num_blocks);
    {
      typename DynList<DynList<GT_Node*>>::Iterator it(blk_list);
      for (int i = 0; it.has_curr(); it.next_ne(), ++i)
      {     // seleccione dos nodos al azar del componente actual
	DynList<typename GT::Node*> & list = it.get_curr_ne();
	b1.access(i) = this->select_random_node(list);
	b2.access(i) = this->select_random_node(list);
      }
    }

    for (int i = 0; i < num_blocks - 1; ++i)
      {
        auto src = b1.access(i);                    // nodo en bloque i
        auto tgt = b1.access((i + 1) % num_blocks); // nodo en bloque i + 1
	
	if (this->idx_arc->search_directed(src, tgt) == nullptr)
	  this->insert_arc(src, tgt);
	
	src = b2.access(i);                    // nodo en bloque i
	tgt = b2.access((i + 1) % num_blocks); // nodo en bloque i + 1

	if (this->idx_arc->search_directed(tgt, src) == nullptr)
	  this->insert_arc(tgt, src);
      }
  }

  // crea un grafo aleatorio en el cual la probabilidad por arco entre un
  // par de nodos es p
  virtual GT create_p(const size_t & __num_nodes, const double & p, 
		      bool connected)
  {
    if (p > 1.0 or p <= 0.0)
      throw std::domain_error("Invalid value for p");

    this->initialize_and_create_nodes(__num_nodes, __num_nodes);

    for (int i = 0; i < this->num_nodes; ++i)
      {
	auto src = this->nodes->access(i);
	for (int j = 0; j < this->num_nodes; ++j)
	  if (i != j and gsl_rng_uniform(this->r) <= p)
	    {
	      auto tgt = this->nodes->access(j);
	      assert(this->idx_arc->search_directed(src, tgt) == nullptr);
	      this->insert_arc(src, tgt);
	    }
      }

    if (connected)
      connect();

    return move(this->g);
  }

public:

  /** Constructor

      @param[in] seed semilla del generador de números aleatorios
      @param[in] __init_node clase de inicialización de nodo
      @param[in] __init_arc clase de inicialización de arco
   */
  Random_Digraph(unsigned long seed, 
		 const Init_Node & __init_node,
		 const Init_Arc  & __init_arc)
    : Random_Graph_Base<GT, Init_Node, Init_Arc>(seed, __init_node, __init_arc)
  {
    this->g.set_digraph(true);
  } 

  Random_Digraph(unsigned long seed = time(nullptr), 
		 const Init_Node && __init_node = Init_Node(),
		 const Init_Arc  && __init_arc  = Init_Arc())
    : Random_Digraph(seed, __init_node, __init_arc)
  {
    // empty
  }

  Random_Digraph(const Init_Node & __init_node,
		 const Init_Arc  & __init_arc)
    : Random_Digraph(time(nullptr), __init_node, __init_arc)
  {
    // empty
  } 

  ~Random_Digraph() 
  {
    this->g.set_digraph(false); 
  }

  /** Crea un grafo aleatorio esparcido.

      Esta versión construye un grafo aleatorio de aproximadamente
      __num_arcs. El grafo  no contiene arcos paralelos. 

      El procedimiento de aleatorización es seleccionar al azar dos
      nodos e insertar un arco entre ellos en caso de que éste no
      exista. La rutina es lineal, proporcional a __num_arcs.

      @param[in] __num_nodes cantidad de nodos que debe tener el grafo.
      @param[in] __num_arcs cantidad de arcos  que debe tener el grafo.
      @param[in] connected si el grafo a generar debe ser conexo o
      no. Por omisión lo es.
      @return puntero al grafo aleatorio creado.
      @throw bad_alloc si no hay suficiente memoria.
   */
  GT operator () (const size_t & __num_nodes, const size_t & __num_arcs, 
		  bool connected = true) 
  {
    return this->create(__num_nodes, __num_arcs, connected);
  }

public:

  /** Crea un grafo aleatorio.

      Esta versión construye un grafo aleatorio con probabilidad p de
      existencia de arco entre cualquier par de nodos.

      Úsese esta rutina para generar grafos densos. En este caso,
      empléese un valor de p mayor o igual a 0.5.

      El procedimiento es cuadrático. Se generan todas las combinaciones
      de nodos y por cada par de nodos (i,j) se
      sortea con probabilidad p si debe haber un arco entre i y j.

      @warning El procedimiento puede ser extremadamente lento y goloso
      en memoria conforme aumento el valor de __num_nodes.

      @param[in] __num_nodes cantidad de nodos que debe tener el grafo.
      @param[in] p probabilidad de existencia de arco entre cualquier
      par de nodos.
      @param[in] connected si el grafo a generar debe ser conexo o
      no. Por omisión lo es.
      @return puntero al grafo aleatorio creado.
      @throw bad_alloc si no hay suficiente memoria.
      @throw domain_error si el valor de p no está entre 0 y 1.
  */
  GT operator () (const size_t & __num_nodes, const double & p, 
		  bool connected = true) 
  {
    return this->create_p(__num_nodes, p, connected);
  }

private:

  virtual void make_eulerian()
  {
    GT_Node * src = nullptr;
    GT_Node * tgt = nullptr;

    while (this->greater.size() > 0 and this->smaller.size() > 0)
      {
	do
	  {
	    tgt = this->greater.select
	      (gsl_rng_uniform_int(this->r, this->greater.size()));
	    src = this->smaller.select
	      (gsl_rng_uniform_int(this->r, this->smaller.size()));
	  }
	while (src == tgt);
      
	if (this->idx_arc->search_directed(src, tgt) == nullptr)
	  this->insert_arc(src, tgt);
	else
	  {
	    auto mid = 
	      this->equal.select(gsl_rng_uniform_int(this->r, 
						     this->equal.size()));

	    while (this->idx_arc->search_directed(src, mid) != nullptr or 
		   this->idx_arc->search_directed(mid, tgt) != nullptr)
	      mid = this->equal.select
		(gsl_rng_uniform_int(this->r, this->equal.size()));
	    
	    this->insert_arc(src, mid);
	    this->insert_arc(mid, tgt);
	  }
      }
  }

  void balance_digraph_node(GT_Node * p)
  {
    const size_t & n = this->g.get_num_nodes();
    const size_t n2 = n/2;

    while (not (this->g.get_num_arcs(p) >= n2 and NODE_COUNTER(p) >= n2))
      {
	auto q = this->nodes->access(gsl_rng_uniform_int(this->r, n));
	if (q == p)
	  continue;

	if (this->idx_arc->search_directed(p, q) == nullptr)
	  {
	    this->insert_arc(p, q);
	    NODE_COUNTER(q)++;
	  }

	if (this->idx_arc->search_directed(q, p) == nullptr)
	  {
	    this->insert_arc(q, p);
	    NODE_COUNTER(p)++;
	  }
      }
  }

  // balancea los dos nodos para que satisfagan la condición de
  // hamiltoniano. Si existe arco src-->tgt
  void balance_digraph_nodes_degree(GT_Node * src, GT_Node * tgt)
  {
    if (this->idx_arc->search_directed(src, tgt) != nullptr)
      {
	balance_digraph_node(src);
	balance_digraph_node(tgt);

	return;
      }

    const size_t & n = this->g.get_num_nodes();

    while (this->g.get_num_arcs(src) + NODE_COUNTER(tgt) < n)
      {
	auto p = this->nodes->access(gsl_rng_uniform_int(this->r, n));
	if (p == src or p == tgt)
	  continue;
	
	if (this->idx_arc->search_directed(src, p) == nullptr)
	  {
	    this->insert_arc(src, p);
	    NODE_COUNTER(p)++;
	  
	    if (this->g.get_num_arcs(src) + NODE_COUNTER(tgt) == n)
	      break;
	  }

	if (this->idx_arc->search_directed(p, tgt) == nullptr)
	  {
	    this->insert_arc(p, tgt);
	    NODE_COUNTER(tgt)++;
	  }
      }

    assert(this->g.get_num_arcs(src) + NODE_COUNTER(tgt) >= n);
  }

  virtual void make_hamiltonian ()
  {
    this->g.reset_counter_nodes();

        // contabiliza el grado de entrada por cada nodo
    for (typename GT::Arc_Iterator it(this->g); it.has_curr(); it.next_ne())
      NODE_COUNTER(it.get_tgt_node_ne())++;

    const size_t & n = this->g.get_num_nodes();

    for (int i = 0; i < n; ++i)
      {
	auto src = this->nodes->access(i);
	for (int j = 0; j < n; ++j)
	  {
	    if (i == j)
	      continue;

	    auto tgt = this->nodes->access(j);	    
	    balance_digraph_nodes_degree(src, tgt);
	  }
      }
  }
};

}

# endif // RANDOM_GRAPH_H
