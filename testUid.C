
# include <iostream>
# include <uid.H>

int main()
{
  Uid uid(127, 10, 10);

  uid.print();

  char buffer[256];

  std::cout << uid.getStringUid(buffer, 256) << std::endl;

  std::cout << buffer << std::endl;

  Uid uid_inv(buffer);

  uid_inv.print();
}
