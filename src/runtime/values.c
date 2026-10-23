#include "runtime_internal.h"

#include <string.h>

/* Operaciones sobre valores: conversion, guardado y comparacion */

double runtime_as_double(const Value *value) {
  return value->type == TYPE_ENTERO ? (double)value->data.int_value
                                    : value->data.decimal_value;
}

/* Un entero se convierte a decimal si el tipo que se espera es decimal */
void runtime_widen(Value *value, DataType target) {
  if (target == TYPE_DECIMAL && value->type == TYPE_ENTERO) {
    value->data.decimal_value = (double)value->data.int_value;
    value->type = TYPE_DECIMAL;
  }
}

/* Guarda un valor en una variable */
void runtime_store(Symbol *symbol, const Value *value) {
  symbol->value = *value;
  runtime_widen(&symbol->value, symbol->type);
  symbol->initialized = 1;
}

int runtime_is_comparison(Operator op) {
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
int runtime_compare(Operator op, const Value *left, const Value *right) {
  int order = 0;
  if ((left->type == TYPE_ENTERO || left->type == TYPE_DECIMAL) &&
      (right->type == TYPE_ENTERO || right->type == TYPE_DECIMAL)) {
    if (left->type == TYPE_ENTERO && right->type == TYPE_ENTERO) {
      long long a = left->data.int_value;
      long long b = right->data.int_value;
      order = (a > b) - (a < b);
    } else {
      double a = runtime_as_double(left);
      double b = runtime_as_double(right);
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