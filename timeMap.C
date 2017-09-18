# include <ahNow.H>
# include <iostream>
# include <map>
# include <Map.H>
# include <Utility.H>
//# include <ah_stdc++_utils.H>

using namespace std;

int main()
{
  double ret_val, promedio;

  Now now;
  
  for (int n_elem = 100; n_elem < 100000000; n_elem=n_elem*10)
    {
      promedio = 0;
      cout << endl << "Numero de elementos: " << n_elem;
 
      for (int rep = 0; rep < 10; rep++)
	{ 
	  now.start();
	  map<int,int> c;

	  for (int i = 0; i < n_elem; i++)
	    {    
	      //std::pair<int,int> _pair(n_elem, n_elem);
	      pair<int,int> _pair(n_elem,n_elem);
	      c.insert(_pair);  
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
