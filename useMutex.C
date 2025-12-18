
# include <string>
# include <useMutex.H>
 # include <ah-errors.H>

using namespace std;

void init_mutex(pthread_mutex_t * m)
{
  const int status = pthread_mutex_init(m, NULL);

  ah_domain_error_if(status != 0)
    << "cannot init mutex; error code " << to_string(status);
}

void init_mutex(pthread_mutex_t & m)
{
  init_mutex(&m);
}

void destroy_mutex(pthread_mutex_t * m)
{
  const int status = pthread_mutex_destroy(m);

  ah_domain_error_if(status != 0)
    << "cannot init mutex; error code " << to_string(status);
}

void destroy_mutex(pthread_mutex_t & m)
{
  destroy_mutex(&m);
} 
