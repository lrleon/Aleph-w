
/*
  This file is part of Aleph system

  Copyright (c) 1999, 2000, 2001, 2002, 2003, 2004
  UNIVERSITY LOS ANDES (ULA) Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA  

  - Center of Studies in Microelectronics & Distributed Systems (CEMISID) 
  - ULA Computer Science Department

  PERMISSION TO USE, COPY, MODIFY AND DISTRIBUTE THIS SOFTWARE AND ITS
  DOCUMENTATION IS HEREBY GRANTED, PROVIDED THAT BOTH THE COPYRIGHT
  NOTICE AND THIS PERMISSION NOTICE APPEAR IN ALL COPIES OF THE
  SOFTWARE, DERIVATIVE WORKS OR MODIFIED VERSIONS, AND ANY PORTIONS
  THEREOF, AND THAT BOTH NOTICES APPEAR IN SUPPORTING DOCUMENTATION.

  Aleph is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  UNIVERSIDAD DE LOS ANDES requests users of this software to return to 

  Leandro Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala Este
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  lrleon@ula.ve

  any improvements or extensions that they make and grant Universidad 
  de Los Andes (ULA) the rights to redistribute these changes. 

  Aleph is (or was) granted by: 
  - Consejo de Desarrollo Cientifico, Humanistico, Tecnico de la ULA 
    (CDCHT)
  - Consejo Nacional de Investigaciones Cientificas y Tecnologicas 
    (CONICIT)
*/

# include <List.H>
# include <Multiset.H> 
# include <iostream>
# include <ahNow.H>

using namespace std;

int main(int argn, char *argc[])
{
  int n = argc[1] ? atoi(argc[1]) : 10000000;
  
  unsigned int t = time(0);
  
  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << "#" << argc[0] << " " << n << " " << t << endl;

  double ret_val, menor, mayor, promedio, inserta, elimina;

  unsigned int elementos;

  cout << endl << "n_elem \t menor \t mayor \t promedio \t inserta \t elimina \t mediana" << endl;
  
  for (int n_elem = 100; n_elem < n; n_elem = n_elem*10)
    {
      menor = 1000000000;
      mayor = 0;
      promedio = 0;
      multiset<double> mediana;
      
      
      for (int rep = 0; rep < 11; rep++)
	{ 
	  auto ret_val_tp = Now::current_time_point();

	  list<int> c;

	  auto inserta_tp = Now::current_time_point();

	  for (int i = 0; i < n_elem; i++)
	    {    
	      c.push_front(i);
	      c.push_back(i);
	    } 

	  inserta = Now::delta(inserta_tp);

	  elementos = c.size();

	  auto elimina_tp = Now::current_time_point();
  
	  for (int j = n_elem; j > 0; j--)
	    {    
	      c.erase(c.begin());
	    }

	  elimina = Now::delta(elimina_tp);

	  ret_val = Now::delta(ret_val_tp);

	  if (ret_val < menor)
	    menor = ret_val;

	  if (ret_val > mayor)
	    mayor = ret_val;

	  promedio = promedio + ret_val;

	  mediana.insert(ret_val);
	}

      multiset<double>::iterator media(mediana.begin());

      for(int n = 0; n < 6; n++)
	media++;

cout << elementos << "\t" << menor << "\t" << mayor << "\t" << promedio/11 << "\t" << inserta << "\t" << elimina << "\t" << *media << endl;      }
}
