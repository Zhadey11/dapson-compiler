#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "ast.h"
#include <stddef.h>

/* Valor en tiempo de ejecucion */
typedef struct {
  DataType type;
  union {
    long long int_value;
    double decimal_value;
    const char *text_value;
    char char_value;
    int bool_value;
  } data;
} Value;

struct Function;

typedef struct Symbol {
  char *name;
  DataType type;
  int is_const;
  int line;        /* linea donde se declaro */
  int is_param;    /* 1 si es parametro de una funcion */
  int initialized; /* lo marca el runtime al asignar un valor */
  Value value;     /* valor actual (runtime) */
  struct Symbol *next;
} Symbol;

typedef struct SymbolTable {
  Symbol *head;
  Symbol *tail;
  int count;
  /* tabla del ambito que contiene a este (un bloque "si" dentro de otro
     bloque o de una funcion); NULL si no hay */
  struct SymbolTable *parent;
  /* funciones definidas (solo la tabla global las usa) */
  struct Function *func_head;
  struct Function *func_tail;
  int func_count;
} SymbolTable;

/* Una funcion registrada: firma + su propio ambito (variables locales) */
typedef struct Function {
  char *name;
  DataType return_type; /* TYPE_NADA si no devuelve valor */
  int param_count;
  DataType *param_types;
  char **param_names;
  int line;
  const ASTNode *node; /* definicion en el AST (cuerpo de la funcion) */
  SymbolTable locals;  /* parametros y variables locales (lo llena el semantico) */
  struct Function *next;
} Function;

void symbol_table_init(SymbolTable *table);
/* 1 = insertado, 0 = ya existia, -1 = sin memoria */
int symbol_table_insert(SymbolTable *table, const char *name, DataType type,
                        int is_const, int line);
Symbol *symbol_table_lookup(const SymbolTable *table, const char *name);
void symbol_table_print(const SymbolTable *table);
void symbol_table_destroy(SymbolTable *table);

/* Funciones: 1 = registrada, 0 = ya existia, -1 = sin memoria */
int function_table_insert(SymbolTable *table, const ASTNode *definition);
Function *function_lookup(const SymbolTable *table, const char *name);
/* Imprime las funciones y el ambito (tabla de simbolos) de cada una */
void function_table_print(const SymbolTable *table);

/* Convierte un valor a texto (quote = 1 pone comillas a texto y caracter) */
void value_to_string(const Value *value, char *out, size_t size, int quote);

#endif