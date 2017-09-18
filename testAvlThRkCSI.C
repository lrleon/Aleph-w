  
# include <stdlib.h>
# include <time.h>
# include <iostream>
# include <aleph.H>
# include <tpl_avlTreeThRkC.H>
# include <tpl_threadNodeUtils.H>

using namespace std;
using namespace ThreadNode_Utils;

long long int sumaPorHilos = 0;
int cont = 0;
long long int sumaInOrderRec = 0; 

static void print(AvlTreeThRk<int>::Node *p) 
{
  cout << "[" << p->get_key() << "]";
}


int main(int argn, char *argc[])
{
  cout << "testAvlThRkTree ";

  AvlTreeThRk<int> treei1,treei2;
  AvlTreeThRk<int>::Node *node;
  AvlTreeThRk<int> treed1,treed2;
  int i;
  for (i = 1; i <= 33; i+=2)
    {  
      node = new AvlTreeThRk<int>::Node (i);
      treei1.insert(node);
      assert(treei1.verifyRank());
      assert(treei1.verifyAvl());
      node = new AvlTreeThRk<int>::Node (i);
      treei2.insert(node);
    }
  for (i = 22; i <= 52; i+=2)
    {  
      node = new AvlTreeThRk<int>::Node (i);
      treed1.insert(node);
      node = new AvlTreeThRk<int>::Node (i);
      treed2.insert(node);
    }
  
  node = new AvlTreeThRk<int>::Node (3);
  treed2.insert(node);
  node = new AvlTreeThRk<int>::Node (9);
  treed2.insert(node);
  node = new AvlTreeThRk<int>::Node (13);
  treed2.insert(node);
  node = new AvlTreeThRk<int>::Node (28);
  treei2.insert(node);
   node = new AvlTreeThRk<int>::Node (32);
  treei2.insert(node);
  cout<<endl;
  cout<< "Union of two trees with interseption of their domains and no common elements:"<<endl;
  cout<< "Left Tree:"<<endl;
  inOrderRec(treei1.getRoot(), print);
  cout<< endl<<"Right Tree:"<<endl;
  inOrderRec(treed1.getRoot(), print);
  treei1.unionWithNoInterception(treed1);
  cout<<endl <<"Union Tree: "<<endl;
  inOrderRec(treei1.getRoot(), print);
  cout<<endl<<endl;

  cout<< "Union of two trees with interseption of their domains and  common elements:"<<endl;
  cout<< "Left Tree:"<<endl;
  inOrderRec(treei2.getRoot(), print);
  cout<< endl<<"Right Tree:"<<endl;
  inOrderRec(treed2.getRoot(), print);
  treei2.unionWithInterception(treed2);
  cout<< endl<<"Union Tree: "<<endl;
  inOrderRec(treei2.getRoot(), print);
  cout<<endl <<"Common elements: "<<endl;
  inOrderRec(treed2.getRoot(), print);
  cout<<endl;


  AvlTreeThRk<int> treei3,treei4;
  AvlTreeThRk<int> treed3,treed4;
 
  for (i = 21; i <= 55; i+=2)
    {  
      node = new AvlTreeThRk<int>::Node (i);
      treei3.insert(node);
      assert(treei3.verifyRank());
      assert(treei3.verifyAvl());
      node = new AvlTreeThRk<int>::Node (i);
      treei4.insert(node);
    }
  for (i = 2; i <= 66; i+=2)
    {  
      node = new AvlTreeThRk<int>::Node (i);
      treed3.insert(node);
      node = new AvlTreeThRk<int>::Node (i);
      treed4.insert(node);
    }
  
  node = new AvlTreeThRk<int>::Node (31);
  treed4.insert(node);
  node = new AvlTreeThRk<int>::Node (29);
  treed4.insert(node);
  node = new AvlTreeThRk<int>::Node (41);
  treed4.insert(node);
  node = new AvlTreeThRk<int>::Node (28);
  treei4.insert(node);
  node = new AvlTreeThRk<int>::Node (32);
  treei4.insert(node);
  cout<<endl;
  cout<< "Union of one tree contein in the domain of other tree with"<<endl
      <<"that tree and no common elements:"<<endl;
  cout<< "Left Tree:"<<endl;
  inOrderRec(treei3.getRoot(), print);
  cout<< endl<<"Right Tree:"<<endl;
  inOrderRec(treed3.getRoot(), print);
  treei3.unionWithNoInterception(treed3);
  cout<<endl <<"Union Tree: "<<endl;
  inOrderRec(treei3.getRoot(), print);
  cout<<endl<<endl;

  cout<< "Union of one tree contein in the domain of other tree with"<<endl
      <<"that tree and common elements:"<<endl;
   cout<< "Left Tree:"<<endl;
  inOrderRec(treei4.getRoot(), print);
  cout<< endl<<"Right Tree:"<<endl;
  inOrderRec(treed4.getRoot(), print);
  treei4.unionWithInterception(treed4);
  cout<< endl<<"Union Tree: "<<endl;
  inOrderRec(treei4.getRoot(), print);
  cout<<endl <<"Common elements: "<<endl;
  inOrderRec(treed4.getRoot(), print);
  cout<<endl;


  cout<<endl;
  destroyRec(treei1.getRoot());
  destroyRec(treed1.getRoot());
  destroyRec(treei2.getRoot());
  destroyRec(treed2.getRoot());
  destroyRec(treei3.getRoot());
  destroyRec(treed3.getRoot());
  destroyRec(treei4.getRoot());
  destroyRec(treed4.getRoot());

}




