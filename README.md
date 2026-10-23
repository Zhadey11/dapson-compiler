# Dapson

Compilador e intérprete de **Dapson**, un lenguaje en español hecho con Flex, Bison y C.

## Requisitos

```
sudo apt install gcc flex bison make
```

## Compilar

```
make
```

Esto crea el ejecutable `dapson` en la raíz del proyecto. Para borrar lo compilado: `make clean`.

## Usar

```
./dapson archivo.dap       # ejecuta el programa
./dapson -d archivo.dap    # además muestra el AST y la tabla de símbolos
```

## Comandos de ejemplo

| Comando | Qué hace |
|---|---|
| `make run` | Ejecuta `examples/programa.dap` |
| `make debug` | Igual, mostrando AST y tabla de símbolos |
| `make errores` | Errores semánticos a propósito |
| `make funciones` | Ejemplo con funciones |
| `make errores-funciones` | Errores de funciones a propósito |
| `make condicionales` | Ejemplo con `si` / `sino` |
| `make errores-si` | Errores de condicionales a propósito |
| `make arreglos` | Ejemplo con arreglos |
| `make errores-arreglos` | Errores de arreglos a propósito |
| `make ajedrez` | Una partida de ajedrez de ejemplo |
| `make errores-ajedrez` | Una jugada ilegal a propósito |

## Reglas del lenguaje

| Qué | Cómo se escribe |
|---|---|
| Variable | `sea nombre: tipo = valor;` |
| Constante | `fijo nombre: tipo = valor;` |
| Asignación | `nombre = valor;` |
| Imprimir | `imprimir expresion;` |
| Tipos | `entero` `decimal` `texto` `caracter` `booleano` |
| Función | `funcion nombre(a: tipo, b: tipo): tipo { ... }` |
| Retornar | `retornar expresion;` |
| Función sin valor | `funcion nombre() { ... }` (o `: nada`) |
| Condicional | `si condicion: { ... } sino { ... }` |
| Comparaciones | `==` `!=` `>` `<` `>=` `<=` (también `es`, `no es`, `mayor`, `menor`) |
| O lógico | `\|\|` (o `o`) |
| Tipo arreglo | `entero[]` `decimal[]` `texto[]` `caracter[]` `booleano[]` |
| Arreglo literal | `[10, 20, 30]` o vacío `[]` |
| Leer elemento | `lista[i]` (la primera posición es 0) |
| Agregar al final | `lista.agregar(x);` |
| Quitar el último | `lista.quitar()` (devuelve el elemento) |
| Cantidad | `lista.largo()` |
| Mover una pieza | `mover peon_b5 hacia e4;` |

## Ajedrez

Las piezas blancas terminan en `_b` y las negras en `_n`. Las blancas siempre empiezan.

| Pieza | Nombres |
|---|---|
| Peones | `peon_b1` … `peon_b8`, `peon_n1` … `peon_n8` |
| Torres | `torre_b1`, `torre_b2`, `torre_n1`, `torre_n2` |
| Caballos | `caballo_b1`, `caballo_b2`, `caballo_n1`, `caballo_n2` |
| Alfiles | `alfil_b1`, `alfil_b2`, `alfil_n1`, `alfil_n2` |
| Reinas | `reina_b`, `reina_n` |
| Reyes | `rey_b`, `rey_n` |

Las casillas van de `a1` a `h8`. Los nombres de las piezas, las casillas y las palabras `mover` y `hacia` no se pueden usar como nombres de variable.

Cada `mover` se valida al ejecutar: que la pieza siga viva, que sea su turno, que el movimiento sea el de esa pieza, que no haya piezas en el camino y que no caiga sobre una pieza propia. Una jugada ilegal detiene el programa con un error.

Todavía no incluye: jaque, enroque, peón al paso ni coronación.

## Carpetas

```
include/        encabezados públicos (.h)
src/ast/        árbol sintáctico
src/chess/      tablero y reglas del ajedrez
src/lexer/      análisis léxico (Flex)
src/parser/     análisis sintáctico (Bison)
src/semantic/   análisis semántico
src/runtime/    ejecución
examples/       programas de ejemplo
```