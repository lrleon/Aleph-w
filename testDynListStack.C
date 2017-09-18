
# include <iostream>
# include <tpl_dynListStack.H>

using namespace std;

using namespace Aleph;

int main()
{
  DynListStack<int> stack;
  int i;

  for (i = 0; i < 20; i++)
    stack.push(i);
    
  try
    {
      while (1)
	cout << stack.pop() << " ";
    }
  catch (std::underflow_error)
    {
      cout << "Stack has been emptied" << endl;
    }

  cout << endl;
}

  
