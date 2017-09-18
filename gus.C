#include<iostream>
#include"tpl_snode.H"

using namespace std;
void invertir(Snode<int>Head,Snode<int>*Aux,Snode<int>*Node);

int main()
{
	Snode<int>head,*aux,*node;
	node=new Snode<int>;
	while(node->get_data()!=-1)
	{
		cout<<"Inserte valores a la lista, -1 si desea salir \n\n";
		node=new Snode<int>;	
		cin>>node->get_data();
		head.insert_next(node);
	}
	
	
	invertir(head,aux,node);
	
	cout<<"aux es ";
	while(aux!=&head)
	{
		cout<<aux->get_data()<<" ";
		aux=aux->get_next();
	}

	

	return 0;

}

void invertir(Snode<int> & head)
{
  Snode<int> aux;


  while (not head.is_empty())
    aux.insert_next(head.remove_next());

  
}
