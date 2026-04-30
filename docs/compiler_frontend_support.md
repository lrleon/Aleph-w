# Soporte de front-end para compiladores

Esta guía documenta la primera capa de soporte orientada a compiladores añadida
a Aleph-w. Es deliberadamente pequeña y reusable: la idea es facilitar la
construcción de lexers, parsers y analizadores semánticos sin comprometer aún un
lenguaje concreto. El roadmap vivo de la plataforma completa está en
[docs/compiler_platform_roadmap.md](compiler_platform_roadmap.md).

## Alcance

El componente actual está formado por cuarenta y tres headers públicos:

| Header | Responsabilidad |
|---|---|
| `ah-source.H` | Registro de archivos fuente, offsets por byte, spans, resolución línea/columna y snippets de una sola línea |
| `ah-diagnostics.H` | Diagnósticos acumulados con labels primarios/secundarios, notas, ayudas y rendering plano determinista |
| `Compiler_Token.H` | Tipos de token, objetos token, clasificación de keywords y metadatos de operadores binarios para parsers futuros |
| `Compiler_Lexer.H` | Lexer de un solo archivo con spans estables, opciones de manejo de comentarios, recuperación mediante tokens inválidos y emisión opcional de diagnósticos |
| `Compiler_Parser_Utils.H` | Stream reusable sobre tokens con lookahead, `expect`, listas delimitadas y sincronización básica para parsers recursive-descent |
| `Compiler_AST.H` | Nodos AST respaldados por arena para expresiones, statements, imports, declaraciones de tipo, funciones, módulos y volcado textual determinista |
| `Compiler_Parser.H` | Parser recursive-descent con precedence climbing, anotaciones de tipo explícitas, declaraciones top-level (`import`/`fn`/`type`/`struct`/`enum`), parseo de bloques/statements y recuperación apoyada en diagnósticos |
| `Compiler_Operator.H` | Vocabulario reusable de operadores compartido por HIR, IR, bytecode y backends, con compatibilidad hacia el lexer MVP |
| `Compiler_Driver_Contracts.H` | Contratos compartidos de acciones, inputs, artifacts y resultados de ejecución para drivers y frontends |
| `Compiler_Module_Resolver.H` | Resolvedor reusable de imports por nombre de fuente con orden topológico, ciclos/dependencias faltantes e artifact textual estable |
| `Compiler_Module_Metadata.H` | Metadatos reusables de superficie de módulo con top-level exports visibles y rendering determinista |
| `Compiler_Module_Linker.H` | Linker reusable de superficies de módulo para calcular visibilidad por imports directos y detectar conflictos de nombres |
| `Compiler_Module_Binder.H` | Binder reusable de nombres top-level visibles por módulo, con resolución determinista y omisión de nombres ambiguos |
| `Compiler_Module_Name_Table.H` | Caché reusable por módulo para nombres top-level ya bindables, útil para consultas repetidas sin reescanear bindings ni issues |
| `Compiler_Module_Name_Resolver.H` | Resolvedor reusable de nombres top-level sobre `modules.binding`, con resultados estructurados para nombre faltante, ambiguo o de kind incorrecto |
| `Compiler_Module_Name_Diagnostics.H` | Diagnósticos reusables sobre resultados de resolución inter-módulo, con notas y ayudas consistentes para frontends externos |
| `Compiler_Module_Semantic_Environment.H` | Entorno semántico reusable por módulo con namespaces separados de valores y tipos para consumo directo por frontends |
| `Compiler_Frontend.H` | Interfaz reusable para frontends de lenguaje que quieran integrarse al pipeline común |
| `Compiler_MVP_Frontend.H` | Adapter del frontend actual del lenguaje MVP expuesto como implementación de referencia de `Compiler_Frontend` |
| `Compiler_Generic_Driver.H` | Driver reusable desacoplado del parser MVP, capaz de orquestar cualquier `Compiler_Frontend` que produzca HIR/IR |
| `Compiler_Line_Frontend.H` | Segundo frontend de ejemplo, con sintaxis propia y lowering directo a HIR/IR usando sólo contratos reusables |
| `tpl_scope.H` | Pila genérica de scopes anidados para bindings léxicos y lookup con shadowing |
| `Compiler_Symbol_Bindings.H` | Bindings léxicos reusables con ids estables, detección estructurada de duplicados/shadowing y dumps deterministas |
| `Compiler_Sema.H` | Resolución de nombres, validación básica de imports top-level, detección de duplicados, chequeos de shadowing y validación de `return`/`break`/`continue` |
| `Compiler_Types.H` | Grafo estable de tipos con builtins, tuplas, funciones, tipos nominales (`struct`/`enum`), variables de tipo y pretty-printing determinista |
| `tpl_constraints.H` | Conjuntos de constraints de igualdad, substitutions, unificación estructural, variables rígidas y soporte de occurs-check |
| `Compiler_Typed_Sema.H` | Pasada semántica tipada con anotaciones explícitas, resolución de declaraciones nominales, aliases transparentes, generalización de `let`, instanciación polimórfica básica y resolución de constraints |
| `Compiler_HIR_Model.H` | Modelo reusable de HIR tipada, ownership por arena y dumps deterministas sin depender del frontend MVP |
| `Compiler_HIR_Builder.H` | Builder reusable para construir HIR directamente desde un frontend externo o desde tests sin depender del lowering MVP |
| `Compiler_HIR_Lowering_MVP.H` | Lowering desde el AST tipado actual del lenguaje MVP hacia la HIR reusable |
| `Compiler_HIR.H` | HIR tipada y estructurada con lowering desde AST tipado, nodos estables para intérpretes o compilación posterior y dumps textuales deterministas |
| `Interpreter_Runtime.H` | Runtime reusable para evaluar HIR con valores dinámicos, environments anidados, call stack, funciones host y errores estructurados |
| `Compiler_CFG.H` | CFG reusable de bloques básicos con lowering desde HIR, terminadores explícitos, validación estructural y dumps deterministas |
| `Compiler_IR_Model.H` | IR reusable con valores e instrucciones explícitas, slots locales/globales, validación estructural y dumps textuales deterministas |
| `Compiler_IR_Builder.H` | Builder reusable para construir IR explícita directamente y mantener spans, edges y predecesores consistentes |
| `Compiler_IR_Lowering_MVP.H` | Lowering desde la HIR del lenguaje MVP actual hacia la IR reusable |
| `Compiler_IR.H` | Header umbrella de compatibilidad que incluye el modelo reusable de IR y el lowering MVP |
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
- Construcción manual de HIR e IR reusable mediante builders públicos, útil para frontends alternativos, transpilers o tests estructurales sin parser
- Validación de un segundo frontend no-MVP (`Compiler_Line_Frontend.H`) que usa su propio parser y se integra al pipeline común mediante `Compiler_Generic_Driver.H`
- Utilidades reusables de parsing token-oriented mediante `Compiler_Parser_Utils.H`, útiles para otros parsers recursive-descent sin AST compartido
- Bindings léxicos reusables mediante `Compiler_Symbol_Bindings.H`, útiles para semántica básica de nombres sin depender del AST MVP
- Resolución reusable de imports y orden de módulos mediante `Compiler_Module_Resolver.H`, útil para frontends multiarchivo sin depender de `Compiler_Driver.H`
- Metadatos reusables de superficie de módulo mediante `Compiler_Module_Metadata.H`, útiles para herramientas, exports implícitos y APIs de inspección
- Linking reusable de superficies de módulo mediante `Compiler_Module_Linker.H`, útil para visibilidad por imports directos y detección temprana de conflictos
- Binding reusable de nombres top-level entre módulos mediante `Compiler_Module_Binder.H`, útil para herramientas que necesiten inspeccionar qué nombres quedan realmente resolubles después del linkage
- Caché reusable por módulo de nombres top-level mediante `Compiler_Module_Name_Table.H`, útil para frontends con muchas consultas semánticas repetidas
- Resolución reusable de nombres top-level entre módulos mediante `Compiler_Module_Name_Resolver.H`, útil para semántica inter-módulo sobre la vista ya ligada
- Diagnósticos reusables para errores de resolución inter-módulo mediante `Compiler_Module_Name_Diagnostics.H`, útiles para reutilizar wording, notas y ayudas estables
- Entorno semántico reusable por módulo mediante `Compiler_Module_Semantic_Environment.H`, útil para consumir namespaces de valores y tipos ya separados

## Punto de extensión reusable

Desde esta iteración, Aleph-w distingue entre:

- el frontend MVP de referencia, implementado por `Compiler_Parser.H`, `Compiler_AST.H`, `Compiler_Sema.H`, `Compiler_Typed_Sema.H` y adaptado por `Compiler_MVP_Frontend.H`
- la infraestructura reusable, expuesta por `Compiler_Frontend.H`, `Compiler_Generic_Driver.H`, `Compiler_HIR_Model.H`, IR, bytecode, runtime y backends

Eso permite dos estrategias para un lenguaje nuevo:

- reutilizar `ah-source.H`, `ah-diagnostics.H`, `Compiler_Parser_Utils.H`, `Compiler_Symbol_Bindings.H`, `Compiler_Module_Resolver.H`, `Compiler_Module_Metadata.H`, `Compiler_Module_Linker.H`, `Compiler_Module_Binder.H`, `Compiler_Module_Name_Table.H`, `Compiler_Module_Name_Resolver.H`, `Compiler_Module_Name_Diagnostics.H`, `Compiler_Module_Semantic_Environment.H`, scopes y constraints para escribir lexer/parser/AST/sema/modularidad propios, para luego bajar a HIR o IR
- reutilizar el frontend MVP como ejemplo y sustituir o extender sus piezas gradualmente

El contrato mínimo para entrar al pipeline común es producir una `Compiler_HIR_Module` o una `Compiler_IR_Module` válida a través de una implementación de `Compiler_Frontend`.

Para frontends multiarchivo, `Compiler_Frontend` también expone ahora `module_descriptors()`, `module_merge_order()` y `module_metadata()`. Eso permite que drivers reusables y herramientas externas inspeccionen imports, orden de merge y superficie top-level de cada módulo sin depender de artifacts específicos de cada frontend.

En particular, `Compiler_HIR.H` queda como header umbrella de compatibilidad. El punto reusable nuevo es `Compiler_HIR_Model.H`; el lowering del lenguaje de referencia vive en `Compiler_HIR_Lowering_MVP.H`.

De forma análoga, `Compiler_IR.H` queda como umbrella de compatibilidad. El punto reusable nuevo es `Compiler_IR_Model.H`; el lowering del lenguaje de referencia vive en `Compiler_IR_Lowering_MVP.H`.

Para frontends que no quieran pasar por el lowering del lenguaje MVP, Aleph-w ahora ofrece `Compiler_HIR_Builder.H` y `Compiler_IR_Builder.H`. Esos builders permiten construir módulos manualmente sobre `Compiler_HIR_Context` y `Compiler_IR_Context` mientras preservan spans agregados, membresía de contenedores y, en IR, las relaciones de control-flow entre bloques.

Como validación concreta de ese punto de extensión, `Compiler_Line_Frontend.H` implementa un lenguaje line-oriented mínimo con parser propio, análisis semántico propio y lowering directo a HIR/IR. Los ejemplos [Examples/compiler_builder_example.cc](../Examples/compiler_builder_example.cc) y [Examples/compiler_line_frontend_example.cc](../Examples/compiler_line_frontend_example.cc), junto con [Tests/compiler_builder_test.cc](../Tests/compiler_builder_test.cc) y [Tests/compiler_line_frontend_test.cc](../Tests/compiler_line_frontend_test.cc), cubren tanto builders manuales como un frontend alternativo completo.

Para frontends token-oriented más pequeños, `Compiler_Parser_Utils.H` ofrece `Compiler_Parser_Stream`, un wrapper reusable sobre `Compiler_Lexer` con `peek`, `next`, `match`, `expect`, `expect_identifier`, parseo de listas separadas por delimitadores y sincronización de statements. El ejemplo [Examples/compiler_parser_utils_example.cc](../Examples/compiler_parser_utils_example.cc) y [Tests/compiler_parser_utils_test.cc](../Tests/compiler_parser_utils_test.cc) muestran el uso de esa capa sin depender del AST del lenguaje MVP.

Para frontends que necesiten semántica básica de nombres sin reutilizar `Compiler_Sema.H`, `Compiler_Symbol_Bindings.H` ofrece un stack reusable de bindings léxicos con ids estables, lookup local/global, y resultados estructurados para inserciones duplicadas o con shadowing. El ejemplo [Examples/compiler_symbol_bindings_example.cc](../Examples/compiler_symbol_bindings_example.cc) y [Tests/compiler_symbol_bindings_test.cc](../Tests/compiler_symbol_bindings_test.cc) cubren esa capa; además, tanto `Compiler_Sema.H` como `Compiler_Line_Frontend.H` la usan ya como infraestructura común.

Para frontends multiarchivo que resuelvan imports por nombre exacto de fuente, `Compiler_Module_Resolver.H` ofrece un resolvedor reusable con orden dependency-first, reporte estructurado de dependencias faltantes y ciclos, y artifact textual estable. El ejemplo [Examples/compiler_module_resolver_example.cc](../Examples/compiler_module_resolver_example.cc) y [Tests/compiler_module_resolver_test.cc](../Tests/compiler_module_resolver_test.cc) cubren esa capa; además, tanto `Compiler_Driver.H` como `Compiler_Line_Frontend.H` la usan ya como infraestructura común.

Para herramientas que necesiten inspeccionar la superficie top-level de cada módulo sin entrar al AST, `Compiler_Module_Metadata.H` ofrece un modelo reusable de exports visibles por módulo y un renderer determinista. `Compiler_Generic_Driver.H` publica esa vista como `modules.surface`, y tanto `Compiler_MVP_Frontend.H` como `Compiler_Line_Frontend.H` implementan ya `module_metadata()` sobre ese contrato.

Para herramientas que necesiten inspeccionar qué nombres quedan visibles después de considerar imports directos, `Compiler_Module_Linker.H` ofrece un linker reusable entre `Compiler_Module_Descriptor` y `Compiler_Module_Metadata`. `Compiler_Generic_Driver.H` publica esa vista como `modules.linkage`, y el ejemplo [Examples/compiler_module_linker_example.cc](../Examples/compiler_module_linker_example.cc) junto con [Tests/compiler_module_linker_test.cc](../Tests/compiler_module_linker_test.cc) cubren tanto el caso normal como conflictos de nombres visibles.

Para herramientas que necesiten inspeccionar qué nombres top-level quedan realmente bindables después del linkage, `Compiler_Module_Binder.H` convierte `modules.linkage` en una vista determinista de bindings por módulo. `Compiler_Generic_Driver.H` publica esa vista como `modules.binding`, y el ejemplo [Examples/compiler_module_binder_example.cc](../Examples/compiler_module_binder_example.cc) junto con [Tests/compiler_module_binder_test.cc](../Tests/compiler_module_binder_test.cc) cubren tanto la resolución normal como la omisión de nombres ambiguos.

Para frontends con muchas consultas semánticas repetidas por módulo, `Compiler_Module_Name_Table.H` ofrece una caché reusable derivada de `modules.binding`. `Compiler_Generic_Driver.H` publica esa vista como `modules.names`, y el ejemplo [Examples/compiler_module_name_table_example.cc](../Examples/compiler_module_name_table_example.cc) junto con [Tests/compiler_module_name_table_test.cc](../Tests/compiler_module_name_table_test.cc) cubren tanto la construcción de la tabla como la resolución a través de esa caché.

Para frontends que necesiten resolver un nombre top-level concreto contra esa vista ya ligada, `Compiler_Module_Name_Resolver.H` ofrece resultados estructurados para `Resolved`, `Missing_Module`, `Missing_Name`, `Wrong_Kind` y `Ambiguous_Name`. El ejemplo [Examples/compiler_module_name_resolver_example.cc](../Examples/compiler_module_name_resolver_example.cc) y [Tests/compiler_module_name_resolver_test.cc](../Tests/compiler_module_name_resolver_test.cc) cubren esa capa; además, `Compiler_Line_Frontend.H` la usa ya para validar llamadas top-level respetando imports reales.

Para frontends que además quieran traducir esos resultados a diagnósticos consistentes sin duplicar wording, `Compiler_Module_Name_Diagnostics.H` ofrece una capa reusable sobre `Diagnostic_Engine`. El ejemplo [Examples/compiler_module_name_diagnostics_example.cc](../Examples/compiler_module_name_diagnostics_example.cc) y [Tests/compiler_module_name_diagnostics_test.cc](../Tests/compiler_module_name_diagnostics_test.cc) cubren esa traducción, y `Compiler_Line_Frontend.H` ya la usa para emitir `LIN015`.

Para frontends que quieran consumir directamente un entorno semántico namespace-separated, `Compiler_Module_Semantic_Environment.H` deriva `modules.semantic` a partir de `modules.names` y separa valores de tipos por módulo. El ejemplo [Examples/compiler_module_semantic_environment_example.cc](../Examples/compiler_module_semantic_environment_example.cc) y [Tests/compiler_module_semantic_environment_test.cc](../Tests/compiler_module_semantic_environment_test.cc) cubren esa capa; además, `Compiler_Line_Frontend.H` la usa ya como fast path para validar llamadas top-level.

## Gramática superficial actual

La gramática implementada hoy por [Compiler_Parser.H](../Compiler_Parser.H) no
está definida en un archivo separado de EBNF formal; la definición canónica es
el parser recursive-descent mismo. Aun así, el lenguaje aceptado actualmente se
puede resumir de manera fiel con la siguiente pseudo-EBNF:

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

Notas de lectura:

- `top_level := statement` aplica solo cuando `Compiler_Parser_Options::allow_top_level_statements`
  está activo.
- `expr` usa precedence climbing; por eso la regla compacta `unary_expr (binop expr)*`
  representa una jerarquía real de precedencias definida por `Compiler_Token.H`.
- Los `enum` actuales son de variantes unitarias únicamente.
- Los `struct` actuales describen tipos nominales y campos, pero todavía no hay
  construcción de valores ni acceso a campos en expresiones.
- `type_expr` soporta nombres, tuplas y tipos función `fn(...) -> ...`.

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
- Ejemplo: `Examples/compiler_builder_example.cc`
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
- Tests: `Tests/compiler_builder_test.cc`
- Tests: `Tests/interpreter_runtime_test.cc`
- Tests: `Tests/compiler_cfg_test.cc`
- Tests: `Tests/compiler_ir_test.cc`
- Tests: `Tests/compiler_dataflow_test.cc`
- Tests: `Tests/ssa_test.cc`
- Tests: `Tests/bytecode_test.cc`
- Tests: `Tests/bytecode_interpreter_test.cc`
- Tests: `Tests/compiler_backend_c_test.cc`
- Tests: `Tests/compiler_driver_test.cc`
- Tests: `Tests/compiler_generic_driver_test.cc`

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
  compiler_builder_test \
  interpreter_runtime_test \
  compiler_cfg_test \
  compiler_ir_test \
  compiler_dataflow_test \
  ssa_test \
  bytecode_test \
  bytecode_interpreter_test \
  compiler_backend_c_test \
  compiler_driver_test \
  compiler_generic_driver_test \
  compiler_diagnostics_example \
  compiler_lexer_example \
  compiler_parser_example \
  compiler_sema_example \
  compiler_types_example \
  compiler_typed_sema_example \
  compiler_hir_example \
  compiler_builder_example \
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
