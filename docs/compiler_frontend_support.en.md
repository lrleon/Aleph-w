# Compiler Front-End Support

This guide documents the first compiler-oriented support layer added to Aleph-w.
It is intentionally small and reusable: the goal is to make lexers, parsers and
semantic analyzers easier to build on top of the library without committing yet
to one concrete language design. The live roadmap for the broader platform is
available at [docs/compiler_platform_roadmap.en.md](compiler_platform_roadmap.en.md).

## Scope

The current component is formed by forty-three public headers:

| Header | Responsibility |
|---|---|
| `ah-source.H` | Source file registry, byte offsets, spans, line/column resolution, and single-line snippets |
| `ah-diagnostics.H` | Buffered diagnostics with primary/secondary labels, notes, help messages, and deterministic plain-text rendering |
| `Compiler_Token.H` | Token kinds, token objects, keyword classification, and binary-operator metadata for future parsers |
| `Compiler_Lexer.H` | Single-file lexer with stable spans, comment handling options, invalid-token recovery, and optional diagnostic emission |
| `Compiler_Parser_Utils.H` | Reusable token stream with lookahead, `expect`, delimited-list helpers, and basic synchronization for recursive-descent parsers |
| `Compiler_AST.H` | Arena-backed AST nodes for expressions, statements, imports, type declarations, functions, modules, and deterministic tree dumping |
| `Compiler_Parser.H` | Recursive-descent parser with precedence climbing, explicit type annotations, top-level declarations (`import`/`fn`/`type`/`struct`/`enum`), block/statement parsing, and diagnostic-based recovery |
| `Compiler_Operator.H` | Reusable operator vocabulary shared by HIR, IR, bytecode, and backends, with compatibility helpers for the MVP lexer |
| `Compiler_Driver_Contracts.H` | Shared action, input, artifact, and execution-result contracts for reusable drivers and front-ends |
| `Compiler_Module_Resolver.H` | Reusable import resolver by source name with dependency-first ordering, cycle/missing-dependency reporting, and stable textual artifacts |
| `Compiler_Module_Metadata.H` | Reusable module-surface metadata with visible top-level exports and deterministic rendering |
| `Compiler_Module_Linker.H` | Reusable linker for module surfaces that computes imported-name visibility and early name conflicts |
| `Compiler_Module_Binder.H` | Reusable binder for top-level names visible per module, with deterministic bindings and ambiguity filtering |
| `Compiler_Module_Name_Table.H` | Reusable per-module cache of bindable top-level names for repeated semantic queries |
| `Compiler_Module_Name_Resolver.H` | Reusable top-level name resolution over `modules.binding`, with structured outcomes for missing, ambiguous, or wrong-kind lookups |
| `Compiler_Module_Name_Diagnostics.H` | Reusable diagnostics over inter-module name-resolution results, with stable wording, notes, and help entries |
| `Compiler_Module_Semantic_Environment.H` | Reusable per-module semantic environment with separated value and type namespaces |
| `Compiler_Frontend.H` | Reusable interface for language front-ends integrating with the common pipeline |
| `Compiler_MVP_Frontend.H` | Adapter exposing the current MVP language front-end as a reference `Compiler_Frontend` implementation |
| `Compiler_Generic_Driver.H` | Reusable driver decoupled from the MVP parser, able to orchestrate any `Compiler_Frontend` that produces HIR or IR |
| `Compiler_Line_Frontend.H` | Second validation front-end with its own syntax and direct lowering into reusable HIR/IR |
| `tpl_scope.H` | Generic nested-scope stack for lexical bindings and shadowing-aware lookup |
| `Compiler_Symbol_Bindings.H` | Reusable lexical bindings with stable ids, duplicate/shadowing reporting, and deterministic dumps |
| `Compiler_Sema.H` | Name resolution, basic top-level import validation, duplicate detection, shadowing checks, and control-flow misuse diagnostics |
| `Compiler_Types.H` | Stable type graph with built-ins, tuples, functions, nominal `struct`/`enum` types, type variables, and deterministic pretty-printing |
| `tpl_constraints.H` | Equality constraint sets, substitutions, structural unification, rigid variables, and occurs-check support |
| `Compiler_Typed_Sema.H` | Typed semantic pass with explicit annotations, nominal declaration resolution, transparent aliases, `let` generalization, basic polymorphic instantiation, and constraint solving |
| `Compiler_HIR_Model.H` | Reusable typed HIR model with arena ownership and deterministic dumps, independent from the MVP front-end |
| `Compiler_HIR_Builder.H` | Reusable builder for constructing HIR directly from an external front-end or from tests |
| `Compiler_HIR_Lowering_MVP.H` | Lowering from the current typed MVP AST into reusable HIR |
| `Compiler_HIR.H` | Compatibility umbrella that exposes reusable HIR plus MVP lowering |
| `Interpreter_Runtime.H` | Reusable runtime for evaluating HIR with dynamic values, nested environments, a call stack, host functions, and structured errors |
| `Compiler_CFG.H` | Reusable basic-block CFG with lowering from HIR, explicit terminators, structural validation, and deterministic dumps |
| `Compiler_IR_Model.H` | Reusable IR with explicit values, instructions, local/global slots, structural validation, and deterministic dumps |
| `Compiler_IR_Builder.H` | Reusable builder for constructing explicit IR while preserving spans, edges, and predecessor relations |
| `Compiler_IR_Lowering_MVP.H` | Lowering from the MVP HIR into reusable IR |
| `Compiler_IR.H` | Compatibility umbrella that exposes reusable IR plus MVP lowering |
| `Compiler_Dataflow.H` | Reusable IR dataflow layer with reachability, liveness, definite assignment, constant propagation, and conservative DCE |
| `SSA.H` | Reusable SSA layer over IR with dominators, dominance frontiers, phi placement, renaming, and structural verification |
| `Bytecode.H` | Reusable VM-oriented bytecode with per-function constant pools, explicit opcodes, patched PCs, and lowering from IR |
| `Bytecode_Interpreter.H` | Portable VM for executing register bytecode with explicit frames, globals, structured runtime errors, and optional host-function support |
| `Compiler_Backend_C.H` | Portable backend that emits readable C from explicit IR using `goto`-based blocks, a small tagged runtime, and deterministic symbols |
| `Compiler_Driver.H` | Reusable multi-file driver that resolves declarative imports by source name, orders modules by dependency, orchestrates parse/sema/HIR/IR/`run`/`emit-c`/`emit-bytecode`, and publishes reproducible artifacts |

## Quick Start

```cpp
#include <iostream>
#include <Compiler_Parser.H>
#include <Compiler_Typed_Sema.H>

using namespace Aleph;

int main()
{
  Source_Manager sm;
  const auto file_id = sm.add_virtual_file(
    "demo.aw",
    "type UserId = Int;\n"
    "fn id(x: T) -> T { return x; }\n"
    "let alias = id;\n"
    "let value: UserId = alias(1);\n");

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
- Top-level imports such as `import "name.aw";` with explicit source names
- Function declarations with parameter lists and optional return annotations
- Top-level type declarations with `type`, `struct`, and `enum`
- Blocks, `let`, `return`, `if`, `while`, `break`, `continue`, and expression statements
- Calls, prefix unary operators, grouping, and precedence-climbing binary expressions
- Explicit type annotations on parameters, returns, `let`, and function types such as `fn(...) -> ...`
- Nested lexical scopes and name lookup
- Duplicate-declaration checks, unresolved-name checks, and optional shadowing diagnostics
- Basic top-level import validation, including per-module duplicate imports and self-imports
- Basic control-flow misuse diagnostics for `return`, `break`, and `continue`
- Standalone compiler types for built-ins, tuples, functions, nominal types, and type variables
- Equality-constraint solving with substitutions, rigid variables, and occurs-checks
- Typed semantic analysis with annotation resolution, nominal declarations (`struct`, `enum`, transparent aliases), `let` generalization, and basic polymorphic instantiation for literals, locals, parameters, returns, conditions, operators, and calls
- Lowering from typed AST to a structured reusable HIR suitable for interpreters or future CFG/MIR stages
- Structured HIR evaluation with runtime values, lexical bindings, HIR calls, and host functions
- Lowering from structured HIR to CFG with basic blocks, explicit branches, joins, loops, and invariant validation
- Lowering from typed HIR to a reusable IR with explicit values, loads/stores, calls, and per-block terminators
- IR dataflow analysis with reachability, local-slot liveness, definite assignment, constant propagation, and conservative dead-code elimination
- SSA construction over explicit IR with dominators, dominance frontiers, per-slot phi placement, and deterministic renaming
- Lowering from explicit IR into reusable register bytecode with constant pools, VM opcodes, and concrete PCs per block
- Direct execution of the reusable bytecode with a portable VM, per-call frames, explicit globals, and structured runtime results
- Emission of portable self-contained C from explicit IR with an embedded runtime, reusable functions, and optional `main()` generation for `emit-c` workflows
- End-to-end multi-file driver workflows for `parse-only`, `sema-only`, `hir`, `ir`, `run`, `emit-c`, and `emit-bytecode`, plus declarative source-name imports, stable topological ordering, and reproducible intermediate artifacts per stage
- Manual construction of reusable HIR and IR via public builders, useful for alternate front-ends, transpilers, or structural tests without a parser
- Validation with a second non-MVP front-end (`Compiler_Line_Frontend.H`) that uses its own parser and integrates through `Compiler_Generic_Driver.H`
- Reusable token-oriented parser helpers through `Compiler_Parser_Utils.H`, useful for other recursive-descent parsers without a shared AST
- Reusable lexical bindings through `Compiler_Symbol_Bindings.H`, useful for basic name semantics without depending on the MVP AST
- Reusable module/import resolution through `Compiler_Module_Resolver.H`, useful for multi-file front-ends without depending on `Compiler_Driver.H`
- Reusable module-surface metadata through `Compiler_Module_Metadata.H`, useful for tooling, export inspection, and public API views
- Reusable module-surface linking through `Compiler_Module_Linker.H`, useful for direct-import visibility and early conflict detection
- Reusable top-level binding across modules through `Compiler_Module_Binder.H`, useful for tooling that needs to inspect what names remain bindable after linkage
- Reusable per-module top-level name caches through `Compiler_Module_Name_Table.H`, useful for front-ends with many repeated semantic lookups
- Reusable top-level name resolution across modules through `Compiler_Module_Name_Resolver.H`, useful for semantic linking over the already-bound module view
- Reusable diagnostics for inter-module name-resolution failures through `Compiler_Module_Name_Diagnostics.H`, useful for stable wording, notes, and help messages
- Reusable per-module semantic environments through `Compiler_Module_Semantic_Environment.H`, useful for consuming already-separated value and type namespaces

## Reusable Extension Point

At this point Aleph-w distinguishes between:

- the MVP reference front-end, implemented by `Compiler_Parser.H`,
  `Compiler_AST.H`, `Compiler_Sema.H`, `Compiler_Typed_Sema.H`, and adapted by
  `Compiler_MVP_Frontend.H`
- the reusable infrastructure, exposed by `Compiler_Frontend.H`,
  `Compiler_Generic_Driver.H`, `Compiler_HIR_Model.H`, reusable IR, bytecode,
  runtime, and backends

That makes two practical strategies possible for a new language:

- reuse `ah-source.H`, `ah-diagnostics.H`, `Compiler_Parser_Utils.H`,
  `Compiler_Symbol_Bindings.H`, `Compiler_Module_Resolver.H`,
  `Compiler_Module_Metadata.H`, `Compiler_Module_Linker.H`,
  `Compiler_Module_Binder.H`, `Compiler_Module_Name_Table.H`,
  `Compiler_Module_Name_Resolver.H`, `Compiler_Module_Name_Diagnostics.H`,
  `Compiler_Module_Semantic_Environment.H`, scopes, and constraints while
  keeping your own lexer/parser/AST/sema pipeline
- reuse the MVP front-end as a reference implementation and replace or extend
  pieces incrementally

The minimum contract for the common pipeline is now a valid
`Compiler_HIR_Module` or `Compiler_IR_Module` produced through
`Compiler_Frontend`.

For multi-file front-ends, `Compiler_Frontend` also exposes
`module_descriptors()`, `module_merge_order()`, and `module_metadata()`. That
lets reusable drivers and tools inspect imports, merge ordering, and top-level
module surface without depending on front-end-specific artifacts.

`Compiler_HIR.H` is now a compatibility umbrella. The stable reusable entry
point is `Compiler_HIR_Model.H`, while the MVP-specific lowering lives in
`Compiler_HIR_Lowering_MVP.H`.

Likewise, `Compiler_IR.H` is a compatibility umbrella. The stable reusable
entry point is `Compiler_IR_Model.H`, while the MVP-specific lowering lives in
`Compiler_IR_Lowering_MVP.H`.

For front-ends that do not want to pass through the MVP lowering, Aleph-w now
provides `Compiler_HIR_Builder.H` and `Compiler_IR_Builder.H`. Those builders
construct modules directly on top of `Compiler_HIR_Context` and
`Compiler_IR_Context` while preserving aggregate spans, container membership,
and, in IR, explicit control-flow relations between blocks.

As a concrete validation of this extension point, `Compiler_Line_Frontend.H`
implements a minimal line-oriented language with its own parser, semantic
analysis, and direct lowering into HIR/IR. See also the focused quickstart:
[docs/compiler_frontend_sdk_quickstart.en.md](compiler_frontend_sdk_quickstart.en.md).

## Current Surface Grammar

The grammar currently implemented by [Compiler_Parser.H](../Compiler_Parser.H)
is not defined in a separate formal EBNF file; the canonical definition is the
recursive-descent parser itself. Still, the accepted language can be summarized
faithfully with the following pseudo-EBNF:

```ebnf
module            := top_level*

top_level         := import_decl
                   | type_alias_decl
                   | struct_decl
                   | enum_decl
                   | function_decl
                   | statement

import_decl       := "import" string_literal ";"

type_alias_decl   := "type" IDENT "=" type_expr ";"

struct_decl       := "struct" IDENT "{" struct_field* "}"
struct_field      := IDENT ":" type_expr ";"

enum_decl         := "enum" IDENT "{" enum_variant ("," enum_variant)* [","] "}"
enum_variant      := IDENT

function_decl     := "fn" IDENT "(" [param ("," param)*] ")" ["->" type_expr] block
param             := IDENT [":" type_expr]

statement         := block
                   | let_stmt
                   | return_stmt
                   | if_stmt
                   | while_stmt
                   | break_stmt
                   | continue_stmt
                   | expr_stmt

block             := "{" statement* "}"

let_stmt          := "let" IDENT [":" type_expr] ["=" expr] ";"
return_stmt       := "return" [expr] ";"
if_stmt           := "if" expr statement ["else" statement]
while_stmt        := "while" expr statement
break_stmt        := "break" ";"
continue_stmt     := "continue" ";"
expr_stmt         := expr ";"

expr              := unary_expr (binop expr)*
unary_expr        := ("!" | "-" | "+" | "~") unary_expr
                   | postfix_expr
postfix_expr      := primary_expr ("(" [expr ("," expr)*] ")")*
primary_expr      := IDENT
                   | integer_literal
                   | string_literal
                   | char_literal
                   | "true"
                   | "false"
                   | "(" expr ")"

type_expr         := IDENT
                   | "(" type_expr [("," type_expr)+] ")"
                   | "fn" "(" [type_expr ("," type_expr)*] ")" "->" type_expr
```

Reading notes:

- `top_level := statement` only applies when `Compiler_Parser_Options::allow_top_level_statements`
  is enabled.
- `expr` uses precedence climbing, so the compact rule `unary_expr (binop expr)*`
  stands for a real precedence hierarchy defined in `Compiler_Token.H`.
- Current `enum` declarations support unit variants only.
- Current `struct` declarations define nominal types and field metadata, but
  there is still no value construction or field access in expressions.
- `type_expr` currently supports named types, tuples, and function types of the
  form `fn(...) -> ...`.

Malformed input produces an `Invalid` token. If a `Diagnostic_Engine` is
attached, the lexer also records a structured error with a stable code such as
`LEX001` or `LEX004`. Parser errors similarly produce invalid AST nodes and
diagnostics such as `PAR001` and `PAR005`. The semantic pass adds codes such as
`SEM001` for duplicates and `SEM002` for unresolved identifiers. The type
unifier reports structured outcomes such as `Occurs_Check_Failed` and
`Rigid_Variable` without emitting diagnostics on its own. The typed semantic
pass maps typing failures to codes such as `TYP002`, `TYP005`, `TYP006`,
`TYP007`, `TYP009`, `TYP010`, and `TYP011`. Base import validation uses codes
such as `SEM009` and `SEM010`, while the driver adds `DRV001`/`DRV002` for
missing or cyclic imports.

## Examples And Tests

- Example: `Examples/compiler_diagnostics_example.cc`
- Example: `Examples/compiler_lexer_example.cc`
- Example: `Examples/compiler_parser_example.cc`
- Example: `Examples/compiler_sema_example.cc`
- Example: `Examples/compiler_types_example.cc`
- Example: `Examples/compiler_typed_sema_example.cc`
- Example: `Examples/compiler_hir_example.cc`
- Example: `Examples/compiler_builder_example.cc`
- Example: `Examples/compiler_line_frontend_example.cc`
- Example: `Examples/interpreter_runtime_example.cc`
- Example: `Examples/compiler_cfg_example.cc`
- Example: `Examples/compiler_ir_example.cc`
- Example: `Examples/compiler_dataflow_example.cc`
- Example: `Examples/ssa_example.cc`
- Example: `Examples/bytecode_example.cc`
- Example: `Examples/bytecode_interpreter_example.cc`
- Example: `Examples/compiler_backend_c_example.cc`
- Example: `Examples/compiler_driver_example.cc`
- Tests: `Tests/compiler_source_test.cc`
- Tests: `Tests/compiler_diagnostics_test.cc`
- Tests: `Tests/compiler_token_test.cc`
- Tests: `Tests/compiler_lexer_test.cc`
- Tests: `Tests/compiler_ast_test.cc`
- Tests: `Tests/compiler_parser_test.cc`
- Tests: `Tests/compiler_parser_utils_test.cc`
- Tests: `Tests/compiler_symbol_bindings_test.cc`
- Tests: `Tests/compiler_module_resolver_test.cc`
- Tests: `Tests/compiler_module_metadata_test.cc`
- Tests: `Tests/compiler_module_linker_test.cc`
- Tests: `Tests/compiler_module_binder_test.cc`
- Tests: `Tests/compiler_module_name_table_test.cc`
- Tests: `Tests/compiler_module_name_resolver_test.cc`
- Tests: `Tests/compiler_module_name_diagnostics_test.cc`
- Tests: `Tests/compiler_module_semantic_environment_test.cc`
- Tests: `Tests/compiler_scope_test.cc`
- Tests: `Tests/compiler_sema_test.cc`
- Tests: `Tests/compiler_types_test.cc`
- Tests: `Tests/compiler_constraints_test.cc`
- Tests: `Tests/compiler_typed_sema_test.cc`
- Tests: `Tests/compiler_hir_test.cc`
- Tests: `Tests/compiler_builder_test.cc`
- Tests: `Tests/compiler_line_frontend_test.cc`
- Tests: `Tests/interpreter_runtime_test.cc`
- Tests: `Tests/compiler_cfg_test.cc`
- Tests: `Tests/compiler_ir_test.cc`
- Tests: `Tests/compiler_dataflow_test.cc`
- Tests: `Tests/ssa_test.cc`
- Tests: `Tests/bytecode_test.cc`
- Tests: `Tests/bytecode_interpreter_test.cc`
- Tests: `Tests/compiler_backend_c_test.cc`
- Tests: `Tests/compiler_driver_test.cc`

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
  compiler_parser_utils_test \
  compiler_symbol_bindings_test \
  compiler_module_resolver_test \
  compiler_module_metadata_test \
  compiler_module_linker_test \
  compiler_module_binder_test \
  compiler_module_name_table_test \
  compiler_module_name_resolver_test \
  compiler_module_name_diagnostics_test \
  compiler_module_semantic_environment_test \
  compiler_scope_test \
  compiler_sema_test \
  compiler_types_test \
  compiler_constraints_test \
  compiler_typed_sema_test \
  compiler_hir_test \
  compiler_builder_test \
  compiler_line_frontend_test \
  interpreter_runtime_test \
  compiler_cfg_test \
  compiler_ir_test \
  compiler_dataflow_test \
  ssa_test \
  bytecode_test \
  bytecode_interpreter_test \
  compiler_backend_c_test \
  compiler_driver_test \
  compiler_diagnostics_example \
  compiler_lexer_example \
  compiler_parser_example \
  compiler_sema_example \
  compiler_types_example \
  compiler_typed_sema_example \
  compiler_hir_example \
  compiler_builder_example \
  compiler_line_frontend_example \
  interpreter_runtime_example \
  compiler_cfg_example \
  compiler_ir_example \
  compiler_dataflow_example \
  ssa_example \
  bytecode_example \
  bytecode_interpreter_example \
  compiler_backend_c_example \
  compiler_driver_example
```

## Current Limits

- Source locations use UTF-8 byte offsets, not grapheme-aware columns
- Snippets are clipped to one line for deterministic plain-text diagnostics
- Numeric literals currently cover integers only
- The original `Compiler_Sema.H` pass is still name-only; actual typing and callability live in `Compiler_Typed_Sema.H`
- Polymorphism is still intentionally basic: explicit annotation variables plus `let` generalization over the current type graph
- The current nominal types are declaration-only and opaque: there is still no value construction, field access, enum payloads, or pattern matching
- The current imports are declarative and global: they resolve by exact source name, reorder modules in the driver, and still have no `export`, namespaces, or selective visibility
- Structured HIR, a reusable runtime, CFG, explicit IR, dataflow, SSA, bytecode, a bytecode interpreter, a portable C backend, and a reusable multi-file driver now exist, but there is still no LLVM/native backend or full module system
- There is no overload resolution, coercion, trait solving, or path-sensitive return analysis yet

That is deliberate: this layer is meant to be the stable foundation for future
compiler modules, not a half-finished fully typed front-end.

## Suggested Next Steps

Once this layer is in place, the natural next modules are:

1. A CLI or tool wrapper on top of `Compiler_Driver.H`
2. `Compiler_Backend_C.H` extensions toward LLVM IR or native code
3. Construction and runtime use of nominal types across HIR/IR/runtime
4. `export`, namespaces, or selective visibility on top of the current import model
5. Explicit types and polymorphism beyond the current MVP
