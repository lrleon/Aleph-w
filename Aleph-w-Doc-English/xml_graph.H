
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
  @file xml_graph.H
  @date 2002 - 2014
  @author Alejandro Mujica (Source Code), Julio Criollo (Documentation)
*/

# ifndef XML_GRAPH_H
# define XML_GRAPH_H

# include <tpl_dynArray.H>
# include <tpl_graph.H>
# include <tpl_dynMapTree.H>

# include <autosprintf.h>

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
      /* Empty */
    }
  };

  template <class GT>
  struct Dft_Node_Writer
  {
    void operator () (GT &, typename GT::Node *, DynArray <Attr> &)
    {
      /* Empty */
    }
  };

  template <class GT>
  struct Dft_Arc_Reader
  {
    void operator () (GT &, typename GT::Arc *, DynArray <Attr> &)
    {
      /* Empty */
    }
  };

  template <class GT>
  struct Dft_Arc_Writer
  {
    void operator () (GT &, typename GT::Arc *, DynArray <Attr> &)
    {
      /* Empty */
    }
  };


/** 
  @brief Class that writes and reads a graph (very basic form) XML.
  
  @param GT Graph type.
  @param Node_Reader Reading function attributes of a node.
  @param Arc_Reader Reading function attributes arc.
  @param Node_Writer Write function of the attributes of a node.
  @param Arc_Writer Write function of the attributes of an arc.
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

      for (typename GT::Node_Iterator it(g); it.has_current(); it.next(), ++i)
        {
          typename GT::Node * p = it.get_current();

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

      for (typename GT::Arc_Iterator it(g); it.has_current(); it.next(), ++i)
        {
          typename GT::Arc * a = it.get_current();
        
          xmlpp::Element * arc = arcs->add_child(arc_name);

          const size_t & src = map.find(g.get_src_node(a));
          std::string s = gnu::autosprintf("%ld", src);
          arc->set_attribute("src", s);

          const size_t & tgt = map.find(g.get_tgt_node(a));
          std::string t = gnu::autosprintf("%ld", tgt);
          arc->set_attribute("tgt", t);

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
      /* Empty */
    }

    Xml_Graph(Node_Reader && _node_reader = Node_Reader(),
              Arc_Reader && _arc_reader = Arc_Reader(),
              Node_Writer && _node_writer = Node_Writer(),
              Arc_Writer && _arc_writer = Arc_Writer())
      : graph_name("graph"), node_name("node"), arc_name("arc"),
        node_reader(_node_reader), arc_reader(_arc_reader),
        node_writer(_node_writer), arc_writer(_arc_writer)
    {
      /* Empty */
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
} /* End namespace Aleph */

# endif // XML_GRAPH_H
