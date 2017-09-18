# include <iostream>
# include <List.H>
//# include <list>

using namespace std;

int main()
{
  list<int> list1, list2;
  
  cout << endl << "Llenando listas..." << endl;

  for (int i = 1; i <= 10; i++)
    {
      list1.push_front(i);
      list2.push_front(i*10);
    }

  cout << "list1: "; 
  list1.print();
  cout << "list2: ";
  list2.print();

  cout << endl << "swaping list1 y list2..." << endl;

  list1.swap(list2);

  cout << "list1: ";
  list1.print();
  cout << "list2: ";
  list2.print();

  cout << endl;

  cout << "Prueba insert(pos, value)..." << endl;

  list<int>::iterator itor1(list2.begin());

  itor1++;

  cout << *list2.insert(itor1,999) << endl;

  list2.print();
  cout << "size list2: " << list2.size() << endl;
  cout << endl;

  cout << "Prueba insert(pos, num, value)..." << endl;

  list<int>::iterator itor2(list2.begin());

  itor2++;

  list2.insert(itor2, 5, 800);

  list2.print();
  cout << "size list2: " << list2.size() << endl;
  cout << endl;

  cout << "Prueba insert(pos, beg, end)..." << endl;

  list<int>::iterator itor3(list1.begin());

  itor3++;

  list1.insert(itor3, list2.begin(),list2.end());

  list1.print();
  cout << "size list1: " << list1.size() << endl;
  cout << endl;

  list<int>::iterator itor4(list2.begin());
  itor4++; itor4++; itor4++; itor4++;
  cout << endl << "Prueba erase (beg, end)..." << endl;
  cout << *list2.erase(list2.begin(), itor4) << endl;
  list2.print();

  cout << endl << "Prueba splice(pos, list)..." << endl;
  list<int> list3(3,3);
  list3.splice(list3.begin(), list2);
  list3.print();
  cout << "size lis2: " << list2.size() << endl;

  cout << endl << "Prueba splice(pos, list, sourcePos)..." << endl;
  list3.splice(list3.begin(), list1, list1.begin());
  list3.print();
  cout << "size lis2: " << list1.size() << endl;

  cout << endl << "Prueba sort..." << endl;
  list3.sort();
  list3.print();

  cout << endl << "Prueba merge..." << endl;
  list3.merge(list1);
  list3.print();
  list1.print();
  list1.push_back(7);

  cout << endl << "Prueba assign..." << endl;
  list1.assign(list3.begin(), list3.end());
  list1.print();
  
  list1.pop_front();
  list3.pop_back(); 
  
  list1.print();
  list3.print();
  
  /*if (list1 > list2)
    cout << "list1 es mayor" << endl;
  else
  cout << "list1 es menor o igual" << endl;*/
  
}

