# Soporte de front-end para compiladores

Esta guía documenta la primera capa de soporte orientada a compiladores añadida
a Aleph-w. Es deliberadamente pequeña y reusable: la idea es facilitar la
construcción de lexers, parsers y analizadores semánticos sin comprometer aún un
lenguaje concreto. El roadmap vivo de la plataforma completa está en
[docs/compiler_platform_roadmap.md](compiler_platform_roadmap.md).

## Alcance

El componente actual está formado por veintiún headers públicos:

| Header | Responsabilidad |
|---|---|
| `ah-source.H` | Registro de archivos fuente, offsets por byte, spans, resolución línea/columna y snippets de una sola línea |
| `ah-diagnostics.H` | Diagnósticos acumulados con labels primarios/secundarios, notas, ayudas y rendering plano determinista |
| `Compiler_Token.H` | Tipos de token, objetos token, clasificación de keywords y metadatos de operadores binarios para parsers futuros |
| `Compiler_Lexer.H` | Lexer de un solo archivo con spans estables, opciones de manejo de comentarios, recuperación mediante tokens inválidos y emisión opcional de diagnósticos |
| `Compiler_AST.H` | Nodos AST respaldados por arena para expresiones, statements, imports, declaraciones de tipo, funciones, módulos y volcado textual determinista |
| `Compiler_Parser.H` | Parser recursive-descent con precedence climbing, anotaciones de tipo explícitas, declaraciones top-level (`import`/`fn`/`type`/`struct`/`enum`), parseo de bloques/statements y recuperación apoyada en diagnósticos |
| `tpl_scope.H` | Pila genérica de scopes anidados para bindings léxicos y lookup con shadowing |
| `Compiler_Sema.H` | Resolución de nombres, validación básica de imports top-level, detección de duplicados, chequeos de shadowing y validación de `return`/`break`/`continue` |
| `Compiler_Types.H` | Grafo estable de tipos con builtins, tuplas, funciones, tipos nominales (`struct`/`enum`), variables de tipo y pretty-printing determinista |
| `tpl_constraints.H` | Conjuntos de constraints de igualdad, substitutions, unificación estructural, variables rígidas y soporte de occurs-check |
| `Compiler_Typed_Sema.H` | Pasada semántica tipada con anotaciones explícitas, resolución de declaraciones nominales, aliases transparentes, generalización de `let`, instanciación polimórfica básica y resolución de constraints |
| `Compiler_HIR.H` | HIR tipada y estructurada con lowering desde AST tipado, nodos estables para intérpretes o compilación posterior y dumps textuales deterministas |
| `Interpreter_Runtime.H` | Runtime reusable para evaluar HIR con valores dinámicos, environments anidados, call stack, funciones host y errores estructurados |
| `Compiler_CFG.H` | CFG reusable de bloques básicos con lowering desde HIR, terminadores explícitos, validación estructural y dumps deterministas |
| `Compiler_IR.H` | IR reusable con valores e instrucciones explícitas, lowering desde HIR, slots locales/globales y dumps textuales deterministas |
| `Compiler_Dataflow.H` | Análisis reusable sobre IR con reachability, liveness, definite assignment, propagación de constantes y dead-code elimination conservador |
| `SSA.H` | Capa SSA reusable sobre IR con dominadores, dominance frontier, inserción de phi, rename pass y verificador estructural |
| `Bytecode.H` | Bytecode reusable orientado a VM con pool de constantes por función, opcodes explícitos, PCs parcheados y lowering desde IR |
| `Bytecode_Interpreter.H` | VM portable para ejecutar bytecode de registros con frames explícitos, globals, errores estructurados y soporte opcional para host functions |
| `Compiler_Backend_C.H` | Backend portable que emite C legible desde IR explícita usando bloques con `goto`, runtime tagged mínimo y símbolos deterministas |
| `Compiler_Driver.H` | Driver reusable multiarchivo que resuelve imports declarativos por nombre de fuente, ordena módulos por dependencia, orquesta parse/sema/HIR/IR/`run`/`emit-c`/`emit-bytecode` y publica artefactos reproducibles |

## Arranque rápido

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

## Qué cubre el front-end actual

- Identificadores y keywords reservadas como `fn`, `let`, `if`, `while` y `return`
- Literales enteros
- Literales de string y de carácter
- Puntuación y operadores comunes, incluyendo `==`, `!=`, `&&`, `||`, `+=`, `->`
- Comentarios de línea `// ...`
- Comentarios de bloque `/* ... */`, que pueden conservarse como tokens o deshabilitarse
- Imports top-level `import "name.aw";` con nombres de fuente explícitos
- Declaraciones de función con listas de parámetros y anotaciones de retorno opcionales
- Declaraciones top-level de tipos `type`, `struct` y `enum`
- Bloques, `let`, `return`, `if`, `while`, `break`, `continue` y statements de expresión
- Llamadas, operadores unarios prefijos, agrupación y expresiones binarias con precedence climbing
- Anotaciones de tipo explícitas en parámetros, retornos, `let` y tipos de función `fn(...) -> ...`
- Scopes léxicos anidados y lookup de nombres
- Chequeos de declaraciones duplicadas, nombres no resueltos y shadowing opcional
- Validación básica de imports top-level, incluyendo self-imports y duplicados por módulo
- Diagnósticos básicos de uso indebido de `return`, `break` y `continue`
- Tipos standalone para builtins, tuplas, funciones, tipos nominales y variables de tipo
- Resolución de constraints de igualdad con substitutions, variables rígidas y occurs-check
- Análisis semántico tipado con resolución de anotaciones, declaraciones nominales (`struct`, `enum`, aliases transparentes), generalización de `let` e instanciación polimórfica básica para literales, locales, parámetros, retornos, condiciones, operadores y llamadas
- Lowering desde AST tipado a una HIR estructurada y reusable para intérpretes o futuras etapas CFG/MIR
- Evaluación estructurada de HIR con valores runtime, bindings léxicos, llamadas a funciones HIR y host functions
- Lowering desde HIR estructurada a CFG con bloques básicos, branches explícitos, joins, loops y validación de invariantes
- Lowering desde HIR tipada a una IR reusable con valores explícitos, cargas/almacenamientos, llamadas y terminadores por bloque
- Análisis de flujo de datos sobre IR con reachability, liveness por slots locales, definite assignment, propagación de constantes y DCE conservador
- Conversión de IR explícita a SSA con dominadores, dominance frontier, phis por slot local/parámetro y renombrado determinista
- Lowering desde IR explícita a bytecode reusable de registros con pool de constantes, opcodes de VM y PCs concretos por bloque
- Ejecución directa del bytecode reusable con una VM portable, frames por llamada, globals explícitos y resultados runtime estructurados
- Emisión de C portable y autosuficiente desde IR explícita con runtime embebido, funciones reusables y `main()` opcional para flujos `emit-c`
- Driver end-to-end multiarchivo con `parse-only`, `sema-only`, `hir`, `ir`, `run`, `emit-c` y `emit-bytecode`, más imports declarativos por nombre de fuente, orden topológico estable y artefactos intermedios reproducibles por etapa

La entrada malformada produce un token `Invalid`. Si el lexer recibe un
`Diagnostic_Engine`, además registra un error estructurado con código estable,
por ejemplo `LEX001` o `LEX004`. De forma análoga, el parser produce nodos AST
inválidos y diagnósticos como `PAR001` y `PAR005`. La fase semántica añade
códigos como `SEM001` para duplicados y `SEM002` para identificadores no resueltos.
La unificación de tipos devuelve resultados estructurados como
`Occurs_Check_Failed` y `Rigid_Variable` sin emitir diagnósticos por sí sola.
La pasada semántica tipada traduce fallos de tipado a códigos como `TYP002`,
`TYP005`, `TYP006`, `TYP007`, `TYP009`, `TYP010` y `TYP011`. La validación
base de imports usa códigos como `SEM009` y `SEM010`, y el driver añade
diagnósticos `DRV001`/`DRV002` para imports faltantes o ciclos.

## Ejemplos y tests

- Ejemplo: `Examples/compiler_diagnostics_example.cc`
- Ejemplo: `Examples/compiler_lexer_example.cc`
- Ejemplo: `Examples/compiler_parser_example.cc`
- Ejemplo: `Examples/compiler_sema_example.cc`
- Ejemplo: `Examples/compiler_types_example.cc`
- Ejemplo: `Examples/compiler_typed_sema_example.cc`
- Ejemplo: `Examples/compiler_hir_example.cc`
- Ejemplo: `Examples/interpreter_runtime_example.cc`
- Ejemplo: `Examples/compiler_cfg_example.cc`
- Ejemplo: `Examples/compiler_ir_example.cc`
- Ejemplo: `Examples/compiler_dataflow_example.cc`
- Ejemplo: `Examples/ssa_example.cc`
- Ejemplo: `Examples/bytecode_example.cc`
- Ejemplo: `Examples/bytecode_interpreter_example.cc`
- Ejemplo: `Examples/compiler_backend_c_example.cc`
- Ejemplo: `Examples/compiler_driver_example.cc`
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
- Tests: `Tests/compiler_hir_test.cc`
- Tests: `Tests/interpreter_runtime_test.cc`
- Tests: `Tests/compiler_cfg_test.cc`
- Tests: `Tests/compiler_ir_test.cc`
- Tests: `Tests/compiler_dataflow_test.cc`
- Tests: `Tests/ssa_test.cc`
- Tests: `Tests/bytecode_test.cc`
- Tests: `Tests/bytecode_interpreter_test.cc`
- Tests: `Tests/compiler_backend_c_test.cc`
- Tests: `Tests/compiler_driver_test.cc`

Para compilar los targets afectados:

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
  compiler_hir_test \
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

## Límites actuales

- Las ubicaciones usan offsets por byte en UTF-8, no columnas por grafema
- Los snippets se recortan a una sola línea para mantener diagnósticos planos y deterministas
- Los literales numéricos cubren por ahora solo enteros
- La pasada original `Compiler_Sema.H` sigue siendo solo de nombres; el tipado y la invocabilidad real viven en `Compiler_Typed_Sema.H`
- El polimorfismo sigue siendo básico: variables de tipo explícitas en anotaciones y generalización de `let` sobre el grafo de tipos actual
- Los tipos nominales actuales son opacos y declarativos: todavía no hay construcción de valores, acceso a campos, payloads de `enum` ni pattern matching
- Los imports actuales son declarativos y globales: se resuelven por nombre exacto de fuente, reordenan módulos en el driver y todavía no hay `export`, namespaces ni visibilidad selectiva
- Ya existen HIR, runtime reusable, CFG, IR explícita, análisis de flujo de datos, SSA, bytecode, intérprete de bytecode, backend C portable y un driver reusable multiarchivo, pero todavía no hay backend LLVM/nativo ni sistema de módulos completo
- Todavía no hay resolución de overloads, coerciones, resolución de traits ni análisis de retornos path-sensitive

Eso es intencional: esta capa debe ser la base estable para módulos futuros, no
un front-end completamente tipado a medio hacer.

## Siguientes pasos sugeridos

Una vez cerrada esta base, los módulos naturales que siguen son:

1. Una herramienta CLI o wrapper sobre `Compiler_Driver.H`
2. Backend a LLVM IR o equivalente
3. Construcción/uso de tipos nominales en HIR/IR/runtime
4. `export`, namespaces o visibilidad selectiva sobre el modelo actual de imports
5. Tipos explícitos y polimorfismo más allá del MVP actual
