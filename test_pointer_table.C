
# include <iostream>
# include <pointer_table.H>

using namespace std;

# define NUM_PTR 100000

void * random_pointer() 
{
  long ret_val = rand();

  while (ret_val == 0)
    ret_val = rand();

  return reinterpret_cast<void*>(ret_val);
}

long random_index()
{
  return (long) (NUM_PTR*1.0*rand()/(RAND_MAX+1.0));
}


int main(int argn, char * argc[])
{
  if (argn > 1)
    srand(atoi(argc[1]));
  else
    srand(time(0));

  try
    {
      DynArray<void*> pointers;
      Pointer_Table table;
      long r;
      int i;
      int dups = 0;
      int outs = 0;
      
      for (i = 0; i < NUM_PTR; i++)
	{
	  pointers[i] = random_pointer();
	  table.insert_pointer(pointers[i]);
	}

      for (i = 0; i < NUM_PTR; i++)
	{
	  r = random_index();
	  
	  assert(table.verify_pointer(r, pointers[r]) == pointers[r]);
	}

      for (i = 0; i < NUM_PTR; i++)
	{
	  r = random_index();

	  try
	    {
	      table.remove_pointer(r);
	      pointers[r] = NULL;
	    }
	  catch (domain_error)
	    { 
	      dups++;
	      assert(pointers[r] == NULL);
	    }
	  catch (range_error)
	    {
	      outs++;
	    }
	}

      cout << endl
	   << "Primera parte del test" << endl
	   << "  Se realizaron " << NUM_PTR << " inserciones" << endl
	   << "  Se eliminaron " << NUM_PTR << " indices aleatoriamente" 
	   << endl
	   << "    de los cuales " << dups << " índices fueron duplicados" 
	   << endl
	   << "    y " << outs << " estuvieron fuera de rango" << endl
	   << "  Size   = " << table.size() << endl
	   << "  Busies = " << table.busies() << endl
	   << "  Frees  = " << table.frees() << endl
	   << endl;

      int ins_counter = 0;
      for (i = 0; i < NUM_PTR; i++)
	{
	  if (pointers[i] == NULL)
	    {
	      pointers[i] = random_pointer();
	      table.insert_pointer(pointers[i]);
	      ins_counter++;
	    }
	}

      dups = 0;
      outs = 0;
      int del_counter = 0;
      for (i = 0; i < NUM_PTR; i++)
	{
	  r = random_index();


	  try
	    {
	      table.remove_pointer(r);
	      del_counter++;
	      pointers[r] = NULL;
	    }
	  catch (domain_error)
	    { 
	      dups++;
	      assert(pointers[r] == NULL);
	    }
	  catch (range_error)
	    {
	      outs++;
	    }
	}

      cout << endl
	   << "Segunda parte del test" << endl
	   << "  Se realizaron " << ins_counter << " inserciones" << endl
	   << "  Se intentaron " << NUM_PTR << " indices aleatoriamente" 
	   << endl
	   << "    de los cuales " << dups << " índices fueron duplicados" 
	   << endl
	   << "    " << outs << " estuvieron fuera de rango" << endl
	   << "    y " << del_counter << " fueron eliminados " << endl
	   << "  Size   = " << table.size() << endl
	   << "  Busies = " << table.busies() << endl
	   << "  Frees  = " << table.frees() << endl
	   << endl;

      cout << "Probando acceso fuera de rango ..." << endl;

      for (i = NUM_PTR - 1; i >= 0; i--)
	{
	  try
	    {
	      table.verify_pointer(i, pointers[i]);
	      break;
	    }
	  catch (range_error) {}
	  catch (domain_error) {}
	}

      cout << "Heap detectado en " << i + 1 << endl;
    }
  catch (exception & e)
    {
      cout << "Excepción inesperada: " << e.what() << endl;
    }
}
