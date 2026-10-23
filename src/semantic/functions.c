#include "semantic_internal.h"

/* Analisis semantico de las funciones: definicion, llamadas y retornar */

/* Llamada a funcion: revisa que exista, la cantidad y el tipo de los argumentos */
DataType semantic_analyze_call(ASTNode *node, Scope *scope) {
  const char *name = node->data.call.name;
  Function *function = function_lookup(scope->global, name);
  int given = ast_list_length(node->data.call.args);
  int check_types = 0;

  if (function == NULL) {
    semantic_error(node->line, "la funcion '%s' no ha sido declarada", name);
  } else if (given != function->param_count) {
    semantic_error(node->line,
                   "la funcion '%s' espera %d argumento(s) y se le pasaron %d",
                   name, function->param_count, given);
  } else {
    check_types = 1;
  }

  /* Los argumentos se analizan siempre, para encontrar sus propios errores */
  int index = 0;
  for (ASTNode *arg = node->data.call.args; arg != NULL; arg = arg->next) {
    DataType type = check_types
        ? semantic_analyze_expected(arg, function->param_types[index], scope)
        : semantic_analyze_value(arg, scope);
    if (check_types && !semantic_assignable(function->param_types[index], type)) {
      semantic_error(arg->line,
                     "argumento %d de '%s': no se puede pasar un valor de tipo %s a un parametro '%s' de tipo %s",
                     index + 1, name, type_name(type), function->param_names[index],
                     type_name(function->param_types[index]));
    }
    index++;
  }

  return function != NULL ? function->return_type : TYPE_UNKNOWN;
}

void semantic_analyze_return(ASTNode *node, Scope *scope) {
  const ASTNode *function = scope->function;
  ASTNode *value = node->data.ret.value;

  if (function == NULL) {
    if (value != NULL) {
      semantic_analyze_value(value, scope);
    }
    semantic_error(node->line, "'retornar' solo se puede usar dentro de una funcion");
    return;
  }

  const char *name = function->data.func.name;
  DataType expected = function->exprType;

  if (value == NULL) {
    if (expected != TYPE_NADA) {
      semantic_error(node->line,
                     "la funcion '%s' debe retornar un valor de tipo %s", name,
                     type_name(expected));
    }
    return;
  }

  DataType actual = semantic_analyze_expected(value, expected, scope);
  node->exprType = actual;
  if (expected == TYPE_NADA) {
    semantic_error(node->line,
                   "la funcion '%s' no devuelve valor, no puede usar 'retornar' con un valor",
                   name);
  } else if (!semantic_assignable(expected, actual)) {
    semantic_error(node->line,
                   "la funcion '%s' retorna %s pero se intento retornar un valor de tipo %s",
                   name, type_name(expected), type_name(actual));
  }
}

/*
 * Una lista de sentencias "siempre retorna" si tiene un 'retornar', o un
 * 'si' con 'sino' donde AMBOS bloques siempre retornan.
 */
static int always_returns(const ASTNode *list) {
  for (const ASTNode *node = list; node != NULL; node = node->next) {
    if (node->type == NODE_RETURN) {
      return 1;
    }
    if (node->type == NODE_IF && node->data.if_stmt.else_block != NULL &&
        always_returns(node->data.if_stmt.then_block->data.block.body) &&
        always_returns(node->data.if_stmt.else_block->data.block.body)) {
      return 1;
    }
  }
  return 0;
}

void semantic_analyze_function(ASTNode *node, Scope *scope) {
  SymbolTable *global = scope->global;
  const char *name = node->data.func.name;

  /* Se registra ANTES de analizar el cuerpo, asi la funcion puede llamarse a si misma */
  SymbolTable temporary; /* ambito de una definicion repetida (se descarta) */
  symbol_table_init(&temporary);
  SymbolTable *locals = &temporary;

  Function *existing = function_lookup(global, name);
  if (existing != NULL) {
    semantic_error(node->line, "la funcion '%s' ya fue declarada en la linea %d",
                   name, existing->line);
  } else if (function_table_insert(global, node) < 0) {
    semantic_error(node->line, "sin memoria al registrar la funcion '%s'", name);
  } else {
    locals = &function_lookup(global, name)->locals;
  }

  /* Los parametros son las primeras variables locales */
  for (ASTNode *param = node->data.func.params; param != NULL; param = param->next) {
    int inserted = symbol_table_insert(locals, param->data.decl.name,
                                       param->exprType, 0, param->line);
    if (inserted == 0) {
      semantic_error(param->line,
                     "el parametro '%s' esta repetido en la funcion '%s'",
                     param->data.decl.name, name);
    } else if (inserted > 0) {
      symbol_table_lookup(locals, param->data.decl.name)->is_param = 1;
    }
  }

  Scope inner = {global, locals, node};
  semantic_analyze_block(node->data.func.body, &inner);

  /* Una funcion con tipo de retorno debe retornar un valor en todos sus caminos */
  if (node->exprType != TYPE_NADA) {
    if (!always_returns(node->data.func.body)) {
      semantic_error(node->line, "la funcion '%s' debe retornar un valor de tipo %s",
                     name, type_name(node->exprType));
    }
  }

  symbol_table_destroy(&temporary);
}