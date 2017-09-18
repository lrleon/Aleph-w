# include <iostream>
# include <List.H>
# include <Vector.H>


using namespace std;

int main(int argn, char *argc[])
{
  int n = argc[1] ? atoi(argc[1]) : 1000;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  list<int> l1; 

  int i;
  for (i = 0; i < n; i++)
    l1.push_back(i);

  cout << "l1"; l1.print();

  list<int> l2(n); 

  cout << "l2"; l2.print();

  list<int> l3(n, 40); 

  cout << "l3"; l3.print();

  l1.insert(l1.begin(), 4, 100);

  l1.print();

  l1.insert(l1.begin(), l3.begin(), l3.end());

  l1.print();

  l1.remove(40);

  l1.print();

  l1.erase(l1.erase(l1.begin()));

  l1.print();

  list<int>::iterator it1 = l1.begin();
  list<int>::iterator it2 = l1.end();
  
  it1 += 3;

  it2 -= 2;

  l1.erase(it1, it2);
 
  l1.print();

  l1.resize(l1.size() + 3);

  l1.print();

  l1.unique();

  l1.print();

  for (list<int>::iterator it(l2.begin()); it not_eq l2.end(); )
    it++ = i++;

  l2.print();

  l1.splice(l1.begin(), l2);

  l1.print();

  l2.print();

  l3.print();

  while (not l3.empty())
    l1.splice(l1.begin(), l3, l3.begin());

  l1.print();
  
  l3.print();

  list<int> l4(n, 55);

  i = 0;
  for (list<int>::iterator beg(l4.begin()); beg not_eq l4.end(); )
    beg++ = i++;

  l4.print();

  list<int>::iterator it4_beg(l4.begin()), it4_end(l4.end());

  for (i = 0; i < n/4; i++)
    {
      it4_beg++;
      --it4_end;
    }

  for (list<int>::iterator beg(it4_beg); beg not_eq it4_end; )
    cout << " " << beg++;

  cout << endl << endl;

  l1.splice(l1.begin(), l4, it4_beg, it4_end);

  l1.print();
  l4.print();

  vector<int> v1(l1.begin(), l1. end());

  for (i = 0; i < v1.size(); i++)
    cout << v1[i] << " ";

  cout << endl;
}
