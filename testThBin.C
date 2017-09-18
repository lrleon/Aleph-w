 
# include <basic.H>
# include <iostream>
# include "tpl_binNode.H"


    template <class Type>
class ThBinNode
{
  Type data;
  ThBinNode *lLink;
  ThBinNode *rLink;

  static bool isThread(ThBinNode* p)
    {
      return p & 1;
    }

  static BinNode* addr(ThBinNode* p)
    {
      return p | ((BinNode*) -2);
    }

public:

  ThBinNode(const Type& d) 
    : data(d), lLink(0), rLink(0)
    {
      // Empty
    }

  ThBinNode()
    : lLink(0), rLink(0)
    {
      // Empty
    }
  
  

};
	  



void printInt(int& data)
{
  cout << data << " ";
}

BinNode<int> vector[15];

int main()
{
  int i, n;
  n = 15;

  for (i = 0; i < 15; i++)
    vector[i].get_data() = i;

  vector[0].getL() = &vector[1];
  vector[0].getR() = &vector[2];
  vector[1].getL() = &vector[3];
  vector[1].getR() = &vector[4];
  vector[2].getL() = &vector[5];
  vector[2].getR() = &vector[6];
  vector[3].getL() = &vector[7];
  vector[3].getR() = &vector[8];
  vector[4].getL() = &vector[9];
  vector[4].getR() = &vector[10];
  vector[5].getL() = &vector[11];
  vector[5].getR() = &vector[12];
  vector[6].getL() = &vector[13];
  vector[6].getR() = &vector[14];

  BinNode<int>* q = vector[0].copyRec();
  BinNode<int>* p = vector[0].copyStack();

  p->preOrderThreaded(&printInt);
  printf("\n");

  q->preOrderThreaded(&printInt);
  printf("\n");

  p->destroyRec();
  q->destroyStack();

  vector[0].preOrderThreaded(&printInt);
  printf("\n");
    
  vector[0].preOrderRec(&printInt);
  printf("\n");

  vector[0].preOrderStack(&printInt);
  printf("\n");

  vector[0].inOrderRec(&printInt);
  printf("\n");

  vector[0].inOrderStack(&printInt);
  printf("\n");
 
  vector[0].inOrderThreaded(&printInt);
  printf("\n");

  vector[0].postOrderRec(&printInt);
  printf("\n");

  vector[0].postOrderStack(&printInt);
  printf("\n");
 
}
	  
