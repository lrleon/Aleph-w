
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
  @file io_graph.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef IO_GRAPH_H
# define IO_GRAPH_H

# include <fstream>
# include <iostream>
# include <tpl_graph.H>

extern bool verbose; /* Comunmente definida en main */

namespace Aleph { 


  template <class GT>
struct Dft_Store_Node
{
  void operator () (ofstream & output, GT &, typename GT::Node * p)
  {
    output.write((char*) &p->get_info(), sizeof(typename GT::Node_Type));
  }

  void operator () (ostream & output, GT &, typename GT::Node * p)
  {
    output << p->get_info() << endl;
  }
};

  template <class GT>
struct Dft_Store_Arc
{
  void operator () (ofstream & output, GT &, typename GT::Arc * a)
  {
    output.write((char*) &a->get_info(), sizeof(typename GT::Arc_Type));
  }

  void operator () (ostream & output, GT &, typename GT::Arc * a)
  {
    output << a->get_info() << endl;
  }
};

  template <class GT>
struct Dft_Load_Node
{
  void operator () (ifstream & input, GT &, typename GT::Node * p)
  {
    input.read((char*) &p->get_info(), sizeof(typename GT::Node_Type));
  }

  void operator () (istream & input, GT &, typename GT::Node * p)
  {
    input >> p->get_info();
  }
};

  template <class GT>
struct Dft_Load_Arc
{
  void operator () (ifstream & input, GT &, typename GT::Arc * a)
  {
    input.read((char*) &a->get_info(), sizeof(typename GT::Arc_Type));
  }

  void operator () (istream & input, GT &, typename GT::Arc * a)
  {
    input >> a->get_info();
  }
};

/** 
  @brief Clase para guardar y recuperar grafos en y de memoria secundaria.

  @details IO_Graph está diseñada para guardar y leer grafos en y desde
  memoria secundaria, a través de objetos derivados de fstream.

  La clase maneja 7 parámetros tipo
  -# GT: Grafo a guardar o leer.
  -# Load_Node: Clase encargada de leer la información relacionada a 
     un nodo y cargarla a su contenido.
  -# Store_Node: Clase encargada de guardar la información relacionada a
     un nodo y cargarla a su contenido. 
    Por omisión se escribe el valor arrojado por get_info() sobre el nodo.
  -#  Load_Arc: Clase encargada de leer la información relacionada a
     un arco y cargarla a su contenido. Por omisión se lee en bruto
	   del stream y se carga a get_info() sobre el arco.
  -# Store_Arc: Clase encargada de guardar la información relacionada a
     un arco y cargarla a su contenido. Por omisión se
	   escribe el valor arrojado por get_info() sobre el arco.
  -# NF: Clase filtro de nodos; sólo se invoca para guardar el
     grafo y sólo se guardan los nodos que pasen por el filtro.
  -# AF: Clase filtro de arcos; sólo se invoca para guardar el
     grafo y sólo se guardan los arcos que pasen por el filtro.
 .

	EL formato para guardar es:
	-# Cantidad de nodos.
	-# Contenido de los nodos escrito por Store_Node.
	-# Cantidad de arcos.
	-# Finalmente se escriben los arcos con el siguiente formato:
	   -# Índice del nodo origen e índice del nodo destino según el
	      orden de aparición cuando se escribieron los nodos.
	   -# Contenido del arco escrito por Store_Arc.
    .
 .

	En el caso más común en el cual sólo se requiera guardar en
	bruto los contenidos ofrecidos por get_info() en los nodos y
	arcos, entonces, a través de las plantillas por omisión, es
	suficiente con invocar a la clase IO_Graph<GT> sin el resto de
	los parámetros tipo.

	Según el compilador (en el caso de este autor gnu), los errores
	de compilación si hay alguna falla de correspondencia con los
	parámetros plantilla o con los parámetros de las funciones
	pueden ser muy crípticos. Por esa razón, se recomienda
	ampliamente que las clases de lectura y escritura sean hechas a
	partir de las de omisión que se incluyen en el archivo.

  @see Filter_Iterator 
  
  @ingroup Grafos
*/
template <class GT, 
	  class Load_Node  = Dft_Load_Node<GT>,
	  class Store_Node = Dft_Store_Node<GT>,
	  class Load_Arc   = Dft_Load_Arc<GT>,
	  class Store_Arc  = Dft_Store_Arc<GT>,
	  class NF         = Aleph::Dft_Show_Node<GT>, /* Filtro de nodos */
	  class AF         = Aleph::Dft_Show_Arc<GT> > /* Filtro de arcos */
class IO_Graph
{

  GT & g;

public:

/** 
  @brief With reference to graph constructor 
*/
  IO_Graph(GT & __g) : g(__g) { /* Empty */ }

/** 
  @brief Constructor pointer graph 
*/
  IO_Graph(GT * gptr) : g(*gptr) { /* Empty */ }

/** 
  @brief Save the graph in the output stream 
*/
  void save(ofstream & output)
  {
/** 
  @brief Save number of nodes 
*/
    const size_t num_nodes = g.get_num_nodes();

    if (verbose)
      cout << "Storing " << num_nodes << " nodes... ";

    output.write((const char*) &num_nodes, sizeof(num_nodes));

    int i = 0; /* Counter nodes. Bring the indexes on the table */

/** 
  @brief Table of pairs <ptr node order isnerción> 
*/
    DynMapTreap<typename GT::Node *, int> nodes_table;

    for (Node_Iterator<GT,NF> it(g); it.has_current(); it.next(), ++i)
      {
	typename GT::Node * p = it.get_current();

	if (verbose)
	  cout << i << " ";

	Store_Node () (output, g, p);

	nodes_table.insert(p, i);
      }

    const size_t num_arcs = g.get_num_arcs();

    if (verbose)
      cout << " done " << endl 
	   << "Storing " << num_arcs << " arcs... " << endl;

    output.write((const char*) &num_arcs, sizeof(num_arcs));

    for (Arc_Iterator<GT,AF> it(g); it.has_current(); it.next())
      {
	typename GT::Arc * a = it.get_current();

	typename GT::Node * src = g.get_src_node(a);
	typename GT::Node * tgt = g.get_tgt_node(a);

	const int & src_idx = nodes_table.find(src);
	const int & tgt_idx = nodes_table.find(tgt);

/** @brief Save indices respectively source and destination nodes */
	output.write((const char*) &src_idx, sizeof(int));
	output.write((const char*) &tgt_idx, sizeof(int));

	if (verbose)
	  cout << " " << src_idx << "--" << tgt_idx << " ";

	Store_Arc () (output, g, a);

	if (verbose)
	  cout << endl;
      }
    
    if (verbose)
      cout << " done " << endl << endl;
  }

/** 
  @brief Load the graph from the stream output  
*/
  void load(ifstream & input)
  {
/** 
  @brief Read the quantity of nodes. 
*/
    size_t num_nodes;
    input.read((char *) &num_nodes, sizeof(num_nodes));

    if (verbose)
      cout << "Loading " << num_nodes << " nodes...";

    DynArray<typename GT::Node*> nodes_table(num_nodes);
    nodes_table.reserve(0, num_nodes - 1);

    for (size_t i = 0; i < num_nodes; ++i)
      {
	typename GT::Node * p = new typename GT::Node;

	if (verbose)
	  cout << " " << i;

	Load_Node () (input, g, p);

	p = g.insert_node(p);
	nodes_table.access(i) = p;
      }

    size_t num_arcs;
    input.read((char *) &num_arcs, sizeof(num_arcs));

    if (verbose)
      cout << " done " << endl 
	   << "Loading " << num_arcs << " arcs... " << endl;

    for (int i = 0; i < num_arcs; ++i)
      {
	int src_idx;
	input.read((char*) &src_idx, sizeof(int));
	typename GT::Node * src = nodes_table.access(src_idx);

	int tgt_idx;
	input.read((char*) &tgt_idx, sizeof(int));
	typename GT::Node * tgt = nodes_table.access(tgt_idx);

	typename GT::Arc * a = g.insert_arc(src, tgt);

	if (verbose)
	  cout << " " << src_idx << "--" << tgt_idx << " ";

	Load_Arc () (input, g, a);

	if (verbose)
	  cout << endl;
      }

    if (verbose)
      cout << " done " << endl << endl;
  }

/** 
  @brief Save the graph in stream output 
*/
  void save_in_text_mode(ostream & output)
  {

/** 
  @brief Save number of nodes. 
*/
    const size_t num_nodes = g.get_num_nodes();
    const size_t num_arcs  = g.get_num_arcs();
    output << num_nodes << endl
	   << num_arcs << endl;

    if (verbose)
      cout << "Storing " << num_nodes << " nodes... ";

/** 
  @brief Counter nodes. 

  @details Bring the indexes on the table. 
*/
    int i = 0; 

/** 
  @brief Table of pairs <ptr node insertion order> 
*/
    DynMapTreap<typename GT::Node *, int> nodes_table;

    for (Node_Iterator<GT,NF> it(g); it.has_current(); it.next(), ++i)
      {
	typename GT::Node * p = it.get_current();

	if (verbose)
	  cout << i << " ";

	Store_Node () (output, g, p);

	nodes_table.insert(p, i);
      }

    if (verbose)
      cout << " done " << endl 
	   << "Storing " << num_arcs << " arcs... " << endl;

    for (Arc_Iterator<GT,AF> it(g); it.has_current(); it.next())
      {
	typename GT::Arc * a = it.get_current();

	typename GT::Node * src = g.get_src_node(a);
	typename GT::Node * tgt = g.get_tgt_node(a);

	const int & src_idx = nodes_table.find(src);
	const int & tgt_idx = nodes_table.find(tgt);

/** 
  @brief Save indices respectively source and destination nodes. 
*/
	output << src_idx << " " << tgt_idx << " ";

	if (verbose)
	  cout << " " << src_idx << "--" << tgt_idx << " ";

	Store_Arc () (output, g, a);

	if (verbose)
	  cout << endl;
      }

    if (verbose)
      cout << " done " << endl << endl;
  }


/** 
  @brief Load the graph from the stream intput  
*/
  void load_in_text_mode(istream & input)
  {

    size_t num_nodes; /* Read number of nodes. */
    size_t num_arcs;
    
    input >> num_nodes >> num_arcs;

    if (verbose)
      cout << "Loading " << num_nodes << " nodes...";

    DynArray<typename GT::Node*> nodes_table(num_nodes);
    nodes_table.reserve(0, num_nodes - 1);

    for (size_t i = 0; i < num_nodes; ++i)
      {
	typename GT::Node * p = new typename GT::Node;

	if (verbose)
	  cout << " " << i;

	Load_Node () (input, g, p);

	p = g.insert_node(p);
	nodes_table.access(i) = p;
      }

    if (verbose)
      cout << " done " << endl 
	   << "Loading " << num_arcs << " arcs... " << endl;

    for (int i = 0; i < num_arcs; ++i)
      {
	int src_idx;
	int tgt_idx;

	input >> src_idx >> tgt_idx;

	typename GT::Node * src = nodes_table.access(src_idx);
	typename GT::Node * tgt = nodes_table.access(tgt_idx);

	typename GT::Arc * a = g.insert_arc(src, tgt);

	if (verbose)
	  cout << " " << src_idx << "--" << tgt_idx << " ";

	Load_Arc () (input, g, a);

	if (verbose)
	  cout << " " << endl;
      }

    if (verbose)
      cout << " done " << endl << endl;
  }
};

} /* End namespace Aleph */

# endif // IO_GRAPH_H
