
# ifndef AHDAEMONIZE_H
# define AHDAEMONIZE_H

# include <syslog.h>

/*
  This function is based on daemon_init function described in "The
  Unix Netwrok Programming", volume 1 by Richard Stevens. Page 336.
*/
extern void daemonize(const char * program_name, int facility);

# endif // AHDAEMONIZE_H
