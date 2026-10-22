#include "symbol_table.h"
#include "string_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void symbol_table_init(SymbolTable *table) {
  table->head = NULL;
  table->tail = NULL;
  table->count = 0;
  table->parent = NULL;
  table->func_head = NULL;
  table->func_tail = NULL;
  table->func_count = 0;
}

Symbol *symbol_table_lookup(const SymbolTable *table, const char *name) {
  for (Symbol *symbol = table->head; symbol != NULL; symbol = symbol->next) {
    if (strcmp(symbol->name, name) == 0) {
      return symbol;
    }
  }
  return NULL;
}

int symbol_table_insert(SymbolTable *table, const char *name, DataType type,
                        int is_const, int line) {
  if (symbol_table_lookup(table, name) != NULL) {
    return 0;
  }

  Symbol *symbol = calloc(1, sizeof(*symbol));
  if (symbol == NULL) {
    return -1;
  }
  symbol->name = string_copy(name);
  if (symbol->name == NULL) {
    free(symbol);
    return -1;
  }
  symbol->type = type;
  symbol->is_const = is_const;
  symbol->line = line;
  symbol->is_param = 0;
  symbol->initialized = 0;

  /* se agrega al final para conservar el orden de declaracion */
  if (table->tail == NULL) {
    table->head = symbol;
  } else {
    table->tail->next = symbol;
  }
  table->tail = symbol;
  table->count++;
  return 1;
}

/* Un decimal entero se muestra como "7.0" para distinguirlo de un entero */
static void format_decimal(double value, char *out, size_t size) {
  snprintf(out, size, "%.10g", value);
  if (strpbrk(out, ".en") == NULL) { /* sin punto, exponente, nan ni inf */
    strncat(out, ".0", size - strlen(out) - 1);
  }
}

void value_to_string(const Value *value, char *out, size_t size, int quote) {
  switch (value->type) {
  case TYPE_ENTERO:
    snprintf(out, size, "%lld", value->data.int_value);
    break;
  case TYPE_DECIMAL:
    format_decimal(value->data.decimal_value, out, size);
    break;
  case TYPE_TEXTO:
    snprintf(out, size, quote ? "\"%s\"" : "%s", value->data.text_value);
    break;
  case TYPE_CARACTER:
    snprintf(out, size, quote ? "'%c'" : "%c", value->data.char_value);
    break;
  case TYPE_BOOLEANO:
    snprintf(out, size, "%s", value->data.bool_value ? "verdadero" : "falso");
    break;
  case TYPE_NADA:
    snprintf(out, size, "nada");
    break;
  case TYPE_UNKNOWN:
    snprintf(out, size, "?");
    break;
  }
}

static const char *symbol_class(const Symbol *symbol) {
  if (symbol->is_const) return "constante";
  if (symbol->is_param) return "parametro";
  return "variable";
}

void symbol_table_print(const SymbolTable *table) {
  const char *headers[6] = {"#", "Nombre", "Tipo", "Clase", "Linea", "Valor final"};
  size_t widths[6];
  for (int i = 0; i < 6; i++) {
    widths[i] = utf8_length(headers[i]);
  }

  char number[32];
  char value[128];
  int index = 0;
  for (const Symbol *symbol = table->head; symbol != NULL; symbol = symbol->next) {
    index++;
    snprintf(number, sizeof(number), "%d", index);
    if (utf8_length(number) > widths[0]) widths[0] = utf8_length(number);
    if (utf8_length(symbol->name) > widths[1]) widths[1] = utf8_length(symbol->name);
    if (utf8_length(type_name(symbol->type)) > widths[2]) {
      widths[2] = utf8_length(type_name(symbol->type));
    }
    if (utf8_length(symbol_class(symbol)) > widths[3]) {
      widths[3] = utf8_length(symbol_class(symbol));
    }
    snprintf(number, sizeof(number), "%d", symbol->line);
    if (utf8_length(number) > widths[4]) widths[4] = utf8_length(number);
    if (symbol->initialized) {
      value_to_string(&symbol->value, value, sizeof(value), 1);
      if (utf8_length(value) > widths[5]) widths[5] = utf8_length(value);
    }
  }

  table_print_rule(widths, 6);
  for (int i = 0; i < 6; i++) {
    table_print_cell(headers[i], widths[i]);
  }
  puts("|");
  table_print_rule(widths, 6);

  index = 0;
  for (const Symbol *symbol = table->head; symbol != NULL; symbol = symbol->next) {
    index++;
    snprintf(number, sizeof(number), "%d", index);
    table_print_cell(number, widths[0]);
    table_print_cell(symbol->name, widths[1]);
    table_print_cell(type_name(symbol->type), widths[2]);
    table_print_cell(symbol_class(symbol), widths[3]);
    snprintf(number, sizeof(number), "%d", symbol->line);
    table_print_cell(number, widths[4]);
    if (symbol->initialized) {
      value_to_string(&symbol->value, value, sizeof(value), 1);
      table_print_cell(value, widths[5]);
    } else {
      table_print_cell("-", widths[5]);
    }
    puts("|");
  }
  table_print_rule(widths, 6);
}

static void function_destroy(Function *function) {
  free(function->name);
  free(function->param_types);
  for (int i = 0; i < function->param_count; i++) {
    free(function->param_names[i]);
  }
  free(function->param_names);
  symbol_table_destroy(&function->locals);
  free(function);
}

void symbol_table_destroy(SymbolTable *table) {
  Symbol *symbol = table->head;
  while (symbol != NULL) {
    Symbol *next = symbol->next;
    free(symbol->name);
    free(symbol);
    symbol = next;
  }
  table->head = NULL;
  table->tail = NULL;
  table->count = 0;

  Function *function = table->func_head;
  while (function != NULL) {
    Function *next = function->next;
    function_destroy(function);
    function = next;
  }
  table->func_head = NULL;
  table->func_tail = NULL;
  table->func_count = 0;
}

/* ------------------------------------------------------------------ */
/* Funciones                                                           */
/* ------------------------------------------------------------------ */

Function *function_lookup(const SymbolTable *table, const char *name) {
  for (Function *function = table->func_head; function != NULL;
       function = function->next) {
    if (strcmp(function->name, name) == 0) {
      return function;
    }
  }
  return NULL;
}

int function_table_insert(SymbolTable *table, const ASTNode *definition) {
  const char *name = definition->data.func.name;
  if (function_lookup(table, name) != NULL) {
    return 0;
  }

  Function *function = calloc(1, sizeof(*function));
  if (function == NULL) {
    return -1;
  }
  function->name = string_copy(name);
  function->return_type = definition->exprType;
  function->param_count = ast_list_length(definition->data.func.params);
  function->line = definition->line;
  function->node = definition;
  symbol_table_init(&function->locals);

  /* calloc(1, ...) evita pedir 0 bytes cuando no hay parametros */
  size_t slots = (size_t)function->param_count + 1;
  function->param_types = calloc(slots, sizeof(DataType));
  function->param_names = calloc(slots, sizeof(char *));
  if (function->name == NULL || function->param_types == NULL ||
      function->param_names == NULL) {
    function->param_count = 0; /* aun no hay nombres copiados que liberar */
    function_destroy(function);
    return -1;
  }

  int index = 0;
  for (const ASTNode *param = definition->data.func.params; param != NULL;
       param = param->next) {
    function->param_types[index] = param->exprType;
    function->param_names[index] = string_copy(param->data.decl.name);
    if (function->param_names[index] == NULL) {
      function->param_count = index; /* solo se liberan los ya copiados */
      function_destroy(function);
      return -1;
    }
    index++;
  }

  if (table->func_tail == NULL) {
    table->func_head = function;
  } else {
    table->func_tail->next = function;
  }
  table->func_tail = function;
  table->func_count++;
  return 1;
}

void function_table_print(const SymbolTable *table) {
  const char *headers[5] = {"#", "Nombre", "Parametros", "Retorna", "Linea"};
  size_t widths[5];
  for (int i = 0; i < 5; i++) {
    widths[i] = utf8_length(headers[i]);
  }

  /* Los parametros se arman como texto: "a: entero, b: entero" */
  char params[256];
  char number[32];
  int index = 0;
  for (const Function *function = table->func_head; function != NULL;
       function = function->next) {
    index++;
    snprintf(number, sizeof(number), "%d", index);
    if (utf8_length(number) > widths[0]) widths[0] = utf8_length(number);
    if (utf8_length(function->name) > widths[1]) widths[1] = utf8_length(function->name);
    params[0] = '\0';
    for (int i = 0; i < function->param_count; i++) {
      size_t used = strlen(params);
      snprintf(params + used, sizeof(params) - used, "%s%s: %s", i > 0 ? ", " : "",
               function->param_names[i], type_name(function->param_types[i]));
    }
    if (function->param_count == 0) snprintf(params, sizeof(params), "-");
    if (utf8_length(params) > widths[2]) widths[2] = utf8_length(params);
    if (utf8_length(type_name(function->return_type)) > widths[3]) {
      widths[3] = utf8_length(type_name(function->return_type));
    }
    snprintf(number, sizeof(number), "%d", function->line);
    if (utf8_length(number) > widths[4]) widths[4] = utf8_length(number);
  }

  table_print_rule(widths, 5);
  for (int i = 0; i < 5; i++) {
    table_print_cell(headers[i], widths[i]);
  }
  puts("|");
  table_print_rule(widths, 5);

  index = 0;
  for (const Function *function = table->func_head; function != NULL;
       function = function->next) {
    index++;
    snprintf(number, sizeof(number), "%d", index);
    table_print_cell(number, widths[0]);
    table_print_cell(function->name, widths[1]);
    params[0] = '\0';
    for (int i = 0; i < function->param_count; i++) {
      size_t used = strlen(params);
      snprintf(params + used, sizeof(params) - used, "%s%s: %s", i > 0 ? ", " : "",
               function->param_names[i], type_name(function->param_types[i]));
    }
    if (function->param_count == 0) snprintf(params, sizeof(params), "-");
    table_print_cell(params, widths[2]);
    table_print_cell(type_name(function->return_type), widths[3]);
    snprintf(number, sizeof(number), "%d", function->line);
    table_print_cell(number, widths[4]);
    puts("|");
  }
  table_print_rule(widths, 5);

  /* El ambito de cada funcion: sus parametros y variables locales */
  for (const Function *function = table->func_head; function != NULL;
       function = function->next) {
    printf("\nAmbito de la funcion '%s':\n", function->name);
    if (function->locals.count == 0) {
      puts("(sin parametros ni variables locales)");
    } else {
      symbol_table_print(&function->locals);
    }
  }
}