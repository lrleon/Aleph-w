# include <ipcClient.H>

# define NUM_MESSAGES 1

int main(int argv, char* argc[])
{
  int num_messages = argv == 2 ? atoi(argc[1]) : NUM_MESSAGES;

  for (int i = 0; i < num_messages; i++)
    {
      Uid uid;

      createUid(&uid);

      uid.print();

      cout << uid.get_random_number() << endl;
    }
}

