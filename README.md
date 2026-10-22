# Dapson

Compilador e intérprete de **Dapson**, un lenguaje propio hecho para el curso IS-581 Compiladores e Intérpretes (UNADECA).
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
| `make condicionales` | Corre `examples/condicionales.dap` (ejemplo de `si` / `sino`) |
| `make errores-si` | Corre `examples/errores_si.dap` (errores de condicionales a propósito) |

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
| Condicional | `si condicion: { ... } sino { ... }` (el `sino` es opcional) |

Tipos: `entero`, `decimal`, `texto`, `caracter`, `booleano`.
Operaciones: `+ - * /` con precedencia y paréntesis.
Comparaciones: `==` `!=` `>` `<` `>=` `<=` (también con palabras: `es`, `no es`, `mayor`, `menor`). El resultado es `booleano`.
Lógico: `||` (también `o`).