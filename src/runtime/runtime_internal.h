#ifndef RUNTIME_INTERNAL_H
#define RUNTIME_INTERNAL_H

/* Solo lo usan los archivos de la carpeta src/runtime */

#include "ast.h"
#include "symbol_table.h"

/*
 * Un "marco" (frame) de ejecucion: las variables que ve el codigo que corre.
 *  - global: las variables del programa
 *  - local:  los parametros y variables de la llamada actual (NULL fuera de funciones)
 * Cada llamada a una funcion crea su propio marco y lo destruye al terminar.
 */
typedef struct {
  SymbolTable *global;
  SymbolTable *local;
  int depth;     /* cuantas llamadas anidadas hay */
  int returned;  /* 1 cuando se ejecuto un 'retornar' */
  Value ret;     /* el valor retornado */
} Frame;

/* runtime.c */
int runtime_error(int line, const char *format, ...);
Symbol *runtime_find_symbol(const Frame *frame, const char *name);

/* values.c */
double runtime_as_double(const Value *value);
void runtime_widen(Value *value, DataType target);
void runtime_store(Symbol *symbol, const Value *value);
int runtime_is_comparison(Operator op);
int runtime_compare(Operator op, const Value *left, const Value *right);

/* expressions.c */
int runtime_evaluate(const ASTNode *node, Frame *frame, Value *out);

/* statements.c */
int runtime_execute_list(const ASTNode *list, Frame *frame);

/* functions.c */
int runtime_call_function(const ASTNode *node, Frame *frame, Value *out);

/* arrays.c */
int runtime_evaluate_array(const ASTNode *node, Frame *frame, Value *out);
int runtime_evaluate_index(const ASTNode *node, Frame *frame, Value *out);
int runtime_evaluate_array_call(const ASTNode *node, Frame *frame, Value *out);

#endif