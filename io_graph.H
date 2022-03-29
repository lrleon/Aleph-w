

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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
 }

    /// Guarda el grafo en el stream output
    void save(ofstream & output)
    {
      // guarda cantidad de nodos
      const size_t num_nodes = g.get_num_nodes();

      if (verbose)
        cout << "Storing " << num_nodes << " nodes ... ";

      output.write((const char*) &num_nodes, sizeof(num_nodes));

      int i = 0; // contador de nodos. Lleva los índices en la tabla

      // tabla de pares <ptr nodo, orden de isnerción>
      DynMapTreap<typename GT::Node *, int> nodes_table;

      for (Node_Iterator<GT,NF> it(g, node_filter); it.has_curr();
           it.next_ne(), ++i)
        {
          auto p = it.get_curr_ne();

          if (verbose)
            cout << i << " ";

          store_node(output, g, p);

          nodes_table.insert(p, i);
        }

      const size_t num_arcs = g.get_num_arcs();

      if (verbose)
        cout << " done " << endl 
             << "Storing " << num_arcs << " arcs ... " << endl;

      output.write((const char*) &num_arcs, sizeof(num_arcs));

      for (Arc_Iterator<GT, AF> it(g, arc_filter); it.has_curr(); it.next_ne())
        {
          auto a = it.get_curr_ne();

          auto src = g.get_src_node(a);
          auto tgt = g.get_tgt_node(a);

          const auto & src_idx = nodes_table.find(src);
          const auto & tgt_idx = nodes_table.find(tgt);

          // guarda índices de nodos origen y destino respectivamente
          output.write((const char*) &src_idx, sizeof(int));
          output.write((const char*) &tgt_idx, sizeof(int));

          if (verbose)
            cout << " " << src_idx << "--" << tgt_idx << " ";

          store_arc(output, g, a);

          if (verbose)
            cout << endl;
        }

      if (verbose)
        cout << " done " << endl << endl;
    }

    /// Carga el grafo desde el stream output
    void load(ifstream & input)
    {
      // lee cantidad de nodos
      size_t num_nodes;
      input.read((char *) &num_nodes, sizeof(num_nodes));

      if (verbose)
        cout << "Loading " << num_nodes << " nodes ...";

      DynArray<typename GT::Node*> nodes_table(num_nodes);
      nodes_table.reserve(0, num_nodes - 1);

      for (size_t i = 0; i < num_nodes; ++i)
        {
          typename GT::Node * p = new typename GT::Node;

          if (verbose)
            cout << " " << i;

          load_node(input, g, p);

          p = g.insert_node(p);
          nodes_table.access(i) = p;
        }

      size_t num_arcs;
      input.read((char *) &num_arcs, sizeof(num_arcs));

      if (verbose)
        cout << " done " << endl
             << "Loading " << num_arcs << " arcs ... " << endl;

      for (int i = 0; i < num_arcs; ++i)
        {
          int src_idx;
          input.read((char*) &src_idx, sizeof(int));
          auto src = nodes_table.access(src_idx);

          int tgt_idx;
          input.read((char*) &tgt_idx, sizeof(int));
          auto tgt = nodes_table.access(tgt_idx);

          auto a = g.insert_arc(src, tgt);

          if (verbose)
            cout << " " << src_idx << "--" << tgt_idx << " ";

          load_arc(input, g, a);

          if (verbose)
            cout << endl;
        }

      if (verbose)
        cout << " done " << endl << endl;
    }

    /// Guarda el grafo en el stream output
    void save_in_text_mode(ostream & output)
    {        // guarda cantidad de nodos
      const size_t num_nodes = g.get_num_nodes();
      const size_t num_arcs  = g.get_num_arcs();
      output << num_nodes << endl
             << num_arcs << endl;

      if (verbose)
        cout << "Storing " << num_nodes << " nodes ... ";

      int i = 0; // contador de nodos. Lleva los índices en la tabla

      // tabla de pares <ptr nodo, orden de inserción>
      DynMapTreap<typename GT::Node *, int> nodes_table;

      for (Node_Iterator<GT,NF> it(g, node_filter); it.has_curr();
           it.next_ne(), ++i)
        {
          typename GT::Node * p = it.get_curr_ne();

          if (verbose)
            cout << i << " ";

          store_node(output, g, p);

          nodes_table.insert(p, i);
        }

      if (verbose)
        cout << " done " << endl 
             << "Storing " << num_arcs << " arcs ... " << endl;

      for (Arc_Iterator<GT, AF> it(g, arc_filter); it.has_curr(); it.next_ne())
        {
          auto a = it.get_curr_ne();

          auto src = g.get_src_node(a);
          auto tgt = g.get_tgt_node(a);

          const auto & src_idx = nodes_table.find(src);
          const auto & tgt_idx = nodes_table.find(tgt);

          // guarda índices de nodos origen y destino respectivamente
          output << src_idx << " " << tgt_idx << " ";

          if (verbose)
            cout << " " << src_idx << "--" << tgt_idx << " ";

          store_arc(output, g, a);

          if (verbose)
            cout << endl;
        }

      if (verbose)
        cout << " done " << endl << endl;
    }

    /// Carga el grafo desde el stream intput
    void load_in_text_mode(istream & input)
    {
      // lee cantidad de nodos
      size_t num_nodes;
      size_t num_arcs;

      input >> num_nodes >> num_arcs;
      input.ignore();

      if (verbose)
        cout << "Loading " << num_nodes << " nodes ...";

      DynArray<typename GT::Node*> nodes_table(num_nodes);
      nodes_table.reserve(0, num_nodes - size_t(1));

      for (size_t i = 0; i < num_nodes; ++i)
        {
          auto p = new typename GT::Node;

          if (verbose)
            cout << " " << i;

          load_node(input, g, p);

          p = g.insert_node(p);
          nodes_table.access(i) = p;
        }

      if (verbose)
        cout << " done " << endl
             << "Loading " << num_arcs << " arcs ... " << endl;

      for (int i = 0; i < num_arcs; ++i)
        {
          int src_idx;
          int tgt_idx;

          input >> src_idx >> tgt_idx;

          auto src = nodes_table.access(src_idx);
          auto tgt = nodes_table.access(tgt_idx);
          auto a = g.insert_arc(src, tgt);

          if (verbose)
            cout << " " << src_idx << "--" << tgt_idx << " ";

          load_arc(input, g, a);

          if (verbose)
            cout << " " << endl;
        }

      if (verbose)
        cout << " done " << endl << endl;
    }
  };

} // end namespace Aleph

# endif // IO_GRAPH_H
