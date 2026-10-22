#include "runtime.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Limite de llamadas anidadas (evita desbordar la pila con recursion infinita) */
#define MAX_CALL_DEPTH 500

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

static int runtime_error(int line, const char *format, ...) {
  va_list args;
  fprintf(stderr, "Error de ejecucion [linea %d]: ", line);
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputc('\n', stderr);
  return 0;
}

static double as_double(const Value *value) {
  return value->type == TYPE_ENTERO ? (double)value->data.int_value
                                    : value->data.decimal_value;
}

/* Busca desde el ambito mas interno hacia afuera: bloque -> funcion -> global */
static Symbol *find_symbol(const Frame *frame, const char *name) {
  for (const SymbolTable *table = frame->local; table != NULL;
       table = table->parent) {
    Symbol *symbol = symbol_table_lookup(table, name);
    if (symbol != NULL) {
      return symbol;
    }
  }
  return symbol_table_lookup(frame->global, name);
}

/* Guarda un valor en una variable (un entero se convierte a decimal si hace falta) */
static void store_value(Symbol *symbol, const Value *value) {
  symbol->value = *value;
  if (symbol->type == TYPE_DECIMAL && value->type == TYPE_ENTERO) {
    symbol->value.type = TYPE_DECIMAL;
    symbol->value.data.decimal_value = (double)value->data.int_value;
  }
  symbol->initialized = 1;
}

static int evaluate(const ASTNode *node, Frame *frame, Value *out);
static int execute_list(const ASTNode *list, Frame *frame);

static int is_comparison(Operator op) {
  return op == OP_EQ || op == OP_NE || op == OP_GT || op == OP_LT ||
         op == OP_GE || op == OP_LE;
}

/* Convierte el resultado de comparar (menor < 0, igual = 0, mayor > 0) segun el operador */
static int compare_result(Operator op, int order) {
  switch (op) {
  case OP_EQ: return order == 0;
  case OP_NE: return order != 0;
  case OP_GT: return order > 0;
  case OP_LT: return order < 0;
  case OP_GE: return order >= 0;
  case OP_LE: return order <= 0;
  default: return 0;
  }
}

/* Compara dos valores ya validados por el analizador semantico */
static int compare_values(Operator op, const Value *left, const Value *right) {
  int order = 0;
  if ((left->type == TYPE_ENTERO || left->type == TYPE_DECIMAL) &&
      (right->type == TYPE_ENTERO || right->type == TYPE_DECIMAL)) {
    if (left->type == TYPE_ENTERO && right->type == TYPE_ENTERO) {
      long long a = left->data.int_value;
      long long b = right->data.int_value;
      order = (a > b) - (a < b);
    } else {
      double a = as_double(left);
      double b = as_double(right);
      order = (a > b) - (a < b);
    }
  } else if (left->type == TYPE_TEXTO) {
    order = strcmp(left->data.text_value, right->data.text_value);
  } else if (left->type == TYPE_CARACTER) {
    order = (int)left->data.char_value - (int)right->data.char_value;
  } else if (left->type == TYPE_BOOLEANO) {
    order = left->data.bool_value - right->data.bool_value;
  }
  return compare_result(op, order);
}

/* Llama a una funcion: evalua los argumentos, crea un marco nuevo y ejecuta el cuerpo */
static int call_function(const ASTNode *node, Frame *frame, Value *out) {
  const char *name = node->data.call.name;
  Function *function = function_lookup(frame->global, name);
  if (function == NULL) {
    return runtime_error(node->line, "la funcion '%s' no existe", name);
  }
  if (frame->depth >= MAX_CALL_DEPTH) {
    return runtime_error(node->line, "demasiadas llamadas anidadas en '%s'", name);
  }

  /* 1. los argumentos se evaluan en el marco de quien llama */
  Value *args = calloc((size_t)function->param_count + 1, sizeof(Value));
  if (args == NULL) {
    return runtime_error(node->line, "sin memoria al llamar a '%s'", name);
  }
  int index = 0;
  for (const ASTNode *arg = node->data.call.args; arg != NULL; arg = arg->next) {
    if (!evaluate(arg, frame, &args[index])) {
      free(args);
      return 0;
    }
    index++;
  }

  /* 2. marco nuevo: los parametros son sus primeras variables */
  SymbolTable locals;
  symbol_table_init(&locals);
  Frame callee = {frame->global, &locals, frame->depth + 1, 0, {TYPE_NADA, {0}}};
  int ok = 1;
  for (int i = 0; i < function->param_count && ok; i++) {
    if (symbol_table_insert(&locals, function->param_names[i],
                            function->param_types[i], 0, function->line) <= 0) {
      ok = runtime_error(node->line, "sin memoria al llamar a '%s'", name);
    } else {
      Symbol *param = symbol_table_lookup(&locals, function->param_names[i]);
      param->is_param = 1;
      store_value(param, &args[i]);
    }
  }
  free(args);

  /* 3. se ejecuta el cuerpo de la funcion */
  if (ok) {
    ok = execute_list(function->node->data.func.body, &callee);
  }

  /* 4. el resultado: lo que dijo 'retornar' (un entero se ensancha a decimal) */
  if (ok) {
    if (function->return_type == TYPE_NADA) {
      out->type = TYPE_NADA;
    } else if (!callee.returned) {
      ok = runtime_error(node->line, "la funcion '%s' termino sin retornar un valor", name);
    } else {
      *out = callee.ret;
      if (function->return_type == TYPE_DECIMAL && out->type == TYPE_ENTERO) {
        out->type = TYPE_DECIMAL;
        out->data.decimal_value = (double)out->data.int_value;
      }
    }
  }

  symbol_table_destroy(&locals);
  return ok;
}

static int evaluate(const ASTNode *node, Frame *frame, Value *out) {
  switch (node->type) {
  case NODE_INT:
    out->type = TYPE_ENTERO;
    out->data.int_value = node->data.intValue;
    return 1;
  case NODE_DECIMAL:
    out->type = TYPE_DECIMAL;
    out->data.decimal_value = node->data.decimalValue;
    return 1;
  case NODE_TEXT:
    out->type = TYPE_TEXTO;
    out->data.text_value = node->data.strValue;
    return 1;
  case NODE_CHAR:
    out->type = TYPE_CARACTER;
    out->data.char_value = node->data.charValue;
    return 1;
  case NODE_BOOL:
    out->type = TYPE_BOOLEANO;
    out->data.bool_value = node->data.boolValue;
    return 1;

  case NODE_IDENT: {
    Symbol *symbol = find_symbol(frame, node->data.identifier);
    if (symbol == NULL || !symbol->initialized) {
      return runtime_error(node->line, "'%s' no tiene valor", node->data.identifier);
    }
    *out = symbol->value;
    return 1;
  }

  case NODE_CALL:
    return call_function(node, frame, out);

  case NODE_UNARY: {
    Value operand;
    if (!evaluate(node->data.binary.left, frame, &operand)) {
      return 0;
    }
    *out = operand;
    if (operand.type == TYPE_ENTERO) {
      if (operand.data.int_value == LLONG_MIN) {
        return runtime_error(node->line, "desbordamiento en la operacion '-'");
      }
      out->data.int_value = -operand.data.int_value;
    } else {
      out->data.decimal_value = -operand.data.decimal_value;
    }
    return 1;
  }

  case NODE_BINARY: {
    Value left, right;
    Operator op = node->data.binary.op;

    /* "o": si el lado izquierdo ya es verdadero, el derecho ni se evalua */
    if (op == OP_OR) {
      if (!evaluate(node->data.binary.left, frame, &left)) {
        return 0;
      }
      out->type = TYPE_BOOLEANO;
      if (left.data.bool_value) {
        out->data.bool_value = 1;
        return 1;
      }
      if (!evaluate(node->data.binary.right, frame, &right)) {
        return 0;
      }
      out->data.bool_value = right.data.bool_value;
      return 1;
    }

    if (!evaluate(node->data.binary.left, frame, &left) ||
        !evaluate(node->data.binary.right, frame, &right)) {
      return 0;
    }

    /* Comparaciones: el resultado siempre es booleano */
    if (is_comparison(op)) {
      out->type = TYPE_BOOLEANO;
      out->data.bool_value = compare_values(op, &left, &right);
      return 1;
    }

    /* Si alguno es decimal, toda la operacion es decimal */
    if (left.type == TYPE_DECIMAL || right.type == TYPE_DECIMAL) {
      double a = as_double(&left);
      double b = as_double(&right);
      if (op == OP_DIV && b == 0.0) {
        return runtime_error(node->line, "division entre cero");
      }
      out->type = TYPE_DECIMAL;
      switch (op) {
      case OP_ADD: out->data.decimal_value = a + b; break;
      case OP_SUB: out->data.decimal_value = a - b; break;
      case OP_MUL: out->data.decimal_value = a * b; break;
      case OP_DIV: out->data.decimal_value = a / b; break;
      default: break;
      }
      return 1;
    }

    long long a = left.data.int_value;
    long long b = right.data.int_value;
    long long result = 0;
    int overflow = 0;
    switch (op) {
    case OP_ADD: overflow = __builtin_add_overflow(a, b, &result); break;
    case OP_SUB: overflow = __builtin_sub_overflow(a, b, &result); break;
    case OP_MUL: overflow = __builtin_mul_overflow(a, b, &result); break;
    case OP_DIV:
      if (b == 0) {
        return runtime_error(node->line, "division entre cero");
      }
      if (a == LLONG_MIN && b == -1) {
        overflow = 1;
      } else {
        result = a / b; /* division entera, como en C */
      }
      break;
    default: break;
    }
    if (overflow) {
      return runtime_error(node->line, "desbordamiento en la operacion '%s'",
                           operator_symbol(op));
    }
    out->type = TYPE_ENTERO;
    out->data.int_value = result;
    return 1;
  }

  case NODE_VAR_DECL:
  case NODE_CONST_DECL:
  case NODE_ASSIGN:
  case NODE_PRINT:
  case NODE_FUNC_DECL:
  case NODE_PARAM:
  case NODE_RETURN:
  case NODE_IF:
  case NODE_BLOCK:
    break;
  }
  return runtime_error(node->line, "expresion invalida");
}

/* Ejecuta una lista de sentencias; se detiene si una ejecuta 'retornar' */
static int execute_list(const ASTNode *list, Frame *frame) {
  for (const ASTNode *node = list; node != NULL && !frame->returned;
       node = node->next) {
    Value value;

    switch (node->type) {
    case NODE_VAR_DECL:
    case NODE_CONST_DECL: {
      /* Dentro de una funcion la variable se crea aqui; en el programa
         principal ya la registro el analizador semantico. */
      if (!evaluate(node->data.decl.init, frame, &value)) {
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
      store_value(symbol, &value);
      break;
    }
    case NODE_ASSIGN: {
      Symbol *symbol = find_symbol(frame, node->data.assign.name);
      if (symbol == NULL || !evaluate(node->data.assign.value, frame, &value)) {
        return 0;
      }
      store_value(symbol, &value);
      break;
    }
    case NODE_PRINT: {
      char text[512];
      if (!evaluate(node->data.print.value, frame, &value)) {
        return 0;
      }
      value_to_string(&value, text, sizeof(text), 0);
      puts(text);
      break;
    }
    case NODE_CALL:
      if (!evaluate(node, frame, &value)) { /* el valor, si hay, se descarta */
        return 0;
      }
      break;
    case NODE_IF: {
      if (!evaluate(node->data.if_stmt.cond, frame, &value)) {
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
        int ok = execute_list(chosen->data.block.body, frame);
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
        if (!evaluate(node->data.ret.value, frame, &frame->ret)) {
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

int execute_program(const ASTNode *program, SymbolTable *table) {
  Frame main_frame = {table, NULL, 0, 0, {TYPE_NADA, {0}}};
  return execute_list(program, &main_frame);
}