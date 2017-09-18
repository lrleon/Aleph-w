
#include <iostream>
#include <stdlib.h>
#include <signal.h>

void handler(int signo, siginfo_t *info, void *ptr)
{
  cout << "Recibida senal " << signo << " (" << info->si_signo 
       << ") desde el proceso " << info->si_pid << endl
       << "info = " << info << endl;

  if (info->si_code == SEGV_MAPERR)
    cout  << "Se~nal enviada porque la direccion " 
	  << info->si_addr << endl
	  << "no esta mapeada a algun objeto" << endl;
  else if (info->si_code == SEGV_ACCERR)
    cout  << "Se~nal enviada porque el objeto almacenado" << endl
	  << "la direccion " << info->si_addr << endl
	  << "no autoriza el acceso" << endl;
  else
    cout  << "Motivo de la senal (" << info->si_code 
	  << "): indeterminado" << endl;

  cout  << "La direccion " << info->si_addr << endl
	<< "no esta mapeada a algun objeto" << endl;

  exit(0);
}

int main(void)
{
  struct sigaction action;

  //  action.sa_handler = NULL;
  action.sa_sigaction = &handler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = SA_SIGINFO;

  sigaction(SIGSEGV, &action, NULL);

  char *ptr = NULL;

  *ptr = '\0';
}
