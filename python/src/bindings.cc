// =====================================================================
// Aleph-w :: Python bindings for the cellular-automata module (Phase 18)
// =====================================================================
//
// Exposes a pragmatic subset of `Aleph::CA` to Python so that the broader
// scientific-computing community (Jupyter, NumPy, Matplotlib) can drive
// the C++ engines without writing C++.
//
// Design choices (kept intentionally small):
//
//   * Only two canonical numeric instantiations are bound: int (default
//     for combinatorial CAs) and float (Gray-Scott style continuous).
//     Anything more exotic should go through the Python-callback engine
//     described below.
//
//   * Frame buffers are exposed both as eager copies (`frame()`) and as
//     zero-copy NumPy views (`frame_view()`). The view becomes stale on
//     the next `step()` because the engine double-buffers; the docstring
//     spells this out so users don't get bitten.
//
//   * The "generic" engine accepts a Python callable as the rule. We pay
//     for one GIL acquisition + list build per cell, which is slow but
//     useful for prototyping. Production rules should live in C++.
//
//   * Checkpointing reuses the v2 atomic-rename + DEFLATE machinery from
//     Phase 17 verbatim; only the Python entry points are new here.
//
// Module layout (visible to Python as `aleph_ca._core`):
//   - GameOfLifeEngine            -- alias of Synchronous_Engine<...>
//   - WolframEngine               -- 1D elementary Wolfram rules
//   - ForestFireEngine            -- Drossel-Schwabl stochastic CA
//   - GenericEngine2D[int|float]  -- Python-rule sandbox
//   - save_checkpoint/load_checkpoint_into
//   - write_png_bytes / write_gif_bytes / write_svg_bytes
//   - RGB8                        -- helper struct for palettes
// =====================================================================

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <ah-errors.H>
#include <ca-checkpoint.H>
#include <ca-engine-utils.H>
#include <ca-gif.H>
#include <ca-io.H>
#include <ca-png.H>
#include <ca-svg.H>
#include <ca-traits.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_stochastic_rules.H>
#include <tpl_ca_storage.H>

namespace py = pybind11;
namespace ca = Aleph::CA;

// =====================================================================
// Helpers: lattice <-> NumPy round-trips.
// =====================================================================

namespace {

template <typename T>
using Lattice2D = ca::Lattice<ca::Dense_Cell_Storage<T, 2>, ca::ToroidalBoundary>;

template <typename T>
using Lattice1D = ca::Lattice<ca::Dense_Cell_Storage<T, 1>, ca::OpenBoundary>;

/// Build a NumPy view that aliases the engine's current frame buffer.
///
/// The capsule keeps a strong reference back to the Python wrapper so the
/// underlying engine cannot be destroyed while the view is alive. The
/// view itself becomes meaningless after `engine.step()` (the buffers
/// are swapped); we document this on the Python side.
template <typename T>
py::array_t<T> view_frame_2d(const Lattice2D<T> &frame, py::object owner)
{
  const auto rows = static_cast<py::ssize_t>(frame.size(0));
  const auto cols = static_cast<py::ssize_t>(frame.size(1));
  const T *base = frame.storage().data();
  py::capsule keep_alive(new py::object(std::move(owner)),
                         [](void *p) noexcept
                         {
                           delete static_cast<py::object *>(p);
                         });
  return py::array_t<T>({rows, cols},
                        {static_cast<py::ssize_t>(sizeof(T) * cols),
                         static_cast<py::ssize_t>(sizeof(T))},
                        base, keep_alive);
}

template <typename T>
py::array_t<T> view_frame_1d(const Lattice1D<T> &frame, py::object owner)
{
  const auto width = static_cast<py::ssize_t>(frame.size(0));
  const T *base = frame.storage().data();
  py::capsule keep_alive(new py::object(std::move(owner)),
                         [](void *p) noexcept
                         {
                           delete static_cast<py::object *>(p);
                         });
  return py::array_t<T>({width}, {static_cast<py::ssize_t>(sizeof(T))},
                        base, keep_alive);
}

/// Copy a contiguous 2D NumPy array into the engine's writable lattice.
template <typename T>
void blit_into_lattice_2d(Lattice2D<T> &dst, py::array_t<T, py::array::c_style | py::array::forcecast> arr)
{
  if (arr.ndim() != 2)
    throw py::value_error("expected a 2D NumPy array");
  const auto rows = static_cast<ca::ca_size_t>(arr.shape(0));
  const auto cols = static_cast<ca::ca_size_t>(arr.shape(1));
  if (rows != dst.size(0) or cols != dst.size(1))
    {
      std::ostringstream oss;
      oss << "array shape (" << rows << ", " << cols << ") does not match lattice shape ("
          << dst.size(0) << ", " << dst.size(1) << ")";
      throw py::value_error(oss.str());
    }
  const T *src = arr.data();
  for (ca::ca_size_t r = 0; r < rows; ++r)
    for (ca::ca_size_t c = 0; c < cols; ++c)
      dst.set({static_cast<ca::ca_index_t>(r), static_cast<ca::ca_index_t>(c)}, src[r * cols + c]);
}

template <typename T>
void blit_into_lattice_1d(Lattice1D<T> &dst, py::array_t<T, py::array::c_style | py::array::forcecast> arr)
{
  if (arr.ndim() != 1)
    throw py::value_error("expected a 1D NumPy array");
  const auto width = static_cast<ca::ca_size_t>(arr.shape(0));
  if (width != dst.size(0))
    {
      std::ostringstream oss;
      oss << "array length " << width << " does not match lattice width " << dst.size(0);
      throw py::value_error(oss.str());
    }
  const T *src = arr.data();
  for (ca::ca_size_t i = 0; i < width; ++i)
    dst.set({static_cast<ca::ca_index_t>(i)}, src[i]);
}

// =====================================================================
// Python-callback rule (Phase 18 "GenericEngine").
//
// The rule satisfies BasicRuleLike by forwarding (state, neighbours) to a
// Python callable. Used only by the Python-driven engines; the GIL must
// be held during dispatch because the rule body may run arbitrary Python.
// =====================================================================

template <typename T>
struct Python_Rule
{
  py::function fn;

  T operator()(const T &state, ca::Neighbor_View<T> nbrs) const
  {
    py::gil_scoped_acquire gil;
    py::list py_nbrs(nbrs.size());
    for (std::size_t i = 0; i < nbrs.size(); ++i)
      py_nbrs[i] = nbrs[i];
    py::object out = fn(state, py_nbrs);
    return out.cast<T>();
  }
};

// =====================================================================
// Generic 2D engine specialised on T (int / float).
//
// Toroidal boundary + Moore radius-1 neighbourhood, the canonical setup
// shared by every textbook 2D CA. The neighbourhood is fixed; users who
// need Von Neumann or radius > 1 should build the engine in C++ or
// adjust the bindings.
// =====================================================================

template <typename T>
using Generic_Engine_2D = ca::Synchronous_Engine<Lattice2D<T>, Python_Rule<T>, ca::Moore<2, 1>>;

template <typename T>
Generic_Engine_2D<T> make_generic_engine_2d(ca::ca_size_t rows, ca::ca_size_t cols, py::function rule)
{
  Lattice2D<T> initial({rows, cols}, T{});
  return Generic_Engine_2D<T>(std::move(initial), Python_Rule<T>{std::move(rule)}, ca::Moore<2, 1>{});
}

// =====================================================================
// Forest-fire engine specialisation (toroidal int lattice).
//
// Exposed because the Phase 19 reproduction roadmap leans on it. Schelling
// is intentionally *not* exposed yet — its discrete enum carries
// uint8_t-typed semantics that need extra wrapping work.
// =====================================================================

using Forest_Fire_Engine_2D
  = ca::Synchronous_Engine<Lattice2D<int>, ca::Forest_Fire_Rule<>, ca::Moore<2, 1>>;

Forest_Fire_Engine_2D make_forest_fire_engine(ca::ca_size_t rows,
                                              ca::ca_size_t cols,
                                              double p_growth,
                                              double p_lightning,
                                              std::uint64_t seed)
{
  Lattice2D<int> initial({rows, cols}, 0);
  return Forest_Fire_Engine_2D(std::move(initial),
                               ca::Forest_Fire_Rule<>(p_growth, p_lightning, seed),
                               ca::Moore<2, 1>{});
}

// =====================================================================
// PNG/GIF/SVG byte exporters.
//
// Returning bytes (rather than writing to disk) lets Python callers pipe
// the output straight into Jupyter's `IPython.display.Image`, into a
// `io.BytesIO`, or into Matplotlib's `imshow`-from-buffer flow.
// =====================================================================

template <typename Lattice>
py::bytes write_png_bytes_impl(const Lattice &frame)
{
  std::ostringstream oss(std::ios::binary);
  ca::write_png(oss, frame);
  const std::string buf = oss.str();
  return py::bytes(buf.data(), buf.size());
}

template <typename Lattice>
py::bytes write_svg_bytes_impl(const Lattice &frame)
{
  std::ostringstream oss;
  ca::render_lattice_svg(oss, frame);
  const std::string buf = oss.str();
  return py::bytes(buf.data(), buf.size());
}

}  // namespace

// =====================================================================
// Module definition.
// =====================================================================

PYBIND11_MODULE(_core, m)
{
  m.doc()
    = "Aleph-w cellular-automata engines (pybind11 surface). See the Python\n"
      "façade in `aleph_ca/__init__.py` for the documented, user-facing API.";

  // -------------------------------------------------------------------
  // RGB8 -- minimal struct used by the PNG/GIF mappers.
  // -------------------------------------------------------------------
  py::class_<ca::RGB8>(m, "RGB8", "8-bit RGB triplet (matches ca::RGB8).")
    .def(py::init<>())
    .def(py::init([](int r, int g, int b)
                  {
                    ca::RGB8 rgb;
                    rgb.r = static_cast<std::uint8_t>(r);
                    rgb.g = static_cast<std::uint8_t>(g);
                    rgb.b = static_cast<std::uint8_t>(b);
                    return rgb;
                  }),
         py::arg("r"), py::arg("g"), py::arg("b"))
    .def_readwrite("r", &ca::RGB8::r)
    .def_readwrite("g", &ca::RGB8::g)
    .def_readwrite("b", &ca::RGB8::b)
    .def("__repr__",
         [](const ca::RGB8 &c)
         {
           std::ostringstream oss;
           oss << "RGB8(" << int(c.r) << ", " << int(c.g) << ", " << int(c.b) << ")";
           return oss.str();
         });

  // -------------------------------------------------------------------
  // GameOfLifeEngine -- Conway's GoL on a toroidal int lattice.
  // -------------------------------------------------------------------
  py::class_<ca::Game_Of_Life_Engine>(m, "GameOfLifeEngine",
    "Conway's Game of Life engine on a toroidal int lattice with Moore radius-1 "
    "neighbourhood. Construct it via `make_game_of_life_engine(rows, cols)`.")
    .def("step", &ca::Game_Of_Life_Engine::step,
         "Advance the simulation by exactly one synchronous step.")
    .def("run", &ca::Game_Of_Life_Engine::run, py::arg("steps"),
         "Advance the simulation by `steps` synchronous steps.")
    .def("steps_run", &ca::Game_Of_Life_Engine::steps_run,
         "Number of `step()` invocations performed so far.")
    .def("extents",
         [](const ca::Game_Of_Life_Engine &e)
         {
           return std::make_pair(e.extents()[0], e.extents()[1]);
         },
         "Return the (rows, cols) extent of the lattice.")
    .def("frame_view",
         [](py::object self)
         {
           auto &eng = self.cast<ca::Game_Of_Life_Engine &>();
           return view_frame_2d<int>(eng.frame(), self);
         },
         "Zero-copy NumPy view of the current frame. The view aliases the "
         "engine's internal buffer; it becomes stale after the next step().")
    .def("frame",
         [](const ca::Game_Of_Life_Engine &e)
         {
           const auto &f = e.frame();
           py::array_t<int> out({static_cast<py::ssize_t>(f.size(0)),
                                  static_cast<py::ssize_t>(f.size(1))});
           auto buf = out.mutable_unchecked<2>();
           for (ca::ca_size_t r = 0; r < f.size(0); ++r)
             for (ca::ca_size_t c = 0; c < f.size(1); ++c)
               buf(r, c) = f.at({static_cast<ca::ca_index_t>(r), static_cast<ca::ca_index_t>(c)});
           return out;
         },
         "Return a fresh (owning) NumPy copy of the current frame.")
    .def("set_frame",
         [](ca::Game_Of_Life_Engine &e,
            py::array_t<int, py::array::c_style | py::array::forcecast> arr)
         {
           blit_into_lattice_2d<int>(e.write_lattice(), arr);
         },
         py::arg("array"),
         "Copy a NumPy array into the engine's current lattice. The array "
         "shape must match `extents()`.");

  m.def("make_game_of_life_engine",
        [](ca::ca_size_t rows, ca::ca_size_t cols)
        {
          return ca::make_gol_engine(rows, cols);
        },
        py::arg("rows"), py::arg("cols"),
        "Build a Game of Life engine with `rows` x `cols` dead cells.");

  // -------------------------------------------------------------------
  // WolframEngine -- elementary 1D Wolfram rule (open boundary, int).
  // -------------------------------------------------------------------
  py::class_<ca::Wolfram_1D_Engine>(m, "WolframEngine",
    "Elementary 1D Wolfram rule engine on an open-boundary int lattice. "
    "Construct via `make_wolfram_engine(rule_no, width)`.")
    .def("step", &ca::Wolfram_1D_Engine::step,
         "Advance the simulation by exactly one synchronous step.")
    .def("run", &ca::Wolfram_1D_Engine::run, py::arg("steps"),
         "Advance the simulation by `steps` synchronous steps.")
    .def("steps_run", &ca::Wolfram_1D_Engine::steps_run,
         "Number of completed steps.")
    .def("width",
         [](const ca::Wolfram_1D_Engine &e) { return e.extents()[0]; },
         "Lattice width.")
    .def("frame_view",
         [](py::object self)
         {
           auto &eng = self.cast<ca::Wolfram_1D_Engine &>();
           return view_frame_1d<int>(eng.frame(), self);
         },
         "Zero-copy NumPy view of the current 1D frame.")
    .def("frame",
         [](const ca::Wolfram_1D_Engine &e)
         {
           const auto &f = e.frame();
           py::array_t<int> out({static_cast<py::ssize_t>(f.size(0))});
           auto buf = out.mutable_unchecked<1>();
           for (ca::ca_size_t i = 0; i < f.size(0); ++i)
             buf(i) = f.at({static_cast<ca::ca_index_t>(i)});
           return out;
         },
         "Return a fresh NumPy copy of the current 1D frame.")
    .def("set_frame",
         [](ca::Wolfram_1D_Engine &e,
            py::array_t<int, py::array::c_style | py::array::forcecast> arr)
         {
           blit_into_lattice_1d<int>(e.write_lattice(), arr);
         },
         py::arg("array"),
         "Copy a 1D NumPy array into the engine's lattice.");

  m.def("make_wolfram_engine",
        [](int rule_no, ca::ca_size_t width)
        {
          if (rule_no < 0 or rule_no > 255)
            throw py::value_error("rule_no must be in [0, 255]");
          return ca::make_wolfram_engine(static_cast<std::uint8_t>(rule_no), width);
        },
        py::arg("rule_no"), py::arg("width"),
        "Build an elementary Wolfram engine. `rule_no` ∈ [0, 255].");

  // -------------------------------------------------------------------
  // ForestFireEngine -- Drossel & Schwabl 1992, deterministic per-cell RNG.
  // -------------------------------------------------------------------
  py::class_<Forest_Fire_Engine_2D>(m, "ForestFireEngine",
    "Forest-fire CA (Drossel & Schwabl 1992) on a toroidal int lattice. "
    "Cell semantics: 0=EMPTY, 1=TREE, 2=BURNING.")
    .def("step", &Forest_Fire_Engine_2D::step)
    .def("run", &Forest_Fire_Engine_2D::run, py::arg("steps"))
    .def("steps_run", &Forest_Fire_Engine_2D::steps_run)
    .def("extents",
         [](const Forest_Fire_Engine_2D &e)
         {
           return std::make_pair(e.extents()[0], e.extents()[1]);
         })
    .def("frame_view",
         [](py::object self)
         {
           auto &eng = self.cast<Forest_Fire_Engine_2D &>();
           return view_frame_2d<int>(eng.frame(), self);
         })
    .def("frame",
         [](const Forest_Fire_Engine_2D &e)
         {
           const auto &f = e.frame();
           py::array_t<int> out({static_cast<py::ssize_t>(f.size(0)),
                                  static_cast<py::ssize_t>(f.size(1))});
           auto buf = out.mutable_unchecked<2>();
           for (ca::ca_size_t r = 0; r < f.size(0); ++r)
             for (ca::ca_size_t c = 0; c < f.size(1); ++c)
               buf(r, c) = f.at({static_cast<ca::ca_index_t>(r), static_cast<ca::ca_index_t>(c)});
           return out;
         })
    .def("set_frame",
         [](Forest_Fire_Engine_2D &e,
            py::array_t<int, py::array::c_style | py::array::forcecast> arr)
         {
           blit_into_lattice_2d<int>(e.write_lattice(), arr);
         },
         py::arg("array"))
    .def("master_seed",
         [](const Forest_Fire_Engine_2D &e) { return e.rule().master_seed(); })
    .def("set_master_seed",
         [](Forest_Fire_Engine_2D &e, std::uint64_t s) { e.rule().set_master_seed(s); });

  m.def("make_forest_fire_engine", &make_forest_fire_engine,
        py::arg("rows"), py::arg("cols"),
        py::arg("p_growth") = 0.005,
        py::arg("p_lightning") = 0.0001,
        py::arg("seed") = 0,
        "Build a Drossel & Schwabl forest-fire engine.");

  // -------------------------------------------------------------------
  // GenericEngine2D[int|float] -- Python-callback rule sandbox.
  //
  // Slow (one GIL acquisition + list build per cell) but useful when
  // prototyping a new rule before promoting it to C++.
  // -------------------------------------------------------------------
  py::class_<Generic_Engine_2D<int>>(m, "GenericEngine2DInt",
    "2D engine driven by a Python callable. Each cell calls `fn(state, "
    "neighbours)` with a Python list of 8 Moore neighbours.")
    .def("step", &Generic_Engine_2D<int>::step)
    .def("run", &Generic_Engine_2D<int>::run, py::arg("steps"))
    .def("steps_run", &Generic_Engine_2D<int>::steps_run)
    .def("extents",
         [](const Generic_Engine_2D<int> &e)
         {
           return std::make_pair(e.extents()[0], e.extents()[1]);
         })
    .def("frame_view",
         [](py::object self)
         {
           auto &eng = self.cast<Generic_Engine_2D<int> &>();
           return view_frame_2d<int>(eng.frame(), self);
         })
    .def("frame",
         [](const Generic_Engine_2D<int> &e)
         {
           const auto &f = e.frame();
           py::array_t<int> out({static_cast<py::ssize_t>(f.size(0)),
                                  static_cast<py::ssize_t>(f.size(1))});
           auto buf = out.mutable_unchecked<2>();
           for (ca::ca_size_t r = 0; r < f.size(0); ++r)
             for (ca::ca_size_t c = 0; c < f.size(1); ++c)
               buf(r, c) = f.at({static_cast<ca::ca_index_t>(r), static_cast<ca::ca_index_t>(c)});
           return out;
         })
    .def("set_frame",
         [](Generic_Engine_2D<int> &e,
            py::array_t<int, py::array::c_style | py::array::forcecast> arr)
         {
           blit_into_lattice_2d<int>(e.write_lattice(), arr);
         },
         py::arg("array"));

  m.def("make_generic_engine_2d_int", &make_generic_engine_2d<int>,
        py::arg("rows"), py::arg("cols"), py::arg("rule"),
        "Build a 2D int engine whose transition is defined by a Python "
        "callable `(state, neighbours) -> new_state`.");

  py::class_<Generic_Engine_2D<float>>(m, "GenericEngine2DFloat",
    "Float variant of GenericEngine2DInt for continuous CAs.")
    .def("step", &Generic_Engine_2D<float>::step)
    .def("run", &Generic_Engine_2D<float>::run, py::arg("steps"))
    .def("steps_run", &Generic_Engine_2D<float>::steps_run)
    .def("extents",
         [](const Generic_Engine_2D<float> &e)
         {
           return std::make_pair(e.extents()[0], e.extents()[1]);
         })
    .def("frame_view",
         [](py::object self)
         {
           auto &eng = self.cast<Generic_Engine_2D<float> &>();
           return view_frame_2d<float>(eng.frame(), self);
         })
    .def("frame",
         [](const Generic_Engine_2D<float> &e)
         {
           const auto &f = e.frame();
           py::array_t<float> out({static_cast<py::ssize_t>(f.size(0)),
                                    static_cast<py::ssize_t>(f.size(1))});
           auto buf = out.mutable_unchecked<2>();
           for (ca::ca_size_t r = 0; r < f.size(0); ++r)
             for (ca::ca_size_t c = 0; c < f.size(1); ++c)
               buf(r, c) = f.at({static_cast<ca::ca_index_t>(r), static_cast<ca::ca_index_t>(c)});
           return out;
         })
    .def("set_frame",
         [](Generic_Engine_2D<float> &e,
            py::array_t<float, py::array::c_style | py::array::forcecast> arr)
         {
           blit_into_lattice_2d<float>(e.write_lattice(), arr);
         },
         py::arg("array"));

  m.def("make_generic_engine_2d_float", &make_generic_engine_2d<float>,
        py::arg("rows"), py::arg("cols"), py::arg("rule"),
        "Build a 2D float engine driven by a Python callable.");

  // -------------------------------------------------------------------
  // Checkpoint save/load.
  //
  // Templated on every engine type the module knows about; each function
  // dispatches to ca::save_checkpoint / ca::load_checkpoint_into.
  // -------------------------------------------------------------------
  py::class_<ca::Checkpoint_Options>(m, "CheckpointOptions",
    "Tunable knobs forwarded to ca::save_checkpoint.")
    .def(py::init<>())
    .def_readwrite("compress", &ca::Checkpoint_Options::compress)
    .def_readwrite("level", &ca::Checkpoint_Options::level)
    .def_readwrite("sync_dir", &ca::Checkpoint_Options::sync_dir);

#define ALEPH_BIND_CHECKPOINT(EngineType, py_name)                                 \
  m.def(py_name,                                                                   \
        [](EngineType & engine, const std::string &path, ca::Checkpoint_Options o) \
        {                                                                          \
          ca::save_checkpoint(engine, std::filesystem::path(path), o);             \
        },                                                                         \
        py::arg("engine"), py::arg("path"),                                        \
        py::arg("options") = ca::Checkpoint_Options{},                             \
        "Persist an engine snapshot to disk (atomic rename, optional "             \
        "DEFLATE).");

  ALEPH_BIND_CHECKPOINT(ca::Game_Of_Life_Engine, "save_checkpoint_gol")
  ALEPH_BIND_CHECKPOINT(ca::Wolfram_1D_Engine, "save_checkpoint_wolfram")
  ALEPH_BIND_CHECKPOINT(Forest_Fire_Engine_2D, "save_checkpoint_forest_fire")

#undef ALEPH_BIND_CHECKPOINT

#define ALEPH_BIND_LOAD(EngineType, py_name)                            \
  m.def(py_name,                                                        \
        [](EngineType & engine, const std::string &path)                \
        {                                                               \
          (void) ca::load_checkpoint_into(engine, std::filesystem::path(path)); \
        },                                                              \
        py::arg("engine"), py::arg("path"),                             \
        "Restore an engine's state from a checkpoint file in place.");

  ALEPH_BIND_LOAD(ca::Game_Of_Life_Engine, "load_checkpoint_into_gol")
  ALEPH_BIND_LOAD(ca::Wolfram_1D_Engine, "load_checkpoint_into_wolfram")
  ALEPH_BIND_LOAD(Forest_Fire_Engine_2D, "load_checkpoint_into_forest_fire")

#undef ALEPH_BIND_LOAD

  // -------------------------------------------------------------------
  // PNG / SVG exporters (rank-2 only).
  // -------------------------------------------------------------------
  m.def("write_png_bytes_gol",
        [](const ca::Game_Of_Life_Engine &e) { return write_png_bytes_impl(e.frame()); },
        py::arg("engine"),
        "Render the engine's current frame as PNG bytes.");
  m.def("write_png_bytes_forest_fire",
        [](const Forest_Fire_Engine_2D &e) { return write_png_bytes_impl(e.frame()); },
        py::arg("engine"));
  m.def("write_png_bytes_generic_int",
        [](const Generic_Engine_2D<int> &e) { return write_png_bytes_impl(e.frame()); },
        py::arg("engine"));

  m.def("write_svg_bytes_gol",
        [](const ca::Game_Of_Life_Engine &e) { return write_svg_bytes_impl(e.frame()); },
        py::arg("engine"));
  m.def("write_svg_bytes_forest_fire",
        [](const Forest_Fire_Engine_2D &e) { return write_svg_bytes_impl(e.frame()); },
        py::arg("engine"));

  // -------------------------------------------------------------------
  // Build metadata (handy for `aleph_ca.__version__` plumbing).
  // -------------------------------------------------------------------
  m.attr("__cxx_standard__") = "C++20";
  m.attr("__build_id__") = "phase-18";
}