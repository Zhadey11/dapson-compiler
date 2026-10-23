#include "runtime.h"
#include "runtime_internal.h"

#include <stdlib.h>

/* Arreglos en tiempo de ejecucion */

/*
 * Todos los arreglos creados se anotan en esta lista. Un arreglo se comparte
 * (b = a apunta al mismo arreglo) y puede salir de una funcion, por eso no se
 * libera al terminar un bloque o una funcion sino todo junto al final.
 */
static Array *all_arrays = NULL;

static Array *array_create(DataType element_type) {
  Array *array = calloc(1, sizeof(*array));
  if (array == NULL) {
    return NULL;
  }
  array->element_type = element_type;
  array->next_allocated = all_arrays;
  all_arrays = array;
  return array;
}

/* Agrega un valor al final; el arreglo crece solo. 0 si no hay memoria */
static int array_push(Array *array, const Value *value) {
  if (array->length == array->capacity) {
    size_t capacity = array->capacity == 0 ? 4 : array->capacity * 2;
    Value *items = realloc(array->items, capacity * sizeof(*items));
    if (items == NULL) {
      return 0;
    }
    array->items = items;
    array->capacity = capacity;
  }
  array->items[array->length++] = *value;
  return 1;
}

void runtime_release_arrays(void) {
  while (all_arrays != NULL) {
    Array *array = all_arrays;
    all_arrays = array->next_allocated;
    free(array->items);
    free(array);
  }
}

/* [a, b, c]: crea el arreglo y evalua cada elemento */
int runtime_evaluate_array(const ASTNode *node, Frame *frame, Value *out) {
  DataType element_type = element_type_of(node->exprType);
  Array *array = array_create(element_type);
  if (array == NULL) {
    return runtime_error(node->line, "sin memoria al crear el arreglo");
  }

  for (const ASTNode *element = node->data.array.elements; element != NULL;
       element = element->next) {
    Value value;
    if (!runtime_evaluate(element, frame, &value)) {
      return 0;
    }
    runtime_widen(&value, element_type);
    if (!array_push(array, &value)) {
      return runtime_error(node->line, "sin memoria al crear el arreglo");
    }
  }

  out->type = node->exprType;
  out->data.array_value = array;
  return 1;
}

/* El arreglo guardado en una variable */
static Array *find_array(const ASTNode *node, const char *name, Frame *frame) {
  Symbol *symbol = runtime_find_symbol(frame, name);
  if (symbol == NULL || !symbol->initialized) {
    runtime_error(node->line, "'%s' no tiene valor", name);
    return NULL;
  }
  return symbol->value.data.array_value;
}

/* lista[i] */
int runtime_evaluate_index(const ASTNode *node, Frame *frame, Value *out) {
  const char *name = node->data.index.name;
  Array *array = find_array(node, name, frame);
  Value position;

  if (array == NULL || !runtime_evaluate(node->data.index.index, frame, &position)) {
    return 0;
  }
  long long index = position.data.int_value;
  if (index < 0 || (unsigned long long)index >= array->length) {
    return runtime_error(node->line,
                         "indice fuera de rango: %s[%lld] (el arreglo tiene %zu elemento(s))",
                         name, index, array->length);
  }
  *out = array->items[index];
  return 1;
}

/* lista.agregar(x)   lista.quitar()   lista.largo() */
int runtime_evaluate_array_call(const ASTNode *node, Frame *frame, Value *out) {
  const char *name = node->data.array_call.name;
  Array *array = find_array(node, name, frame);
  if (array == NULL) {
    return 0;
  }

  switch (node->data.array_call.method) {
  case ARRAY_AGREGAR: {
    Value value;
    if (!runtime_evaluate(node->data.array_call.args, frame, &value)) {
      return 0;
    }
    runtime_widen(&value, array->element_type);
    if (!array_push(array, &value)) {
      return runtime_error(node->line, "sin memoria al agregar a '%s'", name);
    }
    out->type = TYPE_NADA;
    return 1;
  }
  case ARRAY_QUITAR:
    if (array->length == 0) {
      return runtime_error(node->line,
                           "no se puede quitar de '%s': el arreglo esta vacio", name);
    }
    array->length--;
    *out = array->items[array->length];
    return 1;
  case ARRAY_LARGO:
    out->type = TYPE_ENTERO;
    out->data.int_value = (long long)array->length;
    return 1;
  case ARRAY_DESCONOCIDO:
    break;
  }
  return runtime_error(node->line, "metodo de arreglo invalido");
}