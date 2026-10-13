# Dapson

Compilador e intérprete de **Dapson**, un lenguaje propio.
Autora: Jaydanie Dapson Morris.

Fases: analizador léxico (Flex) → analizador sintáctico (Bison) → AST + AST Table → tabla de símbolos → análisis semántico → ejecución.

## Requisitos

`gcc`, `flex`, `bison` y `make`.

En Linux Mint / Ubuntu:

```
sudo apt install gcc flex bison make
```

## Compilar

```
make
```

Esto genera el ejecutable `dapson`. Para borrar lo compilado: `make clean`.

## Correr los ejemplos

| Comando | Qué hace |
|---|---|
| `make run` | Corre `examples/programa.dap` |
| `make debug` | Igual, mostrando el AST, la AST Table y la tabla de símbolos |
| `make errores` | Corre `examples/errores.dap` (errores semánticos a propósito) |
| `make funciones` | Corre `examples/funciones.dap` (ejemplo de funciones) |
| `make errores-funciones` | Corre `examples/errores_funciones.dap` (errores de funciones a propósito) |

Para correr cualquier otro archivo:

```
./dapson archivo.dap
./dapson -d archivo.dap
```

`-d` muestra el árbol, las tablas y la ejecución paso a paso.

## Reglas del lenguaje

| Regla | Sintaxis |
|---|---|
| Variable | `sea nombre: tipo = valor;` |
| Constante | `fijo nombre: tipo = valor;` |
| Asignación | `nombre = valor;` |
| Salida | `imprimir expresion;` |
| Función | `funcion nombre(a: tipo, b: tipo): tipo { ... }` |
| Retorno | `retornar expresion;` |
| Llamada | `nombre(argumentos)` |

Tipos: `entero`, `decimal`, `texto`, `caracter`, `booleano`.
Operaciones: `+ - * /` con precedencia y paréntesis.