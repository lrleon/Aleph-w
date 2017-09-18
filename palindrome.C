

# include <string>
# include <iostream>

using namespace std;

int main()
{
  string cadena;
  cout<<"INTRODUZCA UNA PALABRA (max 15 caracteres): ";
  cin >> cadena;
  cout << endl;

  int i;
  string inversa;

  cout << "Invirtiendo palabra .... ";
  for (i = 0; i < cadena.size(); i++)
    {
      cout << cadena[cadena.size() - i - 1];
      inversa += cadena[cadena.size() - i -1];
    }

  cout << " .... hecho!" << endl
       << "La palabra invertida es " << inversa << endl;

  if (inversa == cadena)
    cout << "Bravo: la palabra introducida es palindrome!" << endl;
  else
    cout << "Lo siento: la palabra introducida no es palindrome!" << endl;
}
