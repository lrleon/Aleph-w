
# include <string.h>
# include <iostream>
# include <fstream>
# include <tpl_sort_utils.H>
# include <tpl_dynSetTree.H>

const size_t Max_Size = 10; // Máximo tamaño de una permutación

DynArray<string> perms[Max_Size]; // guarda todas las permutaciones de
				  // un conjunto de caracteres. perms[i]
				  // contiene las permutaciones de
				  // tamaño i+1

// diccionario en memoria para validar las  permutaciones. Usamos un
// árbol para facilitar que las palabras del archivo puedan estar
// desordenadas y así alguien lo mejore
DynSetAvlTree<string> dic;


    // lee el diccionario del archivo file_name y lo carga a dic
void construir_diccionario(char * file_name)
{
  ifstream words_file(file_name, ios::in);

  if (not words_file.is_open())
    EXIT("No se puede abrir el diccionario %s", file_name);

  string word;

   while (not words_file.eof())
    {
      words_file >> word;

      dic.insert(word);

      char last_char = word[word.size() - 1];
      
          // si el último simbolo es una vocal, la pluralizamos a ver si
          // con suerte tenemos otra palabra
      if (last_char == 'a' or last_char == 'e' or last_char == 'i' or 
	  last_char == 'o' or last_char == 'u')
	{
	  dic.insert(word + "s"); // inserta el plural (suerte!)
	  continue;
	}

      char penultimate = word[word.size() - 2];

          // si termina en r puede ser un verbo; así que la eliminamos y con
          // mucha probabilidad obtendremos algunas conjugaciones
          // concatenandole caracteres como s, d, n, etc
      if (last_char == 'r') 
	{     
	  string without_r = word.erase(word.size() - 1, word.size() - 1);

	  dic.insert(without_r); 

	      // si termina en er o ar añadimos sufijo con n y s
	  if (penultimate == 'a' or penultimate == 'e') 
	    {
	      dic.insert(without_r + "n");
	      dic.insert(without_r + "s");
	      dic.insert(without_r + "d");
	    }	    
	}
    }
}

    // retorna true si perm es uma permutación valida de los símbolos en
    // charset 
bool is_a_permutation(const string & perm, char * charset, const size_t & sz)
{
  assert(sz <= Max_Size);

  char buffer[Max_Size + 1]; 
  
  strncpy(buffer, charset, Max_Size); // copia contunto a buffer de manera tal
				      // de marcar presencia o ausencia

      // recorre cada símbolo de la permutación
  for (const char * pos = perm.c_str(); *pos != '\0'; ++pos)
    {
      int i = 0;

      for (; i < sz; ++i) // busca *pos en buffer
	if (*pos == buffer[i])
	  {
	    buffer[i] = '\0';

	    break; // se encontró el carácter
	  }
      
      if (i == sz) 
	return false; // no se encontró *pos en charset ==> inválida
    }

  return true;
}

    // guarda permutación perm de tamaño sz, del conjunto charset, en array
void save_perm(DynArray<string> & array, const string & perm, 
	       char * charset, const size_t & sz)
{
  const size_t array_size = array.size();

  if (array_size > 0)
    {
      int i = binary_search(array, perm); // verifica si la
					  // permutación no ha sido
					  // generada previamente

      if (array.access(i) == perm)
	return; // la permutación ya fue generada (hay símbolos repetidos)
    }

  if (is_a_permutation(perm, charset, sz))
    array[array_size] = perm;
}

    // Calcula todas las permutaciones posibles de tamaño n con los caracteres
    // contenidos en str
void generar_permutaciones(char * charset, const size_t & n)
{
  const size_t charset_size = strlen(charset);

      // construye permutaciones de tamaño 1
  for (char buf[2], * s = charset; *s != '\0';)
    {
      buf[0] = *s++; buf[1] = '\0'; 

      string perm(buf);

      save_perm(perms[0], perm, charset, charset_size);
    }

  for (int i = 1; i < n; ++i) // i indica el tamaño de la permutación
    {
      for (char * s = charset; *s != '\0'; ++s)
	{
	  const size_t num_perms = perms[i-1].size(); 

	  DynArray<string> & array = perms[i];

	  for (int j = 0; j < num_perms; ++j)
	    {
	      const string & perm = perms[i-1].access(j);
	      save_perm(array, *s + perm, charset, charset_size);
	    }
	}
    }
}

    // toma las permutaciones entre [3,sz], las pasa por el diccionario e
    // imprime aquellas que se encuentren
void imprimir_permutaciones_pertenecientes_a_diccionario(const size_t & sz)
{
  cout << endl << endl << endl;

  for (int i = sz - 1; i >= 2; --i)
    {
      DynArray<string> & array = perms[i];
      
      const size_t size = array.size();

      cout << "|" << i - 1 << "|: ";

      for (int j = 0; j < size; ++j)
	{
	  const string & perm = array.access(j);

	  if (dic.exist(perm))
	    cout << perm << " ";
	}

      cout << "=" << endl;
    }

  for (int i = 0; i < sz; ++i)
    perms[i].cut();
}

int main(int argn, char * argc[])
{
  if (argn != 2)
    EXIT("Uso es wordch nombre del diccionario");

  construir_diccionario(argc[1]);

  while (true)
    {
      cout << "Ingrese letras: ";

      string letras;

      cin >> letras; cout << endl;

      char buffer[Max_Size + 1];

      strncpy(buffer, letras.c_str(), Max_Size);

          // ojo: hay que ordenar para que las permutaciones se generen
          // ordenadas, se concatenen en los arreglos y se pueda usar la
          // búsqueda binaria
      insertion_sort(buffer, 0, letras.size() - 1);
      
      generar_permutaciones(buffer, letras.size());

      imprimir_permutaciones_pertenecientes_a_diccionario(letras.size());
    }
}
