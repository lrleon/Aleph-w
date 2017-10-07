
# include <stdio.h>
# include <tpl_sort_utils.H>


# define NumItems 200

typedef Dnode<unsigned> List;

void printList(List & list)
{
  printf("\n");

  List::Iterator itor(list);
  Dnode<unsigned>* node;

  for (itor.reset_first(); itor.has_current(); itor.next())
    {
      node = itor.get_current();
      printf("%u ", node->get_data());
    }

  printf("\n");
}      


struct Menor_Que
{
  bool operator() (Dlink * l1, Dlink * l2) const
  {
    return static_cast<Dnode<unsigned>*>(l1)->get_data() <
      static_cast<Dnode<unsigned>*>(l2)->get_data();
  }
};

int main()
{
  printf("Starting ........\n\n");

  List list1;
  List list2;

  Dnode<unsigned>* node;

  unsigned long i;

  for (i = 0; i < NumItems; i++)
    {
      node = new Dnode<unsigned>;
      node->get_data() = i;
      list1.insert(node); 
      printf("%lu ", i);
    }

  printf("\n");

  for (i = 0; i < NumItems; i++)
    {
      node = new Dnode<unsigned>;
      node->get_data() = i;
      list2.append(node); 
      printf("%lu ", i);
    }

  List list3;

  merge_lists<List, Menor_Que>(list1, list2, list3);

  printList(list1); 
  printList(list2); 

  quicksort(list1);

  printf("Lists apparently merged and sorted\n");

  printList(list1); 

  list1.remove_all_and_delete();

  printf("Ending .... \n\n");
}
      

