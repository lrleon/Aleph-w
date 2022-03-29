

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

  split_key(new_root, value, t1_it, t2_it);

  inOrderRec(t1_it, print_node); 
  cout << "|" << value << "| ";
  inOrderRec(t2_it, print_node); 
  cout << endl << endl;

  if (not areEquivalents(t1_rec, t1_it))
    AH_ERROR("Lados izquierdos de las particiones no son iguales");

  if (not areEquivalents(t2_rec, t2_it))
    AH_ERROR("Lados derechos de las particiones no son iguales");

  cout << 
    "Resultado de la particion recursiva es identico a la particion iterativa" 
       << endl;

  if (t1_rec not_eq NULL)
    destroyRec(t1_rec);
  if (t2_rec not_eq NULL)
    destroyRec(t2_rec);

  if (t1_it not_eq NULL)
    destroyRec(t1_it);
  if (t2_it not_eq NULL)
    destroyRec(t2_it);

  {
    BinNode<int> * t_rot = NULL;
    DynArray<int> values(n);
    cout << "Insercion recursiva de " << n << " nodos en la raiz ..." << endl;

    for (i = 0; i < n; i++)
      {
	values[i] = 1+(int) (n*100.0*rand()/(RAND_MAX+1.0));
	node = searchInBinTree(t_rot, values[i]);
	if (node == NULL)
	  {
	    cout << values[i] << " ";
	    node = new BinNode<int> (values[i]);
	    t_rot = insert_root(t_rot, node);
	  }
      }

    assert(check_bst(t_rot));

    cout << endl << "Terminado" << endl;

    BinNode<int> * t_it = NULL;
    cout << "Insercion iterativa de " << n << " nodos en la raiz ..." << endl;

    for (i = 0; i < n; i++)
      {
	node = searchInBinTree(t_it, values[i]);
	if (node == NULL)
	  {
	    cout << values[i] << " ";
	    node = new BinNode<int> (values[i]);
	    t_it = insert_root(t_it, node);
	  }
      }

    assert(check_bst(t_it));

    cout << endl << "Terminado" << endl;

    cout << "Comparando resultado recursivo con el iterativo ... " << endl;
    if (areEquivalents(t_rot, t_it))
      cout << "Arboles resultantes son iguales" << endl;
    else
      cout << "Arboles resultantes son diferentes" << endl;

    {
      ofstream out("bintree.tree");
      save_tree(t_it, out);
    }

    ifstream input("bintree.tree");
    BinNode<int> * t_load = load_tree<BinNode<int>>(input);

    cout << "Comparando arbol cargado con el iterativo ... " << endl;
    if (areEquivalents(t_load, t_it))
      cout << "Arboles resultantes son iguales" << endl;
    else
      cout << "Arboles resultantes son diferentes" << endl;
    
    destroyRec(t_rot);
    destroyRec(t_it);
  }
}
