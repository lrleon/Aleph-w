# include <iostream>
# include <List.H>
# include <Multiset.H>
# include <print_container.H>

 
using namespace std;
using namespace Aleph;

int main(int argn, char *argc[])
{
  int n = argc[1] ? atoi(argc[1]) : 1000;

  unsigned int t = time(0);

  size_t m = 2;
  if (argn > 2)
    m = atoi(argc[2]);

  if (argn > 3)
    t = atoi(argc[3]);

  srand(t);

  cout << argc[0] << " " << n << " " << m << " " << t << endl
       << endl
       << "Declarando multiset<int> s1;" << endl
       << endl 
       << endl;
  
  multiset<int> s1;

  cout << "Insertando " << n << " repeticiones de largo " << m << endl
       << endl;

  for (int i = 0; i < n; i++)
    for (int j = 0; j < m; j++)
      s1.insert(i);

  cout << "hecho!" << endl
       << endl
       << "Contando las courrencias de 1 ..." << endl
       << "halladas " << s1.count(1) << " ocurrencias de 1" << endl
       << endl
       << "Declarando multiset<int> s5;" << endl
       << endl;

  cout << "s1:" << endl;
  print_container(s1);

  multiset<int> s5;

  cout << "efectuando s5 = s1" << endl
       << endl;
  s5 = s1;

  cout << "s5:" << endl;
  print_container(s5);

  cout << "s1:" << endl;
  print_container(s1);

  cout << "Instanciando un iterador y mostrando los valores de s1 ..." << endl;
  multiset<int>::iterator fin(s1.end());
  for (multiset<int>::iterator it(s1.begin()); it != fin; it++)
    cout << *it << " ";

  cout << "Hecho!" << endl
       << endl
       << "Insertando s1.insert(s1.begin(), 1)" << endl
       << endl
       <<  * s1.insert(s1.begin(), 1) << endl
       << endl
       << "s1:";
  print_container(s1);

  cout << endl
       << "Insertando s1.insert(s1.begin(), 0)" << endl
       << endl
       <<  * s1.insert(s1.begin(), 0) << endl
       << endl
       << "s1:";
  print_container(s1);
  
  cout << endl
       << "Insertando s1.insert(s1.end(), 1)" << endl
       << endl
       <<  * s1.insert(s1.end(), 1) << endl
       << endl
       << "s1:";
  print_container(s1);

 cout << endl
       << "Insertando s1.insert(s1.end(), 5)" << endl
       << endl
       <<  * s1.insert(s1.end(), 5) << endl
       << endl
       << "s1:";
  print_container(s1);

 cout << endl
       << "Insertando s1.insert(s1.end(), 5)" << endl
       << endl
       <<  * s1.insert(s1.end(), 5) << endl
       << endl
       << "s1:";
  print_container(s1);

  cout << endl
       << "Insertando s1.insert(s1.find(2), 5)" << endl
       << endl
       <<  * s1.insert(s1.find(2), 5) << endl
       << endl
       << "s1:";
  print_container(s1);

  cout << endl
       << "Insertando s1.insert(s1.find(2), 5)" << endl
       << endl
       <<  * s1.insert(s1.find(2), 2) << endl
       << endl
       << "s1:";
  print_container(s1);

  cout << endl
       << "Insertando s1.insert(s1.find(2), 4)" << endl
       << endl
       <<  * s1.insert(s1.find(2), 4) << endl
       << endl
       << "s1:";
  print_container(s1);

  cout << endl
       << "Insertando s1.insert(s1.find(7), 4)" << endl
       << endl
       <<  * s1.insert(s1.find(2), 4) << endl
       << endl
       << "s1:";
  print_container(s1);

  cout << "Instanciando multiset<int> s2(s1.begin(), s1.end()); .." << endl
       << endl;
  multiset<int> s2(s1.begin(), s1.end());
  cout << "s2:";
  print_container(s2);

  cout << "Instanciando list<int> l1(s1.begin(), s1.end()); .." << endl
       << endl;
  list<int> l1(s1.begin(), s1.end());
  cout << "l1:";
  print_container(l1);

  cout << "Ejecutando s3.insert(l1.begin(), l1.end()); .." << endl
       << endl;
  multiset<int> s3(l1.begin(), l1.end());
  s3.insert(l1.begin(), l1.end());
  cout << "s3:";
  print_container(s3);

  cout << "Ejecutando s3.erase(4) .. " << s3.erase(4) 
       << " elementos borrados" << endl
       << "s3:";
  print_container(s3);

  return 0;

  s3.erase(s3.find(3));

  print_container(s3);

  s2 = s3;

  cout << "s2: " << endl;
  print_container(s2);

  cout << "s2.erase(s2.begin(), s2.end())" << endl;
  s2.erase(s2.begin(), s2.end());

  print_container(s2);

  multiset<int> s4(s1);

  cout << "s1 == s4 = " << (s1 == s4 ? "true" : "false") << endl;

  s1.insert(s1.find(4), 5);

  cout << "s1 == s4 = " << (s1 == s4 ? "true" : "false") << endl;

  cout << "s1: " << endl;
  print_container(s1);

  cout << "s4: " << endl;
  print_container(s4);

  cout << "s1 < s4 = " << (s1 < s4 ? "true" : "false") << endl;

  s1 = s4;

  cout << "s1 < s4 = " << (s1 < s4 ? "true" : "false") << endl;

  cout << argc[0] << " " << n << " " << m << " " << t << endl;
}
