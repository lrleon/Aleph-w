/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/**
 * @file ca_tikz_frame_stream_test.cc
 * @brief Phase 11 tests for TikZ rendering and directory frame streams.
 */

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include <ca-frame-stream.H>
#include <ca-tikz.H>
#include <tpl_ca_hex_lattice.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_storage.H>
#include <tpl_ca_triangular_lattice.H>

using namespace Aleph::CA;

namespace
{
  using Grid = Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;
  using HexGrid = Hex_Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;
  using TriGrid = Triangular_Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;

  Grid make_frame()
  {
    Grid g({2, 3}, 0);
    g.set({0, 1}, 1);
    g.set({1, 2}, 1);
    return g;
  }

  std::filesystem::path unique_tmp_dir(const std::string &name)
  {
    const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path()
      / (name + "_" + std::to_string(static_cast<long long>(tick)));
  }

  std::string read_file(const std::filesystem::path &path, bool binary = false)
  {
    std::ifstream in(path, binary ? std::ios::binary : std::ios::in);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
  }
}

TEST(CATikz, RectangularRendererIncludesCellsGridAndLegend)
{
  const Grid frame = make_frame();
  Rect_Tikz_Options opts;
  opts.cell_size = 0.5;
  opts.include_legend = true;
  opts.grid_colour = "blue!20";

  const std::string tikz = render_rect_lattice_tikz(
    frame,
    [](int v) { return v == 0 ? std::string("green!10") : std::string("red!80"); },
    opts);

  EXPECT_NE(tikz.find("% Aleph::CA::render_rect_lattice_tikz"), std::string::npos);
  EXPECT_NE(tikz.find("\\begin{tikzpicture}"), std::string::npos);
  EXPECT_NE(tikz.find("fill=green!10, draw=blue!20"), std::string::npos);
  EXPECT_NE(tikz.find("fill=red!80, draw=blue!20"), std::string::npos);
  EXPECT_NE(tikz.find("rectangle (1, 0.5)"), std::string::npos);
  EXPECT_NE(tikz.find("{alive}"), std::string::npos);
}

TEST(CATikz, RectangularRendererRejectsInvalidCellSize)
{
  Rect_Tikz_Options opts;
  opts.cell_size = 0.0;
  EXPECT_THROW(render_rect_lattice_tikz(make_frame(),
                                        Binary_Tikz_Palette<int>{},
                                        opts),
               std::domain_error);
}

TEST(CATikz, GenericRendererDispatchesToHexAndTriangularRenderers)
{
  HexGrid hex({2, 2}, 0);
  hex.set_axial({1, 0}, 1);
  const std::string hex_tikz = render_lattice_tikz(
    hex, Binary_Tikz_Palette<int>{0, "white", "orange"});
  EXPECT_NE(hex_tikz.find("% Aleph::CA::render_hex_lattice_tikz"), std::string::npos);
  EXPECT_NE(hex_tikz.find("regular polygon sides=6"), std::string::npos);
  EXPECT_NE(hex_tikz.find("fill=orange"), std::string::npos);

  TriGrid tri({2, 2}, 0);
  tri.set_tri({1, 1}, 1);
  const std::string tri_tikz = render_lattice_tikz(
    tri, Binary_Tikz_Palette<int>{0, "white", "cyan"});
  EXPECT_NE(tri_tikz.find("% Aleph::CA::render_triangular_lattice_tikz"), std::string::npos);
  EXPECT_NE(tri_tikz.find("regular polygon sides=3"), std::string::npos);
  EXPECT_NE(tri_tikz.find("fill=cyan"), std::string::npos);
}

TEST(CAFrameStream, DirectorySinkWritesStrideFilteredPgmAndManifest)
{
  const auto dir = unique_tmp_dir("aleph_ca_stream_pgm");
  std::filesystem::remove_all(dir);

  Frame_Stream_Options opts;
  opts.directory = dir;
  opts.prefix = "gol";
  opts.format = Frame_Output_Format::PGM;
  opts.stride = 2;
  opts.zero_pad = 3;

  Directory_Frame_Sink sink(opts);
  const Grid frame = make_frame();
  sink.accept(0, frame);
  sink.accept(1, frame);
  sink.accept(2, frame);
  sink.flush();

  ASSERT_EQ(sink.written_frames().size(), 2u);
  const auto p0 = dir / "gol_000.pgm";
  const auto p2 = dir / "gol_002.pgm";
  EXPECT_TRUE(std::filesystem::exists(p0));
  EXPECT_FALSE(std::filesystem::exists(dir / "gol_001.pgm"));
  EXPECT_TRUE(std::filesystem::exists(p2));
  EXPECT_EQ(read_file(p0, true).rfind("P5\n3 2\n255\n", 0), 0u);

  const std::string manifest = read_file(dir / "manifest.csv");
  EXPECT_NE(manifest.find("step,format,path"), std::string::npos);
  EXPECT_NE(manifest.find("0,pgm,"), std::string::npos);
  EXPECT_NE(manifest.find("2,pgm,"), std::string::npos);

  std::filesystem::remove_all(dir);
}

TEST(CAFrameStream, DirectorySinkDispatchesTextFormats)
{
  const auto dir = unique_tmp_dir("aleph_ca_stream_text");
  std::filesystem::remove_all(dir);
  const Grid frame = make_frame();

  struct Case
  {
    Frame_Output_Format format;
    const char *prefix;
    const char *filename;
    const char *needle;
  };

  const Case cases[] = {
    {Frame_Output_Format::RLE,  "rle",  "rle_000001.rle",  "x = 3, y = 2"},
    {Frame_Output_Format::CSV,  "csv",  "csv_000001.csv",  "0,1,0"},
    {Frame_Output_Format::JSON, "json", "json_000001.json", "\"cells\":[[0,1,0],[0,0,1]]"},
    {Frame_Output_Format::TikZ, "tikz", "tikz_000001.tex", "\\begin{tikzpicture}"}
  };

  for (const auto &tc : cases)
    {
      Frame_Stream_Options opts;
      opts.directory = dir;
      opts.prefix = tc.prefix;
      opts.format = tc.format;
      Directory_Frame_Sink sink(opts);
      sink.accept(1, frame);
      sink.flush();

      const auto path = dir / tc.filename;
      ASSERT_TRUE(std::filesystem::exists(path)) << path;
      EXPECT_NE(read_file(path).find(tc.needle), std::string::npos) << path;
    }

  std::filesystem::remove_all(dir);
}

TEST(CAFrameStream, DirectorySinkRejectsZeroStrideAndBuildsCommandHint)
{
  const auto dir = unique_tmp_dir("aleph_ca_stream_invalid");
  Frame_Stream_Options bad;
  bad.directory = dir;
  bad.stride = 0;
  EXPECT_THROW({ Directory_Frame_Sink bad_sink(bad); }, std::domain_error);

  Frame_Stream_Options opts;
  opts.directory = dir;
  opts.prefix = "frame";
  opts.format = Frame_Output_Format::PPM;
  opts.zero_pad = 4;
  Directory_Frame_Sink sink(opts);
  const std::string cmd = sink.montage_command_hint("movie.mp4", 12);
  EXPECT_NE(cmd.find("ffmpeg -y -framerate 12"), std::string::npos);
  EXPECT_NE(cmd.find("frame_%04d.ppm"), std::string::npos);
  EXPECT_NE(cmd.find("movie.mp4"), std::string::npos);

  std::filesystem::remove_all(dir);
}
