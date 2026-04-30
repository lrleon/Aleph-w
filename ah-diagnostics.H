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


/** @file ah-diagnostics.H
 *  @brief Plain-text diagnostic engine for compiler-style tooling.
 *
 *  The diagnostic engine stores errors, warnings, notes, and fatal messages
 *  together with source spans managed by `Source_Manager`. Diagnostics are
 *  accumulated first and rendered later, which makes the component suitable for
 *  lexers, parsers, semantic analyzers, and IDE-oriented tooling.
 *
 *  The MVP renderer is intentionally simple and deterministic. It produces
 *  stable plain-text output without ANSI color sequences.
 *
 *  @ingroup Utilities
 */

# ifndef AH_DIAGNOSTICS_H
# define AH_DIAGNOSTICS_H

# include <ostream>
# include <sstream>
# include <string>

# include <ah-source.H>
# include <tpl_dynArray.H>

namespace Aleph
{
  /** @brief Severity level of a diagnostic message. */
  enum class Diagnostic_Severity
  {
    Note,
    Warning,
    Error,
    Fatal
  };

  /** @brief Kind of source label attached to a diagnostic. */
  enum class Diagnostic_Label_Kind
  {
    Primary,
    Secondary
  };

  /** @brief Span annotation attached to a diagnostic.
   *
   *  Primary labels usually mark the main source of the error. Secondary labels
   *  provide related context such as previous declarations or conflicting uses.
   */
  struct Diagnostic_Label
  {
    Source_Span span; ///< Highlighted region.
    Diagnostic_Label_Kind kind = Diagnostic_Label_Kind::Primary; ///< Label role.
    std::string message; ///< Optional label-specific message.
  };

  /** @brief Fully accumulated diagnostic.
   *
   *  The `message` is the headline shown next to the severity. Labels, notes,
   *  and help entries provide source and remediation context.
   */
  struct Diagnostic
  {
    Diagnostic_Severity severity = Diagnostic_Severity::Error; ///< Severity level.
    std::string code; ///< Optional stable code such as `E001`.
    std::string message; ///< Main headline message.
    DynArray<Diagnostic_Label> labels; ///< Source annotations.
    DynArray<std::string> notes; ///< Additional notes.
    DynArray<std::string> help; ///< Suggested next actions.
  };

  class Diagnostic_Engine;

  /** @brief Fluent builder used to enrich a just-created diagnostic. */
  class Diagnostic_Builder
  {
    Diagnostic_Engine * engine = nullptr;
    size_t diag_index = 0;

    Diagnostic & diag();

  public:
    /** @brief Constructs a builder bound to `engine[index]`. */
    Diagnostic_Builder(Diagnostic_Engine & engine, const size_t index) noexcept
      : engine(&engine), diag_index(index)
    {}

    /** @brief Sets the stable diagnostic code. */
    Diagnostic_Builder & code(const std::string & value);

    /** @brief Adds a primary label.
     *
     *  @param span Span to highlight.
     *  @param msg Optional label-specific message shown near the underline.
     */
    Diagnostic_Builder & primary(const Source_Span & span,
                                 const std::string & msg = "");

    /** @brief Adds a secondary label. */
    Diagnostic_Builder & secondary(const Source_Span & span,
                                   const std::string & msg);

    /** @brief Appends a note line. */
    Diagnostic_Builder & note(const std::string & msg);

    /** @brief Appends a help line. */
    Diagnostic_Builder & help(const std::string & msg);

    /** @brief Finalizes the builder and returns the diagnostic index. */
    size_t emit() const noexcept
    {
      return diag_index;
    }
  };

  /** @brief Diagnostic accumulator and renderer.
   *
   *  Diagnostics are appended through convenience entry points such as `error()`
   *  and `warning()`. The engine keeps counters by severity and renders plain
   *  text snippets using its associated `Source_Manager`.
   */
  class Diagnostic_Engine
  {
    const Source_Manager * sources = nullptr;
    DynArray<Diagnostic> diags;
    size_t errors = 0;
    size_t warnings = 0;

    static const char * severity_string(const Diagnostic_Severity severity)
    {
      switch (severity)
        {
        case Diagnostic_Severity::Note: return "note";
        case Diagnostic_Severity::Warning: return "warning";
        case Diagnostic_Severity::Error: return "error";
        case Diagnostic_Severity::Fatal: return "fatal";
        }

      return "diagnostic";
    }

    size_t append_diagnostic(const Diagnostic_Severity severity,
                             const Source_Span & span,
                             const std::string & msg)
    {
      Diagnostic diag;
      diag.severity = severity;
      diag.message = msg;
      if (span.is_valid())
        diag.labels.append({span, Diagnostic_Label_Kind::Primary, ""});

      diags.append(std::move(diag));
      if (severity == Diagnostic_Severity::Warning)
        ++warnings;
      else if (severity == Diagnostic_Severity::Error
               or severity == Diagnostic_Severity::Fatal)
        ++errors;

      return diags.size() - 1;
    }

    const Diagnostic_Label *
    first_primary_label(const Diagnostic & diag) const noexcept
    {
      for (size_t i = 0; i < diag.labels.size(); ++i)
        {
          const auto & label = diag.labels.access(i);
          if (label.kind == Diagnostic_Label_Kind::Primary and label.span.is_valid())
            return &label;
        }

      for (size_t i = 0; i < diag.labels.size(); ++i)
        {
          const auto & label = diag.labels.access(i);
          if (label.span.is_valid())
            return &label;
        }

      return nullptr;
    }

    void render_label_snippet(std::ostream & out,
                              const Diagnostic_Label & label) const
    {
      if (not label.span.is_valid())
        return;

      const auto pos = sources->begin_position(label.span);
      const auto snip = sources->snippet(label.span);

      out << "  --> " << pos.to_string() << '\n';

      std::ostringstream ln;
      ln << snip.line;
      const std::string line_no = ln.str();

      out << " " << line_no << " | " << snip.line_text << '\n';
      out << " " << std::string(line_no.size(), ' ') << " | ";
      out << std::string(snip.highlight_column > 1 ? snip.highlight_column - 1 : 0, ' ');
      out << '^';
      for (size_t i = 1; i < snip.highlight_width; ++i)
        out << '~';
      if (not label.message.empty())
        out << " " << label.message;
      out << '\n';
    }

    void render_secondary_labels(std::ostream & out,
                                 const Diagnostic & diag) const
    {
      for (size_t i = 0; i < diag.labels.size(); ++i)
        {
          const auto & label = diag.labels.access(i);
          if (label.kind != Diagnostic_Label_Kind::Secondary)
            continue;

          out << "  = related";
          if (label.span.is_valid())
            out << ": " << sources->begin_position(label.span).to_string();
          if (not label.message.empty())
            out << ": " << label.message;
          out << '\n';
        }
    }

  public:
    /** @brief Constructs an engine bound to `sm`. */
    explicit Diagnostic_Engine(const Source_Manager & sm) noexcept
      : sources(&sm)
    {}

    /** @brief Starts an error diagnostic. */
    Diagnostic_Builder error(const Source_Span & span, const std::string & msg)
    {
      return Diagnostic_Builder(*this,
                                append_diagnostic(Diagnostic_Severity::Error,
                                                  span, msg));
    }

    /** @brief Starts a warning diagnostic. */
    Diagnostic_Builder warning(const Source_Span & span, const std::string & msg)
    {
      return Diagnostic_Builder(*this,
                                append_diagnostic(Diagnostic_Severity::Warning,
                                                  span, msg));
    }

    /** @brief Starts a fatal diagnostic. */
    Diagnostic_Builder fatal(const Source_Span & span, const std::string & msg)
    {
      return Diagnostic_Builder(*this,
                                append_diagnostic(Diagnostic_Severity::Fatal,
                                                  span, msg));
    }

    /** @brief Starts a note diagnostic. */
    Diagnostic_Builder note(const Source_Span & span, const std::string & msg)
    {
      return Diagnostic_Builder(*this,
                                append_diagnostic(Diagnostic_Severity::Note,
                                                  span, msg));
    }

    /** @brief Returns diagnostic `i`. */
    const Diagnostic & get(const size_t i) const
    {
      ah_out_of_range_error_unless(i < diags.size())
        << "Diagnostic_Engine::get(): invalid index " << i;
      return diags.access(i);
    }

    /** @brief Returns the number of stored diagnostics. */
    size_t size() const noexcept
    {
      return diags.size();
    }

    /** @brief Returns the number of error and fatal diagnostics. */
    size_t error_count() const noexcept
    {
      return errors;
    }

    /** @brief Returns the number of warnings. */
    size_t warning_count() const noexcept
    {
      return warnings;
    }

    /** @brief Returns whether any error or fatal diagnostic was emitted. */
    bool has_errors() const noexcept
    {
      return errors > 0;
    }

    /** @brief Removes all accumulated diagnostics and resets counters. */
    void clear() noexcept
    {
      diags.clear();
      errors = 0;
      warnings = 0;
    }

    /** @brief Renders all diagnostics as deterministic plain text. */
    void render_plain(std::ostream & out) const
    {
      for (size_t i = 0; i < diags.size(); ++i)
        {
          const auto & diag = diags.access(i);
          out << severity_string(diag.severity);
          if (not diag.code.empty())
            out << "[" << diag.code << "]";
          out << ": " << diag.message << '\n';

          if (const auto * primary = first_primary_label(diag);
              primary != nullptr)
            render_label_snippet(out, *primary);

          render_secondary_labels(out, diag);

          for (size_t j = 0; j < diag.notes.size(); ++j)
            out << "  = note: " << diag.notes.access(j) << '\n';

          for (size_t j = 0; j < diag.help.size(); ++j)
            out << "  = help: " << diag.help.access(j) << '\n';

          if (i + 1 < diags.size())
            out << '\n';
        }
    }

    friend class Diagnostic_Builder;
  };

  inline Diagnostic &
  Diagnostic_Builder::diag()
  {
    ah_runtime_error_unless(engine != nullptr)
      << "Diagnostic_Builder: detached builder";
    return engine->diags.access(diag_index);
  }

  inline Diagnostic_Builder &
  Diagnostic_Builder::code(const std::string & value)
  {
    diag().code = value;
    return *this;
  }

  inline Diagnostic_Builder &
  Diagnostic_Builder::primary(const Source_Span & span, const std::string & msg)
  {
    diag().labels.append({span, Diagnostic_Label_Kind::Primary, msg});
    return *this;
  }

  inline Diagnostic_Builder &
  Diagnostic_Builder::secondary(const Source_Span & span, const std::string & msg)
  {
    diag().labels.append({span, Diagnostic_Label_Kind::Secondary, msg});
    return *this;
  }

  inline Diagnostic_Builder &
  Diagnostic_Builder::note(const std::string & msg)
  {
    diag().notes.append(msg);
    return *this;
  }

  inline Diagnostic_Builder &
  Diagnostic_Builder::help(const std::string & msg)
  {
    diag().help.append(msg);
    return *this;
  }
}

# endif // AH_DIAGNOSTICS_H
