
# include <math.h>
# include <stdlib.h>
# include <iostream>

using namespace std;

/*
  verificar si un numero es primo
*/
bool is_prime(const int & num)
{
  double raiz = static_cast<int>(ceil(sqrt(num)));

  for (int i = 2; i < raiz; ++i)
    {
      if (num % i == 0)
	return false;
    }

  return true;
}


int main(int, char * argv[])
{
  int n = atoi(argv[1]);

  if (is_prime(n))
    cout << "Es primo";
  else
    cout << "No es primo";
  
  cout << endl;
}
