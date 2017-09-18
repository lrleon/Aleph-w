
# include <log.H>

int main()
{
  Log_Manager log(stdout);

  WRITE_IN_LOG(log, "esta es una prueba (%s)\n", "foo");
  WRITE_IN_LOG(log, "esta es una prueba (%s)\n", "foo");
}
