#include "runtime_internal.h"

#include <limits.h>

/* Evaluacion de expresiones: calcula el valor de una expresion */

int runtime_evaluate(const ASTNode *node, Frame *frame, Value *out) {
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
    Symbol *symbol = runtime_find_symbol(frame, node->data.identifier);
    if (symbol == NULL || !symbol->initialized) {
      return runtime_error(node->line, "'%s' no tiene valor", node->data.identifier);
    }
    *out = symbol->value;
    return 1;
  }

  case NODE_CALL:
    return runtime_call_function(node, frame, out);

  case NODE_ARRAY:
    return runtime_evaluate_array(node, frame, out);
  case NODE_INDEX:
    return runtime_evaluate_index(node, frame, out);
  case NODE_ARRAY_CALL:
    return runtime_evaluate_array_call(node, frame, out);

  case NODE_UNARY: {
    Value operand;
    if (!runtime_evaluate(node->data.binary.left, frame, &operand)) {
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
      if (!runtime_evaluate(node->data.binary.left, frame, &left)) {
        return 0;
      }
      out->type = TYPE_BOOLEANO;
      if (left.data.bool_value) {
        out->data.bool_value = 1;
        return 1;
      }
      if (!runtime_evaluate(node->data.binary.right, frame, &right)) {
        return 0;
      }
      out->data.bool_value = right.data.bool_value;
      return 1;
    }

    if (!runtime_evaluate(node->data.binary.left, frame, &left) ||
        !runtime_evaluate(node->data.binary.right, frame, &right)) {
      return 0;
    }

    /* Comparaciones: el resultado siempre es booleano */
    if (runtime_is_comparison(op)) {
      out->type = TYPE_BOOLEANO;
      out->data.bool_value = runtime_compare(op, &left, &right);
      return 1;
    }

    /* Si alguno es decimal, toda la operacion es decimal */
    if (left.type == TYPE_DECIMAL || right.type == TYPE_DECIMAL) {
      double a = runtime_as_double(&left);
      double b = runtime_as_double(&right);
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
  case NODE_MOVE: /* NUEVO: mover es una sentencia, no una expresion */
    break;
  }
  return runtime_error(node->line, "expresion invalida");
}