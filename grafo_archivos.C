#include <cstdlib>
#include <string>
#include <fstream>
#include <iostream>
#include <tpl_graph.H>

/**

   Errores:

   1-. Operador () de Filter pasa al grafo por valor y no por referencia


*/

using namespace std;

typedef List_Graph < Graph_Node <string>, Graph_Arc<int> > GT;

template <class GT>
	class Filter
	{
		public:

		bool operator()(GT, typename GT::Arc * a)
		{
			return (a->get_info() > 15);
		}

	  bool operator()(typename GT::Arc * a)
		{
			return (a->get_info() > 15);
		}
	};

int main()
{
	GT g;
	ifstream in;
	//Lectura de e insercion nodos
	in.open("nodos.in", ios::in);
	while(!in.eof())
	{
		string city;
		in >> city;
		if(!in.eof())
		{
			GT::Node * node = new GT::Node(city);
			g.insert_node(node);
		}
	}
	in.close();

	//Lectura e insercion de arcos
	in.open("arcos.in", ios::in);
	while(!in.eof())
	{
		string src_city, tgt_city;
		int length;
		in >> src_city >> tgt_city >> length;
		if(!in.eof())
		{
			GT::Node * src = g.find_node(src_city);
			GT::Node * tgt = g.find_node(tgt_city);
			g.insert_arc(src, tgt, length);
		}
	}

	for(Arc_Iterator< GT, Filter<GT> > it(g); it.has_current(); it.next())
	{
		GT::Arc * a = it.get_current();
		cout << g.get_src_node(a)->get_info() << " -- " << g.get_tgt_node(a)->get_info() << " -- " << a->get_info() << endl;
	}
	exit(EXIT_SUCCESS);
}

