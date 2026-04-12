# Compiler Front-End Support

This guide documents the first compiler-oriented support layer added to Aleph-w.
It is intentionally small and reusable: the goal is to make lexers, parsers and
semantic analyzers easier to build on top of the library without committing yet
to one concrete language design.

## Scope

The current component is formed by eleven public headers:

| Header | Responsibility |
|---|---|
| `ah-source.H` | Source file registry, byte offsets, spans, line/column resolution, and single-line snippets |
| `ah-diagnostics.H` | Buffered diagnostics with primary/secondary labels, notes, help messages, and deterministic plain-text rendering |
| `Compiler_Token.H` | Token kinds, token objects, keyword classification, and binary-operator metadata for future parsers |
| `Compiler_Lexer.H` | Single-file lexer with stable spans, comment handling options, invalid-token recovery, and optional diagnostic emission |
| `Compiler_AST.H` | Arena-backed AST nodes for expressions, statements, functions, modules, and deterministic tree dumping |
| `Compiler_Parser.H` | Recursive-descent parser with precedence climbing, block/statement parsing, and diagnostic-based recovery |
| `tpl_scope.H` | Generic nested-scope stack for lexical bindings and shadowing-aware lookup |
| `Compiler_Sema.H` | Name resolution, duplicate detection, shadowing checks, control-flow misuse diagnostics, and call-to-non-function checks |
| `Compiler_Types.H` | Stable type graph with built-ins, tuples, functions, type variables, and deterministic pretty-printing |
| `tpl_constraints.H` | Equality constraint sets, substitutions, structural unification, rigid variables, and occurs-check support |
| `Compiler_Typed_Sema.H` | Typed semantic pass that connects the AST, the base semantic pass, the type graph, and the unifier |

## Quick Start

```cpp
#include <iostream>
#include <Compiler_Parser.H>
#include <Compiler_Typed_Sema.H>

using namespace Aleph;

int main()
{
  Source_Manager sm;
  const auto file_id = sm.add_virtual_file("demo.aw", "fn inc(x) { return x + 1; }\n");

  Diagnostic_Engine dx(sm);
  Compiler_Ast_Context ctx(1 << 15);
  Compiler_Parser parser(ctx, sm, file_id, &dx);
  const auto * module = parser.parse_module();
  Compiler_Typed_Semantic_Analyzer typed(&dx);
  typed.analyze_module(module);

  std::cout << compiler_dump_module(module);
  std::cout << '\n' << typed.dump_inference();

  if (dx.size() > 0)
    dx.render_plain(std::cout);
}
```

## What The Current Front-End Covers

- Identifiers and reserved keywords such as `fn`, `let`, `if`, `while`, and `return`
- Integer literals
- String and character literals
- Punctuation and common operators, including `==`, `!=`, `&&`, `||`, `+=`, `->`
- Line comments `// ...`
- Block comments `/* ... */`, which can be preserved as tokens or disabled
- Function declarations with parameter lists
- Blocks, `let`, `return`, `if`, `while`, `break`, `continue`, and expression statements
- Calls, prefix unary operators, grouping, and precedence-climbing binary expressions
- Nested lexical scopes and name lookup
- Duplicate-declaration checks, unresolved-name checks, and optional shadowing diagnostics
- Basic control-flow misuse diagnostics for `return`, `break`, and `continue`
- Rejection of calls to non-function symbols in the current name-only model
- Standalone compiler types for built-ins, tuples, functions, and type variables
- Equality-constraint solving with substitutions, rigid variables, and occurs-checks
- Monomorphic typed semantic analysis for literals, locals, parameters, returns, conditions, operators, and calls

Malformed input produces an `Invalid` token. If a `Diagnostic_Engine` is
attached, the lexer also records a structured error with a stable code such as
`LEX001` or `LEX004`. Parser errors similarly produce invalid AST nodes and
diagnostics such as `PAR001` and `PAR005`. The semantic pass adds codes such as
`SEM001` for duplicates and `SEM002` for unresolved identifiers. The type
unifier reports structured outcomes such as `Occurs_Check_Failed` and
`Rigid_Variable` without emitting diagnostics on its own. The typed semantic
pass maps typing failures to codes such as `TYP002`, `TYP005`, and `TYP006`.

## Examples And Tests

- Example: `Examples/compiler_diagnostics_example.cc`
- Example: `Examples/compiler_lexer_example.cc`
- Example: `Examples/compiler_parser_example.cc`
- Example: `Examples/compiler_sema_example.cc`
- Example: `Examples/compiler_types_example.cc`
- Example: `Examples/compiler_typed_sema_example.cc`
- Tests: `Tests/compiler_source_test.cc`
- Tests: `Tests/compiler_diagnostics_test.cc`
- Tests: `Tests/compiler_token_test.cc`
- Tests: `Tests/compiler_lexer_test.cc`
- Tests: `Tests/compiler_ast_test.cc`
- Tests: `Tests/compiler_parser_test.cc`
- Tests: `Tests/compiler_scope_test.cc`
- Tests: `Tests/compiler_sema_test.cc`
- Tests: `Tests/compiler_types_test.cc`
- Tests: `Tests/compiler_constraints_test.cc`
- Tests: `Tests/compiler_typed_sema_test.cc`

Build the affected targets with:

```bash
cmake -S . -B build -G Ninja -DBUILD_EXAMPLES=ON
cmake --build build --target \
  compiler_source_test \
  compiler_diagnostics_test \
  compiler_token_test \
  compiler_lexer_test \
  compiler_ast_test \
  compiler_parser_test \
  compiler_scope_test \
  compiler_sema_test \
  compiler_types_test \
  compiler_constraints_test \
  compiler_typed_sema_test \
  compiler_diagnostics_example \
  compiler_lexer_example \
  compiler_parser_example \
  compiler_sema_example \
  compiler_types_example \
  compiler_typed_sema_example
```

## Current Limits

- Source locations use UTF-8 byte offsets, not grapheme-aware columns
- Snippets are clipped to one line for deterministic plain-text diagnostics
- Numeric literals currently cover integers only
- The original `Compiler_Sema.H` pass is still name-only; typing is implemented in `Compiler_Typed_Sema.H`
- Type inference is monomorphic and local; there is no let-generalization or polymorphic instantiation
- There is no explicit type-annotation syntax in the parser or AST yet
- Calls are only validated against named function symbols, not inferred callable types
- There is no overload resolution, coercion, trait solving, path-sensitive return analysis, or dataflow analysis yet

That is deliberate: this layer is meant to be the stable foundation for future
compiler modules, not a half-finished fully typed front-end.

## Suggested Next Steps

Once this layer is in place, the natural next modules are:

1. Parser and AST support for explicit type annotations
2. Polymorphic generalization and instantiation over `Compiler_Types.H`
3. `Compiler_CFG.H`
4. `Compiler_IR.H` / `SSA.H`
