
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

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
# ifndef TPL_ANT_H
# define TPL_ANT_H

# include <gsl/gsl_rng.h>
# include <gsl/gsl_randist.h>

# include <limits>

# include <fstream>

# include <tpl_dynArray.H>
# include <tpl_graph_utils.H>
# include <tpl_agent_graph.H>


extern bool verbose;

using namespace Aleph;

namespace Aleph {


extern double  initial_life; // = 100000

extern double  feromone_importance; // = 2.5

extern double  distance_importance; // = 1;
 
extern double  food_importance; // = 4;

extern double  & alpha; // = feromone_importance;
extern double  & beta;  // = distance_importance;
extern double  & chi;   // = food_importance;

extern double  Q; // = 10000; // constante de feromona

extern double  K; // = 1; // constante  de consumo de energía en transito

extern double  L; // = 1; // constante de pérdida de vida

extern double  F; // = 1; // constante de consumo de alimento

extern double  min_dist; //  = 10;
extern double  max_dist; //  = 1000;


struct Ant_Node : public Agent_Node<Empty_Class>
{
  double food;

  int num;

  Ant_Node() : food(0), num(-1)
  {
    // empty
  }

  Ant_Node(const double & __food)
    : food(__food), num(-1)
  {
    // empty
  }

  Ant_Node(const Empty_Class&) : num(-1) { /* Empty */ }

  Ant_Node(Ant_Node * p) 
    : food(p->food), num(p->num)
  {
    // empty
  }
};


struct Ant_Arc : public Agent_Arc<Empty_Class>
{
  double feromone_level;

  double distance;

  Ant_Arc() : feromone_level(0), distance(0) { /* empty */ }

  Ant_Arc(Ant_Node * src, Ant_Node * tgt) 
    : Agent_Arc <Empty_Class> (src, tgt), feromone_level(0), distance(0) 
  {
    // empty 
  }
};


struct Ant_Graph;


struct Ant : public Walking_Agent<Empty_Class>
{
  Ant() { /* empty */ }

  // Ant(Transit transit) 
  //   : Walking_Agent<Empty_Class>(Empty_Class(), transit)
  // {
  //   // empty
  // }

      // return true si hormiga debe continuar; false de lo contrario
  virtual bool select_path(void *  /* src_node */, 
			   void  *& /* tgt_node */, 
			   void *&  /* ant_arc */)
  {
    EXIT("AH_ERROR: must not be called");

    return true;
  }

  virtual bool walk(Ant_Graph * /* g */, 
		    void *  /* tgt_node */, 
		    void *   /* ant_arc */)
  {
    EXIT("AH_ERROR: must not be called");
    
    return false;
  }
};


struct Ant_Graph : public Agent_Graph<List_Graph, Ant_Node, Ant_Arc, Ant> 
{
  typedef Ant_Graph::Node Node;

  typedef Ant_Graph::Arc Arc;

  pthread_mutex_t bit_mutex;

  BitArray nodes;

  Ant_Graph(const size_t & num_threads = 1) 
    : Agent_Graph<List_Graph, Ant_Node, Ant_Arc, Ant> (num_threads), 
      nodes(nullptr)
  {
    init_mutex(bit_mutex);
  }

  ~Ant_Graph()
  {
    destroy_mutex(bit_mutex);
  }

  Ant * insert_ant(Ant * p)
  {
    
    return p;
  }

      // subclases para cargar y guardar el grafo en disco
  struct Save_Node
  {
    void operator () (ofstream & output, Ant_Graph &, Ant_Graph::Node * p)
    {
      output << p->food << " " << p->num << endl;

      if (verbose)
	cout << endl;
    }
  };

  struct Load_Node
  {
    void operator () (ifstream & input, Ant_Graph &, Ant_Graph::Node * p)
    {
      input >> p->food >> p->num;

      if (verbose)
	cout <<  p->food << " " << p->num;
    }
  };

  struct Save_Arc
  {
    void operator () (ofstream & output, Ant_Graph &, Ant_Graph::Arc * a)
    {
      output << a->feromone_level << " " << a->distance << endl;

      if (verbose)
	cout << a->feromone_level << " " << a->distance;
    }
  };

  struct Load_Arc
  {
    void operator () (ifstream & input, Ant_Graph &, Ant_Graph::Arc * a)
    {
      input >> a->feromone_level >> a->distance;

      if (verbose)
	cout << a->feromone_level << " " << a->distance;
    }
  };
};


struct Working_Ant : public Ant 
{
  double life;

  static long born_count;
  static long died_count;

  static int bit_idx;

  int my_bit; // bit asignado para marcar nodos visitados

  Working_Ant() : life(initial_life)
  {
    // empty
  }

  // Working_Ant(Transit transit, const double & __life) 
  //   : Ant(transit), life(__life)
  // {
  //   // Empty
  // }

  ~Working_Ant()
  {
    // empty
  }

  /// Selecciona el próximo camino
  virtual bool select_path(Ant_Graph::Node *  src_node, 
			   Ant_Graph::Node *& tgt_node, 
			   Ant_Graph::Arc *&  ant_arc);

      /// ejecuta la transición
  virtual bool walk(Ant_Graph * g, 
		    Ant_Graph::Node *  tgt_node, 
		    Ant_Graph::Arc *   ant_arc);
};


void save(Ant_Graph & g, ofstream & output);

void load(Ant_Graph & g, ifstream & input);


} // end namespace Aleph

# endif 
