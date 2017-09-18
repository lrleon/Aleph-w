
# include <tpl_ant.H>
# include <io_graph.H>

namespace Aleph {

double initial_life = 100000; 

double feromone_importance = 2.5;

double distance_importance = 1;
 
double food_importance = 4;

double & alpha = feromone_importance;
double & beta  = distance_importance;
double & chi   = food_importance;

double Q  = 10000; // constante de feromona

double K  = 1; // constante  de consumo de energía en transito

double L  = 1; // constante de pérdida de vida

double F  = 1; // constante de consumo de alimento


double min_dist  = 10;
double max_dist  = 1000;


long Working_Ant::born_count = 0;
long Working_Ant::died_count = 0;

int Working_Ant::bit_idx = 0;


bool Working_Ant::select_path(Ant_Graph::Node *  src_node, 
			      Ant_Graph::Node *& tgt_node, 
			      Ant_Graph::Arc *&  ant_arc)
{
  if (life < 0)
    return false;

  ant_arc  = NULL;
  tgt_node = NULL;

  double max_p = numeric_limits<double>::min();

  Ant_Graph::Node::Critical_Section node_critical_section(src_node);

  for (Node_Arc_Iterator<Ant_Graph> it(src_node); it.has_current(); it.next())
    {
      Ant_Node * tgt = it.get_tgt_node();

      // seleccionar entre los nodos que no hayan sido visitados el que
      // tenga mayor peso parámetros

      const double & food_w = tgt->food;

      if (food_w <= 0)
	continue;

      Ant_Arc * arc = it.get_current_arc();
 
      const double & tau = arc->feromone_level;

      const double & eta = arc->distance;

      const double total = pow(tau, alpha) + pow(eta, beta) + pow(food_w, chi);

      if (total > max_p)
	{
	  max_p    = total;
	  ant_arc  = it.get_current_arc();
	  tgt_node = it.get_tgt_node();
	}
    }

  return ant_arc != NULL ? true : false;
}

bool Working_Ant::walk(Ant_Graph *       g,
		       Ant_Graph::Node * tgt_node, 
		       Ant_Graph::Arc *  ant_arc)
{
  g->leave_agent_from_location(this);

  double distance;

  {
    CRITICAL_SECTION(ant_arc->mutex);

    distance = ant_arc->distance;

        // deposita feromona (mayor distancia ==> menor feromona)
    ant_arc->feromone_level += Q/distance;
  }

  {
    CRITICAL_SECTION(tgt_node->mutex);

        // Comida que consume hormiga (mayor distancia ==> mayor consumo)
    tgt_node->food -= F*distance/max_dist;
  }

      // vida que consume hormiga (mayor distancia ==> menos vida)
  life -= L*distance/max_dist;

  g->enter_agent_in_node(this, tgt_node);

  return true;
}


void ant_transit(void * __graph, void * __ant, void *)
{
  Ant_Graph * graph = static_cast<Ant_Graph*>(__graph);

  Working_Ant * working_ant = static_cast<Working_Ant*>(__ant); 

  {
    CRITICAL_SECTION(graph->bit_mutex);

    working_ant->my_bit = Working_Ant::bit_idx;

    Working_Ant::bit_idx = 
      (Working_Ant::bit_idx + 1) % graph->get_num_threads();
  }

  Ant_Graph::Node * current_node = graph->get_agent_node_location(working_ant);

  Ant_Graph::Node * next_node = NULL;

  Ant_Graph::Arc * next_arc = NULL;

  while (true)
    {
      const bool ant_is_alive = 
	working_ant->select_path(current_node, next_node, next_arc);

      if (not ant_is_alive) break;

      working_ant->walk(graph, next_node, next_arc);

      current_node = next_node;
    }

  graph->remove_agent(working_ant);
}


void save(Ant_Graph & g, ofstream & output)
{
  typedef Ant_Graph::Save_Node SN;
  typedef Ant_Graph::Load_Node LN;
  typedef Ant_Graph::Save_Arc  SA;
  typedef Ant_Graph::Load_Arc  LA;

  IO_Graph <Ant_Graph, LN, SN, LA, SA> (g).save_in_text_mode(output);
}

void load(Ant_Graph & g, ifstream & input)
{
  typedef Ant_Graph::Save_Node SN;
  typedef Ant_Graph::Load_Node LN;
  typedef Ant_Graph::Save_Arc  SA;
  typedef Ant_Graph::Load_Arc  LA;

  IO_Graph <Ant_Graph, LN, SN, LA, SA> (g).load_in_text_mode(input);
}

} // end namespace Aleph
