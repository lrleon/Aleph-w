/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_visualization_gallery_example.cc
 * @brief Phase 11.5 gallery for CA visualisation backends.
 *
 * Usage:
 *
 *   ca_visualization_gallery_example [output-dir] [steps]
 *
 * The example runs the same Game-of-Life glider-gun simulation and writes
 * PNG, GIF, SVG, NPY, VTK, HTML, DOT, ASCII and ffmpeg-command artifacts.
 * The X11 sink is included as an optional no-op by default; pass `--live`
 * as the third argument to show the simulation while exporting.
 */

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <ca-ascii.H>
#include <ca-dot.H>
#include <ca-engine-utils.H>
#include <ca-ffmpeg-sink.H>
#include <ca-gif.H>
#include <ca-html-player.H>
#include <ca-npy.H>
#include <ca-png.H>
#include <ca-svg.H>
#include <ca-vtk.H>
#include <ca-x11-viewer.H>
#include <tpl_ca_graph_automaton.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_storage.H>

using namespace Aleph::CA;

namespace
{
using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

std::size_t parse_size(const char *text, const std::size_t fallback)
{
  if (text == nullptr)
    return fallback;
  char *end = nullptr;
  const unsigned long value = std::strtoul(text, &end, 10);
  if (end == text or *end != '\0' or value == 0
      or value > std::numeric_limits<std::size_t>::max())
    return fallback;
  return static_cast<std::size_t>(value);
}

void stamp(Grid &grid,
           const ca_index_t row,
           const ca_index_t col,
           const std::vector<std::pair<int, int>> &cells)
{
  const ca_index_t rows = static_cast<ca_index_t>(grid.size(0));
  const ca_index_t cols = static_cast<ca_index_t>(grid.size(1));
  for (const auto &[dr, dc] : cells)
    {
      const ca_index_t r = ((row + dr) % rows + rows) % rows;
      const ca_index_t c = ((col + dc) % cols + cols) % cols;
      grid.set({r, c}, 1);
    }
}

std::vector<std::pair<int, int>> gosper_glider_gun()
{
  return {
    {4,0}, {4,1}, {5,0}, {5,1},
    {2,12}, {2,13}, {3,11}, {3,15}, {4,10}, {4,16},
    {5,10}, {5,14}, {5,16}, {5,17}, {6,10}, {6,16},
    {7,11}, {7,15}, {8,12}, {8,13},
    {0,24}, {1,22}, {1,24}, {2,20}, {2,21}, {3,20},
    {3,21}, {4,20}, {4,21}, {5,22}, {5,24}, {6,24},
    {2,34}, {2,35}, {3,34}, {3,35}
  };
}

Graph_Lattice<int> graph_sample_from(const Grid &frame)
{
  constexpr std::size_t rows = 5;
  constexpr std::size_t cols = 5;
  Graph_Lattice<int> graph(make_grid_graph_adjacency(rows, cols), 0);
  const ca_size_t r0 = frame.size(0) / 2;
  const ca_size_t c0 = frame.size(1) / 2;
  for (std::size_t r = 0; r < rows; ++r)
    for (std::size_t c = 0; c < cols; ++c)
      {
        const int value = frame.at({static_cast<ca_index_t>(r0 + r),
                                    static_cast<ca_index_t>(c0 + c)});
        graph.set_node(r * cols + c, value);
      }
  return graph;
}
}

int main(int argc, char **argv)
{
  const std::filesystem::path out_dir = argc >= 2 ? argv[1] : "ca_gallery_output";
  const std::size_t steps = argc >= 3 ? parse_size(argv[2], 200) : 200;
  const bool live = argc >= 4 and std::string(argv[3]) == "--live";
  std::filesystem::create_directories(out_dir);

  Grid initial({40, 80}, 0);
  stamp(initial, 6, 4, gosper_glider_gun());
  auto engine = make_gol_engine(std::move(initial));

  const auto palette = Binary_RGB_Mapper<int>{0, {248, 250, 252}, {5, 28, 44}};
  Png_Frame_Sink png(out_dir / "png" / "life_{step}.png", palette, 4);
  Gif_Frame_Sink gif(out_dir / "life.gif", palette, GIF_Write_Options{5, true});
  HTML_Player_Frame_Sink html(out_dir / "life.html", palette,
                              HTML_Player_Options{"Gosper glider gun", 12, 7, false});

  X11_Viewer_Options xopts;
  xopts.enabled = live;
  xopts.title = "Aleph::CA gallery";
  xopts.cell_size = 8;
  X11_Frame_Sink x11(palette, xopts);

  for (std::size_t t = 0; t <= steps; ++t)
    {
      if (t % 4 == 0)
        {
          png.accept(engine.steps_run(), engine.frame());
          gif.accept(engine.steps_run(), engine.frame());
          html.accept(engine.steps_run(), engine.frame());
        }
      x11.accept(engine.steps_run(), engine.frame());
      if (t != steps)
        engine.step();
    }
  gif.flush();
  html.flush();
  x11.flush();

  {
    std::ofstream out(out_dir / "life.svg");
    SVG_Render_Options opts;
    opts.cell_size = 10.0;
    opts.omit_dead_cells = false;
    render_lattice_svg(out, engine.frame(), palette, opts);
  }
  {
    std::ofstream out(out_dir / "life.npy", std::ios::binary);
    write_npy(out, engine.frame());
  }
  {
    std::ofstream out(out_dir / "life.vtk");
    write_vtk_legacy(out, engine.frame(), VTK_Write_Options{"GoL final frame", "alive"});
  }
  {
    std::ofstream out(out_dir / "life.txt");
    Ascii_Render_Options opts;
    opts.include_border = true;
    render_ascii(out, engine.frame(), Binary_ASCII_Palette<int>{0, ".", "#"}, opts);
  }
  {
    std::ofstream out(out_dir / "life.dot");
    render_graph_lattice_dot(out,
                             graph_sample_from(engine.frame()),
                             Binary_DOT_Palette<int>{0, "white", "black"});
  }
  {
    Ffmpeg_Options opts;
    opts.auto_start = false;
    Ffmpeg_Frame_Sink ffmpeg(out_dir / "life.mp4", palette, opts);
    std::ofstream out(out_dir / "ffmpeg-command.txt");
    out << ffmpeg.command() << '\n';
  }

  std::cout << "Gallery written to " << out_dir << '\n';
  std::cout << "Open life.html in a browser, life.gif in an image viewer, "
            << "and life.vtk in ParaView.\n";
  return 0;
}
