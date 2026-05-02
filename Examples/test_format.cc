#include <cstddef>
#include <tuple>

struct Observer
{
  template <typename Lattice>
  void on_step_end(std::size_t, const Lattice &)
  {}
};

class Test {
  std::tuple<Observer, Observer> obs_;
public:
  template <typename Lattice>
  void on_step_end(std::size_t step, const Lattice &frame)
  {
    std::apply( [&](auto &...o)
    {
      (o.on_step_end(step, frame), ...);
    },
      obs_);
  }
};

int main()
{
  return 0;
}
