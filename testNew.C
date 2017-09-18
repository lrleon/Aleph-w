
# include <iostream>


int main()
{
  
  try 
    {
      new int;
    }

  catch (...)
    {
      std::cout << "**** Exception \n\n";
    }
}
