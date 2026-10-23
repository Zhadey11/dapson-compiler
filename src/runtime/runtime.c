#include "runtime.h"
#include "runtime_internal.h"

#include <stdarg.h>
#include <stdio.h>

int runtime_error(int line, const char *format, ...) {
  va_list args;
  fprintf(stderr, "Error de ejecucion [linea %d]: ", line);
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputc('\n', stderr);
  return 0;
}

/* Busca desde el ambito mas interno hacia afuera: bloque -> funcion -> global */
Symbol *runtime_find_symbol(const Frame *frame, const char *name) {
  for (const SymbolTable *table = frame->local; table != NULL;
       table = table->parent) {
    Symbol *symbol = symbol_table_lookup(table, name);
    if (symbol != NULL) {
      return symbol;
    }
  }
  return symbol_table_lookup(frame->global, name);
}

int execute_program(const ASTNode *program, SymbolTable *table) {
  Frame main_frame = {table, NULL, 0, 0, {TYPE_NADA, {0}}};
  return runtime_execute_list(program, &main_frame);
}