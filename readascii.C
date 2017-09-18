# include <string>
# include <iostream>
# include <fstream>

using namespace std;


int main(int argn, char * argc[])
{
  char c;
  
  while (true)
    {
      cout << endl 
	   << "Ingrese un caracter: ";
      cin >> c;
      cout << endl
	   << "El caracter es: " << c << " (" << (int) c << ")" << endl;
    }
}
