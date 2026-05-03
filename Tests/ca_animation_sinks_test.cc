/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_animation_sinks_test.cc
 * @brief Phase 11.5 tests for animated, interactive and composite CA sinks.
 */

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include <ca-ffmpeg-sink.H>
#include <ca-frame-stream.H>
#include <ca-gif.H>
#include <ca-html-player.H>
#include <ca-npy.H>
#include <ca-png.H>
#include <ca-x11-viewer.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_storage.H>

using namespace Aleph::CA;

namespace
{
  using Grid = Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;

  Grid make_frame(const int phase = 0)
  {
    Grid g({3, 3}, 0);
    if (phase == 0)
      {
        g.set({1, 0}, 1);
        g.set({1, 1}, 1);
        g.set({1, 2}, 1);
      }
    else
      {
        g.set({0, 1}, 1);
        g.set({1, 1}, 1);
        g.set({2, 1}, 1);
      }
    return g;
  }

  std::filesystem::path unique_tmp_dir(const std::string &name)
  {
    const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path()
      / (name + "_" + std::to_string(static_cast<long long>(tick)));
  }

  std::string read_file(const std::filesystem::path &path, const bool binary = false)
  {
    std::ifstream in(path, binary ? std::ios::binary : std::ios::in);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
  }

  std::size_t count_byte(const std::string &s, const unsigned char byte)
  {
    std::size_t count = 0;
    for (const unsigned char c : s)
      if (c == byte)
        ++count;
    return count;
  }
}

TEST(CAGif, WritesAnimatedGifWithOneImagePerAcceptedFrame)
{
  const auto dir = unique_tmp_dir("aleph_ca_gif");
  std::filesystem::remove_all(dir);
  const auto path = dir / "blinker.gif";

  GIF_Write_Options opts;
  opts.delay_cs = 4;
  Gif_Frame_Sink gif(path,
                     Binary_RGB_Mapper<int>{0, {250, 250, 250}, {20, 20, 20}},
                     opts);
  gif.accept(0, make_frame(0));
  gif.accept(1, make_frame(1));
  gif.flush();

  const std::string bytes = read_file(path, true);
  ASSERT_GE(bytes.size(), 16u);
  EXPECT_EQ(bytes.substr(0, 6), "GIF89a");
  EXPECT_EQ(count_byte(bytes, 0x2c), 2u);
  EXPECT_EQ(bytes.back(), ';');

  std::filesystem::remove_all(dir);
}

TEST(CAHtmlPlayer, WritesSelfContainedCanvasPlayer)
{
  const auto dir = unique_tmp_dir("aleph_ca_html");
  std::filesystem::remove_all(dir);
  const auto path = dir / "player.html";

  HTML_Player_Options opts;
  opts.title = "Blinker demo";
  opts.cell_size = 5;
  HTML_Player_Frame_Sink html(path,
                              Binary_RGB_Mapper<int>{0, {255, 255, 255}, {0, 0, 0}},
                              opts);
  html.accept(0, make_frame(0));
  html.accept(1, make_frame(1));
  html.flush();

  const std::string text = read_file(path);
  EXPECT_NE(text.find("<canvas id=\"view\""), std::string::npos);
  EXPECT_NE(text.find("const frames = ["), std::string::npos);
  EXPECT_NE(text.find("\"step\":0"), std::string::npos);
  EXPECT_EQ(text.find("http://"), std::string::npos);
  EXPECT_EQ(text.find("https://"), std::string::npos);

  std::filesystem::remove_all(dir);
}

TEST(CAFfmpegSink, CanBeConfiguredAsNoOpWhenRuntimeEncoderIsDisabled)
{
  Ffmpeg_Options opts;
  opts.auto_start = false;
  Ffmpeg_Frame_Sink sink("movie.mp4",
                         Binary_RGB_Mapper<int>{0, {255, 255, 255}, {0, 0, 0}},
                         opts);
  EXPECT_FALSE(sink.active());
  EXPECT_NE(sink.command().find("ffmpeg -y"), std::string::npos);
  EXPECT_NE(sink.command().find("movie.mp4"), std::string::npos);
  sink.accept(0, make_frame());
  sink.flush();
}

TEST(CAX11Viewer, DisabledViewerIsSafeInHeadlessTests)
{
  X11_Viewer_Options opts;
  opts.enabled = false;
  X11_Frame_Sink viewer(Binary_RGB_Mapper<int>{0, {255, 255, 255}, {0, 0, 0}}, opts);
  EXPECT_FALSE(viewer.active());
  viewer.accept(0, make_frame());
  viewer.flush();
  EXPECT_FALSE(viewer.active());
}

TEST(CACompositeSink, FansOutToSeveralBackends)
{
  const auto dir = unique_tmp_dir("aleph_ca_composite");
  std::filesystem::remove_all(dir);

  auto composite = Composite_Frame_Sink(
    Png_Frame_Sink(dir / "frame_{step}.png",
                   Binary_RGB_Mapper<int>{0, {255, 255, 255}, {0, 0, 0}},
                   2),
    Npy_Frame_Sink(dir / "frame_{step}.npy", 2),
    HTML_Player_Frame_Sink(dir / "trajectory.html",
                           Binary_RGB_Mapper<int>{0, {255, 255, 255}, {0, 0, 0}}));

  composite.accept(0, make_frame(0));
  composite.accept(1, make_frame(1));
  composite.flush();

  EXPECT_TRUE(std::filesystem::exists(dir / "frame_00.png"));
  EXPECT_TRUE(std::filesystem::exists(dir / "frame_00.npy"));
  EXPECT_TRUE(std::filesystem::exists(dir / "trajectory.html"));
  EXPECT_NE(read_file(dir / "trajectory.html").find("\"step\":1"), std::string::npos);

  std::filesystem::remove_all(dir);
}
