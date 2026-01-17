
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file xml_graph.H
 *  @brief XML serialization for graphs.
 *
 *  Load and save graphs in XML format for interchange.
 *
 *  @see generate_graph.H DOT format export
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef XML_GRAPH_H
# define XML_GRAPH_H

# include <tpl_dynArray.H>
# include <tpl_graph.H>
# include <tpl_dynMapTree.H>

# include <libxml++/libxml++.h>
# include <libxml++/parsers/textreader.h>
# include <libxml++/nodes/node.h>
# include <libxml++/document.h>

using namespace Aleph;

namespace Aleph
{
  struct Attr
  {
    std::string name;
    std::string value;
  };

  template <class GT>
  struct Dft_Node_Reader
  {
    void operator () (GT &, typename GT::Node *, DynArray <Attr> &)
    {
      // Empty
    }
  };

  template <class GT>
  struct Dft_Node_Writer
  {
    void operator () (GT &, typename GT::Node *, DynArray <Attr> &)
    {
      // Empty
    }
  };

  template <class GT>
  struct Dft_Arc_Reader
  {
    void operator () (GT &, typename GT::Arc *, DynArray <Attr> &)
    {
      // Empty
    }
  };

  template <class GT>
  struct Dft_Arc_Writer
  {
    void operator () (GT &, typename GT::Arc *, DynArray <Attr> &)
    {
      // Empty
    }
  };

  /* TODO: le debo la explicación de los detalles de cada una de las clases de
     lectura y escritura
  */

  /** Clase que escribe y lee un grafo (de forma muy elemental) en XML.
   * 
   *  @param GT el tipo de grafo.
   *  @param Node_Reader la función de lectura de los atributos de un nodo.
   *  @param Arc_Reader la función de lectura de los atributos de un arco.
   *  @param Node_Writer la función de escritura de los atributos de un nodo.
   *  @param Arc_Writer la función de escritura de los atributos de un arco.
   *  @author Alejandro J. Mujica
   */
  template <class GT,
            class Node_Reader = Dft_Node_Reader<GT>,
            class Arc_Reader = Dft_Arc_Reader<GT>,
            class Node_Writer = Dft_Node_Writer<GT>,
            class Arc_Writer = Dft_Arc_Writer<GT>
           >
  class Xml_Graph
  {
    std::string graph_name;

    std::string node_name;

    std::string arc_name;

    Node_Reader & node_reader;

    Arc_Reader & arc_reader;

    Node_Writer & node_writer;

    Arc_Writer & arc_writer;

    GT read_graph(xmlpp::TextReader & reader)
    {
      GT g;
      DynMapTreap <size_t, typename GT::Node *> map;

      size_t num_nodes = 0;

      while(reader.read())
        {
          if (reader.get_name() == node_name)
            {
              typename GT::Node * p = g.insert_node();
              map.insert(num_nodes++, p);

              if (not reader.has_attributes())
                continue;

              reader.move_to_first_attribute();

              DynArray <Attr> attrs;

              do 
                {
                  Attr & attr = attrs.append();
                  attr.name = reader.get_name();
                  attr.value = reader.get_value();
                }
              while (reader.move_to_next_attribute());

              node_reader(g, p, attrs);

              reader.move_to_element();
           }

          else if (reader.get_name() == arc_name)
            {
              assert(reader.has_attributes());
              reader.move_to_first_attribute();
              size_t src = std::atol(reader.get_value().c_str());
              bool test = reader.move_to_next_attribute();
              assert(test);
              size_t tgt = std::atol(reader.get_value().c_str());
              test = reader.move_to_next_attribute();

              typename GT::Arc * a = g.insert_arc(map.find(src), map.find(tgt));

              if (not test)
                {
                  reader.move_to_element();
                  continue;
                }

              DynArray <Attr> attrs;

              do 
                {
                  Attr & attr = attrs.append();
                  attr.name = reader.get_name();
                  attr.value = reader.get_value();
                }
              while (reader.move_to_next_attribute());

              arc_reader(g, a, attrs);

              reader.move_to_element();
            }
        }
      return g;
    }

    GT read(const std::string & file_name)
    {
      xmlpp::TextReader reader(file_name);
      return read_graph(reader);
    }

    void write_graph(GT & g, xmlpp::Document & doc)
    {
      xmlpp::Element * element = doc.create_root_node(graph_name);

      xmlpp::Element * nodes = element->add_child("nodes");

      DynMapTreap <typename GT::Node *, size_t> map;

      size_t i = 0;

      for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne(), ++i)
        {
          typename GT::Node * p = it.get_curr();

          map.insert(p, i);

          xmlpp::Element * node = nodes->add_child(node_name);

          DynArray <Attr> attrs;

          node_writer(g, p, attrs);

          for (size_t i = 0; i < attrs.size(); ++i)
            {
              Attr & attr = attrs.access(i);
             node->set_attribute(attr.name, attr.value);
            }
        }

      xmlpp::Element * arcs = element->add_child("arcs");

      for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne(), ++i)
        {
          typename GT::Arc * a = it.get_curr();
        
          xmlpp::Element * arc = arcs->add_child(arc_name);

          const size_t & src = map.find(g.get_src_node(a));
          arc->set_attribute("src", std::to_string(src));

          const size_t & tgt = map.find(g.get_tgt_node(a));
          arc->set_attribute("tgt", std::to_string(tgt));

          DynArray <Attr> attrs;

          arc_writer(g, a, attrs);

          for (size_t i = 0; i < attrs.size(); ++i)
            {
              Attr & attr = attrs.access(i);
              arc->set_attribute(attr.name, attr.value);
            }
        }
    }

    void write(GT & g, const std::string & file_name)
    {
      xmlpp::Document doc;
      write_graph(g, doc);
      doc.write_to_file_formatted(file_name, "UTF-8");
    }

  public:
    Xml_Graph(Node_Reader & _node_reader, Arc_Reader & _arc_reader,
              Node_Writer & _node_writer, Arc_Writer & _arc_writer)
      : graph_name("graph"), node_name("node"), arc_name("arc"),
        node_reader(_node_reader), arc_reader(_arc_reader),
        node_writer(_node_writer), arc_writer(_arc_writer)
    {
      // Empty
    }

    Xml_Graph(Node_Reader && _node_reader = Node_Reader(),
              Arc_Reader && _arc_reader = Arc_Reader(),
              Node_Writer && _node_writer = Node_Writer(),
              Arc_Writer && _arc_writer = Arc_Writer())
      : graph_name("graph"), node_name("node"), arc_name("arc"),
        node_reader(_node_reader), arc_reader(_arc_reader),
        node_writer(_node_writer), arc_writer(_arc_writer)
    {
      // Empty
    }

    const std::string & get_graph_name() const
    {
      return graph_name;
    }

    void set_graph_name(const std::string & _graph_name)
    {
      graph_name = _graph_name;
    }
      
    const std::string & get_node_name() const
    {
      return node_name;
    }

    void set_node_name(const std::string & _node_name)
    {
      node_name = _node_name;
    }

    const std::string & get_arc_name() const
    {
      return arc_name;
    }

    void set_arc_name(const std::string & _arc_name)
    {
      arc_name = _arc_name;
    }

    GT operator () (const std::string & file_name)
    {
      return read(file_name);
    }

    void operator () (GT & g, const std::string & file_name)
    {
      write(g, file_name);
    }
  };
} // End namespace Aleph

# endif // XML_GRAPH_H

