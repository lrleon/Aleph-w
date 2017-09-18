
# include <basic.H>
# include "tpl_dynStack.H"
# include <time.h>

class BinLink
{
  int data;
  BinLink*  lLink;
  BinLink*  rLink;

public:

  BinLink(int d) 
    : data(d), lLink(0), rLink(0)
    {
      assert((unsigned) this % 2 == 0);
    }

  BinLink() 
    : lLink(0), rLink(0)
    {
      assert((unsigned) this % 2 == 0);
    }
  
  int& get_data() { return data; }

  const BinLink* getL() const { return lLink; }

  const BinLink* getR() const { return rLink; }

  BinLink*& getL() { return lLink; }

  BinLink*& getR() { return rLink; }

  void preorder1()
    {
      printf("%d ", data);
      if (lLink != NULL)
	lLink->preorder1();
      if (rLink != NULL)
	rLink->preorder1();
    }

  void preorder2()
    {
      DynStack<BinLink*> stack;
      BinLink* node = this;

      while (1)
	{
	  printf("%d ", node->get_data());
	  if (node->lLink)
	    {
	      stack.push(node);
	      node = node->lLink;
	      continue;
	    }
	  while (1)
	    {

	      if (node->rLink)
		{
		  node = node->rLink;
		  break;
		}
	      if (stack.is_empty())
		return;
	      node = stack.pop();
	    }
	}
    }
      
};


BinLink vector[15];

int main()
{
  int i, n;
  srand(time(0));
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

  vector[0].preorder1();
  printf("\n");

  vector[0].preorder2();

  printf("\n");
  

    

}
	  
