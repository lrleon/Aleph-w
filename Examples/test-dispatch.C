
# include <sstream>
# include <ah-dispatcher.H>

string fct1(int p1, int p2)
{
  ostringstream s;
  s << "fct1(" << p1 << ", " << p2 << ")";
  return s.str();
}

string fct2(int p1, int p2)
{
  ostringstream s;
  s << "fct2(" << p1 << ", " << p2 << ")";
  return s.str();
}

int main()
{
  {
    AHDispatcher<string, decltype(&fct1)> dispatcher;
    dispatcher.insert("cadena1", fct1);
    dispatcher.insert("cadena2", fct2);

    cout << dispatcher.run("cadena1", 2, 3) << endl;
  }

  {
    AhHashDispatcher<string, decltype(&fct1)> dispatcher;
    dispatcher.insert("cadena1", fct1);
    dispatcher.insert("cadena2", fct2);

    cout << dispatcher.run("cadena1", 2, 3) << endl;
  }

}
