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

## Carpetas

```
include/     encabezados públicos (.h)
src/ast/     árbol sintáctico
src/lexer/   análisis léxico (Flex)
src/parser/  análisis sintáctico (Bison)
src/semantic/ análisis semántico
src/runtime/ ejecución
examples/    programas de ejemplo
```