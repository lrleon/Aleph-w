
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
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
      auto ptr = node;
      node = node->get_next();
      delete ptr; 
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






































