# Compiler Front-End SDK Quickstart

This guide is for users who want to bring a new language front-end to
Aleph-w without reusing the MVP parser or AST.

The short version is:

- keep your own lexer, parser, AST, and language-specific semantic rules
- reuse Aleph-w where the abstractions are now stable
- lower into reusable HIR or reusable IR
- run the common pipeline through `Compiler_Generic_Driver.H`

For the broader component inventory, see
[docs/compiler_frontend_support.en.md](compiler_frontend_support.en.md). For the
longer-term status of the platform, see
[docs/compiler_platform_roadmap.en.md](compiler_platform_roadmap.en.md).

## Choose Your Integration Depth

Aleph-w now supports three practical entry points:

1. Parser utilities only
   Reuse `Compiler_Lexer.H`, `Compiler_Parser_Utils.H`, diagnostics, spans, and
   lexical bindings, but keep your own AST and semantic pipeline.
2. Front-end infrastructure plus HIR/IR builders
   Reuse parser utilities, bindings, module helpers, and build HIR/IR directly
   with `Compiler_HIR_Builder.H` or `Compiler_IR_Builder.H`.
3. Full reusable pipeline
   Implement `Compiler_Frontend.H`, expose module metadata/import order, and let
   `Compiler_Generic_Driver.H` orchestrate parse-only, sema-only, HIR, IR,
   bytecode, `run`, and `emit-c`.

If your language is substantially different from the MVP language, option 2 or
3 is the intended path.

## Reusable Building Blocks

These are the main layers to reuse when building a new language:

- `ah-source.H`: source registry, spans, snippets, line/column mapping
- `ah-diagnostics.H`: structured diagnostics and deterministic rendering
- `Compiler_Parser_Utils.H`: token-oriented stream helpers for recursive-descent
- `Compiler_Symbol_Bindings.H`: lexical name binding and shadowing-aware lookup
- `Compiler_Module_Resolver.H`: import graph resolution and merge order
- `Compiler_Module_Metadata.H`: public module surface representation
- `Compiler_Module_Linker.H`: imported-name visibility from module surfaces
- `Compiler_Module_Binder.H`: bindable top-level names after linkage
- `Compiler_Module_Name_Table.H`: cached per-module name lookups
- `Compiler_Module_Name_Resolver.H`: structured cross-module name resolution
- `Compiler_Module_Name_Diagnostics.H`: stable diagnostics for name-resolution failures
- `Compiler_Module_Semantic_Environment.H`: per-module value/type namespaces
- `Compiler_HIR_Builder.H`: direct construction of reusable HIR
- `Compiler_IR_Builder.H`: direct construction of reusable IR
- `Compiler_Generic_Driver.H`: common orchestration and reproducible artifacts

## Minimal Shape Of A New Front-End

At a high level, the implementation pattern is:

```text
your lexer/parser/AST
  -> your semantic analysis
  -> HIR or IR via Aleph builders
  -> Compiler_Frontend adapter
  -> Compiler_Generic_Driver
```

The adapter is intentionally small:

- parse your sources
- analyze your modules
- expose `module_descriptors()` and `module_merge_order()` if you support imports
- expose `module_metadata()` if you want reusable surface inspection
- return a valid `Compiler_HIR_Module` or `Compiler_IR_Module`

The MVP front-end in `Compiler_MVP_Frontend.H` is one reference implementation.
`Compiler_Line_Frontend.H` is the more important validation sample because it
does not reuse the MVP parser or AST.

## Recommended Starting Points In This Repo

Use these files as concrete entry points:

- `Compiler_Frontend.H`
- `Compiler_Generic_Driver.H`
- `Compiler_Line_Frontend.H`
- `Compiler_HIR_Builder.H`
- `Compiler_IR_Builder.H`
- `Examples/compiler_builder_example.cc`
- `Examples/compiler_line_frontend_example.cc`
- `Tests/compiler_builder_test.cc`
- `Tests/compiler_line_frontend_test.cc`

If you need reusable module tooling, also inspect:

- `Examples/compiler_module_resolver_example.cc`
- `Examples/compiler_module_linker_example.cc`
- `Examples/compiler_module_binder_example.cc`
- `Examples/compiler_module_name_table_example.cc`
- `Examples/compiler_module_name_resolver_example.cc`
- `Examples/compiler_module_name_diagnostics_example.cc`
- `Examples/compiler_module_semantic_environment_example.cc`

## Installed SDK Validation

Aleph-w now validates the installed compiler SDK in two ways:

- `CompilerSdk.InstallHeaderClosure`
  Audits `HLIST` so installed public headers do not reference missing local
  headers and do not list nonexistent files.
- `CompilerSdk.InstallHeadersSmoke`
  Installs the library to a temporary prefix and compiles an external-consumer
  smoke translation unit against the installed headers.

The external smoke source is:

- `Tests/compiler_sdk_install_smoke.cc`

The install+compile harness is:

- `Tests/compiler_sdk_install_smoke.cmake`

You can run the same validation manually with:

```bash
cmake -S . -B build -G Ninja
ruby scripts/install_header_closure_check.rb --repo .
cmake -DBUILD_DIR=$PWD/build \
      -DCXX_COMPILER=$(command -v c++) \
      -DSMOKE_SOURCE=$PWD/Tests/compiler_sdk_install_smoke.cc \
      -P $PWD/Tests/compiler_sdk_install_smoke.cmake
```

## Practical Advice

- Do not try to force your language into `Compiler_AST.H` unless it is very
  close to the MVP language.
- Treat HIR or IR as the stable reusable boundary.
- Reuse the module helpers if your language has imports; they remove a lot of
  repeated driver and semantic glue.
- Use the builders rather than constructing HIR/IR nodes by hand.
- Keep your front-end artifacts deterministic; the generic driver already
  assumes reproducible text artifacts are useful for debugging and CI.

## Current Limitation

The compiler SDK is now reusable and installable, but the surrounding project
still has pre-existing test-tree issues unrelated to the SDK itself. In
particular, if a generated `ctest` include recursion shows up in a local build
tree, validate the installable SDK through the direct `cmake -P` smoke harness
above. The installed-header coverage remains the same either way.
