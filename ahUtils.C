
# include <unistd.h>
# include <sys/resource.h>
# include <algorithm>
# include <ahUtils.H>


bool Aleph::resize_process_stack(size_t new_size)
{
  const rlim_t kStackSize = new_size;  
  struct rlimit rl;
  int result;

  result = getrlimit(RLIMIT_STACK, &rl);
  if (result == 0)
    if (rl.rlim_cur < kStackSize)
      {
	rl.rlim_cur = kStackSize;
	result = setrlimit(RLIMIT_STACK, &rl);

	return result == 0;
      }

  return false;
}
