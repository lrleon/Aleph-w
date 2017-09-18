# include <Vector.H> 
# include <iostream>
# include <ahNow.H> 

int main()
{
  double ret_val, promedio;

  Now now;
  
  for (int n_elem = 100; n_elem < 1000000000; n_elem = n_elem*10)
    {
      promedio = 0;
      cout << endl << "Numero de elementos: " << n_elem;
 
      for (int rep = 0; rep < 10; rep++)
	{ 
	  now.start();
	  Aleph::vector<int> c(n_elem);

	  for (int i = 0; i < n_elem; i++)
	    {    
	      c[i] = i;
	    } 

	    
	  for (int j = 0; j < n_elem; j++)
	    {    
	      c.pop_back();
	    }
	  c.clear();

	  ret_val = now.delta();

	  promedio = promedio + ret_val;
	}

      cout << endl << "Tiempo Promedio es: " << promedio/10 << " segundos" << endl; 
    }
}
