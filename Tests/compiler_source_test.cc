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
 * @file compiler_source_test.cc
 * @brief Tests for ah-source.H.
 */

# include <gtest/gtest.h>

# include <ah-source.H>

# include <atomic>
# include <chrono>
# include <filesystem>
# include <fstream>
# include <string>
#if defined(_WIN32)
#  include <process.h>
#else
#  include <unistd.h>
#endif

using namespace Aleph;

namespace
{
  /// Process ID of the current process, portable across POSIX and Windows.
  long long process_id() noexcept
  {
#if defined(_WIN32)
    return static_cast<long long>(_getpid());
#else
    return static_cast<long long>(getpid());
#endif
  }

  std::filesystem::path
  make_temp_file_path(const std::string & stem)
  {
    namespace fs = std::filesystem;
    // A steady_clock tick alone is unique *within* a process, but not
    // across the several processes that actually run this suite (each
    // TEST() is its own ctest process, and CI runs ctest with
    // --parallel) -- two processes' calls landing in the same clock
    // tick produce the identical name and race on the same file.
    // Mixing in the process id and a per-process counter closes that
    // gap regardless of clock resolution or call count.
    static std::atomic<unsigned long long> counter{0};
    return fs::temp_directory_path() /
        ("aleph_" + stem + "_" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
         "_" + std::to_string(process_id()) +
         "_" + std::to_string(counter++) +
         ".txt");
  }
}


TEST(SourceManager, AddVirtualFileStoresNameAndText)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "let x = 1;\n");

  EXPECT_EQ(sm.num_files(), 1u);
  EXPECT_EQ(sm.file_name(id), "main.aw");
  EXPECT_EQ(sm.file_text(id), "let x = 1;\n");
  EXPECT_EQ(sm.file_size(id), 11u);
}


TEST(SourceManager, PositionResolvesLineAndColumn)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "ab\ncde\n");

  const auto p0 = sm.position(id, 0);
  EXPECT_EQ(p0.line, 1u);
  EXPECT_EQ(p0.column, 1u);

  const auto p2 = sm.position(id, 2);
  EXPECT_EQ(p2.line, 1u);
  EXPECT_EQ(p2.column, 3u);

  const auto p3 = sm.position(id, 3);
  EXPECT_EQ(p3.line, 2u);
  EXPECT_EQ(p3.column, 1u);

  const auto p7 = sm.position(id, 7);
  EXPECT_EQ(p7.line, 3u);
  EXPECT_EQ(p7.column, 1u);
}


TEST(SourceManager, SpanSliceReturnsExpectedSubstring)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "alpha beta gamma");
  const auto sp = sm.span(id, 6, 10);

  EXPECT_FALSE(sp.empty());
  EXPECT_EQ(sp.size(), 4u);
  EXPECT_TRUE(sp.contains(7));
  EXPECT_EQ(sm.slice(sp), "beta");
}


TEST(SourceManager, SnippetMarksSingleLineSpan)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "alpha beta gamma\n");
  const auto sp = sm.span(id, 6, 10);
  const auto snip = sm.snippet(sp);

  EXPECT_EQ(snip.file_name, "main.aw");
  EXPECT_EQ(snip.line, 1u);
  EXPECT_EQ(snip.column, 7u);
  EXPECT_EQ(snip.line_text, "alpha beta gamma");
  EXPECT_EQ(snip.highlight_column, 7u);
  EXPECT_EQ(snip.highlight_width, 4u);
}


TEST(SourceManager, EmptySpanIsAllowedAndProducesCaretWidthOne)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "abc");
  const auto sp = sm.span(id, 1, 1);
  const auto snip = sm.snippet(sp);

  EXPECT_TRUE(sp.empty());
  EXPECT_EQ(sm.slice(sp), "");
  EXPECT_EQ(snip.highlight_column, 2u);
  EXPECT_EQ(snip.highlight_width, 1u);
}


TEST(SourceManager, LoadFileReadsContent)
{
  namespace fs = std::filesystem;

  const fs::path path = make_temp_file_path("source_manager");
  {
    std::ofstream out(path, std::ios::binary);
    out << "line1\nline2\n";
  }

  Source_Manager sm;
  const auto id = sm.load_file(path.string());
  EXPECT_EQ(sm.file_name(id), path.string());
  EXPECT_EQ(sm.file_text(id), "line1\nline2\n");

  fs::remove(path);
}


TEST(SourceManager, InvalidOffsetThrows)
{
  Source_Manager sm;
  const auto id = sm.add_virtual_file("main.aw", "abc");

  EXPECT_THROW((void) sm.position(id, 4), std::out_of_range);
  EXPECT_THROW((void) sm.span(id, 1, 4), std::out_of_range);
  EXPECT_THROW((void) sm.file(999), std::out_of_range);
}
