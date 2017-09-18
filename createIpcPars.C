
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <stdio.h>
# include <errno.h>
# include <string.h>
# include <stdlib.h>

/**
   createIpcPars.

   This program creates in the current directory a file called .ipcPars
   that contains the ip address and the epoch of the site.
   @autor Carlos Nava <navac@ula.ve>
   @version 1.00 alfa
*/

int main( int argc, char **argv)
{
  int epoch, ipaddress, ipcPars;
  struct in_addr addr;

  if (argc not_eq 3)
    {
      printf("usage: createIpcPars <epoch> <ipaddress>\n");
      return 0;
    }

  epoch = atoi(argv[1]);
  if (inet_aton(argv[2], &addr) == 0)
    {
      printf("Invalid ipaddress\n");
      return 0;
    }

  ipaddress = addr.s_addr;

  /* The .ipcPars file is created with user read and write permisions.
     If the file alredy exist it is an error, and the program exits.
  */

  ipcPars = open("/tmp/.ipcPars", O_WRONLY | O_CREAT | O_EXCL, 
		 S_IRUSR | S_IWUSR);
	
  if (ipcPars == -1)
    {
      printf("%s\n", strerror(errno));
      return 0;
    }

  if (write(ipcPars, &epoch, sizeof(int)) not_eq sizeof(int))
    {
      printf("Epoch write error\n");
      return 0;
    }

  if (write(ipcPars, &ipaddress, sizeof(int)) not_eq sizeof(int))
    {
      printf("IPaddress write error\n");
      return 0;
    }

  close(ipcPars);
  return 0;
}

