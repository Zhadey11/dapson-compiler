#include "runtime_internal.h"

#include <stdio.h>

/* Ejecucion de sentencias: declarar, asignar, imprimir, si y retornar */

/* Ejecuta una lista de sentencias; se detiene si una ejecuta 'retornar' */
int runtime_execute_list(const ASTNode *list, Frame *frame) {
  for (const ASTNode *node = list; node != NULL && !frame->returned;
       node = node->next) {
    Value value;

    switch (node->type) {
    case NODE_VAR_DECL:
    case NODE_CONST_DECL: {
      /* Dentro de una funcion la variable se crea aqui; en el programa
         principal ya la registro el analizador semantico. */
      if (!runtime_evaluate(node->data.decl.init, frame, &value)) {
        return 0;
      }
      SymbolTable *table = frame->local != NULL ? frame->local : frame->global;
      Symbol *symbol = symbol_table_lookup(table, node->data.decl.name);
      if (symbol == NULL) {
        if (symbol_table_insert(table, node->data.decl.name, node->exprType,
                                node->type == NODE_CONST_DECL, node->line) < 0) {
          return runtime_error(node->line, "sin memoria al registrar '%s'",
                               node->data.decl.name);
        }
        symbol = symbol_table_lookup(table, node->data.decl.name);
      }
      runtime_store(symbol, &value);
      break;
    }
    case NODE_ASSIGN: {
      Symbol *symbol = runtime_find_symbol(frame, node->data.assign.name);
      if (symbol == NULL || !runtime_evaluate(node->data.assign.value, frame, &value)) {
        return 0;
      }
      runtime_store(symbol, &value);
      break;
    }
    case NODE_PRINT: {
      char text[4096];
      if (!runtime_evaluate(node->data.print.value, frame, &value)) {
        return 0;
      }
      value_to_string(&value, text, sizeof(text), 0);
      puts(text);
      break;
    }
    case NODE_CALL:
    case NODE_ARRAY_CALL:
      if (!runtime_evaluate(node, frame, &value)) { /* el valor, si hay, se descarta */
        return 0;
      }
      break;
    case NODE_IF: {
      if (!runtime_evaluate(node->data.if_stmt.cond, frame, &value)) {
        return 0;
      }
      const ASTNode *chosen = value.data.bool_value ? node->data.if_stmt.then_block
                                                    : node->data.if_stmt.else_block;
      if (chosen != NULL) {
        /* el bloque tiene su propio ambito: sus variables mueren al terminar */
        SymbolTable block_table;
        symbol_table_init(&block_table);
        block_table.parent = frame->local;
        frame->local = &block_table;
        int ok = runtime_execute_list(chosen->data.block.body, frame);
        frame->local = block_table.parent;
        symbol_table_destroy(&block_table);
        if (!ok) {
          return 0;
        }
      }
      break;
    }
    case NODE_RETURN:
      if (node->data.ret.value != NULL) {
        if (!runtime_evaluate(node->data.ret.value, frame, &frame->ret)) {
          return 0;
        }
      }
      frame->returned = 1;
      break;
    default: /* las definiciones de funciones ya estan registradas */
      break;
    }
  }
  return 1;
}