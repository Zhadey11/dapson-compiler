#include "ast.h"

#include <string.h>

/* Nombres de tipos y operadores */

DataType parse_type_name(const char *name) {
  if (strcmp(name, "entero") == 0) return TYPE_ENTERO;
  if (strcmp(name, "decimal") == 0) return TYPE_DECIMAL;
  if (strcmp(name, "texto") == 0) return TYPE_TEXTO;
  if (strcmp(name, "caracter") == 0) return TYPE_CARACTER;
  if (strcmp(name, "booleano") == 0) return TYPE_BOOLEANO;
  return TYPE_UNKNOWN;
}

const char *type_name(DataType type) {
  switch (type) {
  case TYPE_ENTERO: return "entero";
  case TYPE_DECIMAL: return "decimal";
  case TYPE_TEXTO: return "texto";
  case TYPE_CARACTER: return "caracter";
  case TYPE_BOOLEANO: return "booleano";
  case TYPE_ENTERO_ARR: return "entero[]";
  case TYPE_DECIMAL_ARR: return "decimal[]";
  case TYPE_TEXTO_ARR: return "texto[]";
  case TYPE_CARACTER_ARR: return "caracter[]";
  case TYPE_BOOLEANO_ARR: return "booleano[]";
  case TYPE_NADA: return "nada";
  case TYPE_UNKNOWN: break;
  }
  return "desconocido";
}

int is_array_type(DataType type) {
  return type >= TYPE_ENTERO_ARR && type <= TYPE_BOOLEANO_ARR;
}

/* Los tipos de arreglo estan en el mismo orden que los tipos normales,
   por eso basta con sumar o restar la distancia entre los dos grupos */
DataType array_type_of(DataType element_type) {
  if (element_type < TYPE_ENTERO || element_type > TYPE_BOOLEANO) {
    return TYPE_UNKNOWN;
  }
  return (DataType)(element_type + (TYPE_ENTERO_ARR - TYPE_ENTERO));
}

DataType element_type_of(DataType array_type) {
  if (!is_array_type(array_type)) {
    return TYPE_UNKNOWN;
  }
  return (DataType)(array_type - (TYPE_ENTERO_ARR - TYPE_ENTERO));
}

const char *operator_symbol(Operator op) {
  switch (op) {
  case OP_ADD: return "+";
  case OP_SUB: return "-";
  case OP_MUL: return "*";
  case OP_DIV: return "/";
  case OP_EQ: return "==";
  case OP_NE: return "!=";
  case OP_GT: return ">";
  case OP_LT: return "<";
  case OP_GE: return ">=";
  case OP_LE: return "<=";
  case OP_OR: return "||";
  case OP_NEG: return "-";
  }
  return "?";
}