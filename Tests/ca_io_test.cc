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

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/**
 * @file ca_io_test.cc
 * @brief Phase 11 tests for cellular-automata IO and image exporters.
 */

#include <cstdint>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include <ca-io.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_storage.H>

using namespace Aleph::CA;

namespace
{
  using Grid = Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;
  using ByteGrid = Lattice<Dense_Cell_Storage<std::uint8_t, 2>, OpenBoundary>;
  using Cell = std::pair<ca_index_t, ca_index_t>;

  Grid make_grid(ca_size_t rows, ca_size_t cols,
                 std::initializer_list<Cell> alive)
  {
    Grid g({rows, cols}, 0);
    for (const auto &c : alive)
      g.set({c.first, c.second}, 1);
    return g;
  }

  Grid make_values_grid()
  {
    Grid g({2, 3}, 0);
    int v = 1;
    for (ca_index_t r = 0; r < 2; ++r)
      for (ca_index_t c = 0; c < 3; ++c)
        g.set({r, c}, v++);
    return g;
  }

  std::set<Cell> alive_cells(const Grid &g)
  {
    std::set<Cell> out;
    for (ca_index_t r = 0; r < static_cast<ca_index_t>(g.size(0)); ++r)
      for (ca_index_t c = 0; c < static_cast<ca_index_t>(g.size(1)); ++c)
        if (g.at({r, c}) != 0)
          out.insert({r, c});
    return out;
  }

  void expect_same_frame(const Grid &a, const Grid &b)
  {
    ASSERT_EQ(a.size(0), b.size(0));
    ASSERT_EQ(a.size(1), b.size(1));
    for (ca_index_t r = 0; r < static_cast<ca_index_t>(a.size(0)); ++r)
      for (ca_index_t c = 0; c < static_cast<ca_index_t>(a.size(1)); ++c)
        EXPECT_EQ(a.at({r, c}), b.at({r, c})) << "at (" << r << ", " << c << ")";
  }

  std::string bytes(std::initializer_list<unsigned> values)
  {
    std::string out;
    for (const unsigned v : values)
      out.push_back(static_cast<char>(static_cast<unsigned char>(v)));
    return out;
  }
}

TEST(CAIO, RleFullFrameRoundTripPreservesExtentsAndCells)
{
  const Grid src = make_grid(4, 5, {{1, 2}, {2, 3}, {3, 1}, {3, 2}, {3, 3}});
  RLE_Write_Options opts;
  opts.name = "glider";
  opts.comment = "full-frame snapshot";

  const std::string rle = write_rle_string(src, opts);
  EXPECT_NE(rle.find("#N glider"), std::string::npos);
  EXPECT_NE(rle.find("#C full-frame snapshot"), std::string::npos);
  EXPECT_NE(rle.find("x = 5, y = 4, rule = B3/S23"), std::string::npos);

  const RLE_Pattern pattern = read_rle_string(rle);
  EXPECT_EQ(pattern.width, 5u);
  EXPECT_EQ(pattern.height, 4u);
  EXPECT_EQ(pattern.rule, "B3/S23");
  const Grid dst = make_lattice_from_pattern<Grid>(pattern);
  expect_same_frame(src, dst);
}

TEST(CAIO, RleTrimmedOutputMatchesCanonicalGliderShape)
{
  const Grid src = make_grid(6, 7, {{2, 3}, {3, 4}, {4, 2}, {4, 3}, {4, 4}});
  RLE_Write_Options opts;
  opts.trim_dead_border = true;
  opts.line_width = 0;

  const std::string rle = write_rle_string(src, opts);
  EXPECT_NE(rle.find("x = 3, y = 3, rule = B3/S23"), std::string::npos);
  EXPECT_NE(rle.find("bob$2bo$3o!"), std::string::npos) << rle;

  const Grid trimmed = make_lattice_from_pattern<Grid>(read_rle_string(rle));
  EXPECT_EQ(alive_cells(trimmed),
            (std::set<Cell>{{0, 1}, {1, 2}, {2, 0}, {2, 1}, {2, 2}}));
}

TEST(CAIO, RleParserAcceptsCommentsLongRunsAndConwayAliases)
{
  const std::string text =
    "#N mixed dialect\n"
    "#C A is accepted as alive and . as dead\n"
    "x = 5, y = 3, rule = B36/S23\n"
    "2A3b$5.$4bo!\n";
  const RLE_Pattern pattern = read_rle_string(text);
  EXPECT_EQ(pattern.name, "mixed dialect");
  ASSERT_EQ(pattern.comments.size(), 1u);
  EXPECT_EQ(pattern.rule, "B36/S23");
  EXPECT_EQ(pattern.width, 5u);
  EXPECT_EQ(pattern.height, 3u);
  EXPECT_EQ(pattern.alive,
            (Array<Coord_Vec<2>>{{0, 0}, {0, 1}, {2, 4}}));
}

TEST(CAIO, RleParserRejectsMalformedInput)
{
  EXPECT_THROW(read_rle_string("3o!\n"), std::domain_error);
  EXPECT_THROW(read_rle_string("x = 2, y = 1\n3o!\n"), std::domain_error);
  EXPECT_THROW(read_rle_string("x = 2, y = 1\n2"), std::domain_error);
  EXPECT_THROW(read_rle_string("x = 2, y = 1\nbo?\n"), std::domain_error);
}

// Regression (fuzz_rle_parser out-of-memory): a 34-byte document declared a
// huge row and a live run of 999999999 cells, so read_rle() tried to store
// about 16 GB of coordinates. The live-cell budget rejects it before any
// coordinate is stored.
TEST(CAIO, RleParserBoundsTheNumberOfLiveCells)
{
  const std::string hostile = "x = 1000000000, y = 1\n999999999o!\n";
  EXPECT_THROW((void) read_rle_string(hostile), std::length_error);

  auto r = try_read_rle_string(hostile);
  EXPECT_FALSE(r.has_value());

  // The budget is cumulative across runs and rows, and configurable.
  EXPECT_THROW((void) read_rle_string("x = 3, y = 2\n2o$2o!\n", 3), std::length_error);
  EXPECT_EQ(read_rle_string("x = 3, y = 2\n2o$o!\n", 3).alive.size(), 3u);

  // A sparse pattern may declare a huge area: only live cells are stored.
  const RLE_Pattern sparse = read_rle_string("x = 1000000000, y = 1000000000\no!\n");
  EXPECT_EQ(sparse.alive, (Array<Coord_Vec<2>>{{0, 0}}));
}

TEST(CAIO, PlaintextRoundTripPreservesFullFrame)
{
  const Grid src = make_grid(3, 4, {{0, 1}, {1, 2}, {2, 0}, {2, 1}, {2, 2}});
  Plaintext_Write_Options opts;
  opts.comment = "plaintext glider";
  const std::string text = write_plaintext_string(src, opts);
  EXPECT_NE(text.find("!plaintext glider"), std::string::npos);
  EXPECT_NE(text.find(".O.."), std::string::npos);

  const Binary_Cell_Pattern pattern = read_plaintext_string(text);
  EXPECT_EQ(pattern.width, 4u);
  EXPECT_EQ(pattern.height, 3u);
  const Grid dst = make_lattice_from_pattern<Grid>(pattern);
  expect_same_frame(src, dst);
}

TEST(CAIO, PlaintextParserAcceptsStarsAndRejectsUnknownGlyphs)
{
  const Binary_Cell_Pattern pattern = read_plaintext_string("!demo\n*..\n.O.\n");
  EXPECT_EQ(pattern.width, 3u);
  EXPECT_EQ(pattern.height, 2u);
  EXPECT_EQ(pattern.alive, (Array<Coord_Vec<2>>{{0, 0}, {1, 1}}));
  EXPECT_THROW(read_plaintext_string(".X.\n"), std::domain_error);
}

TEST(CAIO, Life106NormalisesNegativeCoordinates)
{
  const std::string text =
    "#Life 1.06\n"
    "#D shifted pair\n"
    "-2 -1\n"
    "0 1\n";
  std::istringstream in(text);
  const Binary_Cell_Pattern pattern = read_life_106(in);
  EXPECT_EQ(pattern.origin_col, -2);
  EXPECT_EQ(pattern.origin_row, -1);
  EXPECT_EQ(pattern.width, 3u);
  EXPECT_EQ(pattern.height, 3u);
  EXPECT_EQ(pattern.alive, (Array<Coord_Vec<2>>{{0, 0}, {2, 2}}));
}

TEST(CAIO, Life106WriterProducesReadableCoordinates)
{
  const Grid src = make_grid(3, 3, {{0, 2}, {2, 0}});
  std::ostringstream out;
  write_life_106(out, src);

  std::istringstream in(out.str());
  const Binary_Cell_Pattern pattern = read_life_106(in);
  EXPECT_EQ(pattern.width, 3u);
  EXPECT_EQ(pattern.height, 3u);
  EXPECT_EQ(pattern.alive, (Array<Coord_Vec<2>>{{0, 2}, {2, 0}}));
}

TEST(CAIO, Life105RoundTripPreservesFullFrame)
{
  const Grid src = make_grid(3, 4, {{0, 0}, {1, 2}, {2, 3}});
  std::ostringstream out;
  write_life_105(out, src, "single block");
  EXPECT_NE(out.str().find("#Life 1.05"), std::string::npos);
  EXPECT_NE(out.str().find("#P 0 0"), std::string::npos);

  std::istringstream in(out.str());
  const Binary_Cell_Pattern pattern = read_life_105(in);
  const Grid dst = make_lattice_from_pattern<Grid>(pattern);
  expect_same_frame(src, dst);
}

TEST(CAIO, Life105ParserMergesNegativeBlocks)
{
  const std::string text =
    "#Life 1.05\n"
    "#P -1 -1\n"
    "*.\n"
    ".*\n"
    "#P 2 0\n"
    "*\n";
  std::istringstream in(text);
  const Binary_Cell_Pattern pattern = read_life_105(in);
  EXPECT_EQ(pattern.origin_col, -1);
  EXPECT_EQ(pattern.origin_row, -1);
  EXPECT_EQ(pattern.width, 4u);
  EXPECT_EQ(pattern.height, 2u);
  EXPECT_EQ(pattern.alive,
            (Array<Coord_Vec<2>>{{0, 0}, {1, 1}, {1, 3}}));
}

TEST(CAIO, CsvRoundTripPreservesNumericSnapshot)
{
  const Grid src = make_values_grid();
  CSV_Options opts;
  opts.delimiter = ';';

  std::ostringstream out;
  write_csv(out, src, opts);
  EXPECT_EQ(out.str(), "1;2;3\n4;5;6\n");

  std::istringstream in(out.str());
  const Grid_Snapshot<int> snap = read_csv_snapshot<int>(in, opts);
  const Grid dst = make_lattice_from_snapshot<Grid>(snap);
  expect_same_frame(src, dst);
}

TEST(CAIO, CsvReaderRejectsRaggedRowsAndBadScalars)
{
  {
    std::istringstream in("1,2\n3\n");
    EXPECT_THROW(read_csv_snapshot<int>(in), std::domain_error);
  }
  {
    std::istringstream in("1,x\n");
    EXPECT_THROW(read_csv_snapshot<int>(in), std::domain_error);
  }
}

TEST(CAIO, JsonRoundTripPreservesNumericSnapshot)
{
  const Grid src = make_values_grid();
  JSON_Write_Options opts;
  opts.pretty = true;
  opts.rule = "demo";

  std::ostringstream out;
  write_json(out, src, opts);
  EXPECT_NE(out.str().find("\"type\": \"Aleph::CA::frame\""), std::string::npos);
  EXPECT_NE(out.str().find("\"rule\": \"demo\""), std::string::npos);

  std::istringstream in(out.str());
  const Grid_Snapshot<int> snap = read_json_snapshot<int>(in);
  const Grid dst = make_lattice_from_snapshot<Grid>(snap);
  expect_same_frame(src, dst);
}

TEST(CAIO, CsvAndJsonWriteByteStatesAsNumbers)
{
  ByteGrid frame({1, 3}, std::uint8_t{0});
  frame.set({0, 0}, std::uint8_t{7});
  frame.set({0, 1}, std::uint8_t{42});
  frame.set({0, 2}, std::uint8_t{255});

  std::ostringstream csv;
  write_csv(csv, frame);
  EXPECT_EQ(csv.str(), "7,42,255\n");

  std::ostringstream json;
  write_json(json, frame);
  EXPECT_NE(json.str().find("\"cells\":[[7,42,255]]"), std::string::npos);
}

TEST(CAIO, JsonReaderRejectsShapeMismatch)
{
  std::istringstream in(
    "{\"type\":\"Aleph::CA::frame\",\"rank\":2,\"height\":1,\"width\":2,"
    "\"cells\":[[1,2],[3,4]]}");
  EXPECT_THROW(read_json_snapshot<int>(in), std::domain_error);
}

TEST(CAIO, JsonFrameStreamWriterEmitsStepTaggedFrames)
{
  const Grid frame = make_grid(2, 2, {{0, 0}, {1, 1}});
  std::ostringstream out;
  {
    JSON_Frame_Stream_Writer stream(out);
    stream.accept(0, frame);
    stream.accept(4, frame);
    stream.close();
  }
  EXPECT_NE(out.str().find("\"type\": \"Aleph::CA::frame-stream\""), std::string::npos);
  EXPECT_NE(out.str().find("\"step\": 0"), std::string::npos);
  EXPECT_NE(out.str().find("\"step\": 4"), std::string::npos);
  EXPECT_NE(out.str().find("\"cells\":[[1,0],[0,1]]"), std::string::npos);
}

TEST(CAIO, PgmBinaryOutputIsBitStable)
{
  const Grid frame = make_grid(2, 3, {{0, 1}, {1, 2}});
  std::ostringstream out(std::ios::binary);
  write_pgm(out, frame);

  std::string expected = "P5\n3 2\n255\n";
  expected += bytes({255, 0, 255, 255, 255, 0});
  EXPECT_EQ(out.str(), expected);
}

TEST(CAIO, PpmBinaryOutputUsesCustomPalette)
{
  const Grid frame = make_grid(1, 2, {{0, 1}});
  std::ostringstream out(std::ios::binary);
  write_ppm(out, frame, [](int v)
  {
    return v == 0 ? RGB8{1, 2, 3} : RGB8{250, 0, 7};
  });

  std::string expected = "P6\n2 1\n255\n";
  expected += bytes({1, 2, 3, 250, 0, 7});
  EXPECT_EQ(out.str(), expected);
}

// Coordinates that fit in ca_index_t one by one may not fit once they are
// added up or subtracted. The readers computed them with the overflow of
// C++, undefined behaviour; now they reject such patterns with
// std::overflow_error, and the non-throwing readers report it.
TEST(CAIO, Life106RejectsAnExtentThatDoesNotFit)
{
  // max - min is 2^63, one more than the largest ca_index_t
  std::istringstream too_wide("#Life 1.06\n-4611686018427387904 0\n4611686018427387904 0\n");
  EXPECT_THROW((void) read_life_106(too_wide), std::overflow_error);

  std::istringstream too_tall("#Life 1.06\n0 -4611686018427387904\n0 4611686018427387904\n");
  EXPECT_THROW((void) read_life_106(too_tall), std::overflow_error);

  std::istringstream again("#Life 1.06\n-4611686018427387904 0\n4611686018427387904 0\n");
  const auto attempt = try_read_life_106(again);
  ASSERT_FALSE(attempt.has_value());
  EXPECT_NE(attempt.error().find("extent"), std::string::npos);

  // the widest pattern: its columns go from 0 to the largest ca_index_t
  std::istringstream widest("#Life 1.06\n-4611686018427387904 0\n4611686018427387903 0\n");
  const Binary_Cell_Pattern pattern = read_life_106(widest);
  EXPECT_EQ(pattern.width,
            static_cast<ca_size_t>(std::numeric_limits<ca_index_t>::max()) + 1);
  EXPECT_EQ(pattern.height, 1u);
  EXPECT_EQ(pattern.origin_col, -4611686018427387904LL);
  ASSERT_EQ(pattern.alive.size(), 2u);
  const std::set<std::pair<ca_index_t, ca_index_t>> cells = {
    {pattern.alive(0)[0], pattern.alive(0)[1]}, {pattern.alive(1)[0], pattern.alive(1)[1]}};
  const std::set<std::pair<ca_index_t, ca_index_t>> expected = {
    {0, 0}, {0, std::numeric_limits<ca_index_t>::max()}};
  EXPECT_EQ(cells, expected);
}

TEST(CAIO, Life105RejectsCellsBeyondTheLargestCoordinate)
{
  // the block starts at the largest column, so its second cell overflows
  std::istringstream past_end("#Life 1.05\n#P 9223372036854775807 0\n**\n");
  EXPECT_THROW((void) read_life_105(past_end), std::overflow_error);

  std::istringstream past_bottom("#Life 1.05\n#P 0 9223372036854775807\n*\n*\n");
  EXPECT_THROW((void) read_life_105(past_bottom), std::overflow_error);

  std::istringstream again("#Life 1.05\n#P 9223372036854775807 0\n**\n");
  EXPECT_FALSE(try_read_life_105(again).has_value());

  // the last cells that fit
  std::istringstream at_end("#Life 1.05\n#P 9223372036854775806 0\n**\n");
  const Binary_Cell_Pattern pattern = read_life_105(at_end);
  EXPECT_EQ(pattern.origin_col, std::numeric_limits<ca_index_t>::max() - 1);
  EXPECT_EQ(pattern.width, 2u);
  EXPECT_EQ(pattern.alive.size(), 2u);

  // two blocks far apart: each cell fits, the extent does not
  std::istringstream far_apart("#Life 1.05\n#P -9223372036854775807 0\n*\n#P 1 0\n*\n");
  EXPECT_THROW((void) read_life_105(far_apart), std::overflow_error);

  // negative blocks that fit
  std::istringstream negative("#Life 1.05\n#P -9223372036854775807 -5\n*.*\n");
  const Binary_Cell_Pattern left = read_life_105(negative);
  EXPECT_EQ(left.origin_col, -std::numeric_limits<ca_index_t>::max());
  EXPECT_EQ(left.origin_row, -5);
  EXPECT_EQ(left.width, 3u);
}

// A run long enough to wrap the unsigned sum of the size checks went back
// to a previous column or row, and stored the same cell twice.
TEST(CAIO, RleRunsThatWouldWrapAreRejected)
{
  EXPECT_THROW((void) read_rle_string("x = 5, y = 1\n3o18446744073709551615b2o!"),
               std::domain_error);
  EXPECT_THROW((void) read_rle_string("x = 5, y = 1\n3o18446744073709551615o!"),
               std::domain_error);
  EXPECT_THROW((void) read_rle_string("x = 1, y = 2\no$18446744073709551615$o!"),
               std::domain_error);

  // a size whose cells would not have ca_index_t coordinates
  EXPECT_THROW((void) read_rle_string("x = 9223372036854775809, y = 1\n!"),
               std::overflow_error);
  EXPECT_THROW((void) read_rle_string("x = 1, y = 9223372036854775809\n!"),
               std::overflow_error);

  // the largest size: its last cell is at the largest ca_index_t
  const RLE_Pattern largest
    = read_rle_string("x = 9223372036854775808, y = 9223372036854775808\n"
                      "9223372036854775807bo9223372036854775807$9223372036854775807bo!");
  EXPECT_EQ(largest.width, static_cast<ca_size_t>(std::numeric_limits<ca_index_t>::max()) + 1);
  EXPECT_EQ(largest.height, largest.width);
  ASSERT_EQ(largest.alive.size(), 2u);
  EXPECT_EQ(largest.alive(0)[0], 0);
  EXPECT_EQ(largest.alive(0)[1], std::numeric_limits<ca_index_t>::max());
  EXPECT_EQ(largest.alive(1)[0], std::numeric_limits<ca_index_t>::max());
  EXPECT_EQ(largest.alive(1)[1], std::numeric_limits<ca_index_t>::max());
}

// The fit check added the origin to the size, and a huge size wrapped it.
TEST(CAIO, LoadBinaryPatternRejectsSizesThatWouldWrap)
{
  Binary_Cell_Pattern pattern;
  pattern.height = std::numeric_limits<ca_size_t>::max();
  pattern.width = 1;

  Grid grid({4, 4}, 0);
  EXPECT_THROW(load_binary_pattern(pattern, grid, 1, 0, 1, 0), std::domain_error);

  pattern.height = 1;
  pattern.width = std::numeric_limits<ca_size_t>::max();
  EXPECT_THROW(load_binary_pattern(pattern, grid, 1, 0, 0, 1), std::domain_error);

  pattern.width = 4;
  pattern.alive.append(Coord_Vec<2>{0, 3});
  load_binary_pattern(pattern, grid, 1, 0, 3, 0);
  EXPECT_EQ(grid.at({3, 3}), 1);
  EXPECT_THROW(load_binary_pattern(pattern, grid, 1, 0, 4, 0), std::domain_error);
}
