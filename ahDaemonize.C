
# include <unistd.h>
# include <ah-signal.H>
# include <sys/types.h>
# include <sys/stat.h>
# include <ahDaemonize.H>

# define MAXIMUM_FILE_DESC 256

/*
  This function is based on daemon_init function described in "The
  Unix Netwrok Programming", volume 1 by Richard Stevens. Page 336.
*/
void daemonize(const char * pname, int facility)
{
  int i;
  pid_t pid;

  pid = fork();

  if (pid == -1)
    AH_ERROR("cannot fork process");

  if (pid != 0)
    exit(0);       // parent terminates

  /* 1st child continues and become session leader */
  setsid();

  Signal signalHandler(SIGHUP, SIG_IGN);

  pid = fork();

  if (pid == -1)
    AH_ERROR("cannot fork process");

  if (pid != 0)
    exit(0);       // 1st child terminates

  umask(0);        // clear our file mode creation mask

  for (i = 0; i < MAXIMUM_FILE_DESC; i++)
    close(i);

  openlog(pname, LOG_PID, facility);

  Aleph::daemonized = true;
}



