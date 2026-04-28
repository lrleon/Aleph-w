# Compiler and Interpreter Future Work

This document tracks likely future work for Aleph-w's compiler and interpreter
infrastructure now that the reusable front-end SDK baseline is in place.

It is intentionally forward-looking. Unlike
[docs/compiler_platform_roadmap.en.md](compiler_platform_roadmap.en.md), which
tracks implemented milestones, this file focuses on what still looks valuable,
what would improve the SDK for third parties, and what should be prioritized
next depending on product goals.

## Current Baseline

The platform now has a usable reusable boundary for external languages:

- source, spans, and diagnostics
- reusable parser helpers
- reusable lexical bindings
- reusable module/import resolution
- reusable module surface, linkage, binding, name tables, and semantic environments
- reusable HIR and IR models
- reusable HIR and IR builders
- reusable generic driver contracts
- reusable runtime, CFG, dataflow, SSA, bytecode, and C backend
- MVP front-end adapter plus one alternate front-end that does not reuse the MVP parser or AST
- installed-SDK validation for public compiler headers

That means future work should not try to rebuild the same abstraction layer
again. The next phase should focus on capability depth, ergonomics, and
platform maturity.

## Priority Bands

## P0: High-Value Work

These items would materially improve the usefulness of the SDK for real users.

### 1. Stronger Type-System Surface

- Add additional composite types: arrays, records, slices, option/result if needed
- Decide whether algebraic data types should be modeled in the reusable type layer or remain front-end-specific until lowered
- Expand typed semantic support for aggregate literals, field access, and destructuring
- Define which type features belong in reusable HIR/IR versus front-end-only syntax

Why this matters:
The current SDK is structurally reusable, but a front-end for a richer language
will still hit type-system limits quickly.

### 2. Public/Private Module Semantics

- Add reusable visibility metadata for exported vs internal symbols
- Support explicit re-exports
- Support qualified imports and aliasing policies
- Define stable rules for duplicate imported names and shadowing across modules
- Decide whether module metadata should carry visibility and re-export provenance

Why this matters:
The current module tooling is good for direct imports, but real languages often
need richer namespace control.

### 3. Better External Front-End Ergonomics

- Add a third validation front-end with syntax further away from the MVP language
- Provide a minimal template for implementing `Compiler_Frontend`
- Add one example that goes directly from a custom AST to HIR
- Add one example that skips HIR and lowers directly to IR
- Consider a tiny helper layer for front-end artifact emission and boilerplate reduction

Why this matters:
The SDK is technically reusable today, but onboarding will still be easier if
there are more examples and less adapter boilerplate.

### 4. Stable Installed-SDK Story

- Keep the install-header closure audit in CI
- Add an installed-SDK link smoke test, not just a compile smoke test
- Add a smoke test for a consumer CMake project using `find_package()` once packaging is formalized
- Revisit whether compiler-related headers should be grouped into a distinct install component

Why this matters:
Reusable in-tree is not enough. A defendable SDK needs predictable installation
and consumption outside the repo.

## P1: Pipeline Capability Expansion

These are the next major technical capabilities once the SDK boundary is stable.

### 5. Richer HIR

- Add explicit support for aggregates, field access, indexing, and construction
- Decide how closures and captured environments should be represented
- Model more effectful operations explicitly if needed
- Improve source-span preservation through lowering for better downstream diagnostics

Why this matters:
HIR is currently a good structured midpoint, but richer languages will need more
expressive nodes.

### 6. Richer IR

- Add clearer memory and aggregate operations
- Decide whether typed IR values should remain lightweight or become more explicit
- Add source-location or origin metadata on IR instructions where useful
- Add stronger verifiers for control-flow, dominance, and value invariants

Why this matters:
Backends and optimizations become much easier to extend when the IR is more
expressive and better checked.

### 7. Optimization Pipeline Growth

- SCCP / sparse conditional constant propagation
- copy propagation
- common subexpression elimination if justified
- simple inlining policies
- mem2reg-style improvements where reusable
- alias-analysis groundwork if memory operations grow

Why this matters:
The current pipeline already has useful analysis infrastructure, but optimization
coverage is still intentionally conservative.

### 8. Runtime and VM Growth

- Improve host-function registration and embedding APIs
- Add optional tracing hooks for interpreter and bytecode VM execution
- Add reusable disassembly/dump tooling for bytecode
- Consider debugging hooks: breakpoints, stepping, frame inspection
- Evaluate whether garbage collection or reference management should become a platform concern

Why this matters:
Interpreters and embedders often need observability and host integration more
than they need more syntax.

## P2: Backend and Toolchain Maturity

These items are important, but they make more sense after the front-end SDK and
intermediate layers are fully exercised by more languages.

### 9. Additional Backends

- LLVM IR backend
- object-file or native assembly emission if ever justified
- data-layout and calling-convention formalization
- linker/assembler integration where relevant

Why this matters:
The current C backend is a strong portable baseline, but native-code workflows
will eventually need a lower-level target.

### 10. Incremental and Tooling-Oriented Driver Modes

- incremental module rebuilds
- persistent driver caches
- symbol indexing
- reusable semantic database or module cache
- front-end hooks for editor tooling, linting, and static analysis

Why this matters:
Once the SDK is used by real projects, compile latency and tooling workflows
become important.

### 11. Artifact Formalization

- Define a stable schema for text artifacts such as `modules.surface`, `modules.linkage`, `modules.binding`, `modules.names`, and `modules.semantic`
- Decide which artifacts are debug-only and which are part of the supported SDK contract
- Consider machine-readable artifact variants if external tooling needs them

Why this matters:
The generic driver already emits useful artifacts, but they should become more
intentional if third-party tooling starts depending on them.

## P3: Quality, Testing, and Packaging

### 12. Broader Validation Matrix

- GCC and Clang coverage specifically for the compiler SDK headers
- separate install-smoke coverage for examples that consume the SDK
- more end-to-end tests that use alternate front-ends with modules
- regression tests for documentation snippets or quickstart examples where practical

### 13. Packaging and Discoverability

- decide whether to ship a generated package config for consumers
- improve README-level discoverability of compiler SDK features
- keep English and Spanish documentation synchronized where bilingual docs are maintained
- add a clear “start here” route from top-level docs to quickstart, support guide, and roadmap

### 14. Documentation Depth

- add a dedicated guide for writing a custom `Compiler_Frontend`
- add a dedicated guide for lowering custom ASTs to HIR
- add a dedicated guide for using the module/linkage/name-resolution helpers
- document invariants for HIR and IR builders more explicitly

## Questions Worth Deciding Explicitly

These are design questions that should be answered before large new features are
implemented:

- What is the long-term stable reusable boundary: HIR, IR, or both?
- Which type features should be reusable infrastructure, and which should remain language-local?
- Should the SDK aim to support richer front-ends directly, or primarily provide lowering targets?
- Are driver artifacts part of the supported public contract?
- Should interpreter and VM APIs optimize for embedding, debugging, or raw simplicity?
- When native backends arrive, should they be first-class or optional add-ons?

## Recommended Next Steps

If the goal is to improve the SDK for outside users first, the next sequence
should be:

1. Add a third non-MVP validation front-end
2. Add installed-SDK link validation
3. Add richer module visibility/re-export semantics
4. Add a dedicated guide for implementing `Compiler_Frontend`
5. Expand HIR/IR only after more front-end pressure reveals the right shape

If the goal is to improve execution capability first, the next sequence should
be:

1. Expand HIR for aggregates and closures
2. Expand IR for richer memory/aggregate operations
3. Add more optimization passes
4. Improve VM observability and embedding hooks
5. Only then consider LLVM or native-code work

## Non-Goals For Now

The following should probably remain out of scope until the SDK is exercised by
more than one alternate language:

- a fully generic universal AST
- speculative native backend work before IR needs are clearer
- complex package-management integration before the installed-SDK story settles
- over-abstracting front-end semantics that still vary significantly by language
