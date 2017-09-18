# include <tpl_skipList.H>
# include <stdlib.h>
# include <time.h>
# include <iostream>

using namespace std;

typedef SkipList<long,long>       LongSkipList;
typedef SkipList<long,long>::Node LongSkipListNode;


double doubleRand()
{
  return rand()/(RAND_MAX+1.0);
}

void DeleteSkipNodes(LongSkipListNode* node )
{

  while( node != NULL )
    {
      delete node; 
      node = node->get_next();
     }   
}


template <>
long SkipList<long, long>::Node::computeMaxKey()
{
  return LONG_MAX; 
}


int main(int argn, char *argc[])
{
  unsigned long seed, n, val, i = 0;

  
  if ( argn < 2 )
    {
      cout << "Usage: testSkipList [number of nodes] <[seed]> " << endl;
      return 0;
    }

  n    = atol(argc[1]);


  if (argn > 2 )
    seed = atol(argc[2]);
  else
    seed = time(NULL);
 
   srand( seed );
   
  cout << "./testSkipList " << n << " " << seed << endl;
  cout << "Inserting..." << endl;

  LongSkipListNode  *node;

  unsigned long keys[n];

  LongSkipList list(doubleRand);

  for( i = 0; i < n; i++)
    {
      val =  1+(unsigned long) ((n*20.0)*rand()/(RAND_MAX+1.0));
      
      int level = list.generateRandomLevel();

      assert( ( (sizeof(LongSkipListNode*)*level) & 1) == 0 );

//       node = NewSize(LongSkipListNode, sizeof(LongSkipListNode)*level)
//                      (val, val, level);
      
      node = list.new_node();

      keys[i] = val;
      list.insert( node );
      cout << "(" << val << ")";
    }

  cout << endl << "Searching..." << endl;

  for( i = 0; i < n; i++)
    {
      node = list.search(keys[i]);
      if (node == NULL)
	{
	  cout << "Error: Key " << keys[i] << " not found! " << endl;
          DeleteSkipNodes( list.get_first() ); 
	  exit( 0 );
	}
    }  

  cout << "Ok! " << endl;

  cout << endl << "Deleting..." << endl;
  
  for( i = 0; i < n; i++)
    {
      val    =  1+(unsigned long) ((n*20.0)*rand()/(RAND_MAX+1.0));
       node = list.remove(val);
      if ( node == NULL )
	{
	  cout << ".";
	  continue;
	}      
      cout << "(" << val << ")";
      delete node;
    }
 
  cout << endl;

  DeleteSkipNodes( list.get_first() ); 

 return 0;

}






































