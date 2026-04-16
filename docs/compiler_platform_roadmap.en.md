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
- [x] Typed semantic pass with annotations and basic polymorphism (`Compiler_Typed_Sema.H`)
- [x] Tests and examples for all the above
- [x] Doxygen plus Markdown/README documentation for the current block

### 2. Functional state of the current block

- [x] Literal typing
- [x] Inferred parameter and return typing
- [x] `let` binding typing
- [x] Explicit annotations on parameters, returns, and `let`
- [x] Resolution of annotations into `Compiler_Types.H`
- [x] Arithmetic, logical, and comparison operator typing
- [x] `if` and `while` condition typing
- [x] Call typing
- [x] `let` generalization
- [x] Basic polymorphic instantiation
- [x] Basic typed diagnostics (`TYP*`)

## Next stages

### 3. Explicit types and polymorphism

- [x] Type annotations in the parser and AST
- [x] Resolution of annotations into `Compiler_Types.H`
- [x] `let` generalization
- [x] Polymorphic instantiation
- [x] Opaque nominal types: `struct`, `enum`, transparent aliases
- [ ] Additional composite types: arrays, records, option/result if needed
- [x] Base support for declarative imports in the semantic layer and driver

### 4. Reusable HIR for compilers and interpreters

- [x] `Compiler_HIR.H`
- [x] Typed HIR nodes more stable than the AST
- [x] Lowering from typed AST to HIR
- [x] Clear representation of effects, calls, control, and values
- [x] HIR pretty-printer / dumper
- [x] HIR tests and examples

### 5. Runtime for interpreters

- [x] `Interpreter_Runtime.H`
- [x] General runtime `Value`
- [x] Frames and environments
- [x] Call stack
- [x] Closures / host functions
- [x] Base support for `Unit`, `Bool`, `Int`, `String`, `Char`, tuples
- [x] Structured runtime errors
- [x] Evaluator over HIR or typed AST
- [x] Minimal interpreter example

### 6. General-purpose CFG and IR

- [x] `Compiler_CFG.H`
- [x] Per-function CFG builder
- [x] CFG validators
- [x] `Compiler_IR.H` or `Compiler_MIR.H`
- [x] Explicit instructions and values
- [x] Lowering from HIR to MIR/IR
- [x] Deterministic CFG textual dumps
- [x] CFG tests and examples
- [x] MIR/IR tests and examples

### 7. Analysis and optimization

- [x] `Compiler_Dataflow.H`
- [x] Reachability
- [x] Liveness
- [x] Definite assignment / initialization
- [x] Constant propagation
- [x] Dead code elimination
- [x] Per-pass invariant validators

### 8. SSA

- [x] `SSA.H`
- [x] Dominators
- [x] Dominance frontier
- [x] Phi placement
- [x] Rename pass
- [x] SSA verifier
- [x] SSA tests and example

### 9. Bytecode and efficient interpretation

- [x] `Bytecode.H`
- [x] Constants, functions, and opcodes format
- [x] Lowering from MIR/HIR to bytecode
- [x] `Bytecode_Interpreter.H`
- [x] Simple portable VM
- [x] End-to-end execution tests

### 10. Compilation backends

- [x] `Compiler_Backend_C.H`
- [ ] LLVM IR backend or equivalent
- [ ] Native assembly backend if ever needed
- [ ] Data layout and calling conventions
- [ ] Assembler/linker integration where appropriate

### 11. Driver and toolchain

- [x] `Compiler_Driver.H`
- [x] Stage-configurable pipeline
- [x] `parse-only`, `sema-only`, `hir`, `ir`, `run`, `emit-c`, `emit-bytecode`
- [x] Multi-file support
- [x] Reproducible intermediate artifacts
- [x] End-to-end integration tests

## Recommended order

If the priority is to support both compilers and interpreters:

1. [x] Explicit types and basic polymorphism
2. [x] `Compiler_HIR.H`
3. [x] `Interpreter_Runtime.H`
4. [x] `Compiler_CFG.H`
5. [x] `Compiler_IR.H` / `Compiler_MIR.H`
6. [x] `Compiler_Dataflow.H`
7. [x] `SSA.H`
8. [x] `Bytecode.H`
9. [x] `Bytecode_Interpreter.H`
10. [x] `Compiler_Backend_C.H`
11. [x] `Compiler_Driver.H`

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
