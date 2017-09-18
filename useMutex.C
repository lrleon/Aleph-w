
# include <string>
# include <useMutex.H>

using namespace std;

void init_mutex(pthread_mutex_t * m)
{
  int status = pthread_mutex_init(m, NULL);

  if (status != 0)
    throw std::domain_error("cannot init mutex; error code " + 
			    to_string(status));
}

void init_mutex(pthread_mutex_t & m)
{
  init_mutex(&m);
}

void destroy_mutex(pthread_mutex_t * m)
{
  int status = pthread_mutex_destroy(m);

  if (status != 0)
    throw std::domain_error("cannot init mutex; error code " + 
			    to_string(status));
}

void destroy_mutex(pthread_mutex_t & m)
{
  destroy_mutex(&m);
} 
