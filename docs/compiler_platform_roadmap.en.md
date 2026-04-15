# Compiler and Interpreter Platform Roadmap

This document tracks the roadmap for Aleph-w's language implementation
infrastructure. The goal is not to build one specific compiler, but a reusable
platform for:

- compilers
- interpreters
- transpilers
- static analyzers
- VMs or bytecode runtimes

When a stage is fully implemented and verified, it should be marked with `[x]`.

## Current status

### 1. Core front-end infrastructure

- [x] Source management and spans (`ah-source.H`)
- [x] Structured diagnostics (`ah-diagnostics.H`)
- [x] Token model (`Compiler_Token.H`)
- [x] Lexer (`Compiler_Lexer.H`)
- [x] AST (`Compiler_AST.H`)
- [x] Parser (`Compiler_Parser.H`)
- [x] Lexical scopes (`tpl_scope.H`)
- [x] Base semantic analysis and control checks (`Compiler_Sema.H`)
- [x] Type graph (`Compiler_Types.H`)
- [x] Constraints and unification (`tpl_constraints.H`)
- [x] Monomorphic typed semantic pass (`Compiler_Typed_Sema.H`)
- [x] Tests and examples for all the above
- [x] Doxygen plus Markdown/README documentation for the current block

### 2. Functional state of the current block

- [x] Literal typing
- [x] Inferred parameter and return typing
- [x] `let` binding typing
- [x] Arithmetic, logical, and comparison operator typing
- [x] `if` and `while` condition typing
- [x] Call typing
- [x] Basic typed diagnostics (`TYP*`)

## Next stages

### 3. Explicit types and polymorphism

- [ ] Type annotations in the parser and AST
- [ ] Resolution of annotations into `Compiler_Types.H`
- [ ] `let` generalization
- [ ] Polymorphic instantiation
- [ ] Nominal types: `struct`, `enum`, aliases
- [ ] Additional composite types: arrays, records, option/result if needed
- [ ] Module/import support in the semantic layer

### 4. Reusable HIR for compilers and interpreters

- [x] `Compiler_HIR.H`
- [x] Typed HIR nodes more stable than the AST
- [x] Lowering from typed AST to HIR
- [x] Clear representation of effects, calls, control, and values
- [x] HIR pretty-printer / dumper
- [x] HIR tests and examples

### 5. Runtime for interpreters

- [ ] `Interpreter_Runtime.H`
- [ ] General runtime `Value`
- [ ] Frames and environments
- [ ] Call stack
- [ ] Closures / host functions
- [ ] Base support for `Unit`, `Bool`, `Int`, `String`, `Char`, tuples
- [ ] Structured runtime errors
- [ ] Evaluator over HIR or typed AST
- [ ] Minimal interpreter example

### 6. General-purpose CFG and IR

- [ ] `Compiler_CFG.H`
- [ ] Per-function CFG builder
- [ ] CFG validators
- [ ] `Compiler_IR.H` or `Compiler_MIR.H`
- [ ] Explicit instructions and values
- [ ] Lowering from HIR to MIR/IR
- [ ] Deterministic textual dumps
- [ ] CFG/IR tests and examples

### 7. Analysis and optimization

- [ ] `Compiler_Dataflow.H`
- [ ] Reachability
- [ ] Liveness
- [ ] Definite assignment / initialization
- [ ] Constant propagation
- [ ] Dead code elimination
- [ ] Per-pass invariant validators

### 8. SSA

- [ ] `SSA.H`
- [ ] Dominators
- [ ] Dominance frontier
- [ ] Phi placement
- [ ] Rename pass
- [ ] SSA verifier
- [ ] SSA tests and example

### 9. Bytecode and efficient interpretation

- [ ] `Bytecode.H`
- [ ] Constants, functions, and opcodes format
- [ ] Lowering from MIR/HIR to bytecode
- [ ] `Bytecode_Interpreter.H`
- [ ] Simple portable VM
- [ ] End-to-end execution tests

### 10. Compilation backends

- [ ] `Compiler_Backend_C.H`
- [ ] LLVM IR backend or equivalent
- [ ] Native assembly backend if ever needed
- [ ] Data layout and calling conventions
- [ ] Assembler/linker integration where appropriate

### 11. Driver and toolchain

- [ ] `Compiler_Driver.H`
- [ ] Stage-configurable pipeline
- [ ] `parse-only`, `sema-only`, `hir`, `ir`, `run`, `emit-c`, `emit-bytecode`
- [ ] Multi-file support
- [ ] Reproducible intermediate artifacts
- [ ] End-to-end integration tests

## Recommended order

If the priority is to support both compilers and interpreters:

1. [ ] Explicit types and basic polymorphism
2. [x] `Compiler_HIR.H`
3. [ ] `Interpreter_Runtime.H`
4. [ ] `Compiler_CFG.H`
5. [ ] `Compiler_IR.H` / `Compiler_MIR.H`
6. [ ] `Compiler_Dataflow.H`
7. [ ] `SSA.H`
8. [ ] `Bytecode.H`
9. [ ] `Bytecode_Interpreter.H`
10. [ ] `Compiler_Backend_C.H`
11. [ ] `Compiler_Driver.H`

## Completion criteria for each stage

A stage should only be marked complete if it includes all of the following:

- [ ] Public API documented with useful Doxygen
- [ ] Unit tests
- [ ] At least one runnable example
- [ ] Updated Markdown documentation
- [ ] README references when appropriate
- [ ] Local build/test verification

## Design note

The target architecture remains:

`source -> tokens -> AST -> sema -> typed sema -> HIR -> interpreter`

and in parallel:

`HIR -> CFG/IR -> dataflow/SSA -> bytecode/backend -> execution`

That keeps the component general and reusable, without tying it to one specific
language or one execution model.
