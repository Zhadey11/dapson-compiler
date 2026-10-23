#include "semantic_internal.h"

/* Analisis semantico de los arreglos: literales, indices y metodos */

/*
 * [a, b, c] escrito donde se espera un arreglo de tipo "target":
 * cada elemento debe caber en el tipo del arreglo (un entero cabe en un decimal)
 */
DataType semantic_analyze_array_literal(ASTNode *node, DataType target,
                                        Scope *scope) {
  DataType element_type = element_type_of(target);
  int position = 1;

  for (ASTNode *element = node->data.array.elements; element != NULL;
       element = element->next, position++) {
    DataType type = semantic_analyze_value(element, scope);
    if (!semantic_assignable(element_type, type)) {
      semantic_error(element->line,
                     "elemento %d del arreglo: se esperaba %s y se encontro %s",
                     position, type_name(element_type), type_name(type));
    }
  }

  node->exprType = target;
  return target;
}

/* lista[i]: lista debe ser un arreglo e i un entero; el resultado es un elemento */
DataType semantic_analyze_index(ASTNode *node, Scope *scope) {
  const char *name = node->data.index.name;
  Symbol *symbol = semantic_find_symbol(scope, name);
  DataType index = semantic_analyze_value(node->data.index.index, scope);
  DataType result = TYPE_UNKNOWN;

  if (symbol == NULL) {
    semantic_error(node->line, "la variable '%s' no ha sido declarada", name);
  } else if (!is_array_type(symbol->type)) {
    semantic_error(node->line, "'%s' no es un arreglo (es de tipo %s)", name,
                   type_name(symbol->type));
  } else {
    result = element_type_of(symbol->type);
  }

  if (index != TYPE_UNKNOWN && index != TYPE_ENTERO) {
    semantic_error(node->line, "el indice de '%s' debe ser entero, se encontro %s",
                   name, type_name(index));
  }
  return result;
}

/*
 * lista.agregar(x) -> nada     lista.quitar() -> un elemento     lista.largo() -> entero
 */
DataType semantic_analyze_array_call(ASTNode *node, Scope *scope) {
  const char *name = node->data.array_call.name;
  const char *method_name = node->data.array_call.method_name;
  ArrayMethod method = node->data.array_call.method;
  Symbol *symbol = semantic_find_symbol(scope, name);
  int given = ast_list_length(node->data.array_call.args);
  int valid = 0;

  if (symbol == NULL) {
    semantic_error(node->line, "la variable '%s' no ha sido declarada", name);
  } else if (!is_array_type(symbol->type)) {
    semantic_error(node->line, "'%s' no es un arreglo (es de tipo %s)", name,
                   type_name(symbol->type));
  } else if (method == ARRAY_DESCONOCIDO) {
    semantic_error(node->line,
                   "los arreglos no tienen el metodo '%s' (son agregar, quitar y largo)",
                   method_name);
  } else {
    valid = 1;
  }

  /* Los argumentos se analizan siempre, para encontrar sus propios errores */
  DataType first = TYPE_UNKNOWN;
  for (ASTNode *arg = node->data.array_call.args; arg != NULL; arg = arg->next) {
    DataType type = semantic_analyze_value(arg, scope);
    if (arg == node->data.array_call.args) {
      first = type;
    }
  }
  if (!valid) {
    return TYPE_UNKNOWN;
  }

  DataType element = element_type_of(symbol->type);
  int expected = method == ARRAY_AGREGAR ? 1 : 0;
  if (given != expected) {
    semantic_error(node->line, "'%s.%s' espera %d argumento(s) y se le pasaron %d",
                   name, method_name, expected, given);
  } else if (method == ARRAY_AGREGAR && !semantic_assignable(element, first)) {
    semantic_error(node->line,
                   "no se puede agregar un valor de tipo %s a '%s' (arreglo de %s)",
                   type_name(first), name, type_name(element));
  }
  if (method != ARRAY_LARGO && symbol->is_const) {
    semantic_error(node->line,
                   "'%s' es una constante (fijo) y no se puede modificar", name);
  }

  if (method == ARRAY_AGREGAR) return TYPE_NADA;
  if (method == ARRAY_QUITAR) return element;
  return TYPE_ENTERO;
}