#ifndef SEMANTIC_INTERNAL_H
#define SEMANTIC_INTERNAL_H

/* Solo lo usan los archivos de la carpeta src/semantic */

#include "ast.h"
#include "symbol_table.h"

/*
 * Ambito (scope) en el que se analiza el codigo:
 *  - global: las variables del programa y las funciones definidas
 *  - local:  los parametros y variables de la funcion actual (NULL fuera de funciones)
 *  - function: la funcion que se esta analizando (NULL fuera de funciones)
 */
typedef struct {
  SymbolTable *global;
  SymbolTable *local;
  const ASTNode *function;
} Scope;

/* semantic.c: errores y ayudas comunes */
void semantic_error(int line, const char *format, ...);
Symbol *semantic_find_symbol(const Scope *scope, const char *name);
SymbolTable *semantic_current_table(const Scope *scope);
int semantic_assignable(DataType target, DataType source);

/* expressions.c */
DataType semantic_analyze_expression(ASTNode *node, Scope *scope);
DataType semantic_analyze_value(ASTNode *node, Scope *scope);
DataType semantic_analyze_expected(ASTNode *node, DataType target, Scope *scope);

/* arrays.c */
DataType semantic_analyze_array_literal(ASTNode *node, DataType target, Scope *scope);
DataType semantic_analyze_index(ASTNode *node, Scope *scope);
DataType semantic_analyze_array_call(ASTNode *node, Scope *scope);

/* functions.c */
DataType semantic_analyze_call(ASTNode *node, Scope *scope);
void semantic_analyze_return(ASTNode *node, Scope *scope);
void semantic_analyze_function(ASTNode *node, Scope *scope);

/* statements.c */
void semantic_analyze_block(ASTNode *list, Scope *scope);

#endif