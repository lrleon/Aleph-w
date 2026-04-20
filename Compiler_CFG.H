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

/** @file Compiler_CFG.H
 *  @brief Basic-block CFG representation and lowering from structured HIR.
 *
 *  This header introduces the first reusable control-flow graph (CFG) layer for 
 *  the compiler-support platform. It sits after `Compiler_HIR.H`: HIR preserves
 *  structured control flow, while CFG makes branches, loops, exits, and joins
 *  explicit as basic blocks and terminators.
 *
 *  The current CFG layer includes:
 *
 *  - Basic blocks with stable numeric identifiers.
 *  - Explicit terminators (`Jump`, `Branch`, `Return`, `Exit`, `Unreachable`).
 *  - Lowering from HIR functions and top-level HIR statements.
 *  - Successor and predecessor tracking for graph traversal.
 *  - Structural validation with deterministic diagnostics.
 *  - Plain-text dumps suitable for tests, docs, and debugging.
 *
 *  The MVP remains deliberately high-level. It does not yet introduce SSA,
 *  explicit temporaries, or a low-level IR. Its purpose is to provide a stable
 *  bridge between structured interpretation and later MIR/SSA passes.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_CFG_H
#define COMPILER_CFG_H

#include <limits>
#include <sstream>
#include <string>
#include <utility>

#include <Compiler_HIR_Model.H>
#include <ah-arena.H>
#include <ah-errors.H>
#include <tpl_dynArray.H>

namespace Aleph {

/** @brief Typedef for unique block identifiers within a function. */
using Compiler_CFG_Block_Id = size_t;

/** @brief Returns the sentinel value for an invalid block ID. 
 *
 *  @return Maximum possible value for `Compiler_CFG_Block_Id`.
 */
inline constexpr Compiler_CFG_Block_Id compiler_cfg_invalid_block_id() noexcept
{
  return std::numeric_limits<Compiler_CFG_Block_Id>::max();
}

/** @brief Categories of basic block terminators. 
 *
 *  Every basic block must end with exactly one terminator that defines 
 *  how control flow proceeds to successors.
 */
enum class Compiler_CFG_Terminator_Kind
{
  None,        ///< Unset or invalid terminator.
  Jump,        ///< Unconditional jump to exactly one successor.
  Branch,      ///< Conditional branch to two successors (true and false targets).
  Return,      ///< Function exit with an optional return value.
  Exit,        ///< Sentinel terminator for the canonical exit block.
  Unreachable  ///< Marks code paths that should never be executed (e.g., after a break).
};

/** @brief Returns a stable string name for a CFG terminator kind. 
 *
 *  @param kind Terminator kind to convert.
 *  @return String literal name.
 */
inline const char *compiler_cfg_terminator_kind_name(const Compiler_CFG_Terminator_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_CFG_Terminator_Kind::None:
      return "None";
    case Compiler_CFG_Terminator_Kind::Jump:
      return "Jump";
    case Compiler_CFG_Terminator_Kind::Branch:
      return "Branch";
    case Compiler_CFG_Terminator_Kind::Return:
      return "Return";
    case Compiler_CFG_Terminator_Kind::Exit:
      return "Exit";
    case Compiler_CFG_Terminator_Kind::Unreachable:
      return "Unreachable";
    }

  return "Unknown";
}

/** @brief Detailed information about a block's termination. 
 *
 *  Encapsulates the logic of how a block ends.
 */
struct Compiler_CFG_Terminator
{
  Compiler_CFG_Terminator_Kind kind = Compiler_CFG_Terminator_Kind::None;  ///< Category of terminator.
  const Compiler_HIR_Expr *condition = nullptr;  ///< Branch condition (valid if kind is Branch).
  const Compiler_HIR_Expr *value = nullptr;      ///< Return value (valid if kind is Return).
  Source_Span span;                              ///< Source location of the terminator.
  DynArray<Compiler_CFG_Block_Id> successors;    ///< List of successor block IDs.
};

/** @brief Represents a single basic block in a CFG. 
 *
 *  A basic block is a maximal sequence of linear statements that are 
 *  always executed together. It starts at an entry point and ends with 
 *  a terminator.
 */
struct Compiler_CFG_Block
{
  Compiler_CFG_Block_Id id = compiler_cfg_invalid_block_id();  ///< Unique identifier in the function.
  std::string label;  ///< Human-readable debug label (e.g., "entry", "loop.body").
  Source_Span span;   ///< Span covering all statements and the terminator.
  DynArray<const Compiler_HIR_Stmt *> statements;  ///< Ordered list of linear statements.
  Compiler_CFG_Terminator terminator;              ///< How the block ends.
  DynArray<Compiler_CFG_Block_Id> predecessors;    ///< List of block IDs that jump to this block.

  /** @brief Checks if the block has a valid terminator set. */
  bool is_terminated() const noexcept
  {
    return terminator.kind != Compiler_CFG_Terminator_Kind::None;
  }
};

/** @brief CFG representation of a single function or top-level body. 
 *
 *  Organizes basic blocks into a directed graph.
 */
struct Compiler_CFG_Function
{
  std::string name;             ///< Function or script name.
  Source_Span span;             ///< Overall source span.
  Compiler_Type_Id type_id = 0; ///< Return type or function type ID.
  const Compiler_HIR_Function *source_function = nullptr;  ///< Pointer to original HIR (null for top-level).
  DynArray<Compiler_CFG_Block> blocks;  ///< All basic blocks belonging to this function.
  Compiler_CFG_Block_Id entry_block = compiler_cfg_invalid_block_id();  ///< Starting block ID.
  Compiler_CFG_Block_Id exit_block = compiler_cfg_invalid_block_id();   ///< Canonical exit block ID.

  /** @brief Checks if this CFG represents the global top-level script. */
  bool is_top_level() const noexcept
  {
    return source_function == nullptr;
  }
};

/** @brief Aggregates all lowered CFGs for a compilation unit. */
struct Compiler_CFG_Module
{
  DynArray<Compiler_CFG_Function *> functions;  ///< CFGs for all defined functions.
  Compiler_CFG_Function *top_level = nullptr;   ///< Optional CFG for global statements.
};

/** @brief Arena-backed ownership context for CFG objects. 
 *
 *  Ensures efficient allocation and bulk destruction of CFG-related data.
 */
class Compiler_CFG_Context
{
  /** @brief Internal tracker for objects with non-trivial destructors. */
  struct Owned_Object
  {
    void *ptr = nullptr;
    void (*destroy)(void *) noexcept = nullptr;
  };

  AhArenaAllocator arena;
  DynArray<Owned_Object> owned;

public:
  /** @brief Constructs a CFG context. */
  explicit Compiler_CFG_Context(const size_t arena_size = AhArenaAllocator::DEFAULT_SIZE)
    : arena(arena_size)
  {}

  Compiler_CFG_Context(const Compiler_CFG_Context &) = delete;
  Compiler_CFG_Context &operator=(const Compiler_CFG_Context &) = delete;

  /** @brief Cleans up the context and all managed objects. */
  ~Compiler_CFG_Context() noexcept
  {
    reset();
  }

  /** @brief Allocates and constructs an object in the arena. */
  template <typename T, typename... Args>
  T *make(Args &&...args)
  {
    T *ptr = allocate<T>(arena, std::forward<Args>(args)...);
    ah_runtime_error_unless(ptr != nullptr) << "Compiler_CFG_Context: arena allocation failed";

    owned.append({ptr,
                  [](void *raw) noexcept
    {
      static_cast<T *>(raw)->~T();
    }});
    return ptr;
  }

  /** @brief Resets the arena and destroys all tracked objects. */
  void reset() noexcept
  {
    for (size_t i = owned.size(); i > 0; --i)
      {
        auto &[ptr, destroy] = owned.access(i - 1);
        if (destroy != nullptr and ptr != nullptr)
          destroy(ptr);
      }
    owned.clear();
    arena.reset();
  }

  /** @brief Returns currently used bytes. */
  size_t allocated_size() const noexcept
  {
    return arena.allocated_size();
  }
};

/** @brief Detailed results of a CFG structural validation. */
struct Compiler_CFG_Validation_Report
{
  bool valid = true;               ///< True if no errors were found.
  DynArray<std::string> errors;    ///< List of critical violations.
  DynArray<std::string> warnings;  ///< List of non-fatal issues (e.g., unreachable blocks).
};

namespace Compiler_CFG_Detail {

/** @brief Generates a standard block label (e.g., "B0"). */
inline std::string block_name(const Compiler_CFG_Block_Id id)
{
  return "B" + std::to_string(id);
}

/** @brief Checks if a block ID is present in a list of IDs. */
inline bool contains_block_id(const DynArray<Compiler_CFG_Block_Id> &ids,
                              const Compiler_CFG_Block_Id id) noexcept
{
  for (size_t i = 0; i < ids.size(); ++i)
    if (ids.access(i) == id)
      return true;
  return false;
}

/** @brief Appends text to a stream with a consistent indentation. */
inline void append_indented_text(std::ostream &out, const std::string &text, const size_t indent)
{
  const std::string padding(indent, ' ');
  size_t begin = 0;
  while (begin < text.size())
    {
      const auto end = text.find('\n', begin);
      if (end == std::string::npos)
        {
          out << padding << text.substr(begin) << '\n';
          return;
        }

      if (end > begin)
        out << padding << text.substr(begin, end - begin) << '\n';
      else
        out << padding << '\n';
      begin = end + 1;
    }
}

/** @brief Dumps a HIR statement to the stream for debugging. */
inline void append_hir_stmt_dump(std::ostream &out,
                                 const Compiler_HIR_Stmt *stmt,
                                 const Compiler_Type_Context *types,
                                 const size_t indent)
{
  if (types != nullptr)
    append_indented_text(out, compiler_dump_hir_stmt(stmt, *types), indent);
  else
    {
      const Compiler_Type_Context fallback;
      append_indented_text(out, compiler_dump_hir_stmt(stmt, fallback), indent);
    }
}

/** @brief Dumps a HIR expression to the stream for debugging. */
inline void append_hir_expr_dump(std::ostream &out,
                                 const Compiler_HIR_Expr *expr,
                                 const Compiler_Type_Context *types,
                                 const size_t indent)
{
  if (types != nullptr)
    append_indented_text(out, compiler_dump_hir_expr(expr, *types), indent);
  else
    {
      const Compiler_Type_Context fallback;
      append_indented_text(out, compiler_dump_hir_expr(expr, fallback), indent);
    }
}

/** @brief Checks if an HIR statement is linear (non-branching). */
inline bool is_linear_stmt(const Compiler_HIR_Stmt *stmt) noexcept
{
  if (stmt == nullptr)
    return false;

  return stmt->kind == Compiler_HIR_Stmt_Kind::Eval or stmt->kind == Compiler_HIR_Stmt_Kind::Let
         or stmt->kind == Compiler_HIR_Stmt_Kind::Invalid;
}
}  // namespace Compiler_CFG_Detail

/** @brief Transformation engine that lowers typed HIR into reusable CFGs. 
 *
 *  Decomposes structured control flow (if, while, blocks) into a graph of 
 *  basic blocks connected by explicit terminators.
 */
class Compiler_CFG_Lowering
{
  /** @brief Internal state for loop lowering. */
  struct Loop_Targets
  {
    Compiler_CFG_Block_Id break_block = compiler_cfg_invalid_block_id();
    Compiler_CFG_Block_Id continue_block = compiler_cfg_invalid_block_id();
  };

  Compiler_CFG_Context *cfg = nullptr;    ///< CFG owner.
  const Compiler_Type_Context *types = nullptr; ///< Type metadata source.
  size_t if_counter = 0;                  ///< ID generator for conditional blocks.
  size_t while_counter = 0;               ///< ID generator for loop blocks.
  size_t dead_counter = 0;                ///< ID generator for unreachable blocks.

  /** @brief Accesses a block by ID within a function. */
  Compiler_CFG_Block &block(Compiler_CFG_Function *function, const Compiler_CFG_Block_Id id) const
  {
    return function->blocks.access(id);
  }

  /** @brief Utility to merge a source span into a destination span. */
  static void merge_span(Source_Span &dst, const Source_Span &src)
  {
    if (not src.is_valid())
      return;
    if (not dst.is_valid())
      {
        dst = src;
        return;
      }
      dst = compiler_hir_merge_spans(dst, src);
  }

  /** @brief Creates a new basic block in the given function. */
  Compiler_CFG_Block_Id create_block(Compiler_CFG_Function *function, std::string label) const
  {
    Compiler_CFG_Block block;
    block.id = function->blocks.size();
    block.label = std::move(label);
    function->blocks.append(std::move(block));
    return function->blocks.size() - 1;
  }

  /** @brief Adds a directed edge from one block to another. */
  void add_edge(Compiler_CFG_Function *function,
                const Compiler_CFG_Block_Id from,
                const Compiler_CFG_Block_Id to) const
  {
    auto &src = block(function, from);
    auto &dst = block(function, to);

    if (not Compiler_CFG_Detail::contains_block_id(src.terminator.successors, to))
      src.terminator.successors.append(to);
    if (not Compiler_CFG_Detail::contains_block_id(dst.predecessors, from))
      dst.predecessors.append(from);
  }

  /** @brief Appends a linear statement to a block and updates its span. */
  void append_linear_stmt(Compiler_CFG_Function *function,
                          const Compiler_CFG_Block_Id id,
                          const Compiler_HIR_Stmt *stmt) const
  {
    auto &blk = block(function, id);
    blk.statements.append(stmt);
    merge_span(blk.span, stmt->span);
  }

  /** @brief Sets a Jump terminator on a block. */
  void set_jump(Compiler_CFG_Function *function,
                const Compiler_CFG_Block_Id from,
                const Compiler_CFG_Block_Id to,
                const Source_Span &span = {}) const
  {
    auto &blk = block(function, from);
    ah_runtime_error_unless(not blk.is_terminated()) << "Compiler_CFG_Lowering: block already terminated";
    blk.terminator.kind = Compiler_CFG_Terminator_Kind::Jump;
    blk.terminator.span = span;
    merge_span(blk.span, span);
    add_edge(function, from, to);
  }

  /** @brief Sets a Branch terminator on a block. */
  void set_branch(Compiler_CFG_Function *function,
                  const Compiler_CFG_Block_Id from,
                  const Compiler_HIR_Expr *condition,
                  const Compiler_CFG_Block_Id then_block,
                  const Compiler_CFG_Block_Id else_block) const
  {
    auto &blk = block(function, from);
    ah_runtime_error_unless(not blk.is_terminated()) << "Compiler_CFG_Lowering: block already terminated";
    blk.terminator.kind = Compiler_CFG_Terminator_Kind::Branch;
    blk.terminator.condition = condition;
    blk.terminator.span = condition != nullptr ? condition->span : Source_Span();
    merge_span(blk.span, blk.terminator.span);
    add_edge(function, from, then_block);
    add_edge(function, from, else_block);
  }

  /** @brief Sets a Return terminator on a block. */
  void set_return(Compiler_CFG_Function *function,
                  const Compiler_CFG_Block_Id from,
                  const Compiler_HIR_Expr *value,
                  const Source_Span &span = {}) const
  {
    auto &blk = block(function, from);
    ah_runtime_error_unless(not blk.is_terminated()) << "Compiler_CFG_Lowering: block already terminated";
    blk.terminator.kind = Compiler_CFG_Terminator_Kind::Return;
    blk.terminator.value = value;
    blk.terminator.span = span.is_valid() ? span : (value != nullptr ? value->span : Source_Span());
    merge_span(blk.span, blk.terminator.span);
    add_edge(function, from, function->exit_block);
  }

  /** @brief Sets an Exit terminator (only for the canonical exit block). */
  void set_exit(Compiler_CFG_Function *function, const Compiler_CFG_Block_Id id) const
  {
    auto &blk = block(function, id);
    blk.terminator.kind = Compiler_CFG_Terminator_Kind::Exit;
  }

  /** @brief Sets an Unreachable terminator on a block. */
  void set_unreachable(Compiler_CFG_Function *function,
                       const Compiler_CFG_Block_Id id,
                       const Source_Span &span = {}) const
  {
    auto &blk = block(function, id);
    ah_runtime_error_unless(not blk.is_terminated()) << "Compiler_CFG_Lowering: block already terminated";
    blk.terminator.kind = Compiler_CFG_Terminator_Kind::Unreachable;
    blk.terminator.span = span;
    merge_span(blk.span, span);
  }

  /** @brief Ensures a valid current block exists, creating a "dead" one if needed. */
  Compiler_CFG_Block_Id ensure_current_block(Compiler_CFG_Function *function,
                                             Compiler_CFG_Block_Id current)
  {
    if (current != compiler_cfg_invalid_block_id())
      return current;
    return create_block(function, "dead." + std::to_string(dead_counter++));
  }

  /** @brief Recursively lowers an HIR statement into the CFG. */
  Compiler_CFG_Block_Id lower_stmt(Compiler_CFG_Function *function,
                                   const Compiler_HIR_Stmt *stmt,
                                   Compiler_CFG_Block_Id current,
                                   DynArray<Loop_Targets> &loops)
  {
    if (stmt == nullptr)
      return current;

    if (Compiler_CFG_Detail::is_linear_stmt(stmt))
      {
        current = ensure_current_block(function, current);
        append_linear_stmt(function, current, stmt);
        return current;
      }

    switch (stmt->kind)
      {
      case Compiler_HIR_Stmt_Kind::Block:
        {
          const auto *node = static_cast<const Compiler_HIR_Block_Stmt *>(stmt);
          for (size_t i = 0; i < node->statements.size(); ++i)
            current = lower_stmt(function, node->statements.access(i), current, loops);
          return current;
        }

      case Compiler_HIR_Stmt_Kind::Return:
        {
          const auto *node = static_cast<const Compiler_HIR_Return_Stmt *>(stmt);
          current = ensure_current_block(function, current);
          set_return(function, current, node->value, node->span);
          return compiler_cfg_invalid_block_id();
        }

      case Compiler_HIR_Stmt_Kind::If:
        {
          const auto *node = static_cast<const Compiler_HIR_If_Stmt *>(stmt);
          current = ensure_current_block(function, current);

          const auto id = if_counter++;
          const auto then_block = create_block(function, "if.then." + std::to_string(id));
          const auto join_block = create_block(function, "if.end." + std::to_string(id));

          Compiler_CFG_Block_Id else_block = join_block;
          if (node->else_branch != nullptr)
            else_block = create_block(function, "if.else." + std::to_string(id));

          set_branch(function, current, node->condition, then_block, else_block);

          auto then_end = lower_stmt(function, node->then_branch, then_block, loops);
          if (then_end != compiler_cfg_invalid_block_id())
            set_jump(function, then_end, join_block);

          if (node->else_branch != nullptr)
            {
              auto else_end = lower_stmt(function, node->else_branch, else_block, loops);
              if (else_end != compiler_cfg_invalid_block_id())
                set_jump(function, else_end, join_block);
            }

          return join_block;
        }

      case Compiler_HIR_Stmt_Kind::While:
        {
          const auto *node = static_cast<const Compiler_HIR_While_Stmt *>(stmt);
          current = ensure_current_block(function, current);

          const auto id = while_counter++;
          const auto cond_block = create_block(function, "while.cond." + std::to_string(id));
          const auto body_block = create_block(function, "while.body." + std::to_string(id));
          const auto end_block = create_block(function, "while.end." + std::to_string(id));

          set_jump(function, current, cond_block, node->keyword_span);
          set_branch(function, cond_block, node->condition, body_block, end_block);

          loops.append({end_block, cond_block});
          auto body_end = lower_stmt(function, node->body, body_block, loops);
          (void) loops.pop();
          if (body_end != compiler_cfg_invalid_block_id())
            set_jump(function, body_end, cond_block);

          return end_block;
        }

      case Compiler_HIR_Stmt_Kind::Break:
        {
          current = ensure_current_block(function, current);
          if (loops.is_empty())
            {
              set_unreachable(function, current, stmt->span);
              return compiler_cfg_invalid_block_id();
            }

          set_jump(function, current, loops.access(loops.size() - 1).break_block, stmt->span);
          return compiler_cfg_invalid_block_id();
        }

      case Compiler_HIR_Stmt_Kind::Continue:
        {
          current = ensure_current_block(function, current);
          if (loops.is_empty())
            {
              set_unreachable(function, current, stmt->span);
              return compiler_cfg_invalid_block_id();
            }

          set_jump(function, current, loops.access(loops.size() - 1).continue_block, stmt->span);
          return compiler_cfg_invalid_block_id();
        }

      case Compiler_HIR_Stmt_Kind::Invalid:
      case Compiler_HIR_Stmt_Kind::Eval:
      case Compiler_HIR_Stmt_Kind::Let:
        return current;
      }

    return current;
  }

  /** @brief Lowers a sequence of statements into a new CFG function. */
  Compiler_CFG_Function *lower_statement_sequence(const std::string &name,
                                                 const Source_Span &span,
                                                 const Compiler_Type_Id type_id,
                                                 const Compiler_HIR_Function *source_function,
                                                 const DynArray<Compiler_HIR_Stmt *> &statements)
  {
    auto *function = cfg->make<Compiler_CFG_Function>();
    function->name = name;
    function->span = span;
    function->type_id = type_id;
    function->source_function = source_function;
    function->entry_block = create_block(function, "entry");
    function->exit_block = create_block(function, "exit");

    DynArray<Loop_Targets> loops;
    Compiler_CFG_Block_Id current = function->entry_block;
    for (size_t i = 0; i < statements.size(); ++i)
      current = lower_stmt(function, statements.access(i), current, loops);

    if (current != compiler_cfg_invalid_block_id() and not block(function, current).is_terminated())
      set_jump(function, current, function->exit_block);

    set_exit(function, function->exit_block);
    return function;
  }

public:
  /** @brief Builds a CFG lowerer over the specified context.
   *
   *  @param ctx Ownership context for the produced CFG nodes.
   *  @param type_ctx Optional type context used by textual dumps later.
   */
  explicit Compiler_CFG_Lowering(Compiler_CFG_Context &ctx,
                                 const Compiler_Type_Context *type_ctx = nullptr) noexcept
    : cfg(&ctx), types(type_ctx)
  {}

  /** @brief Returns the optional type context used by this lowerer. */
  const Compiler_Type_Context *type_context() const noexcept
  {
    return types;
  }

  /** @brief Lowers one HIR function into a CFG. 
   *
   *  @param function Typed HIR function to lower.
   *  @return Lowered CFG representation.
   */
  Compiler_CFG_Function *lower_function(const Compiler_HIR_Function *function)
  {
    ah_runtime_error_unless(function != nullptr) << "Compiler_CFG_Lowering::lower_function(): null HIR function";
    return lower_statement_sequence(function->name,
                                    function->span,
                                    function->type_id,
                                    function,
                                    function->body->statements);
  }

  /** @brief Lowers one HIR module into function CFGs and optional top-level CFG. 
   *
   *  @param module Typed HIR module to lower.
   *  @return Complete CFG module.
   */
  Compiler_CFG_Module *lower_module(const Compiler_HIR_Module *module)
  {
    ah_runtime_error_unless(module != nullptr) << "Compiler_CFG_Lowering::lower_module(): null HIR module";

    auto *result = cfg->make<Compiler_CFG_Module>();
    for (size_t i = 0; i < module->functions.size(); ++i)
      result->functions.append(lower_function(module->functions.access(i)));

    if (not module->statements.is_empty())
      result->top_level = lower_statement_sequence("<top-level>",
                                                   module->span,
                                                   0,
                                                   nullptr,
                                                   module->statements);
    return result;
  }
};

/** @brief Validates one lowered CFG function structurally.
 *
 *  The validator checks entry/exit IDs, terminator shapes, edge consistency, 
 *  and reports unreachable blocks as warnings.
 *
 *  @param function CFG function to validate.
 *  @return Validation report with errors and warnings.
 */
inline Compiler_CFG_Validation_Report validate_cfg_function(const Compiler_CFG_Function &function)
{
  Compiler_CFG_Validation_Report report;

  auto fail = [&report](const std::string &msg)
  {
    report.valid = false;
    report.errors.append(msg);
  };

  if (function.blocks.is_empty())
    {
      fail("CFG function '" + function.name + "' has no basic blocks");
      return report;
    }

  if (function.entry_block >= function.blocks.size())
    fail("CFG function '" + function.name + "' has invalid entry block");
  if (function.exit_block >= function.blocks.size())
    fail("CFG function '" + function.name + "' has invalid exit block");
  if (not report.valid)
    return report;

  for (size_t i = 0; i < function.blocks.size(); ++i)
    {
      const auto &block = function.blocks.access(i);
      if (block.id != i)
        fail("CFG function '" + function.name + "' has mismatched block id at "
             + Compiler_CFG_Detail::block_name(i));

      if (block.terminator.kind == Compiler_CFG_Terminator_Kind::None)
        fail("CFG block " + Compiler_CFG_Detail::block_name(block.id) + " in function '" + function.name
             + "' is unterminated");

      switch (block.terminator.kind)
        {
        case Compiler_CFG_Terminator_Kind::Jump:
          if (block.terminator.successors.size() != 1)
            fail("Jump terminator in " + Compiler_CFG_Detail::block_name(block.id)
                 + " must have exactly one successor");
          break;

        case Compiler_CFG_Terminator_Kind::Branch:
          if (block.terminator.condition == nullptr)
            fail("Branch terminator in " + Compiler_CFG_Detail::block_name(block.id)
                 + " is missing its condition");
          if (block.terminator.successors.size() != 2)
            fail("Branch terminator in " + Compiler_CFG_Detail::block_name(block.id)
                 + " must have exactly two successors");
          break;

        case Compiler_CFG_Terminator_Kind::Return:
          if (block.terminator.successors.size() != 1)
            fail("Return terminator in " + Compiler_CFG_Detail::block_name(block.id)
                 + " must point to the canonical exit block");
          if (block.terminator.successors.size() >= 1
              and block.terminator.successors.access(0) != function.exit_block)
            fail("Return terminator in " + Compiler_CFG_Detail::block_name(block.id)
                 + " must point to the canonical exit block");
          break;

        case Compiler_CFG_Terminator_Kind::Exit:
          if (block.id != function.exit_block)
            fail("Only the canonical exit block may use the Exit terminator in function '" + function.name
                 + "'");
          if (not block.terminator.successors.is_empty())
            fail("Exit block " + Compiler_CFG_Detail::block_name(block.id) + " must not have successors");
          break;

        case Compiler_CFG_Terminator_Kind::Unreachable:
        case Compiler_CFG_Terminator_Kind::None:
          break;
        }

      for (size_t j = 0; j < block.terminator.successors.size(); ++j)
        {
          const auto succ = block.terminator.successors.access(j);
          if (succ >= function.blocks.size())
            {
              fail("CFG block " + Compiler_CFG_Detail::block_name(block.id)
                   + " references invalid successor " + Compiler_CFG_Detail::block_name(succ));
              continue;
            }

          const auto &succ_block = function.blocks.access(succ);
          if (not Compiler_CFG_Detail::contains_block_id(succ_block.predecessors, block.id))
            fail("CFG edge " + Compiler_CFG_Detail::block_name(block.id) + " -> "
                 + Compiler_CFG_Detail::block_name(succ) + " is missing from predecessor lists");
        }

      for (size_t j = 0; j < block.predecessors.size(); ++j)
        {
          const auto pred = block.predecessors.access(j);
          if (pred >= function.blocks.size())
            {
              fail("CFG block " + Compiler_CFG_Detail::block_name(block.id)
                   + " references invalid predecessor " + Compiler_CFG_Detail::block_name(pred));
              continue;
            }

          const auto &pred_block = function.blocks.access(pred);
          if (not Compiler_CFG_Detail::contains_block_id(pred_block.terminator.successors, block.id))
            fail("CFG predecessor edge " + Compiler_CFG_Detail::block_name(pred) + " -> "
                 + Compiler_CFG_Detail::block_name(block.id) + " is missing from successor lists");
        }
    }

  // Reachability analysis
  DynArray<bool> reachable;
  for (size_t i = 0; i < function.blocks.size(); ++i)
    reachable.append(false);

  DynArray<Compiler_CFG_Block_Id> worklist;
  worklist.append(function.entry_block);
  while (not worklist.is_empty())
    {
      const auto current = worklist.pop();
      if (reachable.access(current))
        continue;
      reachable.access(current) = true;

      const auto &block = function.blocks.access(current);
      for (size_t i = 0; i < block.terminator.successors.size(); ++i)
        {
          const auto succ = block.terminator.successors.access(i);
          if (succ < function.blocks.size() and not reachable.access(succ))
            worklist.append(succ);
        }
    }

  for (size_t i = 0; i < function.blocks.size(); ++i)
    if (not reachable.access(i))
      report.warnings.append("CFG block " + Compiler_CFG_Detail::block_name(i) + " in function '"
                             + function.name + "' is unreachable from entry");

  return report;
}

/** @brief Validates all CFGs in a lowered module. 
 *
 *  @param module CFG module to validate.
 *  @return Aggregated validation report.
 */
inline Compiler_CFG_Validation_Report validate_cfg_module(const Compiler_CFG_Module &module)
{
  Compiler_CFG_Validation_Report report;

  auto merge = [&report](const Compiler_CFG_Validation_Report &child)
  {
    if (not child.valid)
      report.valid = false;
    for (size_t i = 0; i < child.errors.size(); ++i)
      report.errors.append(child.errors.access(i));
    for (size_t i = 0; i < child.warnings.size(); ++i)
      report.warnings.append(child.warnings.access(i));
  };

  for (size_t i = 0; i < module.functions.size(); ++i)
    merge(validate_cfg_function(*module.functions.access(i)));
  if (module.top_level != nullptr)
    merge(validate_cfg_function(*module.top_level));
  return report;
}

/** @brief Dumps a single CFG function in text format for debugging. 
 *
 *  @param function CFG to dump.
 *  @param types Optional type context to resolve type IDs.
 *  @return Multiline string representation.
 */
inline std::string compiler_dump_cfg_function(const Compiler_CFG_Function *function,
                                              const Compiler_Type_Context *types = nullptr)
{
  std::ostringstream out;
  if (function == nullptr)
    {
      out << "<null-cfg-function>\n";
      return out.str();
    }

  out << "CFGFunction(" << function->name << ")";
  if (types != nullptr and function->type_id != 0)
    out << ": " << types->to_string(function->type_id);
  out << '\n';
  out << "  Entry: " << Compiler_CFG_Detail::block_name(function->entry_block) << '\n';
  out << "  Exit: " << Compiler_CFG_Detail::block_name(function->exit_block) << '\n';

  for (size_t i = 0; i < function->blocks.size(); ++i)
    {
      const auto &block = function->blocks.access(i);
      out << "  Block " << Compiler_CFG_Detail::block_name(block.id) << " [" << block.label << "]\n";

      if (block.statements.is_empty())
        out << "    Statements: <none>\n";
      else
        {
          out << "    Statements:\n";
          for (size_t j = 0; j < block.statements.size(); ++j)
            Compiler_CFG_Detail::append_hir_stmt_dump(out, block.statements.access(j), types, 6);
        }

      out << "    Terminator: " << compiler_cfg_terminator_kind_name(block.terminator.kind) << '\n';

      if (block.terminator.kind == Compiler_CFG_Terminator_Kind::Branch
          and block.terminator.condition != nullptr)
        {
          out << "      Condition:\n";
          Compiler_CFG_Detail::append_hir_expr_dump(out, block.terminator.condition, types, 8);
        }
      else if (block.terminator.kind == Compiler_CFG_Terminator_Kind::Return
               and block.terminator.value != nullptr)
        {
          out << "      Value:\n";
          Compiler_CFG_Detail::append_hir_expr_dump(out, block.terminator.value, types, 8);
        }

      out << "    Successors:";
      if (block.terminator.successors.is_empty())
        out << " <none>\n";
      else
        {
          for (size_t j = 0; j < block.terminator.successors.size(); ++j)
            {
              out << (j == 0 ? " " : ", ");
              out << Compiler_CFG_Detail::block_name(block.terminator.successors.access(j));
            }
          out << '\n';
        }

      out << "    Predecessors:";
      if (block.predecessors.is_empty())
        out << " <none>\n";
      else
        {
          for (size_t j = 0; j < block.predecessors.size(); ++j)
            {
              out << (j == 0 ? " " : ", ");
              out << Compiler_CFG_Detail::block_name(block.predecessors.access(j));
            }
          out << '\n';
        }
    }

  return out.str();
}

/** @brief Dumps a complete CFG module in text format for debugging. 
 *
 *  @param module Module to dump.
 *  @param types Optional type context.
 *  @return Multiline string representation.
 */
inline std::string compiler_dump_cfg_module(const Compiler_CFG_Module *module,
                                            const Compiler_Type_Context *types = nullptr)
{
  std::ostringstream out;
  if (module == nullptr)
    {
      out << "<null-cfg-module>\n";
      return out.str();
    }

  out << "CFGModule\n";
  for (size_t i = 0; i < module->functions.size(); ++i)
    Compiler_CFG_Detail::append_indented_text(
      out, compiler_dump_cfg_function(module->functions.access(i), types), 2);
  if (module->top_level != nullptr)
    Compiler_CFG_Detail::append_indented_text(out, compiler_dump_cfg_function(module->top_level, types), 2);
  return out.str();
}

}  // namespace Aleph

#endif  // COMPILER_CFG_H
