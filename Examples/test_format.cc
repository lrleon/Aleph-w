#include <cstddef>
#include <tuple>

class Test {
  int obs_;
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
