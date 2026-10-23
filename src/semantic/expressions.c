#include "semantic_internal.h"

/* Analisis semantico de las expresiones: calcula y guarda el tipo de cada una */

static int is_numeric(DataType type) {
  return type == TYPE_ENTERO || type == TYPE_DECIMAL;
}

static int is_comparison(Operator op) {
  return op == OP_EQ || op == OP_NE || op == OP_GT || op == OP_LT ||
         op == OP_GE || op == OP_LE;
}

static int is_zero_literal(const ASTNode *node) {
  return (node->type == NODE_INT && node->data.intValue == 0) ||
         (node->type == NODE_DECIMAL && node->data.decimalValue == 0.0);
}

/* Calcula y guarda el tipo de una expresion */
DataType semantic_analyze_expression(ASTNode *node, Scope *scope) {
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
    Symbol *symbol = semantic_find_symbol(scope, node->data.identifier);
    if (symbol == NULL) {
      semantic_error(node->line, "la variable '%s' no ha sido declarada",
                     node->data.identifier);
    } else {
      result = symbol->type;
    }
    break;
  }

  case NODE_CALL:
    result = semantic_analyze_call(node, scope);
    break;

  case NODE_INDEX:
    result = semantic_analyze_index(node, scope);
    break;

  case NODE_ARRAY_CALL:
    result = semantic_analyze_array_call(node, scope);
    break;

  /* un [..] suelto: sin un tipo de arreglo esperado no se sabe de que es */
  case NODE_ARRAY:
    semantic_error(node->line,
                   "un arreglo [...] solo se puede usar al declarar, asignar, pasar o retornar un arreglo");
    break;

  case NODE_UNARY: {
    DataType operand = semantic_analyze_value(node->data.binary.left, scope);
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
    DataType left = semantic_analyze_value(node->data.binary.left, scope);
    DataType right = semantic_analyze_value(node->data.binary.right, scope);
    Operator op = node->data.binary.op;

    if (left == TYPE_UNKNOWN || right == TYPE_UNKNOWN) {
      break; /* error ya reportado: no se encadenan mas errores */
    }

    /* "o": los dos lados deben ser booleanos */
    if (op == OP_OR) {
      if (left != TYPE_BOOLEANO || right != TYPE_BOOLEANO) {
        semantic_error(node->line,
                       "el operador 'o' requiere operandos booleanos, se encontro %s y %s",
                       type_name(left), type_name(right));
      } else {
        result = TYPE_BOOLEANO;
      }
      break;
    }

    /* == y !=: numeros entre si, o dos valores del mismo tipo (no arreglos) */
    if (op == OP_EQ || op == OP_NE) {
      if ((is_numeric(left) && is_numeric(right)) ||
          (left == right && !is_array_type(left))) {
        result = TYPE_BOOLEANO;
      } else {
        semantic_error(node->line, "no se puede comparar %s con %s usando '%s'",
                       type_name(left), type_name(right), operator_symbol(op));
      }
      break;
    }

    /* > < >= <=: solo numeros */
    if (is_comparison(op)) {
      if (!is_numeric(left) || !is_numeric(right)) {
        semantic_error(node->line,
                       "el operador '%s' requiere operandos numericos, se encontro %s y %s",
                       operator_symbol(op), type_name(left), type_name(right));
      } else {
        result = TYPE_BOOLEANO;
      }
      break;
    }

    /* + - * /: solo numeros */
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
  case NODE_IF:
  case NODE_BLOCK:
  case NODE_MOVE: /* NUEVO */
    break;
  }

  node->exprType = result;
  return result;
}

/*
 * Igual que semantic_analyze_expression, pero exige que haya un valor: usar
 * dentro de una expresion algo que no devuelve nada es un error.
 */
DataType semantic_analyze_value(ASTNode *node, Scope *scope) {
  DataType type = semantic_analyze_expression(node, scope);
  if (type == TYPE_NADA) {
    if (node->type == NODE_ARRAY_CALL) {
      semantic_error(node->line, "'%s.%s' no devuelve ningun valor",
                     node->data.array_call.name, node->data.array_call.method_name);
    } else {
      semantic_error(node->line, "la funcion '%s' no devuelve ningun valor",
                     node->data.call.name);
    }
    node->exprType = TYPE_UNKNOWN;
    return TYPE_UNKNOWN;
  }
  return type;
}

/*
 * Una expresion que se usa donde se espera un valor de tipo "target"
 * (declarar, asignar, pasar un argumento o retornar). Aqui es donde un
 * [1, 2, 3] sabe de que tipo de arreglo es.
 */
DataType semantic_analyze_expected(ASTNode *node, DataType target, Scope *scope) {
  if (node->type == NODE_ARRAY && is_array_type(target)) {
    return semantic_analyze_array_literal(node, target, scope);
  }
  return semantic_analyze_value(node, scope);
}