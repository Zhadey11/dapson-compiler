#include "semantic_internal.h"

/* Analisis semantico de las sentencias: declarar, asignar, si y bloques */

static void analyze_declaration(ASTNode *node, Scope *scope) {
  int is_const = node->type == NODE_CONST_DECL;
  const char *name = node->data.decl.name;
  DataType declared = node->exprType;
  SymbolTable *table = semantic_current_table(scope);

  /* primero la expresion: asi "sea x: entero = x;" detecta que x no existe */
  DataType value = semantic_analyze_expected(node->data.decl.init, declared, scope);

  if (!semantic_assignable(declared, value)) {
    semantic_error(node->line,
                   "no se puede asignar un valor de tipo %s a '%s' (tipo %s)",
                   type_name(value), name, type_name(declared));
  }

  int inserted = symbol_table_insert(table, name, declared, is_const, node->line);
  if (inserted == 0) {
    Symbol *previous = symbol_table_lookup(table, name);
    semantic_error(node->line,
                   "'%s' ya fue declarada en la linea %d", name,
                   previous != NULL ? previous->line : 0);
  } else if (inserted < 0) {
    semantic_error(node->line, "sin memoria al registrar '%s'", name);
  }
}

static void analyze_assignment(ASTNode *node, Scope *scope) {
  const char *name = node->data.assign.name;
  Symbol *symbol = semantic_find_symbol(scope, name);
  DataType value = semantic_analyze_expected(
      node->data.assign.value, symbol != NULL ? symbol->type : TYPE_UNKNOWN, scope);

  if (symbol == NULL) {
    semantic_error(node->line, "la variable '%s' no ha sido declarada", name);
    return;
  }
  if (symbol->is_const) {
    semantic_error(node->line,
                   "'%s' es una constante (fijo) y no se puede reasignar", name);
    return;
  }
  if (!semantic_assignable(symbol->type, value)) {
    semantic_error(node->line,
                   "no se puede asignar un valor de tipo %s a '%s' (tipo %s)",
                   type_name(value), name, type_name(symbol->type));
  }
  node->exprType = symbol->type;
}

/* Analiza las sentencias de un bloque en un ambito propio (sus variables
   solo existen dentro del bloque) */
static void analyze_scoped_block(const ASTNode *block, Scope *scope) {
  SymbolTable inner_table;
  symbol_table_init(&inner_table);
  inner_table.parent = scope->local;

  Scope inner = *scope;
  inner.local = &inner_table;
  semantic_analyze_block(block->data.block.body, &inner);

  symbol_table_destroy(&inner_table);
}

static void analyze_if(ASTNode *node, Scope *scope) {
  DataType condition = semantic_analyze_value(node->data.if_stmt.cond, scope);
  if (condition != TYPE_UNKNOWN && condition != TYPE_BOOLEANO) {
    semantic_error(node->line,
                   "la condicion del 'si' debe ser booleana, se encontro %s",
                   type_name(condition));
  }

  analyze_scoped_block(node->data.if_stmt.then_block, scope);
  if (node->data.if_stmt.else_block != NULL) {
    analyze_scoped_block(node->data.if_stmt.else_block, scope);
  }
}

void semantic_analyze_block(ASTNode *list, Scope *scope) {
  for (ASTNode *node = list; node != NULL; node = node->next) {
    switch (node->type) {
    case NODE_VAR_DECL:
    case NODE_CONST_DECL:
      analyze_declaration(node, scope);
      break;
    case NODE_ASSIGN:
      analyze_assignment(node, scope);
      break;
    case NODE_PRINT:
      semantic_analyze_value(node->data.print.value, scope);
      break;
    case NODE_CALL:
    case NODE_ARRAY_CALL:
      semantic_analyze_expression(node, scope); /* como sentencia, el valor se descarta */
      break;
    case NODE_RETURN:
      semantic_analyze_return(node, scope);
      break;
    case NODE_IF:
      analyze_if(node, scope);
      break;
    case NODE_FUNC_DECL:
      semantic_analyze_function(node, scope);
      break;
    default:
      break;
    }
  }
}