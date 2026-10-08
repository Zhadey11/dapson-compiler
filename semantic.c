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

/* Calcula y guarda el tipo de una expresion */
static DataType analyze_expression(ASTNode *node, SymbolTable *table) {
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
    Symbol *symbol = symbol_table_lookup(table, node->data.identifier);
    if (symbol == NULL) {
      semantic_error(node->line, "la variable '%s' no ha sido declarada",
                     node->data.identifier);
    } else {
      result = symbol->type;
    }
    break;
  }

  case NODE_UNARY: {
    DataType operand = analyze_expression(node->data.binary.left, table);
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
    DataType left = analyze_expression(node->data.binary.left, table);
    DataType right = analyze_expression(node->data.binary.right, table);
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
    break;
  }

  node->exprType = result;
  return result;
}

static void analyze_declaration(ASTNode *node, SymbolTable *table) {
  int is_const = node->type == NODE_CONST_DECL;
  const char *name = node->data.decl.name;
  DataType declared = node->exprType;

  /* primero la expresion: asi "sea x: entero = x;" detecta que x no existe */
  DataType value = analyze_expression(node->data.decl.init, table);

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

static void analyze_assignment(ASTNode *node, SymbolTable *table) {
  const char *name = node->data.assign.name;
  Symbol *symbol = symbol_table_lookup(table, name);
  DataType value = analyze_expression(node->data.assign.value, table);

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

int semantic_analyze(ASTNode *program, SymbolTable *table) {
  error_count = 0;

  for (ASTNode *node = program; node != NULL; node = node->next) {
    switch (node->type) {
    case NODE_VAR_DECL:
    case NODE_CONST_DECL:
      analyze_declaration(node, table);
      break;
    case NODE_ASSIGN:
      analyze_assignment(node, table);
      break;
    case NODE_PRINT:
      analyze_expression(node->data.print.value, table);
      break;
    default:
      break;
    }
  }
  return error_count;
}
