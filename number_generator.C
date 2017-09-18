# include <gsl/gsl_rng.h>

# include <fstream>
# include <iostream>

using namespace std;

int main(int argc, char * argv[])
{
  if (argc < 4)
    {
      cout << "usage: ./number_generator n max output_file" 
	   << endl;
      return 1;
    }

  // Instanciacion del generador de numeros aleatorios e inicializacion
  // de la semilla  
  gsl_rng * rng = gsl_rng_alloc(gsl_rng_mt19937);
  gsl_rng_set(rng, time(NULL) % gsl_rng_max(rng));

  unsigned long long count = atoi(argv[1]);
  unsigned long long max = atoi(argv[2]);

  std::ofstream file(argv[3]);

  if (not file)
    {
      cout << "No se puede abrir el archivo " << argv[3] << endl;
      return 2;
    }

  for (unsigned long long i = 0; i < count; ++i)
    // Generacion de numeros aleatorios uniformes enteros entre 0 y 99
    file << gsl_rng_uniform_int(rng, max) << endl;

  file.close();

  gsl_rng_free(rng);
}

