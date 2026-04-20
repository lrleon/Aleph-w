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
- [x] Operadores reutilizables desacoplados del lexer (`Compiler_Operator.H`)
- [x] Contratos reusables para frontends y drivers (`Compiler_Driver_Contracts.H`, `Compiler_Frontend.H`)
- [x] Adapter del frontend MVP (`Compiler_MVP_Frontend.H`)
- [x] Scopes léxicos (`tpl_scope.H`)
- [x] Semántica de nombres y control básico (`Compiler_Sema.H`)
- [x] Grafo de tipos (`Compiler_Types.H`)
- [x] Constraints y unificación (`tpl_constraints.H`)
- [x] Semántica tipada con anotaciones y polimorfismo básico (`Compiler_Typed_Sema.H`)
- [x] Tests y examples para todas las piezas anteriores
- [x] Documentación Doxygen y guías Markdown/README para el bloque actual

### 2. Estado funcional del bloque actual

- [x] Tipado de literales
- [x] Tipado de parámetros y retornos inferidos
- [x] Tipado de `let`
- [x] Anotaciones explícitas en parámetros, retornos y `let`
- [x] Resolución de anotaciones hacia `Compiler_Types.H`
- [x] Tipado de operadores aritméticos, lógicos y comparaciones
- [x] Tipado de condiciones `if` y `while`
- [x] Tipado de llamadas
- [x] Generalización de `let`
- [x] Instanciación polimórfica básica
- [x] Diagnósticos tipados básicos (`TYP*`)

## Próximas etapas

### 3. Tipos explícitos y polimorfismo

- [x] Anotaciones de tipo en parser y AST
- [x] Resolución de tipos anotados hacia `Compiler_Types.H`
- [x] Generalización de `let`
- [x] Instanciación polimórfica
- [x] Tipos nominales opacos: `struct`, `enum`, aliases transparentes
- [ ] Tipos compuestos adicionales: arrays, records, option/result si aplica
- [x] Soporte base para imports declarativos en semántica y driver

### 4. HIR reutilizable para compiladores e intérpretes

- [x] `Compiler_HIR.H`
- [x] `Compiler_HIR_Model.H`
- [x] `Compiler_HIR_Lowering_MVP.H`
- [x] Nodos HIR tipados y más estables que el AST
- [x] Lowering desde AST tipado a HIR
- [x] Detección clara de efectos, llamadas, control y valores
- [x] Pretty-printer / dumper de HIR
- [x] Tests y examples de HIR

### 5. Runtime para intérpretes

- [x] `Interpreter_Runtime.H`
- [x] `Value` runtime general
- [x] Frames y environments
- [x] Call stack
- [x] Closures / funciones host
- [x] Soporte base para `Unit`, `Bool`, `Int`, `String`, `Char`, tuplas
- [x] Errores runtime estructurados
- [x] Evaluador sobre HIR o AST tipado
- [x] Example de intérprete mínimo

### 6. CFG e IR de propósito general

- [x] `Compiler_CFG.H`
- [x] Builder de CFG por función
- [x] Validadores de CFG
- [x] `Compiler_IR.H` o `Compiler_MIR.H`
- [x] `Compiler_IR_Model.H`
- [x] `Compiler_IR_Lowering_MVP.H`
- [x] Instrucciones y valores explícitos
- [x] Lowering desde HIR a MIR/IR
- [x] Dumps textuales deterministas de CFG
- [x] Tests y examples de CFG
- [x] Tests y examples de MIR/IR

### 7. Análisis y optimización

- [x] `Compiler_Dataflow.H`
- [x] Reachability
- [x] Liveness
- [x] Definite assignment / initialization
- [x] Constant propagation
- [x] Dead code elimination
- [x] Validadores de invariantes por pass

### 8. SSA

- [x] `SSA.H`
- [x] Dominadores
- [x] Dominance frontier
- [x] Inserción de phi
- [x] Rename pass
- [x] Verificador SSA
- [x] Tests y example de SSA

### 9. Bytecode e interpretación eficiente

- [x] `Bytecode.H`
- [x] Formato de constantes, funciones y opcodes
- [x] Lowering MIR/HIR a bytecode
- [x] `Bytecode_Interpreter.H`
- [x] VM simple y portable
- [x] Tests end-to-end de ejecución

### 10. Backends de compilación

- [x] `Compiler_Backend_C.H`
- [ ] Backend a LLVM IR o equivalente
- [ ] Backend a assembly nativo si llega a ser necesario
- [ ] Layout de datos y calling conventions
- [ ] Integración con assembler/linker cuando aplique

### 11. Driver y toolchain

- [x] `Compiler_Driver.H`
- [x] `Compiler_Generic_Driver.H`
- [x] Pipeline configurable por etapas
- [x] `parse-only`, `sema-only`, `hir`, `ir`, `run`, `emit-c`, `emit-bytecode`
- [x] Soporte multiarchivo
- [x] Artefactos intermedios reproducibles
- [x] Tests de integración end-to-end

## Orden recomendado

Si la prioridad es que sirva tanto para compiladores como para intérpretes:

1. [x] Tipos explícitos y polimorfismo básico
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
