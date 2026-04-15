# Soporte de front-end para compiladores

Esta guía documenta la primera capa de soporte orientada a compiladores añadida
a Aleph-w. Es deliberadamente pequeña y reusable: la idea es facilitar la
construcción de lexers, parsers y analizadores semánticos sin comprometer aún un
lenguaje concreto. El roadmap vivo de la plataforma completa está en
[docs/compiler_platform_roadmap.md](compiler_platform_roadmap.md).

## Alcance

El componente actual está formado por doce headers públicos:

| Header | Responsabilidad |
|---|---|
| `ah-source.H` | Registro de archivos fuente, offsets por byte, spans, resolución línea/columna y snippets de una sola línea |
| `ah-diagnostics.H` | Diagnósticos acumulados con labels primarios/secundarios, notas, ayudas y rendering plano determinista |
| `Compiler_Token.H` | Tipos de token, objetos token, clasificación de keywords y metadatos de operadores binarios para parsers futuros |
| `Compiler_Lexer.H` | Lexer de un solo archivo con spans estables, opciones de manejo de comentarios, recuperación mediante tokens inválidos y emisión opcional de diagnósticos |
| `Compiler_AST.H` | Nodos AST respaldados por arena para expresiones, statements, funciones, módulos y volcado textual determinista |
| `Compiler_Parser.H` | Parser recursive-descent con precedence climbing, parseo de bloques/statements y recuperación apoyada en diagnósticos |
| `tpl_scope.H` | Pila genérica de scopes anidados para bindings léxicos y lookup con shadowing |
| `Compiler_Sema.H` | Resolución de nombres, detección de duplicados, chequeos de shadowing, validación básica de `return`/`break`/`continue` y rechazo de llamadas a no-funciones |
| `Compiler_Types.H` | Grafo estable de tipos con builtins, tuplas, funciones, variables de tipo y pretty-printing determinista |
| `tpl_constraints.H` | Conjuntos de constraints de igualdad, substitutions, unificación estructural, variables rígidas y soporte de occurs-check |
| `Compiler_Typed_Sema.H` | Pasada semántica tipada que conecta el AST, la semántica base, el grafo de tipos y el unifier |
| `Compiler_HIR.H` | HIR tipada y estructurada con lowering desde AST tipado, nodos estables para intérpretes o compilación posterior y dumps textuales deterministas |

## Arranque rápido

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

## Qué cubre el front-end actual

- Identificadores y keywords reservadas como `fn`, `let`, `if`, `while` y `return`
- Literales enteros
- Literales de string y de carácter
- Puntuación y operadores comunes, incluyendo `==`, `!=`, `&&`, `||`, `+=`, `->`
- Comentarios de línea `// ...`
- Comentarios de bloque `/* ... */`, que pueden conservarse como tokens o deshabilitarse
- Declaraciones de función con listas de parámetros
- Bloques, `let`, `return`, `if`, `while`, `break`, `continue` y statements de expresión
- Llamadas, operadores unarios prefijos, agrupación y expresiones binarias con precedence climbing
- Scopes léxicos anidados y lookup de nombres
- Chequeos de declaraciones duplicadas, nombres no resueltos y shadowing opcional
- Diagnósticos básicos de uso indebido de `return`, `break` y `continue`
- Rechazo de llamadas a símbolos que no son funciones en el modelo actual basado en nombres
- Tipos standalone para builtins, tuplas, funciones y variables de tipo
- Resolución de constraints de igualdad con substitutions, variables rígidas y occurs-check
- Análisis semántico tipado monomórfico para literales, locales, parámetros, retornos, condiciones, operadores y llamadas
- Lowering desde AST tipado a una HIR estructurada y reusable para intérpretes o futuras etapas CFG/MIR

La entrada malformada produce un token `Invalid`. Si el lexer recibe un
`Diagnostic_Engine`, además registra un error estructurado con código estable,
por ejemplo `LEX001` o `LEX004`. De forma análoga, el parser produce nodos AST
inválidos y diagnósticos como `PAR001` y `PAR005`. La fase semántica añade
códigos como `SEM001` para duplicados y `SEM002` para identificadores no resueltos.
La unificación de tipos devuelve resultados estructurados como
`Occurs_Check_Failed` y `Rigid_Variable` sin emitir diagnósticos por sí sola.
La pasada semántica tipada traduce fallos de tipado a códigos como `TYP002`,
`TYP005` y `TYP006`.

## Ejemplos y tests

- Ejemplo: `Examples/compiler_diagnostics_example.cc`
- Ejemplo: `Examples/compiler_lexer_example.cc`
- Ejemplo: `Examples/compiler_parser_example.cc`
- Ejemplo: `Examples/compiler_sema_example.cc`
- Ejemplo: `Examples/compiler_types_example.cc`
- Ejemplo: `Examples/compiler_typed_sema_example.cc`
- Ejemplo: `Examples/compiler_hir_example.cc`
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
  compiler_diagnostics_example \
  compiler_lexer_example \
  compiler_parser_example \
  compiler_sema_example \
  compiler_types_example \
  compiler_typed_sema_example \
  compiler_hir_example
```

## Límites actuales

- Las ubicaciones usan offsets por byte en UTF-8, no columnas por grafema
- Los snippets se recortan a una sola línea para mantener diagnósticos planos y deterministas
- Los literales numéricos cubren por ahora solo enteros
- La pasada original `Compiler_Sema.H` sigue siendo solo de nombres; el tipado vive en `Compiler_Typed_Sema.H`
- La inferencia de tipos es monomórfica y local; todavía no hay generalización de `let` ni instanciación polimórfica
- Aún no existe sintaxis de anotaciones de tipo en el parser o el AST
- Las llamadas solo se validan contra símbolos de función nombrados, no contra callables inferidos
- Ya existe una HIR estructurada, pero todavía no hay CFG, MIR/IR, bytecode ni runtime de intérprete
- Todavía no hay resolución de overloads, coerciones, resolución de traits, análisis de retornos por caminos ni análisis de flujo de datos

Eso es intencional: esta capa debe ser la base estable para módulos futuros, no
un front-end completamente tipado a medio hacer.

## Siguientes pasos sugeridos

Una vez cerrada esta base, los módulos naturales que siguen son:

1. Soporte de anotaciones de tipo en parser y AST
2. Generalización e instanciación polimórfica sobre `Compiler_Types.H`
3. `Interpreter_Runtime.H`
4. `Compiler_CFG.H`
5. `Compiler_IR.H` / `SSA.H`
