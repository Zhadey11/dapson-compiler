#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symbol_table.h"

/*
 * Analizador semantico: recorre el AST, llena la tabla de simbolos,
 * calcula el tipo de cada expresion y reporta los errores de significado.
 * Devuelve la cantidad de errores encontrados (0 = programa correcto).
 */
int semantic_analyze(ASTNode *program, SymbolTable *table);

#endif
