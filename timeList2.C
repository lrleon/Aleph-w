# include <List.H>
//# include <list.h>
# include <iostream>
# include <ahNow.H>

int main()
{
  double ret_val, promedio;

  Now now;
  
  for (int n_elem = 100; n_elem < 10000000; n_elem=n_elem*10)
    {
      promedio = 0;
      cout << endl << "Numero de elementos: " << n_elem;
 
      for (int rep = 0; rep < 10; rep++)
	{ 
	  now.start();
	  
	  list<int> c;

	  for (int i = 0; i < n_elem; i++)
	    {    
	      c.insert(i);  
	    } 
  
	  for (int j = n_elem; j > 0; j--)
	    {    
	      c.erase(j-1);
	    }

	  ret_val = now.delta();

	  promedio = promedio + ret_val;
	}

      cout << endl << "Tiempo Promedio es: " << promedio/10 << " segundos" << endl; 
    }
}
