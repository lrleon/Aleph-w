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

/** @file Compiler_HIR.H
 *  @brief Compatibility umbrella for the reusable HIR model and the MVP lowering.
 *
 *  New code that only needs the common HIR node model should prefer
 *  `Compiler_HIR_Model.H`. Code tied to the current reference frontend can
 *  include `Compiler_HIR_Lowering_MVP.H` directly. This umbrella keeps the
 *  previous public include stable for existing users.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_HIR_H
#define COMPILER_HIR_H

#include <Compiler_HIR_Builder.H>
#include <Compiler_HIR_Model.H>
#include <Compiler_HIR_Lowering_MVP.H>

#endif
