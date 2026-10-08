#include "runtime.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

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

static int evaluate(const ASTNode *node, SymbolTable *table, Value *out) {
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
    Symbol *symbol = symbol_table_lookup(table, node->data.identifier);
    if (symbol == NULL || !symbol->initialized) {
      return runtime_error(node->line, "'%s' no tiene valor", node->data.identifier);
    }
    *out = symbol->value;
    return 1;
  }

  case NODE_UNARY: {
    Value operand;
    if (!evaluate(node->data.binary.left, table, &operand)) {
      return 0;
    }
    *out = operand;
    if (operand.type == TYPE_ENTERO) {
      out->data.int_value = -operand.data.int_value;
    } else {
      out->data.decimal_value = -operand.data.decimal_value;
    }
    return 1;
  }

  case NODE_BINARY: {
    Value left, right;
    Operator op = node->data.binary.op;
    if (!evaluate(node->data.binary.left, table, &left) ||
        !evaluate(node->data.binary.right, table, &right)) {
      return 0;
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
      case OP_NEG: break;
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
    case OP_NEG: break;
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
    break;
  }
  return runtime_error(node->line, "expresion invalida");
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

int execute_program(const ASTNode *program, SymbolTable *table) {
  for (const ASTNode *node = program; node != NULL; node = node->next) {
    Value value;

    switch (node->type) {
    case NODE_VAR_DECL:
    case NODE_CONST_DECL: {
      Symbol *symbol = symbol_table_lookup(table, node->data.decl.name);
      if (symbol == NULL || !evaluate(node->data.decl.init, table, &value)) {
        return 0;
      }
      store_value(symbol, &value);
      break;
    }
    case NODE_ASSIGN: {
      Symbol *symbol = symbol_table_lookup(table, node->data.assign.name);
      if (symbol == NULL || !evaluate(node->data.assign.value, table, &value)) {
        return 0;
      }
      store_value(symbol, &value);
      break;
    }
    case NODE_PRINT: {
      char text[512];
      if (!evaluate(node->data.print.value, table, &value)) {
        return 0;
      }
      value_to_string(&value, text, sizeof(text), 0);
      puts(text);
      break;
    }
    default:
      break;
    }
  }
  return 1;
}
