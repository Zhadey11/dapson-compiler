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

typedef struct Symbol {
  char *name;
  DataType type;
  int is_const;
  int line;        /* linea donde se declaro */
  int initialized; /* lo marca el runtime al asignar un valor */
  Value value;     /* valor actual (runtime) */
  struct Symbol *next;
} Symbol;

typedef struct {
  Symbol *head;
  Symbol *tail;
  int count;
} SymbolTable;

void symbol_table_init(SymbolTable *table);
/* 1 = insertado, 0 = ya existia, -1 = sin memoria */
int symbol_table_insert(SymbolTable *table, const char *name, DataType type,
                        int is_const, int line);
Symbol *symbol_table_lookup(const SymbolTable *table, const char *name);
void symbol_table_print(const SymbolTable *table);
void symbol_table_destroy(SymbolTable *table);

/* Convierte un valor a texto (quote = 1 pone comillas a texto y caracter) */
void value_to_string(const Value *value, char *out, size_t size, int quote);

#endif
