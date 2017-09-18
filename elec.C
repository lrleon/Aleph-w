# include <string>
# include <tpl_dnode.H>

struct Profesor
{
  string nombres;
  string apellidos;
  int id;
};

struct Plancha
{
  string nombre;
  int id;
  Dnode<Profesor*> lista_candidatos;
};

DynArray<Profesor> profesores;



