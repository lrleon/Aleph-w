
# include <iostream>
#include "ipcClient.H"

int main(int argn, char * argv[])
{
  int delay = 0;

  if (argn > 1)
    delay = atoi(argv[1]);

  cout << "Server with a delay of " << delay << " seconds" << endl;

  char request_buffer[81];
  RawMsg request_message(&request_buffer[0], 81);

  char reply_buffer[81];
  RawMsg reply_message(&reply_buffer[0], 81);

  char stringPort[Port::stringSize];

  cout << "Port::stringSize = " << Port::stringSize << endl;

  MsgId msgid;

  char * received_string;

  IpcServer server;
  printf("SERVER PORT %s\n", 
	 server.getServerPort().getStringPort(stringPort, Port::stringSize)
	 );

  cout << "strlen(stringPort) = " << strlen(stringPort) << endl;;
  char str[80];

  for (int i = 0; true; i++)
    {
      msgid = server.receive_request(request_message); 

      received_string = static_cast<char*>(request_message.getBodyAddr());

      sprintf(reply_buffer, "Reply %i", i);

      reply_message.setBodySize(strlen(reply_buffer) + 1);

      if (delay not_eq 0)
	sleep(delay);

      server.send_reply(reply_message, msgid);

      cout << "Replied message \"" << received_string << "\" (" 
	   << msgid.msgid_to_char(str) << ")" << endl; 

    }
	
  return 0;
}

