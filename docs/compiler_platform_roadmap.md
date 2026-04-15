# Roadmap de plataforma para compiladores e intérpretes

Este documento define el roadmap de la infraestructura de implementación de
lenguajes en Aleph-w. La meta no es construir un compilador concreto, sino una
plataforma reusable para:

- compiladores
- intérpretes
- transpilers
- analizadores estáticos
- VMs o bytecode runtimes

La regla es simple: cuando una etapa queda cerrada y verificada, se marca con
`[x]`.

## Estado actual

### 1. Infraestructura base de front-end

- [x] Gestión de fuente y spans (`ah-source.H`)
- [x] Diagnósticos estructurados (`ah-diagnostics.H`)
- [x] Modelo de tokens (`Compiler_Token.H`)
- [x] Lexer (`Compiler_Lexer.H`)
- [x] AST (`Compiler_AST.H`)
- [x] Parser (`Compiler_Parser.H`)
- [x] Scopes léxicos (`tpl_scope.H`)
- [x] Semántica de nombres y control básico (`Compiler_Sema.H`)
- [x] Grafo de tipos (`Compiler_Types.H`)
- [x] Constraints y unificación (`tpl_constraints.H`)
- [x] Semántica tipada monomórfica (`Compiler_Typed_Sema.H`)
- [x] Tests y examples para todas las piezas anteriores
- [x] Documentación Doxygen y guías Markdown/README para el bloque actual

### 2. Estado funcional del bloque actual

- [x] Tipado de literales
- [x] Tipado de parámetros y retornos inferidos
- [x] Tipado de `let`
- [x] Tipado de operadores aritméticos, lógicos y comparaciones
- [x] Tipado de condiciones `if` y `while`
- [x] Tipado de llamadas
- [x] Diagnósticos tipados básicos (`TYP*`)

## Próximas etapas

### 3. Tipos explícitos y polimorfismo

- [ ] Anotaciones de tipo en parser y AST
- [ ] Resolución de tipos anotados hacia `Compiler_Types.H`
- [ ] Generalización de `let`
- [ ] Instanciación polimórfica
- [ ] Tipos nominales: `struct`, `enum`, aliases
- [ ] Tipos compuestos adicionales: arrays, records, option/result si aplica
- [ ] Soporte para módulos/imports en la capa semántica

### 4. HIR reutilizable para compiladores e intérpretes

- [x] `Compiler_HIR.H`
- [x] Nodos HIR tipados y más estables que el AST
- [x] Lowering desde AST tipado a HIR
- [x] Detección clara de efectos, llamadas, control y valores
- [x] Pretty-printer / dumper de HIR
- [x] Tests y examples de HIR

### 5. Runtime para intérpretes

- [ ] `Interpreter_Runtime.H`
- [ ] `Value` runtime general
- [ ] Frames y environments
- [ ] Call stack
- [ ] Closures / funciones host
- [ ] Soporte base para `Unit`, `Bool`, `Int`, `String`, `Char`, tuplas
- [ ] Errores runtime estructurados
- [ ] Evaluador sobre HIR o AST tipado
- [ ] Example de intérprete mínimo

### 6. CFG e IR de propósito general

- [ ] `Compiler_CFG.H`
- [ ] Builder de CFG por función
- [ ] Validadores de CFG
- [ ] `Compiler_IR.H` o `Compiler_MIR.H`
- [ ] Instrucciones y valores explícitos
- [ ] Lowering desde HIR a MIR/IR
- [ ] Dumps textuales deterministas
- [ ] Tests y examples de CFG/IR

### 7. Análisis y optimización

- [ ] `Compiler_Dataflow.H`
- [ ] Reachability
- [ ] Liveness
- [ ] Definite assignment / initialization
- [ ] Constant propagation
- [ ] Dead code elimination
- [ ] Validadores de invariantes por pass

### 8. SSA

- [ ] `SSA.H`
- [ ] Dominadores
- [ ] Dominance frontier
- [ ] Inserción de phi
- [ ] Rename pass
- [ ] Verificador SSA
- [ ] Tests y example de SSA

### 9. Bytecode e interpretación eficiente

- [ ] `Bytecode.H`
- [ ] Formato de constantes, funciones y opcodes
- [ ] Lowering MIR/HIR a bytecode
- [ ] `Bytecode_Interpreter.H`
- [ ] VM simple y portable
- [ ] Tests end-to-end de ejecución

### 10. Backends de compilación

- [ ] `Compiler_Backend_C.H`
- [ ] Backend a LLVM IR o equivalente
- [ ] Backend a assembly nativo si llega a ser necesario
- [ ] Layout de datos y calling conventions
- [ ] Integración con assembler/linker cuando aplique

### 11. Driver y toolchain

- [ ] `Compiler_Driver.H`
- [ ] Pipeline configurable por etapas
- [ ] `parse-only`, `sema-only`, `hir`, `ir`, `run`, `emit-c`, `emit-bytecode`
- [ ] Soporte multiarchivo
- [ ] Artefactos intermedios reproducibles
- [ ] Tests de integración end-to-end

## Orden recomendado

Si la prioridad es que sirva tanto para compiladores como para intérpretes:

1. [ ] Tipos explícitos y polimorfismo básico
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

## Criterio de cierre de cada etapa

Una etapa debería marcarse como lista solo si cumple todo esto:

- [ ] API pública documentada con Doxygen útil
- [ ] tests unitarios
- [ ] al menos un example ejecutable
- [ ] docs Markdown actualizadas
- [ ] referencias desde README cuando aplique
- [ ] verificación local de build/test

## Nota de diseño

La arquitectura objetivo sigue siendo:

`source -> tokens -> AST -> sema -> typed sema -> HIR -> interpreter`

y en paralelo:

`HIR -> CFG/IR -> dataflow/SSA -> bytecode/backend -> ejecución`

Eso mantiene el componente general y reusable, sin casarlo con un solo lenguaje
ni con un solo modelo de ejecución.
