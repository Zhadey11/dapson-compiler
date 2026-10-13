#include "semantic.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int error_count = 0;

static void semantic_error(int line, const char *format, ...) {
  va_list args;
  error_count++;
  fprintf(stderr, "Error semantico [linea %d]: ", line);
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputc('\n', stderr);
}

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

/* Busca primero en el ambito local y despues en el global */
static Symbol *find_symbol(const Scope *scope, const char *name) {
  Symbol *symbol = NULL;
  if (scope->local != NULL) {
    symbol = symbol_table_lookup(scope->local, name);
  }
  if (symbol == NULL) {
    symbol = symbol_table_lookup(scope->global, name);
  }
  return symbol;
}

/* La tabla donde se declaran las variables nuevas */
static SymbolTable *current_table(const Scope *scope) {
  return scope->local != NULL ? scope->local : scope->global;
}

static int is_numeric(DataType type) {
  return type == TYPE_ENTERO || type == TYPE_DECIMAL;
}

/* Un valor "source" cabe en una variable "target"? (entero -> decimal si) */
static int assignable(DataType target, DataType source) {
  if (target == TYPE_UNKNOWN || source == TYPE_UNKNOWN) {
    return 1; /* ya se reporto otro error, no se repite */
  }
  if (target == source) {
    return 1;
  }
  return target == TYPE_DECIMAL && source == TYPE_ENTERO;
}

static int is_zero_literal(const ASTNode *node) {
  return (node->type == NODE_INT && node->data.intValue == 0) ||
         (node->type == NODE_DECIMAL && node->data.decimalValue == 0.0);
}

static DataType analyze_expression(ASTNode *node, Scope *scope);
static void analyze_block(ASTNode *list, Scope *scope);

/*
 * Igual que analyze_expression, pero exige que haya un valor: llamar a una
 * funcion que no devuelve nada dentro de una expresion es un error.
 */
static DataType analyze_value(ASTNode *node, Scope *scope) {
  DataType type = analyze_expression(node, scope);
  if (type == TYPE_NADA) {
    semantic_error(node->line, "la funcion '%s' no devuelve ningun valor",
                   node->data.call.name);
    node->exprType = TYPE_UNKNOWN;
    return TYPE_UNKNOWN;
  }
  return type;
}

/* Llamada a funcion: revisa que exista, la cantidad y el tipo de los argumentos */
static DataType analyze_call(ASTNode *node, Scope *scope) {
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
    DataType type = analyze_value(arg, scope);
    if (check_types && !assignable(function->param_types[index], type)) {
      semantic_error(arg->line,
                     "argumento %d de '%s': no se puede pasar un valor de tipo %s a un parametro '%s' de tipo %s",
                     index + 1, name, type_name(type), function->param_names[index],
                     type_name(function->param_types[index]));
    }
    index++;
  }

  return function != NULL ? function->return_type : TYPE_UNKNOWN;
}

/* Calcula y guarda el tipo de una expresion */
static DataType analyze_expression(ASTNode *node, Scope *scope) {
  DataType result = TYPE_UNKNOWN;

  switch (node->type) {
  case NODE_INT:
    result = TYPE_ENTERO;
    break;
  case NODE_DECIMAL:
    result = TYPE_DECIMAL;
    break;
  case NODE_TEXT:
    result = TYPE_TEXTO;
    break;
  case NODE_CHAR:
    result = TYPE_CARACTER;
    break;
  case NODE_BOOL:
    result = TYPE_BOOLEANO;
    break;

  case NODE_IDENT: {
    Symbol *symbol = find_symbol(scope, node->data.identifier);
    if (symbol == NULL) {
      semantic_error(node->line, "la variable '%s' no ha sido declarada",
                     node->data.identifier);
    } else {
      result = symbol->type;
    }
    break;
  }

  case NODE_CALL:
    result = analyze_call(node, scope);
    break;

  case NODE_UNARY: {
    DataType operand = analyze_value(node->data.binary.left, scope);
    if (operand != TYPE_UNKNOWN && !is_numeric(operand)) {
      semantic_error(node->line,
                     "el operador '-' unario requiere un numero, se encontro %s",
                     type_name(operand));
    } else {
      result = operand;
    }
    break;
  }

  case NODE_BINARY: {
    DataType left = analyze_value(node->data.binary.left, scope);
    DataType right = analyze_value(node->data.binary.right, scope);
    Operator op = node->data.binary.op;

    if (left == TYPE_UNKNOWN || right == TYPE_UNKNOWN) {
      break; /* error ya reportado: no se encadenan mas errores */
    }
    if (!is_numeric(left) || !is_numeric(right)) {
      semantic_error(node->line,
                     "el operador '%s' requiere operandos numericos, se encontro %s y %s",
                     operator_symbol(op), type_name(left), type_name(right));
      break;
    }
    if (op == OP_DIV && is_zero_literal(node->data.binary.right)) {
      semantic_error(node->line, "division entre cero");
    }
    /* entero (op) entero = entero;  si alguno es decimal = decimal */
    result = (left == TYPE_DECIMAL || right == TYPE_DECIMAL) ? TYPE_DECIMAL
                                                             : TYPE_ENTERO;
    break;
  }

  /* las sentencias no son expresiones */
  case NODE_VAR_DECL:
  case NODE_CONST_DECL:
  case NODE_ASSIGN:
  case NODE_PRINT:
  case NODE_FUNC_DECL:
  case NODE_PARAM:
  case NODE_RETURN:
    break;
  }

  node->exprType = result;
  return result;
}

static void analyze_declaration(ASTNode *node, Scope *scope) {
  int is_const = node->type == NODE_CONST_DECL;
  const char *name = node->data.decl.name;
  DataType declared = node->exprType;
  SymbolTable *table = current_table(scope);

  /* primero la expresion: asi "sea x: entero = x;" detecta que x no existe */
  DataType value = analyze_value(node->data.decl.init, scope);

  if (!assignable(declared, value)) {
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
  Symbol *symbol = find_symbol(scope, name);
  DataType value = analyze_value(node->data.assign.value, scope);

  if (symbol == NULL) {
    semantic_error(node->line, "la variable '%s' no ha sido declarada", name);
    return;
  }
  if (symbol->is_const) {
    semantic_error(node->line,
                   "'%s' es una constante (fijo) y no se puede reasignar", name);
    return;
  }
  if (!assignable(symbol->type, value)) {
    semantic_error(node->line,
                   "no se puede asignar un valor de tipo %s a '%s' (tipo %s)",
                   type_name(value), name, type_name(symbol->type));
  }
  node->exprType = symbol->type;
}

static void analyze_return(ASTNode *node, Scope *scope) {
  const ASTNode *function = scope->function;
  ASTNode *value = node->data.ret.value;

  if (function == NULL) {
    if (value != NULL) {
      analyze_value(value, scope);
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

  DataType actual = analyze_value(value, scope);
  node->exprType = actual;
  if (expected == TYPE_NADA) {
    semantic_error(node->line,
                   "la funcion '%s' no devuelve valor, no puede usar 'retornar' con un valor",
                   name);
  } else if (!assignable(expected, actual)) {
    semantic_error(node->line,
                   "la funcion '%s' retorna %s pero se intento retornar un valor de tipo %s",
                   name, type_name(expected), type_name(actual));
  }
}

static void analyze_function(ASTNode *node, Scope *scope) {
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
  analyze_block(node->data.func.body, &inner);

  /* Una funcion con tipo de retorno necesita al menos un 'retornar' */
  if (node->exprType != TYPE_NADA) {
    int has_return = 0;
    for (const ASTNode *statement = node->data.func.body; statement != NULL;
         statement = statement->next) {
      if (statement->type == NODE_RETURN) {
        has_return = 1;
      }
    }
    if (!has_return) {
      semantic_error(node->line, "la funcion '%s' debe retornar un valor de tipo %s",
                     name, type_name(node->exprType));
    }
  }

  symbol_table_destroy(&temporary);
}

static void analyze_block(ASTNode *list, Scope *scope) {
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
      analyze_value(node->data.print.value, scope);
      break;
    case NODE_CALL:
      analyze_expression(node, scope); /* como sentencia, el valor se descarta */
      break;
    case NODE_RETURN:
      analyze_return(node, scope);
      break;
    case NODE_FUNC_DECL:
      analyze_function(node, scope);
      break;
    default:
      break;
    }
  }
}

int semantic_analyze(ASTNode *program, SymbolTable *table) {
  error_count = 0;

  Scope global = {table, NULL, NULL};
  analyze_block(program, &global);
  return error_count;
}