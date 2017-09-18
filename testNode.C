
# include <iostream>
# include <tpl_binNodeAux.H>

class Altura 
	{
	private:
	  size_t altura;
	public:
	  Altura() {}
	  Altura(size_t a) : altura(a) { /* empty */ }
	  size_t dar_altura() const { return altura; }
	};


using namespace std;

DECLARE_BINNODE(Nodo, 255, Altura);
SET_BINNODE_NULL_POINTER(NULL, Nodo);

using namespace std;

int main()
{

  {
    BinNode<int> node1;
    BinNode<int> node2(5);

    BinNode<int>::NullPtr = &node1;

    cout << "sizeof(BinNode<int>) = " << sizeof(BinNode<int>) << endl;

    cout << "BinNode<int>::MaxHeight = " << BinNode<int>::MaxHeight  
	 << endl;
  }

  {
    BinNodeVtl<int> node1;
    BinNodeVtl<int> node2(5);

    BinNodeVtl<int>::NullPtr = &node1;

    cout << "sizeof(BinNodeVtl<int>) = " << 
      sizeof(BinNodeVtl<int>) << endl;
    cout << "BinNodeVtl<int>::MaxHeight = " 
	 << BinNodeVtl<int>::MaxHeight << endl;
  }

  {
    Nodo<int> node1;
    Nodo<int> node2(5);
    Nodo<int> node3(Altura(10));

    Nodo<int>::NullPtr = &node1;

    cout << "sizeof(Nodo<int>) = " << sizeof(Nodo<int>) << endl;
    cout << "Nodo<int>::MaxHeight = " << Nodo<int>::MaxHeight << endl;
  }

}
  
