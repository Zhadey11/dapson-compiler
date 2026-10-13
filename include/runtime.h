#ifndef RUNTIME_H
#define RUNTIME_H

#include "ast.h"
#include "symbol_table.h"

/* Ejecuta el programa. Devuelve 1 si termino bien, 0 si hubo un error. */
int execute_program(const ASTNode *program, SymbolTable *table);

#endif
