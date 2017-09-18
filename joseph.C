# include "tpl_dynDlist.H"

void avanceItor(DynDlist<unsigned>::Iterator& itor, unsigned s)
{
  for (unsigned i = 0; i < s; i++)
    {
      if (not itor.has_current())
        itor.reset_first();

      itor.next();
    }

  if (not itor.has_current())
    itor.reset_first();
}
void orden_ejecucion(unsigned num_personas, unsigned paso)
{
  DynDlist<unsigned> list;

  for (unsigned i = 1; i <= num_personas; i++)
    list.append(i);
      
  DynDlist<unsigned>::Iterator itor(list);
  for (/* nothing */; num_personas > 1; num_personas--)
    {
      avanceItor(itor, paso);
      printf("%u ", itor.get_current());
      itor.del();
    }

  printf("\nEl sobreviviente es %u\n", list.get_first());    
}

int main(int, char** args)
{
  unsigned numPer = atoi(args[1]),
           paso   = atoi(args[2]);

  orden_ejecucion(numPer, paso);
}
