
# include "tpl_dnode.H"



Dnode<int>* merge(Dnode<int>* l1, Dnode<int>* l2)
{
  Dnode<int> tmp, *p = &tmp;

  while (l1 != NULL && l2 != NULL)
    if (l1->get_data() <= l2->get_data())
      {
	p->get_next()  = l1; 
	l1->get_prev() = p;
	p             = l1; 
	l1            = l1->get_next();
      } 
    else 
      {
	p->get_next()  = l2; 
	l2->get_prev() = p;
	p             = l2; 
	l2            = l2->get_next();
      } 
  
  if (l1 == NULL)
    {
      p->get_next()  = l2;
      l2->get_prev() = p;
    }
  else
    {
      p->get_next()  = l1;
      l1->get_prev() = p;
    }
  return tmp.get_next();
}


Dnode<int>* mergeSort(Dnode<int>* list)
{
  Dnode<int> *l1, *l2;

  if (list->get_next() == NULL)
    return list;

  l1 = list;
  l2 = list->get_next();

  while  (l2 != NULL && l2->get_next() != NULL)
    {
      list = list->get_next();
      l2 = l2->get_next()->get_next();
    }

  l2              = list->get_next();
  l2->get_prev()   = NULL;
  list->get_next() = NULL;

  return merge( mergeSort(l1), mergeSort(l2) );  
}


void sortVector(Dnode<int>* vector[], int m, int n, Dnode<int>*& list)
{
  int i, j, l = n/m;
  Dnode<int> *p = list->get_next(), *first;

  list->get_prev()->get_next() = NULL;
  p->get_prev()           = NULL;

  first = p = mergeSort(p);

  for (i = 0; i < m; i++)
    {
      vector[i] = p;
      for (j = 0; j < l; j++, p = p->get_next());
    }

  while (p->get_next() != NULL)
    p = p->get_next();

  p->get_next()     = list;
  list->get_next()  = first;
  first->get_prev() = list;
}


# define N 100

# include <stdlib.h>
# include <iostream>

int main()
{
  int i, num;
  Dnode<int> *list = new Dnode<int>, *p;
  Dnode<int> *vector[3];

  for (i = 0; i < N; i++)
    {
      num = 1+(int) (1000.0*rand()/(RAND_MAX+1.0));
      p = new Dnode<int>;
      p->get_data() = num;
      list->insert(p);
      cout << num << " ";
    }      

  cout << "\n\n\n";
  
  sortVector(vector, 3, N, list);

  for (p = list->get_next(); p != list; p = p->get_next())
    {
      cout << p->get_data() << " ";
    }

  cout << "\n";
}
