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


/** @file ah-source.H
 *  @brief Source file and span management utilities for compiler-style tooling.
 *
 *  This header provides a lightweight source manager suitable for lexers,
 *  parsers, semantic analyzers, and diagnostic engines. Source files are stored
 *  as immutable texts indexed by stable file ids. Clients operate on byte
 *  offsets and half-open spans `[begin, end)`.
 *
 *  The current implementation uses byte offsets over UTF-8 text. This keeps the
 *  data model simple and deterministic for lexer/parser pipelines while still
 *  supporting precise line/column reporting.
 *
 *  @ingroup Utilities
 */

# ifndef AH_SOURCE_H
# define AH_SOURCE_H

# include <cstddef>
# include <fstream>
# include <sstream>
# include <string>
# include <utility>

# include <ah-errors.H>
# include <tpl_dynArray.H>

namespace Aleph
{
  using Source_File_Id = size_t;
  using Source_Offset = size_t;

  /** @brief Fully resolved source position.
   *
   *  A position belongs to a file, keeps the absolute byte offset in that file,
   *  and caches 1-based line/column numbers for presentation.
   */
  struct Source_Position
  {
    Source_File_Id file_id = 0; ///< Stable file identifier; `0` means invalid.
    std::string file_name; ///< Human-readable file name.
    Source_Offset offset = 0; ///< Byte offset in the file text.
    size_t line = 1; ///< 1-based line number.
    size_t column = 1; ///< 1-based column number.

    /** @brief Returns whether the position refers to a registered file. */
    bool is_valid() const noexcept
    {
      return file_id != 0;
    }

    /** @brief Formats the position as `file:line:column` or `line:column`. */
    std::string to_string() const
    {
      std::ostringstream out;
      if (not file_name.empty())
        out << file_name << ":";
      out << line << ":" << column;
      return out.str();
    }
  };

  /** @brief Half-open byte range inside a source file.
   *
   *  Spans are stable, cheap value objects used by tokens, AST nodes, symbols,
   *  and diagnostics. The range is `[begin, end)`, so empty spans are allowed.
   */
  struct Source_Span
  {
    Source_File_Id file_id = 0; ///< Stable file identifier; `0` means invalid.
    Source_Offset begin = 0; ///< Inclusive begin byte offset.
    Source_Offset end = 0; ///< Exclusive end byte offset.

    /** @brief Returns whether the span belongs to a registered file. */
    bool is_valid() const noexcept
    {
      return file_id != 0 and begin <= end;
    }

    /** @brief Returns whether the span contains no bytes. */
    bool empty() const noexcept
    {
      return begin == end;
    }

    /** @brief Returns the span length in bytes. */
    size_t size() const noexcept
    {
      return end - begin;
    }

    /** @brief Returns whether `offset` lies inside the half-open range. */
    bool contains(const Source_Offset offset) const noexcept
    {
      return begin <= offset and offset < end;
    }

    /** @brief Formats the span as `#file:[begin,end)`. */
    std::string to_string() const
    {
      std::ostringstream out;
      if (not is_valid())
        {
          out << "<invalid-span>";
          return out.str();
        }

      out << "#" << file_id << ":[" << begin << "," << end << ")";
      return out.str();
    }
  };

  /** @brief Registered source file with cached line starts.
   *
   *  Users typically access files indirectly through `Source_Manager`, but the
   *  record is public so downstream tooling can inspect file metadata when
   *  needed.
   */
  struct Source_File
  {
    Source_File_Id id = 0; ///< Stable file identifier.
    std::string name; ///< User-facing file name or path.
    std::string text; ///< Full file contents.
    DynArray<Source_Offset> line_starts; ///< Byte offsets for each line start.
  };

  /** @brief Render-ready excerpt for a single source line.
   *
   *  Snippets are derived from a `Source_Span` and prepared for plain-text
   *  diagnostic rendering. The highlight is clipped to a single line.
   */
  struct Source_Snippet
  {
    std::string file_name; ///< File name owning the snippet.
    size_t line = 1; ///< 1-based line number.
    size_t column = 1; ///< 1-based begin column.
    std::string line_text; ///< Raw line contents without trailing newline.
    size_t highlight_column = 1; ///< 1-based column where the underline starts.
    size_t highlight_width = 1; ///< Width of the underline, always >= 1.
  };

  /** @brief Stores source files and resolves offsets into human-readable data.
   *
   *  `Source_Manager` owns source texts and offers the core conversions required
   *  by lexer/parser infrastructure:
   *
   *  - file registration from memory or disk
   *  - line/column resolution from byte offsets
   *  - span validation and slicing
   *  - single-line snippets for diagnostics
   */
  class Source_Manager
  {
    DynArray<Source_File> files;

    static DynArray<Source_Offset>
    compute_line_starts(const std::string & text)
    {
      DynArray<Source_Offset> starts;
      starts.append(0);

      for (size_t i = 0; i < text.size(); ++i)
        if (text[i] == '\n')
          starts.append(i + 1);

      return starts;
    }

    size_t checked_index(const Source_File_Id id) const
    {
      ah_out_of_range_error_unless(id > 0 and id <= files.size())
        << "Source_Manager: invalid file id " << id;
      return id - 1;
    }

    const Source_File & checked_file(const Source_File_Id id) const
    {
      return files.access(checked_index(id));
    }

    size_t line_index_from_offset(const Source_File & file,
                                  const Source_Offset off) const
    {
      ah_out_of_range_error_unless(off <= file.text.size())
        << "Source_Manager: offset " << off
        << " out of range for file '" << file.name << "'";

      size_t lo = 0;
      size_t hi = file.line_starts.size();
      while (lo + 1 < hi)
        {
          const size_t mid = lo + (hi - lo)/2;
          if (file.line_starts.access(mid) <= off)
            lo = mid;
          else
            hi = mid;
        }

      return lo;
    }

    size_t line_end_offset(const Source_File & file, const size_t line_idx) const
    {
      ah_out_of_range_error_unless(line_idx < file.line_starts.size())
        << "Source_Manager: invalid line index " << line_idx;

      size_t end = line_idx + 1 < file.line_starts.size() ?
          file.line_starts.access(line_idx + 1) : file.text.size();

      if (end > file.line_starts.access(line_idx)
          and end <= file.text.size() and file.text[end - 1] == '\n')
        --end;
      if (end > file.line_starts.access(line_idx)
          and end <= file.text.size() and file.text[end - 1] == '\r')
        --end;

      return end;
    }

  public:
    /** @brief Registers an in-memory source file.
     *
     *  @param name User-facing file name.
     *  @param text Full source contents.
     *  @return Stable file identifier greater than zero.
     */
    Source_File_Id add_virtual_file(const std::string & name,
                                    const std::string & text)
    {
      Source_File file;
      file.id = files.size() + 1;
      file.name = name;
      file.text = text;
      file.line_starts = compute_line_starts(file.text);
      files.append(std::move(file));
      return files.size();
    }

    /** @brief Loads a file from disk and registers it.
     *
     *  @param path Path to the file to load.
     *  @return Stable file identifier greater than zero.
     *  @throws std::runtime_error If the file cannot be opened.
     */
    Source_File_Id load_file(const std::string & path)
    {
      std::ifstream input(path, std::ios::binary);
      ah_runtime_error_unless(input.is_open())
        << "Source_Manager: cannot open file '" << path << "'";

      std::ostringstream out;
      out << input.rdbuf();
      return add_virtual_file(path, out.str());
    }

    /** @brief Returns the registered file record for `id`. */
    const Source_File & file(const Source_File_Id id) const
    {
      return checked_file(id);
    }

    /** @brief Returns the display name of file `id`. */
    const std::string & file_name(const Source_File_Id id) const
    {
      return checked_file(id).name;
    }

    /** @brief Returns the full text of file `id`. */
    const std::string & file_text(const Source_File_Id id) const
    {
      return checked_file(id).text;
    }

    /** @brief Returns the size in bytes of file `id`. */
    size_t file_size(const Source_File_Id id) const
    {
      return checked_file(id).text.size();
    }

    /** @brief Returns the number of registered files. */
    size_t num_files() const noexcept
    {
      return files.size();
    }

    /** @brief Creates a validated span in file `id`.
     *
     *  @param id File id.
     *  @param begin Inclusive begin offset.
     *  @param end Exclusive end offset.
     *  @return Valid span referencing `id`.
     *  @throws std::out_of_range If offsets are invalid.
     */
    Source_Span span(const Source_File_Id id,
                     const Source_Offset begin,
                     const Source_Offset end) const
    {
      const auto & src = checked_file(id);
      ah_out_of_range_error_unless(begin <= end and end <= src.text.size())
        << "Source_Manager: invalid span [" << begin << ", " << end
        << ") for file '" << src.name << "'";

      return {id, begin, end};
    }

    /** @brief Resolves `off` to file/offset/line/column information. */
    Source_Position position(const Source_File_Id id, const Source_Offset off) const
    {
      const auto & src = checked_file(id);
      const size_t line_idx = line_index_from_offset(src, off);
      const Source_Offset line_start = src.line_starts.access(line_idx);
      return {id, src.name, off, line_idx + 1, off - line_start + 1};
    }

    /** @brief Resolves the begin of `span` to a concrete source position. */
    Source_Position begin_position(const Source_Span & span) const
    {
      ah_invalid_argument_unless(span.is_valid())
        << "Source_Manager::begin_position(): invalid span";
      return position(span.file_id, span.begin);
    }

    /** @brief Resolves the end of `span` to a concrete source position. */
    Source_Position end_position(const Source_Span & span) const
    {
      ah_invalid_argument_unless(span.is_valid())
        << "Source_Manager::end_position(): invalid span";
      return position(span.file_id, span.end);
    }

    /** @brief Returns the exact text covered by `span`. */
    std::string slice(const Source_Span & span) const
    {
      ah_invalid_argument_unless(span.is_valid())
        << "Source_Manager::slice(): invalid span";

      const auto & src = checked_file(span.file_id);
      ah_out_of_range_error_unless(span.end <= src.text.size())
        << "Source_Manager::slice(): span end out of range";

      return src.text.substr(span.begin, span.size());
    }

    /** @brief Builds a single-line diagnostic snippet from `span`.
     *
     *  Multi-line spans are clipped to the first line because plain-text
     *  diagnostics in the MVP render a single highlighted line.
     *
     *  @param span Span to inspect.
     *  @return Render-ready snippet.
     */
    Source_Snippet snippet(const Source_Span & span) const
    {
      ah_invalid_argument_unless(span.is_valid())
        << "Source_Manager::snippet(): invalid span";

      const auto & src = checked_file(span.file_id);
      const auto begin_pos = begin_position(span);
      const size_t line_idx = begin_pos.line - 1;
      const Source_Offset line_begin = src.line_starts.access(line_idx);
      const Source_Offset line_end = line_end_offset(src, line_idx);

      Source_Offset clipped_end = span.end;
      if (clipped_end > line_end)
        clipped_end = line_end;
      if (clipped_end < span.begin)
        clipped_end = span.begin;

      size_t width = clipped_end - span.begin;
      if (width == 0)
        width = 1;

      return {
        src.name,
        begin_pos.line,
        begin_pos.column,
        src.text.substr(line_begin, line_end - line_begin),
        begin_pos.column,
        width
      };
    }
  };
}

# endif // AH_SOURCE_H
