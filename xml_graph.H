
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
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
# include <ah-graph-concepts.H>
# include <tpl_dynMapTree.H>
# include <optional>
# include <utility>

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
  template <AlephGraph GT,
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

    // Functors received as rvalues (including the defaults) are owned here, so
    // the references below never outlive them. Functors received as lvalues
    // are not copied: the references share them with the caller.
    std::optional<Node_Reader> own_node_reader;
    std::optional<Arc_Reader> own_arc_reader;
    std::optional<Node_Writer> own_node_writer;
    std::optional<Arc_Writer> own_arc_writer;

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
    /** @brief Build a reader/writer that shares the caller's functors.

        The functors are neither copied nor owned: they must outlive this
        object, and any state they accumulate is visible to the caller.

        @param[in] _node_reader reads the attributes of a node
        @param[in] _arc_reader reads the attributes of an arc
        @param[in] _node_writer writes the attributes of a node
        @param[in] _arc_writer writes the attributes of an arc
    */
    Xml_Graph(Node_Reader & _node_reader, Arc_Reader & _arc_reader,
              Node_Writer & _node_writer, Arc_Writer & _arc_writer)
      : graph_name("graph"), node_name("node"), arc_name("arc"),
        node_reader(_node_reader), arc_reader(_arc_reader),
        node_writer(_node_writer), arc_writer(_arc_writer)
    {
      // Empty
    }

    /** @brief Build a reader/writer that owns its functors.

        The functors (by default, the `Dft_*` ones) are moved into this
        object. Before, only a reference to these temporaries was kept, and
        it dangled as soon as the constructor returned.

        @param[in] _node_reader reads the attributes of a node
        @param[in] _arc_reader reads the attributes of an arc
        @param[in] _node_writer writes the attributes of a node
        @param[in] _arc_writer writes the attributes of an arc
    */
    Xml_Graph(Node_Reader && _node_reader = Node_Reader(),
              Arc_Reader && _arc_reader = Arc_Reader(),
              Node_Writer && _node_writer = Node_Writer(),
              Arc_Writer && _arc_writer = Arc_Writer())
      : graph_name("graph"), node_name("node"), arc_name("arc"),
        own_node_reader(std::move(_node_reader)),
        own_arc_reader(std::move(_arc_reader)),
        own_node_writer(std::move(_node_writer)),
        own_arc_writer(std::move(_arc_writer)),
        node_reader(*own_node_reader), arc_reader(*own_arc_reader),
        node_writer(*own_node_writer), arc_writer(*own_arc_writer)
    {
      // Empty
    }

    /** @brief Copy constructor.

        Owned functors are copied, so the copy does not refer to `other`'s;
        shared functors stay shared with the caller.

        @param[in] other reader/writer to copy
    */
    Xml_Graph(const Xml_Graph & other)
      : graph_name(other.graph_name), node_name(other.node_name),
        arc_name(other.arc_name),
        own_node_reader(other.own_node_reader),
        own_arc_reader(other.own_arc_reader),
        own_node_writer(other.own_node_writer),
        own_arc_writer(other.own_arc_writer),
        node_reader(own_node_reader ? *own_node_reader : other.node_reader),
        arc_reader(own_arc_reader ? *own_arc_reader : other.arc_reader),
        node_writer(own_node_writer ? *own_node_writer : other.node_writer),
        arc_writer(own_arc_writer ? *own_arc_writer : other.arc_writer)
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

