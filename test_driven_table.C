
# include <iostream>
# include <driven_table.H>


void * f1(void*)
{
  cout << "f1" << endl;

  return NULL;
}

void * f2(void*)
{
  cout << "f1" << endl;

  return NULL;
}

int main()
{
  Dynamic_Event_Table table;

  table.register_event(f1);
  table.register_event(f2);

  cout << "Tabla tiene " << table.size() << " elementos" << endl;

  try
    {
      table.execute_event(0, NULL);
    }
  catch (exception & e)
    {
      cout << e.what() << endl;
    }
}
