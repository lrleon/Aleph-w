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

/** @file Compiler_Symbol_Bindings.H
 *  @brief Reusable lexical bindings and symbol records for frontends.
 *
 *  This header provides a small, language-agnostic layer for lexical
 *  declaration tracking on top of `tpl_scope.H`. It is intended for frontends
 *  that need stable symbol ids, shadowing-aware insertions, and deterministic
 *  dumps without adopting the MVP AST or semantic analyzer.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_SYMBOL_BINDINGS_H
#define COMPILER_SYMBOL_BINDINGS_H

#include <sstream>
#include <string>

#include <ah-source.H>
#include <tpl_scope.H>

namespace Aleph
{
  using Compiler_Symbol_Id = size_t;

  /** @brief Generic symbol kinds reusable across multiple frontends. */
  enum class Compiler_Symbol_Kind
  {
    Function,
    Parameter,
    Local,
    Global,
    Type
  };

  /** @brief Returns a stable debug name for a symbol kind. */
  inline const char *
  compiler_symbol_kind_name(const Compiler_Symbol_Kind kind) noexcept
  {
    switch (kind)
      {
      case Compiler_Symbol_Kind::Function:
        return "Function";
      case Compiler_Symbol_Kind::Parameter:
        return "Parameter";
      case Compiler_Symbol_Kind::Local:
        return "Local";
      case Compiler_Symbol_Kind::Global:
        return "Global";
      case Compiler_Symbol_Kind::Type:
        return "Type";
      }

    return "Unknown";
  }

  /** @brief One reusable symbol record tracked by lexical bindings. */
  struct Compiler_Symbol
  {
    Compiler_Symbol_Id id = 0;  ///< Stable symbol identifier, 1-based.
    Compiler_Symbol_Kind kind = Compiler_Symbol_Kind::Local;  ///< Symbol category.
    std::string name;  ///< Declared name.
    Source_Span declaration_span;  ///< Declaration site.
    size_t scope_depth = 0;  ///< Lexical depth where the symbol was introduced.
  };

  /** @brief Result status for one declaration attempt. */
  enum class Compiler_Symbol_Declare_Status
  {
    Inserted,
    Duplicate_Local,
    Inserted_Shadowing
  };

  /** @brief Structured result returned after one declaration attempt. */
  struct Compiler_Symbol_Declare_Result
  {
    Compiler_Symbol_Declare_Status status = Compiler_Symbol_Declare_Status::Inserted;
    Compiler_Symbol_Id symbol_id = 0;  ///< Newly inserted symbol or the conflicting local symbol.
    Compiler_Symbol_Id related_symbol_id = 0;  ///< Shadowed outer symbol or local duplicate.

    /** @brief Returns whether the declaration produced a new symbol. */
    [[nodiscard]] bool inserted() const noexcept
    {
      return status != Compiler_Symbol_Declare_Status::Duplicate_Local;
    }

    /** @brief Returns whether the declaration collided with the current scope. */
    [[nodiscard]] bool duplicate_local() const noexcept
    {
      return status == Compiler_Symbol_Declare_Status::Duplicate_Local;
    }

    /** @brief Returns whether the new declaration shadows one outer symbol. */
    [[nodiscard]] bool shadowing() const noexcept
    {
      return status == Compiler_Symbol_Declare_Status::Inserted_Shadowing;
    }
  };

  /** @brief Reusable lexical symbol table with stable ids and scope tracking. */
  class Compiler_Symbol_Bindings
  {
    DynArray<Compiler_Symbol> symbols_;
    Scope<std::string, Compiler_Symbol_Id> scopes_;

    Compiler_Symbol_Id
    append_symbol(const Compiler_Symbol_Kind kind,
                  const std::string & name,
                  const Source_Span & span)
    {
      Compiler_Symbol sym;
      sym.id = symbols_.size() + 1;
      sym.kind = kind;
      sym.name = name;
      sym.declaration_span = span;
      sym.scope_depth = scopes_.depth() > 0 ? scopes_.depth() - 1 : 0;
      symbols_.append(std::move(sym));
      return symbols_.size();
    }

  public:
    /** @brief Clears all scopes and previously recorded symbols. */
    void
    clear() noexcept
    {
      symbols_.clear();
      scopes_.clear();
    }

    /** @brief Returns the current lexical depth. */
    [[nodiscard]] size_t
    depth() const noexcept
    {
      return scopes_.depth();
    }

    /** @brief Returns the number of recorded symbols. */
    [[nodiscard]] size_t
    symbol_count() const noexcept
    {
      return symbols_.size();
    }

    /** @brief Enters one nested lexical scope. */
    size_t
    enter_scope()
    {
      return scopes_.enter_scope();
    }

    /** @brief Leaves the current lexical scope. */
    void
    leave_scope()
    {
      scopes_.leave_scope();
    }

    /** @brief Looks up one visible symbol id by name. */
    [[nodiscard]] const Compiler_Symbol_Id *
    lookup(const std::string & name) const noexcept
    {
      return scopes_.lookup(name);
    }

    /** @brief Looks up one symbol id only in the current scope. */
    [[nodiscard]] const Compiler_Symbol_Id *
    lookup_local(const std::string & name) const noexcept
    {
      return scopes_.lookup_local(name);
    }

    /** @brief Declares one symbol in the current scope.
     *
     *  Duplicate declarations in the current scope do not insert a new symbol.
     *  Shadowing of outer scopes does insert a new symbol and is reported in the
     *  returned status so callers can decide whether it is allowed.
     */
    [[nodiscard]] Compiler_Symbol_Declare_Result
    declare(const Compiler_Symbol_Kind kind,
            const std::string & name,
            const Source_Span & span)
    {
      ah_runtime_error_unless(scopes_.depth() > 0)
        << "Compiler_Symbol_Bindings::declare(): no active scope";

      if (const auto * local = scopes_.lookup_local(name); local != nullptr)
        return {Compiler_Symbol_Declare_Status::Duplicate_Local, *local, *local};

      const auto * shadowed = scopes_.lookup(name);
      const auto id = append_symbol(kind, name, span);
      const bool inserted = scopes_.insert(name, id);
      ah_runtime_error_unless(inserted)
        << "Compiler_Symbol_Bindings::declare(): unexpected duplicate insertion for '" << name
        << "'";

      return shadowed == nullptr
               ? Compiler_Symbol_Declare_Result{Compiler_Symbol_Declare_Status::Inserted, id, 0}
               : Compiler_Symbol_Declare_Result{Compiler_Symbol_Declare_Status::Inserted_Shadowing,
                                                id,
                                                *shadowed};
    }

    /** @brief Returns one symbol by its stable 1-based identifier. */
    [[nodiscard]] const Compiler_Symbol &
    symbol(const Compiler_Symbol_Id id) const
    {
      ah_out_of_range_error_unless(id > 0 and id <= symbols_.size())
        << "Compiler_Symbol_Bindings::symbol(): invalid id " << id;
      return symbols_.access(id - 1);
    }

    /** @brief Dumps the current symbol table in deterministic text form. */
    [[nodiscard]] std::string
    dump_symbols() const
    {
      std::ostringstream out;
      out << "Symbols\n";
      for (size_t i = 0; i < symbols_.size(); ++i)
        {
          const auto & sym = symbols_.access(i);
          out << "  #" << sym.id << " " << compiler_symbol_kind_name(sym.kind) << " " << sym.name
              << " scope=" << sym.scope_depth << " @" << sym.declaration_span.to_string() << '\n';
        }
      return out.str();
    }
  };
}

#endif
