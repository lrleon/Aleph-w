
# include <iostream>
# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>

using namespace Aleph;

void printInt(BinNode<int>* p, int, int)
{
  cout << p->get_key() << " ";
}

BinNode<int> nodes[15];

int main()
{
  int i, n;
  n = 15;

  for (i = 0; i < n; i++)
    nodes[i].get_key() = i;

  nodes[0].getL() = &nodes[1];
  nodes[0].getR() = &nodes[2];
  nodes[1].getL() = &nodes[3];
  nodes[1].getR() = &nodes[4];
  nodes[2].getL() = &nodes[5];
  nodes[2].getR() = &nodes[6];
  nodes[3].getL() = &nodes[7];
  nodes[3].getR() = &nodes[8];
  nodes[4].getL() = &nodes[9];
  nodes[4].getR() = &nodes[10];
  nodes[5].getL() = &nodes[11];
  nodes[5].getR() = &nodes[12];
  nodes[6].getL() = &nodes[13];
  nodes[6].getR() = &nodes[14];

  BinNode<int>* p = copyRec(&nodes[0]);

  printf("\n");

  destroyRec(p);

  preOrderRec(&nodes[0], &printInt);
  printf("\n");

  inOrderRec(&nodes[0], &printInt);
  printf("\n");

  postOrderRec(&nodes[0], &printInt);
  printf("\n");
}
	  
