
# include <iostream>
# include <tpl_dnode.H>

typedef Dnode<int> Test;

using namespace std;

void print_list(Test * link)
{
  Test* ptr;
  for (Test::Iterator it(link); it.has_current(); it.next())
    {
      ptr = it.get_current(); 
      cout << ptr->get_data() << " ";
    }
  cout << endl;
}


void create_random_list(Test * link, int n)
{
  Test* testPtr;
  for (int i = 0; i < n; i++)
    {
      testPtr = new Test (1 + (int) (10.0*n*rand()/(RAND_MAX+1.0)));
      link->append(testPtr);
    }
}

int main(int argn, char *argc[])
{
  int n = argc[1] ? atoi(argc[1]) : 1000;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  Test* testPtr;
  Test* headPtr;
  Test* tmpPtr;
  Test  head;

  headPtr = &head;

  unsigned long i;

  for (i = 0; i < n; i++)
    {
      testPtr = new Test;

      testPtr->get_data() = i;
      headPtr->insert(testPtr);
      cout << i << " ";
    }

  cout << endl;

  for (i = 0, testPtr = headPtr->get_next(); i < n; 
       i++, testPtr = testPtr->get_next())
    cout << testPtr->get_data() << " ";

  cout << endl;

  for (testPtr = headPtr->get_prev(); testPtr != headPtr; 
       testPtr = testPtr->get_prev())
    cout << testPtr->get_data() << " ";

  cout << endl;

  while (not headPtr->is_empty())
    {
      testPtr = headPtr->get_next();
      cout << testPtr->get_data() << " ";
      headPtr->remove_next();
      delete testPtr;
    }

  cout << endl;

  for (i = 0; i < n; i++)
    {
      testPtr = new Test;

      testPtr->get_data() = i;
      headPtr->insert(testPtr);
      cout << i << " ";
    }

  cout << endl;

  for (testPtr = headPtr->get_next(); testPtr not_eq headPtr;)
    {
      tmpPtr = testPtr;
      testPtr = testPtr->get_next();
      cout << testPtr->get_data() << " ";
      tmpPtr->del();
      delete tmpPtr;
    }

  cout << endl;

  for (i = 0; i < n; i++)
    {
      testPtr = new Test;

      testPtr->get_data() = i;
      headPtr->insert(testPtr);
      cout << i << " ";
    }

  for (Dnode<unsigned>::Iterator itor(headPtr); itor.has_current(); )
    delete itor.del();

  assert(headPtr == headPtr->get_prev() && headPtr == headPtr->get_next());

  cout << endl << endl;

  Test list;

  create_random_list(&list, n);

  print_list(&list);

  print_list(&list);

  list.remove_all_and_delete();

  printf("Ending .... \n\n");
}
      

