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

/** @file Compiler_AST.H
 *  @brief Arena-backed AST node model for the compiler-support MVP.
 *
 *  This header defines a small typed Abstract Syntax Tree (AST) for
 *  expressions, statements, type annotations, import/type/function
 *  declarations, and modules. Nodes are allocated through
 *  `Compiler_Ast_Context` and remain valid until the context is reset or
 *  destroyed.
 *
 *  The current grammar-oriented AST is intentionally compact: it covers the
 *  language surface parsed by `Compiler_Parser.H` without committing yet to a
 *  richer semantic layer.
 *
 *  @ingroup Utilities
 */

#ifndef COMPILER_AST_H
#define COMPILER_AST_H

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <Compiler_Token.H>
#include <ah-arena.H>
#include <ah-errors.H>
#include <tpl_dynArray.H>

namespace Aleph {

// Forward declarations
struct Compiler_Expr;
struct Compiler_Stmt;
struct Compiler_Block_Stmt;
struct Compiler_Function_Decl;
struct Compiler_Module;
struct Compiler_Import_Decl;
struct Compiler_Type_Expr;
struct Compiler_Type_Decl;

/** @brief Merges two spans from the same source file.
 *
 *  Calculates a span that encompasses both input spans. If either span is 
 *  invalid, the other is returned unchanged.
 *
 *  @param lhs Left-hand side span.
 *  @param rhs Right-hand side span.
 *  @return A span covering the range from the minimum begin to the maximum end.
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
inline Source_Span compiler_merge_spans(const Source_Span &lhs, const Source_Span &rhs) noexcept
{
  if (not lhs.is_valid())
    return rhs;
  if (not rhs.is_valid())
    return lhs;
  if (lhs.file_id != rhs.file_id)
    return lhs;
  return {lhs.file_id,
          lhs.begin < rhs.begin ? lhs.begin : rhs.begin,
          lhs.end > rhs.end ? lhs.end : rhs.end};
}

/** @brief Enumeration of expression node kinds. 
 *
 *  Distinguishes between different types of expressions in the AST for 
 *  runtime type identification and dispatch.
 *
 *  @note Thread-safety: Thread-safe (POD).
 *  @note Complexity: O(1).
 */
enum class Compiler_Expr_Kind
{
  Invalid,         ///< Placeholder for malformed expressions.
  Identifier,      ///< Variable or function name reference.
  Integer_Literal, ///< Numeric integer constant.
  String_Literal,  ///< String constant.
  Char_Literal,    ///< Character constant.
  Bool_Literal,    ///< Boolean constant (true/false).
  Grouping,        ///< Parenthesized expression.
  Unary,           ///< Prefix unary operation (e.g., -x, !p).
  Binary,          ///< Infix binary operation (e.g., x + y).
  Call             ///< Function or method call.
};

/** @brief Enumeration of statement node kinds. 
 *
 *  Identifies the type of a statement for structured control flow analysis 
 *  and execution.
 *
 *  @note Thread-safety: Thread-safe (POD).
 *  @note Complexity: O(1).
 */
enum class Compiler_Stmt_Kind
{
  Invalid,  ///< Placeholder for malformed statements.
  Expr,     ///< Expression evaluated for side effects (e.g., assignment, call).
  Let,      ///< Variable declaration and optional initialization.
  Return,   ///< Exit current function with an optional value.
  Block,    ///< Scoped sequence of statements.
  If,       ///< Conditional execution.
  While,    ///< Loop while a condition is true.
  Break,    ///< Immediate exit from the innermost loop.
  Continue  ///< Skip to the next iteration of the innermost loop.
};

/** @brief Enumeration of parsed type-expression node kinds.
 *
 *  Type expressions represent explicit source-level annotations and are later
 *  resolved into `Compiler_Types.H`.
 *
 *  @note Thread-safety: Thread-safe (POD).
 *  @note Complexity: O(1).
 */
enum class Compiler_Type_Expr_Kind
{
  Invalid,  ///< Placeholder for malformed type syntax.
  Named,    ///< Named type such as `Int` or `T`.
  Tuple,    ///< Tuple type such as `(Int, Bool)`.
  Function  ///< Function type such as `fn(Int) -> Bool`.
};

/** @brief Enumeration of top-level type declaration node kinds.
 *
 *  These declarations introduce named surface-level types such as aliases,
 *  nominal `struct`s, and nominal `enum`s.
 *
 *  @note Thread-safety: Thread-safe (POD).
 *  @note Complexity: O(1).
 */
enum class Compiler_Type_Decl_Kind
{
  Invalid,  ///< Placeholder for malformed type declarations.
  Alias,    ///< Transparent type alias declaration.
  Struct,   ///< Nominal `struct` declaration.
  Enum      ///< Nominal `enum` declaration.
};

/** @brief Enumeration of top-level import declaration node kinds.
 *
 *  Imports are intentionally small in the current MVP: they name one other
 *  source unit through a string literal such as `import "math.aw";`.
 *
 *  @note Thread-safety: Thread-safe (POD).
 *  @note Complexity: O(1).
 */
enum class Compiler_Import_Decl_Kind
{
  Invalid,  ///< Placeholder for malformed import declarations.
  Import    ///< One concrete `import "name";` declaration.
};

/** @brief Returns a stable debug name for an expression kind. 
 *
 *  @param kind Expression kind to convert to string.
 *  @return String literal name suitable for logging.
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
inline const char *compiler_expr_kind_name(const Compiler_Expr_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Expr_Kind::Invalid:
      return "Invalid";
    case Compiler_Expr_Kind::Identifier:
      return "Identifier";
    case Compiler_Expr_Kind::Integer_Literal:
      return "Integer_Literal";
    case Compiler_Expr_Kind::String_Literal:
      return "String_Literal";
    case Compiler_Expr_Kind::Char_Literal:
      return "Char_Literal";
    case Compiler_Expr_Kind::Bool_Literal:
      return "Bool_Literal";
    case Compiler_Expr_Kind::Grouping:
      return "Grouping";
    case Compiler_Expr_Kind::Unary:
      return "Unary";
    case Compiler_Expr_Kind::Binary:
      return "Binary";
    case Compiler_Expr_Kind::Call:
      return "Call";
    }

  return "Unknown";
}

/** @brief Returns a stable debug name for a statement kind. 
 *
 *  @param kind Statement kind to convert to string.
 *  @return String literal name suitable for logging.
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
inline const char *compiler_stmt_kind_name(const Compiler_Stmt_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Stmt_Kind::Invalid:
      return "Invalid";
    case Compiler_Stmt_Kind::Expr:
      return "Expr";
    case Compiler_Stmt_Kind::Let:
      return "Let";
    case Compiler_Stmt_Kind::Return:
      return "Return";
    case Compiler_Stmt_Kind::Block:
      return "Block";
    case Compiler_Stmt_Kind::If:
      return "If";
    case Compiler_Stmt_Kind::While:
      return "While";
    case Compiler_Stmt_Kind::Break:
      return "Break";
    case Compiler_Stmt_Kind::Continue:
      return "Continue";
    }

  return "Unknown";
}

/** @brief Returns a stable debug name for a type-expression kind.
 *
 *  @param kind Type-expression kind.
 *  @return String literal suitable for dumps and logs.
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
inline const char *compiler_type_expr_kind_name(const Compiler_Type_Expr_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Type_Expr_Kind::Invalid:
      return "Invalid";
    case Compiler_Type_Expr_Kind::Named:
      return "Named";
    case Compiler_Type_Expr_Kind::Tuple:
      return "Tuple";
    case Compiler_Type_Expr_Kind::Function:
      return "Function";
    }

  return "Unknown";
}

/** @brief Returns a stable debug name for a type-declaration kind.
 *
 *  @param kind Type-declaration kind.
 *  @return String literal suitable for dumps and logs.
 *  @note Thread-safety: Thread-safe.
 *  @note Complexity: O(1).
 */
inline const char *compiler_type_decl_kind_name(const Compiler_Type_Decl_Kind kind) noexcept
{
  switch (kind)
    {
    case Compiler_Type_Decl_Kind::Invalid:
      return "Invalid";
    case Compiler_Type_Decl_Kind::Alias:
      return "Alias";
    case Compiler_Type_Decl_Kind::Struct:
      return "Struct";
    case Compiler_Type_Decl_Kind::Enum:
      return "Enum";
    }

  return "Unknown";
}

/** @brief Abstract base class for all AST nodes. 
 *
 *  Carries common metadata like the source span.
 */
struct Compiler_Ast_Node
{
  Source_Span span;  ///< Source region covered by this node.

  /** @brief Constructs a node with an optional span. */
  explicit Compiler_Ast_Node(const Source_Span &sp = {}) noexcept : span(sp) {}

  /** @brief Virtual destructor to ensure proper cleanup of derived nodes. */
  virtual ~Compiler_Ast_Node() noexcept = default;
};

/** @brief Arena-backed ownership context for AST nodes.
 *
 *  The context uses an `AhArenaAllocator` for fast allocations and low 
 *  fragmentation. It also tracks destructors to ensure that nodes with 
 *  non-trivial members (like `std::string` or `DynArray`) are properly 
 *  cleaned up.
 *
 *  @note This class is the primary owner of the AST. Nodes allocated here 
 *        should not be deleted manually.
 */
class Compiler_Ast_Context
{
  /** @brief Internal tracking for objects with destructors. */
  struct Owned_Object
  {
    void *ptr = nullptr;                    ///< Pointer to the allocated object.
    void (*destroy)(void *) noexcept = nullptr; ///< Type-erased destructor.
  };

  AhArenaAllocator arena_;        ///< Underlying memory arena.
  DynArray<Owned_Object> owned_;  ///< Tracked objects for destruction.

public:
  /** @brief Constructs a context with the specified arena capacity.
   *
   *  @param arena_size Capacity in bytes (defaults to arena's default size).
   *  @throws std::runtime_error If memory cannot be reserved.
   *  @note Thread-safety: Not thread-safe.
   *  @note Complexity: O(1).
   */
  explicit Compiler_Ast_Context(const size_t arena_size = AhArenaAllocator::DEFAULT_SIZE)
    : arena_(arena_size)
  {}

  // Contexts cannot be copied to avoid double-free of the arena.
  Compiler_Ast_Context(const Compiler_Ast_Context &) = delete;
  Compiler_Ast_Context &operator=(const Compiler_Ast_Context &) = delete;

  /** @brief Destroys the context and all managed nodes.
   *
   *  @note Thread-safety: Not thread-safe.
   *  @note Complexity: O(N) where N is the number of nodes with destructors.
   */
  ~Compiler_Ast_Context() noexcept
  {
    reset();
  }

  /** @brief Allocates and constructs an AST node of type `T`.
   *
   *  Uses arena allocation for performance. If `T` has a non-trivial destructor, 
   *  it is registered for automatic cleanup during context reset.
   *
   *  @tparam T Node type to instantiate.
   *  @tparam Args Constructor argument types.
   *  @param args Values passed to the constructor of `T`.
   *  @return Pointer to the new node, stable until `reset()` is called.
   *  @throws std::runtime_error If the arena is out of memory.
   *  @note Thread-safety: Not thread-safe.
   *  @note Complexity: O(1) amortized.
   */
  template <typename T, typename... Args>
  T *make(Args &&...args)
  {
    T *ptr = allocate<T>(arena_, std::forward<Args>(args)...);
    ah_runtime_error_unless(ptr != nullptr) << "Compiler_Ast_Context: arena allocation failed";

    owned_.append({ptr,
                  [](void *raw) noexcept
    {
      static_cast<T *>(raw)->~T();
    }});
    return ptr;
  }

  /** @brief Reclaims all memory and destroys managed nodes.
   *
   *  Calling this method invalidates all pointers previously returned by `make()`.
   *  Useful for recycling the context between compilation units.
   *
   *  @note Thread-safety: Not thread-safe.
   *  @note Complexity: O(N) where N is the number of tracked objects.
   */
  void reset() noexcept
  {
    for (size_t i = owned_.size(); i > 0; --i)
      {
        auto &[ptr, destroy] = owned_.access(i - 1);
        if (destroy != nullptr and ptr != nullptr)
          destroy(ptr);
      }
    owned_.clear();
    arena_.reset();
  }

  /** @brief Returns the total bytes currently used in the arena. 
   *
   *  @return Consumed memory in bytes.
   *  @note Thread-safety: Safe for concurrent readers.
   *  @note Complexity: O(1).
   */
  size_t allocated_size() const noexcept
  {
    return arena_.allocated_size();
  }

  /** @brief Returns the total capacity of the arena. 
   *
   *  @return Maximum memory in bytes.
   *  @note Thread-safety: Safe for concurrent readers.
   *  @note Complexity: O(1).
   */
  size_t capacity() const noexcept
  {
    return arena_.capacity();
  }
};

/** @brief Abstract base class for expression nodes. 
 *
 *  Expressions always evaluate to a value (in the future, a typed value).
 *
 *  @note Thread-safety: Not thread-safe.
 *  @note Complexity: O(1).
 */
struct Compiler_Expr : public Compiler_Ast_Node
{
  Compiler_Expr_Kind kind = Compiler_Expr_Kind::Invalid;  ///< Specific expression type.

  /** @brief Constructs an expression base. */
  Compiler_Expr(const Compiler_Expr_Kind k = Compiler_Expr_Kind::Invalid,
                const Source_Span &sp = {}) noexcept
    : Compiler_Ast_Node(sp), kind(k)
  {}
};

/** @brief Abstract base class for statement nodes. 
 *
 *  Statements define the structure and flow of the program.
 *
 *  @note Thread-safety: Not thread-safe.
 *  @note Complexity: O(1).
 */
struct Compiler_Stmt : public Compiler_Ast_Node
{
  Compiler_Stmt_Kind kind = Compiler_Stmt_Kind::Invalid;  ///< Specific statement type.

  /** @brief Constructs a statement base. */
  Compiler_Stmt(const Compiler_Stmt_Kind k = Compiler_Stmt_Kind::Invalid,
                const Source_Span &sp = {}) noexcept
    : Compiler_Ast_Node(sp), kind(k)
  {}
};

/** @brief Abstract base class for parsed type-annotation syntax.
 *
 *  Type expressions are part of the surface AST because explicit type
 *  annotations belong to parsing, while their semantic meaning is resolved
 *  later.
 *
 *  @note Thread-safety: Not thread-safe.
 *  @note Complexity: O(1).
 */
struct Compiler_Type_Expr : public Compiler_Ast_Node
{
  Compiler_Type_Expr_Kind kind = Compiler_Type_Expr_Kind::Invalid;  ///< Specific type-expression variant.

  /** @brief Constructs a type-expression base node. */
  Compiler_Type_Expr(const Compiler_Type_Expr_Kind k = Compiler_Type_Expr_Kind::Invalid,
                     const Source_Span &sp = {}) noexcept
    : Compiler_Ast_Node(sp), kind(k)
  {}
};

/** @brief Placeholder node for type syntax that failed to parse. */
struct Compiler_Invalid_Type_Expr final : public Compiler_Type_Expr
{
  /** @brief Constructs an invalid type-expression sentinel. */
  explicit Compiler_Invalid_Type_Expr(const Source_Span &sp = {}) noexcept
    : Compiler_Type_Expr(Compiler_Type_Expr_Kind::Invalid, sp)
  {}
};

/** @brief Named type syntax such as `Int`, `Bool`, or `T`. */
struct Compiler_Named_Type_Expr final : public Compiler_Type_Expr
{
  std::string name;       ///< Raw type-name spelling.
  Source_Span name_span;  ///< Span covering only the name token.

  /** @brief Constructs a named type-expression node. */
  Compiler_Named_Type_Expr(std::string value = "",
                           const Source_Span &sp = {}) noexcept
    : Compiler_Type_Expr(Compiler_Type_Expr_Kind::Named, sp), name(std::move(value)),
      name_span(sp)
  {}
};

/** @brief Tuple type syntax such as `(Int, Bool)`. */
struct Compiler_Tuple_Type_Expr final : public Compiler_Type_Expr
{
  DynArray<Compiler_Type_Expr *> members;  ///< Tuple element types in source order.
  Source_Span lparen_span;                 ///< Span of the opening `(`.
  Source_Span rparen_span;                 ///< Span of the closing `)`.

  /** @brief Constructs a tuple type-expression node. */
  Compiler_Tuple_Type_Expr(const Source_Span &lparen = {},
                           const Source_Span &rparen = {}) noexcept
    : Compiler_Type_Expr(Compiler_Type_Expr_Kind::Tuple,
                         compiler_merge_spans(lparen, rparen)),
      members(), lparen_span(lparen), rparen_span(rparen)
  {}
};

/** @brief Function type syntax such as `fn(Int, T) -> Bool`. */
struct Compiler_Function_Type_Expr final : public Compiler_Type_Expr
{
  Source_Span keyword_span;                   ///< Span of the `fn` keyword.
  DynArray<Compiler_Type_Expr *> parameters;  ///< Parameter types in declaration order.
  Source_Span lparen_span;                    ///< Span of the opening `(`.
  Source_Span rparen_span;                    ///< Span of the closing `)`.
  Source_Span arrow_span;                     ///< Span of the `->` token.
  Compiler_Type_Expr *result = nullptr;       ///< Result type expression.

  /** @brief Constructs a function type-expression node. */
  Compiler_Function_Type_Expr(const Source_Span &kw = {},
                              const Source_Span &lparen = {},
                              const Source_Span &rparen = {},
                              const Source_Span &arrow = {},
                              Compiler_Type_Expr *res = nullptr) noexcept
    : Compiler_Type_Expr(Compiler_Type_Expr_Kind::Function,
                         compiler_merge_spans(kw,
                                              res != nullptr ? res->span
                                                              : compiler_merge_spans(rparen,
                                                                                     arrow))),
      keyword_span(kw), parameters(), lparen_span(lparen), rparen_span(rparen),
      arrow_span(arrow), result(res)
  {}
};

/** @brief Abstract base class for top-level import declarations. */
struct Compiler_Import_Decl : public Compiler_Ast_Node
{
  Compiler_Import_Decl_Kind kind = Compiler_Import_Decl_Kind::Invalid;  ///< Specific declaration variant.
  Source_Span keyword_span;      ///< Span of the introducing `import` keyword.
  std::string module_name;       ///< Imported source name without surrounding quotes.
  Source_Span module_name_span;  ///< Span of the imported string literal.
  Source_Span semicolon_span;    ///< Span of the trailing `;`.

  /** @brief Constructs an import-declaration base node. */
  Compiler_Import_Decl(const Compiler_Import_Decl_Kind k = Compiler_Import_Decl_Kind::Invalid,
                       const Source_Span &kw = {},
                       std::string imported = "",
                       const Source_Span &module_sp = {},
                       const Source_Span &semi_sp = {},
                       const Source_Span &sp = {}) noexcept
    : Compiler_Ast_Node(sp), kind(k), keyword_span(kw), module_name(std::move(imported)),
      module_name_span(module_sp), semicolon_span(semi_sp)
  {}
};

/** @brief Placeholder node for malformed import declarations. */
struct Compiler_Invalid_Import_Decl final : public Compiler_Import_Decl
{
  /** @brief Constructs an invalid import-declaration sentinel. */
  explicit Compiler_Invalid_Import_Decl(const Source_Span &sp = {}) noexcept
    : Compiler_Import_Decl(Compiler_Import_Decl_Kind::Invalid, sp, "", sp, sp, sp)
  {}
};

/** @brief Top-level import declaration naming one other source unit. */
struct Compiler_Source_Import_Decl final : public Compiler_Import_Decl
{
  /** @brief Constructs one `import "name";` declaration. */
  Compiler_Source_Import_Decl(const Source_Span &kw = {},
                              std::string imported = "",
                              const Source_Span &module_sp = {},
                              const Source_Span &semi_sp = {}) noexcept
    : Compiler_Import_Decl(Compiler_Import_Decl_Kind::Import,
                           kw,
                           std::move(imported),
                           module_sp,
                           semi_sp,
                           compiler_merge_spans(kw,
                                                semi_sp.is_valid() ? semi_sp
                                                                   : module_sp))
  {}
};

/** @brief One named field inside a parsed `struct` declaration. */
struct Compiler_Struct_Field
{
  std::string name;                  ///< Field name.
  Source_Span name_span;             ///< Span of the field name.
  Source_Span colon_span;            ///< Span of the separating `:`.
  Compiler_Type_Expr *annotation = nullptr;  ///< Declared field type.
  Source_Span semicolon_span;        ///< Span of the trailing `;`.
};

/** @brief One named variant inside a parsed `enum` declaration. */
struct Compiler_Enum_Variant
{
  std::string name;       ///< Variant name.
  Source_Span name_span;  ///< Span of the variant name.
};

/** @brief Abstract base class for top-level type declarations. */
struct Compiler_Type_Decl : public Compiler_Ast_Node
{
  Compiler_Type_Decl_Kind kind = Compiler_Type_Decl_Kind::Invalid;  ///< Specific declaration variant.
  Source_Span keyword_span;   ///< Span of the introducing keyword.
  std::string name;           ///< Declared type name.
  Source_Span name_span;      ///< Span of the declared type name.

  /** @brief Constructs a type-declaration base node. */
  Compiler_Type_Decl(const Compiler_Type_Decl_Kind k = Compiler_Type_Decl_Kind::Invalid,
                     const Source_Span &kw = {},
                     std::string value = "",
                     const Source_Span &name_sp = {},
                     const Source_Span &sp = {}) noexcept
    : Compiler_Ast_Node(sp), kind(k), keyword_span(kw), name(std::move(value)), name_span(name_sp)
  {}
};

/** @brief Placeholder node for malformed type declarations. */
struct Compiler_Invalid_Type_Decl final : public Compiler_Type_Decl
{
  /** @brief Constructs an invalid type-declaration sentinel. */
  explicit Compiler_Invalid_Type_Decl(const Source_Span &sp = {}) noexcept
    : Compiler_Type_Decl(Compiler_Type_Decl_Kind::Invalid, sp, "", sp, sp)
  {}
};

/** @brief Transparent top-level type alias declaration. */
struct Compiler_Type_Alias_Decl final : public Compiler_Type_Decl
{
  Source_Span equal_span;                ///< Span of the separating `=`.
  Compiler_Type_Expr *aliased_type = nullptr;  ///< Aliased target type expression.
  Source_Span semicolon_span;            ///< Span of the trailing `;`.

  /** @brief Constructs one type alias declaration. */
  Compiler_Type_Alias_Decl(const Source_Span &kw = {},
                           std::string value = "",
                           const Source_Span &name_sp = {},
                           const Source_Span &eq_sp = {},
                           Compiler_Type_Expr *target = nullptr,
                           const Source_Span &semi_sp = {}) noexcept
    : Compiler_Type_Decl(Compiler_Type_Decl_Kind::Alias,
                         kw,
                         std::move(value),
                         name_sp,
                         compiler_merge_spans(kw,
                                              semi_sp.is_valid() ? semi_sp
                                                                 : (target != nullptr ? target->span
                                                                                      : name_sp))),
      equal_span(eq_sp), aliased_type(target), semicolon_span(semi_sp)
  {}
};

/** @brief Nominal top-level `struct` declaration. */
struct Compiler_Struct_Decl final : public Compiler_Type_Decl
{
  DynArray<Compiler_Struct_Field> fields;  ///< Declared fields in source order.
  Source_Span lbrace_span;                 ///< Span of the opening `{`.
  Source_Span rbrace_span;                 ///< Span of the closing `}`.

  /** @brief Constructs one `struct` declaration. */
  Compiler_Struct_Decl(const Source_Span &kw = {},
                       std::string value = "",
                       const Source_Span &name_sp = {},
                       const Source_Span &lbrace_sp = {},
                       const Source_Span &rbrace_sp = {}) noexcept
    : Compiler_Type_Decl(Compiler_Type_Decl_Kind::Struct,
                         kw,
                         std::move(value),
                         name_sp,
                         compiler_merge_spans(kw,
                                              rbrace_sp.is_valid() ? rbrace_sp
                                                                   : (lbrace_sp.is_valid() ? lbrace_sp
                                                                                           : name_sp))), lbrace_span(lbrace_sp), rbrace_span(rbrace_sp)
  {}
};

/** @brief Nominal top-level `enum` declaration with unit variants. */
struct Compiler_Enum_Decl final : public Compiler_Type_Decl
{
  DynArray<Compiler_Enum_Variant> variants;  ///< Declared unit variants in source order.
  Source_Span lbrace_span;                   ///< Span of the opening `{`.
  Source_Span rbrace_span;                   ///< Span of the closing `}`.

  /** @brief Constructs one `enum` declaration. */
  Compiler_Enum_Decl(const Source_Span &kw = {},
                     std::string value = "",
                     const Source_Span &name_sp = {},
                     const Source_Span &lbrace_sp = {},
                     const Source_Span &rbrace_sp = {}) noexcept
    : Compiler_Type_Decl(Compiler_Type_Decl_Kind::Enum,
                         kw,
                         std::move(value),
                         name_sp,
                         compiler_merge_spans(kw,
                                              rbrace_sp.is_valid() ? rbrace_sp
                                                                   : (lbrace_sp.is_valid() ? lbrace_sp
                                                                                           : name_sp))), lbrace_span(lbrace_sp), rbrace_span(rbrace_sp)
  {}
};

/** @brief Placeholder node for expressions that failed to parse. 
 *
 *  Allows the parser to continue despite local errors, preventing cascading 
 *  diagnostics.
 */
struct Compiler_Invalid_Expr final : public Compiler_Expr
{
  /** @brief Constructs an invalid expression sentinel. */
  explicit Compiler_Invalid_Expr(const Source_Span &sp = {}) noexcept
    : Compiler_Expr(Compiler_Expr_Kind::Invalid, sp)
  {}
};

/** @brief Node representing a named identifier reference. 
 *
 *  Examples: `x`, `myFunction`, `count`.
 */
struct Compiler_Identifier_Expr final : public Compiler_Expr
{
  std::string name;       ///< Name of the identifier.
  Source_Span name_span;  ///< Span covering only the name lexeme.

  /** @brief Constructs an identifier reference. */
  Compiler_Identifier_Expr(std::string value = "", const Source_Span &sp = {}) noexcept
    : Compiler_Expr(Compiler_Expr_Kind::Identifier, sp), name(std::move(value)), name_span(sp)
  {}
};

/** @brief Node representing a literal value. 
 *
 *  Supports integer, string, char, and boolean literals.
 */
struct Compiler_Literal_Expr final : public Compiler_Expr
{
  std::string lexeme;       ///< Raw source spelling of the literal.
  bool bool_value = false;  ///< Boolean value (meaningful only for Bool_Literal).

  /** @brief Constructs a literal expression. */
  Compiler_Literal_Expr(const Compiler_Expr_Kind k = Compiler_Expr_Kind::Invalid,
                        std::string value = "",
                        const Source_Span &sp = {},
                        const bool bool_val = false) noexcept
    : Compiler_Expr(k, sp), lexeme(std::move(value)), bool_value(bool_val)
  {}
};

/** @brief Node representing an expression explicitly wrapped in parentheses. 
 *
 *  Preserves user-defined grouping which might be lost after parsing into a 
 *  precedence-based tree.
 */
struct Compiler_Grouping_Expr final : public Compiler_Expr
{
  Compiler_Expr *inner = nullptr;  ///< The expression inside parentheses.
  Source_Span lparen_span;         ///< Location of the opening `'('`.
  Source_Span rparen_span;         ///< Location of the closing `')'`.

  /** @brief Constructs a grouping expression. */
  Compiler_Grouping_Expr(Compiler_Expr *expr = nullptr,
                         const Source_Span &lparen = {},
                         const Source_Span &rparen = {}) noexcept
    : Compiler_Expr(Compiler_Expr_Kind::Grouping,
                    compiler_merge_spans(lparen,
                                         expr != nullptr ? compiler_merge_spans(expr->span, rparen)
                                                         : rparen)),
      inner(expr), lparen_span(lparen), rparen_span(rparen)
  {}
};

/** @brief Node representing a prefix unary operation. 
 *
 *  Examples: `-x`, `!flag`, `~mask`.
 */
struct Compiler_Unary_Expr final : public Compiler_Expr
{
  Compiler_Token_Kind op = Compiler_Token_Kind::Invalid;  ///< Operator kind (e.g., Minus, Not).
  Source_Span operator_span;                              ///< Location of the operator token.
  Compiler_Expr *operand = nullptr;                       ///< Expression being operated upon.

  /** @brief Constructs a unary expression. */
  Compiler_Unary_Expr(const Compiler_Token_Kind op_kind = Compiler_Token_Kind::Invalid,
                      const Source_Span &op_sp = {},
                      Compiler_Expr *expr = nullptr) noexcept
    : Compiler_Expr(Compiler_Expr_Kind::Unary,
                    compiler_merge_spans(op_sp, expr != nullptr ? expr->span : Source_Span())),
      op(op_kind), operator_span(op_sp), operand(expr)
  {}
};

/** @brief Node representing an infix binary operation. 
 *
 *  Examples: `a + b`, `x * y`, `i < 10`.
 */
struct Compiler_Binary_Expr final : public Compiler_Expr
{
  Compiler_Token_Kind op = Compiler_Token_Kind::Invalid;  ///< Operator kind (e.g., Plus, Equal).
  Source_Span operator_span;                              ///< Location of the operator token.
  Compiler_Expr *left = nullptr;                          ///< Left-hand side operand.
  Compiler_Expr *right = nullptr;                         ///< Right-hand side operand.

  /** @brief Constructs a binary expression. */
  Compiler_Binary_Expr(Compiler_Expr *lhs = nullptr,
                       const Compiler_Token_Kind op_kind = Compiler_Token_Kind::Invalid,
                       const Source_Span &op_sp = {},
                       Compiler_Expr *rhs = nullptr) noexcept
    : Compiler_Expr(Compiler_Expr_Kind::Binary,
                    lhs != nullptr
                      ? compiler_merge_spans(lhs->span, rhs != nullptr ? rhs->span : op_sp)
                      : compiler_merge_spans(op_sp, rhs != nullptr ? rhs->span : Source_Span())),
      op(op_kind), operator_span(op_sp), left(lhs), right(rhs)
  {}
};

/** @brief Node representing a function or method call. 
 *
 *  Example: `calculate(a, b, 5)`.
 */
struct Compiler_Call_Expr final : public Compiler_Expr
{
  Compiler_Expr *callee = nullptr;      ///< Expression resolving to a callable entity.
  DynArray<Compiler_Expr *> arguments;  ///< list of call arguments.
  Source_Span lparen_span;              ///< Location of `'('`.
  Source_Span rparen_span;              ///< Location of `')'`.

  /** @brief Constructs a call expression. */
  Compiler_Call_Expr(Compiler_Expr *func = nullptr,
                     const Source_Span &lparen = {},
                     const Source_Span &rparen = {}) noexcept
    : Compiler_Expr(Compiler_Expr_Kind::Call,
                    func != nullptr ? compiler_merge_spans(func->span, rparen)
                                    : compiler_merge_spans(lparen, rparen)),
      callee(func), lparen_span(lparen), rparen_span(rparen)
  {}
};

/** @brief Placeholder node for statements that failed to parse. */
struct Compiler_Invalid_Stmt final : public Compiler_Stmt
{
  /** @brief Constructs an invalid statement sentinel. */
  explicit Compiler_Invalid_Stmt(const Source_Span &sp = {}) noexcept
    : Compiler_Stmt(Compiler_Stmt_Kind::Invalid, sp)
  {}
};

/** @brief Node representing an expression evaluated as a statement. 
 *
 *  Typically terminated by a semicolon.
 */
struct Compiler_Expr_Stmt final : public Compiler_Stmt
{
  Compiler_Expr *expr = nullptr;  ///< The expression being evaluated.
  Source_Span semicolon_span;     ///< Location of the trailing `';'`.

  /** @brief Constructs an expression statement. */
  Compiler_Expr_Stmt(Compiler_Expr *value = nullptr, const Source_Span &semi = {}) noexcept
    : Compiler_Stmt(Compiler_Stmt_Kind::Expr,
                    value != nullptr ? compiler_merge_spans(value->span, semi) : semi),
      expr(value), semicolon_span(semi)
  {}
};

/** @brief Node representing a local variable binding. 
 *
 *  Example: `let x = 10;`.
 */
struct Compiler_Let_Stmt final : public Compiler_Stmt
{
  Source_Span keyword_span;              ///< Location of the `let` keyword.
  std::string name;                      ///< Name of the bound variable.
  Source_Span name_span;                 ///< Location of the variable name.
  Source_Span colon_span;                ///< Location of the optional `:` before the type annotation.
  Compiler_Type_Expr *annotation = nullptr;  ///< Optional declared binding type.
  Compiler_Expr *initializer = nullptr;  ///< Optional initialization expression.
  Source_Span semicolon_span;            ///< Location of the trailing `';'`.

  /** @brief Constructs a variable binding statement. */
  Compiler_Let_Stmt(const Source_Span &kw = {},
                    std::string value = "",
                    const Source_Span &name_sp = {},
                    const Source_Span &colon_sp = {},
                    Compiler_Type_Expr *annot = nullptr,
                    Compiler_Expr *init = nullptr,
                    const Source_Span &semi = {}) noexcept
    : Compiler_Stmt(
        Compiler_Stmt_Kind::Let,
        compiler_merge_spans(kw,
                             semi.is_valid() ? semi
                                             : (init != nullptr ? init->span
                                                                : (annot != nullptr ? annot->span
                                                                                    : name_sp)))),
      keyword_span(kw), name(std::move(value)), name_span(name_sp), colon_span(colon_sp),
      annotation(annot), initializer(init),
      semicolon_span(semi)
  {}
};

/** @brief Node representing a return from the current function. 
 *
 *  Example: `return x + 1;`.
 */
struct Compiler_Return_Stmt final : public Compiler_Stmt
{
  Source_Span keyword_span;        ///< Location of the `return` keyword.
  Compiler_Expr *value = nullptr;  ///< Optional value being returned.
  Source_Span semicolon_span;      ///< Location of the trailing `';'`.

  /** @brief Constructs a return statement. */
  Compiler_Return_Stmt(const Source_Span &kw = {},
                       Compiler_Expr *expr = nullptr,
                       const Source_Span &semi = {}) noexcept
    : Compiler_Stmt(Compiler_Stmt_Kind::Return,
                    compiler_merge_spans(
                      kw, semi.is_valid() ? semi : (expr != nullptr ? expr->span : Source_Span()))),
      keyword_span(kw), value(expr), semicolon_span(semi)
  {}
};

/** @brief Node representing a braced sequence of statements. 
 *
 *  Provides scope for variables declared within it.
 */
struct Compiler_Block_Stmt final : public Compiler_Stmt
{
  DynArray<Compiler_Stmt *> statements;  ///< Sequential statements in the block.
  Source_Span lbrace_span;               ///< Location of the opening `'{'`.
  Source_Span rbrace_span;               ///< Location of the closing `'}'`.

  /** @brief Constructs a block statement. */
  Compiler_Block_Stmt(const Source_Span &lbrace = {}, const Source_Span &rbrace = {}) noexcept
    : Compiler_Stmt(Compiler_Stmt_Kind::Block, compiler_merge_spans(lbrace, rbrace)),
      lbrace_span(lbrace), rbrace_span(rbrace)
  {}
};

/** @brief Node representing a conditional branch. 
 *
 *  Example: `if (x > 0) { ... } else { ... }`.
 */
struct Compiler_If_Stmt final : public Compiler_Stmt
{
  Source_Span if_span;                   ///< Location of the `if` keyword.
  Compiler_Expr *condition = nullptr;    ///< Boolean expression governing the branch.
  Compiler_Stmt *then_branch = nullptr;  ///< Statement executed if true.
  Source_Span else_span;                 ///< Location of the `else` keyword (if present).
  Compiler_Stmt *else_branch = nullptr;  ///< Statement executed if false (optional).

  /** @brief Constructs an if-else statement. */
  Compiler_If_Stmt(const Source_Span &if_kw = {},
                   Compiler_Expr *cond = nullptr,
                   Compiler_Stmt *then_stmt = nullptr,
                   const Source_Span &else_kw = {},
                   Compiler_Stmt *else_stmt = nullptr) noexcept
    : Compiler_Stmt(Compiler_Stmt_Kind::If,
                    compiler_merge_spans(if_kw,
                                         else_stmt != nullptr
                                           ? else_stmt->span
                                           : (then_stmt != nullptr
                                                ? then_stmt->span
                                                : (cond != nullptr ? cond->span : else_kw)))),
      if_span(if_kw), condition(cond), then_branch(then_stmt), else_span(else_kw),
      else_branch(else_stmt)
  {}
};

/** @brief Node representing a while-loop. 
 *
 *  Example: `while (i < 10) { ... }`.
 */
struct Compiler_While_Stmt final : public Compiler_Stmt
{
  Source_Span keyword_span;            ///< Location of the `while` keyword.
  Compiler_Expr *condition = nullptr;  ///< Condition evaluated before each iteration.
  Compiler_Stmt *body = nullptr;       ///< Statement(s) executed repeatedly.

  /** @brief Constructs a while statement. */
  Compiler_While_Stmt(const Source_Span &kw = {},
                      Compiler_Expr *cond = nullptr,
                      Compiler_Stmt *stmt = nullptr) noexcept
    : Compiler_Stmt(Compiler_Stmt_Kind::While,
                    compiler_merge_spans(kw,
                                         stmt != nullptr
                                           ? stmt->span
                                           : (cond != nullptr ? cond->span : Source_Span()))),
      keyword_span(kw), condition(cond), body(stmt)
  {}
};

/** @brief Node representing a loop break. 
 *
 *  Example: `break;`.
 */
struct Compiler_Break_Stmt final : public Compiler_Stmt
{
  Source_Span keyword_span;    ///< Location of the `break` keyword.
  Source_Span semicolon_span;  ///< Location of the trailing `';'`.

  /** @brief Constructs a break statement. */
  Compiler_Break_Stmt(const Source_Span &kw = {}, const Source_Span &semi = {}) noexcept
    : Compiler_Stmt(Compiler_Stmt_Kind::Break, compiler_merge_spans(kw, semi)), keyword_span(kw),
      semicolon_span(semi)
  {}
};

/** @brief Node representing a loop continuation. 
 *
 *  Example: `continue;`.
 */
struct Compiler_Continue_Stmt final : public Compiler_Stmt
{
  Source_Span keyword_span;    ///< Location of the `continue` keyword.
  Source_Span semicolon_span;  ///< Location of the trailing `';'`.

  /** @brief Constructs a continue statement. */
  Compiler_Continue_Stmt(const Source_Span &kw = {}, const Source_Span &semi = {}) noexcept
    : Compiler_Stmt(Compiler_Stmt_Kind::Continue, compiler_merge_spans(kw, semi)), keyword_span(kw),
      semicolon_span(semi)
  {}
};

/** @brief Representation of a function parameter. */
struct Compiler_Param
{
  std::string name;  ///< Parameter identifier.
  Source_Span span;  ///< Location in the source.
  Source_Span colon_span;  ///< Location of the optional `:` before the annotation.
  Compiler_Type_Expr *annotation = nullptr;  ///< Optional declared parameter type.
};

/** @brief Node representing a top-level function declaration. 
 *
 *  Example: `fn add(a, b) { return a + b; }`.
 */
struct Compiler_Function_Decl : public Compiler_Ast_Node
{
  Source_Span keyword_span;             ///< Location of the `fn` keyword.
  std::string name;                     ///< Function name.
  Source_Span name_span;                ///< Location of the function name token.
  DynArray<Compiler_Param> parameters;  ///< Ordered list of parameters.
  Source_Span arrow_span;               ///< Location of the optional `->` before the result annotation.
  Compiler_Type_Expr *return_annotation = nullptr;  ///< Optional declared return type.
  Compiler_Block_Stmt *body = nullptr;  ///< Scoped body of the function.

  /** @brief Constructs a function declaration. */
  Compiler_Function_Decl(const Source_Span &kw = {},
                         std::string value = "",
                         const Source_Span &name_sp = {},
                         const Source_Span &arrow_sp = {},
                         Compiler_Type_Expr *result_annot = nullptr,
                         Compiler_Block_Stmt *block = nullptr) noexcept
    : Compiler_Ast_Node(compiler_merge_spans(kw,
                                             block != nullptr ? block->span
                                                               : (result_annot != nullptr
                                                                    ? result_annot->span
                                                                    : name_sp))),
      keyword_span(kw), name(std::move(value)), name_span(name_sp), arrow_span(arrow_sp),
      return_annotation(result_annot), body(block)
  {}
};

/** @brief Node representing a complete translation unit or module. 
 *
 *  Aggregates one parsed source file, including top-level imports, type
 *  declarations, functions, and statements.
 */
struct Compiler_Module : public Compiler_Ast_Node
{
  std::string source_name;                   ///< User-facing source name that produced the module.
  DynArray<Compiler_Import_Decl *> imports;  ///< Top-level import declarations.
  DynArray<Compiler_Type_Decl *> type_declarations;  ///< Top-level type declarations.
  DynArray<Compiler_Function_Decl *> functions;  ///< Top-level function definitions.
  DynArray<Compiler_Stmt *> statements;          ///< Optional top-level code.

  /** @brief Constructs a module node. */
  explicit Compiler_Module(const Source_Span &sp = {}) noexcept : Compiler_Ast_Node(sp) {}
};

namespace Compiler_Ast_Detail {

/** @brief Escapes control characters in a string for safe text output. */
inline std::string escape_text(std::string_view text)
{
  std::string escaped;
  escaped.reserve(text.size());
  for (const char ch : text)
    switch (ch)
      {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped.push_back(ch);
      }
  return escaped;
}

/** @brief Dumps one parsed type-expression in a compact stable form. */
inline void dump_type_expr(const Compiler_Type_Expr *type,
                           std::ostream &out)
{
  if (type == nullptr)
    {
      out << "<null-type>";
      return;
    }

  switch (type->kind)
    {
    case Compiler_Type_Expr_Kind::Invalid:
      out << "InvalidType";
      return;

    case Compiler_Type_Expr_Kind::Named:
      out << static_cast<const Compiler_Named_Type_Expr *>(type)->name;
      return;

    case Compiler_Type_Expr_Kind::Tuple:
      {
        const auto *node = static_cast<const Compiler_Tuple_Type_Expr *>(type);
        out << '(';
        for (size_t i = 0; i < node->members.size(); ++i)
          {
            if (i > 0)
              out << ", ";
            dump_type_expr(node->members.access(i), out);
          }
        out << ')';
        return;
      }

    case Compiler_Type_Expr_Kind::Function:
      {
        const auto *node = static_cast<const Compiler_Function_Type_Expr *>(type);
        out << "fn(";
        for (size_t i = 0; i < node->parameters.size(); ++i)
          {
            if (i > 0)
              out << ", ";
            dump_type_expr(node->parameters.access(i), out);
          }
        out << ") -> ";
        dump_type_expr(node->result, out);
        return;
      }
    }

  out << "<unknown-type>";
}

/** @brief Utility to append spaces to a stream for indentation. */
inline void append_indent(std::ostream &out, const size_t indent)
{
  out << std::string(indent, ' ');
}

/** @brief Recursive type-declaration printer for internal use. */
inline void dump_type_decl(const Compiler_Type_Decl *decl, std::ostream &out, const size_t indent);

/** @brief Recursive expression printer for internal use. */
inline void dump_expr(const Compiler_Expr *expr, std::ostream &out, const size_t indent);

/** @brief Recursive statement printer for internal use. */
inline void dump_stmt(const Compiler_Stmt *stmt, std::ostream &out, const size_t indent);

inline void dump_expr(const Compiler_Expr *expr, std::ostream &out, const size_t indent)
{
  append_indent(out, indent);
  if (expr == nullptr)
    {
      out << "<null-expr>\n";
      return;
    }

  switch (expr->kind)
    {
    case Compiler_Expr_Kind::Invalid:
      out << "InvalidExpr\n";
      break;
    case Compiler_Expr_Kind::Identifier:
      {
        const auto *node = static_cast<const Compiler_Identifier_Expr *>(expr);
        out << "Identifier(" << node->name << ")\n";
        break;
      }
    case Compiler_Expr_Kind::Integer_Literal:
      {
        const auto *node = static_cast<const Compiler_Literal_Expr *>(expr);
        out << "Integer(" << node->lexeme << ")\n";
        break;
      }
    case Compiler_Expr_Kind::String_Literal:
      {
        const auto *node = static_cast<const Compiler_Literal_Expr *>(expr);
        out << "String(\"" << escape_text(node->lexeme) << "\")\n";
        break;
      }
    case Compiler_Expr_Kind::Char_Literal:
      {
        const auto *node = static_cast<const Compiler_Literal_Expr *>(expr);
        out << "Char(\"" << escape_text(node->lexeme) << "\")\n";
        break;
      }
    case Compiler_Expr_Kind::Bool_Literal:
      {
        const auto *node = static_cast<const Compiler_Literal_Expr *>(expr);
        out << "Bool(" << (node->bool_value ? "true" : "false") << ")\n";
        break;
      }
    case Compiler_Expr_Kind::Grouping:
      {
        const auto *node = static_cast<const Compiler_Grouping_Expr *>(expr);
        out << "Grouping\n";
        dump_expr(node->inner, out, indent + 2);
        break;
      }
    case Compiler_Expr_Kind::Unary:
      {
        const auto *node = static_cast<const Compiler_Unary_Expr *>(expr);
        out << "Unary(" << compiler_token_kind_name(node->op) << ")\n";
        dump_expr(node->operand, out, indent + 2);
        break;
      }
    case Compiler_Expr_Kind::Binary:
      {
        const auto *node = static_cast<const Compiler_Binary_Expr *>(expr);
        out << "Binary(" << compiler_token_kind_name(node->op) << ")\n";
        dump_expr(node->left, out, indent + 2);
        dump_expr(node->right, out, indent + 2);
        break;
      }
    case Compiler_Expr_Kind::Call:
      {
        const auto *node = static_cast<const Compiler_Call_Expr *>(expr);
        out << "Call\n";
        append_indent(out, indent + 2);
        out << "Callee:\n";
        dump_expr(node->callee, out, indent + 4);
        append_indent(out, indent + 2);
        out << "Args:\n";
        for (size_t i = 0; i < node->arguments.size(); ++i)
          dump_expr(node->arguments.access(i), out, indent + 4);
        break;
      }
    }
}

inline void dump_stmt(const Compiler_Stmt *stmt, std::ostream &out, const size_t indent)
{
  append_indent(out, indent);
  if (stmt == nullptr)
    {
      out << "<null-stmt>\n";
      return;
    }

  switch (stmt->kind)
    {
    case Compiler_Stmt_Kind::Invalid:
      out << "InvalidStmt\n";
      break;
    case Compiler_Stmt_Kind::Expr:
      {
        const auto *node = static_cast<const Compiler_Expr_Stmt *>(stmt);
        out << "ExprStmt\n";
        dump_expr(node->expr, out, indent + 2);
        break;
      }
    case Compiler_Stmt_Kind::Let:
      {
        const auto *node = static_cast<const Compiler_Let_Stmt *>(stmt);
        out << "Let(" << node->name << ")";
        if (node->annotation != nullptr)
          {
            out << ": ";
            dump_type_expr(node->annotation, out);
          }
        out << '\n';
        if (node->initializer != nullptr)
          dump_expr(node->initializer, out, indent + 2);
        break;
      }
    case Compiler_Stmt_Kind::Return:
      {
        const auto *node = static_cast<const Compiler_Return_Stmt *>(stmt);
        out << "Return\n";
        if (node->value != nullptr)
          dump_expr(node->value, out, indent + 2);
        break;
      }
    case Compiler_Stmt_Kind::Block:
      {
        const auto *node = static_cast<const Compiler_Block_Stmt *>(stmt);
        out << "Block\n";
        for (size_t i = 0; i < node->statements.size(); ++i)
          dump_stmt(node->statements.access(i), out, indent + 2);
        break;
      }
    case Compiler_Stmt_Kind::If:
      {
        const auto *node = static_cast<const Compiler_If_Stmt *>(stmt);
        out << "If\n";
        append_indent(out, indent + 2);
        out << "Condition:\n";
        dump_expr(node->condition, out, indent + 4);
        append_indent(out, indent + 2);
        out << "Then:\n";
        dump_stmt(node->then_branch, out, indent + 4);
        if (node->else_branch != nullptr)
          {
            append_indent(out, indent + 2);
            out << "Else:\n";
            dump_stmt(node->else_branch, out, indent + 4);
          }
        break;
      }
    case Compiler_Stmt_Kind::While:
      {
        const auto *node = static_cast<const Compiler_While_Stmt *>(stmt);
        out << "While\n";
        append_indent(out, indent + 2);
        out << "Condition:\n";
        dump_expr(node->condition, out, indent + 4);
        append_indent(out, indent + 2);
        out << "Body:\n";
        dump_stmt(node->body, out, indent + 4);
        break;
      }
    case Compiler_Stmt_Kind::Break:
      out << "Break\n";
      break;
    case Compiler_Stmt_Kind::Continue:
      out << "Continue\n";
      break;
    }
}

inline void dump_import_decl(const Compiler_Import_Decl *decl,
                             std::ostream &out,
                             const size_t indent)
{
  append_indent(out, indent);
  if (decl == nullptr)
    {
      out << "<null-import-decl>\n";
      return;
    }

  switch (decl->kind)
    {
    case Compiler_Import_Decl_Kind::Invalid:
      out << "InvalidImportDecl\n";
      return;

    case Compiler_Import_Decl_Kind::Import:
      {
        const auto *node = static_cast<const Compiler_Source_Import_Decl *>(decl);
        out << "Import(\"" << escape_text(node->module_name) << "\")\n";
        return;
      }
    }
}

inline void dump_type_decl(const Compiler_Type_Decl *decl, std::ostream &out, const size_t indent)
{
  append_indent(out, indent);
  if (decl == nullptr)
    {
      out << "<null-type-decl>\n";
      return;
    }

  switch (decl->kind)
    {
    case Compiler_Type_Decl_Kind::Invalid:
      out << "InvalidTypeDecl\n";
      return;

    case Compiler_Type_Decl_Kind::Alias:
      {
        const auto *node = static_cast<const Compiler_Type_Alias_Decl *>(decl);
        out << "TypeAlias(" << node->name << ") = ";
        dump_type_expr(node->aliased_type, out);
        out << '\n';
        return;
      }

    case Compiler_Type_Decl_Kind::Struct:
      {
        const auto *node = static_cast<const Compiler_Struct_Decl *>(decl);
        out << "Struct(" << node->name << ")\n";
        for (size_t i = 0; i < node->fields.size(); ++i)
          {
            append_indent(out, indent + 2);
            out << "Field(" << node->fields.access(i).name << "): ";
            dump_type_expr(node->fields.access(i).annotation, out);
            out << '\n';
          }
        return;
      }

    case Compiler_Type_Decl_Kind::Enum:
      {
        const auto *node = static_cast<const Compiler_Enum_Decl *>(decl);
        out << "Enum(" << node->name << ")\n";
        for (size_t i = 0; i < node->variants.size(); ++i)
          {
            append_indent(out, indent + 2);
            out << "Variant(" << node->variants.access(i).name << ")\n";
          }
        return;
      }
    }
}
}  // namespace Compiler_Ast_Detail

/** @brief Dumps an expression subtree in a deterministic text format. 
 *
 *  @param expr Root of the expression tree.
 *  @return Multiline string representation.
 *  @note Thread-safety: Safe for concurrent readers.
 *  @note Complexity: O(N) where N is number of nodes.
 */
inline std::string compiler_dump_expr(const Compiler_Expr *expr)
{
  std::ostringstream out;
  Compiler_Ast_Detail::dump_expr(expr, out, 0);
  return out.str();
}

/** @brief Dumps a statement subtree in a deterministic text format. 
 *
 *  @param stmt Root of the statement tree.
 *  @return Multiline string representation.
 *  @note Thread-safety: Safe for concurrent readers.
 *  @note Complexity: O(N) where N is number of nodes.
 */
inline std::string compiler_dump_stmt(const Compiler_Stmt *stmt)
{
  std::ostringstream out;
  Compiler_Ast_Detail::dump_stmt(stmt, out, 0);
  return out.str();
}

/** @brief Dumps an import declaration subtree in a deterministic text format.
 *
 *  @param decl Root of the import declaration subtree.
 *  @return Multiline string representation.
 *  @note Thread-safety: Safe for concurrent readers.
 *  @note Complexity: O(N) where N is number of contained nodes.
 */
inline std::string compiler_dump_import_decl(const Compiler_Import_Decl *decl)
{
  std::ostringstream out;
  Compiler_Ast_Detail::dump_import_decl(decl, out, 0);
  return out.str();
}

/** @brief Dumps a type declaration subtree in a deterministic text format.
 *
 *  @param decl Root of the type declaration subtree.
 *  @return Multiline string representation.
 *  @note Thread-safety: Safe for concurrent readers.
 *  @note Complexity: O(N) where N is number of contained nodes.
 */
inline std::string compiler_dump_type_decl(const Compiler_Type_Decl *decl)
{
  std::ostringstream out;
  Compiler_Ast_Detail::dump_type_decl(decl, out, 0);
  return out.str();
}

/** @brief Dumps a function declaration in a deterministic text format. 
 *
 *  @param fn Function declaration node.
 *  @return Multiline string representation.
 *  @note Thread-safety: Safe for concurrent readers.
 *  @note Complexity: O(N) where N is number of nodes.
 */
inline std::string compiler_dump_function(const Compiler_Function_Decl *fn)
{
  std::ostringstream out;
  if (fn == nullptr)
    return "<null-function>\n";

  out << "Function(" << fn->name << ")\n";
  if (fn->parameters.size() > 0)
    {
      out << "  Params:";
      for (size_t i = 0; i < fn->parameters.size(); ++i)
        {
          if (i > 0)
            out << ",";
          out << " " << fn->parameters.access(i).name;
          if (fn->parameters.access(i).annotation != nullptr)
            {
              out << ": ";
              Compiler_Ast_Detail::dump_type_expr(fn->parameters.access(i).annotation, out);
            }
        }
      out << '\n';
    }
  else
    out << "  Params:\n";

  if (fn->return_annotation != nullptr)
    {
      out << "  Return: ";
      Compiler_Ast_Detail::dump_type_expr(fn->return_annotation, out);
      out << '\n';
    }

  out << "  Body:\n";
  Compiler_Ast_Detail::dump_stmt(fn->body, out, 4);
  return out.str();
}

/** @brief Dumps a parsed module in a deterministic text format. 
 *
 *  @param module Module node.
 *  @return Multiline string representation.
 *  @note Thread-safety: Safe for concurrent readers.
 *  @note Complexity: O(N) where N is number of nodes.
 */
inline std::string compiler_dump_module(const Compiler_Module *module)
{
  std::ostringstream out;
  if (module == nullptr)
    return "<null-module>\n";

  out << "Module\n";
  for (size_t i = 0; i < module->imports.size(); ++i)
    Compiler_Ast_Detail::dump_import_decl(module->imports.access(i), out, 2);

  for (size_t i = 0; i < module->type_declarations.size(); ++i)
    Compiler_Ast_Detail::dump_type_decl(module->type_declarations.access(i), out, 2);

  for (size_t i = 0; i < module->functions.size(); ++i)
    {
      const auto *fn = module->functions.access(i);
      Compiler_Ast_Detail::append_indent(out, 2);
      if (fn == nullptr)
        {
          out << "Function(null)\n";
          continue;
        }
      out << "Function(" << fn->name << ")\n";
      if (fn->parameters.size() > 0)
        {
          Compiler_Ast_Detail::append_indent(out, 4);
          out << "Params:";
          for (size_t j = 0; j < fn->parameters.size(); ++j)
            {
              if (j > 0)
                out << ",";
              out << " " << fn->parameters.access(j).name;
              if (fn->parameters.access(j).annotation != nullptr)
                {
                  out << ": ";
                  Compiler_Ast_Detail::dump_type_expr(fn->parameters.access(j).annotation, out);
                }
            }
          out << '\n';
        }
      else
        {
          Compiler_Ast_Detail::append_indent(out, 4);
          out << "Params:\n";
        }

      if (fn->return_annotation != nullptr)
        {
          Compiler_Ast_Detail::append_indent(out, 4);
          out << "Return: ";
          Compiler_Ast_Detail::dump_type_expr(fn->return_annotation, out);
          out << '\n';
        }

      Compiler_Ast_Detail::append_indent(out, 4);
      out << "Body:\n";
      Compiler_Ast_Detail::dump_stmt(fn->body, out, 6);
    }

  for (size_t i = 0; i < module->statements.size(); ++i)
    Compiler_Ast_Detail::dump_stmt(module->statements.access(i), out, 2);

  return out.str();
}

}  // namespace Aleph

#endif  // COMPILER_AST_H
