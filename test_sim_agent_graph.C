#include <iostream>
#include <cstdlib>
#include <ctime>
#include <sstream>

#include <tpl_dynBinHeap.H>
#include <tpl_sim_agent_graph.H>
#include <tpl_graph_utils.H>
#include <tpl_dynDlist.H>

using namespace std;

const string nodos[] = {"Las Tapias", "Humboldt", "Pie del Llano", "Casa Blanca", "Alto Prado", "Mercado", "Medicina", "Santa Juana", "Glorias Patrias", "Santo Niño", "Viaducto", "Santa Barbara", "Mc Donalds" };

#define MAX_TIME 30

struct Agent_Info
{
  int id;
  string descripcion;

  Agent_Info(int id, string desc) : id(id), descripcion(desc) { /* Empty */ }

  bool operator == (const Agent_Info & otroAgente) const 
  {
    return id == otroAgente.id;
  }

};

struct Nodo_Mapa
{
  string descripcion;

  Nodo_Mapa (string desc) : descripcion(desc) { /* Empty */ }

  bool operator == (const Nodo_Mapa & n) const
  {
    return strcasecmp(n.descripcion.c_str(), descripcion.c_str()) == 0;
  }
};

struct Arco_Mapa
{
  string descripcion;
  int delay;

  Arco_Mapa () { }
  Arco_Mapa (string desc, int delay) : descripcion(desc), delay(delay) { /* Empty */ }

};

typedef Sim_Agent<Agent_Info> Agente; 
typedef Sim_Agent_Node<Nodo_Mapa> Nodo;
typedef Sim_Agent_Arc<Arco_Mapa> Arco;
typedef Sim_Agent_Graph<Nodo, Arco, Agente> Grafo_Agentes;

Grafo_Agentes* mundo; 

void ejecutar(void * graph_ptr, void * agent_ptr)
{
  Grafo_Agentes *graph = static_cast<Grafo_Agentes *>(graph_ptr);
  Agente *agent = static_cast<Agente*>(agent_ptr);
  // Tomar nodo actual
  Grafo_Agentes::Node * prev_node = graph->get_agent_node_location(agent);
  // Imprimir posicion actual
  cout << agent->get_info().descripcion << " esta activandose en el tiempo " << agent->actual_time << endl;

  cout << agent->get_info().descripcion << " esta partiendo de nodo " << prev_node->get_info().descripcion << endl;
  // Decidir proximo paso
  int prox_arc_idx = rand() % graph->get_num_arcs(prev_node);
  int actual_arc = 0;
  Grafo_Agentes::Arc * selected_arc;
  for (Grafo_Agentes::Node_Arc_Iterator it(prev_node); it.has_current(); it.next())
  {
    if (actual_arc == prox_arc_idx)
		{
	    selected_arc = it.get_current_arc();
	  	break;
	  } 
	  else
	  {
	    actual_arc++;
		}
  }
  cout << agent->get_info().descripcion << " seleccionó arco " << selected_arc->get_info().descripcion << endl;
  
  graph->set_agent_arc_location(agent, selected_arc);
  // Recorrer proximo paso

  Grafo_Agentes::Node * next_node = graph->get_connected_node(selected_arc, prev_node);
  cout << agent->get_info().descripcion << " se ha movido a nodo " << next_node->get_info().descripcion << endl;
  graph->set_agent_node_location(agent, next_node);

  // Programar proximo evento
  long next_time = agent->actual_time;
  next_time += 20;
  agent->actual_time = next_time;
  graph->schedule_event(agent);

}


void create_street(string src_node_desc, string dest_node_desc, int delay, string street_name) 
{
  Nodo_Mapa info1(src_node_desc);
  Grafo_Agentes::Node * n1 = mundo->search_node(info1);
  if (n1 == NULL) 
    n1 = mundo->insert_node(info1);

  Nodo_Mapa info2(dest_node_desc);
  Grafo_Agentes::Node * n2 = mundo->search_node(info2);
  if (n2 == NULL) 
    n2 = mundo->insert_node(info2);
  Arco_Mapa info_arc(street_name, delay);
  cout << "Creada calle " << info_arc.descripcion << " desde " << info1.descripcion  << " hasta " << info2.descripcion << endl;
  mundo->insert_arc(n1, n2, info_arc);
}

void configure_agent_graph()
{
  // Crear grafo
  create_street("Las Tapias", "Humboldt", 5, "Albarregas");
  create_street("Las Tapias", "Pie del Llano", 10, "Av. Andres Bello");
  create_street("Humboldt", "Santo Niño", 6, "Av. Humboldt");
  create_street("Humboldt", "Santa Barbara", 11, "Las Americas");
  create_street("Santo Niño", "Casa Blanca", 20, "Panamericana");
  create_street("Santa Barbara", "Mc Donalds", 8, "Las Americas");
  create_street("Pie del Llano", "Santa Barbara", 7, "Sucre");
  create_street("Pie del Llano", "Santa Juana", 5, "Av. Ppal Santa Juana");
  create_street("Santa Juana", "Medicina", 16, "16 de Septiembre");
  create_street("Medicina", "Glorias Patrias", 4, "Don Tulio");
  create_street("Pie del Llano", "Glorias Patrias", 15, "Av. Urdaneta");
  create_street("Glorias Patrias", "Mercado", 6, "Miranda");
  create_street("Mc Donalds", "Mercado", 7, "Las Americas");
  create_street("Mc Donalds", "Casa Blanca", 3, "El Rodeo");
  create_street("Casa Blanca", "Alto Prado", 14, "Los Proceres");
  create_street("Mercado", "Viaducto", 7, "Las Americas");
  create_street("Viaducto", "Alto Prado", 6, "Campo Elias");
  
}

void create_agents_in_graph()
{

  cout<<"Creando los agentes en el Grafo"<<endl;
  for (int i = 1; i <= 5; i++) 
  {
 		stringstream agent_name_strm(stringstream::out) ;
    agent_name_strm << "Agente " << i;
    Agent_Info a(i, agent_name_strm.str());  	
    int index = rand() % 13;

    Grafo_Agentes::Node * n1 = mundo->search_node(Nodo_Mapa(nodos[index]));
    mundo->create_agent_in_node(a, &ejecutar,  n1, i);
  }
}

void print_agents_status()
{
  for (int i = 1; i <= 30; i++) 
  {
    Agent_Info a(i, "");
    Agente * agent = mundo->search_agent(a);
    if (agent != NULL) 
    {
      if (mundo->is_agent_in_node(agent))
      {
        Grafo_Agentes::Node * n1 = mundo->get_agent_node_location(agent);
        // Imprimir donde se encuentra
        cout << agent->get_info().descripcion << " se encuentra en nodo " << n1->get_info().descripcion << endl;
      }
      else 
      {
        Grafo_Agentes::Arc * a1 = mundo->get_agent_arc_location(agent);
        // Imprimir donde se encuentra
        cout << agent->get_info().descripcion << " se encuentra en arco " << a1->get_info().descripcion << endl;
      }
      // Imprimir su estado actual
    }
  }
}

int main()
{
  //cout <<"Comenzando ..."<<endl;
  // ejecutar_agente(); //jecuta el agente unicamente 

  srand((unsigned)time(0));
  mundo = new Grafo_Agentes(1);
  configure_agent_graph();
  create_agents_in_graph();
 
  // Comenzar la ejecución
  mundo->start_graph();
  
  
  // Permitir que se ejecute por 10 segundos
  sleep(3);
  // Pausar la ejecucion
  mundo->suspend_graph();
  // Mostrar el estado actual de los agentes
  print_agents_status();
  cout << "Presione cualquier tecla para continuar...";
  cin.get();
  // Reanudar la ejecución
  mundo->resume_graph();
  // Dormir otros 10 seg
  sleep(3);
  mundo->stop_graph(); 
  
  print_agents_status();
  delete mundo;

  return 0;
}


