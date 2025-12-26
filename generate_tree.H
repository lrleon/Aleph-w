/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
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
# ifndef GENERATE_TREE_H
# define GENERATE_TREE_H

# include <fstream>
# include <tpl_tree_node.H>
# include <tpl_binNodeUtils.H>
 # include <ah-errors.H>

namespace Aleph
{

  template <class Node>
  struct Dft_Write
  {
    const string operator () (Node * p) 
    {
      return to_str(p->get_key());
    }
  };

/*
  Write debe exportar const string Write::operator () (Node *)

  Que se encarga de convertir el valor de clave contenido en el nodo a
  un string imprimible 
*/

  template <typename Node, class Write> static
  void __generate_tree(Node *       node, 
                       int            deway [], 
                       const size_t & current_level, 
                       const size_t & size,
                       std::ostream & output)
  { 
    ah_overflow_error_if(current_level >= size)
      << "Allocated size for deway array has been exceeded";

    // imprimir número de Deway del padre
    output << "Node ";
    for (size_t i = 0; i < current_level; ++i)
      {
        output << deway[i];

        if (i < current_level - 1)
          output << ".";
      }
    output << " \"" << Write () (node) << "\" " << std::endl;

    Node * child = (Node*) node->get_left_child(); 
    for (int i = 0; child != nullptr; 
         i++, child = (Node*)child->get_right_sibling())
      {
        deway[current_level + 1] = i;
        __generate_tree<Node, Write>
          (child, deway, current_level + 1, size, output);
      }
  }


  const size_t Max_Tree_Node_Depth = 1024;

  /** Genera una entrada para el programa de dibujado de árboles ntrepic.

      generate_tree(root, out, tree_number) genera una especificación de
      dibujado del árbol con raíz root para el programa de dibujado
      ntreepic. La salida es escrita en el archivo asociado a out.

      Para escribir el contenido del nodo, la rutina usa la clase
      parametrizada Write, cuyo operador Write()(nodo) transforma la
      clave contenida en el nodo a la cadena que se desea sea escrita
      como contenido del nodo.

      @param[in] root raíz del árbol a dibujar.
      @param[out] out archivo asociado donde se desea escribir la
      especificación de dibujado.
      @param[in] tree_number para uso interno \b NO \b USAR.

      @see generate_forest()

      @ingroup Arboles
  */
  template <typename Node, class Write = Dft_Write<Node>>
  void generate_tree(Node *         root, 
                     std::ostream & out, 
                     const int &    tree_number = 0)
  {
    out << "Root \"" << Write () (root) << "\" " << std::endl;

    int * deway = new int [Max_Tree_Node_Depth];

    const int level = 0; // Este es el nivel de partida

    deway[level] = tree_number; // nivel 0 en el número del árbol

    Node * child = (Node*) root->get_left_child(); 
    for (int i = 0; child != nullptr; 
         i++, child = (Node*)child->get_right_sibling())
      {
        deway[1] = i;
        __generate_tree<Node, Write>(child, deway, level + 1, 
                                     Max_Tree_Node_Depth, out);
      }

    delete [] deway;
  }

  /** Genera una entrada para el programa de dibujado de árboles ntreepic.

      generate_forest(root, out, tree_number) genera una especificación de
      dibujado de la arborescencia cuyo primer árbol es root para el
      programa de dibujado ntreepic. La salida es escrita en el
      archivo asociado a out. 

      Para escribir el contenido del nodo, la rutina usa la clase
      parametrizada Write, cuyo operador Write()(nodo) transforma la
      clave contenida en el nodo a la cadena que se desea sea escrita
      como contenido del nodo.

      @param[in] root raíz del árbol a dibujar.
      @param[out] out archivo asociado donde se desea escribir la
      especificación de dibujado.

      @see generate_tree()

      @ingroup Arboles
  */
  template <typename Node, class Write = Dft_Write<Node>>
  void generate_forest(Node * root, std::ostream & out)
  {
    Node * tree = root;

    for (int i = 0; tree != nullptr; i++, tree = tree->get_right_sibling())
      generate_tree<Node, Write>(tree, out, i);
  }


  /** Genera una entrada para el programa de dibujado de árboles
      binarios btrepic.

      Para escribir el contenido del nodo, la rutina usa la clase
      parametrizada Write, cuyo operador Write()(nodo) transforma la
      clave contenida en el nodo a la cadena que se desea sea escrita
      como contenido del nodo.

      @param[in] root raíz del árbol a dibujar.
      @param[out] out archivo asociado donde se desea escribir la
      especificación de dibujado.

      @see generate_tree()

      @ingroup Arboles
  */
  template <typename Node, class Write>
  void generate_btree(Node * root, std::ostream & out)
  { 
    out << "start-prefix ";
    For_Each_Preorder<Node> () (root, Write());
    out << endl
        << "start_infix ";
    For_Each_In_Order<Node> () (root, Write());
    out << endl;
  }

} // end namespace Aleph

# endif // GENERATE_TREE_H

