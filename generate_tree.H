# ifndef GENERATE_TREE_H
# define GENERATE_TREE_H

# include <fstream>
# include <tpl_tree_node.H>
# include <tpl_binNodeUtils.H>

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
void __generate_tree(Node *          node, 
		     int             deway [], 
		     const size_t &  current_level, 
		     const size_t &  size,
		     std::ostream & output)
{ 
  if (current_level >= size)
    throw std::overflow_error("Allocated size for deway array has been "
			      "exceeded");

      // imprimir número de Deway del padre
  output << "Node ";
  for (int i = 0; i < current_level; ++i)
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

