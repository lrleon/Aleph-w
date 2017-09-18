
# include <random_net.H>

typedef Net_Arc<Empty_Class> Arc;
typedef Net_Node<Empty_Class> Node;

typedef Net_Graph<> Net;

int main(int argn, char * argc[])
{
  size_t num_nodes = argn > 1 ? atoi(argc[1]) : 1000;
  size_t num_ranks = argn > 2 ? atoi(argc[2]) : 100;
  unsigned int seed = argn > 3 ? atoi(argc[3]) : 0;

  cout << argc[0] << " " << num_nodes << " " << num_ranks << " " 
       << seed << endl;

  Net * net_ptr = Random_Network_Flow<Net> (seed) (num_ranks, num_nodes);

  delete net_ptr;
}
