

# include <Map.H>
# include <Multiset.H> 
# include <iostream>
# include <ahNow.H>

using namespace std;

int main(int, char *argc[])
{
  int n = argc[1] ? atoi(argc[1]) : 100000000;

  cout << "#" << argc[0] << " " << n << endl;

  double ret_val, menor, mayor, promedio, inserta, elimina;

  unsigned int elementos;

  cout << endl << "n_elem \t menor \t mayor \t promedio \t inserta \t elimina \t mediana" << endl;
  
  for (int n_elem = 100; n_elem < n; n_elem=n_elem*10)
    {
      menor = 1000000000;
      mayor = 0;
      promedio = 0;
      multiset<double> mediana;
      
 
      for (int rep = 0; rep < 11; rep++)
	{ 
	  auto ret_val_tp = Now::current_time_point();
	  map<int,int> c;

	  auto inserta_tp = Now::current_time_point();

	  for (int i = 0; i < n_elem; i++)
	    {    
	      c.insert(std::pair<int,int>(i, n_elem));
	    } 

	  inserta = Now::delta(inserta_tp);
	    
	  elementos = c.size();

	  auto elimina_tp = Now::current_time_point();
  
	  for (int j = n_elem; j > 0; j--)
	    {    
	      c.erase(j-1);
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
      
      cout << elementos << "\t" << menor << "\t" << mayor << "\t" << promedio/11 << "\t" << inserta << "\t" << elimina << "\t" << *media << endl;
    }
}
