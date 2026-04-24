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

/** @file Compiler_Operator.H
 *  @brief Reusable operator vocabulary shared by HIR, IR, and backend layers.
 *
 *  The reference frontend still parses operators from `Compiler_Token.H`, but
 *  middle-end and backend layers should not need to model operators in terms
 *  of lexer token categories. This header provides a stable operator enum plus
 *  compatibility helpers to translate to and from the current token kinds.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_OPERATOR_H
#define COMPILER_OPERATOR_H

#include <Compiler_Token.H>

namespace Aleph {
/** @brief Stable operator kinds shared by reusable compiler layers. */
enum class Compiler_Operator_Kind
{
  Invalid,
  Assign,
  Plus,
  Minus,
  Star,
  Slash,
  Percent,
  EqEq,
  NotEq,
  Less,
  LessEq,
  Greater,
  GreaterEq,
  AndAnd,
  OrOr,
  Bang,
  Amp,
  Pipe,
  Caret,
  Tilde,
  PlusEq,
  MinusEq,
  StarEq,
  SlashEq,
  PercentEq,
  PlusPlus,
  MinusMinus
};

/** @brief Returns a stable debug name for one operator kind. */
inline const char *compiler_operator_name(const Compiler_Operator_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Operator_Kind::Invalid:
      return "Invalid";
    case Compiler_Operator_Kind::Assign:
      return "Assign";
    case Compiler_Operator_Kind::Plus:
      return "Plus";
    case Compiler_Operator_Kind::Minus:
      return "Minus";
    case Compiler_Operator_Kind::Star:
      return "Star";
    case Compiler_Operator_Kind::Slash:
      return "Slash";
    case Compiler_Operator_Kind::Percent:
      return "Percent";
    case Compiler_Operator_Kind::EqEq:
      return "EqEq";
    case Compiler_Operator_Kind::NotEq:
      return "NotEq";
    case Compiler_Operator_Kind::Less:
      return "Less";
    case Compiler_Operator_Kind::LessEq:
      return "LessEq";
    case Compiler_Operator_Kind::Greater:
      return "Greater";
    case Compiler_Operator_Kind::GreaterEq:
      return "GreaterEq";
    case Compiler_Operator_Kind::AndAnd:
      return "AndAnd";
    case Compiler_Operator_Kind::OrOr:
      return "OrOr";
    case Compiler_Operator_Kind::Bang:
      return "Bang";
    case Compiler_Operator_Kind::Amp:
      return "Amp";
    case Compiler_Operator_Kind::Pipe:
      return "Pipe";
    case Compiler_Operator_Kind::Caret:
      return "Caret";
    case Compiler_Operator_Kind::Tilde:
      return "Tilde";
    case Compiler_Operator_Kind::PlusEq:
      return "PlusEq";
    case Compiler_Operator_Kind::MinusEq:
      return "MinusEq";
    case Compiler_Operator_Kind::StarEq:
      return "StarEq";
    case Compiler_Operator_Kind::SlashEq:
      return "SlashEq";
    case Compiler_Operator_Kind::PercentEq:
      return "PercentEq";
    case Compiler_Operator_Kind::PlusPlus:
      return "PlusPlus";
    case Compiler_Operator_Kind::MinusMinus:
      return "MinusMinus";
    }

  return "Invalid";
}

/** @brief Maps one reference-frontend token kind into a reusable operator kind. */
inline Compiler_Operator_Kind compiler_operator_from_token_kind(const Compiler_Token_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Token_Kind::Assign:
      return Compiler_Operator_Kind::Assign;
    case Compiler_Token_Kind::Plus:
      return Compiler_Operator_Kind::Plus;
    case Compiler_Token_Kind::Minus:
      return Compiler_Operator_Kind::Minus;
    case Compiler_Token_Kind::Star:
      return Compiler_Operator_Kind::Star;
    case Compiler_Token_Kind::Slash:
      return Compiler_Operator_Kind::Slash;
    case Compiler_Token_Kind::Percent:
      return Compiler_Operator_Kind::Percent;
    case Compiler_Token_Kind::EqEq:
      return Compiler_Operator_Kind::EqEq;
    case Compiler_Token_Kind::NotEq:
      return Compiler_Operator_Kind::NotEq;
    case Compiler_Token_Kind::Less:
      return Compiler_Operator_Kind::Less;
    case Compiler_Token_Kind::LessEq:
      return Compiler_Operator_Kind::LessEq;
    case Compiler_Token_Kind::Greater:
      return Compiler_Operator_Kind::Greater;
    case Compiler_Token_Kind::GreaterEq:
      return Compiler_Operator_Kind::GreaterEq;
    case Compiler_Token_Kind::AndAnd:
      return Compiler_Operator_Kind::AndAnd;
    case Compiler_Token_Kind::OrOr:
      return Compiler_Operator_Kind::OrOr;
    case Compiler_Token_Kind::Bang:
      return Compiler_Operator_Kind::Bang;
    case Compiler_Token_Kind::Amp:
      return Compiler_Operator_Kind::Amp;
    case Compiler_Token_Kind::Pipe:
      return Compiler_Operator_Kind::Pipe;
    case Compiler_Token_Kind::Caret:
      return Compiler_Operator_Kind::Caret;
    case Compiler_Token_Kind::Tilde:
      return Compiler_Operator_Kind::Tilde;
    case Compiler_Token_Kind::PlusEq:
      return Compiler_Operator_Kind::PlusEq;
    case Compiler_Token_Kind::MinusEq:
      return Compiler_Operator_Kind::MinusEq;
    case Compiler_Token_Kind::StarEq:
      return Compiler_Operator_Kind::StarEq;
    case Compiler_Token_Kind::SlashEq:
      return Compiler_Operator_Kind::SlashEq;
    case Compiler_Token_Kind::PercentEq:
      return Compiler_Operator_Kind::PercentEq;
    case Compiler_Token_Kind::PlusPlus:
      return Compiler_Operator_Kind::PlusPlus;
    case Compiler_Token_Kind::MinusMinus:
      return Compiler_Operator_Kind::MinusMinus;
    default:
      break;
    }

  return Compiler_Operator_Kind::Invalid;
}

/** @brief Maps one reusable operator kind back into the MVP token kind. */
inline Compiler_Token_Kind compiler_operator_to_token_kind(const Compiler_Operator_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Operator_Kind::Assign:
      return Compiler_Token_Kind::Assign;
    case Compiler_Operator_Kind::Plus:
      return Compiler_Token_Kind::Plus;
    case Compiler_Operator_Kind::Minus:
      return Compiler_Token_Kind::Minus;
    case Compiler_Operator_Kind::Star:
      return Compiler_Token_Kind::Star;
    case Compiler_Operator_Kind::Slash:
      return Compiler_Token_Kind::Slash;
    case Compiler_Operator_Kind::Percent:
      return Compiler_Token_Kind::Percent;
    case Compiler_Operator_Kind::EqEq:
      return Compiler_Token_Kind::EqEq;
    case Compiler_Operator_Kind::NotEq:
      return Compiler_Token_Kind::NotEq;
    case Compiler_Operator_Kind::Less:
      return Compiler_Token_Kind::Less;
    case Compiler_Operator_Kind::LessEq:
      return Compiler_Token_Kind::LessEq;
    case Compiler_Operator_Kind::Greater:
      return Compiler_Token_Kind::Greater;
    case Compiler_Operator_Kind::GreaterEq:
      return Compiler_Token_Kind::GreaterEq;
    case Compiler_Operator_Kind::AndAnd:
      return Compiler_Token_Kind::AndAnd;
    case Compiler_Operator_Kind::OrOr:
      return Compiler_Token_Kind::OrOr;
    case Compiler_Operator_Kind::Bang:
      return Compiler_Token_Kind::Bang;
    case Compiler_Operator_Kind::Amp:
      return Compiler_Token_Kind::Amp;
    case Compiler_Operator_Kind::Pipe:
      return Compiler_Token_Kind::Pipe;
    case Compiler_Operator_Kind::Caret:
      return Compiler_Token_Kind::Caret;
    case Compiler_Operator_Kind::Tilde:
      return Compiler_Token_Kind::Tilde;
    case Compiler_Operator_Kind::PlusEq:
      return Compiler_Token_Kind::PlusEq;
    case Compiler_Operator_Kind::MinusEq:
      return Compiler_Token_Kind::MinusEq;
    case Compiler_Operator_Kind::StarEq:
      return Compiler_Token_Kind::StarEq;
    case Compiler_Operator_Kind::SlashEq:
      return Compiler_Token_Kind::SlashEq;
    case Compiler_Operator_Kind::PercentEq:
      return Compiler_Token_Kind::PercentEq;
    case Compiler_Operator_Kind::PlusPlus:
      return Compiler_Token_Kind::PlusPlus;
    case Compiler_Operator_Kind::MinusMinus:
      return Compiler_Token_Kind::MinusMinus;
    default:
      break;
    }

  return Compiler_Token_Kind::Invalid;
}

/** @brief Returns whether `kind` is one compound-assignment operator. */
inline bool compiler_operator_is_compound_assignment(const Compiler_Operator_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Operator_Kind::PlusEq:
    case Compiler_Operator_Kind::MinusEq:
    case Compiler_Operator_Kind::StarEq:
    case Compiler_Operator_Kind::SlashEq:
    case Compiler_Operator_Kind::PercentEq:
      return true;
    default:
      break;
    }

  return false;
}

/** @brief Returns the arithmetic operator underlying one compound assignment. */
inline Compiler_Operator_Kind compiler_operator_compound_base(const Compiler_Operator_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Operator_Kind::PlusEq:
      return Compiler_Operator_Kind::Plus;
    case Compiler_Operator_Kind::MinusEq:
      return Compiler_Operator_Kind::Minus;
    case Compiler_Operator_Kind::StarEq:
      return Compiler_Operator_Kind::Star;
    case Compiler_Operator_Kind::SlashEq:
      return Compiler_Operator_Kind::Slash;
    case Compiler_Operator_Kind::PercentEq:
      return Compiler_Operator_Kind::Percent;
    default:
      break;
    }

  return Compiler_Operator_Kind::Invalid;
}
}  // namespace Aleph

#endif
