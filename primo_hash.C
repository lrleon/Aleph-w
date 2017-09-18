
# include <iostream>
# include <tpl_dynArray.H>

using namespace std;

int n = 7;
int primes[ /* 7 */ ] = {11, 13, 17, 23, 29, 31, 37, 0 };


int numbers[] = { 14, 22, 39, 46, 54, 79, 198, 0 };

int prime_hash(int keys[])
{
  int prime_index = 0;
  int current_prime = primes[0];
  int current_index = 0;
  int i;

  DynArray<int> hashes(100);

  while (true)
    {
    de_nuevo:
      hashes[current_index] = keys[current_index] % current_prime;

      cout << "Probando para primo " << current_prime << " y clave "
	   << keys[current_index] << " (resto = " << hashes[current_index]
	   << ")" << endl;

      /* Busca si existe una colision */
      i = 0; 
      while (i < current_index)
	{
	  if (hashes[current_index] == hashes[i])
	    {
	      cout << "    Colision de " << keys[current_index] << " con " 
		   << keys[i] << endl;
	      current_prime = primes[++prime_index];
	      if (current_prime == 0)
		{
		  cout << "No hay solucion" << endl;
		  return -1;
		}
	      else
		{
		  cout << "    Avanzando a primo " << current_prime << endl;
		  current_index = 0;
		  goto de_nuevo;
		}
	    }		  
	  i++;
	}

      /* no hubo colision, avance a la proxima clave */
      current_index++;
      if (keys[current_index] == 0)
	{
	  cout << "Encontrado numero primo = " << current_prime << endl;
	  for (i = 0; i < current_index; i++)
	    cout << keys[i] << " mod " << current_prime << " = " 
		 << keys[i] % current_prime << endl;
	  cout << endl;
	  return current_prime;
	}
    }
}


int main()
{
  prime_hash(numbers);
}
