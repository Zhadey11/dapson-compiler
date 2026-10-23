#include "semantic.h"
#include "semantic_internal.h"

#include <stdarg.h>
#include <stdio.h>

static int error_count = 0;

void semantic_error(int line, const char *format, ...) {
  va_list args;
  error_count++;
  fprintf(stderr, "Error semantico [linea %d]: ", line);
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputc('\n', stderr);
}

/* Busca desde el ambito mas interno hacia afuera: bloque -> funcion -> global */
Symbol *semantic_find_symbol(const Scope *scope, const char *name) {
  for (const SymbolTable *table = scope->local; table != NULL;
       table = table->parent) {
    Symbol *symbol = symbol_table_lookup(table, name);
    if (symbol != NULL) {
      return symbol;
    }
  }
  return symbol_table_lookup(scope->global, name);
}

/* La tabla donde se declaran las variables nuevas */
SymbolTable *semantic_current_table(const Scope *scope) {
  return scope->local != NULL ? scope->local : scope->global;
}

/* Un valor "source" cabe en una variable "target"? (entero -> decimal si) */
int semantic_assignable(DataType target, DataType source) {
  if (target == TYPE_UNKNOWN || source == TYPE_UNKNOWN) {
    return 1; /* ya se reporto otro error, no se repite */
  }
  if (target == source) {
    return 1;
  }
  return target == TYPE_DECIMAL && source == TYPE_ENTERO;
}

int semantic_analyze(ASTNode *program, SymbolTable *table) {
  error_count = 0;

  Scope global = {table, NULL, NULL};
  semantic_analyze_block(program, &global);
  return error_count;
}