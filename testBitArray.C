
# include <iostream>
# include <bitArray.H>

using namespace std;
using namespace Aleph;

void print(BitArray & array)
{
  cout << "size = " << array.size() << endl;

  for (int i = 0; i < array.size(); i++)
    cout << array[i] << " ";
  cout << endl;
}

void foo(int& i) { i = 5; }

int main()
{
  BitArray array(20);

  for (int i = 0; i < 20; i++)
    {
      array[i] = 0;
    }

  array[20] = 1;
  array[27] = 1;
  array[22] = 1;

  for (int i = 30; i < 40; i++)
    {
      array[i] = array[i];
    }

  print(array);

  cout << endl;  

  ofstream out("test.bits");
  array.save(out);

  ifstream in("test-aux.bits");
  array.load(in);

  print(array);
}
