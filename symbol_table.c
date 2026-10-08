#include "symbol_table.h"
#include "string_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void symbol_table_init(SymbolTable *table) {
  table->head = NULL;
  table->tail = NULL;
  table->count = 0;
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
  case TYPE_UNKNOWN:
    snprintf(out, size, "?");
    break;
  }
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
    if (utf8_length(symbol->is_const ? "constante" : "variable") > widths[3]) {
      widths[3] = utf8_length(symbol->is_const ? "constante" : "variable");
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
    table_print_cell(symbol->is_const ? "constante" : "variable", widths[3]);
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
}
