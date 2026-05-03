/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_visual_sinks_test.cc
 * @brief Phase 11.5 tests for static CA visualisation sinks.
 */

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <ca-ascii.H>
#include <ca-dot.H>
#include <ca-npy.H>
#include <ca-png.H>
#include <ca-svg.H>
#include <ca-vtk.H>
#include <tpl_ca_graph_automaton.H>
#include <tpl_ca_hex_lattice.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_storage.H>
#include <tpl_ca_triangular_lattice.H>

using namespace Aleph::CA;

namespace
{
  using Grid = Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;
  using Grid3 = Lattice<Dense_Cell_Storage<int, 3>, OpenBoundary>;
  using HexGrid = Hex_Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;
  using TriGrid = Triangular_Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;

  struct DecodedPNG
  {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> raw;
  };

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

  std::string read_file(const std::filesystem::path &path, const bool binary = false)
  {
    std::ifstream in(path, binary ? std::ios::binary : std::ios::in);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
  }

  std::uint32_t be32(const std::string &s, std::size_t pos)
  {
    return (static_cast<std::uint32_t>(static_cast<unsigned char>(s[pos])) << 24)
           | (static_cast<std::uint32_t>(static_cast<unsigned char>(s[pos + 1])) << 16)
           | (static_cast<std::uint32_t>(static_cast<unsigned char>(s[pos + 2])) << 8)
           | static_cast<std::uint32_t>(static_cast<unsigned char>(s[pos + 3]));
  }

  DecodedPNG decode_native_png(const std::string &png)
  {
    if (png.size() < 8u)
      {
        ADD_FAILURE() << "PNG too short";
        return {};
      }
    EXPECT_EQ(png.substr(0, 8), std::string("\x89PNG\r\n\x1a\n", 8));

    DecodedPNG decoded;
    std::string idat;
    std::size_t pos = 8;
    while (pos + 12 <= png.size())
      {
        const std::uint32_t len = be32(png, pos);
        const std::string type = png.substr(pos + 4, 4);
        const std::string payload = png.substr(pos + 8, len);
        if (type == "IHDR")
          {
            decoded.width = be32(payload, 0);
            decoded.height = be32(payload, 4);
            EXPECT_EQ(static_cast<unsigned char>(payload[8]), 8u);
            EXPECT_EQ(static_cast<unsigned char>(payload[9]), 2u);
          }
        else if (type == "IDAT")
          {
            idat += payload;
          }
        else if (type == "IEND")
          {
            break;
          }
        pos += 12 + len;
      }

    if (idat.size() < 6u)
      {
        ADD_FAILURE() << "IDAT too short";
        return decoded;
      }
    std::size_t z = 2;
    while (z + 5 <= idat.size() - 4)
      {
        const bool final = (static_cast<unsigned char>(idat[z]) & 1u) != 0;
        if ((static_cast<unsigned char>(idat[z]) & 0x06u) != 0u)
          {
            ADD_FAILURE() << "compressed PNG block was not stored";
            return decoded;
          }
        ++z;
        const std::uint16_t len = static_cast<std::uint16_t>(
          static_cast<unsigned char>(idat[z])
          | (static_cast<unsigned char>(idat[z + 1]) << 8));
        const std::uint16_t nlen = static_cast<std::uint16_t>(
          static_cast<unsigned char>(idat[z + 2])
          | (static_cast<unsigned char>(idat[z + 3]) << 8));
        EXPECT_EQ(static_cast<std::uint16_t>(~len), nlen);
        z += 4;
        if (z + len > idat.size())
          {
            ADD_FAILURE() << "stored block length exceeds IDAT";
            return decoded;
          }
        decoded.raw.insert(decoded.raw.end(), idat.begin() + static_cast<std::ptrdiff_t>(z),
                           idat.begin() + static_cast<std::ptrdiff_t>(z + len));
        z += len;
        if (final)
          break;
      }
    return decoded;
  }

  std::size_t count_substr(const std::string &s, const std::string &needle)
  {
    std::size_t count = 0;
    std::size_t pos = 0;
    while ((pos = s.find(needle, pos)) != std::string::npos)
      {
        ++count;
        pos += needle.size();
      }
    return count;
  }
}

TEST(CAPng, WritesDecodableRgbPng)
{
  std::ostringstream out;
  write_png(out, make_frame(), Binary_RGB_Mapper<int>{0, {255, 255, 255}, {0, 0, 0}});

  const DecodedPNG png = decode_native_png(out.str());
  EXPECT_EQ(png.width, 3u);
  EXPECT_EQ(png.height, 2u);
  ASSERT_EQ(png.raw.size(), 2u * (1u + 3u * 3u));
  EXPECT_EQ(png.raw[0], 0u);
  EXPECT_EQ(png.raw[1], 255u);
  EXPECT_EQ(png.raw[4], 0u);
  EXPECT_EQ(png.raw[10], 0u);
}

TEST(CASvg, RendersRectanglesHexagonsAndTriangles)
{
  const std::string rect = render_lattice_svg_string(
    make_frame(), Binary_RGB_Mapper<int>{0, {250, 250, 250}, {10, 40, 200}});
  EXPECT_NE(rect.find("<svg"), std::string::npos);
  EXPECT_EQ(count_substr(rect, "<rect "), 6u);
  EXPECT_NE(rect.find("fill=\"#0a28c8\""), std::string::npos);

  HexGrid hex({2, 2}, 0);
  hex.set_axial({1, 0}, 1);
  const std::string hs = render_lattice_svg_string(
    hex, Binary_RGB_Mapper<int>{0, {255, 255, 255}, {255, 120, 0}});
  EXPECT_EQ(count_substr(hs, "<polygon "), 4u);

  TriGrid tri({2, 2}, 0);
  tri.set_tri({1, 1}, 1);
  const std::string ts = render_lattice_svg_string(
    tri, Binary_RGB_Mapper<int>{0, {255, 255, 255}, {0, 160, 190}});
  EXPECT_EQ(count_substr(ts, "<polygon "), 4u);
}

TEST(CANpy, WritesVersionOneHeaderAndPayload)
{
  std::ostringstream out;
  write_npy(out, make_frame());
  const std::string npy = out.str();

  ASSERT_GE(npy.size(), 10u);
  EXPECT_EQ(npy.substr(0, 6), std::string("\x93NUMPY", 6));
  EXPECT_EQ(static_cast<unsigned char>(npy[6]), 1u);
  const std::uint16_t header_len = static_cast<std::uint16_t>(
    static_cast<unsigned char>(npy[8]) | (static_cast<unsigned char>(npy[9]) << 8));
  ASSERT_GE(npy.size(), 10u + header_len);
  const std::string header = npy.substr(10, header_len);
  EXPECT_NE(header.find("'shape': (2, 3)"), std::string::npos);
  EXPECT_NE(header.find("'fortran_order': False"), std::string::npos);
  EXPECT_EQ(npy.size(), 10u + header_len + 6u * sizeof(int));
}

TEST(CAVtk, WritesStructuredPointsFor3DFrames)
{
  Grid3 g({2, 2, 2}, 0);
  g.set({1, 0, 1}, 7);
  std::ostringstream out;
  VTK_Write_Options opts;
  opts.scalar_name = "alive";
  write_vtk_legacy(out, g, opts);
  const std::string vtk = out.str();
  EXPECT_NE(vtk.find("DATASET STRUCTURED_POINTS"), std::string::npos);
  EXPECT_NE(vtk.find("DIMENSIONS 2 2 2"), std::string::npos);
  EXPECT_NE(vtk.find("POINT_DATA 8"), std::string::npos);
  EXPECT_NE(vtk.find("SCALARS alive int 1"), std::string::npos);
  EXPECT_NE(vtk.find("\n7\n"), std::string::npos);
}

TEST(CAAscii, RendersGoldenBlinkerSnapshot)
{
  const Grid frame = make_frame();
  EXPECT_EQ(render_ascii_string(frame), ".#.\n..#\n");

  Ascii_Render_Options opts;
  opts.include_border = true;
  EXPECT_EQ(render_ascii_string(frame, opts), "+---+\n|.#.|\n|..#|\n+---+\n");
}

TEST(CADot, RendersGraphAutomatonAsDot)
{
  Graph_Lattice<int> graph(make_path_graph_adjacency(3), 0);
  graph.set_node(1, 1);
  const std::string dot = render_graph_lattice_dot_string(
    graph, Binary_DOT_Palette<int>{0, "white", "black"});
  EXPECT_NE(dot.find("graph CA"), std::string::npos);
  EXPECT_NE(dot.find("n1 [fillcolor=\"black\""), std::string::npos);
  EXPECT_NE(dot.find("n0 -- n1"), std::string::npos);
  EXPECT_NE(dot.find("n1 -- n2"), std::string::npos);
  EXPECT_EQ(count_substr(dot, "--"), 2u);
}

TEST(CAFileSinks, WriteStaticFormatsToStepPaths)
{
  const auto dir = unique_tmp_dir("aleph_ca_visual_sinks");
  std::filesystem::remove_all(dir);
  const Grid frame = make_frame();

  Png_Frame_Sink png(dir / "png_{step}.png",
                     Binary_RGB_Mapper<int>{0, {255, 255, 255}, {0, 0, 0}},
                     3);
  Npy_Frame_Sink npy(dir / "npy_{step}.npy", 3);
  VTK_Frame_Sink vtk(dir / "vtk_{step}.vtk", {}, 3);
  Svg_Frame_Sink svg(dir / "svg_{step}.svg",
                     Binary_RGB_Mapper<int>{0, {255, 255, 255}, {0, 0, 0}},
                     {},
                     3);

  png.accept(4, frame);
  npy.accept(4, frame);
  vtk.accept(4, frame);
  svg.accept(4, frame);

  EXPECT_EQ(read_file(dir / "png_004.png", true).substr(1, 3), "PNG");
  EXPECT_EQ(read_file(dir / "npy_004.npy", true).substr(1, 5), "NUMPY");
  EXPECT_NE(read_file(dir / "vtk_004.vtk").find("STRUCTURED_POINTS"), std::string::npos);
  EXPECT_NE(read_file(dir / "svg_004.svg").find("<svg"), std::string::npos);

  std::filesystem::remove_all(dir);
}
